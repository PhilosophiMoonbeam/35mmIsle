#include "photo_world.hpp"
#include "borrowed_model_kernel.hpp"
#include <algorithm>
#include <cmath>
#include <exception>
#include <optional>
#include <stdexcept>

namespace isle::photo {
namespace {
void require(bool v, const char* message) { if (!v) throw std::runtime_error(message); }
void native(IsleGameResult r) { require(r == ISLE_GAME_OK,"Replay World native operation failed"); }
void animation(menu::animation::Status s) { require(bool(s),"Replay World animation operation failed"); }
float reduced(float t) { while (t >= 100.0f) t -= 100.0f; return t; }
world::Vec3 vec(menu::Vec3 v) { return {v.x,v.y,v.z}; }
std::size_t seek_commands(const menu::Animation& stream, float time, std::span<std::optional<float>> scratch) {
    float left = -time; std::size_t cursor = 0, count = 0;
    auto visited = scratch.first(stream.commands.size()); std::fill(visited.begin(),visited.end(),std::nullopt);
    for (;;) {
        require(cursor < stream.commands.size() && count != SIZE_MAX,"Replay World seek escapes decoded program");
        const auto& command = stream.commands[cursor++]; ++count;
        if (!command.opcode) return count;
        if (command.opcode == 1 || command.opcode == 14) {
            const auto target = command.wire.u32(4); if (!target) return count;
            const auto found = std::find_if(stream.commands.begin(),stream.commands.end(),
                [target](const auto& c) { return c.wire.guest_address == target; });
            require(found != stream.commands.end(),"Replay World seek lacks decoded target");
            cursor = static_cast<std::size_t>(found-stream.commands.begin());
            if (visited[cursor]) require(left > *visited[cursor],"Replay World cycle cannot advance clock");
            visited[cursor] = left;
        }
        switch (command.opcode) {
        case 2: case 3: case 5: case 8: case 10: case 15: case 16: case 17: case 18: case 20: case 23:
            left += static_cast<float>(command.duration); break;
        case 22: left = static_cast<float>(command.duration); break;
        default: break;
        }
        if (left > 0) return count;
    }
}
menu::animation::ExecutionCapacity execution_capacity(const world::Resources& resources) {
    float time = 100; std::size_t stream_size = 0, maximum = 0;
    for (const auto& block : resources.blocks()) time = std::max(time,static_cast<float>(block.metadata.movement_animation_duration));
    for (const auto& stream : resources.animations()) stream_size = std::max(stream_size,stream.commands.size());
    std::vector<std::optional<float>> scratch(stream_size);
    for (const auto& model : resources.models()) {
        std::size_t commands = 0;
        auto charge = [&](std::size_t stream) {
            if (stream == menu::no_index) return;
            const auto count = seek_commands(resources.animations()[stream],time,scratch);
            require(count <= SIZE_MAX-commands,"Replay World execution capacity overflow"); commands += count;
        };
        for (const auto& node : model.nodes) { charge(node.animation); for (auto stream : node.material_animations) charge(stream); }
        maximum = std::max(maximum,commands);
    }
    return {maximum,maximum};
}
}
struct World::Impl {
    struct NativeOwner {
        IsleGameObject* object = nullptr;
        bool observing = false, joined = false;
    };
    struct Binding {
        Impl* owner = nullptr;
        std::size_t block = world::no_index, model_index = world::no_index;
        IsleGameObject* object = nullptr;
        IsleGameModel* model = nullptr;
        menu::animation::InstanceHandle instance;
        std::vector<std::size_t> texture_programs;
        bool road = false, sky = false;
    };
    IsleGame* game;
    const world::Resources& resources;
    world::ModuleState& module;
    world::ResourceView resource_view;
    std::unique_ptr<menu::animation::Runtime> runtime;
    std::vector<world::Block> blocks;
    std::vector<world::RenderObject> render;
    std::vector<Binding> bindings;
    std::vector<NativeOwner> native_owners;
    std::vector<IsleGameModelNodeInit> scratch;
    IsleGameObject* sky_object = nullptr;
    Binding* sky = nullptr;
    IsleWorldMovementState movement = ISLE_WORLD_MOVEMENT_STATE_INITIALIZER;
    bool started = false, published = false, clearing = false, complete = false, failed = false;
    std::size_t clear_index = 0;
    std::string failure = "Replay World publication, reconstruction, scheduling or deletion failed", empty;
    Impl(IsleGame* g, const world::Resources& r, world::ModuleState& m)
        : game(g),resources(r),module(m),resource_view{&r.setup(),&r.sky(),r.blocks(),r.primary_count(),r.models(),
            r.materials(),r.animations(),r.paths(),r.static_definitions(),
            r.height_map().wire.bytes.empty() ? nullptr : &r.height_map(),r.collision_models(),
            r.ceiling_map().wire.bytes.empty() ? nullptr : &r.ceiling_map(),r.sky_model_index()} {}
    void process(IsleGameObject* object, IsleGameCallback callback, void* user, std::uint32_t priority) {
        IsleGameProcess* process = nullptr;
        native(isle_game_add_process(game,object,callback,user,ISLE_GAME_SYNCHRONOUS,priority,&process));
    }
    static void joined(IsleGameObject*, void* user) noexcept {
        static_cast<NativeOwner*>(user)->joined = true;
    }
    void object(std::uint32_t id, IsleGameObject*& output) {
        // Replay owns no gameplay command queue or AI updater; only the exact
        // synchronous World processes below are admitted to native scheduling.
        native(isle_game_add_object(game,id,18,0x80000000,nullptr,nullptr,&output));
        auto& owner = native_owners[static_cast<std::size_t>(id-99)];
        owner.object = output;
        native(isle_game_object_set_retire_callback(game,output,joined,&owner));
        owner.observing = true;
    }
    void attach(Binding& b) {
        const auto acquired = runtime->acquire(static_cast<std::size_t>(&b-bindings.data()),b.model_index,b.road);
        animation(acquired.status); b.instance = acquired.handle;
        require(bool(menu::model::attach_source_model(game,b.object,resources.models()[b.model_index],runtime->model(b.instance),
            menu::model::RootRecipe::source,scratch,&b.model)),"Replay World native model attachment rejected");
    }
    void synchronize(Binding& b, bool preserve_root) {
        const auto state = runtime->model(b.instance);
        for (std::size_t i = 0; i < state.nodes.size(); ++i) {
            const auto& node = state.nodes[i];
            IsleGameModelTransform transform{vec(node.transform.position),vec(node.transform.rotation),vec(node.transform.scale)};
            if (preserve_root && i == 0) {
                const IsleGameModelNode* nodes = nullptr; std::size_t count = 0;
                native(isle_game_model_peek_nodes(game,b.model,&nodes,&count)); transform = nodes[0].transform;
            }
            native(isle_game_model_set_transform(game,b.model,i,&transform)); native(isle_game_model_set_flags(game,b.model,i,node.flags));
        }
        if (b.road) for (const auto& e : runtime->events()) if (e.instance == b.instance && e.event_id >= 0 && e.event_id < 14) {
            auto& event = module.road_events[static_cast<std::size_t>(e.event_id)];
            event.animation_time = state.animation_time; event.visibility = e.argument;
            visibility(static_cast<std::uint32_t>(e.argument));
        }
    }
    void visibility(std::uint32_t mask) {
        for (auto& block : blocks) if (block.object) {
            IsleGameObjectMetadata metadata{}; native(isle_game_object_metadata(game,block.object,&metadata));
            native(isle_game_object_set_flags(game,block.object,(mask&1u) ? metadata.flags&~1u : metadata.flags|1u)); mask >>= 1;
        }
    }
    void texture(Binding& b, float time, bool zero_speed) {
        animation(runtime->bind_materials(b.instance,b.texture_programs,time));
        if (zero_speed) animation(runtime->set_texture_speed(b.instance,0));
        animation(runtime->step_model(b.instance)); synchronize(b,true);
    }
    static void update_timer(IsleGameObject*, void* user) noexcept {
        auto& s = *static_cast<Impl*>(user); if (s.failed || s.clearing) return;
        auto& m = s.module; float time = m.global_timer+m.sky_animation_speed;
        while (time < 0) time += static_cast<float>(m.sky_max_angle);
        time *= 10000.0f; time = static_cast<float>((static_cast<std::int32_t>(time)+5)/10); time /= 1000.0f;
        while (time >= m.sky_max_angle) time -= static_cast<float>(m.sky_max_angle);
        m.global_timer = time;
    }
    static void update_terrain(IsleGameObject*, void* user) noexcept {
        auto& b = *static_cast<Binding*>(user); auto& s = *b.owner; if (s.failed || s.clearing) return;
        try { s.texture(b,reduced(s.module.global_timer),true); } catch (...) { s.failed = true; }
    }
    static void update_sky(IsleGameObject*, void* user) noexcept {
        auto& b = *static_cast<Binding*>(user); auto& s = *b.owner; if (s.failed || s.clearing) return;
        try { animation(s.runtime->step_model(b.instance)); s.synchronize(b,true); } catch (...) { s.failed = true; }
    }
    static void update_yaw(IsleGameObject*, void* user) noexcept {
        auto& b = *static_cast<Binding*>(user); auto& s = *b.owner; if (s.failed || s.clearing) return;
        try {
            const IsleGameModelNode* nodes = nullptr; std::size_t count = 0;
            native(isle_game_model_peek_nodes(s.game,b.model,&nodes,&count)); auto transform = nodes[0].transform;
            transform.rotation.y = 6.283185482f*s.module.global_timer/static_cast<float>(s.module.sky_max_angle);
            native(isle_game_model_set_transform(s.game,b.model,0,&transform));
        } catch (...) { s.failed = true; }
    }
    void road(Binding& b, float time, bool reset) {
        if (reset) animation(runtime->restart_model(b.instance,time));
        else animation(runtime->set_model_speed(b.instance,time-runtime->model(b.instance).animation_time));
        if (reset) animation(runtime->set_model_speed(b.instance,0));
        animation(runtime->step_model(b.instance)); synchronize(b,false);
    }
    bool erase(IsleGameObject*& object) noexcept {
        if (!object) return true;
        auto owner = std::find_if(native_owners.begin(),native_owners.end(),
            [object](const NativeOwner& candidate) { return candidate.object == object; });
        if (owner == native_owners.end()) { failed = true; return false; }
        if (!owner->joined) {
            if (isle_game_delete_object(game,object) != ISLE_GAME_OK) failed = true;
            if (!owner->observing) {
                IsleGameObjectMetadata metadata{};
                owner->joined = isle_game_object_metadata(game,object,&metadata) == ISLE_GAME_INVALID_ARGUMENT;
            }
        }
        if (!owner->joined) { failed = true; return false; }
        owner->object = nullptr;
        for (auto& b : bindings) if (b.object == object) { b.object = nullptr; b.model = nullptr; }
        for (auto& r : render) if (r.object == object) { r.object = nullptr; r.model = nullptr; }
        object = nullptr; return true;
    }
};
World::World(IsleGame* game, const world::Resources& resources, world::ModuleState& module)
    : impl_(std::make_unique<Impl>(game,resources,module)) {
    require(game && module.sky_max_angle > 0 && std::isfinite(module.global_timer),"Replay World requires live native game/module"); resources.require_live();
    auto& s = *impl_; s.blocks.resize(resources.blocks().size());
    s.native_owners.resize(resources.blocks().size()*2+1);
    std::size_t count = resources.sky_model_index() != world::no_index ? 1 : 0;
    for (std::size_t i = 0; i < s.blocks.size(); ++i) {
        auto& b = s.blocks[i]; b.index = i; b.resource = &resources.blocks()[i];
        b.control_points = i < resources.primary_count() && b.resource->metadata.control_line_count > 0 && b.resource->metadata.control_line_count < 4;
        if (i < resources.primary_count()) {
            if (i) b.previous = &s.blocks[i-1]; if (i+1 < resources.primary_count()) b.next = &s.blocks[i+1];
        }
        if (b.resource->model != world::no_index) ++count;
        if (i < resources.primary_count() && b.resource->road_model != world::no_index) ++count;
    }
    if (resources.primary_loop()) { s.blocks.front().previous = &s.blocks[resources.primary_count()-1]; s.blocks[resources.primary_count()-1].next = &s.blocks.front(); }
    s.bindings.reserve(count); s.render.reserve(count);
    auto add = [&](std::size_t index, std::size_t block, bool road, bool sky) {
        Impl::Binding b; b.owner = &s; b.model_index = index; b.block = block; b.road = road; b.sky = sky;
        for (const auto& node : resources.models()[index].nodes)
            for (std::size_t i = 0; i < node.materials.size(); ++i)
                b.texture_programs.push_back(i < node.material_animations.size() ? node.material_animations[i] : menu::no_index);
        s.bindings.push_back(std::move(b));
    };
    if (resources.sky_model_index() != world::no_index) add(resources.sky_model_index(),world::no_index,false,true);
    for (const auto& b : s.blocks) {
        if (b.resource->model != world::no_index) add(b.resource->model,b.index,false,false);
        if (b.index < resources.primary_count() && b.resource->road_model != world::no_index) add(b.resource->road_model,b.index,true,false);
    }
    std::vector<menu::animation::SlotCapacity> capacities; capacities.reserve(count); std::size_t nodes = 0;
    for (const auto& b : s.bindings) { const auto c = menu::animation::slot_capacity(resources.models()[b.model_index]); capacities.push_back(c); nodes = std::max(nodes,c.nodes); }
    s.scratch.resize(nodes);
    auto created = menu::animation::Runtime::create({resources.models(),resources.materials(),resources.animations(),menu::no_index,resources.paths()},
        capacities,{},execution_capacity(resources));
    require(bool(created),"Replay World animation admission failed"); s.runtime = std::move(created.runtime);
}
World::~World() noexcept { if (impl_->started && !impl_->complete) std::terminate(); }
bool World::publish() noexcept {
    auto& s = *impl_; if (s.started || s.clearing || s.failed) return false; s.started = true;
    s.module.reset_road_events();
    try {
        s.resources.require_live();
        s.object(99,s.sky_object); s.process(s.sky_object,Impl::update_timer,&s,2);
        for (auto& b : s.bindings) {
            if (b.sky) { b.object = s.sky_object; s.sky = &b; }
            else {
                auto& block = s.blocks[b.block]; auto*& object = b.road ? block.road_object : block.object;
                s.object(static_cast<std::uint32_t>(100+b.block*2+(b.road ? 1 : 0)),object); b.object = object;
            }
            s.attach(b);
            if (!b.sky) {
                auto& block = s.blocks[b.block];
                if (b.road) { block.road_model = b.model; block.road_instance = b.instance; }
                else { block.model = b.model; block.instance = b.instance;
                    const IsleGameModelTransform t{{},{0,block.resource->metadata.yaw,0},{1,1,1}};
                    native(isle_game_model_set_transform(s.game,b.model,0,&t));
                }
            }
            const auto callback = b.sky ? s.resources.sky().render_callback_guest : b.road ? 0x80014f98u :
                (s.resources.models()[b.model_index].render_type == menu::RenderType::b_fogged ? 0x800a15d8u : s.blocks[b.block].resource->metadata.render_callback_guest);
            // Control roads retain their real source DL registration/state, but
            // are not terrain submissions in the replay borrowed render view.
            if (!b.road) s.render.push_back({b.object,b.model,b.model_index,b.sky ? s.resources.sky().display_list_guest : s.blocks[b.block].resource->metadata.display_list_guest,
                callback,18,19,b.sky ? 0x80000001u : 0x80000000u,b.sky,b.instance});
            if (b.sky) {
                if (s.resources.sky().animation_guest) {
                    s.process(b.object,Impl::update_sky,&b,1);
                    animation(s.runtime->bind_materials(b.instance,b.texture_programs,0));
                    animation(s.runtime->set_texture_speed(b.instance,s.resources.sky().animation_speed));
                }
                if (callback == 0x800e1d80) s.process(b.object,Impl::update_yaw,&b,1);
            } else if (b.road) {
                // fnAnimCallback is installed only after the creation update.
                animation(s.runtime->set_callback_enabled(b.instance,false));
                s.road(b,s.blocks[b.block].resource->metadata.reversed ? static_cast<float>(s.blocks[b.block].resource->metadata.movement_animation_duration) : 0,true);
                animation(s.runtime->set_callback_enabled(b.instance,true));
            } else if (s.blocks[b.block].resource->metadata.texture_animation_guest) s.process(b.object,Impl::update_terrain,&b,1);
        }
        if (!s.blocks.empty() && s.blocks[0].road_model && s.blocks[0].resource->metadata.movement_animation_guest)
            for (auto& b : s.bindings) if (b.road && b.block == 0) { s.road(b,0,true); break; }
        // createWorld(setup speed), then func_8009D184 freezes it to zero.
        s.module.sky_animation_speed = s.resources.setup().sky_animation_speed;
        if (s.sky && s.resources.sky().animation_guest) animation(s.runtime->set_texture_speed(s.sky->instance,s.module.sky_animation_speed));
        s.module.sky_animation_speed = 0;
        if (s.sky && s.resources.sky().animation_guest) animation(s.runtime->set_texture_speed(s.sky->instance,0));
        s.published = true; return true;
    } catch (...) { s.failed = true; return false; }
}
bool World::reconstruct(const WorldRecord& record) noexcept {
    auto& s = *impl_; if (!available()) return false;
    try {
        require(std::isfinite(record.time) && record.time >= 0 && std::isfinite(record.eye.x) && std::isfinite(record.eye.y) && std::isfinite(record.eye.z),"Replay World record is nonfinite");
        const auto index = std::size_t(record.flags&15u); require(index < s.blocks.size(),"Replay World recorded block is absent");
        // Photo time seeks scene-owned animations; the module timer is retained.
        const float time = reduced(record.time);
        for (auto& b : s.bindings) if (!b.sky && !b.road) {
            const auto& m = s.blocks[b.block].resource->metadata;
            if (m.materials_guest && m.texture_animation_guest) s.texture(b,time,false);
        }
        if (s.sky && (s.resources.course_id() == course::beach_id || s.resources.course_id() == course::volcano_id ||
            s.resources.course_id() == course::river_id || s.resources.course_id() == course::valley_id || s.resources.course_id() == course::rainbow_id)) {
            const float yaw = s.resources.course_id() == course::rainbow_id ? record.time*6.283185482f/10000.0f : 0;
            const IsleGameModelTransform t{record.eye,{0,yaw,0},{1,1,1}}; native(isle_game_model_set_transform(s.game,s.sky->model,0,&t));
            if (s.resources.sky().animation_guest) animation(s.runtime->bind_materials(s.sky->instance,s.sky->texture_programs,time));
        }
        auto& reference = s.blocks[index]; const auto origin = reference.resource->metadata.world_position;
        for (auto& block : s.blocks) if (block.model) {
            const auto p = block.resource->metadata.world_position;
            const IsleGameModelTransform t{{(p.x-origin.x)*100,(p.y-origin.y)*100,(p.z-origin.z)*100},{0,block.resource->metadata.yaw,0},{1,1,1}};
            native(isle_game_model_set_transform(s.game,block.model,0,&t));
        }
        s.movement.move_time = 0; s.movement.speed_multiplier = 0; s.movement.control_time = 0.5f; s.movement.block = &reference;
        world::MovementSample sample{};
        for (auto& b : s.bindings) if (b.road && b.block == index && reference.resource->metadata.movement_animation_guest) {
            const auto& m = reference.resource->metadata;
            s.road(b,m.reversed ? static_cast<float>(m.movement_animation_duration) : 0,false);
            sample = world::sample_control_points(s.runtime->model(b.instance),0.5f,m.reversed,m.control_line_count,reference.resource->control_timestamps); break;
        }
        const auto yaw = reference.resource->metadata.yaw; const auto c = std::cos(yaw), sn = std::sin(yaw);
        s.movement.position = {c*sample.position.x+sn*sample.position.z,sample.position.y,-sn*sample.position.x+c*sample.position.z};
        s.movement.rotation = {sample.rotation.x,sample.rotation.y+yaw+(reference.resource->metadata.reversed ? 3.1415927f : 0),sample.rotation.z}; s.movement.move_speed = 0;
        s.visibility(UINT32_MAX); return true;
    } catch (...) { s.failed = true; return false; }
}
bool World::request_clear() noexcept {
    auto& s = *impl_; if (s.complete) return true;
    if (!s.clearing) {
        s.clearing = true;
        if (s.started) s.module.reset_road_events();
    }
    auto pause = [&](IsleGameObject* object) {
        if (!object) return true;
        for (const auto& owner : s.native_owners) if (owner.object == object && owner.joined) return true;
        const auto result = isle_game_pause_object_processes(s.game,object,1);
        if (result != ISLE_GAME_OK) s.failed = true; return result == ISLE_GAME_OK;
    };
    for (auto& block : s.blocks) if (!pause(block.object) || !pause(block.road_object)) return false;
    return pause(s.sky_object);
}
bool World::clear() noexcept {
    auto& s = *impl_; if (s.complete) return true; if (!request_clear()) return false;
    for (; s.clear_index < s.blocks.size(); ++s.clear_index) {
        auto& b = s.blocks[s.clear_index]; if (!s.erase(b.object)) return false; b.model = nullptr;
        if (!s.erase(b.road_object)) return false; b.road_model = nullptr;
    }
    if (!s.erase(s.sky_object)) return false;
    s.runtime->retire(); s.movement.block = nullptr; s.complete = true; return true;
}
bool World::available() const noexcept {
    return impl_->published && !impl_->clearing && !impl_->failed &&
        std::none_of(impl_->native_owners.begin(),impl_->native_owners.end(),
            [](const Impl::NativeOwner& owner) { return owner.joined; });
}
bool World::cleared() const noexcept { return impl_->complete; }
const std::string& World::error() const noexcept { return impl_->failed ? impl_->failure : impl_->empty; }
world::RenderView World::render_view() const noexcept {
    return {&impl_->resource_view,impl_->runtime.get(),impl_->render,this,[](const void* owner) noexcept { return static_cast<const World*>(owner)->available(); }};
}
std::span<const world::Block> World::blocks() const noexcept { return impl_->blocks; }
const IsleWorldMovementState& World::movement() const noexcept { return impl_->movement; }
bool World::pause_thumbnail_processes() noexcept {
    if (!available()) return false;
    if (isle_game_pause_link_processes(impl_->game,18,1) != ISLE_GAME_OK || isle_game_pause_link_processes(impl_->game,17,1) != ISLE_GAME_OK) { impl_->failed = true; return false; }
    return true;
}
} // namespace isle::photo

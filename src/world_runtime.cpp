#include "world_runtime.hpp"
#include "world_collision.hpp"
#include "scene_services.hpp"
#include "borrowed_model_kernel.hpp"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <utility>

// GAME pin df8815e75f3d79618466e01e0827c6165b93d11c world/block.c,
// world/movement.c and world/60860.c. Native OM records, not SDK object layouts.
namespace isle::world {
namespace {
void check(IsleGameResult r) { if (r != ISLE_GAME_OK) throw std::runtime_error("Native world OM operation failed"); }
[[noreturn]] void animation_failure(const char* operation, menu::animation::Status s) {
    std::array<char,160> message{};
    std::snprintf(message.data(),message.size(),"%s (error=%u, address=0x%08x)",operation,
        static_cast<unsigned>(s.error),static_cast<unsigned>(s.address));
    throw std::runtime_error(message.data());
}
void check(menu::animation::Status s) {
    if (!s) animation_failure("Native world animation operation failed",s);
}
Vec3 converted(menu::Vec3 v) { return {v.x,v.y,v.z}; }
Vec3 relative(const Block& a, const Block& b) {
    const auto x = a.resource->metadata.world_position, y = b.resource->metadata.world_position;
    return {(x.x-y.x)*100.0f,(x.y-y.y)*100.0f,(x.z-y.z)*100.0f};
}
ResourceView view(const Resources& r) {
    r.require_live();
    // Preserve Source's NULL map absence. Missing ground is not a malformed
    // present height map; missing ceilings leave caller-owned output intact.
    return {&r.setup(),&r.sky(),r.blocks(),r.primary_count(),r.models(),r.materials(),r.animations(),r.paths(),
            r.static_definitions(),r.height_map().wire.bytes.empty() ? nullptr : &r.height_map(),r.collision_models(),
            r.ceiling_map().wire.bytes.empty() ? nullptr : &r.ceiling_map(),r.sky_model_index()};
}
}
struct World::Impl {
    struct Binding {
        Impl* owner; Block* block; menu::animation::InstanceHandle instance; IsleGameModel* model; bool road;
        IsleGameProcess* process = nullptr;
    };
    IsleGame* game;
    scene::Services* services;
    ResourceView resources;
    ModuleState& module;
    Callbacks callbacks;
    std::vector<Block> blocks;
    std::vector<RenderObject> render;
    std::vector<Binding> bindings;
    std::unique_ptr<menu::animation::Runtime> animation;
    std::unique_ptr<collision::PreparedModels> collisions;
    std::vector<IsleGameModelNodeInit> attachment_scratch;
    Block* current = nullptr;
    IsleGameObject* sky_object = nullptr;
    IsleGameModel* sky_model = nullptr;
    menu::animation::InstanceHandle sky_instance;
    IsleGameCamera* camera = nullptr;
    IsleGameProcess* timer_process = nullptr;
    IsleGameProcess* follow_process = nullptr;
    IsleGameProcess* sky_texture_process = nullptr;
    IsleGameProcess* sky_yaw_process = nullptr;
    float sky_yaw = 0;
    bool retired = false, constructed = false, failed = false;
    bool retirement_requested = false, retirement_running = false, native_stopped = false;
    bool publication_started = false, publishing = false, destroy_started = false;
    std::array<bool,13> retirement_removed{};
    std::size_t retirement_index = 0, logical_retirement_index = 0, callback_depth = 0;
    std::string error_text = "Native world callback or retirement operation failed";
    std::string no_error;

    Impl(IsleGame* g, scene::Services* s, ResourceView r, ModuleState& m, Callbacks c)
        : game(g),services(s),resources(r),module(m),callbacks(c) {}
    void live() const {
        if (retired || retirement_requested) throw std::logic_error("Retired world component");
        if (!constructed && !publishing) throw std::logic_error("World native records are not published");
    }
    template<class Function, class... Args>
    bool invoke(Function function, Args&&... args) noexcept {
        ++callback_depth;
        const bool result = function(callbacks.user,std::forward<Args>(args)...);
        --callback_depth;
        return result;
    }
    bool callback(bool result) {
        if (!result) { failed = true; throw std::runtime_error(error_text); }
        return !retirement_requested;
    }
    void owns(const Block& b) const {
        live();
        if (b.index >= blocks.size() || &blocks[b.index] != &b) throw std::invalid_argument("Foreign world block");
    }
    static void default_update(IsleGameObject* object, void* user) noexcept {
        auto& s = *static_cast<Impl*>(user);
        if (s.retired || s.retirement_requested || s.failed) return;
        if (!s.invoke(s.callbacks.process_commands,object)) s.failed = true;
    }
    IsleGameObject* object(std::uint32_t id, std::uint32_t priority = 0x80000000) {
        IsleGameObject* result = nullptr;
        if (services) {
            result = services->object(id,9,priority);
            if (!result) throw std::runtime_error("World scene object allocation failed");
            if (!services->set_render(result,{3,priority,UINT32_MAX})) {
                services->erase(result);
                throw std::runtime_error("World scene render registration failed");
            }
        } else check(isle_game_add_object(game,id,9,priority,default_update,this,&result));
        return result;
    }
    bool erase(IsleGameObject*& object) noexcept {
        if (!object) return true;
        if (services) {
            services->erase(object);
            if (services->failed()) failed = true;
        } else if (isle_game_delete_object(game,object) != ISLE_GAME_OK) failed = true;
        // A latched native fault can accompany a completed delete. Conversely,
        // a deferred self-delete or failed join must keep this context alive.
        IsleGameObjectMetadata metadata{};
        if (isle_game_object_metadata(game,object,&metadata) == ISLE_GAME_INVALID_ARGUMENT) {
            for (auto& binding : bindings)
                if ((binding.road ? binding.block->road_object : binding.block->object) == object) {
                    binding.process = nullptr;
                    binding.model = nullptr;
                }
            for (auto& r : render) if (r.object == object) { r.object = nullptr; r.model = nullptr; }
            object = nullptr;
            return true;
        }
        failed = true;
        return false;
    }
    bool stop_object(IsleGameObject* object) noexcept {
        if (!object) return true;
        IsleGameObjectMetadata metadata{};
        if (isle_game_object_metadata(game,object,&metadata) != ISLE_GAME_OK ||
            isle_game_object_set_flags(game,object,metadata.flags|1u) != ISLE_GAME_OK ||
            isle_game_pause_updates(game,object,1) != ISLE_GAME_OK) {
            failed = true;
            return false;
        }
        return true;
    }
    bool stop_native() noexcept {
        if (native_stopped) return true;
        // Native objects/models remain live for already-admitted consumers.
        // Suppress their real scheduling, not merely our C++ callback bodies.
        for (auto& b : blocks)
            if (!stop_object(b.object) || !stop_object(b.road_object)) return false;
        if (!stop_object(sky_object)) return false;
        for (auto& binding : bindings)
            if (binding.process && isle_game_pause_process(game,binding.process,1) != ISLE_GAME_OK) {
                failed = true;
                return false;
            }
        if ((timer_process && isle_game_pause_process(game,timer_process,1) != ISLE_GAME_OK) ||
            (follow_process && isle_game_pause_process(game,follow_process,1) != ISLE_GAME_OK) ||
            (sky_texture_process && isle_game_pause_process(game,sky_texture_process,1) != ISLE_GAME_OK) ||
            (sky_yaw_process && isle_game_pause_process(game,sky_yaw_process,1) != ISLE_GAME_OK)) {
            failed = true;
            return false;
        }
        native_stopped = true;
        return true;
    }
    void begin_destroy() noexcept {
        if (publication_started && !destroy_started) {
            destroy_started = true;
            module.reset_road_events();
        }
    }
    bool advance_retirement(bool collect) noexcept {
        auto& index = collect ? retirement_index : logical_retirement_index;
        for (; index < blocks.size(); ++index) {
            auto& b = blocks[index];
            if (!retirement_removed[index] && constructed && callbacks.remove &&
                !invoke(callbacks.remove,b)) { failed = true; return false; }
            retirement_removed[index] = true;
            if (!collect) continue;
            if (b.instance) {
                if (!animation->begin_retire(b.instance)) { failed = true; return false; }
                b.instance = {};
            }
            if (b.road_instance) {
                if (!animation->begin_retire(b.road_instance)) { failed = true; return false; }
                b.road_instance = {};
            }
            // Whole-object deletion performs the actual native process joins.
            // erase retains an object/context if a join has not completed.
            if (!erase(b.object)) return false;
            b.model = nullptr;
            if (!erase(b.road_object)) return false;
            b.road_model = nullptr;
        }
        if (!collect) return true;
        if (!erase(sky_object)) return false;
        sky_model = nullptr; timer_process = nullptr; follow_process = nullptr;
        sky_texture_process = nullptr; sky_yaw_process = nullptr; sky_instance = {};
        render.clear(); current = nullptr;
        if (animation) animation->retire();
        retired = true;
        return true;
    }
    void process(IsleGameObject* o, IsleGameCallback callback, void* user, std::uint32_t priority,
                 IsleGameProcess** output = nullptr) {
        IsleGameProcess* p = nullptr;
        if (services) {
            p = services->process(o,callback,user,ISLE_GAME_SYNCHRONOUS,priority);
            if (!p) throw std::runtime_error("World scene process allocation failed");
        } else check(isle_game_add_process(game,o,callback,user,ISLE_GAME_SYNCHRONOUS,priority,&p));
        if (output) *output = p;
    }
    IsleGameModel* attach(IsleGameObject* object, std::size_t index, bool translate_root,
                          menu::animation::InstanceHandle instance) {
        if (index >= resources.models.size()) throw std::invalid_argument("World model index escapes resources");
        IsleGameModel* model = nullptr;
        const auto status = menu::model::attach_source_model(game,object,resources.models[index],
            animation->model(instance),translate_root ? menu::model::RootRecipe::translate :
            menu::model::RootRecipe::source,attachment_scratch,&model);
        if (!status) throw std::runtime_error("World source model attachment rejected");
        return model;
    }
    void visibility(std::uint32_t mask) {
        for (auto& b : blocks) {
            // Source shifts the mask only for a block that owns a model.
            if (!b.object) continue;
            const bool visible = (mask&1u) != 0; mask >>= 1;
            IsleGameObjectMetadata metadata{};
            check(isle_game_object_metadata(game,b.object,&metadata));
            check(isle_game_object_set_flags(game,b.object,visible ? metadata.flags&~1u : metadata.flags|1u));
        }
    }
    void synchronize(Binding& b) {
        const auto state = animation->model(b.instance);
        for (std::size_t i = 0; i < state.nodes.size(); ++i) {
            const auto& n = state.nodes[i];
            IsleGameModelTransform transform{converted(n.transform.position),converted(n.transform.rotation),converted(n.transform.scale)};
            if (!b.road && i == 0) {
                // block.c's texture task does not redefine the terrain root's
                // canonical pose, even though it renders translation only.
                const IsleGameModelNode* nodes = nullptr; std::size_t count = 0;
                check(isle_game_model_peek_nodes(game,b.model,&nodes,&count));
                transform = nodes[0].transform;
            }
            check(isle_game_model_set_transform(game,b.model,i,&transform));
            check(isle_game_model_set_flags(game,b.model,i,n.flags));
        }
        if (b.road) for (const auto& e : animation->events()) if (e.instance == b.instance && e.event_id >= 0 && e.event_id < 14) {
            auto& road = module.road_events[static_cast<std::size_t>(e.event_id)];
            road.animation_time = state.animation_time;
            road.visibility = e.argument;
            visibility(static_cast<std::uint32_t>(e.argument));
        }
    }
    static void update_texture(IsleGameObject*, void* user) noexcept {
        auto& b = *static_cast<Binding*>(user);
        auto& s = *b.owner;
        if (s.retired || s.retirement_requested || s.failed) return;
        try {
            float time = s.module.global_timer;
            while (time >= 100) time -= 100;
            check(s.animation->restart_model(b.instance,time));
            check(s.animation->set_texture_speed(b.instance,0));
            check(s.animation->step_model(b.instance));
            s.synchronize(b);
        } catch (...) { s.failed = true; }
    }
    static void update_timer(IsleGameObject*, void* user) noexcept {
        auto& s = *static_cast<Impl*>(user);
        if (s.retired || s.retirement_requested || s.failed) return;
        float time = s.module.global_timer+s.module.sky_animation_speed;
        while (time < 0) time += static_cast<float>(s.module.sky_max_angle);
        time *= 10000.0f;
        time = static_cast<float>((static_cast<std::int32_t>(time)+5)/10);
        time /= 1000.0f;
        while (time >= s.module.sky_max_angle) time -= static_cast<float>(s.module.sky_max_angle);
        s.module.global_timer = time;
    }
    static void update_sky_texture(IsleGameObject*, void* user) noexcept {
        auto& s = *static_cast<Impl*>(user);
        if (s.retired || s.retirement_requested || s.failed) return;
        try { check(s.animation->step_model(s.sky_instance)); }
        catch (...) { s.failed = true; }
    }
    static void update_sky_yaw(IsleGameObject*, void* user) noexcept {
        auto& s = *static_cast<Impl*>(user);
        if (s.retired || s.retirement_requested || s.failed) return;
        try {
            s.sky_yaw = 6.283185482f*s.module.global_timer/static_cast<float>(s.module.sky_max_angle);
            const IsleGameModelNode* nodes = nullptr; std::size_t count = 0;
            check(isle_game_model_peek_nodes(s.game,s.sky_model,&nodes,&count));
            auto transform = nodes[0].transform;
            transform.rotation.y = s.sky_yaw;
            check(isle_game_model_set_transform(s.game,s.sky_model,0,&transform));
        } catch (...) { s.failed = true; }
    }
    static void update_follow(IsleGameObject*, void* user) noexcept {
        auto& s = *static_cast<Impl*>(user);
        if (s.retired || s.retirement_requested || s.failed || !s.camera || !s.sky_model) return;
        const IsleGameCameraParameters* p = nullptr;
        if (isle_game_camera_parameters(s.game,s.camera,&p) != ISLE_GAME_OK) { s.failed = true; return; }
        const IsleGameModelNode* nodes = nullptr; std::size_t count = 0;
        if (isle_game_model_peek_nodes(s.game,s.sky_model,&nodes,&count) != ISLE_GAME_OK) { s.failed = true; return; }
        auto t = nodes[0].transform;
        t.position = {p->look_at.eye[0],p->look_at.eye[1],p->look_at.eye[2]};
        if (isle_game_model_set_transform(s.game,s.sky_model,0,&t) != ISLE_GAME_OK) s.failed = true;
    }
    void relocate(Block& reference) {
        for (auto& b : blocks) if (b.model) {
            const IsleGameModelNode* nodes = nullptr; std::size_t count = 0;
            check(isle_game_model_peek_nodes(game,b.model,&nodes,&count));
            auto transform = nodes[0].transform;
            transform.position = relative(b,reference);
            check(isle_game_model_set_transform(game,b.model,0,&transform));
        }
    }
};
World::World(scene::Services& s, const Resources& r, ModuleState& m, Callbacks c,
             menu::animation::ExecutionCapacity execution): World(s.game,&s,view(r),m,c,false,execution) {}
World::World(scene::Services& s, const Resources& r, ModuleState& m, Callbacks c, bool defer_publication,
             menu::animation::ExecutionCapacity execution)
    : World(s.game,&s,view(r),m,c,defer_publication,execution) {}
World::World(IsleGame* g, ResourceView r, ModuleState& m, Callbacks c,
             menu::animation::ExecutionCapacity execution): World(g,nullptr,r,m,c,false,execution) {}
World::World(IsleGame* g, scene::Services* services, ResourceView r, ModuleState& m, Callbacks c, bool defer_publication,
             menu::animation::ExecutionCapacity execution)
    : impl_(std::make_unique<Impl>(g,services,r,m,c)) {
    if (!g || !r.setup || !r.sky || r.blocks.empty() || r.blocks.size() >= 13 || r.primary_count == 0 ||
        r.primary_count > r.blocks.size() || !c.shift_camera || !c.shift_effects || (!services && !c.process_commands))
        throw std::invalid_argument("World requires source resources, command service and position-shift callbacks");
    auto& s = *impl_;
    std::vector<menu::animation::SlotCapacity> capacities;
    for (std::size_t i = 0; i < r.blocks.size(); ++i) {
        const auto& block = r.blocks[i];
        if (block.model != no_index) {
            if (block.model >= r.models.size()) throw std::invalid_argument("World model escapes resource pool");
            capacities.push_back(menu::animation::slot_capacity(r.models[block.model]));
        }
        if (i < r.primary_count && block.metadata.control_line_count >= 2 && block.road_model != no_index) {
            if (block.road_model >= r.models.size()) throw std::invalid_argument("World road escapes resource pool");
            capacities.push_back(menu::animation::slot_capacity(r.models[block.road_model]));
        }
    }
    if (r.sky_model_index != no_index) {
        if (r.sky_model_index >= r.models.size()) throw std::invalid_argument("World sky escapes resource pool");
        capacities.insert(capacities.begin(),menu::animation::slot_capacity(r.models[r.sky_model_index]));
    }
    std::size_t max_nodes = 0;
    for (const auto capacity : capacities) max_nodes = std::max(max_nodes,capacity.nodes);
    s.attachment_scratch.resize(max_nodes);
    auto animation = menu::animation::Runtime::create({r.models,r.materials,r.animations,menu::no_index,r.paths},capacities,{},execution);
    if (!animation) animation_failure("World native animation resource admission failed",animation.status);
    s.animation = std::move(animation.runtime);
    s.collisions = std::make_unique<collision::PreparedModels>(r.collision_models);
    s.blocks.resize(r.blocks.size()); s.bindings.reserve(r.blocks.size()*2); s.render.reserve(r.blocks.size()*2+1);
    for (std::size_t i = 0; i < s.blocks.size(); ++i) {
        auto& b = s.blocks[i]; b.index = i; b.resource = &r.blocks[i];
        b.control_points = i < r.primary_count && b.resource->metadata.control_line_count > 0 &&
            b.resource->metadata.control_line_count < 4;
        if (i < r.primary_count) {
            if (i) b.previous = &s.blocks[i-1];
            if (i+1 < r.primary_count) b.next = &s.blocks[i+1];
        }
    }
    if (!defer_publication) {
        try { publish(); } catch (...) { if (!retire()) std::terminate(); throw; }
    }
}
void World::publish() {
    auto& s = *impl_;
    if (s.publication_started || s.retired || s.retirement_requested ||
        isle_game_require_idle(s.game) != ISLE_GAME_OK)
        throw std::logic_error("World publication requires a prepared, unpublished idle owner");
    s.publication_started = true;
    s.publishing = true;
    s.module.reset_road_events();
    const auto& r = s.resources;
    auto* g = s.game;
    auto* services = s.services;
    try {
        s.sky_object = s.object(99);
        s.process(s.sky_object,Impl::update_timer,&s,2,&s.timer_process);
        if (r.sky->display_list_guest) {
            if (r.sky_model_index == no_index) throw std::invalid_argument("World sky lacks authoritative catalog model");
            const auto acquired = s.animation->acquire(0,r.sky_model_index,false);
            check(acquired.status); s.sky_instance = acquired.handle;
            s.sky_model = s.attach(s.sky_object,r.sky_model_index,false,s.sky_instance);
            s.render.push_back({s.sky_object,s.sky_model,r.sky_model_index,r.sky->display_list_guest,r.sky->render_callback_guest,9,3,0x80000001,true,s.sky_instance});
            if (services && !services->set_render(s.sky_object,{3,0x80000001,UINT32_MAX})) throw std::runtime_error("World sky render ordering failed");
            if (r.sky->animation_guest) {
                s.process(s.sky_object,Impl::update_sky_texture,&s,1,&s.sky_texture_process);
                check(s.animation->set_texture_speed(s.sky_instance,r.sky->animation_speed));
            }
            if (r.sky->render_callback_guest == 0x800e1d80)
                s.process(s.sky_object,Impl::update_sky_yaw,&s,1,&s.sky_yaw_process);
        }
        for (std::size_t i = 0; i < s.blocks.size(); ++i) {
            auto& b = s.blocks[i];
            if (b.resource->model != no_index) {
                const auto acquired = s.animation->acquire(s.bindings.size()+(s.sky_instance ? 1 : 0),b.resource->model,false);
                check(acquired.status);
                b.instance = acquired.handle;
                // block.c appends the translation recipe before setting the
                // instance root's raw yaw. Shared immutable resources stay intact.
                const menu::animation::Transform root{{},{0,b.resource->metadata.yaw,0},{1,1,1}};
                check(s.animation->set_node_transform(b.instance,0,root));
                b.object = s.object(static_cast<std::uint32_t>(100+i*2));
                const auto& model = r.models[b.resource->model];
                b.model = s.attach(b.object,b.resource->model,
                    model.matrix_kinds == std::array<std::uint8_t,3>{1,0,0},b.instance);
                const auto callback = model.render_type == menu::RenderType::b_fogged ?
                    0x800a15d8u : b.resource->metadata.render_callback_guest;
                s.render.push_back({b.object,b.model,b.resource->model,b.resource->metadata.display_list_guest,callback,9,3,0x80000000,false,b.instance});
                s.bindings.push_back({&s,&b,b.instance,b.model,false});
                if (b.resource->metadata.texture_animation_guest)
                    s.process(b.object,Impl::update_texture,&s.bindings.back(),1,&s.bindings.back().process);
            }
            if (i < r.primary_count && b.resource->metadata.control_line_count >= 2 && b.resource->road_model != no_index) {
                const auto acquired = s.animation->acquire(s.bindings.size()+(s.sky_instance ? 1 : 0),b.resource->road_model,false);
                check(acquired.status);
                b.road_instance = acquired.handle;
                b.road_object = s.object(static_cast<std::uint32_t>(101+i*2));
                b.road_model = s.attach(b.road_object,b.resource->road_model,false,b.road_instance);
                s.render.push_back({b.road_object,b.road_model,b.resource->road_model,0,0x80014f98,9,3,0x80000000,false,b.road_instance});
                s.bindings.push_back({&s,&b,b.road_instance,b.road_model,true});
                reset_road(b,b.resource->metadata.reversed ? static_cast<float>(b.resource->metadata.movement_animation_duration) : 0);
                check(s.animation->set_callback_enabled(b.road_instance,true));
            }
        }
        // Beach primary list is null-terminated (not the source's optional
        // repeated-first-descriptor loop form). Secondary blocks stay unlinked.
        if (s.blocks[0].road_model) reset_road(s.blocks[0],0);
        set_sky_animation_speed(r.setup->sky_animation_speed);
        s.constructed = true;
        s.publishing = false;
    } catch (...) {
        s.publishing = false;
        s.failed = true;
        throw;
    }
}
bool World::published() const noexcept { return impl_->constructed && !retired(); }
World::~World() noexcept { if (!retire()) std::terminate(); }
std::span<Block> World::blocks() noexcept { return impl_->blocks; }
std::span<const Block> World::blocks() const noexcept { return impl_->blocks; }
Block* World::current() const noexcept { return impl_->current; }
Block* World::enter_first(std::size_t index) {
    auto& s = *impl_; s.live();
    if (index >= s.blocks.size()) return nullptr;
    auto& b = s.blocks[index]; s.current = &b; s.relocate(b);
    if (s.callbacks.add) {
        if (!s.callback(s.invoke(s.callbacks.add,b,b))) return nullptr;
        if (b.previous && !s.callback(s.invoke(s.callbacks.add,*b.previous,b))) return nullptr;
        if (b.next && !s.callback(s.invoke(s.callbacks.add,*b.next,b))) return nullptr;
    }
    return &b;
}
Block* World::enter_next() {
    auto& s = *impl_; s.live();
    auto* old = s.current;
    if (!old || !old->next) return nullptr;
    auto& next = *old->next; s.current = &next;
    const auto delta = relative(*old,next);
    if (!s.callback(s.invoke(s.callbacks.shift_camera,delta))) return nullptr;
    if (!s.callback(s.invoke(s.callbacks.shift_effects,delta))) return nullptr;
    s.relocate(next);
    if (old->previous && s.callbacks.remove &&
        !s.callback(s.invoke(s.callbacks.remove,*old->previous))) return nullptr;
    if (next.next && s.callbacks.add &&
        !s.callback(s.invoke(s.callbacks.add,*next.next,next))) return nullptr;
    if (s.callbacks.change) {
        for (std::size_t i = 0; i <= old->index && i < s.blocks.size(); ++i)
            if (!s.callback(s.invoke(s.callbacks.change,s.blocks[i],next))) return nullptr;
        if (!s.callback(s.invoke(s.callbacks.change,next,next))) return nullptr;
    }
    return &next;
}
void World::reset_road(Block& b, float skip) {
    auto& s = *impl_; s.owns(b);
    if (!b.road_model) return;
    check(s.animation->restart_model(b.road_instance,skip));
    check(s.animation->set_model_speed(b.road_instance,0));
    check(s.animation->step_model(b.road_instance));
    for (auto& binding : s.bindings) if (binding.model == b.road_model) { s.synchronize(binding); break; }
}
MovementSample sample_control_points(menu::animation::ModelState state, float cp, bool reversed, std::int32_t lines,
                              std::span<const float> timestamps) {
    if (lines < 2 || state.nodes.size() < static_cast<std::size_t>(lines)) return {};
    cp = std::clamp(cp,0.0f,1.0f); if (reversed) cp = 1-cp;
    const auto segments = static_cast<std::size_t>(lines-1);
    std::size_t a = 1;
    for (std::size_t i = 2; i < segments; ++i) { a = i; if (cp <= timestamps[i-2]) { a = i-1; break; } }
    const auto& first = state.nodes[a].transform;
    if (segments == 1) return {converted(first.position),converted(first.rotation),first.scale.x};
    const float start = a < 2 || segments < 3 ? 0 : timestamps[a-2];
    const float end = a+1 >= segments ? 1 : timestamps[a-1];
    const auto& second = state.nodes[a+1].transform;
    const auto lerp = [=](float x, float y) { return (y-x)*(cp-start)/(end-start)+x; };
    return {{lerp(first.position.x,second.position.x),lerp(first.position.y,second.position.y),lerp(first.position.z,second.position.z)},
            {lerp(first.rotation.x,second.rotation.x),lerp(first.rotation.y,second.rotation.y),lerp(first.rotation.z,second.rotation.z)},
            lerp(first.scale.x,second.scale.x)};
}
MovementSample World::sample(Block& b, float move, float cp) {
    auto& s = *impl_; s.owns(b);
    if (!b.road_model || !b.control_points || !b.resource->metadata.movement_animation_guest || b.resource->road_model == no_index) return {};
    move = std::clamp(move,0.0f,1.0f); if (b.resource->metadata.reversed) move = 1-move;
    const float target = static_cast<float>(b.resource->metadata.movement_animation_duration)*move;
    const auto index = b.road_instance;
    check(s.animation->set_model_speed(index,target-s.animation->model(index).animation_time));
    check(s.animation->step_model(index));
    for (auto& binding : s.bindings) if (binding.model == b.road_model) { s.synchronize(binding); break; }
    return sample_control_points(s.animation->model(index),cp,b.resource->metadata.reversed,
                          b.resource->metadata.control_line_count,b.resource->control_timestamps);
}
float World::next_speed(const Block& b, float cp) const {
    impl_->owns(b);
    if (!b.road_model || !b.control_points || !b.resource->metadata.movement_animation_guest) return 0;
    return sample_control_points(impl_->animation->model(b.road_instance),cp,b.resource->metadata.reversed,
                          b.resource->metadata.control_line_count,b.resource->control_timestamps).speed;
}
void World::set_visibility(std::uint32_t mask) {
    auto& s = *impl_; s.live();
    s.visibility(mask);
}
void World::follow_camera(IsleGameCamera* camera) {
    auto& s = *impl_; s.live();
    if (!camera) throw std::invalid_argument("World sky follow requires a live camera");
    const IsleGameCameraParameters* p = nullptr; check(isle_game_camera_parameters(s.game,camera,&p));
    s.camera = camera;
    if (!s.follow_process && s.sky_model) s.process(s.sky_object,Impl::update_follow,&s,0,&s.follow_process);
}
void World::set_sky_position(Vec3 position, float yaw, float animation_time) {
    auto& s = *impl_; s.live();
    if (!std::isfinite(yaw) || !std::isfinite(animation_time)) throw std::invalid_argument("Invalid world sky pose");
    s.sky_yaw = yaw;
    if (s.sky_model) { const IsleGameModelTransform t{position,{},{1,1,1}}; check(isle_game_model_set_transform(s.game,s.sky_model,0,&t)); }
}
void World::set_sky_animation_speed(float speed) {
    impl_->live();
    if (!std::isfinite(speed) || speed < -10000 || speed > 10000) throw std::invalid_argument("Invalid world sky animation speed");
    impl_->module.sky_animation_speed = speed;
    if (impl_->sky_instance && impl_->resources.sky->animation_guest)
        check(impl_->animation->set_texture_speed(impl_->sky_instance,speed));
}
float World::global_time() const noexcept { return impl_->module.global_timer; }
std::span<const RenderObject> World::render_objects() const noexcept {
    if (impl_->retirement_requested) return {};
    return impl_->render;
}
menu::animation::ModelState World::animation_model(menu::animation::InstanceHandle h) const { impl_->live(); return impl_->animation->model(h); }
const menu::animation::Runtime& World::animation_runtime() const { impl_->live(); return *impl_->animation; }
const ResourceView& World::resources() const noexcept { return impl_->resources; }
RenderView World::render_view() const noexcept {
    return {&impl_->resources,impl_->animation.get(),impl_->render,this,
        [](const void* owner) noexcept { return !static_cast<const World*>(owner)->retired() &&
            static_cast<const World*>(owner)->published() && static_cast<const World*>(owner)->error().empty(); }};
}
bool World::ground_at(float x, float z, GroundResult& result) const {
    const auto& s = *impl_; s.live(); result = {};
    if (!s.current || !s.current->resource || !s.resources.height_map) return false;
    const auto p = s.current->resource->metadata.world_position;
    collision::GroundResult ground;
    const bool found = collision::ground_at(*s.resources.height_map,{p.x,p.y,p.z},x,z,ground);
    result = {ground.height,ground.normal,ground.surface}; return found;
}
bool World::ceiling_at_global(float x, float z, GroundResult& result) const {
    const auto& s = *impl_; s.live(); result = {};
    if (!s.resources.ceiling_map) return false;
    collision::GroundResult ceiling;
    const bool found = collision::ground_at_global(*s.resources.ceiling_map,x,z,ceiling);
    result = {ceiling.height,ceiling.normal,ceiling.surface}; return found;
}
bool World::ceiling_at(float x, float z, GroundResult& result) const {
    const auto& s = *impl_; s.live();
    // The source tests map absence before even looking up the current block.
    if (!s.resources.ceiling_map) return false;
    result = {};
    if (!s.current || !s.current->resource) return false;
    const auto p = s.current->resource->metadata.world_position;
    collision::GroundResult ceiling;
    const bool found = collision::ground_at(*s.resources.ceiling_map,{p.x,p.y,p.z},x,z,ceiling);
    result = {ceiling.height,ceiling.normal,ceiling.surface}; return found;
}
std::int32_t World::collide(Vec3 position, Vec3 velocity, Vec3& out_position, Vec3& out_velocity) const {
    const auto& s = *impl_; s.live();
    if (!s.current) return -1;
    const Block* neighbors[]{s.current->previous,s.current,s.current->next};
    const auto origin = s.current->resource->metadata.world_position;
    for (const auto* b : neighbors) if (b) for (const auto& object : b->resource->collision_objects)
        for (std::size_t model = 0; model < s.resources.collision_models.size(); ++model)
            if (s.resources.collision_models[model].id == object.id) {
            auto translated = object;
            const auto p = b->resource->metadata.world_position;
            translated.position.x += p.x-origin.x; translated.position.y += p.y-origin.y; translated.position.z += p.z-origin.z;
            if (s.collisions->collision_one(model,translated,position,velocity,out_position,out_velocity)) return object.id;
        }
    return -1;
}
void World::clear_block(Block& b) {
    auto& s = *impl_; s.owns(b);
    if (s.callbacks.remove && !s.callback(s.invoke(s.callbacks.remove,b))) return;
    // Public animation borrows end now. This operation does not claim a joined
    // native/graphics fence and deliberately does not release/reuse either slot.
    if (b.instance) { check(s.animation->begin_retire(b.instance)); b.instance = {}; }
    if (b.road_instance) { check(s.animation->begin_retire(b.road_instance)); b.road_instance = {}; }
    if (!s.erase(b.object)) throw std::runtime_error(s.error_text);
    b.model = nullptr;
    if (!s.erase(b.road_object)) throw std::runtime_error(s.error_text);
    b.road_model = nullptr;
}
bool World::request_retire() noexcept {
    if (!impl_ || impl_->retired) return true;
    auto& s = *impl_;
    IsleGameCounts counts{};
    if (isle_game_counts(s.game,&counts) != ISLE_GAME_OK) { s.failed = true; return false; }
    s.begin_destroy();
    s.retirement_requested = true;
    if (s.retirement_running) return false;
    s.retirement_running = true;
    const bool complete = s.stop_native() && s.advance_retirement(false);
    s.retirement_running = false;
    return complete;
}
bool World::retire() noexcept {
    if (!impl_ || impl_->retired) return true;
    auto& s = *impl_;
    // Native idle is not proof that GPU, Photo, semantic references or other
    // admitted consumers released their borrows. The scene owner supplies those
    // fences before calling this physical collector, never from a callback.
    if (s.callback_depth || s.retirement_running || isle_game_require_idle(s.game) != ISLE_GAME_OK) return false;
    s.begin_destroy();
    s.retirement_running = true;
    const bool complete = s.advance_retirement(true);
    s.retirement_running = false;
    return complete;
}
bool World::retired() const noexcept { return impl_->retired || impl_->retirement_requested; }
bool World::retirement_complete() const noexcept { return impl_->retired; }
const std::string& World::error() const noexcept { return impl_->failed ? impl_->error_text : impl_->no_error; }
} // namespace isle::world

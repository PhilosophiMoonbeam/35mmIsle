// Opt-in private admitted-ROM native/RSP/RDP consumer. Main executes; workers do
// not. The original four-family API cohort is retained unchanged. A separate
// actual Source ItemOwner/World/Actor cohort follows it, with API-authored Player
// AUX, camera/throw inputs and explicit private C process entries. Neither
// cohort is a Player FSM, Original capture, complete game or platform proof.
#include "item_producer.hpp"
#include "item_owner.hpp"
#include "effect_bank_resources.hpp"
#include "course_model_producer.hpp"
#include "actual_audio_driver.hpp"
#include "photo_lighting.hpp"
#include "scene_graphics.hpp"
#include "scene_storage.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <cmath>
#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace {
using namespace isle;
namespace a = menu::animation;
namespace k = menu::model;
namespace g = scene::graphics;
constexpr std::size_t plane = 320*240*2, slots = 6;
constexpr menu::RamRange joined{0x680000,0x10000}, guard{0x7f0000,256};
constexpr world::ProducerLayout world_layout{{0x200000,0x10000},{0x210000,0x10000},{0x220000,0x10000}};
constexpr items::ProducerLayout item_layout{{0x400000,0x10000},{0x410000,0x10000},{0x420000,0x10000},
    {{{0x3b5000,plane},{0x3da800,plane}}}};
constexpr actor::ProducerLayout actor_layout{{0x430000,0x10000},{0x440000,0x10000},{0x450000,0x10000},
    {{{0x3b5000,plane},{0x3da800,plane}}}};
constexpr g::Layout frame_layout{{0x626000,0x2000},{0x630000,0x10000},{0x640000,0x40000},0x600000};
void require(bool value, const char* text) { if (!value) throw std::runtime_error(text); }
void native(IsleGameResult result) { require(result == ISLE_GAME_OK,"Item graphics fixture native operation failed"); }
void accepted(a::Status status) { require(bool(status),"Item graphics fixture animator operation failed"); }
std::uint32_t physical(std::uint32_t address) { return address&0x1fffffffu; }
template<class F> void rejects(F&& action, const char* text) {
    bool rejected = false; try { action(); } catch (const std::exception&) { rejected = true; }
    require(rejected,text);
}
struct Capture {
    std::vector<std::uint32_t> triangles;
    std::vector<std::byte> pixels, depths;
    bool operator==(const Capture&) const = default;
};
struct Readback {
    Capture* target = nullptr;
    unsigned calls = 0;
    static void finish(void* opaque, rsp::CanonicalExecutor::MemoryLease& lease, std::uint32_t color) {
        auto& self = *static_cast<Readback*>(opaque);
        require(++self.calls == 1 && self.target,"Item draw did not acquire exactly one actual completed mapping");
        auto& result = *self.target;
        const auto& packets = lease.packets();
        for (const auto packet : packets.packets()) {
            const auto data = packets.words().subspan(packet.offset,packet.count);
            const auto opcode = (data[0]>>24)&63;
            if (opcode >= 8 && opcode <= 15) result.triangles.insert(result.triangles.end(),data.begin(),data.end());
        }
        const auto bytes = lease.rdram();
        result.pixels.resize(plane); result.depths.resize(plane);
        for (std::size_t i = 0; i < plane; ++i) {
            result.pixels[i] = bytes[(color+i)^3]; result.depths[i] = bytes[(frame_layout.depth+i)^3];
        }
        const auto words = std::span(reinterpret_cast<const std::uint32_t*>(bytes.data()),bytes.size()/4);
        for (std::size_t i = 0; i < guard.size/4; ++i)
            require(words[guard.offset/4+i] == 0x13579bdfu,"Item GPU consumer crossed its disjoint guard");
        require(!lease.postprocess_active(),"Item consumer retained postprocess ownership");
    }
};
struct ApiSlot {
    scene::Services* services = nullptr;
    a::Runtime* runtime = nullptr;
    a::InstanceHandle instance;
    IsleGameObject* object = nullptr;
    IsleGameModel* model = nullptr;
    void synchronize() {
        const auto state = runtime->model(instance); require(bool(state),"API Item lost actual Runtime incarnation");
        for (std::size_t n = 0; n < state.nodes.size(); ++n) {
            const auto& t = state.nodes[n].transform;
            const IsleGameModelTransform native_pose{{t.position.x,t.position.y,t.position.z},
                {t.rotation.x,t.rotation.y,t.rotation.z},{t.scale.x,t.scale.y,t.scale.z}};
            native(isle_game_model_set_transform(services->game,model,n,&native_pose));
            native(isle_game_model_set_flags(services->game,model,n,state.nodes[n].flags));
        }
    }
    static void animate(IsleGameObject*, void* opaque) noexcept {
        auto& self = *static_cast<ApiSlot*>(opaque);
        try { accepted(self.runtime->step_model(self.instance)); self.synchronize(); }
        catch (...) { self.services->fail("API Item animator publication failed"); }
    }
};
struct Prepare {
    items::Producer& items;
    course::ModelProducer& course;
    world::Producer& world;
    scene::Services& services;
    const photo::LightingInputs& lighting;
    IsleGameCamera* camera;
    bool mixed = false;
    bool witnessed_interleave = false;
    static bool run(void* opaque, std::span<std::uint32_t> words, std::uint64_t epoch,
                    const scene::render::Pass& pass, scene::render::Prepared& output) {
        auto& self = *static_cast<Prepare*>(opaque);
        self.lighting.install(std::as_writable_bytes(words));
        const IsleGameCameraParameters* camera = nullptr;
        native(isle_game_camera_parameters(self.services.game,self.camera,&camera));
        const auto& eye = camera->look_at.eye; const auto& at = camera->look_at.at;
        self.lighting.update_direction(std::as_writable_bytes(words),{eye[0],eye[1],eye[2]},{at[0],at[1],at[2]});
        std::fill_n(words.begin()+guard.offset/4,guard.size/4,0x13579bdfu);
        if (!self.mixed) return items::Producer::prepare_pass(&self.items,words,epoch,pass,output);
        output = self.course.prepare(words,epoch,pass);
        // Independently reduce the same actual native draw list to expected
        // callback bytes. This checks the consumer-visible ordering, not a
        // producer forwarding counter. The final task executes these bytes.
        const auto world = self.world.prepare(words,epoch,pass);
        const auto items = self.items.prepare(words,epoch,pass);
        std::array<scene::RenderOrderEntry,128> order{}; std::size_t count = 0;
        require(self.services.render_order(3,order,count),"actual source DL3 exceeded fixture scratch");
        bool saw_item = false, saw_world_after_item = false, saw_fume_after_world = false;
        for (unsigned lane = 0; lane < 4; ++lane) {
            std::size_t cursor = output.lanes[lane].address/4;
            bool started = false;
            const auto take = [&](std::uint32_t a, std::uint32_t b) {
                require(cursor+1 < words.size() && words[cursor] == a && words[cursor+1] == b,
                    "mixed callback bytes differ from actual global priority/insertion order");
                cursor += 2;
            };
            for (const auto& entry : std::span(order).first(count)) {
                const auto append = [&](const auto& objects, bool is_item) {
                    for (const auto& object : objects) if (object.object == entry.object) {
                        if (!lane) {
                            if (is_item) { saw_item = true; saw_fume_after_world |= saw_world_after_item; }
                            else saw_world_after_item |= saw_item;
                        }
                        const auto range = object.lanes[lane];
                        if (!range.commands) return;
                        if (!started) { take(0xde000000,world.camera.address); started = true; }
                        for (std::size_t i = 0; i < range.commands; ++i)
                            take(words[range.address/4+i*2],words[range.address/4+i*2+1]);
                        return;
                    }
                };
                append(world.objects,false); append(items.objects,true);
            }
            if (!output.lanes[lane].words) { require(!started,"mixed output omitted a populated lane"); continue; }
            take(0xdf000000,0);
            require(cursor == output.lanes[lane].address/4+output.lanes[lane].words,
                "mixed output retained obsolete or duplicate source callback bytes");
        }
        self.witnessed_interleave |= saw_item && saw_world_after_item && saw_fume_after_world;
        return true;
    }
};
void accepted(items::OwnerStatus status) { require(bool(status),status.detail); }
void accepted(actor::OwnerStatus status) { require(bool(status),status.detail); }
struct FixtureShift {
    scene::Services& services;
    IsleGameCamera* camera;
    menu::EffectRuntime* effects = nullptr;
    items::Owner* items = nullptr;
    static void camera_shift(void* opaque, world::Vec3 delta) noexcept {
        auto& self = *static_cast<FixtureShift*>(opaque);
        const IsleGameCameraParameters* parameters = nullptr;
        if (!self.services.check(isle_game_camera_parameters(self.services.game,self.camera,&parameters),
                "Item GPU World camera borrow failed")) return;
        auto look = parameters->look_at;
        for (unsigned axis = 0; axis < 3; ++axis) {
            const auto d = axis == 0 ? delta.x : axis == 1 ? delta.y : delta.z;
            look.eye[axis] += d; look.at[axis] += d;
        }
        self.services.check(isle_game_camera_set_look_at(self.services.game,self.camera,&look),
            "Item GPU World camera shift failed");
        if (self.items && !self.items->displace({delta.x,delta.y,delta.z}))
            self.services.fail("Item GPU World canonical Item displacement failed");
    }
    static void effects_shift(void* opaque, world::Vec3 delta) noexcept {
        auto& self = *static_cast<FixtureShift*>(opaque);
        if (!self.effects) { self.services.fail("Item GPU World FX owner absent"); return; }
        self.effects->translate({delta.x,delta.y,delta.z});
        if (!self.effects->status()) self.services.fail("Item GPU World FX translation failed");
    }
};
std::size_t rgb5_changes(const Capture& x, const Capture& y) {
    require(x.pixels.size() == plane && y.pixels.size() == plane,"actual Item RGB5 extent changed");
    std::size_t count = 0;
    for (std::size_t p = 0; p < plane; p += 2)
        count += ((std::to_integer<unsigned>(x.pixels[p]^y.pixels[p])<<8)|
            (std::to_integer<unsigned>(x.pixels[p+1]^y.pixels[p+1])&0xfeu)) != 0;
    return count; // RGBA16 bit0 alone is coverage, not visible color.
}
bool same(IsleGameVec3 x, IsleGameVec3 y) {
    return x.x == y.x && x.y == y.y && x.z == y.z;
}
// A fresh idle-boundary snapshot is taken before Frame::begin. No native
// metadata/Owner mutation is attempted inside its preparation observer.
struct OwnerPrepare {
    items::Producer& items;
    actor::Producer& actors;
    world::Producer& world;
    course::ModelProducer& course;
    const photo::LightingInputs& lighting;
    scene::Services& services;
    IsleGameCamera* camera;
    std::array<scene::RenderOrderEntry,128> order{};
    std::size_t order_count = 0;
    std::array<IsleGameObject*,2> fumes{};
    IsleGameObject* late_world = nullptr;
    bool witnessed_interleave = false;
    void snapshot() {
        require(services.render_order(3,order,order_count),"actual Owner source DL3 exceeds fixed scratch");
    }
    static bool run(void* opaque, std::span<std::uint32_t> words, std::uint64_t epoch,
                    const scene::render::Pass& pass, scene::render::Prepared& output) {
        auto& self = *static_cast<OwnerPrepare*>(opaque);
        self.lighting.install(std::as_writable_bytes(words));
        const IsleGameCameraParameters* parameters = nullptr;
        native(isle_game_camera_parameters(self.services.game,self.camera,&parameters));
        const auto& eye = parameters->look_at.eye; const auto& at = parameters->look_at.at;
        self.lighting.update_direction(std::as_writable_bytes(words),{eye[0],eye[1],eye[2]},{at[0],at[1],at[2]});
        std::fill_n(words.begin()+guard.offset/4,guard.size/4,0x13579bdfu);
        output = self.course.prepare(words,epoch,pass);
        const auto world = self.world.prepare(words,epoch,pass);
        const auto items = self.items.prepare(words,epoch,pass);
        const auto actors = self.actors.prepare(words,epoch,pass);
        bool item_before_late = false, reached_late = false, fume_after_late = false;
        for (unsigned lane = 0; lane < 4; ++lane) {
            std::size_t cursor = output.lanes[lane].address/4;
            bool started = false;
            const auto take = [&](std::uint32_t a, std::uint32_t b) {
                require(cursor+1 < words.size() && words[cursor] == a && words[cursor+1] == b,
                    "actual Owner mixed bytes differ from source global DL3 priority/insertion order");
                cursor += 2;
            };
            for (const auto& entry : std::span(self.order).first(self.order_count)) {
                const auto append = [&](const auto& objects, bool item) {
                    for (const auto& object : objects) if (object.object == entry.object) {
                        const auto range = object.lanes[lane];
                        if (!lane && range.commands) {
                            if (item && !reached_late) item_before_late = true;
                            if (object.object == self.late_world) reached_late = true;
                            if (reached_late && (object.object == self.fumes[0] || object.object == self.fumes[1]))
                                fume_after_late = true;
                        }
                        if (!range.commands) return;
                        if (!started) { take(0xde000000,world.camera.address); started = true; }
                        for (std::size_t i = 0; i < range.commands; ++i)
                            take(words[range.address/4+i*2],words[range.address/4+i*2+1]);
                        return;
                    }
                };
                append(world.objects,false); append(items.objects,true);
            }
            const bool have_actors = actors.lanes[lane].commands > 1;
            if (!output.lanes[lane].words) {
                require(!started && !have_actors,"actual Owner course omitted a populated source lane"); continue;
            }
            if (have_actors) {
                take(0xde000000,actors.camera.address); take(0xde000000,actors.lanes[lane].address);
            }
            take(0xdf000000,0);
            require(cursor == output.lanes[lane].address/4+output.lanes[lane].words,
                "actual Owner course duplicated or retained obsolete callback bytes");
        }
        self.witnessed_interleave |= item_before_late && reached_late && fume_after_late;
        return true;
    }
};
// Declared before producers/Frame: real GPU draining and producer retirement
// happen first on exceptions too. Actor queue/field borrows end before Item
// physical collection; neither this ordering nor native join is a GPU fence.
struct OwnerScope {
    actor::Owner& actors;
    items::Owner& items;
    world::World& world;
    environment_sound::Environment& environment;
    FixtureShift& shift;
    ~OwnerScope() {
        const auto actor_retired = actors.reset();
        const auto item_retired = items.retire();
        const auto actors_collected = actors.collect_joined();
        const auto items_collected = items.collect_joined();
        const bool failed = shift.services.failed() || isle_game_status(shift.services.game) != ISLE_GAME_OK;
        const bool actor_status = bool(actor_retired) || (failed && actor_retired.error == actor::OwnerError::services);
        const bool item_status = bool(item_retired) || (failed && item_retired.error == items::OwnerError::services);
        if (!actor_status || !item_status || !actors_collected || !items_collected) std::terminate();
        shift.items = nullptr; shift.effects = nullptr;
        world.retire(); environment.cleanup();
    }
};
void run_actual_owner(const rom::Snapshot& snapshot, game::State& state,
    game_audio::Session& audio, private_audio::Execution& execution, private_audio::Capture& pcm,
    const world::Resources& world_resources, const items::Resources& item_resources,
    const beach::ActorResources& actor_resources, const photo::LightingInputs& lighting) {
    const auto& setup = world_resources.setup();
    const environment_sound::BeachResources sound_resources(snapshot);
    const menu::EffectResources effect_resources(snapshot);
    const std::array banks{&effect_resources.bank(menu::EffectGroup::common),&effect_resources.bank(menu::EffectGroup::beach)};
    std::uint64_t retrace = 0; unsigned audio_tasks = 0;
    const auto pump = [&] {
        ++retrace;
        require(audio.retrace(retrace,retrace*1000000000ull/60) == audio::Error::none &&
            audio.submit() == audio::Error::none,"actual Owner audio retrace/owned PCM acceptance failed");
        audio::TaskView task; require(audio.build(task) == audio::Error::none,"actual Owner source audio task failed");
        if (task.token.serial) {
            private_audio::execute(audio,execution,task); ++audio_tasks;
            require(audio.submit() == audio::Error::none,"actual Owner completed RSP PCM submission failed");
        }
    };
    game_input::Service input; scene::Storage storage(32*1024*1024); native(storage.create());
    {
        scene::Services services(storage.game(),storage.arena(),{{4,6,0,1,2},{}},input,audio,state);
        auto* camera_object = services.object(1,0); require(camera_object,"actual Owner camera allocation failed");
        IsleGameCamera* camera = nullptr; native(isle_game_attach_camera(services.game,camera_object,&camera));
        native(isle_game_camera_add_matrix(services.game,camera,ISLE_GAME_CAMERA_PERSPECTIVE_FAST));
        native(isle_game_camera_add_matrix(services.game,camera,ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL));
        const IsleGamePerspective perspective{55,4.0f/3.0f,10,25600,1};
        const IsleGameViewport viewport{{640,480,511,0},{640,480,511,0}};
        native(isle_game_camera_set_perspective(services.game,camera,&perspective));
        native(isle_game_camera_set_viewport(services.game,camera,&viewport));
        // Fixed API-authored cohort and views, all declared before execution.
        // Ground views use the immutable real World height query, not a pose
        // search or source Runtime override. None is Original/PlayerFSM input.
        constexpr IsleGameLookAt flight_view{{0,1600,6000},{0,500,0},{0,1,0}};
        constexpr std::array<IsleGameVec3,2> flight_positions{{{-180,1233,4000},{180,1233,4000}}};
        constexpr std::array<IsleGameVec3,2> splash_positions{{{-180,1000,4000},{180,1000,4000}}};
        constexpr IsleGameVec3 throw_target{0,1133,5000}, actor_position{-600,900,4000};
        constexpr unsigned startup_frames = 6, moving_frames = 8, splash_frames = 17;
        native(isle_game_camera_set_look_at(services.game,camera,&flight_view));
        auto* player = services.object(2,9); require(player,"actual Owner API Player allocation failed");
        IsleGameModelNodeInit node{}; node.parent = UINT32_MAX; node.transform.scale = {1,1,1};
        node.matrix_kind = ISLE_GAME_MODEL_CONTEXTUAL_RPY;
        IsleGameModel* player_model = nullptr; native(isle_game_attach_model(services.game,player,&node,1,&player_model));
        const IsleGameModelTransform player_placement{{0,900,4000},{0,0,0},{1,1,1}};
        native(isle_game_model_set_placement(services.game,player_model,ISLE_GAME_MODEL_PLACEMENT_RTS,&player_placement));
        environment_sound::Environment environment(services,storage.arena(),sound_resources);
        require(environment.initialize(nullptr,camera),"environment source initialization failed");
        focus::Detector detector; FixtureShift shift{services,camera};
        world::World world(services,world_resources,{&shift,nullptr,nullptr,nullptr,
            FixtureShift::camera_shift,FixtureShift::effects_shift,nullptr});
        world.follow_camera(camera);
        a::SlotCapacity actor_capacity{}, item_capacity{};
        for (const auto& model : actor_resources.models()) {
            const auto c = a::slot_capacity(model);
            actor_capacity.nodes = std::max(actor_capacity.nodes,c.nodes);
            actor_capacity.material_attachments = std::max(actor_capacity.material_attachments,c.material_attachments);
        }
        for (const auto& model : item_resources.models()) {
            const auto c = a::slot_capacity(model);
            item_capacity.nodes = std::max(item_capacity.nodes,c.nodes);
            item_capacity.material_attachments = std::max(item_capacity.material_attachments,c.material_attachments);
        }
        // Independent fixed physical capacities, never the logical/cache20 bound.
        const std::array actor_capacities{actor_capacity,actor_capacity};
        std::array<a::SlotCapacity,8> item_capacities; item_capacities.fill(item_capacity);
        actor::Owner actors(services,actor_resources,world,environment,detector,actor_capacities,banks,{32768,4096});
        accepted(actors.publish_effects());
        shift.effects = &actors.effects(); accepted(actors.set_player(player,player_model,camera));
        require(environment.set_listener(player_model,camera),"environment listener binding failed");
        items::Globals globals;
        items::Owner owner(services,item_resources,world,actors,globals,
            {player,player_model,camera,&throw_target},item_capacities,{32768,4096});
        shift.items = &owner;
        OwnerScope owner_scope{actors,owner,world,environment,shift};
        require(world.enter_first(0) == &world.blocks()[0],"actual Owner World initial block absent");
        std::array<IsleGameLookAt,2> splash_views{};
        for (std::size_t n = 0; n < splash_positions.size(); ++n) {
            world::GroundResult ground;
            world.ground_at(splash_positions[n].x,splash_positions[n].z,ground);
            require(std::isfinite(ground.height),"actual World supplied invalid predeclared splash ground");
            splash_views[n] = {{splash_positions[n].x,ground.height+700,splash_positions[n].z+1800},
                {splash_positions[n].x,ground.height+120,splash_positions[n].z},{0,1,0}};
        }
        const auto definitions = actor_resources.definitions();
        const auto found = std::find_if(definitions.begin(),definitions.end(),
            [](const auto& definition) { return definition.actor_id == 1003; });
        require(found != definitions.end(),"actual Owner mixed common1003 definition absent");
        world::Spawn actor_spawn; actor_spawn.id = 1003; actor_spawn.behavior = 1;
        actor_spawn.position = {-6,0,40}; actor_spawn.scale = {1,1,1};
        accepted(owner.init()); accepted(owner.set_custom_functions(nullptr,0,nullptr));
        const auto actor = actors.spawn({10000,static_cast<std::size_t>(found-definitions.begin()),
            &world.blocks()[0],world.current(),&actor_spawn}); accepted(actor.status);
        const auto ball = owner.spawn_ball(flight_positions[0],{}); accepted(ball.status);
        const auto apple = owner.spawn_apple(flight_positions[1],{}); accepted(apple.status);
        const std::array flight{ball,apple};
        auto created = g::Frame::create(snapshot,{true,{16000000,4*1024*1024},frame_layout});
        require(bool(created),created.message.data());
        std::vector<menu::RamRange> mutable_ranges{world_layout.payloads,world_layout.lists,world_layout.materials,
            item_layout.payloads,item_layout.lists,item_layout.materials,actor_layout.payloads,actor_layout.lists,
            actor_layout.materials,joined,guard};
        for (const auto& span : lighting.installation_spans())
            mutable_ranges.push_back({physical(span.guest_address),span.bytes.size()});
        auto all = mutable_ranges;
        for (const auto range : created.frame->reservations()) if (range.address) all.push_back({range.address,range.size});
        for (const auto range : item_layout.framebuffers) all.push_back(range);
        const auto other = [&](std::array<menu::RamRange,3> own, bool framebuffers) {
            std::vector<menu::RamRange> result;
            for (const auto range : all) {
                bool skip = false;
                for (const auto mine : own) skip |= range.offset == mine.offset && range.size == mine.size;
                if (framebuffers) for (const auto fb : item_layout.framebuffers)
                    skip |= range.offset == fb.offset && range.size == fb.size;
                if (!skip) result.push_back(range);
            }
            return result;
        };
        auto item_other = other({item_layout.payloads,item_layout.lists,item_layout.materials},true);
        auto actor_other = other({actor_layout.payloads,actor_layout.lists,actor_layout.materials},true);
        auto world_other = other({world_layout.payloads,world_layout.lists,world_layout.materials},false);
        k::CameraBasisState basis;
        world::Producer world_producer(services.game,camera,basis,world_resources,world,world_layout,world_other);
        actor::Producer actor_producer(services.game,camera,basis,actor_resources,actor_capacities,
            actors.render_bindings(),actor_layout,actor_other,nullptr);
        items::Producer item_producer(services.game,camera,basis,item_resources,item_capacities,
            owner.render_bindings(),item_layout,item_other);
        course::ItemComposition composition{services,item_producer,128};
        course::ModelProducer course(world_producer,actor_producer,joined,&composition);
        OwnerPrepare preparation{item_producer,actor_producer,world_producer,course,lighting,services,camera};
        std::vector<sprite::AssetSpan> immutable;
        const auto append = [&](auto spans) {
            for (const auto& span : spans) immutable.push_back({physical(span.guest_address),
                {reinterpret_cast<const std::uint8_t*>(span.bytes.data()),span.bytes.size()}});
        };
        append(world_resources.installation_spans()); append(actor_resources.installation_spans());
        append(item_resources.installation_spans());
        std::vector<sprite::RamRange> reservations;
        for (const auto range : mutable_ranges)
            reservations.push_back({static_cast<std::uint32_t>(range.offset),static_cast<std::uint32_t>(range.size)});
        scene::render::Pass pass; pass.camera = camera; pass.priority = 5; pass.flags = 3;
        pass.background_rgba = (std::uint32_t(setup.background_rgb[0])<<24)|(std::uint32_t(setup.background_rgb[1])<<16)|
            (std::uint32_t(setup.background_rgb[2])<<8)|255u;
        pass.link_mask = (std::uint64_t(1)<<3)|(std::uint64_t(1)<<4)|(std::uint64_t(1)<<5);
        pass.camera_tag = UINT32_MAX; pass.prepare = OwnerPrepare::run; pass.userdata = &preparation;
        scene::render::Input scene_input{services.game,{1,nullptr,immutable,reservations,
            {0x3b5000,0x3da800},physical(lighting.pre_render_guest())},{&pass,1},{}};
        Readback readback;
        auto frame = std::move(created.frame); // Actual GPU/readback release precedes every owner on unwind.
        const auto boundary = [&] { require(frame->quiescent(),"actual Owner native mutation crossed GPU/preparation/readback/postprocess"); };
        const auto tick = [&](unsigned count = 1) {
            for (unsigned n = 0; n < count; ++n) {
                boundary(); native(isle_game_tick(services.game)); require(!services.failed(),services.error()); pump();
            }
        };
        const auto draw = [&] {
            boundary(); owner.render_bindings(); actors.render_bindings(); preparation.snapshot();
            Capture result; readback.target = &result; readback.calls = 0;
            require(frame->begin(scene_input,{0},{&readback,Readback::finish}),frame->message());
            while (frame->poll() == g::Poll::pending) std::this_thread::yield();
            require(frame->error() == g::Error::none && frame->quiescent() && readback.calls == 1,frame->message());
            return result;
        };
        const auto command = [&](IsleGameObject* object, std::int32_t value) {
            boundary(); require(services.send_command(object,value),"actual Owner FIFO enqueue failed");
        };
        const auto root = [&](a::InstanceHandle instance) {
            const IsleGameModelNode* nodes = nullptr; std::size_t count = 0;
            native(isle_game_model_peek_nodes(services.game,owner.native_model(instance),&nodes,&count));
            const auto model_state = owner.animation().model(instance);
            require(count && bool(model_state) && same(nodes[0].transform.position,
                {model_state.nodes[0].transform.position.x,model_state.nodes[0].transform.position.y,model_state.nodes[0].transform.position.z}) &&
                same(nodes[0].transform.scale,
                {model_state.nodes[0].transform.scale.x,model_state.nodes[0].transform.scale.y,model_state.nodes[0].transform.scale.z}),
                "actual Source Item ROOT was not published to native and authoritative Runtime");
            return nodes[0].transform;
        };
        tick(startup_frames);
        accepted(actors.trigger_common1003(actor.instance,{actor_position.x,actor_position.y,actor_position.z}));
        tick(); command(actor.object,-2);
        for (const auto& object : world.render_objects()) command(object.object,-2);
        tick(); // Freeze the unrelated mixed image through actual FIFO, not Runtime flags.
        const auto first = draw(); const auto before_root = root(ball.instance);
        const auto before_image = owner.animation().model(ball.instance).materials[0].image_index;
        tick(moving_frames);
        const auto moved = draw();
        require(!same(root(ball.instance).position,before_root.position) &&
            owner.animation().model(ball.instance).materials[0].image_index != before_image &&
            moved.triangles != first.triangles && moved.depths != first.depths && rgb5_changes(moved,first),
            "actual Source movement/priority3 texture animation/ROOT did not reach mixed GPU color/depth");
        for (const auto& item : flight) command(item.object,-2);
        tick();
        const auto with_actor = draw(); command(actor.object,-4); tick();
        const auto without_actor = draw();
        require(rgb5_changes(with_actor,without_actor),"actual native common1003 Actor pass did not contribute visible mixed pixels");
        command(actor.object,-5); tick(); require(draw() == with_actor,"paused mixed Actor reveal changed held source geometry");
        for (const auto& item : flight) command(item.object,-4);
        tick(); const auto no_items = draw();
        for (const auto& item : flight) {
            command(item.object,-5); tick();
            require(rgb5_changes(draw(),no_items),"actual Source ball/apple did not change no-Item mixed framebuffer");
            command(item.object,-4); tick();
        }
        for (const auto& item : flight) command(item.object,-5);
        tick();
        // Both real factories are tested at fixed, held poses. -4 must first
        // reach the actual source animator; then -2,-5 show while ALL processes
        // remain paused. Never clear sampled Runtime flags to evade Kernel609.
        for (const auto& item : flight) {
            command(item.object,-3); command(item.object,-4); tick();
            const auto held_root = root(item.instance);
            const auto held_state = owner.animation().model(item.instance);
            const auto held_time = held_state.animation_time;
            require(held_state.object_flags&1u,"real -4 was not sampled by the actual Item animator");
            const auto hidden = draw();
            command(item.object,-2); command(item.object,-5); tick();
            IsleGameObjectMetadata metadata{}; native(isle_game_object_metadata(services.game,item.object,&metadata));
            require(!(metadata.flags&1u) && (owner.animation().model(item.instance).object_flags&1u) &&
                owner.animation().model(item.instance).animation_time == held_time &&
                same(root(item.instance).position,held_root.position),
                "real FIFO -2,-5 did not preserve hidden-sampled paused source pose");
            const auto shown = draw();
            require(rgb5_changes(shown,hidden) && shown.depths != hidden.depths && shown.triangles != hidden.triangles,
                "actual Owner paused FIFO reveal lost visible GPU color/depth: native visibility must not use stale Runtime hidden flags");
            tick(2); require(draw() == shown && owner.animation().model(item.instance).animation_time == held_time &&
                same(root(item.instance).position,held_root.position),"paused source pose or mixed framebuffer continued advancing");
            command(item.object,-4); tick();
            require(draw() == hidden && owner.animation().model(item.instance).animation_time == held_time,
                "real -4 did not rehide the paused native Item without resuming");
            command(item.object,-5); tick(); require(draw() == shown,"second paused reveal retained stale GPU residency");
        }
        // Full active preflight includes hidden AND pass-filtered native owner
        // identities. These rejected standalone preparations publish no scene
        // lease; no downstream mixed-failure/global-rollback claim is made.
        std::array<items::RenderBinding,8> invalid{};
        const auto active_bindings = owner.render_bindings();
        std::copy(active_bindings.begin(),active_bindings.end(),invalid.begin());
        const auto valid = invalid[0];
        // An unrendered foreign native owner supplies a unique model with the
        // exact same topology/pose. Duplicate-model or topology rejection cannot
        // satisfy this object-to-model ownership oracle.
        boundary();
        IsleGameCounts before_donor{}; native(isle_game_counts(services.game,&before_donor));
        const IsleGameModelNode* donor_nodes = nullptr; std::size_t donor_count = 0;
        native(isle_game_model_peek_nodes(services.game,valid.model.model,&donor_nodes,&donor_count));
        std::vector<IsleGameModelNodeInit> donor_initial(donor_count);
        for (std::size_t n = 0; n < donor_count; ++n) {
            donor_initial[n].parent = donor_nodes[n].parent;
            donor_initial[n].transform = donor_nodes[n].transform;
            donor_initial[n].matrix_kind = donor_nodes[n].matrix_kind;
        }
        auto* donor_object = services.object(90000,9); require(donor_object,"foreign model owner allocation failed");
        IsleGameModel* donor_model = nullptr;
        native(isle_game_attach_model(services.game,donor_object,donor_initial.data(),donor_initial.size(),&donor_model));
        for (std::size_t n = 0; n < donor_count; ++n)
            native(isle_game_model_set_flags(services.game,donor_model,n,donor_nodes[n].flags));
        invalid[0].model.model = donor_model;
        std::vector<std::uint32_t> admission(menu::rdram_size/4,0x5a3cc3a5u);
        const auto untouched = admission;
        items::Producer invalid_producer(services.game,camera,basis,item_resources,item_capacities,invalid,item_layout,item_other);
        command(valid.model.object,-4); tick();
        const auto retained_basis = basis.value;
        rejects([&] { invalid_producer.prepare(admission,100,pass); },"hidden actual Owner native model mismatch escaped active preflight");
        auto filtered = pass; filtered.tag_match = scene::render::TagMatch::equality; filtered.camera_tag = 0;
        rejects([&] { invalid_producer.prepare(admission,101,filtered); },"pass-filtered actual Owner model mismatch escaped active preflight");
        invalid[0] = valid; invalid[0].model.state.instance = {};
        rejects([&] { invalid_producer.prepare(admission,102,filtered); },"filtered actual Owner stale incarnation escaped active preflight");
        require(admission == untouched && std::memcmp(&retained_basis,&basis.value,sizeof(retained_basis)) == 0,
            "actual Owner preflight rejection published RAM/basis state");
        invalid_producer.retire(); boundary();
        services.erase(donor_object);
        IsleGameCounts after_donor{}; native(isle_game_counts(services.game,&after_donor));
        require(!services.failed() && after_donor.objects==before_donor.objects &&
            after_donor.models==before_donor.models && after_donor.model_nodes==before_donor.model_nodes,
            "foreign model owner did not actually retire its native object and model");
        command(valid.model.object,-5); tick();
        for (const auto& object : world.render_objects()) if (!object.sky) { preparation.late_world = object.object; break; }
        require(preparation.late_world && services.set_render(preparation.late_world,{3,0x80000000u,UINT32_MAX}),
            "actual source late World insertion failed");
        std::array<items::SpawnResult,2> triggers{};
        for (std::size_t n = 0; n < triggers.size(); ++n) {
            triggers[n] = owner.spawn_apple(splash_positions[n],{}); accepted(triggers[n].status);
            owner.source(triggers[n].instance)->state = ISLE_ITEM_STILL;
            native(isle_game_end_source_processes(services.game,triggers[n].object,Items_UpdateItemMovement));
            IsleGameProcess* process = nullptr;
            native(isle_game_add_source_process(services.game,triggers[n].object,
                n == 0 ? Items_CreateWaterSplash : Items_CreateLavaSplash,ISLE_GAME_STACKFUL,7,&process));
        }
        tick();
        std::array<a::InstanceHandle,2> fumes{};
        for (const auto& binding : owner.render_bindings()) if (binding.model.object &&
            (binding.family == items::Family::water_splash || binding.family == items::Family::lava_splash)) {
            const auto n = binding.family == items::Family::water_splash ? 0u : 1u;
            fumes[n] = binding.model.state.instance; preparation.fumes[n] = binding.model.object;
        }
        require(fumes[0] && fumes[1] && globals.source.total == 4 && globals.source.balls == 1 && globals.source.apples == 3,
            "actual water/lava fumes were missing or acquired logical Item payload/counts");
        for (std::size_t n = 0; n < triggers.size(); ++n)
            require(!owner.source(preparation.fumes[n]) && root(triggers[n].instance).position.y == splash_positions[n].y-2 &&
                same(root(fumes[n]).scale,{.1f,.1f,.1f}),"actual splash sink-before-wait or independent NULL-payload fume changed");
        tick(splash_frames-1); boundary(); accepted(owner.pause());
        const auto held_sink = root(triggers[0].instance).position;
        const auto held_fume_time = owner.animation().model(fumes[0]).animation_time;
        for (auto* object : preparation.fumes) command(object,-4);
        for (const auto& trigger : triggers) command(trigger.object,-4);
        tick(2);
        require(same(root(triggers[0].instance).position,held_sink) &&
            owner.animation().model(fumes[0]).animation_time == held_fume_time,"source pause omitted waiting sink/fume animation");
        for (std::size_t n = 0; n < fumes.size(); ++n) {
            boundary(); native(isle_game_camera_set_look_at(services.game,camera,&splash_views[n]));
            const auto hidden = draw(); command(preparation.fumes[n],-5); tick();
            const auto visible = draw();
            require(rgb5_changes(visible,hidden) && visible.triangles != hidden.triangles,
                "actual source water/lava animated fume did not reach mixed framebuffer RGB5");
            command(preparation.fumes[n],-4); tick(); require(draw() == hidden,"actual paused fume rehide changed unrelated mixed image");
        }
        for (auto* object : preparation.fumes) command(object,-5);
        tick(); const auto source_fume_frame = draw();
        require(preparation.witnessed_interleave,"actual Owner mixed GPU bytes lost Item-before-lateWorld-before-newFume global order");
        // Resume only these genuine waiting/animation processes, not the frozen
        // World/Actor/flight cohort. Exactly 17 of the source's 59 sinks ran.
        for (const auto& trigger : triggers) command(trigger.object,-3);
        for (auto* object : preparation.fumes) command(object,-3);
        tick(moving_frames);
        const auto advanced_fume_frame = draw();
        root(fumes[0]); root(fumes[1]);
        require(owner.animation().model(fumes[0]).animation_time > held_fume_time &&
            rgb5_changes(advanced_fume_frame,source_fume_frame),
            "actual .9 source fume animation did not change the held-view mixed GPU framebuffer");
        tick(59-splash_frames-moving_frames);
        for (const auto& trigger : triggers) require(owner.exists(trigger.object),"actual splash deleted before final source wait");
        tick();
        for (const auto& trigger : triggers) require(!owner.object(trigger.instance),"actual 59-sink final wait did not delete trigger");
        for (const auto instance : fumes) require(owner.object(instance),"actual fume lifetime was coupled to trigger deletion");
        tick(8);
        for (const auto instance : fumes) require(!owner.object(instance),"actual .9 source animation cutoff did not delete independent fume");
        boundary(); accepted(owner.collect_joined());
        native(isle_game_camera_set_look_at(services.game,camera,&flight_view));
        // Real semantic reference ownership must survive logical deletion and
        // actual native join, then release through Actor's genuine queued22.
        auto* actor_source = actors.source(actor.instance); require(actor_source,"actual mixed Actor lost source record");
        require(services.source_reference_assign(actor.object,&actor_source->interactionTarget,apple.object),
            "actual mixed Actor failed canonical Item target adoption");
        boundary(); accepted(owner.delete_item(apple.object)); tick();
        require(owner.membership(apple.instance).joined,"actual Item deletion did not reach native join");
        const auto held = owner.collect_joined();
        require(!held && held.error == items::OwnerError::lifetime && actor_source->interactionTarget == apple.object,
            "joined Item was physically collected before real queued target22/semantic release");
        tick(); require(!actor_source->interactionTarget,"actual Actor queued22 failed canonical target release");
        boundary(); accepted(owner.collect_joined());
        const auto replacement = owner.spawn_apple(flight_positions[1],{}); accepted(replacement.status);
        require(replacement.instance != apple.instance,"physical Item reuse admitted stale incarnation");
        tick(startup_frames);
        IsleGameObjectMetadata replacement_metadata{};
        native(isle_game_object_metadata(services.game,replacement.object,&replacement_metadata));
        require(!(replacement_metadata.flags&1u),"source replacement was not revealed after its unchanged startup delay");
        boundary(); native(isle_game_object_set_flags(services.game,replacement.object,replacement_metadata.flags|1u));
        const auto excluding_replacement = draw();
        boundary(); native(isle_game_object_set_flags(services.game,replacement.object,replacement_metadata.flags));
        const auto including_replacement = draw();
        require(rgb5_changes(including_replacement,excluding_replacement) &&
            including_replacement.depths != excluding_replacement.depths &&
            including_replacement.triangles != excluding_replacement.triangles,
            "reused actual Owner incarnation contributed no geometry/depth/RGB5 beyond the contemporaneous surviving cohort");
        require(frame->retire(),"actual Owner final GPU/readback/postprocess retirement failed");
        item_producer.retire(); actor_producer.retire(); world_producer.retire();
        accepted(actors.reset()); accepted(owner.retire()); accepted(owner.collect_joined());
    }
    native(storage.destroy());
    require(audio.stop() == audio::Error::none,"actual Owner source audio stop failed");
    for (unsigned n = 0; n < 240 && audio.state() != game_audio::Session::State::draining; ++n) pump();
    require(audio_tasks && pcm.used && audio.state() == game_audio::Session::State::draining &&
        audio.service().voices_retired() && audio.poll_drain() == audio::Error::none && audio.poll_drain() == audio::Error::none &&
        audio.state() == game_audio::Session::State::drained,"actual Owner native/source audio task or owned PCM retirement failed");
}
void run(const std::filesystem::path& filename, const std::filesystem::path& profile) {
    require(std::filesystem::is_directory(profile) && std::filesystem::is_empty(profile),
        "private Item graphics profile must exist and be empty");
    const auto snapshot = rom::read_snapshot(filename);
    const world::Resources world_resources(snapshot); const auto& setup = world_resources.setup();
    const items::Resources item_resources(snapshot,setup.fog_rgb,setup.fog_min,setup.fog_max);
    const beach::ActorResources actor_resources(snapshot,{setup.fog_rgb,setup.fog_min,setup.fog_max});
    const photo::LightingInputs lighting(snapshot);
    const menu::EffectResources api_effect_resources(snapshot);
    const std::array api_banks{&api_effect_resources.bank(menu::EffectGroup::common),
        &api_effect_resources.bank(menu::EffectGroup::beach)};
    // The original API cohort only borrows audio. The separate actual Source
    // cohort below requests real factory sounds and executes/drains their RSP PCM.
    auto hydrated = game_audio::hydrate(snapshot); require(bool(hydrated),"actual shared audio resources failed");
    std::array<audio::effects::Section,8> sections; audio::effects::Config effects;
    require(game_audio::native_startup_effects(hydrated.resources,sections,effects) == audio::Error::none,
        "actual shared audio configuration failed");
    private_audio::Capture pcm; private_audio::Execution execution{snapshot,nullptr,{}};
    auto audio = game_audio::Session::create(std::move(hydrated.resources),effects,pcm.output(),execution.executor());
    require(bool(audio),"actual shared Session creation failed");
    game::State state(profile); require(state.initialize(1),"private Item graphics profile init failed");
    game_input::Service input; scene::Storage storage(16*1024*1024); native(storage.create());
    {
        scene::Services services(storage.game(),storage.arena(),{{4,6,0,1,2},{}},input,*audio.session,state);
        auto* camera_object = services.object(1,0); require(camera_object,"actual camera owner missing");
        IsleGameCamera* camera = nullptr; native(isle_game_attach_camera(services.game,camera_object,&camera));
        native(isle_game_camera_add_matrix(services.game,camera,ISLE_GAME_CAMERA_PERSPECTIVE_FAST));
        native(isle_game_camera_add_matrix(services.game,camera,ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL));
        const IsleGamePerspective perspective{55,4.0f/3.0f,10,25600,1};
        const IsleGameLookAt look{{0,1600,6000},{0,500,0},{0,1,0}}; // Existing explicit diagnostic camera, not Player.
        const IsleGameViewport viewport{{640,480,511,0},{640,480,511,0}};
        native(isle_game_camera_set_perspective(services.game,camera,&perspective));
        native(isle_game_camera_set_look_at(services.game,camera,&look));
        native(isle_game_camera_set_viewport(services.game,camera,&viewport));
        a::SlotCapacity maximum{};
        for (const auto& model : item_resources.models()) {
            const auto c = a::slot_capacity(model);
            maximum.nodes = std::max(maximum.nodes,c.nodes);
            maximum.material_attachments = std::max(maximum.material_attachments,c.material_attachments);
        }
        std::array<a::SlotCapacity,slots> capacities; capacities.fill(maximum);
        auto animation = a::Runtime::create({item_resources.models(),item_resources.materials(),item_resources.animations(),menu::no_index},
            capacities,{},{64,0}); require(bool(animation),"actual four-family Runtime failed");
        std::array<items::RenderBinding,slots> bindings{};
        std::array<ApiSlot,slots> live{};
        const auto spawn = [&](std::size_t slot, items::Family family, menu::Vec3 position) {
            const auto& definition = item_resources.definition(family);
            const auto acquired = animation.runtime->acquire(slot,definition.model,false);
            require(bool(acquired),"API Item actual incarnation acquire failed");
            auto root = animation.runtime->model(acquired.handle).nodes[0].transform;
            root.position = position; root.scale = definition.root_scale;
            accepted(animation.runtime->set_node_transform(acquired.handle,0,root));
            accepted(animation.runtime->set_texture_speed(acquired.handle,definition.texture_speed));
            auto* object = services.object(static_cast<std::uint32_t>(300+slot),4);
            require(object && services.set_render(object,{3,0x80000000u,UINT32_MAX}),"API native Item/DL3 registration failed");
            std::array<IsleGameModelNodeInit,4> scratch{}; IsleGameModel* model = nullptr;
            require(bool(k::attach_source_model(services.game,object,item_resources.models()[definition.model],
                animation.runtime->model(acquired.handle),k::RootRecipe::source,scratch,&model)),"API actual Item model attachment failed");
            live[slot] = {&services,animation.runtime.get(),acquired.handle,object,model};
            bindings[slot] = {{slot,animation.runtime->model(acquired.handle),object,model,k::PoseSource::native,
                k::RootRecipe::source,true,animation.runtime.get()},family,{3,0x80000000u,UINT32_MAX}};
            require(services.process(object,ApiSlot::animate,&live[slot],ISLE_GAME_SYNCHRONOUS,3),"API actual priority3 animator failed");
        };
        // Creation order intentionally differs from physical slot/resource order.
        spawn(3,items::Family::pester_ball,{-180,1233,4000});
        spawn(0,items::Family::apple,{180,1233,4000});
        spawn(2,items::Family::pester_ball,{-540,1233,4000});
        spawn(5,items::Family::apple,{540,1233,4000});
        menu::EffectRuntime api_effects(api_banks,services.rng);
        require(bool(api_effects.status()),"API World actual FX owner admission failed");
        FixtureShift api_shift{services,camera,&api_effects};
        world::World world(services,world_resources,{&api_shift,nullptr,nullptr,nullptr,
            FixtureShift::camera_shift,FixtureShift::effects_shift,nullptr}); world.follow_camera(camera);
        require(world.enter_first(0) == &world.blocks()[0],"actual World initial block missing");
        spawn(4,items::Family::water_splash,{-180,900,4000});
        spawn(1,items::Family::lava_splash,{180,900,4000});
        auto created = g::Frame::create(snapshot,{true,{16000000,4*1024*1024},frame_layout});
        require(bool(created),created.message.data());
        std::vector<menu::RamRange> mutable_ranges{world_layout.payloads,world_layout.lists,world_layout.materials,
            item_layout.payloads,item_layout.lists,item_layout.materials,actor_layout.payloads,actor_layout.lists,
            actor_layout.materials,joined,guard};
        for (const auto& span : lighting.installation_spans()) mutable_ranges.push_back({physical(span.guest_address),span.bytes.size()});
        auto all = mutable_ranges;
        for (const auto range : created.frame->reservations()) if (range.address) all.push_back({range.address,range.size});
        for (const auto range : item_layout.framebuffers) all.push_back(range);
        const auto other = [&](std::array<menu::RamRange,3> own, bool framebuffers) {
            std::vector<menu::RamRange> result;
            for (const auto range : all) {
                bool skip = false;
                for (const auto mine : own) skip |= range.offset == mine.offset && range.size == mine.size;
                if (framebuffers) for (const auto fb : item_layout.framebuffers)
                    skip |= range.offset == fb.offset && range.size == fb.size;
                if (!skip) result.push_back(range);
            }
            return result;
        };
        auto item_other = other({item_layout.payloads,item_layout.lists,item_layout.materials},true);
        auto world_other = other({world_layout.payloads,world_layout.lists,world_layout.materials},false);
        auto actor_other = other({actor_layout.payloads,actor_layout.lists,actor_layout.materials},true);
        a::SlotCapacity actor_capacity{};
        for (const auto& model : actor_resources.models()) {
            const auto c = a::slot_capacity(model);
            actor_capacity.nodes = std::max(actor_capacity.nodes,c.nodes);
            actor_capacity.material_attachments = std::max(actor_capacity.material_attachments,c.material_attachments);
        }
        const std::array actor_capacities{actor_capacity};
        const std::array<actor::ActorRenderBinding,1> no_actors{};
        menu::model::CameraBasisState basis;
        world::Producer world_producer(services.game,camera,basis,world_resources,world,world_layout,world_other);
        actor::Producer actors(services.game,camera,basis,actor_resources,actor_capacities,no_actors,actor_layout,actor_other,nullptr);
        items::Producer producer(services.game,camera,basis,item_resources,capacities,bindings,item_layout,item_other);
        course::ItemComposition composition{services,producer,128};
        course::ModelProducer models(world_producer,actors,joined,&composition);
        Prepare preparation{producer,models,world_producer,services,lighting,camera};
        std::vector<sprite::AssetSpan> immutable;
        const auto append = [&](auto spans) { for (const auto& span : spans) immutable.push_back({physical(span.guest_address),
            {reinterpret_cast<const std::uint8_t*>(span.bytes.data()),span.bytes.size()}}); };
        append(world_resources.installation_spans()); append(actor_resources.installation_spans()); append(item_resources.installation_spans());
        std::vector<sprite::RamRange> reservations;
        for (const auto range : mutable_ranges) reservations.push_back({static_cast<std::uint32_t>(range.offset),static_cast<std::uint32_t>(range.size)});
        const auto rgba = (std::uint32_t(setup.background_rgb[0])<<24)|(std::uint32_t(setup.background_rgb[1])<<16)|
            (std::uint32_t(setup.background_rgb[2])<<8)|255u;
        scene::render::Pass pass; pass.camera = camera; pass.priority = 5; pass.flags = 3;
        pass.background_rgba = rgba; pass.link_mask = (std::uint64_t(1)<<3)|(std::uint64_t(1)<<4)|(std::uint64_t(1)<<5);
        pass.camera_tag = UINT32_MAX; pass.prepare = Prepare::run; pass.userdata = &preparation;
        scene::render::Input scene_input{services.game,{1,nullptr,immutable,reservations,{0x3b5000,0x3da800},physical(lighting.pre_render_guest())},{&pass,1},{}};
        Readback readback;
        auto frame = std::move(created.frame); // Destroy/drain graphics before every borrowed producer/model owner.
        const auto draw = [&] {
            require(frame->quiescent(),"API Item mutation/draw crossed actual GPU completion");
            Capture result; readback.target = &result; readback.calls = 0;
            require(frame->begin(scene_input,{0},{&readback,Readback::finish}),frame->message());
            while (frame->poll() == g::Poll::pending) std::this_thread::yield();
            require(frame->error() == g::Error::none && frame->quiescent() && readback.calls == 1,frame->message());
            return result;
        };
        const auto hide = [&](std::size_t slot, bool hidden) {
            IsleGameObjectMetadata metadata{}; native(isle_game_object_metadata(services.game,live[slot].object,&metadata));
            native(isle_game_object_set_flags(services.game,live[slot].object,(metadata.flags&~1u)|(hidden?1u:0u)));
        };
        constexpr std::array active{0u,1u,2u,3u,4u,5u};
        for (unsigned step = 0; step < 17; ++step) native(isle_game_tick(services.game));
        for (auto slot : active) hide(slot,true);
        const auto hidden = draw(); require(hidden.triangles.empty(),"hidden actual Items emitted geometry");
        for (auto slot : active) {
            hide(slot,false); const auto visible = draw();
            require(!visible.triangles.empty() && visible.pixels != hidden.pixels,
                "actual fogged B/D family did not reach GPU geometry and visible color");
            hide(slot,true);
        }
        for (auto slot : active) hide(slot,false);
        const auto initial = draw();
        pass.tag_match = scene::render::TagMatch::equality; pass.camera_tag = 0;
        require(draw() == hidden,"camera-tag equality filter retained Item geometry/residency");
        pass.tag_match = scene::render::TagMatch::intersection; pass.camera_tag = UINT32_MAX;
        require(draw() == initial,"same-epoch hidden/tag reveal retained stale GPU payload");
        std::array<std::uint32_t,2> domains{};
        for (auto slot : active) hide(slot,slot != 0 && slot != 3);
        const auto material_baseline = draw();
        bool material_pixels_changed = false;
        std::array<std::uint32_t,2> sampled_images{};
        const auto arena_used = storage.arena().cursor;
        for (unsigned step = 0; step < 256; ++step) {
            require(frame->quiescent(),"native Item animator tick crossed GPU fence");
            native(isle_game_tick(services.game)); require(!services.failed(),services.error());
            require(animation.runtime->events().empty(),"Item API fixture acquired Actor animation callbacks");
            require(storage.arena().cursor == arena_used,"native Item animation/rendering allocated hot-path arena storage");
            domains[0] |= 1u<<animation.runtime->model(live[3].instance).materials[0].image_index;
            domains[1] |= 1u<<animation.runtime->model(live[0].instance).materials[0].image_index;
            const auto ball_image = animation.runtime->model(live[3].instance).materials[0].image_index;
            const auto apple_image = animation.runtime->model(live[0].instance).materials[0].image_index;
            // Fixed source-speed envelope. Sample each genuine newly reached
            // image, not an advance-until-passing texture/pose/camera workaround.
            if (!(sampled_images[0]&(1u<<ball_image)) || !(sampled_images[1]&(1u<<apple_image))) {
                const auto sampled = draw();
                material_pixels_changed |= sampled.pixels != material_baseline.pixels;
                sampled_images[0] |= 1u<<ball_image; sampled_images[1] |= 1u<<apple_image;
            }
        }
        require(domains[0] == 0xff && domains[1] == 0x3f,"actual Item material cycles lost complete 8/6 image domains");
        require(material_pixels_changed,"actual Item texture animation did not change GPU color");
        for (auto slot : active) hide(slot,false);
        const auto animated = draw();
        auto root = animation.runtime->model(live[0].instance).nodes[0].transform;
        root.scale.x *= 0.9f; root.scale.y *= 0.9f; root.scale.z *= 0.9f;
        root.position.y -= root.scale.y*12.0f;
        accepted(animation.runtime->set_node_transform(live[0].instance,0,root)); live[0].synchronize();
        const auto shrunk = draw();
        require(shrunk.triangles != animated.triangles && shrunk.depths != animated.depths,
            "native/Runtime root shrink and sink did not reach rendered geometry/depth");
        // Hidden/stale and undersized-slot admission reject before ANY RAM,
        // native camera or retained basis publication, even when no draw occurs.
        std::vector<std::uint32_t> admission(menu::rdram_size/4,0x5a3cc3a5u);
        const auto before = admission;
        hide(0,true); const auto old_binding = bindings[0];
        bindings[0].model.state.instance = {};
        const auto retained = basis.value;
        const IsleGameCameraMatrix* camera_before = nullptr;
        native(isle_game_camera_peek_matrix(services.game,camera,0,&camera_before)); const auto revision = camera_before->revision;
        rejects([&] { producer.prepare(admission,100,pass); },"hidden stale Item binding was skipped");
        require(admission == before && camera_before->revision == revision && std::memcmp(&retained,&basis.value,sizeof(retained)) == 0,
            "hidden stale rejection partially published native/RAM/basis state");
        bindings[0] = old_binding; hide(0,false);
        auto tight_capacities = capacities; tight_capacities[0].nodes = 1;
        items::Producer tight(services.game,camera,basis,item_resources,tight_capacities,bindings,item_layout,item_other);
        hide(0,true); rejects([&] { tight.prepare(admission,101,pass); },"hidden Item bypassed physical matrix capacity");
        require(admission == before,"hidden Item capacity rejection changed RAM"); hide(0,false); tight.retire();
        preparation.mixed = true;
        const auto mixed = draw();
        require(preparation.witnessed_interleave && !mixed.triangles.empty(),
            "real mixed GPU consumer lost Item-before-World-before-fume source ordering");
        IsleGameObject* later_world = nullptr;
        for (const auto& object : world.render_objects()) if (!object.sky) { later_world = object.object; break; }
        require(later_world && services.set_render(later_world,{3,0x80000000u,UINT32_MAX}),"actual later World insertion failed");
        draw(); // Independent byte oracle proves the later World insertion is interleaved, not appended by family.
        preparation.mixed = false;
        const auto stable = draw();
        require(frame->retire(),"real graphics Frame retirement failed");
        // New canonical executor/epoch cannot trust old equal matrix/material
        // revisions. Compare actual geometry/color/depth, not upload callcounts.
        auto relocated = g::Frame::create(snapshot,{true,{16000000,4*1024*1024},frame_layout});
        require(bool(relocated),relocated.message.data()); frame = std::move(relocated.frame);
        require(draw() == stable,"new canonical RAM owner retained stale Item payload residency");
        const auto held = live[0].instance;
        require(frame->quiescent(),"physical Item reuse preceded graphics/readback release");
        services.erase(live[0].object); accepted(animation.runtime->begin_retire(held));
        bindings[0] = {}; live[0] = {}; accepted(animation.runtime->release_retired(held));
        spawn(0,items::Family::pester_ball,{180,1233,4000});
        native(isle_game_tick(services.game));
        require(!draw().triangles.empty(),"same physical slot/new family incarnation failed actual rendering");
        require(frame->retire(),"final actual GPU/readback fence failed");
        producer.retire(); actors.retire(); world_producer.retire(); world.retire();
        for (auto slot : active) {
            services.erase(live[slot].object); accepted(animation.runtime->begin_retire(live[slot].instance));
            accepted(animation.runtime->release_retired(live[slot].instance));
        }
        animation.runtime->retire();
    }
    native(storage.destroy());
    pcm.samples.resize(8192*audio::AudioService::max_frames*2);
    run_actual_owner(snapshot,state,*audio.session,execution,pcm,world_resources,item_resources,actor_resources,lighting);
    std::cout << "native unchanged four-family API and actual Source ItemOwner mixed World/Actor GPU color/depth/animation/"
        "paused-FIFO-visibility/global-DL3/native-semantic-audio-retirement passed; PlayerFSM/Original/fullgame/Windows11 not claimed\n";
}
} // namespace
int main(int argc, char** argv) {
    if (argc != 3) { std::cerr << "usage: item-producer-rsp-smoke <admitted-original.z64> <empty-private-profile>\n"; return 1; }
    try { run(argv[1],argv[2]); return 0; }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}

#include "rainbow_lifecycle.hpp"
#include "player_root.hpp"
#include "player_renderer.hpp"
#include "player_commands.hpp"
#include "player_ui.hpp"
#include "player_bump.hpp"
#include "player_cart.hpp"
#include "player_camera_animation.hpp"
#include "actor_photo_scene.hpp"
#include "actor_interaction_services.hpp"
#include "rainbow_actor.hpp"
#include "world_collision.hpp"
#include "course_model_producer.hpp"
#include "effect_bank_resources.hpp"
#include "screen_transition_resources.hpp"
#include "scene_storage.hpp"
#include "menu_scene_bridge.h"
#include <algorithm>
#include <cstdio>
#include <exception>
#include <source_location>
#include <stdexcept>
#include <utility>
#include <vector>

struct IsleRainbowLifecycleAsset {
    IsleRainbowLifecycleAssetId id;
    std::span<const std::byte> bytes;
    bool loaded=false;
};
struct IsleRainbowLifecycleEffect { isle::menu::EffectHandle handle; };
struct IsleRainbowNativeCollision {
    const isle::world::RainbowGateCollision* resources;
    const isle::world::collision::PreparedModels* prepared;
};
namespace isle::rainbow {
namespace {
namespace g=scene::graphics;
namespace a=menu::animation;
constexpr std::size_t plane=320*240*2;
constexpr world::ProducerLayout world_layout{{0x200000,0x10000},{0x210000,0x10000},{0x220000,0x10000}};
constexpr actor::ProducerLayout actor_layout{{0x400000,0x80000},{0x480000,0x80000},{0x500000,0x100000},{{{0x3b5000,plane},{0x3da800,plane}}}};
constexpr menu::model::PacketLayout item_layout{{0x230000,0x10000},{0x240000,0x10000},{0x250000,0x10000},{{{0x3b5000,plane},{0x3da800,plane}}}};
constexpr player::cart::ProducerLayout cart_layout{{0x260000,0x10000},{0x270000,0x10000},{0x2a0000,0x10000},{{{0x3b5000,plane},{0x3da800,plane}}}};
constexpr focus::CaptureLayout capture_layout{{0x680000,0x50},{0x680080,0x80},{0x680100,0xa00}};
constexpr menu::RamRange fx_commands{0x680c00,0x5400},joined_lists{0x686000,0x5000};
constexpr sprite::RamRange lanes{0x690000,0x40000},sprite_commands{0x780000,0x50000},message_commands{0x6d0000,0x30000};
constexpr g::Layout frame_layout{{0x626000,0x2000},{0x630000,0x10000},{0x640000,0x40000},0x600000};
std::uint32_t physical(std::uint32_t value) { return value&0x1fffffffu; }
void require(bool value,const char* message) { if (!value) throw std::runtime_error(message); }
void native(IsleGameResult value,const char* message) { require(value==ISLE_GAME_OK,message); }
world::Block* block_cast(IsleRainbowLifecycleBlock* value) { return reinterpret_cast<world::Block*>(value); }
IsleRainbowLifecycleBlock* source_block(world::Block* value) { return reinterpret_cast<IsleRainbowLifecycleBlock*>(value); }
}
struct Lifecycle::Impl {
    const rom::Snapshot& snapshot;
    application::Manager& manager;
    game::State& state;
    game_input::Service& input;
    game_audio::Session& audio;
    host::GtlClock& clock;
    application::Photos& photos;
    world::ModuleState& world_state;
    IsleRainbowActorSource actor_source{};
    std::unique_ptr<CourseState> course_state;
    bool require_cpu_renderer=false;
    IsleRainbowLifecycleSource source{};
    IsleContinuationDomain* domain=nullptr;
    IsleContinuation* continuation=nullptr;
    LifecycleStatus status=LifecycleStatus::ready;
    IsleGameResult error=ISLE_GAME_OK;
    std::array<char,512> message{};
    bool initializing=false,initialized=false,exit_requested=false,cancelling=false;
    bool requested=false,world_published=false,sky_follow=false,screen_applied=false;
    bool helper_joined=false,helper_registered=false,scene_retired=false;
    bool helper_observed=false;
    bool input_registered=false,input_joined=false;
    bool domains_bound=false, ui_bound=false, collision_bound=false;
    bool join_receipt=false;
    player::cart::Owner::Reference cutscene_cart;
    IsleRainbowAnimationCallback cutscene_callback=nullptr;
    bool near_disabled=false;
    world::Resources world_resources;
    actor::Resources actor_resources;
    actor::BehaviorResources behavior_resources;
    world::RainbowGateCollision gate_resources;
    world::collision::PreparedModels gate_collision;
    IsleRainbowNativeCollision gate_native{&gate_resources,&gate_collision};
    items::Resources item_resources;
    player::CartResources cart_resources;
    player::UiResources ui_resources;
    environment_sound::Resources sounds;
    menu::EffectResources effect_resources;
    photo::LightingInputs lighting;
    graphics::SourcePrograms programs;
    screen_transition::Resources transition_resources;
    std::array<const menu::EffectBank*,2> banks;
    menu::EffectGraphicsResources effect_graphics;
    sprite::Producer sprites;
    // Native physical capacities include held retired incarnations. They are
    // deliberately independent of zero source setup pool counts: logical Actor
    // limit40/list100 remains enforced by ActorOwner, not enlarged here.
    std::array<a::SlotCapacity,80> actor_capacities{};
    std::array<a::SlotCapacity,32> item_capacities{};
    std::array<a::SlotCapacity,2> cart_capacities{};
    scene::Storage storage{24*1024*1024};
    std::unique_ptr<scene::Services> services;
    std::unique_ptr<player::Root> root;
    std::unique_ptr<player::commands::Binding> commands;
    std::unique_ptr<player::Ui> ui;
    std::unique_ptr<player::BumpOwner> bump;
    focus::Detector detector;
    focus::RegionCopies copies{capture_layout};
    actor::DetectorBinding detection{copies,detector};
    std::unique_ptr<g::Frame> frame;
    const g::DepthReservation* depth=nullptr;
    std::vector<menu::RamRange> all_ranges;
    std::vector<sprite::AssetSpan> immutable;
    std::vector<sprite::RamRange> reservations;
    std::unique_ptr<player::Renderer> renderer;
    std::unique_ptr<environment_sound::Environment> environment;
    std::unique_ptr<world::World> world;
    std::unique_ptr<actor::Owner> actors;
    items::Globals globals;
    std::unique_ptr<items::Owner> items;
    std::unique_ptr<player::cart::Owner> cart;
    std::unique_ptr<actor::BehaviorBindings> bindings;
    std::unique_ptr<actor::InteractionServices> interactions;
    std::unique_ptr<player::camera_animation::NativeDriver> camera;
    IslePlayerCameraAnimationClip intro_camera{},outro_camera{};
    std::unique_ptr<actor::CollisionPass> collisions;
    std::unique_ptr<player::Photo> photo;
    std::unique_ptr<actor::PhotoScene> photo_scene;
    std::unique_ptr<player::Controller> controller;
    std::unique_ptr<world::Producer> worlds;
    std::unique_ptr<actor::Producer> actor_producer;
    std::unique_ptr<menu::model::PacketProducer> item_producer;
    std::unique_ptr<player::cart::Producer> carts;
    std::unique_ptr<course::ItemComposition> composition;
    std::unique_ptr<course::ModelProducer> models;
    std::array<scene::RenderOrderEntry,256> cart_order{};
    IsleGameObject* helper=nullptr;
    IsleGameObject* input_object=nullptr;
    a::InstanceHandle intro_instance;
    std::array<IsleGameObject**,4> helper_fields{{&actor_source.D_8034AB94_82A304,
        &actor_source.D_8034AB98_82A308,&actor_source.D_8034AF2C_82A69C,&source.D_8034B674_82ADE4}};
    std::array<IsleRainbowLifecycleAsset,4> assets{};
    std::array<IsleRainbowLifecycleEffect,10> effects{};

    Impl(const rom::Snapshot& rom,application::Manager& m,game::State& s,
        game_input::Service& i,game_audio::Session& au,host::GtlClock& cl,
        application::Photos& ph,world::ModuleState& module,bool cpu)
        :snapshot(rom),manager(m),state(s),input(i),audio(au),clock(cl),photos(ph),world_state(module),require_cpu_renderer(cpu),
         world_resources(rom, isle::course::rainbow_id),actor_resources(rom,isle::course::rainbow_id,{world_resources.setup().fog_rgb,world_resources.setup().fog_min,world_resources.setup().fog_max}),
         behavior_resources(rom,isle::course::rainbow_id),gate_resources(rom),gate_collision(gate_resources.models()),item_resources(rom,world_resources.setup().fog_rgb,world_resources.setup().fog_min,world_resources.setup().fog_max),
         cart_resources(rom,isle::course::rainbow_id,{world_resources.setup().fog_rgb,world_resources.setup().fog_min,world_resources.setup().fog_max}),
         ui_resources(rom),sounds(rom,isle::course::rainbow_id),effect_resources(rom),lighting(rom),programs(rom),transition_resources(rom),
         banks{&effect_resources.bank(menu::EffectGroup::common),&effect_resources.bank(menu::EffectGroup::rainbow)},
         effect_graphics(rom,banks,{0x700000,0x80000}),sprites(ui_resources.roots(),ui_resources.banks()) {
        constexpr std::array<std::array<std::uint32_t,2>,4> ranges{{{0xab5860,0xab5980},{0xab5980,0xab85e0},{0xadd310,0xadd5d0},{0xadd5d0,0xadec60}}};
        for (std::size_t n=0;n<assets.size();++n)
            assets[n]={static_cast<IsleRainbowLifecycleAssetId>(n),rom.canonical().subspan(ranges[n][0],ranges[n][1]-ranges[n][0]),false};
    }
    IsleGameResult fail(IsleGameResult result,const char* text) noexcept {
        if (result==ISLE_GAME_OK) return result;
        if (error==ISLE_GAME_OK) { error=result; std::snprintf(message.data(),message.size(),"%s",text); }
        status=LifecycleStatus::failed;
        return error;
    }
    bool valid_block(const world::Block* value) const noexcept {
        if (!world || !value) return false;
        for (const auto& candidate:world->blocks()) if (&candidate==value) return true;
        return false;
    }
    static bool add(void* user,world::Block& first,world::Block& second) noexcept {
        auto& self=*static_cast<Impl*>(user);
        return isle_rainbow_lifecycle_world_add(&self.source,source_block(&first),source_block(&second))==ISLE_GAME_OK;
    }
    static bool remove(void* user,world::Block& block) noexcept {
        auto& self=*static_cast<Impl*>(user);
        if (self.cancelling) return !self.actors || bool(self.actors->remove_block(block));
        return isle_rainbow_lifecycle_world_remove(&self.source,source_block(&block))==ISLE_GAME_OK;
    }
    static bool change(void* user,world::Block& first,world::Block& second) noexcept {
        auto& self=*static_cast<Impl*>(user);
        return isle_rainbow_lifecycle_world_change(&self.source,source_block(&first),source_block(&second))==ISLE_GAME_OK;
    }
    static bool shift_camera(void* user,world::Vec3 delta) noexcept {
        auto& self=*static_cast<Impl*>(user);
        return isle_player_root_invoke_bind_camera_next_block(&self.root->source(),delta.x,delta.y,delta.z)==ISLE_GAME_OK;
    }
    static bool shift_effects(void* user,world::Vec3 delta) noexcept {
        auto& self=*static_cast<Impl*>(user);
        self.actors->effects().translate({delta.x,delta.y,delta.z});
        return bool(self.actors->effects().status());
    }
    static bool exit_block(void* user,world::Block& block) noexcept {
        auto& self=*static_cast<Impl*>(user);
        return isle_rainbow_lifecycle_exit_block(&self.source,source_block(&block))==ISLE_GAME_OK;
    }
    static void helper_retiring(IsleGameObject* object,void* user) noexcept {
        auto& self=*static_cast<Impl*>(user);
        if (object!=self.helper) std::terminate();
    }
    static void helper_join(IsleGameObject* object,void* user) noexcept {
        auto& self=*static_cast<Impl*>(user);
        if (object!=self.helper) std::terminate();
        self.helper_joined=true;
    }
    static void input_join(IsleGameObject* object,void* user) noexcept {
        auto& self=*static_cast<Impl*>(user);
        if (object!=self.input_object) std::terminate();
        self.input_joined=true;
    }
    static bool prepare(void* user,std::span<std::uint32_t> ram,std::uint64_t epoch,
        const scene::render::Pass& pass,scene::render::Prepared& output) {
        auto& self=*static_cast<Impl*>(user);
        if (!self.models || self.copies.install(ram)!=focus::CaptureStatus::ok) return false;
        self.actors->render_bindings(); self.items->render_bindings();
        output=self.models->prepare(ram,epoch,pass);
        return true;
    }
    void cold();
    void producers();
    bool close_scene() noexcept;
    static const IsleRainbowActorServices actor_services;
    static IsleGameResult collide(void*,IsleGameVec3*,IsleGameVec3*,IsleGameVec3*,IsleGameVec3*,std::int32_t*) noexcept;
    ~Impl() {
        if (continuation && isle_continuation_destroy(continuation)!=ISLE_CONT_OK) std::terminate();
        if (domain && isle_continuation_domain_destroy(domain)!=ISLE_CONT_OK) std::terminate();
    }
};

void Lifecycle::Impl::cold() {
    native(storage.create(),"Rainbow native engine domain creation rejected");
    course_state=std::make_unique<CourseState>(actor_source,source);
    services=std::make_unique<scene::Services>(storage.game(),storage.arena(),
        scene::ResourceView{{source.scene_setup.objects,source.scene_setup.processes,source.scene_setup.sprites,
            source.scene_setup.cameras,source.scene_setup.matrices},ui_resources.resources()},input,audio,state);
    player::Root::Config config{6,isle_rainbow_lifecycle_end_event,&source,
        isle_rainbow_lifecycle_pause_event,&source,{this,exit_block,nullptr},
        nullptr,0,nullptr};
    root=std::make_unique<player::Root>(*services,ui_resources,config);
    source.player=&root->source();
    commands=std::make_unique<player::commands::Binding>(*services,root->source());
    ui=std::make_unique<player::Ui>(ui_resources,*services,root->source(),sprites,0);
    bump=std::make_unique<player::BumpOwner>(*services,root->source());
    auto made=g::Frame::create(snapshot,{require_cpu_renderer,{16000000,4*1024*1024},frame_layout});
    require(bool(made),made.message.data()); frame=std::move(made.frame);
    all_ranges={world_layout.payloads,world_layout.lists,world_layout.materials,
        actor_layout.payloads,actor_layout.lists,actor_layout.materials,
        item_layout.payloads,item_layout.lists,item_layout.materials,
        cart_layout.payloads,cart_layout.lists,cart_layout.materials,fx_commands,joined_lists,
        {lanes.address,lanes.size},{sprite_commands.address,sprite_commands.size},
        {message_commands.address,message_commands.size},
        {screen_transition::Resources::workspace.address,screen_transition::Resources::workspace.size}};
    for (const auto range:copies.reservations()) all_ranges.push_back(range);
    for (const auto& span:lighting.installation_spans()) all_ranges.push_back({physical(span.guest_address),span.bytes.size()});
    for (const auto range:frame->reservations()) if (range.address) all_ranges.push_back({range.address,range.size});
    for (const auto range:actor_layout.framebuffers) all_ranges.push_back(range);
    for (const auto range:effect_graphics.reservations()) all_ranges.push_back(range);
    immutable.assign(sprites.assets().begin(),sprites.assets().end());
    const auto append=[&](auto spans) {
        for (const auto& span:spans) immutable.push_back({physical(span.guest_address),
            {reinterpret_cast<const std::uint8_t*>(span.bytes.data()),span.bytes.size()}});
    };
    append(world_resources.installation_spans()); append(actor_resources.installation_spans());
    append(item_resources.installation_spans()); append(cart_resources.installation_spans());
    for (const auto span:effect_graphics.installation_spans()) immutable.push_back(span);
    for (const auto span:transition_resources.immutable()) immutable.push_back(span);
    for (const auto range:all_ranges) {
        bool external=range.offset==lanes.address && range.size==lanes.size;
        for (const auto core:frame->reservations()) external|=range.offset==core.address && range.size==core.size;
        for (const auto fb:actor_layout.framebuffers) external|=range.offset==fb.offset && range.size==fb.size;
        for (const auto asset:effect_graphics.reservations()) external|=range.offset==asset.offset && range.size==asset.size;
        if (!external) reservations.push_back({static_cast<std::uint32_t>(range.offset),static_cast<std::uint32_t>(range.size)});
    }
    player::Renderer::Config render;
    render.resources={1,&sprites,immutable,reservations,{0x3b5000,0x3da800},physical(lighting.pre_render_guest())};
    render.prepare=prepare; render.prepare_user=this; render.programs=&programs;
    render.effect_resources=&effect_graphics; render.sprite_capacity=1024;
    render.program_selection.disable_near_clipping=near_disabled;
    renderer=std::make_unique<player::Renderer>(*services,root->source(),lighting,render);
    environment=std::make_unique<environment_sound::Environment>(*services,storage.arena(),sounds);
    // Native per-call physical work envelope, matching Actor/Items below: not
    // a Source pool constant or guaranteed coverage of every speed/skip. Real
    // road seeks, terrain restarts and sky steps retain Runtime preflight.
    world=std::make_unique<world::World>(*services,world_resources,world_state,
        world::Callbacks{this,add,remove,change,shift_camera,shift_effects,nullptr},true,a::ExecutionCapacity{32768,4096});
    const auto capacity=[](const auto& models) {
        a::SlotCapacity maximum{};
        for (const auto& model:models) {
            const auto value=a::slot_capacity(model);
            maximum.nodes=std::max(maximum.nodes,value.nodes);
            maximum.material_attachments=std::max(maximum.material_attachments,value.material_attachments);
        }
        return maximum;
    };
    actor_capacities.fill(capacity(actor_resources.models()));
    item_capacities.fill(capacity(item_resources.models()));
    cart_capacities.fill(a::slot_capacity(cart_resources.animation_resources().models[0]));
    actors=std::make_unique<actor::Owner>(*services,actor_resources,actor::CourseContext(*course_state),*world,*environment,detector,
        root->source(),actor_capacities,banks,a::ExecutionCapacity{32768,4096});
    items=std::make_unique<items::Owner>(*services,item_resources,*world,*actors,globals,
        root->source(),item_capacities,a::ExecutionCapacity{32768,4096});
    cart=std::make_unique<player::cart::Owner>(*services,cart_resources,root->source(),cart_capacities,a::ExecutionCapacity{});
    bindings=std::make_unique<actor::BehaviorBindings>(behavior_resources,actor_callbacks(),
        actor_graphs(),actor_weighted_states(),actor_roots());
    native(root->bind({*world,*actors,*items,*ui,*bump,*renderer,photos.source(),nullptr}),
        "Rainbow cold canonical Root domain association rejected");
    domains_bound=true;
    native(isle_rainbow_actor_activate(&actor_source,services->game,&actor_services),
        "Rainbow canonical Actor Source activation rejected");
    interactions=std::make_unique<actor::InteractionServices>(*services,*actors,*items,*world,
        detector,*bindings,actor_resources);
    require(bool(actors->bind_interactions(*interactions)),"Rainbow interaction binding rejected");
    input_object=services->object(1u<<9,9,9);
    require(input_object &&
        services->set_lifecycle(input_object,{nullptr,input_join,this}),
        "Rainbow original Input object publication rejected");
    require(services->source_owner_register(input_object,{}),
        "Rainbow original Input identity retain rejected");
    input_registered=true;
    IsleGameProcess* process=nullptr;
    native(isle_game_add_source_process(services->game,input_object,
        isle_rainbow_lifecycle_input_update,ISLE_GAME_STACKFUL,0,&process),
        "Rainbow original Input process publication rejected");
}
void Lifecycle::Impl::producers() {
    native(renderer->bind_ui(*ui,message_commands),renderer->message());
    const auto others=[&](std::array<menu::RamRange,3> own,bool framebuffer) {
        std::vector<menu::RamRange> result;
        for (const auto range:all_ranges) {
            bool skip=false;
            for (const auto mine:own) skip|=range.offset==mine.offset && range.size==mine.size;
            if (framebuffer) for (const auto fb:actor_layout.framebuffers) skip|=range.offset==fb.offset && range.size==fb.size;
            if (!skip) result.push_back(range);
        }
        return result;
    };
    const auto wo=others({world_layout.payloads,world_layout.lists,world_layout.materials},false);
    const auto ao=others({actor_layout.payloads,actor_layout.lists,actor_layout.materials},true);
    const auto io=others({item_layout.payloads,item_layout.lists,item_layout.materials},true);
    const auto co=others({cart_layout.payloads,cart_layout.lists,cart_layout.materials},true);
    worlds=std::make_unique<world::Producer>(services->game,source.player->camera,services->camera_basis(),world_resources,world->render_view(),world_layout,wo);
    actor_producer=std::make_unique<actor::Producer>(services->game,source.player->camera,services->camera_basis(),
        actor_resources,actor_capacities,actors->render_bindings(),actor_layout,ao,&detection);
    item_producer=std::make_unique<menu::model::PacketProducer>(services->game,source.player->camera,services->camera_basis(),
        menu::model::PacketResources{item_resources.models(),item_resources.materials(),item_resources.installation_spans()},
        item_capacities,items->render_bindings(),item_layout,io);
    carts=std::make_unique<player::cart::Producer>(services->game,source.player->camera,services->camera_basis(),
        cart_resources,cart_capacities,cart->render_bindings(cart_order),cart_layout,co);
    composition=std::make_unique<course::ItemComposition>(course::ItemComposition{
        *services,*item_producer,256,carts.get(),photo_scene->prefix_object(),true});
    models=std::make_unique<course::ModelProducer>(*worlds,*actor_producer,joined_lists,composition.get());
    native(renderer->bind_cart(*cart,cart_order),renderer->message());
    native(renderer->bind_transition(transition_resources,services->screen_transition()),renderer->message());
    require(renderer->reserve(*frame),frame->message());
}
Lifecycle::Lifecycle(std::unique_ptr<Impl> impl) noexcept:impl_(std::move(impl)) {}
LifecycleCreateResult Lifecycle::create(const rom::Snapshot& snapshot,application::Manager& manager,
    game::State& state,game_input::Service& input,game_audio::Session& audio,host::GtlClock& clock,
    application::Photos& photos,world::ModuleState& module,std::int32_t scene,bool require_cpu_renderer) noexcept {
    LifecycleCreateResult result;
    try {
        auto impl=std::make_unique<Impl>(snapshot,manager,state,input,audio,clock,photos,module,require_cpu_renderer);
        require(isle_continuation_domain_create(&impl->domain)==ISLE_CONT_OK,"Rainbow independent domain creation failed");
        const auto rom=snapshot.canonical();
        native(isle_rainbow_actor_prepare(&impl->actor_source,impl.get(),
            reinterpret_cast<const std::uint8_t*>(rom.data()),rom.size()),
            "Rainbow independent Actor Source preparation rejected");
        native(isle_rainbow_lifecycle_prepare(&impl->source,impl.get(),impl->domain,&impl->actor_source,
            reinterpret_cast<const std::uint8_t*>(rom.data()),rom.size(),scene),
            "Rainbow original Source data preparation rejected");
        require(isle_continuation_create(impl->domain,isle_rainbow_lifecycle_run,&impl->source,
            &impl->continuation)==ISLE_CONT_OK,"Rainbow independent continuation creation failed");
        result.lifecycle=std::unique_ptr<Lifecycle>(new Lifecycle(std::move(impl)));
    } catch (const std::exception& exception) {
        result.error=ISLE_GAME_INVALID_STATE;
        std::snprintf(result.message.data(),result.message.size(),"%s",exception.what());
    }
    return result;
}
Lifecycle::~Lifecycle() { if (retire()!=ISLE_GAME_OK) std::terminate(); }
IsleGameResult Lifecycle::resume() noexcept {
    auto& self=*impl_;
    if (self.error!=ISLE_GAME_OK) return self.error;
    if (self.cancelling) return ISLE_GAME_STOPPED;
    if (!self.continuation || self.initializing || (self.services && isle_game_require_idle(self.services->game)!=ISLE_GAME_OK))
        return ISLE_GAME_INVALID_STATE;
    if (isle_continuation_resume(self.continuation)!=ISLE_CONT_OK)
        return self.fail(self.source.status==ISLE_GAME_OK?ISLE_GAME_INVALID_STATE:self.source.status,
            isle_continuation_error(self.domain));
    if (self.source.completed) self.status=LifecycleStatus::completed;
    else if (self.requested && !self.initialized) self.status=LifecycleStatus::setup_pending;
    return ISLE_GAME_OK;
}
const IsleRainbowLifecycleSceneSetup* Lifecycle::pending_setup() const noexcept {
    return impl_->status==LifecycleStatus::setup_pending && !impl_->cancelling &&
        impl_->requested && !impl_->initialized && !impl_->scene_retired ? &impl_->source.scene_setup:nullptr;
}
IsleGameResult Lifecycle::initialize_scene() noexcept {
    auto& self=*impl_;
    if (self.status!=LifecycleStatus::setup_pending || self.initializing || self.services) return ISLE_GAME_INVALID_STATE;
    self.initializing=true;
    try {
        self.cold();
        native(isle_rainbow_lifecycle_invoke_init(&self.source),"Rainbow original init Source native call rejected");
        self.producers();
        self.initialized=true; self.initializing=false; self.status=LifecycleStatus::running;
        return ISLE_GAME_OK;
    } catch (const std::exception& exception) {
        self.initializing=false;
        return self.fail(self.source.status==ISLE_GAME_OK?ISLE_GAME_INVALID_STATE:self.source.status,exception.what());
    }
}
IsleGameResult Lifecycle::update(std::uint32_t raw_game_time) noexcept {
    auto& self=*impl_;
    if (self.status!=LifecycleStatus::running || !self.frame->quiescent() || self.exit_requested)
        return ISLE_GAME_INVALID_STATE;
    self.services->raw_game_time=raw_game_time;
    auto result=self.root->publish_input();
    if (result==ISLE_GAME_OK) result=isle_game_tick(self.services->game);
    if (self.cart && self.intro_instance && self.source.D_8034B674_82ADE4) {
        const auto state=self.cart->membership(self.intro_instance);
        if (state.retiring && !state.joined) {
            const auto retired=self.cart->retire(self.source.D_8034B674_82ADE4);
            if (!retired && result==ISLE_GAME_OK) result=ISLE_GAME_INVALID_STATE;
        }
    }
    if (result==ISLE_GAME_OK && self.services->failed()) result=ISLE_GAME_INVALID_STATE;
    if (result==ISLE_GAME_OK) result=self.source.status;
    if (result==ISLE_GAME_OK) result=self.root->source().status;
    if (result!=ISLE_GAME_OK) return self.fail(result,self.services->error());
    if (self.exit_requested) self.status=LifecycleStatus::retiring;
    return ISLE_GAME_OK;
}
bool Lifecycle::draw(g::FrameContext context) noexcept {
    auto& self=*impl_;
    if (self.status!=LifecycleStatus::running || self.exit_requested || !self.renderer) return false;
    // Presentation/GTL release is not Source Photo retirement. CopyInfo and
    // shutter updates consume the last completed capture until the next draw.
    if (self.controller && self.controller->has_pending(*self.frame)) {
        const auto retired=self.renderer->release_completed();
        if (retired!=ISLE_GAME_OK) {
            self.fail(retired,self.renderer->message()); return false;
        }
    }
    if (!self.renderer->begin(*self.frame,context)) {
        self.fail(self.renderer->status(),self.renderer->message()); return false;
    }
    return true;
}
g::Poll Lifecycle::poll() noexcept {
    auto& self=*impl_;
    if (!self.renderer) return g::Poll::idle;
    const auto result=self.renderer->poll();
    if (result==g::Poll::failed) self.fail(ISLE_GAME_INVALID_STATE,self.renderer->message());
    return result;
}
g::Completed Lifecycle::completed_frame() const noexcept { return impl_->frame?impl_->frame->completed():g::Completed{}; }
bool Lifecycle::graphics_busy() const noexcept { return impl_->frame && impl_->frame->busy(); }
IsleGameResult Lifecycle::release_completed() noexcept {
    auto& self=*impl_;
    if (!self.frame || !self.frame->quiescent() || !self.frame->completed()) return ISLE_GAME_INVALID_STATE;
    if (self.controller && self.controller->has_pending(*self.frame)) return self.renderer->release_completed();
    return ISLE_GAME_OK;
}
LifecycleStatus Lifecycle::status() const noexcept { return impl_->status; }
IsleGameResult Lifecycle::error() const noexcept { return impl_->error; }
const char* Lifecycle::message() const noexcept { return impl_->message.data(); }
const IsleRainbowLifecycleSource& Lifecycle::source() const noexcept { return impl_->source; }
const IsleRainbowActorSource& Lifecycle::actors_source() const noexcept { return impl_->actor_source; }
std::optional<std::int32_t> Lifecycle::result() const noexcept {
    if (!impl_->source.completed || impl_->error!=ISLE_GAME_OK || !impl_->scene_retired ||
        impl_->cancelling || !impl_->join_receipt || impl_->actor_source.status!=ISLE_GAME_OK) return {};
    return impl_->source.output_scene;
}

bool Lifecycle::Impl::close_scene() noexcept {
    if (scene_retired) return true;
    if (!services) {
        if (storage.game() && storage.destroy()!=ISLE_GAME_OK) return false;
        scene_retired=true;
        return true;
    }
    if (isle_game_require_idle(services->game)!=ISLE_GAME_OK ||
        (frame && !frame->quiescent())) return false;
    if (controller && frame && controller->has_pending(*frame) &&
        renderer->release_completed()!=ISLE_GAME_OK) return false;
    if (photo && !photo->quiescent()) return false;
    if (photo_scene) { photo_scene->retire(); photo_scene.reset(); }
    if (controller && controller->queued_requests()) return false;
    models.reset(); composition.reset(); carts.reset(); item_producer.reset();
    actor_producer.reset(); worlds.reset(); collisions.reset();
    if (environment) environment->cleanup();
    services->stop_sound_ramps();
    if (input_object) {
        if (!input_joined) services->erase(input_object);
        if (!input_joined) return false;
        if (input_registered && !services->source_owner_release(input_object)) return false;
        input_registered=false;
        input_object=nullptr;
    }
    // End real FX attachments before native model retirement. Their node credits
    // are separate from the canonical Source field holds below.
    if (actors) actors->effects().reset();
    if (root && root->retire_objects()!=ISLE_GAME_OK) return false;
    if (world && !world->retire()) return false;
    if (items) {
        const auto result=items->retire();
        if (!result && result.error!=items::OwnerError::lifetime &&
            !(cancelling && result.error==items::OwnerError::services)) return false;
    }
    if (cart) {
        const auto result=cart->retire();
        if (!result && result.error!=player::cart::OwnerError::lifetime &&
            !(cancelling && result.error==player::cart::OwnerError::services)) return false;
    }
    if (cutscene_cart) {
        if (!services->source_owner_joined(cutscene_cart.object())) return false;
        cutscene_cart={};
        cutscene_callback=nullptr;
    }
    if (actors) {
        const auto result=actors->reset();
        if (!result && result.error!=actor::OwnerError::lifetime &&
            !(cancelling && result.error==actor::OwnerError::services)) return false;
    }
    // All consumers have stopped. Joined GObjs can still be named, but their
    // native ModelRefs must never survive release of these exact field credits.
    if (helper_registered) {
        for (auto** field:helper_fields) {
            if (*field && !services->source_owner_joined(*field)) return false;
            if (!services->source_reference_assign(helper,field,nullptr)) return false;
        }
    }
    actor_source.mew_model={};
    actor_source.shield_model={};
    actor_source.gate_model={};
    actor_source.D_8034AF20_82A690.native=nullptr;
    if (actors) {
        const auto result=actors->reset();
        if (!result && !(cancelling && result.error==actor::OwnerError::services)) return false;
    }
    if (renderer) {
        if (cancelling) {
            if (renderer->abort_main()!=ISLE_GAME_OK) return false;
        } else if (source.player && source.player->camera &&
            (renderer->remove_main()!=ISLE_GAME_OK ||
             renderer->service_main_removal()!=ISLE_GAME_OK)) return false;
    }
    if (source.player && (source.player->D_80382D10_523120 ||
        source.player->D_80382D14_523124)) return false;
    if (root && root->collect_joined()!=ISLE_GAME_OK) return false;
    if (items && !items->collect_joined()) return false;
    if (cart && !cart->collect_joined()) return false;
    // Item native processes and incoming semantic holds are now genuinely
    // closed; only now may the paired callback stop borrowing Actor Source.
    if (items && collision_bound) {
        if (!items->set_extra_collision(services->game,nullptr,nullptr)) return false;
        collision_bound=false;
    }
    if (bump && bump->retire()!=ISLE_GAME_OK) return false;
    if (ui && ui->retire()!=ISLE_GAME_OK) return false;
    if (actors) {
        if (actors->particle_object()) services->erase(actors->particle_object());
        if (actors->effect_object()) services->erase(actors->effect_object());
    }
    if (renderer) {
        if (renderer->detach()!=ISLE_GAME_OK) return false;
        renderer.reset();
    }
    if (actors && !actors->collect_joined()) return false;
    if (camera) { camera->retire(); camera.reset(); }
    if (frame) { if (!frame->retire()) return false; frame.reset(); depth=nullptr; }
    if (helper) {
        if (!helper_observed) {
            if (isle_game_delete_object(services->game,helper)!=ISLE_GAME_OK) return false;
        } else {
            if (!helper_joined) services->erase(helper);
            if (!helper_joined) return false;
        }
        if (helper_registered &&
            isle_rainbow_lifecycle_native_release_cart_reference(&source)!=ISLE_GAME_OK) return false;
        helper=nullptr;
    }
    if (commands) { if (commands->detach()!=ISLE_GAME_OK) return false; commands.reset(); }
    if (!services->retire_objects()) return false;
    // Deactivate while the genuine, idle Game and canonical owner association
    // still exist. The C phase gate also observes zero actual native processes.
    if (actor_source.phase==ISLE_RAINBOW_ACTOR_ACTIVE &&
        isle_rainbow_actor_deactivate(&actor_source)!=ISLE_GAME_OK) return false;
    controller.reset(); photo.reset();
    items.reset(); cart.reset(); bump.reset(); ui.reset(); actors.reset();
    interactions.reset(); bindings.reset(); world.reset(); environment.reset();
    if (root) {
        const auto result=root->deactivate();
        if (root->source().native_root ||
            (result!=ISLE_GAME_OK && result!=isle_game_status(services->game))) return false;
    }
    root.reset();
    source.player=nullptr;
    source.screen.depth_buffer=nullptr;
    course_state.reset();
    services.reset();
    if (storage.destroy()!=ISLE_GAME_OK) return false;
    // This independent receipt is published only at the end of the actual
    // process/semantic/model/Photo/GPU/input/audio-context teardown above.
    join_receipt=actor_source.phase==ISLE_RAINBOW_ACTOR_DETACHED;
    scene_retired=true;
    return true;
}
bool Lifecycle::retire_scene() noexcept {
    auto& self=*impl_;
    if (self.scene_retired) return true;
    if (!self.exit_requested && self.error==ISLE_GAME_OK && !self.cancelling)
        return false;
    if (self.error!=ISLE_GAME_OK) self.cancelling=true;
    if (self.error==ISLE_GAME_OK && !self.cancelling) self.status=LifecycleStatus::retiring;
    if (!self.close_scene()) return false;
    if (!self.cancelling && self.source.scene_requested && !self.source.scene_completion_ready) {
        const auto result=isle_rainbow_lifecycle_complete_scene(&self.source,ISLE_GAME_OK);
        if (result!=ISLE_GAME_OK) { self.fail(result,"Rainbow Source completion rejected real scene retirement"); return false; }
    }
    return true;
}
IsleGameResult Lifecycle::cancel() noexcept {
    auto& self=*impl_;
    self.cancelling=true;
    if (self.continuation && isle_continuation_cancel(self.continuation)!=ISLE_CONT_OK)
        return self.fail(ISLE_GAME_INVALID_STATE,"Rainbow Source cancellation did not join");
    if (!self.close_scene()) return ISLE_GAME_INVALID_STATE;
    if (self.error==ISLE_GAME_OK) self.status=LifecycleStatus::cancelled;
    return ISLE_GAME_OK;
}
IsleGameResult Lifecycle::retire() noexcept {
    auto& self=*impl_;
    if (self.status==LifecycleStatus::retired) return ISLE_GAME_OK;
    if (!self.scene_retired || !self.source.completed) {
        const auto result=cancel();
        if (result!=ISLE_GAME_OK) return result;
    }
    if (self.continuation) {
        if (isle_continuation_destroy(self.continuation)!=ISLE_CONT_OK) return ISLE_GAME_INVALID_STATE;
        self.continuation=nullptr;
    }
    if (self.domain) {
        if (isle_continuation_domain_destroy(self.domain)!=ISLE_CONT_OK) return ISLE_GAME_INVALID_STATE;
        self.domain=nullptr; self.source.domain=nullptr;
    }
    self.status=LifecycleStatus::retired;
    return ISLE_GAME_OK;
}
struct LifecycleAccess {
    using Impl=Lifecycle::Impl;
    template<class Function> static IsleGameResult invoke(IsleRainbowLifecycleSource* source,Function function,
        std::source_location call=std::source_location::current()) noexcept {
        if (!source || !source->owner) return ISLE_GAME_INVALID_ARGUMENT;
        auto& self=*static_cast<Impl*>(source->owner);
        if (&self.source!=source) return ISLE_GAME_INVALID_ARGUMENT;
        try {
            const auto result=function(self);
            return result==ISLE_GAME_OK?result:self.fail(result,call.function_name());
        }
        catch (const std::exception& exception) { return self.fail(ISLE_GAME_INVALID_STATE,exception.what()); }
        catch (...) { return self.fail(ISLE_GAME_INVALID_STATE,"Rainbow native service threw"); }
    }
    static IsleGameResult setup(Impl& self) noexcept {
        return self.initializing && self.services && isle_game_require_idle(self.services->game)==ISLE_GAME_OK ?
            ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    }
    template<class Function> static IsleGameResult actor(IsleRainbowActorSource* source,
        Function function) noexcept {
        if (!source || !source->owner) return ISLE_GAME_INVALID_ARGUMENT;
        auto& self=*static_cast<Impl*>(source->owner);
        if (&self.actor_source!=source || source->phase!=ISLE_RAINBOW_ACTOR_ACTIVE ||
            !self.services || source->nativeGame!=self.services->game ||
            source->services!=&Impl::actor_services || self.cancelling)
            return ISLE_GAME_INVALID_STATE;
        return invoke(&self.source,[&](Impl& owner) { return function(owner); });
    }
    static IsleGameResult audio(Impl& self,audio::Error result) noexcept {
        return self.services && self.services->check(result,"Rainbow original audio leaf rejected") ?
            ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE;
    }
    static bool definitions(Impl& self,uint32_t table,
        const IsleRainbowLifecyclePokemonDef* rows,size_t count) {
        if (!self.actors || table!=0x8034ab34u || rows!=self.source.pokemon_defs || count!=5)
            return false;
        const auto& terminal=rows[count];
        if (terminal.id || terminal.initialize_identity || terminal.update_identity || terminal.remove_identity)
            return false;
        for (size_t n=0;n<count;++n) {
            bool found=false;
            for (const auto& definition:self.actor_resources.definitions())
                if (definition.wire.guest_address==table+n*16) {
                    found=definition.actor_id==rows[n].id &&
                        definition.factory_identity==rows[n].initialize_identity &&
                        definition.change_block_identity==rows[n].update_identity &&
                        definition.remove_identity==rows[n].remove_identity &&
                        self.actors->admitted_factory(rows[n].id,rows[n].initialize_identity);
                    break;
                }
            if (!found) return false;
        }
        return true;
    }
};
} // namespace isle::rainbow

namespace {
using Access=isle::rainbow::LifecycleAccess;
using Impl=Access::Impl;
using isle::rainbow::block_cast;
using isle::rainbow::source_block;
}
extern "C" {
IsleGameResult isle_rainbow_lifecycle_native_intervals(IsleRainbowLifecycleSource* s,uint16_t update,uint16_t draw) {
    return Access::invoke(s,[&](Impl& self) {
        return self.clock.set_intervals(update,draw)==isle::host::ClockError::none?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_near_clipping(IsleRainbowLifecycleSource* s,int32_t disabled) {
    return Access::invoke(s,[&](Impl& self) {
        if (disabled!=0 && disabled!=1) return ISLE_GAME_INVALID_ARGUMENT;
        self.near_disabled=disabled!=0;
        return self.renderer?self.renderer->set_near_clipping_disabled(self.near_disabled):ISLE_GAME_OK;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_request_scene(IsleRainbowLifecycleSource* s,const IsleRainbowLifecycleSceneSetup* setup) {
    return Access::invoke(s,[&](Impl& self) {
        if (setup!=&s->scene_setup || self.requested || self.services || setup->contexts!=2 ||
            setup->heap_base!=0x80139c50u || setup->heap_size!=0x803466c0u-0x80139c50u ||
            setup->display_list_bytes[0]!=0x5000 || setup->display_list_bytes[1]!=0x1000 ||
            setup->display_list_bytes[2]!=0x400 || setup->display_list_bytes[3]!=0 ||
            setup->graphics_heap_bytes!=0xc800 || setup->rdp_output_bytes!=0x4000 ||
            setup->thread_stack_bytes!=768 || self.actor_source.phase!=ISLE_RAINBOW_ACTOR_PREPARED) return ISLE_GAME_INVALID_STATE;
        self.requested=true;
        return ISLE_GAME_OK;
    });
}
IsleGameResult isle_rainbow_lifecycle_owner_transition_end(IsleRainbowLifecycleSource* s,int32_t reason) {
    return Access::invoke(s,[&](Impl& self) { return self.manager.transition_end(reason); });
}
IsleGameResult isle_rainbow_lifecycle_native_exit_scene(IsleRainbowLifecycleSource* s) {
    return Access::invoke(s,[&](Impl& self) {
        if (!self.services) return ISLE_GAME_INVALID_STATE;
        self.exit_requested=true; self.clock.request_stop();
        return ISLE_GAME_OK;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_destroy_world(IsleRainbowLifecycleSource* s) {
    return Access::invoke(s,[&](Impl& self) {
        return self.world && self.world_published && self.world->request_retire()?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_remove_main_camera(IsleRainbowLifecycleSource* s) {
    return Access::invoke(s,[&](Impl& self) { return self.renderer?self.renderer->remove_main():ISLE_GAME_INVALID_STATE; });
}
IsleGameResult isle_rainbow_lifecycle_native_environment_cleanup(IsleRainbowLifecycleSource* s) {
    return Access::invoke(s,[&](Impl& self) {
        if (!self.environment) return ISLE_GAME_INVALID_STATE;
        self.environment->cleanup(); return ISLE_GAME_OK;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_detector_disable(IsleRainbowLifecycleSource* s) {
    return Access::invoke(s,[&](Impl& self) {
        return self.photo?isle_player_photo_PokemonDetector_Disable(&self.photos.source()):ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_depth_buffer(IsleRainbowLifecycleSource* s,uint32_t bytes,uint32_t alignment,void** out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    *out=nullptr;
    return Access::invoke(s,[&](Impl& self) {
        if (Access::setup(self)!=ISLE_GAME_OK || !self.frame || self.depth) return ISLE_GAME_INVALID_STATE;
        self.depth=self.frame->reserve_source_depth(bytes,alignment);
        if (!self.depth) return ISLE_GAME_INVALID_STATE;
        *out=const_cast<isle::scene::graphics::DepthReservation*>(self.depth); return ISLE_GAME_OK;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_screen(IsleRainbowLifecycleSource* s,const IsleRainbowLifecycleScreen* screen) {
    return Access::invoke(s,[&](Impl& self) {
        if (Access::setup(self)!=ISLE_GAME_OK || screen!=&s->screen || !self.depth ||
            screen->depth_buffer!=self.depth || self.screen_applied) return ISLE_GAME_INVALID_STATE;
        isle::scene::graphics::SourceScreen config{screen->width,screen->height,screen->flags,
            {screen->framebuffer_identity[0],screen->framebuffer_identity[1],screen->framebuffer_identity[2]}};
        if (!self.frame->configure_source_screen(config,self.depth)) return ISLE_GAME_INVALID_STATE;
        self.screen_applied=true; return ISLE_GAME_OK;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_fx_bank_count(IsleRainbowLifecycleSource* s,size_t* out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    return Access::invoke(s,[&](Impl& self) {
        if (Access::setup(self)!=ISLE_GAME_OK || !self.actors) return ISLE_GAME_INVALID_STATE;
        *out=isle::menu::EffectRuntime::bank_count; return ISLE_GAME_OK;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_fx_reset_bank(IsleRainbowLifecycleSource* s,size_t bank) {
    return Access::invoke(s,[&](Impl& self) {
        return Access::setup(self)==ISLE_GAME_OK && self.actors &&
            self.actors->effects().reset_bank(bank)?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_reset_photo_roll(IsleRainbowLifecycleSource* s) {
    return Access::invoke(s,[&](Impl& self) {
        return Access::setup(self)==ISLE_GAME_OK?self.photos.reset():ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_create_helper(IsleRainbowLifecycleSource* s,int32_t id,IsleGameSourceCallback update,int32_t link,uint32_t priority) {
    return Access::invoke(s,[&](Impl& self) {
        if (Access::setup(self)!=ISLE_GAME_OK || self.helper || id!=0 || link!=0 || priority!=0x80000000u ||
            update!=isle_rainbow_lifecycle_func_80346B0C_82627C) return ISLE_GAME_INVALID_STATE;
        self.helper=self.services->object(0,0,0x80000000u);
        if (!self.helper || !self.services->set_lifecycle(self.helper,{Impl::helper_retiring,Impl::helper_join,&self}))
            return ISLE_GAME_INVALID_STATE;
        self.helper_observed=true;
        if (!self.services->source_owner_register(self.helper,self.helper_fields)) return ISLE_GAME_INVALID_STATE;
        self.helper_registered=true;
        return isle_game_object_set_source_update(self.services->game,self.helper,update);
    });
}
IsleGameResult isle_rainbow_lifecycle_native_pokemons_init(IsleRainbowLifecycleSource* s) {
    return Access::invoke(s,[&](Impl& self) {
        if (Access::setup(self)!=ISLE_GAME_OK || !self.helper || !self.actors || self.collisions)
            return ISLE_GAME_INVALID_STATE;
        self.collisions=std::make_unique<isle::actor::CollisionPass>(*self.services,*self.actors,self.helper);
        return ISLE_GAME_OK;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_create_world(IsleRainbowLifecycleSource* s,uint32_t identity,
    int32_t sky,int32_t first,int32_t last,int32_t link,int32_t render,IsleRainbowLifecycleWorldPair add,
    IsleRainbowLifecycleWorldSingle remove,IsleRainbowLifecycleWorldPair change,void* user) {
    return Access::invoke(s,[&](Impl& self) {
        if (Access::setup(self)!=ISLE_GAME_OK || self.world_published ||
            identity!=0x800f5da0u || sky!=99 || first!=100 || last!=126 || link!=9 || render!=3 ||
            add!=isle_rainbow_lifecycle_world_add || remove!=isle_rainbow_lifecycle_world_remove ||
            change!=isle_rainbow_lifecycle_world_change || user!=s) return ISLE_GAME_INVALID_ARGUMENT;
        self.world->publish(); self.world_published=true; return ISLE_GAME_OK;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_sky_follow(IsleRainbowLifecycleSource* s) {
    return Access::invoke(s,[&](Impl& self) {
        if (Access::setup(self)!=ISLE_GAME_OK || !self.world_published) return ISLE_GAME_INVALID_STATE;
        self.sky_follow=true;
        if (s->player->camera) self.world->follow_camera(s->player->camera);
        return ISLE_GAME_OK;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_pokemon_registry(IsleRainbowLifecycleSource* s,int32_t minimum,int32_t maximum,int32_t link,int32_t arg3) {
    return Access::invoke(s,[&](Impl& self) {
        return Access::setup(self)==ISLE_GAME_OK && self.actors &&
            self.actors->configure_registry(minimum,maximum,link,arg3)?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_background(IsleRainbowLifecycleSource* s,uint8_t* r,uint8_t* g,uint8_t* b) {
    if (!r || !g || !b) return ISLE_GAME_INVALID_ARGUMENT;
    return Access::invoke(s,[&](Impl& self) {
        if (!self.world_published) return ISLE_GAME_INVALID_STATE;
        const auto rgb=self.world_resources.setup().background_rgb;
        *r=rgb[0]; *g=rgb[1]; *b=rgb[2]; return ISLE_GAME_OK;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_create_cameras(IsleRainbowLifecycleSource* s,uint32_t color) {
    return Access::invoke(s,[&](Impl& self) {
        if (Access::setup(self)!=ISLE_GAME_OK || !self.renderer || self.camera) return ISLE_GAME_INVALID_STATE;
        IsleGameObject* ui=nullptr;
        const auto result=self.renderer->create_main(static_cast<std::int32_t>(color),&ui);
        if (result!=ISLE_GAME_OK) return result;
        auto made=isle::player::camera_animation::NativeDriver::create(self.services->game,s->player->camera,
            self.cart_resources.animation_resources(),{});
        if (!made || !made.driver) return ISLE_GAME_INVALID_STATE;
        self.camera=std::move(made.driver);
        if (!self.camera->clip(self.cart_resources.intro().camera_animation,self.intro_camera) ||
            !self.camera->clip(self.cart_resources.outro().camera_animation,self.outro_camera))
            return ISLE_GAME_INVALID_STATE;
        const auto bound=self.root->bind_camera_animation(*self.camera);
        if (bound!=ISLE_GAME_OK) return bound;
        if (self.sky_follow) self.world->follow_camera(s->player->camera);
        return ISLE_GAME_OK;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_load(IsleRainbowLifecycleSource* s,IsleRainbowLifecycleAssetId id,
    uint32_t first,uint32_t end,const IsleRainbowLifecycleAsset** out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    *out=nullptr;
    return Access::invoke(s,[&](Impl& self) {
        if (Access::setup(self)!=ISLE_GAME_OK || unsigned(id)>=self.assets.size() || first>=end ||
            end>self.snapshot.canonical().size()) return ISLE_GAME_INVALID_ARGUMENT;
        auto& asset=self.assets[unsigned(id)];
        if (asset.bytes.data()!=self.snapshot.canonical().data()+first || asset.bytes.size()!=end-first)
            return ISLE_GAME_INVALID_ARGUMENT;
        asset.loaded=true; *out=&asset; return ISLE_GAME_OK;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_ui_bind(IsleRainbowLifecycleSource* s,const IsleRainbowLifecycleUiCallbacks* callbacks) {
    return Access::invoke(s,[&](Impl& self) {
        if (Access::setup(self)!=ISLE_GAME_OK || !callbacks || callbacks->user!=s ||
            callbacks->exit_block!=isle_rainbow_lifecycle_exit_block || callbacks->movement ||
            callbacks->collide_item || callbacks->update_items || callbacks->update_items_kind ||
            !self.domains_bound || self.ui_bound || !self.camera || !s->player ||
            !self.camera->matches(self.services->game,s->player->camera) ||
            self.root->bound_actors()!=self.actors.get()) return ISLE_GAME_INVALID_ARGUMENT;
        self.ui_bound=true;
        return ISLE_GAME_OK;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_ui_initialized(IsleRainbowLifecycleSource* s,IsleGameObject* object) {
    return Access::invoke(s,[&](Impl& self) {
        return Access::setup(self)==ISLE_GAME_OK?self.root->complete_initialization(object):ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_environment_init(IsleRainbowLifecycleSource* s,
    const IsleRainbowLifecycleEnvironment* values,size_t count) {
    return Access::invoke(s,[&](Impl& self) {
        const auto descriptors=self.sounds.descriptors();
        if (Access::setup(self)!=ISLE_GAME_OK || values!=s->environment || count!=descriptors.size())
            return ISLE_GAME_INVALID_ARGUMENT;
        for (std::size_t i=0;i<count;++i)
            if (values[i].sound!=descriptors[i].sound || values[i].pitch_modifier!=descriptors[i].pitch_modifier ||
                values[i].hearing_range!=descriptors[i].hearing_range) return ISLE_GAME_INVALID_STATE;
        return self.environment->initialize(s->player->model,s->player->camera)?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_fx_setup_bank(IsleRainbowLifecycleSource* s,int32_t bank,
    const IsleRainbowLifecycleAsset* script,const IsleRainbowLifecycleAsset* sprites) {
    return Access::invoke(s,[&](Impl& self) {
        if (Access::setup(self)!=ISLE_GAME_OK || !script || !sprites) return ISLE_GAME_INVALID_ARGUMENT;
        const isle::menu::EffectBank* selected=nullptr;
        if (bank==3 && script==&self.assets[0] && sprites==&self.assets[1]) selected=self.banks[0];
        if (bank==0 && script==&self.assets[2] && sprites==&self.assets[3]) selected=self.banks[1];
        return selected && script->loaded && sprites->loaded && self.actors->effects().install_bank(*selected)?
            ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_fx_camera(IsleRainbowLifecycleSource* s,int32_t lists,int32_t particles,
    IsleGameCamera* camera,IsleGameObject** out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    *out=nullptr;
    return Access::invoke(s,[&](Impl& self) {
        if (Access::setup(self)!=ISLE_GAME_OK || lists!=4 || particles!=100 || camera!=s->player->camera)
            return ISLE_GAME_INVALID_STATE;
        const auto publication=self.actors->publish_particle_camera();
        if (!publication) return self.fail(ISLE_GAME_INVALID_STATE,publication.detail);
        auto* object=self.actors->particle_object();
        const auto result=self.renderer->adopt_particle(*self.actors,object);
        if (result!=ISLE_GAME_OK) return self.fail(result,self.renderer->message());
        *out=object;
        return ISLE_GAME_OK;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_fx_effects(IsleRainbowLifecycleSource* s,int32_t count,IsleGameObject** out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    *out=nullptr;
    return Access::invoke(s,[&](Impl& self) {
        if (Access::setup(self)!=ISLE_GAME_OK || count!=10) return ISLE_GAME_INVALID_STATE;
        const auto publication=self.actors->publish_effect_emitters();
        if (!publication) return self.fail(ISLE_GAME_INVALID_STATE,publication.detail);
        auto* object=self.actors->effect_object();
        const auto result=self.renderer->adopt_effect(*self.actors,object);
        if (result!=ISLE_GAME_OK) return self.fail(result,self.renderer->message());
        *out=object;
        return ISLE_GAME_OK;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_fx_dither(IsleRainbowLifecycleSource* s,uint32_t color,uint32_t alpha) {
    return Access::invoke(s,[&](Impl& self) {
        if (!self.actors || color>255 || alpha>255) return ISLE_GAME_INVALID_ARGUMENT;
        self.actors->effects().set_dither(static_cast<std::uint8_t>(color),static_cast<std::uint8_t>(alpha));
        return ISLE_GAME_OK;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_detector_create(IsleRainbowLifecycleSource* s) {
    return Access::invoke(s,[&](Impl& self) {
        if (Access::setup(self)!=ISLE_GAME_OK || self.photo || !self.collisions) return ISLE_GAME_INVALID_STATE;
        self.photo=std::make_unique<isle::player::Photo>(self.photos.source(),*s->player,*self.actors,*self.items,
            self.detection,self.photos.resources(),*self.world,6);
        self.photo_scene=std::make_unique<isle::actor::PhotoScene>(*self.services,*self.actors,self.helper,
            s->player->camera,self.detection,&self.photos.source(),self.collisions.get());
        self.controller=std::make_unique<isle::player::Controller>(*self.photo,self.detection);
        return self.renderer->bind_photos(*self.controller);
    });
}
IsleGameResult isle_rainbow_lifecycle_native_detector_enable(IsleRainbowLifecycleSource* s) {
    return Access::invoke(s,[&](Impl& self) {
        return self.photo?isle_player_photo_PokemonDetector_Enable(&self.photos.source()):ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_photo_count(IsleRainbowLifecycleSource* s,int32_t* out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    return Access::invoke(s,[&](Impl& self) { *out=self.photos.count(); return ISLE_GAME_OK; });
}
IsleGameResult isle_rainbow_lifecycle_native_or_notice(IsleRainbowLifecycleSource* s,uint32_t flags) {
    return Access::invoke(s,[&](Impl& self) { self.state.add_scene_notices(flags); return ISLE_GAME_OK; });
}
IsleGameResult isle_rainbow_lifecycle_native_block_next(IsleRainbowLifecycleSource* s,IsleRainbowLifecycleBlock* value,IsleRainbowLifecycleBlock** out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    *out=nullptr;
    return Access::invoke(s,[&](Impl& self) {
        const auto* block=block_cast(value);
        if (!self.valid_block(block)) return ISLE_GAME_INVALID_ARGUMENT;
        if (block->next && !self.valid_block(block->next)) return ISLE_GAME_INVALID_STATE;
        *out=source_block(block->next); return ISLE_GAME_OK;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_block_position(IsleRainbowLifecycleSource* s,IsleRainbowLifecycleBlock* value,IsleGameVec3* out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    return Access::invoke(s,[&](Impl& self) {
        const auto* block=block_cast(value);
        if (!self.valid_block(block) || !block->resource) return ISLE_GAME_INVALID_ARGUMENT;
        const auto position=block->resource->metadata.world_position;
        *out={position.x,position.y,position.z}; return ISLE_GAME_OK;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_displace_items(IsleRainbowLifecycleSource* s,float x,float y,float z) {
    return Access::invoke(s,[&](Impl& self) {
        return self.items && self.items->displace({x,y,z})?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_context(IsleGame* game,IsleGameObject* object,IsleRainbowLifecycleSource** out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    *out=nullptr;
    if (!game || !object) return ISLE_GAME_INVALID_ARGUMENT;
    IsleGameObjectMetadata metadata{};
    auto result=isle_game_object_metadata(game,object,&metadata);
    if (result!=ISLE_GAME_OK) return result;
    IslePlayerSource* player=nullptr;
    result=isle_player_source_context(game,&player);
    if (result!=ISLE_GAME_OK) return result;
    if (!player || !player->native_root || player->game!=game) return ISLE_GAME_INVALID_STATE;
    auto& root=*static_cast<isle::player::Root*>(player->native_root);
    auto* actors=root.bound_actors();
    if (&root.source()!=player || !actors || &actors->player_source()!=player ||
        actors->course_context().course_id()!=6) return ISLE_GAME_INVALID_STATE;
    auto& course=actors->course_context().rainbow();
    if (course.actors.phase!=ISLE_RAINBOW_ACTOR_ACTIVE ||
        course.actors.nativeGame!=game || course.source.player!=player ||
        course.source.actors!=&course.actors || course.source.owner!=course.actors.owner)
        return ISLE_GAME_INVALID_STATE;
    *out=&course.source;
    return ISLE_GAME_OK;
}
IsleGameResult isle_rainbow_lifecycle_native_scene_joined(IsleRainbowLifecycleSource* s) {
    return Access::invoke(s,[](Impl& self) {
        return self.join_receipt && self.scene_retired && !self.services && !self.storage.game() &&
            self.actor_source.phase==ISLE_RAINBOW_ACTOR_DETACHED &&
            !self.actor_source.nativeGame && !self.actor_source.services &&
            !self.actor_source.D_8034AB94_82A304 && !self.actor_source.D_8034AB98_82A308 &&
            !self.actor_source.D_8034AF2C_82A69C && !self.source.D_8034B674_82ADE4 ?
            ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_stop_bgm(IsleRainbowLifecycleSource* s) {
    return Access::invoke(s,[](Impl& self) { return Access::audio(self,self.audio.stop_bgm()); });
}
IsleGameResult isle_rainbow_lifecycle_native_stop_all_sounds(IsleRainbowLifecycleSource* s) {
    return Access::invoke(s,[](Impl& self) { return Access::audio(self,self.audio.stop_sounds()); });
}
IsleGameResult isle_rainbow_lifecycle_native_flute_command(IsleRainbowLifecycleSource* s,int32_t* out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    return Access::invoke(s,[&](Impl& self) {
        return self.items?isle_item_owner_flute(&self.globals.source,2,out):ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_bgm_tempo(IsleRainbowLifecycleSource* s,int32_t player,int32_t tempo) {
    return Access::invoke(s,[&](Impl& self) {
        return Access::audio(self,self.audio.bgm_tempo(static_cast<unsigned>(player),static_cast<uint32_t>(tempo)));
    });
}
IsleGameResult isle_rainbow_lifecycle_native_sky_speed(IsleRainbowLifecycleSource* s,float speed) {
    return Access::invoke(s,[&](Impl& self) {
        if (!self.world || !self.world_published) return ISLE_GAME_INVALID_STATE;
        self.world->set_sky_animation_speed(speed);
        return ISLE_GAME_OK;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_bgm_reverb(IsleRainbowLifecycleSource* s,int32_t player,int32_t amount) {
    return Access::invoke(s,[&](Impl& self) {
        return Access::audio(self,self.audio.bgm_extra_reverb(static_cast<unsigned>(player),static_cast<int8_t>(amount)));
    });
}
IsleGameResult isle_rainbow_lifecycle_native_sound_reverb(IsleRainbowLifecycleSource* s,int32_t amount) {
    return Access::invoke(s,[&](Impl& self) {
        return Access::audio(self,self.audio.global_sound_reverb(static_cast<uint8_t>(amount)));
    });
}
IsleGameResult isle_rainbow_lifecycle_native_play_sound(IsleRainbowLifecycleSource* s,int32_t id,
    int32_t volume,int32_t pan,float pitch,int32_t reverb,int32_t* out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    *out=-1;
    return Access::invoke(s,[&](Impl& self) {
        int slot=-1;
        const auto result=self.audio.play_sound(static_cast<uint32_t>(id),
            {static_cast<uint16_t>(volume),static_cast<uint8_t>(pan),static_cast<uint8_t>(reverb),pitch},slot);
        *out=slot;
        return Access::audio(self,result);
    });
}
IsleGameResult isle_rainbow_lifecycle_native_stop_sound(IsleRainbowLifecycleSource* s,uint32_t handle) {
    return Access::invoke(s,[&](Impl& self) { return Access::audio(self,self.audio.stop_sound(handle)); });
}
IsleGameResult isle_rainbow_lifecycle_native_sound_identity(IsleRainbowLifecycleSource* s,uint32_t handle,int32_t* out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    return Access::invoke(s,[&](Impl& self) { *out=self.audio.playing_sound(handle); return ISLE_GAME_OK; });
}
IsleGameResult isle_rainbow_lifecycle_native_pokemon_change(IsleRainbowLifecycleSource* s,IsleRainbowLifecycleBlock* first,
    IsleRainbowLifecycleBlock* second,uint32_t table,const IsleRainbowLifecyclePokemonDef* definitions,size_t count) {
    return Access::invoke(s,[&](Impl& self) {
        if (!self.services || !Access::definitions(self,table,definitions,count))
            return ISLE_GAME_INVALID_ARGUMENT;
        if (!self.services->send_command_to_link(3,19,nullptr)) return ISLE_GAME_INVALID_STATE;
        auto* block=block_cast(first); auto* reference=block_cast(second);
        if (!block || !reference) return ISLE_GAME_OK;
        if (!self.valid_block(block) || !self.valid_block(reference)) return ISLE_GAME_INVALID_ARGUMENT;
        if (!reference->resource || !reference->previous || !reference->previous->resource) return ISLE_GAME_OK;
        std::array<isle::menu::animation::InstanceHandle,100> members{};
        size_t written=0;
        if (!self.actors->block_members(*block,members,written)) return ISLE_GAME_INVALID_STATE;
        const auto previous=reference->previous->resource->metadata.world_position;
        const auto current=reference->resource->metadata.world_position;
        for (size_t n=0;n<written;++n) {
            const auto* actor=self.actors->source(members[n]);
            if (!actor) return ISLE_GAME_INVALID_STATE;
            for (size_t i=0;i<count;++i) if (definitions[i].id==static_cast<uint32_t>(actor->id)) {
                if (definitions[i].update_identity!=0x80363dbcu) return ISLE_GAME_INVALID_STATE;
                if (!self.actors->change_block(members[n],
                    {previous.x,previous.y,previous.z},{current.x,current.y,current.z})) return ISLE_GAME_EXECUTION_FAILURE;
                break;
            }
        }
        return ISLE_GAME_OK;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_pokemon_remove(IsleRainbowLifecycleSource* s,IsleRainbowLifecycleBlock* value,
    uint32_t table,const IsleRainbowLifecyclePokemonDef* definitions,size_t count) {
    return Access::invoke(s,[&](Impl& self) {
        if (!Access::definitions(self,table,definitions,count))
            return ISLE_GAME_INVALID_ARGUMENT;
        auto* block=block_cast(value);
        if (!block) return ISLE_GAME_OK;
        if (!self.valid_block(block)) return ISLE_GAME_INVALID_ARGUMENT;
        if (!block->resource) return ISLE_GAME_OK;
        std::array<isle::menu::animation::InstanceHandle,100> members{};
        size_t written=0;
        if (!self.actors->block_members(*block,members,written)) return ISLE_GAME_INVALID_STATE;
        for (size_t n=0;n<written;++n) {
            const auto* actor=self.actors->source(members[n]);
            if (!actor) return ISLE_GAME_INVALID_STATE;
            for (size_t i=0;i<count;++i) if (definitions[i].id==static_cast<uint32_t>(actor->id)) {
                if (definitions[i].remove_identity!=0x80364280u ||
                    !self.actors->request_cleanup(members[n])) return ISLE_GAME_INVALID_STATE;
                break;
            }
        }
        return ISLE_GAME_OK;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_camera_clip(IsleRainbowLifecycleSource* s,uint32_t identity,const IslePlayerCameraAnimationClip** out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    *out=nullptr;
    return Access::invoke(s,[&](Impl& self) {
        if (!self.camera) return ISLE_GAME_INVALID_STATE;
        if (identity==self.cart_resources.intro().camera_identity) *out=&self.intro_camera;
        else if (identity==self.cart_resources.outro().camera_identity) *out=&self.outro_camera;
        return *out?ISLE_GAME_OK:ISLE_GAME_INVALID_ARGUMENT;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_cart_model_clip(IsleRainbowLifecycleSource* s,uint32_t identity,const IslePlayerCartModelAnimation** out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    *out=nullptr;
    return Access::invoke(s,[&](Impl& self) {
        if (!self.cart) return ISLE_GAME_INVALID_STATE;
        if (identity==self.cart_resources.intro().model_animation_table.guest_address) *out=self.cart->intro_model();
        else if (identity==self.cart_resources.outro().model_animation_table.guest_address) *out=self.cart->outro_model();
        return *out?ISLE_GAME_OK:ISLE_GAME_INVALID_ARGUMENT;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_cart_material_clip(IsleRainbowLifecycleSource* s,uint32_t identity,const IslePlayerCartMaterialAnimation** out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    *out=nullptr;
    return Access::invoke(s,[&](Impl& self) {
        if (!self.cart) return ISLE_GAME_INVALID_STATE;
        if (identity==self.cart_resources.intro().material_animation_table.guest_address) *out=self.cart->intro_material();
        else if (identity==self.cart_resources.outro().material_animation_table.guest_address) *out=self.cart->outro_material();
        return *out?ISLE_GAME_OK:ISLE_GAME_INVALID_ARGUMENT;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_camera_speed(IsleRainbowLifecycleSource* s,IsleGameCamera* camera,float speed) {
    return Access::invoke(s,[&](Impl& self) {
        return self.camera && s->player && camera==s->player->camera && self.camera->set_speed(speed)?
            ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_create_effect(IsleRainbowLifecycleSource* s,int32_t bank,int32_t script,IsleRainbowLifecycleEffect** out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    *out=nullptr;
    return Access::invoke(s,[&](Impl& self) {
        if (!self.actors || bank<0 || bank>UINT16_MAX || script<0) return ISLE_GAME_INVALID_ARGUMENT;
        const auto made=self.actors->effects().create_effect(static_cast<uint16_t>(bank),static_cast<size_t>(script));
        if (!made.status) return ISLE_GAME_INVALID_STATE;
        if (made.spawned) {
            if (made.handle.slot>=self.effects.size()) return ISLE_GAME_INVALID_STATE;
            auto& effect=self.effects[made.handle.slot]; effect.handle=made.handle; *out=&effect;
        }
        return ISLE_GAME_OK;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_attach_effect(IsleRainbowLifecycleSource* s,IsleRainbowLifecycleEffect* effect,const IslePlayerCartNode* node) {
    return Access::invoke(s,[&](Impl& self) {
        if (!effect || !node || !self.cart || !self.actors) return ISLE_GAME_INVALID_ARGUMENT;
        bool owned=false;
        for (const auto& candidate:self.effects) if (&candidate==effect) { owned=true; break; }
        if (!owned) return ISLE_GAME_INVALID_ARGUMENT;
        auto attachment=self.cart->retain_effect_node(node);
        if (!attachment) return ISLE_GAME_INVALID_STATE;
        return self.actors->effects().set_effect_attachment(effect->handle,std::move(attachment))?
            ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_store_cart(IsleRainbowLifecycleSource* s,IsleGameObject* object) {
    return Access::invoke(s,[&](Impl& self) {
        if (!self.helper_registered || !self.cart || (object && !self.cart->instance(object)))
            return ISLE_GAME_INVALID_ARGUMENT;
        self.intro_instance=object?self.cart->instance(object):isle::menu::animation::InstanceHandle{};
        return self.services->source_reference_assign(self.helper,&s->D_8034B674_82ADE4,object)?
            ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_retire_intro_cart(IsleRainbowLifecycleSource* s,IsleGameObject* object) {
    return Access::invoke(s,[&](Impl& self) {
        if (!object || object!=s->D_8034B674_82ADE4 || !self.cart) return ISLE_GAME_INVALID_ARGUMENT;
        return self.cart->request_retire(object)?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_release_cart_reference(IsleRainbowLifecycleSource* s) {
    return Access::invoke(s,[&](Impl& self) {
        if (!self.services || isle_game_require_idle(self.services->game)!=ISLE_GAME_OK ||
            (self.frame && !self.frame->quiescent()) || (self.actors && self.actors->effects().effect_count()))
            return ISLE_GAME_INVALID_STATE;
        if (!self.helper_registered) return s->D_8034B674_82ADE4?ISLE_GAME_INVALID_STATE:ISLE_GAME_OK;
        if (s->D_8034B674_82ADE4 && !self.services->source_owner_joined(s->D_8034B674_82ADE4))
            return ISLE_GAME_INVALID_STATE;
        // close_scene stores NULL while this receiver is still live, before
        // joining it. Joined receiver records reject further canonical stores.
        if (s->D_8034B674_82ADE4 &&
            !self.services->source_reference_assign(self.helper,&s->D_8034B674_82ADE4,nullptr))
            return ISLE_GAME_INVALID_STATE;
        if (!self.helper_joined || s->actors->D_8034AB94_82A304 ||
            s->actors->D_8034AB98_82A308 || s->actors->D_8034AF2C_82A69C ||
            !self.services->source_owner_release(self.helper)) return ISLE_GAME_INVALID_STATE;
        self.helper_registered=false;
        return ISLE_GAME_OK;
    });
}
IsleGameResult isle_rainbow_lifecycle_native_release_fx_references(IsleRainbowLifecycleSource* s) {
    return Access::invoke(s,[&](Impl& self) {
        if (!self.services || isle_game_require_idle(self.services->game)!=ISLE_GAME_OK || !self.renderer)
            return ISLE_GAME_INVALID_STATE;
        const auto result=self.renderer->service_main_removal();
        if (result!=ISLE_GAME_OK) return result;
        return s->player && !s->player->D_80382D10_523120 && !s->player->D_80382D14_523124?
            ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
}
} // extern "C"

namespace isle::rainbow {
IsleGameResult Lifecycle::Impl::collide(void* context,IsleGameVec3* a,IsleGameVec3* b,
    IsleGameVec3* c,IsleGameVec3* d,int32_t* hit) noexcept {
    auto* source=static_cast<IsleRainbowActorSource*>(context);
    if (!source || !source->owner || source->phase!=ISLE_RAINBOW_ACTOR_ACTIVE)
        return ISLE_GAME_INVALID_STATE;
    auto& self=*static_cast<Impl*>(source->owner);
    if (&self.actor_source!=source || !self.services || source->nativeGame!=self.services->game ||
        !self.collision_bound || self.cancelling) return ISLE_GAME_INVALID_STATE;
    return isle_rainbow_actor_invoke_collision(source,a,b,c,d,hit);
}
const IsleRainbowActorServices Lifecycle::Impl::actor_services{
    // Exact original global references, accounted by one real semantic owner.
    [](IsleRainbowActorSource* s,IsleGameObject** field,IsleGameObject* object) {
        return LifecycleAccess::actor(s,[&](Impl& self) {
            if (!self.helper_registered || (field!=&s->D_8034AB94_82A304 &&
                field!=&s->D_8034AB98_82A308 && field!=&s->D_8034AF2C_82A69C))
                return ISLE_GAME_INVALID_ARGUMENT;
            if (object) {
                IsleActorSource* actor=nullptr;
                const auto result=isle_actor_source_get(self.services->game,object,&actor);
                if (result!=ISLE_GAME_OK || !actor) return result==ISLE_GAME_OK?ISLE_GAME_INVALID_STATE:result;
            }
            return self.services->source_reference_assign(self.helper,field,object)?
                ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
        });
    },
    [](IsleRainbowActorSource* s,void** out) {
        if (!out) return ISLE_GAME_INVALID_ARGUMENT;
        *out=nullptr;
        return LifecycleAccess::actor(s,[&](Impl& self) {
            if (!self.world || !self.valid_block(self.world->current())) return ISLE_GAME_INVALID_STATE;
            *out=self.world->current();
            return ISLE_GAME_OK;
        });
    },
    [](IsleRainbowActorSource* s,void* block,void* reference,const IsleRainbowAutomaticSpawn* spawn,
        const IsleRainbowDefinition* definition,IsleGameObject** out) {
        if (!out) return ISLE_GAME_INVALID_ARGUMENT;
        *out=nullptr;
        return LifecycleAccess::actor(s,[&](Impl& self) {
            auto* first=static_cast<world::Block*>(block);
            auto* second=static_cast<world::Block*>(reference);
            if (!spawn || !definition || !self.valid_block(first) || !self.valid_block(second))
                return ISLE_GAME_INVALID_ARGUMENT;
            const auto made=self.actors->add_one_rainbow(*definition,*spawn,first,second);
            if (!made) return self.fail(ISLE_GAME_INVALID_STATE,made.status.detail);
            *out=made.object;
            return ISLE_GAME_OK;
        });
    },
    [](IsleRainbowActorSource* s,int32_t id,int32_t* out) {
        return LifecycleAccess::actor(s,[&](Impl& self) {
            return isle_player_root_native_play_sound(self.source.player,id,out);
        });
    },
    [](IsleRainbowActorSource* s,int32_t handle,int32_t* out) {
        if (!out) return ISLE_GAME_INVALID_ARGUMENT;
        return LifecycleAccess::actor(s,[&](Impl& self) {
            *out=self.audio.playing_sound(static_cast<unsigned>(handle))!=-1;
            return ISLE_GAME_OK;
        });
    },
    [](IsleRainbowActorSource* s,int32_t handle) {
        return LifecycleAccess::actor(s,[&](Impl& self) {
            return isle_player_root_native_stop_sound(self.source.player,handle);
        });
    },
    [](IsleRainbowActorSource* s) {
        return LifecycleAccess::actor(s,[](Impl& self) {
            return isle_player_root_native_stop_all_sounds(self.source.player);
        });
    },
    [](IsleRainbowActorSource* s,int32_t player,int32_t song) {
        return LifecycleAccess::actor(s,[&](Impl& self) {
            return isle_player_root_native_play_song(self.source.player,player,song);
        });
    },
    [](IsleRainbowActorSource* s,int32_t player) {
        return LifecycleAccess::actor(s,[&](Impl& self) {
            return isle_player_root_native_stop_song(self.source.player,player);
        });
    },
    [](IsleRainbowActorSource* s,int32_t player,uint32_t volume) {
        return LifecycleAccess::actor(s,[&](Impl& self) {
            return LifecycleAccess::audio(self,self.audio.bgm_volume(static_cast<unsigned>(player),volume));
        });
    },
    [](IsleRainbowActorSource* s,int32_t player,uint8_t channel,int32_t volume) {
        return LifecycleAccess::actor(s,[&](Impl& self) {
            return isle_player_root_native_bgm_channel_volume(self.source.player,player,channel,volume);
        });
    },
    [](IsleRainbowActorSource* s) {
        return LifecycleAccess::actor(s,[](Impl& self) {
            return self.items->remove_flying()?ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE;
        });
    },
    [](IsleRainbowActorSource* s) {
        return LifecycleAccess::actor(s,[](Impl& self) {
            return isle_player_root_invoke_reset_main_camera_settings(self.source.player);
        });
    },
    [](IsleRainbowActorSource* s) {
        return LifecycleAccess::actor(s,[](Impl& self) { return isle_player_ui_Msg_Reset(&self.ui->source()); });
    },
    [](IsleRainbowActorSource* s,const int16_t* exceptions) {
        return LifecycleAccess::actor(s,[&](Impl& self) {
            if (!exceptions || exceptions[0]!=151 || exceptions[1]!=1001 || exceptions[2]!=0)
                return ISLE_GAME_INVALID_ARGUMENT;
            return isle_actor_source_remove_pokemons_call(self.services->game,
                isle_menu_current_object(self.services->game),
                reinterpret_cast<const uint16_t*>(exceptions),3);
        });
    },
    [](IsleRainbowActorSource* s,IsleGameObject* object,float time) {
        return LifecycleAccess::actor(s,[&](Impl& self) {
            return isle_player_root_invoke_start_cutscene(self.source.player,object,nullptr,time);
        });
    },
    [](IsleRainbowActorSource* s,IsleGameObject* object) {
        return LifecycleAccess::actor(s,[&](Impl& self) {
            return isle_game_pause_object_processes(self.services->game,object,0);
        });
    },
    [](IsleRainbowActorSource* s,IsleGameObject** out) {
        if (!out) return ISLE_GAME_INVALID_ARGUMENT;
        *out=nullptr;
        return LifecycleAccess::actor(s,[&](Impl& self) {
            if (!self.source.player->camera) return ISLE_GAME_INVALID_STATE;
            return isle_game_camera_object(self.services->game,self.source.player->camera,out);
        });
    },
    [](IsleRainbowActorSource* s,IsleGameObject* object) {
        return LifecycleAccess::actor(s,[&](Impl& self) {
            return isle_game_pause_object_processes(self.services->game,object,1);
        });
    },
    [](IsleRainbowActorSource* s,IsleGameObject* object,float speed) {
        return LifecycleAccess::actor(s,[&](Impl& self) {
            IsleGameObject* actual=nullptr;
            const auto result=isle_game_camera_object(self.services->game,self.source.player->camera,&actual);
            if (result!=ISLE_GAME_OK) return result;
            return actual==object && self.camera && self.camera->set_speed(speed)?
                ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
        });
    },
    [](IsleRainbowActorSource* s,IsleGameObject* object,uint32_t identity,float time) {
        return LifecycleAccess::actor(s,[&](Impl& self) {
            IsleGameObject* actual=nullptr;
            const auto result=isle_game_camera_object(self.services->game,self.source.player->camera,&actual);
            if (result!=ISLE_GAME_OK) return result;
            if (actual!=object || identity!=self.cart_resources.outro().camera_identity || !self.camera)
                return ISLE_GAME_INVALID_ARGUMENT;
            return self.camera->set(&self.outro_camera,time)?ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE;
        });
    },
    [](IsleRainbowActorSource* s,IsleGameObject* object,int32_t kind,int32_t priority) {
        return LifecycleAccess::actor(s,[&](Impl& self) {
            IsleGameObject* actual=nullptr;
            auto result=isle_game_camera_object(self.services->game,self.source.player->camera,&actual);
            if (result!=ISLE_GAME_OK) return result;
            if (actual!=object || kind!=1 || priority!=1) return ISLE_GAME_INVALID_ARGUMENT;
            IsleGameProcess* process=nullptr;
            return isle_game_add_source_process(self.services->game,object,
                isle_rainbow_lifecycle_camera_animation_update,ISLE_GAME_SYNCHRONOUS,1,&process);
        });
    },
    [](IsleRainbowActorSource* s,float speed) {
        return LifecycleAccess::actor(s,[&](Impl& self) {
            self.world->set_sky_animation_speed(speed);
            return ISLE_GAME_OK;
        });
    },
    [](IsleRainbowActorSource* s,IsleGameObject** out) {
        if (!out) return ISLE_GAME_INVALID_ARGUMENT;
        *out=nullptr;
        return LifecycleAccess::actor(s,[&](Impl& self) {
            if (!self.cart || self.cutscene_cart) return ISLE_GAME_INVALID_STATE;
            const auto made=self.cart->init();
            if (!made) return made.status.error==player::cart::OwnerError::capacity?
                ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE;
            self.cutscene_cart=self.cart->retain(made.object);
            if (!self.cutscene_cart) return ISLE_GAME_INVALID_STATE;
            *out=made.object;
            return ISLE_GAME_OK;
        });
    },
    [](IsleRainbowActorSource* s,uint32_t model,uint32_t material,float time,float speed) {
        return LifecycleAccess::actor(s,[&](Impl& self) {
            const auto& outro=self.cart_resources.outro();
            if (!self.cart || model!=outro.model_animation_table.guest_address ||
                material!=outro.material_animation_table.guest_address) return ISLE_GAME_INVALID_ARGUMENT;
            return self.cart->set_animation(self.cart->outro_model(),self.cart->outro_material(),time,speed)?
                ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE;
        });
    },
    [](IsleRainbowActorSource* s,IsleGameObject* object,IsleRainbowAnimationCallback callback) {
        return LifecycleAccess::actor(s,[&](Impl& self) {
            if (callback!=func_80348BDC_82834C || !self.cutscene_cart ||
                self.cutscene_cart.object()!=object) return ISLE_GAME_INVALID_ARGUMENT;
            const auto result=self.cart->set_callback(object,isle_rainbow_lifecycle_actor_cart_event,&self.source);
            if (!result) return ISLE_GAME_EXECUTION_FAILURE;
            self.cutscene_callback=callback;
            return ISLE_GAME_OK;
        });
    },
    [](IsleRainbowActorSource* s,IsleRainbowCollision* collision) {
        return LifecycleAccess::actor(s,[&](Impl& self) {
            self.gate_resources.require_live();
            const auto& model=self.gate_resources.model();
            if (collision!=&s->D_8034AF20_82A690 || collision->native ||
                collision->identity!=model.wire.guest_address ||
                collision->tree_identity!=model.tree_wire.guest_address ||
                collision->actor_id!=model.id || collision->scale!=model.scale)
                return ISLE_GAME_INVALID_ARGUMENT;
            collision->native=&self.gate_native;
            return ISLE_GAME_OK;
        });
    },
    [](IsleRainbowActorSource* s,IsleGameVec3* a,IsleGameVec3* b,IsleGameVec3* c,IsleGameVec3* d,
        const IsleRainbowCollision* collision,const IsleGameVec3* position,const IsleGameVec3* rotation,int32_t* hit) {
        if (!a || !b || !c || !d || !position || !rotation || !hit) return ISLE_GAME_INVALID_ARGUMENT;
        return LifecycleAccess::actor(s,[&](Impl& self) {
            if (collision!=&s->D_8034AF20_82A690 || collision->native!=&self.gate_native ||
                self.gate_native.resources!=&self.gate_resources ||
                self.gate_native.prepared!=&self.gate_collision) return ISLE_GAME_INVALID_STATE;
            self.gate_resources.require_live();
            IsleActorSource* gate=nullptr;
            auto result=isle_actor_source_get(self.services->game,s->D_8034AF2C_82A69C,&gate);
            if (result!=ISLE_GAME_OK) return result;
            const IsleGameModelTransform* current=nullptr;
            result=isle_actor_owner_aux_read(gate,&s->gate_model,&current);
            if (result!=ISLE_GAME_OK) return result;
            if (position!=&current->position || rotation!=&current->rotation) return ISLE_GAME_INVALID_ARGUMENT;
            world::Vec3 output_position{c->x,c->y,c->z},output_velocity{d->x,d->y,d->z};
            const bool collided=self.gate_collision.collision_model_one(0,
                {position->x,position->y,position->z},{rotation->x,rotation->y,rotation->z},
                {a->x,a->y,a->z},{b->x,b->y,b->z},output_position,output_velocity);
            // Negative results can still mutate outputs in the original
            // primitive solver; do not condition these stores on a hit.
            *c={output_position.x,output_position.y,output_position.z};
            *d={output_velocity.x,output_velocity.y,output_velocity.z};
            *hit=collided?1001:-1;
            return ISLE_GAME_OK;
        });
    },
    [](IsleRainbowActorSource* s,IsleRainbowCollisionCallback callback) {
        return LifecycleAccess::actor(s,[&](Impl& self) {
            if (callback!=func_80348FF0_828760 || s->D_8034AF20_82A690.native!=&self.gate_native)
                return ISLE_GAME_INVALID_ARGUMENT;
            const auto result=self.items->set_extra_collision(self.services->game,Impl::collide,s);
            if (!result) return ISLE_GAME_INVALID_STATE;
            self.collision_bound=true;
            return ISLE_GAME_OK;
        });
    }
};
} // namespace isle::rainbow
extern "C" IsleGameResult isle_rainbow_lifecycle_native_invalidate_input(IsleRainbowLifecycleSource* s) {
    return Access::invoke(s,[](Impl& self) {
        if (!self.services || self.cancelling || !self.input_object || self.input_joined ||
            isle_menu_current_object(self.services->game)!=self.input_object)
            return ISLE_GAME_INVALID_STATE;
        return self.input.begin_update()==isle::game_input::Error::none ?
            ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE;
    });
}

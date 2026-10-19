#include "beach_lifecycle.hpp"
#include "player_root.hpp"
#include "player_renderer.hpp"
#include "player_commands.hpp"
#include "player_ui.hpp"
#include "player_bump.hpp"
#include "player_cart.hpp"
#include "player_camera_animation.hpp"
#include "actor_photo_scene.hpp"
#include "actor_interaction_services.hpp"
#include "beach_music.hpp"
#include "beach_ending.hpp"
#include "course_model_producer.hpp"
#include "effect_bank_resources.hpp"
#include "screen_transition_resources.hpp"
#include "beach_meowth_source.h"
#include "scene_storage.hpp"
#include "menu_scene_bridge.h"
#include <algorithm>
#include <bit>
#include <cstdio>
#include <exception>
#include <source_location>
#include <stdexcept>
#include <vector>

struct IsleBeachLifecycleAsset {
    IsleBeachLifecycleAssetId id;
    std::span<const std::byte> bytes;
    bool loaded=false;
};
struct IsleBeachLifecycleEffect { isle::menu::EffectHandle handle; };
namespace isle::beach {
namespace {
namespace g=scene::graphics;
namespace a=menu::animation;
constexpr std::size_t plane=320*240*2;
constexpr world::ProducerLayout world_layout{{0x200000,0x10000},{0x210000,0x10000},{0x220000,0x10000}};
constexpr actor::ProducerLayout actor_layout{{0x400000,0x80000},{0x480000,0x80000},{0x500000,0x100000},{{{0x3b5000,plane},{0x3da800,plane}}}};
constexpr menu::model::PacketLayout item_layout{{0x230000,0x10000},{0x240000,0x10000},{0x250000,0x10000},{{{0x3b5000,plane},{0x3da800,plane}}}};
constexpr player::cart::ProducerLayout cart_layout{{0x260000,0x10000},{0x270000,0x10000},{0x2a0000,0x10000},{{{0x3b5000,plane},{0x3da800,plane}}}};
constexpr focus::CaptureLayout capture_layout{{0x680000,0x50},{0x680080,0x80},{0x680100,0xa00}};
constexpr std::size_t render_order_capacity=256;
constexpr menu::RamRange fx_commands{0x680c00,0x5400},
    joined_lists{0x686000,course::source_join_bytes(render_order_capacity)};
constexpr sprite::RamRange lanes{0x690000,0x40000},sprite_commands{0x780000,0x50000},message_commands{0x6d0000,0x30000};
static_assert(joined_lists.size<=lanes.address-joined_lists.offset);
constexpr g::Layout frame_layout{{0x626000,0x2000},{0x630000,0x10000},{0x640000,0x40000},0x600000};
std::uint32_t physical(std::uint32_t value) { return value&0x1fffffffu; }
void require(bool value,const char* message) { if (!value) throw std::runtime_error(message); }
void native(IsleGameResult value,const char* message) { require(value==ISLE_GAME_OK,message); }
world::Block* block_cast(IsleBeachLifecycleBlock* value) { return reinterpret_cast<world::Block*>(value); }
IsleBeachLifecycleBlock* source_block(world::Block* value) { return reinterpret_cast<IsleBeachLifecycleBlock*>(value); }
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
    Behaviors behaviors;
    bool require_cpu_renderer=false;
    IsleBeachLifecycleSource source{};
    IsleContinuationDomain* domain=nullptr;
    IsleContinuation* continuation=nullptr;
    LifecycleStatus status=LifecycleStatus::ready;
    IsleGameResult error=ISLE_GAME_OK;
    std::array<char,512> message{};
    bool initializing=false,initialized=false,exit_requested=false,cancelling=false;
    bool requested=false,world_published=false,height_map=false,sky_follow=false,screen_applied=false;
    bool helper_joined=false,helper_registered=false,scene_retired=false;
    bool helper_observed=false;
    bool near_disabled=false;
    world::Resources world_resources;
    actor::Resources actor_resources;
    actor::BehaviorResources behavior_resources;
    ActorSceneResources actor_scene_resources;
    std::unique_ptr<ActorSceneState> actor_scene_state;
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
    std::unique_ptr<Music> music;
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
    std::unique_ptr<Ending> ending;
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
    std::array<scene::RenderOrderEntry,render_order_capacity> cart_order{};
    IsleGameObject* helper=nullptr;
    a::InstanceHandle intro_instance;
    std::array<IsleGameObject**,1> helper_fields{{&source.D_beach_802CC0E4}};
    std::array<IsleBeachLifecycleAsset,5> assets{};
    std::array<IsleBeachLifecycleEffect,10> effects{};

    Impl(const rom::Snapshot& rom,application::Manager& m,game::State& s,
        game_input::Service& i,game_audio::Session& au,host::GtlClock& cl,
        application::Photos& ph,world::ModuleState& module,Behaviors b,bool cpu)
        :snapshot(rom),manager(m),state(s),input(i),audio(au),clock(cl),photos(ph),world_state(module),behaviors(b),require_cpu_renderer(cpu),
         world_resources(rom, isle::course::beach_id),actor_resources(rom,isle::course::beach_id,{world_resources.setup().fog_rgb,world_resources.setup().fog_min,world_resources.setup().fog_max}),
         behavior_resources(rom,isle::course::beach_id),actor_scene_resources(rom),item_resources(rom,world_resources.setup().fog_rgb,world_resources.setup().fog_min,world_resources.setup().fog_max),
         cart_resources(rom,isle::course::beach_id,{world_resources.setup().fog_rgb,world_resources.setup().fog_min,world_resources.setup().fog_max}),
         ui_resources(rom),sounds(rom,isle::course::beach_id),effect_resources(rom),lighting(rom),programs(rom),transition_resources(rom),
         banks{&effect_resources.bank(menu::EffectGroup::common),&effect_resources.bank(menu::EffectGroup::beach)},
         effect_graphics(rom,banks,{0x700000,0x80000}),sprites(ui_resources.roots(),ui_resources.banks()) {
        constexpr std::array<std::array<std::uint32_t,2>,5> ranges{{{0xab5860,0xab5980},{0xab5980,0xab85e0},{0xab85e0,0xab8780},{0xab8780,0xabe7a0},{0xaaa660,0xab1470}}};
        for (std::size_t n=0;n<assets.size();++n)
            assets[n]={static_cast<IsleBeachLifecycleAssetId>(n),rom.canonical().subspan(ranges[n][0],ranges[n][1]-ranges[n][0]),false};
    }
    IsleGameResult fail(IsleGameResult result,const char* text) noexcept {
        if (result==ISLE_GAME_OK) return result;
        if (error==ISLE_GAME_OK) { error=result; std::snprintf(message.data(),message.size(),"%s",text); }
        status=LifecycleStatus::failed;
        return result;
    }
    bool valid_block(const world::Block* value) const noexcept {
        if (!world || !value) return false;
        for (const auto& candidate:world->blocks()) if (&candidate==value) return true;
        return false;
    }
    static bool add(void* user,world::Block& first,world::Block& second) noexcept {
        auto& self=*static_cast<Impl*>(user);
        return isle_beach_lifecycle_world_add(&self.source,source_block(&first),source_block(&second))==ISLE_GAME_OK;
    }
    static bool remove(void* user,world::Block& block) noexcept {
        auto& self=*static_cast<Impl*>(user);
        if (self.cancelling) return !self.actors || bool(self.actors->remove_block(block));
        return isle_beach_lifecycle_world_remove(&self.source,source_block(&block))==ISLE_GAME_OK;
    }
    static bool change(void* user,world::Block& first,world::Block& second) noexcept {
        auto& self=*static_cast<Impl*>(user);
        return isle_beach_lifecycle_world_change(&self.source,source_block(&first),source_block(&second))==ISLE_GAME_OK;
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
        return isle_beach_lifecycle_exit_block(&self.source,source_block(&block))==ISLE_GAME_OK;
    }
    static bool movement(void* user,std::size_t block,float fraction) noexcept {
        auto& self=*static_cast<Impl*>(user);
        return block<=INT32_MAX && isle_beach_lifecycle_movement(&self.source,static_cast<std::int32_t>(block),fraction)==ISLE_GAME_OK;
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
    IsleGameResult spawn(std::size_t,world::Block&,world::Block&,const world::Spawn&,IsleGameObject**) noexcept;
    ~Impl() {
        if (continuation && isle_continuation_destroy(continuation)!=ISLE_CONT_OK) std::terminate();
        if (domain && isle_continuation_domain_destroy(domain)!=ISLE_CONT_OK) std::terminate();
    }
};

void Lifecycle::Impl::cold() {
    native(storage.create(),"Beach native engine domain creation rejected");
    actor_scene_state=std::make_unique<ActorSceneState>(actor_scene_resources,source.D_beach_802CC014);
    services=std::make_unique<scene::Services>(storage.game(),storage.arena(),
        scene::ResourceView{{source.scene_setup.objects,source.scene_setup.processes,source.scene_setup.sprites,
            source.scene_setup.cameras,source.scene_setup.matrices},ui_resources.resources()},input,audio,state);
    music=std::make_unique<Music>(*services,snapshot);
    player::Root::Config config{0,isle_beach_lifecycle_end_event,&source,
        isle_beach_lifecycle_pause_event,&source,{this,exit_block,movement},
        nullptr,0,isle_beach_lifecycle_item_collision};
    root=std::make_unique<player::Root>(*services,ui_resources,config);
    native(music->bind(root->source()),"Beach canonical Root/Music binding rejected");
    source.player=&root->source(); source.music=&music->source();
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
    world=std::make_unique<world::World>(*services,world_resources,world_state,
        world::Callbacks{this,add,remove,change,shift_camera,shift_effects,nullptr},true);
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
    actors=std::make_unique<actor::Owner>(*services,actor_resources,actor::CourseContext(*actor_scene_state),*world,*environment,detector,
        root->source(),actor_capacities,banks,a::ExecutionCapacity{32768,4096});
    items=std::make_unique<items::Owner>(*services,item_resources,*world,*actors,globals,
        root->source(),item_capacities,a::ExecutionCapacity{32768,4096});
    cart=std::make_unique<player::cart::Owner>(*services,cart_resources,root->source(),cart_capacities,a::ExecutionCapacity{});
    bindings=std::make_unique<actor::BehaviorBindings>(behavior_resources,behaviors.callbacks,
        behaviors.graphs,std::span<const std::uint32_t>{},behaviors.roots);
    interactions=std::make_unique<actor::InteractionServices>(*services,*actors,*items,*world,
        detector,*bindings,actor_resources,effect_resources);
    require(bool(actors->bind_interactions(*interactions)),"Beach interaction binding rejected");
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
        *services,*item_producer,render_order_capacity,carts.get(),photo_scene->prefix_object(),true});
    models=std::make_unique<course::ModelProducer>(*worlds,*actor_producer,joined_lists,composition.get());
    native(renderer->bind_cart(*cart,cart_order),renderer->message());
    native(renderer->bind_transition(transition_resources,services->screen_transition()),renderer->message());
    require(renderer->reserve(*frame),frame->message());
}
IsleGameResult Lifecycle::Impl::spawn(std::size_t definition,world::Block& block,world::Block& reference,
    const world::Spawn& row,IsleGameObject** output) noexcept {
    if (output) *output=nullptr;
    const auto made=actors->add_one(definition,&block,&reference,row);
    if (!made) return fail(ISLE_GAME_INVALID_STATE,made.status.detail);
    if (output) *output=made.object;
    return ISLE_GAME_OK;
}

Lifecycle::Lifecycle(std::unique_ptr<Impl> impl) noexcept:impl_(std::move(impl)) {}
LifecycleCreateResult Lifecycle::create(const rom::Snapshot& snapshot,application::Manager& manager,
    game::State& state,game_input::Service& input,game_audio::Session& audio,host::GtlClock& clock,
    application::Photos& photos,world::ModuleState& module,std::int32_t scene,Behaviors behaviors,bool require_cpu_renderer) noexcept {
    LifecycleCreateResult result;
    try {
        auto impl=std::make_unique<Impl>(snapshot,manager,state,input,audio,clock,photos,module,behaviors,require_cpu_renderer);
        require(isle_continuation_domain_create(&impl->domain)==ISLE_CONT_OK,"Beach independent domain creation failed");
        const auto rom=snapshot.canonical();
        native(isle_beach_lifecycle_prepare(&impl->source,impl.get(),impl->domain,
            reinterpret_cast<const std::uint8_t*>(rom.data()),rom.size(),scene),
            "Beach original Source data preparation rejected");
        require(isle_continuation_create(impl->domain,isle_beach_lifecycle_run,&impl->source,
            &impl->continuation)==ISLE_CONT_OK,"Beach independent continuation creation failed");
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
    if (!self.continuation || self.initializing || (self.services && isle_game_require_idle(self.services->game)!=ISLE_GAME_OK))
        return ISLE_GAME_INVALID_STATE;
    if (self.error!=ISLE_GAME_OK) return self.error;
    if (isle_continuation_resume(self.continuation)!=ISLE_CONT_OK)
        return self.fail(self.source.status==ISLE_GAME_OK?ISLE_GAME_INVALID_STATE:self.source.status,
            isle_continuation_error(self.domain));
    if (self.source.completed) self.status=LifecycleStatus::completed;
    else if (self.requested && !self.initialized) self.status=LifecycleStatus::setup_pending;
    return ISLE_GAME_OK;
}
const IsleBeachLifecycleSceneSetup* Lifecycle::pending_setup() const noexcept {
    return impl_->requested && !impl_->initialized && !impl_->scene_retired ? &impl_->source.scene_setup:nullptr;
}
IsleGameResult Lifecycle::initialize_scene() noexcept {
    auto& self=*impl_;
    if (self.status!=LifecycleStatus::setup_pending || self.initializing || self.services) return ISLE_GAME_INVALID_STATE;
    self.initializing=true;
    try {
        self.cold();
        native(isle_beach_lifecycle_invoke_init(&self.source),"Beach original init Source native call rejected");
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
    if (result==ISLE_GAME_OK) result=self.services->tick();
    // Native deletion is not physical Item collection. Both OM traversal and
    // the previous GPU frame are excluded at this live update boundary.
    if (self.items) {
        const auto collected=self.items->collect_joined();
        if (!collected && collected.error!=items::OwnerError::lifetime && result==ISLE_GAME_OK) {
            self.services->fail(collected.detail);
            result=ISLE_GAME_INVALID_STATE;
        }
    }
    if (self.cart && self.intro_instance && self.source.D_beach_802CC0E4) {
        const auto state=self.cart->membership(self.intro_instance);
        if (state.retiring && !state.joined) {
            const auto retired=self.cart->retire(self.source.D_beach_802CC0E4);
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
const IsleBeachLifecycleSource& Lifecycle::source() const noexcept { return impl_->source; }
const actor::Resources& Lifecycle::actor_resources() const noexcept { return impl_->actor_resources; }
const actor::Owner* Lifecycle::actors() const noexcept { return impl_->actors.get(); }
const actor::Producer* Lifecycle::actor_packet_producer() const noexcept { return impl_->actor_producer.get(); }
const world::World* Lifecycle::world_owner() const noexcept { return impl_->world.get(); }
const IsleBeachEndingSource* Lifecycle::ending_source() const noexcept {
    return impl_->actors?&impl_->actors->ending_source():nullptr;
}
std::optional<std::int32_t> Lifecycle::result() const noexcept {
    if (!impl_->source.completed || impl_->error!=ISLE_GAME_OK || !impl_->scene_retired) return {};
    return impl_->source.output_scene;
}

bool Lifecycle::Impl::close_scene() noexcept {
    if (scene_retired) return true;
    if (!services) {
        if (storage.game() && storage.destroy()!=ISLE_GAME_OK) return false;
        scene_retired=true; return true;
    }
    if (isle_game_require_idle(services->game)!=ISLE_GAME_OK ||
        (frame && !frame->quiescent())) return false;
    // A terminal Frame is not closure of Photo consumers or its Source FIFO.
    if (controller && frame && controller->has_pending(*frame) &&
        renderer->release_completed()!=ISLE_GAME_OK) return false;
    if (photo && !photo->quiescent()) return false;
    if (photo_scene) { photo_scene->retire(); photo_scene.reset(); }
    if (controller && controller->queued_requests()) return false;
    models.reset(); composition.reset(); carts.reset(); item_producer.reset();
    actor_producer.reset(); worlds.reset();
    collisions.reset();
    if (environment) environment->cleanup();
    // Scene-local FX hold live model incarnations. Reset the real pool, not
    // application Audio or outstanding PCM, before requesting Cart joins.
    if (actors) actors->effects().reset();
    if (root && root->retire_objects()!=ISLE_GAME_OK) return false;
    if (items) {
        const auto result=items->retire();
        if (!result && !(cancelling && result.error==isle::items::OwnerError::services)) return false;
    }
    if (cart) {
        const auto result=cart->retire();
        if (!result && !(cancelling && result.error==player::cart::OwnerError::services)) return false;
    }
    if (actors) {
        const auto result=actors->reset();
        // Actor reset's failed-scene result is returned only after its real
        // joins/collection succeed; this closes cancellation, never success.
        if (!result && !(cancelling && result.error==actor::OwnerError::services)) return false;
    }
    if (renderer) {
        if (cancelling) {
            if (renderer->abort_main()!=ISLE_GAME_OK) return false;
        } else if (source.player && source.player->camera &&
            (renderer->remove_main()!=ISLE_GAME_OK ||
             renderer->service_main_removal()!=ISLE_GAME_OK)) return false;
    }
    if (ending) {
        if (!ending->release()) return false;
        ending.reset();
    }
    if (source.D_beach_802CC0E4 && !services->source_owner_joined(source.D_beach_802CC0E4))
        return false;
    if (helper_registered) {
        if (!services->source_reference_assign(helper,&source.D_beach_802CC0E4,nullptr)) return false;
    }
    if (source.player && (source.player->D_80382D10_523120 || source.player->D_80382D14_523124))
        return false; // Only the real main-camera retirement closes these fields.
    if (root && root->collect_joined()!=ISLE_GAME_OK) return false;
    if (items && !items->collect_joined()) return false;
    if (cart && !cart->collect_joined()) return false;
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
    if (world && !world->retire()) return false;
    if (helper) {
        if (!helper_observed) {
            if (isle_game_delete_object(services->game,helper)!=ISLE_GAME_OK) return false;
        } else {
            if (!helper_joined) services->erase(helper);
            if (!helper_joined) return false;
        }
        if (helper_registered && !services->source_owner_release(helper)) return false;
        helper_registered=false; helper=nullptr;
    }
    controller.reset(); photo.reset();
    items.reset(); cart.reset(); bump.reset(); ui.reset(); actors.reset();
    interactions.reset(); bindings.reset(); world.reset(); environment.reset();
    if (commands) { if (commands->detach()!=ISLE_GAME_OK) return false; commands.reset(); }
    if (root) {
        const auto result=root->deactivate();
        if (root->source().native_root || (result!=ISLE_GAME_OK && result!=isle_game_status(services->game)))
            return false;
    }
    if (music) { if (music->detach()!=ISLE_GAME_OK) return false; music.reset(); }
    root.reset();
    source.player=nullptr; source.music=nullptr;
    services.reset();
    if (storage.destroy()!=ISLE_GAME_OK) return false;
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
        const auto result=isle_beach_lifecycle_complete_scene(&self.source,ISLE_GAME_OK);
        if (result!=ISLE_GAME_OK) { self.fail(result,"Beach Source completion rejected real scene retirement"); return false; }
    }
    return true;
}
IsleGameResult Lifecycle::cancel() noexcept {
    auto& self=*impl_;
    self.cancelling=true;
    if (self.continuation && isle_continuation_cancel(self.continuation)!=ISLE_CONT_OK)
        return self.fail(ISLE_GAME_INVALID_STATE,"Beach Source cancellation did not join");
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
    template<class Function> static IsleGameResult invoke(IsleBeachLifecycleSource* source,Function function,
        std::source_location call=std::source_location::current()) noexcept {
        if (!source || !source->owner) return ISLE_GAME_INVALID_ARGUMENT;
        auto& self=*static_cast<Impl*>(source->owner);
        if (&self.source!=source) return ISLE_GAME_INVALID_ARGUMENT;
        try {
            const auto result=function(self);
            return result==ISLE_GAME_OK?result:self.fail(result,call.function_name());
        }
        catch (const std::exception& exception) { return self.fail(ISLE_GAME_INVALID_STATE,exception.what()); }
        catch (...) { return self.fail(ISLE_GAME_INVALID_STATE,"Beach native service threw"); }
    }
    static IsleGameResult setup(Impl& self) noexcept {
        return self.initializing && self.services && isle_game_require_idle(self.services->game)==ISLE_GAME_OK ?
            ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    }
};
} // namespace isle::beach

namespace {
using Access=isle::beach::LifecycleAccess;
using Impl=Access::Impl;
using isle::beach::block_cast;
using isle::beach::source_block;
}
extern "C" {
IsleGameResult isle_beach_lifecycle_native_intervals(IsleBeachLifecycleSource* s,uint16_t update,uint16_t draw) {
    return Access::invoke(s,[&](Impl& self) {
        return self.clock.set_intervals(update,draw)==isle::host::ClockError::none?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_beach_lifecycle_native_near_clipping(IsleBeachLifecycleSource* s,int32_t disabled) {
    return Access::invoke(s,[&](Impl& self) {
        if (disabled!=0 && disabled!=1) return ISLE_GAME_INVALID_ARGUMENT;
        self.near_disabled=disabled!=0;
        return self.renderer?self.renderer->set_near_clipping_disabled(self.near_disabled):ISLE_GAME_OK;
    });
}
IsleGameResult isle_beach_lifecycle_native_request_scene(IsleBeachLifecycleSource* s,const IsleBeachLifecycleSceneSetup* setup) {
    return Access::invoke(s,[&](Impl& self) {
        if (setup!=&s->scene_setup || self.requested || self.services || setup->contexts!=2 ||
            setup->heap_base!=0x801b0310u || setup->heap_size!=0x802c40a0u-0x801b0310u ||
            setup->display_list_bytes[0]!=0x5000 || setup->display_list_bytes[1]!=0xc00 ||
            setup->display_list_bytes[2]!=0x400 || setup->display_list_bytes[3]!=0 ||
            setup->graphics_heap_bytes!=0xc800 || setup->rdp_output_bytes!=0x4000 ||
            setup->thread_stack_bytes!=0x440) return ISLE_GAME_INVALID_STATE;
        self.requested=true;
        return ISLE_GAME_OK;
    });
}
IsleGameResult isle_beach_lifecycle_owner_transition_end(IsleBeachLifecycleSource* s,int32_t reason) {
    return Access::invoke(s,[&](Impl& self) { return self.manager.transition_end(reason); });
}
IsleGameResult isle_beach_lifecycle_native_exit_scene(IsleBeachLifecycleSource* s) {
    return Access::invoke(s,[&](Impl& self) {
        if (!self.services) return ISLE_GAME_INVALID_STATE;
        self.exit_requested=true; self.clock.request_stop();
        return ISLE_GAME_OK;
    });
}
IsleGameResult isle_beach_lifecycle_native_destroy_world(IsleBeachLifecycleSource* s) {
    return Access::invoke(s,[&](Impl& self) {
        return self.world && self.world_published && self.world->request_retire()?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_beach_lifecycle_native_remove_main_camera(IsleBeachLifecycleSource* s) {
    return Access::invoke(s,[&](Impl& self) { return self.renderer?self.renderer->remove_main():ISLE_GAME_INVALID_STATE; });
}
IsleGameResult isle_beach_lifecycle_native_environment_cleanup(IsleBeachLifecycleSource* s) {
    return Access::invoke(s,[&](Impl& self) {
        if (!self.environment) return ISLE_GAME_INVALID_STATE;
        self.environment->cleanup(); return ISLE_GAME_OK;
    });
}
IsleGameResult isle_beach_lifecycle_native_detector_disable(IsleBeachLifecycleSource* s) {
    return Access::invoke(s,[&](Impl& self) {
        return self.photo?isle_player_photo_PokemonDetector_Disable(&self.photos.source()):ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_beach_lifecycle_native_depth_buffer(IsleBeachLifecycleSource* s,uint32_t bytes,uint32_t alignment,void** out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    *out=nullptr;
    return Access::invoke(s,[&](Impl& self) {
        if (Access::setup(self)!=ISLE_GAME_OK || !self.frame || self.depth) return ISLE_GAME_INVALID_STATE;
        self.depth=self.frame->reserve_source_depth(bytes,alignment);
        if (!self.depth) return ISLE_GAME_INVALID_STATE;
        *out=const_cast<isle::scene::graphics::DepthReservation*>(self.depth); return ISLE_GAME_OK;
    });
}
IsleGameResult isle_beach_lifecycle_native_screen(IsleBeachLifecycleSource* s,const IsleBeachLifecycleScreen* screen) {
    return Access::invoke(s,[&](Impl& self) {
        if (Access::setup(self)!=ISLE_GAME_OK || screen!=&s->screen || !self.depth ||
            screen->depth_buffer!=self.depth || self.screen_applied) return ISLE_GAME_INVALID_STATE;
        isle::scene::graphics::SourceScreen config{screen->width,screen->height,screen->flags,
            {screen->framebuffer_identity[0],screen->framebuffer_identity[1],screen->framebuffer_identity[2]}};
        if (!self.frame->configure_source_screen(config,self.depth)) return ISLE_GAME_INVALID_STATE;
        self.screen_applied=true; return ISLE_GAME_OK;
    });
}
IsleGameResult isle_beach_lifecycle_native_fx_bank_count(IsleBeachLifecycleSource* s,size_t* out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    return Access::invoke(s,[&](Impl& self) {
        if (Access::setup(self)!=ISLE_GAME_OK || !self.actors) return ISLE_GAME_INVALID_STATE;
        *out=isle::menu::EffectRuntime::bank_count; return ISLE_GAME_OK;
    });
}
IsleGameResult isle_beach_lifecycle_native_fx_reset_bank(IsleBeachLifecycleSource* s,size_t bank) {
    return Access::invoke(s,[&](Impl& self) {
        return Access::setup(self)==ISLE_GAME_OK && self.actors &&
            self.actors->effects().reset_bank(bank)?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_beach_lifecycle_native_reset_photo_roll(IsleBeachLifecycleSource* s) {
    return Access::invoke(s,[&](Impl& self) {
        return Access::setup(self)==ISLE_GAME_OK?self.photos.reset():ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_beach_lifecycle_native_create_helper(IsleBeachLifecycleSource* s) {
    return Access::invoke(s,[&](Impl& self) {
        if (Access::setup(self)!=ISLE_GAME_OK || self.helper) return ISLE_GAME_INVALID_STATE;
        self.helper=self.services->object(0,0,0x80000000u);
        if (!self.helper || !self.services->set_lifecycle(self.helper,{Impl::helper_retiring,Impl::helper_join,&self}))
            return ISLE_GAME_INVALID_STATE;
        self.helper_observed=true;
        if (!self.services->source_owner_register(self.helper,self.helper_fields)) return ISLE_GAME_INVALID_STATE;
        self.helper_registered=true;
        return isle_game_object_set_source_update(self.services->game,self.helper,isle_beach_lifecycle_func_802C4430);
    });
}
IsleGameResult isle_beach_lifecycle_native_pokemons_init(IsleBeachLifecycleSource* s) {
    return Access::invoke(s,[&](Impl& self) {
        if (Access::setup(self)!=ISLE_GAME_OK || !self.helper || !self.actors || self.collisions)
            return ISLE_GAME_INVALID_STATE;
        self.collisions=std::make_unique<isle::actor::CollisionPass>(*self.services,*self.actors,self.helper);
        return ISLE_GAME_OK;
    });
}
IsleGameResult isle_beach_lifecycle_native_height_map(IsleBeachLifecycleSource* s,uint32_t identity) {
    return Access::invoke(s,[&](Impl& self) {
        if (Access::setup(self)!=ISLE_GAME_OK || identity!=0x80318f00u ||
            !self.world->resources().height_map || self.height_map) return ISLE_GAME_INVALID_STATE;
        self.height_map=true; return ISLE_GAME_OK;
    });
}
IsleGameResult isle_beach_lifecycle_native_create_world(IsleBeachLifecycleSource* s,uint32_t identity,
    int32_t sky,int32_t first,int32_t last,int32_t link,int32_t render,IsleBeachLifecycleWorldPair add,
    IsleBeachLifecycleWorldSingle remove,IsleBeachLifecycleWorldPair change,void* user) {
    return Access::invoke(s,[&](Impl& self) {
        if (Access::setup(self)!=ISLE_GAME_OK || !self.height_map || self.world_published ||
            identity!=0x8011b914u || sky!=99 || first!=100 || last!=126 || link!=9 || render!=3 ||
            add!=isle_beach_lifecycle_world_add || remove!=isle_beach_lifecycle_world_remove ||
            change!=isle_beach_lifecycle_world_change || user!=s) return ISLE_GAME_INVALID_ARGUMENT;
        self.state.background_rgb=self.world_resources.setup().background_rgb;
        self.world->publish(); self.world_published=true; return ISLE_GAME_OK;
    });
}
IsleGameResult isle_beach_lifecycle_native_sky_follow(IsleBeachLifecycleSource* s) {
    return Access::invoke(s,[&](Impl& self) {
        if (Access::setup(self)!=ISLE_GAME_OK || !self.world_published) return ISLE_GAME_INVALID_STATE;
        self.sky_follow=true;
        if (s->player->camera) self.world->follow_camera(s->player->camera);
        return ISLE_GAME_OK;
    });
}
IsleGameResult isle_beach_lifecycle_native_pokemon_registry(IsleBeachLifecycleSource* s,int32_t minimum,int32_t maximum,int32_t link,int32_t arg3) {
    return Access::invoke(s,[&](Impl& self) {
        return Access::setup(self)==ISLE_GAME_OK && self.actors &&
            self.actors->configure_registry(minimum,maximum,link,arg3)?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_beach_lifecycle_native_background(IsleBeachLifecycleSource* s,uint8_t* r,uint8_t* g,uint8_t* b) {
    if (!r || !g || !b) return ISLE_GAME_INVALID_ARGUMENT;
    return Access::invoke(s,[&](Impl& self) {
        if (!self.world_published) return ISLE_GAME_INVALID_STATE;
        const auto& rgb=self.state.background_rgb;
        *r=rgb[0]; *g=rgb[1]; *b=rgb[2]; return ISLE_GAME_OK;
    });
}
IsleGameResult isle_beach_lifecycle_native_create_cameras(IsleBeachLifecycleSource* s,uint32_t color) {
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
        if (self.sky_follow) self.world->follow_camera(s->player->camera);
        return ISLE_GAME_OK;
    });
}
IsleGameResult isle_beach_lifecycle_native_idle_enabled(IsleBeachLifecycleSource* s,int32_t* out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    return Access::invoke(s,[&](Impl& self) { *out=self.state.scene_latch(); return ISLE_GAME_OK; });
}
IsleGameResult isle_beach_lifecycle_native_load(IsleBeachLifecycleSource* s,IsleBeachLifecycleAssetId id,
    uint32_t first,uint32_t end,const IsleBeachLifecycleAsset** out) {
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
IsleGameResult isle_beach_lifecycle_native_idle_script(IsleBeachLifecycleSource* s,const IsleBeachLifecycleAsset* asset) {
    return Access::invoke(s,[&](Impl& self) {
        if (Access::setup(self)!=ISLE_GAME_OK || asset!=&self.assets[ISLE_BEACH_LIFECYCLE_IDLE_SCRIPT] || !asset->loaded)
            return ISLE_GAME_INVALID_ARGUMENT;
        self.state.set_idle_script(&self.photos.beach_idle_script()); return ISLE_GAME_OK;
    });
}
IsleGameResult isle_beach_lifecycle_native_ui_bind(IsleBeachLifecycleSource* s,const IsleBeachLifecycleUiCallbacks* callbacks) {
    return Access::invoke(s,[&](Impl& self) {
        if (Access::setup(self)!=ISLE_GAME_OK || !callbacks || callbacks->user!=s ||
            callbacks->exit_block!=isle_beach_lifecycle_exit_block || callbacks->movement!=isle_beach_lifecycle_movement ||
            callbacks->collide_item!=isle_beach_lifecycle_collide_item || callbacks->update_items ||
            callbacks->update_items_kind || !self.camera || self.ending) return ISLE_GAME_INVALID_ARGUMENT;
        const auto result=self.root->bind({*self.world,*self.actors,*self.items,*self.ui,*self.bump,
            *self.renderer,self.photos.source(),self.camera.get()});
        if (result!=ISLE_GAME_OK) return result;
        self.ending=std::make_unique<isle::beach::Ending>(*self.services,*self.actors,
            isle::beach::Ending::Domains{*self.root,*self.cart,*self.camera,self.cart_resources,
                *self.items,*self.ui,self.music->source(),self.snapshot});
        return ISLE_GAME_OK;
    });
}
IsleGameResult isle_beach_lifecycle_native_ui_initialized(IsleBeachLifecycleSource* s,IsleGameObject* object) {
    return Access::invoke(s,[&](Impl& self) {
        return Access::setup(self)==ISLE_GAME_OK?self.root->complete_initialization(object):ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_beach_lifecycle_native_environment_init(IsleBeachLifecycleSource* s,
    const IsleBeachLifecycleEnvironment* values,size_t count) {
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
IsleGameResult isle_beach_lifecycle_native_fx_setup_bank(IsleBeachLifecycleSource* s,int32_t bank,
    const IsleBeachLifecycleAsset* script,const IsleBeachLifecycleAsset* sprites) {
    return Access::invoke(s,[&](Impl& self) {
        if (Access::setup(self)!=ISLE_GAME_OK || !script || !sprites) return ISLE_GAME_INVALID_ARGUMENT;
        const isle::menu::EffectBank* selected=nullptr;
        if (bank==3 && script==&self.assets[0] && sprites==&self.assets[1]) selected=self.banks[0];
        if (bank==0 && script==&self.assets[2] && sprites==&self.assets[3]) selected=self.banks[1];
        return selected && script->loaded && sprites->loaded && self.actors->effects().install_bank(*selected)?
            ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_beach_lifecycle_native_fx_camera(IsleBeachLifecycleSource* s,int32_t lists,int32_t particles,
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
IsleGameResult isle_beach_lifecycle_native_fx_effects(IsleBeachLifecycleSource* s,int32_t count,IsleGameObject** out) {
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
IsleGameResult isle_beach_lifecycle_native_fx_dither(IsleBeachLifecycleSource* s,uint32_t color,uint32_t alpha) {
    return Access::invoke(s,[&](Impl& self) {
        if (!self.actors || color>255 || alpha>255) return ISLE_GAME_INVALID_ARGUMENT;
        self.actors->effects().set_dither(static_cast<std::uint8_t>(color),static_cast<std::uint8_t>(alpha));
        return ISLE_GAME_OK;
    });
}
IsleGameResult isle_beach_lifecycle_native_detector_create(IsleBeachLifecycleSource* s) {
    return Access::invoke(s,[&](Impl& self) {
        if (Access::setup(self)!=ISLE_GAME_OK || self.photo || !self.collisions) return ISLE_GAME_INVALID_STATE;
        self.photo=std::make_unique<isle::player::Photo>(self.photos.source(),*s->player,*self.actors,*self.items,
            self.detection,self.photos.resources(),*self.world,0);
        self.photo_scene=std::make_unique<isle::actor::PhotoScene>(*self.services,*self.actors,self.helper,
            s->player->camera,self.detection,&self.photos.source(),self.collisions.get());
        self.controller=std::make_unique<isle::player::Controller>(*self.photo,self.detection);
        return self.renderer->bind_photos(*self.controller);
    });
}
IsleGameResult isle_beach_lifecycle_native_detector_enable(IsleBeachLifecycleSource* s) {
    return Access::invoke(s,[&](Impl& self) {
        return self.photo?isle_player_photo_PokemonDetector_Enable(&self.photos.source()):ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_beach_lifecycle_native_photo_count(IsleBeachLifecycleSource* s,int32_t* out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    return Access::invoke(s,[&](Impl& self) { *out=self.photos.count(); return ISLE_GAME_OK; });
}
IsleGameResult isle_beach_lifecycle_native_or_notice(IsleBeachLifecycleSource* s,uint32_t flags) {
    return Access::invoke(s,[&](Impl& self) { self.state.add_scene_notices(flags); return ISLE_GAME_OK; });
}
IsleGameResult isle_beach_lifecycle_native_current_block(IsleBeachLifecycleSource* s,IsleBeachLifecycleBlock** out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    *out=nullptr;
    return Access::invoke(s,[&](Impl& self) {
        if (!self.world || !self.world_published) return ISLE_GAME_INVALID_STATE;
        *out=source_block(self.world->current()); return ISLE_GAME_OK;
    });
}
IsleGameResult isle_beach_lifecycle_native_block_next(IsleBeachLifecycleSource* s,IsleBeachLifecycleBlock* value,IsleBeachLifecycleBlock** out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    *out=nullptr;
    return Access::invoke(s,[&](Impl& self) {
        const auto* block=block_cast(value);
        if (!self.valid_block(block)) return ISLE_GAME_INVALID_ARGUMENT;
        if (block->next && !self.valid_block(block->next)) return ISLE_GAME_INVALID_STATE;
        *out=source_block(block->next); return ISLE_GAME_OK;
    });
}
IsleGameResult isle_beach_lifecycle_native_block_position(IsleBeachLifecycleSource* s,IsleBeachLifecycleBlock* value,IsleGameVec3* out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    return Access::invoke(s,[&](Impl& self) {
        const auto* block=block_cast(value);
        if (!self.valid_block(block) || !block->resource) return ISLE_GAME_INVALID_ARGUMENT;
        const auto position=block->resource->metadata.world_position;
        *out={position.x,position.y,position.z}; return ISLE_GAME_OK;
    });
}
IsleGameResult isle_beach_lifecycle_native_displace_items(IsleBeachLifecycleSource* s,float x,float y,float z) {
    return Access::invoke(s,[&](Impl& self) {
        return self.items && self.items->displace({x,y,z})?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_beach_lifecycle_native_pokemon_add(IsleBeachLifecycleSource* s,IsleBeachLifecycleBlock* first,
    IsleBeachLifecycleBlock* second,uint32_t table) {
    return Access::invoke(s,[&](Impl& self) {
        auto* block=block_cast(first); auto* reference=block_cast(second);
        if (table!=0x802cbee4u || !self.actors) return ISLE_GAME_INVALID_ARGUMENT;
        if (!block) return ISLE_GAME_OK;
        if (!self.valid_block(block)) return ISLE_GAME_INVALID_ARGUMENT;
        if (!block->resource || block->resource->spawns.empty()) return ISLE_GAME_OK;
        if (!self.valid_block(reference) || !reference->resource) return ISLE_GAME_INVALID_ARGUMENT;
        const auto definitions=self.actor_resources.definitions();
        for (const auto& row:block->resource->spawns) {
            std::size_t selected=isle::actor::actor_no_index;
            for (std::size_t i=0;i<definitions.size();++i) {
                const auto& definition=definitions[i];
                if (definition.wire.guest_address>=table && definition.wire.guest_address<0x802cbff4u &&
                    definition.actor_id==static_cast<std::uint32_t>(row.id)) {
                    if (selected!=isle::actor::actor_no_index) return ISLE_GAME_INVALID_STATE;
                    selected=i;
                }
            }
            if (selected==isle::actor::actor_no_index) return ISLE_GAME_INVALID_STATE;
            const auto result=self.spawn(selected,*block,*reference,row,nullptr);
            if (result!=ISLE_GAME_OK) return result;
        }
        return ISLE_GAME_OK;
    });
}
IsleGameResult isle_beach_lifecycle_native_pokemon_change(IsleBeachLifecycleSource* s,IsleBeachLifecycleBlock* first,
    IsleBeachLifecycleBlock* second,uint32_t table) {
    return Access::invoke(s,[&](Impl& self) {
        if (table!=0x802cbee4u || !self.actors || !self.services) return ISLE_GAME_INVALID_ARGUMENT;
        // The original sends command19 before testing optional block pointers.
        if (!self.services->send_command_to_link(3,19,nullptr)) return ISLE_GAME_INVALID_STATE;
        auto* block=block_cast(first); auto* reference=block_cast(second);
        if (!block || !reference) return ISLE_GAME_OK;
        if (!self.valid_block(block) || !self.valid_block(reference)) return ISLE_GAME_INVALID_ARGUMENT;
        if (!reference->resource || !reference->previous || !reference->previous->resource) return ISLE_GAME_OK;
        std::array<isle::menu::animation::InstanceHandle,100> members{};
        std::size_t count=0;
        if (!self.actors->block_members(*block,members,count)) return ISLE_GAME_INVALID_STATE;
        const auto previous=reference->previous->resource->metadata.world_position;
        const auto current=reference->resource->metadata.world_position;
        for (std::size_t n=0;n<count;++n) {
            auto* actor=self.actors->source(members[n]);
            if (!actor) return ISLE_GAME_INVALID_STATE;
            const isle::actor::ActorDefinition* selected=nullptr;
            for (const auto& definition:self.actor_resources.definitions())
                if (definition.wire.guest_address>=table && definition.wire.guest_address<0x802cbff4u &&
                    definition.actor_id==static_cast<std::uint32_t>(actor->id)) { selected=&definition; break; }
            if (!selected || !selected->change_block_identity) continue;
            int policy;
            switch (selected->change_block_identity) {
                case 0x80363dbcu: policy=0; break;
                case 0x80363eb4u: policy=1; break;
                case 0x802c71e8u: policy=-1; break;
                case 0x80362d2cu: policy=2; break;
                default: return ISLE_GAME_INVALID_STATE;
            }
            const auto result=isle_meowth_source_call_change_block(self.actors->object(members[n]),
                previous.x,previous.y,previous.z,current.x,current.y,current.z,policy);
            if (result!=ISLE_GAME_OK) return result;
        }
        return ISLE_GAME_OK;
    });
}
IsleGameResult isle_beach_lifecycle_native_pokemon_remove(IsleBeachLifecycleSource* s,IsleBeachLifecycleBlock* value,uint32_t table) {
    return Access::invoke(s,[&](Impl& self) {
        auto* block=block_cast(value);
        if (table!=0x802cbee4u || !self.actors) return ISLE_GAME_INVALID_ARGUMENT;
        if (!block) return ISLE_GAME_OK;
        if (!self.valid_block(block)) return ISLE_GAME_INVALID_ARGUMENT;
        if (!block->resource) return ISLE_GAME_OK;
        std::array<isle::menu::animation::InstanceHandle,100> members{};
        std::size_t count=0;
        if (!self.actors->block_members(*block,members,count)) return ISLE_GAME_INVALID_STATE;
        for (std::size_t n=0;n<count;++n) {
            const auto* actor=self.actors->source(members[n]);
            if (!actor) return ISLE_GAME_INVALID_STATE;
            for (const auto& definition:self.actor_resources.definitions()) {
                if (definition.wire.guest_address<table || definition.wire.guest_address>=0x802cbff4u ||
                    definition.actor_id!=static_cast<std::uint32_t>(actor->id)) continue;
                if (definition.remove_identity) {
                    if (definition.remove_identity!=0x80364280u || !self.actors->request_cleanup(members[n]))
                        return ISLE_GAME_INVALID_STATE;
                }
                break;
            }
        }
        return ISLE_GAME_OK;
    });
}
IsleGameResult isle_beach_lifecycle_native_model(IsleBeachLifecycleSource* s,IsleGameObject* object,IsleGameModel** out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    *out=nullptr;
    return Access::invoke(s,[&](Impl& self) {
        if (!object || !self.services || !s->player) return ISLE_GAME_INVALID_ARGUMENT;
        IsleGameModel* model=nullptr;
        if (object==s->player->object) model=s->player->model;
        if (!model && self.actors) for (const auto& binding:self.actors->render_bindings())
            if (binding.model.object==object) { model=binding.model.model; break; }
        if (!model && self.items) model=self.items->native_model(self.items->instance(object));
        if (!model && self.cart) model=self.cart->native_model(self.cart->instance(object));
        if (!model) return ISLE_GAME_INVALID_ARGUMENT;
        const auto result=isle_game_model_validate_binding(self.services->game,object,model);
        if (result==ISLE_GAME_OK) *out=model;
        return result;
    });
}
IsleGameResult isle_beach_lifecycle_native_model_position(IsleBeachLifecycleSource* s,IsleGameModel* model,int32_t transformed,IsleGameVec3* out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    return Access::invoke(s,[&](Impl& self) {
        if (!self.services || !model || (transformed!=0 && transformed!=1)) return ISLE_GAME_INVALID_ARGUMENT;
        if (transformed) {
            const IsleGameModelPlacement* placement=nullptr;
            const auto result=isle_game_model_placement(self.services->game,model,&placement);
            if (result!=ISLE_GAME_OK) return result;
            *out=placement->transform.position; return ISLE_GAME_OK;
        }
        const IsleGameModelNode* nodes=nullptr; std::size_t count=0;
        const auto result=isle_game_model_peek_nodes(self.services->game,model,&nodes,&count);
        if (result!=ISLE_GAME_OK) return result;
        if (!count) return ISLE_GAME_INVALID_STATE;
        *out=nodes[0].transform.position; return ISLE_GAME_OK;
    });
}
IsleGameResult isle_beach_lifecycle_native_set_position_component(IsleBeachLifecycleSource* s,IsleGameModel* model,uint32_t axis,float value) {
    return Access::invoke(s,[&](Impl& self) {
        if (!self.services || !self.actors || !model || axis>2) return ISLE_GAME_INVALID_ARGUMENT;
        const IsleGameModelPlacement* placement=nullptr;
        const auto result=isle_game_model_placement(self.services->game,model,&placement);
        if (result!=ISLE_GAME_OK) return result;
        auto transform=placement->transform;
        if (axis==0) transform.position.x=value;
        if (axis==1) transform.position.y=value;
        if (axis==2) transform.position.z=value;
        for (const auto& binding:self.actors->render_bindings())
            if (binding.model.model==model)
                return self.actors->set_placement(binding.model.state.instance,transform)?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
        return ISLE_GAME_INVALID_ARGUMENT;
    });
}
IsleGameResult isle_beach_lifecycle_native_camera_clip(IsleBeachLifecycleSource* s,uint32_t identity,const IslePlayerCameraAnimationClip** out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    *out=nullptr;
    return Access::invoke(s,[&](Impl& self) {
        if (!self.camera) return ISLE_GAME_INVALID_STATE;
        if (identity==self.cart_resources.intro().camera_identity) *out=&self.intro_camera;
        else if (identity==self.cart_resources.outro().camera_identity) *out=&self.outro_camera;
        return *out?ISLE_GAME_OK:ISLE_GAME_INVALID_ARGUMENT;
    });
}
IsleGameResult isle_beach_lifecycle_native_cart_model_clip(IsleBeachLifecycleSource* s,uint32_t identity,const IslePlayerCartModelAnimation** out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    *out=nullptr;
    return Access::invoke(s,[&](Impl& self) {
        if (!self.cart) return ISLE_GAME_INVALID_STATE;
        if (identity==self.cart_resources.intro().model_animation_table.guest_address) *out=self.cart->intro_model();
        else if (identity==self.cart_resources.outro().model_animation_table.guest_address) *out=self.cart->outro_model();
        return *out?ISLE_GAME_OK:ISLE_GAME_INVALID_ARGUMENT;
    });
}
IsleGameResult isle_beach_lifecycle_native_cart_material_clip(IsleBeachLifecycleSource* s,uint32_t identity,const IslePlayerCartMaterialAnimation** out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    *out=nullptr;
    return Access::invoke(s,[&](Impl& self) {
        if (!self.cart) return ISLE_GAME_INVALID_STATE;
        if (identity==self.cart_resources.intro().material_animation_table.guest_address) *out=self.cart->intro_material();
        else if (identity==self.cart_resources.outro().material_animation_table.guest_address) *out=self.cart->outro_material();
        return *out?ISLE_GAME_OK:ISLE_GAME_INVALID_ARGUMENT;
    });
}
IsleGameResult isle_beach_lifecycle_native_camera_speed(IsleBeachLifecycleSource* s,IsleGameCamera* camera,float speed) {
    return Access::invoke(s,[&](Impl& self) {
        return self.camera && s->player && camera==s->player->camera && self.camera->set_speed(speed)?
            ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_beach_lifecycle_native_sound_identity(IsleBeachLifecycleSource* s,uint32_t handle,int32_t* out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    return Access::invoke(s,[&](Impl& self) { *out=self.audio.playing_sound(handle); return ISLE_GAME_OK; });
}
IsleGameResult isle_beach_lifecycle_native_create_effect(IsleBeachLifecycleSource* s,int32_t bank,int32_t script,IsleBeachLifecycleEffect** out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    *out=nullptr;
    return Access::invoke(s,[&](Impl& self) {
        if (!self.actors || bank<0 || bank>UINT16_MAX || script<0) return ISLE_GAME_INVALID_ARGUMENT;
        const auto made=self.actors->effects().create_effect(static_cast<std::uint16_t>(bank),static_cast<std::size_t>(script));
        if (!made.status) return ISLE_GAME_INVALID_STATE;
        if (made.spawned) {
            if (made.handle.slot>=self.effects.size()) return ISLE_GAME_INVALID_STATE;
            auto& effect=self.effects[made.handle.slot]; effect.handle=made.handle; *out=&effect;
        }
        return ISLE_GAME_OK;
    });
}
IsleGameResult isle_beach_lifecycle_native_attach_effect(IsleBeachLifecycleSource* s,IsleBeachLifecycleEffect* effect,const IslePlayerCartNode* node) {
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
IsleGameResult isle_beach_lifecycle_native_store_cart(IsleBeachLifecycleSource* s,IsleGameObject* object) {
    return Access::invoke(s,[&](Impl& self) {
        if (!self.helper_registered || !self.cart || (object && !self.cart->instance(object)))
            return ISLE_GAME_INVALID_ARGUMENT;
        self.intro_instance=object?self.cart->instance(object):isle::menu::animation::InstanceHandle{};
        return self.services->source_reference_assign(self.helper,&s->D_beach_802CC0E4,object)?
            ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_beach_lifecycle_native_retire_intro_cart(IsleBeachLifecycleSource* s,IsleGameObject* object) {
    return Access::invoke(s,[&](Impl& self) {
        if (!object || object!=s->D_beach_802CC0E4 || !self.cart) return ISLE_GAME_INVALID_ARGUMENT;
        return self.cart->request_retire(object)?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_beach_lifecycle_native_release_cart_reference(IsleBeachLifecycleSource* s) {
    return Access::invoke(s,[&](Impl& self) {
        if (!self.services || isle_game_require_idle(self.services->game)!=ISLE_GAME_OK ||
            (self.frame && !self.frame->quiescent()) || (self.actors && self.actors->effects().effect_count()))
            return ISLE_GAME_INVALID_STATE;
        if (!self.helper_registered) return s->D_beach_802CC0E4?ISLE_GAME_INVALID_STATE:ISLE_GAME_OK;
        if (s->D_beach_802CC0E4 && !self.services->source_owner_joined(s->D_beach_802CC0E4))
            return ISLE_GAME_INVALID_STATE;
        if (!self.services->source_reference_assign(self.helper,&s->D_beach_802CC0E4,nullptr))
            return ISLE_GAME_INVALID_STATE;
        if (!self.helper_joined) return ISLE_GAME_INVALID_STATE;
        if (!self.services->source_owner_release(self.helper)) return ISLE_GAME_INVALID_STATE;
        self.helper_registered=false; return ISLE_GAME_OK;
    });
}
IsleGameResult isle_beach_lifecycle_native_release_fx_references(IsleBeachLifecycleSource* s) {
    return Access::invoke(s,[&](Impl& self) {
        if (!self.services || isle_game_require_idle(self.services->game)!=ISLE_GAME_OK || !self.renderer)
            return ISLE_GAME_INVALID_STATE;
        const auto result=self.renderer->service_main_removal();
        if (result!=ISLE_GAME_OK) return result;
        return s->player && !s->player->D_80382D10_523120 && !s->player->D_80382D14_523124?
            ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_beach_lifecycle_native_add_one(IsleBeachLifecycleSource* s,IsleBeachLifecycleBlock* first,
    IsleBeachLifecycleBlock* second,const IsleBeachLifecycleSpawn* spawn,
    const IsleBeachLifecyclePokemonDef* definition,IsleGameObject** out) {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    *out=nullptr;
    return Access::invoke(s,[&](Impl& self) {
        auto* block=block_cast(first); auto* reference=block_cast(second);
        if (!block || !definition) return ISLE_GAME_OK;
        if (!spawn || definition!=&s->magikarp || spawn->id!=129 || !self.valid_block(block) ||
            !self.valid_block(reference) || !block->resource || !reference->resource || !self.actors)
            return ISLE_GAME_INVALID_ARGUMENT;
        const auto definitions=self.actor_resources.definitions();
        for (std::size_t n=0;n<definitions.size();++n) {
            const auto& actual=definitions[n];
            if (actual.wire.guest_address!=0x802cbff4u) continue;
            if (actual.actor_id!=definition->id || actual.factory_identity!=definition->initialize_identity ||
                actual.change_block_identity!=definition->update_identity || actual.remove_identity!=definition->remove_identity)
                return ISLE_GAME_INVALID_STATE;
            const auto made=self.actors->add_one_magikarp_unwritten(n,block,reference,
                spawn->translation,spawn->euler,spawn->scale);
            if (!made) return self.fail(ISLE_GAME_INVALID_STATE,made.status.detail);
            *out=made.object; return ISLE_GAME_OK;
        }
        return ISLE_GAME_INVALID_STATE;
    });
}
} // extern "C"

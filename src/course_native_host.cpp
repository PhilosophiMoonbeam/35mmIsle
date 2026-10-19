#include "course_native_host.hpp"
#include "tunnel_actor_native_state.hpp"
#include "volcano_actor_native.hpp"
#include "host_clock.hpp"
#include "game_audio.hpp"
#include "player_renderer.hpp"
#include "player_commands.hpp"
#include "player_ui.hpp"
#include "player_bump.hpp"
#include "player_cart.hpp"
#include "player_camera_animation.hpp"
#include "player_cart_resources.hpp"
#include "player_ui_resources.hpp"
#include "actor_photo_scene.hpp"
#include "actor_interaction_services.hpp"
#include "actor_owner_source.h"
#include "world_producer.hpp"
#include "item_owner.hpp"
#include "course_model_producer.hpp"
#include "screen_transition_resources.hpp"
#include "scene_storage.hpp"
#include "scene_services.hpp"
#include "menu_scene_bridge.h"
#include <algorithm>
#include <stdexcept>
#include <vector>

namespace isle::course {
namespace {
namespace g = scene::graphics;
namespace a = menu::animation;
constexpr std::size_t plane = 320*240*2;
constexpr world::ProducerLayout world_layout{{0x200000,0x10000},{0x210000,0x10000},{0x220000,0x10000}};
constexpr actor::ProducerLayout actor_layout{{0x400000,0x80000},{0x480000,0x80000},{0x500000,0x100000},{{{0x3b5000,plane},{0x3da800,plane}}}};
constexpr menu::model::PacketLayout item_layout{{0x230000,0x10000},{0x240000,0x10000},{0x250000,0x10000},{{{0x3b5000,plane},{0x3da800,plane}}}};
constexpr player::cart::ProducerLayout cart_layout{{0x260000,0x10000},{0x270000,0x10000},{0x2a0000,0x10000},{{{0x3b5000,plane},{0x3da800,plane}}}};
constexpr focus::CaptureLayout capture_layout{{0x680000,0x50},{0x680080,0x80},{0x680100,0xa00}};
constexpr menu::RamRange fx_commands{0x680c00,0x5400};
constexpr sprite::RamRange lanes{0x690000,0x40000}, sprite_commands{0x780000,0x50000}, message_commands{0x6d0000,0x30000};
constexpr g::Layout frame_layout{{0x626000,0x2000},{0x630000,0x10000},{0x640000,0x40000},0x600000};
std::uint32_t physical(std::uint32_t value) noexcept { return value & 0x1fffffffu; }
void require(bool value, const char* text) { if (!value) throw std::runtime_error(text); }
void native(IsleGameResult value, const char* text) { require(value == ISLE_GAME_OK,text); }
a::SlotCapacity maximum_capacity(std::span<const menu::Model> models) {
    a::SlotCapacity maximum{};
    for (const auto& model : models) {
        const auto value = a::slot_capacity(model);
        maximum.nodes = std::max(maximum.nodes,value.nodes);
        maximum.material_attachments = std::max(maximum.material_attachments,value.material_attachments);
    }
    return maximum;
}
bool explicit_execution(a::ExecutionCapacity value) noexcept {
    return value.commands != menu::no_index && value.events != menu::no_index;
}
template<class T>
std::span<const T> compose_sprites(std::span<const T> common, std::span<const T> course,
    std::vector<T>& storage) {
    if (course.empty()) return common;
    storage.reserve(common.size()+course.size());
    storage.insert(storage.end(),common.begin(),common.end());
    storage.insert(storage.end(),course.begin(),course.end());
    return storage;
}
}
struct NativeHost::Impl {
    NativeHost& owner;
    const rom::Snapshot& snapshot;
    application::Manager& manager;
    game::State& state;
    game_input::Service& input;
    game_audio::Session& audio;
    host::GtlClock& clock;
    application::Photos& photos;
    world::ModuleState& module;
    NativeDescription description;
    const menu::RamRange joined_lists;
    bool cpu = false, prepared = false, initialized = false, exiting = false;
    bool cancelling = false, closing = false, retired = false, detach_authorized = false;
    bool detached = false, references_released = false, scene_cleared = false;
    bool world_published = false, follows_camera = false, screen_applied = false;
    bool collision_bound = false, near_disabled = false, audio_stopped = false;
    IsleGameResult error = ISLE_GAME_OK;
    std::array<char,512> message{};
    world::Resources world_resources;
    actor::Resources actor_resources;
    actor::BehaviorResources behavior_resources;
    items::Resources item_resources;
    player::CartResources cart_resources;
    player::UiResources ui_resources;
    environment_sound::Resources sound_resources;
    menu::EffectResources effect_resources;
    photo::LightingInputs lighting;
    graphics::SourcePrograms programs;
    screen_transition::Resources transition_resources;
    std::array<const menu::EffectBank*,2> graphics_banks;
    menu::EffectGraphicsResources effect_graphics;
    std::vector<scene::SpriteResource> sprite_resource_storage;
    std::vector<const IsleSprite*> sprite_root_storage;
    std::vector<sprite::BankMapping> sprite_bank_storage;
    std::span<const scene::SpriteResource> sprite_resources;
    std::span<const IsleSprite* const> sprite_roots;
    std::span<const sprite::BankMapping> sprite_banks;
    sprite::Producer sprites;
    std::vector<a::SlotCapacity> actor_capacities, item_capacities, cart_capacities;
    std::unique_ptr<TunnelActorNativeState> tunnel_actors;
    std::unique_ptr<scene::Storage> storage;
    std::unique_ptr<scene::Services> services;
    std::unique_ptr<player::Root> root;
    std::unique_ptr<player::commands::Binding> commands;
    std::unique_ptr<player::Ui> ui;
    std::unique_ptr<player::BumpOwner> bump;
    focus::Detector detector;
    focus::RegionCopies copies{capture_layout};
    actor::DetectorBinding detection{copies,detector};
    std::unique_ptr<g::Frame> frame;
    const g::DepthReservation* depth = nullptr;
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
    std::unique_ptr<actor::CollisionPass> collisions;
    std::unique_ptr<player::Photo> photo;
    std::unique_ptr<actor::PhotoScene> photo_scene;
    std::unique_ptr<player::Controller> controller;
    std::unique_ptr<world::Producer> worlds;
    std::unique_ptr<actor::Producer> actor_producer;
    std::unique_ptr<menu::model::PacketProducer> item_producer;
    std::unique_ptr<player::cart::Producer> carts;
    std::unique_ptr<ItemComposition> composition;
    std::unique_ptr<ModelProducer> models;
    std::vector<scene::RenderOrderEntry> cart_order;
    struct Helper {
        Impl* owner = nullptr;
        IsleGameObject* object = nullptr;
        bool observed = false, registered = false, joined = false;
        std::span<IsleGameObject** const> fields;
    };
    // Sized exactly once before publication; callback userdata never moves.
    std::vector<Helper> helpers;
    Helper input_owner;
    Helper* references = nullptr;

    Impl(NativeHost& host, const rom::Snapshot& rom, application::Manager& m,
        game::State& s, game_input::Service& i, game_audio::Session& au,
        host::GtlClock& cl, application::Photos& ph, world::ModuleState& mod,
        NativeDescription desc, bool require_cpu)
        : owner(host), snapshot(rom), manager(m), state(s), input(i), audio(au), clock(cl),
          photos(ph), module(mod), description(desc),
          joined_lists{0x686000,source_join_bytes(desc.capacity.render_order)}, cpu(require_cpu),
          world_resources(rom,desc.course_id),
          actor_resources(rom,desc.course_id,{world_resources.setup().fog_rgb,world_resources.setup().fog_min,world_resources.setup().fog_max}),
          behavior_resources(rom,desc.course_id),
          item_resources(rom,world_resources.setup().fog_rgb,world_resources.setup().fog_min,world_resources.setup().fog_max),
          cart_resources(rom,desc.course_id,{world_resources.setup().fog_rgb,world_resources.setup().fog_min,world_resources.setup().fog_max}),
          ui_resources(rom), sound_resources(rom,desc.course_id), effect_resources(rom),
          lighting(rom), programs(rom), transition_resources(rom),
          graphics_banks{&effect_resources.bank(menu::EffectGroup::common),&effect_resources.bank(desc.effects)},
          effect_graphics(rom,graphics_banks,{0x700000,0x80000}),
          sprite_resources(compose_sprites<scene::SpriteResource>(ui_resources.resources(),desc.extra_sprites,sprite_resource_storage)),
          sprite_roots(compose_sprites<const IsleSprite*>(ui_resources.roots(),desc.extra_sprite_roots,sprite_root_storage)),
          sprite_banks(compose_sprites<sprite::BankMapping>(ui_resources.banks(),desc.extra_sprite_banks,sprite_bank_storage)),
          sprites(sprite_roots,sprite_banks),
          actor_capacities(desc.capacity.actors), item_capacities(desc.capacity.items),
          cart_capacities(desc.capacity.carts), cart_order(desc.capacity.render_order), helpers(desc.capacity.helpers) {
        const auto& cap = description.capacity;
        require(desc.course_id >= 1 && desc.course_id <= 5 && desc.player.level_id == desc.course_id,
            "shared native host requires explicit selected course and canonical Player course");
        require(cap.arena_bytes && cap.pools.objects && cap.pools.processes && cap.pools.cameras &&
            cap.command_nodes && cap.actors && cap.items && cap.carts && cap.helpers &&
            cap.render_order && cap.sprite_draws,"missing cold native physical capacity");
        require(joined_lists.size <= lanes.address-joined_lists.offset,
            "native course source join exceeds its disjoint RAM window");
        require(explicit_execution(cap.world_execution) && explicit_execution(cap.actor_execution) &&
            explicit_execution(cap.item_execution) && explicit_execution(cap.cart_execution) &&
            explicit_execution(cap.camera_execution),"native execution envelopes must be explicit cold capacities");
        require(desc.player.end_level && desc.player.pause && desc.player.movement.exit_block &&
            desc.world.add && desc.world.remove && desc.world.change && desc.input.callback &&
            !desc.callbacks.empty(),"incomplete typed course callback closure");
        require(desc.extra_sprites.size()==desc.extra_sprite_roots.size(),
            "course sprite resource/root order mismatch");
        for (std::size_t n=0;n<desc.extra_sprites.size();++n)
            require(desc.extra_sprites[n].sprite.view()==desc.extra_sprite_roots[n],
                "course sprite root is not its admitted resource");
        for (auto& helper : helpers) helper.owner = this;
        input_owner.owner = this;
        if (desc.course_id==1) tunnel_actors=std::make_unique<TunnelActorNativeState>(rom);
    }
    static bool add(void* user, world::Block& first, world::Block& second) noexcept {
        auto& self = *static_cast<Impl*>(user);
        return !self.closing && self.description.world.add(self.description.world.user,first,second);
    }
    static bool remove(void* user, world::Block& block) noexcept {
        auto& self = *static_cast<Impl*>(user);
        if (self.cancelling) return !self.actors || bool(self.actors->remove_block(block));
        return self.description.world.remove(self.description.world.user,block);
    }
    static bool change(void* user, world::Block& first, world::Block& second) noexcept {
        auto& self = *static_cast<Impl*>(user);
        return !self.closing && self.description.world.change(self.description.world.user,first,second);
    }
    static bool shift_camera(void* user, world::Vec3 delta) noexcept {
        auto& self = *static_cast<Impl*>(user);
        return self.root && isle_player_root_invoke_bind_camera_next_block(&self.root->source(),delta.x,delta.y,delta.z) == ISLE_GAME_OK;
    }
    static bool shift_effects(void* user, world::Vec3 delta) noexcept {
        auto& self = *static_cast<Impl*>(user);
        if (!self.actors) return false;
        self.actors->effects().translate({delta.x,delta.y,delta.z});
        return bool(self.actors->effects().status());
    }
    static void helper_joined(IsleGameObject* object, void* user) noexcept {
        auto& helper = *static_cast<Helper*>(user);
        if (helper.object != object) std::terminate();
        helper.joined = true;
    }
    static bool prepare_render(void* user, std::span<std::uint32_t> ram, std::uint64_t epoch,
        const scene::render::Pass& pass, scene::render::Prepared& output) {
        auto& self = *static_cast<Impl*>(user);
        if (!self.models || self.copies.install(ram) != focus::CaptureStatus::ok) return false;
        self.actors->render_bindings();
        self.items->render_bindings();
        output = self.models->prepare(ram,epoch,pass);
        return true;
    }
    bool setup() const noexcept { return prepared && !initialized && !closing && services && isle_game_require_idle(services->game) == ISLE_GAME_OK; }
    void cold(actor::CourseContext context);
    void producers();
    IsleGameResult publish_helper(Helper& helper, std::uint32_t id, std::uint8_t link,
        std::uint32_t priority, IsleGameSourceCallback callback, std::span<IsleGameObject** const> fields) noexcept {
        helper.object = services->object(id,link,priority);
        if (!helper.object) return ISLE_GAME_EXHAUSTED;
        helper.fields = fields;
        if (!services->set_lifecycle(helper.object,{nullptr,helper_joined,&helper})) return ISLE_GAME_INVALID_STATE;
        helper.observed = true;
        if (!services->source_owner_register(helper.object,fields)) return ISLE_GAME_INVALID_STATE;
        helper.registered = true;
        return callback ? isle_game_object_set_source_update(services->game,helper.object,callback) : ISLE_GAME_OK;
    }
    bool join_helper(Helper& helper) noexcept {
        if (!helper.object) return true;
        if (!helper.joined) {
            if (helper.observed) services->erase(helper.object);
            else if (isle_game_delete_object(services->game,helper.object) == ISLE_GAME_OK) helper.joined = true;
        }
        if (!helper.joined) return false;
        if (helper.registered && !services->source_owner_release(helper.object)) return false;
        helper = Helper{this};
        return true;
    }
};

NativeHost::NativeHost(const rom::Snapshot& snapshot, application::Manager& manager,
    game::State& state, game_input::Service& input, game_audio::Session& audio,
    host::GtlClock& clock, application::Photos& photos, world::ModuleState& module,
    NativeDescription description, bool cpu)
    : impl_(std::make_unique<Impl>(*this,snapshot,manager,state,input,audio,clock,photos,module,description,cpu)) {}
NativeHost::~NativeHost() {
    if (impl_->storage || impl_->services || impl_->root) std::terminate();
}
IsleGameResult NativeHost::fail(IsleGameResult result, const char* text) noexcept {
    auto& self = *impl_;
    if (result != ISLE_GAME_OK && self.error == ISLE_GAME_OK) {
        self.error = result;
        if (text && text[0]) std::snprintf(self.message.data(),self.message.size(),"%s",text);
        else std::snprintf(self.message.data(),self.message.size(),"native course operation failed (%u)",static_cast<unsigned>(result));
    }
    return self.error;
}
void NativeHost::Impl::cold(actor::CourseContext context) {
    require(!storage && !prepared && !retired && context.course_id() == description.course_id,
        "native course preparation requires exact selected PREPARED context");
    context.require_live();
    storage = std::make_unique<scene::Storage>(description.capacity.arena_bytes);
    native(storage->create(),"native course Game/domain creation rejected");
    services = std::make_unique<scene::Services>(storage->game(),storage->arena(),
        scene::ResourceView{description.capacity.pools,sprite_resources},input,audio,state);
    require(!services->failed(),services->error());
    // Native envelope is separate from the unchanged Original SceneSetup recipe.
    // This returning reservation also freezes warm pool/command growth.
    native(services->reserve_native_capacity(description.capacity.command_nodes,
        description.capacity.model_blocks),"native fixed model/pool/command reservation rejected");
    root = std::make_unique<player::Root>(*services,ui_resources,description.player);
    commands = std::make_unique<player::commands::Binding>(*services,root->source());
    ui = std::make_unique<player::Ui>(ui_resources,*services,root->source(),sprites,0);
    bump = std::make_unique<player::BumpOwner>(*services,root->source());
    auto made = g::Frame::create(snapshot,{cpu,{16000000,4*1024*1024},frame_layout});
    require(bool(made),made.message.data());
    frame = std::move(made.frame);
    all_ranges = {world_layout.payloads,world_layout.lists,world_layout.materials,
        actor_layout.payloads,actor_layout.lists,actor_layout.materials,
        item_layout.payloads,item_layout.lists,item_layout.materials,
        cart_layout.payloads,cart_layout.lists,cart_layout.materials,fx_commands,joined_lists,
        {lanes.address,lanes.size},{sprite_commands.address,sprite_commands.size},
        {message_commands.address,message_commands.size},
        {screen_transition::Resources::workspace.address,screen_transition::Resources::workspace.size}};
    for (const auto range : copies.reservations()) all_ranges.push_back(range);
    for (const auto& span : lighting.installation_spans()) all_ranges.push_back({physical(span.guest_address),span.bytes.size()});
    for (const auto range : frame->reservations()) if (range.address) all_ranges.push_back({range.address,range.size});
    for (const auto range : actor_layout.framebuffers) all_ranges.push_back(range);
    for (const auto range : effect_graphics.reservations()) all_ranges.push_back(range);
    immutable.assign(sprites.assets().begin(),sprites.assets().end());
    const auto append = [&](auto spans) {
        for (const auto& span : spans) immutable.push_back({physical(span.guest_address),
            {reinterpret_cast<const std::uint8_t*>(span.bytes.data()),span.bytes.size()}});
    };
    append(world_resources.installation_spans()); append(actor_resources.installation_spans());
    append(item_resources.installation_spans()); append(cart_resources.installation_spans());
    for (const auto span : effect_graphics.installation_spans()) immutable.push_back(span);
    for (const auto span : transition_resources.immutable()) immutable.push_back(span);
    for (const auto range : all_ranges) {
        bool external = range.offset == lanes.address && range.size == lanes.size;
        for (const auto core : frame->reservations()) external |= range.offset == core.address && range.size == core.size;
        for (const auto fb : actor_layout.framebuffers) external |= range.offset == fb.offset && range.size == fb.size;
        for (const auto asset : effect_graphics.reservations()) external |= range.offset == asset.offset && range.size == asset.size;
        if (!external) reservations.push_back({static_cast<std::uint32_t>(range.offset),static_cast<std::uint32_t>(range.size)});
    }
    player::Renderer::Config render;
    render.resources = {1,&sprites,immutable,reservations,{0x3b5000,0x3da800},physical(lighting.pre_render_guest())};
    render.prepare = prepare_render; render.prepare_user = this; render.programs = &programs;
    render.effect_resources = &effect_graphics; render.sprite_capacity = description.capacity.sprite_draws;
    render.program_selection.disable_near_clipping = near_disabled;
    renderer = std::make_unique<player::Renderer>(*services,root->source(),lighting,render);
    environment = std::make_unique<environment_sound::Environment>(*services,storage->arena(),sound_resources);
    world = std::make_unique<world::World>(*services,world_resources,module,
        world::Callbacks{this,add,remove,change,shift_camera,shift_effects,nullptr},true,description.capacity.world_execution);
    std::fill(actor_capacities.begin(),actor_capacities.end(),maximum_capacity(actor_resources.models()));
    std::fill(item_capacities.begin(),item_capacities.end(),maximum_capacity(item_resources.models()));
    std::fill(cart_capacities.begin(),cart_capacities.end(),maximum_capacity(cart_resources.animation_resources().models));
    // Cold graphics closure is not a Source bank installation. Runtime owns its
    // eight initially NULL slots; original C resets/loads slots3 and0 in order.
    actors = std::make_unique<actor::Owner>(*services,actor_resources,context,*world,*environment,detector,
        root->source(),actor_capacities,std::span<const menu::EffectBank* const>{},
        description.capacity.actor_execution,description.retained_fields);
    items = std::make_unique<items::Owner>(*services,item_resources,*world,*actors,globals,
        root->source(),item_capacities,description.capacity.item_execution);
    cart = std::make_unique<player::cart::Owner>(*services,cart_resources,root->source(),cart_capacities,description.capacity.cart_execution);
    bindings = std::make_unique<actor::BehaviorBindings>(behavior_resources,description.callbacks,
        description.graphs,description.weighted_states,description.roots);
    native(root->bind({*world,*actors,*items,*ui,*bump,*renderer,photos.source(),nullptr}),
        "native cold canonical Root domain association rejected");
    interactions = std::make_unique<actor::InteractionServices>(*services,*actors,*items,*world,detector,*bindings,actor_resources,effect_resources);
    require(bool(actors->bind_interactions(*interactions)),"native prepared interaction association rejected");
    // No callbacks execute here; the exact typed Source remains PREPARED.
    native(publish_helper(input_owner,description.input.id,description.input.link,description.input.priority,nullptr,{}),
        "native original Input identity publication rejected");
    IsleGameProcess* process = nullptr;
    native(isle_game_add_source_process(services->game,input_owner.object,description.input.callback,
        ISLE_GAME_STACKFUL,description.input.process_priority,&process),"native original Input process publication rejected");
    prepared = true;
}
IsleGameResult NativeHost::prepare(actor::CourseContext context) noexcept {
    try { impl_->cold(context); return ISLE_GAME_OK; }
    catch (const std::exception& e) { return fail(ISLE_GAME_INVALID_STATE,e.what()); }
    catch (...) { return fail(ISLE_GAME_INVALID_STATE,"native cold composition threw"); }
}
void NativeHost::Impl::producers() {
    require(setup() && photo_scene && photo && controller && camera && world_published && screen_applied,
        "Original init did not publish complete native render/Photo closure");
    native(renderer->bind_ui(*ui,message_commands),renderer->message());
    const auto others = [&](std::array<menu::RamRange,3> own, bool framebuffer) {
        std::vector<menu::RamRange> result;
        result.reserve(all_ranges.size());
        for (const auto range : all_ranges) {
            bool skip = false;
            for (const auto mine : own) skip |= range.offset == mine.offset && range.size == mine.size;
            if (framebuffer) for (const auto fb : actor_layout.framebuffers) skip |= range.offset == fb.offset && range.size == fb.size;
            if (!skip) result.push_back(range);
        }
        return result;
    };
    const auto wo = others({world_layout.payloads,world_layout.lists,world_layout.materials},false);
    const auto ao = others({actor_layout.payloads,actor_layout.lists,actor_layout.materials},true);
    const auto io = others({item_layout.payloads,item_layout.lists,item_layout.materials},true);
    const auto co = others({cart_layout.payloads,cart_layout.lists,cart_layout.materials},true);
    worlds = std::make_unique<world::Producer>(services->game,root->source().camera,services->camera_basis(),world_resources,world->render_view(),world_layout,wo);
    actor_producer = std::make_unique<actor::Producer>(services->game,root->source().camera,services->camera_basis(),actor_resources,
        actor_capacities,actors->render_bindings(),actor_layout,ao,&detection,
        description.course_id==2 ? actor::SourceRender{&owner,volcano_actor_source_render} : actor::SourceRender{},
        description.source_vertices);
    item_producer = std::make_unique<menu::model::PacketProducer>(services->game,root->source().camera,services->camera_basis(),
        menu::model::PacketResources{item_resources.models(),item_resources.materials(),item_resources.installation_spans()},
        item_capacities,items->render_bindings(),item_layout,io);
    carts = std::make_unique<player::cart::Producer>(services->game,root->source().camera,services->camera_basis(),cart_resources,
        cart_capacities,cart->render_bindings(cart_order),cart_layout,co);
    composition = std::make_unique<ItemComposition>(ItemComposition{*services,*item_producer,description.capacity.render_order,
        carts.get(),photo_scene->prefix_object(),true});
    models = std::make_unique<ModelProducer>(*worlds,*actor_producer,joined_lists,composition.get());
    native(renderer->bind_cart(*cart,cart_order),renderer->message());
    native(renderer->bind_transition(transition_resources,services->screen_transition()),renderer->message());
    require(renderer->reserve(*frame),frame->message());
}
IsleGameResult NativeHost::finish_initialization() noexcept {
    try { impl_->producers(); impl_->initialized = true; return ISLE_GAME_OK; }
    catch (const std::exception& e) { return fail(ISLE_GAME_INVALID_STATE,e.what()); }
    catch (...) { return fail(ISLE_GAME_INVALID_STATE,"native producer composition threw"); }
}
IsleGameResult NativeHost::update(std::uint32_t raw_game_time) noexcept {
    auto& self = *impl_;
    if (self.error != ISLE_GAME_OK) return self.error;
    if (!self.initialized || self.closing || self.exiting || !self.frame || !self.frame->quiescent()) return ISLE_GAME_INVALID_STATE;
    self.services->raw_game_time = raw_game_time;
    auto result = self.root->publish_input();
    if (result == ISLE_GAME_OK) result = self.services->tick();
    // Real Source deletion retains Item identities/Runtime incarnations until
    // outside-OM collection, with the prior GPU frame excluded by update().
    if (self.items) {
        const auto collected = self.items->collect_joined();
        if (!collected && collected.error != items::OwnerError::lifetime && result == ISLE_GAME_OK) {
            self.services->fail(collected.detail);
            result = ISLE_GAME_INVALID_STATE;
        }
    }
    // Recycle only actual joined helpers after OM and previous GPU borrowers
    // have ended. The canonical global-reference owner stays entry-resident.
    for (auto& helper : self.helpers)
        if (&helper!=self.references && helper.object && helper.joined && helper.registered &&
            self.services->source_owner_collectable(helper.object) && !self.join_helper(helper) &&
            result==ISLE_GAME_OK) result=ISLE_GAME_INVALID_STATE;
    if (result == ISLE_GAME_OK && self.services->failed()) result = ISLE_GAME_INVALID_STATE;
    if (result == ISLE_GAME_OK) result = self.root->source().status;
    return result == ISLE_GAME_OK ? result : fail(result,
        isle_game_status(self.services->game)!=ISLE_GAME_OK ?
            isle_game_error(self.services->game) : self.services->error());
}
bool NativeHost::draw(g::FrameContext context) noexcept {
    auto& self = *impl_;
    if (self.error != ISLE_GAME_OK || !self.initialized || self.closing || self.exiting || !self.renderer) return false;
    // Presentation release is not retirement of CopyInfo/shutter consumption.
    if (self.controller && self.controller->has_pending(*self.frame)) {
        const auto result = self.renderer->release_completed();
        if (result != ISLE_GAME_OK) { fail(result,self.renderer->message()); return false; }
    }
    if (!self.renderer->begin(*self.frame,context)) {
        const auto result = self.renderer->status();
        fail(result == ISLE_GAME_OK ? ISLE_GAME_INVALID_STATE : result,self.renderer->message());
        return false;
    }
    return true;
}
g::Poll NativeHost::poll() noexcept {
    auto& self = *impl_;
    if (!self.renderer) return g::Poll::idle;
    const auto result = self.renderer->poll();
    if (result == g::Poll::failed) fail(ISLE_GAME_INVALID_STATE,self.renderer->message());
    return result;
}
bool NativeHost::graphics_busy() const noexcept { return impl_->frame && impl_->frame->busy(); }
g::Completed NativeHost::completed_frame() const noexcept { return impl_->frame ? impl_->frame->completed() : g::Completed{}; }
IsleGameResult NativeHost::release_completed() noexcept {
    auto& self = *impl_;
    if (!self.frame || !self.frame->quiescent() || !self.frame->completed()) return ISLE_GAME_INVALID_STATE;
    return self.controller && self.controller->has_pending(*self.frame) ? self.renderer->release_completed() : ISLE_GAME_OK;
}

bool NativeHost::retire_scene(bool cancelling, RetirementHooks hooks, SceneJoinReceipt& receipt) noexcept {
    auto& self = *impl_;
    if (self.retired) return receipt.joined();
    if (!hooks.owner || !hooks.release_references || !hooks.deactivate || !hooks.clear_scene) return false;
    if (self.services) {
        const auto native_status = isle_game_status(self.services->game);
        if (native_status != ISLE_GAME_OK)
            fail(native_status,isle_game_error(self.services->game));
        else if (self.services->failed())
            fail(ISLE_GAME_EXECUTION_FAILURE,self.services->error());
    }
    self.cancelling |= cancelling || self.error != ISLE_GAME_OK;
    self.closing = true;
    if (self.services) {
        if (isle_game_require_idle(self.services->game) != ISLE_GAME_OK || (self.frame && !self.frame->quiescent())) return false;
        if (self.controller && self.frame && self.controller->has_pending(*self.frame)) {
            const auto released = self.renderer->release_completed();
            if (released != ISLE_GAME_OK) return false;
        }
        if (self.photo && !self.photo->quiescent()) return false;
        // PhotoScene closes snapshot-owner/FIFO credits via the real
        // Photo::detach_snapshot_owner -> teardown_queue path, not count reset.
        if (self.photo_scene) { self.photo_scene->retire(); self.photo_scene.reset(); }
        if (self.controller && self.controller->queued_requests()) return false;
        self.models.reset(); self.composition.reset(); self.carts.reset(); self.item_producer.reset();
        self.actor_producer.reset(); self.worlds.reset(); self.collisions.reset();
        if (self.environment) self.environment->cleanup();
        self.services->stop_sound_ramps();
        if (self.cancelling && !self.audio_stopped) {
            const auto bgm = self.audio.stop_bgm();
            const auto sounds = self.audio.stop_sounds();
            if (bgm != audio::Error::none || sounds != audio::Error::none) {
                fail(ISLE_GAME_EXECUTION_FAILURE,"scene cancellation audio borrowers did not stop");
                return false;
            }
            self.audio_stopped = true;
        }
        if (!self.join_helper(self.input_owner)) return false;
        // Helper identity stays live while its registered canonical fields are
        // cleared; its updates/processes cannot execute once closing begins.
        for (auto& helper : self.helpers) if (helper.object && !helper.joined)
            self.services->end_processes(helper.object);
        if (self.actors) self.actors->effects().reset();
        if (self.actors && (self.services->failed() ||
                isle_game_status(self.services->game) != ISLE_GAME_OK)) {
            // A terminal Game cannot admit World's new Source cleanup processes.
            // Real erase/join unlinks block registrations first; semantic slots
            // may remain retained until the canonical field release below.
            const auto result = self.actors->reset();
            if (!result && result.error != actor::OwnerError::lifetime &&
                result.error != actor::OwnerError::services) return false;
        }
        if (self.root && self.root->retire_objects() != ISLE_GAME_OK) return false;
        // The real main-camera owner releases Player FX field credits. This
        // must precede World's/reference hooks, which require those joins.
        if (self.cancelling && self.renderer && self.renderer->abort_main() != ISLE_GAME_OK) return false;
        if (self.world && !self.world->retire()) return false;
        if (self.items) {
            const auto result = self.items->retire();
            if (!result && result.error != items::OwnerError::lifetime &&
                !(self.cancelling && result.error == items::OwnerError::services)) return false;
        }
        if (self.cart) {
            const auto result = self.cart->retire();
            if (!result && result.error != player::cart::OwnerError::lifetime &&
                !(self.cancelling && result.error == player::cart::OwnerError::services)) return false;
        }
        if (self.actors) {
            const auto result = self.actors->reset();
            if (!result && result.error != actor::OwnerError::lifetime &&
                !(self.cancelling && result.error == actor::OwnerError::services)) return false;
        }
        if (!self.references_released) {
            for (auto& helper : self.helpers) if (helper.registered) {
                for (auto** field : helper.fields) {
                    if (*field && !self.services->source_owner_joined(*field)) return false;
                    if (!self.services->source_reference_assign(helper.object,field,nullptr)) return false;
                }
            }
            // Exact per-course model/camera/installed-callback/cart credits; no
            // host copy, generic Source record, or inferred field enumeration.
            if (hooks.release_references(hooks.owner,*this) != ISLE_GAME_OK) return false;
            self.references_released = true;
        }
        if (self.actors) {
            const auto result = self.actors->reset();
            if (!result && !(self.cancelling && result.error == actor::OwnerError::services)) return false;
        }
        if (self.renderer && !self.cancelling && self.root && self.root->source().camera &&
            (self.renderer->remove_main() != ISLE_GAME_OK || self.renderer->service_main_removal() != ISLE_GAME_OK)) return false;
        if (self.root && (self.root->source().D_80382D10_523120 || self.root->source().D_80382D14_523124)) return false;
        if (self.root && self.root->collect_joined() != ISLE_GAME_OK) return false;
        if (self.items && !self.items->collect_joined()) return false;
        if (self.cart && !self.cart->collect_joined()) return false;
        if (self.items && self.collision_bound) {
            if (!self.items->set_extra_collision(self.services->game,nullptr,nullptr)) return false;
            self.collision_bound = false;
        }
        if (self.bump && self.bump->retire() != ISLE_GAME_OK) return false;
        if (self.ui && self.ui->retire() != ISLE_GAME_OK) return false;
        if (self.actors) {
            if (self.actors->particle_object()) self.services->erase(self.actors->particle_object());
            if (self.actors->effect_object()) self.services->erase(self.actors->effect_object());
        }
        if (self.renderer) {
            if (self.renderer->detach() != ISLE_GAME_OK) return false;
            self.renderer.reset();
        }
        if (self.actors && !self.actors->collect_joined()) return false;
        if (self.camera) { self.camera->retire(); self.camera.reset(); }
        if (self.frame) { if (!self.frame->retire()) return false; self.frame.reset(); self.depth = nullptr; }
        for (auto& helper : self.helpers) if (!self.join_helper(helper)) return false;
        self.references = nullptr;
        if (self.commands) { if (self.commands->detach() != ISLE_GAME_OK) return false; self.commands.reset(); }
        if (!self.services->retire_objects()) return false;
        // Actor detach proof deliberately precedes destruction of the genuine
        // canonical Root/Actor association; final completion proof does not.
        self.detach_authorized = true;
        if (!self.detached) {
            if (hooks.deactivate(hooks.owner,*this) != ISLE_GAME_OK) return false;
            self.detached = true;
        }
        self.controller.reset(); self.photo.reset();
        self.items.reset(); self.cart.reset(); self.bump.reset(); self.ui.reset(); self.actors.reset();
        self.interactions.reset(); self.bindings.reset();
        self.world.reset(); self.environment.reset();
        if (self.root) {
            const auto result = self.root->deactivate();
            if (self.root->source().native_root ||
                (result != ISLE_GAME_OK && result != isle_game_status(self.services->game))) return false;
            self.root.reset();
        }
        if (!self.scene_cleared) { hooks.clear_scene(hooks.owner); self.scene_cleared = true; }
        self.services.reset();
    }
    // Partial construction can have a bare Game with no Services/Root at all.
    if (!self.detached) {
        self.detach_authorized = true;
        if (hooks.deactivate(hooks.owner,*this) != ISLE_GAME_OK) return false;
        self.detached = true;
    }
    if (!self.scene_cleared) { hooks.clear_scene(hooks.owner); self.scene_cleared = true; }
    if (self.storage) {
        if (self.storage->destroy() != ISLE_GAME_OK) return false;
        self.storage.reset();
    }
    self.detach_authorized = false;
    self.retired = true;
    receipt.joined_ = true;
    return true;
}

IsleGameResult NativeHost::intervals(std::uint16_t update, std::uint16_t draw) noexcept {
    return impl_->clock.set_intervals(update,draw) == host::ClockError::none ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
}
IsleGameResult NativeHost::near_clipping(bool disabled) noexcept {
    auto& self = *impl_;
    const auto result = self.renderer ? self.renderer->set_near_clipping_disabled(disabled) : ISLE_GAME_OK;
    if (result == ISLE_GAME_OK) self.near_disabled = disabled;
    return result;
}
IsleGameResult NativeHost::exit_scene() noexcept {
    auto& self = *impl_;
    if (!self.services || self.closing) return ISLE_GAME_INVALID_STATE;
    self.exiting = true; self.clock.request_stop(); return ISLE_GAME_OK;
}
IsleGameResult NativeHost::destroy_world() noexcept {
    auto& self = *impl_;
    return self.world && self.world_published && self.world->request_retire() ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
}
IsleGameResult NativeHost::publish_world() noexcept {
    auto& self = *impl_;
    if (!self.setup() || !self.world || self.world_published) return ISLE_GAME_INVALID_STATE;
    self.state.background_rgb=world_resources().setup().background_rgb;
    try { self.world->publish(); self.world_published = true; return ISLE_GAME_OK; }
    catch (const std::exception& e) { return fail(ISLE_GAME_INVALID_STATE,e.what()); }
    catch (...) { return fail(ISLE_GAME_INVALID_STATE,"native world publication threw"); }
}
IsleGameResult NativeHost::sky_follow() noexcept {
    auto& self = *impl_;
    if (!self.setup() || !self.world_published) return ISLE_GAME_INVALID_STATE;
    self.follows_camera = true;
    if (self.root->source().camera) self.world->follow_camera(self.root->source().camera);
    return ISLE_GAME_OK;
}
IsleGameResult NativeHost::allocate_depth(std::uint32_t bytes, std::uint32_t alignment, void** output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = nullptr;
    auto& self = *impl_;
    if (!self.setup() || !self.frame || self.depth) return ISLE_GAME_INVALID_STATE;
    self.depth = self.frame->reserve_source_depth(bytes,alignment);
    if (!self.depth) return ISLE_GAME_INVALID_STATE;
    *output = const_cast<g::DepthReservation*>(self.depth); return ISLE_GAME_OK;
}
IsleGameResult NativeHost::apply_screen(const g::SourceScreen& screen, const void* depth) noexcept {
    auto& self = *impl_;
    if (!self.setup() || !self.depth || depth != self.depth || self.screen_applied) return ISLE_GAME_INVALID_STATE;
    if (!self.frame->configure_source_screen(screen,self.depth)) return ISLE_GAME_INVALID_STATE;
    self.screen_applied = true; return ISLE_GAME_OK;
}
IsleGameResult NativeHost::create_helper(std::uint32_t id, IsleGameSourceCallback update,
    std::uint8_t link, std::uint32_t priority, IsleGameObject** output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = nullptr;
    auto& self = *impl_;
    auto* current = self.services ? isle_menu_current_object(self.services->game) : nullptr;
    const bool running_source = self.prepared && self.initialized && !self.closing &&
        !self.exiting && self.error==ISLE_GAME_OK && self.services && !self.services->failed() &&
        self.references && self.references->registered && self.frame && self.frame->quiescent() &&
        current && self.services->source_owner_registered(current) &&
        !self.services->source_owner_joined(current) &&
        isle_actor_source_execution_status(self.services->game)==ISLE_GAME_OK;
    const auto& initial = self.description.initial_helper;
    const bool initial_update = self.setup() && !self.references && update &&
        update == initial.callback && id == initial.id &&
        link == initial.link && priority == initial.priority;
    if ((!self.setup() && !running_source) || link>31 ||
        (update && !initial_update && (!self.bindings || !self.bindings->contains(update)))) return ISLE_GAME_INVALID_STATE;
    for (auto& helper : self.helpers) if (!helper.object) {
        const auto fields = self.references ? std::span<IsleGameObject** const>{} : self.description.reference_fields;
        const auto result = self.publish_helper(helper,id,link,priority,update,fields);
        if (helper.registered && !self.references) self.references = &helper;
        *output = helper.object;
        return result;
    }
    return ISLE_GAME_EXHAUSTED;
}
IsleGameResult NativeHost::initialize_collisions(IsleGameObject* helper) noexcept {
    auto& self = *impl_;
    if (!self.setup() || !helper || !self.services->source_owner_registered(helper) || self.collisions) return ISLE_GAME_INVALID_STATE;
    try { self.collisions = std::make_unique<actor::CollisionPass>(*self.services,*self.actors,helper); return ISLE_GAME_OK; }
    catch (const std::exception& e) { return fail(ISLE_GAME_INVALID_STATE,e.what()); }
    catch (...) { return fail(ISLE_GAME_INVALID_STATE,"native collision owner construction threw"); }
}
IsleGameResult NativeHost::create_cameras(std::uint32_t background) noexcept {
    auto& self = *impl_;
    if (!self.setup() || !self.renderer || self.camera) return ISLE_GAME_INVALID_STATE;
    IsleGameObject* ui = nullptr;
    const auto result = self.renderer->create_main(static_cast<std::int32_t>(background),&ui);
    if (result != ISLE_GAME_OK) return result;
    auto made = player::camera_animation::NativeDriver::create(self.services->game,self.root->source().camera,
        self.cart_resources.animation_resources(),self.description.capacity.camera_execution);
    if (!made || !made.driver) return ISLE_GAME_INVALID_STATE;
    self.camera = std::move(made.driver);
    const auto bound = self.root->bind_camera_animation(*self.camera);
    if (bound != ISLE_GAME_OK) return bound;
    if (self.follows_camera) self.world->follow_camera(self.root->source().camera);
    return ISLE_GAME_OK;
}
IsleGameResult NativeHost::initialize_detector(IsleGameObject* helper) noexcept {
    auto& self = *impl_;
    if (!self.setup() || self.photo || !self.collisions || !helper || !self.camera) return ISLE_GAME_INVALID_STATE;
    try {
        self.photo = std::make_unique<player::Photo>(self.photos.source(),self.root->source(),*self.actors,*self.items,
            self.detection,self.photos.resources(),*self.world,self.description.course_id);
        self.photo_scene = std::make_unique<actor::PhotoScene>(*self.services,*self.actors,helper,
            self.root->source().camera,self.detection,&self.photos.source(),self.collisions.get());
        self.controller = std::make_unique<player::Controller>(*self.photo,self.detection);
        return self.renderer->bind_photos(*self.controller);
    } catch (const std::exception& e) { return fail(ISLE_GAME_INVALID_STATE,e.what()); }
    catch (...) { return fail(ISLE_GAME_INVALID_STATE,"native Photo owner construction threw"); }
}
IsleGameResult NativeHost::detector_enabled(bool enabled) noexcept {
    if (!impl_->photo) return ISLE_GAME_INVALID_STATE;
    return enabled ? isle_player_photo_PokemonDetector_Enable(&impl_->photos.source()) :
        isle_player_photo_PokemonDetector_Disable(&impl_->photos.source());
}
IsleGameResult NativeHost::reset_photo_roll() noexcept { return impl_->setup() ? impl_->photos.reset() : ISLE_GAME_INVALID_STATE; }
IsleGameResult NativeHost::environment_initialize() noexcept {
    auto& self = *impl_;
    return self.setup() && self.environment && self.root->source().model && self.root->source().camera &&
        self.environment->initialize(self.root->source().model,self.root->source().camera) ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
}
IsleGameResult NativeHost::environment_cleanup() noexcept {
    if (!impl_->environment) return ISLE_GAME_INVALID_STATE;
    impl_->environment->cleanup(); return ISLE_GAME_OK;
}
IsleGameResult NativeHost::reset_effect_bank(std::size_t bank) noexcept {
    return impl_->setup() && impl_->actors && impl_->actors->effects().reset_bank(bank) ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
}
IsleGameResult NativeHost::install_effect_bank(menu::EffectGroup group) noexcept {
    auto& self = *impl_;
    if (!self.setup() || (group != menu::EffectGroup::common && group != self.description.effects)) return ISLE_GAME_INVALID_STATE;
    try { return self.actors->effects().install_bank(self.effect_resources.bank(group)) ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE; }
    catch (const std::exception& e) { return fail(ISLE_GAME_INVALID_STATE,e.what()); }
    catch (...) { return fail(ISLE_GAME_INVALID_STATE,"native effect bank installation threw"); }
}
IsleGameResult NativeHost::publish_particle_camera(IsleGameObject** output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = nullptr;
    auto& self = *impl_;
    if (!self.setup()) return ISLE_GAME_INVALID_STATE;
    const auto made = self.actors->publish_particle_camera();
    if (!made) return fail(ISLE_GAME_INVALID_STATE,made.detail);
    const auto result = self.renderer->adopt_particle(*self.actors,self.actors->particle_object());
    if (result == ISLE_GAME_OK) *output = self.actors->particle_object();
    return result;
}
IsleGameResult NativeHost::publish_effect_emitters(IsleGameObject** output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = nullptr;
    auto& self = *impl_;
    if (!self.setup()) return ISLE_GAME_INVALID_STATE;
    const auto made = self.actors->publish_effect_emitters();
    if (!made) return fail(ISLE_GAME_INVALID_STATE,made.detail);
    const auto result = self.renderer->adopt_effect(*self.actors,self.actors->effect_object());
    if (result == ISLE_GAME_OK) *output = self.actors->effect_object();
    return result;
}
IsleGameResult NativeHost::bind_item_collision(IsleItemCollisionProvider callback, void* user) noexcept {
    auto& self = *impl_;
    // Original may first publish the gate callback from a later live World block.
    const bool admitted=self.setup() || (self.initialized && execution_open());
    if (!admitted || !callback || self.collision_bound || !self.items) return ISLE_GAME_INVALID_STATE;
    if (!self.items->set_extra_collision(self.services->game,callback,user)) return ISLE_GAME_INVALID_STATE;
    self.collision_bound = true; return ISLE_GAME_OK;
}
IsleGameResult NativeHost::reference(IsleGameObject** field, IsleGameObject* object) noexcept {
    auto& self = *impl_;
    if (!field || !self.services) return ISLE_GAME_INVALID_STATE;
    if (self.references && self.references->registered)
        for (auto** registered:self.references->fields)
            if (registered==field)
                return self.services->source_reference_assign(self.references->object,field,object)?
                    ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    if (self.actors) for (const auto& binding:self.actors->render_bindings()) {
        if (!binding.model.object) continue;
        IsleActorSource* source=nullptr;
        const auto result=isle_actor_source_get(self.services->game,binding.model.object,&source);
        if (result!=ISLE_GAME_OK) return result;
        if (field==&source->apple || field==&source->interactionTarget)
            return isle_actor_owner_reference_assign(source,field,object);
    }
    return ISLE_GAME_INVALID_STATE;
}
bool NativeHost::valid_block(const world::Block* block) const noexcept {
    if (!impl_->world || !block) return false;
    for (const auto& candidate : impl_->world->blocks()) if (&candidate == block) return true;
    return false;
}
bool NativeHost::detach_ready() const noexcept {
    return impl_->detach_authorized && impl_->services && impl_->root && impl_->actors &&
        impl_->root->bound_actors() == impl_->actors.get() &&
        isle_game_require_idle(impl_->services->game) == ISLE_GAME_OK &&
        (!impl_->frame || impl_->frame->quiescent());
}
bool NativeHost::closure_prepared() const noexcept {
    const auto& self = *impl_;
    return self.prepared && !self.closing && !self.retired && self.services &&
        self.root && self.actors && self.items && self.cart && self.bindings &&
        self.interactions && self.root->bound_actors() == self.actors.get();
}
bool NativeHost::execution_open() const noexcept {
    return closure_prepared() && !impl_->exiting && impl_->error == ISLE_GAME_OK;
}
bool NativeHost::exit_requested() const noexcept { return impl_->exiting; }
bool NativeHost::initialized() const noexcept { return impl_->initialized; }
bool NativeHost::retired() const noexcept { return impl_->retired; }
bool NativeHost::owns_input(IsleGameObject* object) const noexcept {
    return object && impl_->services && object==impl_->input_owner.object &&
        !impl_->input_owner.joined && !impl_->closing;
}
IsleGameResult NativeHost::invalidate_input(IsleGameObject* object) noexcept {
    if (!owns_input(object) || isle_menu_current_object(impl_->services->game)!=object)
        return ISLE_GAME_INVALID_STATE;
    return impl_->input.begin_update()==game_input::Error::none ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
}
IsleGameResult NativeHost::flute_command(std::int32_t* output) noexcept {
    if (!output || !impl_->services || !impl_->items) return ISLE_GAME_INVALID_ARGUMENT;
    return isle_item_owner_flute(&impl_->globals.source,2,output);
}
IsleGameResult NativeHost::error() const noexcept { return impl_->error; }
const char* NativeHost::message() const noexcept { return impl_->message.data(); }
IsleGame* NativeHost::game() const noexcept { return impl_->services ? impl_->services->game : (impl_->storage ? impl_->storage->game() : nullptr); }
scene::Services* NativeHost::services() const noexcept { return impl_->services.get(); }
player::Root* NativeHost::root() const noexcept { return impl_->root.get(); }
player::Ui* NativeHost::ui() const noexcept { return impl_->ui.get(); }
std::size_t NativeHost::extra_sprite_base() const noexcept { return impl_->ui_resources.roots().size(); }
player::Renderer* NativeHost::renderer() const noexcept { return impl_->renderer.get(); }
world::World* NativeHost::world_owner() const noexcept { return impl_->world.get(); }
actor::Owner* NativeHost::actors() const noexcept { return impl_->actors.get(); }
actor::Producer* NativeHost::actor_packet_producer() const noexcept { return impl_->actor_producer.get(); }
items::Owner* NativeHost::items_owner() const noexcept { return impl_->items.get(); }
player::cart::Owner* NativeHost::cart() const noexcept { return impl_->cart.get(); }
player::camera_animation::NativeDriver* NativeHost::camera() const noexcept { return impl_->camera.get(); }
const player::CartResources& NativeHost::cart_resources() const noexcept { return impl_->cart_resources; }
const world::Resources& NativeHost::world_resources() const noexcept { return impl_->world_resources; }
const actor::Resources& NativeHost::actor_resources() const noexcept { return impl_->actor_resources; }
const actor::BehaviorBindings* NativeHost::bindings() const noexcept { return impl_->bindings.get(); }
environment_sound::Environment* NativeHost::environment() const noexcept { return impl_->environment.get(); }
game_audio::Session& NativeHost::audio() const noexcept { return impl_->audio; }
application::Photos& NativeHost::photos() const noexcept { return impl_->photos; }
application::Manager& NativeHost::manager() const noexcept { return impl_->manager; }
game::State& NativeHost::game_state() const noexcept { return impl_->state; }
IsleGameObject* NativeHost::reference_owner() const noexcept { return impl_->references ? impl_->references->object : nullptr; }
TunnelActorNativeState* NativeHost::tunnel_actor_state() const noexcept { return impl_->tunnel_actors.get(); }
} // namespace isle::course

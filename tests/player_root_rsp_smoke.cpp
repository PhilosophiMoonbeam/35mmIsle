#include "beach_actor_scene.hpp"
// Opt-in private-ROM consumer of the canonical original-C Player Root.
// This is generic Root acceptance with Beach resources, not the Beach intro,
// a second Player FSM, full-game qualification, or hardware/Windows parity.
// Usage: player-root-rsp-smoke PRIVATE_ROM NEW_OUTPUT_DIRECTORY
#include "player_root.hpp"
#include "player_renderer.hpp"
#include "player_commands.hpp"
#include "player_ui.hpp"
#include "player_bump.hpp"
#include "player_cart.hpp"
#include "actor_photo_scene.hpp"
#include "actor_behavior_bindings.hpp"
#include "actor_interaction_services.hpp"
#include "beach_initial_species.hpp"
#include "beach_magikarp_scene.hpp"
#include "beach_music.hpp"
#include "course_model_producer.hpp"
#include "effect_bank_resources.hpp"
#include "screen_transition_resources.hpp"
#include "actual_audio_driver.hpp"
#include "scene_storage.hpp"
#include "source_camera_contract.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <source_location>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace {
using namespace isle;
namespace g = scene::graphics;
namespace r = scene::render;
namespace a = menu::animation;
constexpr std::size_t plane = 320 * 240 * 2;
constexpr std::uint16_t button_a = 0x8000, button_b = 0x4000,
    button_z = 0x2000, button_start = 0x1000;
constexpr world::ProducerLayout world_layout{{0x200000,0x10000},{0x210000,0x10000},{0x220000,0x10000}};
constexpr actor::ProducerLayout actor_layout{{0x400000,0x80000},{0x480000,0x80000},{0x500000,0x100000},
    {{{0x3b5000,plane},{0x3da800,plane}}}};
constexpr menu::model::PacketLayout item_layout{{0x230000,0x10000},{0x240000,0x10000},{0x250000,0x10000},
    {{{0x3b5000,plane},{0x3da800,plane}}}};
constexpr player::cart::ProducerLayout cart_layout{{0x260000,0x10000},{0x270000,0x10000},{0x2a0000,0x10000},
    {{{0x3b5000,plane},{0x3da800,plane}}}};
constexpr focus::CaptureLayout capture_layout{{0x680000,0x50},{0x680080,0x80},{0x680100,0xa00}};
constexpr menu::RamRange fx_commands{0x680c00,0x5400}, joined_lists{0x686000,0x5000};
constexpr sprite::RamRange lanes{0x690000,0x40000}, sprite_commands{0x780000,0x50000},
    message_commands{0x6d0000,0x30000};
constexpr g::Layout frame_layout{{0x626000,0x2000},{0x630000,0x10000},{0x640000,0x40000},0x600000};
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
void native(IsleGameResult result, std::source_location where = std::source_location::current()) {
    if (result != ISLE_GAME_OK) throw std::runtime_error(std::string(where.file_name()) + ":" +
        std::to_string(where.line()) + " native result=" + std::to_string(int(result)));
}
template<class Status> void accepted(Status status) { require(bool(status),status.detail); }
std::uint32_t physical(std::uint32_t address) { return address & 0x1fffffffu; }
std::uint64_t monotonic_ns() {
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}
std::uint32_t word(std::span<const std::byte> ram, std::uint32_t address) {
    require(address <= ram.size() - 4,"completed-memory evidence escaped RDRAM");
    std::uint32_t result = 0;
    for (unsigned i=0;i<4;++i) result = (result<<8) | std::to_integer<unsigned>(ram[(address+i)^3u]);
    return result;
}
void save(const std::filesystem::path& path, std::span<const std::byte> bytes) {
    require(!std::filesystem::exists(path),"Root evidence would overwrite an artifact");
    std::ofstream stream(path,std::ios::binary);
    stream.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
    require(bool(stream),"Root evidence write failed");
}
struct Log {
    std::ofstream stream;
    std::uint64_t event = 0;
    explicit Log(const std::filesystem::path& path) : stream(path) {
        require(bool(stream),"Root chronology creation failed");
        stream << "event\tkind\tdetails\n";
    }
    template<class... Values> void add(std::string_view kind, const Values&... values) {
        stream << ++event << '\t' << kind;
        ((stream << '\t' << values),...);
        stream << '\n';
        require(bool(stream),"Root chronology write failed");
    }
};
struct Audio {
    game_audio::Session& session;
    private_audio::Execution& execution;
    Log& log;
    std::uint64_t retrace = 0, tasks = 0;
    std::chrono::steady_clock::time_point last{};
    void tick() {
        // An actual host boundary: do not turn an OM/draw ordinal into an AI
        // timestamp. Slow GPU work naturally changes the measured AI elapsed time.
        if (retrace) std::this_thread::sleep_until(last + std::chrono::nanoseconds(16666667));
        last = std::chrono::steady_clock::now();
        const auto now = monotonic_ns();
        require(session.retrace(++retrace,now)==audio::Error::none,"actual Audio retrace failed");
        require(session.submit()==audio::Error::none,"actual owned PCM submission failed");
        audio::TaskView task;
        require(session.build(task)==audio::Error::none,"actual Audio task build failed");
        if (task.token.serial) {
            const auto result = private_audio::execute(session,execution,task);
            ++tasks;
            log.add("audio-terminal",retrace,now,session.source_frame_epoch(),task.token.serial,
                result.sp_status,result.packets,result.packet_words,result.full_syncs);
            require(session.submit()==audio::Error::none,"actual Audio completed PCM submission failed");
        }
    }
};
struct Content {
    focus::RegionCopies& copies;
    course::ModelProducer* models = nullptr;
    actor::Owner* actors = nullptr;
    items::Owner* items = nullptr;
    player::cart::Owner* cart = nullptr;
    std::array<scene::RenderOrderEntry,256> cart_order{};
    std::uint64_t calls = 0;
    static bool prepare(void* user, std::span<std::uint32_t> ram, std::uint64_t epoch,
                        const r::Pass& pass, r::Prepared& output) {
        auto& self = *static_cast<Content*>(user);
        require(self.models && self.actors && self.items && self.cart,
            "cold Root content was drawn before real producers were attached");
        require(self.copies.install(ram)==focus::CaptureStatus::ok,"real capture regions did not install");
        self.actors->render_bindings();
        self.items->render_bindings();
        output = self.models->prepare(ram,epoch,pass);
        ++self.calls;
        return true;
    }
};
struct Hooks {
    struct Factory {
        actor::SpawnResult spawn{};
        world::Block* block = nullptr;
        world::Block* reference = nullptr;
        std::int32_t species = -1, behavior = 0;
        std::uint32_t definition = 0, factory = 0, row = 0;
        std::size_t path = world::no_index;
        bool before_player = false;
        unsigned native_retiring = 0, native_joined = 0;
        bool invalid_join = false;
    };
    struct Command { IsleGameObject* receiver; IsleGameObject* sender; std::int32_t command; };
    scene::Services& services;
    player::Root& root;
    const actor::Resources& resources;
    actor::Owner* actors = nullptr;
    world::World* world = nullptr;
    std::array<Factory,256> factories{};
    std::array<Command,256> commands{};
    std::size_t count = 0, command_count = 0;
    bool overflow = false;
    static void actor_retiring(IsleGameObject* object, void* user) noexcept {
        auto& entry=*static_cast<Factory*>(user);
        entry.invalid_join|=object!=entry.spawn.object || entry.native_retiring!=0 || entry.native_joined!=0;
        ++entry.native_retiring;
    }
    static void actor_joined(IsleGameObject* object, void* user) noexcept {
        auto& entry=*static_cast<Factory*>(user);
        entry.invalid_join|=object!=entry.spawn.object || entry.native_retiring!=1 || entry.native_joined!=0;
        ++entry.native_joined;
    }
    static bool add(void* user, world::Block& block, world::Block& reference) noexcept {
        auto& self = *static_cast<Hooks*>(user);
        if (!self.actors || !self.world || !block.resource || !reference.resource) return false;
        const auto definitions = self.resources.definitions();
        for (const auto& row : block.resource->spawns) {
            std::size_t definition = actor::actor_no_index;
            for (std::size_t i=0;i<definitions.size();++i) {
                if (!definitions[i].singleton && definitions[i].actor_id==static_cast<std::uint32_t>(row.id)) {
                    if (definition!=actor::actor_no_index) return false;
                    definition=i;
                }
            }
            // No unadmitted cohort is silently skipped or replaced by a singleton.
            if (definition==actor::actor_no_index || self.count==self.factories.size()) return false;
            auto& player = self.root.source();
            IslePlayerSource* active = nullptr;
            if (isle_player_source_context(self.services.game,&active)!=ISLE_GAME_OK || active!=&player) return false;
            const bool cold = !player.object && !player.model;
            if (cold && (self.actors->particle_object() || self.actors->effect_object())) return false;
            const auto spawned = self.actors->add_one(definition,&block,&reference,row);
            if (!spawned) return false;
            const auto* source = self.actors->source(spawned.instance);
            const auto paths=self.world->resources().paths;
            if (row.path!=world::no_index && row.path>=paths.size()) return false;
            const void* expected_path=row.path==world::no_index?nullptr:&paths[row.path];
            if (!source || !source->stateProc || !self.actors->native_model(spawned.instance) ||
                source->behavior!=row.behavior || source->path!=expected_path || (cold && (source->pathProc || source->counter!=0 ||
                player.object || player.model || self.actors->particle_object() || self.actors->effect_object()))) return false;
            self.factories[self.count++] = {spawned,&block,&reference,row.id,row.behavior,
                definitions[definition].wire.guest_address,definitions[definition].factory_identity,
                row.wire.guest_address,row.path,cold};
            if (!self.services.observe_lifecycle(spawned.object,
                {actor_retiring,actor_joined,&self.factories[self.count-1]})) return false;
        }
        return true;
    }
    static bool remove(void* user, world::Block& block) noexcept {
        auto& self = *static_cast<Hooks*>(user);
        return self.actors ? bool(self.actors->remove_block(block)) : self.count==0;
    }
    static bool change(void* user, world::Block& block, world::Block& reference) noexcept {
        auto& self = *static_cast<Hooks*>(user);
        if (!self.actors || !reference.resource) return false;
        for (std::size_t i=0;i<self.count;++i) {
            auto& entry = self.factories[i];
            if (entry.block!=&block || !self.actors->registered(entry.spawn.instance)) continue;
            if (!entry.reference || !entry.reference->resource) return false;
            const auto old = entry.reference->resource->metadata.world_position;
            const auto next = reference.resource->metadata.world_position;
            if (!self.actors->change_block(entry.spawn.instance,{old.x,old.y,old.z},{next.x,next.y,next.z})) return false;
            entry.reference=&reference; // Membership bookkeeping only, never a pose mirror.
        }
        return true;
    }
    static bool shift_camera(void* user, world::Vec3 delta) noexcept {
        auto& self = *static_cast<Hooks*>(user);
        return isle_player_root_invoke_bind_camera_next_block(&self.root.source(),delta.x,delta.y,delta.z)==ISLE_GAME_OK;
    }
    static bool shift_effects(void* user, world::Vec3 delta) noexcept {
        auto& self = *static_cast<Hooks*>(user);
        if (!self.actors) return false;
        self.actors->effects().translate({delta.x,delta.y,delta.z});
        return bool(self.actors->effects().status());
    }
    static void queued(IsleGameObject* receiver, std::int32_t command,
                       IsleGameObject* sender, void* user) noexcept {
        auto& self = *static_cast<Hooks*>(user);
        if (self.command_count==self.commands.size()) { self.overflow=true; return; }
        self.commands[self.command_count++]={receiver,sender,command};
    }
};
struct Events {
    bool request_exit = false;
    std::int32_t reason = 0;
    unsigned end_calls = 0, pause_calls = 0, resume_calls = 0;
    static IsleGameResult end(void* user, std::int32_t reason) noexcept {
        auto& self = *static_cast<Events*>(user);
        self.request_exit=true; self.reason=reason; ++self.end_calls;
        return ISLE_GAME_OK; // Application exit latch, never destruction inside OM.
    }
    static IsleGameResult pause(void* user, std::int32_t paused) noexcept {
        auto& self = *static_cast<Events*>(user);
        if (paused) ++self.pause_calls; else ++self.resume_calls;
        // The admitted Beach callback body is genuinely empty.
        return ISLE_GAME_OK;
    }
};
struct MovementHook {
    beach::Music* music = nullptr;
    std::uint64_t calls = 0;
    static bool update(void* user, std::size_t block, float part) noexcept {
        auto& self=*static_cast<MovementHook*>(user);
        if (!self.music) return false;
        ++self.calls;
        return self.music->update(block,part)==ISLE_GAME_OK;
    }
};
struct NativeJoin {
    IsleGameObject* object = nullptr;
    unsigned retiring = 0, joined = 0;
    bool invalid = false;
    static void on_retiring(IsleGameObject* object, void* user) noexcept {
        auto& self=*static_cast<NativeJoin*>(user);
        self.invalid|=object!=self.object || self.retiring!=0 || self.joined!=0;
        ++self.retiring;
    }
    static void on_joined(IsleGameObject* object, void* user) noexcept {
        auto& self=*static_cast<NativeJoin*>(user);
        self.invalid|=object!=self.object || self.retiring!=1 || self.joined!=0;
        ++self.joined;
    }
};
enum class Scenario { fresh, idle_interrupted, idle_finished, no_background_music };
struct Case {
    const rom::Snapshot& snapshot;
    std::filesystem::path output;
    Scenario scenario;
    Log log;
    world::Resources world_resources;
    beach::ActorSceneResources sceneData;
    std::uint16_t scene_latch;
    beach::ActorSceneState sceneState;
    actor::Resources actor_resources;
    actor::BehaviorResources behaviors;
    items::Resources item_resources;
    player::CartResources cart_resources;
    player::UiResources ui_resources;
    player::PhotoResources photo_resources;
    environment_sound::Resources sounds;
    menu::EffectResources effect_resources;
    photo::LightingInputs lighting;
    graphics::SourcePrograms programs;
    screen_transition::Resources transition_resources;
    std::array<const menu::EffectBank*,2> banks;
    menu::EffectGraphicsResources effect_graphics;
    sprite::Producer sprites;
    std::array<a::SlotCapacity,40> actor_capacities{};
    std::array<a::SlotCapacity,32> item_capacities{};
    std::array<a::SlotCapacity,2> cart_capacities{};
    std::array<IslePlayerInput,128> idle_inputs{};
    IslePlayerIdleScript idle{};
    private_audio::Capture pcm;
    private_audio::Execution execution;
    std::unique_ptr<game_audio::Session> audio;
    std::unique_ptr<Audio> audio_clock;
    game::State state;
    IslePlayerPhotoSource application{};
    game_input::Service input;
    scene::Storage storage;
    std::unique_ptr<scene::Services> services;
    Events events;
    std::unique_ptr<beach::Music> music;
    MovementHook movement_hook;
    std::unique_ptr<player::Root> root;
    IsleGameObject* scene_root = nullptr;
    IsleGameObject* root_identity = nullptr;
    IsleGameObject* reticle_identity = nullptr;
    IsleGameObject* main_identity = nullptr;
    std::array<NativeJoin,5> native_joins{};
    bool scene_root_registered = false;
    std::unique_ptr<player::commands::Binding> commands;
    std::unique_ptr<player::Ui> ui;
    std::unique_ptr<player::BumpOwner> bump;
    focus::Detector detector;
    focus::RegionCopies copies{capture_layout};
    actor::DetectorBinding detection{copies,detector};
    std::unique_ptr<g::Frame> frame;
    Content content{copies};
    std::vector<menu::RamRange> all_ranges;
    std::vector<sprite::AssetSpan> immutable;
    std::vector<sprite::RamRange> reservations;
    std::unique_ptr<player::Renderer> renderer;
    std::unique_ptr<environment_sound::Environment> environment;
    std::unique_ptr<Hooks> hooks;
    world::ModuleState world_module;
    std::unique_ptr<world::World> world;
    std::unique_ptr<actor::Owner> actors;
    items::Globals globals;
    std::unique_ptr<items::Owner> items;
    std::unique_ptr<player::cart::Owner> cart;
    std::unique_ptr<actor::BehaviorBindings> behavior_bindings;
    std::unique_ptr<actor::InteractionServices> interactions;
    std::unique_ptr<beach::MagikarpScene> magikarp;
    std::unique_ptr<player::Photo> photo;
    std::unique_ptr<actor::PhotoScene> photo_scene;
    std::unique_ptr<player::Controller> controller;
    std::unique_ptr<world::Producer> worlds;
    std::unique_ptr<actor::Producer> actor_producer;
    std::unique_ptr<menu::model::PacketProducer> item_producer;
    std::unique_ptr<player::cart::Producer> carts;
    std::unique_ptr<course::ItemComposition> composition;
    std::unique_ptr<course::ModelProducer> models;
    std::vector<std::byte> color = std::vector<std::byte>(plane);
    std::vector<std::byte> depth = std::vector<std::byte>(plane);
    std::uint64_t om = 0, draws = 0;
    std::size_t logged_factories = 0, logged_commands = 0, triangles = 0, rectangles = 0;
    std::size_t blackened_pixels = 0, nonblack_pixels = 0;
    unsigned completed_calls = 0, message_frames = 0, transition_frames = 0, visible_transition_frames = 0;
    std::uintptr_t mapping = 0;
    bool completed_hold = false, closed = false, storage_created = false;
    bool first_frame_saved = false, message_saved = false, fade_saved = false;
    bool transition_visible = false;
    std::int32_t last_total = 0, last_qualifying = 0;

    Case(const rom::Snapshot& rom, std::filesystem::path directory, Scenario selected)
        : snapshot(rom),output(std::move(directory)),scenario(selected),log(output/"chronology.tsv"),
          world_resources(rom, isle::course::beach_id),sceneData(rom),scene_latch(sceneData.initial_scalars().magikarp_active),sceneState(sceneData,scene_latch),actor_resources(rom,0,{world_resources.setup().fog_rgb,
              world_resources.setup().fog_min,world_resources.setup().fog_max}),behaviors(rom,0),
          item_resources(rom,world_resources.setup().fog_rgb,world_resources.setup().fog_min,world_resources.setup().fog_max),
          cart_resources(rom,0,{world_resources.setup().fog_rgb,world_resources.setup().fog_min,world_resources.setup().fog_max}),
          ui_resources(rom),photo_resources(rom),sounds(rom,0),effect_resources(rom),lighting(rom),programs(rom),
          transition_resources(rom),banks{&effect_resources.bank(menu::EffectGroup::common),&effect_resources.bank(menu::EffectGroup::beach)},
          effect_graphics(rom,banks,{0x700000,0x80000}),sprites(ui_resources.roots(),ui_resources.banks()),
          execution{rom,nullptr,{}},state(output/"profile"),storage(32*1024*1024) {}
    IslePlayerSource& source() { return root->source(); }
    void checkpoint(std::string_view name) {
        const auto& s=source();
        log.add("source",name,om,draws,s.gDirectionIndex,s.IsInputDisabled,int(s.IsTutorialEnabled),
            int(s.IsPaused),int(s.D_80382D04_523114),s.D_80382D08_523118,s.sTimerAfterPhotoTaken,s.sMinTimeBetweenPhotos,
            application.gPhotoCount,application.D_800AE27C,application.gPokemonIdInFocus,
            detection.context,renderer->drawn_frame_counter(),audio->source_frame_epoch(),
            s.movement.move_time,s.LevelBGMSongID);
        if (music) log.add("source-Beach-music",om,movement_hook.calls,music->source().D_beach_802CC0D0,
            music->source().D_beach_802CC0D4,music->source().D_beach_802CC0D8,
            int(music->source().D_beach_80347578),int(music->source().D_beach_80347579),
            int(music->source().D_beach_8034757A),int(music->source().D_beach_8034757B),
            music->source().D_beach_8034757C);
    }
    game_input::HostFrame sample(std::uint16_t buttons=0, std::int8_t x=0, std::int8_t y=0,
                                std::uint16_t second=0) const {
        game_input::HostFrame result{};
        result[0]={game_input::Connection::present,0x524f4f5430ull,0,buttons,x,y};
        result[1]={game_input::Connection::present,0x524f4f5431ull,0,second,0,0};
        return result;
    }
    void flush_callbacks() {
        require(!hooks->overflow,"native FIFO observation capacity exceeded");
        while (logged_factories<hooks->count) {
            const auto& f=hooks->factories[logged_factories++];
            std::uint32_t id=0;
            native(isle_game_object_id(services->game,f.spawn.object,&id));
            log.add("factory",logged_factories,f.block->index,f.reference->index,f.species,f.behavior,
                f.definition,f.factory,f.row,f.path,id,static_cast<const void*>(f.spawn.object),f.before_player);
        }
        while (logged_commands<hooks->command_count) {
            const auto& c=hooks->commands[logged_commands++];
            log.add("native-fifo",om,static_cast<const void*>(c.receiver),c.command,static_cast<const void*>(c.sender));
        }
    }
    void check() {
        require(!services->failed(),services->error());
        native(source().status); native(application.status); native(ui->source().status);
        native(globals.source.status); native(services->screen_transition_source()->status);
        native(renderer->status());
        flush_callbacks();
    }
    void tick(std::uint16_t buttons=0, std::int8_t x=0, std::int8_t y=0, std::uint16_t second=0) {
        require(frame->quiescent(),"OM crossed actual GPU/readback ownership");
        const auto request=input.read_and_update();
        require(request.error==game_input::Error::none && request.accepted && request.read,
            "physical input read_and_update admission failed");
        const auto completed=input.complete_read(request.read,sample(buttons,x,y,second));
        require(completed.error==game_input::Error::none && completed.acknowledged==request.accepted,
            "physical input publication did not acknowledge its own request");
        require(input.begin_update()==game_input::Error::none,"physical input OM cache boundary failed");
        native(root->publish_input());
        const auto before=source().input;
        require(before.current_buttons==input.snapshot().pads[0].buttons &&
            before.pressed_buttons==input.snapshot().pads[0].pressed &&
            before.stick_x==input.snapshot().pads[0].stick_x && before.stick_y==input.snapshot().pads[0].stick_y,
            "Root did not publish physical port zero");
        log.add("input-published",om+1,request.accepted.generation,request.accepted.serial,
            request.read.serial,completed.acknowledged.serial,before.current_buttons,before.pressed_buttons,
            before.stick_x,before.stick_y,input.snapshot().pads[1].buttons);
        native(isle_game_tick(services->game)); ++om;
        // Never republish: IdleScript and pause own these scalars after OM.
        log.add("input-after-om",om,source().input.current_buttons,source().input.pressed_buttons,
            source().input.stick_x,source().input.stick_y);
        check(); audio_clock->tick();
        if (application.gPhotoCount!=last_total || application.D_800AE27C!=last_qualifying) {
            log.add("source-shutter",om,frame->submission(),controller->ticket(*frame).context,
                controller->ticket(*frame).generation,last_total,application.gPhotoCount,
                last_qualifying,application.D_800AE27C,photo->quiescent());
            require(application.gPhotoCount==last_total+1,"Source shutter skipped/duplicated a photo count");
            std::array<std::byte,0x3a0> encoded{};
            player::encode_photo(application.D_800B0598[static_cast<std::size_t>(application.gPhotoCount-1)],encoded);
            save(output/("photo-"+std::to_string(application.gPhotoCount)+".bin"),encoded);
            last_total=application.gPhotoCount; last_qualifying=application.D_800AE27C;
        }
    }
    static void completed(void* user, rsp::CanonicalExecutor::MemoryLease& lease, std::uint32_t framebuffer) {
        auto& self=*static_cast<Case*>(user);
        require(++self.completed_calls==1,"Root completed mapping ran more than once");
        const auto ram=lease.rdram();
        require(ram.size()==8*1024*1024,"Root completed mapping is not actual RDRAM");
        self.mapping=reinterpret_cast<std::uintptr_t>(ram.data());
        std::size_t changed_color_bytes=0;
        self.blackened_pixels=0; self.nonblack_pixels=0;
        for (std::size_t i=0;i<plane;i+=2) {
            const auto high=ram[(framebuffer+i)^3u], low=ram[(framebuffer+i+1)^3u];
            const bool before=self.color[i]!=std::byte{} || (std::to_integer<unsigned>(self.color[i+1])&0xfeu)!=0;
            const bool after=high!=std::byte{} || (std::to_integer<unsigned>(low)&0xfeu)!=0;
            self.blackened_pixels+=before && !after;
            self.nonblack_pixels+=after;
            changed_color_bytes+=(self.color[i]!=high)+(self.color[i+1]!=low);
            self.color[i]=high; self.color[i+1]=low;
            self.depth[i]=ram[(frame_layout.depth+i)^3u];
            self.depth[i+1]=ram[(frame_layout.depth+i+1)^3u];
        }
        self.triangles=0; self.rectangles=0;
        std::array<std::uint32_t,32> transition_colors{}; std::size_t color_count=0;
        bool has_transition=false;
        for (auto* object=self.services->screen_transition().objects;object;object=object->next)
            has_transition|=!object->joined;
        if (has_transition) {
            for (std::uint32_t at=screen_transition::Resources::workspace.address+0x100;
                 at<screen_transition::Resources::workspace.address+0x8000;at+=8) {
                const auto first=word(ram,at);
                if (first==0xdf000000u) break;
                if (first==0xfa000000u && color_count<transition_colors.size())
                    transition_colors[color_count++]=word(ram,at+4);
            }
        }
        bool transition_executed=false;
        for (const auto packet:lease.packets().packets()) {
            const auto words=lease.packets().words().subspan(packet.offset,packet.count);
            require(!words.empty(),"Root produced empty real RDP packet");
            const auto op=(words[0]>>24)&63u;
            self.triangles+=op>=8 && op<=15;
            self.rectangles+=op==0x24 || op==0x25;
            if (op==0x3a && words.size()>=2)
                for (std::size_t i=0;i<color_count;++i) transition_executed|=words[1]==transition_colors[i];
        }
        self.transition_visible=false;
        if (has_transition) {
            require(color_count && transition_executed,"Source fade did not execute in this actual Root GPU task");
            ++self.transition_frames;
            // Opaque iris rendering uses primitive alpha zero. Observe its
            // actual RGB blackening, excluding the framebuffer coverage bit.
            self.transition_visible=self.blackened_pixels!=0;
            self.visible_transition_frames+=self.transition_visible;
        }
        self.log.add("completed-memory",self.om,self.draws,self.frame->submission(),framebuffer,
            self.mapping,self.triangles,self.rectangles,has_transition,color_count,transition_executed,
            changed_color_bytes,self.blackened_pixels,self.nonblack_pixels,self.transition_visible);
    }
    void capture(std::string_view name) {
        require(completed_hold && frame->completed(),"Root artifact lacks actual completed readback");
        save(output/(std::string(name)+".rgba8"),frame->completed().rgba);
        save(output/(std::string(name)+".rgba16be"),color);
        save(output/(std::string(name)+".depth16be"),depth);
    }
    void draw(std::string_view name={}) {
        require(!completed_hold && frame->quiescent(),"Root reused a completed Photo/GPU hold");
        const auto focus_before=detector.published();
        const auto message_before=ui->source().Msg_FrameCounter;
        const auto counter_before=renderer->drawn_frame_counter();
        const auto queue_before=controller->queued_requests();
        completed_calls=0; ++draws;
        const bool begun=renderer->begin(*frame,{static_cast<unsigned>(draws&1u)},{this,completed});
        require(begun,renderer->message());
        completed_hold=true;
        const auto ticket=controller->ticket(*frame);
        log.add("submitted",om,draws,frame->submission(),frame->submission_context(),ticket.context,
            ticket.generation,ticket.submission,detection.context,counter_before,
            renderer->drawn_frame_counter(),queue_before,controller->queued_requests());
        require(renderer->drawn_frame_counter()==counter_before+1,"Source draw counter is not one per complete draw");
        g::Poll result;
        do { result=renderer->poll(); if (result==g::Poll::pending) std::this_thread::yield(); }
        while (result==g::Poll::pending);
        require(result==g::Poll::completed && completed_calls==1,renderer->message());
        require(detector.published().present==focus_before.present &&
            detector.published().context==focus_before.context && detector.published().object==focus_before.object,
            "GPU completion published Source focus before later native OM CopyInfo");
        player::Renderer::Dispatch main{},message{};
        native(renderer->dispatch_metadata(main_identity,main));
        native(renderer->dispatch_metadata(ui->source().Msg_CameraObject,message));
        require(main.epoch==message.epoch,"Source message was not dispatched in the same Root frame");
        const auto message_delta=ui->source().Msg_FrameCounter-message_before;
        require(message_delta<=1,"Source Msg_DrawMessage ran more than once in one frame");
        log.add("poll-completed",om,draws,frame->submission(),ticket.context,ticket.generation,
            main.epoch,main.framebuffer,main.depth,message.epoch,message_before,ui->source().Msg_FrameCounter,
            ui->message_draws().size(),controller->queued_requests(),audio->source_frame_epoch());
        if (!ui->message_draws().empty()) {
            ++message_frames;
            if (!message_saved) { capture("source-message"); message_saved=true; }
        }
        if (transition_visible && !fade_saved) { capture("source-fade"); fade_saved=true; }
        if (!first_frame_saved) {
            require(triangles && rectangles && content.calls,"actual cold Root frame lacks real geometry/UI");
            capture("cold-root"); first_frame_saved=true;
        }
        if (!name.empty()) capture(name);
        check();
    }
    void release() {
        if (!completed_hold) return;
        log.add("release-completed",om,draws,frame->submission(),controller->ticket(*frame).context,
            controller->ticket(*frame).generation,application.gPhotoCount,application.D_800AE27C,
            controller->has_pending(*frame));
        // A genuine Source blink/no-prefix frame can finish with no Photo
        // capability at all. poll already released that Renderer frame; do not
        // invent a completed ticket or a second retirement for it.
        if (controller->has_pending(*frame)) native(renderer->release_completed());
        completed_hold=false;
        require(photo->quiescent() && controller->quiescent(),"Source shutter/readback capability survived release");
    }
    void cycle(std::uint16_t buttons=0, std::int8_t x=0, std::int8_t y=0) {
        tick(buttons,x,y); release(); draw();
    }
    void build() {
        require(std::filesystem::create_directory(output/"profile"),"fresh Root profile directory already exists");
        require(state.initialize(1) && state.save(),"fresh Root profile initialization failed");
        // This isolated host supplies the real post-Boot manager entry service.
        {
            const auto initial_settings=state.save()->snapshot();require(bool(initial_settings),"Root entry settings unavailable");
            const auto initial_vi=initial_settings.value.screen_offsets();require(bool(initial_vi),"Root entry VI unavailable");
            state.set_screen_offsets({initial_vi.value.horizontal,initial_vi.value.horizontal,initial_vi.value.vertical,initial_vi.value.vertical});
        }
        const bool idle_selected=scenario==Scenario::idle_interrupted || scenario==Scenario::idle_finished;
        idle={idle_inputs.data(),scenario==Scenario::idle_finished?8u:idle_inputs.size(),0x13579bdfu,0};
        if (idle_selected) state.set_idle_script(&idle);
        require(input.start(sample())==game_input::Error::none,"recorded physical acquisition start failed");
        state.select_controller(input.snapshot()); require(state.controller==0,"fresh application did not select physical port zero");
        native(photo_resources.bind(application,*state.save()));
        isle_player_photo_func_8009CE00(&application);
        auto hydrated=game_audio::hydrate(snapshot); require(bool(hydrated),"Root audio resource admission failed");
        std::array<audio::effects::Section,8> sections; audio::effects::Config effects;
        require(game_audio::native_startup_effects(hydrated.resources,sections,effects)==audio::Error::none,"Root audio effects admission failed");
        pcm.samples.resize(8192*audio::AudioService::max_frames*2);
        auto made_audio=game_audio::Session::create(std::move(hydrated.resources),effects,pcm.output(),execution.executor());
        require(bool(made_audio),"Root actual Audio Session creation failed"); audio=std::move(made_audio.session);
        audio_clock=std::make_unique<Audio>(Audio{*audio,execution,log});
        native(storage.create()); storage_created=true;
        services=std::make_unique<scene::Services>(storage.game(),storage.arena(),
            scene::ResourceView{{8,16,32,3,4},ui_resources.resources()},input,*audio,state);
        music=std::make_unique<beach::Music>(*services,snapshot);
        movement_hook.music=music.get();
        player::Root::Config config{0,Events::end,&events,Events::pause,&events,
            {&movement_hook,nullptr,scenario==Scenario::no_background_music?nullptr:MovementHook::update},
            nullptr,0,isle_item_source_beach};
        root=std::make_unique<player::Root>(*services,ui_resources,config);
        native(music->bind(source()));
        IslePlayerSource* canonical=nullptr; native(isle_player_source_context(services->game,&canonical));
        require(canonical==&source() && !source().object && !source().model,"Root cold canonical activation lost identity");
        scene_root=services->object(0,0,0x80000000u); require(scene_root,"actual scene root0 allocation failed");
        // This fixture owns helper0, but CollisionPass owns its secondary
        // lifecycle slot. Keep one native identity hold and use the public
        // joined/collectable queries instead of borrowing that observer slot.
        require(services->source_owner_register(scene_root,{}),
            "actual scene root native identity hold admission failed");
        scene_root_registered=true;
        commands=std::make_unique<player::commands::Binding>(*services,source());
        ui=std::make_unique<player::Ui>(ui_resources,*services,source(),sprites,0);
        bump=std::make_unique<player::BumpOwner>(*services,source());
        require(!source().ui,"cold Ui constructor published Source.ui before Root.bind");
        auto made=g::Frame::create(snapshot,{true,{16000000,4*1024*1024},frame_layout});
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
        const auto append=[&](auto spans) { for (const auto& span:spans) immutable.push_back({physical(span.guest_address),
            {reinterpret_cast<const std::uint8_t*>(span.bytes.data()),span.bytes.size()}}); };
        append(world_resources.installation_spans()); append(actor_resources.installation_spans());
        append(item_resources.installation_spans()); append(cart_resources.installation_spans());
        for (const auto span:effect_graphics.installation_spans()) immutable.push_back(span);
        for (const auto span:transition_resources.immutable()) immutable.push_back(span);
        // Renderer adds its actual Frame/camera reservations itself. The complete
        // union below is nevertheless fixed before constructing ANY producer.
        for (const auto range:all_ranges) {
            bool external=range.offset==lanes.address && range.size==lanes.size;
            for (const auto core:frame->reservations())
                external|=range.offset==core.address && range.size==core.size;
            for (const auto fb:actor_layout.framebuffers)
                external|=range.offset==fb.offset && range.size==fb.size;
            for (const auto asset:effect_graphics.reservations())
                external|=range.offset==asset.offset && range.size==asset.size;
            if (!external) reservations.push_back({static_cast<std::uint32_t>(range.offset),static_cast<std::uint32_t>(range.size)});
        }
        player::Renderer::Config render_config;
        render_config.resources={1,&sprites,immutable,reservations,{0x3b5000,0x3da800},physical(lighting.pre_render_guest())};
        render_config.prepare=Content::prepare; render_config.prepare_user=&content;
        render_config.programs=&programs; render_config.effect_resources=&effect_graphics;
        render_config.sprite_capacity=1024;
        renderer=std::make_unique<player::Renderer>(*services,source(),lighting,render_config);
        IsleGameObject* ui_camera=nullptr; native(renderer->create_main(0x385878ff,&ui_camera));
        require(ui_camera && source().camera,"original Renderer cameras were not created before Root.init");
        native(isle_game_camera_object(services->game,source().camera,&main_identity));
        environment=std::make_unique<environment_sound::Environment>(*services,storage.arena(),sounds);
        hooks=std::make_unique<Hooks>(Hooks{*services,*root,actor_resources});
        world=std::make_unique<world::World>(*services,world_resources,world_module,
            world::Callbacks{hooks.get(),Hooks::add,Hooks::remove,Hooks::change,Hooks::shift_camera,Hooks::shift_effects,nullptr});
        hooks->world=world.get();
        const auto capacity=[](const auto& definitions,a::SlotCapacity& maximum) {
            for (const auto& model:definitions) { const auto value=a::slot_capacity(model);
                maximum.nodes=std::max(maximum.nodes,value.nodes);
                maximum.material_attachments=std::max(maximum.material_attachments,value.material_attachments); }
        };
        a::SlotCapacity actor_capacity{},item_capacity{};
        capacity(actor_resources.models(),actor_capacity); capacity(item_resources.models(),item_capacity);
        actor_capacities.fill(actor_capacity); item_capacities.fill(item_capacity);
        cart_capacities.fill(a::slot_capacity(cart_resources.animation_resources().models[0]));
        actors=std::make_unique<actor::Owner>(*services,actor_resources,actor::CourseContext(sceneState),*world,*environment,detector,source(),actor_capacities,banks,a::ExecutionCapacity{32768,4096});
        hooks->actors=actors.get();
        items=std::make_unique<items::Owner>(*services,item_resources,*world,*actors,globals,source(),item_capacities,a::ExecutionCapacity{32768,4096});
        cart=std::make_unique<player::cart::Owner>(*services,cart_resources,source(),cart_capacities,a::ExecutionCapacity{});
        behavior_bindings=std::make_unique<actor::BehaviorBindings>(behaviors,beach::initial_species_callbacks(),
            beach::initial_species_graphs(),std::span<const std::uint32_t>{},beach::initial_species_roots());
        interactions=std::make_unique<actor::InteractionServices>(*services,*actors,*items,*world,detector,*behavior_bindings,actor_resources,effect_resources);
        accepted(actors->bind_interactions(*interactions));
        magikarp=std::make_unique<beach::MagikarpScene>(*services,*actors,*world,actor_resources,sceneState);
        accepted(items->set_beach_collision(*magikarp));
        require(!source().ui && !source().object && !source().model && !actors->particle_object() && !actors->effect_object(),
            "Root prebind ordering was bypassed");
        native(root->bind({*world,*actors,*items,*ui,*bump,*renderer,application}));
        require(source().ui==&ui->source(),"Root.bind did not publish the genuine retained Ui");
        native(root->initialize());
        native(renderer->bind_ui(*ui,message_commands));
        require(hooks->count==8,"original Movement_Init did not invoke exactly the current/next eight factories");
        constexpr std::array<std::int32_t,8> expected{12,84,25,1004,16,16,16,131};
        for (std::size_t i=0;i<expected.size();++i) {
            const auto& entry=hooks->factories[i];
            require(entry.species==expected[i] && entry.before_player && entry.block->index==(i==7?1u:0u) && entry.reference->index==0,
                "original current/next factory tuple or prepublication source order changed");
        }
        require(hooks->factories[2].behavior==4 && hooks->factories[4].factory==0x802c8d3cu,
            "ordinary Pikachu behavior4/Pidgey factory authority was replaced");
        root_identity=source().object; reticle_identity=source().D_80382C38_523048;
        const auto identity=[&](IsleGameObject* object,std::uint32_t id,std::uint8_t link) {
            IsleGameObjectMetadata metadata{}; native(isle_game_object_metadata(services->game,object,&metadata));
            require(metadata.id==id && metadata.update_link==link && metadata.priority==0x80000000u,
                "original Root/reticle/pause identity was substituted");
            require(services->source_owner_registered(object),"Root native FIFO identity lacks semantic owner registration");
        };
        identity(root_identity,7,9); identity(reticle_identity,25,0);
        identity(source().ObjectPauseMenu,26,0);
        identity(bump->source().BumpDetector_Object,8,0);
        const std::array observed{root_identity,reticle_identity,source().ObjectPauseMenu,
            bump->source().BumpDetector_Object};
        for (std::size_t i=0;i<observed.size();++i) {
            require(services->observe_lifecycle(observed[i],
                {NativeJoin::on_retiring,NativeJoin::on_joined,&native_joins[i]}),
                "native Root/reticle/pause/Bump join observer admission failed");
            native_joins[i].object=observed[i];
        }
        IslePlayerSource* bound=nullptr;
        native(isle_player_source_get(services->game,root_identity,&bound));
        require(bound==&source() && source().commands==commands->source(),
            "Player7 native payload/FIFO did not use the sole canonical Source");
        log.add("native-identities",static_cast<const void*>(root_identity),
            static_cast<const void*>(reticle_identity),static_cast<const void*>(source().ObjectPauseMenu),
            static_cast<const void*>(bump->source().BumpDetector_Object),
            static_cast<const void*>(ui->source().Msg_CameraObject),static_cast<const void*>(bound));
        require(source().ObjectPauseMenu && source().MainCameraReticles[0] && source().MainCameraReticles[3] &&
            source().D_803AE458_54E868[5] && ui->source().Icons_MainObject && ui->source().D_803B09D8_550DE8 &&
            ui->source().Msg_CameraObject && source().bump && globals.source.owner,
            "original initUI did not construct its reticle/pause/Ui/Items/Bump/message domains");
        const IsleGameModelNode* nodes=nullptr; std::size_t node_count=0;
        native(isle_game_model_peek_nodes(services->game,source().model,&nodes,&node_count));
        const IsleGameModelPlacement* placement=nullptr;
        native(isle_game_model_placement(services->game,source().model,&placement));
        require(node_count==1 && nodes[0].matrix_kind==ISLE_GAME_MODEL_ROTATE_RPY_TRANSLATE_SCALE &&
            nodes[0].transform.scale.x==1 && nodes[0].transform.scale.y==1 && nodes[0].transform.scale.z==1 &&
            placement->mode==ISLE_GAME_MODEL_PLACEMENT_RT && placement->transform.rotation.y==source().movement.rotation.x &&
            placement->transform.position.x==source().movement.position.x && placement->transform.position.y==source().movement.position.y &&
            placement->transform.position.z==source().movement.position.z,
            "original Player7 unit RPY/RTS plus RT placement (including rot.y=x bug) changed");
        require(environment->initialize(source().model,source().camera),"late actual Root Environment initialization failed");
        accepted(actors->publish_effects()); native(renderer->adopt_effects(*actors));
        require(source().D_80382D10_523120==actors->particle_object() && source().D_80382D14_523124==actors->effect_object(),
            "late original FX identities were not adopted");
        photo=std::make_unique<player::Photo>(application,source(),*actors,*items,detection,photo_resources,*world,0);
        photo_scene=std::make_unique<actor::PhotoScene>(*services,*actors,scene_root,source().camera,detection,&application);
        controller=std::make_unique<player::Controller>(*photo,detection); native(renderer->bind_photos(*controller));
        native(isle_player_photo_PokemonDetector_Enable(&application));
        require(detection.enabled,"actual Beach Source detector-enable stage did not enable capture");
        const auto initialized_cart=cart->init(); require(bool(initialized_cart),initialized_cart.status.detail);
        require(services->observe_lifecycle(initialized_cart.object,
            {NativeJoin::on_retiring,NativeJoin::on_joined,&native_joins[4]}),
            "actual Cart native/semantic join observer admission failed");
        native_joins[4].object=initialized_cart.object;
        accepted(cart->set_animation(cart->intro_model(),cart->intro_material(),0,0));
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
        worlds=std::make_unique<world::Producer>(services->game,source().camera,services->camera_basis(),world_resources,world->render_view(),world_layout,wo);
        actor_producer=std::make_unique<actor::Producer>(services->game,source().camera,services->camera_basis(),actor_resources,actor_capacities,actors->render_bindings(),actor_layout,ao,&detection);
        item_producer=std::make_unique<menu::model::PacketProducer>(services->game,source().camera,services->camera_basis(),
            menu::model::PacketResources{item_resources.models(),item_resources.materials(),item_resources.installation_spans()},
            item_capacities,items->render_bindings(),item_layout,io);
        carts=std::make_unique<player::cart::Producer>(services->game,source().camera,services->camera_basis(),cart_resources,cart_capacities,cart->render_bindings(content.cart_order),cart_layout,co);
        composition=std::make_unique<course::ItemComposition>(course::ItemComposition{*services,*item_producer,256,carts.get(),photo_scene->prefix_object(),true});
        models=std::make_unique<course::ModelProducer>(*worlds,*actor_producer,joined_lists,composition.get());
        content.models=models.get(); content.actors=actors.get(); content.items=items.get(); content.cart=cart.get();
        native(renderer->bind_cart(*cart,content.cart_order));
        native(renderer->bind_transition(transition_resources,services->screen_transition()));
        require(renderer->reserve(*frame),frame->message());
        require(services->set_command_queued_observer(root_identity,Hooks::queued,hooks.get()) &&
            services->set_command_queued_observer(reticle_identity,Hooks::queued,hooks.get()),"native Root/reticle FIFO observer admission failed");
        std::int32_t registered=-1; native(isle_player_root_native_registered_species_count(&source(),&registered));
        require(registered==0 && application.gPhotoCount==0 && application.D_800AE27C==0 &&
            source().IsTutorialEnabled && source().IsInputDisabled && source().D_80382D04_523114==0,
            "Root did not take the actual untouched fresh-profile tutorial branch");
        if (idle_selected) require(source().gIdleScript==&idle && services->rng.seed==idle.rand_seed && source().ProgressFlags==idle.player_flags,
            "application-selected IdleScript did not use genuine original initIdle adoption");
        require(source().sMinTimeBetweenPhotos==ui_resources.minimum_photo_interval() && source().sMinTimeBetweenPhotos==10,
            "Source shutter cooldown did not adopt the admitted ROM value10");
        log.add("scope","generic-original-C-Root-with-Beach-resources","df8815e75f3d79618466e01e0827c6165b93d11c",
            scenario==Scenario::no_background_music?"original-null-course-movement-callback":
                "actual-original-C-Beach-music-update-null-generic-exit-callback",
            "not-complete-Beach-intro-or-fullgame");
        checkpoint("initUI-2324-2550"); check();
        native(isle_player_root_invoke_show_controls(&source()));
        require(source().D_80382D04_523114==0,"show-controls bypassed its original two-tick wait");
        tick(0,0,0,button_b);
        require(source().input.current_buttons==0 && source().D_80382D04_523114==0,
            "physical port1 leaked into Root or Source show-controls did not wait");
        tick(); require(source().D_80382D04_523114==0,"Source show-controls resumed before two real waits");
        tick(); require(source().D_80382D04_523114==1,"Source show-controls did not finish after two real waits");
        // This reaches the third original updateMusic iteration. Only the real
        // admitted Beach C callback may select a song; absent selection, the
        // original unsigned(-1) admission rejection remains nonfatal to Player.
        if (source().LevelBGMSongID==-1) require(!audio->bgm_playing(0),"default no-song semantics changed");
        else require(audio->has_song(static_cast<std::uint32_t>(source().LevelBGMSongID)),
            "actual Source course callback selected an unadmitted song");
        checkpoint("show-controls-2637-2653-and-third-music-343-361");
        draw();
    }
    void no_background_music() {
        require(!source().gIdleScript && source().LevelBGMSongID==-1,
            "no-song regression did not reach the ordinary Root music path");
        tick(); release();
        require(!audio->bgm_playing(0),
            "original rejected song request unexpectedly started a background sequence");
        checkpoint("original-no-song-third-music-iteration-343-361");
        draw("no-background-music");
    }
    void fresh() {
        require(!source().gIdleScript,"fresh tutorial unexpectedly adopted an idle script");
        unsigned zoom_ticks=0;
        for (;zoom_ticks<32 && source().gDirectionIndex!=-1;++zoom_ticks) cycle(button_z);
        require(source().gDirectionIndex==-1 && zoom_ticks>=10,"physical Z did not execute genuine ten-wait zoom");
        capture("zoomed-root"); checkpoint("zoom-934-1006");
        unsigned observations=0, since_shutter=16;
        while (application.D_800AE27C<3 && observations<600) {
            // The prior actual GPU completion remains held across this later OM
            // CopyInfo, and through any physical A shutter on the same context.
            tick(button_z); ++observations; ++since_shutter;
            const auto published=detector.published();
            const auto ticket=controller->ticket(*frame);
            log.add("later-om-copy-info",om,draws,frame->submission(),ticket.context,ticket.generation,
                published.present,published.context,reinterpret_cast<const void*>(published.object),published.pokemon_id,published.flags);
            const bool eligible=published.present && published.object && published.pokemon_id>0 &&
                published.pokemon_id<=151 && (published.flags&4u) && source().gDirectionIndex==-1 &&
                source().TargetDirectionZoomedIn==0 && source().sTimerAfterPhotoTaken<0 && since_shutter>=16;
            if (eligible) {
                const auto before=application.D_800AE27C;
                const auto total=application.gPhotoCount;
                tick(button_z|button_a); ++observations;
                require(application.gPhotoCount==total+1 && application.D_800AE27C==before+1,
                    "eligible actual physical A shutter did not produce a qualifying Pokemon photo");
                require(controller->ticket(*frame)==ticket,"Source shutter changed its held completed GPU ticket");
                capture("qualifying-"+std::to_string(application.D_800AE27C));
                checkpoint("physical-shutter-1362-1411"); since_shutter=0;
            }
            release(); draw();
        }
        require(application.D_800AE27C>=3,"600 real focus observations exhausted before THREE qualifying photos; no state/focus patch permitted");
        require(application.gPhotoCount>=application.D_800AE27C,"qualifying and total photo counters were conflated");
        for (unsigned n=0;n<8 && source().IsInputDisabled;++n) cycle(button_z);
        require(!source().IsInputDisabled,"three qualifying photos did not release original tutorial input gate");
        checkpoint("tutorial-three-qualifying-2026-2039");
        // Original follow-up: 90 ticks, an A-dismissible message, then 600 ticks
        // observing real stick input. These are physical records, not C++ Player
        // transitions; Source alone owns all pause/resume and message gates.
        for (unsigned n=0;n<760;++n) cycle((n%16==1)?button_a:0,8,0);
        require(message_frames && message_saved,"actual tutorial Source message/glyph draw was never visible");
        checkpoint("tutorial-follow-up-1943-1957");
        release(); tick(button_start);
        require(source().IsPaused && events.pause_calls==1 && !detection.enabled,
            "physical Start did not enter original pause callback/UI/Detector path");
        checkpoint("pause-1753-1817");
        for (unsigned n=0;n<31;++n) cycle();
        const auto* paused_label=services->view(source().Pause_LabelPause2);
        require(paused_label && !(paused_label->attr&ISLE_SP_HIDDEN) && rectangles,
            "original pause label/UI was not visible in actual Root GPU output");
        capture("paused-root");
        release(); tick(button_b);
        require(source().IsPaused && !events.resume_calls,"resume bypassed its genuine 21-tick process");
        unsigned waited=0;
        while (!events.resume_calls && waited<24) {
            cycle(); ++waited;
            if (waited<21) require(source().IsPaused,"Source pause cleared before 21 actual resume waits");
        }
        require(waited>=21 && events.resume_calls==1 && !source().IsPaused && detection.enabled,
            "original resume process did not restore controls/Detector after 21 waits");
        for (unsigned n=0;n<24;++n) cycle();
        const auto* resumed_label=services->view(source().Pause_LabelPause2);
        require(resumed_label && (resumed_label->attr&ISLE_SP_HIDDEN),
            "original pause label survived the actual resume hide process");
        capture("resumed-root");
        require(!events.request_exit,"fresh tutorial/pause recording unexpectedly requested course exit");
        checkpoint("resume-1730-1749"); release();
    }
    void idle_case() {
        tick(button_b);
        require(source().input.current_buttons==idle_inputs[3].current_buttons && source().input.pressed_buttons==0,
            "original priority10 IdleScript did not override pre-OM physical input");
        checkpoint("idle-287-337"); release(); draw("idle-script");
        const auto before_fade_nonblack=nonblack_pixels;
        require(before_fade_nonblack!=0,"actual idle fade has no visible RGB baseline");
        const bool interrupt=scenario==Scenario::idle_interrupted;
        // The original check runs AFTER its wait, before the next scripted
        // publication. A fresh physical A edge is visible at precisely that gate.
        for (unsigned n=0;n<240 && !events.request_exit;++n) {
            cycle(interrupt && n==0 ? button_a : 0);
        }
        require(events.request_exit && events.end_calls==1 && events.reason==(interrupt?8:7),
            "actual application IdleScript did not reach its original interrupted/finished end callback");
        require(!events.pause_calls && !events.resume_calls,"idle scene claimed nonexistent normal pause updater coverage");
        if (!(transition_frames && visible_transition_frames && fade_saved && nonblack_pixels<before_fade_nonblack))
            throw std::runtime_error("actual Source idle fade lacks visible RGB blackening: before="+
                std::to_string(before_fade_nonblack)+" after="+std::to_string(nonblack_pixels)+
                " active_frames="+std::to_string(transition_frames)+" visible_frames="+
                std::to_string(visible_transition_frames)+" saved="+std::to_string(fade_saved));
        capture("idle-terminal"); checkpoint(interrupt?"idle-interrupted-301-312":"idle-finished-316-329");
        log.add("source-postprocess-removed",om,controller->queued_requests(),frame->submission(),
            controller->has_pending(*frame),photo->quiescent());
        const auto queued=controller->queued_requests();
        require(queued==2,"original postprocess removal dropped or failed to retain its real two-entry Source FIFO");
        release();
        require(controller->queued_requests()==queued && photo->quiescent(),
            "GPU hold release was conflated with unconsumed Source FIFO teardown");
    }
    bool close_failure(const char* operation, const char* detail, int status) noexcept {
        // Failure-only, bounded observations. Never advance OM or release a
        // borrower merely to make this diagnostic look quiescent.
        try {
            std::cerr << "Root cleanup failure: operation=" << operation
                << " status=" << status << " detail=" << detail << '\n';
            std::cerr << "  om=" << om << " draws=" << draws
                << " completed_hold=" << completed_hold << " storage=" << storage_created
                << " input_running=" << input.running() << '\n';
            std::cerr << "  scene-root object=" << static_cast<const void*>(scene_root)
                << " registered=" << scene_root_registered
                << " joined=" << bool(services && scene_root && services->source_owner_joined(scene_root))
                << " collectable=" << bool(services && scene_root && services->source_owner_collectable(scene_root))
                << '\n';
            if (frame) std::cerr << "  Frame busy=" << frame->busy()
                << " quiescent=" << frame->quiescent() << " completed=" << bool(frame->completed())
                << " submission=" << frame->submission() << " error=" << int(frame->error())
                << " message=" << frame->message() << '\n';
            if (renderer) std::cerr << "  Renderer status=" << int(renderer->status())
                << " message=" << renderer->message() << '\n';
            std::cerr << "  Photo quiescent=" << (!photo || photo->quiescent())
                << " pending_frame=" << bool(controller && frame && controller->has_pending(*frame))
                << " queued=" << (controller ? controller->queued_requests() : 0) << '\n';
            if (services) {
                IsleGameCounts counts{};
                const auto counted=isle_game_counts(services->game,&counts);
                std::cerr << "  Services failed=" << services->failed()
                    << " game_status=" << int(isle_game_status(services->game))
                    << " message=" << services->error() << " counts_status=" << int(counted)
                    << " objects=" << counts.objects << " processes=" << counts.processes
                    << " models=" << counts.models << " cameras=" << counts.cameras << '\n';
                for (std::size_t i=0;i<native_joins.size();++i) {
                    const auto& join=native_joins[i];
                    if (!join.object) continue;
                    const auto borrows=services->source_owner_borrows(join.object);
                    std::cerr << "  native[" << i << "] object=" << static_cast<const void*>(join.object)
                        << " retiring=" << join.retiring << " joined=" << join.joined
                        << " invalid=" << join.invalid
                        << " registered=" << services->source_owner_registered(join.object)
                        << " collectable=" << services->source_owner_collectable(join.object)
                        << " queued=" << borrows.queued << " fields=" << borrows.fields << '\n';
                }
                std::size_t debt=0;
                if (actors && hooks) for (std::size_t i=0;i<hooks->count;++i) {
                    const auto& factory=hooks->factories[i];
                    const auto membership=actors->membership(factory.spawn.instance);
                    const auto borrows=services->source_owner_borrows(factory.spawn.object);
                    const bool registered=services->source_owner_registered(factory.spawn.object);
                    if (!factory.invalid_join && factory.native_retiring==1 && factory.native_joined==1 &&
                        !registered && !membership.logical && !membership.registered && !membership.retiring)
                        continue;
                    if (++debt>8) continue;
                    std::cerr << "  Actor[" << i << "] species=" << factory.species
                        << " retiring=" << factory.native_retiring << " joined=" << factory.native_joined
                        << " invalid=" << factory.invalid_join << " registered=" << registered
                        << " logical=" << membership.logical << " block=" << membership.registered
                        << " slot_retiring=" << membership.retiring << " slot_joined=" << membership.joined
                        << " queued=" << borrows.queued << " fields=" << borrows.fields << '\n';
                }
                std::cerr << "  Actor debt_records=" << debt << " shown=" << std::min<std::size_t>(debt,8)
                    << " Items total=" << globals.source.total << " apples=" << globals.source.apples
                    << " balls=" << globals.source.balls << '\n';
            }
            if (root) {
                const auto& s=root->source();
                std::cerr << "  Source status=" << int(s.status) << " active=" << bool(s.native_root)
                    << " call_frame=" << bool(s.nonwaiting_call_frame) << " object=" << static_cast<const void*>(s.object)
                    << " renderer=" << bool(s.native_renderer) << " commands=" << bool(s.commands)
                    << " ui=" << bool(s.ui) << " bump=" << bool(s.bump) << " cart=" << bool(s.cart)
                    << " photo_owner=" << bool(s.photo && s.photo->owner) << '\n';
                if (services) for (auto* object : {s.D_80382D10_523120,s.D_80382D14_523124}) {
                    if (!object) continue;
                    const auto borrows=services->source_owner_borrows(object);
                    std::cerr << "  Source FX object=" << static_cast<const void*>(object)
                        << " registered=" << services->source_owner_registered(object)
                        << " joined=" << services->source_owner_joined(object)
                        << " queued=" << borrows.queued << " fields=" << borrows.fields << '\n';
                }
            }
            if (ui) {
                const auto& s=ui->source();
                std::cerr << "  Ui icons=" << static_cast<const void*>(s.Icons_MainObject)
                    << " digits=" << static_cast<const void*>(s.D_803B09D8_550DE8)
                    << " message_camera=" << static_cast<const void*>(s.Msg_CameraObject)
                    << " status=" << int(s.status) << '\n';
            }
            if (audio) std::cerr << "  Audio state=" << int(audio->state())
                << " accepted_frames=" << audio->accepted_frames() << " pcm_used=" << pcm.used
                << " finishes=" << pcm.finishes << '\n';
            std::cerr << std::flush;
            log.add("cleanup-failure",operation,status,detail);
            log.stream.flush();
        } catch (...) {
            std::fputs("Root cleanup diagnostic could not finish; borrowed graph retained\n",stderr);
        }
        return false;
    }
    bool close() noexcept {
        // Never let an error replace the original failure or destroy an owner
        // whose actual native/semantic/GPU borrowers have not been released.
        const char* operation="begin";
        const auto reject=[&](const char* detail, int status=0) {
            return close_failure(operation,detail,status);
        };
        try {
            if (closed) return true;
            if (renderer && frame) {
                operation="Renderer.poll-terminal";
                if (frame->busy()) {
                    g::Poll result;
                    do { result=renderer->poll(); if (result==g::Poll::pending) std::this_thread::yield(); }
                    while (result==g::Poll::pending);
                    if (result!=g::Poll::completed)
                        return reject("actual GPU terminal poll did not complete",int(result));
                }
                operation="Frame.quiescent";
                // This is real GPU/readback exclusion, not the independent
                // completed Photo hold. release_completed itself requires it.
                if (!frame->quiescent()) return reject("actual executor/readback ownership remains");
                operation="Renderer.release_completed";
                if (controller && controller->has_pending(*frame)) {
                    const auto result=renderer->release_completed();
                    if (result!=ISLE_GAME_OK) return reject("completed Photo hold release rejected",int(result));
                }
                completed_hold=false;
            }
            operation="Photo.quiescent";
            if (photo && !photo->quiescent()) return reject("actual Photo snapshot/ticket borrower remains");
            operation="PhotoScene.retire";
            if (photo_scene) { photo_scene->retire(); photo_scene.reset(); }
            operation="Photo.Source-FIFO-teardown";
            if (controller && controller->queued_requests()!=0) return reject("unconsumed Source FIFO remains");
            log.add("retire","PhotoScene-real-queue-teardown",controller?controller->queued_requests():0);
            operation="producers.release";
            models.reset(); composition.reset(); carts.reset(); item_producer.reset(); actor_producer.reset(); worlds.reset();
            content.models=nullptr; content.actors=nullptr; content.items=nullptr; content.cart=nullptr;
            // Native joins close each receiver's actual FIFO/field credits.
            // Only then collect Source identities, once, outside OM.
            operation="Root.retire_objects";
            const auto root_request=root?root->retire_objects():ISLE_GAME_OK;
            if (root_request!=ISLE_GAME_OK) return reject("actual Root native join remains",int(root_request));
            operation="Items.retire";
            if (items) {
                const auto result=items->retire();
                if (!result) return reject(result.detail,int(result.error));
            }
            operation="Cart.retire";
            if (cart) {
                const auto result=cart->retire();
                if (!result) return reject(result.detail,int(result.error));
            }
            operation="Actors.reset";
            if (actors) {
                const auto result=actors->reset();
                if (!result) return reject(result.detail,int(result.error));
            }
            operation="Actors.native-join-observers";
            if (actors && hooks) for (std::size_t i=0;i<hooks->count;++i) {
                const auto& factory=hooks->factories[i];
                const auto membership=actors->membership(factory.spawn.instance);
                const auto borrows=services->source_owner_borrows(factory.spawn.object);
                log.add("join","Actor",i,factory.species,factory.native_retiring,factory.native_joined,
                    membership.logical,membership.registered,membership.retiring,membership.joined,borrows.queued,borrows.fields);
                if (factory.invalid_join || factory.native_retiring!=1 || factory.native_joined!=1)
                    return reject("Actor observer did not prove exactly one actual retire/join",int(i));
            }
            operation="Root.collect_joined";
            if (root) {
                const auto result=root->collect_joined();
                if (result!=ISLE_GAME_OK) return reject("Root incoming Source FIFO/field credits remain",int(result));
            }
            operation="Items.collect_joined";
            if (items) {
                const auto result=items->collect_joined();
                if (!result) return reject(result.detail,int(result.error));
            }
            log.add("join","Items-logical-counts",globals.source.total,globals.source.apples,globals.source.balls);
            operation="Items.logical-counts";
            if (globals.source.total || globals.source.apples || globals.source.balls)
                return reject("Source Item logical counts remain after collection");
            operation="Cart.collect_joined";
            if (cart) {
                const auto result=cart->collect_joined();
                if (!result) return reject(result.detail,int(result.error));
            }
            operation="Bump.retire";
            if (bump) {
                const auto result=bump->retire();
                if (result!=ISLE_GAME_OK) return reject("actual Bump native/semantic join remains",int(result));
            }
            operation="native-join-observers";
            for (std::size_t i=0;i<native_joins.size();++i) {
                const auto& join=native_joins[i];
                if (!join.object) continue;
                log.add("native-join",i,static_cast<const void*>(join.object),join.retiring,join.joined,
                    services->source_owner_registered(join.object));
                if (join.invalid || join.retiring!=1 || join.joined!=1 ||
                    services->source_owner_registered(join.object))
                    return reject("native observer/Source identity did not close",int(i));
            }
            operation="Actors.semantic-identities";
            if (actors && hooks) for (std::size_t i=0;i<hooks->count;++i)
                if (services->source_owner_registered(hooks->factories[i].spawn.object))
                    return reject("Actor semantic identity remains registered",int(i));
            log.add("join","Items-Actors-Cart-Bump-native-and-semantic-collection");
            operation="Root.semantic-identity";
            if (services && root_identity) {
                const auto borrows=services->source_owner_borrows(root_identity);
                log.add("join","Player7-semantic-release",root_request,native_joins[0].joined,
                    services->source_owner_registered(root_identity),borrows.queued,borrows.fields);
                if (services->source_owner_registered(root_identity)) return reject("Player7 semantic identity remains");
            }
            operation="reticle.semantic-identity";
            if (services && reticle_identity) {
                const auto borrows=services->source_owner_borrows(reticle_identity);
                log.add("join","reticle25",services->source_owner_registered(reticle_identity),borrows.queued,borrows.fields);
                if (services->source_owner_registered(reticle_identity)) return reject("reticle25 semantic identity remains");
            }
            // Foreign primary owners stay alive while composed Renderer
            // observers receive genuine native joins and release identity holds.
            operation="Ui.retire";
            if (ui) {
                const auto result=ui->retire();
                if (result!=ISLE_GAME_OK) return reject("actual Ui native join remains",int(result));
            }
            operation="Actors.FX-native-joins";
            if (actors) {
                if (actors->particle_object()) services->erase(actors->particle_object());
                if (actors->effect_object()) services->erase(actors->effect_object());
                if (actors->particle_object() || actors->effect_object()) return reject("actual Actor FX joins remain");
            }
            // Main camera owns the real canonical -6/-7 field credits.
            // detach retires that receiver before collecting its joined FX
            // senders; collect_joined before detach would reject true debt.
            operation="Renderer.detach-native-receivers-then-collect";
            if (renderer) {
                const auto detached=renderer->detach();
                if (detached!=ISLE_GAME_OK) return reject("Renderer camera/observer detach rejected",int(detached));
                renderer.reset();
            }
            log.add("join","Renderer-foreign-primary-then-secondary-detach");
            operation="Frame.retire";
            if (frame) { if (!frame->retire()) return reject("actual Frame ownership remains"); frame.reset(); }
            operation="World.retire";
            if (world && !world->retire()) return reject("actual World retirement rejected");
            operation="scene-root.erase";
            if (scene_root) {
                const auto prior_fault=isle_game_status(services->game);
                if (!scene_root_registered || !services->source_owner_joined(scene_root)) {
                    const auto erased=services->erase(scene_root);
                    if (erased!=ISLE_GAME_OK && erased!=prior_fault)
                        return reject("actual scene root native deletion rejected",int(erased));
                }
                if (scene_root_registered) {
                    operation="scene-root.native-join";
                    IsleGameObjectMetadata expired{};
                    const auto metadata=isle_game_object_metadata(services->game,scene_root,&expired);
                    const bool joined=services->source_owner_joined(scene_root);
                    const bool collectable=services->source_owner_collectable(scene_root);
                    log.add("native-join","scene-root0",static_cast<const void*>(scene_root),
                        joined,collectable,int(metadata));
                    if (!joined || !collectable || metadata!=ISLE_GAME_INVALID_ARGUMENT)
                        return reject("scene root identity hold did not preserve actual joined-dead state",int(metadata));
                    // Exercise the real native allocator only on a healthy
                    // run; earlier terminal faults still require genuine join
                    // and hold release, never new work or failure suppression.
                    operation="scene-root.identity-pressure";
                    if (!services->failed() && prior_fault==ISLE_GAME_OK) {
                        IsleGameObject* pressure=nullptr;
                        const auto allocated=isle_game_add_object(services->game,UINT32_MAX,0,0,nullptr,nullptr,&pressure);
                        if (allocated!=ISLE_GAME_OK)
                            return reject("native identity pressure allocation rejected",int(allocated));
                        const bool distinct=pressure!=scene_root;
                        const auto erased=isle_game_delete_object(services->game,pressure);
                        if (erased!=ISLE_GAME_OK)
                            return reject("native identity pressure retirement rejected",int(erased));
                        log.add("native-identity-pressure","scene-root0",static_cast<const void*>(scene_root),
                            static_cast<const void*>(pressure),distinct);
                        if (!distinct) return reject("native allocator reused scene root identity before release");
                    }
                    operation="scene-root.identity-release";
                    if (!services->source_owner_release(scene_root))
                        return reject("actual scene root native identity hold remains");
                    scene_root_registered=false;
                }
                scene_root=nullptr;
            }
            operation="dependent-owners.release";
            controller.reset(); photo.reset();
            // Successful Actor reset ends InteractionServices borrowing. Keep
            // World/Magikarp/Environment alive through dependent destruction.
            items.reset(); cart.reset(); bump.reset(); ui.reset(); actors.reset();
            if (environment) environment->cleanup();
            magikarp.reset(); interactions.reset(); behavior_bindings.reset();
            world.reset(); environment.reset(); hooks.reset();
            operation="Commands.detach";
            if (commands) {
                const auto result=commands->detach();
                if (result!=ISLE_GAME_OK) return reject("actual command binding detach rejected",int(result));
                commands.reset();
            }
            if (root) {
                operation="Root.deactivate-LAST";
                const auto status=root->deactivate();
                if (root->source().native_root ||
                    (status!=ISLE_GAME_OK && status!=isle_game_status(services->game)))
                    return reject("canonical Source context was not actually deactivated",int(status));
                log.add("join","Source-deactivate-LAST",status);
                operation="Music.detach";
                if (music) {
                    const auto result=music->detach();
                    if (result!=ISLE_GAME_OK) return reject("retained course music Source borrow remains",int(result));
                }
                movement_hook.music=nullptr;
                music.reset();
                root.reset();
            }
            if (music) {
                operation="Music.detach";
                const auto result=music->detach();
                if (result!=ISLE_GAME_OK) return reject("retained course music Source borrow remains",int(result));
                music.reset();
            }
            operation="Services.release";
            services.reset();
            operation="Storage.destroy";
            if (storage_created) {
                const auto result=storage.destroy();
                if (result!=ISLE_GAME_OK) return reject("actual native Storage borrowers remain",int(result));
                storage_created=false;
            }
            operation="Input.stop";
            if (input.running()) {
                const auto result=input.stop();
                if (result.error!=game_input::Error::none) return reject("actual input stop rejected",int(result.error));
            }
            // Audio is independent: native/graphics joins do not finish PCM.
            if (audio) {
                operation="Audio.stop-and-independent-PCM-drain";
                require(audio->stop()==audio::Error::none,"Root independent Audio stop failed");
                for (unsigned n=0;n<240 && audio->state()!=game_audio::Session::State::draining;++n) audio_clock->tick();
                require(audio->state()==game_audio::Session::State::draining && audio->service().voices_retired(),
                    "Root actual Audio voices did not retire");
                require(audio->poll_drain()==audio::Error::none && audio->poll_drain()==audio::Error::none &&
                    audio->state()==game_audio::Session::State::drained && pcm.finishes==1 &&
                    !audio->service().has_lease() && audio->accepted_frames()*2==pcm.used,
                    "Root actual owned PCM drain/finish did not close exactly once");
                require(audio_clock->tasks && pcm.used,"Root actual Audio produced no owned PCM evidence");
                log.add("audio-drained",audio_clock->retrace,audio_clock->tasks,audio->source_frame_epoch(),
                    audio->accepted_frames(),pcm.used,pcm.finishes,pcm.polls);
                // Explicit LE wire output, independent of the host's int16 layout.
                std::vector<std::byte> bytes(pcm.used*2);
                for (std::size_t i=0;i<pcm.used;++i) {
                    const auto value=static_cast<std::uint16_t>(pcm.samples[i]);
                    bytes[i*2]=std::byte(value&255u); bytes[i*2+1]=std::byte(value>>8);
                }
                save(output/"audio.s16le",bytes);
                audio_clock.reset(); audio.reset();
            }
            closed=true; log.add("closed","native-and-semantic-joins-distinct-from-GPU-and-PCM-drain");
            return true;
        } catch (const std::exception& error) {
            return reject(error.what());
        } catch (...) { return reject("non-standard cleanup exception"); }
    }
};
void run(const rom::Snapshot& snapshot, const std::filesystem::path& output, Scenario scenario) {
    require(std::filesystem::create_directory(output),"Root scenario output must be new");
    auto scene=std::make_unique<Case>(snapshot,output,scenario);
    std::exception_ptr first_failure;
    try {
        scene->build();
        if (scenario==Scenario::fresh) scene->fresh();
        else if (scenario==Scenario::no_background_music) scene->no_background_music();
        else scene->idle_case();
    } catch (...) { first_failure=std::current_exception(); }
    if (!scene->close()) {
        // An unresolved borrower is an error, NEVER authority to free userdata.
        // Keep this entire private process-lifetime graph intact on that path.
        (void)scene.release();
        if (first_failure) std::rethrow_exception(first_failure);
        throw std::runtime_error("Root cleanup could not prove all actual joins; borrowed graph deliberately retained");
    }
    if (first_failure) std::rethrow_exception(first_failure);
}
}
int main(int argc, char** argv) {
    try {
        require(argc==3,"usage: player-root-rsp-smoke PRIVATE_ROM NEW_OUTPUT_DIRECTORY");
        const std::filesystem::path output=argv[2];
        require(!output.empty() && !std::filesystem::exists(output),"Root output root must be a fresh directory");
        require(std::filesystem::create_directories(output),"Root output root creation failed");
        const auto snapshot=rom::read_snapshot(argv[1]);
        isle::test::source_camera_contract(snapshot,output/"camera-contract");
        run(snapshot,output/"fresh-tutorial",Scenario::fresh);
        run(snapshot,output/"idle-interrupted",Scenario::idle_interrupted);
        run(snapshot,output/"idle-finished",Scenario::idle_finished);
        run(snapshot,output/"no-background-music",Scenario::no_background_music);
        std::cout << "generic original-C Root/physical-input/tutorial/idle/no-song acceptance passed; "
            "complete Beach intro/fullgame/hardware/Windows11 not claimed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "player-root-rsp-smoke: " << error.what() << '\n';
        return 1;
    }
}

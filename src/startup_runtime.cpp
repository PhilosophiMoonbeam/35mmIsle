#include "startup_runtime.hpp"
#include "menu_scene_bridge.h"
#include "scene_storage.hpp"
#include "screen_transition_source.h"

#include <algorithm>
#include <exception>
#include <utility>

namespace isle::startup {
struct Runtime::Impl {
    const menu::Catalog& catalog;
    game_input::Service& input;
    game_audio::Session& audio;
    ::isle::game::State& state;
    scene::Storage& storage;
    isle_arena& arena;
    Phase phase = Phase::logo;
    Error error = Error::none;
    std::array<char,512> message{};
    std::uint32_t raw_time = 0, logo_ticks = 0;
    std::uint64_t updates = 0;
    bool logo_exit = false;
    bool native_owned = false;
    Exit requested{};
    IsleGameSprite* logo_sprite = nullptr;
    IsleGameCamera* sprite_camera = nullptr;
    std::unique_ptr<scene::Services> services;
    std::unique_ptr<menu::scene::Title> title;
    std::unique_ptr<menu::scene::Ui> ui;
    IsleGameCommandPool logo_commands{};
    struct CommandOwner {
        Impl* runtime = nullptr;
        IsleGameObject* object = nullptr;
        IsleGameCommandQueue commands{};
    };
    std::array<CommandOwner,3> logo_command_owners{};

    Impl(const menu::Catalog& resources, game_input::Service& controls,
         game_audio::Session& sounds, ::isle::game::State& persistent,
         scene::Storage& backing, std::uint32_t now)
        : catalog(resources), input(controls), audio(sounds), state(persistent),
          storage(backing), arena(storage.arena()), raw_time(now) {
        logo_commands.arena = &arena;
        for (auto& owner : logo_command_owners) owner.runtime = this;
    }
    ~Impl() { release_scene(); }
    void fail(Error value, const char* reason) noexcept {
        if (error != Error::none) return;
        error = value;
        phase = Phase::failed;
        if (reason == nullptr) reason = "native startup failed";
        std::size_t n = 0;
        while (n + 1 < message.size() && reason[n] != '\0') { message[n] = reason[n]; ++n; }
        message[n] = '\0';
    }
    bool native(IsleGameResult result, const char* reason) noexcept {
        if (result != ISLE_GAME_OK) fail(Error::native,reason);
        return error == Error::none;
    }
    bool sound(::isle::audio::Error result, const char* reason) noexcept {
        if (result != ::isle::audio::Error::none) fail(Error::audio,reason);
        return error == Error::none;
    }
    bool wait(std::uint32_t ticks = 1) noexcept {
        return native(isle_game_wait(storage.game(),ticks),"startup continuation wait failed");
    }
    bool create_world() noexcept {
        if (!native(storage.create(),"startup native world creation failed")) return false;
        native_owned = true;
        return true;
    }
    IsleGameObject* object(std::uint32_t id, std::uint8_t link = 0,
                           std::uint32_t priority = 0x80000000u,
                           CommandOwner* commands = nullptr) noexcept {
        IsleGameObject* result = nullptr;
        if (!native(isle_game_add_object(storage.game(),id,link,priority,
                commands == nullptr ? nullptr : default_update,commands,&result),"startup source object creation failed")) return nullptr;
        if (commands != nullptr) commands->object = result;
        return result;
    }
    IsleGameCamera* camera(IsleGameObject* owner, bool matrices) noexcept {
        if (owner == nullptr) return nullptr;
        IsleGameCamera* result = nullptr;
        if (!native(isle_game_attach_camera(storage.game(),owner,&result),"startup native camera attachment failed")) return nullptr;
        if (matrices &&
            (!native(isle_game_camera_add_matrix(storage.game(),result,ISLE_GAME_CAMERA_PERSPECTIVE_FAST),"startup camera perspective matrix") ||
             !native(isle_game_camera_add_matrix(storage.game(),result,ISLE_GAME_CAMERA_LOOK_AT),"startup camera look-at matrix"))) return nullptr;
        return result;
    }
    bool process(IsleGameObject* owner, IsleGameCallback entry, std::uint32_t priority = 1) noexcept {
        if (owner == nullptr) return false;
        IsleGameProcess* result = nullptr;
        return native(isle_game_add_process(storage.game(),owner,entry,this,ISLE_GAME_STACKFUL,priority,&result),
                      "startup source process creation failed");
    }
    static void default_update(IsleGameObject* object, void* opaque) noexcept {
        auto& owner = *static_cast<CommandOwner*>(opaque);
        auto& self = *owner.runtime;
        int erase = 0;
        if (!self.native(isle_game_commands(self.storage.game(),object,&self.logo_commands,&owner.commands,nullptr,nullptr,&erase),
                         "logo default command processing failed")) return;
        if (erase) {
            isle_game_commands_release(&self.logo_commands,&owner.commands);
            self.native(isle_game_delete_object(self.storage.game(),object),"logo command object deletion failed");
            owner.object = nullptr;
        }
    }
    bool initialize_save() noexcept {
        if (!state.initialize(raw_time)) fail(Error::profile,state.error());
        return error == Error::none;
    }
    static void logo_control(IsleGameObject*, void* opaque) noexcept {
        auto& self = *static_cast<Impl*>(opaque);
        self.state.select_controller(self.input.snapshot());
        const IsleSprite* source = nullptr;
        try { source = self.catalog.sprite(menu::Scene::logo,0x802bc0c0).view(); }
        catch (...) { self.fail(Error::native,"logo initialized sprite resource unavailable"); return; }
        auto* object = self.object(14,0,0x80000000u,&self.logo_command_owners[2]);
        if (object == nullptr || !self.native(isle_game_add_sprite(self.storage.game(),object,source,&self.logo_sprite),
                                             "logo native sprite attachment failed")) return;
        IsleSprite* sprite = nullptr;
        if (!self.native(isle_game_sprite_view(self.storage.game(),self.logo_sprite,&sprite),"logo mutable sprite view failed")) return;
        sprite->attr = ISLE_SP_TEXSHUF | ISLE_SP_TRANSPARENT;
        sprite->x = static_cast<std::int16_t>(160 - sprite->width / 2);
        sprite->y = static_cast<std::int16_t>(120 - sprite->height / 2);
        if (!self.wait() || !self.initialize_save() || !self.wait()) return;
        if (self.logo_ticks < 60 && !self.wait(60-self.logo_ticks)) return;
        if (!self.native(isle_menu_logo_fade(self.storage.game(),self.logo_sprite,20),"exact logo RGB fade failed")) return;
        self.logo_exit = true; // Source func_800067DC: stop GTL, not a timer alias.
        while (self.wait()) {}
    }
    static void logo_counter_entry(IsleGameObject*, void* opaque) noexcept {
        auto& self = *static_cast<Impl*>(opaque);
        do { ++self.logo_ticks; } while (self.wait());
    }
    bool start_logo() {
        if (!native(storage.reset(),"startup closed arena reset failed") || !create_world()) return false;
        const auto& setup = catalog.setup(menu::Scene::logo);
        if (!native(isle_menu_reserve_pools(storage.game(),&arena,setup.objects,setup.processes,
                setup.sprites,setup.cameras,setup.matrices),"logo native initial pools failed")) return false;
        if (camera(object(0xffffffffu,0,0x80000000u,&logo_command_owners[0]),false) == nullptr) return false;
        sprite_camera = camera(object(3,0,0x80000000u,&logo_command_owners[1]),true);
        if (sprite_camera == nullptr) return false;
        auto* control = object(14);
        return process(control,logo_control) && process(control,logo_counter_entry);
    }
    static void common_input(IsleGameObject*, void* opaque) noexcept {
        auto& self = *static_cast<Impl*>(opaque);
        // Reached startup closure never sets source recording mode away from
        // its initialized zero. Its priority0 process invalidates after the
        // priority1 menu controls, exactly once per OM traversal.
        do {
            if (self.input.begin_update() != game_input::Error::none) {
                self.fail(Error::input,"source common menu input invalidation failed"); return;
            }
        } while (self.wait());
    }
    bool start_menu() {
        if (!state.initialized()) { fail(Error::profile,"persistent State has no writable save session"); return false; }
        if (!release_scene() || storage.reset() != ISLE_GAME_OK) {
            fail(Error::native,"main-menu native retirement failed"); return false;
        }
        if (!create_world()) return false;
        const auto& setup = catalog.setup(menu::Scene::title_menu);
        const scene::ResourceView resources{{setup.objects,setup.processes,setup.sprites,setup.cameras,setup.matrices},
                                            catalog.sprites(menu::Scene::title_menu)};
        services = std::make_unique<scene::Services>(storage.game(),arena,resources,input,audio,state);
        services->raw_game_time = raw_time;
        if (services->failed()) { fail(Error::native,services->error()); return false; }
        // Source AA E28 common prefix: player-link priority0 recording/cache
        // process, then the Pokemon-link and link10 auxiliary cameras.
        if (!process(object(1u<<9,9,9),common_input,0) ||
            camera(object(0x200,3,3),true) == nullptr ||
            camera(object(0x200,10,10),true) == nullptr ||
            camera(services->object(0xffffffffu),false) == nullptr) return false;
        title = std::make_unique<menu::scene::Title>(*services,catalog);
        sprite_camera = camera(services->object(3),true);
        if (!native(isle_screen_transition_func_800A7F68(
                services->screen_transition_source(),0,1),"source transition camera")) return false;
        if (sprite_camera == nullptr || services->failed()) {
            fail(Error::native,services->failed() ? services->error() : "main-menu static camera initialization failed"); return false;
        }
        ui = std::make_unique<menu::scene::Ui>(*services,*title,catalog);
        if (!ui->start() || services->failed()) { fail(Error::native,services->error()); return false; }
        phase = Phase::title_menu;
        return true;
    }
    bool release_scene() noexcept {
        if (ui) { ui->retire(); ui.reset(); }
        if (title) { title->retire(); title.reset(); }
        services.reset();
        if (native_owned) {
            if (storage.destroy() != ISLE_GAME_OK) {
                fail(Error::native,"startup native world retirement failed"); return false;
            }
            native_owned = false;
        }
        for (auto& owner : logo_command_owners) {
            isle_game_commands_release(&logo_commands,&owner.commands);
            owner.object = nullptr;
        }
        logo_sprite = nullptr;
        sprite_camera = nullptr;
        return true;
    }
};
Runtime::Runtime(std::unique_ptr<Impl> value) noexcept : impl_(std::move(value)) {}
Runtime::~Runtime() = default;
CreateResult Runtime::create_boot(const menu::Catalog& catalog, game_input::Service& input,
    game_audio::Session& audio, ::isle::game::State& persistent,
    scene::Storage& storage, std::uint32_t now) noexcept {
    CreateResult result;
    if (!input.running() || !audio.initialization_ready() || persistent.initialization_attempted() ||
        persistent.profile_path().empty() || !persistent.profile_path().is_absolute() || storage.game() != nullptr) {
        result.error = Error::invalid; return result;
    }
    try {
        auto state = std::make_unique<Impl>(catalog,input,audio,persistent,storage,now);
        if (state->error == Error::none && state->start_logo())
            result.runtime.reset(new Runtime(std::move(state)));
        else { result.error = state->error; result.message = state->message; }
    } catch (const std::exception& e) {
        result.error = Error::allocation;
        const auto count = std::min(result.message.size()-1,std::char_traits<char>::length(e.what()));
        std::copy_n(e.what(),count,result.message.data());
    } catch (...) { result.error = Error::allocation; }
    return result;
}
CreateResult Runtime::create_title(const menu::Catalog& catalog, game_input::Service& input,
    game_audio::Session& audio, ::isle::game::State& persistent, scene::Storage& storage, std::uint32_t now) noexcept {
    CreateResult result;
    if (!input.running() || !audio.initialization_ready() || !persistent.initialized() || storage.game() != nullptr) {
        result.error = Error::invalid;
        return result;
    }
    try {
        auto state = std::make_unique<Impl>(catalog,input,audio,persistent,storage,now);
        if (state->error == Error::none && state->start_menu())
            result.runtime.reset(new Runtime(std::move(state)));
        else { result.error = state->error; result.message = state->message; }
    } catch (const std::exception& e) {
        result.error = Error::allocation;
        const auto count = std::min(result.message.size()-1,std::char_traits<char>::length(e.what()));
        std::copy_n(e.what(),count,result.message.data());
    } catch (...) { result.error = Error::allocation; }
    return result;
}
Step Runtime::tick(std::uint32_t raw_time) noexcept {
    auto& self = *impl_;
    if (self.phase == Phase::scene_exit) return Step::scene_exit;
    if (self.phase == Phase::boot_complete) return Step::boot_complete;
    if (self.phase == Phase::retired || self.error != Error::none) return Step::failed;
    self.raw_time = raw_time;
    if (self.services) self.services->raw_game_time = raw_time;
    const auto updated = self.services ? self.services->tick() : isle_game_tick(self.storage.game());
    if (!self.native(updated,"source startup OM traversal failed")) return Step::failed;
    ++self.updates;
    if (self.services && self.services->failed()) { self.fail(Error::native,self.services->error()); return Step::failed; }
    if (self.phase == Phase::logo && self.logo_exit) {
        self.phase = Phase::boot_complete;
        return Step::boot_complete;
    }
    if (self.ui && self.services->exit_requested()) {
        self.requested = {static_cast<menu::scene::SourceScene>(self.services->exit_result()),self.ui->entry_flags()};
        self.state.add_scene_notices(self.requested.entry_flags);
        self.phase = Phase::scene_exit;
        return Step::scene_exit;
    }
    return Step::updated;
}
Phase Runtime::phase() const noexcept { return impl_->phase; }
Error Runtime::error() const noexcept { return impl_->error; }
const char* Runtime::message() const noexcept { return impl_->message.data(); }
std::uint64_t Runtime::updates() const noexcept { return impl_->updates; }
std::uint32_t Runtime::logo_counter() const noexcept { return impl_->logo_ticks; }
int Runtime::controller() const noexcept { return impl_->state.controller; }
const menu::scene::Ui* Runtime::menu_ui() const noexcept { return impl_->ui.get(); }
View Runtime::view() noexcept {
    auto& self = *impl_;
    if (self.phase != Phase::logo && self.phase != Phase::title_menu) return {};
    save::ScreenOffsets offsets;
    if (!self.state.screen_offsets().translation(offsets)) {
        self.fail(Error::native,"startup live VI endpoints cannot be represented by native translation");
        return {};
    }
    return {self.phase == Phase::logo ? menu::Scene::logo : menu::Scene::title_menu,self.storage.game(),
        self.sprite_camera,self.logo_sprite,self.services.get(),self.title.get(),
        self.phase == Phase::logo ? 0u : 0x6464b4ffu,offsets};
}
Exit Runtime::exit() const noexcept { return impl_->requested; }
const ::isle::save::GameOpenResult* Runtime::save_initialization() const noexcept {
    return impl_->state.save_initialization();
}
::isle::save::GameSession* Runtime::save() noexcept { return impl_->state.save(); }
bool Runtime::retire() noexcept {
    if (impl_->phase == Phase::retired) return impl_->error == Error::none;
    const bool retired = impl_->release_scene();
    if (retired) impl_->phase = Phase::retired;
    return retired;
}
} // namespace isle::startup

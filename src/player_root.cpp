#include "player_root.hpp"
#include "actor_producer.hpp"
#include "actor_owner.hpp"
#include "game_state.hpp"
#include "item_owner.hpp"
#include "menu_scene_bridge.h"
#include "player_bump.hpp"
#include "player_camera_animation.hpp"
#include "player_photo.hpp"
#include "player_renderer.hpp"
#include "player_ui.hpp"
#include "scene_services.hpp"
#include <algorithm>
#include <exception>
#include <stdexcept>

namespace isle::player {
Root::Root(scene::Services& services, const UiResources& resources, Config config)
    : services_(services), config_(config) {
    if (!config.end_level || !config.pause || config.level_id < 0 || config.level_id > 63 ||
        services.failed() || !services.state.initialized())
        throw std::invalid_argument("Player Root requires initialized application and retained end/pause callbacks");
    auto result = isle_player_source_prepare(services.game, &source_, &services.rng, this,
        resources.minimum_photo_interval());
    if (result != ISLE_GAME_OK) throw std::invalid_argument("Player Root source preparation failed");
    source_.native_root = this;
    source_.EndLevelCb = config.end_level;
    source_.end_level_user = config.end_level_user;
    source_.PauseCb = config.pause;
    source_.pause_user = config.pause_user;
    source_.screen = services.screen_transition_source();
    result = isle_player_source_activate(&source_);
    if (result != ISLE_GAME_OK) throw std::invalid_argument("Player Root source activation failed");
}
Root::~Root() {
    if (!deactivated_) std::terminate();
}
IsleGameResult Root::ready() const noexcept {
    IslePlayerSource* canonical = nullptr;
    const auto result = isle_player_source_context(services_.game, &canonical);
    if (result != ISLE_GAME_OK) return result;
    return !deactivated_ && !services_.failed() && canonical == &source_ &&
        source_.native_root == this && source_.owner == this ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
}
IsleGameResult Root::bind(Domains domains) noexcept {
    const auto result = ready();
    if (result != ISLE_GAME_OK) return result;
    if (domains_ || source_.object || source_.model ||
        !source_.native_renderer || !source_.commands || source_.ui ||
        source_.bump != &domains.bump.source() || &domains.actors.player_source() != &source_ ||
        !domains.items.matches(services_, domains.world, domains.actors) ||
        domains.ui.source().player != &source_ || domains.photo.save != &services_.save ||
        !domains.photo.D_800AE4E4 || !domains.photo.D_800AE284 || domains.photo.owner ||
        domains.photo.status != ISLE_GAME_OK)
        return ISLE_GAME_INVALID_STATE;
    domains_.emplace(domains);
    source_.ui = &domains.ui.source();
    source_.photo = &domains.photo;
    return ISLE_GAME_OK;
}
IsleGameResult Root::camera_ready() const noexcept {
    const auto result = ready();
    if (result != ISLE_GAME_OK) return result;
    if (!source_.camera || !source_.native_renderer) return ISLE_GAME_INVALID_STATE;
    IsleGameObject* object = nullptr;
    const auto camera = isle_game_camera_object(services_.game, source_.camera, &object);
    if (camera != ISLE_GAME_OK) return camera;
    return object && services_.source_owner_registered(object) &&
        (!domains_ || !domains_->camera_animation ||
            domains_->camera_animation->matches(services_.game, source_.camera))
        ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
}
IsleGameResult Root::bind_camera_animation(camera_animation::NativeDriver& driver) noexcept {
    const auto result = camera_ready();
    if (result != ISLE_GAME_OK) return result;
    if (!domains_ || initialized_ || source_.object || source_.model ||
        isle_menu_current_object(services_.game) ||
        isle_game_require_idle(services_.game) != ISLE_GAME_OK ||
        !driver.matches(services_.game, source_.camera) ||
        (domains_->camera_animation && domains_->camera_animation != &driver))
        return ISLE_GAME_INVALID_STATE;
    domains_->camera_animation = &driver;
    return ISLE_GAME_OK;
}
IsleGameResult Root::initialize() noexcept {
    const auto result = camera_ready();
    if (result != ISLE_GAME_OK) return result;
    if (!domains_ || initialized_ || objects_[0].identity || source_.nonwaiting_call_frame ||
        isle_menu_current_object(services_.game)) return ISLE_GAME_INVALID_STATE;
    IsleGameObject* object = nullptr;
    const auto initialized = isle_player_root_invoke_init(&source_, &object);
    if (initialized != ISLE_GAME_OK) return initialized;
    return complete_initialization(object);
}
IsleGameResult Root::complete_initialization(IsleGameObject* object) noexcept {
    const auto result = camera_ready();
    if (result != ISLE_GAME_OK) return result;
    if (!domains_ || initialized_ || !object ||
        object != source_.object || object != objects_[0].identity || !source_.model || !movement_)
        return ISLE_GAME_INVALID_STATE;
    initialized_ = true;
    return ISLE_GAME_OK;
}
IsleGameResult Root::publish_input() noexcept {
    const auto result = ready();
    if (result != ISLE_GAME_OK) return result;
    if (!initialized_ || !source_.object || !services_.input.running()) return ISLE_GAME_INVALID_STATE;
    const auto& pad = services_.input.snapshot().primary();
    source_.input = {pad.buttons, pad.pressed, pad.held, pad.released,
        static_cast<float>(pad.stick_x), static_cast<float>(pad.stick_y)};
    return ISLE_GAME_OK;
}
void Root::joined(IsleGameObject* object, void* user) noexcept {
    auto& record = *static_cast<Object*>(user);
    auto& source = record.owner->source_;
    if (record.identity != object || record.joined) std::terminate();
    record.joined = true;
    if (source.object == object) { source.object = nullptr; source.model = nullptr; }
    if (source.D_80382C38_523048 == object) {
        source.D_80382C38_523048 = nullptr;
        std::fill(std::begin(source.MainCameraReticles), std::end(source.MainCameraReticles), nullptr);
        std::fill(std::begin(source.D_803AE458_54E868), std::end(source.D_803AE458_54E868), nullptr);
    }
    if (source.ObjectPauseMenu == object) {
        source.ObjectPauseMenu = nullptr;
        source.D_80382C70_523080 = nullptr;
        source.Pause_LabelPause2 = source.Pause_LabelPause = nullptr;
        source.Pause_QuitCourseSelected = source.Pause_QuitCourse = nullptr;
        source.Pause_ContinueSelected = source.Pause_Continue = nullptr;
        source.Pause_RetrySelected = source.Pause_Retry = nullptr;
    }
}
IsleGameResult Root::object(std::uint32_t id, IsleGameSourceCallback update,
    std::uint8_t link, std::uint32_t priority, IsleGameObject** output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = nullptr;
    if (initialized_ || !source_.nonwaiting_call_frame || isle_menu_current_object(services_.game))
        return ISLE_GAME_INVALID_STATE;
    const auto camera = camera_ready();
    if (camera != ISLE_GAME_OK) return camera;
    auto slot = std::find_if(objects_.begin(), objects_.end(), [](const Object& value) { return !value.identity; });
    if (slot == objects_.end()) return ISLE_GAME_EXHAUSTED;
    auto* object = services_.object(id, link, priority);
    if (!object) return ISLE_GAME_EXHAUSTED;
    *slot = {this, object, false, false};
    if (!services_.set_lifecycle(object, {nullptr, joined, &*slot})) {
        services_.erase(object);
        // Cold init has published no callback/process or semantic sender yet.
        // Services owns and synchronously joins this failed fresh allocation.
        *slot = {};
        return ISLE_GAME_INVALID_STATE;
    }
    const auto fields = id == 7 ? std::span<IsleGameObject** const>(fields_) : std::span<IsleGameObject** const>{};
    if (!services_.source_owner_register(object, fields)) {
        services_.erase(object);
        return ISLE_GAME_INVALID_STATE;
    }
    slot->registered = true;
    if (update) {
        const auto result = isle_game_object_set_source_update(services_.game, object, update);
        if (result != ISLE_GAME_OK || !services_.source_commands_native(object)) {
            services_.erase(object);
            return result != ISLE_GAME_OK ? result : ISLE_GAME_INVALID_STATE;
        }
    }
    *output = object;
    return ISLE_GAME_OK;
}
IsleGameResult Root::prepare_owners(IsleGameObject* object, IsleGameModel* model) noexcept {
    if (!domains_ || movement_ || !object || object != objects_[0].identity ||
        !objects_[0].registered || source_.object || source_.model || !source_.camera)
        return ISLE_GAME_INVALID_STATE;
    const auto camera = camera_ready();
    if (camera != ISLE_GAME_OK) return camera;
    const auto result = isle_game_model_validate_binding(services_.game, object, model);
    if (result != ISLE_GAME_OK) return result;
    movement_.emplace(domains_->world, source_.movement, config_.movement);
    domains_->world.follow_camera(source_.camera);
    return ISLE_GAME_OK;
}
IsleGameResult Root::retire_objects() noexcept {
    if (source_.nonwaiting_call_frame || isle_menu_current_object(services_.game)) return ISLE_GAME_INVALID_STATE;
    for (auto it = objects_.rbegin(); it != objects_.rend(); ++it)
        if (it->identity && !it->joined) services_.erase(it->identity);
    return std::any_of(objects_.begin(), objects_.end(),
        [](const Object& object) { return object.identity && !object.joined; })
        ? ISLE_GAME_INVALID_STATE : ISLE_GAME_OK;
}
IsleGameResult Root::collect_joined() noexcept {
    if (source_.nonwaiting_call_frame || isle_menu_current_object(services_.game) ||
        std::any_of(objects_.begin(), objects_.end(),
            [](const Object& object) { return object.identity && !object.joined; }))
        return ISLE_GAME_INVALID_STATE;
    for (auto& object : objects_) {
        if (!object.identity) continue;
        if (object.registered && (!services_.source_owner_collectable(object.identity) ||
            !services_.source_owner_release(object.identity))) return ISLE_GAME_INVALID_STATE;
        object = {};
    }
    return ISLE_GAME_OK;
}
IsleGameResult Root::deactivate() noexcept {
    if (deactivated_ || source_.nonwaiting_call_frame || isle_menu_current_object(services_.game) ||
        source_.native_renderer || source_.commands || source_.ui || source_.bump || source_.cart ||
        (source_.photo && source_.photo->owner) ||
        std::any_of(objects_.begin(), objects_.end(), [](const Object& object) { return object.identity != nullptr; }))
        return ISLE_GAME_INVALID_STATE;
    const auto result = isle_player_source_deactivate(&source_);
    // Existing Source release returns the preserved game fault after clearing
    // its context. This is not recovery or permission to continue executing C.
    if (result != ISLE_GAME_OK && result != isle_game_status(services_.game)) return result;
    movement_.reset();
    domains_.reset();
    source_.movement.block = nullptr;
    source_.photo = nullptr;
    source_.screen = nullptr;
    source_.native_root = nullptr;
    deactivated_ = true;
    return result;
}
struct RootAccess {
    static Root* owner(IslePlayerSource* source) noexcept {
        if (!source || !source->native_root) return nullptr;
        auto* root = static_cast<Root*>(source->native_root);
        return &root->source_ == source && source->owner == root ? root : nullptr;
    }
    template<class F> static IsleGameResult invoke(IslePlayerSource* source, F&& function) noexcept {
        auto* root = owner(source);
        if (!root) return ISLE_GAME_INVALID_ARGUMENT;
        const auto ready = root->ready();
        if (ready != ISLE_GAME_OK) return ready;
        try { return function(*root); }
        catch (const std::exception& error) { root->services_.fail(error.what()); }
        catch (...) { root->services_.fail("Player Root native operation failed"); }
        return ISLE_GAME_EXECUTION_FAILURE;
    }
    static scene::Services& services(Root& root) noexcept { return root.services_; }
    static Root::Domains& domains(Root& root) { return root.domains_.value(); }
    static const Root::Config& config(Root& root) noexcept { return root.config_; }
    static world::Movement& movement(Root& root) { return root.movement_.value(); }
    static IsleGameResult object(Root& root, std::uint32_t id, IsleGameSourceCallback update,
        std::uint8_t link, std::uint32_t priority, IsleGameObject** output) noexcept {
        return root.object(id, update, link, priority, output);
    }
    static IsleGameResult prepare(Root& root, IsleGameObject* object, IsleGameModel* model) noexcept {
        return root.prepare_owners(object, model);
    }
    static IsleGameResult audio(Root& root, audio::Error result) noexcept {
        return root.services_.check(result, "original Player audio command failed") ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;
    }
    template<class Status> static IsleGameResult accepted(Root& root, Status status) noexcept {
        if (status) return ISLE_GAME_OK;
        root.services_.fail(status.detail);
        return ISLE_GAME_EXECUTION_FAILURE;
    }
    static IsleGameResult camera(camera_animation::Status status) noexcept {
        return status ? ISLE_GAME_OK : status.native != ISLE_GAME_OK ? status.native : ISLE_GAME_EXECUTION_FAILURE;
    }
};
}

using Root = isle::player::Root;
using Access = isle::player::RootAccess;
extern "C" {
IsleGameResult isle_player_root_native_object(IslePlayerSource* source, uint32_t id,
    IsleGameSourceCallback update, uint8_t link, uint32_t priority, IsleGameObject** output) {
    if (output) *output = nullptr;
    return Access::invoke(source, [&](Root& root) { return Access::object(root, id, update, link, priority, output); });
}
IsleGameResult isle_player_root_native_prepare_owners(IslePlayerSource* source, IsleGameObject* object, IsleGameModel* model) {
    return Access::invoke(source, [&](Root& root) { return Access::prepare(root, object, model); });
}
IsleGameResult isle_player_root_native_focus_assign(IslePlayerSource* source, IsleGameObject* object) {
    return Access::invoke(source, [&](Root& root) {
        return Access::services(root).source_reference_assign(source->object, &source->gPreviousPokemonInFocus, object)
            ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_player_root_native_sprite_render(IslePlayerSource* source, IsleGameObject* object,
    uint8_t link, uint32_t priority, int32_t tag) {
    return Access::invoke(source, [&](Root& root) {
        return Access::services(root).set_render(object, {link, priority, static_cast<uint32_t>(tag)}) ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_player_root_native_sprite(IslePlayerSource* source, IsleGameObject* object,
    uint32_t identity, IsleGameSprite** output) {
    if (output) *output = nullptr;
    return Access::invoke(source, [&](Root& root) {
        auto& ui = Access::domains(root).ui;
        return ui.append(object, ui.resources().sprite(identity), output);
    });
}
IsleGameResult isle_player_root_native_movement_init(IslePlayerSource* source, int32_t block) {
    return Access::invoke(source, [&](Root& root) {
        if (block < 0) return ISLE_GAME_INVALID_ARGUMENT;
        const bool moved=Access::movement(root).initialize(static_cast<std::size_t>(block));
        const auto& world=Access::domains(root).world;
        return moved || (world.retired() && world.error().empty()) ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_player_root_native_movement_update(IslePlayerSource* source) {
    return Access::invoke(source, [&](Root& root) {
        const bool moved=Access::movement(root).update();
        const auto& world=Access::domains(root).world;
        return moved || (world.retired() && world.error().empty()) ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult isle_player_root_native_world_block_index(IslePlayerSource* source, const void* block, int32_t* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return Access::invoke(source, [&](Root& root) {
        for (const auto& candidate : Access::domains(root).world.blocks()) if (&candidate == block) {
            *output = static_cast<int32_t>(candidate.index);
            return ISLE_GAME_OK;
        }
        return ISLE_GAME_INVALID_ARGUMENT;
    });
}
IsleGameResult isle_player_root_native_idle_script(IslePlayerSource* source, const IslePlayerIdleScript** output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = nullptr;
    return Access::invoke(source, [&](Root& root) { *output = Access::services(root).state.idle_script(); return ISLE_GAME_OK; });
}
IsleGameResult isle_player_root_native_idle_interrupt_gate(IslePlayerSource* source, int32_t* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return Access::invoke(source, [&](Root& root) { *output = Access::services(root).controller; return ISLE_GAME_OK; });
}
IsleGameResult isle_player_root_native_set_flag(IslePlayerSource* source, int32_t flag, int32_t value) {
    return Access::invoke(source, [&](Root& root) {
        return !Access::services(root).save.set_player_flag(static_cast<unsigned>(flag), value != 0) ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;
    });
}
IsleGameResult isle_player_root_native_registered_species_count(IslePlayerSource* source, int32_t* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = 0;
    return Access::invoke(source, [&](Root& root) {
        if (!source->photo || !source->photo->D_800AE4E4) return ISLE_GAME_INVALID_STATE;
        const auto saved = Access::services(root).save.snapshot();
        if (!saved) return ISLE_GAME_EXECUTION_FAILURE;
        // Pinned5BF20.c258–269 enumerates species1..151 and the report mapping,
        // not unlocked courses or the tutorial-completed flag.
        int32_t count = 0;
        for (int species = 1; species <= 151; ++species) {
            const int index = source->photo->D_800AE4E4[species - 1];
            if (index < 0 || static_cast<std::size_t>(index) >= isle::save::game_report_count) continue;
            const auto present = saved.value.photo_present(isle::save::PhotoSet::report, static_cast<std::size_t>(index));
            if (!present) return ISLE_GAME_EXECUTION_FAILURE;
            if (present.value) ++count;
        }
        *output = count;
        return ISLE_GAME_OK;
    });
}
IsleGameResult isle_player_root_native_level_id(IslePlayerSource* source, int32_t* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return Access::invoke(source, [&](Root& root) { *output = Access::config(root).level_id; return ISLE_GAME_OK; });
}
IsleGameResult isle_player_root_native_postprocess(IslePlayerSource* source, int enabled) {
    return Access::invoke(source, [&](Root& root) { return Access::domains(root).renderer.set_postprocess(enabled != 0); });
}
IsleGameResult isle_player_root_native_drawn_frame_counter(IslePlayerSource* source, int32_t* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return Access::invoke(source, [&](Root& root) { *output = Access::domains(root).renderer.drawn_frame_counter(); return ISLE_GAME_OK; });
}
IsleGameResult isle_player_root_native_detector_enabled(IslePlayerSource* source, int enabled) {
    return Access::invoke(source, [&](Root&) {
        return enabled ? isle_player_photo_PokemonDetector_Enable(source->photo)
                       : isle_player_photo_PokemonDetector_Disable(source->photo);
    });
}
IsleGameResult isle_player_root_native_items_init(IslePlayerSource* source) {
    return Access::invoke(source, [&](Root& root) { return Access::accepted(root, Access::domains(root).items.init()); });
}
IsleGameResult isle_player_root_native_items_custom_functions(IslePlayerSource* source) {
    return Access::invoke(source, [&](Root& root) {
        const auto& config = Access::config(root);
        return Access::accepted(root, Access::domains(root).items.set_custom_functions(config.update_items, config.update_items_kind, config.collide_items));
    });
}
IsleGameResult isle_player_root_native_items_pause(IslePlayerSource* source, int paused) {
    return Access::invoke(source, [&](Root& root) {
        auto& items = Access::domains(root).items;
        return Access::accepted(root, paused ? items.pause() : items.resume());
    });
}
IsleGameResult isle_player_root_native_flute_state(IslePlayerSource* source, int32_t* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return Access::invoke(source, [&](Root& root) { *output = Access::domains(root).items.flute().state(); return ISLE_GAME_OK; });
}
IsleGameResult isle_player_root_native_flute_play(IslePlayerSource* source) {
    return Access::invoke(source, [&](Root& root) { return Access::audio(root, Access::domains(root).items.flute().play()); });
}
IsleGameResult isle_player_root_native_flute_stop(IslePlayerSource* source) {
    return Access::invoke(source, [&](Root& root) { return Access::audio(root, Access::domains(root).items.flute().stop()); });
}
IsleGameResult isle_player_root_native_spawn_item(IslePlayerSource* source, int32_t item,
    const IsleGameVec3* origin, const IsleGameVec3* velocity) {
    if (!origin || !velocity) return ISLE_GAME_INVALID_ARGUMENT;
    return Access::invoke(source, [&](Root& root) {
        auto& items = Access::domains(root).items;
        if (item == ISLE_ITEM_PESTER_BALL) return Access::accepted(root, items.spawn_ball(*origin, *velocity).status);
        if (item == ISLE_ITEM_APPLE) return Access::accepted(root, items.spawn_apple(*origin, *velocity).status);
        return ISLE_GAME_INVALID_ARGUMENT;
    });
}
IsleGameResult isle_player_root_native_skybox_animation_speed(IslePlayerSource* source, float speed) {
    return Access::invoke(source, [&](Root& root) { Access::domains(root).world.set_sky_animation_speed(speed); return ISLE_GAME_OK; });
}
IsleGameResult isle_player_root_native_camera_animation_set(IslePlayerSource* source,
    IsleGameCamera* camera, const IslePlayerCameraAnimationClip* clip, float start) {
    return Access::invoke(source, [&](Root& root) {
        auto* driver = Access::domains(root).camera_animation;
        if (camera != source->camera || !driver) return ISLE_GAME_INVALID_STATE;
        return Access::camera(driver->set(clip, start));
    });
}
IsleGameResult isle_player_root_native_camera_animation_update(IslePlayerSource* source, IsleGameObject* object) {
    return Access::invoke(source, [&](Root& root) {
        auto* driver = Access::domains(root).camera_animation;
        if (!driver || !source->camera) return ISLE_GAME_INVALID_STATE;
        IsleGameObject* actual = nullptr;
        auto result = isle_game_camera_object(source->game, source->camera, &actual);
        if (result != ISLE_GAME_OK) return result;
        if (object != actual) return ISLE_GAME_INVALID_ARGUMENT;
        IsleGameObjectMetadata metadata{};
        result = isle_game_object_metadata(source->game, object, &metadata);
        return result != ISLE_GAME_OK ? result : Access::camera(driver->update(metadata.flags));
    });
}
IsleGameResult isle_player_root_native_play_sound(IslePlayerSource* source, int32_t sound, int32_t* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = -1;
    return Access::invoke(source, [&](Root& root) {
        auto& services = Access::services(root);
        *output = services.play_sound(static_cast<uint32_t>(sound));
        return services.failed() ? ISLE_GAME_EXECUTION_FAILURE : ISLE_GAME_OK;
    });
}
IsleGameResult isle_player_root_native_stop_sound(IslePlayerSource* source, int32_t handle) {
    return Access::invoke(source, [&](Root& root) { return Access::audio(root, Access::services(root).audio.stop_sound(static_cast<unsigned>(handle))); });
}
IsleGameResult isle_player_root_native_play_song(IslePlayerSource* source, int32_t player, int32_t song) {
    return Access::invoke(source, [&](Root& root) {
        auto& audio = Access::services(root).audio;
        const auto id = static_cast<uint32_t>(song);
        // Own sys/audio.c1163–1173 returns -1 without changing requests for
        // absent songs; original Player ignores that ordinary source result.
        if (!audio.has_song(id)) return ISLE_GAME_OK;
        return Access::audio(root, audio.play_song(static_cast<unsigned>(player), id));
    });
}
IsleGameResult isle_player_root_native_stop_song(IslePlayerSource* source, int32_t player) {
    return Access::invoke(source, [&](Root& root) { return Access::audio(root, Access::services(root).audio.stop_song(static_cast<unsigned>(player))); });
}
IsleGameResult isle_player_root_native_stop_bgm(IslePlayerSource* source) {
    return Access::invoke(source, [&](Root& root) { return Access::audio(root, Access::services(root).audio.stop_bgm()); });
}
IsleGameResult isle_player_root_native_stop_all_sounds(IslePlayerSource* source) {
    return Access::invoke(source, [&](Root& root) { return Access::audio(root, Access::services(root).audio.stop_sounds()); });
}
IsleGameResult isle_player_root_native_bgm_volume(IslePlayerSource* source, int32_t player, int32_t volume) {
    return Access::invoke(source, [&](Root& root) { return Access::audio(root, Access::services(root).audio.bgm_volume(static_cast<unsigned>(player), static_cast<uint32_t>(volume))); });
}
IsleGameResult isle_player_root_native_bgm_volume_smooth(IslePlayerSource* source, int32_t player, int32_t volume, int32_t frames) {
    return Access::invoke(source, [&](Root& root) {
        return Access::audio(root, Access::services(root).audio.bgm_volume(static_cast<unsigned>(player), static_cast<uint32_t>(volume), static_cast<uint32_t>(frames)));
    });
}
IsleGameResult isle_player_root_native_bgm_channel_volume(IslePlayerSource* source, int32_t player, int32_t channel, int32_t volume) {
    return Access::invoke(source, [&](Root& root) {
        return Access::audio(root, Access::services(root).audio.bgm_channel_volume(static_cast<unsigned>(player), static_cast<uint8_t>(channel), static_cast<int8_t>(volume)));
    });
}
IsleGameResult isle_player_root_native_sound_global_volume(IslePlayerSource* source, int32_t volume) {
    return Access::invoke(source, [&](Root& root) { return Access::audio(root, Access::services(root).audio.global_sound_volume(static_cast<uint8_t>(volume))); });
}
IsleGameResult isle_player_root_native_current_sounds_global_volume(IslePlayerSource* source, int32_t volume) {
    return Access::invoke(source, [&](Root& root) { return Access::audio(root, Access::services(root).audio.global_sound_volume(static_cast<uint8_t>(volume), true)); });
}
}

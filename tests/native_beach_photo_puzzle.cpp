#include "native_beach_photo_puzzle.hpp"
#include "application_runtime.hpp"
#include "game_state.hpp"
#include "actor_owner_source.h"
#include "beach_eevee_chansey_source.h"
#include "player_source.h"
#include "player_ui_source.h"
#include "actor_owner.hpp"
#include "actor_course_context.hpp"
#include "beach_actor_scene.hpp"
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace isle::private_campaign {
namespace {
constexpr std::uint16_t apple_button = 0x8000, zoom_button = 0x2000;
constexpr std::uint32_t initial_graph = 0x802cd3f8, paused_graph = 0x802cd438;
constexpr std::uint64_t lapras_steering_budget = 360;
// A bounded physical search around the actual collision volume, not mesh
// bounds or predicted pixels. Only exact published Source focus authorizes A.
constexpr std::array<std::array<float, 2>, 9> lapras_aim_samples{{
    {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, -1.0f},
    {0.0f, -1.0f}, {-1.0f, -1.0f}, {-1.0f, 0.0f}, {-1.0f, 1.0f}
}};
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
struct Chansey {
    bool present = false, tangible = false, visible = false, focused = false;
    std::uint16_t flags = 0, focus_flags = 0;
    std::uint32_t graph = 0, state = 0, process_flags = 0;
    int behavior = 0, focus_id = 0;
    float distance = 0, radius = 0;
    IsleGameVec3 position{}, collision{}, scale{};
};
Chansey observe(const IslePlayerSource& player) {
    Chansey result;
    IslePlayerPhotoFocus focus{};
    require(isle_player_photo_published_focus(player.photo, &focus) == ISLE_GAME_OK,
            "Beach photo actual published focus unavailable");
    result.focus_id = focus.pokemon_id;
    result.focus_flags = focus.flags;
    IsleGameObject* previous = nullptr;
    for (;;) {
        IsleGameObject* object = nullptr;
        IsleActorSource* source = nullptr;
        IsleGameModel* model = nullptr;
        require(isle_actor_source_link3_next(player.game, previous, &object, &source, &model) == ISLE_GAME_OK,
                "Beach photo actual Actor traversal rejected");
        if (!object) break;
        previous = object;
        if (source->id != 113) continue;
        require(!result.present, "Beach photo duplicate actual Chansey");
        result.present = true;
        result.tangible = source->tangible != 0;
        result.flags = source->flags;
        result.graph = source->transitionGraph ? source->transitionGraph->source_identity : 0;
        result.process_flags = source->processFlags;
        result.behavior = source->behavior;
        result.distance = source->playerDist;
        result.radius = source->collisionRadius;
        result.collision = source->collPosition;
        result.focused = focus.present && focus.object == object && focus.pokemon_id == 113 &&
                         (focus.flags & 4u) && !(focus.flags & 8u);
        IsleGameSourceCallback function = nullptr;
        if (source->stateProc)
            require(isle_game_process_source_function(player.game, source->stateProc, &function) == ISLE_GAME_OK,
                    "Beach photo actual Chansey state unavailable");
        // Callback equality observes identity only; no Source handler is called.
        if (function == func_beach_802C92BC) result.state = 0x802c92bc;
        else if (function == func_beach_802C93B8) result.state = 0x802c93b8;
        else if (function == func_beach_802C943C) result.state = 0x802c943c;
        else if (function == func_beach_802C9580) result.state = 0x802c9580;
        else if (function == func_beach_802C95F0) result.state = 0x802c95f0;
        if (!model) continue;
        const IsleGameModelPlacement* placement = nullptr;
        require(isle_game_model_placement(player.game, model, &placement) == ISLE_GAME_OK && placement,
                "Beach photo actual Chansey placement unavailable");
        if (placement->mode == ISLE_GAME_MODEL_PLACEMENT_DISABLED) continue;
        result.position = placement->transform.position;
        result.scale = placement->transform.scale;
        IsleGameObjectMetadata metadata{};
        require(isle_game_object_metadata(player.game, object, &metadata) == ISLE_GAME_OK,
                "Beach photo actual Chansey visibility unavailable");
        result.visible = !(metadata.flags & 1u);
    }
    // All native Actor/model/Photo-focus borrows end here; only values escape.
    return result;
}
bool in_photo_pitch(const IslePlayerSource& player, const IsleGameVec3& target) {
    const float horizontal = std::hypot(target.x-player.CameraEyePos.x, target.z-player.CameraEyePos.z);
    const float pitch = std::atan2(target.y-player.CameraEyePos.y, horizontal);
    return pitch >= player.MinPitch && pitch <= player.MaxPitch;
}
} // namespace

BeachPhotoPuzzle::BeachPhotoPuzzle(private_post_course::Controller controller, Aim aim)
    : controller_(controller), aim_(aim) {
    require(controller_.set && aim_.point, "Beach photo requires actual pad and pure aim bridges");
}
void BeachPhotoPuzzle::publish(std::uint16_t buttons, std::int8_t x, std::int8_t y) noexcept {
    controller_.set(controller_.owner, buttons, x, y);
}
bool BeachPhotoPuzzle::update_lapras(const application::RuntimeView& view, std::uint64_t updates) {
    auto& f = facts_.lapras;
    f.owns_pad = false;
    f.target_focused = false;
    f.focus_live = false;
    f.focus_behavior = -1;
    f.focus_graph = f.focus_object_id = f.target_object_id = 0;
    const auto& player = *view.beach->source().player;
    const auto* actors = view.beach->actors();
    require(actors, "Lapras photo actual Actor owner unavailable");
    const auto& scene = actors->course_context().beach();
    if (!f.initialized) {
        f.initial_counters = scene.resources.initial_scalars().lapras_counters;
        require(f.initial_counters == std::array<std::int32_t, 3>{0, 0, 0},
                "Lapras photo initial Source counters differ from admitted data");
        f.initialized = true;
    }
    f.counters = scene.lapras_counters;
    IslePlayerPhotoFocus focus{};
    require(isle_player_photo_published_focus(player.photo, &focus) == ISLE_GAME_OK,
            "Lapras photo actual published focus unavailable");
    f.focus_present = focus.present;
    f.focus_species = focus.pokemon_id;
    f.focus_flags = focus.flags;
    const int film = player.photo->gPhotoCount, qualified = player.photo->D_800AE27C;
    require(film >= 0 && film <= 60 && qualified >= 0 && qualified <= film,
            "Lapras photo actual roll counters invalid");
    if (lapras_observed_role_ >= 0 && film > lapras_observed_slot_) {
        const int role = lapras_observed_role_;
        const auto& photo = player.photo->D_800B0598[lapras_observed_slot_];
        bool coherent = film == lapras_observed_slot_ + 1 &&
            qualified == lapras_observed_qualified_ + 1 && photo.levelID == 0 &&
            (photo.unk_00_16 & 0xe0u) == 0x20u && photo.pokemons[0].pokemonID == 131;
        auto expected = lapras_observed_counters_;
        if (role == 0) ++expected[0];
        if (role == 1) { ++expected[1]; ++expected[2]; }
        // Player publishes the exposure before CMD24's queued Actor state runs.
        // Keep the actual focus/roll witness until that bounded acknowledgment;
        // neither the photograph alone nor an eventual counter alone is credit.
        if (coherent && f.counters != expected && f.counters == lapras_observed_counters_ &&
            updates - lapras_request_update_ <= 120) {
            lapras_pending_ = true;
            f.owns_pad = facts_.owns_pad = true;
            publish(zoom_button);
            return true;
        }
        coherent &= f.counters == expected;
        if (coherent) {
            ++f.captures[role];
            if (role == 0) f.first_triggered = true;
            if (role == 1) f.second_triggered = true;
            if (role == 2) { f.late_captured = true; f.late_slot = lapras_observed_slot_; }
        } else {
            ++f.mismatches;
        }
        lapras_pending_ = false;
        lapras_release_ = true;
        lapras_observed_role_ = -1;
    }
    f.complete = f.first_triggered && f.second_triggered && f.late_captured &&
                 f.mismatches == 0;
    if (lapras_pending_ && updates - lapras_request_update_ > 120) {
        lapras_pending_ = false;
        lapras_release_ = true;
        lapras_observed_role_ = -1;
    }
    if (lapras_release_) {
        lapras_release_ = false;
        if (!pending_throw_ && !pending_shutter_ && !release_pending_) {
            f.owns_pad = facts_.owns_pad = true;
            publish();
            return true;
        }
    }
    if (lapras_pending_) {
        f.owns_pad = facts_.owns_pad = true;
        publish(zoom_button);
        return true;
    }
    // Chansey's outstanding physical press/release owns the shared pad first.
    if (pending_throw_ || pending_shutter_ || release_pending_) return false;
    lapras_observed_role_ = -1;
    f.target_behavior = -1;
    if (f.complete || f.mismatches || film >= 59) return false;
    const int role = !f.first_triggered ? 0 : (!f.second_triggered ? 1 : 2);
    bool found = false, focused = false;
    IsleGameVec3 target{};
    float target_radius = 0;
    IsleGameObject* previous = nullptr;
    for (;;) {
        IsleGameObject* object = nullptr;
        IsleActorSource* actor = nullptr;
        IsleGameModel* model = nullptr;
        require(isle_actor_source_link3_next(player.game, previous, &object, &actor, &model) == ISLE_GAME_OK,
                "Lapras photo actual Actor traversal rejected");
        if (!object) break;
        previous = object;
        if (focus.object == object && actor->id == focus.pokemon_id) {
            IsleGameObjectMetadata focus_metadata{};
            require(isle_game_object_metadata(player.game, object, &focus_metadata) == ISLE_GAME_OK,
                    "Lapras photo actual focused-object membership unavailable");
            f.focus_live = true;
            f.focus_behavior = actor->behavior;
            f.focus_graph = actor->transitionGraph ? actor->transitionGraph->source_identity : 0;
            f.focus_object_id = focus_metadata.id;
        }
        if (actor->id != 131 || !model || !actor->tangible ||
            !(actor->flags & 4u) || (actor->flags & 8u)) continue;
        const bool behavior = role == 0 ? actor->behavior == 0 :
            (role == 1 ? actor->behavior == 2 : actor->behavior == 4 || actor->behavior == 5);
        if (!behavior || !actor->transitionGraph ||
            actor->transitionGraph->source_identity != 0x802cce1c) continue;
        IsleGameObjectMetadata metadata{};
        require(isle_game_object_metadata(player.game, object, &metadata) == ISLE_GAME_OK,
                "Lapras photo actual visibility unavailable");
        const IsleGameModelPlacement* placement = nullptr;
        require(isle_game_model_placement(player.game, model, &placement) == ISLE_GAME_OK && placement,
                "Lapras photo actual placement unavailable");
        if ((metadata.flags & 1u) || placement->mode == ISLE_GAME_MODEL_PLACEMENT_DISABLED ||
            !(actor->playerDist >= 0 && actor->playerDist <= 10000) ||
            !in_photo_pitch(player, actor->collPosition)) continue;
        const bool exact_focus = focus.present && focus.object == object &&
            focus.pokemon_id == 131 && (focus.flags & 4u) && !(focus.flags & 8u);
        if (found && !exact_focus) continue;
        found = true;
        focused = exact_focus;
        target = actor->collPosition;
        target_radius = actor->collisionRadius;
        f.target_behavior = actor->behavior;
        f.target_object_id = metadata.id;
        if (focused) break;
    }
    // Only scalar identities/geometry escape the borrowed native traversal.
    f.target_focused = focused;
    if (!found) return false;
    if (focused) {
        lapras_observed_role_ = role;
        lapras_observed_slot_ = film;
        lapras_observed_qualified_ = qualified;
        lapras_observed_counters_ = f.counters;
        lapras_request_update_ = updates;
    }
    // Probe admits control only after the real Beach follow-up/stick witnesses.
    // IsTutorialEnabled is a profile choice, not an active tutorial-process flag.
    if (player.D_80382D04_523114 != 1 ||
        player.IsInputDisabled || player.IsPaused ||
        f.shutter_requests[role] >= 4 || f.control_updates[role] >= lapras_steering_budget) return false;
    require(std::isfinite(target_radius) && target_radius >= 0,
            "Lapras actual collision-volume aim radius invalid");
    f.aim_radius = target_radius;
    f.aim_sample = static_cast<unsigned>(f.control_updates[role] *
        lapras_aim_samples.size() / lapras_steering_budget);
    const auto& sample = lapras_aim_samples[f.aim_sample];
    const float dx = target.x - player.CameraEyePos.x, dz = target.z - player.CameraEyePos.z;
    const float horizontal = std::hypot(dx, dz);
    if (horizontal > 0) {
        const float right = target_radius * sample[0] / horizontal;
        target.x += dz * right;
        target.z -= dx * right;
    }
    target.y += target_radius * sample[1];
    f.aim_hint = target;
    ++f.control_updates[role];
    std::int8_t x = 0, y = 0;
    const bool settled = player.gDirectionIndex == -1 && !player.TargetDirectionZoomedIn &&
                         player.TurnToDirSpeed == 0;
    if (settled) aim_.point(aim_.owner, player, target, true, x, y);
    std::uint16_t buttons = zoom_button;
    // Actual exact-object Source focus authorizes A while the moving cart is
    // still being tracked; a perfectly zero analog chase error is not required.
    if (settled && focused && player.sTimerAfterPhotoTaken < 0 &&
        !(player.input.current_buttons & apple_button)) {
        buttons |= apple_button;
        ++f.shutter_requests[role];
        ++facts_.shutter_requests;
        lapras_pending_ = true;
        lapras_request_update_ = updates;
    }
    f.owns_pad = facts_.owns_pad = true;
    publish(buttons, x, y);
    return true;
}
bool BeachPhotoPuzzle::update(const application::RuntimeView& view, const game::State& state,
                              std::uint64_t updates) {
    if (sampled_ && updates == last_update_) return facts_.owns_pad;
    require(!sampled_ || updates > last_update_, "Beach photo update ordinal regressed");
    sampled_ = true;
    last_update_ = updates;
    facts_.owns_pad = false;
    facts_.dependency = BeachPhotoDependency::course_not_active;
    if (!view.beach || view.beach->status() != beach::LifecycleStatus::running) return false;
    const auto& source = view.beach->source();
    const auto* player = source.player;
    if (!player || source.D_beach_802CC018) return false;
    require(state.save() && player->photo && player->photo->status == ISLE_GAME_OK,
            "Beach photo requires actual canonical save and Photo owner");
    const auto apple = state.save()->player_flag(0);
    require(bool(apple), "Beach photo canonical Apple reward unavailable");
    facts_.apple_available = apple.value;
    // This is the no-item bridge. Real Apple access retains the already
    // qualified Chansey/ordinary schedule instead of competing for its pad.
    if (!apple.value && update_lapras(view, updates)) return true;
    if (!apple.value) { facts_.dependency = BeachPhotoDependency::apple_reward; return false; }
    if (!facts_.prior_report_checked) {
        // Same admitted Source-ID table and canonical wire-presence predicate
        // as application::Photos::report_present; no snapshot/image copy.
        facts_.report_index = isle_player_photo_func_8009BB4C(player->photo, 113);
        require(facts_.report_index >= 0 && facts_.report_index < int(save::game_report_count),
                "Beach photo canonical Chansey Report mapping unavailable");
        const auto wire = state.save()->photo(save::PhotoSet::report, std::size_t(facts_.report_index));
        require(bool(wire) && wire.value.size() == save::game_photo_bytes,
                "Beach photo canonical Chansey Report unavailable");
        facts_.prior_report = wire.value[4] != std::byte{0xff} || wire.value[5] != std::byte{0xff} ||
                              wire.value[6] != std::byte{0xff} || wire.value[7] != std::byte{0xff};
        facts_.prior_report_checked = true;
    }
    if (facts_.prior_report) { facts_.dependency = BeachPhotoDependency::prior_report; return false; }
    if (facts_.complete) { facts_.dependency = BeachPhotoDependency::none; return false; }
    facts_.film = player->photo->gPhotoCount;
    facts_.qualified = player->photo->D_800AE27C;
    require(facts_.film >= 0 && facts_.film <= 60 && facts_.qualified >= 0 && facts_.qualified <= facts_.film,
            "Beach photo actual roll counters invalid");
    facts_.player_ready = player->D_80382D04_523114 == 1 && !player->IsInputDisabled && !player->IsPaused;
    if (pending_throw_ && facts_.player_ready && (player->input.current_buttons & apple_button)) {
        const bool pressed = (player->input.pressed_buttons & apple_button) != 0;
        if (pressed) ++facts_.throws_pressed;
        if (pressed && player->ThrowItemTimeout > 0 && player->LastItemId == 163) {
            ++facts_.throws_consumed;
            facts_.throw_consumed_update = updates;
        }
        pending_throw_ = false;
        release_pending_ = true;
    }
    if (pending_shutter_) {
        if (!shutter_press_seen_ && (player->input.pressed_buttons & apple_button)) {
            shutter_press_seen_ = true;
            ++facts_.shutters_pressed;
        }
        if (facts_.film > pending_slot_) {
            const auto& photo = player->photo->D_800B0598[pending_slot_];
            require(photo.levelID == 0, "Beach photo consumed exposure belongs to another course");
            ++facts_.captures_consumed;
            facts_.capture_observed = true;
            facts_.capture_slot = pending_slot_;
            facts_.capture_update = updates;
            facts_.captured_subject_zero = photo.pokemons[0].pokemonID;
            bool chansey = false;
            for (const auto& subject : photo.pokemons) chansey |= subject.pokemonID == 113;
            facts_.captured_chansey |= chansey;
            facts_.qualified_capture |= facts_.qualified > pending_qualified_;
            facts_.complete = chansey && facts_.qualified > pending_qualified_;
            pending_shutter_ = false;
            pending_slot_ = -1;
            release_pending_ = true;
        } else if (shutter_press_seen_ && !(player->input.current_buttons & apple_button) &&
                   player->sTimerAfterPhotoTaken < 0) {
            // Actual input/cooldown settled without a roll advance: no capture
            // credit. A subsequent attempt still needs a new physical press.
            pending_shutter_ = false;
            pending_slot_ = -1;
        }
    }
    if (release_pending_ && !(player->input.current_buttons & apple_button)) {
        if (facts_.throw_release_update < facts_.throw_consumed_update)
            facts_.throw_release_update = updates;
        release_pending_ = false;
    }
    if (facts_.complete) { facts_.dependency = BeachPhotoDependency::none; return false; }
    if (!facts_.player_ready) { facts_.dependency = BeachPhotoDependency::player_not_ready; return false; }
    if (facts_.film >= 59) { facts_.dependency = BeachPhotoDependency::film_reserved; return false; }

    const auto target = observe(*player);
    facts_.target_present = target.present;
    facts_.target_tangible = target.tangible;
    facts_.target_visible = target.visible;
    facts_.target_flags = target.flags;
    facts_.target_graph = target.graph;
    facts_.target_state = target.state;
    facts_.target_process_flags = target.process_flags;
    facts_.target_behavior = target.behavior;
    facts_.target_distance = target.distance;
    facts_.collision_radius = target.radius;
    facts_.target_position = target.position;
    facts_.target_collision = target.collision;
    facts_.target_scale = target.scale;
    facts_.focus_id = target.focus_id;
    facts_.focus_flags = target.focus_flags;
    facts_.published_focus = target.focused;
    const bool initial = target.present && (target.flags & 8u) &&
                         (target.graph == initial_graph || target.graph == paused_graph);
    if (initial && !facts_.initial_flag8_seen) {
        facts_.initial_flag8_seen = true;
        facts_.initial_update = updates;
    }
    if (target.present && target.state == 0x802c943c && !facts_.reveal_state_seen) {
        facts_.reveal_state_seen = true;
        facts_.reveal_update = updates;
    }
    facts_.graph_changed_after_consumption |= target.present && facts_.throws_consumed &&
        target.graph != initial_graph && target.graph != paused_graph;
    if (facts_.graph_changed_after_consumption && !facts_.graph_change_update)
        facts_.graph_change_update = updates;
    if (target.present && facts_.initial_flag8_seen && !(target.flags & 8u) && !facts_.flag8_cleared) {
        facts_.flag8_cleared = true;
        facts_.flag8_clear_update = updates;
    }
    facts_.photo_eligible = target.present && target.tangible && target.visible &&
        (target.flags & 4u) && !(target.flags & 8u) && target.distance >= 0 && target.distance <= 10000 &&
        in_photo_pitch(*player, target.collision);
    if (pending_throw_ || release_pending_ || pending_shutter_) {
        facts_.dependency = BeachPhotoDependency::none;
        facts_.owns_pad = true;
        publish(pending_throw_ ? apple_button : (pending_shutter_ ? zoom_button : 0));
        return true;
    }
    if (!target.present) { facts_.dependency = BeachPhotoDependency::target_not_present; return false; }
    if (!target.tangible || !target.visible) {
        facts_.dependency = BeachPhotoDependency::target_not_ready; return false;
    }
    if (!(target.distance >= 0 && target.distance <= 10000)) {
        facts_.dependency = BeachPhotoDependency::target_out_of_range; return false;
    }
    if (facts_.photo_eligible) {
        std::int8_t x = 0, y = 0;
        const bool settled = player->gDirectionIndex == -1 && !player->TargetDirectionZoomedIn &&
                             player->TurnToDirSpeed == 0;
        facts_.zoom_settled = settled;
        if (settled) aim_.point(aim_.owner, *player, target.collision, true, x, y);
        std::uint16_t buttons = zoom_button;
        if (settled && !x && !y && target.focused && player->sTimerAfterPhotoTaken < 0 &&
            !(player->input.current_buttons & apple_button)) {
            buttons |= apple_button;
            pending_shutter_ = true;
            shutter_press_seen_ = false;
            pending_slot_ = facts_.film;
            pending_qualified_ = facts_.qualified;
            ++facts_.shutter_requests;
            facts_.shutter_request_update = updates;
        }
        facts_.input_target = target.collision;
        facts_.dependency = BeachPhotoDependency::none;
        facts_.owns_pad = true;
        publish(buttons, x, y);
        return true;
    }
    // Immutable Source df8815e, beach/5612B0.c21-73: half-scale FLAG8
    // Chansey installs these two graphs. Exact admitted US ROM SHA256
    // a1d5d816db7f8557ee04c35a011326d058b2c1fbca76b57b352b1d705a1ec1cc:
    // 802cd3f8/ROM565468 and802cd438/ROM5654a8 both map Apple CMD13
    // (also Pester9) to802c943c. That real state clears FLAG8 only after its
    // first animation. Eevee133's Apple rows pause/CMD33 instead; never aim
    // at Eevee or equate an Apple request/consumption with a physical hit.
    if (!initial) {
        facts_.dependency = target.state == 0x802c943c
            ? BeachPhotoDependency::reveal_in_progress : BeachPhotoDependency::target_not_ready;
        return false;
    }
    if (!player->ui || player->ui->Icons_NumItemsAvailable < 1) {
        facts_.dependency = BeachPhotoDependency::item_ui_not_ready; return false;
    }
    IsleGameVec3 aimed{};
    if (!projectile_target(*player, target.collision, aimed)) {
        facts_.dependency = BeachPhotoDependency::projectile_out_of_reach; return false;
    }
    facts_.dependency = BeachPhotoDependency::none;
    facts_.owns_pad = true;
    facts_.input_target = target.collision;
    // Real Player must finish unzoom/turn before A can dispatch an Apple.
    if (player->gDirectionIndex < 0 || player->TurnToDirSpeed != 0 || player->TargetDirectionZoomedIn) {
        publish();
        return true;
    }
    std::int8_t x = 0, y = 0;
    aim_.point(aim_.owner, *player, aimed, false, x, y);
    std::uint16_t buttons = 0;
    if (!x && !y && player->ThrowItemTimeout == 0 && !(player->input.current_buttons & apple_button)) {
        buttons = apple_button;
        pending_throw_ = true;
        ++facts_.throw_requests;
        facts_.throw_request_update = updates;
    }
    publish(buttons, x, y);
    return true;
}
} // namespace isle::private_campaign

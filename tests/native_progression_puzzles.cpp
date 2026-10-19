#include "native_progression_puzzles.hpp"
#include "application_runtime.hpp"
#include "game_state.hpp"
#include "tunnel_scene.hpp"
#include "river_scene.hpp"
#include "tunnel_actor_source.h"
#include "river_actor_source.h"
#include "actor_owner_source.h"
#include "player_source.h"
#include "player_ui_source.h"
#include "world_runtime.hpp"
#include <cmath>
#include <stdexcept>

namespace isle::private_campaign {
namespace {
// Immutable authority: df8815e75f3d79618466e01e0827c6165b93d11c.
// BehaviorResources' course-local mappings/counts (actor_behavior_resources.cpp
// 109-130,180-199,298-314) decode the admitted US ROM, SHA1
// edc7c49cc568c045fe48be0d18011c30f393cbaf:
// Tunnel802f0228 / ROM5ed2f8: cmd54 ->802edb3c, then cmd58.
// River802e4b14 / ROM6cc8f4: cmd43 ->802e1d10, then cmd58.
// These are evidence identities, never callbacks invoked by this driver.
//
// Tunnel electrode/code.c95-100,256-284,292-353: behavior5 installs802ee900
// until progress block5,.35; real Apple hit cmd13 (also Pester9/splash10)
// selects RevealHiddenPath, which waits for actual170/220 animation frames,
// removes other Actors and sends54. Endpoint pkmn_1014/code.c138-208 writes
// progress2/notice400 when Original getter<2, then runs its real outro.
// River6C4EC0.c79-168,587-675: hidden behavior2 installs802e39b8; real
// terrain Pester29 within1000 selects398, block4 selects524, actual30/1
// waits and path/animation completions select870 ->cmd43. The legal ROM row
// 802e39b8 is29 ->802dd1d4 (and9 ->same), followed by58. River6C9680.c139-202
// then writes progress4/notice400 when Original getter<4 and runs the outro.
// Player380-413 gates Apple A/Pester B on actual item UI and45-update cooldown;
// immutable enums.h32-35 and57-60 define canonical rewards0/1 and live bits1/2.
constexpr std::uint16_t apple_button = 0x8000, pester_button = 0x4000;
constexpr std::uint32_t tunnel_guard_graph = 0x802ee900;
constexpr std::uint32_t river_hidden_graph = 0x802e39b8;
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
struct Actor {
    bool present = false, tangible = false, trigger = false, endpoint = false;
    std::uint32_t graph = 0;
    int behavior = 0;
    IsleGameVec3 position{}, collision{};
};
struct Observations { Actor target, endpoint; };
Observations observe(IsleGame* game, Puzzle puzzle) {
    Observations result;
    IsleGameObject* previous = nullptr;
    for (;;) {
        IsleGameObject* object = nullptr;
        IsleActorSource* source = nullptr;
        IsleGameModel* model = nullptr;
        require(isle_actor_source_link3_next(game, previous, &object, &source, &model) == ISLE_GAME_OK,
                "progression puzzle Actor traversal rejected");
        if (!object) break;
        previous = object;
        const bool target = puzzle == Puzzle::tunnel1014
            ? source->id == 101 && source->behavior == 5
            : source->id == 137 && source->behavior == 2;
        const bool endpoint = source->id == (puzzle == Puzzle::tunnel1014 ? 1014 : 1019);
        if (!target && !endpoint) continue;
        auto& copy = target ? result.target : result.endpoint;
        require(!copy.present, "duplicate actual progression puzzle Actor");
        IsleGameSourceCallback function = nullptr;
        if (source->stateProc)
            require(isle_game_process_source_function(game, source->stateProc, &function) == ISLE_GAME_OK,
                    "progression puzzle state observation rejected");
        copy.present = true;
        copy.behavior = source->behavior;
        if(target) {
            const IsleGameModelPlacement* placement = nullptr;
            require(model && isle_game_model_placement(game, model, &placement) == ISLE_GAME_OK && placement &&
                    placement->mode != ISLE_GAME_MODEL_PLACEMENT_DISABLED,
                    "progression puzzle actual target model observation rejected");
            copy.tangible = source->tangible != 0;
            copy.position = placement->transform.position;
            copy.collision = source->collPosition;
        }
        copy.graph = source->transitionGraph ? source->transitionGraph->source_identity : 0;
        copy.trigger = function == (puzzle == Puzzle::tunnel1014
            ? electrode_RevealHiddenPath : func_802DE524_6C6304);
        copy.endpoint = function == (puzzle == Puzzle::tunnel1014
            ? func_802EDB3C_5EAC0C : func_802E1D10_6C9AF0);
    }
    // Every native handle/Source/placement borrow ends here. Only scalars escape.
    return result;
}
float distance(const IsleGameVec3& a, const IsleGameVec3& b) {
    return std::hypot(a.x-b.x, a.y-b.y, a.z-b.z);
}
float distance_squared(const IsleGameVec3& a, const IsleGameVec3& b) {
    const float x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;
    return x*x+y*y+z*z;
}
// River6C05E0.c235-260: ONLY a Pester ground contact on FF7FB2 sends
// cmd29 WITH its real Item sender. 6C4EC0.c109-129 requires distance<1000;
// c147-168 additionally requires behavior2 and current block4. Direct hit cmd9
// has a NULL sender (items.c1068), whose distance is FLOAT_MAX (pokemon.c452).
// Choose a real queried interior of that surface, never the hidden Actor itself.
bool river_surface(const world::World& world, IsleGameVec3 origin, IsleGameVec3& target) {
    const auto valid = [&](float x, float z, world::GroundResult& ground) {
        return world.ground_at(x, z, ground) && ground.surface == 0xff7fb2;
    };
    // The legal ROM's behavior2 spawn is block4 (17.3718,1.03125,13.1906).
    // Its FF7FB2 patch is within 50 native units of the actual placement.
    // The finite search is a terrain observation, NOT a success/transition clock;
    // every accepted point and its four 25-unit margins must actually query as
    // FF7FB2, and the real sender distance must remain strictly below1000.
    for (int ring = 0; ring < 40; ++ring) {
        for (int x = -ring; x <= ring; ++x) for (int z = -ring; z <= ring; ++z) {
            if (std::abs(x) != ring && std::abs(z) != ring) continue;
            const float px = origin.x + 25.0f*static_cast<float>(x), pz = origin.z + 25.0f*static_cast<float>(z);
            world::GroundResult ground{}, margin{};
            if (!valid(px, pz, ground)) continue;
            const IsleGameVec3 point{px, ground.height, pz};
            if (distance_squared(point, origin) >= 1000000 ||
                !valid(px-25, pz, margin) || !valid(px+25, pz, margin) ||
                !valid(px, pz-25, margin) || !valid(px, pz+25, margin)) continue;
            target = point;
            return true;
        }
    }
    return false;
}
} // namespace
// Original items.c231-259,855-1094: launch speed50 plus PlayerVelocity;
// each update integrates half velocity after subtracting .6 vertical velocity.
// Find the low-arc launch for the actual stationary target. This is input aiming
// only: obstruction, quantization and impact remain real Items/World decisions.
bool projectile_target(const IslePlayerSource& player, const IsleGameVec3& target, IsleGameVec3& aimed) {
    const IsleGameCameraParameters* camera = nullptr;
    require(isle_game_camera_parameters(player.game, player.camera, &camera) == ISLE_GAME_OK && camera,
            "progression puzzle launch camera observation rejected");
    const IsleGameVec3 eye{camera->look_at.eye[0], camera->look_at.eye[1], camera->look_at.eye[2]};
    const IsleGameVec3 delta{target.x-eye.x, target.y-eye.y, target.z-eye.z};
    const auto velocity = [&](float time) {
        return IsleGameVec3{2*delta.x/time-player.PlayerVelocity.x,
            2*delta.y/time-player.PlayerVelocity.y+.3f*(time+1),
            2*delta.z/time-player.PlayerVelocity.z};
    };
    const auto speed_squared = [&](float time) {
        const auto v = velocity(time);
        return v.x*v.x+v.y*v.y+v.z*v.z;
    };
    float low = .01f, high = 1;
    const float last = (50+std::abs(player.PlayerVelocity.y)+.3f)/.3f+1;
    // This enumerates candidate ballistic times, not elapsed game updates.
    while (high <= last && speed_squared(high) > 2500) { low = high; high += 1; }
    if (high > last) return false;
    for (unsigned iteration = 0; iteration < 16; ++iteration) {
        const float middle = (low+high)*.5f;
        if (speed_squared(middle) > 2500) low = middle;
        else high = middle;
    }
    const auto v = velocity(high);
    const float norm = std::hypot(v.x, v.y, v.z);
    const float ray = distance(player.cam_target, eye);
    if (!(norm > 0 && ray > 0)) return false;
    const float ux = v.x/norm, uy = v.y/norm, uz = v.z/norm;
    // Invert Source's +100 vertical launch bias using the actual reticle ray
    // length, rather than aiming a straight camera ray at the collision center.
    const float discriminant = ray*ray-10000*(1-uy*uy);
    if (discriminant < 0) return false;
    const float scale = 100*uy+std::sqrt(discriminant);
    if (!(scale > 0)) return false;
    aimed = {eye.x+scale*ux, eye.y+scale*uy-100, eye.z+scale*uz};
    return true;
}

ProgressionPuzzles::ProgressionPuzzles(Puzzle puzzle, private_post_course::Controller controller, Aim aim)
    : puzzle_(puzzle), controller_(controller), aim_(aim) {
    require(controller_.set && aim_.point, "progression puzzle requires actual pad and aim bridges");
}
void ProgressionPuzzles::publish(std::uint16_t buttons, std::int8_t x, std::int8_t y) noexcept {
    controller_.set(controller_.owner, buttons, x, y);
}
bool ProgressionPuzzles::update(const application::RuntimeView& view, const game::State& state,
                               Progress progress, std::uint64_t updates) {
    if (sampled_ && updates == last_update_) return facts_.owns_pad;
    require(!sampled_ || updates > last_update_, "progression puzzle update ordinal regressed");
    sampled_ = true;
    last_update_ = updates;
    facts_.owns_pad = false;
    facts_.dependency = PuzzleDependency::course_not_active;
    const IslePlayerSource* player = nullptr;
    const course::NativeHost* host = nullptr;
    bool endpoint_camera = false, endpoint_animation = false, endpoint_requested = false;
    if (puzzle_ == Puzzle::tunnel1014 && view.tunnel) {
        const auto& entry = *view.tunnel;
        if (entry.status() == course::LifecycleStatus::running || entry.status() == course::LifecycleStatus::retiring) {
            player = entry.adapter().native.player(); host = entry.native_host();
            const auto& source = entry.actors_source();
            endpoint_camera = source.D_802F0214_5ED2E4 != nullptr;
            endpoint_animation = source.D_802F0218_5ED2E8 == 1;
            endpoint_requested = entry.source().tunnel_EndLevelReason != 0;
        }
    } else if (puzzle_ == Puzzle::river1019 && view.river) {
        const auto& entry = *view.river;
        if (entry.status() == course::LifecycleStatus::running || entry.status() == course::LifecycleStatus::retiring) {
            player = entry.adapter().native.player(); host = entry.native_host();
            const auto& source = entry.actors_source();
            endpoint_camera = source.D_802E4B04_6CC8E4 != nullptr;
            endpoint_animation = source.D_802E4B08_6CC8E8 == 1;
            endpoint_requested = source.D_802E28B4_6CA694 != 0;
        }
    }
    // The last Original command can close execution in this same completed
    // update. Its copied course reason still proves a real endpoint request.
    facts_.endpoint_animation_complete |= facts_.endpoint_entered && endpoint_animation;
    facts_.endpoint_requested |= facts_.endpoint_entered && endpoint_requested;
    if (facts_.complete) {
        // The actual endpoint/progress/notice transaction is complete. Its
        // native tail may retire stateProc handles; no further Actor scan is
        // needed to keep neutral input through the real scene retirement.
        facts_.dependency = PuzzleDependency::none;
        facts_.owns_pad = true;
        publish();
        return true;
    }
    if (!player || !host || !host->execution_open()) return false;
    const int goal = puzzle_ == Puzzle::tunnel1014 ? 2 : 4;
    require(progress.known_course >= 0 && progress.unlocked_course >= 0 && state.save(),
            "progression puzzle requires actual canonical read-only progress");
    if (!baseline_) {
        baseline_ = true; initial_unlocked_ = progress.unlocked_course;
    }
    const auto observed = observe(player->game, puzzle_);
    const auto& target = observed.target;
    facts_.target_present = target.present;
    facts_.target_position = target.position;
    facts_.target_graph = target.graph;
    facts_.target_behavior = target.behavior;
    facts_.target_ready = target.present && target.graph ==
        (puzzle_ == Puzzle::tunnel1014 ? tunnel_guard_graph : river_hidden_graph);
    facts_.endpoint_ready = observed.endpoint.present && observed.endpoint.graph ==
        (puzzle_ == Puzzle::tunnel1014 ? 0x802f0228u : 0x802e4b14u);
    facts_.trigger_transition |= target.trigger;
    facts_.endpoint_entered |= observed.endpoint.endpoint || endpoint_camera;
    facts_.endpoint_animation_complete |= facts_.endpoint_entered && endpoint_animation;
    facts_.endpoint_requested |= facts_.endpoint_entered && endpoint_requested;
    // Endpoint setters occur at cutscene entry, before the Original10800 bound.
    // Observe their actual effects; elapsed time can never complete this helper.
    facts_.progress_observed |= initial_unlocked_ < goal && progress.unlocked_course == goal;
    facts_.notice_observed |= facts_.endpoint_entered && (state.scene_notices() & 0x400u);
    facts_.complete = facts_.endpoint_entered && facts_.progress_observed && facts_.notice_observed;
    const auto button = puzzle_ == Puzzle::tunnel1014 ? apple_button : pester_button;
    if (pending_throw_ && (player->input.current_buttons & button)) {
        // app_level.h25-29: Apple163, Pester162; actual Player dispatch wait.
        if (player->ThrowItemTimeout > 0 &&
            player->LastItemId == (puzzle_ == Puzzle::tunnel1014 ? 163 : 162)) ++facts_.throws_consumed;
        pending_throw_ = false;
        release_pending_ = true;
    }
    if (release_pending_ && !(player->input.current_buttons & button)) release_pending_ = false;
    if (facts_.complete || facts_.trigger_transition || facts_.endpoint_entered) {
        facts_.dependency = PuzzleDependency::none;
        facts_.owns_pad = true;
        publish();
        return true;
    }
    facts_.player_ready = player->D_80382D04_523114 == 1 && !player->IsInputDisabled && !player->IsPaused;
    if (!facts_.player_ready) { facts_.dependency = PuzzleDependency::player_not_ready; return false; }
    const auto ability = state.save()->player_flag(puzzle_ == Puzzle::tunnel1014 ? 0 : 1);
    require(bool(ability), "progression puzzle actual ability read failed");
    facts_.ability_available = ability.value;
    if (!ability.value) {
        facts_.dependency = puzzle_ == Puzzle::tunnel1014 ? PuzzleDependency::apple_reward : PuzzleDependency::pester_reward;
        return false;
    }
    if (!target.present) { facts_.dependency = PuzzleDependency::target_not_present; return false; }
    if (!facts_.endpoint_ready) { facts_.dependency = PuzzleDependency::endpoint_not_ready; return false; }
    if (!facts_.target_ready) { facts_.dependency = PuzzleDependency::target_not_ready; return false; }
    if (!player->ui || player->ui->Icons_NumItemsAvailable < (puzzle_ == Puzzle::tunnel1014 ? 1 : 2)) {
        facts_.dependency = PuzzleDependency::item_ui_not_ready; return false;
    }
    auto point = target.collision;
    if (puzzle_ == Puzzle::river1019) {
        const auto* world = host->world_owner();
        const auto* block = world ? world->current() : nullptr;
        facts_.current_block = block ? static_cast<int>(block->index) : -1;
        if (!block || block->index != 4) { facts_.dependency = PuzzleDependency::river_block4; return false; }
        require(block->resource, "progression puzzle current block metadata absent");
        const auto origin = block->resource->metadata.world_position;
        if (!surface_cached_) {
            surface_cached_ = river_surface(*world, target.position, point);
            if (surface_cached_) global_surface_ = {
                point.x+100*origin.x, point.y+100*origin.y, point.z+100*origin.z};
        } else point = {global_surface_.x-100*origin.x, global_surface_.y-100*origin.y,
                        global_surface_.z-100*origin.z};
        world::GroundResult ground{};
        facts_.trigger_surface = surface_cached_ && world->ground_at(point.x, point.z, ground) &&
            ground.surface == 0xff7fb2 && distance_squared(point, target.position) < 1000000;
        if (!facts_.trigger_surface) { facts_.dependency = PuzzleDependency::river_trigger_surface; return false; }
    } else if (!target.tangible) {
        facts_.dependency = PuzzleDependency::target_not_present; return false;
    }
    // Preserve pending requests until actual Player input consumption, then
    // require actual neutral release AND the Source45-update timeout to expire.
    // No guessed repeat interval or timer can manufacture an impact/transition.
    if (acquired_ || pending_throw_ || release_pending_) {
        facts_.owns_pad = true;
        facts_.dependency = PuzzleDependency::none;
        if (pending_throw_) { publish(button); return true; }
    }
    // Releasing Z and all turn buttons lets real Player complete zoom/turn first.
    if (player->gDirectionIndex < 0 || player->TurnToDirSpeed != 0) {
        acquired_ = true; facts_.owns_pad = true; facts_.dependency = PuzzleDependency::none;
        publish(); return true;
    }
    IsleGameVec3 aimed{};
    if (!projectile_target(*player, point, aimed)) {
        facts_.dependency = PuzzleDependency::projectile_out_of_reach;
        if (acquired_) publish();
        return facts_.owns_pad;
    }
    acquired_ = true;
    facts_.input_target = point;
    facts_.owns_pad = true;
    facts_.dependency = PuzzleDependency::none;
    std::int8_t x = 0, y = 0;
    aim_.point(aim_.owner, *player, aimed, false, x, y);
    std::uint16_t buttons = 0;
    if (!x && !y && !release_pending_ && player->ThrowItemTimeout == 0 &&
        !(player->input.current_buttons & button)) {
        buttons = button; pending_throw_ = true; ++facts_.throw_requests;
    }
    publish(buttons, x, y);
    return true;
}
} // namespace isle::private_campaign

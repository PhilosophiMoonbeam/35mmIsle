#include "native_valley_dash_puzzle.hpp"
#include "application_runtime.hpp"
#include "game_state.hpp"
#include "valley_scene.hpp"
#include "valley_actor_source.h"
#include "actor_owner_source.h"
#include "player_source.h"
#include "player_ui_source.h"
#include "item_owner.hpp"
#include "world_runtime.hpp"
#include <cmath>
#include <stdexcept>

namespace isle::private_campaign {
namespace {
// Authority: pokemonsnap df8815e75f3d79618466e01e0827c6165b93d11c.
// ROM metadata below was read from the admitted US ROM (SHA1
// edc7c49cc568c045fe48be0d18011c30f393cbaf), through the same address
// mapping/count conventions as actor_behavior_resources.cpp242-273. Only
// semantic identities/commands are recorded here, never ROM asset payloads.
//
// Valley primary block6 descriptor80118498 (world160,-70,200):
// spawn801182c4 Squirtle7/behavior4; 801180e4 Mankey56/behavior2;
// 80118114 Mankey56/behavior0; 80118204 switch1034; 80118024 endpoint1032.
// WorldSetup80100720 and spawn stride48/behavior+4 agree with
// course_resources.cpp54-58,83,102-112 and world_resources.cpp238-248.
// These are spawn windows, NOT a synthetic current-block activation predicate.
//
// src/valley/7A4610.c69-81,443-485: default Squirtle installs376c;
// ROM7accfc cmd16/value1500 ->802cc320,58. pokemon.c665-673 selects
// the actual Player when playerDist<1500 (NOT an Apple prerequisite).
// cc320 turns first to Player, then to actual behavior2 Mankey. Idle374c
// (ROM7accdc) cmd9->802cc4dc; attack378c (ROM7acd1c) cmd9->802cc514;
// both end58. Thus both idle and attack accept an actual Pester direct hit.
// 7A4610.c487-561 consumes the real sender Item+14 collisionVelocity:
// naive abs(targetYaw-itemYaw)<PI/9 snaps toward behavior2 Mankey; slide
// hSpeed300 must get within100 before the water/+-4000 termination to send33.
// Behavior2 Squirtles instead have372c cmd16/value1700 ->cc104/path cleanup;
// they are NOT targets for this chain.
//
// 7A8030.c32-65,244-260: behavior2 Mankey sets the mountain reference and
// graph40b0 (ROM7ad640) cmd33->cf594/fall+cleanup. Behavior0 starts hidden,
// graph3ffc (ROM7ad58c) cmd33->ceb9c/reveal. Both receive the real broadcast.
// Revealed Mankey graph401c (ROM7ad5ac) cmd9->cf07c, cmd13->cefb8,58:
// Apple kicks are NOT the endpoint trigger. c92-138 continuously turns to
// Player; processFlag1000 means targetYaw>=initial euler.y. c151-209 selects
// cf114 ONLY on that actual flag, else the terminal wrong-side animation.
// cf114 sends29 to switch1034, waits actual30 animation frames, starts its
// cutscene, sends29 twice more at actual animation boundaries, then sends32
// to endpoint1032 after real lastAnimationFrame59 completion.
// Switch graph41a8/ROM7ad738 cmd29->cf8bc,58 (7A8DD0.c58-89); its own
// actual sender-node distance squared<=100 and animation drive the switch.
// Endpoint graphEC658/ROM7c5be8 cmd32->d07bc,58 (c288-293,405-475).
//
// Endpoint c405-469 waits its real camera/player animation (bounded10800),
// then c161-237 dialogue. Every printed message accepts B; unlike A, B never
// jumps back to dialogue index2 at the optional question. ONLY after actual
// dialogue completion does it publish notice800 and canonical flag5. It then
// fades and sends Player7; no elapsed-update or request-count success here.
// Player.c380-413: B requires item UI>=2 and ThrowItemTimeout0, sets
// LastItemId162/timeout45. items.c1060-1073 emits actual hit9, never our code.
// sys/cmd.c30-41 resolves a NULL command sender to omCurrentObject: this
// Pester hit therefore carries the real Item, not a fabricated/null sender.
// 79F1B0.c39-49 copies the actual end reason into D_802D297C on Player7.
//
// Unexercised physical assumptions: Aim convergence/quantization; real Pester
// flight reaching the current collision point before its accepting graph or
// Mankey safe-side flag changes; actual slide terrain permitting distance100;
// and real animation/UI/camera scheduling. Pester movement is kind1:
// Original omEndProcess(NULL) returns, so the collision tail stores the real
// incoming collisionVelocity before the queued receiver runs. Observe that
// canonical field and the Item's root-local position, never fabricate a
// velocity or require an AUX placement that genuine native Items do not own.
constexpr std::uint16_t pester = 0x4000;
constexpr std::uint32_t squirtle_proximity = 0x802d376c;
constexpr std::uint32_t squirtle_idle = 0x802d374c;
constexpr std::uint32_t squirtle_attack = 0x802d378c;
constexpr std::uint32_t mountain_graph = 0x802d40b0;
constexpr std::uint32_t hidden_graph = 0x802d3ffc;
constexpr std::uint32_t revealed_graph = 0x802d401c;
constexpr std::uint32_t switch_graph = 0x802d41a8;
constexpr std::uint32_t endpoint_graph = 0x802ec658;
constexpr float pi = 3.14159265358979323846f;
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
IsleGameSourceCallback state_function(IsleGame* game, const IsleActorSource& source) {
    IsleGameSourceCallback function = nullptr;
    if (source.stateProc)
        require(isle_game_process_source_function(game, source.stateProc, &function) == ISLE_GAME_OK,
                "Valley Dash actual state observation rejected");
    return function;
}
void observe(const course::NativeHost& host, IsleGame* game, ValleyDashFacts& facts) {
    facts.squirtle = {}; facts.mountain_mankey = {}; facts.mankey = {};
    facts.switch_actor = {}; facts.endpoint = {};
    facts.mankey_safe_side = false;
    const IsleActorSource* sliding = nullptr;
    IsleGameObject* previous = nullptr;
    for (;;) {
        IsleGameObject* object = nullptr;
        IsleActorSource* actor = nullptr;
        IsleGameModel* model = nullptr;
        require(isle_actor_source_link3_next(game, previous, &object, &actor, &model) == ISLE_GAME_OK,
                "Valley Dash actual Actor traversal rejected");
        if (!object) break;
        previous = object;
        ValleyDashActorFacts* copy = nullptr;
        if (actor->id == 7 && actor->behavior == 4) copy = &facts.squirtle;
        else if (actor->id == 56 && actor->behavior == 2) copy = &facts.mountain_mankey;
        else if (actor->id == 56 && actor->behavior == 0) copy = &facts.mankey;
        else if (actor->id == 1034) copy = &facts.switch_actor;
        else if (actor->id == 1032) copy = &facts.endpoint;
        if (!copy) continue;
        require(!copy->present, "Valley Dash duplicate chain Actor");
        copy->present = true;
        copy->tangible = actor->tangible != 0;
        copy->behavior = actor->behavior;
        copy->graph = actor->transitionGraph ? actor->transitionGraph->source_identity : 0;
        copy->process_flags = actor->processFlags;
        copy->player_distance = actor->playerDist;
        copy->collision_radius = actor->collisionRadius;
        copy->initial_yaw = actor->euler.y;
        copy->collision = actor->collPosition;
        // Logical switch/endpoint Actors need no rendered model. Only actual
        // physical targets use model placement for aiming/slide observations.
        if (copy == &facts.squirtle || copy == &facts.mankey || copy == &facts.mountain_mankey) {
            const IsleGameModelPlacement* placement = nullptr;
            if (model) {
                require(isle_game_model_placement(game, model, &placement) == ISLE_GAME_OK,
                        "Valley Dash target placement observation rejected");
                if (placement && placement->mode != ISLE_GAME_MODEL_PLACEMENT_DISABLED) {
                    copy->model_ready = true;
                    copy->position = placement->transform.position;
                    copy->yaw = placement->transform.rotation.y;
                }
            }
        }
        const auto function = state_function(game, *actor);
        if (copy == &facts.squirtle) {
            facts.squirtle_activated |= function == func_802CC320_7A58B0 ||
                copy->graph == squirtle_idle || copy->graph == squirtle_attack;
            facts.squirtle_hit |= function == func_802CC4DC_7A5A6C || function == func_802CC514_7A5AA4;
            if (function == func_802CC514_7A5AA4) { facts.squirtle_sliding = true; sliding = actor; }
        } else if (copy == &facts.mountain_mankey) {
            facts.mountain_mankey_hit |= function == func_802CF594_7A8B24;
        } else if (copy == &facts.mankey) {
            facts.mankey_revealed |= function == func_802CEB9C_7A812C || copy->graph == revealed_graph;
            facts.mankey_safe_side = (actor->processFlags & 0x1000u) != 0;
            facts.mankey_hit |= function == func_802CF07C_7A860C || function == func_802CF114_7A86A4;
            facts.mankey_failed |= function == func_802CF07C_7A860C && !facts.mankey_safe_side;
            facts.trigger_transition |= function == func_802CF114_7A86A4;
        } else if (copy == &facts.switch_actor) {
            facts.switch_triggered |= function == func_802CF8BC_7A8E4C;
        } else {
            facts.endpoint_entered |= function == func_802D07BC_7A9D4C;
        }
    }
    if (sliding && sliding->interactionTarget && host.items_owner()) {
        auto* items = host.items_owner();
        const auto* item = items->source(sliding->interactionTarget);
        const auto instance = items->instance(sliding->interactionTarget);
        auto* model = instance ? items->native_model(instance) : nullptr;
        if (item) {
            facts.slide_item_direction_observed = true;
            facts.slide_item_direction = item->collisionVelocity;
            facts.slide_item_yaw = std::atan2(item->collisionVelocity.x, item->collisionVelocity.z);
            if (model && facts.mountain_mankey.model_ready) {
                const IsleGameModelNode* nodes = nullptr;
                std::size_t count = 0;
                require(isle_game_model_peek_nodes(game,model,&nodes,&count)==ISLE_GAME_OK && nodes && count,
                        "Valley Dash actual sender root-local observation rejected");
                facts.slide_item_position = nodes[0].transform.position;
                const auto& p = facts.slide_item_position;
                facts.slide_geometry_observed = true;
                facts.slide_target_yaw = std::atan2(facts.mountain_mankey.position.x-p.x,
                                                   facts.mountain_mankey.position.z-p.z);
                facts.slide_snap_observed |= std::abs(facts.slide_target_yaw-facts.slide_item_yaw) < pi/9 &&
                    std::abs(sliding->facingYaw-facts.slide_target_yaw) < 0.0001f;
            }
        }
    }
    // All native/Source/model borrows end here. Only scalar facts survive.
}
} // namespace

ValleyDashPuzzle::ValleyDashPuzzle(private_post_course::Controller controller, Aim aim)
    : controller_(controller), aim_(aim) {
    require(controller_.set && aim_.point, "Valley Dash requires actual pad and aim bridges");
}
void ValleyDashPuzzle::publish(std::uint16_t buttons, std::int8_t x, std::int8_t y) noexcept {
    controller_.set(controller_.owner, buttons, x, y);
}
bool ValleyDashPuzzle::update(const application::RuntimeView& view, const game::State& state,
                             std::uint64_t updates) {
    if (sampled_ && updates == last_update_) return facts_.owns_pad;
    require(!sampled_ || updates > last_update_, "Valley Dash update ordinal regressed");
    sampled_ = true; last_update_ = updates;
    facts_.owns_pad = false;
    facts_.dependency = ValleyDashDependency::course_not_active;
    if (!view.valley) return false;
    const auto& entry = *view.valley;
    if (entry.status() != course::LifecycleStatus::running &&
        entry.status() != course::LifecycleStatus::retiring) return false;
    const auto& source = entry.actors_source();
    facts_.endpoint_animation_complete |= facts_.endpoint_entered && source.D_802EC64C_7C5BDC == 1;
    facts_.endpoint_requested |= facts_.endpoint_entered && source.D_802D297C_7ABF0C != 0;
    require(state.save(), "Valley Dash requires actual canonical save");
    const auto dash = state.save()->player_flag(5), ability = state.save()->player_flag(1);
    require(bool(dash) && bool(ability), "Valley Dash canonical ability read failed");
    if (!baseline_) { baseline_ = true; facts_.dash_preexisting = dash.value; }
    facts_.pester_available = ability.value;
    facts_.dash_observed |= !facts_.dash_preexisting && facts_.endpoint_entered && dash.value;
    facts_.notice_observed |= facts_.endpoint_entered && (state.scene_notices() & 0x800u) != 0;
    facts_.complete = facts_.endpoint_entered && facts_.dash_observed && facts_.notice_observed;
    if (facts_.complete) {
        facts_.dependency = ValleyDashDependency::none;
        facts_.owns_pad = true;
        publish();
        return true;
    }
    const auto* host = entry.native_host();
    const auto* player = entry.adapter().native.player();
    if (!host || !player || !host->execution_open()) return false;
    observe(*host, player->game, facts_);
    const auto* world = host->world_owner();
    const auto* block = world ? world->current() : nullptr;
    facts_.current_block = block ? static_cast<std::int32_t>(block->index) : -1;
    facts_.block_progress = player->movement.move_time;
    facts_.player_ready = player->D_80382D04_523114 == 1 && !player->IsInputDisabled && !player->IsPaused;
    const auto* ui = player->ui;
    facts_.dialogue_ready = facts_.endpoint_entered && ui && ui->Msg_IsInteractive &&
        ui->Msg_IsPrinted && ui->Msg_MessageLength > 0;
    const auto reserve = [&](ValleyDashDependency dependency) {
        facts_.dependency = dependency; facts_.owns_pad = true; publish(); return true;
    };
    const auto unmet = [&](ValleyDashDependency dependency) {
        facts_.dependency = dependency;
        if (acquired_) { facts_.owns_pad = true; publish(); }
        return facts_.owns_pad;
    };
    if (pending_ != Press::none) {
        if (player->input.current_buttons & pester) {
            if (pending_ == Press::dialogue) {
                if (player->input.pressed_buttons & pester) ++facts_.dialogue_presses_consumed;
            } else if ((player->input.pressed_buttons & pester) && player->ThrowItemTimeout > 0 &&
                       player->LastItemId == ISLE_ITEM_PESTER_BALL) ++facts_.throws_consumed;
            else ++facts_.throw_presses_rejected;
            pending_ = Press::none; release_pending_ = true;
        } else {
            facts_.owns_pad = true; facts_.dependency = ValleyDashDependency::none;
            publish(pester); return true;
        }
    }
    if (release_pending_) {
        if (player->input.current_buttons & pester) return reserve(ValleyDashDependency::none);
        release_pending_ = false;
    }
    if (facts_.dash_preexisting) return unmet(ValleyDashDependency::dash_already_owned);
    if (facts_.endpoint_entered) {
        acquired_ = true;
        if (!facts_.endpoint_animation_complete) return reserve(ValleyDashDependency::endpoint_animation);
        if (!facts_.dialogue_ready) return reserve(ValleyDashDependency::dialogue_not_ready);
        pending_ = Press::dialogue; ++facts_.dialogue_requests;
        facts_.owns_pad = true; facts_.dependency = ValleyDashDependency::none;
        publish(pester); return true;
    }
    if (facts_.trigger_transition || facts_.switch_triggered) {
        acquired_ = true; return reserve(ValleyDashDependency::switch_sequence);
    }
    if (facts_.mankey_failed) return unmet(ValleyDashDependency::mankey_failed_side);
    if (!facts_.player_ready) return unmet(ValleyDashDependency::player_not_ready);
    if (!ability.value) return unmet(ValleyDashDependency::pester_reward);
    if (!ui || ui->Icons_NumItemsAvailable < 2) return unmet(ValleyDashDependency::item_ui_not_ready);
    if (!facts_.endpoint.present || facts_.endpoint.graph != endpoint_graph)
        return unmet(ValleyDashDependency::endpoint_not_ready);
    if (!facts_.switch_actor.present || facts_.switch_actor.graph != switch_graph)
        return unmet(ValleyDashDependency::switch_not_ready);
    const ValleyDashActorFacts* target = nullptr;
    Press press = Press::none;
    if (facts_.mankey_revealed) {
        if (!facts_.mankey.present || facts_.mankey.graph != revealed_graph ||
            !facts_.mankey.model_ready || !facts_.mankey.tangible)
            return unmet(ValleyDashDependency::hidden_mankey_not_ready);
        if (!facts_.mankey_safe_side) return unmet(ValleyDashDependency::mankey_safe_side);
        target = &facts_.mankey; press = Press::mankey;
    } else {
        if (facts_.squirtle_hit) {
            if (!facts_.squirtle.present) return unmet(ValleyDashDependency::squirtle_slide_missed);
            return reserve(ValleyDashDependency::squirtle_slide);
        }
        if (!facts_.squirtle.present || !facts_.mountain_mankey.present || !facts_.mankey.present)
            return unmet(ValleyDashDependency::final_block_actors);
        if (facts_.mountain_mankey.graph != mountain_graph || !facts_.mountain_mankey.model_ready)
            return unmet(ValleyDashDependency::mountain_mankey_not_ready);
        if (facts_.mankey.graph != hidden_graph) return unmet(ValleyDashDependency::mankey_reveal);
        if (facts_.squirtle.graph == squirtle_proximity)
            return unmet(ValleyDashDependency::squirtle_proximity);
        if ((facts_.squirtle.graph != squirtle_idle && facts_.squirtle.graph != squirtle_attack) ||
            !facts_.squirtle.model_ready || !facts_.squirtle.tangible)
            return unmet(ValleyDashDependency::squirtle_not_ready);
        target = &facts_.squirtle; press = Press::squirtle;
    }
    acquired_ = true;
    if (player->gDirectionIndex < 0 || player->TurnToDirSpeed != 0)
        return reserve(ValleyDashDependency::player_not_ready);
    IsleGameVec3 aimed{};
    facts_.input_target = target->collision;
    if (!projectile_target(*player, facts_.input_target, aimed))
        return unmet(ValleyDashDependency::projectile_out_of_reach);
    std::int8_t x = 0, y = 0;
    aim_.point(aim_.owner, *player, aimed, false, x, y);
    facts_.owns_pad = true;
    facts_.dependency = player->ThrowItemTimeout ? ValleyDashDependency::item_cooldown : ValleyDashDependency::none;
    std::uint16_t buttons = 0;
    if (!x && !y && player->ThrowItemTimeout == 0 && !(player->input.current_buttons & pester)) {
        if (press == Press::squirtle) {
            // Original Items_InitItem231-247: actual reticle ray +100Y,
            // speed50 plus Player velocity. Gravity leaves its XZ unchanged.
            const IsleGameVec3 ray{player->cam_target.x-player->CameraEyePos.x,
                player->cam_target.y-player->CameraEyePos.y+100,
                player->cam_target.z-player->CameraEyePos.z};
            const float length=std::hypot(ray.x,ray.y,ray.z);
            if (!(length>0)) return reserve(ValleyDashDependency::projectile_out_of_reach);
            const float vx=50*ray.x/length+player->PlayerVelocity.x;
            const float vz=50*ray.z/length+player->PlayerVelocity.z;
            const float dx=facts_.mountain_mankey.position.x-target->collision.x;
            const float dz=facts_.mountain_mankey.position.z-target->collision.z;
            const float distance=std::hypot(dx,dz);
            facts_.launch_yaw=std::atan2(vx,vz);
            facts_.launch_target_yaw=std::atan2(dx,dz);
            facts_.launch_uncertainty=distance>target->collision_radius?
                std::asin(target->collision_radius/distance):pi;
            // The real sender's post-response position still decides Source's
            // strict, unwrapped PI/9 snap. Reserve a collision-sphere margin;
            // actual collision vector/snap/broadcast remain required evidence.
            facts_.launch_alignment_ready=std::abs(facts_.launch_target_yaw-facts_.launch_yaw)+
                facts_.launch_uncertainty<pi/9;
            if (!facts_.launch_alignment_ready) return reserve(ValleyDashDependency::launch_alignment);
        }
        pending_ = press; buttons = pester;
        if (press == Press::squirtle) ++facts_.squirtle_throw_requests;
        else ++facts_.mankey_throw_requests;
    }
    publish(buttons, x, y);
    return true;
}
} // namespace isle::private_campaign

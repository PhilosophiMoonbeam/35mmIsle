#include "native_tunnel_photo_puzzles.hpp"
#include "application_runtime.hpp"
#include "game_state.hpp"
#include "tunnel_scene.hpp"
#include "tunnel_actor_source.h"
#include "actor_owner_source.h"
#include "player_source.h"
#include "player_photo_source.h"
#include "player_ui_source.h"
#include "item_owner.hpp"
#include "world_runtime.hpp"
#include <cmath>
#include <stdexcept>

namespace isle::private_campaign {
namespace {
// Immutable GAME df8815e75f3d79618466e01e0827c6165b93d11c:
// tunnel/diglett/code.c129-306 accepts CMD24 separately in each rise and
// advances actual stage1,2,4 only after that rise finishes. A roll count is
// neither command acceptance nor progression. Magnemite code.c188-195,266-315
// makes a nearby Apple necessary when zoom has set FLAG8: AppleNearby clears
// it, and the Apple graphs do not accept the repeated zoom command.
constexpr std::uint16_t a = 0x8000, z = 0x2000;
constexpr std::array<int, 4> ids{50, 51, 81, 82};
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
float squared_distance(const IsleGameVec3& x, const IsleGameVec3& y) {
    const float dx=x.x-y.x, dy=x.y-y.y, dz=x.z-y.z;
    return dx*dx+dy*dy+dz*dz;
}
bool accepts(const IsleActorSource& actor, int command) {
    if (!actor.transitionGraph) return false;
    for (std::size_t i=0; i<actor.transitionGraph->count; ++i) {
        const auto& row=actor.transitionGraph->rows[i];
        if (row.cmd==58) break;
        if (row.cmd==command) return true;
    }
    return false;
}
struct Actor {
    bool present=false, visible=false, eligible=false, focused=false;
    bool photo_ready=false, leader=false, attracted=false, luring=false, lure_ready=false;
    bool zoom_recovery=false, approaching_apple=false;
    int role=0;
    std::uint32_t graph=0;
    float distance=0, radius=0;
    IsleGameVec3 root{}, collision{};
};
struct Observation {
    Actor diglett{}, dugtrio{}, magnemite{}, magneton{};
    std::array<Actor, 3> magnets{};
    int active_role=0;
};
bool rise(IsleGameSourceCallback state, int id, int role) {
    if (id==50) return (role==1 && state==diglett_FirstRise) ||
        (role==2 && state==diglett_SecondRise) || (role==3 && state==diglett_ThirdRise);
    return state==dugtrio_FirstRise || state==dugtrio_SecondRise || state==dugtrio_ThirdRise;
}
Observation observe(const IslePlayerSource& player, const IsleTunnelActorSource& tunnel,
                    const IslePlayerPhotoFocus& focus, TunnelPhotoFacts& facts) {
    Observation result;
    IsleGameObject* previous=nullptr;
    for (;;) {
        IsleGameObject* object=nullptr;
        IsleActorSource* source=nullptr;
        IsleGameModel* model=nullptr;
        require(isle_actor_source_link3_next(player.game, previous, &object, &source, &model)==ISLE_GAME_OK,
                "Tunnel photo Actor traversal rejected");
        if (!object) break;
        previous=object;
        const int id=source->id;
        if (id!=50 && id!=51 && id!=81 && id!=82) continue;
        IsleGameSourceCallback state=nullptr;
        if (source->stateProc)
            require(isle_game_process_source_function(player.game, source->stateProc, &state)==ISLE_GAME_OK,
                    "Tunnel photo state observation rejected");
        Actor actor;
        actor.present=true;
        actor.role=source->behavior;
        actor.graph=source->transitionGraph ? source->transitionGraph->source_identity : 0;
        actor.collision=source->collPosition;
        actor.distance=source->playerDist;
        actor.radius=source->collisionRadius;
        actor.photo_ready=accepts(*source, 24) && rise(state, id, actor.role);
        if (id==50 && object==tunnel.D_802EEEC4_5EBF94 && rise(state, id, actor.role)) {
            result.active_role=actor.role;
            if (actor.role>=1 && actor.role<=3) {
                facts.diglett_roles_seen |= 1u << (actor.role-1);
                if (tunnel.diglett_IsPhotoTaken)
                    facts.rise_accepted[static_cast<std::size_t>(actor.role-1)]=true;
            }
        }
        if (id==51) facts.dugtrio_rise_seen |= rise(state, id, actor.role);
        if (id==82) facts.magneton_spawn_seen=true;
        if (id==81) {
            actor.leader=object==tunnel.magnemite_GroupLeader;
            actor.attracted=source->miscVars[1].field1!=0;
            actor.lure_ready=accepts(*source, 14) || accepts(*source, 15);
            actor.zoom_recovery=state==magnemite_ZoomedIn;
            actor.luring=state==magnemite_AppleNearby || state==magnemite_SearchApple || state==magnemite_WithApple;
            actor.approaching_apple=state==magnemite_AppleNearby || state==magnemite_SearchApple;
            if (actor.role>=1 && actor.role<=3)
                facts.magnemite_lure_seen[static_cast<std::size_t>(actor.role-1)] |= actor.luring;
            facts.magnemite_recoil_seen |= state==magnemite_HitByApple;
            facts.group_transition_seen |= state==magnemite_AllMagnemitesCombined ||
                state==magnemite_EvolveIntoMagneton;
        }
        if (model) {
            const IsleGameModelPlacement* placement=nullptr;
            require(isle_game_model_placement(player.game, model, &placement)==ISLE_GAME_OK && placement,
                    "Tunnel photo model placement rejected");
            if (placement->mode!=ISLE_GAME_MODEL_PLACEMENT_DISABLED) {
                actor.root=placement->transform.position;
                IsleGameObjectMetadata metadata{};
                require(isle_game_object_metadata(player.game, object, &metadata)==ISLE_GAME_OK,
                        "Tunnel photo Actor visibility rejected");
                const float horizontal=std::hypot(actor.collision.x-player.CameraEyePos.x,
                                                 actor.collision.z-player.CameraEyePos.z);
                const float pitch=std::atan2(actor.collision.y-player.CameraEyePos.y, horizontal);
                actor.visible=!(metadata.flags&1u) && source->tangible && (source->flags&4u) &&
                    actor.distance>=0 && actor.distance<=10000 && pitch>=player.MinPitch && pitch<=player.MaxPitch;
                actor.eligible=actor.visible && !(source->flags&8u);
            }
        }
        actor.focused=actor.eligible && focus.present && focus.object==object &&
            focus.pokemon_id==id && (focus.flags&4u) && !(focus.flags&8u);
        if (id==81 && actor.role>=1 && actor.role<=3) {
            const unsigned bit=1u << (actor.role-1);
            if (actor.lure_ready) facts.magnemite_ready_mask |= bit;
            if (actor.zoom_recovery) facts.magnemite_zoom_mask |= bit;
            if (actor.luring) facts.magnemite_luring_mask |= bit;
            if (state==magnemite_WithApple) facts.magnemite_arrived_mask |= bit;
            if (actor.attracted) facts.magnemite_attracted_mask |= bit;
            result.magnets[static_cast<std::size_t>(actor.role-1)]=actor;
        }
        auto* selected=id==50 ? &result.diglett : id==51 ? &result.dugtrio :
                       id==81 ? &result.magnemite : &result.magneton;
        if (id==81 && selected->present && selected->eligible && selected->luring &&
            (!actor.eligible || !actor.luring)) continue;
        // Observe and preaim the actual active unburrow before CMD24 opens;
        // eligibility and its real graph remain mandatory at shutter time.
        if (id==50 && (object!=tunnel.D_802EEEC4_5EBF94 || !rise(state, id, actor.role))) continue;
        if (id==51 && !rise(state, id, actor.role)) continue;
        if (!selected->present || (actor.eligible && !selected->eligible) ||
            (id==81 && actor.eligible && actor.luring && !selected->luring) ||
            (actor.eligible==selected->eligible && actor.distance<selected->distance)) *selected=actor;
    }
    if (result.active_role>=1 && result.active_role<=3) {
        const auto i=static_cast<std::size_t>(result.active_role-1);
        if (result.diglett.visible) ++facts.rise_visible_updates[i];
        if (result.diglett.photo_ready) ++facts.rise_ready_updates[i];
        if (result.diglett.focused && result.diglett.photo_ready) {
            ++facts.rise_focus_updates[i];
            if (player.sTimerAfterPhotoTaken>=0) ++facts.rise_cooldown_updates[i];
        }
    }
    for (std::size_t i=0; i<3; ++i) for (std::size_t j=i+1; j<3; ++j) {
        if (!result.magnets[i].present || !result.magnets[j].present) continue;
        const float distance=std::sqrt(squared_distance(result.magnets[i].root, result.magnets[j].root));
        if (facts.nearest_magnemite_distance<0 || distance<facts.nearest_magnemite_distance)
            facts.nearest_magnemite_distance=distance;
    }
    // No object/model/Photo borrow escapes: only bounded scalar observations.
    return result;
}

// Read-only launch forecast, never an Item update or landing credit. Reject
// obstructed arcs and sphere hits along the flight, not just at its endpoint.
bool landing(const world::World& world, const IslePlayerSource& player,
             const Observation& observation, const IsleGameVec3& ray, IsleGameVec3& result) {
    const IsleGameCameraParameters* camera=nullptr;
    require(isle_game_camera_parameters(player.game, player.camera, &camera)==ISLE_GAME_OK && camera,
            "Tunnel Apple camera observation rejected");
    IsleGameVec3 position{camera->look_at.eye[0], camera->look_at.eye[1], camera->look_at.eye[2]};
    IsleGameVec3 velocity{ray.x-position.x, ray.y-position.y+100, ray.z-position.z};
    const float length=std::hypot(velocity.x, velocity.y, velocity.z);
    if (!(length>0)) return false;
    velocity={50*velocity.x/length+player.PlayerVelocity.x,
              50*velocity.y/length+player.PlayerVelocity.y,
              50*velocity.z/length+player.PlayerVelocity.z};
    for (unsigned step=0; step<400; ++step) {
        world::GroundResult ground{}, ceiling{};
        if (!world.ground_at(position.x, position.z, ground) ||
            (world.ceiling_at(position.x, position.z, ceiling) && position.y>ceiling.height)) return false;
        if (position.y<=ground.height+.375f) {
            if (ground.surface==0x337fb2) return false;
            result={position.x, ground.height, position.z};
            return true;
        }
        world::Vec3 hit{}, reflected{};
        if (world.collide({position.x, position.y, position.z},
                          {velocity.x*.5f, velocity.y*.5f, velocity.z*.5f}, hit, reflected)>=0) return false;
        // Source checks Actor spheres before gravity; use that swept segment.
        const IsleGameVec3 segment{velocity.x*.5f, velocity.y*.5f, velocity.z*.5f};
        const float segment_squared=segment.x*segment.x+segment.y*segment.y+segment.z*segment.z;
        for (const auto& actor:observation.magnets) if (actor.present && actor.radius>0 && segment_squared>0) {
            const float projection=((actor.collision.x-position.x)*segment.x+
                (actor.collision.y-position.y)*segment.y+(actor.collision.z-position.z)*segment.z)/segment_squared;
            const float t=projection<0 ? 0 : projection>1 ? 1 : projection;
            const IsleGameVec3 closest{position.x+t*segment.x, position.y+t*segment.y, position.z+t*segment.z};
            if (squared_distance(closest, actor.collision)<=actor.radius*actor.radius) return false;
        }
        velocity.y-=.6f;
        position.x+=velocity.x*.5f; position.y+=velocity.y*.5f; position.z+=velocity.z*.5f;
    }
    return false;
}
unsigned near_mask(const Observation& observation, const IsleGameVec3& point) {
    unsigned mask=0;
    for (std::size_t i=0; i<3; ++i)
        if (observation.magnets[i].present && squared_distance(point, observation.magnets[i].root)<600*600)
            mask |= 1u << i;
    return mask;
}
bool ground_target(const world::World& world, const IslePlayerSource& player,
                   const Observation& observation, IsleGameVec3& target, IsleGameVec3& predicted,
                   TunnelPhotoFacts& facts) {
    // Distances are to independent roots: a connected follower is not a
    // second lure recipient. If no common 600-radius region exists, move the
    // leader (or one unconnected actor) toward the others in a physical step.
    std::array<IsleGameVec3, 13> centers{};
    std::size_t count=0, live=0;
    IsleGameVec3 sum{};
    const auto independent=[](const Actor& actor) { return actor.present && (!actor.attracted || actor.leader); };
    for (const auto& actor:observation.magnets) if (independent(actor)) {
        sum.x+=actor.root.x; sum.z+=actor.root.z; ++live;
    }
    if (live<2) { facts.apple_plan_result=1; return false; }
    facts.apple_plan_result=2;
    centers[count++]={sum.x/static_cast<float>(live), 0, sum.z/static_cast<float>(live)};
    for (std::size_t i=0; i<3; ++i) for (std::size_t j=i+1; j<3; ++j) {
        const auto& first=observation.magnets[i]; const auto& second=observation.magnets[j];
        if (!independent(first) || !independent(second)) continue;
        centers[count++]={(first.root.x+second.root.x)*.5f, 0, (first.root.z+second.root.z)*.5f};
        const float distance=std::hypot(first.root.x-second.root.x, first.root.z-second.root.z);
        if (distance>0) {
            const float fraction=distance>900 ? 450/distance : .5f;
            centers[count++]={first.root.x+(second.root.x-first.root.x)*fraction, 0,
                              first.root.z+(second.root.z-first.root.z)*fraction};
            centers[count++]={second.root.x+(first.root.x-second.root.x)*fraction, 0,
                              second.root.z+(first.root.z-second.root.z)*fraction};
        }
    }
    for (const auto& actor:observation.magnets) if (independent(actor)) centers[count++]=actor.root;
    bool found=false;
    unsigned best_count=0;
    float best_distance=0;
    for (std::size_t c=0; c<count; ++c) for (int dx=-2; dx<=2; ++dx) for (int dz=-2; dz<=2; ++dz) {
        world::GroundResult ground{};
        const float x=centers[c].x+75*static_cast<float>(dx), zz=centers[c].z+75*static_cast<float>(dz);
        if (!world.ground_at(x, zz, ground) || ground.surface==0x337fb2) continue;
        const IsleGameVec3 point{x, ground.height, zz};
        unsigned nearby=0;
        float total=0;
        bool leader_outside=false, can_lure=false;
        for (const auto& actor:observation.magnets) if (independent(actor)) {
            const float distance=squared_distance(point, actor.root);
            total+=distance;
            // Leave room for pad quantization, ground crossing and flight-time
            // motion. Actual CMD14/15 remains strict <600 with no such margin.
            if (distance<525*525) {
                ++nearby;
                can_lure |= actor.lure_ready || actor.zoom_recovery;
            } else if (actor.leader) leader_outside=true;
        }
        if (!can_lure || leader_outside ||
            (found && (nearby<best_count || (nearby==best_count && total>=best_distance)))) continue;
        ++facts.apple_plan_candidates;
        IsleGameVec3 aimed{}, impact{};
        if (!projectile_target(player, point, aimed) || !landing(world, player, observation, aimed, impact) ||
            squared_distance(point, impact)>75*75) {
            ++facts.apple_plan_arc_rejections;
            if (!found) facts.apple_plan_result=3;
            continue;
        }
        target=point; predicted=impact; best_count=nearby; best_distance=total; found=true;
        facts.apple_plan_result=4;
    }
    return found;
}
struct AppleObservation {
    bool present=false;
    std::uint32_t id=0;
    std::array<std::uint64_t, 3> identity{};
    int state=0;
    unsigned flags=0;
    IsleGameVec3 position{};
};
void remember_apples(items::Owner& owner,
                     std::array<std::array<std::uint64_t, 3>, 20>& identities, std::size_t& count) {
    count=0;
    for (const auto& binding:owner.render_bindings()) {
        if (!binding.model.object) continue;
        const auto* source=owner.source(binding.model.object);
        if (!source || source->itemID!=ISLE_ITEM_APPLE) continue;
        require(count<identities.size(), "Tunnel Apple live identities exceed Source capacity");
        const auto identity=binding.model.state.instance;
        require(bool(identity), "Tunnel Apple incarnation observation rejected");
        identities[count++]={identity.pool, identity.slot, identity.incarnation};
    }
}
AppleObservation observe_apple(items::Owner& owner, IsleGame* game,
                              const std::array<std::uint64_t, 3>& tracked,
                              const std::array<std::array<std::uint64_t, 3>, 20>& before,
                              std::size_t before_count) {
    AppleObservation result;
    // Owner enumerates its real live bindings; no iterator, Item/Actor
    // mutation or retained object/model handle is used by this observer.
    for (const auto& binding:owner.render_bindings()) {
        if (!binding.model.object) continue;
        const auto* source=owner.source(binding.model.object);
        if (!source || source->itemID!=ISLE_ITEM_APPLE) continue;
        IsleGameObjectMetadata metadata{};
        require(isle_game_object_metadata(game, binding.model.object, &metadata)==ISLE_GAME_OK,
                "Tunnel Apple identity observation rejected");
        const auto instance=binding.model.state.instance;
        require(bool(instance), "Tunnel Apple incarnation observation rejected");
        const std::array<std::uint64_t, 3> identity{instance.pool, instance.slot, instance.incarnation};
        if (tracked[0]) {
            if (identity!=tracked) continue;
        } else {
            bool existed=false;
            for (std::size_t i=0; i<before_count; ++i) existed |= before[i]==identity;
            if (existed) continue;
            require(!result.present, "Tunnel Apple creation identity is ambiguous");
        }
        const IsleGameModelPlacement* placement=nullptr;
        require(isle_game_model_placement(game, binding.model.model, &placement)==ISLE_GAME_OK && placement,
                "Tunnel Apple placement observation rejected");
        result={true, metadata.id, identity, source->state, source->flags, placement->transform.position};
    }
    return result;
}
} // namespace

TunnelPhotoPuzzles::TunnelPhotoPuzzles(private_post_course::Controller controller, Aim aim)
    : controller_(controller), aim_(aim) {
    require(controller_.set && aim_.point, "Tunnel photos require actual pad and camera bridges");
}
void TunnelPhotoPuzzles::publish(std::uint16_t buttons, std::int8_t x, std::int8_t y) noexcept {
    controller_.set(controller_.owner, buttons, x, y);
}
bool TunnelPhotoPuzzles::update(const application::RuntimeView& view, const game::State& state,
                               std::uint64_t updates) {
    if (sampled_ && updates==last_update_) return facts_.owns_pad;
    require(!sampled_ || updates>last_update_, "Tunnel photo update ordinal regressed");
    sampled_=true; last_update_=updates;
    facts_.owns_pad=false; facts_.phase=TunnelPhotoPhase::idle;
    facts_.target_present=false; facts_.target_eligible=false; facts_.focus_eligible=false;
    facts_.target_id=0; facts_.target_role=0; facts_.target_graph=0;
    facts_.zoom_recovery_active=false;
    facts_.magnemite_ready_mask=0; facts_.magnemite_zoom_mask=0;
    facts_.magnemite_luring_mask=0; facts_.magnemite_arrived_mask=0; facts_.magnemite_attracted_mask=0;
    facts_.nearest_magnemite_distance=-1;
    facts_.apple_item_present=false; facts_.apple_flight_active=false; facts_.apple_near_mask=0;
    facts_.apple_root_distances={{-1, -1, -1}};
    facts_.apple_plan_result=0;
    if (!view.tunnel || view.tunnel->status()!=course::LifecycleStatus::running) return false;
    const auto& entry=*view.tunnel;
    const auto* host=entry.native_host();
    const auto* player=entry.adapter().native.player();
    if (!host || !host->execution_open() || !player || entry.source().tunnel_EndLevelReason) return false;
    require(player->status==ISLE_GAME_OK && player->photo && player->photo->status==ISLE_GAME_OK,
            "Tunnel photo actual Player/Photo failed");
    if (!reports_sampled_) {
        require(state.save(), "Tunnel photos require actual canonical Save");
        for (std::size_t i=0; i<ids.size(); ++i) {
            const auto present=entry.photos().report_present(ids[i]);
            require(bool(present), "Tunnel photo canonical Report presence unavailable");
            facts_.saved_reports[i]=present.value;
        }
        reports_sampled_=true;
    }
    const auto& photo=*player->photo;
    require(photo.gPhotoCount>=observed_roll_ && photo.gPhotoCount<=60,
            "Tunnel photo actual roll count invalid");
    for (; observed_roll_<photo.gPhotoCount; ++observed_roll_) {
        const auto& captured=photo.D_800B0598[observed_roll_];
        facts_.last_capture_slot=observed_roll_;
        facts_.last_capture_raw_subject_zero=captured.pokemons[0].pokemonID;
        facts_.last_capture_requested_id=0;
        if (captured.levelID==1 && (captured.unk_00_16&0xe0u)==0x20u) {
            const int primary=captured.pokemons[0].pokemonID;
            if (!primary) ++facts_.unknown_primary_exposures;
            for (std::size_t i=0; i<ids.size(); ++i)
                if (primary==ids[i]) ++facts_.captures[i];
        }
    }
    if ((pending_shot_>=0 || pending_apple_) && (player->input.current_buttons&a)) {
        const bool pressed=(player->input.pressed_buttons&a)!=0;
        if (pending_shot_>=0 && pressed && photo.gPhotoCount>pending_slot_) {
            const auto index=static_cast<std::size_t>(pending_shot_);
            ++facts_.shutters_consumed[index];
            const auto& captured=photo.D_800B0598[pending_slot_];
            facts_.last_capture_slot=pending_slot_;
            facts_.last_capture_raw_subject_zero=captured.pokemons[0].pokemonID;
            facts_.last_capture_requested_id=ids[index];
            // Requests describe pad intent only. Candidate satisfaction is
            // counted above from actual canonical raw primary, never this ID.
            if (captured.levelID==1 && (captured.unk_00_16&0xe0u)==0x20u) {
                ++facts_.requested_exposures[index];
                if (captured.pokemons[0].pokemonID!=ids[index])
                    ++facts_.requested_primary_mismatches;
            }
        }
        if (pending_apple_ && pressed && player->ThrowItemTimeout>0 && player->LastItemId==163) {
            ++facts_.apples_consumed;
            apple_tracking_=true; apple_landed_=false; apple_tracked_identity_={};
            apple_flight_updates_=0; apple_response_updates_=0;
        }
        release_zoomed_=pending_shot_>=0;
        pending_shot_=-1; pending_slot_=-1; pending_apple_=false; release_pending_=true;
    }
    if (release_pending_ && !(player->input.current_buttons&a)) release_pending_=false;
    const auto& tunnel=entry.actors_source();
    if (facts_.diglett_stage==4 && tunnel.D_802EEED0_5EBFA0==1)
        ++facts_.diglett_stage_restarts;
    facts_.diglett_stage=tunnel.D_802EEED0_5EBFA0;
    if (facts_.diglett_stage==1) facts_.stage_values_seen|=1u;
    if (facts_.diglett_stage==2) facts_.stage_values_seen|=2u;
    if (facts_.diglett_stage==4) facts_.stage_values_seen|=4u;
    facts_.diglett_photo_taken=tunnel.diglett_IsPhotoTaken!=0;
    facts_.dugtrio_photo_taken=tunnel.dugtrio_IsPhotoTaken!=0;
    facts_.num_connected=tunnel.magnemite_NumConnected;
    if (facts_.num_connected>facts_.max_connected) facts_.max_connected=facts_.num_connected;
    IslePlayerPhotoFocus focus{};
    require(isle_player_photo_published_focus(player->photo, &focus)==ISLE_GAME_OK,
            "Tunnel photo published focus unavailable");
    const auto observed=observe(*player, tunnel, focus, facts_);
    facts_.active_diglett_role=observed.active_role;
    facts_.player_ready=player->D_80382D04_523114==1 && !player->IsInputDisabled && !player->IsPaused;
    if (!facts_.player_ready) return false;
    if (apple_tracking_) {
        auto* items=host->items_owner();
        require(items, "Tunnel Apple actual Item owner unavailable");
        const auto apple=observe_apple(*items, player->game, apple_tracked_identity_,
                                       apple_prethrow_identities_, apple_prethrow_count_);
        facts_.apple_item_present=apple.present;
        if (apple.present) {
            apple_tracked_identity_=apple.identity;
            facts_.apple_item_pool=apple.identity[0];
            facts_.apple_item_slot=static_cast<std::uint32_t>(apple.identity[1]);
            facts_.apple_item_incarnation=apple.identity[2];
            facts_.apple_item_id=apple.id; facts_.apple_item_state=apple.state;
            facts_.apple_item_flags=apple.flags; facts_.apple_position=apple.position;
            facts_.apple_near_mask=near_mask(observed, apple.position);
            for (std::size_t i=0; i<3; ++i) if (observed.magnets[i].present)
                facts_.apple_root_distances[i]=std::sqrt(squared_distance(apple.position, observed.magnets[i].root));
            if ((apple.flags&ISLE_ITEM_TOUCHED_GROUND) && !apple_landed_) {
                apple_landed_=true; ++facts_.apple_landings_seen;
            }
            // A still Apple must be offered to the recovered Source graph
            // before another driver's ordinary Z can retrigger CMD55. Flight
            // time itself grants neither a nearby-item nor a group fact.
            const bool awaiting_source=apple.state==ISLE_ITEM_STILL && facts_.apple_near_mask &&
                !(facts_.apple_near_mask&facts_.magnemite_luring_mask) &&
                !(facts_.apple_near_mask&facts_.magnemite_attracted_mask) && ++apple_response_updates_<=16;
            if (apple.state==ISLE_ITEM_FLYING || awaiting_source) {
                if (++apple_flight_updates_<=512) {
                    ++facts_.apple_flight_updates;
                    facts_.apple_flight_active=true;
                } else {
                    facts_.apple_flight_exhausted=true;
                    apple_tracking_=false;
                }
            } else apple_tracking_=false;
        } else {
            ++facts_.apple_lost_flights; apple_tracking_=false;
        }
    }
    // Finish actual request consumption/release before handing the pad back.
    if (pending_shot_>=0 || pending_apple_ || release_pending_) {
        facts_.owns_pad=true; facts_.phase=TunnelPhotoPhase::release;
        const bool zoom=pending_shot_>=0 || (release_pending_ && release_zoomed_);
        publish(static_cast<std::uint16_t>((zoom ? z : 0) | ((pending_shot_>=0 || pending_apple_) ? a : 0)));
        return true;
    }
    if (facts_.apple_flight_active) {
        facts_.owns_pad=true; facts_.phase=TunnelPhotoPhase::apple;
        publish();
        return true;
    }
    const auto satisfied=[&](std::size_t index) { return facts_.saved_reports[index] || facts_.captures[index]>0; };
    if (photo.gPhotoCount>=59) return false;
    const Actor* target=nullptr;
    std::size_t index=0;
    TunnelPhotoPhase phase=TunnelPhotoPhase::idle;
    if (!satisfied(1) && observed.dugtrio.visible) {
        target=&observed.dugtrio; index=1; phase=TunnelPhotoPhase::dugtrio;
    } else if ((!satisfied(1) || !satisfied(0)) && !tunnel.diglett_IsPhotoTaken &&
               observed.diglett.visible) {
        target=&observed.diglett; index=0; phase=TunnelPhotoPhase::diglett;
    } else if (!satisfied(3) && observed.magneton.eligible) {
        target=&observed.magneton; index=3; phase=TunnelPhotoPhase::magneton;
    } else if (!satisfied(2) && observed.magnemite.eligible && observed.magnemite.luring) {
        // Apple graphs clear FLAG8 and do not accept repeated zoom CMD55.
        target=&observed.magnemite; index=2; phase=TunnelPhotoPhase::magnemite;
    }
    if (target) {
        facts_.phase=phase; facts_.owns_pad=true;
        facts_.target_id=ids[index]; facts_.target_role=target->role; facts_.target_graph=target->graph;
        facts_.target_present=target->present; facts_.target_eligible=target->eligible;
        facts_.focus_eligible=target->focused; facts_.target_position=target->collision;
        std::int8_t x=0, y=0;
        if (player->gDirectionIndex==-1 && !player->TargetDirectionZoomedIn)
            aim_.point(aim_.owner, *player, target->collision, true, x, y);
        std::uint16_t buttons=z;
        // The canonical eligible focus is the shutter authority. Requiring
        // zero tracking stick missed an entire third rise while a moving cart
        // needed -8/-9 to keep that very focus centered (643d,5912-5968).
        if (player->gDirectionIndex==-1 && !player->TargetDirectionZoomedIn &&
            target->focused && (index>1 || target->photo_ready) &&
            player->sTimerAfterPhotoTaken<0 && !(player->input.current_buttons&a)) {
            buttons|=a; pending_shot_=static_cast<int>(index); pending_slot_=photo.gPhotoCount;
            // Preserve the published eligible focus on the pressed-A update.
            x=0; y=0;
            ++facts_.shutter_requests[index];
            if (index==0 && target->role>=1 && target->role<=3)
                ++facts_.rise_shutter_requests[static_cast<std::size_t>(target->role-1)];
        }
        publish(buttons, x, y);
        return true;
    }
    // Outside a real rise, launch/recovery, or observed in-flight Apple window,
    // yield to ordinary photos. Cooldown and a pending group are not success.
    if ((satisfied(2) && satisfied(3)) || facts_.num_connected>=2 || facts_.magneton_spawn_seen ||
        facts_.apple_requests>=6 || facts_.apple_flight_exhausted || player->ThrowItemTimeout!=0) return false;
    bool lure_ready=false, approaching=false, recovering=false;
    for (const auto& actor:observed.magnets) if (actor.present) {
        lure_ready |= actor.lure_ready && !actor.attracted;
        recovering |= actor.zoom_recovery && !actor.attracted;
        approaching |= actor.approaching_apple;
    }
    if (approaching) { apple_recovery_=false; return false; }
    if ((!lure_ready && !recovering && !apple_recovery_) ||
        !player->ui || player->ui->Icons_NumItemsAvailable<1) return false;
    const auto apple=state.save()->player_flag(0);
    require(bool(apple), "Tunnel photo actual Apple reward unavailable");
    facts_.apple_available=apple.value;
    if (!apple.value) return false;
    const auto* world=host->world_owner();
    IsleGameVec3 ground{}, aimed{}, predicted{};
    if (!world || !ground_target(*world, *player, observed, ground, predicted, facts_)) return false;
    // Release Z even while Source is in one of the three ZoomedIn graphs,
    // which deliberately accept no CMD14/15. Otherwise ordinary Z input keeps
    // retriggering CMD55 forever. This bound limits neutral pad ownership only;
    // elapsed updates never prove graph readiness, an Apple, or group success.
    if (recovering || player->gDirectionIndex<0 || player->TurnToDirSpeed!=0)
        apple_recovery_=true;
    if (apple_recovery_) {
        if (lure_ready && !recovering && player->gDirectionIndex>=0 && player->TurnToDirSpeed==0) {
            apple_recovery_=false;
        } else {
            if (facts_.zoom_recovery_updates>=128) {
                facts_.zoom_recovery_exhausted=true;
                return false;
            }
            ++facts_.zoom_recovery_updates;
            facts_.zoom_recovery_active=true;
            facts_.phase=TunnelPhotoPhase::apple; facts_.owns_pad=true;
            publish();
            return true;
        }
    }
    if (!lure_ready) return false;
    facts_.phase=TunnelPhotoPhase::apple; facts_.owns_pad=true; facts_.apple_ground=ground;
    facts_.apple_predicted_landing=predicted;
    facts_.apple_plan_near_mask=near_mask(observed, predicted);
    // Real items require actual recovered graph, unzoom, completed turn,
    // A released and Source timeout0; recovery updates grant no throw credit.
    if (!projectile_target(*player, ground, aimed)) { publish(); return true; }
    std::int8_t x=0, y=0;
    aim_.point(aim_.owner, *player, aimed, false, x, y);
    std::uint16_t buttons=0;
    if (!x && !y && player->gDirectionIndex>=0 && player->TurnToDirSpeed==0 &&
        player->ThrowItemTimeout==0 && !(player->input.current_buttons&a)) {
        IsleGameVec3 actual_ray_landing{};
        const bool clear=landing(*world, *player, observed, player->cam_target, actual_ray_landing);
        const unsigned actual_mask=clear ? near_mask(observed, actual_ray_landing) : 0;
        if (clear && (actual_mask&facts_.apple_plan_near_mask)==facts_.apple_plan_near_mask &&
            squared_distance(actual_ray_landing, ground)<=75*75) {
            facts_.apple_predicted_landing=actual_ray_landing;
            auto* items=host->items_owner();
            require(items, "Tunnel Apple actual Item owner unavailable");
            remember_apples(*items, apple_prethrow_identities_, apple_prethrow_count_);
            buttons=a; pending_apple_=true; ++facts_.apple_requests;
        } else ++facts_.apple_arc_rejections;
    }
    publish(buttons, x, y);
    return true;
}
} // namespace isle::private_campaign

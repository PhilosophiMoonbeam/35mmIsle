#include "native_river_photo_puzzle.hpp"
#include "application_runtime.hpp"
#include "game_state.hpp"
#include "river_scene.hpp"
#include "river_actor_source.h"
#include "actor_owner_source.h"
#include "actor_owner.hpp"
#include "item_owner.hpp"
#include "player_source.h"
#include "player_photo_source.h"
#include "player_ui_source.h"
#include "world_runtime.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace isle::private_campaign {
namespace {
constexpr std::uint16_t a_button=0x8000;
constexpr std::uint16_t z_button=0x2000;
constexpr float trigger_squared=225000.0f;
constexpr std::uint64_t overall_cap=8192, observation_cap=512, evolution_cap=2048;
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
bool finite(const IsleGameVec3& p) {
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
}
float distance_squared(const IsleGameVec3& a, const IsleGameVec3& b) {
    const float x=a.x-b.x, y=a.y-b.y, z=a.z-b.z;
    return x*x+y*y+z*z;
}
float endpoint_squared(const IsleGameVec3& a, const IsleGameVec3& b) {
    const float x=a.x-b.x, z=a.z-b.z;
    return x*x+z*z;
}
IsleGameVec3 global(const IsleGameVec3& p, const IsleGameVec3& origin) {
    return {p.x+origin.x, p.y+origin.y, p.z+origin.z};
}
float angle_error(float a, float b) {
    return std::remainder(a-b, 6.2831853071795864769f);
}
RiverIncarnation key(menu::animation::InstanceHandle instance) {
    return {instance.pool, instance.slot, instance.incarnation};
}
bool accepts(const IsleActorSource& actor, int cmd) {
    if (!actor.transitionGraph) return false;
    for (std::size_t i=0; i<actor.transitionGraph->count; ++i) {
        const auto& row=actor.transitionGraph->rows[i];
        if (row.cmd==58) break;
        if (row.cmd==cmd && (row.state || row.aux)) return true;
    }
    return false;
}
using river_detail::Actor;
using river_detail::Sphere;
using river_detail::Planning;
using river_detail::Item;
bool same_subject(const IslePlayerPokemonPhoto& a, const IslePlayerPokemonPhoto& b) {
    return a.pokemonID==b.pokemonID && a.unk_00_13==b.unk_00_13 &&
        a.specialID==b.specialID && a.poseID==b.poseID && a.animationTime==b.animationTime &&
        a.position.x==b.position.x && a.position.y==b.position.y && a.position.z==b.position.z && a.yaw==b.yaw;
}
void snapshot(Planning& out, const IslePlayerSource& player, actor::Owner& actors, items::Owner& items,
              const RiverIncarnation& selected, const RiverIncarnation& child,
              const std::array<RiverIncarnation, 2>& attempted, const IsleGameVec3& origin) {
    out.parent={};
    out.sphere_count=0; out.item_count=0; out.child_count=0; out.capacity_ok=true;
    out.other_evolving=false; out.multiple_other_evolving=false; out.other_global={};
    out.captured_child_context=false; out.focus={};
    IslePlayerPhotoFocus focus{};
    require(isle_player_photo_published_focus(player.photo, &focus)==ISLE_GAME_OK,
            "River published focus observation rejected");
    out.focus.present=focus.present; out.focus.pokemon_id=focus.pokemon_id;
    out.focus.flags=focus.flags; out.focus.context=focus.context;
    IsleGameObject* previous=nullptr;
    for (;;) {
        IsleGameObject* object=nullptr;
        IsleActorSource* source=nullptr;
        IsleGameModel* model=nullptr;
        require(isle_actor_source_link3_next(player.game, previous, &object, &source, &model)==ISLE_GAME_OK,
                "River live Actor traversal rejected");
        if (!object) break;
        previous=object;
        if (source->tangible && source->collisionRadius>0) {
            if (out.sphere_count==out.spheres.size()) out.capacity_ok=false;
            else out.spheres[out.sphere_count++]={source->collPosition, source->collisionRadius};
        }
        if ((source->id!=79 && source->id!=80) || !model) continue;
        const auto instance=actors.instance(object);
        require(bool(instance), "River Actor incarnation unavailable");
        const IsleGameModelPlacement* placement=nullptr;
        require(isle_game_model_placement(player.game, model, &placement)==ISLE_GAME_OK && placement,
                "River Actor placement observation rejected");
        if (placement->mode==ISLE_GAME_MODEL_PLACEMENT_DISABLED) continue;
        Actor actor;
        actor.present=true; actor.identity=key(instance);
        actor.root=placement->transform.position; actor.global_root=global(actor.root, origin);
        actor.yaw=placement->transform.rotation.y; actor.collision=source->collPosition;
        actor.endpoint=source->pos1; actor.radius=source->collisionRadius;
        actor.flags=source->flags; actor.behavior=source->behavior; actor.counter=source->counter;
        actor.process_flags=source->processFlags; actor.model_animation=source->modelAnims;
        actor.animation_time=actors.animation().model(instance).animation_time;
        actor.graph=source->transitionGraph ? source->transitionGraph->source_identity : 0;
        actor.apple=key(items.instance(source->apple));
        actor.interaction=key(items.instance(source->interactionTarget));
        IsleGameSourceCallback callback=nullptr;
        if (source->stateProc)
            require(isle_game_process_source_function(player.game, source->stateProc, &callback)==ISLE_GAME_OK,
                    "River Actor state identity observation rejected");
        // Equality observes admitted callbacks. Never call Source handlers.
        actor.approaching=callback==func_802D9808_6C15E8;
        actor.evolving=callback==func_802D9AC4_6C18A4;
        actor.state=actor.evolving ? 0x802d9ac4u : actor.approaching ? 0x802d9808u : 0;
        actor.ready=accepts(*source, 15) && accepts(*source, 28);
        actor.endpoint_ready=source->path && finite(actor.endpoint) && finite(actor.root) &&
            (source->miscVars[1].field1==1 || actor.evolving);
        actor.endpoint_distance=endpoint_squared(actor.root, actor.endpoint);
        actor.explicit_ground=source->forbiddenGround!=nullptr;
        if (actor.explicit_ground) {
            actor.ground_list_valid=false;
            for (std::size_t i=0; i<source->forbiddenGroundCount && i<actor.forbidden.size(); ++i) {
                actor.forbidden[actor.forbidden_count++]=source->forbiddenGround[i];
                if (source->forbiddenGround[i]==0) { actor.ground_list_valid=true; break; }
            }
        }
        IsleGameObjectMetadata metadata{};
        require(isle_game_object_metadata(player.game, object, &metadata)==ISLE_GAME_OK,
                "River Actor metadata observation rejected");
        actor.object_id=metadata.id;
        const float horizontal=std::hypot(actor.collision.x-player.CameraEyePos.x,
                                          actor.collision.z-player.CameraEyePos.z);
        const float pitch=std::atan2(actor.collision.y-player.CameraEyePos.y, horizontal);
        actor.eligible=source->tangible && !(metadata.flags&1u) && (source->flags&4u) &&
            !(source->flags&8u) && source->playerDist>=0 && source->playerDist<=10000 &&
            pitch>=player.MinPitch && pitch<=player.MaxPitch;
        actor.focused=actor.eligible && focus.present && focus.object==object &&
            out.focus.pokemon_id==source->id && (out.focus.flags&4u) && !(out.focus.flags&8u);
        if (source->id==80) {
            if (out.child_count==out.children.size()) out.capacity_ok=false;
            else out.children[out.child_count++]=actor;
            const int taken=player.photo->PokemonDetector_TakenPhotoIndex;
            if (actor.identity==child && actor.eligible && taken>=0 && taken<2) {
                const auto& context=player.photo->PokemonDetector_Photo[taken];
                if (context.pokemonInFocus==object) for (std::size_t i=0; i<12; ++i) {
                    if (context.pokemonObjects[i]!=object || context.main.pokemons[i].pokemonID!=80) continue;
                    const int count=player.photo->gPhotoCount;
                    if (count>0 && count<=60)
                        out.captured_child_context=same_subject(context.main.pokemons[i],
                            player.photo->D_800B0598[count-1].pokemons[0]);
                }
            }
        } else {
            if (actor.identity!=selected && actor.evolving) {
                out.multiple_other_evolving |= out.other_evolving;
                out.other_evolving=true; out.other_global=actor.global_root;
            }
            bool already_attempted=false;
            for (const auto& prior:attempted) already_attempted |= prior==actor.identity;
            const bool candidate=!selected[0] && !already_attempted && actor.eligible &&
                actor.ready && actor.endpoint_ready;
            if (actor.identity==selected || (candidate && (!out.parent.present ||
                actor.endpoint_distance<out.parent.endpoint_distance))) out.parent=actor;
        }
    }
    for (const auto& binding:items.render_bindings()) {
        if (!binding.model.object) continue;
        const auto instance=items.instance(binding.model.object);
        require(bool(instance), "River Item incarnation unavailable");
        const auto family=items.family(instance);
        if (!family || (*family!=items::Family::apple && *family!=items::Family::pester_ball)) continue;
        const auto* source=items.source(binding.model.object);
        require(source, "River logical Item source unavailable");
        if (out.item_count==out.items.size()) { out.capacity_ok=false; break; }
        const IsleGameModelNode* nodes=nullptr;
        std::size_t node_count=0;
        require(isle_game_model_peek_nodes(player.game, binding.model.model, &nodes, &node_count)==ISLE_GAME_OK &&
                nodes && node_count, "River Item root-node observation rejected");
        IsleGameObjectMetadata metadata{};
        require(isle_game_object_metadata(player.game, binding.model.object, &metadata)==ISLE_GAME_OK,
                "River Item metadata observation rejected");
        out.items[out.item_count++]={key(instance), source->itemID, source->state, source->flags,
                                    metadata.flags, metadata.id, nodes[0].transform.position};
    }
}
// Exact Source items.c splash/delete surfaces, separate from walking rules.
bool apple_surface(std::int32_t surface) {
    switch (surface) {
    case 0x00ff00: case 0xff4c19: case 0x0019ff: case 0x007f66:
    case 0x337fb2: case 0x4ccccc: case 0xff0000: return false;
    default: return surface>=0;
    }
}
float restitution(std::int32_t surface) {
    switch (surface) {
    case 0x4c7f00: case 0x996666: case 0xff9919: case 0xb2997f: return .2f;
    case 0x331919: case 0x193333: case 0x4c1900: case 0x4c4c33: case 0x7f4c00:
    case 0x7f667f: case 0x7f7f7f: case 0xff7fb2: case 0x7f6633: return .3f;
    default: return 0;
    }
}
bool walk_surface(const Actor& actor, std::int32_t surface) {
    if (surface<0 || !actor.ground_list_valid) return false;
    if (actor.explicit_ground) {
        for (std::size_t i=0; i<actor.forbidden_count; ++i) {
            if (actor.forbidden[i]==0) return true;
            if (actor.forbidden[i]==static_cast<float>(static_cast<std::uint32_t>(surface))) return false;
        }
        return false;
    }
    switch (surface) {
    case 0x007f66: case 0x00ff00: case 0x337fb2: case 0x4ccccc: case 0x7f6633:
    case 0x7f667f: case 0xff0000: case 0xff4c19: case 0xff7fb2: return false;
    default: return true;
    }
}
bool safe_ground(const world::World& world, const IsleGameVec3& p, world::GroundResult& ground) {
    return finite(p) && std::abs(p.x)<9990 && std::abs(p.z)<9990 &&
        world.ground_at(p.x, p.z, ground) && std::isfinite(ground.height) &&
        std::abs(ground.height+12)<9990 && apple_surface(ground.surface) &&
        std::isfinite(ground.normal.y) && ground.normal.y>=.9f;
}
bool corridor(const world::World& world, const Actor& actor, const IsleGameVec3& target) {
    const float dx=target.x-actor.root.x, dz=target.z-actor.root.z;
    const float length=std::hypot(dx, dz);
    if (!(length>0 && length<=550 && actor.radius>=0 && std::isfinite(actor.radius))) return false;
    const float ux=dx/length, uz=dz/length;
    // Source speed100 integrates3.3/update. Test every smaller root step,
    // radius-ahead probe, and a half-radius lateral guard for turning error.
    const unsigned steps=static_cast<unsigned>(std::ceil(length/3.0f));
    for (unsigned i=0; i<=steps; ++i) {
        const float along=length*static_cast<float>(i)/static_cast<float>(steps);
        for (int ahead=0; ahead<2; ++ahead) for (int side=-1; side<=1; ++side) {
            const float at=along+(ahead ? actor.radius : 0);
            const float lateral=static_cast<float>(side)*actor.radius*.5f;
            world::GroundResult ground{};
            const float x=actor.root.x+ux*at-uz*lateral;
            const float z=actor.root.z+uz*at+ux*lateral;
            if (!world.ground_at(x, z, ground) || !walk_surface(actor, ground.surface) ||
                !std::isfinite(ground.height)) return false;
        }
    }
    return true;
}
bool segment_hits(const Sphere& sphere, const IsleGameVec3& p, const IsleGameVec3& v) {
    const float length=v.x*v.x+v.y*v.y+v.z*v.z;
    if (!(length>0)) return false;
    const float projection=((sphere.center.x-p.x)*v.x+(sphere.center.y-p.y)*v.y+
                            (sphere.center.z-p.z)*v.z)/length;
    const float t=std::clamp(projection, 0.0f, 1.0f);
    return distance_squared({p.x+t*v.x, p.y+t*v.y, p.z+t*v.z}, sphere.center)<=sphere.radius*sphere.radius;
}
// Bounded Source ground-bounce/settling forecast. Splash, ceilings, walls and
// Actor impacts remain rejected; only actual native flight establishes landing.
bool landing(const world::World& world, const IslePlayerSource& player, const Planning& observed,
             const IsleGameVec3& ray, IsleGameVec3& result, int& surface) {
    if (!observed.capacity_ok || !finite(ray)) return false;
    const IsleGameCameraParameters* camera=nullptr;
    require(isle_game_camera_parameters(player.game, player.camera, &camera)==ISLE_GAME_OK && camera,
            "River launch camera observation rejected");
    IsleGameVec3 p{camera->look_at.eye[0], camera->look_at.eye[1], camera->look_at.eye[2]};
    IsleGameVec3 v{ray.x-p.x, ray.y-p.y+100, ray.z-p.z};
    const float norm=std::hypot(v.x, v.y, v.z);
    if (!(norm>0 && std::isfinite(norm))) return false;
    v={50*v.x/norm+player.PlayerVelocity.x, 50*v.y/norm+player.PlayerVelocity.y,
       50*v.z/norm+player.PlayerVelocity.z};
    IsleGameVec3 prior=p;
    for (unsigned step=0; step<400; ++step) {
        bool ground_bounce=false;
        world::GroundResult ground{}, ceiling{};
        if (!finite(p) || std::abs(p.x)>=9990 || std::abs(p.y)>=9990 || std::abs(p.z)>=9990 ||
            !world.ground_at(p.x, p.z, ground) ||
            (world.ceiling_at(p.x, p.z, ceiling) && p.y>ceiling.height)) return false;
        if (p.y<=ground.height+.375f) {
            if (p.y<ground.height-.375f) {
                IsleGameVec3 high=prior, low=p;
                for (unsigned i=0; i<16; ++i) {
                    p={(high.x+low.x)*.5f, (high.y+low.y)*.5f, (high.z+low.z)*.5f};
                    if (!world.ground_at(p.x, p.z, ground)) return false;
                    if (std::abs(p.y-ground.height)<=.375f) break;
                    if (p.y<ground.height) low=p; else high=p;
                }
            }
            if (!safe_ground(world, p, ground)) return false;
            const float coefficient=restitution(ground.surface);
            const float speed=std::hypot(v.x, v.y, v.z)*coefficient;
            p.y=ground.height+12;
            if (speed<13) { result=p; surface=ground.surface; return true; }
            const float dot=v.x*ground.normal.x+v.y*ground.normal.y+v.z*ground.normal.z;
            v={(v.x-2*dot*ground.normal.x)*coefficient,
               (v.y-2*dot*ground.normal.y)*coefficient,
               (v.z-2*dot*ground.normal.z)*coefficient};
            ground_bounce=true;
        }
        const IsleGameVec3 segment{v.x*.5f, v.y*.5f, v.z*.5f};
        world::Vec3 hit{}, reflected{};
        if (world.collide({p.x,p.y,p.z}, {segment.x,segment.y,segment.z}, hit, reflected)>=0) return false;
        for (std::size_t i=0; i<observed.sphere_count; ++i)
            if (segment_hits(observed.spheres[i], p, segment)) return false;
        prior=p;
        if (!ground_bounce) v.y-=.6f;
        p.x+=v.x*.5f; p.y+=v.y*.5f; p.z+=v.z*.5f;
    }
    return false;
}
bool progress_candidate(const Actor& actor, const IsleGameVec3& point) {
    const float old_distance=std::sqrt(actor.endpoint_distance);
    const float remaining=std::sqrt(endpoint_squared(point, actor.endpoint));
    return distance_squared(actor.root, point)<525*525 && remaining+110<old_distance;
}
bool closest_candidate(const Planning& observed, const IsleGameVec3& point) {
    const float distance=std::sqrt(distance_squared(observed.parent.root, point));
    for (std::size_t i=0; i<observed.item_count; ++i) {
        const auto& item=observed.items[i];
        if (!(item.flags&ISLE_ITEM_DELETED) &&
            std::sqrt(distance_squared(observed.parent.root, item.position))<=distance+25) return false;
    }
    return true;
}
bool plan(const world::World& world, const IslePlayerSource& player, const Planning& observed,
          RiverPhotoFacts& facts, IsleGameVec3& aimed) {
    const auto& actor=observed.parent;
    const float distance=std::sqrt(actor.endpoint_distance);
    if (!(distance>0)) return false;
    const float ux=(actor.endpoint.x-actor.root.x)/distance;
    const float uz=(actor.endpoint.z-actor.root.z)/distance;
    bool found=false;
    float best=0;
    for (const float forward:{500.0f, 375.0f, 250.0f}) for (const float lateral:{0.0f, -70.0f, 70.0f}) {
        ++facts.candidates;
        IsleGameVec3 point{actor.root.x+ux*forward-uz*lateral, 0,
                           actor.root.z+uz*forward+ux*lateral};
        world::GroundResult ground{};
        if (!safe_ground(world, point, ground) || !walk_surface(actor, ground.surface)) {
            ++facts.ground_rejections; continue;
        }
        point.y=ground.height+12;
        if (!progress_candidate(actor, point) || !closest_candidate(observed, point)) {
            ++facts.corridor_rejections; continue;
        }
        IsleGameVec3 ray{}, impact{}; int surface=-1;
        if (!projectile_target(player, {point.x, ground.height, point.z}, ray)) {
            ++facts.arc_rejections; continue;
        }
        const float pitch=std::atan2(ray.y-player.CameraEyePos.y,
            std::hypot(ray.x-player.CameraEyePos.x, ray.z-player.CameraEyePos.z));
        if (!(pitch>=player.MinPitch && pitch<=player.MaxPitch)) { ++facts.arc_rejections; continue; }
        if (!corridor(world, actor, point)) { ++facts.corridor_rejections; continue; }
        if (!landing(world, player, observed, ray, impact, surface) ||
            !progress_candidate(actor, impact) || !closest_candidate(observed, impact) ||
            !corridor(world, actor, impact)) { ++facts.arc_rejections; continue; }
        const float remaining=endpoint_squared(impact, actor.endpoint);
        if (!found || remaining<best) {
            found=true; best=remaining; aimed=ray;
            facts.candidate=point; facts.predicted_landing=impact; facts.predicted_surface=surface;
        }
    }
    return found;
}
bool contains(const std::array<RiverIncarnation, 20>& keys, std::size_t count, const RiverIncarnation& identity) {
    for (std::size_t i=0; i<count; ++i) if (keys[i]==identity) return true;
    return false;
}
} // namespace

RiverPhotoPuzzle::RiverPhotoPuzzle(private_post_course::Controller controller, Aim aim)
    : controller_(controller), aim_(aim) {
    require(controller_.set && aim_.point, "River photos require real pad and pure aim bridges");
}
void RiverPhotoPuzzle::publish(std::uint16_t buttons, std::int8_t x, std::int8_t y) noexcept {
    controller_.set(controller_.owner, buttons, x, y);
    facts_.owns_pad=true;
}
void RiverPhotoPuzzle::stop(RiverPhotoDisposition disposition) noexcept {
    if (facts_.exhausted) return;
    facts_.exhausted=true; facts_.disposition=disposition; facts_.phase=RiverPhotoPhase::finished;
    capture_armed_=false;
}
void RiverPhotoPuzzle::abandon_parent(RiverPhotoDisposition disposition) noexcept {
    facts_.parent={}; capture_armed_=false;
    attempt_approach_=false; previous_parent_evolving_=false;
    facts_.approach_seen=false; facts_.endpoint_progress=false;
    facts_.command28_seen=false; facts_.evolution_seen=false;
    actor_steering_=0; actor_recovery_=0; actor_planning_=0; zoom_out_requested_=false;
    facts_.disposition=disposition; facts_.phase=RiverPhotoPhase::observe;
}
void RiverPhotoPuzzle::observe(const application::RuntimeView& view, std::uint64_t updates) {
    if (sampled_ && updates==previous_update_) return;
    require(!sampled_ || updates>previous_update_, "River completed update ordinal regressed");
    const bool consecutive=sampled_ && updates==previous_update_+1;
    previous_update_=updates; sampled_=true; facts_.last_update=updates; facts_.owns_pad=false;
    observation_active_=false;
    if (!view.river || view.river->status()!=course::LifecycleStatus::running) {
        if (started_ && !facts_.complete) stop(RiverPhotoDisposition::course_ended);
        return;
    }
    const auto& entry=*view.river;
    const auto* host=entry.native_host();
    const auto* player=entry.adapter().native.player();
    if (!host || !host->execution_open() || !player) return;
    require(player->status==ISLE_GAME_OK && player->photo && player->photo->status==ISLE_GAME_OK,
            "River actual Player/Photo failed");
    auto* actors=host->actors(); auto* items=host->items_owner(); auto* world=host->world_owner();
    require(actors && items && world, "River actual Actor/Item/World unavailable");
    const auto* block=world->current();
    // Adapter::requested witnesses initial scene setup, not course retirement.
    if (!block || !block->resource || world->retired()) {
        if (started_ && !facts_.complete) stop(RiverPhotoDisposition::course_ended);
        return;
    }
    facts_.current_block=static_cast<int>(block->index);
    const auto offset=block->resource->metadata.world_position;
    const IsleGameVec3 origin{offset.x*100, offset.y*100, offset.z*100};
    snapshot(planning_, *player, *actors, *items, facts_.parent, facts_.child, attempted_parents_, origin);
    const auto& observed=planning_;
    observation_active_=true;
    const auto& parent=observed.parent;
    const auto& photo=*player->photo;
    facts_.film=photo.gPhotoCount; facts_.qualified=photo.D_800AE27C;
    require(facts_.film>=0 && facts_.film<=60 && facts_.qualified>=0 && facts_.qualified<=facts_.film,
            "River actual roll counters invalid");
    facts_.focus_species=observed.focus.pokemon_id; facts_.focus_flags=observed.focus.flags;
    facts_.focus_context=observed.focus.context; facts_.taken_context=photo.PokemonDetector_TakenPhotoIndex;
    facts_.player_ready=player->D_80382D04_523114==1 && !player->IsInputDisabled && !player->IsPaused;
    require(entry.game_state().save(), "River canonical Save unavailable");
    const auto apple=entry.game_state().save()->player_flag(0);
    require(bool(apple), "River actual Apple flag read failed");
    facts_.apple_available=apple.value; facts_.available=apple.value;
    if (!started_ && facts_.player_ready && apple.value) {
        started_=true; facts_.initial_update=updates;
    }
    if (!reports_sampled_) {
        const auto report=entry.photos().report_present(79);
        require(bool(report), "River canonical Slowpoke Report read unavailable");
        facts_.prior_slowpoke_report=report.value; reports_sampled_=true;
    }
    if (observed_roll_<0) { observed_roll_=facts_.film; observed_qualified_=facts_.qualified; }
    require(facts_.film>=observed_roll_ && facts_.qualified>=observed_qualified_, "River roll counters regressed");
    const bool one_exposure=facts_.film==observed_roll_+1 && facts_.qualified==observed_qualified_+1;
    for (int slot=observed_roll_; slot<facts_.film; ++slot) {
        const auto& captured=photo.D_800B0598[slot];
        facts_.capture_observed=true; facts_.raw_primary=captured.pokemons[0].pokemonID;
        if (one_exposure && captured.levelID==3 && (captured.unk_00_16&0xe0u)==0x20u &&
            captured.pokemons[0].pokemonID==79) facts_.slowpoke_captured=true;
        if (captured.pokemons[0].pokemonID!=80) continue;
        ++facts_.captures;
        const bool coherent=!facts_.exhausted && capture_armed_ && consecutive &&
            armed_child_==facts_.child && armed_slot_==slot && armed_qualified_+1==facts_.qualified &&
            facts_.spawn_seen && facts_.parent_cleanup_seen && one_exposure &&
            (player->input.pressed_buttons&a_button) && (player->input.current_buttons&a_button) &&
            facts_.taken_context>=0 && facts_.taken_context<2 && observed.captured_child_context &&
            captured.levelID==3 && (captured.unk_00_16&0xe0u)==0x20u;
        if (coherent) {
            facts_.complete=true; facts_.slowbro_slot=slot; facts_.capture_update=updates;
            facts_.captured_slots|=std::uint64_t{1}<<static_cast<unsigned>(slot);
            facts_.disposition=RiverPhotoDisposition::qualified; facts_.phase=RiverPhotoPhase::finished;
        } else { ++facts_.capture_mismatches; ++facts_.mismatches; }
    }
    observed_roll_=facts_.film; observed_qualified_=facts_.qualified; capture_armed_=false;
    facts_.target_present=parent.present;
    if (parent.present) {
        facts_.root=parent.root; facts_.global_root=parent.global_root; facts_.endpoint=parent.endpoint;
        facts_.collision=parent.collision; facts_.collision_radius=parent.radius;
        facts_.target_graph=parent.graph; facts_.target_state=parent.state; facts_.target_flags=parent.flags;
        facts_.target_behavior=parent.behavior; facts_.target_object_id=parent.object_id;
        facts_.process_flags=parent.process_flags; facts_.counter=parent.counter;
        facts_.model_animation=parent.model_animation; facts_.animation_time=parent.animation_time;
        facts_.target_yaw=parent.yaw; facts_.endpoint_ready=parent.endpoint_ready;
        facts_.root_endpoint_squared=parent.endpoint_distance;
        if (facts_.parent[0] && parent.endpoint_distance<facts_.best_endpoint_squared)
            facts_.best_endpoint_squared=parent.endpoint_distance;
        facts_.endpoint_progress=facts_.parent[0] &&
            std::sqrt(facts_.best_endpoint_squared)+.5f<std::sqrt(facts_.initial_endpoint_squared);
    }
    // Attribute only an actual new child at the cleanup boundary of this exact
    // evolving parent, in global coordinates across current-block relocation.
    if (evolution_entered_ && !parent.present && !facts_.spawn_seen && !facts_.exhausted) {
        const Actor* child=nullptr;
        unsigned matches=0;
        for (std::size_t i=0; i<observed.child_count; ++i) {
            const auto& candidate=observed.children[i];
            bool existed=false;
            for (std::size_t j=0; j<previous_child_count_; ++j) existed |= candidate.identity==previous_children_[j];
            if (existed) continue;
            const float separation=distance_squared(candidate.global_root, previous_parent_global_);
            const float yaw_error=angle_error(candidate.yaw, previous_parent_yaw_+entry.actors_source().D_802E4B90_6CC970);
            facts_.spawn_distance_squared=separation; facts_.spawn_yaw_error=yaw_error;
            if (separation<=16 && std::isfinite(yaw_error) && std::abs(yaw_error)<=.01f) { child=&candidate; ++matches; }
        }
        const bool other_close=previous_other_ambiguous_ || (previous_other_evolving_ && child &&
            distance_squared(child->global_root, previous_other_global_)<=64);
        if (consecutive && previous_parent_evolving_ && matches==1 && !other_close && observed.capacity_ok) {
            facts_.parent_cleanup_seen=true; facts_.spawn_seen=true; facts_.spawn_update=updates;
            facts_.child=child->identity; facts_.child_object_id=child->object_id;
            ++facts_.spawns; facts_.phase=RiverPhotoPhase::capture;
        } else { ++facts_.mismatches; stop(RiverPhotoDisposition::spawn_unattributed); }
    }
    facts_.child_present=false; facts_.child_eligible=false; facts_.child_focused=false;
    for (std::size_t i=0; i<observed.child_count; ++i) {
        const auto& child=observed.children[i];
        if (child.identity!=facts_.child) continue;
        facts_.child_present=true; facts_.child_eligible=child.eligible; facts_.child_focused=child.focused;
        facts_.child_root=child.root; facts_.child_yaw=child.yaw; facts_.child_flags=child.flags;
        if (child.focused) ++facts_.capture_focus_updates;
        if (!facts_.exhausted && facts_.spawn_seen && child.focused &&
            facts_.player_ready && player->gDirectionIndex==-1 && !player->TargetDirectionZoomedIn &&
            player->sTimerAfterPhotoTaken<0 && !(player->input.current_buttons&a_button) && facts_.film<60) {
            capture_armed_=true; armed_child_=child.identity;
            armed_slot_=facts_.film; armed_qualified_=facts_.qualified;
        }
    }
    previous_child_count_=observed.child_count;
    for (std::size_t i=0; i<observed.child_count; ++i) previous_children_[i]=observed.children[i].identity;
    previous_parent_evolving_=parent.present && parent.evolving;
    if (parent.present) { previous_parent_global_=parent.global_root; previous_parent_yaw_=parent.yaw; }
    previous_other_evolving_=observed.other_evolving || observed.multiple_other_evolving;
    previous_other_ambiguous_=observed.multiple_other_evolving;
    previous_other_global_=observed.other_global;
    if (!observed.capacity_ok && !facts_.complete) stop(RiverPhotoDisposition::unsafe_geometry);
    if (started_ && updates-facts_.initial_update>=overall_cap-1 && !facts_.complete)
        stop(RiverPhotoDisposition::overall_limit);
    // An acknowledged Apple press is never equated with a landing or approach.
    if (pending_press_ && (player->input.current_buttons&a_button)) {
        if (player->input.pressed_buttons&a_button) ++facts_.pressed;
        if ((player->input.pressed_buttons&a_button) && player->ThrowItemTimeout>0 && player->LastItemId==163) {
            ++facts_.consumed; facts_.consumed_update=updates;
            tracking_item_=true; facts_.item={}; attempt_start_=updates; attempt_approach_=false;
        } else { ++facts_.mismatches; stop(RiverPhotoDisposition::press_timeout); }
        pending_press_=false; pending_release_=true; release_start_=updates;
    }
    if (pending_press_ && updates-facts_.request_update>=120) {
        pending_press_=false; pending_release_=true; release_start_=updates;
        stop(RiverPhotoDisposition::press_timeout);
    }
    if (pending_release_ && !(player->input.current_buttons&a_button)) {
        pending_release_=false; ++facts_.releases; facts_.release_update=updates;
    }
    if (pending_release_ && updates-release_start_>=120) {
        pending_release_=false; stop(RiverPhotoDisposition::release_timeout);
    }
    if (facts_.complete || facts_.exhausted) return;
    if (facts_.spawn_seen) {
        facts_.phase=RiverPhotoPhase::capture; facts_.disposition=RiverPhotoDisposition::capture_pending;
        return;
    }
    if (tracking_item_) {
        const Item* item=nullptr; unsigned matches=0;
        for (std::size_t i=0; i<observed.item_count; ++i) {
            const auto& candidate=observed.items[i];
            if (facts_.item[0] ? candidate.identity!=facts_.item :
                (candidate.kind!=ISLE_ITEM_APPLE || contains(prethrow_items_, prethrow_count_, candidate.identity))) continue;
            item=&candidate; ++matches;
        }
        facts_.item_present=matches==1;
        if (matches>1) { ++facts_.ambiguous_items; stop(RiverPhotoDisposition::item_ambiguous); return; }
        if (item) {
            facts_.item=item->identity; facts_.item_position=item->position; facts_.item_state=item->state;
            facts_.item_flags=item->flags; facts_.item_kind=item->kind; facts_.item_object_id=item->object_id;
            facts_.item_still=item->state==ISLE_ITEM_STILL;
            world::GroundResult ground{};
            facts_.item_safe=!(item->object_flags&1u) && !(item->flags&ISLE_ITEM_DELETED) &&
                safe_ground(*world, item->position, ground) &&
                (!facts_.item_still || ((item->flags&ISLE_ITEM_TOUCHED_GROUND) &&
                    std::abs(item->position.y-ground.height-12)<=1));
            facts_.ground_surface=ground.surface;
            if (parent.present) {
                facts_.item_root_distance=std::sqrt(distance_squared(item->position, parent.root));
                facts_.different_apple=prethrow_actor_apple_!=item->identity;
                facts_.apple_reference_matches=parent.apple==item->identity;
                facts_.interaction_reference_matches=parent.interaction==item->identity;
                const Item* nearest=nullptr; float best=0;
                for (std::size_t i=0; i<observed.item_count; ++i) {
                    const auto& candidate=observed.items[i];
                    if (candidate.flags&ISLE_ITEM_DELETED) continue;
                    const float d=distance_squared(candidate.position, parent.root);
                    if (!nearest || d<best) { nearest=&candidate; best=d; }
                }
                facts_.item_closest=nearest && nearest->identity==item->identity;
                if (parent.approaching && facts_.apple_reference_matches && facts_.interaction_reference_matches &&
                    facts_.different_apple && facts_.item_kind==ISLE_ITEM_APPLE && facts_.item_root_distance<600 &&
                    facts_.item_safe && (attempt_approach_ || (facts_.item_still && facts_.item_closest))) {
                    if (!attempt_approach_) { facts_.approach_update=updates; facts_.approach_seen=true; }
                    attempt_approach_=true; ++facts_.approach_updates;
                }
            }
            if (item->state==ISLE_ITEM_FLYING) { ++facts_.flight_updates; facts_.phase=RiverPhotoPhase::flight; }
            else facts_.phase=RiverPhotoPhase::approach;
            if ((item->flags&ISLE_ITEM_TOUCHED_GROUND) && !facts_.item_safe && !parent.evolving) {
                ++facts_.blocked_items; stop(RiverPhotoDisposition::item_unsafe); return;
            }
        } else {
            ++facts_.lost_items;
            if (!attempt_approach_ && !parent.evolving) { stop(RiverPhotoDisposition::item_lost); return; }
            // Disappearance is not consumption/evolution evidence. Wait for the
            // actual state or return to an idle graph before another throw.
            if (parent.present && parent.ready && !parent.approaching && !parent.evolving) tracking_item_=false;
        }
        if (updates-attempt_start_>=observation_cap-1 && !parent.evolving) {
            stop(RiverPhotoDisposition::approach_timeout); return;
        }
    }
    if (parent.present && parent.evolving && !evolution_entered_) {
        if (!facts_.consumed || !attempt_approach_ || !facts_.endpoint_progress ||
            !(parent.endpoint_distance<trigger_squared)) {
            stop(RiverPhotoDisposition::evolution_unattributed); return;
        }
        evolution_entered_=true; facts_.command28_seen=true; facts_.evolution_seen=true;
        facts_.evolution_update=updates; tracking_item_=false;
    }
    if (evolution_entered_) {
        facts_.phase=RiverPhotoPhase::evolution; ++facts_.evolution_updates;
        if (updates-facts_.evolution_update>=evolution_cap-1) stop(RiverPhotoDisposition::evolution_timeout);
        return;
    }
}

bool RiverPhotoPuzzle::update(const application::RuntimeView& view, std::uint64_t updates) {
    observe(view, updates);
    if (control_sampled_ && control_update_==updates) return facts_.owns_pad;
    control_sampled_=true; control_update_=updates; facts_.owns_pad=false;
    if (!observation_active_ || !view.river) return false;
    const auto* host=view.river->native_host();
    const auto* player=view.river->adapter().native.player();
    const auto* world=host ? host->world_owner() : nullptr;
    if (!player || !world || world->retired()) return false;
    const auto& observed=planning_;
    const auto& parent=observed.parent;
    // Only this control entry may publish input. Observation has already
    // accounted for actual consumption/release, including ordinary shutters.
    if (pending_press_ || pending_release_) {
        facts_.phase=pending_press_ ? RiverPhotoPhase::press : RiverPhotoPhase::release;
        publish(pending_press_ && !facts_.exhausted ? a_button : 0); return true;
    }
    if (facts_.complete || facts_.exhausted) return false;
    if (!facts_.apple_available) { facts_.disposition=RiverPhotoDisposition::apple_unavailable; return false; }
    if (!facts_.player_ready) { facts_.disposition=RiverPhotoDisposition::player_not_ready; return false; }
    if (facts_.spawn_seen || evolution_entered_) return false;
    if ((player->input.current_buttons&a_button) || player->sTimerAfterPhotoTaken>=0) return false;
    if (tracking_item_) { facts_.disposition=RiverPhotoDisposition::item_outstanding; return false; }
    if (facts_.parent[0] && !parent.present) {
        abandon_parent(RiverPhotoDisposition::actor_lost); return false;
    }
    if (!facts_.prior_slowpoke_report && !facts_.slowpoke_captured) {
        facts_.disposition=RiverPhotoDisposition::preserve_slowpoke; return false;
    }
    if (facts_.film>=59) { facts_.disposition=RiverPhotoDisposition::film_reserved; return false; }
    if (!parent.present) { facts_.disposition=RiverPhotoDisposition::target_absent; return false; }
    if (!parent.endpoint_ready) { facts_.disposition=RiverPhotoDisposition::endpoint_not_ready; return false; }
    if (!parent.ready || parent.approaching || parent.evolving || parent.endpoint_distance<trigger_squared) {
        facts_.disposition=RiverPhotoDisposition::target_not_ready; return false;
    }
    IsleGameVec3 aimed{};
    bool planned=false;
    if (!facts_.parent[0]) {
        if (facts_.incarnation_count>=attempted_parents_.size()) {
            stop(RiverPhotoDisposition::incarnation_limit); return false;
        }
        // A live distant actor is not yet a physical control opportunity.
        // Wait without owning pad or consuming an incarnation until a real
        // bounded trajectory and safe walking corridor are feasible.
        planned=plan(*world, *player, observed, facts_, aimed);
        if (!planned) { facts_.disposition=RiverPhotoDisposition::unsafe_geometry; return false; }
        facts_.parent=parent.identity;
        attempted_parents_[static_cast<std::size_t>(facts_.incarnation_count++)]=parent.identity;
        facts_.initial_endpoint_squared=parent.endpoint_distance; facts_.best_endpoint_squared=parent.endpoint_distance;
        actor_steering_=0; actor_recovery_=0; actor_planning_=0;
    }
    if (facts_.requests>=6) { stop(RiverPhotoDisposition::request_limit); return false; }
    if (!player->ui || player->ui->Icons_NumItemsAvailable<1) {
        facts_.disposition=RiverPhotoDisposition::item_ui_not_ready; return false;
    }
    // Existing unrelated Items are considered by the actual closest-item
    // candidate gate, rather than globally excluded from the course.
    if (player->ThrowItemTimeout!=0) {
        facts_.disposition=RiverPhotoDisposition::item_outstanding; return false;
    }
    if (player->gDirectionIndex<0 || player->TargetDirectionZoomedIn || player->TurnToDirSpeed!=0) {
        if (actor_recovery_>=128) { abandon_parent(RiverPhotoDisposition::recovery_limit); return false; }
        ++actor_recovery_; ++facts_.recovery_updates;
        facts_.phase=RiverPhotoPhase::unzoom; facts_.disposition=RiverPhotoDisposition::none;
        // Hold mode exits on release. Switch mode needs one fresh Z edge,
        // followed by release through the actual zoom-out transition.
        std::uint16_t recovery=0;
        if (player->ZoomSwitchMode==1 && player->gDirectionIndex==-1 &&
            !player->TargetDirectionZoomedIn && !zoom_out_requested_ &&
            !(player->input.current_buttons&z_button)) {
            recovery=z_button; zoom_out_requested_=true;
        }
        publish(recovery); return true;
    }
    zoom_out_requested_=false;
    if (actor_steering_+actor_planning_>=360) { abandon_parent(RiverPhotoDisposition::steering_limit); return false; }
    if (!planned && !plan(*world, *player, observed, facts_, aimed)) {
        ++actor_planning_; ++facts_.planning_updates;
        facts_.phase=RiverPhotoPhase::observe; facts_.disposition=RiverPhotoDisposition::unsafe_geometry;
        // Keep the finite planning window unzoomed; returning it to ordinary
        // photography here would immediately re-enter zoom and spend recovery.
        publish(); return true;
    }
    std::int8_t x=0, y=0;
    aim_.point(aim_.owner, *player, aimed, false, x, y);
    facts_.input_target=aimed; facts_.phase=RiverPhotoPhase::aim;
    facts_.disposition=RiverPhotoDisposition::none;
    ++actor_steering_; ++facts_.steering_updates;
    std::uint16_t buttons=0;
    if (!x && !y && !(player->input.current_buttons&a_button) && player->ThrowItemTimeout==0) {
        IsleGameVec3 impact{}; int surface=-1;
        if (landing(*world, *player, observed, player->cam_target, impact, surface) &&
            distance_squared(impact, facts_.predicted_landing)<=50*50 && progress_candidate(parent, impact) &&
            closest_candidate(observed, impact) && corridor(*world, parent, impact)) {
            prethrow_count_=0; prethrow_actor_apple_=parent.apple;
            for (std::size_t i=0; i<observed.item_count; ++i)
                prethrow_items_[prethrow_count_++]=observed.items[i].identity;
            facts_.predicted_landing=impact; facts_.predicted_surface=surface;
            facts_.request_update=updates; ++facts_.requests; pending_press_=true;
            buttons=a_button; facts_.phase=RiverPhotoPhase::press;
        } else ++facts_.actual_ray_rejections;
    }
    publish(buttons, x, y); return true;
}
} // namespace isle::private_campaign

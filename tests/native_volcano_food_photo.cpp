#include "native_volcano_food_photo.hpp"
#include "application_runtime.hpp"
#include "application_photos.hpp"
#include "game_state.hpp"
#include "volcano_scene.hpp"
#include "course_native_host.hpp"
#include "volcano_actor_source.h"
#include "actor_owner_source.h"
#include "actor_owner.hpp"
#include "item_owner.hpp"
#include "player_source.h"
#include "player_ui_source.h"
#include "world_runtime.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace isle::private_campaign {
namespace {
constexpr std::uint16_t a = 0x8000, z = 0x2000;
// Conservative acquisition range, not a replacement for Original raster scoring.
constexpr float portrait_range_squared = 1000.0f*1000.0f;
using Key = std::array<std::uint64_t, 3>;
using Phase = VolcanoFoodPhase;
using End = VolcanoFoodEnd;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
Key key(menu::animation::InstanceHandle value) { return {value.pool, value.slot, value.incarnation}; }
float squared(const IsleGameVec3& p, const IsleGameVec3& q) {
    const float x=p.x-q.x, y=p.y-q.y, dz=p.z-q.z; return x*x+y*y+dz*dz;
}
float wrapped(float value) { return std::remainder(value, 6.28318530717958647692f); }
bool finite(const IsleGameVec3& p) { return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z); }
IsleGameVec3 translated(const IsleGameVec3& p, const IsleGameVec3& origin, float sign) {
    return {p.x+sign*origin.x, p.y+sign*origin.y, p.z+sign*origin.z};
}
bool accepts(const IsleActorSource& actor, int command) {
    if (!actor.transitionGraph) return false;
    for (std::size_t i=0; i<actor.transitionGraph->count; ++i) {
        const auto& row=actor.transitionGraph->rows[i];
        if (row.cmd==58) break;
        if (row.cmd==command && (row.state || row.aux)) return true;
    }
    return false;
}
std::uint32_t callback(IsleGameSourceCallback fn) {
    if (fn==charmander_Idle) return 0x802d8ca4;
    if (fn==charmander_Walk) return 0x802d8d14;
    if (fn==charmander_Run) return 0x802d8e14;
    if (fn==charmander_AppleNearby) return 0x802d93dc;
    if (fn==charmander_RunToApple) return 0x802d9464;
    if (fn==charmander_WithApple) return 0x802d94e8;
    if (fn==charmander_MoveTurnToTarget) return 0x802d95ac;
    return 0;
}
struct Sphere { IsleGameVec3 center{}; float radius=0; bool primary=false; };
struct Observation {
    bool present=false, eligible=false, ready=false, focused=false, capacity=true, forbidden_valid=true;
    Key actor{}, apple{}, interaction{}, only_item{};
    std::uint32_t graph=0, state=0, path=0;
    unsigned valid_items=0;
    int context=-1, item_state=ISLE_ITEM_INVALID;
    float radius=0, yaw=0;
    IsleGameVec3 root{}, collision{}, origin{};
    std::array<Sphere,128> spheres{};
    std::size_t sphere_count=0;
    std::array<float,16> forbidden{};
    std::size_t forbidden_count=0;
};
Observation observe(const IslePlayerSource& player, const actor::Owner& actors,
                    const items::Owner& items, const world::World& world, const Key& selected) {
    Observation out;
    const auto world_position=world.current()->resource->metadata.world_position;
    out.origin={world_position.x*100,world_position.y*100,world_position.z*100};
    IslePlayerPhotoFocus focus{};
    require(isle_player_photo_published_focus(player.photo,&focus)==ISLE_GAME_OK,
            "Volcano food published focus unavailable");
    IsleGameObject* previous=nullptr;
    for (;;) {
        IsleGameObject* object=nullptr; IsleActorSource* source=nullptr; IsleGameModel* model=nullptr;
        require(isle_actor_source_link3_next(player.game,previous,&object,&source,&model)==ISLE_GAME_OK,
                "Volcano food native Actor traversal rejected");
        if (!object) break;
        previous=object;
        if (source->tangible && source->collisionRadius>0) {
            if (out.sphere_count==out.spheres.size()) out.capacity=false;
            else out.spheres[out.sphere_count++]={source->collPosition,source->collisionRadius,
                                                 source->id==4 && source->behavior==5};
        }
        if (source->id!=4 || source->behavior!=5 || !model) continue;
        const auto identity=key(actors.instance(object));
        if (selected[0] && identity!=selected) continue;
        require(!out.present && identity[0],"Volcano food duplicate or unowned Group5 Charmander");
        out.present=true; out.actor=identity; out.apple=key(items.instance(source->apple));
        out.interaction=key(items.instance(source->interactionTarget));
        out.graph=source->transitionGraph ? source->transitionGraph->source_identity : 0;
        out.radius=source->collisionRadius; out.collision=source->collPosition;
        IsleGameSourceCallback state=nullptr,path=nullptr;
        if (source->stateProc) require(isle_game_process_source_function(player.game,source->stateProc,&state)==ISLE_GAME_OK,
                                      "Volcano food state observation rejected");
        if (source->pathProc) require(isle_game_process_source_function(player.game,source->pathProc,&path)==ISLE_GAME_OK,
                                     "Volcano food path observation rejected");
        out.state=callback(state); out.path=callback(path);
        out.ready=accepts(*source,14) && (state==charmander_Idle || state==charmander_Walk || state==charmander_Run);
        const IsleGameModelPlacement* placement=nullptr;
        require(isle_game_model_placement(player.game,model,&placement)==ISLE_GAME_OK && placement,
                "Volcano food Actor placement unavailable");
        out.root=placement->transform.position; out.yaw=placement->transform.rotation.y;
        IsleGameObjectMetadata metadata{};
        require(isle_game_object_metadata(player.game,object,&metadata)==ISLE_GAME_OK,
                "Volcano food Actor visibility unavailable");
        out.eligible=placement->mode!=ISLE_GAME_MODEL_PLACEMENT_DISABLED && !(metadata.flags&1u) &&
            source->tangible && (source->flags&4u) && !(source->flags&8u) && source->playerDist>=0 && source->playerDist<=10000;
        out.focused=focus.present && focus.object==object && focus.pokemon_id==4 && (focus.flags&4u) && !(focus.flags&8u);
        out.context=focus.context;
        if (source->forbiddenGround) {
            out.forbidden_valid=false;
            for (std::size_t i=0; i<source->forbiddenGroundCount && i<out.forbidden.size(); ++i) {
                const float value=source->forbiddenGround[i]; out.forbidden[out.forbidden_count++]=value;
                if (value==0) { out.forbidden_valid=true; break; }
            }
        }
    }
    for (std::size_t slot=0; slot<items.animation().slot_count(); ++slot) {
        const auto instance=items.slot_instance(slot);
        if (!instance) continue;
        const auto family=items.family(instance);
        require(bool(family),"Volcano food Item family unavailable");
        if (*family!=items::Family::apple && *family!=items::Family::pester_ball) continue;
        const auto* source=items.source(instance);
        require(source,"Volcano food Item payload unavailable");
        if (source->state<=ISLE_ITEM_INVALID) continue;
        ++out.valid_items; out.only_item=*family==items::Family::apple ? key(instance) : Key{};
        out.item_state=source->state;
    }
    return out;
}
bool food_surface(int value) {
    switch (value) {
    case 0x00ff00: case 0xff4c19: case 0x0019ff: case 0x007f66:
    case 0x337fb2: case 0x4ccccc: case 0xff0000: return false;
    default: return value>=0;
    }
}
bool safe_ground(const world::GroundResult& result) {
    return std::isfinite(result.height) && std::abs(result.height)<9900 && food_surface(result.surface) &&
        std::isfinite(result.normal.x) && std::isfinite(result.normal.y) &&
        std::isfinite(result.normal.z) && result.normal.y>=.9f;
}
bool ground(const world::World& world, const IsleGameVec3& point, world::GroundResult& result) {
    return finite(point) && std::abs(point.x)<9900 && std::abs(point.z)<9900 &&
        world.ground_at(point.x,point.z,result) && safe_ground(result);
}
bool walkable(const Observation& observed, int surface) {
    if (!observed.forbidden_valid || !food_surface(surface)) return false;
    if (observed.forbidden_count) {
        for (std::size_t i=0;i<observed.forbidden_count;++i) {
            if (!observed.forbidden[i]) return true;
            if (observed.forbidden[i]==static_cast<float>(surface)) return false;
        }
        return false;
    }
    return surface!=0x7f6633 && surface!=0x7f667f && surface!=0xff7fb2;
}
bool static_clear(const world::World& world, const IsleGameVec3& from, const IsleGameVec3& to) {
    world::Vec3 hit{},reflected{};
    return world.collide({from.x,from.y,from.z},{to.x-from.x,to.y-from.y,to.z-from.z},hit,reflected)<0;
}
bool sphere_hit(const Sphere& sphere, const IsleGameVec3& from, const IsleGameVec3& to, float extra=0) {
    const IsleGameVec3 delta{to.x-from.x,to.y-from.y,to.z-from.z};
    const float length=squared(from,to);
    const float t=length>0 ? std::clamp(((sphere.center.x-from.x)*delta.x+(sphere.center.y-from.y)*delta.y+
                                        (sphere.center.z-from.z)*delta.z)/length,0.0f,1.0f) : 0;
    return squared({from.x+t*delta.x,from.y+t*delta.y,from.z+t*delta.z},sphere.center)<=
        (sphere.radius+12+extra)*(sphere.radius+12+extra);
}
bool corridor(const world::World& world, const Observation& observed, const IsleGameVec3& target,
              IsleGameVec3& stopped) {
    const float distance=std::hypot(target.x-observed.root.x,target.z-observed.root.z);
    if (!(distance>observed.radius+75 && distance<550)) return false;
    // Pure prospective geometry at Source's 80*.033 step: check the next root
    // AND the current-root forward-radius probe, then the 3D stopping rule.
    // Actual later RunToApple/WithApple, not this forecast, earns the receipt.
    constexpr float step=80.0f*.033f;
    IsleGameVec3 root=observed.root;
    for (unsigned i=0;i<224;++i) {
        float dx=target.x-root.x,dz=target.z-root.z;
        if (std::abs(dz)<.00001f) { dx=1; dz=0; } // Pinned GetHeadingVector.
        const float length=std::hypot(dx,dz);
        if (!(length>0 && std::isfinite(length))) return false;
        const float ux=dx/length,uz=dz/length;
        IsleGameVec3 next{root.x+ux*step,0,root.z+uz*step};
        const IsleGameVec3 ahead{root.x+ux*observed.radius,0,root.z+uz*observed.radius};
        world::GroundResult floor{},forward{};
        if (!ground(world,next,floor) || !walkable(observed,floor.surface) ||
            !ground(world,ahead,forward) || !walkable(observed,forward.surface)) return false;
        next.y=floor.height;
        if (std::abs(next.y-root.y)>16 ||
            !static_clear(world,{root.x,root.y+observed.radius+12,root.z},
                               {next.x,next.y+observed.radius+12,next.z})) return false;
        root=next;
        const float remaining=std::sqrt(squared(target,root));
        if (std::abs(remaining-40)<step || remaining<40) { stopped=root; return true; }
    }
    return false;
}
float damping(int surface) {
    // Pinned items.c404-474: unknown/default ordinary surfaces stop the Apple.
    switch (surface) {
    case 0x4c7f00: case 0x996666: case 0xff9919: case 0xb2997f: return .2f;
    case 0x331919: case 0x193333: case 0x4c1900: case 0x4c4c33: case 0x7f4c00:
    case 0x7f667f: case 0x7f7f7f: case 0xff7fb2: case 0x7f6633: return .3f;
    default: return 0;
    }
}
bool landing(const world::World& world, const IslePlayerSource& player, const Observation& observed,
             const IsleGameVec3& ray, const IsleGameVec3& target, IsleGameVec3& contact, int& surface, unsigned& rebounds) {
    const IsleGameCameraParameters* camera=nullptr;
    require(isle_game_camera_parameters(player.game,player.camera,&camera)==ISLE_GAME_OK && camera,
            "Volcano food actual launch camera unavailable");
    IsleGameVec3 position{camera->look_at.eye[0],camera->look_at.eye[1],camera->look_at.eye[2]};
    IsleGameVec3 velocity{ray.x-position.x,ray.y-position.y+100,ray.z-position.z};
    const float length=std::hypot(velocity.x,velocity.y,velocity.z);
    if (!(finite(position) && std::isfinite(length) && length>0 && observed.capacity)) return false;
    velocity={50*velocity.x/length+player.PlayerVelocity.x,50*velocity.y/length+player.PlayerVelocity.y,
              50*velocity.z/length+player.PlayerVelocity.z};
    IsleGameVec3 previous=position;
    rebounds=0;
    unsigned first_contact_step=0;
    for (unsigned step=0;step<160;++step) {
        world::GroundResult floor{},ceiling{};
        // Airborne travel may cross lava/water; only actual contact must be
        // ordinary safe food ground. Do not reject terrain merely below an arc.
        if (!world.ground_at(position.x,position.z,floor) || !std::isfinite(floor.height) ||
            (world.ceiling_at(position.x,position.z,ceiling) && position.y+12>=ceiling.height)) return false;
        if (position.y<=floor.height+.375f) {
            const IsleGameVec3 penetrated=position;
            if (position.y<floor.height-.375f) {
                // Pinned Items_BounceFromGround recovers the first contact,
                // including its material/normal, before deciding settlement.
                IsleGameVec3 above=previous,below=position;
                for (unsigned subdivision=0;subdivision<16;++subdivision) {
                    position={(above.x+below.x)*.5f,(above.y+below.y)*.5f,(above.z+below.z)*.5f};
                    if (!world.ground_at(position.x,position.z,floor) || !std::isfinite(floor.height)) return false;
                    if (std::abs(position.y-floor.height)<=.375f) break;
                    if (position.y<floor.height) below=position; else above=position;
                }
            }
            if (!safe_ground(floor)) return false;
            contact={position.x,floor.height+12,position.z}; surface=floor.surface;
            if (!rebounds && squared(contact,{target.x,target.y+12,target.z})>35*35) return false;
            const float restitution=damping(floor.surface);
            const float incoming_speed=std::hypot(velocity.x,velocity.y,velocity.z);
            const float speed=restitution*incoming_speed;
            if (speed>=13) {
                if (rebounds) return false; // At most one rebound; no horizon reset.
                const IsleGameVec3 unit{velocity.x/incoming_speed,velocity.y/incoming_speed,velocity.z/incoming_speed};
                const float dot=unit.x*floor.normal.x+unit.y*floor.normal.y+unit.z*floor.normal.z;
                velocity={speed*(unit.x-2*dot*floor.normal.x),
                          speed*(unit.y-2*dot*floor.normal.y),
                          speed*(unit.z-2*dot*floor.normal.z)};
                // Items_CollideWithGround advances this half-step immediately,
                // without gravity or another collision query in this update.
                previous=penetrated;
                position={contact.x+velocity.x*.5f,contact.y+velocity.y*.5f,contact.z+velocity.z*.5f};
                rebounds=1; first_contact_step=step;
                continue;
            }
            if (!(speed<13)) return false;
            if (!(squared(contact,observed.collision)<590*590)) return false;
            for (std::size_t i=0;i<observed.sphere_count;++i) {
                // Prospective horizontal-pursuit margin, not a guarantee
                // about future 3D Actor motion or Source update ordering.
                const float pursuit=observed.spheres[i].primary && rebounds
                    ? 80*.033f*float(step-first_contact_step) : 0;
                if (sphere_hit(observed.spheres[i],contact,contact,pursuit)) return false;
            }
            return true;
        }
        // Source queries static/Actor collisions BEFORE applying gravity.
        const IsleGameVec3 query{position.x+velocity.x*.5f,position.y+velocity.y*.5f,position.z+velocity.z*.5f};
        if (!static_clear(world,position,query)) return false;
        for (std::size_t i=0;i<observed.sphere_count;++i) {
            const float pursuit=observed.spheres[i].primary && rebounds
                ? 80*.033f*float(step-first_contact_step) : 0;
            if (sphere_hit(observed.spheres[i],position,query,pursuit)) return false;
        }
        previous=position; velocity.y-=.6f;
        position={position.x+velocity.x*.5f,position.y+velocity.y*.5f,position.z+velocity.z*.5f};
    }
    return false;
}
bool plan(const world::World& world, const IslePlayerSource& player, const Observation& observed,
          VolcanoFoodFacts& facts, IsleGameVec3& target) {
    const float dx=player.CameraEyePos.x-observed.root.x,dz=player.CameraEyePos.z-observed.root.z;
    const float length=std::hypot(dx,dz);
    if (!(length>0 && finite(observed.root))) return false;
    const float ux=dx/length,uz=dz/length;
    bool found=false; float best_distance=0;
    for (float offset:{400.0f,500.0f,320.0f,240.0f}) for (float lateral:{0.0f,-80.0f,80.0f}) {
        ++facts.candidates;
        IsleGameVec3 candidate{observed.root.x+offset*ux-lateral*uz,0,observed.root.z+offset*uz+lateral*ux};
        world::GroundResult result{};
        if (!ground(world,candidate,result) || !walkable(observed,result.surface)) { ++facts.ground_rejections; continue; }
        candidate.y=result.height;
        IsleGameVec3 ray{},contact{},stopped{}; int surface=-1; unsigned rebounds=0;
        if (!projectile_target(player,candidate,ray) || !landing(world,player,observed,ray,candidate,contact,surface,rebounds)) {
            ++facts.arc_rejections; continue;
        }
        if (!corridor(world,observed,contact,stopped)) { ++facts.corridor_rejections; continue; }
        const float distance=squared(stopped,player.CameraEyePos);
        if (distance>portrait_range_squared) { ++facts.portrait_rejections; continue; }
        if (found && distance>=best_distance) continue;
        // Food supplies an explicit Source pose, not ordinary yaw-based points.
        // Prefer the closest reachable stop; future cart motion remains unproved.
        found=true; best_distance=distance; target=candidate;
        facts.surface=surface; facts.predicted_contact=contact; facts.predicted_bounces=rebounds;
        facts.predicted_stop=stopped;
    }
    return found;
}
bool same_subject(const IslePlayerPokemonPhoto& p,const IslePlayerPokemonPhoto& q) {
    return p.pokemonID==q.pokemonID && p.unk_00_13==q.unk_00_13 && p.poseID==q.poseID && p.specialID==q.specialID &&
        p.animationTime==q.animationTime && p.yaw==q.yaw && p.position.x==q.position.x &&
        p.position.y==q.position.y && p.position.z==q.position.z;
}
} // namespace
VolcanoFoodPhoto::VolcanoFoodPhoto(private_post_course::Controller controller,Aim aim):controller_(controller),aim_(aim) {}
void VolcanoFoodPhoto::publish(std::uint16_t buttons,std::int8_t x,std::int8_t y) noexcept {
    controller_.set(controller_.owner,buttons,x,y);
}
bool VolcanoFoodPhoto::update(const application::RuntimeView& view,const game::State& state,
                             std::uint64_t updates,bool available) {
    auto& f=facts_; f.owns_pad=false;
    if (sampled_ && updates==last_update_) return false;
    require(!sampled_ || updates>last_update_,"Volcano food update ordinal regressed");
    sampled_=true; last_update_=updates;
    if (f.phase==Phase::closed || !view.volcano) return false;
    const auto* host=view.volcano->native_host();
    const auto* player=view.volcano->adapter().native.player();
    if (!host || !player || !host->actors() || !host->items_owner() || !host->world_owner()) return false;
    const auto& world=*host->world_owner(); const auto& items=*host->items_owner();
    if (!world.current() || !world.current()->resource || world.retired()) return false;
    require(state.save() && player->photo && player->photo->status==ISLE_GAME_OK,
            "Volcano food canonical Save and Photo owners unavailable");
    if (!f.checked) {
        const auto apple=state.save()->player_flag(0);
        const auto charm=view.volcano->photos().report_present(4), magmar=view.volcano->photos().report_present(126);
        require(apple && charm && magmar,"Volcano food canonical prerequisites unavailable");
        f.prior_reports=apple.value && charm.value && magmar.value; f.checked=true;
    }
    if (!f.prior_reports) return false;
    const auto observed=observe(*player,*host->actors(),items,world,f.actor);
    f.present=observed.present; f.graph=observed.graph; f.state=observed.state; f.path=observed.path;
    f.apple=observed.apple; f.interaction=observed.interaction; f.valid_items=observed.valid_items;
    f.position=observed.root; f.collision=observed.collision; f.yaw=observed.yaw;
    f.facing_error=wrapped(std::atan2(player->CameraEyePos.x-observed.root.x,player->CameraEyePos.z-observed.root.z)-observed.yaw);
    const bool ready=player->D_80382D04_523114==1 && !player->IsInputDisabled && !player->IsPaused;
    const auto enter=[&](Phase phase,std::uint64_t bound) { f.phase=phase; f.phase_started=updates; f.deadline=updates+bound; };
    const auto finish=[&](End end) { f.end=end; enter(Phase::release,4); };
    const auto zoom=[&]() -> std::uint16_t { return player->ZoomSwitchMode==0 && player->gDirectionIndex<0 ? z : 0; };
    if (!available) {
        if (f.phase!=Phase::dormant) { f.end=End::preempted; f.phase=Phase::closed; }
        return false;
    }
    if (f.phase==Phase::dormant) {
        if (!ready || !observed.eligible || !observed.ready || !observed.capacity || observed.valid_items ||
            player->photo->gPhotoCount>=57 || player->input.current_buttons&a || updates%8 || f.plans>=128) return false;
        // Group5 is already visible one whole block before useful range. Do
        // not exhaust the fixed search at 4–6k distance, before a portrait or
        // its camera-side food is a physically reachable close opportunity.
        IsleGameVec3 reachable{};
        if (squared(player->CameraEyePos,observed.collision)>2000*2000 ||
            !projectile_target(*player,observed.collision,reachable)) return false;
        ++f.plans; IsleGameVec3 target{};
        if (!plan(world,*player,observed,f,target)) return false;
        f.actor=observed.actor; f.candidate_global=translated(target,observed.origin,1); enter(Phase::aiming,360);
    }
    if (f.phase!=Phase::release && (!observed.present || observed.actor!=f.actor)) finish(End::actor_lost);
    if (f.throw_consumed && !f.throw_release && updates>f.throw_consumed && !(player->input.current_buttons&a)) {
        f.throw_release=updates; ++f.releases;
    }
    if (f.phase==Phase::throw_receipt) {
        const bool pressed=(player->input.current_buttons&a) && (player->input.pressed_buttons&a);
        if (pressed) ++f.throw_presses;
        const bool consumed=updates==f.throw_request+1 && pressed && ready && player->gDirectionIndex>=0 &&
            !player->TargetDirectionZoomedIn && player->TurnToDirSpeed==0 && player->input.current_buttons==a &&
            player->LastItemId==163 && player->ThrowItemTimeout>0 && player->photo->gPhotoCount==f.film_before &&
            player->photo->D_800AE27C==f.qualified_before && observed.valid_items==1 && observed.only_item[0] &&
            observed.item_state==ISLE_ITEM_FLYING;
        if (!consumed) { ++f.mismatches; finish(End::throw_mismatch); }
        else { ++f.throws_consumed; f.throw_consumed=updates; f.item=observed.only_item; enter(Phase::approach,512); }
    }
    const auto instance=items.slot_instance(std::size_t(f.item[1]));
    const auto* food=f.item[0] && key(instance)==f.item ? items.source(instance) : nullptr;
    const auto family=food ? items.family(instance) : std::optional<items::Family>{};
    const bool live=food && family && *family==items::Family::apple && food->state>ISLE_ITEM_INVALID &&
        !(food->flags&ISLE_ITEM_DELETED) && food->restTimer<=140;
    if ((f.phase==Phase::approach || f.phase==Phase::framing) && !live) finish(End::food_lost);
    if (f.phase==Phase::approach || f.phase==Phase::framing) {
        if (food->state==ISLE_ITEM_FLYING && (food->flags&ISLE_ITEM_TOUCHED_GROUND) && !f.flying_ground_contact) {
            f.flying_ground_contact=true; f.flying_ground_contact_update=updates;
        }
        if (food->state==ISLE_ITEM_STILL && !f.settled) {
            const IsleGameModelNode* nodes=nullptr; std::size_t count=0;
            require(isle_game_model_peek_nodes(player->game,items.native_model(instance),&nodes,&count)==ISLE_GAME_OK && nodes && count,
                    "Volcano food actual settled root unavailable");
            world::GroundResult floor{};
            const auto point=nodes[0].transform.position;
            if (ground(world,point,floor) && std::abs(point.y-floor.height-12)<.5f) {
                f.settled=true; f.settled_update=updates; f.surface=floor.surface;
            }
        }
        const bool adopted=observed.apple==f.item && observed.interaction==f.item;
        if (adopted && !f.adopted) { f.adopted=true; f.adopted_update=updates; }
        if (adopted && observed.state==0x802d93dc && observed.path==0x802d9464 && !f.approached) {
            f.approached=true; f.approached_update=updates;
        }
        const bool with_apple=adopted && observed.state==0x802d94e8 && observed.path==0x802d95ac && observed.graph==0x802e1ba4;
        if (f.phase==Phase::approach && f.settled && f.approached && with_apple) {
            f.with_apple=true; f.with_apple_update=updates; enter(Phase::framing,360);
        }
        if (f.phase==Phase::framing && !with_apple) finish(End::food_lost);
        else if (updates>=f.deadline) finish(f.phase==Phase::approach ? End::approach_timeout : End::framing_timeout);
    }
    if (f.phase==Phase::shutter_receipt) {
        const bool pressed=(player->input.current_buttons&a) && (player->input.pressed_buttons&a);
        if (pressed) ++f.shutter_presses;
        f.taken_context=player->photo->PokemonDetector_TakenPhotoIndex;
        bool coherent=updates==f.shutter_request+1 && pressed && ready && live && food->state==ISLE_ITEM_STILL &&
            observed.apple==f.item && observed.interaction==f.item && observed.state==0x802d94e8 &&
            observed.path==0x802d95ac && observed.graph==0x802e1ba4 &&
            player->photo->gPhotoCount==f.slot+1 && player->photo->D_800AE27C==f.qualified_before+1 &&
            f.taken_context>=0 && f.taken_context<2 && f.decision_context>=0 && f.decision_context<2;
        if (coherent) {
            const auto& image=player->photo->D_800B0598[f.slot];
            const auto& context=player->photo->PokemonDetector_Photo[f.taken_context];
            coherent=image.levelID==2 && (image.unk_00_16&0xe0u)==0x20u && image.pokemons[0].pokemonID==4 &&
                key(host->actors()->instance(context.pokemonInFocus))==f.actor;
            bool same=false;
            for (std::size_t i=0;i<12 && coherent;++i) if (key(host->actors()->instance(context.pokemonObjects[i]))==f.actor) {
                same=same_subject(context.main.pokemons[i],image.pokemons[0]); break;
            }
            coherent=coherent && same;
        }
        f.coherent=coherent;
        if (coherent) { f.capture_update=updates; finish(End::none); }
        else { ++f.mismatches; finish(End::capture_mismatch); }
    }
    if (f.phase==Phase::release) {
        if (!(player->input.current_buttons&a)) {
            if (f.coherent) { f.shutter_release=updates; ++f.releases; }
            f.complete=f.coherent && f.settled && f.approached && f.with_apple && f.throw_release && f.shutter_release>f.capture_update;
            if (f.complete) f.end=End::complete;
            f.phase=Phase::closed; return false;
        }
        if (updates>=f.deadline) { f.end=End::release_timeout; f.phase=Phase::closed; return false; }
        f.owns_pad=true; ++f.controls; publish(zoom()); return true;
    }
    f.owns_pad=true; ++f.controls;
    if (f.phase==Phase::aiming && updates>=f.deadline) { finish(End::aim_timeout); publish(); return true; }
    if (!ready) { publish(zoom()); return true; }
    if (player->photo->gPhotoCount>=57) { finish(End::film_reserved); publish(zoom()); return true; }
    if (f.phase==Phase::aiming) {
        if (!observed.ready || !observed.eligible || observed.valid_items) { finish(End::food_lost); publish(); return true; }
        if (player->gDirectionIndex<0 || player->TargetDirectionZoomedIn || player->TurnToDirSpeed!=0) {
            publish(player->ZoomSwitchMode==1 && player->gDirectionIndex==-1 && !(player->input.current_buttons&z) ? z : 0); return true;
        }
        const auto target=translated(f.candidate_global,observed.origin,-1); IsleGameVec3 ray{};
        if (!projectile_target(*player,target,ray)) { publish(); return true; }
        const float dx=ray.x-player->CameraEyePos.x,dz=ray.z-player->CameraEyePos.z;
        const float sn=std::sin(-player->movement.rotation.y),cs=std::cos(-player->movement.rotation.y);
        const float yaw=std::atan2(-(dx*cs+dz*sn),dz*cs-dx*sn);
        int wanted=player->gDirectionIndex;
        const auto error=[&](int index) { return std::abs(wrapped(yaw-(float(index)-2)*1.5707963267948966f)); };
        for (int index=0;index<4;++index) if (error(index)<error(wanted)) wanted=index;
        if (wanted!=player->gDirectionIndex) {
            const std::uint16_t turn=(wanted-player->gDirectionIndex+4)%4==3 ? 0x0002 : 0x0001;
            publish(!(player->input.current_buttons&turn) ? turn : 0); return true;
        }
        std::int8_t x=0,y=0; aim_.point(aim_.owner,*player,ray,false,x,y);
        if (!x && !y && player->ThrowItemTimeout==0 && !(player->input.current_buttons&(a|z)) &&
            player->ui && player->ui->Icons_NumItemsAvailable>=1 &&
            landing(world,*player,observed,player->cam_target,target,f.predicted_contact,f.surface,f.predicted_bounces) &&
            corridor(world,observed,f.predicted_contact,f.predicted_stop) &&
            squared(f.predicted_stop,player->CameraEyePos)<=portrait_range_squared) {
            f.film_before=player->photo->gPhotoCount; f.qualified_before=player->photo->D_800AE27C;
            f.throw_request=updates; ++f.throw_requests; enter(Phase::throw_receipt,1); publish(a); return true;
        }
        publish(0,x,y); return true;
    }
    // Frame while the actor approaches; Source, not an invented delay, decides
    // WithApple readiness. Hold/toggle zoom follows the established pad policy.
    const bool zoomed=player->gDirectionIndex==-1 && !player->TargetDirectionZoomedIn && player->TurnToDirSpeed==0;
    if (!zoomed) {
        const bool enter_zoom=player->ZoomSwitchMode==0 || (player->gDirectionIndex>=0 &&
            !player->TargetDirectionZoomedIn && !(player->input.current_buttons&z));
        publish(enter_zoom ? z : 0); return true;
    }
    std::int8_t x=0,y=0; aim_.point(aim_.owner,*player,observed.collision,true,x,y);
    // Actual proximity gates this sole shutter; Original still decides size,
    // explicit pose and center coverage from the completed captured context.
    f.aligned_updates=f.phase==Phase::framing &&
        squared(player->CameraEyePos,observed.collision)<=portrait_range_squared ? f.aligned_updates+1 : 0;
    std::uint16_t buttons=zoom();
    if (f.phase==Phase::framing && f.throw_release && f.aligned_updates>=2 && observed.focused &&
        player->sTimerAfterPhotoTaken<0 && !(player->input.current_buttons&a)) {
        f.slot=player->photo->gPhotoCount; f.qualified_before=player->photo->D_800AE27C;
        f.decision_context=observed.context; f.shutter_request=updates; ++f.shutter_requests;
        enter(Phase::shutter_receipt,1); buttons|=a;
    }
    publish(buttons,x,y); return true;
}
} // namespace isle::private_campaign

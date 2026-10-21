#include "native_volcano_magmar_pair.hpp"
#include "native_volcano_food_geometry.hpp"
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
#include "player_photo.hpp"
#include "world_runtime.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace isle::private_campaign {
namespace {
namespace geometry = food_geometry;
using Key = MagmarPairKey;
using Phase = MagmarPairPhase;
using End = MagmarPairEnd;
constexpr std::uint16_t a = 0x8000, z = 0x2000;
constexpr std::uint32_t wait_graph = 0x802e10a4, normal_graph = 0x802e10c4;
constexpr std::uint32_t walk_graph = 0x802e1154, running_graph = 0x802e11f4, fighting_graph = 0x802e1434;
constexpr std::uint32_t initial = 0x802d73a8, idle = 0x802d7510, look = 0x802d7584, walk = 0x802d75f4;
constexpr std::uint32_t apple_nearby = 0x802d78a4, run_to_apple = 0x802d798c;
constexpr std::uint32_t interact = 0x802d7b54, turn_to_rival = 0x802d7c30;
constexpr float portrait_range_squared = 1000.0f*1000.0f;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
Key key(menu::animation::InstanceHandle value) { return {value.pool,value.slot,value.incarnation}; }
float wrapped(float value) { return std::remainder(value,6.28318530717958647692f); }
IsleGameVec3 translated(const IsleGameVec3& value,const IsleGameVec3& origin,float sign) {
    return {value.x+sign*origin.x,value.y+sign*origin.y,value.z+sign*origin.z};
}
bool finite(const IsleGameVec3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}
bool accepts(const IsleActorSource& actor,int command,IsleGameSourceCallback handler) {
    if (!actor.transitionGraph) return false;
    for (std::size_t i=0;i<actor.transitionGraph->count;++i) {
        const auto& row=actor.transitionGraph->rows[i];
        if (row.cmd==58) break;
        if (row.cmd==command && row.state==handler) return true;
    }
    return false;
}
std::uint32_t callback(IsleGameSourceCallback function) {
    if (function==magmar_InitialState) return initial;
    if (function==magmar_Idle) return idle;
    if (function==magmar_LookAround) return look;
    if (function==magmar_Walk) return walk;
    if (function==magmar_AppleNearby) return apple_nearby;
    if (function==magmar_RunToApple) return run_to_apple;
    if (function==magmar_InteractWithPokemon) return interact;
    if (function==magmar_TurnToRival) return turn_to_rival;
    if (function==magmar_BurnedDown) return 0x802d7cb4;
    return 0;
}
MagmarPairSubject subject(const IslePlayerPokemonPhoto& photo) {
    return {photo.pokemonID,photo.unk_00_13,photo.poseID,photo.specialID,
            photo.animationTime,photo.yaw,photo.position};
}
bool same(const MagmarPairSubject& left,const MagmarPairSubject& right) {
    return left.pokemon==right.pokemon && left.animation==right.animation && left.pose==right.pose &&
        left.special==right.special && left.time==right.time && left.yaw==right.yaw &&
        left.position.x==right.position.x && left.position.y==right.position.y && left.position.z==right.position.z;
}
bool fighting_subject(const MagmarPairSubject& value) {
    return value.pokemon==126 && value.animation==9 && value.pose==111 && value.special==11 &&
        std::isfinite(value.time) && value.time>=0 && std::isfinite(value.yaw) && finite(value.position);
}
bool same_position(const IsleGameVec3& left,const IsleGameVec3& right) {
    return left.x==right.x && left.y==right.y && left.z==right.z;
}
bool full_copy(const IslePlayerPhotoData& image,const IslePlayerPhotoContext& context,std::size_t primary) {
    const auto& source=context.main;
    if (image.levelID!=source.levelID || image.unk_00_24!=source.unk_00_24 ||
        image.unk_00_16!=(source.unk_00_16|0x20u) || image.unk_04.f32!=source.unk_04.f32 ||
        !same_position(image.unk_08,source.unk_08) || !same_position(image.unk_14,source.unk_14) ||
        !same(subject(image.pokemons[0]),subject(source.pokemons[primary]))) return false;
    // C738–833 copies camera, all Items/FX and every Pokemon, moving the real
    // focus to slot0 and retaining the order of all other captured subjects.
    std::size_t output=1;
    for (std::size_t i=0;i<12;++i) if (i!=primary) {
        if (!same(subject(image.pokemons[output++]),subject(source.pokemons[i]))) return false;
    }
    for (std::size_t i=0;i<6;++i) {
        const auto& left=image.items[i]; const auto& right=source.items[i];
        if (left.itemType!=right.itemType || left.animationTime!=right.animationTime || !same_position(left.pos,right.pos)) return false;
    }
    for (std::size_t i=0;i<32;++i) {
        const auto& left=image.effects[i]; const auto& right=source.effects[i];
        if (left.textureID!=right.textureID || left.dataID!=right.dataID || left.bankID!=right.bankID ||
            left.unk_03!=right.unk_03 || left.posX!=right.posX || left.posY!=right.posY ||
            left.posZ!=right.posZ || left.size!=right.size) return false;
        for (std::size_t component=0;component<4;++component)
            if (left.primColor[component]!=right.primColor[component]) return false;
    }
    return true;
}
struct ActorObservation {
    bool present=false, eligible=false, ready=false, focused=false, accepts26=false, collision_enabled=false;
    Key actor{}, apple{}, food_target{}, interaction{};
    std::uint32_t graph=0,state=0,path=0,model_anims=0,process_flags=0;
    int context=-1;
    float yaw=0;
    geometry::Walker walker{};
    MagmarPairSubject live{};
};
struct Observation {
    std::array<ActorObservation,2> pair{};
    geometry::Scene scene{};
    bool capacity=true, provenance=true, moltres_appeared=false;
    unsigned pair_count=0,valid_items=0;
    Key moltres{},only_item{};
    int item_state=ISLE_ITEM_INVALID;
    IsleGameVec3 origin{};
};
Observation observe(const IslePlayerSource& player,const actor::Owner& actors,const items::Owner& items,
                    const world::World& world,const isle::player::PhotoResources& photos,const std::array<Key,2>& selected) {
    Observation out;
    const auto origin=world.current()->resource->metadata.world_position;
    out.origin={origin.x*100,origin.y*100,origin.z*100};
    IslePlayerPhotoFocus focus{};
    require(isle_player_photo_published_focus(player.photo,&focus)==ISLE_GAME_OK,"Magmar pair published focus unavailable");
    IsleGameObject* previous=nullptr;
    for (;;) {
        IsleGameObject* object=nullptr; IsleActorSource* source=nullptr; IsleGameModel* model=nullptr;
        require(isle_actor_source_link3_next(player.game,previous,&object,&source,&model)==ISLE_GAME_OK,
                "Magmar pair native Actor traversal rejected");
        if (!object) break;
        previous=object;
        const bool pair=source->id==126 && source->behavior==0;
        if (source->tangible && source->collisionRadius>0) {
            if (out.scene.sphere_count==out.scene.spheres.size()) out.capacity=false;
            else out.scene.spheres[out.scene.sphere_count++]={source->collPosition,source->collisionRadius,pair ? 160.0f : 0.0f};
        }
        if (!pair && source->id!=146) continue;
        const auto identity=key(actors.instance(object));
        require(identity[0]!=0,"Magmar pair native Actor identity unavailable");
        const auto membership=actors.membership(actors.instance(object));
        IsleGameSourceCallback state=nullptr,path=nullptr;
        if (source->stateProc) require(isle_game_process_source_function(player.game,source->stateProc,&state)==ISLE_GAME_OK,
                                      "Magmar pair state observation rejected");
        if (source->pathProc) require(isle_game_process_source_function(player.game,source->pathProc,&path)==ISLE_GAME_OK,
                                     "Magmar pair path observation rejected");
        IsleGameObjectMetadata metadata{};
        require(isle_game_object_metadata(player.game,object,&metadata)==ISLE_GAME_OK,"Magmar pair visibility unavailable");
        if (!pair) {
            // Source's pre-egg Moltres already exists but is hidden/intangible.
            // A Report or that initial instance is not an appearance receipt.
            if (membership.logical && !membership.retiring && source->tangible && !(metadata.flags&1u) &&
                state==moltres_Appear) {
                out.moltres_appeared=true; out.moltres=identity;
            }
            continue;
        }
        const auto ordinal=out.pair_count++;
        bool block4=false;
        for (const auto& block:world.blocks()) if (source->baseBlock==&block) block4=block.index==4;
        if (!block4 || !membership.logical || !membership.registered || membership.retiring || !model) out.provenance=false;
        if (ordinal>=2) continue;
        std::size_t index=ordinal;
        if (selected[0][0]) {
            if (identity==selected[0]) index=0;
            else if (identity==selected[1]) index=1;
            else { out.provenance=false; continue; }
        }
        auto& actor=out.pair[index];
        if (actor.present) { out.provenance=false; continue; }
        actor.present=true; actor.actor=identity;
        actor.apple=key(items.instance(source->apple));
        actor.food_target=key(items.instance(source->interactionTarget));
        actor.interaction=key(actors.instance(source->interactionTarget));
        actor.graph=source->transitionGraph ? source->transitionGraph->source_identity : 0;
        actor.state=callback(state); actor.path=callback(path);
        actor.model_anims=source->modelAnims; actor.process_flags=source->processFlags;
        actor.collision_enabled=(source->flags&0x200u)!=0;
        actor.accepts26=accepts(*source,26,magmar_InteractWithPokemon) &&
            (actor.graph==walk_graph || actor.graph==running_graph);
        actor.ready=accepts(*source,15,magmar_SearchApple) &&
            ((actor.graph==normal_graph && (state==magmar_Idle || state==magmar_LookAround)) ||
             (actor.graph==walk_graph && state==magmar_Walk));
        const IsleGameModelPlacement* placement=nullptr;
        require(isle_game_model_placement(player.game,model,&placement)==ISLE_GAME_OK && placement,
                "Magmar pair Actor placement unavailable");
        actor.walker.root=placement->transform.position; actor.yaw=placement->transform.rotation.y;
        actor.walker.collision=source->collPosition; actor.walker.radius=source->collisionRadius;
        actor.eligible=placement->mode!=ISLE_GAME_MODEL_PLACEMENT_DISABLED && !(metadata.flags&1u) &&
            source->tangible && (source->flags&4u) && !(source->flags&8u) && source->playerDist>=0 && source->playerDist<=10000;
        actor.focused=focus.present && focus.object==object && focus.pokemon_id==126 && (focus.flags&4u) && !(focus.flags&8u);
        actor.context=focus.context;
        if (source->forbiddenGround) {
            actor.walker.forbidden_valid=false;
            for (std::size_t i=0;i<source->forbiddenGroundCount && i<actor.walker.forbidden.size();++i) {
                const float value=source->forbiddenGround[i];
                actor.walker.forbidden[actor.walker.forbidden_count++]=value;
                if (!value) { actor.walker.forbidden_valid=true; break; }
            }
        }
        IsleGameModelRef reference{};
        const IsleGameModelTransform* auxiliary=nullptr;
        require(isle_actor_owner_self_model(source,&reference)==ISLE_GAME_OK &&
                isle_actor_owner_aux_read(source,&reference,&auxiliary)==ISLE_GAME_OK && auxiliary,
                "Magmar pair photo placement unavailable");
        const int animation=photos.animation_index(126,source->modelAnims);
        const int pose=source->poseID ? source->poseID : static_cast<int>(photos.pose(126,animation));
        actor.live={126,animation,pose,source->specialPoseID,isle_actor_owner_animation_time(source),
                    auxiliary->rotation.y,auxiliary->position};
    }
    for (std::size_t slot=0;slot<items.animation().slot_count();++slot) {
        const auto instance=items.slot_instance(slot);
        if (!instance) continue;
        const auto family=items.family(instance);
        require(bool(family),"Magmar pair Item family unavailable");
        if (*family!=items::Family::apple && *family!=items::Family::pester_ball) continue;
        const auto* source=items.source(instance);
        require(source,"Magmar pair Item payload unavailable");
        if (source->state<=ISLE_ITEM_INVALID || (source->flags&ISLE_ITEM_DELETED)) continue;
        ++out.valid_items; out.only_item=*family==items::Family::apple ? key(instance) : Key{};
        out.item_state=source->state;
    }
    return out;
}
struct Forecast {
    geometry::LandingForecast landing{};
    std::array<IsleGameVec3,2> stop{};
    std::array<unsigned,2> steps{};
    unsigned arrival_delta=0;
    float overlap_margin=0;
};
bool forecast(const world::World& world,const IslePlayerSource& player,const Observation& observed,
              const IsleGameVec3& ray,const IsleGameVec3& target,Forecast& out,MagmarPairFacts& facts) {
    world::GroundResult floor{};
    if (!geometry::ground(world,target,floor) || !geometry::walkable(observed.pair[0].walker,floor.surface) ||
        !geometry::walkable(observed.pair[1].walker,floor.surface)) { ++facts.ground_rejections; return false; }
    if (!geometry::landing(world,player,observed.pair[0].walker,observed.scene,ray,target,out.landing) ||
        !(geometry::squared(out.landing.root,observed.pair[1].walker.collision)<590*590) ||
        !geometry::walkable(observed.pair[1].walker,out.landing.surface)) { ++facts.arc_rejections; return false; }
    for (std::size_t i=0;i<2;++i) {
        if (!geometry::corridor(world,observed.pair[i].walker,out.landing.root,160,50,out.stop[i],out.steps[i])) {
            ++facts.corridor_rejections; return false;
        }
    }
    out.arrival_delta=std::max(out.steps[0],out.steps[1])-std::min(out.steps[0],out.steps[1]);
    std::array<IsleGameVec3,2> centers{};
    for (std::size_t i=0;i<2;++i) {
        const auto& walker=observed.pair[i].walker;
        centers[i]={out.stop[i].x+walker.collision.x-walker.root.x,
                    out.stop[i].y+walker.collision.y-walker.root.y,
                    out.stop[i].z+walker.collision.z-walker.root.z};
    }
    const float radii=observed.pair[0].walker.radius+observed.pair[1].walker.radius;
    // A last grounded 160*.033 step can change y by at most16 in this
    // conservative corridor. Reserve that whole 3D step before the later
    // stop: an Eating-only overlap is not an accepting collision forecast.
    out.overlap_margin=radii-std::sqrt(geometry::squared(centers[0],centers[1]))-
        std::hypot(160.0f*.033f,16.0f);
    //48 is Source's eating counter, not a promised scheduling/consumption
    // window. This admission leaves two updates and still needs live receipts.
    if (!(out.overlap_margin>0) || out.arrival_delta+2>=48) { ++facts.race_rejections; return false; }
    if (std::min(geometry::squared(centers[0],player.CameraEyePos),
                 geometry::squared(centers[1],player.CameraEyePos))>portrait_range_squared) {
        ++facts.portrait_rejections; return false;
    }
    return true;
}
void remember(const Forecast& prediction,MagmarPairFacts& facts) {
    facts.surface=prediction.landing.surface; facts.predicted_contact=prediction.landing.root;
    facts.predicted_bounces=prediction.landing.rebounds; facts.predicted_stop=prediction.stop;
    facts.predicted_steps=prediction.steps; facts.predicted_arrival_delta=prediction.arrival_delta;
    facts.predicted_overlap_margin=prediction.overlap_margin; facts.prospective_overlap=true;
}
bool plan(const world::World& world,const IslePlayerSource& player,const Observation& observed,
          MagmarPairFacts& facts,IsleGameVec3& target) {
    const auto& left=observed.pair[0].walker.root;
    const auto& right=observed.pair[1].walker.root;
    const IsleGameVec3 midpoint{(left.x+right.x)*.5f,(left.y+right.y)*.5f,(left.z+right.z)*.5f};
    const float dx=player.CameraEyePos.x-midpoint.x,dz=player.CameraEyePos.z-midpoint.z;
    const float length=std::hypot(dx,dz);
    if (!(length>0 && finite(midpoint))) return false;
    const float ux=dx/length,uz=dz/length;
    bool found=false; float best=0;
    for (float offset:{240.0f,320.0f,400.0f,500.0f}) for (float lateral:{0.0f,-80.0f,80.0f}) {
        ++facts.candidates;
        IsleGameVec3 candidate{midpoint.x+offset*ux-lateral*uz,0,midpoint.z+offset*uz+lateral*ux};
        world::GroundResult floor{};
        if (!geometry::ground(world,candidate,floor)) { ++facts.ground_rejections; continue; }
        candidate.y=floor.height;
        IsleGameVec3 ray{}; Forecast prediction{};
        if (!projectile_target(player,candidate,ray)) { ++facts.arc_rejections; continue; }
        if (!forecast(world,player,observed,ray,candidate,prediction,facts)) continue;
        const float distance=std::min(geometry::squared(prediction.stop[0],player.CameraEyePos),
                                      geometry::squared(prediction.stop[1],player.CameraEyePos));
        if (found && distance>=best) continue;
        found=true; best=distance; target=candidate; remember(prediction,facts);
    }
    return found;
}
bool fight(const ActorObservation& actor,const Key& partner) {
    return actor.state==interact && actor.path==turn_to_rival && actor.graph==fighting_graph &&
        actor.interaction==partner && fighting_subject(actor.live);
}
} // namespace

MagmarPairPhoto::MagmarPairPhoto(private_post_course::Controller controller,Aim aim):controller_(controller),aim_(aim) {}
void MagmarPairPhoto::publish(std::uint16_t buttons,std::int8_t x,std::int8_t y) noexcept {
    controller_.set(controller_.owner,buttons,x,y);
}
bool MagmarPairPhoto::update(const application::RuntimeView& view,const game::State& state,
                            std::uint64_t updates,bool available) {
    auto& f=facts_; f.owns_pad=false;
    if (sampled_ && updates==last_update_) return false;
    require(!sampled_ || updates>last_update_,"Magmar pair update ordinal regressed");
    const bool consecutive=observed_ && updates==last_observation_+1;
    sampled_=true; last_update_=updates;
    if (f.phase==Phase::closed) return false;
    if (!available && f.phase!=Phase::dormant) { f.end=End::preempted; f.phase=Phase::closed; return false; }
    const auto close=[&](End end) { f.end=end; f.phase=Phase::closed; };
    if (!view.volcano) { if (f.phase!=Phase::dormant) close(End::retired); return false; }
    const auto* host=view.volcano->native_host();
    const auto* player=view.volcano->adapter().native.player();
    if (!host || !player || !host->actors() || !host->items_owner() || !host->world_owner()) {
        if (f.phase!=Phase::dormant) close(End::retired);
        return false;
    }
    const auto& world=*host->world_owner(); const auto& items=*host->items_owner();
    if (!world.current() || !world.current()->resource || world.retired()) {
        if (f.phase!=Phase::dormant) close(End::retired);
        return false;
    }
    require(state.save() && player->photo && player->photo->status==ISLE_GAME_OK,
            "Magmar pair canonical Save and Photo owners unavailable");
    if (!f.checked) {
        const auto apple=state.save()->player_flag(0);
        auto& photos=view.volcano->photos();
        const auto charm=photos.record(ISLE_PHOTO_REPORT,photos.report_index(4));
        const auto magmar=photos.record(ISLE_PHOTO_REPORT,photos.report_index(126));
        require(apple && charm && magmar,"Magmar pair canonical prerequisites unavailable");
        f.prior_reports=apple.value && charm.value.photo && magmar.value.photo &&
            charm.value.photo->pokemons[0].pokemonID==4 && charm.value.photo->pokemons[0].poseID==106 &&
            magmar.value.photo->pokemons[0].pokemonID==126;
        f.checked=true;
        if (!f.prior_reports) { close(End::prerequisites); return false; }
    }
    const auto previous_graph=f.graph;
    const auto previous_state=f.state;
    const auto previous_accepting=f.accepting26;
    const bool previous_overlap=f.overlap;
    const auto observed=observe(*player,*host->actors(),items,world,view.volcano->photos().resources(),f.actor);
    observed_=true; last_observation_=updates;
    f.pair_count=observed.pair_count; f.capacity=observed.capacity; f.valid_items=observed.valid_items;
    f.pair_present=observed.pair_count==2 && observed.provenance && observed.pair[0].present && observed.pair[1].present &&
        observed.pair[0].actor!=observed.pair[1].actor;
    if (!f.actor[0][0] && f.pair_present) {
        f.actor[0]=observed.pair[0].actor; f.actor[1]=observed.pair[1].actor;
    }
    if (observed.moltres_appeared && !f.moltres_appeared) {
        f.moltres_appeared=true; f.moltres=observed.moltres; f.moltres_update=updates;
    }
    for (std::size_t i=0;i<2;++i) {
        const auto& actor=observed.pair[i];
        f.present[i]=actor.present; f.graph[i]=actor.graph; f.state[i]=actor.state; f.path[i]=actor.path;
        f.model_anims[i]=actor.model_anims; f.process_flags[i]=actor.process_flags;
        f.apple[i]=actor.apple; f.food_target[i]=actor.food_target; f.interaction[i]=actor.interaction;
        f.accepting26[i]=actor.accepts26; f.position[i]=actor.walker.root; f.collision[i]=actor.walker.collision;
        f.radius[i]=actor.walker.radius; f.yaw[i]=actor.yaw; f.live[i]=actor.live;
        if (!actor.present || actor.actor!=f.actor[i]) continue;
        if (actor.graph==wait_graph && actor.state==initial && !f.wait_seen[i]) {
            f.wait_seen[i]=true; f.wait_update[i]=updates;
        }
        // M136–139/294–330: the Moltres command schedules Idle. P252–262
        // clears its graph before the body installs Normal, so observe either
        // actual Idle publication, but never bridge an unobserved update.
        if (!f.activated[i] && f.wait_seen[i] && consecutive && previous_graph[i]==wait_graph &&
            previous_state[i]==initial && actor.state==idle && (actor.graph==0 || actor.graph==normal_graph)) {
            f.activated[i]=true; f.activation_update[i]=updates;
        }
    }
    const float radii=f.radius[0]+f.radius[1];
    f.overlap=f.pair_present && observed.pair[0].collision_enabled && observed.pair[1].collision_enabled &&
        radii>0 && geometry::squared(f.collision[0],f.collision[1])<radii*radii;
    if (f.overlap && !f.overlap_update) f.overlap_update=updates;
    const bool ready=player->D_80382D04_523114==1 && !player->IsInputDisabled && !player->IsPaused;
    const auto enter=[&](Phase phase,std::uint64_t bound) { f.phase=phase; f.phase_started=updates; f.deadline=updates+bound; };
    const auto finish=[&](End end) { f.end=end; enter(Phase::release,4); };
    const auto zoom=[&]() -> std::uint16_t { return player->ZoomSwitchMode==0 && player->gDirectionIndex<0 ? z : 0; };
    if (f.phase==Phase::dormant) {
        if (observed.pair_count>2 || (observed.pair_count && !observed.provenance)) { close(End::pair_invalid); return false; }
        if (f.actor[0][0] && !f.pair_present) { close(End::actor_lost); return false; }
        if (f.pair_present) for (std::size_t i=0;i<2;++i) {
            if (!f.activated[i] && observed.pair[i].graph && observed.pair[i].graph!=wait_graph) {
                close(End::activation_missing); return false;
            }
        }
        if (!available || !f.pair_present || !f.activated[0] || !f.activated[1] || !f.moltres_appeared) return false;
        if (f.plans>=128) { close(End::plan_exhausted); return false; }
        if (!ready || !observed.pair[0].ready || !observed.pair[1].ready || !observed.pair[0].eligible ||
            !observed.pair[1].eligible || !observed.capacity || observed.valid_items ||
            player->photo->gPhotoCount>=57 || (player->input.current_buttons&a) || updates%8) return false;
        if (std::min(geometry::squared(player->CameraEyePos,f.collision[0]),
                     geometry::squared(player->CameraEyePos,f.collision[1]))>2000*2000) return false;
        ++f.plans; IsleGameVec3 target{};
        if (!plan(world,*player,observed,f,target)) return false;
        f.candidate_global=translated(target,observed.origin,1); enter(Phase::aiming,360);
    }
    if (f.phase!=Phase::release && !f.pair_present) finish(End::actor_lost);
    if (f.throw_consumed && !f.throw_release && updates>f.throw_consumed && !(player->input.current_buttons&a)) {
        f.throw_release=updates; ++f.releases;
    }
    if (f.throw_consumed && !f.throw_release && updates>=f.throw_consumed+4 && f.phase!=Phase::release)
        finish(End::throw_release_timeout);
    if (f.phase==Phase::throw_receipt) {
        const bool pressed=(player->input.current_buttons&a) && (player->input.pressed_buttons&a);
        if (pressed) ++f.throw_presses;
        const bool consumed=updates==f.throw_request+1 && pressed && ready && player->gDirectionIndex>=0 &&
            !player->TargetDirectionZoomedIn && player->TurnToDirSpeed==0 && player->input.current_buttons==a &&
            player->LastItemId==163 && player->ThrowItemTimeout>0 && player->photo->gPhotoCount==f.film_before &&
            player->photo->D_800AE27C==f.qualified_before && observed.valid_items==1 && observed.only_item[0] &&
            observed.item_state==ISLE_ITEM_FLYING;
        if (!consumed) { ++f.mismatches; finish(End::throw_mismatch); }
        else {
            ++f.throws_consumed; f.throw_consumed=updates; f.item=observed.only_item;
            enter(Phase::convergence,512); f.convergence_deadline=f.deadline;
        }
    }
    const auto instance=items.slot_instance(std::size_t(f.item[1]));
    const auto* food=f.item[0] && key(instance)==f.item ? items.source(instance) : nullptr;
    const auto family=food ? items.family(instance) : std::optional<items::Family>{};
    const bool live=food && family && *family==items::Family::apple && food->state>ISLE_ITEM_INVALID &&
        !(food->flags&ISLE_ITEM_DELETED) && food->restTimer<=140;
    if (f.phase==Phase::convergence || f.phase==Phase::framing) {
        if (player->photo->gPhotoCount!=f.film_before || player->photo->D_800AE27C!=f.qualified_before) {
            ++f.mismatches; finish(End::capture_mismatch);
        }
        if (!f.accepted_interaction && !live) finish(End::food_lost);
        if (live && food->state==ISLE_ITEM_FLYING && (food->flags&ISLE_ITEM_TOUCHED_GROUND) && !f.flying_ground_contact) {
            f.flying_ground_contact=true; f.flying_ground_contact_update=updates;
        }
        if (live && food->state==ISLE_ITEM_STILL && !f.settled) {
            const IsleGameModelNode* nodes=nullptr; std::size_t count=0;
            require(isle_game_model_peek_nodes(player->game,items.native_model(instance),&nodes,&count)==ISLE_GAME_OK && nodes && count,
                    "Magmar pair actual settled root unavailable");
            world::GroundResult floor{}; const auto point=nodes[0].transform.position;
            if (geometry::ground(world,point,floor) && geometry::walkable(observed.pair[0].walker,floor.surface) &&
                geometry::walkable(observed.pair[1].walker,floor.surface) && std::abs(point.y-floor.height-12)<.5f) {
                f.settled=true; f.settled_update=updates; f.surface=floor.surface; f.actual_settlement=point;
            } else finish(End::unsafe_settlement);
        }
        for (std::size_t i=0;i<2;++i) {
            const auto& actor=observed.pair[i];
            const bool adopted=f.settled && live && food->state==ISLE_ITEM_STILL &&
                actor.apple==f.item && actor.food_target==f.item;
            if (adopted && !f.adopted[i]) { f.adopted[i]=true; f.adoption_update[i]=updates; }
            if (adopted && actor.state==apple_nearby && actor.path==run_to_apple && actor.graph==running_graph &&
                actor.accepts26 && !f.running_seen[i]) { f.running_seen[i]=true; f.running_update[i]=updates; }
        }
        if (!f.accepted_interaction && f.phase==Phase::convergence) {
            int selected=-1; float selected_distance=0;
            for (std::size_t i=0;i<2;++i) {
                const auto& actor=observed.pair[i];
                if (actor.state!=interact) {
                    if (f.accepted26[i]) { finish(End::interaction_lost); break; }
                    continue;
                }
                if (actor.interaction!=f.actor[1-i]) { finish(End::wrong_partner); break; }
                // P1983–1994 assigns the real CMD26 sender before entering
                // M468. A newly scheduled Interact callback can be observed
                // before its body installs TurnToRival/special11: retain the
                // real accepting transition, then require that body separately.
                if (!f.accepted26[i]) {
                    const bool accepting=previous_accepting[i] &&
                        ((previous_graph[i]==running_graph && previous_state[i]==apple_nearby) ||
                         (previous_graph[i]==walk_graph && previous_state[i]==walk));
                    if (!(consecutive && accepting && (f.overlap || previous_overlap) &&
                          f.adopted[0] && f.adopted[1] && f.running_seen[0] && f.running_seen[1])) {
                        finish(End::interaction_history); break;
                    }
                    f.accepted26[i]=true; f.acceptance_update[i]=updates; f.acceptance_graph[i]=previous_graph[i];
                    f.acceptance_world_time[i]=world.global_time();
                }
                if (!fight(actor,f.actor[1-i])) continue;
                const float distance=geometry::squared(actor.walker.collision,player->CameraEyePos);
                // If both actual callbacks qualify together, choose the real
                // focused/closer aggressor, never a guessed link3-list leader.
                if (selected<0 || (actor.focused && !observed.pair[std::size_t(selected)].focused) ||
                    (actor.focused==observed.pair[std::size_t(selected)].focused && distance<selected_distance)) {
                    selected=static_cast<int>(i); selected_distance=distance;
                }
            }
            if (selected>=0 && f.phase==Phase::convergence) {
                const auto chosen=std::size_t(selected);
                f.accepted_interaction=true; f.interaction_update=updates; f.aggressor_index=selected;
                f.aggressor=f.actor[chosen]; f.partner=f.actor[1-chosen]; f.special11=true;
                enter(Phase::framing,360); f.deadline=std::min(f.deadline,f.convergence_deadline);
            }
        }
        if (f.phase==Phase::framing && !fight(observed.pair[std::size_t(f.aggressor_index)],f.partner)) finish(End::interaction_lost);
        if ((f.phase==Phase::convergence || f.phase==Phase::framing) && updates>=f.deadline)
            finish(f.phase==Phase::convergence ? End::convergence_timeout : End::framing_timeout);
    }
    if (f.phase==Phase::shutter_receipt) {
        const bool pressed=(player->input.current_buttons&a) && (player->input.pressed_buttons&a);
        if (pressed) ++f.shutter_presses;
        f.taken_context=player->photo->PokemonDetector_TakenPhotoIndex;
        const auto chosen=std::size_t(f.aggressor_index);
        bool coherent=updates==f.shutter_request+1 && pressed && ready && f.throw_release &&
            f.accepted_interaction && fight(observed.pair[chosen],f.partner) &&
            player->photo->gPhotoCount==f.slot+1 && player->photo->D_800AE27C==f.qualified_before+1 &&
            f.taken_context>=0 && f.taken_context<2 && f.decision_context>=0 && f.decision_context<2;
        if (coherent) {
            const auto& image=player->photo->D_800B0598[f.slot];
            const auto& context=player->photo->PokemonDetector_Photo[f.taken_context];
            f.taken_actor=key(host->actors()->instance(context.pokemonInFocus));
            f.taken=subject(image.pokemons[0]); f.taken_world_time=image.unk_04.f32;
            coherent=image.levelID==2 && (image.unk_00_16&0xe0u)==0x20u && f.taken_actor==f.aggressor &&
                fighting_subject(f.taken) && std::isfinite(f.taken_world_time) &&
                f.taken_world_time>=f.acceptance_world_time[chosen];
            bool copied=false,expected=false;
            for (std::size_t i=0;i<12 && coherent;++i) {
                if (key(host->actors()->instance(context.pokemonObjects[i]))!=f.aggressor) continue;
                copied=full_copy(image,context,i); break;
            }
            // Completed contexts may lag; a draw after the request may instead
            // capture its live subject. Never equate decision and taken indices.
            for (std::size_t context_index=0;context_index<2;++context_index)
                expected=expected || (request_context_present_[context_index][chosen] &&
                    same(request_context_[context_index][chosen],f.taken));
            expected=expected || same(request_live_,f.taken) || same(observed.pair[chosen].live,f.taken);
            coherent=coherent && copied && expected;
        }
        f.coherent=coherent;
        if (coherent) { f.capture_update=updates; finish(End::none); }
        else { ++f.mismatches; finish(End::capture_mismatch); }
    }
    if (f.phase==Phase::release) {
        if (!(player->input.current_buttons&a)) {
            if (f.coherent && updates>f.capture_update) { f.shutter_release=updates; ++f.releases; }
            f.complete=f.coherent && f.settled && f.adopted[0] && f.adopted[1] && f.running_seen[0] && f.running_seen[1] &&
                f.accepted_interaction && f.throw_release && f.shutter_release>f.capture_update;
            if (f.complete) f.end=End::complete;
            f.phase=Phase::closed; return false;
        }
        if (updates>=f.deadline) { close(End::release_timeout); return false; }
        f.owns_pad=true; ++f.controls; publish(zoom()); return true;
    }
    f.owns_pad=true; ++f.controls;
    if (f.phase==Phase::aiming && updates>=f.deadline) { finish(End::aim_timeout); publish(); return true; }
    if (player->photo->gPhotoCount>=57) { finish(End::film_reserved); publish(zoom()); return true; }
    if (!ready) { publish(zoom()); return true; }
    if (f.phase==Phase::aiming) {
        if (!observed.pair[0].ready || !observed.pair[1].ready || !observed.pair[0].eligible ||
            !observed.pair[1].eligible || !observed.capacity || observed.valid_items) {
            finish(End::geometry_lost); publish(); return true;
        }
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
            player->ui && player->ui->Icons_NumItemsAvailable>=1) {
            Forecast prediction{};
            // Screen alignment is not physical ray acceptance. Keep this
            // unspent candidate within its original aim deadline; never throw
            // until the actual ray passes every forecast gate.
            if (!forecast(world,*player,observed,player->cam_target,target,prediction,f)) {
                publish(); return true;
            }
            remember(prediction,f); f.film_before=player->photo->gPhotoCount; f.qualified_before=player->photo->D_800AE27C;
            f.throw_request=updates; ++f.throw_requests; enter(Phase::throw_receipt,1); publish(a); return true;
        }
        publish(0,x,y); return true;
    }
    // Frame throughout the one512-update convergence/fight envelope, without
    // resetting it when the real command26 callback starts.
    const bool zoomed=player->gDirectionIndex==-1 && !player->TargetDirectionZoomedIn && player->TurnToDirSpeed==0;
    if (!zoomed) {
        const bool enter_zoom=player->ZoomSwitchMode==0 || (player->gDirectionIndex>=0 &&
            !player->TargetDirectionZoomedIn && !(player->input.current_buttons&z));
        publish(enter_zoom ? z : 0); return true;
    }
    IsleGameVec3 frame{};
    if (f.aggressor_index>=0) frame=observed.pair[std::size_t(f.aggressor_index)].walker.collision;
    else frame={(f.collision[0].x+f.collision[1].x)*.5f,(f.collision[0].y+f.collision[1].y)*.5f,
                (f.collision[0].z+f.collision[1].z)*.5f};
    std::int8_t x=0,y=0; aim_.point(aim_.owner,*player,frame,true,x,y);
    // The moving cart needs tracking axes even with exact live actor focus.
    // Source's captured pose and score, not stationary sticks, judge the image.
    f.aligned_updates=f.phase==Phase::framing &&
        geometry::squared(player->CameraEyePos,frame)<=portrait_range_squared ? f.aligned_updates+1 : 0;
    std::uint16_t buttons=zoom();
    if (f.phase==Phase::framing && f.throw_release && f.aligned_updates>=2 && player->sTimerAfterPhotoTaken<0 &&
        !(player->input.current_buttons&a)) {
        const auto chosen=std::size_t(f.aggressor_index); const auto& actor=observed.pair[chosen];
        if (actor.eligible && actor.focused && fight(actor,f.partner)) {
            request_context_present_={};
            for (std::size_t context_index=0;context_index<2;++context_index) {
                const auto& context=player->photo->PokemonDetector_Photo[context_index];
                for (std::size_t i=0;i<12;++i) for (std::size_t pair_index=0;pair_index<2;++pair_index) {
                    if (key(host->actors()->instance(context.pokemonObjects[i]))!=f.actor[pair_index]) continue;
                    request_context_[context_index][pair_index]=subject(context.main.pokemons[i]);
                    request_context_present_[context_index][pair_index]=true;
                }
            }
            const auto decision=std::size_t(actor.context);
            if (decision<2 && request_context_present_[decision][chosen] &&
                fighting_subject(request_context_[decision][chosen]) &&
                player->photo->PokemonDetector_Photo[decision].main.unk_04.f32>=f.acceptance_world_time[chosen]) {
                f.slot=player->photo->gPhotoCount;
                request_live_=actor.live;
                f.decision_context=actor.context; f.shutter_request=updates; ++f.shutter_requests;
                enter(Phase::shutter_receipt,1); buttons|=a;
            }
        }
    }
    publish(buttons,x,y); return true;
}
} // namespace isle::private_campaign

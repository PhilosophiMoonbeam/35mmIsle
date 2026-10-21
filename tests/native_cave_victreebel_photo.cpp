#include "native_cave_victreebel_photo.hpp"
#include "captured_photo_identity.hpp"
#include "application_runtime.hpp"
#include "cave_scene.hpp"
#include "game_state.hpp"
#include "actor_owner.hpp"
#include "actor_owner_source.h"
#include "cave_actor_source.h"
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
using cave_victreebel_detail::Actor;
using cave_victreebel_detail::Item;
using cave_victreebel_detail::Snapshot;
using Disposition=CaveVictreebelDisposition;
using Phase=CaveVictreebelPhase;
using ForecastDisposition=CaveVictreebelForecastDisposition;
constexpr std::uint16_t b_button=0x4000, z_button=0x2000, a_button=0x8000;
// Controller policy, not Source timing constants. Each deadline is monotonic.
constexpr std::uint64_t overall_cap=8192, flight_cap=192, reaction_cap=1024, capture_cap=1024;
constexpr std::uint32_t water=0x007f66;
void require(bool value,const char* message) { if (!value) throw std::runtime_error(message); }
CaveVictreebelIncarnation key(menu::animation::InstanceHandle h) { return {h.pool,h.slot,h.incarnation}; }
menu::animation::InstanceHandle handle(const CaveVictreebelIncarnation& k) {
    menu::animation::InstanceHandle result;
    result.pool=k[0]; result.slot=static_cast<std::uint32_t>(k[1]); result.incarnation=k[2]; return result;
}
float squared(const IsleGameVec3& a,const IsleGameVec3& b) {
    const float x=a.x-b.x,y=a.y-b.y,z=a.z-b.z; return x*x+y*y+z*z;
}
bool finite(const IsleGameVec3& p) { return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z); }
IsleGameVec3 global(const IsleGameVec3& p,const IsleGameVec3& origin) { return {p.x+origin.x,p.y+origin.y,p.z+origin.z}; }
std::uint32_t callback_identity(IsleGameSourceCallback callback) {
    if (callback==func_802BF894_641D44) return 0x802bf894;
    if (callback==func_802BF9B4_641E64) return 0x802bf9b4;
    if (callback==func_802BFA3C_641EEC) return 0x802bfa3c;
    if (callback==func_802BFB9C_64204C) return 0x802bfb9c;
    if (callback==func_802BFBF0_6420A0) return 0x802bfbf0;
    if (callback==func_802BFC84_642134) return 0x802bfc84;
    if (callback==func_802BFDE0_642290) return 0x802bfde0;
    if (callback==func_802BFE34_6422E4) return 0x802bfe34;
    if (callback==func_802BFEF0_6423A0) return 0x802bfef0;
    if (callback==func_802BFF74_642424) return 0x802bff74;
    if (callback==func_802C0058_642508) return 0x802c0058;
    return 0;
}
std::uint32_t process_identity(IsleGame* game,IsleGameProcess* process) {
    if (!process) return 0;
    IsleGameSourceCallback callback=nullptr;
    require(isle_game_process_source_function(game,process,&callback)==ISLE_GAME_OK,
            "Victreebel actual process identity rejected");
    return callback_identity(callback);
}
bool accepts_hit(const IsleActorSource& source) {
    if (!source.transitionGraph) return false;
    const auto identity=source.transitionGraph->source_identity;
    if (identity!=0x802c6ad8 && identity!=0x802c6aa8) return false;
    const auto callback=identity==0x802c6ad8 ? func_802BFA3C_641EEC : func_802BF894_641D44;
    for (std::size_t i=0;i<source.transitionGraph->count;++i) {
        const auto& row=source.transitionGraph->rows[i];
        if (row.cmd==58) break;
        if (row.cmd==9 && row.state==callback) return true;
    }
    return false;
}
void snapshot(Snapshot& out,const IslePlayerSource& player,actor::Owner& actors,items::Owner& items,
              const CaveVictreebelFacts& facts,const IsleGameVec3& origin) {
    out.parent={}; out.subject={}; out.subject_count=0; out.item_count=0; out.sphere_count=0;
    out.capacity=true; out.copied_context=false; out.taken_species=0; out.evaluator_slot=-1; out.taken_actor={};
    IslePlayerPhotoFocus focus{};
    require(isle_player_photo_published_focus(player.photo,&focus)==ISLE_GAME_OK,"Victreebel actual focus rejected");
    out.focus_species=focus.pokemon_id; out.focus_flags=focus.flags; out.focus_context=focus.context;
    out.focus_actor=key(actors.instance(focus.object));
    IsleGameObject* previous=nullptr;
    for (;;) {
        IsleGameObject* object=nullptr; IsleActorSource* source=nullptr; IsleGameModel* model=nullptr;
        require(isle_actor_source_link3_next(player.game,previous,&object,&source,&model)==ISLE_GAME_OK,
                "Victreebel actual Actor traversal rejected");
        if (!object) break;
        previous=object;
        const auto identity=key(actors.instance(object));
        require(identity[0]!=0,"Victreebel actual Actor incarnation unavailable");
        if (source->tangible && source->collisionRadius>0) {
            if (out.sphere_count==out.spheres.size()) out.capacity=false;
            else out.spheres[out.sphere_count++]={identity,source->collPosition,source->collisionRadius};
        }
        if (!model || (source->id!=70 && source->id!=71)) continue;
        const IsleGameModelPlacement* placement=nullptr;
        require(isle_game_model_placement(player.game,model,&placement)==ISLE_GAME_OK && placement,
                "Victreebel actual Actor placement rejected");
        if (placement->mode==ISLE_GAME_MODEL_PLACEMENT_DISABLED) continue;
        IsleGameObjectMetadata metadata{};
        require(isle_game_object_metadata(player.game,object,&metadata)==ISLE_GAME_OK,"Victreebel Actor metadata rejected");
        Actor actor;
        actor.present=true; actor.identity=identity; actor.object=object; actor.interaction=source->interactionTarget;
        actor.root=placement->transform.position; actor.global_root=global(actor.root,origin);
        actor.collision=source->collPosition; actor.global_collision=global(actor.collision,origin);
        actor.radius=source->collisionRadius; actor.flags=source->flags; actor.tangible=source->tangible!=0;
        actor.graph=source->transitionGraph ? source->transitionGraph->source_identity : 0;
        actor.state=process_identity(player.game,source->stateProc); actor.path=process_identity(player.game,source->pathProc);
        actor.process_flags=source->processFlags; actor.object_flags=metadata.flags; actor.object_id=metadata.id;
        actor.surface=source->currGround.surfaceType; actor.ground_height=source->currGround.height;
        actor.facing=source->facingYaw; actor.hspeed=source->hSpeed; actor.jump=source->jumpVel;
        actor.empty_forbidden=source->forbiddenGround && source->forbiddenGroundCount>0 && source->forbiddenGround[0]==0.0f;
        const float pitch=std::atan2(actor.collision.y-player.CameraEyePos.y,
            std::hypot(actor.collision.x-player.CameraEyePos.x,actor.collision.z-player.CameraEyePos.z));
        actor.eligible=actor.tangible && !(metadata.flags&1u) && (source->flags&4u) && !(source->flags&8u) &&
            source->playerDist>=0 && source->playerDist<=10000 && pitch>=player.MinPitch && pitch<=player.MaxPitch;
        actor.focused=actor.eligible && focus.present && focus.object==object && focus.pokemon_id==source->id &&
            (focus.flags&4u) && !(focus.flags&8u);
        actor.ready=accepts_hit(*source) && actor.tangible && !(metadata.flags&1u) && actor.radius>0 &&
            finite(actor.root) && finite(actor.collision) && actor.empty_forbidden;
        actor.distance=squared(actor.collision,player.CameraEyePos);
        if (source->id==70) {
            if (identity==facts.parent || (!facts.parent[0] && actor.ready &&
                (!out.parent.present || actor.distance<out.parent.distance))) out.parent=actor;
        } else {
            if (out.subject_count==out.subjects.size()) out.capacity=false;
            else out.subjects[out.subject_count++]=actor;
            if (identity==facts.subject) out.subject=actor;
        }
    }
    if (!facts.subject[0] && out.parent.present) {
        float nearest=0;
        for (std::size_t i=0;i<out.subject_count;++i) {
            const auto& candidate=out.subjects[i];
            if (candidate.graph!=0x802c6c10 || !(candidate.object_flags&1u) || candidate.tangible) continue;
            const float distance=squared(candidate.root,out.parent.root);
            if (!out.subject.present || distance<nearest) { out.subject=candidate; nearest=distance; }
        }
    }
    const int taken=player.photo->PokemonDetector_TakenPhotoIndex;
    if (taken>=0 && taken<2) {
        const auto& context=player.photo->PokemonDetector_Photo[taken];
        out.taken_actor=key(actors.instance(context.pokemonInFocus));
        for (std::size_t i=0;i<12;++i) {
            if (!context.pokemonInFocus || context.pokemonObjects[i]!=context.pokemonInFocus) continue;
            out.taken_species=context.main.pokemons[i].pokemonID;
            // Exposure validation deliberately has no post-shutter eligibility guard.
            if (facts.subject[0] && out.taken_actor==facts.subject && out.taken_species==71 &&
                context.pokemonInFocus==reinterpret_cast<IsleGameObject*>(facts.subject_object)) {
                out.evaluator_slot=static_cast<int>(i);
                const int count=player.photo->gPhotoCount;
                if (count>0 && count<=60) out.copied_context=
                    private_course::photo_context_copied(player.photo->D_800B0598[count-1],context,i);
            }
        }
    }
    for (const auto& binding:items.render_bindings()) {
        if (!binding.model.object) continue;
        const auto instance=items.instance(binding.model.object);
        const auto family=items.family(instance);
        if (!family || (*family!=items::Family::pester_ball && *family!=items::Family::apple)) continue;
        const auto* source=items.source(binding.model.object);
        require(source && bool(instance),"Victreebel actual Item source/incarnation unavailable");
        if (out.item_count==out.items.size()) { out.capacity=false; break; }
        const IsleGameModelNode* nodes=nullptr; std::size_t count=0;
        require(isle_game_model_peek_nodes(player.game,binding.model.model,&nodes,&count)==ISLE_GAME_OK && nodes && count,
                "Victreebel actual Item root rejected");
        IsleGameObjectMetadata metadata{};
        require(isle_game_object_metadata(player.game,binding.model.object,&metadata)==ISLE_GAME_OK,"Victreebel Item metadata rejected");
        out.items[out.item_count++]={key(instance),binding.model.object,nodes[0].transform.position,source->collisionVelocity,
            source->flags,metadata.flags,metadata.id,source->itemID,source->state,source->restTimer};
    }
}
float intersection(const IsleGameVec3& start,const IsleGameVec3& delta,const IsleGameVec3& center,float radius) {
    const float x=center.x-start.x,y=center.y-start.y,z=center.z-start.z;
    const float length=std::hypot(delta.x,delta.y,delta.z);
    if (!(length>0) || !(radius>0)) return 2;
    const float projection=(x*delta.x+y*delta.y+z*delta.z)/length;
    const float perpendicular=x*x+y*y+z*z-projection*projection;
    if (!(projection>0) || perpendicular>radius*radius) return 2;
    const float entry=(projection-std::sqrt(std::max(0.0f,radius*radius-perpendicular)))/length;
    return entry<1 ? entry : 2;
}
// Pinned pokemon.c1121-1226: nonNULL empty forbidden list; destination query
// survives the independent radius-ahead query; horizontal, then y, then gravity.
bool knock_forecast(const world::World& world,const Snapshot& observed,CaveVictreebelForecast& receipt) {
    const auto& parent=observed.parent;
    if (!parent.empty_forbidden) { receipt.disposition=ForecastDisposition::forbidden_ground_unavailable; return false; }
    auto position=receipt.hit_root;
    const float yaw=std::atan2(receipt.incoming_velocity.x,receipt.incoming_velocity.z);
    const float speed=static_cast<float>(200.0f*0.033);
    const float vx=std::sin(yaw)*speed,vz=std::cos(yaw)*speed;
    const float norm=std::sqrt(vx*vx+vz*vz);
    if (!(norm>0) || !std::isfinite(norm)) { receipt.disposition=ForecastDisposition::invalid_launch; return false; }
    const float ahead_x=vx/norm*parent.radius,ahead_z=vz/norm*parent.radius;
    float jump=static_cast<float>(300.0f*0.033);
    float height=parent.ground_height;
    std::uint32_t surface=parent.surface;
    for (unsigned step=0;step<256;++step) {
        receipt.knock_steps=step+1;
        if (!(parent.process_flags&POKEMON_PROCESS_FLAG_MOVEMENT_PAUSED)) {
            world::GroundResult next{},ahead{};
            const float nx=position.x+vx,nz=position.z+vz;
            if (!world.ground_at(nx,nz,next) || !world.ground_at(position.x+ahead_x,position.z+ahead_z,ahead)) {
                receipt.disposition=ForecastDisposition::ground_unavailable; return false;
            }
            // Both list scans are empty: no default water exclusion applies.
            height=next.height; surface=static_cast<std::uint32_t>(next.surface);
            position.x=nx; position.z=nz;
        }
        position.y+=jump;
        jump+=static_cast<float>(-9.8f*0.033);
        if (!finite(position) || !std::isfinite(height)) { receipt.disposition=ForecastDisposition::invalid_launch; return false; }
        if (jump<0.0f && position.y<height) {
            position.y=height; receipt.landing=position; receipt.ground_height=height;
            receipt.ground_surface=static_cast<int>(surface);
            receipt.landing_distance=std::sqrt(squared(position,observed.subject.root));
            if (surface!=water) { receipt.disposition=ForecastDisposition::wrong_surface; return false; }
            if (!(receipt.landing_distance<=500.0f)) { receipt.disposition=ForecastDisposition::receiver_distance; return false; }
            receipt.disposition=ForecastDisposition::accepted; return true;
        }
    }
    receipt.disposition=ForecastDisposition::knock_limit; return false;
}
// Prospective input gate only: no forecast establishes any real receipt.
// Cave installs default synchronous Item movement and no custom collision hook.
bool forecast(const world::World& world,const IslePlayerSource& player,const Snapshot& observed,
              const IsleGameVec3& ray,const IsleGameVec3& motion,CaveVictreebelForecast& receipt) {
    receipt={}; receipt.aimed=ray; receipt.target=observed.parent.collision;
    const auto reject=[&receipt](ForecastDisposition result) { receipt.disposition=result; return false; };
    const IsleGameCameraParameters* camera=nullptr;
    require(isle_game_camera_parameters(player.game,player.camera,&camera)==ISLE_GAME_OK && camera,"Victreebel launch camera unavailable");
    IsleGameVec3 position{camera->look_at.eye[0],camera->look_at.eye[1],camera->look_at.eye[2]};
    IsleGameVec3 velocity{ray.x-position.x,ray.y-position.y+100.0f,ray.z-position.z};
    const float length=std::hypot(velocity.x,velocity.y,velocity.z);
    if (!(length>0) || !std::isfinite(length)) return reject(ForecastDisposition::invalid_launch);
    velocity={velocity.x*50/length+player.PlayerVelocity.x,velocity.y*50/length+player.PlayerVelocity.y,
        velocity.z*50/length+player.PlayerVelocity.z};
    for (unsigned step=0;step<160;++step) {
        receipt.flight_step=step; receipt.position=position;
        if (!finite(position)) return reject(ForecastDisposition::invalid_launch);
        if (position.x<=-10000 || position.x>=10000 || position.y<=-10000 || position.y>=10000 ||
            position.z<=-10000 || position.z>=10000) return reject(ForecastDisposition::bounds);
        world::GroundResult ground{},ceiling{};
        if (!world.ground_at(position.x,position.z,ground)) return reject(ForecastDisposition::ground_unavailable);
        receipt.ground_height=ground.height; receipt.ground_surface=ground.surface;
        if (world.ceiling_at(position.x,position.z,ceiling) && position.y>ceiling.height)
            return reject(ForecastDisposition::ceiling_contact);
        if (position.y<=ground.height+.375f) return reject(ForecastDisposition::ground_contact);
        const IsleGameVec3 delta{velocity.x*.5f,velocity.y*.5f,velocity.z*.5f};
        world::Vec3 hit{},reflected{};
        receipt.static_object=world.collide({position.x,position.y,position.z},{delta.x,delta.y,delta.z},hit,reflected);
        if (receipt.static_object>=0) return reject(ForecastDisposition::static_hit);
        float nearest=1; bool selected=false;
        for (std::size_t i=0;i<observed.sphere_count;++i) {
            const auto& sphere=observed.spheres[i]; auto center=sphere.center;
            const bool target=sphere.identity==observed.parent.identity;
            if (target) {
                const float elapsed=static_cast<float>(step+1);
                center={center.x+motion.x*elapsed,center.y+motion.y*elapsed,center.z+motion.z*elapsed};
            }
            const float entry=intersection(position,delta,center,sphere.radius);
            if (target) receipt.target_entry=entry;
            if (entry<nearest) {
                nearest=entry; selected=target; receipt.blocker=sphere.identity; receipt.blocker_center=center;
                receipt.blocker_radius=sphere.radius; receipt.entry=entry;
            }
        }
        if (nearest<1) {
            if (!selected) return reject(ForecastDisposition::competing_actor);
            receipt.flight=static_cast<float>(step)+std::max(0.0f,nearest);
            receipt.incoming_velocity=velocity;
            const float elapsed=static_cast<float>(step+1);
            receipt.hit_root={observed.parent.root.x+motion.x*elapsed,observed.parent.root.y+motion.y*elapsed,
                observed.parent.root.z+motion.z*elapsed};
            return knock_forecast(world,observed,receipt);
        }
        velocity.y-=.6f;
        position={position.x+velocity.x*.5f,position.y+velocity.y*.5f,position.z+velocity.z*.5f};
    }
    return reject(ForecastDisposition::target_missed);
}
bool plan(const world::World& world,const IslePlayerSource& player,const Snapshot& observed,
          CaveVictreebelFacts& facts,IsleGameVec3& aimed) {
    facts.plan_count=0;
    // Bounded target-sphere offsets, each with one observed-motion refinement.
    constexpr std::array<std::array<float,2>,9> offsets{{{0,0},{-.55f,0},{.55f,0},{0,-.55f},{0,.55f},
        {-.4f,-.4f},{-.4f,.4f},{.4f,-.4f},{.4f,.4f}}};
    const float dx=observed.parent.collision.x-player.CameraEyePos.x;
    const float dz=observed.parent.collision.z-player.CameraEyePos.z;
    const float horizontal=std::hypot(dx,dz);
    if (!(horizontal>0)) return false;
    for (const auto& offset:offsets) {
        float elapsed=0;
        for (unsigned refinement=0;refinement<2;++refinement) {
            auto& receipt=facts.plan_forecasts[facts.plan_count++];
            const float lateral=offset[0]*observed.parent.radius;
            const IsleGameVec3 target{observed.parent.collision.x+dz/horizontal*lateral+facts.motion.x*elapsed,
                observed.parent.collision.y+offset[1]*observed.parent.radius+facts.motion.y*elapsed,
                observed.parent.collision.z-dx/horizontal*lateral+facts.motion.z*elapsed};
            if (!projectile_target(player,target,aimed)) {
                receipt={}; receipt.target=target; receipt.disposition=ForecastDisposition::inverse_rejected; break;
            }
            if (forecast(world,player,observed,aimed,facts.motion,receipt)) { receipt.target=target; return true; }
            receipt.target=target;
            elapsed=receipt.flight>0 ? receipt.flight+1 : horizontal/20.0f+1;
        }
    }
    return false;
}
} // namespace

CaveVictreebelPhotoPuzzle::CaveVictreebelPhotoPuzzle(private_post_course::Controller controller,Aim aim)
    :controller_(controller),aim_(aim) {
    require(controller_.set && aim_.point,"Victreebel photos require real pad and pure aim bridges");
}
void CaveVictreebelPhotoPuzzle::publish(std::uint16_t buttons,std::int8_t x,std::int8_t y) noexcept {
    controller_.set(controller_.owner,buttons,x,y); facts_.owns_pad=true; pad_published_=buttons || x || y;
}
void CaveVictreebelPhotoPuzzle::stop(Disposition disposition) noexcept {
    if (facts_.exhausted || facts_.complete) return;
    facts_.exhausted=true; facts_.capture_pending=false; facts_.disposition=disposition; facts_.phase=Phase::finished;
    pending_press_=false; tracking_=false; capture_armed_=false; neutral_due_=true;
}
void CaveVictreebelPhotoPuzzle::observe(const application::RuntimeView& view,std::uint64_t updates) {
    if (sampled_ && updates==previous_update_) return;
    require(!sampled_ || updates>previous_update_,"Victreebel completed-update ordinal regressed");
    const bool consecutive=sampled_ && updates==previous_update_+1;
    const bool had_sample=sampled_;
    sampled_=true; previous_update_=updates; facts_.last_update=updates; facts_.owns_pad=false; observation_active_=false;
    if (view.cave) {
        facts_.source_status=view.cave->adapter().source_status(); facts_.host_status=view.cave->error();
        if (const auto* host=view.cave->native_host()) if (host->error()!=ISLE_GAME_OK) facts_.host_status=host->error();
        if (facts_.source_status!=ISLE_GAME_OK || facts_.host_status!=ISLE_GAME_OK) {
            stop(Disposition::source_error);
            throw std::runtime_error("Victreebel actual Cave Source/host failed");
        }
    }
    if (!view.cave || view.cave->status()!=course::LifecycleStatus::running) {
        if (started_) stop(Disposition::course_ended);
        scene_=nullptr; parent_object_=nullptr; subject_object_=nullptr; capture_armed_=false; return;
    }
    const auto& entry=*view.cave;
    const auto* host=entry.native_host(); const auto* player=entry.adapter().native.player();
    if (!host || !host->execution_open() || !player) return;
    require(player->status==ISLE_GAME_OK && player->photo && player->photo->status==ISLE_GAME_OK,"Victreebel actual Player/Photo failed");
    if (scene_ && scene_!=player->game) { stop(Disposition::scene_changed); parent_object_=nullptr; subject_object_=nullptr; return; }
    scene_=player->game;
    auto* actors=host->actors(); auto* items=host->items_owner(); auto* world=host->world_owner();
    require(actors && items && world,"Victreebel actual Actor/Item/World unavailable");
    require(world->error().empty(),"Victreebel actual World failed");
    const auto* block=world->current();
    if (!block || !block->resource || world->retired()) {
        if (started_) stop(Disposition::course_ended);
        parent_object_=nullptr; subject_object_=nullptr; capture_armed_=false; return;
    }
    if (started_ && had_sample && !consecutive) stop(Disposition::observation_gap);
    const auto offset=block->resource->metadata.world_position;
    const IsleGameVec3 origin{offset.x*100,offset.y*100,offset.z*100};
    snapshot(snapshot_,*player,*actors,*items,facts_,origin); observation_active_=true;
    const auto& observed=snapshot_; const auto& parent=observed.parent; const auto& subject=observed.subject;
    const auto& photo=*player->photo;
    facts_.current_block=static_cast<int>(block->index); facts_.film=photo.gPhotoCount; facts_.qualified=photo.D_800AE27C;
    require(facts_.film>=0 && facts_.film<=60 && facts_.qualified>=0 && facts_.qualified<=facts_.film,"Victreebel actual roll counters invalid");
    facts_.focus_species=observed.focus_species; facts_.focus_context=observed.focus_context; facts_.focus_flags=observed.focus_flags;
    facts_.taken_context=photo.PokemonDetector_TakenPhotoIndex; facts_.taken_species=observed.taken_species;
    facts_.evaluator_slot=observed.evaluator_slot; facts_.focus_actor=observed.focus_actor; facts_.taken_actor=observed.taken_actor;
    facts_.player_ready=player->D_80382D04_523114==1 && !player->IsInputDisabled && !player->IsPaused;
    require(entry.game_state().save(),"Victreebel canonical Save unavailable");
    const auto pester=entry.game_state().save()->player_flag(1);
    require(bool(pester),"Victreebel actual Pester reward unavailable"); facts_.available=facts_.pester_available=pester.value;
    if (!started_ && facts_.player_ready && facts_.available) { started_=true; facts_.initial_update=updates; }
    if (observed_roll_<0) { observed_roll_=facts_.film; observed_qualified_=facts_.qualified; }
    require(facts_.film>=observed_roll_ && facts_.qualified>=observed_qualified_,"Victreebel actual roll counters regressed");
    const bool one_exposure=facts_.film==observed_roll_+1 && facts_.qualified==observed_qualified_+1;
    for (int slot=observed_roll_;slot<facts_.film;++slot) {
        const auto& captured=photo.D_800B0598[slot]; facts_.capture_observed=true; facts_.raw_primary=captured.pokemons[0].pokemonID;
        if (facts_.raw_primary!=71) continue;
        ++facts_.captures;
        const bool coherent=!facts_.exhausted && capture_armed_ && consecutive && one_exposure &&
            armed_subject_==facts_.subject && armed_slot_==slot && armed_qualified_+1==facts_.qualified &&
            facts_.command31_seen && facts_.parent_cleanup_seen && facts_.release_update>=facts_.consumed_update &&
            (player->input.pressed_buttons&a_button) && (player->input.current_buttons&a_button) &&
            facts_.taken_context>=0 && facts_.taken_context<2 && observed.copied_context &&
            observed.taken_actor==armed_subject_ && captured.levelID==4 && (captured.unk_00_16&0xe0u)==0x20u;
        if (coherent) {
            facts_.complete=true; facts_.capture_pending=false; facts_.captured_slot=slot; facts_.capture_update=updates;
            facts_.captured_evaluator_slot=observed.evaluator_slot; facts_.captured_actor=armed_subject_;
            facts_.captured_slots|=std::uint64_t{1}<<static_cast<unsigned>(slot);
            std::array<std::byte,save::game_photo_bytes> bytes{};
            facts_.captured_identity_hash=private_course::photo_identity(captured,bytes);
            facts_.phase=Phase::finished; facts_.disposition=Disposition::qualified;
            if (pad_published_) neutral_due_=true;
        } else { ++facts_.capture_mismatches; ++facts_.mismatches; }
    }
    observed_roll_=facts_.film; observed_qualified_=facts_.qualified; capture_armed_=false;
    facts_.target_present=parent.present; facts_.target_ready=parent.ready; facts_.motion={};
    if (parent.present) {
        facts_.root=parent.root; facts_.global_root=parent.global_root; facts_.collision=parent.collision;
        facts_.collision_radius=parent.radius; facts_.target_graph=parent.graph; facts_.target_state=parent.state;
        facts_.target_path=parent.path; facts_.target_flags=parent.flags; facts_.target_object_flags=parent.object_flags;
        facts_.target_process_flags=parent.process_flags; facts_.target_object_id=parent.object_id;
        facts_.hspeed=parent.hspeed; facts_.jump_velocity=parent.jump; facts_.facing_yaw=parent.facing;
        if (consecutive && previous_parent_==parent.identity)
            facts_.motion={parent.global_collision.x-previous_global_collision_.x,parent.global_collision.y-previous_global_collision_.y,
                parent.global_collision.z-previous_global_collision_.z};
    }
    facts_.subject_present=subject.present; facts_.subject_eligible=subject.eligible; facts_.subject_focused=subject.focused;
    if (subject.present) {
        facts_.subject_root=subject.root; facts_.subject_graph=subject.graph; facts_.subject_state=subject.state;
        facts_.subject_path=subject.path; facts_.subject_flags=subject.flags; facts_.subject_object_flags=subject.object_flags;
        facts_.subject_object_id=subject.object_id; facts_.subject_hidden=(subject.object_flags&1u)!=0;
        facts_.subject_visible=!(subject.object_flags&1u) && subject.tangible; facts_.subject_flag4=(subject.flags&4u)!=0;
    } else { facts_.subject_hidden=false; facts_.subject_visible=false; facts_.subject_flag4=false; }
    if (pending_press_ && (player->input.current_buttons&b_button)) {
        const bool pressed=(player->input.pressed_buttons&b_button)!=0;
        if (pressed) ++facts_.pressed;
        if (pressed && player->ThrowItemTimeout>0 && player->LastItemId==ISLE_ITEM_PESTER_BALL &&
            player->gDirectionIndex>=0 && !player->TargetDirectionZoomedIn && player->TurnToDirSpeed==0) {
            ++facts_.consumed; facts_.consumed_update=updates; tracking_=true; attempt_start_=updates;
        } else { ++facts_.mismatches; stop(Disposition::press_timeout); }
        pending_press_=false; pending_release_=true; release_start_=updates;
    }
    if (pending_press_ && updates-facts_.request_update>=8) {
        pending_press_=false; pending_release_=true; release_start_=updates; stop(Disposition::press_timeout);
    }
    if (pending_release_ && !(player->input.current_buttons&b_button)) {
        pending_release_=false; ++facts_.releases; facts_.release_update=updates;
    }
    if (pending_release_ && updates-release_start_>=8) { pending_release_=false; stop(Disposition::release_timeout); }
    if (!observed.capacity) stop(Disposition::capacity);
    if (started_ && updates-facts_.initial_update>=overall_cap) stop(Disposition::overall_limit);
    if (!facts_.exhausted && !facts_.complete && tracking_) {
        const Item* item=nullptr; unsigned matches=0;
        for (std::size_t i=0;i<observed.item_count;++i) {
            const auto& candidate=observed.items[i]; bool existed=false;
            for (std::size_t j=0;j<prethrow_count_;++j) existed |= prethrow_items_[j]==candidate.identity;
            if (candidate.kind!=ISLE_ITEM_PESTER_BALL || (facts_.item[0] ? candidate.identity!=facts_.item : existed)) continue;
            item=&candidate; ++matches;
        }
        facts_.item_present=matches==1;
        if (matches>1) stop(Disposition::hit_unattributed);
        if (matches==1) {
            facts_.item=item->identity; facts_.item_object=reinterpret_cast<std::uintptr_t>(item->object);
            facts_.item_position=item->position; facts_.item_kind=item->kind; facts_.item_state=item->state;
            facts_.item_flags=item->flags; facts_.item_object_flags=item->object_flags; facts_.item_object_id=item->object_id;
            facts_.item_hidden=(item->object_flags&1u)!=0;
            facts_.interaction_matches=parent.present && parent.interaction==item->object;
            if (parent.present && parent.state==0x802bf894 && !previous_knock_) {
                const bool accepted=consecutive && previous_parent_==facts_.parent && facts_.interaction_matches &&
                    facts_.item_hidden && !(item->flags&(ISLE_ITEM_TOUCHED_GROUND|ISLE_ITEM_DELETED)) &&
                    facts_.consumed==1 && facts_.consumed_update>=facts_.request_update;
                if (!accepted) stop(Disposition::hit_unattributed);
                else {
                    facts_.collision_velocity=item->collision_velocity;
                    facts_.incoming_yaw=std::atan2(item->collision_velocity.x,item->collision_velocity.z);
                    if (!finite(item->collision_velocity) || !(std::hypot(item->collision_velocity.x,item->collision_velocity.z)>0))
                        stop(Disposition::heading_unattributed);
                    else {
                        tracking_=false; facts_.hit_seen=true; facts_.hit_update=updates;
                        facts_.accepted_item=item->identity; ++facts_.accepted_hits; facts_.phase=Phase::knock;
                    }
                }
            }
        }
        if (tracking_) {
            ++facts_.flight_updates; facts_.phase=Phase::flight;
            if (updates-attempt_start_>=flight_cap) stop(Disposition::flight_timeout);
        }
    }
    if (!facts_.exhausted && !facts_.complete && facts_.hit_seen) {
        if (!subject.present || subject.identity!=facts_.subject) stop(Disposition::actor_lost);
        // StartPathProc may create its stackful continuation after the sampled
        // state transfer. Heading is a separate actual-path witness, never a prediction.
        if (!facts_.heading_seen && parent.present && parent.path==0x802bf9b4 && parent.hspeed==200.0f) {
            facts_.heading_error=std::remainder(parent.facing-facts_.incoming_yaw,6.2831853071795864769f);
            if (!std::isfinite(facts_.heading_error) || std::abs(facts_.heading_error)>.01f)
                stop(Disposition::heading_unattributed);
            else facts_.heading_seen=true;
        }
        if (!facts_.heading_seen && updates-facts_.hit_update>=8) stop(Disposition::heading_unattributed);
        if (parent.present && facts_.heading_seen && !facts_.landing_seen && parent.surface==water &&
            ((parent.state==0x802bf894 && !parent.path && (parent.process_flags&POKEMON_PROCESS_FLAG_PATH_ENDED)) ||
                parent.graph==0x802c6b08 || parent.state==0x802bfbf0)) {
            facts_.landing_seen=true; facts_.landing_update=updates; facts_.actual_surface=static_cast<int>(parent.surface);
            facts_.actual_landing=parent.root; facts_.actual_distance=std::sqrt(squared(parent.root,subject.root));
        }
        if (parent.present && !facts_.landing_seen && parent.state==0x802bf894 && !parent.path &&
            (parent.process_flags&POKEMON_PROCESS_FLAG_PATH_ENDED) && parent.surface!=water) stop(Disposition::wrong_landing);
        // The wait31 graph proves the distance guard and send30 body executed.
        // A sampled handler29 body can also prove it with its actual root distance.
        if (!facts_.command29_seen && subject.present &&
            (subject.state==0x802bfe34 || subject.graph==0x802c6c30)) {
            const float distance=parent.present ? std::sqrt(squared(parent.root,subject.root)) : 100000.0f;
            if (!facts_.landing_seen || subject.interaction!=parent_object_ || !parent.present || !(distance<=500.0f))
                stop(Disposition::handshake_unattributed);
            else {
                facts_.command29_seen=true; facts_.command29_update=updates; facts_.sender29=facts_.parent;
                facts_.actual_distance=distance; facts_.phase=Phase::handshake;
            }
        }
        if (!facts_.command30_seen && parent.present && parent.state==0x802bfbf0) {
            if (!facts_.command29_seen || parent.interaction!=subject_object_) stop(Disposition::handshake_unattributed);
            else { facts_.command30_seen=true; facts_.command30_update=updates; facts_.sender30=facts_.subject; }
        }
        if (!facts_.command31_seen && subject.present && (subject.state==0x802bfef0 || subject.path==0x802bff74)) {
            if (!facts_.command30_seen || subject.interaction!=parent_object_) stop(Disposition::handshake_unattributed);
            else { facts_.command31_seen=true; facts_.command31_update=updates; facts_.sender31=facts_.parent; }
        }
        if (facts_.command31_seen && subject.present && facts_.subject_visible && !facts_.reveal_seen) {
            facts_.reveal_seen=true; facts_.reveal_update=updates; facts_.phase=Phase::reveal;
        }
        if (!parent.present && !facts_.parent_cleanup_seen) {
            const auto membership=actors->membership(handle(facts_.parent));
            if (!facts_.command31_seen || !facts_.command30_seen || membership.logical ||
                (membership.registered && !membership.retiring && !membership.joined)) stop(Disposition::actor_lost);
            else { facts_.parent_cleanup_seen=true; facts_.cleanup_update=updates; }
        }
        // Source22 clears the field after cleanup. The previously latched31
        // association is the receipt; no dead sender is dereferenced/reacquired.
        if (facts_.parent_cleanup_seen && subject.present && !subject.interaction) facts_.target_death_cleared=true;
        if (facts_.reveal_seen && facts_.subject_flag4 && !facts_.flag4_update) facts_.flag4_update=updates;
        if (!facts_.exhausted && facts_.parent_cleanup_seen && facts_.reveal_seen && facts_.subject_flag4 && subject.eligible) {
            if (!capture_start_) capture_start_=updates;
            facts_.capture_pending=true; facts_.phase=Phase::capture; facts_.disposition=Disposition::capture_pending;
            if (pad_published_) neutral_due_=true;
        }
        if (capture_start_ && updates-capture_start_>=capture_cap) stop(Disposition::capture_timeout);
        if (!capture_start_ && updates-facts_.hit_update>=reaction_cap) stop(Disposition::reaction_timeout);
    }
    if (!facts_.complete && !facts_.exhausted && facts_.capture_pending && subject.focused && facts_.player_ready &&
        player->gDirectionIndex==-1 && !player->TargetDirectionZoomedIn && player->sTimerAfterPhotoTaken<0 &&
        !(player->input.current_buttons&a_button) && facts_.film<60 && facts_.releases==1) {
        capture_armed_=true; armed_subject_=facts_.subject; armed_slot_=facts_.film; armed_qualified_=facts_.qualified;
    }
    previous_parent_=parent.identity; previous_knock_=parent.present && parent.state==0x802bf894;
    if (parent.present) previous_global_collision_=parent.global_collision;
}
bool CaveVictreebelPhotoPuzzle::update(const application::RuntimeView& view,std::uint64_t updates) {
    observe(view,updates);
    if (control_sampled_ && control_update_==updates) return facts_.owns_pad;
    control_sampled_=true; control_update_=updates; facts_.owns_pad=false;
    if (neutral_due_) { neutral_due_=false; publish(); return true; }
    if (!observation_active_ || !view.cave) return false;
    const auto* player=view.cave->adapter().native.player(); const auto* host=view.cave->native_host();
    const auto* world=host ? host->world_owner() : nullptr;
    if (!player || !world || world->retired()) return false;
    if (pending_press_ || pending_release_) {
        facts_.phase=pending_press_ ? Phase::press : Phase::release;
        publish(pending_press_ && !facts_.exhausted ? b_button : 0); return true;
    }
    if (facts_.complete || facts_.exhausted || facts_.capture_pending) return false;
    if (!facts_.available) { facts_.disposition=Disposition::pester_unavailable; return false; }
    if (!facts_.player_ready) { facts_.disposition=Disposition::player_not_ready; return false; }
    if (tracking_ || facts_.hit_seen) {
        facts_.disposition=tracking_ ? Disposition::item_outstanding : Disposition::none; publish(); return true;
    }
    if (facts_.film>=59) { facts_.disposition=Disposition::film_reserved; return false; }
    const auto& parent=snapshot_.parent; const auto& subject=snapshot_.subject;
    if (!parent.present) { facts_.disposition=Disposition::target_absent; return false; }
    if (!parent.ready) { facts_.disposition=Disposition::target_not_ready; return false; }
    if (!subject.present || subject.graph!=0x802c6c10 || !(subject.object_flags&1u) || subject.tangible) {
        facts_.disposition=Disposition::hidden_subject_absent; return false;
    }
    if (facts_.requests>=1) { stop(Disposition::request_limit); publish(); neutral_due_=false; return true; }
    if (!player->ui || player->ui->Icons_NumItemsAvailable<2) { facts_.disposition=Disposition::item_ui_not_ready; return false; }
    if ((player->input.current_buttons&a_button) || player->sTimerAfterPhotoTaken>=0) return false;
    if (player->ThrowItemTimeout!=0) {
        facts_.disposition=Disposition::item_outstanding;
        if (facts_.parent[0]) { publish(); return true; }
        return false;
    }
    if (!facts_.parent[0]) {
        IsleGameVec3 aimed{};
        if (!plan(*world,*player,snapshot_,facts_,aimed)) {
            ++facts_.arc_rejections; facts_.disposition=Disposition::unsafe_geometry; return false;
        }
        facts_.parent=parent.identity; facts_.subject=subject.identity;
        parent_object_=parent.object; subject_object_=subject.object;
        facts_.parent_object=reinterpret_cast<std::uintptr_t>(parent_object_);
        facts_.subject_object=reinterpret_cast<std::uintptr_t>(subject_object_);
        facts_.preplaced_seen=true; facts_.preplaced_update=updates;
    }
    if (player->gDirectionIndex<0 || player->TargetDirectionZoomedIn || player->TurnToDirSpeed!=0) {
        if (++recovery_>128) { stop(Disposition::recovery_limit); publish(); neutral_due_=false; return true; }
        ++facts_.recovery_updates; facts_.phase=Phase::unzoom;
        std::uint16_t buttons=0;
        if (player->ZoomSwitchMode==1 && player->gDirectionIndex==-1 && !player->TargetDirectionZoomedIn &&
            !zoom_out_requested_ && !(player->input.current_buttons&z_button)) { buttons=z_button; zoom_out_requested_=true; }
        publish(buttons); return true;
    }
    zoom_out_requested_=false;
    if (++steering_>512) { stop(Disposition::steering_limit); publish(); neutral_due_=false; return true; }
    ++facts_.steering_updates;
    IsleGameVec3 aimed{};
    if (!plan(*world,*player,snapshot_,facts_,aimed)) {
        ++facts_.arc_rejections; facts_.disposition=Disposition::unsafe_geometry; publish(); return true;
    }
    std::int8_t x=0,y=0; aim_.point(aim_.owner,*player,aimed,false,x,y);
    facts_.input_target=aimed; facts_.phase=Phase::aim; facts_.disposition=Disposition::none;
    std::uint16_t buttons=0;
    // Quantized actual camera ray must pass both physical forecasts again.
    if (!x && !y && !(player->input.current_buttons&b_button) &&
        forecast(*world,*player,snapshot_,player->cam_target,facts_.motion,facts_.actual_forecast)) {
        bool outstanding=false;
        for (std::size_t i=0;i<snapshot_.item_count;++i) {
            const auto& item=snapshot_.items[i];
            outstanding |= item.kind==ISLE_ITEM_PESTER_BALL && item.state==ISLE_ITEM_FLYING &&
                (!(item.object_flags&1u) || item.rest_timer<=0) && !(item.flags&ISLE_ITEM_DELETED);
        }
        if (!outstanding) {
            prethrow_count_=snapshot_.item_count;
            for (std::size_t i=0;i<prethrow_count_;++i) prethrow_items_[i]=snapshot_.items[i].identity;
            facts_.request_update=updates; ++facts_.requests; pending_press_=true; facts_.phase=Phase::press; buttons=b_button;
        } else facts_.disposition=Disposition::item_outstanding;
    }
    publish(buttons,x,y); return true;
}
} // namespace isle::private_campaign

#include "native_valley_ground_reveal_photo.hpp"
#include "captured_photo_identity.hpp"
#include "application_runtime.hpp"
#include "valley_scene.hpp"
#include "game_state.hpp"
#include "actor_owner.hpp"
#include "actor_owner_source.h"
#include "valley_actor_source.h"
#include "item_owner.hpp"
#include "player_ui_source.h"
#include "world_runtime.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace isle::private_campaign {
namespace {
using Key=ValleyGroundRevealIncarnation;
using Facts=ValleyGroundRevealFacts;
using Receipt=ValleyGroundRevealThrowReceipt;
using Forecast=ValleyGroundRevealForecast;
using FD=ValleyGroundRevealForecastDisposition;
using Disposition=ValleyGroundRevealDisposition;
using Phase=ValleyGroundRevealPhase;
using valley_ground_reveal_detail::Actor;
using valley_ground_reveal_detail::Item;
using valley_ground_reveal_detail::Snapshot;
constexpr std::uint16_t b_button=0x4000,z_button=0x2000,a_button=0x8000;
constexpr std::uint64_t overall_cap=8192,flight_cap=192,reaction_cap=1024,capture_cap=1024;
constexpr float pi=3.14159265358979323846f;
// Authority: df8815e75f3d79618466e01e0827c6165b93d11c, ROM SHA256
// a1d5d816db7f8557ee04c35a011326d058b2c1fbca76b57b352b1d705a1ec1cc.
// 7A2DD0.c94-124: real cmd9 -> backward bounce ->20; behavior2 then sends38.
// 7A1850.c54-144: registered behavior1 Graveler, BB4->BD4->8650/8704->39.
// 7A3C50.c71-101 / 7A7790.c55-87: 16 BURROWS, hidden20/39 SHOWS.
// pokemon.c1070-1119: original bounce; 1952-1968: only20 has root3D<600.
// All physics here is prospective. Receiver transitions witness acceptance,
// not observation of a command emission. This controller never calls Source.
void require(bool v,const char* m) { if (!v) throw std::runtime_error(m); }
Key key(menu::animation::InstanceHandle h) { return {h.pool,h.slot,h.incarnation}; }
float squared(IsleGameVec3 a,IsleGameVec3 b) {
    const float x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;return x*x+y*y+z*z;
}
bool finite(IsleGameVec3 p) { return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z); }
std::uint32_t process_id(IsleGame* game,IsleGameProcess* process) {
    if (!process) return 0;
    IsleGameSourceCallback f=nullptr;
    require(isle_game_process_source_function(game,process,&f)==ISLE_GAME_OK,"Ground reveal actual Source process unavailable");
    if (f==func_802C9B78_7A3108) return 0x802c9b78;
    if (f==func_802C9BD0_7A3160) return 0x802c9bd0;
    if (f==func_802C9C84_7A3214) return 0x802c9c84;
    if (f==func_802C9DC4_7A3354) return 0x802c9dc4;
    if (f==func_802C853C_7A1ACC) return 0x802c853c;
    if (f==func_802C8594_7A1B24) return 0x802c8594;
    if (f==func_802C8650_7A1BE0) return 0x802c8650;
    if (f==func_802C8704_7A1C94) return 0x802c8704;
    if (f==func_802CA8F8_7A3E88) return 0x802ca8f8;
    if (f==func_802CA948_7A3ED8) return 0x802ca948;
    if (f==func_802CE37C_7A790C) return 0x802ce37c;
    if (f==func_802CE3CC_7A795C) return 0x802ce3cc;
    return 0;
}
const Actor* find(const Snapshot& s,Key identity) {
    if (!identity[0]) return nullptr;
    for (std::size_t i=0;i<s.actor_count;++i) if (s.actors[i].identity==identity) return &s.actors[i];
    return nullptr;
}
bool hidden(const Actor& a,int wanted) {
    return a.species==wanted && a.graph==(wanted==27?0x802d32b4u:0x802d3dc4u) &&
        (a.object_flags&1u) && !a.tangible;
}
void snapshot(Snapshot& s,const IslePlayerSource& p,actor::Owner& actors,items::Owner& items,
              Key registered,const Facts& facts) {
    s.actor_count=s.item_count=s.sphere_count=0;s.capacity=true;s.copied=false;
    s.taken_actor={};s.taken_species=0;s.evaluator_slot=-1;
    IslePlayerPhotoFocus focus{};
    require(isle_player_photo_published_focus(p.photo,&focus)==ISLE_GAME_OK,"Ground reveal actual focus unavailable");
    s.focus_actor=key(actors.instance(focus.object));s.focus_species=focus.pokemon_id;
    s.focus_context=focus.context;s.focus_flags=focus.flags;
    IsleGameObject* previous=nullptr;
    std::array<unsigned,4> counts{};
    for (;;) {
        IsleGameObject* object=nullptr;IsleActorSource* source=nullptr;IsleGameModel* model=nullptr;
        require(isle_actor_source_link3_next(p.game,previous,&object,&source,&model)==ISLE_GAME_OK,
            "Ground reveal actual Actor traversal rejected");
        if (!object) break;
        previous=object;const auto identity=key(actors.instance(object));
        require(identity[0]!=0,"Ground reveal Actor incarnation unavailable");
        if (source->tangible && source->collisionRadius>0) {
            if (s.sphere_count==s.spheres.size()) s.capacity=false;
            else s.spheres[s.sphere_count++]={identity,source->collPosition,source->collisionRadius};
        }
        if (source->id!=74 && source->id!=27 && source->id!=28 && !(source->id==75 && identity==registered)) continue;
        const unsigned group=source->id==74?0u:source->id==75?1u:source->id==27?2u:3u;
        constexpr std::array<unsigned,4> limits{5,1,3,1};
        if (++counts[group]>limits[group] || s.actor_count==s.actors.size()) { s.capacity=false;continue; }
        require(model,"Ground reveal actual Actor model unavailable");
        const IsleGameModelPlacement* placement=nullptr;
        require(isle_game_model_placement(p.game,model,&placement)==ISLE_GAME_OK && placement &&
            placement->mode!=ISLE_GAME_MODEL_PLACEMENT_DISABLED,"Ground reveal Actor root unavailable");
        IsleGameObjectMetadata metadata{};
        require(isle_game_object_metadata(p.game,object,&metadata)==ISLE_GAME_OK,"Ground reveal Actor metadata rejected");
        Actor a{};a.identity=identity;a.object=object;a.species=source->id;a.behavior=source->behavior;
        a.actor_sender=key(actors.instance(source->interactionTarget));a.item_sender=key(items.instance(source->interactionTarget));
        a.root=placement->transform.position;a.yaw=placement->transform.rotation.y;
        a.collision=source->collPosition;a.radius=source->collisionRadius;a.tangible=source->tangible!=0;
        a.graph=source->transitionGraph?source->transitionGraph->source_identity:0;
        a.state=process_id(p.game,source->stateProc);a.path=process_id(p.game,source->pathProc);
        a.flags=source->flags;a.object_flags=metadata.flags;a.process_flags=source->processFlags;
        a.ground=source->currGround.height;a.surface=source->currGround.surfaceType;
        a.ready=a.species==74 && a.graph==0x802d2ef8 && a.tangible && !(metadata.flags&1u) && a.radius>0 && finite(a.root);
        if (a.ready) {
            a.ready=false;
            for (std::size_t i=0;i<source->transitionGraph->count;++i) {
                const auto& row=source->transitionGraph->rows[i];
                if (row.cmd==58) break;
                if (row.cmd==9 && row.state==func_802C9BD0_7A3160) a.ready=true;
            }
        }
        const float pitch=std::atan2(a.collision.y-p.CameraEyePos.y,
            std::hypot(a.collision.x-p.CameraEyePos.x,a.collision.z-p.CameraEyePos.z));
        a.eligible=a.tangible && !(metadata.flags&1u) && (a.flags&4u) && !(a.flags&8u) &&
            source->playerDist>=0 && source->playerDist<=10000 && pitch>=p.MinPitch && pitch<=p.MaxPitch;
        a.focused=a.eligible && focus.present && focus.object==object && focus.pokemon_id==a.species &&
            (focus.flags&4u) && !(focus.flags&8u);
        s.actors[s.actor_count++]=a;
    }
    const int taken=p.photo->PokemonDetector_TakenPhotoIndex;
    if (taken>=0 && taken<2) {
        const auto& context=p.photo->PokemonDetector_Photo[taken];
        s.taken_actor=key(actors.instance(context.pokemonInFocus));
        for (std::size_t i=0;i<12;++i) if (context.pokemonInFocus && context.pokemonObjects[i]==context.pokemonInFocus) {
            s.taken_species=context.main.pokemons[i].pokemonID;s.evaluator_slot=static_cast<int>(i);
            const int count=p.photo->gPhotoCount;
            if (facts.subject[0] && s.taken_actor==facts.subject && s.taken_species==facts.required_species && count>0 && count<=60)
                s.copied=private_course::photo_context_copied(p.photo->D_800B0598[count-1],context,i);
        }
    }
    for (const auto& binding:items.render_bindings()) {
        if (!binding.model.object) continue;
        const auto instance=items.instance(binding.model.object);const auto family=items.family(instance);
        if (!family || (*family!=items::Family::pester_ball && *family!=items::Family::apple)) continue;
        const auto* source=items.source(binding.model.object);
        require(source && bool(instance),"Ground reveal actual Item incarnation unavailable");
        if (s.item_count==s.items.size()) { s.capacity=false;break; }
        const IsleGameModelNode* nodes=nullptr;std::size_t count=0;
        require(isle_game_model_peek_nodes(p.game,binding.model.model,&nodes,&count)==ISLE_GAME_OK && nodes && count,
            "Ground reveal actual Item position unavailable");
        IsleGameObjectMetadata metadata{};
        require(isle_game_object_metadata(p.game,binding.model.object,&metadata)==ISLE_GAME_OK,"Ground reveal Item metadata rejected");
        s.items[s.item_count++]={key(instance),nodes[0].transform.position,source->flags,metadata.flags,
            source->itemID,source->state,source->restTimer};
    }
}
// Separate from Cave's Item-heading knock. Source auxiliary priority6 executes
// its first turn before the new priority5 bounce reads model yaw.
bool bounce_forecast(const world::World& world,const Actor& a,const Actor* receiver,Forecast& out) {
    auto pos=a.root;const float speed=static_cast<float>(200.0f*0.033);
    constexpr float tau=2*pi, turn_speed=pi/45.0f;
    float target_yaw=a.yaw+pi;
    target_yaw-=static_cast<float>(static_cast<int>(target_yaw/tau))*tau;
    float yaw=a.yaw;
    yaw-=static_cast<float>(static_cast<int>(yaw/tau))*tau;
    if(yaw<0)yaw+=tau;
    const float delta=target_yaw-yaw;
    if(std::abs(delta)>turn_speed) {
        float sign=delta>0?1.0f:delta<0?-1.0f:0.0f;
        if(!(std::abs(delta)<pi))sign=-sign; // Original PI tie turns the other way.
        yaw+=sign*turn_speed;
    } else yaw=target_yaw;
    out.hit_root=pos;out.yaw=yaw+pi;
    if (out.yaw<0) out.yaw+=tau;
    else if (out.yaw>tau) out.yaw-=static_cast<float>(static_cast<int>(out.yaw/tau))*tau;
    IsleGameVec3 v{std::sin(out.yaw)*speed,speed,std::cos(out.yaw)*speed};
    for (unsigned step=0;step<256;++step) {
        world::GroundResult ground{};out.bounce_steps=step+1;
        if (!finite(pos) || !finite(v)) { out.disposition=FD::invalid_launch;return false; }
        if (!world.ground_at(pos.x,pos.z,ground)) { out.disposition=FD::ground_unavailable;return false; }
        if (pos.y<ground.height) {
            const auto n=ground.normal;const float length=std::hypot(n.x,n.y,n.z);
            if (!(length>0) || !std::isfinite(length)) { out.disposition=FD::ground_unavailable;return false; }
            const float cosine=std::clamp(n.y/length,-1.0f,1.0f);
            if (std::acos(cosine)<pi/3.0f) {
                pos.y=ground.height;out.landing=pos;out.normal={n.x,n.y,n.z};out.surface=ground.surface;
                if (receiver) {
                    out.receiver_distance=std::sqrt(squared(pos,receiver->root));
                    if (!(out.receiver_distance<600.0f)) { out.disposition=FD::receiver_distance;return false; }
                }
                out.disposition=FD::accepted;return true;
            }
            const float current_speed=std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
            if (!(current_speed>0) || !std::isfinite(current_speed)) { out.disposition=FD::invalid_launch;return false; }
            IsleGameVec3 direction{v.x/current_speed,v.y/current_speed,v.z/current_speed};
            const float twice_dot=2*(direction.x*n.x+direction.y*n.y+direction.z*n.z);
            direction={direction.x-twice_dot*n.x,direction.y-twice_dot*n.y,direction.z-twice_dot*n.z};
            v={direction.x*current_speed,direction.y*current_speed,direction.z*current_speed};++out.reflections;
        }
        v.y-=.6f;pos={pos.x+v.x,pos.y+v.y,pos.z+v.z};
    }
    out.disposition=FD::bounce_limit;return false;
}
float intersection(IsleGameVec3 start,IsleGameVec3 delta,IsleGameVec3 center,float radius) {
    const float x=center.x-start.x,y=center.y-start.y,z=center.z-start.z;
    const float length=std::hypot(delta.x,delta.y,delta.z);
    if (!(length>0) || !(radius>0)) return 2;
    const float projection=(x*delta.x+y*delta.y+z*delta.z)/length;
    const float perpendicular=x*x+y*y+z*z-projection*projection;
    if (!(projection>0) || perpendicular>radius*radius) return 2;
    const float entry=(projection-std::sqrt(std::max(0.0f,radius*radius-perpendicular)))/length;
    return entry<1?entry:2;
}
// Original Item first-collision gate, using the shared inverse and real World
// collision primitives. Geodudes are planned only in their stationary9 graph.
bool forecast(const world::World& world,const IslePlayerSource& p,const Snapshot& s,const Actor& target,
              const Actor* receiver,IsleGameVec3 ray,Forecast& out) {
    out={};out.aimed=ray;
    const auto reject=[&out](FD why) { out.disposition=why;return false; };
    const IsleGameCameraParameters* camera=nullptr;
    require(isle_game_camera_parameters(p.game,p.camera,&camera)==ISLE_GAME_OK && camera,"Ground reveal launch camera unavailable");
    IsleGameVec3 pos{camera->look_at.eye[0],camera->look_at.eye[1],camera->look_at.eye[2]};
    IsleGameVec3 v{ray.x-pos.x,ray.y-pos.y+100,ray.z-pos.z};
    const float length=std::hypot(v.x,v.y,v.z);
    if (!(length>0) || !finite(v)) return reject(FD::invalid_launch);
    v={v.x*50/length+p.PlayerVelocity.x,v.y*50/length+p.PlayerVelocity.y,v.z*50/length+p.PlayerVelocity.z};
    for (unsigned step=0;step<192;++step) {
        if (!finite(pos)) return reject(FD::invalid_launch);
        if (std::abs(pos.x)>=10000 || std::abs(pos.y)>=10000 || std::abs(pos.z)>=10000) return reject(FD::bounds);
        world::GroundResult ground{},ceiling{};
        if (!world.ground_at(pos.x,pos.z,ground)) return reject(FD::ground_unavailable);
        if (world.ceiling_at(pos.x,pos.z,ceiling) && pos.y>ceiling.height) return reject(FD::ceiling_contact);
        if (pos.y<=ground.height+.375f) return reject(FD::ground_contact);
        const IsleGameVec3 delta{v.x*.5f,v.y*.5f,v.z*.5f};world::Vec3 hit{},reflected{};
        out.static_object=world.collide({pos.x,pos.y,pos.z},{delta.x,delta.y,delta.z},hit,reflected);
        if (out.static_object>=0) return reject(FD::static_hit);
        float nearest=2;Key selected{};
        for (std::size_t i=0;i<s.sphere_count;++i) {
            const auto& sphere=s.spheres[i];const float entry=intersection(pos,delta,sphere.center,sphere.radius);
            if (entry<nearest) { nearest=entry;selected=sphere.identity;out.blocker_center=sphere.center; }
        }
        if (nearest<1) {
            out.flight=static_cast<float>(step)+std::max(0.0f,nearest);
            if (selected!=target.identity) { out.blocker=selected;return reject(FD::competing_actor); }
            return bounce_forecast(world,target,receiver,out);
        }
        v.y-=.6f;pos={pos.x+v.x*.5f,pos.y+v.y*.5f,pos.z+v.z*.5f};
    }
    return reject(FD::target_missed);
}
bool plan(const world::World& world,const IslePlayerSource& p,const Snapshot& s,const Actor& a,
          const Actor* receiver,IsleGameVec3& aimed,Forecast& receipt) {
    constexpr std::array<std::array<float,2>,9> offsets{{{0,0},{-.55f,0},{.55f,0},{0,-.55f},{0,.55f},
        {-.4f,-.4f},{-.4f,.4f},{.4f,-.4f},{.4f,.4f}}};
    const float dx=a.collision.x-p.CameraEyePos.x,dz=a.collision.z-p.CameraEyePos.z;
    const float length=std::hypot(dx,dz);if (!(length>0)) return false;
    for (const auto& offset:offsets) {
        const float lateral=offset[0]*a.radius;
        const IsleGameVec3 point{a.collision.x+dz/length*lateral,a.collision.y+offset[1]*a.radius,a.collision.z-dx/length*lateral};
        if (!projectile_target(p,point,aimed)) { receipt={};receipt.disposition=FD::inverse_rejected;continue; }
        if (forecast(world,p,s,a,receiver,aimed,receipt)) return true;
    }
    return false;
}
bool ended(const Actor& a) { return !a.path && (a.process_flags&POKEMON_PROCESS_FLAG_PATH_ENDED); }
}

ValleyGroundRevealPhotoPuzzle::ValleyGroundRevealPhotoPuzzle(private_post_course::Controller controller,Aim aim,int wanted)
    :controller_(controller),aim_(aim) {
    require(controller_.set && aim_.point,"Ground reveal requires real pad and pure aim bridges");
    require(wanted==27 || wanted==28,"Ground reveal objective must be27 or28");facts_.required_species=wanted;
}
void ValleyGroundRevealPhotoPuzzle::publish(std::uint16_t buttons,std::int8_t x,std::int8_t y) noexcept {
    controller_.set(controller_.owner,buttons,x,y);facts_.owns_pad=true;pad_published_=buttons || x || y;
}
void ValleyGroundRevealPhotoPuzzle::stop(Disposition why) noexcept {
    if (facts_.exhausted || facts_.complete) return;
    facts_.exhausted=true;facts_.capture_pending=false;facts_.offered_subject={};facts_.offered_subject_object=0;
    facts_.phase=Phase::finished;facts_.disposition=why;capture_armed_=false;pending_press_=false;neutral_due_=true;
}
void ValleyGroundRevealPhotoPuzzle::observe(const application::RuntimeView& view,std::uint64_t updates) {
    if (sampled_ && updates==previous_update_) return;
    require(!sampled_ || updates>previous_update_,"Ground reveal completed-update ordinal regressed");
    const bool had_sample=sampled_,consecutive=sampled_ && updates==previous_update_+1;
    sampled_=true;previous_update_=updates;facts_.last_update=updates;facts_.owns_pad=false;active_=false;
    if (view.valley) {
        facts_.source_status=view.valley->adapter().source_status();facts_.host_status=view.valley->error();
        const auto* host=view.valley->native_host();if (host && host->error()!=ISLE_GAME_OK) facts_.host_status=host->error();
        if (facts_.source_status!=ISLE_GAME_OK || facts_.host_status!=ISLE_GAME_OK) {
            stop(Disposition::source_error);
            const char* message=host && host->error()!=ISLE_GAME_OK?host->message():view.valley->message();
            throw std::runtime_error(message && *message?message:"Ground reveal actual Source execution failed");
        }
    }
    if (!view.valley || view.valley->status()!=course::LifecycleStatus::running) {
        if (started_) stop(Disposition::course_ended);
        return;
    }
    const auto& entry=*view.valley;const auto* host=entry.native_host();const auto* p=entry.adapter().native.player();
    if (!host || !host->execution_open() || !p) return;
    require(p->status==ISLE_GAME_OK && p->photo && p->photo->status==ISLE_GAME_OK,"Ground reveal Player/Photo failed");
    if (scene_ && scene_!=p->game) { stop(Disposition::scene_changed);return; }scene_=p->game;
    auto* actors=host->actors();auto* items=host->items_owner();const auto* world=host->world_owner();
    require(actors && items && world && world->error().empty(),"Ground reveal Actor/Item/World unavailable");
    const auto* block=world->current();
    if (!block || !block->resource || world->retired()) { if (started_) stop(Disposition::course_ended);return; }
    if (started_ && had_sample && !consecutive) stop(Disposition::observation_gap);
    facts_.registered_graveler=key(actors->instance(entry.actors_source().D_802D2B8C_7AC11C));
    snapshot(snapshot_,*p,*actors,*items,facts_.registered_graveler,facts_);active_=true;
    const auto& s=snapshot_;const auto& photo=*p->photo;
    facts_.current_block=static_cast<int>(block->index);facts_.block_progress=p->movement.move_time;
    facts_.film=photo.gPhotoCount;facts_.qualified=photo.D_800AE27C;
    require(facts_.film>=0 && facts_.film<=60 && facts_.qualified>=0 && facts_.qualified<=facts_.film,"Ground reveal roll counters invalid");
    facts_.focus_actor=s.focus_actor;facts_.focus_species=s.focus_species;facts_.focus_context=s.focus_context;
    facts_.taken_actor=s.taken_actor;facts_.taken_species=s.taken_species;facts_.taken_context=photo.PokemonDetector_TakenPhotoIndex;
    facts_.evaluator_slot=s.evaluator_slot;facts_.full_context_copied=s.copied;
    facts_.player_ready=p->D_80382D04_523114==1 && !p->IsInputDisabled && !p->IsPaused;
    require(entry.game_state().save(),"Ground reveal canonical Save unavailable");
    const auto reward=entry.game_state().save()->player_flag(1);require(bool(reward),"Ground reveal Pester reward unavailable");
    facts_.available=facts_.pester_available=reward.value;
    if (!started_ && facts_.available && facts_.player_ready) { started_=true;facts_.initial_update=updates; }
    if (observed_roll_<0) { observed_roll_=facts_.film;observed_qualified_=facts_.qualified; }
    require(facts_.film>=observed_roll_ && facts_.qualified>=observed_qualified_,"Ground reveal roll counters regressed");
    if (facts_.complete) {
        std::array<std::byte,save::game_photo_bytes> bytes{};
        require(facts_.captured_slot>=0 && facts_.captured_slot<facts_.film &&
            private_course::photo_identity(photo.D_800B0598[facts_.captured_slot],bytes)==facts_.captured_identity_hash,
            "Ground reveal frozen captured identity changed");
    }
    for (int slot=observed_roll_;slot<facts_.film;++slot) {
        const auto& image=photo.D_800B0598[slot];facts_.capture_observed=true;facts_.raw_primary=image.pokemons[0].pokemonID;
        if (facts_.raw_primary!=facts_.required_species) continue;
        ++facts_.captures;
        const bool accepted=!facts_.exhausted && !facts_.complete && facts_.receiver_acceptance_seen && facts_.reveal_seen &&
            capture_armed_ && consecutive && facts_.film==observed_roll_+1 && facts_.qualified==observed_qualified_+1 &&
            armed_slot_==slot && armed_qualified_+1==facts_.qualified && armed_subject_==facts_.subject && s.taken_actor==armed_subject_ &&
            facts_.taken_context>=0 && facts_.taken_context<2 && s.copied && image.levelID==5 && (image.unk_00_16&0xe0u)==0x20u &&
            (p->input.pressed_buttons&a_button) && (p->input.current_buttons&a_button) && facts_.consumed==facts_.requests &&
            facts_.releases==facts_.consumed && facts_.requests==static_cast<std::uint64_t>(facts_.required_species==27?1:2);
        if (accepted) {
            facts_.complete=true;facts_.capture_pending=false;facts_.capture_update=updates;facts_.captured_slot=slot;
            facts_.captured_actor=s.taken_actor;facts_.captured_evaluator_slot=s.evaluator_slot;
            facts_.captured_slots|=std::uint64_t{1}<<static_cast<unsigned>(slot);
            facts_.captured_context=photo.PokemonDetector_Photo[facts_.taken_context];facts_.captured_image=image;
            std::array<std::byte,save::game_photo_bytes> bytes{};facts_.captured_identity_hash=private_course::photo_identity(image,bytes);
            facts_.offered_subject={};facts_.offered_subject_object=0;
            facts_.phase=Phase::finished;facts_.disposition=Disposition::qualified;if (pad_published_) neutral_due_=true;
        } else ++facts_.capture_mismatches;
    }
    observed_roll_=facts_.film;observed_qualified_=facts_.qualified;capture_armed_=false;
    auto& current=facts_.throws[facts_.active_throw];
    if (pending_press_ && (p->input.current_buttons&b_button)) {
        const bool pressed=(p->input.pressed_buttons&b_button)!=0;if (pressed) ++facts_.pressed;
        if (pressed && p->ThrowItemTimeout>0 && p->LastItemId==ISLE_ITEM_PESTER_BALL && p->gDirectionIndex>=0 &&
            !p->TargetDirectionZoomedIn && p->TurnToDirSpeed==0) { ++facts_.consumed;current.consumed_update=updates; }
        else stop(Disposition::press_timeout);
        pending_press_=false;pending_release_=true;release_start_=updates;
    }
    if (pending_press_ && updates-current.request_update>=8) {
        pending_press_=false;pending_release_=true;release_start_=updates;stop(Disposition::press_timeout);
    }
    if (pending_release_ && !(p->input.current_buttons&b_button)) {
        pending_release_=false;++facts_.releases;current.release_update=updates;
    }
    if (pending_release_ && updates-release_start_>=8) { pending_release_=false;stop(Disposition::release_timeout); }
    if (!s.capacity) stop(Disposition::capacity);
    if (started_ && updates-facts_.initial_update>=overall_cap) stop(Disposition::overall_limit);
    const Actor* subject=find(s,facts_.subject);const Actor* graveler=find(s,facts_.graveler);
    facts_.subject_present=subject!=nullptr;facts_.subject_hidden=subject && hidden(*subject,facts_.required_species);
    facts_.subject_visible=subject && subject->tangible && !(subject->object_flags&1u);
    facts_.subject_eligible=subject && subject->eligible;facts_.subject_focused=subject && subject->focused;
    if (subject) { facts_.subject_root=subject->root;facts_.subject_graph=subject->graph;facts_.subject_state=subject->state;
        facts_.subject_flags=subject->flags;facts_.subject_object_flags=subject->object_flags; }
    if (graveler) { facts_.graveler_graph=graveler->graph;facts_.graveler_state=graveler->state;
        facts_.graveler_path=graveler->path;facts_.graveler_root=graveler->root; }
    if (!facts_.complete && !facts_.exhausted && facts_.subject[0]) {
        if (!subject || (facts_.required_species==28 && (!graveler || facts_.registered_graveler!=facts_.graveler))) stop(Disposition::actor_lost);
        const float progress=static_cast<float>(facts_.current_block)+facts_.block_progress;
        if (facts_.required_species==27 && progress>=2.1f && !facts_.reveal_seen) stop(Disposition::window_ended);
        for (unsigned index=0;index<(facts_.required_species==27?1u:2u);++index) {
            auto& r=facts_.throws[index];const auto* actor=find(s,r.actor);
            if (!r.actor[0]) continue;
            if (!r.landing_seen && (!actor || progress>=(r.behavior==1?1.85f:2.4f))) { stop(Disposition::window_ended);break; }
            if (!actor) continue;
            if (index==facts_.active_throw) {
                facts_.target_present=true;facts_.target_ready=actor->ready;facts_.root=actor->root;
                facts_.target_graph=actor->graph;facts_.target_state=actor->state;facts_.target_path=actor->path;
                facts_.target_flags=actor->flags;facts_.target_object_flags=actor->object_flags;
            }
            if (r.consumed_update && !r.hit_seen) {
                const Item* item=nullptr;unsigned matches=0;
                for (std::size_t i=0;i<s.item_count;++i) {
                    const auto& candidate=s.items[i];bool existed=false;
                    for (std::size_t j=0;j<prethrow_count_;++j) existed |= prethrow_items_[j]==candidate.identity;
                    if (candidate.kind!=ISLE_ITEM_PESTER_BALL || (r.item[0]?candidate.identity!=r.item:existed)) continue;
                    item=&candidate;++matches;
                }
                if (matches>1) { stop(Disposition::hit_unattributed);break; }
                const bool was_present=r.item_present;r.item_present=matches==1;
                if (item) {
                    r.item=item->identity;r.item_seen=true;r.item_hidden=(item->object_flags&1u)!=0;
                    r.item_position=item->position;r.item_flags=item->flags;r.item_object_flags=item->object_flags;
                    r.item_kind=item->kind;r.item_state=item->state;
                } else if (was_present) r.item_removed=true;
                r.interaction_matches=r.item[0] && actor->item_sender==r.item;
                if (actor->state==0x802c9bd0 || actor->path==0x802c9c84) {
                    if (!consecutive || previous_target_graph_[index]!=0x802d2ef8 || !r.interaction_matches ||
                        !r.item_seen || (!r.item_hidden && !r.item_removed) || (r.item_flags&ISLE_ITEM_TOUCHED_GROUND)) {
                        stop(Disposition::hit_unattributed);break;
                    }
                    r.hit_seen=true;r.hit_update=updates;r.accepted_item=r.item;r.hit_root=actor->root;
                    r.hit_state=actor->state;r.hit_path=actor->path;facts_.phase=Phase::bounce;
                } else { ++facts_.flight_updates;if (updates-r.consumed_update>=flight_cap) stop(Disposition::flight_timeout); }
            }
            if (r.hit_seen && !r.bounce_seen && actor->path==0x802c9c84) {
                r.bounce_seen=true;r.bounce_update=updates;r.bounce_root=actor->root;r.bounce_yaw=actor->yaw;
            }
            if (r.bounce_seen && !r.landing_seen && ended(*actor) &&
                (actor->state==0x802c9bd0 || actor->state==0x802c9dc4)) {
                r.landing_seen=true;r.landing_update=updates;r.landing=actor->root;r.actual_surface=actor->surface;
                r.landing_state=actor->state;r.landing_path=actor->path;facts_.command20_source_derived=true;
                if (r.behavior==2) facts_.command38_source_derived=true;
            }
            if (r.hit_seen && !r.landing_seen && updates-r.hit_update>=reaction_cap) stop(Disposition::reaction_timeout);
            previous_target_graph_[index]=actor->graph;
        }
        if (!facts_.exhausted && facts_.required_species==28 && graveler) {
            auto& first=facts_.throws[0];auto& second=facts_.throws[1];
            if (!facts_.first38_accepted && graveler->graph==0x802d2bd4 && graveler->state==0x802c8594) {
                if (!consecutive || previous_graveler_graph_!=0x802d2bb4 || !first.landing_seen || graveler->actor_sender!=first.actor)
                    stop(Disposition::handshake_unattributed);
                else {
                    facts_.first38_accepted=true;facts_.first38_update=updates;facts_.sender38_first=first.actor;
                    facts_.first38_before_graph=previous_graveler_graph_;facts_.first38_after_graph=graveler->graph;
                    facts_.first38_root=graveler->root;facts_.phase=Phase::handshake;
                }
            }
            if (!facts_.second38_accepted && (graveler->state==0x802c8650 || graveler->path==0x802c8704)) {
                if (!consecutive || !facts_.first38_accepted || previous_graveler_graph_!=0x802d2bd4 ||
                    !second.landing_seen || graveler->actor_sender!=second.actor || second.actor==first.actor)
                    stop(Disposition::handshake_unattributed);
                else {
                    facts_.second38_accepted=true;facts_.second38_update=updates;facts_.sender38_second=second.actor;
                    facts_.second38_before_graph=previous_graveler_graph_;facts_.second38_after_state=graveler->state;
                    facts_.second38_after_path=graveler->path;facts_.second38_root=graveler->root;
                }
            }
            if (facts_.first38_accepted && !facts_.second38_accepted && graveler->graph!=0x802d2bd4) stop(Disposition::window_ended);
            if (facts_.first38_accepted && !pending_release_ && first.release_update && !second.request_update) facts_.active_throw=1;
            if (facts_.second38_accepted && !facts_.graveler_bounce_seen && graveler->path==0x802c8704) {
                facts_.graveler_bounce_seen=true;facts_.graveler_bounce_update=updates;
                facts_.graveler_bounce_root=graveler->root;facts_.graveler_bounce_yaw=graveler->yaw;
            }
            if (facts_.graveler_bounce_seen && !facts_.graveler_landing_seen && ended(*graveler) && graveler->state==0x802c8650) {
                facts_.graveler_landing_seen=true;facts_.graveler_landing_update=updates;facts_.graveler_landing=graveler->root;
                facts_.graveler_landing_surface=graveler->surface;
                facts_.command39_source_derived=true;
            }
            if (facts_.second38_accepted && !facts_.reveal_seen && updates-facts_.second38_update>=reaction_cap) stop(Disposition::reaction_timeout);
        }
        if (!facts_.exhausted && subject && !facts_.receiver_acceptance_seen &&
            subject->state==(facts_.required_species==27?0x802ca948u:0x802ce3ccu)) {
            const auto* sender=facts_.required_species==27?find(s,facts_.throws[0].actor):graveler;
            const bool landed=facts_.required_species==27?facts_.throws[0].landing_seen:facts_.graveler_landing_seen;
            const float distance=sender?std::sqrt(squared(sender->root,subject->root)):0;
            if (!consecutive || previous_subject_graph_!=(facts_.required_species==27?0x802d32b4u:0x802d3dc4u) ||
                !landed || !sender || subject->actor_sender!=sender->identity || (facts_.required_species==27 && !(distance<600.0f)))
                stop(Disposition::handshake_unattributed);
            else {
                facts_.receiver_acceptance_seen=true;facts_.actual_receiver_distance=distance;
                facts_.receiver_before_graph=previous_subject_graph_;facts_.receiver_after_state=subject->state;
                facts_.receiver_sender_root=sender->root;facts_.receiver_root=subject->root;
                if (facts_.required_species==27) { facts_.sender20=sender->identity;facts_.throws[0].receiver_distance=distance; }
                else facts_.sender39=sender->identity;
            }
        }
        if (!facts_.exhausted && facts_.receiver_acceptance_seen && facts_.subject_visible && !facts_.reveal_seen) {
            facts_.reveal_seen=true;facts_.reveal_update=updates;
        }
        if (!facts_.exhausted && facts_.reveal_seen && subject && subject->eligible) {
            if (!capture_start_) capture_start_=updates;
            facts_.capture_pending=true;facts_.offered_subject=subject->identity;
            facts_.offered_subject_object=reinterpret_cast<std::uintptr_t>(subject->object);
            facts_.phase=Phase::capture;facts_.disposition=Disposition::capture_pending;if (pad_published_) neutral_due_=true;
        }
        if (capture_start_ && updates-capture_start_>=capture_cap) stop(Disposition::capture_timeout);
        if (!capture_start_ && facts_.throws[0].consumed_update && updates-facts_.throws[0].consumed_update>=reaction_cap)
            stop(Disposition::reaction_timeout);
    }
    if (!facts_.complete && !facts_.exhausted && facts_.capture_pending && subject && subject->focused && facts_.player_ready &&
        p->gDirectionIndex==-1 && !p->TargetDirectionZoomedIn && p->sTimerAfterPhotoTaken<0 &&
        !(p->input.current_buttons&a_button) && facts_.film<60 && facts_.releases==facts_.requests) {
        capture_armed_=true;armed_subject_=subject->identity;armed_slot_=facts_.film;armed_qualified_=facts_.qualified;
    }
    previous_subject_graph_=subject?subject->graph:0;previous_graveler_graph_=graveler?graveler->graph:0;
}

bool ValleyGroundRevealPhotoPuzzle::update(const application::RuntimeView& view,std::uint64_t updates) {
    observe(view,updates);
    if (control_sampled_ && control_update_==updates) return facts_.owns_pad;
    control_sampled_=true;control_update_=updates;facts_.owns_pad=false;
    if (neutral_due_) { neutral_due_=false;publish();return true; }
    if (!active_ || !view.valley) return false;
    const auto* p=view.valley->adapter().native.player();const auto* host=view.valley->native_host();
    const auto* world=host?host->world_owner():nullptr;if (!p || !world || world->retired()) return false;
    if (pending_press_ || pending_release_) {
        facts_.phase=pending_press_?Phase::press:Phase::release;publish(pending_press_ && !facts_.exhausted?b_button:0);return true;
    }
    if (facts_.complete || facts_.exhausted || facts_.capture_pending) return false;
    if (!facts_.available) { facts_.disposition=Disposition::pester_unavailable;return false; }
    if (!facts_.player_ready) { facts_.disposition=Disposition::player_not_ready;return false; }
    auto& r=facts_.throws[facts_.active_throw];
    if (r.consumed_update) {
        facts_.phase=r.hit_seen?Phase::bounce:Phase::flight;facts_.disposition=Disposition::item_outstanding;publish();return true;
    }
    if (facts_.film>=59) { facts_.disposition=Disposition::film_reserved;return false; }
    if (!p->ui || p->ui->Icons_NumItemsAvailable<2) { facts_.disposition=Disposition::item_ui_not_ready;return false; }
    if ((p->input.current_buttons&a_button) || p->sTimerAfterPhotoTaken>=0) return false;
    if (p->ThrowItemTimeout!=0) {
        facts_.disposition=Disposition::item_outstanding;if (facts_.subject[0]) { publish();return true; }return false;
    }
    const auto& s=snapshot_;
    if (!facts_.subject[0]) {
        const Actor* graveler=find(s,facts_.registered_graveler);
        if (facts_.required_species==28 && (!graveler || graveler->species!=75 || graveler->behavior!=1 || graveler->graph!=0x802d2bb4)) {
            facts_.disposition=Disposition::receiver_not_ready;return false;
        }
        if (facts_.required_species==28) {
            facts_.graveler_forecast={};
            if (!bounce_forecast(*world,*graveler,nullptr,facts_.graveler_forecast)) {
                ++facts_.arc_rejections;facts_.disposition=Disposition::unsafe_geometry;return false;
            }
        }
        bool bound=false;
        // Prefer behavior1 for27; behavior2 remains legal if its actual landing
        // forecast satisfies the SAME hidden receiver's strict3D distance gate.
        for (int behavior=1;behavior<=2 && !bound;++behavior) {
            if (facts_.required_species==28 && behavior!=2) continue;
            for (std::size_t i=0;i<s.actor_count && !bound;++i) {
                const auto& target=s.actors[i];if (!target.ready || target.behavior!=behavior) continue;
                for (std::size_t j=0;j<s.actor_count && !bound;++j) {
                    const auto& subject=s.actors[j];if (!hidden(subject,facts_.required_species)) continue;
                    IsleGameVec3 aimed{};Forecast receipt{};
                    if (!plan(*world,*p,s,target,facts_.required_species==27?&subject:nullptr,aimed,receipt)) {
                        ++facts_.arc_rejections;r.forecast=receipt;continue;
                    }
                    const Actor* second=nullptr;Forecast second_forecast{};
                    if (facts_.required_species==28) {
                        // Plan both real preplaced senders before spending the
                        // first Item. No retries or later same-sender recycling.
                        for (std::size_t k=0;k<s.actor_count;++k) {
                            const auto& other=s.actors[k];IsleGameVec3 second_aim{};
                            if (other.identity!=target.identity && other.ready && other.behavior==2 &&
                                plan(*world,*p,s,other,nullptr,second_aim,second_forecast)) { second=&other;break; }
                        }
                        if (!second) continue;
                    }
                    facts_.subject=subject.identity;r.actor=target.identity;r.behavior=target.behavior;r.ready_graph=target.graph;
                    r.forecast=receipt;previous_target_graph_[0]=target.graph;previous_subject_graph_=subject.graph;
                    if (second) {
                        facts_.throws[1].actor=second->identity;facts_.throws[1].behavior=second->behavior;
                        facts_.throws[1].ready_graph=second->graph;facts_.throws[1].forecast=second_forecast;
                        previous_target_graph_[1]=second->graph;facts_.graveler=graveler->identity;previous_graveler_graph_=graveler->graph;
                    }
                    bound=true;
                }
            }
        }
        if (!bound) { facts_.disposition=Disposition::unsafe_geometry;return false; }
    }
    const auto* target=find(s,r.actor);const auto* subject=find(s,facts_.subject);const auto* graveler=find(s,facts_.graveler);
    if (!target || !subject) { stop(Disposition::actor_lost);publish();neutral_due_=false;return true; }
    facts_.target_present=true;facts_.target_ready=target->ready;
    if (!target->ready) { facts_.disposition=Disposition::target_not_ready;publish();return true; }
    if (!hidden(*subject,facts_.required_species)) { stop(Disposition::hidden_subject_absent);publish();neutral_due_=false;return true; }
    if (facts_.required_species==28 && (!graveler || graveler->graph!=(facts_.active_throw==0?0x802d2bb4u:0x802d2bd4u))) {
        stop(Disposition::window_ended);publish();neutral_due_=false;return true;
    }
    if (facts_.required_species==28) {
        facts_.graveler_forecast={};
        if (!bounce_forecast(*world,*graveler,nullptr,facts_.graveler_forecast)) {
            ++facts_.arc_rejections;facts_.disposition=Disposition::unsafe_geometry;publish();return true;
        }
    }
    if (facts_.requests>=static_cast<std::uint64_t>(facts_.required_species==27?1:2)) {
        stop(Disposition::request_limit);publish();neutral_due_=false;return true;
    }
    if (p->gDirectionIndex<0 || p->TargetDirectionZoomedIn || p->TurnToDirSpeed!=0) {
        if (++facts_.recovery_updates>128) { stop(Disposition::recovery_limit);publish();neutral_due_=false;return true; }
        facts_.phase=Phase::unzoom;std::uint16_t buttons=0;
        if (p->ZoomSwitchMode==1 && p->gDirectionIndex==-1 && !p->TargetDirectionZoomedIn &&
            !zoom_out_requested_ && !(p->input.current_buttons&z_button)) { buttons=z_button;zoom_out_requested_=true; }
        publish(buttons);return true;
    }
    zoom_out_requested_=false;
    if (++facts_.steering_updates>512) { stop(Disposition::steering_limit);publish();neutral_due_=false;return true; }
    IsleGameVec3 aimed{};
    if (!plan(*world,*p,s,*target,facts_.required_species==27?subject:nullptr,aimed,r.forecast)) {
        ++facts_.arc_rejections;facts_.disposition=Disposition::unsafe_geometry;publish();return true;
    }
    std::int8_t x=0,y=0;aim_.point(aim_.owner,*p,aimed,false,x,y);
    facts_.input_target=aimed;facts_.phase=Phase::aim;facts_.disposition=Disposition::none;std::uint16_t buttons=0;
    if (!x && !y && !(p->input.current_buttons&b_button) &&
        forecast(*world,*p,s,*target,facts_.required_species==27?subject:nullptr,p->cam_target,r.actual_forecast)) {
        bool outstanding=false;
        for (std::size_t i=0;i<s.item_count;++i) {
            const auto& item=s.items[i];outstanding |= item.kind==ISLE_ITEM_PESTER_BALL && item.state==ISLE_ITEM_FLYING &&
                (!(item.object_flags&1u) || item.rest_timer<=0) && !(item.flags&ISLE_ITEM_DELETED);
        }
        if (!outstanding) {
            prethrow_count_=s.item_count;for (std::size_t i=0;i<prethrow_count_;++i) prethrow_items_[i]=s.items[i].identity;
            r.request_update=updates;++facts_.requests;pending_press_=true;facts_.phase=Phase::press;buttons=b_button;
        } else facts_.disposition=Disposition::item_outstanding;
    }
    publish(buttons,x,y);return true;
}
}

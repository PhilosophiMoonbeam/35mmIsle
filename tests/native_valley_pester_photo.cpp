#include "native_valley_pester_photo.hpp"
#include "captured_photo_identity.hpp"
#include "application_runtime.hpp"
#include "application_photos.hpp"
#include "valley_scene.hpp"
#include "game_state.hpp"
#include "actor_owner.hpp"
#include "actor_owner_source.h"
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
// Authority: pokemonsnap df8815e75f3d79618466e01e0827c6165b93d11c;
// US ROM SHA256 a1d5d816db7f8557ee04c35a011326d058b2c1fbca76b57b352b1d705a1ec1cc.
// 7AA790.c173-326: behavior2 needs cmd10 (<150 in pokemon.c1838-1854,
// not merely the state body's <=400), total progress<=1.3; its real miss
// leap sets EC86C only while cliff Mankey's 3FF8 remains zero. Behavior3
// waits block3/.18, then its cmd9 leap sends30 to the preplaced Gyarados.
// ROM 7AD0C8/F8/128/158, stride16: FOUR cmd43 advances, to CDF68,
// CDFFC, CE090, CE124. cmd37 is NOT an advance: it repeats/backtracks.
// Each advance clears the graph during an actual animation (7A7410.c).
// 79F1B0.c182-218: Pester ground0019FF sends43; ground337FB2 at
// player/item distance>500 draws 10/10/10/70. The real 2980 latch/factory
// owns every fish. Goldeen's cmd31 sender is the actual landing Item.
// No callbacks, RNG, latches, pose, progress or capture state are written here.
constexpr std::uint16_t b_button=0x4000,z_button=0x2000,a_button=0x8000;
constexpr std::uint64_t overall_cap=16384,flight_cap=256,reaction_cap=1536,capture_cap=2048;
constexpr float tau=6.2831853071795864769f;
constexpr std::array<std::uint32_t,4> shell_graphs{0x802d3b38u,0x802d3b68u,0x802d3b98u,0x802d3bc8u};
constexpr std::array<std::uint32_t,4> shell_states{0x802cdf68u,0x802cdffcu,0x802ce090u,0x802ce124u};
using valley_pester_detail::Actor;
using valley_pester_detail::Item;
using valley_pester_detail::Snapshot;
void require(bool v,const char* m) { if (!v) throw std::runtime_error(m); }
ValleyPesterIncarnation key(menu::animation::InstanceHandle h) { return {h.pool,h.slot,h.incarnation}; }
float squared(IsleGameVec3 a,IsleGameVec3 b) { const float x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;return x*x+y*y+z*z; }
bool finite(IsleGameVec3 p) { return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z); }
std::uint32_t state_id(IsleGameSourceCallback f) {
    if (f==func_802D17D8_7AAD68) return 0x802d17d8u;
    if (f==func_802D1ACC_7AB05C) return 0x802d1accu;
    if (f==func_802D1F18_7AB4A8) return 0x802d1f18u;
    if (f==func_802C9634_7A2BC4) return 0x802c9634u;
    if (f==func_802CA020_7A35B0) return 0x802ca020u;
    if (f==func_802CDF68_7A74F8) return shell_states[0];
    if (f==func_802CDFFC_7A758C) return shell_states[1];
    if (f==func_802CE090_7A7620) return shell_states[2];
    if (f==func_802CE124_7A76B4) return shell_states[3];
    if (f==func_802D237C_7AB90C) return 0x802d237cu;
    return 0;
}
const Actor* find_actor(const Snapshot& s,ValleyPesterIncarnation identity) {
    if (!identity[0]) return nullptr;
    for (std::size_t i=0;i<s.actor_count;++i) if (s.actors[i].identity==identity) return &s.actors[i];
    return nullptr;
}
const Actor* species(const Snapshot& s,int id,int behavior=-1) {
    const Actor* best=nullptr;
    for (std::size_t i=0;i<s.actor_count;++i) {
        const auto& a=s.actors[i];
        if (a.species==id && (behavior<0 || a.behavior==behavior) &&
            (!best || a.player_distance<best->player_distance)) best=&a;
    }
    return best;
}
void snapshot(Snapshot& s,const IslePlayerSource& p,actor::Owner& actors,items::Owner& items,
              const world::World& world,IsleGameVec3 origin,ValleyPesterIncarnation subject,int wanted) {
    s.actor_count=s.item_count=s.sphere_count=0;s.capacity=true;s.copied=false;s.evaluator_slot=-1;s.taken_species=0;
    const IsleGameModelPlacement* player_placement=nullptr;
    require(p.model && isle_game_model_placement(p.game,p.model,&player_placement)==ISLE_GAME_OK &&
        player_placement && player_placement->mode!=ISLE_GAME_MODEL_PLACEMENT_DISABLED,
        "Valley actual Player model position unavailable");
    s.player_root=player_placement->transform.position;
    IslePlayerPhotoFocus focus{};
    require(isle_player_photo_published_focus(p.photo,&focus)==ISLE_GAME_OK,"Valley actual focus observation rejected");
    s.focus_species=focus.pokemon_id;s.focus_context=focus.context;s.focus_flags=focus.flags;
    s.focus_actor=key(actors.instance(focus.object));s.taken_actor={};
    const int taken=p.photo->PokemonDetector_TakenPhotoIndex;
    if (taken>=0 && taken<2) {
        const auto& c=p.photo->PokemonDetector_Photo[taken];
        s.taken_actor=key(actors.instance(c.pokemonInFocus));
        for (std::size_t i=0;i<12;++i) if (c.pokemonInFocus && c.pokemonObjects[i]==c.pokemonInFocus) {
            s.taken_species=c.main.pokemons[i].pokemonID;s.evaluator_slot=static_cast<int>(i);
            const int count=p.photo->gPhotoCount;
            if (subject[0] && s.taken_actor==subject && s.taken_species==wanted && count>0 && count<=60)
                s.copied=private_course::photo_context_copied(p.photo->D_800B0598[count-1],c,i);
        }
    }
    IsleGameObject* previous=nullptr;
    for (;;) {
        IsleGameObject* object=nullptr;IsleActorSource* source=nullptr;IsleGameModel* model=nullptr;
        require(isle_actor_source_link3_next(p.game,previous,&object,&source,&model)==ISLE_GAME_OK,
            "Valley actual Actor traversal rejected");
        if (!object) break;
        previous=object;
        const auto identity=key(actors.instance(object));require(identity[0]!=0,"Valley Actor incarnation unavailable");
        if (source->tangible && source->collisionRadius>0) {
            if (s.sphere_count==s.spheres.size()) s.capacity=false;
            else s.spheres[s.sphere_count++]={identity,source->collPosition,source->collisionRadius};
        }
        if (source->id!=118 && source->id!=129 && source->id!=130 && source->id!=149 && source->id!=1033) continue;
        if (s.actor_count==s.actors.size()) { s.capacity=false;continue; }
        Actor a{};a.identity=identity;a.species=source->id;a.behavior=source->behavior;
        a.object=object;a.interaction=source->interactionTarget;
        a.graph=source->transitionGraph?source->transitionGraph->source_identity:0;
        a.flags=source->flags;a.animation=source->modelAnims;a.collision=source->collPosition;
        a.global_collision={a.collision.x+origin.x,a.collision.y+origin.y,a.collision.z+origin.z};
        a.radius=source->collisionRadius;a.tangible=source->tangible!=0;a.player_distance=source->playerDist;
        a.interaction_distance=source->interactionDist;a.path=source->pathParam;
        a.item_sender=key(items.instance(source->interactionTarget));a.actor_sender=key(actors.instance(source->interactionTarget));
        IsleGameSourceCallback f=nullptr;
        if (source->stateProc) require(isle_game_process_source_function(p.game,source->stateProc,&f)==ISLE_GAME_OK,
            "Valley actual Source state observation rejected");
        a.state=state_id(f);
        IsleGameObjectMetadata m{};
        require(isle_game_object_metadata(p.game,object,&m)==ISLE_GAME_OK,"Valley Actor metadata rejected");
        a.object_flags=m.flags;
        if (model) {
            const IsleGameModelPlacement* placement=nullptr;
            require(isle_game_model_placement(p.game,model,&placement)==ISLE_GAME_OK && placement,
                "Valley Actor placement rejected");
            if (placement->mode!=ISLE_GAME_MODEL_PLACEMENT_DISABLED) {
                a.root=placement->transform.position;
                a.animation_time=actors.animation().model(actors.instance(object)).animation_time;
            }
        }
        a.visible=model && !(m.flags&1u) && a.tangible;
        const float pitch=std::atan2(a.collision.y-p.CameraEyePos.y,
            std::hypot(a.collision.x-p.CameraEyePos.x,a.collision.z-p.CameraEyePos.z));
        a.eligible=a.visible && (source->flags&4u) && !(source->flags&8u) && source->playerDist>=0 &&
            source->playerDist<=10000 && pitch>=p.MinPitch && pitch<=p.MaxPitch;
        a.focused=a.eligible && focus.present && focus.object==object && focus.pokemon_id==source->id &&
            (focus.flags&4u) && !(focus.flags&8u);
        s.actors[s.actor_count++]=a;
    }
    for (const auto& binding:items.render_bindings()) {
        if (!binding.model.object) continue;
        const auto h=items.instance(binding.model.object);const auto family=items.family(h);
        if (!family || (*family!=items::Family::pester_ball && *family!=items::Family::apple)) continue;
        const auto* source=items.source(binding.model.object);require(source && bool(h),"Valley actual Item unavailable");
        if (s.item_count==s.items.size()) { s.capacity=false;break; }
        const IsleGameModelNode* nodes=nullptr;std::size_t count=0;
        require(isle_game_model_peek_nodes(p.game,binding.model.model,&nodes,&count)==ISLE_GAME_OK && nodes && count,
            "Valley actual Item root rejected");
        IsleGameObjectMetadata m{};
        require(isle_game_object_metadata(p.game,binding.model.object,&m)==ISLE_GAME_OK,"Valley Item metadata rejected");
        world::GroundResult ground{};const auto pos=nodes[0].transform.position;
        const bool has_ground=world.ground_at(pos.x,pos.z,ground);
        s.items[s.item_count++]={key(h),pos,source->flags,m.flags,has_ground?static_cast<std::uint32_t>(ground.surface):0u,
            source->itemID,source->state,source->restTimer};
    }
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
// Predict only the FIRST Source-relevant physical collision. No prediction is
// an event receipt. Ground refinement matches items.c717-768 (16 bisections).
bool forecast(const world::World& world,const IslePlayerSource& p,const Snapshot& s,const Actor* target,
              int mode,IsleGameVec3 ray,IsleGameVec3 motion,ValleyPesterFacts& facts) {
    const IsleGameCameraParameters* camera=nullptr;
    require(isle_game_camera_parameters(p.game,p.camera,&camera)==ISLE_GAME_OK && camera,
        "Valley actual launch camera unavailable");
    IsleGameVec3 pos{camera->look_at.eye[0],camera->look_at.eye[1],camera->look_at.eye[2]},previous=pos;
    IsleGameVec3 v{ray.x-pos.x,ray.y-pos.y+100,ray.z-pos.z};
    const float length=std::hypot(v.x,v.y,v.z);
    if (!(length>0) || !finite(v)) return false;
    v={v.x*50/length+p.PlayerVelocity.x,v.y*50/length+p.PlayerVelocity.y,v.z*50/length+p.PlayerVelocity.z};
    for (unsigned step=0;step<192;++step) {
        world::GroundResult ground{},ceiling{};
        if (!finite(pos) || std::abs(pos.x)>=10000 || std::abs(pos.y)>=10000 || std::abs(pos.z)>=10000 ||
            !world.ground_at(pos.x,pos.z,ground) || (world.ceiling_at(pos.x,pos.z,ceiling) && pos.y>ceiling.height)) return false;
        if (pos.y<=ground.height+.375f) {
            if (mode==1) return false;
            if (pos.y<ground.height-.375f) {
                auto low=previous,high=pos;
                for (unsigned i=0;i<16;++i) {
                    pos={(low.x+high.x)*.5f,(low.y+high.y)*.5f,(low.z+high.z)*.5f};
                    if (!world.ground_at(pos.x,pos.z,ground)) return false;
                    if (std::abs(pos.y-ground.height)<=.375f) break;
                    if (pos.y<ground.height) high=pos;else low=pos;
                }
            }
            pos.y=ground.height+12;
            facts.predicted_material=ground.surface;facts.predicted_flight=static_cast<float>(step);
            const float distance=std::sqrt(squared(pos,s.player_root));
            if (mode==0) return target && squared(pos,target->root)<140*140;
            return mode==2?ground.surface==0x0019ffu:ground.surface==0x337fb2u && distance>550;
        }
        const IsleGameVec3 delta{v.x*.5f,v.y*.5f,v.z*.5f};
        world::Vec3 hit{},reflected{};
        if (world.collide({pos.x,pos.y,pos.z},{delta.x,delta.y,delta.z},hit,reflected)>=0) return false;
        float nearest=2;bool selected=false;
        for (std::size_t i=0;i<s.sphere_count;++i) {
            const auto& sphere=s.spheres[i];auto center=sphere.center;
            const bool is_target=target && sphere.identity==target->identity;
            if (is_target && mode==1) {
                const float t=static_cast<float>(step+1);
                center={center.x+motion.x*t,center.y+motion.y*t,center.z+motion.z*t};
            }
            const float entry=intersection(pos,delta,center,sphere.radius);
            if (entry<nearest) { nearest=entry;selected=is_target; }
        }
        if (nearest<1) { facts.predicted_flight=static_cast<float>(step)+std::max(0.0f,nearest);return mode==1 && selected; }
        previous=pos;v.y-=.6f;pos={pos.x+v.x*.5f,pos.y+v.y*.5f,pos.z+v.z*.5f};
    }
    return false;
}
bool plan(const world::World& world,const IslePlayerSource& p,const Snapshot& s,const Actor* target,
          int mode,ValleyPesterFacts& facts,IsleGameVec3& aimed) {
    if (mode==1) {
        if (!target) return false;
        auto point=target->collision;
        for (unsigned i=0;i<3;++i) {
            if (projectile_target(p,point,aimed) && forecast(world,p,s,target,mode,aimed,facts.motion,facts)) return true;
            const float t=std::hypot(point.x-p.CameraEyePos.x,point.z-p.CameraEyePos.z)/20+1;
            point={target->collision.x+facts.motion.x*t,target->collision.y+facts.motion.y*t,target->collision.z+facts.motion.z*t};
        }
        return false;
    }
    const auto origin=mode==3?s.player_root:(target?target->root:s.player_root);
    // Fixed finite rings; every candidate is queried against real World material,
    // then the actual launch path, other Actors and first collision are checked.
    const unsigned rings=mode==0?3u:mode==2?7u:6u;
    for (unsigned ring=0;ring<rings;++ring) for (unsigned spoke=0;spoke<16;++spoke) {
        const float radius=mode==0?static_cast<float>(ring)*40:mode==2?static_cast<float>(ring)*100:700+static_cast<float>(ring)*200;
        const float angle=static_cast<float>(spoke)*tau/16;
        IsleGameVec3 point{origin.x+std::sin(angle)*radius,0,origin.z+std::cos(angle)*radius};
        world::GroundResult ground{};
        if (!world.ground_at(point.x,point.z,ground) || (mode==2 && ground.surface!=0x0019ffu) ||
            (mode==3 && ground.surface!=0x337fb2u)) continue;
        point.y=ground.height;
        if (projectile_target(p,point,aimed) && forecast(world,p,s,target,mode,aimed,{},facts)) return true;
        if (ring==0 && mode!=3) break;
    }
    return false;
}
}
int choose_valley_pester_photo_species(application::Photos& photos) {
    for (const int species:{130,27,28,149,118}) {
        const auto present=photos.report_present(species);
        require(bool(present),"Valley canonical Report unavailable");
        if (!present.value) return species;
    }
    return 0;
}
ValleyPesterPhotoPuzzle::ValleyPesterPhotoPuzzle(private_post_course::Controller controller,Aim aim,int species)
    :controller_(controller),aim_(aim) {
    require(controller_.set && aim_.point,"Valley Pester requires actual pad and pure aim bridges");
    require(species==130 || species==149 || species==118,"Valley Pester objective invalid");
    facts_.required_species=species;
}
void ValleyPesterPhotoPuzzle::publish(std::uint16_t buttons,std::int8_t x,std::int8_t y) noexcept {
    controller_.set(controller_.owner,buttons,x,y);facts_.owns_pad=true;pad_published_=true;
}
bool ValleyPesterPhotoPuzzle::relinquish() noexcept {
    if (pad_published_) controller_.set(controller_.owner,0,0,0);
    pad_published_=false;facts_.owns_pad=false;return false;
}
void ValleyPesterPhotoPuzzle::stop(ValleyPesterDisposition why) noexcept {
    if (facts_.exhausted || facts_.complete) return;
    facts_.exhausted=true;facts_.capture_pending=false;facts_.phase=ValleyPesterPhase::finished;facts_.disposition=why;
    pending_press_=false;tracking_=false;waiting_reaction_=false;capture_armed_=false;neutral_due_=true;
}
void ValleyPesterPhotoPuzzle::observe(const application::RuntimeView& view,std::uint64_t updates) {
    if (sampled_ && updates==previous_update_) return;
    require(!sampled_ || updates>previous_update_,"Valley completed-update ordinal regressed");
    const bool consecutive=sampled_ && updates==previous_update_+1;
    sampled_=true;previous_update_=updates;facts_.last_update=updates;facts_.owns_pad=false;active_=false;
    if (view.valley) {
        facts_.source_status=view.valley->adapter().source_status();facts_.host_status=view.valley->error();
        if (const auto* host=view.valley->native_host()) if (host->error()!=ISLE_GAME_OK) facts_.host_status=host->error();
        if (facts_.source_status!=ISLE_GAME_OK || facts_.host_status!=ISLE_GAME_OK) {
            stop(ValleyPesterDisposition::source_error);
            const auto* host=view.valley->native_host();
            const char* message=host && host->error()!=ISLE_GAME_OK?host->message():view.valley->message();
            throw std::runtime_error(message && *message?message:"Valley actual Source execution failed");
        }
    }
    if (!view.valley || view.valley->status()!=course::LifecycleStatus::running) {
        if (started_ && !facts_.complete) stop(ValleyPesterDisposition::course_ended);
        return;
    }
    const auto& entry=*view.valley;const auto* host=entry.native_host();const auto* p=entry.adapter().native.player();
    if (!host || !host->execution_open() || !p) return;
    require(p->status==ISLE_GAME_OK && p->photo && p->photo->status==ISLE_GAME_OK,"Valley actual Player/Photo failed");
    auto* actors=host->actors();auto* items=host->items_owner();const auto* world=host->world_owner();
    require(actors && items && world && world->error().empty(),"Valley actual Actor/Item/World unavailable");
    const auto* block=world->current();
    if (!block || !block->resource || world->retired()) { if (started_) stop(ValleyPesterDisposition::course_ended);return; }
    const auto offset=block->resource->metadata.world_position;
    snapshot(snapshot_,*p,*actors,*items,*world,{offset.x*100,offset.y*100,offset.z*100},facts_.subject,facts_.required_species);
    active_=true;const auto& s=snapshot_;const auto& source=entry.actors_source();const auto& photo=*p->photo;
    facts_.current_block=static_cast<int>(block->index);facts_.block_progress=p->movement.move_time;
    facts_.film=photo.gPhotoCount;facts_.qualified=photo.D_800AE27C;
    require(facts_.film>=0 && facts_.film<=60 && facts_.qualified>=0 && facts_.qualified<=facts_.film,"Valley roll counters invalid");
    facts_.focus_species=s.focus_species;facts_.focus_context=s.focus_context;facts_.focus_actor=s.focus_actor;facts_.focus_flags=s.focus_flags;
    facts_.taken_context=photo.PokemonDetector_TakenPhotoIndex;facts_.taken_species=s.taken_species;facts_.taken_actor=s.taken_actor;
    facts_.evaluator_slot=s.evaluator_slot;facts_.full_context_copied=s.copied;
    facts_.player_ready=p->D_80382D04_523114==1 && !p->IsInputDisabled && !p->IsPaused;
    require(entry.game_state().save(),"Valley canonical Save unavailable");
    const auto reward=entry.game_state().save()->player_flag(1);require(bool(reward),"Valley Pester reward unavailable");
    facts_.available=facts_.pester_available=reward.value;
    if (!started_ && facts_.available && facts_.player_ready) { started_=true;facts_.initial_update=updates; }
    if (observed_roll_<0) { observed_roll_=facts_.film;observed_qualified_=facts_.qualified; }
    require(facts_.film>=observed_roll_ && facts_.qualified>=observed_qualified_,"Valley roll counters regressed");
    for (int slot=observed_roll_;slot<facts_.film;++slot) {
        const auto& image=photo.D_800B0598[slot];facts_.capture_observed=true;facts_.raw_primary=image.pokemons[0].pokemonID;
        if (facts_.raw_primary!=facts_.required_species) continue;
        ++facts_.captures;
        const bool accepted=!facts_.exhausted && facts_.reveal_seen && capture_armed_ && consecutive &&
            facts_.film==observed_roll_+1 && facts_.qualified==observed_qualified_+1 && armed_slot_==slot &&
            armed_qualified_+1==facts_.qualified && armed_subject_==facts_.subject && s.taken_actor==armed_subject_ &&
            facts_.taken_context>=0 && facts_.taken_context<2 && s.copied && image.levelID==5 &&
            (p->input.pressed_buttons&a_button) && (p->input.current_buttons&a_button) &&
            facts_.releases>=facts_.consumed;
        if (accepted) {
            facts_.complete=true;facts_.capture_pending=false;facts_.capture_update=updates;facts_.captured_slot=slot;
            facts_.captured_actor=s.taken_actor;facts_.captured_slots|=std::uint64_t{1}<<static_cast<unsigned>(slot);
            facts_.phase=ValleyPesterPhase::finished;facts_.disposition=ValleyPesterDisposition::qualified;neutral_due_=true;
        } else ++facts_.capture_mismatches;
    }
    observed_roll_=facts_.film;observed_qualified_=facts_.qualified;capture_armed_=false;
    facts_.mankey_latched=source.D_802D3FF8_7AD588!=0;
    facts_.fish_latch=key(actors->instance(source.D_802D2980_7ABF10));facts_.fish_latch_busy=source.D_802D2980_7ABF10!=nullptr;
    const Actor* target=nullptr;
    if (facts_.required_species==130) {
        if (!facts_.early_latch) target=facts_.early_actor[0]?find_actor(s,facts_.early_actor):species(s,129,2);
        else target=facts_.swimmer[0]?find_actor(s,facts_.swimmer):species(s,129,3);
    } else if (facts_.required_species==149) target=facts_.shell[0]?find_actor(s,facts_.shell):species(s,1033);
    facts_.target_present=target!=nullptr;facts_.target_ready=false;facts_.motion={};
    if (target) {
        facts_.root=target->root;facts_.collision=target->collision;facts_.target_behavior=target->behavior;
        facts_.target_graph=target->graph;facts_.target_state=target->state;facts_.target_animation=target->animation;
        facts_.animation_time=target->animation_time;facts_.target_flags=target->flags;facts_.target_object_flags=target->object_flags;
        facts_.player_distance=target->player_distance;facts_.interaction_distance=target->interaction_distance;
        if (consecutive && previous_target_==target->identity) facts_.motion={
            target->global_collision.x-previous_global_collision_.x,target->global_collision.y-previous_global_collision_.y,
            target->global_collision.z-previous_global_collision_.z};
        if (facts_.required_species==130) facts_.target_ready=!facts_.early_latch?
            target->graph==0x802ec8b8u && static_cast<float>(facts_.current_block)+facts_.block_progress<=1.3f && !facts_.mankey_latched:
            target->graph==0x802ec8e4u && target->visible && target->radius>0 && target->player_distance<=1950 &&
            (facts_.current_block>3 || (facts_.current_block==3 && facts_.block_progress>=.18f));
        else facts_.target_ready=facts_.shell_stage<4 && target->graph==shell_graphs[static_cast<std::size_t>(facts_.shell_stage)];
    }
    if (pending_press_ && (p->input.current_buttons&b_button)) {
        if ((p->input.pressed_buttons&b_button) && p->ThrowItemTimeout>0 && p->LastItemId==ISLE_ITEM_PESTER_BALL &&
            p->gDirectionIndex>=0 && !p->TargetDirectionZoomedIn && p->TurnToDirSpeed==0) {
            ++facts_.pressed;++facts_.consumed;facts_.consumed_update=updates;tracking_=true;accepted_attempt_=false;
            ground_seen_=false;facts_.physical_ground=false;facts_.item={};attempt_start_=updates;
        } else stop(ValleyPesterDisposition::press_timeout);
        pending_press_=false;pending_release_=true;release_start_=updates;
    }
    if (pending_press_ && updates-facts_.request_update>=8) stop(ValleyPesterDisposition::press_timeout);
    if (pending_release_ && !(p->input.current_buttons&b_button)) { pending_release_=false;++facts_.releases;facts_.release_update=updates; }
    if (pending_release_ && updates-release_start_>=8) { pending_release_=false;stop(ValleyPesterDisposition::release_timeout); }
    if (!s.capacity) stop(ValleyPesterDisposition::capacity);
    if (started_ && updates-facts_.initial_update>=overall_cap) stop(ValleyPesterDisposition::overall_limit);
    const Item* item=nullptr;unsigned matches=0;
    if (tracking_) for (std::size_t i=0;i<s.item_count;++i) {
        const auto& candidate=s.items[i];bool existed=false;
        for (std::size_t j=0;j<prethrow_item_count_;++j) existed|=prethrow_items_[j]==candidate.identity;
        if (candidate.kind!=ISLE_ITEM_PESTER_BALL || (facts_.item[0]?facts_.item!=candidate.identity:existed)) continue;
        item=&candidate;++matches;
    }
    facts_.item_present=matches==1;
    if (matches>1) stop(ValleyPesterDisposition::hit_unattributed);
    if (item) {
        facts_.item=item->identity;facts_.item_position=item->position;facts_.item_kind=item->kind;facts_.item_state=item->state;
        facts_.item_flags=item->flags;facts_.item_object_flags=item->object_flags;facts_.item_hidden=(item->object_flags&1u)!=0;
        facts_.interaction_matches=target && target->item_sender==item->identity;
        if (!ground_seen_ && (item->flags&ISLE_ITEM_TOUCHED_GROUND)) {
            ground_seen_=true;facts_.physical_ground=true;facts_.physical_update=updates;++facts_.physical_landings;
            facts_.material=item->material;facts_.ground_position=item->position;
            facts_.ground_player_distance=std::sqrt(squared(item->position,s.player_root));
            facts_.ground_target_distance=target?std::sqrt(squared(item->position,target->root)):0;
        }
    }
    if (!facts_.exhausted && !facts_.complete && facts_.required_species==130) {
        if (!facts_.early_latch) {
            if (facts_.mankey_latched) stop(ValleyPesterDisposition::mankey_latched);
            if (tracking_ && target && target->state==0x802d17d8u && !facts_.early_hit) {
                if (!consecutive || !item || !facts_.interaction_matches || !ground_seen_ ||
                    !facts_.item_hidden || target->interaction_distance>=150 || previous_target_!=target->identity ||
                    previous_target_graph_!=0x802ec8b8u) stop(ValleyPesterDisposition::hit_unattributed);
                else {
                    facts_.early_hit=true;facts_.early_hit_update=updates;facts_.accepted_item=item->identity;
                    facts_.early_item=item->identity;
                    ++facts_.accepted_hits;accepted_attempt_=true;waiting_reaction_=true;reaction_start_=updates;
                }
            }
            if (target && facts_.early_hit && target->state==0x802d1accu && !facts_.early_leap) {
                facts_.early_leap=true;facts_.early_leap_update=updates;
            }
            if (source.D_802EC86C_7C5DFC && !previous_latch_) {
                if (!consecutive || !facts_.early_hit || !facts_.early_leap || facts_.mankey_latched)
                    stop(ValleyPesterDisposition::stage_unattributed);
                else { facts_.early_latch=true;facts_.early_latch_update=updates;waiting_reaction_=false;facts_.target={}; }
            }
            if (!facts_.early_hit && static_cast<float>(facts_.current_block)+facts_.block_progress>1.3f)
                stop(ValleyPesterDisposition::early_window_missed);
        } else if (tracking_ && target && target->state==0x802d1f18u && !facts_.swimmer_hit) {
            if (!consecutive || !item || !facts_.interaction_matches || !facts_.item_hidden || ground_seen_ ||
                previous_target_!=target->identity || previous_target_graph_!=0x802ec8e4u)
                stop(ValleyPesterDisposition::hit_unattributed);
            else {
                facts_.swimmer_hit=facts_.source_leap=true;facts_.swimmer_hit_update=updates;
                facts_.accepted_item=item->identity;++facts_.accepted_hits;accepted_attempt_=true;
                facts_.swimmer_item=item->identity;
                waiting_reaction_=true;reaction_start_=updates;
            }
        }
    }
    if (!facts_.exhausted && !facts_.complete && facts_.required_species==149 && target) {
        const auto stage=static_cast<std::size_t>(facts_.shell_stage);
        if (tracking_ && !accepted_attempt_ && stage<4 && target->state==shell_states[stage] && !target->graph) {
            if (!consecutive || !item || !ground_seen_ || facts_.material!=0x0019ffu ||
                !facts_.interaction_matches || !facts_.item_hidden || previous_target_!=target->identity ||
                previous_target_graph_!=shell_graphs[stage]) stop(ValleyPesterDisposition::stage_unattributed);
            else {
                accepted_attempt_=true;waiting_reaction_=true;reaction_start_=updates;++facts_.accepted_hits;
                facts_.accepted_item=item->identity;facts_.shell_items[stage]=item->identity;facts_.shell_hit_updates[stage]=updates;
                facts_.shell_animations[stage]=target->animation;
            }
        }
        if (waiting_reaction_ && stage<3 && target->graph) {
            if (!consecutive || target->graph!=shell_graphs[stage+1] || target->state!=shell_states[stage] ||
                !accepted_attempt_ || !facts_.shell_hit_updates[stage]) stop(ValleyPesterDisposition::stage_unattributed);
            else { facts_.shell_ready_updates[stage]=updates;++facts_.shell_stage;waiting_reaction_=false; }
        }
    }
    // Reveal witnesses use actual accepted interactionTarget and actual callback,
    // never elapsed time, proximity, a predicted collision, or request count.
    const Actor* subject=facts_.subject[0]?find_actor(s,facts_.subject):species(s,facts_.required_species);
    if (!facts_.exhausted && !facts_.complete && subject && !facts_.reveal_seen) {
        if (facts_.required_species!=118 && !facts_.subject[0] && !subject->visible) facts_.subject=subject->identity;
        bool revealed=false,attributed=false;
        if (facts_.required_species==130) {
            revealed=subject->state==0x802c9634u && subject->visible;
            // Native queued-command and canonical-field borrows retain this
            // identity after Source's immediate swimmer RunCleanup. Never
            // require the sender to remain an active Actor after delivery.
            attributed=facts_.source_leap && previous_subject_==subject->identity && previous_subject_hidden_ &&
                swimmer_object_ && subject->interaction==swimmer_object_ &&
                facts_.swimmer[0] && source.D_802D2D78_7AC308;
        } else if (facts_.required_species==149) {
            revealed=subject->state==0x802ca020u && subject->visible;
            attributed=facts_.shell_stage==3 && facts_.shell_hit_updates[3] && accepted_attempt_ &&
                previous_subject_==subject->identity && previous_subject_hidden_ && subject->actor_sender==facts_.shell &&
                facts_.shell[0] && source.D_802D2FF4_7AC584;
        } else {
            bool existed=false;
            for (std::size_t i=0;i<prethrow_actor_count_;++i) existed|=prethrow_actors_[i]==subject->identity;
            revealed=tracking_ && !existed && subject->visible && subject->state==0x802d237cu;
            attributed=tracking_ && item && ground_seen_ && facts_.material==0x337fb2u &&
                facts_.ground_player_distance>500 && !existed && facts_.fish_latch==subject->identity &&
                subject->item_sender==item->identity && facts_.item_hidden;
        }
        if (revealed) {
            if (!consecutive || !attributed) stop(ValleyPesterDisposition::reveal_unattributed);
            else {
                facts_.subject=subject->identity;facts_.reveal_seen=true;facts_.reveal_update=updates;
                facts_.accepted_sender=facts_.required_species==118?subject->item_sender:
                    facts_.required_species==130?facts_.swimmer:subject->actor_sender;
                if (facts_.required_species==118) { facts_.accepted_item=item->identity;accepted_attempt_=true;++facts_.accepted_hits; }
                if (facts_.required_species==149) { facts_.shell_ready_updates[3]=updates;facts_.shell_stage=4; }
                waiting_reaction_=false;facts_.capture_pending=true;
            }
        }
    }
    facts_.subject_present=subject && subject->identity==facts_.subject;
    facts_.subject_visible=facts_.subject_present && subject->visible;
    facts_.subject_eligible=facts_.subject_present && subject->eligible;
    facts_.subject_focused=facts_.subject_present && subject->focused;
    if (facts_.subject_present) {
        facts_.subject_state=subject->state;facts_.subject_animation=subject->animation;
        facts_.subject_flags=subject->flags;facts_.subject_path=subject->path;
        if (!facts_.exhausted && facts_.reveal_seen && subject->focused && facts_.player_ready &&
            p->gDirectionIndex==-1 && !p->TargetDirectionZoomedIn && p->sTimerAfterPhotoTaken<0 &&
            !(p->input.current_buttons&a_button) && facts_.film<60) {
            capture_armed_=true;armed_subject_=subject->identity;armed_slot_=facts_.film;armed_qualified_=facts_.qualified;
        }
    }
    if (!facts_.complete && !facts_.exhausted) {
        if (tracking_) {
            ++facts_.flight_updates;
            if (!item && facts_.item[0]) {
                tracking_=false;
                // A finite unsuccessful lottery attempt is not progress. Only
                // a real free latch AND deleted own Item permits another draw.
                if (facts_.required_species==118 && !facts_.reveal_seen) ++facts_.fish_attempts;
            } else if (updates-attempt_start_>=flight_cap) stop(ValleyPesterDisposition::flight_timeout);
        }
        if (waiting_reaction_ && updates-reaction_start_>=reaction_cap) stop(ValleyPesterDisposition::reaction_timeout);
        if (facts_.reveal_seen) {
            facts_.phase=ValleyPesterPhase::capture;facts_.disposition=ValleyPesterDisposition::capture_pending;
            if (!facts_.subject_present) {
                if (facts_.required_species!=118) stop(ValleyPesterDisposition::actor_lost);
                else {
                    facts_.reveal_seen=false;facts_.capture_pending=false;facts_.subject={};capture_armed_=false;
                    facts_.accepted_sender={};
                }
            } else if (updates-facts_.reveal_update>=capture_cap) stop(ValleyPesterDisposition::reveal_timeout);
        }
    }
    previous_latch_=source.D_802EC86C_7C5DFC!=0;
    previous_subject_=subject?subject->identity:ValleyPesterIncarnation{};previous_subject_hidden_=subject && !subject->visible;
    previous_target_=target?target->identity:ValleyPesterIncarnation{};
    previous_target_graph_=target?target->graph:0;
    if (target) previous_global_collision_=target->global_collision;
}
bool ValleyPesterPhotoPuzzle::update(const application::RuntimeView& view,std::uint64_t updates) {
    observe(view,updates);
    if (control_sampled_ && control_update_==updates) return facts_.owns_pad;
    control_sampled_=true;control_update_=updates;facts_.owns_pad=false;
    if (neutral_due_) { neutral_due_=false;publish();return true; }
    if (!active_ || !view.valley) return relinquish();
    const auto* p=view.valley->adapter().native.player();const auto* host=view.valley->native_host();
    const auto* world=host?host->world_owner():nullptr;
    if (!p || !world || world->retired()) return relinquish();
    if (pending_press_ || pending_release_) {
        facts_.phase=pending_press_?ValleyPesterPhase::press:ValleyPesterPhase::release;
        publish(pending_press_ && !facts_.exhausted?b_button:0);return true;
    }
    if (facts_.complete || facts_.exhausted) return relinquish();
    if (facts_.capture_pending) return relinquish();
    if (!facts_.available) { facts_.disposition=ValleyPesterDisposition::pester_unavailable;return relinquish(); }
    if (!facts_.player_ready) { facts_.disposition=ValleyPesterDisposition::player_not_ready;return relinquish(); }
    if (tracking_ || waiting_reaction_) {
        facts_.phase=tracking_?ValleyPesterPhase::flight:ValleyPesterPhase::reaction;
        facts_.disposition=ValleyPesterDisposition::item_outstanding;publish();return true;
    }
    if (facts_.film>=59) { facts_.disposition=ValleyPesterDisposition::film_reserved;return relinquish(); }
    if (facts_.required_species==130 && facts_.swimmer_hit) {
        facts_.phase=ValleyPesterPhase::reveal;publish();return true;
    }
    if (facts_.required_species==118 && facts_.fish_latch_busy) {
        facts_.disposition=ValleyPesterDisposition::fish_latch_busy;return relinquish();
    }
    const int mode=facts_.required_species==118?3:facts_.required_species==149?2:facts_.early_latch?1:0;
    const auto* target=facts_.target[0]?find_actor(snapshot_,facts_.target):
        mode==0?species(snapshot_,129,2):mode==1?species(snapshot_,129,3):mode==2?species(snapshot_,1033):nullptr;
    if (mode!=3 && !target) { facts_.disposition=ValleyPesterDisposition::target_absent;return relinquish(); }
    if (mode!=3 && !facts_.target_ready) { facts_.disposition=ValleyPesterDisposition::target_not_ready;return relinquish(); }
    const std::uint64_t request_cap=mode==3?48:mode==2?12:8;
    if (facts_.requests>=request_cap) { stop(ValleyPesterDisposition::request_limit);publish();neutral_due_=false;return true; }
    if (!p->ui || p->ui->Icons_NumItemsAvailable<2) { facts_.disposition=ValleyPesterDisposition::item_ui_not_ready;return relinquish(); }
    if ((p->input.current_buttons&a_button) || p->sTimerAfterPhotoTaken>=0) return relinquish();
    if (p->ThrowItemTimeout!=0) { facts_.disposition=ValleyPesterDisposition::item_outstanding;return relinquish(); }
    IsleGameVec3 aimed{};
    bool planned=false;
    if (!facts_.target[0]) {
        // Unbound objectives still yield without a physical opportunity. Once
        // a target is bound, do not re-plan through Source's moving zoom camera.
        if (!plan(*world,*p,snapshot_,target,mode,facts_,aimed)) {
            ++facts_.arc_rejections;facts_.disposition=ValleyPesterDisposition::unsafe_geometry;return relinquish();
        }
        planned=true;
        if (target) {
            facts_.target=target->identity;
            if (mode==0) facts_.early_actor=target->identity;
            else if (mode==1) { facts_.swimmer=target->identity;swimmer_object_=target->object; }
            else facts_.shell=target->identity;
        }
    }
    if (p->gDirectionIndex<0 || p->TargetDirectionZoomedIn || p->TurnToDirSpeed!=0) {
        if (++recovery_>256) { stop(ValleyPesterDisposition::recovery_limit);publish();neutral_due_=false;return true; }
        ++facts_.recovery_updates;facts_.phase=ValleyPesterPhase::unzoom;
        facts_.disposition=ValleyPesterDisposition::none;
        std::uint16_t buttons=0;
        if (p->ZoomSwitchMode==1 && p->gDirectionIndex==-1 && !p->TargetDirectionZoomedIn &&
            !zoom_out_requested_ && !(p->input.current_buttons&z_button)) { buttons=z_button;zoom_out_requested_=true; }
        publish(buttons);return true;
    }
    zoom_out_requested_=false;
    // Neutral re-planning spends the same finite budget as actual steering.
    if (++acquisition_updates_>2048) { stop(ValleyPesterDisposition::steering_limit);publish();neutral_due_=false;return true; }
    if (!planned && !plan(*world,*p,snapshot_,target,mode,facts_,aimed)) {
        ++facts_.arc_rejections;facts_.phase=ValleyPesterPhase::aim;facts_.disposition=ValleyPesterDisposition::unsafe_geometry;
        // Giving this bound window to generic photography would immediately
        // re-enter zoom and spend recovery again instead of acquiring the swimmer.
        publish();return true;
    }
    ++facts_.steering_updates;
    std::int8_t x=0,y=0;aim_.point(aim_.owner,*p,aimed,false,x,y);
    facts_.input_target=aimed;facts_.phase=ValleyPesterPhase::aim;facts_.disposition=ValleyPesterDisposition::none;
    std::uint16_t buttons=0;
    // The final check uses the CURRENT quantized camera ray, not the plan ray.
    if (!x && !y && !(p->input.current_buttons&b_button) &&
        forecast(*world,*p,snapshot_,target,mode,p->cam_target,facts_.motion,facts_)) {
        bool outstanding=false;
        for (std::size_t i=0;i<snapshot_.item_count;++i) {
            const auto& item=snapshot_.items[i];
            outstanding|=item.kind==ISLE_ITEM_PESTER_BALL && !(item.flags&ISLE_ITEM_DELETED) &&
                item.state==ISLE_ITEM_FLYING && (!(item.object_flags&1u) || item.rest_timer<=0);
        }
        if (!outstanding) {
            prethrow_item_count_=snapshot_.item_count;prethrow_actor_count_=snapshot_.actor_count;
            for (std::size_t i=0;i<prethrow_item_count_;++i) prethrow_items_[i]=snapshot_.items[i].identity;
            for (std::size_t i=0;i<prethrow_actor_count_;++i) prethrow_actors_[i]=snapshot_.actors[i].identity;
            facts_.request_update=updates;++facts_.requests;pending_press_=true;buttons=b_button;
            facts_.phase=ValleyPesterPhase::press;
        } else facts_.disposition=ValleyPesterDisposition::item_outstanding;
    }
    publish(buttons,x,y);return true;
}
}

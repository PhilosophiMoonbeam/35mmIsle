#include "native_beach_pester_photo.hpp"
#include "captured_photo_identity.hpp"
#include "application_runtime.hpp"
#include "application_photos.hpp"
#include "actor_owner.hpp"
#include "actor_owner_source.h"
#include "beach_snorlax_kangaskhan_source.h"
#include "beach_scyther_special_source.h"
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
constexpr std::uint16_t pester=0x4000, zoom=0x2000, shutter=0x8000;
constexpr std::uint64_t overall_cap=8192, flight_cap=256, reaction_cap=512, capture_cap=1024;
using beach_pester_detail::Actor;
using beach_pester_detail::Item;
using beach_pester_detail::Sphere;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
BeachPesterIncarnation key(menu::animation::InstanceHandle value) { return {value.pool,value.slot,value.incarnation}; }
float squared(const IsleGameVec3& a,const IsleGameVec3& b) {
    const float x=a.x-b.x,y=a.y-b.y,z=a.z-b.z; return x*x+y*y+z*z;
}
bool finite(const IsleGameVec3& p) { return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z); }
bool inside(const IsleGameVec3& p) {
    return finite(p) && p.x>-10000 && p.x<10000 && p.y>-10000 && p.y<10000 && p.z>-10000 && p.z<10000;
}
IsleGameVec3 global(const IsleGameVec3& p,const IsleGameVec3& origin) { return {p.x+origin.x,p.y+origin.y,p.z+origin.z}; }
std::uint32_t state_identity(IsleGameSourceCallback callback) {
    if (callback==func_beach_802C5700) return 0x802c5700;
    if (callback==func_beach_802C5828) return 0x802c5828;
    if (callback==func_beach_802C5884) return 0x802c5884;
    if (callback==func_beach_802C5768) return 0x802c5768;
    if (callback==func_beach_802C8E90) return 0x802c8e90;
    if (callback==func_beach_802C8F48) return 0x802c8f48;
    return 0;
}
bool accepts(const IsleActorSource& actor,int species) {
    // Pinned df8815e + ROM: 564244/96 bytes and 565254/48 bytes.
    if (!actor.transitionGraph || actor.transitionGraph->source_identity!=
        (species==143 ? 0x802cc1d4u : 0x802cd1e4u)) return false;
    for (std::size_t i=0;i<actor.transitionGraph->count;++i) {
        const auto& row=actor.transitionGraph->rows[i];
        if (row.cmd==58) break;
        if (row.cmd==(species==143 ? 9 : 10) && row.state==
            (species==143 ? func_beach_802C5884 : func_beach_802C8F48)) return true;
    }
    return false;
}
// Same Source half-velocity ray/sphere test as the existing Cave controller.
float intersection(const IsleGameVec3& start,const IsleGameVec3& delta,const IsleGameVec3& center,float radius) {
    const float length=std::hypot(delta.x,delta.y,delta.z);
    if (!(length>0) || !(radius>0)) return 2;
    const float x=center.x-start.x,y=center.y-start.y,z=center.z-start.z;
    const float projection=(x*delta.x+y*delta.y+z*delta.z)/length;
    const float perpendicular=x*x+y*y+z*z-projection*projection;
    if (!(projection>0) || perpendicular>radius*radius) return 2;
    const float entry=(projection-std::sqrt(std::max(0.0f,radius*radius-perpendicular)))/length;
    return entry<1 ? entry : 2;
}
// Prospective input gate only. Source order: current terrain/ceiling, static
// sweep, nearest tangible Actor, then gravity and movement. Never a receipt.
bool forecast(const world::World& world,const IslePlayerSource& player,const Actor& actor,
              const Sphere* spheres,std::size_t sphere_count,const IsleGameVec3& motion,
              const IsleGameVec3& ray,int species,BeachPesterForecast& out) {
    out={};
    const IsleGameCameraParameters* camera=nullptr;
    require(isle_game_camera_parameters(player.game,player.camera,&camera)==ISLE_GAME_OK && camera,
            "Beach Pester launch camera unavailable");
    IsleGameVec3 position{camera->look_at.eye[0],camera->look_at.eye[1],camera->look_at.eye[2]};
    auto previous=position;
    IsleGameVec3 velocity{ray.x-position.x,ray.y-position.y+100,ray.z-position.z};
    const float length=std::hypot(velocity.x,velocity.y,velocity.z);
    if (!(length>0) || !std::isfinite(length)) return false;
    velocity={velocity.x*50/length+player.PlayerVelocity.x,velocity.y*50/length+player.PlayerVelocity.y,
              velocity.z*50/length+player.PlayerVelocity.z};
    for (unsigned step=0;step<flight_cap;++step) {
        out.updates=step;
        const float elapsed=static_cast<float>(step);
        world::GroundResult ground{},ceiling{};
        if (!inside(position) || !world.ground_at(position.x,position.z,ground)) return false;
        if (world.ceiling_at(position.x,position.z,ceiling) && position.y>ceiling.height) return false;
        const float difference=std::abs(position.y-ground.height);
        if (position.y<ground.height || difference<=.375f) {
            if (difference>.375f) {
                auto above=previous,below=position;
                for (unsigned refinement=0;refinement<16;++refinement) {
                    position={(above.x+below.x)*.5f,(above.y+below.y)*.5f,(above.z+below.z)*.5f};
                    if (!world.ground_at(position.x,position.z,ground)) return false;
                    if (std::abs(position.y-ground.height)<=.375f) break;
                    if (position.y<ground.height) below=position; else above=position;
                }
                position.y=ground.height;
            }
            if (!inside(position)) return false;
            position.y=ground.height+12;
            out.ground=true; out.contact=position; out.surface=ground.surface;
            const IsleGameVec3 target{actor.root.x+motion.x*elapsed,actor.root.y+motion.y*elapsed,actor.root.z+motion.z*elapsed};
            out.actor_distance=std::sqrt(squared(position,target));
            out.accepted=species==123 && out.actor_distance<150;
            return out.accepted;
        }
        const IsleGameVec3 delta{velocity.x*.5f,velocity.y*.5f,velocity.z*.5f};
        world::Vec3 hit{},reflected{};
        out.static_object=world.collide({position.x,position.y,position.z},{delta.x,delta.y,delta.z},hit,reflected);
        if (out.static_object>=0) return false;
        float nearest=2; bool selected=false;
        for (std::size_t i=0;i<sphere_count;++i) {
            auto center=spheres[i].center;
            const bool target=spheres[i].identity==actor.identity;
            if (target) center={center.x+motion.x*elapsed,center.y+motion.y*elapsed,center.z+motion.z*elapsed};
            const float entry=intersection(position,delta,center,spheres[i].radius);
            if (entry<nearest) { nearest=entry; selected=target; }
        }
        if (nearest<1) {
            out.contact=position; out.accepted=species==143 && selected; return out.accepted;
        }
        previous=position;
        velocity.y-=.6f;
        position={position.x+velocity.x*.5f,position.y+velocity.y*.5f,position.z+velocity.z*.5f};
    }
    return false;
}
bool plan(const world::World& world,const IslePlayerSource& player,const Actor& actor,
          const Sphere* spheres,std::size_t count,BeachPesterFacts& facts,IsleGameVec3& aimed) {
    if (facts.required_species==143) {
        if (!projectile_target(player,actor.collision,aimed)) return false;
        return forecast(world,player,actor,spheres,count,facts.motion,aimed,143,facts.ideal_forecast);
    }
    // Search actual queried terrain, not the hidden collision sphere (radius0).
    // The Source 150-unit bound is 3D and uses the Item's contact+12 root.
    constexpr std::array<float,5> offsets{0,-48,48,-96,96};
    for (float x:offsets) for (float z:offsets) {
        world::GroundResult ground{};
        IsleGameVec3 point{actor.root.x+x,0,actor.root.z+z};
        if (!world.ground_at(point.x,point.z,ground)) continue;
        point.y=ground.height;
        const IsleGameVec3 sender{point.x,point.y+12,point.z};
        if (squared(sender,actor.root)>=22500 || !projectile_target(player,point,aimed)) continue;
        if (forecast(world,player,actor,spheres,count,facts.motion,aimed,123,facts.ideal_forecast)) {
            facts.surface_target=point; return true;
        }
        ++facts.forecast_rejections;
    }
    return false;
}
}
int choose_beach_pester_photo_species(application::Photos& photos) {
    const auto snorlax=photos.report_present(143);
    require(bool(snorlax),"Beach canonical Snorlax Report unavailable");
    if (!snorlax.value) return 143;
    const auto scyther=photos.report_present(123);
    require(bool(scyther),"Beach canonical Scyther Report unavailable");
    return scyther.value ? 0 : 123;
}
BeachPesterPhotoPuzzle::BeachPesterPhotoPuzzle(private_post_course::Controller controller,Aim aim,int species)
    : controller_(controller),aim_(aim) {
    require(controller_.set && aim_.point,"Beach Pester requires real pad and pure aim bridges");
    require(species==143 || species==123,"Beach Pester species must be143 or123");
    facts_.required_species=species;
}
void BeachPesterPhotoPuzzle::publish(std::uint16_t buttons,std::int8_t x,std::int8_t y) noexcept {
    controller_.set(controller_.owner,buttons,x,y); facts_.owns_pad=true;
    engaged_=buttons!=0 || x!=0 || y!=0;
}
void BeachPesterPhotoPuzzle::stop(BeachPesterDisposition reason) noexcept {
    if (facts_.complete || facts_.exhausted) return;
    facts_.exhausted=true; facts_.capture_pending=false; facts_.phase=BeachPesterPhase::finished;
    facts_.disposition=reason; capture_armed_=false; tracking_=false;
    neutral_pending_=engaged_ || pending_press_ || pending_release_;
}
void BeachPesterPhotoPuzzle::observe(const application::RuntimeView& view,std::uint64_t updates) {
    if (sampled_ && updates==observation_update_) return;
    require(!sampled_ || updates>observation_update_,"Beach Pester completed-update ordinal regressed");
    const bool consecutive=sampled_ && updates==observation_update_+1;
    sampled_=true; observation_update_=updates; facts_.last_update=updates; facts_.owns_pad=false; active_=false;
    if (view.beach && view.beach->error()!=ISLE_GAME_OK) {
        facts_.source_status=view.beach->error(); stop(BeachPesterDisposition::source_error);
        throw std::runtime_error(view.beach->message());
    }
    if (!view.beach || view.beach->status()!=beach::LifecycleStatus::running || view.beach->source().D_beach_802CC018) {
        if (started_) stop(BeachPesterDisposition::course_ended);
        return;
    }
    const auto& entry=*view.beach; const auto* player=entry.source().player;
    const auto* actors=entry.actors(); const auto* items=entry.items_owner(); const auto* world=entry.world_owner();
    if (!player || !actors || !items || !world || world->retired()) {
        if (started_) stop(BeachPesterDisposition::course_ended);
        return;
    }
    require(player->status==ISLE_GAME_OK && player->photo && player->photo->status==ISLE_GAME_OK,
            "Beach Pester actual Player/Photo failed");
    require(world->error().empty(),"Beach Pester actual World failed");
    const auto* block=world->current();
    if (!block || !block->resource) {
        if (started_) stop(BeachPesterDisposition::course_ended);
        return;
    }
    active_=true; actor_count_=0; sphere_count_=0; item_count_=0;
    const auto offset=block->resource->metadata.world_position;
    const IsleGameVec3 origin{offset.x*100,offset.y*100,offset.z*100};
    facts_.current_block=static_cast<int>(block->index);
    facts_.progress=static_cast<float>(block->index)+player->movement.move_time;
    IslePlayerPhotoFocus focus{};
    require(isle_player_photo_published_focus(player->photo,&focus)==ISLE_GAME_OK,"Beach Pester actual focus unavailable");
    facts_.focus_species=focus.pokemon_id; facts_.focus_context=focus.context; facts_.focus_flags=focus.flags;
    facts_.focus_actor=key(actors->instance(focus.object));
    IsleGameObject* previous=nullptr;
    for (;;) {
        IsleGameObject* object=nullptr; IsleActorSource* source=nullptr; IsleGameModel* model=nullptr;
        require(isle_actor_source_link3_next(player->game,previous,&object,&source,&model)==ISLE_GAME_OK,
                "Beach Pester actual Actor traversal rejected");
        if (!object) break;
        previous=object;
        const auto instance=actors->instance(object);
        require(bool(instance),"Beach Pester Actor incarnation unavailable");
        if (source->tangible && source->collisionRadius>0) {
            if (sphere_count_==spheres_.size()) { stop(BeachPesterDisposition::capacity); break; }
            spheres_[sphere_count_++]={key(instance),source->collPosition,source->collisionRadius};
        }
        if (source->id!=facts_.required_species || !model) continue;
        const IsleGameModelPlacement* placement=nullptr;
        require(isle_game_model_placement(player->game,model,&placement)==ISLE_GAME_OK && placement,
                "Beach Pester actual Actor placement unavailable");
        if (placement->mode==ISLE_GAME_MODEL_PLACEMENT_DISABLED) continue;
        if (actor_count_==actors_.size()) { stop(BeachPesterDisposition::capacity); break; }
        auto& a=actors_[actor_count_++]; a={}; a.identity=key(instance);
        a.interaction=key(items->instance(source->interactionTarget)); a.interaction_distance=source->interactionDist;
        a.root=placement->transform.position; a.collision=source->collPosition; a.radius=source->collisionRadius;
        a.flags=source->flags; a.behavior=source->behavior;
        a.graph=source->transitionGraph ? source->transitionGraph->source_identity : 0;
        IsleGameSourceCallback callback=nullptr;
        if (source->stateProc) require(isle_game_process_source_function(player->game,source->stateProc,&callback)==ISLE_GAME_OK,
                                      "Beach Pester actual Source callback unavailable");
        a.state=state_identity(callback); a.process_flags=source->processFlags; a.model_animation=source->modelAnims;
        a.material_animation=source->matAnims;
        a.animation_time=actors->animation().model(instance).animation_time;
        IsleGameObjectMetadata metadata{};
        require(isle_game_object_metadata(player->game,object,&metadata)==ISLE_GAME_OK,"Beach Pester Actor metadata unavailable");
        a.object_flags=metadata.flags; a.object_id=metadata.id;
        a.hidden=facts_.required_species==123 ? a.state==0x802c8e90 && a.radius==0 : (a.flags&8u)!=0;
        a.reaction=facts_.required_species==143 ? a.state==0x802c5768 : a.state==0x802c8f48;
        a.ready=accepts(*source,facts_.required_species) && source->tangible && finite(a.root) && finite(a.collision) &&
            (facts_.required_species==143 ? !a.reaction && a.radius>0 && !(metadata.flags&1u) && (a.flags&8u) : a.hidden);
        const float pitch=std::atan2(a.collision.y-player->CameraEyePos.y,
            std::hypot(a.collision.x-player->CameraEyePos.x,a.collision.z-player->CameraEyePos.z));
        a.eligible=source->tangible && a.radius>0 && !(metadata.flags&1u) && (a.flags&4u) && !(a.flags&8u) &&
            source->playerDist>=0 && source->playerDist<=10000 && pitch>=player->MinPitch && pitch<=player->MaxPitch;
        a.focused=a.eligible && focus.present && focus.object==object && focus.pokemon_id==facts_.required_species &&
            (focus.flags&4u) && !(focus.flags&8u);
        a.distance=squared(a.root,player->CameraEyePos);
    }
    for (std::size_t slot=0;slot<items->animation().slot_count();++slot) {
        const auto instance=items->slot_instance(slot);
        if (!instance) continue;
        const auto family=items->family(instance);
        if (!family || (*family!=items::Family::pester_ball && *family!=items::Family::apple)) continue;
        auto* object=items->object(instance);
        auto* model=items->native_model(instance);
        const auto* source=items->source(instance);
        require(object && model,"Beach Pester Item native identity unavailable");
        require(source,"Beach Pester actual Item Source unavailable");
        if (item_count_==items_.size()) { stop(BeachPesterDisposition::capacity); break; }
        const IsleGameModelNode* nodes=nullptr; std::size_t count=0;
        require(isle_game_model_peek_nodes(player->game,model,&nodes,&count)==ISLE_GAME_OK && nodes && count,
                "Beach Pester actual Item root unavailable");
        IsleGameObjectMetadata metadata{};
        require(isle_game_object_metadata(player->game,object,&metadata)==ISLE_GAME_OK,
                "Beach Pester Item metadata unavailable");
        items_[item_count_++]={key(instance),nodes[0].transform.position,source->prevPos,source->itemID,source->state,
                              source->flags,metadata.flags,metadata.id,source->restTimer};
    }
    const Actor* target=nullptr;
    for (std::size_t i=0;i<actor_count_;++i) {
        const auto& a=actors_[i];
        if (facts_.target[0] ? a.identity==facts_.target : a.ready && (!target || a.distance<target->distance)) target=&a;
    }
    facts_.target_present=target!=nullptr; facts_.target_ready=target && target->ready;
    facts_.eligible=target && target->eligible; facts_.focused=target && target->focused; facts_.motion={};
    if (target) {
        facts_.root=target->root; facts_.global_root=global(target->root,origin); facts_.collision=target->collision;
        facts_.target_graph=target->graph; facts_.target_state=target->state; facts_.target_flags=target->flags;
        facts_.target_object_flags=target->object_flags; facts_.target_object_id=target->object_id;
        facts_.target_behavior=target->behavior; facts_.collision_radius=target->radius;
        facts_.process_flags=target->process_flags; facts_.model_animation=target->model_animation;
        facts_.animation_time=target->animation_time;
        facts_.material_animation=target->material_animation;
        if (consecutive && previous_target_==target->identity)
            facts_.motion={facts_.global_root.x-previous_global_root_.x,facts_.global_root.y-previous_global_root_.y,
                           facts_.global_root.z-previous_global_root_.z};
    }
    const auto& photo=*player->photo;
    facts_.film=photo.gPhotoCount; facts_.qualified=photo.D_800AE27C;
    require(facts_.film>=0 && facts_.film<=60 && facts_.qualified>=0 && facts_.qualified<=facts_.film,
            "Beach Pester actual roll counters invalid");
    facts_.taken_context=photo.PokemonDetector_TakenPhotoIndex;
    facts_.player_ready=player->D_80382D04_523114==1 && !player->IsInputDisabled && !player->IsPaused;
    // Beach does not expose GameState. These are the actual Source UI abilities,
    // initialized from canonical rewards; real B consumption is still mandatory.
    facts_.available=facts_.pester_available=player->ui && player->ui->Icons_NumItemsAvailable>=2;
    if (!started_ && facts_.player_ready && facts_.available) { started_=true; facts_.initial_update=updates; }
    if (observed_roll_<0) { observed_roll_=facts_.film; observed_qualified_=facts_.qualified; }
    require(facts_.film>=observed_roll_ && facts_.qualified>=observed_qualified_,"Beach Pester roll counters regressed");
    const bool one=facts_.film==observed_roll_+1 && facts_.qualified==observed_qualified_+1;
    for (int slot=observed_roll_;slot<facts_.film;++slot) {
        const auto& image=photo.D_800B0598[slot]; facts_.capture_observed=true;
        facts_.raw_primary=image.pokemons[0].pokemonID;
        if (facts_.raw_primary!=facts_.required_species) continue;
        ++facts_.captures;
        bool coherent=!facts_.exhausted && one && consecutive && capture_armed_ && target &&
            target->identity==armed_subject_ && armed_subject_==facts_.target &&
            armed_slot_==slot && armed_qualified_+1==facts_.qualified && facts_.reveal_seen && facts_.sender_seen &&
            facts_.release_update>=facts_.consumed_update &&
            (player->input.pressed_buttons&shutter) && (player->input.current_buttons&shutter) &&
            image.levelID==0 && (image.unk_00_16&0xe0u)==0x20u && facts_.taken_context>=0 && facts_.taken_context<2;
        if (coherent) {
            const auto& context=photo.PokemonDetector_Photo[facts_.taken_context];
            facts_.taken_actor=key(actors->instance(context.pokemonInFocus));
            coherent=facts_.taken_actor==facts_.target;
            bool copied=false;
            for (std::size_t i=0;i<12 && coherent;++i) {
                if (key(actors->instance(context.pokemonObjects[i]))!=facts_.target) continue;
                facts_.taken_species=context.main.pokemons[i].pokemonID;
                copied=facts_.taken_species==facts_.required_species && private_course::photo_context_copied(image,context,i);
                break;
            }
            if (!copied) ++facts_.full_copy_mismatches;
            coherent=coherent && copied;
        }
        if (coherent) {
            facts_.complete=true; facts_.capture_pending=false; facts_.capture_update=updates; facts_.captured_slot=slot;
            facts_.captured_actor=facts_.taken_actor; facts_.captured_slots|=std::uint64_t{1}<<static_cast<unsigned>(slot);
            facts_.phase=BeachPesterPhase::finished; facts_.disposition=BeachPesterDisposition::qualified;
            neutral_pending_=engaged_;
        } else ++facts_.capture_mismatches;
    }
    observed_roll_=facts_.film; observed_qualified_=facts_.qualified; capture_armed_=false;
    if (pending_press_ && (player->input.current_buttons&pester)) {
        const bool pressed=(player->input.pressed_buttons&pester)!=0;
        if (pressed) ++facts_.pressed;
        if (pressed && player->ThrowItemTimeout>0 && player->LastItemId==ISLE_ITEM_PESTER_BALL &&
            player->gDirectionIndex>=0 && !player->TargetDirectionZoomedIn && player->TurnToDirSpeed==0) {
            ++facts_.consumed; facts_.consumed_update=updates; tracking_=true; facts_.item={};
            previous_item_={}; previous_ground_=false;
        } else stop(BeachPesterDisposition::press_timeout);
        pending_press_=false; pending_release_=true; release_start_=updates;
    }
    if (pending_press_ && updates-facts_.request_update>=8) {
        pending_press_=false; pending_release_=true; release_start_=updates; stop(BeachPesterDisposition::press_timeout);
    }
    if (pending_release_ && !(player->input.current_buttons&pester)) {
        pending_release_=false; ++facts_.releases; facts_.release_update=updates;
    }
    if (pending_release_ && updates-release_start_>=8) { pending_release_=false; stop(BeachPesterDisposition::release_timeout); }
    if (started_ && updates-facts_.initial_update>=overall_cap) stop(BeachPesterDisposition::overall_limit);
    if (!facts_.complete && !facts_.exhausted && facts_.target[0] && !target) {
        facts_.cleanup_seen=facts_.reaction_seen && consecutive && previous_target_==facts_.target;
        if (facts_.cleanup_seen) facts_.cleanup_update=updates;
        stop(facts_.cleanup_seen ? BeachPesterDisposition::cleanup : BeachPesterDisposition::actor_lost);
    }
    if (!facts_.complete && !facts_.exhausted && facts_.required_species==143 &&
        (facts_.current_block>3 || (facts_.current_block==3 && player->movement.move_time>=.3f)))
        stop(BeachPesterDisposition::progress_cleanup);
    if (!facts_.complete && !facts_.exhausted && tracking_) {
        const Item* item=nullptr; unsigned matches=0;
        for (std::size_t i=0;i<item_count_;++i) {
            const auto& candidate=items_[i]; bool existed=false;
            for (std::size_t j=0;j<prethrow_count_;++j) existed|=candidate.identity==prethrow_items_[j];
            if (candidate.kind!=ISLE_ITEM_PESTER_BALL || (facts_.item[0] ? candidate.identity!=facts_.item : existed)) continue;
            item=&candidate; ++matches;
        }
        facts_.item_present=matches==1;
        if (matches>1) stop(BeachPesterDisposition::item_ambiguous);
        else if (item) {
            facts_.item=item->identity; facts_.item_kind=item->kind; facts_.item_state=item->state;
            facts_.item_flags=item->flags; facts_.item_object_flags=item->object_flags;
            facts_.item_object_id=item->object_id; facts_.item_position=item->position;
            const bool ground=(item->flags&(ISLE_ITEM_BOUNCED|ISLE_ITEM_TOUCHED_GROUND|ISLE_ITEM_DELETED))==
                (ISLE_ITEM_BOUNCED|ISLE_ITEM_TOUCHED_GROUND);
            bool physical=false;
            if (facts_.required_species==143) physical=(item->object_flags&1u) &&
                !(item->flags&(ISLE_ITEM_BOUNCED|ISLE_ITEM_TOUCHED_GROUND|ISLE_ITEM_DELETED));
            else if (ground && !previous_ground_ && consecutive && (!previous_item_[0] || previous_item_==item->identity)) {
                world::GroundResult contact{},prior{};
                physical=(item->object_flags&1u) && world->ground_at(item->position.x,item->position.z,contact) &&
                    std::abs(item->position.y-contact.height-12)<=.5f &&
                    world->ground_at(item->previous.x,item->previous.z,prior) && item->previous.y<=prior.height+.375f;
            }
            // Item movement and Actor command dispatch need not occupy the same
            // completed update. Retain the real first-ground receipt, then join
            // it to the later Source state/sender; never re-infer a landing.
            if (facts_.required_species==123 && physical && target && consecutive &&
                previous_target_==facts_.target && previous_hidden_ && !facts_.physical_seen) {
                facts_.physical_seen=true; facts_.physical_update=updates;
            }
            const bool event=facts_.required_species==143 ? physical :
                facts_.physical_seen && updates-facts_.physical_update<=8;
            if (event && target && consecutive && previous_target_==facts_.target && previous_hidden_ &&
                !previous_reaction_ && target->reaction && target->interaction==item->identity &&
                (facts_.required_species==143 || (target->interaction_distance>=0 && target->interaction_distance<150 && target->radius>0))) {
                if (!facts_.physical_seen) { facts_.physical_seen=true; facts_.physical_update=updates; }
                facts_.sender_seen=true; facts_.sender_update=updates; facts_.interaction_sender=target->interaction;
                facts_.accepted_item=item->identity; facts_.accepted_distance=target->interaction_distance;
                facts_.reaction_seen=true; facts_.reaction_update=updates; tracking_=false;
                facts_.phase=BeachPesterPhase::reaction;
            } else if (target && target->reaction) stop(BeachPesterDisposition::reaction_unattributed);
            previous_item_=item->identity; previous_ground_=ground;
        } else if (updates>facts_.consumed_update+2) stop(BeachPesterDisposition::item_lost);
        ++facts_.flight_updates;
        if (tracking_ && updates-facts_.consumed_update>=flight_cap) stop(BeachPesterDisposition::flight_timeout);
        if (tracking_ && facts_.physical_seen && updates-facts_.physical_update>=8)
            stop(BeachPesterDisposition::reaction_timeout);
    }
    if (!facts_.complete && !facts_.exhausted && target && facts_.reaction_seen) {
        const bool visible=target->reaction && !(target->flags&8u) && !(target->object_flags&1u) && target->radius>0;
        if (visible && !facts_.reveal_seen) {
            facts_.reveal_seen=true; facts_.reveal_update=updates; neutral_pending_=engaged_;
        }
        if (!facts_.reveal_seen && updates-facts_.reaction_update>=reaction_cap) stop(BeachPesterDisposition::reaction_timeout);
        if (facts_.reveal_seen && !visible) stop(BeachPesterDisposition::window_closed);
        if (facts_.reveal_seen && updates-facts_.reveal_update>=capture_cap) stop(BeachPesterDisposition::window_closed);
        if (!facts_.exhausted && facts_.reveal_seen) {
            facts_.capture_pending=true; facts_.phase=BeachPesterPhase::capture;
            facts_.disposition=BeachPesterDisposition::capture_pending;
            if (target->focused && facts_.player_ready && player->gDirectionIndex==-1 &&
                !player->TargetDirectionZoomedIn && player->sTimerAfterPhotoTaken<0 &&
                !(player->input.current_buttons&shutter) && facts_.film<60) {
                capture_armed_=true; armed_subject_=target->identity; armed_slot_=facts_.film; armed_qualified_=facts_.qualified;
            }
        }
    }
    if (target && facts_.target[0] && target->hidden && !facts_.hidden_seen) {
        facts_.hidden_seen=true; facts_.hidden_update=updates;
    }
    previous_target_=target ? target->identity : BeachPesterIncarnation{};
    previous_hidden_=target && target->hidden; previous_reaction_=target && target->reaction;
    if (target) previous_global_root_=global(target->root,origin);
}
bool BeachPesterPhotoPuzzle::update(const application::RuntimeView& view,std::uint64_t updates) {
    observe(view,updates);
    if (control_sampled_ && control_update_==updates) return facts_.owns_pad;
    control_sampled_=true; control_update_=updates; facts_.owns_pad=false;
    const auto relinquish=[this]() {
        if (!engaged_) return false;
        publish(); return true;
    };
    if (neutral_pending_) { neutral_pending_=false; publish(); return true; }
    if (!active_ || !view.beach) return relinquish();
    const auto* player=view.beach->source().player; const auto* world=view.beach->world_owner();
    if (!player || !world || world->retired()) return relinquish();
    if (pending_press_ || pending_release_) {
        facts_.phase=pending_press_ ? BeachPesterPhase::press : BeachPesterPhase::release;
        publish(pending_press_ && !facts_.exhausted ? pester : 0); return true;
    }
    if (facts_.complete || facts_.exhausted || facts_.capture_pending) return relinquish();
    if (!facts_.available) { facts_.disposition=BeachPesterDisposition::pester_unavailable; return relinquish(); }
    if (!facts_.player_ready) { facts_.disposition=BeachPesterDisposition::player_not_ready; return relinquish(); }
    if (tracking_ || facts_.reaction_seen) {
        facts_.phase=tracking_ ? BeachPesterPhase::flight : BeachPesterPhase::reaction;
        facts_.disposition=tracking_ ? BeachPesterDisposition::item_outstanding : BeachPesterDisposition::none;
        publish(); return true;
    }
    if (facts_.film>=59) { facts_.disposition=BeachPesterDisposition::film_reserved; return relinquish(); }
    const Actor* target=nullptr;
    for (std::size_t i=0;i<actor_count_;++i) {
        const auto& candidate=actors_[i];
        if (facts_.target[0] ? candidate.identity==facts_.target : candidate.ready && (!target || candidate.distance<target->distance))
            target=&candidate;
    }
    if (!target) { facts_.disposition=BeachPesterDisposition::target_absent; return relinquish(); }
    if (!target->ready) { facts_.disposition=BeachPesterDisposition::target_not_ready; return relinquish(); }
    if (facts_.requests>=1) { stop(BeachPesterDisposition::request_limit); publish(); neutral_pending_=false; return true; }
    if ((player->input.current_buttons&shutter) || player->sTimerAfterPhotoTaken>=0) return relinquish();
    if (player->ThrowItemTimeout!=0) { facts_.disposition=BeachPesterDisposition::item_outstanding; return relinquish(); }
    if (!facts_.target[0]) {
        IsleGameVec3 aimed{};
        if (!plan(*world,*player,*target,spheres_.data(),sphere_count_,facts_,aimed)) {
            ++facts_.forecast_rejections; facts_.disposition=BeachPesterDisposition::unsafe_geometry; return false;
        }
        facts_.target=target->identity; facts_.hidden_seen=target->hidden;
        if (target->hidden) facts_.hidden_update=updates;
    }
    if (player->gDirectionIndex<0 || player->TargetDirectionZoomedIn || player->TurnToDirSpeed!=0) {
        if (++recovery_>128) { stop(BeachPesterDisposition::recovery_limit); publish(); neutral_pending_=false; return true; }
        ++facts_.recovery_updates; facts_.phase=BeachPesterPhase::unzoom;
        std::uint16_t buttons=0;
        if (player->ZoomSwitchMode==1 && player->gDirectionIndex==-1 && !player->TargetDirectionZoomedIn &&
            !zoom_out_requested_ && !(player->input.current_buttons&zoom)) { buttons=zoom; zoom_out_requested_=true; }
        publish(buttons); return true;
    }
    zoom_out_requested_=false;
    if (++steering_>512) { stop(BeachPesterDisposition::steering_limit); publish(); neutral_pending_=false; return true; }
    ++facts_.steering_updates; facts_.phase=BeachPesterPhase::aim;
    IsleGameVec3 aimed{};
    if (!plan(*world,*player,*target,spheres_.data(),sphere_count_,facts_,aimed)) {
        ++facts_.forecast_rejections; facts_.disposition=BeachPesterDisposition::unsafe_geometry; publish(); return true;
    }
    std::int8_t x=0,y=0; aim_.point(aim_.owner,*player,aimed,false,x,y);
    facts_.input_target=aimed; facts_.disposition=BeachPesterDisposition::none;
    std::uint16_t buttons=0;
    // Actual quantized cam_target must independently pass before publishing B.
    if (!x && !y && !(player->input.current_buttons&pester) &&
        forecast(*world,*player,*target,spheres_.data(),sphere_count_,facts_.motion,player->cam_target,
                 facts_.required_species,facts_.actual_forecast)) {
        bool outstanding=false;
        for (std::size_t i=0;i<item_count_;++i) {
            const auto& item=items_[i];
            outstanding|=item.kind==ISLE_ITEM_PESTER_BALL && item.state==ISLE_ITEM_FLYING &&
                (!(item.object_flags&1u) || item.rest_timer<=0) && !(item.flags&ISLE_ITEM_DELETED);
        }
        if (!outstanding) {
            prethrow_count_=item_count_;
            for (std::size_t i=0;i<item_count_;++i) prethrow_items_[i]=items_[i].identity;
            facts_.request_update=updates; ++facts_.requests; pending_press_=true;
            facts_.phase=BeachPesterPhase::press; buttons=pester;
        } else facts_.disposition=BeachPesterDisposition::item_outstanding;
    }
    publish(buttons,x,y); return true;
}
}

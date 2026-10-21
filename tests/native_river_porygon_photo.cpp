#include "native_river_porygon_photo.hpp"
#include "captured_photo_identity.hpp"
#include "application_runtime.hpp"
#include "application_photos.hpp"
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
#include <cmath>
#include <stdexcept>

namespace isle::private_campaign {
namespace {
constexpr std::uint16_t shutter=0x8000, pester=0x4000, zoom=0x2000;
constexpr std::uint64_t overall_cap=8192, flight_cap=512, reveal_cap=256;
using porygon_detail::Actor;
using porygon_detail::Item;
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
PorygonIncarnation key(menu::animation::InstanceHandle value) {
    return {value.pool, value.slot, value.incarnation};
}
bool finite(const IsleGameVec3& p) {
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
}
float squared(const IsleGameVec3& a, const IsleGameVec3& b) {
    const float x=a.x-b.x, y=a.y-b.y, z=a.z-b.z;
    return x*x+y*y+z*z;
}
IsleGameVec3 global(const IsleGameVec3& p, const IsleGameVec3& origin) {
    return {p.x+origin.x, p.y+origin.y, p.z+origin.z};
}
std::uint32_t state_identity(IsleGameSourceCallback callback) {
    if (callback==func_802DD0E0_6C4EC0) return 0x802dd0e0u;
    if (callback==func_802DD1D4_6C4FB4) return 0x802dd1d4u;
    if (callback==func_802DD2BC_6C509C) return 0x802dd2bcu;
    if (callback==func_802DD398_6C5178) return 0x802dd398u;
    return 0;
}
bool accepts_ground(const IsleActorSource& actor) {
    if (!actor.transitionGraph || actor.transitionGraph->source_identity!=0x802e39b8u) return false;
    for (std::size_t i=0;i<actor.transitionGraph->count;++i) {
        const auto& row=actor.transitionGraph->rows[i];
        if (row.cmd==58) break;
        if (row.cmd==29 && row.state==func_802DD1D4_6C4FB4) return true;
    }
    return false;
}
bool inside_world(const IsleGameVec3& p) {
    return finite(p) && p.x>-10000 && p.x<10000 && p.y>-10000 && p.y<10000 &&
        p.z>-10000 && p.z<10000;
}
// df8815e items.c Items_UpdateItemMovement/BounceFromGround: test the
// current position before static collision, then subtract gravity and advance.
// This forecasts only input geometry. Item flags/sender/reveal remain receipts.
bool forecast(const world::World& world, const IslePlayerSource& player, const IsleGameVec3& actor,
              const IsleGameVec3& ray, RiverPorygonForecast& out) {
    using Result=RiverPorygonForecastDisposition;
    out={};
    const IsleGameCameraParameters* camera=nullptr;
    require(isle_game_camera_parameters(player.game,player.camera,&camera)==ISLE_GAME_OK && camera,
            "River Porygon actual launch camera unavailable");
    IsleGameVec3 position{camera->look_at.eye[0],camera->look_at.eye[1],camera->look_at.eye[2]};
    IsleGameVec3 previous=position;
    IsleGameVec3 velocity{ray.x-position.x,ray.y-position.y+100,ray.z-position.z};
    const float length=std::hypot(velocity.x,velocity.y,velocity.z);
    if (!(length>0) || !std::isfinite(length) || !finite(actor)) {
        out.disposition=Result::invalid_launch; return false;
    }
    velocity={velocity.x*50/length+player.PlayerVelocity.x,velocity.y*50/length+player.PlayerVelocity.y,
        velocity.z*50/length+player.PlayerVelocity.z};
    if (!finite(velocity)) { out.disposition=Result::invalid_launch; return false; }
    for (unsigned step=0;step<flight_cap;++step) {
        out.flight_updates=step;
        if (!inside_world(position)) { out.disposition=Result::outside_world; return false; }
        world::GroundResult ground{},ceiling{};
        if (!world.ground_at(position.x,position.z,ground)) {
            out.disposition=Result::ground_unavailable; return false;
        }
        // Source gives ceiling (including simultaneous ground/ceiling) priority.
        if (world.ceiling_at(position.x,position.z,ceiling) && position.y>ceiling.height) {
            out.first_surface=ceiling.surface; out.first_position=position;
            out.disposition=Result::ceiling_obstruction; return false;
        }
        const float ground_delta=std::abs(position.y-ground.height);
        if (position.y<ground.height || ground_delta<=.375f) {
            if (ground_delta>.375f) {
                // Reproduce Source's 16-step contact refinement, including
                // the last queried surface, not a straight-ray intersection.
                auto above=previous, below=position;
                for (unsigned refinement=0;refinement<16;++refinement) {
                    position={(above.x+below.x)*.5f,(above.y+below.y)*.5f,(above.z+below.z)*.5f};
                    if (!world.ground_at(position.x,position.z,ground)) {
                        out.disposition=Result::ground_unavailable; return false;
                    }
                    if (std::abs(position.y-ground.height)<=.375f) break;
                    if (position.y<ground.height) below=position;
                    else above=position;
                }
            }
            if (ground_delta>.375f) position.y=ground.height;
            // CollideWithGround bounds-checks the contact before lifting root.
            if (!inside_world(position)) { out.disposition=Result::outside_world; return false; }
            position.y=ground.height+12;
            out.first_surface=ground.surface; out.first_position=position;
            out.actor_distance=std::sqrt(squared(position,actor));
            if (ground.surface!=0xff7fb2) { out.disposition=Result::wrong_surface; return false; }
            if (!(squared(position,actor)<1000000)) {
                out.disposition=Result::sender_out_of_range; return false;
            }
            out.disposition=Result::qualifying_ground; return true;
        }
        const IsleGameVec3 delta{velocity.x*.5f,velocity.y*.5f,velocity.z*.5f};
        world::Vec3 hit{},reflected{};
        out.static_object=world.collide({position.x,position.y,position.z},{delta.x,delta.y,delta.z},hit,reflected);
        if (out.static_object>=0) {
            out.first_position={hit.x,hit.y,hit.z};
            out.disposition=Result::static_obstruction; return false;
        }
        previous=position;
        velocity.y-=.6f;
        position={position.x+velocity.x*.5f,position.y+velocity.y*.5f,position.z+velocity.z*.5f};
    }
    out.disposition=Result::flight_limit; return false;
}
struct SurfacePlan {
    const world::World& world;
    const IslePlayerSource& player;
    const IsleGameVec3& actor;
    RiverPorygonFacts& facts;
    IsleGameVec3& aimed;
    static bool accept(void* owner, const IsleGameVec3& point) {
        auto& plan=*static_cast<SurfacePlan*>(owner);
        ++plan.facts.forecast_candidates;
        plan.facts.ideal_forecast={};
        if (!projectile_target(plan.player,point,plan.aimed)) {
            plan.facts.ideal_forecast.disposition=RiverPorygonForecastDisposition::invalid_launch;
        } else if (forecast(plan.world,plan.player,plan.actor,plan.aimed,plan.facts.ideal_forecast)) return true;
        ++plan.facts.forecast_rejections;
        plan.facts.rejected_forecast=plan.facts.ideal_forecast;
        return false;
    }
};
}

bool needs_river_porygon_photo(application::Photos& photos) {
    const auto report=photos.report_present(137);
    require(bool(report), "River canonical Porygon Report unavailable");
    return !report.value;
}
RiverPorygonPhotoPuzzle::RiverPorygonPhotoPuzzle(private_post_course::Controller controller, Aim aim)
    : controller_(controller), aim_(aim) {
    require(controller_.set && aim_.point, "River Porygon requires real pad and pure aim bridges");
}
void RiverPorygonPhotoPuzzle::publish(std::uint16_t buttons, std::int8_t x, std::int8_t y) noexcept {
    controller_.set(controller_.owner,buttons,x,y);
    facts_.owns_pad=true;
    neutral_pending_=buttons || x || y;
}
void RiverPorygonPhotoPuzzle::stop(RiverPorygonDisposition disposition) noexcept {
    if (facts_.exhausted) return;
    facts_.exhausted=true; facts_.disposition=disposition;
    facts_.phase=RiverPorygonPhase::finished; capture_armed_=false;
}
void RiverPorygonPhotoPuzzle::observe(const application::RuntimeView& view, std::uint64_t updates) {
    if (sampled_ && updates==observation_update_) return;
    require(!sampled_ || updates>observation_update_, "River Porygon completed update regressed");
    const bool consecutive=sampled_ && updates==observation_update_+1;
    sampled_=true; observation_update_=updates; facts_.last_update=updates;
    facts_.owns_pad=false; active_=false;
    if (!view.river || view.river->status()!=course::LifecycleStatus::running) {
        if (started_ && !facts_.complete) stop(RiverPorygonDisposition::course_ended);
        return;
    }
    const auto& entry=*view.river;
    const auto* host=entry.native_host();
    const auto* player=entry.adapter().native.player();
    if (!host || !host->execution_open() || !player) return;
    require(player->status==ISLE_GAME_OK && player->photo && player->photo->status==ISLE_GAME_OK,
            "River Porygon Player/Photo failed");
    auto* actors=host->actors(); auto* items=host->items_owner(); auto* world=host->world_owner();
    require(actors && items && world, "River Porygon Actor/Item/World unavailable");
    const auto* block=world->current();
    if (!block || !block->resource || world->retired() || !world->error().empty()) {
        if (started_ && !facts_.complete) stop(RiverPorygonDisposition::course_ended);
        return;
    }
    active_=true; actor_count_=0; item_count_=0;
    facts_.current_block=static_cast<int>(block->index);
    const auto offset=block->resource->metadata.world_position;
    const IsleGameVec3 origin{100*offset.x,100*offset.y,100*offset.z};
    IslePlayerPhotoFocus focus{};
    require(isle_player_photo_published_focus(player->photo,&focus)==ISLE_GAME_OK,
            "River Porygon published focus unavailable");
    facts_.focus_species=focus.pokemon_id; facts_.focus_flags=focus.flags; facts_.focus_context=focus.context;
    IsleGameObject* previous=nullptr;
    for (;;) {
        IsleGameObject* object=nullptr; IsleActorSource* source=nullptr; IsleGameModel* model=nullptr;
        require(isle_actor_source_link3_next(player->game,previous,&object,&source,&model)==ISLE_GAME_OK,
                "River Porygon actual Actor traversal rejected");
        if (!object) break;
        previous=object;
        // Behavior2 is the hidden-exit progression subject, never a photo lure.
        if (source->id!=137 || (source->behavior!=1 && source->behavior!=3) || !model) continue;
        if (actor_count_==actors_.size()) { stop(RiverPorygonDisposition::capacity_limit); break; }
        const auto instance=actors->instance(object);
        require(bool(instance), "River Porygon Actor incarnation unavailable");
        const IsleGameModelPlacement* placement=nullptr;
        require(isle_game_model_placement(player->game,model,&placement)==ISLE_GAME_OK && placement,
                "River Porygon model placement unavailable");
        if (placement->mode==ISLE_GAME_MODEL_PLACEMENT_DISABLED) continue;
        IsleGameObjectMetadata metadata{};
        require(isle_game_object_metadata(player->game,object,&metadata)==ISLE_GAME_OK,
                "River Porygon object metadata unavailable");
        IsleGameSourceCallback callback=nullptr;
        if (source->stateProc)
            require(isle_game_process_source_function(player->game,source->stateProc,&callback)==ISLE_GAME_OK,
                    "River Porygon Source state unavailable");
        auto& a=actors_[actor_count_++]; a={}; a.present=true;
        a.identity=key(instance); a.interaction=key(items->instance(source->interactionTarget));
        a.behavior=source->behavior; a.graph=source->transitionGraph ? source->transitionGraph->source_identity : 0;
        a.state=state_identity(callback); a.flags=source->flags; a.object_flags=metadata.flags;
        a.object_id=metadata.id; a.position=placement->transform.position; a.collision=source->collPosition;
        a.process_flags=source->processFlags; a.model_animation=source->modelAnims;
        a.animation_time=actors->animation().model(instance).animation_time;
        a.hidden=callback==func_802DD0E0_6C4EC0 && (a.flags&8u) && accepts_ground(*source);
        a.revealing=callback==(a.behavior==1 ? func_802DD2BC_6C509C : func_802DD398_6C5178);
        const float horizontal=std::hypot(a.collision.x-player->CameraEyePos.x,a.collision.z-player->CameraEyePos.z);
        const float pitch=std::atan2(a.collision.y-player->CameraEyePos.y,horizontal);
        a.eligible=source->tangible && !(metadata.flags&1u) && (source->flags&4u) && !(source->flags&8u) &&
            source->playerDist>=0 && source->playerDist<=10000 && pitch>=player->MinPitch && pitch<=player->MaxPitch;
        a.focused=a.eligible && focus.present && focus.object==object && focus.pokemon_id==137 &&
            (focus.flags&4u) && !(focus.flags&8u);
    }
    for (const auto& binding:items->render_bindings()) {
        if (!binding.model.object) continue;
        const auto instance=items->instance(binding.model.object);
        require(bool(instance), "River Porygon Item incarnation unavailable");
        const auto family=items->family(instance);
        if (!family || *family!=items::Family::pester_ball) continue;
        if (item_count_==items_.size()) { stop(RiverPorygonDisposition::capacity_limit); break; }
        const auto* source=items->source(binding.model.object);
        require(source, "River Porygon Item Source unavailable");
        const IsleGameModelNode* nodes=nullptr; std::size_t count=0;
        require(isle_game_model_peek_nodes(player->game,binding.model.model,&nodes,&count)==ISLE_GAME_OK && nodes && count,
                "River Porygon Item root unavailable");
        IsleGameObjectMetadata metadata{};
        require(isle_game_object_metadata(player->game,binding.model.object,&metadata)==ISLE_GAME_OK,
                "River Porygon Item metadata unavailable");
        items_[item_count_++]={key(instance),source->itemID,source->state,source->flags,metadata.flags,metadata.id,
                              nodes[0].transform.position,source->prevPos};
    }
    const Actor* target=nullptr;
    for (std::size_t i=0;i<actor_count_;++i) if (actors_[i].identity==facts_.target) target=&actors_[i];
    facts_.target_present=target!=nullptr;
    if (target) {
        facts_.target_position=target->position; facts_.collision=target->collision;
        facts_.target_graph=target->graph; facts_.target_state=target->state;
        facts_.target_behavior=target->behavior; facts_.target_flags=target->flags;
        facts_.target_object_flags=target->object_flags; facts_.target_object_id=target->object_id;
        facts_.process_flags=target->process_flags; facts_.model_animation=target->model_animation;
        facts_.animation_time=target->animation_time;
    }
    const auto& photo=*player->photo;
    facts_.film=photo.gPhotoCount; facts_.qualified=photo.D_800AE27C;
    require(facts_.film>=0 && facts_.film<=60 && facts_.qualified>=0 && facts_.qualified<=facts_.film,
            "River Porygon actual roll counters invalid");
    facts_.taken_context=photo.PokemonDetector_TakenPhotoIndex;
    facts_.player_ready=player->D_80382D04_523114==1 && !player->IsInputDisabled && !player->IsPaused;
    require(entry.game_state().save(), "River Porygon canonical Save unavailable");
    const auto ability=entry.game_state().save()->player_flag(1);
    require(bool(ability), "River Porygon Pester ability unavailable");
    facts_.pester_available=ability.value; facts_.available=ability.value;
    if (!started_ && facts_.player_ready && ability.value) { started_=true; facts_.initial_update=updates; }
    if (observed_roll_<0) { observed_roll_=facts_.film; observed_qualified_=facts_.qualified; }
    require(facts_.film>=observed_roll_ && facts_.qualified>=observed_qualified_, "River Porygon roll counters regressed");
    const bool one=facts_.film==observed_roll_+1 && facts_.qualified==observed_qualified_+1;
    for (int slot=observed_roll_;slot<facts_.film;++slot) {
        const auto& image=photo.D_800B0598[slot];
        facts_.capture_observed=true; facts_.raw_primary=image.pokemons[0].pokemonID;
        if (facts_.raw_primary!=137) continue;
        ++facts_.captures;
        bool coherent=one && consecutive && capture_armed_ && !facts_.exhausted &&
            target && target->identity==armed_subject_ && target->eligible && armed_slot_==slot &&
            armed_qualified_+1==facts_.qualified && facts_.notification_observed && facts_.sender_seen &&
            facts_.reveal_seen && facts_.release_update>=facts_.consumed_update &&
            (player->input.pressed_buttons&shutter) && (player->input.current_buttons&shutter) &&
            image.levelID==3 && (image.unk_00_16&0xe0u)==0x20u &&
            facts_.taken_context>=0 && facts_.taken_context<2;
        if (coherent) {
            const auto& context=photo.PokemonDetector_Photo[facts_.taken_context];
            facts_.taken_actor=key(actors->instance(context.pokemonInFocus));
            coherent=facts_.taken_actor==facts_.target;
            bool copied=false;
            for (std::size_t i=0;i<12 && coherent;++i) {
                if (key(actors->instance(context.pokemonObjects[i]))!=facts_.target) continue;
                copied=private_course::photo_context_copied(image,context,i); break;
            }
            coherent=coherent && copied;
        }
        if (coherent) {
            facts_.complete=true; facts_.captured_slot=slot; facts_.capture_update=updates;
            facts_.captured_slots|=std::uint64_t{1}<<static_cast<unsigned>(slot);
            facts_.phase=RiverPorygonPhase::finished; facts_.disposition=RiverPorygonDisposition::qualified;
        } else ++facts_.capture_mismatches;
    }
    observed_roll_=facts_.film; observed_qualified_=facts_.qualified; capture_armed_=false;
    // Only actual Player consumption acknowledges an owned B request.
    if (pending_press_ && (player->input.current_buttons&pester)) {
        if (player->input.pressed_buttons&pester) ++facts_.pressed;
        if ((player->input.pressed_buttons&pester) && player->ThrowItemTimeout>0 && player->LastItemId==162) {
            ++facts_.consumed; facts_.consumed_update=updates; tracking_item_=true;
            facts_.item={}; previous_item_={}; ground_before_=false;
        } else stop(RiverPorygonDisposition::press_timeout);
        pending_press_=false; pending_release_=true; release_start_=updates;
    }
    if (pending_press_ && updates-facts_.request_update>=120) {
        pending_press_=false; pending_release_=true; release_start_=updates;
        stop(RiverPorygonDisposition::press_timeout);
    }
    if (pending_release_ && !(player->input.current_buttons&pester)) {
        pending_release_=false; ++facts_.releases; facts_.release_update=updates;
    }
    if (pending_release_ && updates-release_start_>=120) {
        pending_release_=false; stop(RiverPorygonDisposition::release_timeout);
    }
    if (!facts_.complete && !facts_.exhausted && started_ && updates-facts_.initial_update>=overall_cap)
        stop(RiverPorygonDisposition::overall_limit);
    if (!facts_.exhausted && facts_.target[0] && !target && !facts_.complete)
        stop(RiverPorygonDisposition::actor_lost);
    if (tracking_item_ && !facts_.exhausted && !facts_.reveal_seen) {
        const Item* item=nullptr; unsigned matches=0;
        for (std::size_t i=0;i<item_count_;++i) {
            const auto& candidate=items_[i];
            bool existed=false;
            for (std::size_t j=0;j<prethrow_count_;++j) existed |= candidate.identity==prethrow_items_[j];
            if (facts_.item[0] ? candidate.identity!=facts_.item : existed || candidate.kind!=162) continue;
            item=&candidate; ++matches;
        }
        facts_.item_present=matches==1;
        if (matches>1) stop(RiverPorygonDisposition::item_ambiguous);
        else if (item) {
            facts_.item=item->identity; facts_.item_kind=item->kind; facts_.item_state=item->state;
            facts_.item_flags=item->flags; facts_.item_object_flags=item->object_flags;
            facts_.item_object_id=item->object_id; facts_.item_position=item->position;
            if (target) facts_.item_actor_distance=std::sqrt(squared(item->position,target->position));
            world::GroundResult ground{}, prior_ground{};
            const bool touched=(item->flags&(ISLE_ITEM_BOUNCED|ISLE_ITEM_TOUCHED_GROUND|ISLE_ITEM_DELETED))==
                (ISLE_ITEM_BOUNCED|ISLE_ITEM_TOUCHED_GROUND);
            const bool fresh=(!ground_before_ || previous_item_!=item->identity) &&
                (!previous_item_[0] || consecutive);
            const bool first_ground=!facts_.first_ground_observed && (item->flags&ISLE_ITEM_TOUCHED_GROUND);
            const bool ground_queried=(first_ground || (fresh && touched)) && finite(item->position) &&
                world->ground_at(item->position.x,item->position.z,ground);
            // Latch even a nonqualifying first ground event; never replace its
            // surface with an unqueried default on following hidden-item updates.
            if (first_ground) {
                facts_.first_ground_observed=true; facts_.first_ground_update=updates;
                facts_.first_ground_queried=ground_queried;
                facts_.first_ground_consecutive=consecutive;
                facts_.ground_surface=ground_queried ? ground.surface : -1;
                facts_.first_ground_position=item->position; facts_.first_ground_flags=item->flags;
                facts_.first_ground_object_flags=item->object_flags;
                if (target) {
                    const auto actor_global=consecutive && previous_target_==target->identity ?
                        previous_global_target_ : global(target->position,origin);
                    facts_.first_ground_actor_distance=std::sqrt(squared(global(item->position,origin),actor_global));
                }
            }
            // Source BounceFromGround replaces prevPos with the penetrated
            // endpoint; exactly-on-ground sets it to the contact. Both satisfy
            // this prior-position check. Pester root remains contact+12/hidden.
            const bool contact=fresh && touched && (item->object_flags&1u) && ground_queried &&
                ground.surface==0xff7fb2 && std::abs(item->position.y-ground.height-12)<=.5f &&
                world->ground_at(item->previous_position.x,item->previous_position.z,prior_ground) &&
                item->previous_position.y<=prior_ground.height+.375f;
            if (contact && target && facts_.hidden_seen) {
                const auto actor_global=consecutive && previous_target_==target->identity ?
                    previous_global_target_ : global(target->position,origin);
                const float distance=squared(global(item->position,origin),actor_global);
                if (distance<1000000) {
                    facts_.notification_observed=true; facts_.notification_update=updates;
                    facts_.notification_position=item->position; facts_.notification_actor_distance=std::sqrt(distance);
                }
            }
            if (target && facts_.notification_observed && target->interaction==item->identity) {
                facts_.sender_seen=true; facts_.interaction_sender=item->identity; facts_.sender_update=updates;
            }
            previous_item_=item->identity; ground_before_=touched;
            ++facts_.flight_updates;
            facts_.phase=facts_.notification_observed ? RiverPorygonPhase::reveal : RiverPorygonPhase::flight;
        } else if (updates>facts_.consumed_update+2) {
            ++facts_.lost_items; stop(RiverPorygonDisposition::item_lost);
        }
        if (target && target->revealing) {
            if (!facts_.hidden_seen || !facts_.notification_observed || !facts_.sender_seen ||
                facts_.interaction_sender!=facts_.item || (target->flags&8u) || (target->object_flags&1u))
                stop(RiverPorygonDisposition::reveal_unattributed);
            else {
                facts_.reveal_seen=true; facts_.reveal_update=updates; tracking_item_=false;
                facts_.phase=RiverPorygonPhase::capture; facts_.disposition=RiverPorygonDisposition::capture_pending;
            }
        }
        if (!facts_.reveal_seen && !facts_.exhausted) {
            if (facts_.notification_observed && updates-facts_.notification_update>=reveal_cap)
                stop(RiverPorygonDisposition::reveal_timeout);
            else if (updates-facts_.consumed_update>=flight_cap) stop(RiverPorygonDisposition::flight_timeout);
        }
    }
    facts_.revealed_eligible=target && facts_.reveal_seen && target->eligible;
    facts_.revealed_focused=target && facts_.reveal_seen && target->focused;
    if (!facts_.exhausted && target && facts_.reveal_seen && target->focused && facts_.player_ready &&
        player->gDirectionIndex==-1 && !player->TargetDirectionZoomedIn && player->sTimerAfterPhotoTaken<0 &&
        !(player->input.current_buttons&shutter) && facts_.film<59) {
        capture_armed_=true; armed_subject_=target->identity;
        armed_slot_=facts_.film; armed_qualified_=facts_.qualified;
    }
    previous_target_=target ? target->identity : PorygonIncarnation{};
    if (target) previous_global_target_=global(target->position,origin);
}

bool RiverPorygonPhotoPuzzle::update(const application::RuntimeView& view, std::uint64_t updates) {
    observe(view,updates);
    if (control_sampled_ && control_update_==updates) return facts_.owns_pad;
    control_sampled_=true; control_update_=updates; facts_.owns_pad=false;
    if (pending_press_ || pending_release_) {
        facts_.phase=pending_press_ ? RiverPorygonPhase::press : RiverPorygonPhase::release;
        publish(pending_press_ && !facts_.exhausted ? pester : 0); return true;
    }
    if (!active_ || facts_.exhausted || facts_.complete || facts_.reveal_seen) {
        if (neutral_pending_) { publish(); return true; }
        return false;
    }
    const auto* host=view.river->native_host();
    const auto* world=host->world_owner();
    const auto* player=view.river->adapter().native.player();
    if (!facts_.pester_available) { facts_.disposition=RiverPorygonDisposition::pester_unavailable; return false; }
    if (!facts_.player_ready) {
        facts_.disposition=RiverPorygonDisposition::player_not_ready;
        if (neutral_pending_) { publish(); return true; }
        return false;
    }
    if (tracking_item_) { facts_.disposition=RiverPorygonDisposition::item_outstanding; publish(); return true; }
    if (facts_.film>=59) {
        facts_.disposition=RiverPorygonDisposition::film_reserved;
        if (neutral_pending_) { publish(); return true; }
        return false;
    }
    bool hidden_present=false;
    for (std::size_t i=0;i<actor_count_;++i)
        hidden_present |= actors_[i].hidden && finite(actors_[i].position) &&
            (!facts_.target[0] || actors_[i].identity==facts_.target);
    // Planning and its reticle length must use the unzoomed throwing domain.
    // Recover before searching terrain, not only after a reachable plan exists.
    if (hidden_present) {
        if (player->gDirectionIndex<0 || player->TargetDirectionZoomedIn || player->TurnToDirSpeed!=0) {
            if (++recovery_>128) { stop(RiverPorygonDisposition::recovery_limit); publish(); return true; }
            ++facts_.recovery_updates; facts_.phase=RiverPorygonPhase::unzoom;
            std::uint16_t buttons=0;
            if (player->ZoomSwitchMode==1 && player->gDirectionIndex==-1 && !player->TargetDirectionZoomedIn &&
                !zoom_out_requested_ && !(player->input.current_buttons&zoom)) {
                buttons=zoom; zoom_out_requested_=true;
            }
            publish(buttons); return true;
        }
        zoom_out_requested_=false;
    }
    facts_.trigger_surface=false;
    const Actor* target=nullptr;
    const auto offset=world->current()->resource->metadata.world_position;
    const IsleGameVec3 origin{100*offset.x,100*offset.y,100*offset.z};
    IsleGameVec3 point{}, aimed{};
    for (std::size_t i=0;i<actor_count_;++i) {
        const auto& candidate=actors_[i];
        if (facts_.target[0] && candidate.identity!=facts_.target) continue;
        if (!candidate.hidden || !finite(candidate.position)) continue;
        SurfacePlan plan{*world,*player,candidate.position,facts_,aimed};
        bool accepted=false;
        if (surface_cached_) {
            point={global_surface_.x-origin.x,global_surface_.y-origin.y,global_surface_.z-origin.z};
            world::GroundResult ground{};
            if (world->ground_at(point.x,point.z,ground) && ground.surface==0xff7fb2 &&
                squared(point,candidate.position)<1000000) {
                point.y=ground.height;
                accepted=SurfacePlan::accept(&plan,point);
            }
            if (!accepted) surface_cached_=false;
        }
        if (!accepted && !river_pester_surface_target(*world,candidate.position,point,
                                                       {&plan,SurfacePlan::accept})) continue;
        target=&candidate; break;
    }
    if (!target) {
        facts_.disposition=actor_count_ ? RiverPorygonDisposition::surface_unavailable : RiverPorygonDisposition::target_absent;
        if (hidden_present || neutral_pending_) { publish(); return true; }
        return false;
    }
    if (!facts_.target[0]) {
        facts_.target=target->identity; facts_.hidden_seen=true; facts_.hidden_update=updates;
        facts_.target_behavior=target->behavior; facts_.target_graph=target->graph; facts_.target_state=target->state;
        facts_.target_position=target->position; facts_.collision=target->collision; facts_.target_present=true;
        previous_target_=target->identity; previous_global_target_=global(target->position,origin);
    }
    global_surface_=global(point,origin); surface_cached_=true;
    facts_.surface_target=point; facts_.trigger_surface=true;
    if (facts_.requests>=3) { stop(RiverPorygonDisposition::request_limit); if (neutral_pending_) { publish(); return true; } return false; }
    for (std::size_t i=0;i<item_count_;++i) if (!(items_[i].flags&ISLE_ITEM_DELETED)) {
        facts_.disposition=RiverPorygonDisposition::item_outstanding;
        if (neutral_pending_) { publish(); return true; }
        return false;
    }
    if (!player->ui || player->ui->Icons_NumItemsAvailable<2) {
        facts_.disposition=RiverPorygonDisposition::item_ui_not_ready;
        if (neutral_pending_) { publish(); return true; }
        return false;
    }
    if ((player->input.current_buttons&shutter) || player->sTimerAfterPhotoTaken>=0 || player->ThrowItemTimeout!=0) {
        if (neutral_pending_) { publish(); return true; }
        return false;
    }
    if (++steering_>360) { stop(RiverPorygonDisposition::steering_limit); publish(); return true; }
    ++facts_.steering_updates; facts_.phase=RiverPorygonPhase::aim;
    facts_.disposition=RiverPorygonDisposition::none; facts_.input_target=aimed;
    std::int8_t x=0,y=0;
    aim_.point(aim_.owner,*player,aimed,false,x,y);
    std::uint16_t buttons=0;
    if (!x && !y && !(player->input.current_buttons&pester)) {
        // Aim's neutral pad only means angular tolerance. Quantized actual
        // cam_target must independently forecast the FIRST qualifying contact.
        if (!forecast(*world,*player,target->position,player->cam_target,facts_.actual_forecast)) {
            ++facts_.actual_forecast_rejections;
            facts_.disposition=RiverPorygonDisposition::projectile_out_of_reach;
            publish(); return true;
        }
        prethrow_count_=item_count_;
        for (std::size_t i=0;i<item_count_;++i) prethrow_items_[i]=items_[i].identity;
        facts_.request_update=updates; ++facts_.requests; pending_press_=true;
        buttons=pester; facts_.phase=RiverPorygonPhase::press;
    }
    publish(buttons,x,y); return true;
}
} // namespace isle::private_campaign

#include "native_cave_pester_photo.hpp"
#include "captured_photo_identity.hpp"
#include "application_runtime.hpp"
#include "application_photos.hpp"
#include "cave_scene.hpp"
#include "game_state.hpp"
#include "actor_owner.hpp"
#include "actor_photo_eligibility.hpp"
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
constexpr std::uint16_t b_button=0x4000, z_button=0x2000, a_button=0x8000;
constexpr std::uint64_t overall_cap=8192, flight_cap=192, evolution_cap=1024;
using cave_pester_detail::Actor;
using cave_pester_detail::Item;
using cave_pester_detail::Snapshot;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
CavePesterIncarnation key(menu::animation::InstanceHandle h) { return {h.pool,h.slot,h.incarnation}; }
float squared(const IsleGameVec3& a, const IsleGameVec3& b) {
    const float x=a.x-b.x,y=a.y-b.y,z=a.z-b.z; return x*x+y*y+z*z;
}
bool finite(const IsleGameVec3& p) { return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z); }
IsleGameVec3 global(const IsleGameVec3& p, const IsleGameVec3& origin) {
    return {p.x+origin.x,p.y+origin.y,p.z+origin.z};
}
bool accepts_hit(const IsleActorSource& source, int species) {
    if (!source.transitionGraph || source.transitionGraph->source_identity!=
        (species==132 ? 0x802c65e8u : 0x802c6fd4u)) return false;
    for (std::size_t i=0;i<source.transitionGraph->count;++i) {
        const auto& row=source.transitionGraph->rows[i];
        if (row.cmd==58) break;
        if (row.cmd==9 && row.state==(species==132 ? func_802BEB24_640FD4 : func_802C0D34_6431E4)) return true;
    }
    return false;
}
void snapshot(Snapshot& out, const IslePlayerSource& player, actor::Owner& actors, items::Owner& items,
              int species, const CavePesterIncarnation& selected, const CavePesterIncarnation& child,
              const IsleGameVec3& origin, const IsleGameVec3& parent_global) {
    out.parent={}; out.child_count=0; out.item_count=0; out.sphere_count=0;
    out.capacity=true; out.captured_subject_context=false; out.other_evolving_near=false; out.taken_species=0;
    IslePlayerPhotoFocus focus{};
    require(isle_player_photo_published_focus(player.photo,&focus)==ISLE_GAME_OK,"Cave actual focus observation rejected");
    out.focus_species=focus.pokemon_id; out.focus_flags=focus.flags; out.focus_context=focus.context;
    out.focus_actor=key(actors.instance(focus.object)); out.taken_actor={};
    const int taken=player.photo->PokemonDetector_TakenPhotoIndex;
    if (taken>=0 && taken<2) {
        const auto& context=player.photo->PokemonDetector_Photo[taken];
        out.taken_actor=key(actors.instance(context.pokemonInFocus));
        if (context.pokemonInFocus) for (std::size_t i=0;i<12;++i)
            if (context.pokemonObjects[i]==context.pokemonInFocus) out.taken_species=context.main.pokemons[i].pokemonID;
    }
    IsleGameObject* previous=nullptr;
    for (;;) {
        IsleGameObject* object=nullptr; IsleActorSource* source=nullptr; IsleGameModel* model=nullptr;
        require(isle_actor_source_link3_next(player.game,previous,&object,&source,&model)==ISLE_GAME_OK,
                "Cave actual Actor traversal rejected");
        if (!object) break;
        previous=object;
        const auto identity=key(actors.instance(object));
        require(identity[0]!=0,"Cave Actor incarnation unavailable");
        if (source->tangible && source->collisionRadius>0) {
            if (out.sphere_count==out.spheres.size()) out.capacity=false;
            else out.spheres[out.sphere_count++]={identity,source->collPosition,global(source->collPosition,origin),{},
                source->collisionRadius,source->id,false};
        }
        if (!model || (source->id!=(species==132 ? 1 : 88) && source->id!=species)) continue;
        const IsleGameModelPlacement* placement=nullptr;
        require(isle_game_model_placement(player.game,model,&placement)==ISLE_GAME_OK && placement,
                "Cave actual Actor placement rejected");
        if (placement->mode==ISLE_GAME_MODEL_PLACEMENT_DISABLED) continue;
        Actor actor;
        actor.present=true; actor.identity=identity; actor.root=placement->transform.position;
        actor.global_root=global(actor.root,origin); actor.collision=source->collPosition;
        actor.yaw=placement->transform.rotation.y;
        actor.radius=source->collisionRadius; actor.flags=source->flags; actor.behavior=source->behavior;
        actor.counter=source->miscVars[0].field1; actor.window=source->miscVars[1].field1;
        actor.graph=source->transitionGraph ? source->transitionGraph->source_identity : 0;
        actor.process_flags=source->processFlags; actor.model_animation=source->modelAnims;
        actor.animation_time=actors.animation().model(actors.instance(object)).animation_time;
        actor.interaction=key(items.instance(source->interactionTarget)); actor.tangible=source->tangible!=0;
        IsleGameSourceCallback callback=nullptr;
        if (source->stateProc)
            require(isle_game_process_source_function(player.game,source->stateProc,&callback)==ISLE_GAME_OK,
                    "Cave actual Actor state identity rejected");
        actor.hit=callback==(species==132 ? func_802BEB24_640FD4 : func_802C0D34_6431E4);
        actor.evolving=species==132 ? actor.hit : callback==func_802C0F30_6433E0;
        actor.state=actor.hit ? (species==132 ? 0x802beb24u : 0x802c0d34u) :
            actor.evolving ? 0x802c0f30u : 0;
        IsleGameObjectMetadata metadata{};
        require(isle_game_object_metadata(player.game,object,&metadata)==ISLE_GAME_OK,"Cave Actor metadata rejected");
        actor.object_flags=metadata.flags; actor.object_id=metadata.id;
        const float pitch=std::atan2(actor.collision.y-player.CameraEyePos.y,
            std::hypot(actor.collision.x-player.CameraEyePos.x,actor.collision.z-player.CameraEyePos.z));
        // Source flags 0x200/0x800 do not establish visibility or capture eligibility.
        actor.eligible=actor.tangible && !(metadata.flags&1u) && (source->flags&4u) && !(source->flags&8u) &&
            source->playerDist>=0 && source->playerDist<=10000 && pitch>=player.MinPitch && pitch<=player.MaxPitch;
        actor.focused=actor.eligible && focus.present && focus.object==object && focus.pokemon_id==source->id &&
            (focus.flags&4u) && !(focus.flags&8u);
        actor.ready=accepts_hit(*source,species) && actor.tangible && !(metadata.flags&1u) &&
            actor.radius>0 && finite(actor.collision) && finite(actor.root);
        actor.distance=squared(actor.collision,player.CameraEyePos);
        if (source->id==species) {
            if (out.child_count==out.children.size()) out.capacity=false;
            else out.children[out.child_count++]=actor;
            if (identity==child && actor.eligible) {
                if (taken>=0 && taken<2) {
                    const auto& context=player.photo->PokemonDetector_Photo[taken];
                    if (context.pokemonInFocus==object) for (std::size_t i=0;i<12;++i) {
                        if (context.pokemonObjects[i]!=object || context.main.pokemons[i].pokemonID!=species) continue;
                        out.taken_species=context.main.pokemons[i].pokemonID;
                        const int count=player.photo->gPhotoCount;
                        if (count>0 && count<=60) out.captured_subject_context=
                            private_course::photo_context_copied(player.photo->D_800B0598[count-1],context,i);
                    }
                }
            }
        } else {
            if (selected[0] && identity!=selected && actor.evolving && squared(actor.global_root,parent_global)<=64)
                out.other_evolving_near=true;
            if (identity==selected || (!selected[0] && actor.ready &&
                (!out.parent.present || actor.distance<out.parent.distance))) out.parent=actor;
        }
    }
    for (const auto& binding:items.render_bindings()) {
        if (!binding.model.object) continue;
        const auto handle=items.instance(binding.model.object);
        const auto family=items.family(handle);
        if (!family || (*family!=items::Family::pester_ball && *family!=items::Family::apple)) continue;
        const auto* source=items.source(binding.model.object);
        require(source && bool(handle),"Cave actual Item source/incarnation unavailable");
        if (out.item_count==out.items.size()) { out.capacity=false; break; }
        const IsleGameModelNode* nodes=nullptr; std::size_t count=0;
        require(isle_game_model_peek_nodes(player.game,binding.model.model,&nodes,&count)==ISLE_GAME_OK && nodes && count,
                "Cave actual Item root observation rejected");
        IsleGameObjectMetadata metadata{};
        require(isle_game_object_metadata(player.game,binding.model.object,&metadata)==ISLE_GAME_OK,
                "Cave actual Item metadata rejected");
        out.items[out.item_count++]={key(handle),nodes[0].transform.position,source->velocity,
            source->flags,metadata.flags,metadata.id,source->itemID,source->state,source->restTimer};
    }
}
void sample_collision_motion(Snapshot& observed, std::array<cave_pester_detail::CollisionSample,256>& previous,
                             std::size_t& previous_count, std::uint64_t& previous_update, std::uint64_t update) {
    const bool consecutive=previous_count && update==previous_update+1;
    const auto end=previous.begin()+previous_count;
    for (std::size_t i=0;i<observed.sphere_count;++i) {
        auto& sphere=observed.spheres[i];
        if (!consecutive || !finite(sphere.center) || !finite(sphere.global_center) ||
            !std::isfinite(sphere.radius)) continue;
        // Link-3 order is usually unchanged; a spawn/removal can reorder it.
        // Match the full pool/slot/incarnation, never a reused slot or index.
        const auto* prior=i<previous_count && previous[i].identity==sphere.identity ? &previous[i] : nullptr;
        if (!prior) {
            const auto found=std::find_if(previous.begin(),end,[&sphere](const auto& candidate) {
                return candidate.identity==sphere.identity;
            });
            if (found!=end) prior=&*found;
        }
        if (!prior || !finite(prior->global_center)) continue;
        sphere.motion={sphere.global_center.x-prior->global_center.x,sphere.global_center.y-prior->global_center.y,
            sphere.global_center.z-prior->global_center.z};
        sphere.motion_valid=finite(sphere.motion);
    }
    // Store global centers: local collPosition is rebased on a block change.
    // A missing sample or newly tangible incarnation must wait for real motion.
    previous_count=observed.sphere_count; previous_update=update;
    for (std::size_t i=0;i<previous_count;++i)
        previous[i]={observed.spheres[i].identity,observed.spheres[i].global_center};
}
// Prospective input gate only. Actual Source hit state and exact Item sender
// below, never this geometric prediction, establish an accepted hit.
float intersection(const IsleGameVec3& start, const IsleGameVec3& delta, const IsleGameVec3& center, float radius) {
    const float x=center.x-start.x,y=center.y-start.y,z=center.z-start.z;
    const float length=std::hypot(delta.x,delta.y,delta.z);
    if (!(length>0) || !(radius>0)) return 2;
    const float projection=(x*delta.x+y*delta.y+z*delta.z)/length;
    const float perpendicular=x*x+y*y+z*z-projection*projection;
    if (!(projection>0) || perpendicular>radius*radius) return 2;
    const float entry=(projection-std::sqrt(std::max(0.0f,radius*radius-perpendicular)))/length;
    return entry<1 ? entry : 2;
}
bool forecast_flight(const world::World& world, const Snapshot& observed, IsleGameVec3 position,
                     IsleGameVec3 velocity, float& time, CavePesterForecast& receipt) {
    using Result=CavePesterForecastDisposition;
    receipt={}; receipt.target=observed.parent.collision;
    const auto reject=[&receipt](Result result) { receipt.disposition=result; return false; };
    if (!finite(position) || !finite(velocity)) return reject(Result::invalid_launch);
    for (std::size_t i=0;i<observed.sphere_count;++i) {
        const auto& sphere=observed.spheres[i];
        if (sphere.motion_valid) continue;
        receipt.blocker=sphere.identity; receipt.blocker_species=sphere.species;
        receipt.blocker_center=sphere.center; receipt.blocker_radius=sphere.radius;
        return reject(Result::actor_motion_unobserved);
    }
    // This is a constant-motion forecast, not an assertion that an Actor's
    // future path/animation/state cannot change. Actual receipts remain proof.
    receipt.constant_motion=true;
    for (unsigned step=0;step<160;++step) {
        receipt.step=step; receipt.position=position;
        world::GroundResult ground{},ceiling{};
        if (!finite(position) || !world.ground_at(position.x,position.z,ground))
            return reject(Result::ground_unavailable);
        receipt.ground_height=ground.height; receipt.ground_surface=ground.surface;
        if (position.y<=ground.height+.375f) return reject(Result::ground_contact);
        if (world.ceiling_at(position.x,position.z,ceiling) && position.y>ceiling.height)
            return reject(Result::ceiling_contact);
        const IsleGameVec3 delta{velocity.x*.5f,velocity.y*.5f,velocity.z*.5f};
        world::Vec3 hit{},reflected{};
        receipt.static_object=world.collide({position.x,position.y,position.z},{delta.x,delta.y,delta.z},hit,reflected);
        if (receipt.static_object>=0) return reject(Result::static_hit);
        float nearest=2; bool selected=false;
        for (std::size_t i=0;i<observed.sphere_count;++i) {
            const auto& sphere=observed.spheres[i];
            // Items priority7 precedes the next priority2 collision refresh.
            // Step0 consumes the observed collPosition; later steps extrapolate
            // its consecutive global delta. Keep link order and strict ties,
            // not a continuous relative-motion sweep.
            const float elapsed=static_cast<float>(step);
            const IsleGameVec3 center{sphere.center.x+sphere.motion.x*elapsed,
                sphere.center.y+sphere.motion.y*elapsed,sphere.center.z+sphere.motion.z*elapsed};
            const bool target=sphere.identity==observed.parent.identity;
            const float entry=intersection(position,delta,center,sphere.radius);
            if (target) receipt.target_entry=entry;
            if (entry<nearest) {
                nearest=entry; selected=target; receipt.blocker=sphere.identity;
                receipt.blocker_center=center; receipt.blocker_radius=sphere.radius; receipt.entry=entry;
                receipt.blocker_motion=sphere.motion; receipt.blocker_species=sphere.species;
            }
        }
        if (nearest<1) {
            time=static_cast<float>(step)+std::max(0.0f,nearest);
            receipt.disposition=selected ? Result::target_hit : Result::competing_actor; return selected;
        }
        velocity.y-=.6f;
        position={position.x+velocity.x*.5f,position.y+velocity.y*.5f,position.z+velocity.z*.5f};
    }
    return reject(Result::flight_limit);
}
bool predict_consume_ray(const IslePlayerSource& player, CavePesterConsumeRay& receipt) {
    receipt={};
    const IsleGameCameraParameters* camera=nullptr;
    require(isle_game_camera_parameters(player.game,player.camera,&camera)==ISLE_GAME_OK && camera,
            "Cave prospective consume camera unavailable");
    receipt.camera=*camera; receipt.reticle_x=player.ReticleScreenX; receipt.reticle_y=player.ReticleScreenY;
    receipt.direction=player.gDirectionIndex; receipt.input_x=player.input.stick_x; receipt.input_y=player.input.stick_y;
    receipt.stick_x_value=player.StickXValue; receipt.stick_y_value=player.StickYValue;
    receipt.stick_x_accum=player.StickXAccum; receipt.stick_y_accum=player.StickYAccum;
    receipt.view_yaw=player.PlayerViewYaw; receipt.view_pitch=player.ViewPitch;
    if (receipt.direction<0) return false;
    // Pinned om.c1826-1856 runs ALL object updates before ANY processes.
    // player.c2285-2292 therefore unprojects the completed camera/reticle at
    // the next OM boundary, before neutral B reaches handleAnalogStick.
    // Later priority-9 processes are analog -> throw -> yaw -> camera
    // (player.c2388-2394); throw uses this ray and the still-completed eye.
    // Do not advance cached Stick* or relax ReticleScreen*: those processes
    // affect the following reticle-object update, not this consumption.
    actor::photo::Camera prepared;
    if (actor::photo::prepare_camera(receipt.camera,prepared)!=ISLE_GAME_OK) return false;
    // player.c2722-2726,2790-2822 uses the adjugate homogeneously; its common
    // determinant cancels. Keep its binary32 cofactor/evaluation convention.
    // The existing pure camera helper supplies owned-GAME math, no Source
    // callback, SDK producer, camera cache publication or second projectile solver.
    float adjugate[4][4]{};
    for (unsigned row=0;row<4;++row) for (unsigned column=0;column<4;++column) {
        float minor[3][3]{};
        unsigned minor_row=0;
        for (unsigned r=0;r<4;++r) {
            if (r==row) continue;
            unsigned minor_column=0;
            for (unsigned c=0;c<4;++c)
                if (c!=column) minor[minor_row][minor_column++]=prepared.view_projection[r][c];
            ++minor_row;
        }
        const float determinant=minor[0][0]*minor[1][1]*minor[2][2]+
            minor[1][0]*minor[2][1]*minor[0][2]+minor[2][0]*minor[0][1]*minor[1][2]-
            minor[0][2]*minor[1][1]*minor[2][0]-minor[1][2]*minor[2][1]*minor[0][0]-
            minor[2][2]*minor[0][1]*minor[1][0];
        adjugate[column][row]=((row+column)&1u ? -1.0f:1.0f)*determinant;
    }
    const float x=static_cast<float>(receipt.reticle_x)/160.0f;
    const float y=-static_cast<float>(receipt.reticle_y)/120.0f;
    constexpr float z=.99f;
    const float w=adjugate[0][3]*x+adjugate[1][3]*y+adjugate[2][3]*z+adjugate[3][3];
    if (!std::isfinite(w) || w==0) return false;
    const float inverse_w=1.0f/w;
    receipt.ray={(adjugate[0][0]*x+adjugate[1][0]*y+adjugate[2][0]*z+adjugate[3][0])*inverse_w,
        (adjugate[0][1]*x+adjugate[1][1]*y+adjugate[2][1]*z+adjugate[3][1])*inverse_w,
        (adjugate[0][2]*x+adjugate[1][2]*y+adjugate[2][2]*z+adjugate[3][2])*inverse_w};
    receipt.valid=finite(receipt.ray);
    return receipt.valid;
}
bool forecast(const world::World& world, const IslePlayerSource& player, const Snapshot& observed,
              const IsleGameVec3& ray, float& time, CavePesterForecast& receipt) {
    const IsleGameCameraParameters* camera=nullptr;
    require(isle_game_camera_parameters(player.game,player.camera,&camera)==ISLE_GAME_OK && camera,
            "Cave actual launch camera unavailable");
    const IsleGameVec3 position{camera->look_at.eye[0],camera->look_at.eye[1],camera->look_at.eye[2]};
    IsleGameVec3 velocity{ray.x-position.x,ray.y-position.y+100,ray.z-position.z};
    const float length=std::hypot(velocity.x,velocity.y,velocity.z);
    if (!(length>0) || !std::isfinite(length)) {
        receipt={}; receipt.aimed=ray; receipt.aimed_valid=true; receipt.target=observed.parent.collision;
        receipt.disposition=CavePesterForecastDisposition::invalid_launch; return false;
    }
    velocity={velocity.x*50/length+player.PlayerVelocity.x,velocity.y*50/length+player.PlayerVelocity.y,
        velocity.z*50/length+player.PlayerVelocity.z};
    const bool clear=forecast_flight(world,observed,position,velocity,time,receipt);
    receipt.aimed=ray; receipt.aimed_valid=true; return clear;
}
std::uint64_t impact_update(std::uint64_t update, float flight) {
    // Next physical input/movement boundary plus command-9 receiver dispatch.
    // This is a scheduling estimate only; acceptance always needs real sender evidence.
    return update+static_cast<std::uint64_t>(std::ceil(flight))+1;
}
bool qualify_reaction(const actor::Resources& resources, CavePesterFacts& facts) {
    // Pinned 642CC0.c149-169 selects this admitted header. Its FIRST animated
    // node receives the loop callback (sys/anim.c132-153,217-553), not unknown_04.
    const actor::ActorAnimationHeader* header=nullptr;
    for (const auto& candidate:resources.headers())
        if (candidate.wire.guest_address==0x802c6f88u) { header=&candidate; break; }
    if (!header || header->speed!=0x1.99999ap-1f) return false;
    std::size_t binding=menu::no_index;
    for (const auto candidate:header->model_bindings)
        if (candidate!=menu::no_index) { binding=candidate; break; }
    const auto animations=resources.animations();
    if (binding>=animations.size()) return false;
    const auto& receiver=animations[binding];
    if (receiver.kind!=menu::AnimationKind::model || receiver.commands.size()!=3) return false;
    const auto& wait=receiver.commands[0];
    const auto& callback=receiver.commands[1];
    const auto& loop=receiver.commands[2];
    if (wait.opcode!=2 || wait.duration!=3 || callback.opcode!=16 || callback.duration!=33 ||
        loop.opcode!=14 || loop.wire.u32(4)!=wait.wire.guest_address) return false;
    // Reproduce only the two binary32 clock countdowns, never playback/callbacks.
    // SET_ANIMATION emits -2 and Pokemon_Update then marks animation-ended.
    float left=static_cast<float>(wait.duration);
    bool second=false;
    const unsigned bound=static_cast<unsigned>(std::ceil(
        static_cast<double>(wait.duration+callback.duration)/header->speed))+2;
    for (unsigned advances=1;advances<=bound;++advances) {
        left-=header->speed;
        if (left>0) continue;
        if (!second) { left+=static_cast<float>(callback.duration); second=true; continue; }
        facts.reaction_header=header->wire.guest_address;
        facts.reaction_receiver=wait.wire.guest_address; facts.reaction_speed=header->speed;
        facts.reaction_wait=wait.duration; facts.reaction_callback_wait=callback.duration;
        facts.reaction_advances=advances; return true;
    }
    return false;
}
bool plan(const world::World& world, const IslePlayerSource& player, const Snapshot& observed,
          CavePesterFacts& facts, IsleGameVec3& aimed) {
    const IsleGameCameraParameters* camera=nullptr;
    require(isle_game_camera_parameters(player.game,player.camera,&camera)==ISLE_GAME_OK && camera,
            "Cave planning camera unavailable");
    facts.planning_eye={camera->look_at.eye[0],camera->look_at.eye[1],camera->look_at.eye[2]};
    facts.planning_velocity=player.PlayerVelocity; facts.planning_ray=player.cam_target;
    facts.planning_motion=facts.motion; facts.planning_radius=observed.parent.radius;
    facts.planning_update=facts.last_update; facts.plan_count=0;
    auto target=observed.parent.collision;
    for (unsigned refinement=0;refinement<facts.plan_forecasts.size();++refinement) {
        auto& receipt=facts.plan_forecasts[refinement];
        ++facts.plan_count;
        if (!projectile_target(player,target,aimed)) {
            receipt={}; receipt.target=target;
            receipt.disposition=CavePesterForecastDisposition::inverse_rejected; return false;
        }
        float time=0;
        const bool accepted=forecast(world,player,observed,aimed,time,receipt);
        receipt.target=target;
        if (accepted) { facts.predicted_flight=time; return true; }
        // Refine from horizontal launch time. The real ray is independently
        // checked again immediately before B, including every other Actor.
        const float horizontal=std::hypot(target.x-player.CameraEyePos.x,target.z-player.CameraEyePos.z);
        const float elapsed=horizontal/20.0f+1;
        target={observed.parent.collision.x+facts.motion.x*elapsed,observed.parent.collision.y+facts.motion.y*elapsed,
            observed.parent.collision.z+facts.motion.z*elapsed};
    }
    return false;
}
} // namespace
int choose_cave_pester_photo_species(application::Photos& photos) {
    const auto ditto=photos.report_present(132);
    require(bool(ditto),"Cave canonical Ditto Report unavailable");
    if (!ditto.value) return 132;
    const auto victreebel=photos.report_present(71);
    require(bool(victreebel),"Cave canonical Victreebel Report unavailable");
    if (!victreebel.value) return 71;
    const auto muk=photos.report_present(89);
    require(bool(muk),"Cave canonical Muk Report unavailable");
    return muk.value ? 0 : 89;
}
CavePesterPhotoPuzzle::CavePesterPhotoPuzzle(private_post_course::Controller controller, Aim aim, int species)
    : controller_(controller),aim_(aim) {
    require(controller_.set && aim_.point,"Cave Pester photos require real pad and pure aim bridges");
    require(species==132 || species==89,"Cave Pester photo objective invalid");
    facts_.required_species=species;
}
void CavePesterPhotoPuzzle::publish(std::uint16_t buttons, std::int8_t x, std::int8_t y) noexcept {
    controller_.set(controller_.owner,buttons,x,y); facts_.owns_pad=true;
}
void CavePesterPhotoPuzzle::stop(CavePesterDisposition disposition) noexcept {
    if (facts_.exhausted) return;
    facts_.exhausted=true; facts_.disposition=disposition; facts_.phase=CavePesterPhase::finished;
    pending_press_=false; tracking_=false; capture_armed_=false; neutral_due_=true;
}
void CavePesterPhotoPuzzle::observe(const application::RuntimeView& view, std::uint64_t updates) {
    if (sampled_ && updates==previous_update_) return;
    require(!sampled_ || updates>previous_update_,"Cave completed-update ordinal regressed");
    const bool consecutive=sampled_ && updates==previous_update_+1;
    sampled_=true; previous_update_=updates; facts_.last_update=updates; facts_.owns_pad=false; observation_active_=false;
    if (view.cave) {
        facts_.source_status=view.cave->adapter().source_status();
        facts_.host_status=view.cave->error();
        if (const auto* host=view.cave->native_host()) {
            if (host->error()!=ISLE_GAME_OK) facts_.host_status=host->error();
        }
        if (facts_.source_status!=ISLE_GAME_OK || facts_.host_status!=ISLE_GAME_OK) {
            stop(CavePesterDisposition::source_error);
            const auto* host=view.cave->native_host();
            const char* message=host && host->error()!=ISLE_GAME_OK ? host->message() : view.cave->message();
            throw std::runtime_error(message && *message ? message :
                "Cave actual Source failed during Pester hit/child factory continuation");
        }
    }
    if (!view.cave || view.cave->status()!=course::LifecycleStatus::running) {
        if (started_ && !facts_.complete) stop(CavePesterDisposition::course_ended);
        return;
    }
    const auto& entry=*view.cave;
    const auto* host=entry.native_host(); const auto* player=entry.adapter().native.player();
    if (!host || !host->execution_open() || !player) return;
    require(player->status==ISLE_GAME_OK && player->photo && player->photo->status==ISLE_GAME_OK,
            "Cave actual Player/Photo failed");
    auto* actors=host->actors(); auto* items=host->items_owner(); auto* world=host->world_owner();
    require(actors && items && world,"Cave actual Actor/Item/World unavailable");
    require(world->error().empty(),"Cave actual World failed");
    const auto* block=world->current();
    if (!block || !block->resource || world->retired()) {
        if (started_ && !facts_.complete) stop(CavePesterDisposition::course_ended);
        return;
    }
    const auto offset=block->resource->metadata.world_position;
    const IsleGameVec3 origin{offset.x*100,offset.y*100,offset.z*100};
    snapshot(snapshot_,*player,*actors,*items,facts_.required_species,facts_.parent,facts_.child,origin,previous_global_root_);
    sample_collision_motion(snapshot_,previous_spheres_,previous_sphere_count_,previous_sphere_update_,updates);
    observation_active_=true;
    const auto& observed=snapshot_; const auto& parent=observed.parent;
    const auto& photo=*player->photo;
    facts_.current_block=static_cast<int>(block->index);
    facts_.film=photo.gPhotoCount; facts_.qualified=photo.D_800AE27C;
    require(facts_.film>=0 && facts_.film<=60 && facts_.qualified>=0 && facts_.qualified<=facts_.film,
            "Cave actual roll counters invalid");
    facts_.focus_species=observed.focus_species; facts_.focus_context=observed.focus_context;
    facts_.focus_flags=observed.focus_flags; facts_.taken_context=photo.PokemonDetector_TakenPhotoIndex;
    facts_.taken_species=observed.taken_species;
    facts_.focus_actor=observed.focus_actor; facts_.taken_actor=observed.taken_actor;
    facts_.player_ready=player->D_80382D04_523114==1 && !player->IsInputDisabled && !player->IsPaused;
    require(entry.game_state().save(),"Cave canonical Save unavailable");
    const auto pester=entry.game_state().save()->player_flag(1);
    require(bool(pester),"Cave actual Pester reward unavailable");
    facts_.available=facts_.pester_available=pester.value;
    if (!started_ && facts_.player_ready && facts_.available) { started_=true; facts_.initial_update=updates; }
    if (observed_roll_<0) { observed_roll_=facts_.film; observed_qualified_=facts_.qualified; }
    require(facts_.film>=observed_roll_ && facts_.qualified>=observed_qualified_,"Cave actual roll counters regressed");
    const bool one_exposure=facts_.film==observed_roll_+1 && facts_.qualified==observed_qualified_+1;
    for (int slot=observed_roll_;slot<facts_.film;++slot) {
        const auto& captured=photo.D_800B0598[slot];
        facts_.capture_observed=true; facts_.raw_primary=captured.pokemons[0].pokemonID;
        if (facts_.raw_primary!=facts_.required_species) continue;
        ++facts_.captures;
        const bool coherent=!facts_.exhausted && capture_armed_ && consecutive && one_exposure &&
            armed_subject_==facts_.child && armed_slot_==slot && armed_qualified_+1==facts_.qualified &&
            facts_.spawn_seen && facts_.parent_cleanup_seen && facts_.release_update>=facts_.consumed_update &&
            (player->input.pressed_buttons&a_button) && (player->input.current_buttons&a_button) &&
            facts_.taken_context>=0 && facts_.taken_context<2 && observed.captured_subject_context &&
            captured.levelID==4 && (captured.unk_00_16&0xe0u)==0x20u;
        if (coherent) {
            facts_.complete=true; facts_.captured_slot=slot; facts_.capture_update=updates;
            facts_.captured_actor=observed.taken_actor;
            facts_.captured_slots|=std::uint64_t{1}<<static_cast<unsigned>(slot);
            facts_.phase=CavePesterPhase::finished; facts_.disposition=CavePesterDisposition::qualified;
        } else { ++facts_.capture_mismatches; ++facts_.mismatches; }
    }
    observed_roll_=facts_.film; observed_qualified_=facts_.qualified; capture_armed_=false;
    facts_.target_present=parent.present; facts_.target_ready=parent.ready; facts_.motion={};
    if (parent.present) {
        facts_.root=parent.root; facts_.global_root=parent.global_root; facts_.collision=parent.collision;
        facts_.collision_radius=parent.radius; facts_.target_graph=parent.graph; facts_.target_state=parent.state;
        facts_.target_flags=parent.flags; facts_.target_object_flags=parent.object_flags; facts_.target_object_id=parent.object_id;
        facts_.target_behavior=parent.behavior; facts_.process_flags=parent.process_flags;
        facts_.model_animation=parent.model_animation; facts_.animation_time=parent.animation_time;
        facts_.hit_counter=parent.counter; facts_.hit_window=parent.window;
        for (std::size_t i=0;i<observed.sphere_count;++i) {
            const auto& sphere=observed.spheres[i];
            if (sphere.identity!=parent.identity) continue;
            if (sphere.motion_valid) facts_.motion=sphere.motion;
            break;
        }
    }
    if (pending_press_ && (player->input.current_buttons&b_button)) {
        const bool pressed=(player->input.pressed_buttons&b_button)!=0;
        if (pressed) ++facts_.pressed;
        if (pressed && player->ThrowItemTimeout>0 && player->LastItemId==ISLE_ITEM_PESTER_BALL &&
            player->gDirectionIndex>=0 && !player->TargetDirectionZoomedIn && player->TurnToDirSpeed==0) {
            ++facts_.consumed; facts_.consumed_update=updates;
            if (facts_.required_species==89) {
                if (pending_throw_>=facts_.throw_count) stop(CavePesterDisposition::hit_unattributed);
                else {
                    auto& receipt=facts_.throws[pending_throw_];
                    receipt.consumed_update=updates; receipt.consumed_buttons=player->input.current_buttons;
                    receipt.pressed_buttons=player->input.pressed_buttons; receipt.consumed_timeout=player->ThrowItemTimeout;
                    receipt.consumed_ray=player->cam_target; receipt.consumed_player_velocity=player->PlayerVelocity;
                    const IsleGameCameraParameters* camera=nullptr;
                    require(isle_game_camera_parameters(player->game,player->camera,&camera)==ISLE_GAME_OK && camera,
                            "Cave consumed Pester camera unavailable");
                    receipt.consumed_eye={camera->look_at.eye[0],camera->look_at.eye[1],camera->look_at.eye[2]};
                }
            } else {
                tracking_=true; accepted_attempt_=false; attempt_start_=updates;
            }
            facts_.item={}; facts_.item_present=false;
        } else { ++facts_.mismatches; stop(CavePesterDisposition::press_timeout); }
        pending_press_=false; pending_release_=true; release_start_=updates;
    }
    if (pending_press_ && updates-facts_.request_update>=8) {
        pending_press_=false; pending_release_=true; release_start_=updates; stop(CavePesterDisposition::press_timeout);
    }
    if (pending_release_ && !(player->input.current_buttons&b_button)) {
        pending_release_=false; ++facts_.releases; facts_.release_update=updates;
        if (facts_.required_species==89 && pending_throw_<facts_.throw_count)
            facts_.throws[pending_throw_].release_update=updates;
    }
    if (pending_release_ && updates-release_start_>=8) { pending_release_=false; stop(CavePesterDisposition::release_timeout); }
    if (!observed.capacity && !facts_.complete) stop(CavePesterDisposition::capacity);
    if (started_ && updates-facts_.initial_update>=overall_cap && !facts_.complete) stop(CavePesterDisposition::overall_limit);
    if (facts_.required_species==89) {
        facts_.throw_cooldown=player->ThrowItemTimeout; facts_.live_pester_items=0;
        for (std::size_t i=0;i<observed.item_count;++i)
            if (observed.items[i].kind==ISLE_ITEM_PESTER_BALL) ++facts_.live_pester_items;
        for (std::size_t index=0;index<facts_.throw_count;++index) {
            auto& flight=facts_.throws[index];
            if (!flight.consumed_update || flight.retired) continue;
            const Item* item=nullptr; unsigned matches=0;
            for (std::size_t i=0;i<observed.item_count;++i) {
                const auto& candidate=observed.items[i];
                if (candidate.kind!=ISLE_ITEM_PESTER_BALL) continue;
                bool excluded=false;
                if (flight.created) excluded=candidate.identity!=flight.item;
                else {
                    for (std::size_t j=0;j<flight.prethrow_count;++j)
                        excluded |= flight.prethrow_items[j]==candidate.identity;
                    for (std::size_t j=0;j<facts_.throw_count;++j)
                        if (j!=index) excluded |= facts_.throws[j].item==candidate.identity;
                }
                if (!excluded) { item=&candidate; ++matches; }
            }
            flight.present=matches==1; flight.observed_update=updates;
            if (index+1==facts_.throw_count) facts_.item_present=flight.present;
            if (matches>1 && !facts_.exhausted) stop(CavePesterDisposition::hit_unattributed);
            if (matches==1) {
                if (!flight.created) {
                    flight.created=true; flight.created_update=updates; flight.item=item->identity;
                    flight.created_position=item->position; flight.created_velocity=item->velocity;
                }
                flight.position=item->position; flight.velocity=item->velocity; flight.kind=item->kind;
                flight.state=item->state; flight.flags=item->flags; flight.object_flags=item->object_flags;
                flight.object_id=item->object_id; flight.rest_timer=item->rest_timer;
                flight.hidden=(item->object_flags&1u)!=0;
                if (index+1==facts_.throw_count) {
                    facts_.item=item->identity; facts_.item_present=true; facts_.item_position=item->position;
                    facts_.item_kind=item->kind; facts_.item_state=item->state; facts_.item_flags=item->flags;
                    facts_.item_object_flags=item->object_flags; facts_.item_object_id=item->object_id;
                    facts_.item_hidden=flight.hidden;
                    facts_.interaction_matches=parent.present && parent.interaction==item->identity;
                    facts_.item_distance=parent.present ? std::sqrt(squared(item->position,parent.collision)) : 0;
                }
                if (!flight.accepted && parent.present && !facts_.exhausted) {
                    float remaining=0;
                    flight.forecast_update=updates; flight.predicted_impact=0;
                    if (!(item->flags&(ISLE_ITEM_TOUCHED_GROUND|ISLE_ITEM_DELETED)) && item->rest_timer<=0 &&
                        forecast_flight(*world,observed,item->position,item->velocity,remaining,flight.remaining_forecast))
                        flight.predicted_impact=impact_update(updates,remaining);
                }
            } else if (flight.created) {
                flight.retired=true; flight.retired_update=updates;
                if (!flight.accepted && !facts_.exhausted) stop(CavePesterDisposition::hit_unattributed);
            }
            if (!flight.accepted && !facts_.exhausted) {
                ++facts_.flight_updates;
                if (updates-flight.consumed_update>=flight_cap) stop(CavePesterDisposition::flight_timeout);
            }
        }
        if (!facts_.exhausted && parent.present && parent.identity==facts_.parent && parent.hit && !previous_hit_) {
            std::size_t sender=facts_.throws.size(); unsigned matches=0;
            for (std::size_t i=0;i<facts_.throw_count;++i) {
                const auto& flight=facts_.throws[i];
                if (flight.created && flight.present && !flight.accepted && flight.item==parent.interaction) {
                    sender=i; ++matches;
                }
            }
            const bool attributed=consecutive && previous_parent_==parent.identity && matches==1 &&
                !waiting_counter_ && sender==facts_.accepted_hits;
            if (!attributed) stop(CavePesterDisposition::hit_unattributed);
            else {
                auto& flight=facts_.throws[sender];
                if (!flight.hidden || (flight.flags&(ISLE_ITEM_TOUCHED_GROUND|ISLE_ITEM_DELETED)) ||
                    !flight.consumed_update || flight.consumed_update<flight.request_update ||
                    flight.release_update<flight.consumed_update) stop(CavePesterDisposition::hit_unattributed);
                else {
                    flight.accepted=true; flight.hit_update=updates; flight.sender=parent.interaction;
                    flight.receiver=parent.identity; flight.receiver_graph=parent.graph; flight.receiver_state=parent.state;
                    flight.hit_process_flags=parent.process_flags; flight.hit_model_animation=parent.model_animation;
                    flight.hit_animation_time=parent.animation_time; flight.counter_before=parent.counter;
                    facts_.accepted_item=flight.item; ++facts_.accepted_hits;
                    facts_.hit_seen=true; facts_.hit_update=updates; accepted_attempt_=true;
                    waiting_counter_=true; counter_before_=parent.counter; counter_throw_=sender;
                }
            }
        }
    }
    if (!facts_.exhausted && !facts_.complete && facts_.required_species==132 && tracking_) {
        const Item* item=nullptr; unsigned matches=0;
        for (std::size_t i=0;i<observed.item_count;++i) {
            const auto& candidate=observed.items[i];
            bool existed=false;
            for (std::size_t j=0;j<prethrow_count_;++j) existed |= prethrow_items_[j]==candidate.identity;
            if (candidate.kind!=ISLE_ITEM_PESTER_BALL || (facts_.item[0] ? candidate.identity!=facts_.item : existed)) continue;
            item=&candidate; ++matches;
        }
        facts_.item_present=matches==1;
        if (matches>1) stop(CavePesterDisposition::hit_unattributed);
        if (matches==1) {
            facts_.item=item->identity; facts_.item_position=item->position; facts_.item_kind=item->kind; facts_.item_state=item->state;
            facts_.item_flags=item->flags; facts_.item_object_flags=item->object_flags; facts_.item_object_id=item->object_id;
            facts_.item_hidden=(item->object_flags&1u)!=0;
            facts_.interaction_matches=parent.present && parent.interaction==item->identity;
            facts_.item_distance=parent.present ? std::sqrt(squared(item->position,parent.collision)) : 0;
            if (parent.present && parent.identity==facts_.parent && parent.hit && !accepted_attempt_) {
                // A new admitted hit callback plus the actual Source sender is
                // acceptance evidence. A hidden ball, forecast or graph alone is not.
                const bool accepted=consecutive && previous_parent_==parent.identity &&
                    !previous_hit_ && facts_.interaction_matches && facts_.item_hidden &&
                    !(item->flags&ISLE_ITEM_TOUCHED_GROUND) && !(item->flags&ISLE_ITEM_DELETED) &&
                    facts_.consumed_update>=facts_.request_update && facts_.consumed>facts_.accepted_hits;
                if (!accepted) stop(CavePesterDisposition::hit_unattributed);
                else {
                    accepted_attempt_=true; tracking_=false; facts_.hit_seen=true; facts_.hit_update=updates;
                    facts_.accepted_item=item->identity; ++facts_.accepted_hits;
                    facts_.evolution_seen=true; facts_.evolution_update=updates;
                }
            }
        }
        if (tracking_) {
            ++facts_.flight_updates; facts_.phase=CavePesterPhase::flight;
            if (updates-attempt_start_>=flight_cap) { tracking_=false; stop(CavePesterDisposition::flight_timeout); }
        }
    }
    // The hit body itself may reset an expired sequence before its animation.
    // Follow that actual scalar while waiting; only the later +1 is a hit count.
    if (!facts_.exhausted && waiting_counter_ && parent.present && parent.hit) {
        counter_before_=parent.counter;
        if (facts_.required_species==89 && counter_throw_<facts_.throw_count)
            facts_.throws[counter_throw_].counter_before=parent.counter;
        if (updates-facts_.hit_update>=512) stop(CavePesterDisposition::evolution_timeout);
    }
    if (!facts_.exhausted && waiting_counter_ && parent.present && !parent.hit) {
        if (!consecutive || previous_parent_!=parent.identity || !previous_hit_ ||
            parent.counter!=counter_before_+1 || parent.window<=0 || parent.window>180)
            stop(CavePesterDisposition::counter_unattributed);
        else if (facts_.required_species==89 && (counter_throw_>=facts_.throw_count ||
                 parent.interaction!=facts_.throws[counter_throw_].item))
            stop(CavePesterDisposition::counter_unattributed);
        else {
            waiting_counter_=false; ++facts_.counter_increments;
            if (facts_.required_species==89) {
                auto& flight=facts_.throws[counter_throw_];
                flight.counter_incremented=true; flight.counter_update=updates;
                flight.counter_after=parent.counter; flight.counter_window=parent.window;
                flight.counter_graph=parent.graph; flight.counter_state=parent.state;
                // Expiration/reset is Source behavior, but cannot qualify this three-Item sequence.
                if (parent.counter!=static_cast<int>(facts_.counter_increments))
                    stop(CavePesterDisposition::counter_unattributed);
            }
        }
    }
    if (!facts_.exhausted && facts_.parent[0] && parent.present && parent.evolving && !facts_.evolution_seen) {
        if (facts_.required_species!=89 || facts_.counter_increments<3 || parent.counter<3 ||
            facts_.accepted_hits<3 || !accepted_attempt_) stop(CavePesterDisposition::counter_unattributed);
        else { facts_.evolution_seen=true; facts_.evolution_update=updates; }
    }
    // Ditto is added while its impostor's final animation still runs; Muk is
    // added at the end of evolution. Observe the new incarnation at either
    // genuine boundary, then independently wait for actual parent cleanup.
    if (!facts_.exhausted && facts_.evolution_seen && !facts_.spawn_seen) {
        const Actor* child=nullptr; unsigned matches=0;
        for (std::size_t i=0;i<observed.child_count;++i) {
            const auto& candidate=observed.children[i]; bool existed=false;
            for (std::size_t j=0;j<previous_child_count_;++j) existed |= previous_children_[j]==candidate.identity;
            if (existed) continue;
            const auto root=parent.present ? parent.global_root : previous_global_root_;
            const float yaw=parent.present ? parent.yaw : previous_yaw_;
            const float separation=squared(candidate.global_root,root);
            const float error=std::remainder(candidate.yaw-yaw,6.2831853071795864769f);
            if (separation<=16 && std::isfinite(error) && std::abs(error)<=.01f) {
                child=&candidate; ++matches; facts_.spawn_distance_squared=separation; facts_.spawn_yaw_error=error;
            }
        }
        if (matches) {
            if (!consecutive || matches!=1 || observed.other_evolving_near || previous_other_evolving_ ||
                !(parent.evolving || previous_evolving_)) stop(CavePesterDisposition::spawn_unattributed);
            else { facts_.child=child->identity; facts_.child_object_id=child->object_id;
                facts_.spawn_seen=true; facts_.spawn_update=updates; }
        }
    }
    if (!facts_.exhausted && facts_.parent[0] && !parent.present && !facts_.parent_cleanup_seen) {
        if (consecutive && previous_parent_present_ && previous_parent_==facts_.parent && previous_evolving_ &&
            facts_.evolution_seen && facts_.spawn_seen) {
            facts_.parent_cleanup_seen=true; facts_.cleanup_update=updates;
        } else stop(CavePesterDisposition::actor_lost);
    }
    facts_.child_present=false; facts_.child_eligible=false; facts_.child_focused=false;
    for (std::size_t i=0;i<observed.child_count;++i) {
        const auto& child=observed.children[i];
        if (child.identity!=facts_.child) continue;
        facts_.child_present=true; facts_.child_eligible=child.eligible; facts_.child_focused=child.focused;
        facts_.child_root=child.root; facts_.child_flags=child.flags;
        if (!facts_.exhausted && facts_.spawn_seen && facts_.parent_cleanup_seen && child.focused && facts_.player_ready &&
            player->gDirectionIndex==-1 && !player->TargetDirectionZoomedIn && player->sTimerAfterPhotoTaken<0 &&
            !(player->input.current_buttons&a_button) && facts_.film<60) {
            capture_armed_=true; armed_subject_=child.identity; armed_slot_=facts_.film; armed_qualified_=facts_.qualified;
        }
    }
    if (!facts_.complete && !facts_.exhausted) {
        if (facts_.spawn_seen && facts_.parent_cleanup_seen) {
            facts_.phase=CavePesterPhase::capture; facts_.disposition=CavePesterDisposition::capture_pending;
            if (!facts_.child_present) stop(CavePesterDisposition::actor_lost);
        } else if (facts_.evolution_seen) {
            facts_.phase=CavePesterPhase::evolution;
            if (updates-facts_.evolution_update>=evolution_cap) stop(CavePesterDisposition::evolution_timeout);
        } else if (waiting_counter_) facts_.phase=CavePesterPhase::hit_animation;
    }
    previous_child_count_=observed.child_count;
    for (std::size_t i=0;i<observed.child_count;++i) previous_children_[i]=observed.children[i].identity;
    previous_parent_present_=parent.present; previous_parent_=parent.identity;
    previous_evolving_=parent.present && parent.evolving; previous_hit_=parent.present && parent.hit;
    previous_other_evolving_=observed.other_evolving_near;
    if (parent.present) {
        previous_global_root_=parent.global_root; previous_yaw_=parent.yaw;
    }
}
bool CavePesterPhotoPuzzle::update(const application::RuntimeView& view, std::uint64_t updates) {
    observe(view,updates);
    if (control_sampled_ && control_update_==updates) return facts_.owns_pad;
    control_sampled_=true; control_update_=updates; facts_.owns_pad=false;
    if (neutral_due_) { neutral_due_=false; publish(); return true; }
    if (!observation_active_ || !view.cave) return false;
    const auto* player=view.cave->adapter().native.player(); const auto* host=view.cave->native_host();
    const auto* world=host ? host->world_owner() : nullptr;
    if (!player || !world || world->retired()) return false;
    if (pending_press_ || pending_release_) {
        facts_.phase=pending_press_ ? CavePesterPhase::press : CavePesterPhase::release;
        publish(pending_press_ && !facts_.exhausted ? b_button : 0); return true;
    }
    if (facts_.complete || facts_.exhausted) return false;
    if (!facts_.available) { facts_.disposition=CavePesterDisposition::pester_unavailable; return false; }
    if (!facts_.player_ready) { facts_.disposition=CavePesterDisposition::player_not_ready; return false; }
    if (facts_.spawn_seen && facts_.parent_cleanup_seen) return false;
    const bool muk=facts_.required_species==89;
    if (muk && facts_.throw_count==facts_.throws.size()) {
        facts_.launch_gate=CavePesterLaunchGate::throws_committed;
        if (!waiting_counter_ && !facts_.evolution_seen) facts_.phase=CavePesterPhase::flight;
        publish(); return true;
    }
    if ((!muk && (tracking_ || waiting_counter_)) || facts_.evolution_seen) {
        facts_.disposition=tracking_ ? CavePesterDisposition::item_outstanding : CavePesterDisposition::none;
        publish(); return true;
    }
    if (facts_.film>=59) { facts_.disposition=CavePesterDisposition::film_reserved; return false; }
    const auto& parent=snapshot_.parent;
    if (!parent.present) { facts_.disposition=CavePesterDisposition::target_absent; return false; }
    if (!parent.ready && !(muk && parent.hit && waiting_counter_ && parent.identity==facts_.parent)) {
        facts_.disposition=CavePesterDisposition::target_not_ready; return false;
    }
    if (facts_.requests>=9) { stop(CavePesterDisposition::request_limit); publish(); neutral_due_=false; return true; }
    if (muk && !facts_.reaction_advances && !qualify_reaction(host->actor_resources(),facts_)) {
        stop(CavePesterDisposition::source_error); publish(); neutral_due_=false; return true;
    }
    if (!player->ui || player->ui->Icons_NumItemsAvailable<2) {
        facts_.disposition=CavePesterDisposition::item_ui_not_ready; return false;
    }
    if ((player->input.current_buttons&a_button) || player->sTimerAfterPhotoTaken>=0) return false;
    if (!muk && player->ThrowItemTimeout!=0) {
        facts_.disposition=CavePesterDisposition::item_outstanding;
        if (facts_.parent[0]) { publish(); return true; }
        return false;
    }
    if (!facts_.parent[0]) {
        IsleGameVec3 aimed{};
        if (!plan(*world,*player,snapshot_,facts_,aimed)) {
            ++facts_.arc_rejections; facts_.disposition=CavePesterDisposition::unsafe_geometry; return false;
        }
        facts_.parent=parent.identity;
    }
    if (player->gDirectionIndex<0 || player->TargetDirectionZoomedIn || player->TurnToDirSpeed!=0) {
        if (++recovery_>128) { stop(CavePesterDisposition::recovery_limit); publish(); neutral_due_=false; return true; }
        ++facts_.recovery_updates; facts_.phase=CavePesterPhase::unzoom;
        std::uint16_t buttons=0;
        if (player->ZoomSwitchMode==1 && player->gDirectionIndex==-1 && !player->TargetDirectionZoomedIn &&
            !zoom_out_requested_ && !(player->input.current_buttons&z_button)) {
            buttons=z_button; zoom_out_requested_=true;
        }
        publish(buttons); return true;
    }
    zoom_out_requested_=false;
    if (++steering_>512) { stop(CavePesterDisposition::steering_limit); publish(); neutral_due_=false; return true; }
    ++facts_.steering_updates;
    IsleGameVec3 aimed{};
    const bool ideal_clear=plan(*world,*player,snapshot_,facts_,aimed);
    if (!ideal_clear) {
        ++facts_.arc_rejections;
        if (!muk) { facts_.disposition=CavePesterDisposition::unsafe_geometry; publish(); return true; }
    }
    std::int8_t x=0,y=0;
    if (ideal_clear) aim_.point(aim_.owner,*player,aimed,false,x,y);
    facts_.input_target=aimed; facts_.phase=CavePesterPhase::aim;
    facts_.disposition=ideal_clear ? CavePesterDisposition::none : CavePesterDisposition::unsafe_geometry;
    std::uint16_t buttons=0;
    float flight=0;
    facts_.launch_update=updates; facts_.launch_aim_x=x; facts_.launch_aim_y=y;
    facts_.launch_reticle_x=static_cast<float>(player->ReticleScreenX);
    facts_.launch_reticle_y=static_cast<float>(player->ReticleScreenY);
    facts_.blocking_item={};
    // cam_target was published in this OM's object pass, before its camera
    // and reticle processes. Muk must qualify NEXT OM's consumed ray instead.
    IsleGameVec3 consumed_ray=player->cam_target;
    bool consume_ray_valid=true;
    if (muk) {
        consume_ray_valid=predict_consume_ray(*player,facts_.consume_prediction);
        consumed_ray=facts_.consume_prediction.ray;
        facts_.actual_forecast={};
        facts_.actual_forecast.target=parent.collision;
        facts_.actual_forecast.disposition=CavePesterForecastDisposition::invalid_launch;
    }
    const bool actual_clear=consume_ray_valid &&
        forecast(*world,*player,snapshot_,consumed_ray,flight,facts_.actual_forecast);
    if (muk) {
        facts_.predicted_impact=actual_clear ? impact_update(updates,flight) : 0;
        facts_.admission_after=0; facts_.admission_before=0;
        bool eligible=actual_clear;
        facts_.launch_gate=actual_clear ? CavePesterLaunchGate::clear_ray:CavePesterLaunchGate::actual_ray_rejected;
        if (player->input.current_buttons&b_button) {
            eligible=false; facts_.launch_gate=CavePesterLaunchGate::b_held;
        } else if (player->ThrowItemTimeout!=0) {
            eligible=false; facts_.launch_gate=CavePesterLaunchGate::cooldown;
        }
        if (eligible && facts_.throw_count) {
            const auto& preceding=facts_.throws[facts_.throw_count-1];
            const auto impact=preceding.accepted ? preceding.hit_update : preceding.predicted_impact;
            if (!impact || (!preceding.accepted && preceding.forecast_update!=updates)) {
                eligible=false; facts_.launch_gate=CavePesterLaunchGate::flight_unqualified;
            } else {
                // Four private scheduling guard updates cover the animation -> flag
                // -> graph/command boundaries and rounded forecast edge. They are
                // NOT a claimed Source reaction length. Actual counter receipts
                // immediately supersede this prospective reservation.
                facts_.admission_after=preceding.counter_incremented ? preceding.counter_update :
                    impact+facts_.reaction_advances+4;
                facts_.admission_before=preceding.counter_incremented ?
                    updates+static_cast<std::uint64_t>(std::max(0,parent.window)) :
                    impact+facts_.reaction_advances+180;
                if (facts_.predicted_impact<=facts_.admission_after) {
                    eligible=false; facts_.launch_gate=CavePesterLaunchGate::impact_spacing;
                } else if (facts_.predicted_impact>=facts_.admission_before) {
                    eligible=false; facts_.launch_gate=CavePesterLaunchGate::impact_window;
                }
            }
        }
        if (eligible) {
            // Source evicts at five Pester balls; hidden removal tails still
            // occupy capacity. No own flight is evicted to make another throw.
            if (facts_.live_pester_items>=5 || snapshot_.item_count>=snapshot_.items.size()) {
                eligible=false; facts_.launch_gate=CavePesterLaunchGate::item_capacity;
            }
            for (std::size_t i=0;i<snapshot_.item_count;++i) {
                const auto& item=snapshot_.items[i]; bool owned=false;
                for (std::size_t j=0;j<facts_.throw_count;++j) owned |= facts_.throws[j].item==item.identity;
                const bool blocked=!owned && item.kind==ISLE_ITEM_PESTER_BALL && item.state==ISLE_ITEM_FLYING &&
                    (!(item.object_flags&1u) || item.rest_timer<=0) && !(item.flags&ISLE_ITEM_DELETED);
                if (!blocked) continue;
                eligible=false; facts_.launch_gate=CavePesterLaunchGate::outstanding_item;
                facts_.blocking_item=item.identity; facts_.blocking_kind=item.kind; facts_.blocking_state=item.state;
                facts_.blocking_flags=item.flags; facts_.blocking_object_flags=item.object_flags;
                facts_.blocking_rest_timer=item.rest_timer; break;
            }
        }
        if (eligible) {
            pending_throw_=facts_.throw_count++;
            auto& receipt=facts_.throws[pending_throw_];
            receipt.request_update=updates; receipt.launch_forecast=facts_.actual_forecast;
            receipt.launch_eye=facts_.planning_eye; receipt.launch_ray=player->cam_target;
            receipt.launch_player_velocity=player->PlayerVelocity; receipt.receiver=parent.identity;
            receipt.consume_prediction=facts_.consume_prediction;
            receipt.launch_predicted_impact=receipt.predicted_impact=facts_.predicted_impact;
            if (pending_throw_) {
                const auto& preceding=facts_.throws[pending_throw_-1];
                receipt.preceding_impact=preceding.accepted ? preceding.hit_update : preceding.predicted_impact;
            }
            receipt.admission_after=facts_.admission_after; receipt.admission_before=facts_.admission_before;
            receipt.prethrow_count=snapshot_.item_count;
            for (std::size_t i=0;i<receipt.prethrow_count;++i) receipt.prethrow_items[i]=snapshot_.items[i].identity;
            facts_.request_update=updates; ++facts_.requests; pending_press_=true;
            facts_.predicted_flight=flight; facts_.phase=CavePesterPhase::press;
            facts_.launch_gate=CavePesterLaunchGate::requested; facts_.disposition=CavePesterDisposition::none;
            // Neutral B cannot freeze cam_target. The qualified ray is the
            // prospective reticle-object publication, not the stale request ray.
            publish(b_button); return true;
        }
        publish(0,x,y); return true;
    }
    facts_.launch_gate=x || y ? CavePesterLaunchGate::steering:
        player->input.current_buttons&b_button ? CavePesterLaunchGate::b_held:
        actual_clear ? CavePesterLaunchGate::clear_ray:CavePesterLaunchGate::actual_ray_rejected;
    if (!x && !y && !(player->input.current_buttons&b_button) && actual_clear) {
        bool outstanding=false;
        for (std::size_t i=0;i<snapshot_.item_count;++i) {
            const auto& item=snapshot_.items[i];
            const bool blocked=item.kind==ISLE_ITEM_PESTER_BALL && item.state==ISLE_ITEM_FLYING &&
                (!(item.object_flags&1u) || item.rest_timer<=0) && !(item.flags&ISLE_ITEM_DELETED);
            if(blocked && !outstanding) {
                facts_.blocking_item=item.identity; facts_.blocking_kind=item.kind; facts_.blocking_state=item.state;
                facts_.blocking_flags=item.flags; facts_.blocking_object_flags=item.object_flags;
                facts_.blocking_rest_timer=item.rest_timer;
            }
            outstanding|=blocked;
        }
        if (!outstanding) {
            facts_.launch_gate=CavePesterLaunchGate::requested;
            prethrow_count_=snapshot_.item_count;
            for (std::size_t i=0;i<prethrow_count_;++i) prethrow_items_[i]=snapshot_.items[i].identity;
            facts_.request_update=updates; ++facts_.requests; pending_press_=true;
            facts_.predicted_flight=flight; facts_.phase=CavePesterPhase::press; buttons=b_button;
        } else { facts_.disposition=CavePesterDisposition::item_outstanding; facts_.launch_gate=CavePesterLaunchGate::outstanding_item; }
    }
    publish(buttons,x,y); return true;
}
}

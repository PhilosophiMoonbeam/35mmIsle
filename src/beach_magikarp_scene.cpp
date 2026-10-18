#include "beach_magikarp_scene.hpp"
#include <cmath>
#include <exception>

namespace isle::beach {
namespace {
constexpr std::uint32_t helper_identity = 0x802c40a0;
constexpr std::uint32_t definition_identity = 0x802cbff4;
constexpr std::uint32_t chooser_identity = 0x802cc004;
constexpr std::int32_t source_surface = 0x337fb2;
MagikarpSceneResult reject(MagikarpSceneError error, const char* detail) noexcept {
    MagikarpSceneResult result;
    result.error=error; result.detail=detail;
    return result;
}
bool trigger_position(IsleGame* game, MagikarpTrigger trigger, IsleGameVec3& output) noexcept {
    const IsleGameModelNode* nodes=nullptr;
    std::size_t count=0;
    if (isle_game_model_validate_binding(game,trigger.object,trigger.model)!=ISLE_GAME_OK ||
        isle_game_model_peek_nodes(game,trigger.model,&nodes,&count)!=ISLE_GAME_OK ||
        !count || nodes[0].parent!=UINT32_MAX) return false;
    output=nodes[0].transform.position;
    return true;
}
} // namespace
MagikarpScene::MagikarpScene(scene::Services& services, actor::Owner& owner, world::World& world,
                           const actor::Resources& actors, ActorSceneState& state) noexcept
    : services_(services), owner_(owner), world_(world), actors_(actors), state_(state) {}
MagikarpSceneResult MagikarpScene::spawn_at(MagikarpTrigger trigger) noexcept {
    if (actors_.course_id()!=0 || owner_.course_context().course_id()!=0 || &owner_.course_context().beach()!=&state_)
        return reject(MagikarpSceneError::resources,"scene helper requires the canonical Beach actor state");
    // Native identity admission does not sample/copy the source position early:
    // source reads that position only AFTER the genuine current/current spawn.
    if (isle_game_model_validate_binding(services_.game,trigger.object,trigger.model)!=ISLE_GAME_OK)
        return reject(MagikarpSceneError::native,"scene trigger is not a live native GObj/model pair");
    std::size_t definition=actor::actor_no_index;
    try {
        const auto definitions=actors_.definitions();
        for (std::size_t i=0;i<definitions.size();++i) {
            if (definitions[i].wire.guest_address==definition_identity) {
                if (definitions[i].actor_id!=129 || definitions[i].factory_identity!=0x802cac48)
                    return reject(MagikarpSceneError::resources,"scene definition is not the pinned Magikarp factory");
                definition=i; break;
            }
        }
    } catch (const std::exception&) {
        return reject(MagikarpSceneError::resources,"scene ActorResources borrow is not live");
    }
    if (definition==actor::actor_no_index)
        return reject(MagikarpSceneError::resources,"scene definition802CBFF4 is absent");
    auto* current=world_.current();
    world::Spawn spawn{};
    spawn.id=129;
    spawn.position={0,0,0}; spawn.rotation={0,0,0}; spawn.scale={1,1,1};
    // Explicit modern initialization, not a claim about source stack contents.
    spawn.path=world::no_index; spawn.behavior=0;
    MagikarpSceneResult result;
    result.actor=owner_.add_one(definition,current,current,spawn);
    if (!result.actor) {
        result.error=MagikarpSceneError::factory; result.detail=result.actor.status.detail;
        return result;
    }
    IsleGameVec3 position{};
    const IsleGameModelPlacement* placement=nullptr;
    if (!trigger_position(services_.game,trigger,position) ||
        isle_game_model_placement(services_.game,owner_.native_model(result.actor.instance),&placement)!=ISLE_GAME_OK) {
        result.error=MagikarpSceneError::postcopy; result.detail="scene postspawn native position borrow failed";
        return result;
    }
    auto transform=placement->transform;
    transform.position=position;
    const auto copied=owner_.set_placement(result.actor.instance,transform);
    if (!copied) {
        // The result retains the actual published actor on this explicit error.
        // Caller owns cleanup; never pretend the factory did not run, rewrite
        // factory record pos1/targetPos, normalize/reground, or set the latch.
        result.error=MagikarpSceneError::postcopy; result.detail=copied.detail;
        return result;
    }
    result.outcome=MagikarpSceneOutcome::spawned;
    return result;
}
MagikarpSceneResult MagikarpScene::choose(MagikarpTrigger trigger, const world::GroundResult& ground) noexcept {
    if (actors_.course_id()!=0 || owner_.course_context().course_id()!=0 || &owner_.course_context().beach()!=&state_)
        return reject(MagikarpSceneError::resources,"scene chooser requires the canonical Beach actor state");
    if (ground.surface!=source_surface) return {};
    IsleGameVec3 position{};
    if (!trigger_position(services_.game,trigger,position))
        return reject(MagikarpSceneError::native,"scene trigger DObj position is not live");
    const auto& player=owner_.player_source();
    IslePlayerSource* active=nullptr;
    const IsleGameModelPlacement* placement=nullptr;
    if (isle_player_source_context(services_.game,&active)!=ISLE_GAME_OK || active!=&player ||
        isle_game_model_validate_binding(services_.game,player.object,player.model)!=ISLE_GAME_OK ||
        isle_game_model_placement(services_.game,player.model,&placement)!=ISLE_GAME_OK ||
        placement->mode==ISLE_GAME_MODEL_PLACEMENT_DISABLED)
        return reject(MagikarpSceneError::native,"scene requires the actual canonical player AUX placement");
    // Own vector.c304-311,18-20: binary32 subtraction, sum of squares, sqrtf.
    const float dx=position.x-placement->transform.position.x;
    const float dy=position.y-placement->transform.position.y;
    const float dz=position.z-placement->transform.position.z;
    if (!(std::sqrt(dx*dx+dy*dy+dz*dz)>50.0f)) return {};
    const SceneChooser* chooser=nullptr;
    try { chooser=&state_.resources.scene_chooser(chooser_identity); }
    catch (const std::exception&) {
        return reject(MagikarpSceneError::resources,"Beach scene resources borrow is not live");
    }
    std::int32_t total=0;
    for (const auto& entry:chooser->entries) total+=entry.weight;
    if (total<=0)
        return reject(MagikarpSceneError::resources,"scene chooser has no positive weight total");
    MagikarpSceneResult result;
    result.draw=isle_rng_range(&services_.rng,total);
    total=0;
    for (std::size_t i=0;i<chooser->entries.size();++i) {
        const auto& entry=chooser->entries[i];
        total+=entry.weight;
        if (total>result.draw) {
            result.choice=i;
            if (!entry.function_identity) { result.outcome=MagikarpSceneOutcome::null_choice; return result; }
            if (state_.magikarp_active) { result.outcome=MagikarpSceneOutcome::already_active; return result; }
            if (entry.function_identity!=helper_identity) {
                result.error=MagikarpSceneError::resources;
                result.detail="selected scene callback identity has no native source binding";
                return result;
            }
            auto spawned=spawn_at(trigger);
            spawned.draw=result.draw; spawned.choice=i;
            if (spawned) state_.magikarp_active=1;
            return spawned;
        }
    }
    result.error=MagikarpSceneError::resources;
    result.detail="scene draw is outside the source cumulative weights";
    return result;
}
} // namespace isle::beach

#include "actor_collision.hpp"

#include <cmath>

// Own GAME df8815e75f3d79618466e01e0827c6165b93d11c:
// app_level/pokemon.c1146-1210, app_level/app_level.h115-124,
// sys/vector.c5-16,54-60 and world/ground.c40-86.
namespace isle::actor::collision {
namespace {
OwnerStatus rejected(OwnerError error, const char* detail) noexcept { return {error,detail}; }

OwnerStatus forbidden(const IsleActorSource& actor, const IsleActorGround& ground,
                      bool& result) noexcept {
    if (!actor.forbiddenGround) {
        switch (ground.surfaceType) {
        case 0x007f66: case 0x00ff00: case 0x337fb2:
        case 0x4ccccc: case 0x7f6633: case 0x7f667f:
        case 0xff0000: case 0xff4c19: case 0xff7fb2:
            result = true; return {};
        default:
            result = false; return {};
        }
    }
    // The source comparison promotes the unsigned surface to binary32, not a
    // signed surface or an integer-converted list value. NaN/Inf entries have
    // their ordinary float comparison behavior. An ignored tail is not read.
    const float surface = static_cast<float>(ground.surfaceType);
    for (std::size_t i = 0; i < actor.forbiddenGroundCount; ++i) {
        const float entry = actor.forbiddenGround[i];
        if (entry == 0.0f) { result = false; return {}; }
        if (entry == surface) { result = true; return {}; }
    }
    return rejected(OwnerError::admission,"actor forbidden-ground readable extent exhausted");
}

}

OwnerStatus ground_query(world::World& world, float x, float z, IsleActorGround& output, bool& found) noexcept {
    if (world.retired()) return rejected(OwnerError::lifetime,"actor ground query uses retired World");
    world::GroundResult result;
    bool queried = false;
    try {
        const auto* block = world.current();
        if (block) {
            bool member = false;
            for (const auto& candidate : world.blocks()) if (&candidate == block) { member = true; break; }
            if (!member) return rejected(OwnerError::admission,"actor ground current block is foreign");
            if (block->resource) {
                member = false;
                for (const auto& candidate : world.resources().blocks)
                    if (&candidate == block->resource) { member = true; break; }
                if (!member) return rejected(OwnerError::admission,"actor ground block descriptor is foreign");
            }
        }
        queried = world.ground_at(x,z,result);
        if (!queried && block && block->resource && world.resources().height_map)
            return rejected(OwnerError::admission,"actor ground query has unsafe coordinates or malformed/uncovered map");
        // Missing current/descriptor/map is the real source fallback;
        // coordinates remain unconsumed in this arm.
    } catch (...) {
        return rejected(OwnerError::native,"actor native World ground query threw");
    }
    output = {result.height,{result.normal.x,result.normal.y,result.normal.z},
              static_cast<std::uint32_t>(result.surface)};
    found = queried;
    return {};
}

OwnerStatus step_walk(world::World& world, IsleActorSource& actor, IsleGameModel* model,
                      float velocity_x, float velocity_z, std::uint32_t flags,
                      bool& blocked) noexcept {
    // The owning caller supplies its actual World from the same live native
    // game. There is no second game/world registry or shadow actor binding.
    IsleActorSource* bound = nullptr;
    if (isle_actor_source_get(actor.game,actor.object,&bound) != ISLE_GAME_OK || bound != &actor ||
        isle_game_model_validate_binding(actor.game,actor.object,model) != ISLE_GAME_OK)
        return rejected(OwnerError::native,"actor walk requires canonical native source/model binding");
    const IsleGameModelPlacement* current = nullptr;
    if (isle_game_model_placement(actor.game,model,&current) != ISLE_GAME_OK || !current ||
        (current->mode != ISLE_GAME_MODEL_PLACEMENT_RT && current->mode != ISLE_GAME_MODEL_PLACEMENT_RTS))
        return rejected(OwnerError::native,"actor walk requires live native AUX placement");
    const auto& old = current->transform.position;
    const float next_x = old.x+velocity_x;
    const float next_z = old.z+velocity_z;
    if (!std::isfinite(next_x) || !std::isfinite(next_z))
        return rejected(OwnerError::admission,"actor walk has nonfinite prospective position");

    bool found = false;
    auto status = ground_query(world,next_x,next_z,actor.currGround,found);
    if (!status) return status;
    bool prohibited = false;
    status = forbidden(actor,actor.currGround,prohibited);
    if (!status) return status;
    if (prohibited) { blocked = true; return {}; }

    // vector.c uses reciprocal multiplication, and leaves the input untouched
    // when squared components underflow to a zero magnitude. Infinite squared
    // magnitude also has defined source behavior (finite components times 0).
    float offset_x = velocity_x, offset_z = velocity_z;
    const float magnitude = std::sqrt(offset_x*offset_x+offset_z*offset_z);
    if (magnitude != 0.0f) {
        const float reciprocal = 1.0f/magnitude;
        offset_x *= reciprocal;
        offset_z *= reciprocal;
    }
    offset_x *= actor.collisionRadius;
    offset_z *= actor.collisionRadius;
    const float probe_x = old.x+offset_x;
    const float probe_z = old.z+offset_z;
    if (!std::isfinite(probe_x) || !std::isfinite(probe_z))
        return rejected(OwnerError::admission,"actor walk has nonfinite radius probe");
    IsleActorGround probe;
    status = ground_query(world,probe_x,probe_z,probe,found);
    if (!status) return status;
    status = forbidden(actor,probe,prohibited);
    if (!status) return status;
    if (prohibited) { blocked = true; return {}; }

    if ((flags&MOVEMENT_FLAG_FIXED_HEIGHT) && std::fabs(old.y-actor.currGround.height) != 0.0f) {
        blocked = true; return {};
    }
    auto next = current->transform;
    next.position.x = next_x;
    next.position.z = next_z;
    if (flags&MOVEMENT_FLAG_ON_GROUND) next.position.y = actor.currGround.height;
    if (isle_game_model_set_placement(actor.game,model,current->mode,&next) != ISLE_GAME_OK)
        return rejected(OwnerError::native,"actor walk native placement publication failed");
    blocked = false;
    return {};
}
} // namespace isle::actor::collision

#ifndef ISLE_ACTOR_NATIVE_CONTEXT_HPP
#define ISLE_ACTOR_NATIVE_CONTEXT_HPP
#include "actor_course_context.hpp"
#include "actor_owner_source.h"
#include "player_source.h"
namespace isle::actor {
class Owner;
IsleGameResult native_owner(IsleTunnelActorSource*, Owner**) noexcept;
IsleGameResult native_host(IsleTunnelActorSource*, course::NativeHost**) noexcept;
bool native_services_complete(const IsleTunnelActorServices*) noexcept;
IsleGameResult native_owner(IsleVolcanoActorSource*, Owner**) noexcept;
IsleGameResult native_host(IsleVolcanoActorSource*, course::NativeHost**) noexcept;
bool native_services_complete(const IsleVolcanoActorServices*) noexcept;
IsleGameResult native_owner(IsleRiverActorSource*, Owner**) noexcept;
IsleGameResult native_host(IsleRiverActorSource*, course::NativeHost**) noexcept;
bool native_services_complete(const IsleRiverActorServices*) noexcept;
IsleGameResult native_owner(IsleCaveActorSource*, Owner**) noexcept;
IsleGameResult native_host(IsleCaveActorSource*, course::NativeHost**) noexcept;
bool native_services_complete(const IsleCaveActorServices*) noexcept;
IsleGameResult native_owner(IsleValleyActorSource*, Owner**) noexcept;
IsleGameResult native_host(IsleValleyActorSource*, course::NativeHost**) noexcept;
bool native_services_complete(const IsleValleyActorServices*) noexcept;
} // namespace isle::actor
extern "C" {
IsleGameResult isle_actor_owner_associated_player(IsleGame*, IslePlayerSource**);
using IsleActorSixFactory = IsleGameObject* (*)(int32_t,uint16_t,IsleActorFactoryBlock*,
    IsleActorFactoryBlock*,IsleActorFactorySpawn*,IsleActorFactoryInitialization*);
IsleGameResult isle_actor_source_factory_six_call(IsleGame*, IsleActorSixFactory,
    int32_t,uint16_t,IsleActorFactoryBlock*,IsleActorFactoryBlock*,IsleActorFactorySpawn*,
    IsleActorFactoryInitialization*,IsleGameObject**);
IsleGameResult isle_actor_source_add_geo_call(IsleGame*, IsleGameObject*, uint16_t,
    const IsleActorGeoDefinition*, IsleGameObject**);
using IsleActorBlockChange = void (*)(IsleGameObject*,float,float,float,float,float,float);
IsleGameResult isle_actor_source_block_callback_call(IsleGame*, IsleCourseActorBase*, IsleActorBlockChange,
    IsleGameObject*,float,float,float,float,float,float);
IsleGameResult isle_actor_native_tunnel_animation_event(IsleTunnelActorSource*, IsleTunnelAnimationCallback, const IsleActorNodeRef*, int32_t, float);
IsleGameResult isle_actor_native_volcano_animation_event(IsleVolcanoActorSource*, IsleVolcanoAnimationCallback, const IsleActorNodeRef*, int32_t, float);
IsleGameResult isle_actor_native_river_animation_event(IsleRiverActorSource*, IsleRiverAnimationCallback, const IsleActorNodeRef*, int32_t, float);
IsleGameResult isle_actor_native_cave_animation_event(IsleCaveActorSource*, IsleCaveAnimationCallback, const IsleActorNodeRef*, int32_t, float);
IsleGameResult isle_actor_native_valley_animation_event(IsleValleyActorSource*, IsleValleyAnimationCallback, const IsleActorNodeRef*, int32_t, float);
}
#endif

#ifndef ISLE_COURSE_ACTOR_NATIVE_SERVICES_HPP
#define ISLE_COURSE_ACTOR_NATIVE_SERVICES_HPP
#include "tunnel_actor_source.h"
#include "volcano_actor_source.h"
#include "river_actor_source.h"
#include "cave_actor_source.h"
#include "valley_actor_source.h"
namespace isle::course {
IsleTunnelActorServices make_tunnel_actor_services() noexcept;
IsleVolcanoActorServices make_volcano_actor_services() noexcept;
IsleRiverActorServices make_river_actor_services() noexcept;
IsleCaveActorServices make_cave_actor_services() noexcept;
IsleValleyActorServices make_valley_actor_services() noexcept;
}
#endif

#ifndef ISLE_PRIVATE_NATIVE_VOLCANO_FOOD_GEOMETRY_HPP
#define ISLE_PRIVATE_NATIVE_VOLCANO_FOOD_GEOMETRY_HPP

#include "35mmisle/engine.h"
#include <array>
#include <cstddef>

struct IslePlayerSource;
namespace isle::world { class World; struct GroundResult; }

namespace isle::private_campaign::food_geometry {
struct Sphere { IsleGameVec3 center{}; float radius=0; float pursuit_speed=0; };
struct Walker {
    bool forbidden_valid=true;
    float radius=0;
    IsleGameVec3 root{}, collision{};
    std::array<float,16> forbidden{};
    std::size_t forbidden_count=0;
};
struct Scene { std::array<Sphere,128> spheres{}; std::size_t sphere_count=0; };
struct LandingForecast { IsleGameVec3 root{}; int surface=-1; unsigned rebounds=0; };
float squared(const IsleGameVec3&,const IsleGameVec3&) noexcept;
bool ground(const world::World&,const IsleGameVec3&,world::GroundResult&);
bool walkable(const Walker&,int surface);
bool static_clear(const world::World&,const IsleGameVec3&,const IsleGameVec3&);
bool landing(const world::World&,const IslePlayerSource&,const Walker&,const Scene&,
             const IsleGameVec3& ray,const IsleGameVec3& target,LandingForecast&);
bool corridor(const world::World&,const Walker&,const IsleGameVec3& target,
              float speed,float stopping_distance,IsleGameVec3& stopped,unsigned& steps);
} // namespace isle::private_campaign::food_geometry
#endif

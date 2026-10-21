#include "native_volcano_food_geometry.hpp"
#include "player_source.h"
#include "world_runtime.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace isle::private_campaign::food_geometry {
namespace {
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
bool finite(const IsleGameVec3& p) { return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z); }
} // namespace
float squared(const IsleGameVec3& p, const IsleGameVec3& q) noexcept {
    const float x=p.x-q.x, y=p.y-q.y, dz=p.z-q.z; return x*x+y*y+dz*dz;
}
static bool food_surface(int value) {
    switch (value) {
    case 0x00ff00: case 0xff4c19: case 0x0019ff: case 0x007f66:
    case 0x337fb2: case 0x4ccccc: case 0xff0000: return false;
    default: return value>=0;
    }
}
static bool safe_ground(const world::GroundResult& result) {
    return std::isfinite(result.height) && std::abs(result.height)<9900 && food_surface(result.surface) &&
        std::isfinite(result.normal.x) && std::isfinite(result.normal.y) &&
        std::isfinite(result.normal.z) && result.normal.y>=.9f;
}
bool ground(const world::World& world, const IsleGameVec3& point, world::GroundResult& result) {
    return finite(point) && std::abs(point.x)<9900 && std::abs(point.z)<9900 &&
        world.ground_at(point.x,point.z,result) && safe_ground(result);
}
bool walkable(const Walker& observed, int surface) {
    if (!observed.forbidden_valid || observed.forbidden_count>observed.forbidden.size() || !food_surface(surface)) return false;
    if (observed.forbidden_count) {
        for (std::size_t i=0;i<observed.forbidden_count;++i) {
            if (!observed.forbidden[i]) return true;
            if (observed.forbidden[i]==static_cast<float>(surface)) return false;
        }
        return false;
    }
    return surface!=0x7f6633 && surface!=0x7f667f && surface!=0xff7fb2;
}
bool static_clear(const world::World& world, const IsleGameVec3& from, const IsleGameVec3& to) {
    world::Vec3 hit{},reflected{};
    return world.collide({from.x,from.y,from.z},{to.x-from.x,to.y-from.y,to.z-from.z},hit,reflected)<0;
}
static bool sphere_hit(const Sphere& sphere, const IsleGameVec3& from, const IsleGameVec3& to, float extra=0) {
    const IsleGameVec3 delta{to.x-from.x,to.y-from.y,to.z-from.z};
    const float length=squared(from,to);
    const float t=length>0 ? std::clamp(((sphere.center.x-from.x)*delta.x+(sphere.center.y-from.y)*delta.y+
                                        (sphere.center.z-from.z)*delta.z)/length,0.0f,1.0f) : 0;
    return squared({from.x+t*delta.x,from.y+t*delta.y,from.z+t*delta.z},sphere.center)<=
        (sphere.radius+12+extra)*(sphere.radius+12+extra);
}
bool corridor(const world::World& world, const Walker& observed, const IsleGameVec3& target,
              float speed, float stopping_distance, IsleGameVec3& stopped, unsigned& steps) {
    steps=0;
    if (!(std::isfinite(speed) && speed>0 && std::isfinite(stopping_distance) && stopping_distance>0)) return false;
    const float distance=std::hypot(target.x-observed.root.x,target.z-observed.root.z);
    if (!(distance>observed.radius+75 && distance<550)) return false;
    // Pure prospective geometry at Source's speed*.033 step: check the next root
    // AND the current-root forward-radius probe, then the 3D stopping rule.
    // Actual later RunToApple/WithApple, not this forecast, earns the receipt.
    const float step=speed*.033f;
    IsleGameVec3 root=observed.root;
    for (unsigned i=0;i<224;++i) {
        float dx=target.x-root.x,dz=target.z-root.z;
        if (std::abs(dz)<.00001f) { dx=1; dz=0; } // Pinned GetHeadingVector.
        const float length=std::hypot(dx,dz);
        if (!(length>0 && std::isfinite(length))) return false;
        const float ux=dx/length,uz=dz/length;
        IsleGameVec3 next{root.x+ux*step,0,root.z+uz*step};
        const IsleGameVec3 ahead{root.x+ux*observed.radius,0,root.z+uz*observed.radius};
        world::GroundResult floor{},forward{};
        if (!ground(world,next,floor) || !walkable(observed,floor.surface) ||
            !ground(world,ahead,forward) || !walkable(observed,forward.surface)) return false;
        next.y=floor.height;
        if (std::abs(next.y-root.y)>16 ||
            !static_clear(world,{root.x,root.y+observed.radius+12,root.z},
                               {next.x,next.y+observed.radius+12,next.z})) return false;
        root=next;
        const float remaining=std::sqrt(squared(target,root));
        steps=i+1;
        if (std::abs(remaining-stopping_distance)<step || remaining<stopping_distance) { stopped=root; return true; }
    }
    return false;
}
static float damping(int surface) {
    // Pinned items.c404-474: unknown/default ordinary surfaces stop the Apple.
    switch (surface) {
    case 0x4c7f00: case 0x996666: case 0xff9919: case 0xb2997f: return .2f;
    case 0x331919: case 0x193333: case 0x4c1900: case 0x4c4c33: case 0x7f4c00:
    case 0x7f667f: case 0x7f7f7f: case 0xff7fb2: case 0x7f6633: return .3f;
    default: return 0;
    }
}
bool landing(const world::World& world, const IslePlayerSource& player, const Walker& observed, const Scene& scene,
             const IsleGameVec3& ray, const IsleGameVec3& target, LandingForecast& forecast) {
    auto& contact=forecast.root;
    auto& surface=forecast.surface;
    auto& rebounds=forecast.rebounds;
    const IsleGameCameraParameters* camera=nullptr;
    require(isle_game_camera_parameters(player.game,player.camera,&camera)==ISLE_GAME_OK && camera,
            "Volcano food actual launch camera unavailable");
    IsleGameVec3 position{camera->look_at.eye[0],camera->look_at.eye[1],camera->look_at.eye[2]};
    IsleGameVec3 velocity{ray.x-position.x,ray.y-position.y+100,ray.z-position.z};
    const float length=std::hypot(velocity.x,velocity.y,velocity.z);
    if (!(finite(position) && std::isfinite(length) && length>0 && scene.sphere_count<=scene.spheres.size())) return false;
    velocity={50*velocity.x/length+player.PlayerVelocity.x,50*velocity.y/length+player.PlayerVelocity.y,
              50*velocity.z/length+player.PlayerVelocity.z};
    IsleGameVec3 previous=position;
    rebounds=0;
    unsigned first_contact_step=0;
    for (unsigned step=0;step<160;++step) {
        world::GroundResult floor{},ceiling{};
        // Airborne travel may cross lava/water; only actual contact must be
        // ordinary safe food ground. Do not reject terrain merely below an arc.
        if (!world.ground_at(position.x,position.z,floor) || !std::isfinite(floor.height) ||
            (world.ceiling_at(position.x,position.z,ceiling) && position.y+12>=ceiling.height)) return false;
        if (position.y<=floor.height+.375f) {
            const IsleGameVec3 penetrated=position;
            if (position.y<floor.height-.375f) {
                // Pinned Items_BounceFromGround recovers the first contact,
                // including its material/normal, before deciding settlement.
                IsleGameVec3 above=previous,below=position;
                for (unsigned subdivision=0;subdivision<16;++subdivision) {
                    position={(above.x+below.x)*.5f,(above.y+below.y)*.5f,(above.z+below.z)*.5f};
                    if (!world.ground_at(position.x,position.z,floor) || !std::isfinite(floor.height)) return false;
                    if (std::abs(position.y-floor.height)<=.375f) break;
                    if (position.y<floor.height) below=position; else above=position;
                }
            }
            if (!safe_ground(floor)) return false;
            contact={position.x,floor.height+12,position.z}; surface=floor.surface;
            if (!rebounds && squared(contact,{target.x,target.y+12,target.z})>35*35) return false;
            const float restitution=damping(floor.surface);
            const float incoming_speed=std::hypot(velocity.x,velocity.y,velocity.z);
            const float speed=restitution*incoming_speed;
            if (speed>=13) {
                if (rebounds) return false; // At most one rebound; no horizon reset.
                const IsleGameVec3 unit{velocity.x/incoming_speed,velocity.y/incoming_speed,velocity.z/incoming_speed};
                const float dot=unit.x*floor.normal.x+unit.y*floor.normal.y+unit.z*floor.normal.z;
                velocity={speed*(unit.x-2*dot*floor.normal.x),
                          speed*(unit.y-2*dot*floor.normal.y),
                          speed*(unit.z-2*dot*floor.normal.z)};
                // Items_CollideWithGround advances this half-step immediately,
                // without gravity or another collision query in this update.
                previous=penetrated;
                position={contact.x+velocity.x*.5f,contact.y+velocity.y*.5f,contact.z+velocity.z*.5f};
                rebounds=1; first_contact_step=step;
                continue;
            }
            if (!(speed<13)) return false;
            if (!(squared(contact,observed.collision)<590*590)) return false;
            for (std::size_t i=0;i<scene.sphere_count;++i) {
                // Prospective horizontal-pursuit margin, not a guarantee
                // about future 3D Actor motion or Source update ordering.
                const float pursuit=scene.spheres[i].pursuit_speed>0 && rebounds
                    ? scene.spheres[i].pursuit_speed*.033f*float(step-first_contact_step) : 0;
                if (sphere_hit(scene.spheres[i],contact,contact,pursuit)) return false;
            }
            return true;
        }
        // Source queries static/Actor collisions BEFORE applying gravity.
        const IsleGameVec3 query{position.x+velocity.x*.5f,position.y+velocity.y*.5f,position.z+velocity.z*.5f};
        if (!static_clear(world,position,query)) return false;
        for (std::size_t i=0;i<scene.sphere_count;++i) {
            const float pursuit=scene.spheres[i].pursuit_speed>0 && rebounds
                ? scene.spheres[i].pursuit_speed*.033f*float(step-first_contact_step) : 0;
            if (sphere_hit(scene.spheres[i],position,query,pursuit)) return false;
        }
        previous=position; velocity.y-=.6f;
        position={position.x+velocity.x*.5f,position.y+velocity.y*.5f,position.z+velocity.z*.5f};
    }
    return false;
}
} // namespace isle::private_campaign::food_geometry

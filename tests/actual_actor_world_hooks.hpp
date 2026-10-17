#ifndef ISLE_TEST_ACTUAL_ACTOR_WORLD_HOOKS_HPP
#define ISLE_TEST_ACTUAL_ACTOR_WORLD_HOOKS_HPP
#include "actor_owner.hpp"
#include <array>
#include <exception>

namespace isle::private_actor {
// Real native services for manually spawned actual-factory API consumers.
// Matches the existing genuine course fixture; no no-op shift providers.
struct WorldHooks {
    scene::Services& services;
    IsleGameCamera* camera;
    actor::Owner* owner=nullptr;
    static bool remove(void* user,world::Block& block) noexcept {
        auto& self=*static_cast<WorldHooks*>(user);
        if(!self.owner) { self.services.fail("actual actor World removal lost its live owner");return false; }
        const auto result=self.owner->remove_block(block);
        if(!result) {self.services.fail(result.detail);return false;}
        return true;
    }
    static bool camera_shift(void* user,world::Vec3 delta) noexcept {
        auto& self=*static_cast<WorldHooks*>(user);
        const IsleGameCameraParameters* current=nullptr;
        if(!self.services.check(isle_game_camera_parameters(self.services.game,self.camera,&current),"actual actor World camera borrow"))return false;
        auto view=current->look_at;const std::array offset{delta.x,delta.y,delta.z};
        for(unsigned i=0;i<3;++i) {view.eye[i]+=offset[i];view.at[i]+=offset[i];}
        return self.services.check(isle_game_camera_set_look_at(self.services.game,self.camera,&view),"actual actor World camera shift");
    }
    static bool effects_shift(void* user,world::Vec3 delta) noexcept {
        auto& self=*static_cast<WorldHooks*>(user);
        if(!self.owner) {self.services.fail("actual actor World FX owner absent");return false;}
        self.owner->effects().translate({delta.x,delta.y,delta.z});
        if(!self.owner->effects().status()) {self.services.fail("actual actor World FX translation failed");return false;}
        return true;
    }
    world::Callbacks callbacks() noexcept { return {this,nullptr,remove,nullptr,camera_shift,effects_shift,nullptr}; }
};
struct WorldLifetime {
    world::World& world;WorldHooks& hooks;
    ~WorldLifetime() {if(!world.retire())std::terminate();hooks.owner=nullptr;}
};
} // namespace isle::private_actor
#endif

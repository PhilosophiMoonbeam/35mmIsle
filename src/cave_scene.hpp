#ifndef ISLE_CAVE_SCENE_HPP
#define ISLE_CAVE_SCENE_HPP
#include "course_scene_native.hpp"
#include "course_actor_native_services.hpp"
#include "course_actor_egg_native.hpp"
#include "cave_actor.hpp"
#include "cave_lifecycle_source.h"
#include <optional>
#include <type_traits>
namespace isle::cave {
struct Adapter final {
    using ActorSource=IsleCaveActorSource;
    using LifecycleSource=IsleCaveLifecycleSource;
    using SceneSetup=IsleCaveLifecycleSceneSetup;
    const IsleCaveActorServices actor_services;
    const IsleCaveLifecycleServices lifecycle_services;
    ActorSource actors{};
    course::EggGeometry egg_geometry;
    LifecycleSource source{};
    std::optional<CourseState> course_state;
    course::SceneMechanics native;
    course::Lifecycle<Adapter>* entry=nullptr;
    std::array<IsleGameObject**,13> reference_fields{{&actors.D_802C6D4C_6491FC[0],&actors.D_802C6D4C_6491FC[1],&actors.D_802C6D4C_6491FC[2],&actors.D_802C6D4C_6491FC[3],&actors.D_802C72A0_649750[0],&actors.D_802C72A0_649750[1],&actors.D_802C72A0_649750[2],&actors.D_802C72A0_649750[3],&actors.D_802C75EC_649A9C,&actors.D_802C75F0_649AA0,&actors.D_802C7B4C_649FFC,&actors.D_802C7C3C_64A0EC,&actors.D_802C8044_64A4F4}};
    Adapter() noexcept;
    Adapter(const Adapter&)=delete;
    Adapter& operator=(const Adapter&)=delete;
    Adapter(Adapter&&)=delete;
    Adapter& operator=(Adapter&&)=delete;
    static constexpr auto run_entry=&isle_cave_lifecycle_run;
    IsleGameResult prepare(void*,IsleContinuationDomain*,std::span<const std::byte>,std::int32_t) noexcept;
    course::NativeDescription description();
    actor::CourseContext context();
    IsleGameResult bind_scene(course::NativeHost&) noexcept;
    IsleGameResult activate(course::NativeHost&) noexcept;
    IsleGameResult invoke_init() noexcept;
    IsleGameResult release_references(course::NativeHost&) noexcept;
    IsleGameResult deactivate(course::NativeHost&) noexcept;
    void clear_scene() noexcept;
    IsleGameResult source_status() const noexcept;
    bool requested() const noexcept;
    bool completed() const noexcept;
    bool completion_ready() const noexcept;
    const SceneSetup& setup() const noexcept;
    std::int32_t output() const noexcept { return source.output_scene; }
    IsleGameResult complete_scene(const course::SceneJoinReceipt&,IsleGameResult) noexcept;
    bool active() const noexcept;
    IsleGameResult validate_admission() const noexcept;
    static IsleGameResult native_context(IsleGame*,IsleGameObject*,LifecycleSource**) noexcept;
    static IsleGameResult block_change(IsleCourseActorBase*,std::uint32_t,IsleGameObject*,const IsleGameVec3*,const IsleGameVec3*) noexcept;
    static IsleGameResult block_remove(IsleCourseActorBase*,std::uint32_t,IsleGameObject*) noexcept;
    static bool add(void*,world::Block&,world::Block&) noexcept;
    static bool change(void*,world::Block&,world::Block&) noexcept;
    static bool remove(void*,world::Block&) noexcept;
    static bool exit_block(void*,world::Block&) noexcept;
    static bool movement(void*,std::size_t,float) noexcept;
    static IsleGameResult end(void*,std::int32_t) noexcept;
    static IsleGameResult pause(void*,std::int32_t) noexcept;
    template<class Function> static IsleGameResult invoke(LifecycleSource* s,Function function) noexcept {
        if (!s || !s->owner) return ISLE_GAME_INVALID_ARGUMENT;
        auto& self=*static_cast<Adapter*>(s->owner);
        if (&self.source!=s || s->actors!=&self.actors || self.actors.base.owner!=&self || !self.entry || !s->domain) return ISLE_GAME_INVALID_STATE;
        if (s->services!=&self.lifecycle_services) return ISLE_GAME_INVALID_STATE;
        IsleGameResult result;
        try { result=function(self); } catch (...) { result=ISLE_GAME_INVALID_STATE; }
        if (result!=ISLE_GAME_OK && s->status==ISLE_GAME_OK) s->status=result;
        return result;
    }
};
using Lifecycle=course::Lifecycle<Adapter>;
using LifecycleCreateResult=course::LifecycleCreateResult<Adapter>;
}
#endif

#ifndef ISLE_TUNNEL_SCENE_HPP
#define ISLE_TUNNEL_SCENE_HPP
#include "course_scene_native.hpp"
#include "course_actor_native_services.hpp"
#include "course_actor_egg_native.hpp"
#include "tunnel_actor.hpp"
#include "tunnel_lifecycle_source.h"
#include <optional>
#include <type_traits>
namespace isle::tunnel {
struct Adapter final {
    using ActorSource=IsleTunnelActorSource;
    using LifecycleSource=IsleTunnelLifecycleSource;
    using SceneSetup=IsleTunnelLifecycleSceneSetup;
    const IsleTunnelActorServices actor_services;
    ActorSource actors{};
    course::EggGeometry egg_geometry;
    LifecycleSource source{};
    std::optional<CourseState> course_state;
    course::SceneMechanics native;
    course::Lifecycle<Adapter>* entry=nullptr;
    std::array<IsleGameObject**,22> reference_fields{{&actors.D_802EF484_5EC554,&actors.D_802EF488_5EC558,&actors.D_802EFFD0_5ED0A0,&actors.D_802F0070_5ED140,&actors.D_802EE8AC_5EB97C,&actors.magnemite_FirstMagnemite,&actors.magnemite_SecondMagnemite,&actors.magnemite_ThirdMagnemite,&actors.magnemite_GroupLeader,&actors.D_802EE294_5EB364,&actors.D_802EEEC0_5EBF90,&actors.D_802EEEC4_5EBF94,&actors.D_802EEEC8_5EBF98,&actors.D_802EEECC_5EBF9C,&actors.D_802EF9F4_5ECAC4[0],&actors.D_802EF9F4_5ECAC4[1],&actors.D_802EF9F4_5ECAC4[2],&actors.D_802F0210_5ED2E0,&actors.D_80344748_641818,&actors.D_802EFBA4_5ECC74,&actors.D_802EFF28_5ECFF8,&source.D_802F01E4_5ED2B4}};
    std::array<IslePlayerCameraAnimationClip,2> camera_clips{};
    Adapter() noexcept;
    Adapter(const Adapter&)=delete;
    Adapter& operator=(const Adapter&)=delete;
    Adapter(Adapter&&)=delete;
    Adapter& operator=(Adapter&&)=delete;
    static constexpr auto run_entry=&isle_tunnel_lifecycle_run;
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

#ifndef ISLE_VALLEY_SCENE_HPP
#define ISLE_VALLEY_SCENE_HPP
#include "course_scene_native.hpp"
#include "course_actor_native_services.hpp"
#include "valley_actor.hpp"
#include "valley_lifecycle_source.h"
#include "valley_actor_native.hpp"
#include <optional>
#include <type_traits>
namespace isle::valley {
struct Adapter final {
    using ActorSource=IsleValleyActorSource;
    using LifecycleSource=IsleValleyLifecycleSource;
    using SceneSetup=IsleValleyLifecycleSceneSetup;
    const IsleValleyActorServices actor_services;
    const IsleValleyLifecycleServices lifecycle_services;
    ActorSource actors{};
    LifecycleSource source{};
    std::optional<CourseState> course_state;
    ActorNativeMechanics actor_native;
    course::SceneMechanics native;
    course::Lifecycle<Adapter>* entry=nullptr;
    std::array<IsleGameObject**,12> reference_fields{{&actors.D_802D2980_7ABF10,&actors.D_802D3B34_7AD0C4,&actors.D_802D2A84_7AC014,&actors.D_802D2B8C_7AC11C,&actors.D_802D2B90_7AC120,&actors.D_802D2D74_7AC304,&actors.D_802D2FF0_7AC580,&actors.D_802D3FF0_7AD580,&actors.D_802D3FF4_7AD584,&actors.D_802D41A4_7AD734,&actors.D_802EC644_7C5BD4,&actors.D_8034E0A8_827638}};
    Adapter() noexcept;
    Adapter(const Adapter&)=delete;
    Adapter& operator=(const Adapter&)=delete;
    Adapter(Adapter&&)=delete;
    Adapter& operator=(Adapter&&)=delete;
    static constexpr auto run_entry=&isle_valley_lifecycle_run;
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

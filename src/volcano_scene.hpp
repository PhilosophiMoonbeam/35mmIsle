#ifndef ISLE_VOLCANO_SCENE_HPP
#define ISLE_VOLCANO_SCENE_HPP
#include "course_scene_native.hpp"
#include "course_actor_native_services.hpp"
#include "volcano_actor.hpp"
#include "volcano_lifecycle_source.h"
#include <optional>
#include <type_traits>
namespace isle::volcano {
struct Adapter final {
    using ActorSource=IsleVolcanoActorSource;
    using LifecycleSource=IsleVolcanoLifecycleSource;
    using SceneSetup=IsleVolcanoScene;
    const IsleVolcanoActorServices actor_services;
    const IsleVolcanoLifecycleServices lifecycle_services;
    ActorSource actors{};
    LifecycleSource source{};
    std::optional<CourseState> course_state;
    course::SceneMechanics native;
    course::Lifecycle<Adapter>* entry=nullptr;
    std::array<IsleGameObject**,7> reference_fields{{&actors.charmander_Group1Pokemon,&actors.charmander_Group2Pokemon,&actors.charmander_Group3Pokemon,&actors.charmander_Group4Pokemon,&actors.charmander_Group5Pokemon,&actors.volcano_PlayerObj,&actors.magmar_loneMagmar}};
    Adapter() noexcept;
    Adapter(const Adapter&)=delete;
    Adapter& operator=(const Adapter&)=delete;
    Adapter(Adapter&&)=delete;
    Adapter& operator=(Adapter&&)=delete;
    static constexpr auto run_entry=&isle_volcano_lifecycle_run;
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

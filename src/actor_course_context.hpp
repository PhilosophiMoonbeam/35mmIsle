#ifndef ISLE_ACTOR_COURSE_CONTEXT_HPP
#define ISLE_ACTOR_COURSE_CONTEXT_HPP
#include "beach_actor_scene.hpp"
#include "tunnel_actor_source.h"
#include "tunnel_lifecycle_source.h"
#include "volcano_actor_source.h"
#include "volcano_lifecycle_source.h"
#include "river_actor_source.h"
#include "river_lifecycle_source.h"
#include "cave_actor_source.h"
#include "cave_lifecycle_source.h"
#include "valley_actor_source.h"
#include "valley_lifecycle_source.h"
#include "rainbow_actor_source.h"
#include "rainbow_lifecycle_source.h"
#include <cstdint>
#include <stdexcept>
#include <variant>

namespace isle::course { class NativeHost; class EggGeometry; }

namespace isle::tunnel {
class CourseState final {
public:
    CourseState(IsleTunnelActorSource& actor, IsleTunnelLifecycleSource& lifecycle,
        const IsleTunnelActorServices& services)
        : actors(actor), source(lifecycle), actor_services(services) { require_live(); }
    CourseState(const CourseState&) = delete;
    CourseState& operator=(const CourseState&) = delete;
    CourseState(CourseState&&) = delete;
    CourseState& operator=(CourseState&&) = delete;
    void require_live() const {
        const auto& base = actors.base;
        if (!base.owner || source.owner != base.owner || source.actors != &actors ||
            !source.domain || base.course_id != 1 || base.status != ISLE_GAME_OK ||
            source.status != ISLE_GAME_OK ||
            (base.phase != ISLE_COURSE_ACTOR_PREPARED && base.phase != ISLE_COURSE_ACTOR_ACTIVE) ||
            (base.phase == ISLE_COURSE_ACTOR_PREPARED && (base.nativeGame || actors.services)) ||
            (base.phase == ISLE_COURSE_ACTOR_ACTIVE &&
                (!base.nativeGame || actors.services != &actor_services)))
            throw std::invalid_argument("Tunnel course state requires its exact live Source pair and table");
    }
    IsleTunnelActorSource& actors;
    IsleTunnelLifecycleSource& source;
    const IsleTunnelActorServices& actor_services;
    course::NativeHost* native_host = nullptr;
    course::EggGeometry* native_egg_geometry = nullptr;
};
} // namespace isle::tunnel

namespace isle::volcano {
class CourseState final {
public:
    CourseState(IsleVolcanoActorSource& actor, IsleVolcanoLifecycleSource& lifecycle,
        const IsleVolcanoActorServices& services)
        : actors(actor), source(lifecycle), actor_services(services) { require_live(); }
    CourseState(const CourseState&) = delete;
    CourseState& operator=(const CourseState&) = delete;
    CourseState(CourseState&&) = delete;
    CourseState& operator=(CourseState&&) = delete;
    void require_live() const {
        const auto& base = actors.base;
        if (!base.owner || source.owner != base.owner || source.actors != &actors ||
            !source.domain || base.course_id != 2 || base.status != ISLE_GAME_OK ||
            source.status != ISLE_GAME_OK ||
            (base.phase != ISLE_COURSE_ACTOR_PREPARED && base.phase != ISLE_COURSE_ACTOR_ACTIVE) ||
            (base.phase == ISLE_COURSE_ACTOR_PREPARED && (base.nativeGame || actors.services)) ||
            (base.phase == ISLE_COURSE_ACTOR_ACTIVE &&
                (!base.nativeGame || actors.services != &actor_services)))
            throw std::invalid_argument("Volcano course state requires its exact live Source pair and table");
    }
    IsleVolcanoActorSource& actors;
    IsleVolcanoLifecycleSource& source;
    const IsleVolcanoActorServices& actor_services;
    course::NativeHost* native_host = nullptr;
};
} // namespace isle::volcano

namespace isle::river {
class CourseState final {
public:
    CourseState(IsleRiverActorSource& actor, IsleRiverLifecycleSource& lifecycle,
        const IsleRiverActorServices& services)
        : actors(actor), source(lifecycle), actor_services(services) { require_live(); }
    CourseState(const CourseState&) = delete;
    CourseState& operator=(const CourseState&) = delete;
    CourseState(CourseState&&) = delete;
    CourseState& operator=(CourseState&&) = delete;
    void require_live() const {
        const auto& base = actors.base;
        if (!base.owner || source.owner != base.owner || source.actors != &actors ||
            !source.domain || base.course_id != 3 || base.status != ISLE_GAME_OK ||
            source.status != ISLE_GAME_OK ||
            (base.phase != ISLE_COURSE_ACTOR_PREPARED && base.phase != ISLE_COURSE_ACTOR_ACTIVE) ||
            (base.phase == ISLE_COURSE_ACTOR_PREPARED && (base.nativeGame || actors.services)) ||
            (base.phase == ISLE_COURSE_ACTOR_ACTIVE &&
                (!base.nativeGame || actors.services != &actor_services)))
            throw std::invalid_argument("River course state requires its exact live Source pair and table");
    }
    IsleRiverActorSource& actors;
    IsleRiverLifecycleSource& source;
    const IsleRiverActorServices& actor_services;
    course::NativeHost* native_host = nullptr;
};
} // namespace isle::river

namespace isle::cave {
class CourseState final {
public:
    CourseState(IsleCaveActorSource& actor, IsleCaveLifecycleSource& lifecycle,
        const IsleCaveActorServices& services)
        : actors(actor), source(lifecycle), actor_services(services) { require_live(); }
    CourseState(const CourseState&) = delete;
    CourseState& operator=(const CourseState&) = delete;
    CourseState(CourseState&&) = delete;
    CourseState& operator=(CourseState&&) = delete;
    void require_live() const {
        const auto& base = actors.base;
        if (!base.owner || source.owner != base.owner || source.actors != &actors ||
            !source.domain || base.course_id != 4 || base.status != ISLE_GAME_OK ||
            source.status != ISLE_GAME_OK ||
            (base.phase != ISLE_COURSE_ACTOR_PREPARED && base.phase != ISLE_COURSE_ACTOR_ACTIVE) ||
            (base.phase == ISLE_COURSE_ACTOR_PREPARED && (base.nativeGame || actors.services)) ||
            (base.phase == ISLE_COURSE_ACTOR_ACTIVE &&
                (!base.nativeGame || actors.services != &actor_services)))
            throw std::invalid_argument("Cave course state requires its exact live Source pair and table");
    }
    IsleCaveActorSource& actors;
    IsleCaveLifecycleSource& source;
    const IsleCaveActorServices& actor_services;
    course::NativeHost* native_host = nullptr;
    course::EggGeometry* native_egg_geometry = nullptr;
};
} // namespace isle::cave

namespace isle::valley {
class ActorNativeMechanics;
class CourseState final {
public:
    CourseState(IsleValleyActorSource& actor, IsleValleyLifecycleSource& lifecycle,
        const IsleValleyActorServices& services)
        : actors(actor), source(lifecycle), actor_services(services) { require_live(); }
    CourseState(const CourseState&) = delete;
    CourseState& operator=(const CourseState&) = delete;
    CourseState(CourseState&&) = delete;
    CourseState& operator=(CourseState&&) = delete;
    void require_live() const {
        const auto& base = actors.base;
        if (!base.owner || source.owner != base.owner || source.actors != &actors ||
            !source.domain || base.course_id != 5 || base.status != ISLE_GAME_OK ||
            source.status != ISLE_GAME_OK ||
            (base.phase != ISLE_COURSE_ACTOR_PREPARED && base.phase != ISLE_COURSE_ACTOR_ACTIVE) ||
            (base.phase == ISLE_COURSE_ACTOR_PREPARED && (base.nativeGame || actors.services)) ||
            (base.phase == ISLE_COURSE_ACTOR_ACTIVE &&
                (!base.nativeGame || actors.services != &actor_services)))
            throw std::invalid_argument("Valley course state requires its exact live Source pair and table");
    }
    IsleValleyActorSource& actors;
    IsleValleyLifecycleSource& source;
    const IsleValleyActorServices& actor_services;
    course::NativeHost* native_host = nullptr;
    // Fixed Adapter-owned native backing; published after both Source prepares,
    // bound after Host.prepare and cleared only after actual scene joins.
    ActorNativeMechanics* native_actor_mechanics = nullptr;
};
} // namespace isle::valley

namespace isle::rainbow {
// Borrow the two original fixed-address records. The independent entry retains
// them beyond native Storage; no actor scalar, BGM state or clock is mirrored.
class CourseState final {
public:
    CourseState(IsleRainbowActorSource& actor, IsleRainbowLifecycleSource& lifecycle)
        : actors(actor), source(lifecycle) { require_live(); }
    CourseState(const CourseState&) = delete;
    CourseState& operator=(const CourseState&) = delete;
    CourseState(CourseState&&) = delete;
    CourseState& operator=(CourseState&&) = delete;
    void require_live() const {
        if (!actors.owner || source.owner != actors.owner || source.actors != &actors ||
            !source.domain || actors.status != ISLE_GAME_OK || source.status != ISLE_GAME_OK ||
            (actors.phase != ISLE_RAINBOW_ACTOR_PREPARED && actors.phase != ISLE_RAINBOW_ACTOR_ACTIVE) ||
            (actors.phase == ISLE_RAINBOW_ACTOR_PREPARED && (actors.nativeGame || actors.services)) ||
            (actors.phase == ISLE_RAINBOW_ACTOR_ACTIVE && (!actors.nativeGame || !actors.services)))
            throw std::invalid_argument("Rainbow course state requires matching live prepared/active original Source owners");
    }
    IsleRainbowActorSource& actors;
    IsleRainbowLifecycleSource& source;
};
} // namespace isle::rainbow

namespace isle::actor {
// Closed borrowed alternatives, copied by value into the native Actor Owner.
// The selected state outlives all Source/native/semantic/model/graphics users.
// Wrong-course access is an admission error, never a fabricated Beach fallback.
class CourseContext final {
public:
    explicit CourseContext(beach::ActorSceneState& state) noexcept : state_(&state) {}
    explicit CourseContext(tunnel::CourseState& state) noexcept : state_(&state) {}
    explicit CourseContext(volcano::CourseState& state) noexcept : state_(&state) {}
    explicit CourseContext(river::CourseState& state) noexcept : state_(&state) {}
    explicit CourseContext(cave::CourseState& state) noexcept : state_(&state) {}
    explicit CourseContext(valley::CourseState& state) noexcept : state_(&state) {}
    explicit CourseContext(rainbow::CourseState& state) noexcept : state_(&state) {}
    CourseContext(beach::ActorSceneState&&) = delete;
    CourseContext(tunnel::CourseState&&) = delete;
    CourseContext(volcano::CourseState&&) = delete;
    CourseContext(river::CourseState&&) = delete;
    CourseContext(cave::CourseState&&) = delete;
    CourseContext(valley::CourseState&&) = delete;
    CourseContext(rainbow::CourseState&&) = delete;
    std::int32_t course_id() const noexcept { return static_cast<std::int32_t>(state_.index()); }
    beach::ActorSceneState& beach() const {
        const auto value = std::get_if<beach::ActorSceneState*>(&state_);
        if (!value) throw std::invalid_argument("Actor course context is not Beach");
        return **value;
    }
    tunnel::CourseState& tunnel() const {
        const auto value = std::get_if<tunnel::CourseState*>(&state_);
        if (!value) throw std::invalid_argument("Actor course context is not Tunnel");
        return **value;
    }
    volcano::CourseState& volcano() const {
        const auto value = std::get_if<volcano::CourseState*>(&state_);
        if (!value) throw std::invalid_argument("Actor course context is not Volcano");
        return **value;
    }
    river::CourseState& river() const {
        const auto value = std::get_if<river::CourseState*>(&state_);
        if (!value) throw std::invalid_argument("Actor course context is not River");
        return **value;
    }
    cave::CourseState& cave() const {
        const auto value = std::get_if<cave::CourseState*>(&state_);
        if (!value) throw std::invalid_argument("Actor course context is not Cave");
        return **value;
    }
    valley::CourseState& valley() const {
        const auto value = std::get_if<valley::CourseState*>(&state_);
        if (!value) throw std::invalid_argument("Actor course context is not Valley");
        return **value;
    }
    rainbow::CourseState& rainbow() const {
        const auto value = std::get_if<rainbow::CourseState*>(&state_);
        if (!value) throw std::invalid_argument("Actor course context is not Rainbow");
        return **value;
    }
    void require_live() const {
        switch (course_id()) {
        case 0: beach().resources.require_live(); return;
        case 1: tunnel().require_live(); return;
        case 2: volcano().require_live(); return;
        case 3: river().require_live(); return;
        case 4: cave().require_live(); return;
        case 5: valley().require_live(); return;
        case 6: rainbow().require_live(); return;
        }
        throw std::invalid_argument("Actor course context has no selected course");
    }
private:
    std::variant<beach::ActorSceneState*, tunnel::CourseState*, volcano::CourseState*,
        river::CourseState*, cave::CourseState*, valley::CourseState*, rainbow::CourseState*> state_;
};
} // namespace isle::actor
#endif

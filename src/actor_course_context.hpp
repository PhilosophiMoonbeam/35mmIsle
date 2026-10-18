#ifndef ISLE_ACTOR_COURSE_CONTEXT_HPP
#define ISLE_ACTOR_COURSE_CONTEXT_HPP
#include "beach_actor_scene.hpp"
#include "rainbow_actor_source.h"
#include "rainbow_lifecycle_source.h"
#include <cstdint>
#include <stdexcept>
#include <variant>

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
    explicit CourseContext(rainbow::CourseState& state) noexcept : state_(&state) {}
    CourseContext(beach::ActorSceneState&&) = delete;
    CourseContext(rainbow::CourseState&&) = delete;
    std::int32_t course_id() const noexcept { return state_.index() == 0 ? 0 : 6; }
    beach::ActorSceneState& beach() const {
        const auto value = std::get_if<beach::ActorSceneState*>(&state_);
        if (!value) throw std::invalid_argument("Actor course context is not Beach");
        return **value;
    }
    rainbow::CourseState& rainbow() const {
        const auto value = std::get_if<rainbow::CourseState*>(&state_);
        if (!value) throw std::invalid_argument("Actor course context is not Rainbow");
        return **value;
    }
    void require_live() const {
        if (course_id() == 0) beach().resources.require_live();
        else rainbow().require_live();
    }
private:
    std::variant<beach::ActorSceneState*, rainbow::CourseState*> state_;
};
} // namespace isle::actor
#endif

#ifndef ISLE_BEACH_MEOWTH_HPP
#define ISLE_BEACH_MEOWTH_HPP
#include "actor_behavior_bindings.hpp"
namespace isle::beach {
std::span<const actor::BehaviorCallbackBinding> meowth_callbacks();
std::span<const std::uint32_t> meowth_graphs();
std::span<const std::uint32_t> meowth_roots();
}
#endif

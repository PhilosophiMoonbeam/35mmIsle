#ifndef ISLE_BEACH_SCYTHER_SPECIAL_HPP
#define ISLE_BEACH_SCYTHER_SPECIAL_HPP
#include "actor_behavior_bindings.hpp"
namespace isle::beach {
std::span<const actor::BehaviorCallbackBinding> scyther_special_callbacks();
std::span<const std::uint32_t> scyther_special_graphs();
std::span<const std::uint32_t> scyther_special_roots();
}
#endif

#ifndef ISLE_BEACH_SNORLAX_KANGASKHAN_HPP
#define ISLE_BEACH_SNORLAX_KANGASKHAN_HPP
#include "actor_behavior_bindings.hpp"
namespace isle::beach {
std::span<const actor::BehaviorCallbackBinding> snorlax_kangaskhan_callbacks();
std::span<const std::uint32_t> snorlax_kangaskhan_graphs();
std::span<const std::uint32_t> snorlax_kangaskhan_roots();
}
#endif

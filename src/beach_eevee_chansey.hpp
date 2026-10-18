#ifndef ISLE_BEACH_EEVEE_CHANSEY_HPP
#define ISLE_BEACH_EEVEE_CHANSEY_HPP
#include "actor_behavior_bindings.hpp"
namespace isle::beach {
std::span<const actor::BehaviorCallbackBinding> eevee_chansey_callbacks();
std::span<const std::uint32_t> eevee_chansey_graphs();
std::span<const std::uint32_t> eevee_chansey_roots();
}
#endif

#ifndef ISLE_BEACH_INITIAL_SPECIES_HPP
#define ISLE_BEACH_INITIAL_SPECIES_HPP
#include "actor_behavior_bindings.hpp"
namespace isle::beach {
std::span<const actor::BehaviorCallbackBinding> initial_species_callbacks();
std::span<const std::uint32_t> initial_species_graphs();
std::span<const std::uint32_t> initial_species_roots();
}
#endif

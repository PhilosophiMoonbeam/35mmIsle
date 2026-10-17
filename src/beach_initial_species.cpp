#include "beach_initial_species.hpp"
#include "beach_initial_species_source.h"
#include <array>
namespace isle::beach {
namespace {
constexpr std::uint32_t butterfly_initial[] = {0x802c7bb8, 0x802c7c7c};
constexpr std::uint32_t butterfly_wait[] = {0x802c7bb8};
constexpr std::uint32_t butterfly_hit[] = {0x802c7da0, 0x802c7dec, 0x802c7bb8};
constexpr std::uint32_t butterfly_graph[] = {0x802ccd60};
constexpr std::uint32_t doduo_initial[] = {0x802c8438};
constexpr std::uint32_t doduo_resume[] = {0x802c83d0};
constexpr std::uint32_t doduo_wait_graph[] = {0x802ccf44};
constexpr std::uint32_t doduo_run_graph[] = {0x802ccf14};
const actor::BehaviorCallbackBinding callbacks[] = {
    {0x802c7b10, func_beach_802C7B10, butterfly_initial, {}, {}},
    {0x802c7bb8, func_beach_802C7BB8, butterfly_wait, butterfly_graph, {}},
    {0x802c7c7c, func_beach_802C7C7C, {}, {}, {}},
    {0x802c7cf0, func_beach_802C7CF0, butterfly_hit, butterfly_graph, {}},
    {0x802c7da0, func_beach_802C7DA0, {}, {}, {}},
    {0x802c7dec, func_beach_802C7DEC, {}, {}, {}},
    {0x802c82b0, func_beach_802C82B0, doduo_initial, doduo_wait_graph, {}},
    {0x802c8354, func_beach_802C8354, doduo_resume, doduo_run_graph, {}},
    {0x802c83d0, func_beach_802C83D0, {}, doduo_run_graph, {}},
    {0x802c8438, func_beach_802C8438, {}, {}, {}},
    {0x802c84a4, func_beach_802C84A4, doduo_resume, {}, {}},
};
constexpr std::uint32_t graphs[] = {0x802ccd60, 0x802ccf14, 0x802ccf44};
}
std::span<const actor::BehaviorCallbackBinding> initial_species_callbacks() { return callbacks; }
std::span<const std::uint32_t> initial_species_graphs() { return graphs; }
}

#include "beach_scyther_special.hpp"
#include "beach_scyther_special_source.h"

namespace isle::beach {
namespace {
constexpr std::uint32_t scyther_initial[] = {0x802c8de8};
constexpr std::uint32_t scyther_start[] = {0x802c8e20, 0x802c8e90};
constexpr std::uint32_t scyther_wait[] = {0x802c8e90};
constexpr std::uint32_t scyther_wait_graph[] = {0x802cd1e4};
constexpr std::uint32_t scyther_jump[] = {0x802c907c, 0x802c90e0, 0x802c8f48};
constexpr std::uint32_t scyther_jump_graph[] = {0x802cd214};
constexpr std::uint32_t special_trigger_graph[] = {0x802cd858};
constexpr std::uint32_t special_choices[] = {0x802cd8b8};
constexpr std::uint32_t special_pidgey[] = {0x802ca0a0};
constexpr std::uint32_t special_scyther[] = {0x802ca0f4};
constexpr std::uint32_t special_ending[] = {0x802cbd04};
const actor::BehaviorCallbackBinding callbacks[] = {
    {0x802c8db0, func_beach_802C8DB0, scyther_initial, {}, {}},
    {0x802c8de8, func_beach_802C8DE8, scyther_start, {}, {}},
    {0x802c8e20, func_beach_802C8E20, {}, {}, {}},
    {0x802c8e90, func_beach_802C8E90, scyther_wait, scyther_wait_graph, {}},
    {0x802c8f48, func_beach_802C8F48, scyther_jump, scyther_jump_graph, {}},
    {0x802c907c, func_beach_802C907C, {}, {}, {}},
    {0x802c90e0, func_beach_802C90E0, {}, {}, {}},
    {0x802c914c, func_beach_802C914C, {}, {}, {}},
    {0x802ca0a0, func_beach_802CA0A0, {}, {}, {}},
    {0x802ca0f4, func_beach_802CA0F4, {}, {}, {}},
    {0x802ca148, func_beach_802CA148, {}, special_trigger_graph, {}},
    {0x802ca198, func_beach_802CA198, {}, {}, special_choices},
    {0x802ca1bc, func_beach_802CA1BC, special_pidgey, {}, {}},
    {0x802ca23c, func_beach_802CA23C, special_scyther, {}, {}},
    {0x802cbce0, func_beach_802CBCE0, special_ending, {}, {}},
    {0x802cbd04, func_beach_802CBD04, {}, {}, {}},
};
constexpr std::uint32_t graphs[] = {0x802cd1e4, 0x802cd214, 0x802cd858};
constexpr std::uint32_t roots[] = {0x802c8db0, 0x802ca148, 0x802cbce0};
}
std::span<const actor::BehaviorCallbackBinding> scyther_special_callbacks() { return callbacks; }
std::span<const std::uint32_t> scyther_special_graphs() { return graphs; }
std::span<const std::uint32_t> scyther_special_roots() { return roots; }
}

#include "beach_meowth.hpp"
#include "beach_meowth_source.h"
namespace isle::beach {
namespace {
constexpr std::uint32_t entry_802c5dac_callbacks[] = {0x802c6658, 0x802c6c10, 0x802c6d20, 0x802c6fd4, 0x802c5e88};
constexpr std::uint32_t entry_802c5e88_graphs[] = {0x802cc40c};
constexpr std::uint32_t entry_802c5e88_weighted[] = {0x802cc4ac};
constexpr std::uint32_t entry_802c5ef8_callbacks[] = {0x802c5f9c, 0x802c5e88};
constexpr std::uint32_t entry_802c5ef8_graphs[] = {0x802cc40c};
constexpr std::uint32_t entry_802c5ef8_weighted[] = {0x802cc4ac};
constexpr std::uint32_t entry_802c5ff8_callbacks[] = {0x802c619c};
constexpr std::uint32_t entry_802c5ff8_graphs[] = {0x802cc4c4};
constexpr std::uint32_t entry_802c5ff8_weighted[] = {0x802cc4ac};
constexpr std::uint32_t entry_802c60e8_callbacks[] = {0x802c619c};
constexpr std::uint32_t entry_802c60e8_graphs[] = {0x802cc4f4};
constexpr std::uint32_t entry_802c60e8_weighted[] = {0x802cc4ac};
constexpr std::uint32_t entry_802c6210_callbacks[] = {0x802c6288};
constexpr std::uint32_t entry_802c6210_graphs[] = {0x802cc524};
constexpr std::uint32_t entry_802c6210_weighted[] = {0x802cc4ac};
constexpr std::uint32_t entry_802c62e4_callbacks[] = {0x802c63f4, 0x802c5e88};
constexpr std::uint32_t entry_802c62e4_graphs[] = {0x802cc564};
constexpr std::uint32_t entry_802c62e4_weighted[] = {0x802cc4ac};
constexpr std::uint32_t entry_802c6478_callbacks[] = {0x802c62e4};
constexpr std::uint32_t entry_802c649c_callbacks[] = {0x802c6514};
constexpr std::uint32_t entry_802c649c_graphs[] = {0x802cc594};
constexpr std::uint32_t entry_802c649c_weighted[] = {0x802cc4ac};
constexpr std::uint32_t entry_802c6580_callbacks[] = {0x802c660c, 0x802c5c60};
constexpr std::uint32_t entry_802c6580_graphs[] = {0x802cc5c4};
constexpr std::uint32_t entry_802c6580_weighted[] = {0x802cc4ac};
constexpr std::uint32_t entry_802c6658_callbacks[] = {0x802c667c};
constexpr std::uint32_t entry_802c667c_graphs[] = {0x802cc644};
constexpr std::uint32_t entry_802c6700_callbacks[] = {0x802c660c, 0x802c6aa0};
constexpr std::uint32_t entry_802c67e0_callbacks[] = {0x802c667c};
constexpr std::uint32_t entry_802c6854_callbacks[] = {0x802c68d8, 0x802c667c};
constexpr std::uint32_t entry_802c6854_graphs[] = {0x802cc6b4};
constexpr std::uint32_t entry_802c6944_callbacks[] = {0x802c660c, 0x802c5c60, 0x802c69d4};
constexpr std::uint32_t entry_802c6944_graphs[] = {0x802cc6e4};
constexpr std::uint32_t entry_802c69d4_graphs[] = {0x802cc714};
constexpr std::uint32_t entry_802c6a3c_callbacks[] = {0x802c6aa0};
constexpr std::uint32_t entry_802c6aa0_callbacks[] = {0x802c69d4};
constexpr std::uint32_t entry_802c6aa0_graphs[] = {0x802cc784};
constexpr std::uint32_t entry_802c6b28_callbacks[] = {0x802c69d4};
constexpr std::uint32_t entry_802c6b8c_callbacks[] = {0x802c68d8, 0x802c69d4};
constexpr std::uint32_t entry_802c6b8c_graphs[] = {0x802cc7b4};
constexpr std::uint32_t entry_802c6c10_graphs[] = {0x802cc7e4};
constexpr std::uint32_t entry_802c6c60_graphs[] = {0x802cc804};
constexpr std::uint32_t entry_802c6cc4_callbacks[] = {0x802c6580};
constexpr std::uint32_t entry_802c6cc4_graphs[] = {0x802cc824};
constexpr std::uint32_t entry_802c6d20_callbacks[] = {0x802c6d44};
constexpr std::uint32_t entry_802c6d44_graphs[] = {0x802cc854};
constexpr std::uint32_t entry_802c6d9c_callbacks[] = {0x802c6dd4, 0x802c6e40};
constexpr std::uint32_t entry_802c6e40_graphs[] = {0x802cc874};
constexpr std::uint32_t entry_802c6ec0_callbacks[] = {0x802c6e40};
constexpr std::uint32_t entry_802c6ec0_graphs[] = {0x802cc8a4};
constexpr std::uint32_t entry_802c6f6c_callbacks[] = {0x802c6e40};
constexpr std::uint32_t entry_802c6f6c_graphs[] = {0x802cc8d4};
constexpr std::uint32_t entry_802c6fd4_callbacks[] = {0x802c6ff8};
constexpr std::uint32_t entry_802c6ff8_graphs[] = {0x802cc904};
constexpr std::uint32_t entry_802c70c0_callbacks[] = {0x802c660c};
constexpr std::uint32_t entry_802c70c0_graphs[] = {0x802cc934};
constexpr std::uint32_t entry_802c7130_callbacks[] = {0x802c70c0};
constexpr std::uint32_t entry_802c7130_graphs[] = {0x802cc934};
const actor::BehaviorCallbackBinding callbacks[] = {
    {0x802c5c60, func_beach_802C5C60, {}, {}, {}},
    {0x802c5dac, func_beach_802C5DAC, entry_802c5dac_callbacks, {}, {}},
    {0x802c5e88, func_beach_802C5E88, {}, entry_802c5e88_graphs, entry_802c5e88_weighted},
    {0x802c5ef8, func_beach_802C5EF8, entry_802c5ef8_callbacks, entry_802c5ef8_graphs, entry_802c5ef8_weighted},
    {0x802c5f9c, func_beach_802C5F9C, {}, {}, {}},
    {0x802c5ff8, func_beach_802C5FF8, entry_802c5ff8_callbacks, entry_802c5ff8_graphs, entry_802c5ff8_weighted},
    {0x802c60e8, func_beach_802C60E8, entry_802c60e8_callbacks, entry_802c60e8_graphs, entry_802c60e8_weighted},
    {0x802c619c, func_beach_802C619C, {}, {}, {}},
    {0x802c6210, func_beach_802C6210, entry_802c6210_callbacks, entry_802c6210_graphs, entry_802c6210_weighted},
    {0x802c6288, func_beach_802C6288, {}, {}, {}},
    {0x802c62e4, func_beach_802C62E4, entry_802c62e4_callbacks, entry_802c62e4_graphs, entry_802c62e4_weighted},
    {0x802c63f4, func_beach_802C63F4, {}, {}, {}},
    {0x802c6478, func_beach_802C6478, entry_802c6478_callbacks, {}, {}},
    {0x802c649c, func_beach_802C649C, entry_802c649c_callbacks, entry_802c649c_graphs, entry_802c649c_weighted},
    {0x802c6514, func_beach_802C6514, {}, {}, {}},
    {0x802c6580, func_beach_802C6580, entry_802c6580_callbacks, entry_802c6580_graphs, entry_802c6580_weighted},
    {0x802c660c, func_beach_802C660C, {}, {}, {}},
    {0x802c6658, func_beach_802C6658, entry_802c6658_callbacks, {}, {}},
    {0x802c667c, func_beach_802C667C, {}, entry_802c667c_graphs, {}},
    {0x802c6700, func_beach_802C6700, entry_802c6700_callbacks, {}, {}},
    {0x802c67e0, func_beach_802C67E0, entry_802c67e0_callbacks, {}, {}},
    {0x802c6854, func_beach_802C6854, entry_802c6854_callbacks, entry_802c6854_graphs, {}},
    {0x802c68d8, func_beach_802C68D8, {}, {}, {}},
    {0x802c6944, func_beach_802C6944, entry_802c6944_callbacks, entry_802c6944_graphs, {}},
    {0x802c69d4, func_beach_802C69D4, {}, entry_802c69d4_graphs, {}},
    {0x802c6a3c, func_beach_802C6A3C, entry_802c6a3c_callbacks, {}, {}},
    {0x802c6aa0, func_beach_802C6AA0, entry_802c6aa0_callbacks, entry_802c6aa0_graphs, {}},
    {0x802c6b28, func_beach_802C6B28, entry_802c6b28_callbacks, {}, {}},
    {0x802c6b8c, func_beach_802C6B8C, entry_802c6b8c_callbacks, entry_802c6b8c_graphs, {}},
    {0x802c6c10, func_beach_802C6C10, {}, entry_802c6c10_graphs, {}},
    {0x802c6c60, func_beach_802C6C60, {}, entry_802c6c60_graphs, {}},
    {0x802c6cc4, func_beach_802C6CC4, entry_802c6cc4_callbacks, entry_802c6cc4_graphs, {}},
    {0x802c6d20, func_beach_802C6D20, entry_802c6d20_callbacks, {}, {}},
    {0x802c6d44, func_beach_802C6D44, {}, entry_802c6d44_graphs, {}},
    {0x802c6d9c, func_beach_802C6D9C, entry_802c6d9c_callbacks, {}, {}},
    {0x802c6dd4, func_beach_802C6DD4, {}, {}, {}},
    {0x802c6e40, func_beach_802C6E40, {}, entry_802c6e40_graphs, {}},
    {0x802c6ec0, func_beach_802C6EC0, entry_802c6ec0_callbacks, entry_802c6ec0_graphs, {}},
    {0x802c6f6c, func_beach_802C6F6C, entry_802c6f6c_callbacks, entry_802c6f6c_graphs, {}},
    {0x802c6fd4, func_beach_802C6FD4, entry_802c6fd4_callbacks, {}, {}},
    {0x802c6ff8, func_beach_802C6FF8, {}, entry_802c6ff8_graphs, {}},
    {0x802c7050, func_beach_802C7050, {}, {}, {}},
    {0x802c70c0, func_beach_802C70C0, entry_802c70c0_callbacks, entry_802c70c0_graphs, {}},
    {0x802c7130, func_beach_802C7130, entry_802c7130_callbacks, entry_802c7130_graphs, {}},
};
constexpr std::uint32_t graphs[] = {0x802cc40c, 0x802cc4c4, 0x802cc4f4, 0x802cc524, 0x802cc564, 0x802cc594, 0x802cc5c4, 0x802cc644, 0x802cc6b4, 0x802cc6e4, 0x802cc714, 0x802cc784, 0x802cc7b4, 0x802cc7e4, 0x802cc804, 0x802cc824, 0x802cc854, 0x802cc874, 0x802cc8a4, 0x802cc8d4, 0x802cc904, 0x802cc934};
constexpr std::uint32_t roots[] = {0x802c5dac};
}
std::span<const actor::BehaviorCallbackBinding> meowth_callbacks() { return callbacks; }
std::span<const std::uint32_t> meowth_graphs() { return graphs; }
std::span<const std::uint32_t> meowth_roots() { return roots; }
}

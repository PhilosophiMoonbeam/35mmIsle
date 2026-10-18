#include "beach_snorlax_kangaskhan.hpp"
#include "beach_snorlax_kangaskhan_source.h"

namespace isle::beach {
namespace {
constexpr std::uint32_t entry_802c56c8_callbacks[] = {0x802c5620, 0x802c5700};
constexpr std::uint32_t entry_802c5700_graphs[] = {0x802cc1d4};
constexpr std::uint32_t entry_802c5700_weighted[] = {0x802cc2c4};
constexpr std::uint32_t entry_802c5768_graphs[] = {0x802cc1d4};
constexpr std::uint32_t entry_802c5768_weighted[] = {0x802cc2c4};
constexpr std::uint32_t entry_802c5828_callbacks[] = {0x802c5700};
constexpr std::uint32_t entry_802c5828_graphs[] = {0x802cc1d4};
constexpr std::uint32_t entry_802c5884_callbacks[] = {0x802c5768};
constexpr std::uint32_t entry_802c58b8_callbacks[] = {0x802c5828};
constexpr std::uint32_t entry_802c58ec_callbacks[] = {0x802c594c};
constexpr std::uint32_t entry_802c594c_callbacks[] = {0x802c5b68, 0x802c5bd4};
constexpr std::uint32_t entry_802c594c_graphs[] = {0x802cc234};
constexpr std::uint32_t entry_802c59c0_callbacks[] = {0x802c5a20};
constexpr std::uint32_t entry_802c5a20_callbacks[] = {0x802c5b68, 0x802c5bd4};
constexpr std::uint32_t entry_802c5a20_graphs[] = {0x802cc264};
constexpr std::uint32_t entry_802c5a94_callbacks[] = {0x802c5af4};
constexpr std::uint32_t entry_802c5af4_callbacks[] = {0x802c5b68, 0x802c5bd4};
constexpr std::uint32_t entry_802c5af4_graphs[] = {0x802cc294};
constexpr std::uint32_t entry_802c5bd4_callbacks[] = {0x802c5700};
constexpr std::uint32_t entry_802c9ac0_callbacks[] = {0x802c9ae4};
constexpr std::uint32_t entry_802c9ae4_graphs[] = {0x802cd694};
constexpr std::uint32_t entry_802c9ae4_weighted[] = {0x802cd774};
constexpr std::uint32_t entry_802c9b40_graphs[] = {0x802cd694};
constexpr std::uint32_t entry_802c9b40_weighted[] = {0x802cd774};
constexpr std::uint32_t entry_802c9b9c_graphs[] = {0x802cd694};
constexpr std::uint32_t entry_802c9b9c_weighted[] = {0x802cd774};
constexpr std::uint32_t entry_802c9bf8_callbacks[] = {0x802c9c88, 0x802c9f9c};
constexpr std::uint32_t entry_802c9cf8_graphs[] = {0x802cd634};
constexpr std::uint32_t entry_802c9cf8_weighted[] = {0x802cd764};
constexpr std::uint32_t entry_802c9dcc_callbacks[] = {0x802c9e70, 0x802c9cf8};
constexpr std::uint32_t entry_802c9dcc_graphs[] = {0x802cd6c4};
constexpr std::uint32_t entry_802c9ebc_callbacks[] = {0x802c9f30, 0x802c9cf8};
constexpr std::uint32_t entry_802c9ebc_graphs[] = {0x802cd6f4};
constexpr std::uint32_t entry_802c9f9c_callbacks[] = {0x802ca010};
constexpr std::uint32_t entry_802c9f9c_graphs[] = {0x802cd6c4};
constexpr std::uint32_t entry_802c9f9c_weighted[] = {0x802cd764};
const actor::BehaviorCallbackBinding callbacks[] = {
    {0x802c5620, func_beach_802C5620, {}, {}, {}},
    {0x802c56c8, func_beach_802C56C8, entry_802c56c8_callbacks, {}, {}},
    {0x802c5700, func_beach_802C5700, {}, entry_802c5700_graphs, entry_802c5700_weighted},
    {0x802c5768, func_beach_802C5768, {}, entry_802c5768_graphs, entry_802c5768_weighted},
    {0x802c5828, func_beach_802C5828, entry_802c5828_callbacks, entry_802c5828_graphs, {}},
    {0x802c5884, func_beach_802C5884, entry_802c5884_callbacks, {}, {}},
    {0x802c58b8, func_beach_802C58B8, entry_802c58b8_callbacks, {}, {}},
    {0x802c58ec, func_beach_802C58EC, entry_802c58ec_callbacks, {}, {}},
    {0x802c594c, func_beach_802C594C, entry_802c594c_callbacks, entry_802c594c_graphs, {}},
    {0x802c59c0, func_beach_802C59C0, entry_802c59c0_callbacks, {}, {}},
    {0x802c5a20, func_beach_802C5A20, entry_802c5a20_callbacks, entry_802c5a20_graphs, {}},
    {0x802c5a94, func_beach_802C5A94, entry_802c5a94_callbacks, {}, {}},
    {0x802c5af4, func_beach_802C5AF4, entry_802c5af4_callbacks, entry_802c5af4_graphs, {}},
    {0x802c5b68, func_beach_802C5B68, {}, {}, {}},
    {0x802c5bd4, func_beach_802C5BD4, entry_802c5bd4_callbacks, {}, {}},
    {0x802c9ac0, func_beach_802C9AC0, entry_802c9ac0_callbacks, {}, {}},
    {0x802c9ae4, func_beach_802C9AE4, {}, entry_802c9ae4_graphs, entry_802c9ae4_weighted},
    {0x802c9b40, func_beach_802C9B40, {}, entry_802c9b40_graphs, entry_802c9b40_weighted},
    {0x802c9b9c, func_beach_802C9B9C, {}, entry_802c9b9c_graphs, entry_802c9b9c_weighted},
    {0x802c9bf8, func_beach_802C9BF8, entry_802c9bf8_callbacks, {}, {}},
    {0x802c9c88, func_beach_802C9C88, {}, {}, {}},
    {0x802c9cf8, func_beach_802C9CF8, {}, entry_802c9cf8_graphs, entry_802c9cf8_weighted},
    {0x802c9d70, func_beach_802C9D70, {}, {}, {}},
    {0x802c9dcc, func_beach_802C9DCC, entry_802c9dcc_callbacks, entry_802c9dcc_graphs, {}},
    {0x802c9e70, func_beach_802C9E70, {}, {}, {}},
    {0x802c9ebc, func_beach_802C9EBC, entry_802c9ebc_callbacks, entry_802c9ebc_graphs, {}},
    {0x802c9f30, func_beach_802C9F30, {}, {}, {}},
    {0x802c9f9c, func_beach_802C9F9C, entry_802c9f9c_callbacks, entry_802c9f9c_graphs, entry_802c9f9c_weighted},
    {0x802ca010, func_beach_802CA010, {}, {}, {}},
};
constexpr std::uint32_t graphs[] = {0x802cc1d4, 0x802cc234, 0x802cc264, 0x802cc294, 0x802cd694, 0x802cd634, 0x802cd6c4, 0x802cd6f4};
constexpr std::uint32_t roots[] = {0x802c56c8, 0x802c9ac0};
}
std::span<const actor::BehaviorCallbackBinding> snorlax_kangaskhan_callbacks() { return callbacks; }
std::span<const std::uint32_t> snorlax_kangaskhan_graphs() { return graphs; }
std::span<const std::uint32_t> snorlax_kangaskhan_roots() { return roots; }
}

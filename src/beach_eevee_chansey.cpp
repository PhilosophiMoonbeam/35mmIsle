#include "beach_eevee_chansey.hpp"
#include "beach_eevee_chansey_source.h"

namespace isle::beach {
namespace {
constexpr std::uint32_t entry_802c72e0_callbacks[] = {0x802c78b0, 0x802c737c};
constexpr std::uint32_t entry_802c737c_graphs[] = {0x802cca10};
constexpr std::uint32_t entry_802c737c_weighted[] = {0x802ccaa0};
constexpr std::uint32_t entry_802c73ec_callbacks[] = {0x802c7490, 0x802c737c};
constexpr std::uint32_t entry_802c73ec_graphs[] = {0x802cca10};
constexpr std::uint32_t entry_802c73ec_weighted[] = {0x802ccaa0};
constexpr std::uint32_t entry_802c74ec_callbacks[] = {0x802c737c};
constexpr std::uint32_t entry_802c74ec_graphs[] = {0x802ccab8};
constexpr std::uint32_t entry_802c755c_callbacks[] = {0x802c75d0, 0x802c737c};
constexpr std::uint32_t entry_802c755c_graphs[] = {0x802ccae8};
constexpr std::uint32_t entry_802c762c_callbacks[] = {0x802c76c4, 0x802c737c, 0x802c7748};
constexpr std::uint32_t entry_802c762c_graphs[] = {0x802ccb28};
constexpr std::uint32_t entry_802c7748_callbacks[] = {0x802c7748};
constexpr std::uint32_t entry_802c7748_graphs[] = {0x802ccba8};
constexpr std::uint32_t entry_802c7748_weighted[] = {0x802ccaa0};
constexpr std::uint32_t entry_802c77cc_callbacks[] = {0x802c762c};
constexpr std::uint32_t entry_802c77f0_callbacks[] = {0x802c7864, 0x802c737c};
constexpr std::uint32_t entry_802c77f0_graphs[] = {0x802ccbf8};
constexpr std::uint32_t entry_802c78b0_callbacks[] = {0x802c796c, 0x802c78e8};
constexpr std::uint32_t entry_802c78e8_graphs[] = {0x802ccc28};
constexpr std::uint32_t entry_802c79dc_callbacks[] = {0x802c78e8};
constexpr std::uint32_t entry_802c79dc_graphs[] = {0x802ccc68};
constexpr std::uint32_t entry_802c7a60_callbacks[] = {0x802c737c};
constexpr std::uint32_t entry_802c7a60_graphs[] = {0x802ccca8};
constexpr std::uint32_t entry_802c9240_callbacks[] = {0x802c9274};
constexpr std::uint32_t entry_802c9274_callbacks[] = {0x802c9348, 0x802c92bc};
constexpr std::uint32_t entry_802c92bc_callbacks[] = {0x802c92bc};
constexpr std::uint32_t entry_802c92bc_graphs[] = {0x802cd3f8};
constexpr std::uint32_t entry_802c93b8_graphs[] = {0x802cd438};
constexpr std::uint32_t entry_802c943c_callbacks[] = {0x802c94f0, 0x802c9580};
constexpr std::uint32_t entry_802c9580_graphs[] = {0x802cd35c};
constexpr std::uint32_t entry_802c9580_weighted[] = {0x802cd3dc};
constexpr std::uint32_t entry_802c95f0_callbacks[] = {0x802c9694, 0x802c9580};
constexpr std::uint32_t entry_802c95f0_graphs[] = {0x802cd35c};
constexpr std::uint32_t entry_802c95f0_weighted[] = {0x802cd3dc};
constexpr std::uint32_t entry_802c96f0_callbacks[] = {0x802c9580};
constexpr std::uint32_t entry_802c96f0_graphs[] = {0x802cd478};
constexpr std::uint32_t entry_802c9760_callbacks[] = {0x802c97d4, 0x802c9580};
constexpr std::uint32_t entry_802c9760_graphs[] = {0x802cd4a8};
constexpr std::uint32_t entry_802c9830_callbacks[] = {0x802c98f4, 0x802c9580};
constexpr std::uint32_t entry_802c9830_graphs[] = {0x802cd4e8};
constexpr std::uint32_t entry_802c9830_weighted[] = {0x802cd3dc};
constexpr std::uint32_t entry_802c9978_callbacks[] = {0x802c9830};
constexpr std::uint32_t entry_802c999c_callbacks[] = {0x802c9a10, 0x802c9580};
constexpr std::uint32_t entry_802c999c_graphs[] = {0x802cd518};
const actor::BehaviorCallbackBinding callbacks[] = {
    {0x802c72e0, func_beach_802C72E0, entry_802c72e0_callbacks, {}, {}},
    {0x802c737c, func_beach_802C737C, {}, entry_802c737c_graphs, entry_802c737c_weighted},
    {0x802c73ec, func_beach_802C73EC, entry_802c73ec_callbacks, entry_802c73ec_graphs, entry_802c73ec_weighted},
    {0x802c7490, func_beach_802C7490, {}, {}, {}},
    {0x802c74ec, func_beach_802C74EC, entry_802c74ec_callbacks, entry_802c74ec_graphs, {}},
    {0x802c755c, func_beach_802C755C, entry_802c755c_callbacks, entry_802c755c_graphs, {}},
    {0x802c75d0, func_beach_802C75D0, {}, {}, {}},
    {0x802c762c, func_beach_802C762C, entry_802c762c_callbacks, entry_802c762c_graphs, {}},
    {0x802c76c4, func_beach_802C76C4, {}, {}, {}},
    {0x802c7748, func_beach_802C7748, entry_802c7748_callbacks, entry_802c7748_graphs, entry_802c7748_weighted},
    {0x802c77cc, func_beach_802C77CC, entry_802c77cc_callbacks, {}, {}},
    {0x802c77f0, func_beach_802C77F0, entry_802c77f0_callbacks, entry_802c77f0_graphs, {}},
    {0x802c7864, func_beach_802C7864, {}, {}, {}},
    {0x802c78b0, func_beach_802C78B0, entry_802c78b0_callbacks, {}, {}},
    {0x802c78e8, func_beach_802C78E8, {}, entry_802c78e8_graphs, {}},
    {0x802c796c, func_beach_802C796C, {}, {}, {}},
    {0x802c79dc, func_beach_802C79DC, entry_802c79dc_callbacks, entry_802c79dc_graphs, {}},
    {0x802c7a60, func_beach_802C7A60, entry_802c7a60_callbacks, entry_802c7a60_graphs, {}},
    {0x802c9240, func_beach_802C9240, entry_802c9240_callbacks, {}, {}},
    {0x802c9274, func_beach_802C9274, entry_802c9274_callbacks, {}, {}},
    {0x802c92bc, func_beach_802C92BC, entry_802c92bc_callbacks, entry_802c92bc_graphs, {}},
    {0x802c9348, func_beach_802C9348, {}, {}, {}},
    {0x802c93b8, func_beach_802C93B8, {}, entry_802c93b8_graphs, {}},
    {0x802c943c, func_beach_802C943C, entry_802c943c_callbacks, {}, {}},
    {0x802c94f0, func_beach_802C94F0, {}, {}, {}},
    {0x802c9580, func_beach_802C9580, {}, entry_802c9580_graphs, entry_802c9580_weighted},
    {0x802c95f0, func_beach_802C95F0, entry_802c95f0_callbacks, entry_802c95f0_graphs, entry_802c95f0_weighted},
    {0x802c9694, func_beach_802C9694, {}, {}, {}},
    {0x802c96f0, func_beach_802C96F0, entry_802c96f0_callbacks, entry_802c96f0_graphs, {}},
    {0x802c9760, func_beach_802C9760, entry_802c9760_callbacks, entry_802c9760_graphs, {}},
    {0x802c97d4, func_beach_802C97D4, {}, {}, {}},
    {0x802c9830, func_beach_802C9830, entry_802c9830_callbacks, entry_802c9830_graphs, entry_802c9830_weighted},
    {0x802c98f4, func_beach_802C98F4, {}, {}, {}},
    {0x802c9978, func_beach_802C9978, entry_802c9978_callbacks, {}, {}},
    {0x802c999c, func_beach_802C999C, entry_802c999c_callbacks, entry_802c999c_graphs, {}},
    {0x802c9a10, func_beach_802C9A10, {}, {}, {}},
};
constexpr std::uint32_t graphs[] = {0x802cca10, 0x802ccab8, 0x802ccae8, 0x802ccb28, 0x802ccba8, 0x802ccbf8, 0x802ccc28, 0x802ccc68, 0x802ccca8, 0x802cd35c, 0x802cd3f8, 0x802cd438, 0x802cd478, 0x802cd4a8, 0x802cd4e8, 0x802cd518};
constexpr std::uint32_t roots[] = {0x802c72e0, 0x802c9240};
}
std::span<const actor::BehaviorCallbackBinding> eevee_chansey_callbacks() { return callbacks; }
std::span<const std::uint32_t> eevee_chansey_graphs() { return graphs; }
std::span<const std::uint32_t> eevee_chansey_roots() { return roots; }
}

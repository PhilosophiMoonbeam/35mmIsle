#include "rainbow_actor.hpp"

namespace isle::rainbow {
namespace {
constexpr std::uint32_t d_803466c0_callbacks[] = {0x80348938};
constexpr std::uint32_t d_803466c0_child_factories[] = {0x80348938};
constexpr std::uint32_t d_803467a4_callbacks[] = {0x80349eb8};
constexpr std::uint32_t d_803467a4_child_factories[] = {0x80349eb8};
constexpr std::uint32_t d_80346898_callbacks[] = {0x8034a4e8};
constexpr std::uint32_t d_80346898_child_factories[] = {0x8034a4e8};
constexpr std::uint32_t d_80346fa0_callbacks[] = {0x80349084};
constexpr std::uint32_t d_803470cc_callbacks[] = {0x803467a4, 0x80346fa0, 0x80347188, 0x803471d4};
constexpr std::uint32_t d_803471d4_callbacks[] = {0x80347334, 0x80347398};
constexpr std::uint32_t d_803471d4_graphs[] = {0x8034ad50, 0x8034ae10};
constexpr std::uint32_t d_803471d4_headers[] = {0x8034acd8};
constexpr std::uint32_t d_80347334_graphs[] = {0x8034ad50};
constexpr std::uint32_t d_80347334_weighted_states[] = {0x8034ae30};
constexpr std::uint32_t d_80347334_headers[] = {0x8034acb0};
constexpr std::uint32_t d_80347574_graphs[] = {0x8034ad50};
constexpr std::uint32_t d_80347574_weighted_states[] = {0x8034ae30};
constexpr std::uint32_t d_80347574_headers[] = {0x8034acc4};
constexpr std::uint32_t d_803475d0_callbacks[] = {0x803471d4, 0x80347724, 0x80347cc8};
constexpr std::uint32_t d_803475d0_graphs[] = {0x8034ae10};
constexpr std::uint32_t d_803475d0_headers[] = {0x8034acec};
constexpr std::uint32_t d_80347724_callbacks[] = {0x80347900, 0x80347940, 0x8034799c};
constexpr std::uint32_t d_80347724_graphs[] = {0x8034ad90, 0x8034ae10};
constexpr std::uint32_t d_80347724_headers[] = {0x8034acd8};
constexpr std::uint32_t d_80347900_callbacks[] = {0x80347724};
constexpr std::uint32_t d_80347940_callbacks[] = {0x80347900};
constexpr std::uint32_t d_80347940_graphs[] = {0x8034ad90};
constexpr std::uint32_t d_80347940_headers[] = {0x8034acb0};
constexpr std::uint32_t d_80347b88_callbacks[] = {0x80347724, 0x80347cc8, 0x80347e0c};
constexpr std::uint32_t d_80347b88_graphs[] = {0x8034ae10};
constexpr std::uint32_t d_80347b88_headers[] = {0x8034acec};
constexpr std::uint32_t d_80347e0c_callbacks[] = {0x80347e44, 0x80348540};
constexpr std::uint32_t d_80347e44_callbacks[] = {0x8034800c, 0x803480c0};
constexpr std::uint32_t d_80347e44_graphs[] = {0x8034add0, 0x8034ae10};
constexpr std::uint32_t d_80347e44_headers[] = {0x8034ad28};
constexpr std::uint32_t d_8034800c_callbacks[] = {0x80347724};
constexpr std::uint32_t d_8034800c_graphs[] = {0x8034add0};
constexpr std::uint32_t d_8034800c_headers[] = {0x8034ad00, 0x8034ad14};
constexpr std::uint32_t d_80348208_callbacks[] = {0x80347e44, 0x803482ec};
constexpr std::uint32_t d_80348208_graphs[] = {0x8034add0};
constexpr std::uint32_t d_80348208_headers[] = {0x8034ad3c};
constexpr std::uint32_t d_80348540_callbacks[] = {0x803484f0};
constexpr std::uint32_t d_8034877c_callbacks[] = {0x803487cc, 0x80348850};
constexpr std::uint32_t d_803487cc_headers[] = {0x8034ad3c};
constexpr std::uint32_t d_80348970_callbacks[] = {0x80348994};
constexpr std::uint32_t d_80348994_callbacks[] = {0x80348b34};
constexpr std::uint32_t d_80348994_graphs[] = {0x8034af3c};
constexpr std::uint32_t d_80348994_headers[] = {0x8034aed0};
constexpr std::uint32_t d_80348a04_callbacks[] = {0x80348dd4};
constexpr std::uint32_t d_80348a04_headers[] = {0x8034aee4, 0x8034aef8, 0x8034af0c};
constexpr std::uint32_t d_80348dd4_callbacks[] = {0x80348bdc, 0x80348c08, 0x80348d48};
constexpr std::uint32_t d_80349084_callbacks[] = {0x80348ff0};
constexpr std::uint32_t d_80349084_child_factories[] = {0x80348fb8};
constexpr std::uint32_t d_80349180_callbacks[] = {0x803491d4, 0x8034930c, 0x80349e28};
constexpr std::uint32_t d_8034930c_graphs[] = {0x8034b188};
constexpr std::uint32_t d_8034935c_callbacks[] = {0x803493f4};
constexpr std::uint32_t d_8034935c_graphs[] = {0x8034b1a8};
constexpr std::uint32_t d_8034935c_headers[] = {0x8034b0c0, 0x8034b0fc};
constexpr std::uint32_t d_803493f4_graphs[] = {0x8034b1a8};
constexpr std::uint32_t d_803493f4_headers[] = {0x8034b098, 0x8034b0ac};
constexpr std::uint32_t d_80349480_graphs[] = {0x8034b208};
constexpr std::uint32_t d_80349480_headers[] = {0x8034b0ac};
constexpr std::uint32_t d_80349514_graphs[] = {0x8034b238};
constexpr std::uint32_t d_80349514_headers[] = {0x8034b0fc};
constexpr std::uint32_t d_8034956c_graphs[] = {0x8034b268};
constexpr std::uint32_t d_8034956c_headers[] = {0x8034b0ac};
constexpr std::uint32_t d_803495fc_graphs[] = {0x8034b298};
constexpr std::uint32_t d_803495fc_headers[] = {0x8034b0fc};
constexpr std::uint32_t d_80349654_callbacks[] = {0x80349714};
constexpr std::uint32_t d_80349654_headers[] = {0x8034b0e8};
constexpr std::uint32_t d_803496bc_graphs[] = {0x8034b2c8};
constexpr std::uint32_t d_803496bc_headers[] = {0x8034b0fc};
constexpr std::uint32_t d_80349714_callbacks[] = {0x803491d4};
constexpr std::uint32_t d_80349714_graphs[] = {0x8034b328};
constexpr std::uint32_t d_8034977c_callbacks[] = {0x80349814};
constexpr std::uint32_t d_8034977c_graphs[] = {0x8034b348};
constexpr std::uint32_t d_8034977c_headers[] = {0x8034b138, 0x8034b174};
constexpr std::uint32_t d_80349814_graphs[] = {0x8034b348};
constexpr std::uint32_t d_80349814_headers[] = {0x8034b110, 0x8034b124};
constexpr std::uint32_t d_803498a0_graphs[] = {0x8034b3b8};
constexpr std::uint32_t d_803498a0_headers[] = {0x8034b124};
constexpr std::uint32_t d_80349934_graphs[] = {0x8034b3e8};
constexpr std::uint32_t d_80349934_headers[] = {0x8034b174};
constexpr std::uint32_t d_8034998c_graphs[] = {0x8034b418};
constexpr std::uint32_t d_8034998c_headers[] = {0x8034b124};
constexpr std::uint32_t d_80349a1c_graphs[] = {0x8034b448};
constexpr std::uint32_t d_80349a1c_headers[] = {0x8034b174};
constexpr std::uint32_t d_80349a74_callbacks[] = {0x80349b34};
constexpr std::uint32_t d_80349a74_headers[] = {0x8034b160};
constexpr std::uint32_t d_80349adc_graphs[] = {0x8034b478};
constexpr std::uint32_t d_80349adc_headers[] = {0x8034b174};
constexpr std::uint32_t d_80349b34_graphs[] = {0x8034b4e8};
constexpr std::uint32_t d_80349b98_callbacks[] = {0x80349c18, 0x80349d40};
constexpr std::uint32_t d_80349b98_graphs[] = {0x8034b4e8};
constexpr std::uint32_t d_80349b98_headers[] = {0x8034b174};
constexpr std::uint32_t d_80349d40_callbacks[] = {0x803491d4};
constexpr std::uint32_t d_80349d40_graphs[] = {0x8034b4e8};
constexpr std::uint32_t d_80349d40_headers[] = {0x8034b110};
constexpr std::uint32_t d_80349f00_callbacks[] = {0x80349f88};
constexpr std::uint32_t d_80349f00_headers[] = {0x8034b590};
constexpr std::uint32_t d_8034a09c_child_factories[] = {0x8034a064};
constexpr std::uint32_t d_8034a190_callbacks[] = {0x8034a334};
constexpr std::uint32_t d_8034a27c_callbacks[] = {0x8034a334};
constexpr std::uint32_t d_8034a334_callbacks[] = {0x8034a27c, 0x8034a3a0};
constexpr std::uint32_t d_8034a334_headers[] = {0x8034b600};
constexpr std::uint32_t b_80348dd4_callbacks[] = {0x80348c08, 0x80348d48};
const actor::BehaviorCallbackBinding callback_bindings[] = {
    {0x803467a4, func_803467A4_825F14, {}, {}, {}},
    {0x80346fa0, func_80346FA0_826710, d_80346fa0_callbacks, {}, {}},
    {0x803470cc, func_803470CC_82683C, d_803470cc_callbacks, {}, {}},
    {0x80347188, func_80347188_8268F8, {}, {}, {}},
    {0x803471d4, func_803471D4_826944, d_803471d4_callbacks, d_803471d4_graphs, {}},
    {0x80347334, func_80347334_826AA4, {}, d_80347334_graphs, d_80347334_weighted_states},
    {0x80347398, func_80347398_826B08, {}, {}, {}},
    {0x80347574, func_80347574_826CE4, {}, d_80347574_graphs, d_80347574_weighted_states},
    {0x803475d0, func_803475D0_826D40, d_803475d0_callbacks, d_803475d0_graphs, {}},
    {0x80347724, func_80347724_826E94, d_80347724_callbacks, d_80347724_graphs, {}},
    {0x80347900, func_80347900_827070, d_80347900_callbacks, {}, {}},
    {0x80347940, func_80347940_8270B0, d_80347940_callbacks, d_80347940_graphs, {}},
    {0x8034799c, func_8034799C_82710C, {}, {}, {}},
    {0x80347b88, func_80347B88_8272F8, d_80347b88_callbacks, d_80347b88_graphs, {}},
    {0x80347cc8, func_80347CC8_827438, {}, {}, {}},
    {0x80347e0c, func_80347E0C_82757C, d_80347e0c_callbacks, {}, {}},
    {0x80347e44, func_80347E44_8275B4, d_80347e44_callbacks, d_80347e44_graphs, {}},
    {0x8034800c, func_8034800C_82777C, d_8034800c_callbacks, d_8034800c_graphs, {}},
    {0x803480c0, func_803480C0_827830, {}, {}, {}},
    {0x80348208, func_80348208_827978, d_80348208_callbacks, d_80348208_graphs, {}},
    {0x803482ec, func_803482EC_827A5C, {}, {}, {}},
    {0x80348540, func_80348540_827CB0, {}, {}, {}},
    {0x8034877c, func_8034877C_827EEC, d_8034877c_callbacks, {}, {}},
    {0x803487cc, func_803487CC_827F3C, {}, {}, {}},
    {0x80348850, func_80348850_827FC0, {}, {}, {}},
    {0x80348970, func_80348970_8280E0, d_80348970_callbacks, {}, {}},
    {0x80348994, func_80348994_828104, d_80348994_callbacks, d_80348994_graphs, {}},
    {0x80348a04, func_80348A04_828174, d_80348a04_callbacks, {}, {}},
    {0x80348b34, func_80348B34_8282A4, {}, {}, {}},
    {0x80348c08, func_80348C08_828378, {}, {}, {}},
    {0x80348d48, func_80348D48_8284B8, {}, {}, {}},
    {0x80348dd4, func_80348DD4_828544, b_80348dd4_callbacks, {}, {}},
    {0x80349084, func_80349084_8287F4, {}, {}, {}},
    {0x80349180, func_80349180_8288F0, d_80349180_callbacks, {}, {}},
    {0x803491d4, func_803491D4_828944, {}, {}, {}},
    {0x8034930c, func_8034930C_828A7C, {}, d_8034930c_graphs, {}},
    {0x8034935c, func_8034935C_828ACC, d_8034935c_callbacks, d_8034935c_graphs, {}},
    {0x803493f4, func_803493F4_828B64, {}, d_803493f4_graphs, {}},
    {0x80349480, func_80349480_828BF0, {}, d_80349480_graphs, {}},
    {0x80349514, func_80349514_828C84, {}, d_80349514_graphs, {}},
    {0x8034956c, func_8034956C_828CDC, {}, d_8034956c_graphs, {}},
    {0x803495fc, func_803495FC_828D6C, {}, d_803495fc_graphs, {}},
    {0x80349654, func_80349654_828DC4, d_80349654_callbacks, {}, {}},
    {0x803496bc, func_803496BC_828E2C, {}, d_803496bc_graphs, {}},
    {0x80349714, func_80349714_828E84, d_80349714_callbacks, d_80349714_graphs, {}},
    {0x8034977c, func_8034977C_828EEC, d_8034977c_callbacks, d_8034977c_graphs, {}},
    {0x80349814, func_80349814_828F84, {}, d_80349814_graphs, {}},
    {0x803498a0, func_803498A0_829010, {}, d_803498a0_graphs, {}},
    {0x80349934, func_80349934_8290A4, {}, d_80349934_graphs, {}},
    {0x8034998c, func_8034998C_8290FC, {}, d_8034998c_graphs, {}},
    {0x80349a1c, func_80349A1C_82918C, {}, d_80349a1c_graphs, {}},
    {0x80349a74, func_80349A74_8291E4, d_80349a74_callbacks, {}, {}},
    {0x80349adc, func_80349ADC_82924C, {}, d_80349adc_graphs, {}},
    {0x80349b34, func_80349B34_8292A4, {}, d_80349b34_graphs, {}},
    {0x80349b98, func_80349B98_829308, d_80349b98_callbacks, d_80349b98_graphs, {}},
    {0x80349c18, func_80349C18_829388, {}, {}, {}},
    {0x80349d40, func_80349D40_8294B0, d_80349d40_callbacks, d_80349d40_graphs, {}},
    {0x80349db0, func_80349DB0_829520, {}, {}, {}},
    {0x80349de8, func_80349DE8_829558, {}, {}, {}},
    {0x80349e28, func_80349E28_829598, {}, {}, {}},
    {0x80349f00, func_80349F00_829670, d_80349f00_callbacks, {}, {}},
    {0x80349f88, func_80349F88_8296F8, {}, {}, {}},
    {0x8034a09c, func_8034A09C_82980C, {}, {}, {}},
    {0x8034a190, func_8034A190_829900, d_8034a190_callbacks, {}, {}},
    {0x8034a27c, func_8034A27C_8299EC, d_8034a27c_callbacks, {}, {}},
    {0x8034a334, func_8034A334_829AA4, d_8034a334_callbacks, {}, {}},
    {0x8034a3a0, func_8034A3A0_829B10, {}, {}, {}},
};
const ActorProviderDependency provider_dependencies[] = {
    {0x803466c0, d_803466c0_callbacks, {}, {}, {}, d_803466c0_child_factories, UINT64_C(0x7)},
    {0x803467a4, d_803467a4_callbacks, {}, {}, {}, d_803467a4_child_factories, UINT64_C(0x7)},
    {0x80346898, d_80346898_callbacks, {}, {}, {}, d_80346898_child_factories, UINT64_C(0x6)},
    {0x80346fa0, d_80346fa0_callbacks, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x803470cc, d_803470cc_callbacks, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x80347188, {}, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x803471d4, d_803471d4_callbacks, d_803471d4_graphs, {}, d_803471d4_headers, {}, UINT64_C(0x0)},
    {0x80347334, {}, d_80347334_graphs, d_80347334_weighted_states, d_80347334_headers, {}, UINT64_C(0x0)},
    {0x80347398, {}, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x80347574, {}, d_80347574_graphs, d_80347574_weighted_states, d_80347574_headers, {}, UINT64_C(0x0)},
    {0x803475d0, d_803475d0_callbacks, d_803475d0_graphs, {}, d_803475d0_headers, {}, UINT64_C(0x0)},
    {0x80347724, d_80347724_callbacks, d_80347724_graphs, {}, d_80347724_headers, {}, UINT64_C(0x0)},
    {0x80347900, d_80347900_callbacks, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x80347940, d_80347940_callbacks, d_80347940_graphs, {}, d_80347940_headers, {}, UINT64_C(0x0)},
    {0x8034799c, {}, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x80347b88, d_80347b88_callbacks, d_80347b88_graphs, {}, d_80347b88_headers, {}, UINT64_C(0x0)},
    {0x80347cc8, {}, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x80347e0c, d_80347e0c_callbacks, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x80347e44, d_80347e44_callbacks, d_80347e44_graphs, {}, d_80347e44_headers, {}, UINT64_C(0x0)},
    {0x8034800c, d_8034800c_callbacks, d_8034800c_graphs, {}, d_8034800c_headers, {}, UINT64_C(0x0)},
    {0x803480c0, {}, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x80348208, d_80348208_callbacks, d_80348208_graphs, {}, d_80348208_headers, {}, UINT64_C(0x0)},
    {0x803482ec, {}, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x803484f0, {}, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x80348540, d_80348540_callbacks, {}, {}, {}, {}, UINT64_C(0x780)},
    {0x8034877c, d_8034877c_callbacks, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x803487cc, {}, {}, {}, d_803487cc_headers, {}, UINT64_C(0x0)},
    {0x80348850, {}, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x80348938, {}, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x80348970, d_80348970_callbacks, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x80348994, d_80348994_callbacks, d_80348994_graphs, {}, d_80348994_headers, {}, UINT64_C(0x0)},
    {0x80348a04, d_80348a04_callbacks, {}, {}, d_80348a04_headers, {}, UINT64_C(0x8)},
    {0x80348b34, {}, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x80348bdc, {}, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x80348c08, {}, {}, {}, {}, {}, UINT64_C(0x200)},
    {0x80348d48, {}, {}, {}, {}, {}, UINT64_C(0x38)},
    {0x80348dd4, d_80348dd4_callbacks, {}, {}, {}, {}, UINT64_C(0x3fff840)},
    {0x80348fb8, {}, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x80348ff0, {}, {}, {}, {}, {}, UINT64_C(0x8000000)},
    {0x80349084, d_80349084_callbacks, {}, {}, {}, d_80349084_child_factories, UINT64_C(0x14000007)},
    {0x80349180, d_80349180_callbacks, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x803491d4, {}, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x8034930c, {}, d_8034930c_graphs, {}, {}, {}, UINT64_C(0x0)},
    {0x8034935c, d_8034935c_callbacks, d_8034935c_graphs, {}, d_8034935c_headers, {}, UINT64_C(0x0)},
    {0x803493f4, {}, d_803493f4_graphs, {}, d_803493f4_headers, {}, UINT64_C(0x0)},
    {0x80349480, {}, d_80349480_graphs, {}, d_80349480_headers, {}, UINT64_C(0x0)},
    {0x80349514, {}, d_80349514_graphs, {}, d_80349514_headers, {}, UINT64_C(0x0)},
    {0x8034956c, {}, d_8034956c_graphs, {}, d_8034956c_headers, {}, UINT64_C(0x0)},
    {0x803495fc, {}, d_803495fc_graphs, {}, d_803495fc_headers, {}, UINT64_C(0x0)},
    {0x80349654, d_80349654_callbacks, {}, {}, d_80349654_headers, {}, UINT64_C(0x0)},
    {0x803496bc, {}, d_803496bc_graphs, {}, d_803496bc_headers, {}, UINT64_C(0x0)},
    {0x80349714, d_80349714_callbacks, d_80349714_graphs, {}, {}, {}, UINT64_C(0x0)},
    {0x8034977c, d_8034977c_callbacks, d_8034977c_graphs, {}, d_8034977c_headers, {}, UINT64_C(0x0)},
    {0x80349814, {}, d_80349814_graphs, {}, d_80349814_headers, {}, UINT64_C(0x0)},
    {0x803498a0, {}, d_803498a0_graphs, {}, d_803498a0_headers, {}, UINT64_C(0x0)},
    {0x80349934, {}, d_80349934_graphs, {}, d_80349934_headers, {}, UINT64_C(0x0)},
    {0x8034998c, {}, d_8034998c_graphs, {}, d_8034998c_headers, {}, UINT64_C(0x0)},
    {0x80349a1c, {}, d_80349a1c_graphs, {}, d_80349a1c_headers, {}, UINT64_C(0x0)},
    {0x80349a74, d_80349a74_callbacks, {}, {}, d_80349a74_headers, {}, UINT64_C(0x0)},
    {0x80349adc, {}, d_80349adc_graphs, {}, d_80349adc_headers, {}, UINT64_C(0x0)},
    {0x80349b34, {}, d_80349b34_graphs, {}, {}, {}, UINT64_C(0x0)},
    {0x80349b98, d_80349b98_callbacks, d_80349b98_graphs, {}, d_80349b98_headers, {}, UINT64_C(0x0)},
    {0x80349c18, {}, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x80349d40, d_80349d40_callbacks, d_80349d40_graphs, {}, d_80349d40_headers, {}, UINT64_C(0x0)},
    {0x80349db0, {}, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x80349de8, {}, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x80349e28, {}, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x80349eb8, {}, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x80349f00, d_80349f00_callbacks, {}, {}, d_80349f00_headers, {}, UINT64_C(0x0)},
    {0x80349f88, {}, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x8034a064, {}, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x8034a09c, {}, {}, {}, {}, d_8034a09c_child_factories, UINT64_C(0x6)},
    {0x8034a190, d_8034a190_callbacks, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x8034a27c, d_8034a27c_callbacks, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x8034a334, d_8034a334_callbacks, {}, {}, d_8034a334_headers, {}, UINT64_C(0x0)},
    {0x8034a3a0, {}, {}, {}, {}, {}, UINT64_C(0x0)},
    {0x8034a4e8, {}, {}, {}, {}, {}, UINT64_C(0x0)},
};
const ActorFactoryBinding factory_bindings[] = {
    {151, 0x80348938, 0x8034ae90, 0x803470cc, ActorFactoryWrapper::ground, func_80348938_8280A8},
    {1001, 0x80348fb8, 0x8034af78, 0x80348970, ActorFactoryWrapper::ordinary_dl4, func_80348FB8_828728},
    {1037, 0x80349eb8, 0x8034b554, 0x80349180, ActorFactoryWrapper::ordinary_dl4, func_80349EB8_829628},
    {1002, 0x8034a064, 0x8034b5b8, 0x80349f00, ActorFactoryWrapper::ordinary_dl4, func_8034A064_8297D4},
    {1038, 0x8034a4e8, 0x8034b638, 0x8034a190, ActorFactoryWrapper::ordinary_dl4, func_8034A4E8_829C58},
};
constexpr ActorChildFactoryDependency child_factories[] = {
    {0x803466c0, {0x00000000, 0x803466c0, 151, 0x80348938, 0x80363dbc, 0x80364280, 0x8034ae90}, ISLE_RAINBOW_SPAWN_ID_TRANSFORMS},
    {0x803467a4, {0x00000000, 0x803467a4, 1037, 0x80349eb8, 0x80363dbc, 0x80364280, 0x8034b554}, ISLE_RAINBOW_SPAWN_ID_TRANSFORMS},
    {0x80346898, {0x00000000, 0x80346898, 1038, 0x8034a4e8, 0x80363dbc, 0x80364280, 0x8034b638}, ISLE_RAINBOW_SPAWN_ID_TRANSFORMS},
    {0x80349084, {0x8034afac, 0x80349084, 1001, 0x80348fb8, 0x80363dbc, 0x80364280, 0x8034af78}, ISLE_RAINBOW_SPAWN_ID_TRANSFORMS},
    {0x8034a09c, {0x8034b5ec, 0x8034a09c, 1002, 0x8034a064, 0x80363dbc, 0x80364280, 0x8034b5b8}, ISLE_RAINBOW_SPAWN_ID_TRANSFORMS},
};
constexpr ActorAnimationBinding animation_callbacks[] = {{0x80348bdc, func_80348BDC_82834C}};
constexpr ActorCollisionBinding collision_callbacks[] = {{0x80348ff0, func_80348FF0_828760}};
constexpr std::uint32_t graphs[] = {0x8034ad50, 0x8034ad90, 0x8034add0, 0x8034ae10, 0x8034af3c, 0x8034b188, 0x8034b1a8, 0x8034b208, 0x8034b238, 0x8034b268, 0x8034b298, 0x8034b2c8, 0x8034b328, 0x8034b348, 0x8034b3b8, 0x8034b3e8, 0x8034b418, 0x8034b448, 0x8034b478, 0x8034b4e8};
constexpr std::uint32_t weighted[] = {0x8034ae30};
constexpr std::uint32_t roots[] = {0x803470cc, 0x80348970, 0x80349180, 0x80349f00, 0x8034a190, 0x803467a4, 0x80349084, 0x8034a09c};
} // namespace
std::span<const actor::BehaviorCallbackBinding> actor_callbacks() noexcept { return callback_bindings; }
std::span<const std::uint32_t> actor_graphs() noexcept { return graphs; }
std::span<const std::uint32_t> actor_weighted_states() noexcept { return weighted; }
std::span<const std::uint32_t> actor_roots() noexcept { return roots; }
std::span<const ActorFactoryBinding> actor_factories() noexcept { return factory_bindings; }
std::span<const ActorChildFactoryDependency> actor_child_factories() noexcept { return child_factories; }
std::span<const ActorProviderDependency> actor_dependencies() noexcept { return provider_dependencies; }
std::span<const ActorAnimationBinding> actor_animation_callbacks() noexcept { return animation_callbacks; }
std::span<const ActorCollisionBinding> actor_collision_callbacks() noexcept { return collision_callbacks; }
std::uint64_t actor_required_services() noexcept { return UINT64_C(0x1fffffff); }
} // namespace isle::rainbow

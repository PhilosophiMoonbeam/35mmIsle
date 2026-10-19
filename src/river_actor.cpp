#include "river_actor.hpp"

namespace isle::river {
namespace {
constexpr std::uint32_t d_37_func_802D886C_6C064C_functions[] = {0x802d8800u, 0x802d8824u, 0x802d8848u};
constexpr std::uint32_t d_69_func_802D89B4_6C0794_functions[] = {0x802d88f8u};
constexpr ActorNativeService d_85_func_802D8A48_6C0828_services[] = {ActorNativeService::add_one, ActorNativeService::current_block, ActorNativeService::definition, ActorNativeService::model, ActorNativeService::model_store};
constexpr ActorNativeService d_116_func_802D8B34_6C0914_services[] = {ActorNativeService::add_one, ActorNativeService::current_block, ActorNativeService::definition, ActorNativeService::model, ActorNativeService::model_store};
constexpr ActorNativeService d_146_func_802D8C24_6C0A04_services[] = {ActorNativeService::add_one, ActorNativeService::current_block, ActorNativeService::definition, ActorNativeService::model, ActorNativeService::model_store};
constexpr ActorNativeService d_175_func_802D8CF0_6C0AD0_services[] = {ActorNativeService::add_one, ActorNativeService::current_block, ActorNativeService::definition, ActorNativeService::model, ActorNativeService::model_store};
constexpr ActorNativeService d_205_func_802D8DC4_6C0BA4_services[] = {ActorNativeService::add_one, ActorNativeService::current_block, ActorNativeService::definition, ActorNativeService::model, ActorNativeService::model_store};
constexpr std::uint32_t d_235_func_802D8E98_6C0C78_counted_options[] = {0x802e2908u};
constexpr ActorNativeService d_235_func_802D8E98_6C0C78_services[] = {ActorNativeService::item, ActorNativeService::model, ActorNativeService::player_model, ActorNativeService::rng_range, ActorNativeService::root_position, ActorNativeService::send_command_link};
constexpr std::uint32_t d_282_func_802D90B8_6C0E98_functions[] = {0x802d886cu, 0x802d88f8u, 0x802d895cu, 0x802d89acu, 0x802d89b4u, 0x802d8e98u, 0x802d8fccu, 0x802d92b0u, 0x802e25c4u};
constexpr ActorNativeService d_32_func_802D9520_6C1300_services[] = {ActorNativeService::block_position, ActorNativeService::current_block, ActorNativeService::model};
constexpr std::uint32_t d_65_func_802D9664_6C1444_functions[] = {0x802d9a58u, 0x802d9dd8u};
constexpr std::uint32_t d_65_func_802D9664_6C1444_graphs[] = {0x802e2ac4u};
constexpr std::uint32_t d_65_func_802D9664_6C1444_weighted_states[] = {0x802e2b24u};
constexpr std::uint32_t d_65_func_802D9664_6C1444_headers[] = {0x802e29fcu};
constexpr ActorNativeService d_65_func_802D9664_6C1444_services[] = {ActorNativeService::graph, ActorNativeService::weighted};
constexpr std::uint32_t b_func_802D9664_6C1444_callbacks[] = {0x802d9a58u, 0x802d9dd8u};
constexpr std::uint32_t d_79_func_802D96FC_6C14DC_graphs[] = {0x802e2ac4u};
constexpr std::uint32_t d_79_func_802D96FC_6C14DC_weighted_states[] = {0x802e2b24u};
constexpr std::uint32_t d_79_func_802D96FC_6C14DC_headers[] = {0x802e29fcu};
constexpr ActorNativeService d_79_func_802D96FC_6C14DC_services[] = {ActorNativeService::graph, ActorNativeService::weighted};
constexpr std::uint32_t d_90_func_802D976C_6C154C_graphs[] = {0x802e2ac4u};
constexpr std::uint32_t d_90_func_802D976C_6C154C_weighted_states[] = {0x802e2b24u};
constexpr std::uint32_t d_90_func_802D976C_6C154C_headers[] = {0x802e2a10u};
constexpr ActorNativeService d_90_func_802D976C_6C154C_services[] = {ActorNativeService::graph, ActorNativeService::model, ActorNativeService::weighted};
constexpr std::uint32_t d_104_func_802D9808_6C15E8_functions[] = {0x802d96fcu, 0x802d990cu};
constexpr std::uint32_t d_104_func_802D9808_6C15E8_graphs[] = {0x802e2b3cu, 0x802e2b7cu};
constexpr std::uint32_t d_104_func_802D9808_6C15E8_headers[] = {0x802e2a24u, 0x802e2a4cu};
constexpr ActorNativeService d_104_func_802D9808_6C15E8_services[] = {ActorNativeService::graph};
constexpr std::uint32_t b_func_802D9808_6C15E8_callbacks[] = {0x802d96fcu, 0x802d990cu};
constexpr std::uint32_t d_128_func_802D990C_6C16EC_headers[] = {0x802e2a38u};
constexpr std::uint32_t d_141_func_802D9990_6C1770_functions[] = {0x802d96fcu, 0x802d9a0cu};
constexpr std::uint32_t d_141_func_802D9990_6C1770_graphs[] = {0x802e2b9cu};
constexpr std::uint32_t d_141_func_802D9990_6C1770_headers[] = {0x802e2a24u};
constexpr ActorNativeService d_141_func_802D9990_6C1770_services[] = {ActorNativeService::graph};
constexpr std::uint32_t b_func_802D9990_6C1770_callbacks[] = {0x802d96fcu, 0x802d9a0cu};
constexpr std::uint32_t d_163_func_802D9A58_6C1838_functions[] = {0x802d9520u};
constexpr ActorNativeService d_163_func_802D9A58_6C1838_services[] = {ActorNativeService::send_command};
constexpr std::uint32_t d_177_func_802D9AC4_6C18A4_functions[] = {0x802d8a48u, 0x802d8b34u, 0x802d9c8cu, 0x802d9d7cu, 0x802dff94u};
constexpr std::uint32_t d_177_func_802D9AC4_6C18A4_headers[] = {0x802e2a24u, 0x802e2a38u, 0x802e2a60u, 0x802e2a74u, 0x802e2a88u, 0x802e2a9cu, 0x802e2ab0u};
constexpr ActorNativeService d_177_func_802D9AC4_6C18A4_services[] = {ActorNativeService::source_process};
constexpr std::uint32_t b_func_802D9AC4_6C18A4_callbacks[] = {0x802d8a48u, 0x802d8b34u, 0x802d9c8cu, 0x802d9d7cu, 0x802dff94u};
constexpr ActorNativeService d_219_func_802D9C8C_6C1A6C_services[] = {ActorNativeService::model};
constexpr std::uint32_t d_24_func_802D9F80_6C1D60_graphs[] = {0x802e2ca8u};
constexpr std::uint32_t d_24_func_802D9F80_6C1D60_weighted_states[] = {0x802e2d18u};
constexpr std::uint32_t d_24_func_802D9F80_6C1D60_headers[] = {0x802e2c1cu};
constexpr ActorNativeService d_24_func_802D9F80_6C1D60_services[] = {ActorNativeService::graph, ActorNativeService::weighted};
constexpr std::uint32_t d_34_func_802D9FDC_6C1DBC_graphs[] = {0x802e2ca8u};
constexpr std::uint32_t d_34_func_802D9FDC_6C1DBC_weighted_states[] = {0x802e2d18u};
constexpr std::uint32_t d_34_func_802D9FDC_6C1DBC_headers[] = {0x802e2c1cu};
constexpr ActorNativeService d_34_func_802D9FDC_6C1DBC_services[] = {ActorNativeService::graph, ActorNativeService::weighted};
constexpr std::uint32_t d_45_func_802DA04C_6C1E2C_graphs[] = {0x802e2ca8u};
constexpr std::uint32_t d_45_func_802DA04C_6C1E2C_weighted_states[] = {0x802e2d18u};
constexpr std::uint32_t d_45_func_802DA04C_6C1E2C_headers[] = {0x802e2c30u};
constexpr ActorNativeService d_45_func_802DA04C_6C1E2C_services[] = {ActorNativeService::graph, ActorNativeService::weighted};
constexpr std::uint32_t d_56_func_802DA0BC_6C1E9C_graphs[] = {0x802e2ca8u};
constexpr std::uint32_t d_56_func_802DA0BC_6C1E9C_weighted_states[] = {0x802e2d18u};
constexpr std::uint32_t d_56_func_802DA0BC_6C1E9C_headers[] = {0x802e2c94u};
constexpr ActorNativeService d_56_func_802DA0BC_6C1E9C_services[] = {ActorNativeService::graph, ActorNativeService::weighted};
constexpr std::uint32_t d_67_func_802DA12C_6C1F0C_functions[] = {0x802da1acu};
constexpr std::uint32_t d_67_func_802DA12C_6C1F0C_graphs[] = {0x802e2ca8u};
constexpr std::uint32_t d_67_func_802DA12C_6C1F0C_weighted_states[] = {0x802e2d18u};
constexpr std::uint32_t d_67_func_802DA12C_6C1F0C_headers[] = {0x802e2c44u};
constexpr ActorNativeService d_67_func_802DA12C_6C1F0C_services[] = {ActorNativeService::graph, ActorNativeService::weighted};
constexpr std::uint32_t b_func_802DA12C_6C1F0C_callbacks[] = {0x802da1acu};
constexpr std::uint32_t d_90_func_802DA208_6C1FE8_functions[] = {0x802da04cu, 0x802da30cu};
constexpr std::uint32_t d_90_func_802DA208_6C1FE8_graphs[] = {0x802e2d80u, 0x802e2dc0u};
constexpr std::uint32_t d_90_func_802DA208_6C1FE8_headers[] = {0x802e2c80u};
constexpr ActorNativeService d_90_func_802DA208_6C1FE8_services[] = {ActorNativeService::graph, ActorNativeService::model};
constexpr std::uint32_t b_func_802DA208_6C1FE8_callbacks[] = {0x802da04cu, 0x802da30cu};
constexpr std::uint32_t d_109_func_802DA30C_6C20EC_headers[] = {0x802e2c58u};
constexpr std::uint32_t d_121_func_802DA390_6C2170_functions[] = {0x802d9fdcu};
constexpr std::uint32_t d_121_func_802DA390_6C2170_headers[] = {0x802e2c6cu};
constexpr std::uint32_t b_func_802DA390_6C2170_callbacks[] = {0x802d9fdcu};
constexpr std::uint32_t d_132_func_802DA3F8_6C21D8_functions[] = {0x802d9fdcu, 0x802da46cu};
constexpr std::uint32_t d_132_func_802DA3F8_6C21D8_graphs[] = {0x802e2d80u};
constexpr std::uint32_t d_132_func_802DA3F8_6C21D8_headers[] = {0x802e2c44u};
constexpr ActorNativeService d_132_func_802DA3F8_6C21D8_services[] = {ActorNativeService::graph};
constexpr std::uint32_t b_func_802DA3F8_6C21D8_callbacks[] = {0x802d9fdcu, 0x802da46cu};
constexpr std::uint32_t d_17_func_802DA500_6C22E0_functions[] = {0x802da590u, 0x802da73cu, 0x802da814u};
constexpr std::uint32_t d_17_func_802DA500_6C22E0_graphs[] = {0x802e2e58u};
constexpr std::uint32_t d_17_func_802DA500_6C22E0_headers[] = {0x802e2e44u};
constexpr ActorNativeService d_17_func_802DA500_6C22E0_services[] = {ActorNativeService::graph};
constexpr std::uint32_t b_func_802DA500_6C22E0_callbacks[] = {0x802da590u, 0x802da73cu, 0x802da814u};
constexpr std::uint32_t d_31_func_802DA590_6C2370_functions[] = {0x802da620u, 0x802da674u};
constexpr std::uint32_t b_func_802DA590_6C2370_callbacks[] = {0x802da620u, 0x802da674u};
constexpr ActorNativeService d_44_func_802DA620_6C2400_services[] = {ActorNativeService::model};
constexpr ActorNativeService d_56_func_802DA674_6C2454_services[] = {ActorNativeService::environment_sound, ActorNativeService::model, ActorNativeService::model_read};
constexpr ActorNativeService d_91_func_802DA768_6C2548_services[] = {ActorNativeService::model};
constexpr std::uint32_t d_105_func_802DA814_6C25F4_graphs[] = {0x802e2e88u};
constexpr ActorNativeService d_105_func_802DA814_6C25F4_services[] = {ActorNativeService::graph};
constexpr std::uint32_t d_114_func_802DA864_6C2644_functions[] = {0x802da590u, 0x802da73cu};
constexpr std::uint32_t d_114_func_802DA864_6C2644_graphs[] = {0x802e2e58u};
constexpr ActorNativeService d_114_func_802DA864_6C2644_services[] = {ActorNativeService::graph};
constexpr std::uint32_t b_func_802DA864_6C2644_callbacks[] = {0x802da590u, 0x802da73cu};
constexpr std::uint32_t d_11_func_802DA910_6C26F0_graphs[] = {0x802e2f34u};
constexpr std::uint32_t d_11_func_802DA910_6C26F0_headers[] = {0x802e2f0cu, 0x802e2f20u};
constexpr ActorNativeService d_11_func_802DA910_6C26F0_services[] = {ActorNativeService::graph};
constexpr ActorNativeService d_20_func_802DA9E0_6C27C0_services[] = {ActorNativeService::add_one, ActorNativeService::current_block, ActorNativeService::definition, ActorNativeService::model, ActorNativeService::model_store};
constexpr ActorNativeService d_52_func_802DAACC_6C28AC_services[] = {ActorNativeService::add_one, ActorNativeService::current_block, ActorNativeService::definition, ActorNativeService::model, ActorNativeService::model_store};
constexpr std::uint32_t d_84_func_802DABB8_6C2998_graphs[] = {0x802e3028u};
constexpr ActorNativeService d_84_func_802DABB8_6C2998_services[] = {ActorNativeService::graph};
constexpr std::uint32_t d_93_func_802DAC04_6C29E4_weighted_states[] = {0x802e3048u};
constexpr ActorNativeService d_93_func_802DAC04_6C29E4_services[] = {ActorNativeService::weighted};
constexpr std::uint32_t d_97_func_802DAC28_6C2A08_functions[] = {0x802da9e0u};
constexpr ActorNativeService d_97_func_802DAC28_6C2A08_services[] = {ActorNativeService::source_process};
constexpr std::uint32_t b_func_802DAC28_6C2A08_callbacks[] = {0x802da9e0u};
constexpr std::uint32_t d_109_func_802DACA8_6C2A88_functions[] = {0x802daaccu};
constexpr ActorNativeService d_109_func_802DACA8_6C2A88_services[] = {ActorNativeService::source_process};
constexpr std::uint32_t b_func_802DACA8_6C2A88_callbacks[] = {0x802daaccu};
constexpr std::uint32_t d_125_func_802DAD60_6C2B40_functions[] = {0x802dadd8u};
constexpr std::uint32_t d_125_func_802DAD60_6C2B40_graphs[] = {0x802e30e0u};
constexpr std::uint32_t d_125_func_802DAD60_6C2B40_headers[] = {0x802e30ccu};
constexpr ActorNativeService d_125_func_802DAD60_6C2B40_services[] = {ActorNativeService::graph};
constexpr std::uint32_t b_func_802DAD60_6C2B40_callbacks[] = {0x802dadd8u};
constexpr std::uint32_t d_137_func_802DADD8_6C2BB8_functions[] = {0x802dae68u, 0x802daed4u};
constexpr std::uint32_t b_func_802DADD8_6C2BB8_callbacks[] = {0x802dae68u, 0x802daed4u};
constexpr ActorNativeService d_150_func_802DAE68_6C2C48_services[] = {ActorNativeService::model};
constexpr ActorNativeService d_163_func_802DAED4_6C2CB4_services[] = {ActorNativeService::environment_sound, ActorNativeService::model};
constexpr ActorNativeService d_192_func_802DAFB8_6C2D98_services[] = {ActorNativeService::model};
constexpr std::uint32_t d_38_func_802DB0A0_6C2E80_functions[] = {0x802db388u, 0x802db8ecu};
constexpr std::uint32_t d_38_func_802DB0A0_6C2E80_weighted_states[] = {0x802e32bcu};
constexpr ActorNativeService d_38_func_802DB0A0_6C2E80_services[] = {ActorNativeService::weighted};
constexpr std::uint32_t b_func_802DB0A0_6C2E80_callbacks[] = {0x802db388u, 0x802db8ecu};
constexpr std::uint32_t d_54_func_802DB11C_6C2EFC_functions[] = {0x802db1c4u};
constexpr std::uint32_t d_54_func_802DB11C_6C2EFC_headers[] = {0x802e319cu};
constexpr std::uint32_t b_func_802DB11C_6C2EFC_callbacks[] = {0x802db1c4u};
constexpr std::uint32_t d_59_func_802DB154_6C2F34_functions[] = {0x802db1c4u};
constexpr std::uint32_t d_59_func_802DB154_6C2F34_headers[] = {0x802e31b0u};
constexpr std::uint32_t b_func_802DB154_6C2F34_callbacks[] = {0x802db1c4u};
constexpr std::uint32_t d_64_func_802DB18C_6C2F6C_functions[] = {0x802db1c4u};
constexpr std::uint32_t d_64_func_802DB18C_6C2F6C_headers[] = {0x802e31c4u};
constexpr std::uint32_t b_func_802DB18C_6C2F6C_callbacks[] = {0x802db1c4u};
constexpr std::uint32_t d_69_func_802DB1C4_6C2FA4_functions[] = {0x802db270u};
constexpr ActorNativeService d_69_func_802DB1C4_6C2FA4_services[] = {ActorNativeService::model, ActorNativeService::rng_range};
constexpr std::uint32_t b_func_802DB1C4_6C2FA4_callbacks[] = {0x802db270u};
constexpr ActorNativeService d_84_func_802DB270_6C3050_services[] = {ActorNativeService::ground, ActorNativeService::model, ActorNativeService::node_descend, ActorNativeService::node_world};
constexpr std::uint32_t d_116_func_802DB388_6C3168_functions[] = {0x802db41cu};
constexpr std::uint32_t d_116_func_802DB388_6C3168_graphs[] = {0x802e324cu};
constexpr std::uint32_t d_116_func_802DB388_6C3168_headers[] = {0x802e31d8u};
constexpr ActorNativeService d_116_func_802DB388_6C3168_services[] = {ActorNativeService::graph, ActorNativeService::model};
constexpr std::uint32_t b_func_802DB388_6C3168_callbacks[] = {0x802db41cu};
constexpr std::uint32_t d_137_func_802DB468_6C3248_functions[] = {0x802db4a8u, 0x802db630u};
constexpr std::uint32_t b_func_802DB468_6C3248_callbacks[] = {0x802db4a8u, 0x802db630u};
constexpr std::uint32_t d_145_func_802DB4A8_6C3288_functions[] = {0x802db554u, 0x802db5c0u, 0x802db71cu};
constexpr std::uint32_t d_145_func_802DB4A8_6C3288_graphs[] = {0x802e328cu};
constexpr std::uint32_t d_145_func_802DB4A8_6C3288_headers[] = {0x802e31ecu};
constexpr ActorNativeService d_145_func_802DB4A8_6C3288_services[] = {ActorNativeService::graph};
constexpr std::uint32_t b_func_802DB4A8_6C3288_callbacks[] = {0x802db554u, 0x802db5c0u, 0x802db71cu};
constexpr ActorNativeService d_183_func_802DB630_6C3410_services[] = {ActorNativeService::model, ActorNativeService::model_read, ActorNativeService::model_store};
constexpr std::uint32_t d_204_func_802DB71C_6C34FC_functions[] = {0x802db270u};
constexpr std::uint32_t d_204_func_802DB71C_6C34FC_headers[] = {0x802e3228u};
constexpr std::uint32_t b_func_802DB71C_6C34FC_callbacks[] = {0x802db270u};
constexpr std::uint32_t d_216_func_802DB78C_6C356C_functions[] = {0x802db270u, 0x802db85cu};
constexpr std::uint32_t d_216_func_802DB78C_6C356C_headers[] = {0x802e3200u, 0x802e3214u};
constexpr ActorNativeService d_216_func_802DB78C_6C356C_services[] = {ActorNativeService::send_command_link};
constexpr std::uint32_t b_func_802DB78C_6C356C_callbacks[] = {0x802db270u, 0x802db85cu};
constexpr ActorNativeService d_237_func_802DB85C_6C363C_services[] = {ActorNativeService::model, ActorNativeService::model_read, ActorNativeService::model_store};
constexpr std::uint32_t d_254_func_802DB8EC_6C36CC_graphs[] = {0x802e32e8u};
constexpr ActorNativeService d_254_func_802DB8EC_6C36CC_services[] = {ActorNativeService::graph};
constexpr std::uint32_t d_263_func_802DB93C_6C371C_headers[] = {0x802e319cu};
constexpr ActorNativeService d_263_func_802DB93C_6C371C_services[] = {ActorNativeService::block_position, ActorNativeService::current_block, ActorNativeService::ground, ActorNativeService::model, ActorNativeService::path_count, ActorNativeService::path_parameter, ActorNativeService::rng_range};
constexpr std::uint32_t b_func_802DB93C_6C371C_callbacks[] = {0x802db270u};
constexpr std::uint32_t d_52_func_802DBBA0_6C3980_functions[] = {0x802dbeccu, 0x802dc150u, 0x802dc404u, 0x802dcbb8u};
constexpr std::uint32_t d_52_func_802DBBA0_6C3980_weighted_states[] = {0x802e3474u};
constexpr ActorNativeService d_52_func_802DBBA0_6C3980_services[] = {ActorNativeService::model, ActorNativeService::weighted};
constexpr std::uint32_t b_func_802DBBA0_6C3980_callbacks[] = {0x802dbeccu, 0x802dc150u, 0x802dc404u, 0x802dcbb8u};
constexpr std::uint32_t d_74_func_802DBC5C_6C3A3C_functions[] = {0x802dbd04u};
constexpr std::uint32_t d_74_func_802DBC5C_6C3A3C_headers[] = {0x802e339cu};
constexpr std::uint32_t b_func_802DBC5C_6C3A3C_callbacks[] = {0x802dbd04u};
constexpr std::uint32_t d_79_func_802DBC94_6C3A74_functions[] = {0x802dbd04u};
constexpr std::uint32_t d_79_func_802DBC94_6C3A74_headers[] = {0x802e33b0u};
constexpr std::uint32_t b_func_802DBC94_6C3A74_callbacks[] = {0x802dbd04u};
constexpr std::uint32_t d_84_func_802DBCCC_6C3AAC_functions[] = {0x802dbd04u};
constexpr std::uint32_t d_84_func_802DBCCC_6C3AAC_headers[] = {0x802e33c4u};
constexpr std::uint32_t b_func_802DBCCC_6C3AAC_callbacks[] = {0x802dbd04u};
constexpr std::uint32_t d_89_func_802DBD04_6C3AE4_functions[] = {0x802dbdb0u};
constexpr ActorNativeService d_89_func_802DBD04_6C3AE4_services[] = {ActorNativeService::model, ActorNativeService::rng_range};
constexpr std::uint32_t b_func_802DBD04_6C3AE4_callbacks[] = {0x802dbdb0u};
constexpr ActorNativeService d_104_func_802DBDB0_6C3B90_services[] = {ActorNativeService::ground, ActorNativeService::model, ActorNativeService::node_descend, ActorNativeService::node_world};
constexpr std::uint32_t d_132_func_802DBECC_6C3CAC_graphs[] = {0x802e3494u};
constexpr std::uint32_t d_132_func_802DBECC_6C3CAC_weighted_states[] = {0x802e3504u};
constexpr std::uint32_t d_132_func_802DBECC_6C3CAC_headers[] = {0x802e33d8u};
constexpr ActorNativeService d_132_func_802DBECC_6C3CAC_services[] = {ActorNativeService::graph, ActorNativeService::weighted};
constexpr std::uint32_t d_143_func_802DBF3C_6C3D1C_graphs[] = {0x802e3494u};
constexpr std::uint32_t d_143_func_802DBF3C_6C3D1C_weighted_states[] = {0x802e3504u};
constexpr std::uint32_t d_143_func_802DBF3C_6C3D1C_headers[] = {0x802e33ecu};
constexpr ActorNativeService d_143_func_802DBF3C_6C3D1C_services[] = {ActorNativeService::graph, ActorNativeService::weighted};
constexpr std::uint32_t d_154_func_802DBFAC_6C3D8C_functions[] = {0x802dc05cu};
constexpr std::uint32_t d_154_func_802DBFAC_6C3D8C_headers[] = {0x802e3428u, 0x802e343cu, 0x802e3450u};
constexpr ActorNativeService d_154_func_802DBFAC_6C3D8C_services[] = {ActorNativeService::model};
constexpr std::uint32_t b_func_802DBFAC_6C3D8C_callbacks[] = {0x802dc05cu};
constexpr std::uint32_t d_168_func_802DC05C_6C3E3C_functions[] = {0x802dc0a4u, 0x802dca7cu};
constexpr ActorNativeService d_168_func_802DC05C_6C3E3C_services[] = {ActorNativeService::model};
constexpr std::uint32_t b_func_802DC05C_6C3E3C_callbacks[] = {0x802dc0a4u, 0x802dca7cu};
constexpr std::uint32_t d_174_func_802DC0A4_6C3E84_functions[] = {0x802dc0a4u, 0x802dc150u};
constexpr ActorNativeService d_174_func_802DC0A4_6C3E84_services[] = {ActorNativeService::graph, ActorNativeService::model};
constexpr std::uint32_t b_func_802DC0A4_6C3E84_callbacks[] = {0x802dc0a4u, 0x802dc150u};
constexpr std::uint32_t d_187_func_802DC150_6C3F30_graphs[] = {0x802e351cu};
constexpr std::uint32_t d_187_func_802DC150_6C3F30_weighted_states[] = {0x802e356cu};
constexpr std::uint32_t d_187_func_802DC150_6C3F30_headers[] = {0x802e33d8u};
constexpr ActorNativeService d_187_func_802DC150_6C3F30_services[] = {ActorNativeService::graph, ActorNativeService::weighted};
constexpr std::uint32_t d_198_func_802DC1C0_6C3FA0_graphs[] = {0x802e351cu};
constexpr std::uint32_t d_198_func_802DC1C0_6C3FA0_weighted_states[] = {0x802e356cu};
constexpr std::uint32_t d_198_func_802DC1C0_6C3FA0_headers[] = {0x802e33ecu};
constexpr ActorNativeService d_198_func_802DC1C0_6C3FA0_services[] = {ActorNativeService::graph, ActorNativeService::weighted};
constexpr std::uint32_t d_209_func_802DC230_6C4010_functions[] = {0x802dc2f4u};
constexpr std::uint32_t d_209_func_802DC230_6C4010_headers[] = {0x802e3428u, 0x802e343cu, 0x802e3450u};
constexpr ActorNativeService d_209_func_802DC230_6C4010_services[] = {ActorNativeService::model, ActorNativeService::send_command_link};
constexpr std::uint32_t b_func_802DC230_6C4010_callbacks[] = {0x802dc2f4u};
constexpr std::uint32_t d_224_func_802DC2F4_6C40D4_functions[] = {0x802dc358u, 0x802dca7cu};
constexpr ActorNativeService d_224_func_802DC2F4_6C40D4_services[] = {ActorNativeService::model, ActorNativeService::send_command_link};
constexpr std::uint32_t b_func_802DC2F4_6C40D4_callbacks[] = {0x802dc358u, 0x802dca7cu};
constexpr std::uint32_t d_231_func_802DC358_6C4138_functions[] = {0x802dc358u, 0x802dc404u};
constexpr ActorNativeService d_231_func_802DC358_6C4138_services[] = {ActorNativeService::graph, ActorNativeService::model};
constexpr std::uint32_t b_func_802DC358_6C4138_callbacks[] = {0x802dc358u, 0x802dc404u};
constexpr std::uint32_t d_244_func_802DC404_6C41E4_graphs[] = {0x802e3584u};
constexpr std::uint32_t d_244_func_802DC404_6C41E4_weighted_states[] = {0x802e35d4u};
constexpr std::uint32_t d_244_func_802DC404_6C41E4_headers[] = {0x802e33d8u};
constexpr ActorNativeService d_244_func_802DC404_6C41E4_services[] = {ActorNativeService::graph, ActorNativeService::weighted};
constexpr std::uint32_t d_255_func_802DC474_6C4254_graphs[] = {0x802e3584u};
constexpr std::uint32_t d_255_func_802DC474_6C4254_weighted_states[] = {0x802e35d4u};
constexpr std::uint32_t d_255_func_802DC474_6C4254_headers[] = {0x802e33ecu};
constexpr ActorNativeService d_255_func_802DC474_6C4254_services[] = {ActorNativeService::graph, ActorNativeService::weighted};
constexpr std::uint32_t d_266_func_802DC4E4_6C42C4_functions[] = {0x802dc5a8u};
constexpr std::uint32_t d_266_func_802DC4E4_6C42C4_headers[] = {0x802e3428u, 0x802e343cu, 0x802e3450u};
constexpr ActorNativeService d_266_func_802DC4E4_6C42C4_services[] = {ActorNativeService::send_command_link};
constexpr std::uint32_t b_func_802DC4E4_6C42C4_callbacks[] = {0x802dc5a8u};
constexpr std::uint32_t d_284_func_802DC5A8_6C4388_functions[] = {0x802dc60cu, 0x802dca7cu};
constexpr ActorNativeService d_284_func_802DC5A8_6C4388_services[] = {ActorNativeService::send_command_link};
constexpr std::uint32_t b_func_802DC5A8_6C4388_callbacks[] = {0x802dc60cu, 0x802dca7cu};
constexpr std::uint32_t d_294_func_802DC60C_6C43EC_functions[] = {0x802dc60cu, 0x802dc6bcu};
constexpr ActorNativeService d_294_func_802DC60C_6C43EC_services[] = {ActorNativeService::graph, ActorNativeService::model};
constexpr std::uint32_t b_func_802DC60C_6C43EC_callbacks[] = {0x802dc60cu, 0x802dc6bcu};
constexpr std::uint32_t d_309_func_802DC6BC_6C449C_functions[] = {0x802dc7acu, 0x802dc820u};
constexpr ActorNativeService d_309_func_802DC6BC_6C449C_services[] = {ActorNativeService::model, ActorNativeService::send_command_link};
constexpr std::uint32_t b_func_802DC6BC_6C449C_callbacks[] = {0x802dc7acu, 0x802dc820u};
constexpr ActorNativeService d_334_func_802DC7AC_6C458C_services[] = {ActorNativeService::model, ActorNativeService::path_parameter};
constexpr ActorNativeService d_341_func_802DC820_6C4600_services[] = {ActorNativeService::block_position, ActorNativeService::current_block, ActorNativeService::environment_sound, ActorNativeService::ground, ActorNativeService::model, ActorNativeService::path_duration, ActorNativeService::path_parameter};
constexpr ActorNativeService d_392_func_802DCA7C_6C485C_services[] = {ActorNativeService::model, ActorNativeService::path_parameter};
constexpr std::uint32_t d_405_func_802DCB0C_6C48EC_headers[] = {0x802e3428u, 0x802e343cu, 0x802e3450u};
constexpr ActorNativeService d_405_func_802DCB0C_6C48EC_services[] = {ActorNativeService::model};
constexpr std::uint32_t d_420_func_802DCBB8_6C4998_graphs[] = {0x802e35ecu, 0x802e360cu, 0x802e362cu};
constexpr ActorNativeService d_420_func_802DCBB8_6C4998_services[] = {ActorNativeService::graph};
constexpr std::uint32_t d_441_func_802DCC6C_6C4A4C_headers[] = {0x802e339cu};
constexpr ActorNativeService d_441_func_802DCC6C_6C4A4C_services[] = {ActorNativeService::block_position, ActorNativeService::current_block, ActorNativeService::ground, ActorNativeService::model, ActorNativeService::path_count, ActorNativeService::path_parameter, ActorNativeService::rng_range};
constexpr std::uint32_t b_func_802DCC6C_6C4A4C_callbacks[] = {0x802dbdb0u};
constexpr std::uint32_t d_16_func_802DCEF0_6C4CD0_weighted_states[] = {0x802e3748u};
constexpr ActorNativeService d_16_func_802DCEF0_6C4CD0_services[] = {ActorNativeService::weighted};
constexpr std::uint32_t d_20_func_802DCF14_6C4CF4_functions[] = {0x802dcfbcu};
constexpr std::uint32_t d_20_func_802DCF14_6C4CF4_headers[] = {0x802e36fcu};
constexpr std::uint32_t b_func_802DCF14_6C4CF4_callbacks[] = {0x802dcfbcu};
constexpr std::uint32_t d_25_func_802DCF4C_6C4D2C_functions[] = {0x802dcfbcu};
constexpr std::uint32_t d_25_func_802DCF4C_6C4D2C_headers[] = {0x802e3710u};
constexpr std::uint32_t b_func_802DCF4C_6C4D2C_callbacks[] = {0x802dcfbcu};
constexpr std::uint32_t d_30_func_802DCF84_6C4D64_functions[] = {0x802dcfbcu};
constexpr std::uint32_t d_30_func_802DCF84_6C4D64_headers[] = {0x802e3724u};
constexpr std::uint32_t b_func_802DCF84_6C4D64_callbacks[] = {0x802dcfbcu};
constexpr std::uint32_t d_35_func_802DCFBC_6C4D9C_functions[] = {0x802dd068u};
constexpr ActorNativeService d_35_func_802DCFBC_6C4D9C_services[] = {ActorNativeService::model, ActorNativeService::rng_range};
constexpr std::uint32_t b_func_802DCFBC_6C4D9C_callbacks[] = {0x802dd068u};
constexpr std::uint32_t d_79_func_802DD0E0_6C4EC0_graphs[] = {0x802e39b8u};
constexpr std::uint32_t d_79_func_802DD0E0_6C4EC0_headers[] = {0x802e388cu, 0x802e392cu};
constexpr ActorNativeService d_79_func_802DD0E0_6C4EC0_services[] = {ActorNativeService::graph, ActorNativeService::model};
constexpr std::uint32_t d_109_func_802DD1D4_6C4FB4_functions[] = {0x802dd0e0u, 0x802dd2bcu, 0x802dd398u};
constexpr ActorNativeService d_109_func_802DD1D4_6C4FB4_services[] = {ActorNativeService::distance, ActorNativeService::send_command_link};
constexpr std::uint32_t b_func_802DD1D4_6C4FB4_callbacks[] = {0x802dd0e0u, 0x802dd2bcu, 0x802dd398u};
constexpr std::uint32_t d_132_func_802DD2BC_6C509C_functions[] = {0x802dd4acu, 0x802dd53cu, 0x802ddc94u};
constexpr std::uint32_t d_132_func_802DD2BC_6C509C_graphs[] = {0x802e3c28u};
constexpr std::uint32_t d_132_func_802DD2BC_6C509C_headers[] = {0x802e39a4u};
constexpr ActorNativeService d_132_func_802DD2BC_6C509C_services[] = {ActorNativeService::model};
constexpr std::uint32_t b_func_802DD2BC_6C509C_callbacks[] = {0x802dd4acu, 0x802dd53cu, 0x802ddc94u};
constexpr ActorNativeService d_147_func_802DD398_6C5178_services[] = {ActorNativeService::model};
constexpr std::uint32_t b_func_802DD398_6C5178_callbacks[] = {0x802dd4acu, 0x802dd53cu, 0x802de0dcu, 0x802de524u};
constexpr ActorNativeService d_170_func_802DD4AC_6C528C_services[] = {ActorNativeService::model, ActorNativeService::model_read, ActorNativeService::model_store};
constexpr ActorNativeService d_185_func_802DD53C_6C531C_services[] = {ActorNativeService::block_position, ActorNativeService::current_block, ActorNativeService::ground, ActorNativeService::model, ActorNativeService::model_read, ActorNativeService::model_store, ActorNativeService::path_duration};
constexpr std::uint32_t d_231_func_802DD6F0_6C54D0_graphs[] = {0x802e39e8u};
constexpr std::uint32_t d_231_func_802DD6F0_6C54D0_weighted_states[] = {0x802e3bc8u};
constexpr std::uint32_t d_231_func_802DD6F0_6C54D0_headers[] = {0x802e3800u};
constexpr ActorNativeService d_231_func_802DD6F0_6C54D0_services[] = {ActorNativeService::graph, ActorNativeService::weighted};
constexpr std::uint32_t d_241_func_802DD74C_6C552C_functions[] = {0x802dd7ccu};
constexpr std::uint32_t d_241_func_802DD74C_6C552C_graphs[] = {0x802e39e8u};
constexpr std::uint32_t d_241_func_802DD74C_6C552C_weighted_states[] = {0x802e3bc8u};
constexpr std::uint32_t d_241_func_802DD74C_6C552C_headers[] = {0x802e3814u};
constexpr ActorNativeService d_241_func_802DD74C_6C552C_services[] = {ActorNativeService::graph, ActorNativeService::weighted};
constexpr std::uint32_t b_func_802DD74C_6C552C_callbacks[] = {0x802dd7ccu};
constexpr std::uint32_t d_264_func_802DD828_6C5608_functions[] = {0x802dd8a8u};
constexpr std::uint32_t d_264_func_802DD828_6C5608_graphs[] = {0x802e39e8u};
constexpr std::uint32_t d_264_func_802DD828_6C5608_weighted_states[] = {0x802e3bc8u};
constexpr std::uint32_t d_264_func_802DD828_6C5608_headers[] = {0x802e3828u};
constexpr ActorNativeService d_264_func_802DD828_6C5608_services[] = {ActorNativeService::graph, ActorNativeService::weighted};
constexpr std::uint32_t b_func_802DD828_6C5608_callbacks[] = {0x802dd8a8u};
constexpr std::uint32_t d_287_func_802DD904_6C56E4_functions[] = {0x802dd6f0u};
constexpr std::uint32_t d_287_func_802DD904_6C56E4_headers[] = {0x802e383cu};
constexpr std::uint32_t b_func_802DD904_6C56E4_callbacks[] = {0x802dd6f0u};
constexpr std::uint32_t d_298_func_802DD96C_6C574C_functions[] = {0x802dd6f0u};
constexpr std::uint32_t d_298_func_802DD96C_6C574C_graphs[] = {0x802e39e8u};
constexpr std::uint32_t d_298_func_802DD96C_6C574C_headers[] = {0x802e383cu};
constexpr ActorNativeService d_298_func_802DD96C_6C574C_services[] = {ActorNativeService::graph};
constexpr std::uint32_t b_func_802DD96C_6C574C_callbacks[] = {0x802dd6f0u};
constexpr std::uint32_t d_309_func_802DD9DC_6C57BC_functions[] = {0x802dd6f0u, 0x802ddaa0u, 0x802ddaecu, 0x802ddb70u};
constexpr std::uint32_t d_309_func_802DD9DC_6C57BC_graphs[] = {0x802e3a38u};
constexpr std::uint32_t d_309_func_802DD9DC_6C57BC_headers[] = {0x802e3850u};
constexpr ActorNativeService d_309_func_802DD9DC_6C57BC_services[] = {ActorNativeService::graph};
constexpr std::uint32_t b_func_802DD9DC_6C57BC_callbacks[] = {0x802dd6f0u, 0x802ddaa0u, 0x802ddaecu, 0x802ddb70u};
constexpr std::uint32_t d_336_func_802DDAEC_6C58CC_headers[] = {0x802e3828u};
constexpr std::uint32_t d_349_func_802DDB70_6C5950_functions[] = {0x802dd6f0u, 0x802ddb70u, 0x802ddc28u};
constexpr std::uint32_t d_349_func_802DDB70_6C5950_graphs[] = {0x802e3a38u};
constexpr std::uint32_t d_349_func_802DDB70_6C5950_headers[] = {0x802e3864u};
constexpr ActorNativeService d_349_func_802DDB70_6C5950_services[] = {ActorNativeService::graph};
constexpr std::uint32_t b_func_802DDB70_6C5950_callbacks[] = {0x802dd6f0u, 0x802ddb70u, 0x802ddc28u};
constexpr std::uint32_t d_373_func_802DDC94_6C5A74_functions[] = {0x802ddcb8u};
constexpr std::uint32_t b_func_802DDC94_6C5A74_callbacks[] = {0x802ddcb8u};
constexpr std::uint32_t d_377_func_802DDCB8_6C5A98_functions[] = {0x802dd7ccu};
constexpr std::uint32_t d_377_func_802DDCB8_6C5A98_graphs[] = {0x802e3b28u};
constexpr std::uint32_t d_377_func_802DDCB8_6C5A98_weighted_states[] = {0x802e3be8u};
constexpr std::uint32_t d_377_func_802DDCB8_6C5A98_headers[] = {0x802e3940u};
constexpr ActorNativeService d_377_func_802DDCB8_6C5A98_services[] = {ActorNativeService::graph, ActorNativeService::weighted};
constexpr std::uint32_t b_func_802DDCB8_6C5A98_callbacks[] = {0x802dd7ccu};
constexpr std::uint32_t d_389_func_802DDD38_6C5B18_functions[] = {0x802dd8a8u};
constexpr std::uint32_t d_389_func_802DDD38_6C5B18_graphs[] = {0x802e3b28u};
constexpr std::uint32_t d_389_func_802DDD38_6C5B18_weighted_states[] = {0x802e3be8u};
constexpr std::uint32_t d_389_func_802DDD38_6C5B18_headers[] = {0x802e3954u};
constexpr ActorNativeService d_389_func_802DDD38_6C5B18_services[] = {ActorNativeService::graph, ActorNativeService::weighted};
constexpr std::uint32_t b_func_802DDD38_6C5B18_callbacks[] = {0x802dd8a8u};
constexpr std::uint32_t d_401_func_802DDDB8_6C5B98_functions[] = {0x802dd6f0u};
constexpr std::uint32_t d_401_func_802DDDB8_6C5B98_headers[] = {0x802e3878u};
constexpr std::uint32_t b_func_802DDDB8_6C5B98_callbacks[] = {0x802dd6f0u};
constexpr std::uint32_t d_412_func_802DDE20_6C5C00_functions[] = {0x802ddc94u};
constexpr std::uint32_t d_412_func_802DDE20_6C5C00_graphs[] = {0x802e3b28u};
constexpr std::uint32_t d_412_func_802DDE20_6C5C00_headers[] = {0x802e3968u};
constexpr ActorNativeService d_412_func_802DDE20_6C5C00_services[] = {ActorNativeService::graph};
constexpr std::uint32_t b_func_802DDE20_6C5C00_callbacks[] = {0x802ddc94u};
constexpr std::uint32_t d_423_func_802DDE90_6C5C70_functions[] = {0x802ddc94u, 0x802ddf54u, 0x802ddfa0u, 0x802de024u};
constexpr std::uint32_t d_423_func_802DDE90_6C5C70_graphs[] = {0x802e3a88u};
constexpr std::uint32_t d_423_func_802DDE90_6C5C70_headers[] = {0x802e397cu};
constexpr ActorNativeService d_423_func_802DDE90_6C5C70_services[] = {ActorNativeService::graph};
constexpr std::uint32_t b_func_802DDE90_6C5C70_callbacks[] = {0x802ddc94u, 0x802ddf54u, 0x802ddfa0u, 0x802de024u};
constexpr std::uint32_t d_450_func_802DDFA0_6C5D80_headers[] = {0x802e3954u};
constexpr std::uint32_t d_463_func_802DE024_6C5E04_functions[] = {0x802ddc28u, 0x802ddc94u, 0x802de024u};
constexpr std::uint32_t d_463_func_802DE024_6C5E04_graphs[] = {0x802e3a88u};
constexpr std::uint32_t d_463_func_802DE024_6C5E04_headers[] = {0x802e3990u};
constexpr ActorNativeService d_463_func_802DE024_6C5E04_services[] = {ActorNativeService::graph};
constexpr std::uint32_t b_func_802DE024_6C5E04_callbacks[] = {0x802ddc28u, 0x802ddc94u, 0x802de024u};
constexpr std::uint32_t d_480_func_802DE0DC_6C5EBC_functions[] = {0x802de100u};
constexpr std::uint32_t b_func_802DE0DC_6C5EBC_callbacks[] = {0x802de100u};
constexpr std::uint32_t d_484_func_802DE100_6C5EE0_functions[] = {0x802dd7ccu};
constexpr std::uint32_t d_484_func_802DE100_6C5EE0_graphs[] = {0x802e3b78u};
constexpr std::uint32_t d_484_func_802DE100_6C5EE0_weighted_states[] = {0x802e3c08u};
constexpr std::uint32_t d_484_func_802DE100_6C5EE0_headers[] = {0x802e38a0u};
constexpr ActorNativeService d_484_func_802DE100_6C5EE0_services[] = {ActorNativeService::graph, ActorNativeService::weighted};
constexpr std::uint32_t b_func_802DE100_6C5EE0_callbacks[] = {0x802dd7ccu};
constexpr std::uint32_t d_496_func_802DE180_6C5F60_functions[] = {0x802dd8a8u};
constexpr std::uint32_t d_496_func_802DE180_6C5F60_graphs[] = {0x802e3b78u};
constexpr std::uint32_t d_496_func_802DE180_6C5F60_weighted_states[] = {0x802e3c08u};
constexpr std::uint32_t d_496_func_802DE180_6C5F60_headers[] = {0x802e38b4u};
constexpr ActorNativeService d_496_func_802DE180_6C5F60_services[] = {ActorNativeService::graph, ActorNativeService::weighted};
constexpr std::uint32_t b_func_802DE180_6C5F60_callbacks[] = {0x802dd8a8u};
constexpr std::uint32_t d_508_func_802DE200_6C5FE0_functions[] = {0x802dd6f0u};
constexpr std::uint32_t d_508_func_802DE200_6C5FE0_headers[] = {0x802e3918u};
constexpr std::uint32_t b_func_802DE200_6C5FE0_callbacks[] = {0x802dd6f0u};
constexpr std::uint32_t d_519_func_802DE268_6C6048_functions[] = {0x802de0dcu};
constexpr std::uint32_t d_519_func_802DE268_6C6048_graphs[] = {0x802e3b78u};
constexpr std::uint32_t d_519_func_802DE268_6C6048_headers[] = {0x802e38c8u};
constexpr ActorNativeService d_519_func_802DE268_6C6048_services[] = {ActorNativeService::graph};
constexpr std::uint32_t b_func_802DE268_6C6048_callbacks[] = {0x802de0dcu};
constexpr std::uint32_t d_530_func_802DE2D8_6C60B8_functions[] = {0x802de0dcu, 0x802de39cu, 0x802de3e8u, 0x802de46cu};
constexpr std::uint32_t d_530_func_802DE2D8_6C60B8_graphs[] = {0x802e3ad8u};
constexpr std::uint32_t d_530_func_802DE2D8_6C60B8_headers[] = {0x802e38dcu};
constexpr ActorNativeService d_530_func_802DE2D8_6C60B8_services[] = {ActorNativeService::graph};
constexpr std::uint32_t b_func_802DE2D8_6C60B8_callbacks[] = {0x802de0dcu, 0x802de39cu, 0x802de3e8u, 0x802de46cu};
constexpr std::uint32_t d_557_func_802DE3E8_6C61C8_headers[] = {0x802e38b4u};
constexpr std::uint32_t d_570_func_802DE46C_6C624C_functions[] = {0x802ddc28u, 0x802de0dcu, 0x802de46cu};
constexpr std::uint32_t d_570_func_802DE46C_6C624C_graphs[] = {0x802e3ad8u};
constexpr std::uint32_t d_570_func_802DE46C_6C624C_headers[] = {0x802e38f0u};
constexpr ActorNativeService d_570_func_802DE46C_6C624C_services[] = {ActorNativeService::graph};
constexpr std::uint32_t b_func_802DE46C_6C624C_callbacks[] = {0x802ddc28u, 0x802de0dcu, 0x802de46cu};
constexpr std::uint32_t d_587_func_802DE524_6C6304_headers[] = {0x802e3904u};
constexpr ActorNativeService d_587_func_802DE524_6C6304_services[] = {ActorNativeService::block_position, ActorNativeService::camera, ActorNativeService::camera_read, ActorNativeService::camera_store, ActorNativeService::current_block, ActorNativeService::model, ActorNativeService::path_count, ActorNativeService::path_point, ActorNativeService::player_object, ActorNativeService::send_command};
constexpr std::uint32_t b_func_802DE524_6C6304_callbacks[] = {0x802dd4acu, 0x802dd53cu, 0x802de870u};
constexpr ActorNativeService d_670_func_802DE870_6C6650_services[] = {ActorNativeService::resume_processes, ActorNativeService::send_command_link};
constexpr std::uint32_t d_19_func_802DE900_6C66E0_graphs[] = {0x802e3d34u};
constexpr std::uint32_t d_19_func_802DE900_6C66E0_headers[] = {0x802e3d20u};
constexpr ActorNativeService d_19_func_802DE900_6C66E0_services[] = {ActorNativeService::graph, ActorNativeService::model};
constexpr std::uint32_t d_26_func_802DE958_6C6738_headers[] = {0x802e3cf8u, 0x802e3d0cu};
constexpr ActorNativeService d_26_func_802DE958_6C6738_services[] = {ActorNativeService::model};
constexpr ActorNativeService d_34_func_802DE9C0_6C67A0_services[] = {ActorNativeService::reference_assign};
constexpr ActorNativeService d_40_func_802DEA00_6C67E0_services[] = {ActorNativeService::model};
constexpr std::uint32_t b_func_802DEA00_6C67E0_callbacks[] = {0x802deb00u, 0x802debb8u};
constexpr ActorNativeService d_62_func_802DEB00_6C68E0_services[] = {ActorNativeService::model};
constexpr ActorNativeService d_79_func_802DEBB8_6C6998_services[] = {ActorNativeService::model};
constexpr std::uint32_t d_33_func_802DEC90_6C6A70_functions[] = {0x802dedd4u};
constexpr ActorNativeService d_33_func_802DEC90_6C6A70_services[] = {ActorNativeService::ground, ActorNativeService::model};
constexpr std::uint32_t b_func_802DEC90_6C6A70_callbacks[] = {0x802dedd4u};
constexpr std::uint32_t d_40_func_802DED0C_6C6AEC_functions[] = {0x802ded0cu};
constexpr std::uint32_t d_40_func_802DED0C_6C6AEC_graphs[] = {0x802e3e40u};
constexpr std::uint32_t d_40_func_802DED0C_6C6AEC_headers[] = {0x802e3e04u};
constexpr ActorNativeService d_40_func_802DED0C_6C6AEC_services[] = {ActorNativeService::graph, ActorNativeService::model};
constexpr std::uint32_t b_func_802DED0C_6C6AEC_callbacks[] = {0x802ded0cu};
constexpr std::uint32_t d_47_func_802DED68_6C6B48_functions[] = {0x802dedd4u, 0x802deea0u};
constexpr std::uint32_t d_47_func_802DED68_6C6B48_headers[] = {0x802e3e18u};
constexpr ActorNativeService d_47_func_802DED68_6C6B48_services[] = {ActorNativeService::model};
constexpr std::uint32_t b_func_802DED68_6C6B48_callbacks[] = {0x802dedd4u, 0x802deea0u};
constexpr std::uint32_t d_55_func_802DEDD4_6C6BB4_graphs[] = {0x802e3e70u};
constexpr std::uint32_t d_55_func_802DEDD4_6C6BB4_headers[] = {0x802e3e2cu};
constexpr ActorNativeService d_55_func_802DEDD4_6C6BB4_services[] = {ActorNativeService::graph, ActorNativeService::model};
constexpr std::uint32_t d_62_func_802DEE2C_6C6C0C_functions[] = {0x802ded0cu, 0x802def28u};
constexpr std::uint32_t d_62_func_802DEE2C_6C6C0C_graphs[] = {0x802e3e40u};
constexpr std::uint32_t d_62_func_802DEE2C_6C6C0C_headers[] = {0x802e3e18u};
constexpr ActorNativeService d_62_func_802DEE2C_6C6C0C_services[] = {ActorNativeService::graph, ActorNativeService::model};
constexpr std::uint32_t b_func_802DEE2C_6C6C0C_callbacks[] = {0x802ded0cu, 0x802def28u};
constexpr ActorNativeService d_70_func_802DEEA0_6C6C80_services[] = {ActorNativeService::model};
constexpr ActorNativeService d_78_func_802DEF28_6C6D08_services[] = {ActorNativeService::model};
constexpr ActorNativeService d_90_func_802DEFC0_6C6DA0_services[] = {ActorNativeService::model};
constexpr std::uint32_t d_106_func_802DF078_6C6E58_functions[] = {0x802defc0u, 0x802df154u};
constexpr std::uint32_t b_func_802DF078_6C6E58_callbacks[] = {0x802defc0u, 0x802df154u};
constexpr ActorNativeService d_112_func_802DF0B8_6C6E98_services[] = {ActorNativeService::model};
constexpr ActorNativeService d_122_func_802DF104_6C6EE4_services[] = {ActorNativeService::model};
constexpr std::uint32_t d_132_func_802DF154_6C6F34_graphs[] = {0x802e3f6cu};
constexpr std::uint32_t d_132_func_802DF154_6C6F34_headers[] = {0x802e3ef4u};
constexpr ActorNativeService d_132_func_802DF154_6C6F34_services[] = {ActorNativeService::graph, ActorNativeService::model};
constexpr std::uint32_t d_144_func_802DF1F0_6C6FD0_functions[] = {0x802df154u};
constexpr std::uint32_t d_144_func_802DF1F0_6C6FD0_graphs[] = {0x802e3f6cu};
constexpr std::uint32_t d_144_func_802DF1F0_6C6FD0_headers[] = {0x802e3ef4u};
constexpr ActorNativeService d_144_func_802DF1F0_6C6FD0_services[] = {ActorNativeService::animation_speed, ActorNativeService::graph, ActorNativeService::model, ActorNativeService::model_speed};
constexpr std::uint32_t b_func_802DF1F0_6C6FD0_callbacks[] = {0x802df154u};
constexpr std::uint32_t d_165_func_802DF360_6C7140_functions[] = {0x802df104u, 0x802df5b8u};
constexpr std::uint32_t d_165_func_802DF360_6C7140_graphs[] = {0x802e3fccu};
constexpr std::uint32_t d_165_func_802DF360_6C7140_headers[] = {0x802e3f44u};
constexpr ActorNativeService d_165_func_802DF360_6C7140_services[] = {ActorNativeService::graph, ActorNativeService::model};
constexpr std::uint32_t b_func_802DF360_6C7140_callbacks[] = {0x802df104u, 0x802df5b8u};
constexpr std::uint32_t d_177_func_802DF404_6C71E4_functions[] = {0x802df560u, 0x802df5b8u};
constexpr std::uint32_t d_177_func_802DF404_6C71E4_graphs[] = {0x802e403cu};
constexpr std::uint32_t d_177_func_802DF404_6C71E4_headers[] = {0x802e3f08u};
constexpr ActorNativeService d_177_func_802DF404_6C71E4_services[] = {ActorNativeService::graph, ActorNativeService::model};
constexpr std::uint32_t b_func_802DF404_6C71E4_callbacks[] = {0x802df560u, 0x802df5b8u};
constexpr std::uint32_t d_185_func_802DF478_6C7258_functions[] = {0x802df560u, 0x802df5b8u};
constexpr std::uint32_t d_185_func_802DF478_6C7258_graphs[] = {0x802e400cu};
constexpr std::uint32_t d_185_func_802DF478_6C7258_headers[] = {0x802e3f1cu};
constexpr ActorNativeService d_185_func_802DF478_6C7258_services[] = {ActorNativeService::graph, ActorNativeService::model};
constexpr std::uint32_t b_func_802DF478_6C7258_callbacks[] = {0x802df560u, 0x802df5b8u};
constexpr std::uint32_t d_193_func_802DF4EC_6C72CC_functions[] = {0x802df560u, 0x802df5b8u};
constexpr std::uint32_t d_193_func_802DF4EC_6C72CC_graphs[] = {0x802e406cu};
constexpr std::uint32_t d_193_func_802DF4EC_6C72CC_headers[] = {0x802e3f30u};
constexpr ActorNativeService d_193_func_802DF4EC_6C72CC_services[] = {ActorNativeService::graph, ActorNativeService::model};
constexpr std::uint32_t b_func_802DF4EC_6C72CC_callbacks[] = {0x802df560u, 0x802df5b8u};
constexpr ActorNativeService d_201_func_802DF560_6C7340_services[] = {ActorNativeService::model};
constexpr std::uint32_t d_209_func_802DF5B8_6C7398_functions[] = {0x802df0b8u, 0x802df154u};
constexpr std::uint32_t d_209_func_802DF5B8_6C7398_headers[] = {0x802e3f58u};
constexpr ActorNativeService d_209_func_802DF5B8_6C7398_services[] = {ActorNativeService::model};
constexpr std::uint32_t b_func_802DF5B8_6C7398_callbacks[] = {0x802df0b8u, 0x802df154u};
constexpr std::uint32_t d_30_func_802DF670_6C7450_functions[] = {0x802df6bcu, 0x802dfa30u};
constexpr std::uint32_t b_func_802DF670_6C7450_callbacks[] = {0x802df6bcu, 0x802dfa30u};
constexpr std::uint32_t d_39_func_802DF6BC_6C749C_graphs[] = {0x802e42a4u};
constexpr ActorNativeService d_39_func_802DF6BC_6C749C_services[] = {ActorNativeService::graph};
constexpr std::uint32_t d_49_func_802DF710_6C74F0_functions[] = {0x802df758u, 0x802df7d8u};
constexpr std::uint32_t b_func_802DF710_6C74F0_callbacks[] = {0x802df758u, 0x802df7d8u};
constexpr std::uint32_t d_57_func_802DF758_6C7538_functions[] = {0x802df6bcu};
constexpr std::uint32_t d_57_func_802DF758_6C7538_graphs[] = {0x802e4234u};
constexpr std::uint32_t d_57_func_802DF758_6C7538_headers[] = {0x802e4208u};
constexpr ActorNativeService d_57_func_802DF758_6C7538_services[] = {ActorNativeService::graph, ActorNativeService::send_command};
constexpr std::uint32_t b_func_802DF758_6C7538_callbacks[] = {0x802df6bcu};
constexpr std::uint32_t d_80_func_802DF844_6C7624_functions[] = {0x802df758u};
constexpr std::uint32_t d_80_func_802DF844_6C7624_graphs[] = {0x802e4234u};
constexpr std::uint32_t d_80_func_802DF844_6C7624_headers[] = {0x802e4190u};
constexpr ActorNativeService d_80_func_802DF844_6C7624_services[] = {ActorNativeService::graph};
constexpr std::uint32_t b_func_802DF844_6C7624_callbacks[] = {0x802df758u};
constexpr std::uint32_t d_93_func_802DF8C4_6C76A4_functions[] = {0x802df91cu, 0x802df9ccu};
constexpr std::uint32_t b_func_802DF8C4_6C76A4_callbacks[] = {0x802df91cu, 0x802df9ccu};
constexpr std::uint32_t d_104_func_802DF91C_6C76FC_functions[] = {0x802df758u, 0x802df91cu};
constexpr std::uint32_t d_104_func_802DF91C_6C76FC_graphs[] = {0x802e4234u};
constexpr std::uint32_t d_104_func_802DF91C_6C76FC_headers[] = {0x802e4140u};
constexpr ActorNativeService d_104_func_802DF91C_6C76FC_services[] = {ActorNativeService::graph, ActorNativeService::model};
constexpr std::uint32_t b_func_802DF91C_6C76FC_callbacks[] = {0x802df758u, 0x802df91cu};
constexpr std::uint32_t d_132_func_802DFA30_6C7810_functions[] = {0x802dfa5cu};
constexpr ActorNativeService d_132_func_802DFA30_6C7810_services[] = {ActorNativeService::reference_assign};
constexpr std::uint32_t b_func_802DFA30_6C7810_callbacks[] = {0x802dfa5cu};
constexpr std::uint32_t d_140_func_802DFA5C_6C783C_functions[] = {0x802dfa5cu, 0x802dfb74u, 0x802dfc38u};
constexpr std::uint32_t d_140_func_802DFA5C_6C783C_graphs[] = {0x802e42c4u, 0x802e4314u};
constexpr std::uint32_t d_140_func_802DFA5C_6C783C_headers[] = {0x802e4140u};
constexpr ActorNativeService d_140_func_802DFA5C_6C783C_services[] = {ActorNativeService::graph, ActorNativeService::rng_range};
constexpr std::uint32_t b_func_802DFA5C_6C783C_callbacks[] = {0x802dfa5cu, 0x802dfb74u, 0x802dfc38u};
constexpr ActorNativeService d_164_func_802DFB74_6C7954_services[] = {ActorNativeService::ground, ActorNativeService::model};
constexpr ActorNativeService d_186_func_802DFC38_6C7A18_services[] = {ActorNativeService::model};
constexpr std::uint32_t d_203_func_802DFCD0_6C7AB0_functions[] = {0x802dfc38u};
constexpr std::uint32_t d_203_func_802DFCD0_6C7AB0_graphs[] = {0x802e4334u};
constexpr std::uint32_t d_203_func_802DFCD0_6C7AB0_headers[] = {0x802e4190u};
constexpr ActorNativeService d_203_func_802DFCD0_6C7AB0_services[] = {ActorNativeService::graph, ActorNativeService::send_command_link};
constexpr std::uint32_t b_func_802DFCD0_6C7AB0_callbacks[] = {0x802dfc38u};
constexpr std::uint32_t d_218_func_802DFD78_6C7B58_functions[] = {0x802dfdf8u, 0x802dfe4cu};
constexpr ActorNativeService d_218_func_802DFD78_6C7B58_services[] = {ActorNativeService::ground, ActorNativeService::model};
constexpr std::uint32_t b_func_802DFD78_6C7B58_callbacks[] = {0x802dfdf8u, 0x802dfe4cu};
constexpr std::uint32_t d_232_func_802DFDF8_6C7BD8_functions[] = {0x802dfdf8u};
constexpr std::uint32_t d_232_func_802DFDF8_6C7BD8_headers[] = {0x802e4140u};
constexpr std::uint32_t b_func_802DFDF8_6C7BD8_callbacks[] = {0x802dfdf8u};
constexpr ActorNativeService d_242_func_802DFE4C_6C7C2C_services[] = {ActorNativeService::sound_params};
constexpr std::uint32_t d_10_func_802DFEF0_6C7CD0_headers[] = {0x802e43b0u};
constexpr ActorNativeService d_10_func_802DFEF0_6C7CD0_services[] = {ActorNativeService::model};
constexpr ActorNativeService d_23_func_802DFF94_6C7D74_services[] = {ActorNativeService::add_one, ActorNativeService::current_block, ActorNativeService::definition, ActorNativeService::model, ActorNativeService::model_store};
constexpr std::uint32_t d_28_func_802E0070_6C7E50_functions[] = {0x802e0094u};
constexpr std::uint32_t b_func_802E0070_6C7E50_callbacks[] = {0x802e0094u};
constexpr std::uint32_t d_32_func_802E0094_6C7E74_graphs[] = {0x802e4478u};
constexpr std::uint32_t d_32_func_802E0094_6C7E74_headers[] = {0x802e4420u};
constexpr ActorNativeService d_32_func_802E0094_6C7E74_services[] = {ActorNativeService::graph};
constexpr std::uint32_t d_42_func_802E00EC_6C7ECC_functions[] = {0x802e03c0u};
constexpr std::uint32_t d_42_func_802E00EC_6C7ECC_headers[] = {0x802e4434u, 0x802e4448u, 0x802e445cu};
constexpr ActorNativeService d_42_func_802E00EC_6C7ECC_services[] = {ActorNativeService::player_object, ActorNativeService::send_command, ActorNativeService::sound};
constexpr std::uint32_t b_func_802E00EC_6C7ECC_callbacks[] = {0x802e03c0u};
constexpr ActorNativeService d_68_func_802E01F4_6C7FD4_services[] = {ActorNativeService::bgm_volume};
constexpr ActorNativeService d_89_func_802E0334_6C8114_services[] = {ActorNativeService::sound, ActorNativeService::sound_active, ActorNativeService::sound_stop};
constexpr ActorNativeService d_102_func_802E03C0_6C81A0_services[] = {ActorNativeService::animation_callback, ActorNativeService::camera, ActorNativeService::camera_animation, ActorNativeService::camera_process, ActorNativeService::camera_speed, ActorNativeService::pause_processes, ActorNativeService::player_animation, ActorNativeService::player_model_init, ActorNativeService::player_object, ActorNativeService::remove_flying_items, ActorNativeService::reset_messages, ActorNativeService::send_command, ActorNativeService::sky_speed, ActorNativeService::sound_stop_all};
constexpr std::uint32_t b_func_802E03C0_6C81A0_callbacks[] = {0x802e01f4u, 0x802e0334u};
constexpr std::uint32_t d_11_func_802E05C0_6C83A0_graphs[] = {0x802e4518u};
constexpr std::uint32_t d_11_func_802E05C0_6C83A0_headers[] = {0x802e44f0u};
constexpr ActorNativeService d_11_func_802E05C0_6C83A0_services[] = {ActorNativeService::graph};
constexpr std::uint32_t d_21_func_802E0618_6C83F8_headers[] = {0x802e44f0u, 0x802e4504u};
constexpr std::uint32_t d_11_func_802E06D0_6C84B0_graphs[] = {0x802e45a8u};
constexpr std::uint32_t d_11_func_802E06D0_6C84B0_headers[] = {0x802e4580u};
constexpr ActorNativeService d_11_func_802E06D0_6C84B0_services[] = {ActorNativeService::graph};
constexpr std::uint32_t d_21_func_802E0728_6C8508_headers[] = {0x802e4580u, 0x802e4594u};
constexpr std::uint32_t d_11_func_802E07F0_6C85D0_graphs[] = {0x802e4638u};
constexpr std::uint32_t d_11_func_802E07F0_6C85D0_headers[] = {0x802e4610u};
constexpr ActorNativeService d_11_func_802E07F0_6C85D0_services[] = {ActorNativeService::graph};
constexpr std::uint32_t d_21_func_802E0848_6C8628_headers[] = {0x802e4610u, 0x802e4624u};

constexpr std::uint32_t d_63_func_802E09A8_6C8788_functions[] = {0x802e0900u, 0x802e0a48u, 0x802e115cu, 0x802e1570u};
constexpr std::uint32_t b_func_802E09A8_6C8788_callbacks[] = {0x802e0900u, 0x802e0a48u, 0x802e115cu, 0x802e1570u};
constexpr std::uint32_t d_80_func_802E0A48_6C8828_graphs[] = {0x802e4838u};
constexpr std::uint32_t d_80_func_802E0A48_6C8828_weighted_states[] = {0x802e4978u};
constexpr std::uint32_t d_80_func_802E0A48_6C8828_headers[] = {0x802e46d0u};
constexpr ActorNativeService d_80_func_802E0A48_6C8828_services[] = {ActorNativeService::graph, ActorNativeService::weighted};
constexpr std::uint32_t d_91_func_802E0AB8_6C8898_graphs[] = {0x802e4838u};
constexpr std::uint32_t d_91_func_802E0AB8_6C8898_weighted_states[] = {0x802e4978u};
constexpr std::uint32_t d_91_func_802E0AB8_6C8898_headers[] = {0x802e46e4u};
constexpr ActorNativeService d_91_func_802E0AB8_6C8898_services[] = {ActorNativeService::graph, ActorNativeService::weighted};
constexpr std::uint32_t d_102_func_802E0B28_6C8908_graphs[] = {0x802e4838u};
constexpr std::uint32_t d_102_func_802E0B28_6C8908_weighted_states[] = {0x802e4978u};
constexpr std::uint32_t d_102_func_802E0B28_6C8908_headers[] = {0x802e46f8u};
constexpr ActorNativeService d_102_func_802E0B28_6C8908_services[] = {ActorNativeService::graph, ActorNativeService::weighted};
constexpr std::uint32_t d_113_func_802E0B98_6C8978_functions[] = {0x802e0a48u, 0x802e0c3cu};
constexpr std::uint32_t d_113_func_802E0B98_6C8978_graphs[] = {0x802e4838u};
constexpr std::uint32_t d_113_func_802E0B98_6C8978_weighted_states[] = {0x802e4978u};
constexpr std::uint32_t d_113_func_802E0B98_6C8978_headers[] = {0x802e470cu};
constexpr ActorNativeService d_113_func_802E0B98_6C8978_services[] = {ActorNativeService::graph, ActorNativeService::weighted};
constexpr std::uint32_t b_func_802E0B98_6C8978_callbacks[] = {0x802e0a48u, 0x802e0c3cu};
constexpr std::uint32_t d_140_func_802E0C98_6C8A78_functions[] = {0x802e0a48u};
constexpr std::uint32_t d_140_func_802E0C98_6C8A78_headers[] = {0x802e4748u, 0x802e475cu, 0x802e4770u};
constexpr std::uint32_t b_func_802E0C98_6C8A78_callbacks[] = {0x802e0a48u};
constexpr std::uint32_t d_159_func_802E0D44_6C8B24_functions[] = {0x802e0a48u};
constexpr std::uint32_t d_159_func_802E0D44_6C8B24_headers[] = {0x802e4734u};
constexpr std::uint32_t b_func_802E0D44_6C8B24_callbacks[] = {0x802e0a48u};
constexpr std::uint32_t d_170_func_802E0DAC_6C8B8C_functions[] = {0x802e0a48u, 0x802e0e20u};
constexpr std::uint32_t d_170_func_802E0DAC_6C8B8C_graphs[] = {0x802e4938u};
constexpr std::uint32_t d_170_func_802E0DAC_6C8B8C_headers[] = {0x802e4720u};
constexpr ActorNativeService d_170_func_802E0DAC_6C8B8C_services[] = {ActorNativeService::graph};
constexpr std::uint32_t b_func_802E0DAC_6C8B8C_callbacks[] = {0x802e0a48u, 0x802e0e20u};
constexpr std::uint32_t d_192_func_802E0E7C_6C8C5C_functions[] = {0x802e0a48u, 0x802e0f04u, 0x802e0f9cu};
constexpr std::uint32_t d_192_func_802E0E7C_6C8C5C_graphs[] = {0x802e4838u};
constexpr ActorNativeService d_192_func_802E0E7C_6C8C5C_services[] = {ActorNativeService::graph};
constexpr std::uint32_t b_func_802E0E7C_6C8C5C_callbacks[] = {0x802e0a48u, 0x802e0f04u, 0x802e0f9cu};
constexpr std::uint32_t d_206_func_802E0F04_6C8CE4_headers[] = {0x802e470cu, 0x802e4720u};
constexpr ActorNativeService d_206_func_802E0F04_6C8CE4_services[] = {ActorNativeService::model};
constexpr std::uint32_t d_222_func_802E0F9C_6C8D7C_functions[] = {0x802e0f9cu, 0x802e1030u};
constexpr std::uint32_t d_222_func_802E0F9C_6C8D7C_graphs[] = {0x802e48b8u};
constexpr std::uint32_t d_222_func_802E0F9C_6C8D7C_weighted_states[] = {0x802e4978u};
constexpr std::uint32_t d_222_func_802E0F9C_6C8D7C_headers[] = {0x802e4784u};
constexpr ActorNativeService d_222_func_802E0F9C_6C8D7C_services[] = {ActorNativeService::graph, ActorNativeService::weighted};
constexpr std::uint32_t b_func_802E0F9C_6C8D7C_callbacks[] = {0x802e0f9cu, 0x802e1030u};
constexpr std::uint32_t d_246_func_802E109C_6C8E7C_functions[] = {0x802e0a48u, 0x802e1110u};
constexpr std::uint32_t d_246_func_802E109C_6C8E7C_graphs[] = {0x802e4908u};
constexpr std::uint32_t d_246_func_802E109C_6C8E7C_headers[] = {0x802e470cu};
constexpr ActorNativeService d_246_func_802E109C_6C8E7C_services[] = {ActorNativeService::graph};
constexpr std::uint32_t b_func_802E109C_6C8E7C_callbacks[] = {0x802e0a48u, 0x802e1110u};
constexpr std::uint32_t d_267_func_802E115C_6C8F3C_functions[] = {0x802e1194u};
constexpr std::uint32_t b_func_802E115C_6C8F3C_callbacks[] = {0x802e1194u};
constexpr std::uint32_t d_275_func_802E1194_6C8F74_graphs[] = {0x802e49e0u};
constexpr ActorNativeService d_275_func_802E1194_6C8F74_services[] = {ActorNativeService::graph};
constexpr std::uint32_t d_284_func_802E11D4_6C8FB4_functions[] = {0x802e1194u, 0x802e12d8u};
constexpr ActorNativeService d_284_func_802E11D4_6C8FB4_services[] = {ActorNativeService::block_position, ActorNativeService::current_block, ActorNativeService::model, ActorNativeService::path_count, ActorNativeService::path_point};
constexpr std::uint32_t b_func_802E11D4_6C8FB4_callbacks[] = {0x802e1194u, 0x802e12d8u};
constexpr std::uint32_t d_309_func_802E12D8_6C90B8_functions[] = {0x802e0a48u, 0x802e0e7cu, 0x802e1374u};
constexpr std::uint32_t d_309_func_802E12D8_6C90B8_headers[] = {0x802e4720u};
constexpr std::uint32_t b_func_802E12D8_6C90B8_callbacks[] = {0x802e0a48u, 0x802e0e7cu, 0x802e1374u};
constexpr ActorNativeService d_324_func_802E1374_6C9154_services[] = {ActorNativeService::path_parameter};
constexpr std::uint32_t d_338_func_802E13F4_6C91D4_functions[] = {0x802e142cu, 0x802e1488u};
constexpr std::uint32_t b_func_802E13F4_6C91D4_callbacks[] = {0x802e142cu, 0x802e1488u};
constexpr std::uint32_t d_343_func_802E142C_6C920C_functions[] = {0x802e13f4u};
constexpr std::uint32_t d_343_func_802E142C_6C920C_graphs[] = {0x802e49a0u};
constexpr std::uint32_t d_343_func_802E142C_6C920C_headers[] = {0x802e4720u};
constexpr ActorNativeService d_343_func_802E142C_6C920C_services[] = {ActorNativeService::graph};
constexpr std::uint32_t b_func_802E142C_6C920C_callbacks[] = {0x802e13f4u};
constexpr std::uint32_t d_364_func_802E14F4_6C92D4_functions[] = {0x802e142cu};
constexpr std::uint32_t d_364_func_802E14F4_6C92D4_graphs[] = {0x802e49a0u};
constexpr std::uint32_t d_364_func_802E14F4_6C92D4_headers[] = {0x802e4734u};
constexpr ActorNativeService d_364_func_802E14F4_6C92D4_services[] = {ActorNativeService::graph};
constexpr std::uint32_t b_func_802E14F4_6C92D4_callbacks[] = {0x802e142cu};
constexpr std::uint32_t d_376_func_802E1570_6C9350_functions[] = {0x802e1594u};
constexpr std::uint32_t b_func_802E1570_6C9350_callbacks[] = {0x802e1594u};
constexpr std::uint32_t d_380_func_802E1594_6C9374_functions[] = {0x802e1604u};
constexpr std::uint32_t d_380_func_802E1594_6C9374_graphs[] = {0x802e4a10u};
constexpr std::uint32_t d_380_func_802E1594_6C9374_headers[] = {0x802e46d0u};
constexpr ActorNativeService d_380_func_802E1594_6C9374_services[] = {ActorNativeService::graph};
constexpr std::uint32_t b_func_802E1594_6C9374_callbacks[] = {0x802e1604u};
constexpr ActorNativeService d_391_func_802E1604_6C93E4_services[] = {ActorNativeService::ground, ActorNativeService::model, ActorNativeService::model_read, ActorNativeService::model_store};
constexpr std::uint32_t d_404_func_802E1678_6C9458_functions[] = {0x802e1594u};
constexpr std::uint32_t d_404_func_802E1678_6C9458_graphs[] = {0x802e4a40u};
constexpr std::uint32_t d_404_func_802E1678_6C9458_headers[] = {0x802e4734u};
constexpr std::uint32_t b_func_802E1678_6C9458_callbacks[] = {0x802e1594u};
constexpr std::uint32_t d_416_func_802E171C_6C94FC_functions[] = {0x802e0a48u, 0x802e17d4u};
constexpr std::uint32_t d_416_func_802E171C_6C94FC_headers[] = {0x802e4748u, 0x802e475cu, 0x802e4770u};
constexpr std::uint32_t b_func_802E171C_6C94FC_callbacks[] = {0x802e0a48u, 0x802e17d4u};
constexpr ActorNativeService d_434_func_802E17D4_6C95B4_services[] = {ActorNativeService::item};
constexpr std::uint32_t d_32_func_802E18A0_6C9680_functions[] = {0x802e18c4u};
constexpr std::uint32_t b_func_802E18A0_6C9680_callbacks[] = {0x802e18c4u};
constexpr std::uint32_t d_36_func_802E18C4_6C96A4_graphs[] = {0x802e4b14u};
constexpr std::uint32_t d_36_func_802E18C4_6C96A4_headers[] = {0x802e4ae0u};
constexpr ActorNativeService d_36_func_802E18C4_6C96A4_services[] = {ActorNativeService::graph};
constexpr ActorNativeService d_46_func_802E191C_6C96FC_services[] = {ActorNativeService::camera_speed, ActorNativeService::player_animation_speed};
constexpr ActorNativeService d_53_func_802E1950_6C9730_services[] = {ActorNativeService::camera_animation, ActorNativeService::player_change_animation};
constexpr ActorNativeService d_87_func_802E1A4C_6C982C_services[] = {ActorNativeService::audio_cutscene, ActorNativeService::bgm_play, ActorNativeService::bgm_stop_all, ActorNativeService::bgm_volume, ActorNativeService::bgm_volume_smooth, ActorNativeService::sound_params, ActorNativeService::sound_stop, ActorNativeService::sound_volume_smooth};
constexpr std::uint32_t d_134_func_802E1CBC_6C9A9C_functions[] = {0x802e1a4cu};
constexpr ActorNativeService d_134_func_802E1CBC_6C9A9C_services[] = {ActorNativeService::create_helper, ActorNativeService::reference_assign, ActorNativeService::source_process};
constexpr ActorNativeService d_139_func_802E1D10_6C9AF0_services[] = {ActorNativeService::animation_callback, ActorNativeService::animation_speed, ActorNativeService::bgm_volume_smooth, ActorNativeService::camera, ActorNativeService::camera_process, ActorNativeService::delete_object, ActorNativeService::notice, ActorNativeService::pause_processes, ActorNativeService::player_model_init, ActorNativeService::player_object, ActorNativeService::progress_read, ActorNativeService::progress_write, ActorNativeService::reference_assign, ActorNativeService::remove_flying_items, ActorNativeService::reset_messages, ActorNativeService::resume_processes, ActorNativeService::send_command, ActorNativeService::sky_speed, ActorNativeService::sound_global_volume, ActorNativeService::transition_active, ActorNativeService::transition_color, ActorNativeService::transition_fade, ActorNativeService::transition_prepare};
constexpr ActorNativeService d_204_func_802E2084_6C9E64_services[] = {ActorNativeService::reference_assign};
constexpr std::uint32_t d_76_func_802E22E4_6CA0C4_functions[] = {0x802e2120u, 0x802e2194u, 0x802e222cu};
constexpr std::uint32_t b_func_802E22E4_6CA0C4_callbacks[] = {0x802e222cu};
constexpr std::uint32_t d_142_func_802E25C4_6CA3A4_functions[] = {0x802e20d0u, 0x802e22e4u};
constexpr LifecycleNativeService d_func_802D8800_6C05E0_lifecycle_services[] = {LifecycleNativeService::world_add};
constexpr LifecycleNativeService d_func_802D8824_6C0604_lifecycle_services[] = {LifecycleNativeService::world_change};
constexpr LifecycleNativeService d_func_802D8848_6C0628_lifecycle_services[] = {LifecycleNativeService::world_remove};
constexpr LifecycleNativeService d_func_802D886C_6C064C_lifecycle_services[] = {LifecycleNativeService::create_world, LifecycleNativeService::height_map, LifecycleNativeService::pokemon_registry, LifecycleNativeService::sky_follow};
constexpr LifecycleNativeService d_func_802D88F8_6C06D8_lifecycle_services[] = {LifecycleNativeService::bgm_stop_all, LifecycleNativeService::course_cleanup, LifecycleNativeService::destroy_world, LifecycleNativeService::detector_disable, LifecycleNativeService::environment_cleanup, LifecycleNativeService::exit_scene, LifecycleNativeService::remove_camera, LifecycleNativeService::sound_stop_all, LifecycleNativeService::transition_end};
constexpr LifecycleNativeService d_func_802D895C_6C073C_lifecycle_services[] = {LifecycleNativeService::bgm_channel_volume, LifecycleNativeService::flute_command};
constexpr LifecycleNativeService d_func_802D89B4_6C0794_lifecycle_services[] = {LifecycleNativeService::block_next, LifecycleNativeService::block_position, LifecycleNativeService::displace_items};
constexpr std::uint32_t d_func_802D8A48_6C0828_child_factories[] = {0x802da9a4u};
constexpr std::uint32_t d_func_802D8B34_6C0914_child_factories[] = {0x802da4b8u};
constexpr std::uint32_t d_func_802D8C24_6C0A04_child_factories[] = {0x802dd0a8u};
constexpr std::uint32_t d_func_802D8CF0_6C0AD0_child_factories[] = {0x802dceb0u};
constexpr std::uint32_t d_func_802D8DC4_6C0BA4_child_factories[] = {0x802dbb54u};
constexpr LifecycleNativeService d_func_802D8FCC_6C0DAC_lifecycle_services[] = {LifecycleNativeService::camera, LifecycleNativeService::fx_allocate, LifecycleNativeService::fx_bank, LifecycleNativeService::fx_camera, LifecycleNativeService::fx_dither, LifecycleNativeService::load_asset};
constexpr LifecycleNativeService d_func_802D90B8_6C0E98_lifecycle_services[] = {LifecycleNativeService::background, LifecycleNativeService::create_cameras, LifecycleNativeService::create_helper, LifecycleNativeService::depth_allocate, LifecycleNativeService::detector_create, LifecycleNativeService::detector_enable, LifecycleNativeService::end_callback, LifecycleNativeService::environment_init, LifecycleNativeService::fx_bank_count, LifecycleNativeService::fx_bank_reset, LifecycleNativeService::pause_callback, LifecycleNativeService::photo_reset, LifecycleNativeService::pokemons_init, LifecycleNativeService::screen, LifecycleNativeService::ui_bind};
constexpr LifecycleNativeService d_func_802D9210_6C0FF0_lifecycle_services[] = {LifecycleNativeService::intervals, LifecycleNativeService::near_clipping, LifecycleNativeService::notice, LifecycleNativeService::photo_count};
constexpr LifecycleNativeService d_func_802D92B0_6C1090_lifecycle_services[] = {LifecycleNativeService::bgm_play, LifecycleNativeService::bgm_stop, LifecycleNativeService::bgm_volume, LifecycleNativeService::sound_params};
constexpr std::uint32_t d_func_802DA9E0_6C27C0_child_factories[] = {0x802da8ccu};
constexpr std::uint32_t d_func_802DAACC_6C28AC_child_factories[] = {0x802db064u};
constexpr std::uint32_t d_func_802DFF94_6C7D74_child_factories[] = {0x802dff5cu};


constexpr LifecycleNativeService d_func_802E2120_6C9F00_lifecycle_services[] = {LifecycleNativeService::effect_attach, LifecycleNativeService::effect_create};
constexpr LifecycleNativeService d_func_802E2194_6C9F74_lifecycle_services[] = {LifecycleNativeService::delete_object, LifecycleNativeService::sound_active, LifecycleNativeService::sound_stop};
constexpr LifecycleNativeService d_func_802E222C_6CA00C_lifecycle_services[] = {LifecycleNativeService::sound_params};
constexpr LifecycleNativeService d_func_802E22E4_6CA0C4_lifecycle_services[] = {LifecycleNativeService::animation_callback, LifecycleNativeService::camera, LifecycleNativeService::camera_animation, LifecycleNativeService::camera_process, LifecycleNativeService::camera_read, LifecycleNativeService::camera_speed, LifecycleNativeService::camera_store, LifecycleNativeService::input_pressed, LifecycleNativeService::player_animation, LifecycleNativeService::source_process};
constexpr LifecycleNativeService d_func_802E25C4_6CA3A4_lifecycle_services[] = {LifecycleNativeService::player_model_init, LifecycleNativeService::reference_assign, LifecycleNativeService::source_process};
const actor::BehaviorCallbackBinding callbacks[] = {
    {0x802d8a48u, func_802D8A48_6C0828, {}, {}, {}},
    {0x802d8b34u, func_802D8B34_6C0914, {}, {}, {}},
    {0x802d8c24u, func_802D8C24_6C0A04, {}, {}, {}},
    {0x802d8cf0u, func_802D8CF0_6C0AD0, {}, {}, {}},
    {0x802d8dc4u, func_802D8DC4_6C0BA4, {}, {}, {}},
    {0x802d9664u, func_802D9664_6C1444, b_func_802D9664_6C1444_callbacks, d_65_func_802D9664_6C1444_graphs, d_65_func_802D9664_6C1444_weighted_states},
    {0x802d96fcu, func_802D96FC_6C14DC, {}, d_79_func_802D96FC_6C14DC_graphs, d_79_func_802D96FC_6C14DC_weighted_states},
    {0x802d976cu, func_802D976C_6C154C, {}, d_90_func_802D976C_6C154C_graphs, d_90_func_802D976C_6C154C_weighted_states},
    {0x802d9808u, func_802D9808_6C15E8, b_func_802D9808_6C15E8_callbacks, d_104_func_802D9808_6C15E8_graphs, {}},
    {0x802d990cu, func_802D990C_6C16EC, {}, {}, {}},
    {0x802d9990u, func_802D9990_6C1770, b_func_802D9990_6C1770_callbacks, d_141_func_802D9990_6C1770_graphs, {}},
    {0x802d9a0cu, func_802D9A0C_6C17EC, {}, {}, {}},
    {0x802d9a58u, func_802D9A58_6C1838, {}, {}, {}},
    {0x802d9ac4u, func_802D9AC4_6C18A4, b_func_802D9AC4_6C18A4_callbacks, {}, {}},
    {0x802d9c8cu, func_802D9C8C_6C1A6C, {}, {}, {}},
    {0x802d9d7cu, func_802D9D7C_6C1B5C, {}, {}, {}},
    {0x802d9dd8u, func_802D9DD8_6C1BB8, {}, {}, {}},
    {0x802d9f80u, func_802D9F80_6C1D60, {}, d_24_func_802D9F80_6C1D60_graphs, d_24_func_802D9F80_6C1D60_weighted_states},
    {0x802d9fdcu, func_802D9FDC_6C1DBC, {}, d_34_func_802D9FDC_6C1DBC_graphs, d_34_func_802D9FDC_6C1DBC_weighted_states},
    {0x802da04cu, func_802DA04C_6C1E2C, {}, d_45_func_802DA04C_6C1E2C_graphs, d_45_func_802DA04C_6C1E2C_weighted_states},
    {0x802da0bcu, func_802DA0BC_6C1E9C, {}, d_56_func_802DA0BC_6C1E9C_graphs, d_56_func_802DA0BC_6C1E9C_weighted_states},
    {0x802da12cu, func_802DA12C_6C1F0C, b_func_802DA12C_6C1F0C_callbacks, d_67_func_802DA12C_6C1F0C_graphs, d_67_func_802DA12C_6C1F0C_weighted_states},
    {0x802da1acu, func_802DA1AC_6C1F8C, {}, {}, {}},
    {0x802da208u, func_802DA208_6C1FE8, b_func_802DA208_6C1FE8_callbacks, d_90_func_802DA208_6C1FE8_graphs, {}},
    {0x802da30cu, func_802DA30C_6C20EC, {}, {}, {}},
    {0x802da390u, func_802DA390_6C2170, b_func_802DA390_6C2170_callbacks, {}, {}},
    {0x802da3f8u, func_802DA3F8_6C21D8, b_func_802DA3F8_6C21D8_callbacks, d_132_func_802DA3F8_6C21D8_graphs, {}},
    {0x802da46cu, func_802DA46C_6C224C, {}, {}, {}},
    {0x802da500u, func_802DA500_6C22E0, b_func_802DA500_6C22E0_callbacks, d_17_func_802DA500_6C22E0_graphs, {}},
    {0x802da590u, func_802DA590_6C2370, b_func_802DA590_6C2370_callbacks, {}, {}},
    {0x802da620u, func_802DA620_6C2400, {}, {}, {}},
    {0x802da674u, func_802DA674_6C2454, {}, {}, {}},
    {0x802da73cu, func_802DA73C_6C251C, {}, {}, {}},
    {0x802da768u, func_802DA768_6C2548, {}, {}, {}},
    {0x802da814u, func_802DA814_6C25F4, {}, d_105_func_802DA814_6C25F4_graphs, {}},
    {0x802da864u, func_802DA864_6C2644, b_func_802DA864_6C2644_callbacks, d_114_func_802DA864_6C2644_graphs, {}},
    {0x802da910u, func_802DA910_6C26F0, {}, d_11_func_802DA910_6C26F0_graphs, {}},
    {0x802da9e0u, func_802DA9E0_6C27C0, {}, {}, {}},
    {0x802daaccu, func_802DAACC_6C28AC, {}, {}, {}},
    {0x802dabb8u, func_802DABB8_6C2998, {}, d_84_func_802DABB8_6C2998_graphs, {}},
    {0x802dac04u, func_802DAC04_6C29E4, {}, {}, d_93_func_802DAC04_6C29E4_weighted_states},
    {0x802dac28u, func_802DAC28_6C2A08, b_func_802DAC28_6C2A08_callbacks, {}, {}},
    {0x802daca8u, func_802DACA8_6C2A88, b_func_802DACA8_6C2A88_callbacks, {}, {}},
    {0x802dad60u, func_802DAD60_6C2B40, b_func_802DAD60_6C2B40_callbacks, d_125_func_802DAD60_6C2B40_graphs, {}},
    {0x802dadd8u, func_802DADD8_6C2BB8, b_func_802DADD8_6C2BB8_callbacks, {}, {}},
    {0x802dae68u, func_802DAE68_6C2C48, {}, {}, {}},
    {0x802daed4u, func_802DAED4_6C2CB4, {}, {}, {}},
    {0x802dafb8u, func_802DAFB8_6C2D98, {}, {}, {}},
    {0x802db0a0u, func_802DB0A0_6C2E80, b_func_802DB0A0_6C2E80_callbacks, {}, d_38_func_802DB0A0_6C2E80_weighted_states},
    {0x802db11cu, func_802DB11C_6C2EFC, b_func_802DB11C_6C2EFC_callbacks, {}, {}},
    {0x802db154u, func_802DB154_6C2F34, b_func_802DB154_6C2F34_callbacks, {}, {}},
    {0x802db18cu, func_802DB18C_6C2F6C, b_func_802DB18C_6C2F6C_callbacks, {}, {}},
    {0x802db1c4u, func_802DB1C4_6C2FA4, b_func_802DB1C4_6C2FA4_callbacks, {}, {}},
    {0x802db270u, func_802DB270_6C3050, {}, {}, {}},
    {0x802db388u, func_802DB388_6C3168, b_func_802DB388_6C3168_callbacks, d_116_func_802DB388_6C3168_graphs, {}},
    {0x802db41cu, func_802DB41C_6C31FC, {}, {}, {}},
    {0x802db468u, func_802DB468_6C3248, b_func_802DB468_6C3248_callbacks, {}, {}},
    {0x802db4a8u, func_802DB4A8_6C3288, b_func_802DB4A8_6C3288_callbacks, d_145_func_802DB4A8_6C3288_graphs, {}},
    {0x802db554u, func_802DB554_6C3334, {}, {}, {}},
    {0x802db5c0u, func_802DB5C0_6C33A0, {}, {}, {}},
    {0x802db630u, func_802DB630_6C3410, {}, {}, {}},
    {0x802db71cu, func_802DB71C_6C34FC, b_func_802DB71C_6C34FC_callbacks, {}, {}},
    {0x802db78cu, func_802DB78C_6C356C, b_func_802DB78C_6C356C_callbacks, {}, {}},
    {0x802db85cu, func_802DB85C_6C363C, {}, {}, {}},
    {0x802db8ecu, func_802DB8EC_6C36CC, {}, d_254_func_802DB8EC_6C36CC_graphs, {}},
    {0x802db93cu, func_802DB93C_6C371C, b_func_802DB93C_6C371C_callbacks, {}, {}},
    {0x802dbba0u, func_802DBBA0_6C3980, b_func_802DBBA0_6C3980_callbacks, {}, d_52_func_802DBBA0_6C3980_weighted_states},
    {0x802dbc5cu, func_802DBC5C_6C3A3C, b_func_802DBC5C_6C3A3C_callbacks, {}, {}},
    {0x802dbc94u, func_802DBC94_6C3A74, b_func_802DBC94_6C3A74_callbacks, {}, {}},
    {0x802dbcccu, func_802DBCCC_6C3AAC, b_func_802DBCCC_6C3AAC_callbacks, {}, {}},
    {0x802dbd04u, func_802DBD04_6C3AE4, b_func_802DBD04_6C3AE4_callbacks, {}, {}},
    {0x802dbdb0u, func_802DBDB0_6C3B90, {}, {}, {}},
    {0x802dbeccu, func_802DBECC_6C3CAC, {}, d_132_func_802DBECC_6C3CAC_graphs, d_132_func_802DBECC_6C3CAC_weighted_states},
    {0x802dbf3cu, func_802DBF3C_6C3D1C, {}, d_143_func_802DBF3C_6C3D1C_graphs, d_143_func_802DBF3C_6C3D1C_weighted_states},
    {0x802dbfacu, func_802DBFAC_6C3D8C, b_func_802DBFAC_6C3D8C_callbacks, {}, {}},
    {0x802dc05cu, func_802DC05C_6C3E3C, b_func_802DC05C_6C3E3C_callbacks, {}, {}},
    {0x802dc0a4u, func_802DC0A4_6C3E84, b_func_802DC0A4_6C3E84_callbacks, {}, {}},
    {0x802dc150u, func_802DC150_6C3F30, {}, d_187_func_802DC150_6C3F30_graphs, d_187_func_802DC150_6C3F30_weighted_states},
    {0x802dc1c0u, func_802DC1C0_6C3FA0, {}, d_198_func_802DC1C0_6C3FA0_graphs, d_198_func_802DC1C0_6C3FA0_weighted_states},
    {0x802dc230u, func_802DC230_6C4010, b_func_802DC230_6C4010_callbacks, {}, {}},
    {0x802dc2f4u, func_802DC2F4_6C40D4, b_func_802DC2F4_6C40D4_callbacks, {}, {}},
    {0x802dc358u, func_802DC358_6C4138, b_func_802DC358_6C4138_callbacks, {}, {}},
    {0x802dc404u, func_802DC404_6C41E4, {}, d_244_func_802DC404_6C41E4_graphs, d_244_func_802DC404_6C41E4_weighted_states},
    {0x802dc474u, func_802DC474_6C4254, {}, d_255_func_802DC474_6C4254_graphs, d_255_func_802DC474_6C4254_weighted_states},
    {0x802dc4e4u, func_802DC4E4_6C42C4, b_func_802DC4E4_6C42C4_callbacks, {}, {}},
    {0x802dc5a8u, func_802DC5A8_6C4388, b_func_802DC5A8_6C4388_callbacks, {}, {}},
    {0x802dc60cu, func_802DC60C_6C43EC, b_func_802DC60C_6C43EC_callbacks, {}, {}},
    {0x802dc6bcu, func_802DC6BC_6C449C, b_func_802DC6BC_6C449C_callbacks, {}, {}},
    {0x802dc7acu, func_802DC7AC_6C458C, {}, {}, {}},
    {0x802dc820u, func_802DC820_6C4600, {}, {}, {}},
    {0x802dca7cu, func_802DCA7C_6C485C, {}, {}, {}},
    {0x802dcb0cu, func_802DCB0C_6C48EC, {}, {}, {}},
    {0x802dcbb8u, func_802DCBB8_6C4998, {}, d_420_func_802DCBB8_6C4998_graphs, {}},
    {0x802dcc6cu, func_802DCC6C_6C4A4C, b_func_802DCC6C_6C4A4C_callbacks, {}, {}},
    {0x802dcef0u, func_802DCEF0_6C4CD0, {}, {}, d_16_func_802DCEF0_6C4CD0_weighted_states},
    {0x802dcf14u, func_802DCF14_6C4CF4, b_func_802DCF14_6C4CF4_callbacks, {}, {}},
    {0x802dcf4cu, func_802DCF4C_6C4D2C, b_func_802DCF4C_6C4D2C_callbacks, {}, {}},
    {0x802dcf84u, func_802DCF84_6C4D64, b_func_802DCF84_6C4D64_callbacks, {}, {}},
    {0x802dcfbcu, func_802DCFBC_6C4D9C, b_func_802DCFBC_6C4D9C_callbacks, {}, {}},
    {0x802dd068u, func_802DD068_6C4E48, {}, {}, {}},
    {0x802dd0e0u, func_802DD0E0_6C4EC0, {}, d_79_func_802DD0E0_6C4EC0_graphs, {}},
    {0x802dd1d4u, func_802DD1D4_6C4FB4, b_func_802DD1D4_6C4FB4_callbacks, {}, {}},
    {0x802dd2bcu, func_802DD2BC_6C509C, b_func_802DD2BC_6C509C_callbacks, d_132_func_802DD2BC_6C509C_graphs, {}},
    {0x802dd398u, func_802DD398_6C5178, b_func_802DD398_6C5178_callbacks, {}, {}},
    {0x802dd4acu, func_802DD4AC_6C528C, {}, {}, {}},
    {0x802dd53cu, func_802DD53C_6C531C, {}, {}, {}},
    {0x802dd6f0u, func_802DD6F0_6C54D0, {}, d_231_func_802DD6F0_6C54D0_graphs, d_231_func_802DD6F0_6C54D0_weighted_states},
    {0x802dd74cu, func_802DD74C_6C552C, b_func_802DD74C_6C552C_callbacks, d_241_func_802DD74C_6C552C_graphs, d_241_func_802DD74C_6C552C_weighted_states},
    {0x802dd7ccu, func_802DD7CC_6C55AC, {}, {}, {}},
    {0x802dd828u, func_802DD828_6C5608, b_func_802DD828_6C5608_callbacks, d_264_func_802DD828_6C5608_graphs, d_264_func_802DD828_6C5608_weighted_states},
    {0x802dd8a8u, func_802DD8A8_6C5688, {}, {}, {}},
    {0x802dd904u, func_802DD904_6C56E4, b_func_802DD904_6C56E4_callbacks, {}, {}},
    {0x802dd96cu, func_802DD96C_6C574C, b_func_802DD96C_6C574C_callbacks, d_298_func_802DD96C_6C574C_graphs, {}},
    {0x802dd9dcu, func_802DD9DC_6C57BC, b_func_802DD9DC_6C57BC_callbacks, d_309_func_802DD9DC_6C57BC_graphs, {}},
    {0x802ddaa0u, func_802DDAA0_6C5880, {}, {}, {}},
    {0x802ddaecu, func_802DDAEC_6C58CC, {}, {}, {}},
    {0x802ddb70u, func_802DDB70_6C5950, b_func_802DDB70_6C5950_callbacks, d_349_func_802DDB70_6C5950_graphs, {}},
    {0x802ddc28u, func_802DDC28_6C5A08, {}, {}, {}},
    {0x802ddc94u, func_802DDC94_6C5A74, b_func_802DDC94_6C5A74_callbacks, {}, {}},
    {0x802ddcb8u, func_802DDCB8_6C5A98, b_func_802DDCB8_6C5A98_callbacks, d_377_func_802DDCB8_6C5A98_graphs, d_377_func_802DDCB8_6C5A98_weighted_states},
    {0x802ddd38u, func_802DDD38_6C5B18, b_func_802DDD38_6C5B18_callbacks, d_389_func_802DDD38_6C5B18_graphs, d_389_func_802DDD38_6C5B18_weighted_states},
    {0x802dddb8u, func_802DDDB8_6C5B98, b_func_802DDDB8_6C5B98_callbacks, {}, {}},
    {0x802dde20u, func_802DDE20_6C5C00, b_func_802DDE20_6C5C00_callbacks, d_412_func_802DDE20_6C5C00_graphs, {}},
    {0x802dde90u, func_802DDE90_6C5C70, b_func_802DDE90_6C5C70_callbacks, d_423_func_802DDE90_6C5C70_graphs, {}},
    {0x802ddf54u, func_802DDF54_6C5D34, {}, {}, {}},
    {0x802ddfa0u, func_802DDFA0_6C5D80, {}, {}, {}},
    {0x802de024u, func_802DE024_6C5E04, b_func_802DE024_6C5E04_callbacks, d_463_func_802DE024_6C5E04_graphs, {}},
    {0x802de0dcu, func_802DE0DC_6C5EBC, b_func_802DE0DC_6C5EBC_callbacks, {}, {}},
    {0x802de100u, func_802DE100_6C5EE0, b_func_802DE100_6C5EE0_callbacks, d_484_func_802DE100_6C5EE0_graphs, d_484_func_802DE100_6C5EE0_weighted_states},
    {0x802de180u, func_802DE180_6C5F60, b_func_802DE180_6C5F60_callbacks, d_496_func_802DE180_6C5F60_graphs, d_496_func_802DE180_6C5F60_weighted_states},
    {0x802de200u, func_802DE200_6C5FE0, b_func_802DE200_6C5FE0_callbacks, {}, {}},
    {0x802de268u, func_802DE268_6C6048, b_func_802DE268_6C6048_callbacks, d_519_func_802DE268_6C6048_graphs, {}},
    {0x802de2d8u, func_802DE2D8_6C60B8, b_func_802DE2D8_6C60B8_callbacks, d_530_func_802DE2D8_6C60B8_graphs, {}},
    {0x802de39cu, func_802DE39C_6C617C, {}, {}, {}},
    {0x802de3e8u, func_802DE3E8_6C61C8, {}, {}, {}},
    {0x802de46cu, func_802DE46C_6C624C, b_func_802DE46C_6C624C_callbacks, d_570_func_802DE46C_6C624C_graphs, {}},
    {0x802de524u, func_802DE524_6C6304, b_func_802DE524_6C6304_callbacks, {}, {}},
    {0x802de870u, func_802DE870_6C6650, {}, {}, {}},
    {0x802de900u, func_802DE900_6C66E0, {}, d_19_func_802DE900_6C66E0_graphs, {}},
    {0x802de958u, func_802DE958_6C6738, {}, {}, {}},
    {0x802dea00u, func_802DEA00_6C67E0, b_func_802DEA00_6C67E0_callbacks, {}, {}},
    {0x802deb00u, func_802DEB00_6C68E0, {}, {}, {}},
    {0x802debb8u, func_802DEBB8_6C6998, {}, {}, {}},
    {0x802dec90u, func_802DEC90_6C6A70, b_func_802DEC90_6C6A70_callbacks, {}, {}},
    {0x802ded0cu, func_802DED0C_6C6AEC, b_func_802DED0C_6C6AEC_callbacks, d_40_func_802DED0C_6C6AEC_graphs, {}},
    {0x802ded68u, func_802DED68_6C6B48, b_func_802DED68_6C6B48_callbacks, {}, {}},
    {0x802dedd4u, func_802DEDD4_6C6BB4, {}, d_55_func_802DEDD4_6C6BB4_graphs, {}},
    {0x802dee2cu, func_802DEE2C_6C6C0C, b_func_802DEE2C_6C6C0C_callbacks, d_62_func_802DEE2C_6C6C0C_graphs, {}},
    {0x802deea0u, func_802DEEA0_6C6C80, {}, {}, {}},
    {0x802def28u, func_802DEF28_6C6D08, {}, {}, {}},
    {0x802defc0u, func_802DEFC0_6C6DA0, {}, {}, {}},
    {0x802df078u, func_802DF078_6C6E58, b_func_802DF078_6C6E58_callbacks, {}, {}},
    {0x802df0b8u, func_802DF0B8_6C6E98, {}, {}, {}},
    {0x802df104u, func_802DF104_6C6EE4, {}, {}, {}},
    {0x802df154u, func_802DF154_6C6F34, {}, d_132_func_802DF154_6C6F34_graphs, {}},
    {0x802df1f0u, func_802DF1F0_6C6FD0, b_func_802DF1F0_6C6FD0_callbacks, d_144_func_802DF1F0_6C6FD0_graphs, {}},
    {0x802df360u, func_802DF360_6C7140, b_func_802DF360_6C7140_callbacks, d_165_func_802DF360_6C7140_graphs, {}},
    {0x802df404u, func_802DF404_6C71E4, b_func_802DF404_6C71E4_callbacks, d_177_func_802DF404_6C71E4_graphs, {}},
    {0x802df478u, func_802DF478_6C7258, b_func_802DF478_6C7258_callbacks, d_185_func_802DF478_6C7258_graphs, {}},
    {0x802df4ecu, func_802DF4EC_6C72CC, b_func_802DF4EC_6C72CC_callbacks, d_193_func_802DF4EC_6C72CC_graphs, {}},
    {0x802df560u, func_802DF560_6C7340, {}, {}, {}},
    {0x802df5b8u, func_802DF5B8_6C7398, b_func_802DF5B8_6C7398_callbacks, {}, {}},
    {0x802df670u, func_802DF670_6C7450, b_func_802DF670_6C7450_callbacks, {}, {}},
    {0x802df6bcu, func_802DF6BC_6C749C, {}, d_39_func_802DF6BC_6C749C_graphs, {}},
    {0x802df710u, func_802DF710_6C74F0, b_func_802DF710_6C74F0_callbacks, {}, {}},
    {0x802df758u, func_802DF758_6C7538, b_func_802DF758_6C7538_callbacks, d_57_func_802DF758_6C7538_graphs, {}},
    {0x802df7d8u, func_802DF7D8_6C75B8, {}, {}, {}},
    {0x802df844u, func_802DF844_6C7624, b_func_802DF844_6C7624_callbacks, d_80_func_802DF844_6C7624_graphs, {}},
    {0x802df8c4u, func_802DF8C4_6C76A4, b_func_802DF8C4_6C76A4_callbacks, {}, {}},
    {0x802df91cu, func_802DF91C_6C76FC, b_func_802DF91C_6C76FC_callbacks, d_104_func_802DF91C_6C76FC_graphs, {}},
    {0x802df9ccu, func_802DF9CC_6C77AC, {}, {}, {}},
    {0x802dfa30u, func_802DFA30_6C7810, b_func_802DFA30_6C7810_callbacks, {}, {}},
    {0x802dfa5cu, func_802DFA5C_6C783C, b_func_802DFA5C_6C783C_callbacks, d_140_func_802DFA5C_6C783C_graphs, {}},
    {0x802dfb74u, func_802DFB74_6C7954, {}, {}, {}},
    {0x802dfc38u, func_802DFC38_6C7A18, {}, {}, {}},
    {0x802dfcd0u, func_802DFCD0_6C7AB0, b_func_802DFCD0_6C7AB0_callbacks, d_203_func_802DFCD0_6C7AB0_graphs, {}},
    {0x802dfd78u, func_802DFD78_6C7B58, b_func_802DFD78_6C7B58_callbacks, {}, {}},
    {0x802dfdf8u, func_802DFDF8_6C7BD8, b_func_802DFDF8_6C7BD8_callbacks, {}, {}},
    {0x802dfe4cu, func_802DFE4C_6C7C2C, {}, {}, {}},
    {0x802dfef0u, func_802DFEF0_6C7CD0, {}, {}, {}},
    {0x802dff94u, func_802DFF94_6C7D74, {}, {}, {}},
    {0x802e0070u, func_802E0070_6C7E50, b_func_802E0070_6C7E50_callbacks, {}, {}},
    {0x802e0094u, func_802E0094_6C7E74, {}, d_32_func_802E0094_6C7E74_graphs, {}},
    {0x802e00ecu, func_802E00EC_6C7ECC, b_func_802E00EC_6C7ECC_callbacks, {}, {}},
    {0x802e01f4u, func_802E01F4_6C7FD4, {}, {}, {}},
    {0x802e0334u, func_802E0334_6C8114, {}, {}, {}},
    {0x802e03c0u, func_802E03C0_6C81A0, b_func_802E03C0_6C81A0_callbacks, {}, {}},
    {0x802e05c0u, func_802E05C0_6C83A0, {}, d_11_func_802E05C0_6C83A0_graphs, {}},
    {0x802e0618u, func_802E0618_6C83F8, {}, {}, {}},
    {0x802e06d0u, func_802E06D0_6C84B0, {}, d_11_func_802E06D0_6C84B0_graphs, {}},
    {0x802e0728u, func_802E0728_6C8508, {}, {}, {}},
    {0x802e07f0u, func_802E07F0_6C85D0, {}, d_11_func_802E07F0_6C85D0_graphs, {}},
    {0x802e0848u, func_802E0848_6C8628, {}, {}, {}},
    {0x802e0900u, func_802E0900_6C86E0, {}, {}, {}},
    {0x802e09a8u, func_802E09A8_6C8788, b_func_802E09A8_6C8788_callbacks, {}, {}},
    {0x802e0a48u, func_802E0A48_6C8828, {}, d_80_func_802E0A48_6C8828_graphs, d_80_func_802E0A48_6C8828_weighted_states},
    {0x802e0ab8u, func_802E0AB8_6C8898, {}, d_91_func_802E0AB8_6C8898_graphs, d_91_func_802E0AB8_6C8898_weighted_states},
    {0x802e0b28u, func_802E0B28_6C8908, {}, d_102_func_802E0B28_6C8908_graphs, d_102_func_802E0B28_6C8908_weighted_states},
    {0x802e0b98u, func_802E0B98_6C8978, b_func_802E0B98_6C8978_callbacks, d_113_func_802E0B98_6C8978_graphs, d_113_func_802E0B98_6C8978_weighted_states},
    {0x802e0c3cu, func_802E0C3C_6C8A1C, {}, {}, {}},
    {0x802e0c98u, func_802E0C98_6C8A78, b_func_802E0C98_6C8A78_callbacks, {}, {}},
    {0x802e0d44u, func_802E0D44_6C8B24, b_func_802E0D44_6C8B24_callbacks, {}, {}},
    {0x802e0dacu, func_802E0DAC_6C8B8C, b_func_802E0DAC_6C8B8C_callbacks, d_170_func_802E0DAC_6C8B8C_graphs, {}},
    {0x802e0e20u, func_802E0E20_6C8C00, {}, {}, {}},
    {0x802e0e7cu, func_802E0E7C_6C8C5C, b_func_802E0E7C_6C8C5C_callbacks, d_192_func_802E0E7C_6C8C5C_graphs, {}},
    {0x802e0f04u, func_802E0F04_6C8CE4, {}, {}, {}},
    {0x802e0f9cu, func_802E0F9C_6C8D7C, b_func_802E0F9C_6C8D7C_callbacks, d_222_func_802E0F9C_6C8D7C_graphs, d_222_func_802E0F9C_6C8D7C_weighted_states},
    {0x802e1030u, func_802E1030_6C8E10, {}, {}, {}},
    {0x802e109cu, func_802E109C_6C8E7C, b_func_802E109C_6C8E7C_callbacks, d_246_func_802E109C_6C8E7C_graphs, {}},
    {0x802e1110u, func_802E1110_6C8EF0, {}, {}, {}},
    {0x802e115cu, func_802E115C_6C8F3C, b_func_802E115C_6C8F3C_callbacks, {}, {}},
    {0x802e1194u, func_802E1194_6C8F74, {}, d_275_func_802E1194_6C8F74_graphs, {}},
    {0x802e11d4u, func_802E11D4_6C8FB4, b_func_802E11D4_6C8FB4_callbacks, {}, {}},
    {0x802e12d8u, func_802E12D8_6C90B8, b_func_802E12D8_6C90B8_callbacks, {}, {}},
    {0x802e1374u, func_802E1374_6C9154, {}, {}, {}},
    {0x802e13f4u, func_802E13F4_6C91D4, b_func_802E13F4_6C91D4_callbacks, {}, {}},
    {0x802e142cu, func_802E142C_6C920C, b_func_802E142C_6C920C_callbacks, d_343_func_802E142C_6C920C_graphs, {}},
    {0x802e1488u, func_802E1488_6C9268, {}, {}, {}},
    {0x802e14f4u, func_802E14F4_6C92D4, b_func_802E14F4_6C92D4_callbacks, d_364_func_802E14F4_6C92D4_graphs, {}},
    {0x802e1570u, func_802E1570_6C9350, b_func_802E1570_6C9350_callbacks, {}, {}},
    {0x802e1594u, func_802E1594_6C9374, b_func_802E1594_6C9374_callbacks, d_380_func_802E1594_6C9374_graphs, {}},
    {0x802e1604u, func_802E1604_6C93E4, {}, {}, {}},
    {0x802e1678u, func_802E1678_6C9458, b_func_802E1678_6C9458_callbacks, d_404_func_802E1678_6C9458_graphs, {}},
    {0x802e171cu, func_802E171C_6C94FC, b_func_802E171C_6C94FC_callbacks, {}, {}},
    {0x802e17d4u, func_802E17D4_6C95B4, {}, {}, {}},
    {0x802e18a0u, func_802E18A0_6C9680, b_func_802E18A0_6C9680_callbacks, {}, {}},
    {0x802e18c4u, func_802E18C4_6C96A4, {}, d_36_func_802E18C4_6C96A4_graphs, {}},
    {0x802e1a4cu, func_802E1A4C_6C982C, {}, {}, {}},
    {0x802e1d10u, func_802E1D10_6C9AF0, {}, {}, {}},
};
const actor::BehaviorCallbackBinding entry_callbacks[] = {
    {0x802d895cu, func_802D895C_6C073C, {}, {}, {}},
    {0x802e20d0u, func_802E20D0_6C9EB0, {}, {}, {}},
    {0x802e222cu, func_802E222C_6CA00C, {}, {}, {}},
    {0x802e22e4u, func_802E22E4_6CA0C4, b_func_802E22E4_6CA0C4_callbacks, {}, {}},
};
constexpr std::uint32_t shared_source_func_802D92B0_6C1090[] = {0x803507e0u};
constexpr std::uint32_t shared_source_func_802DD398_6C5178[] = {0x802dd4acu, 0x802dd53cu, 0x802de0dcu, 0x802de524u, 0x80357384u};
constexpr std::uint32_t shared_source_func_802DE524_6C6304[] = {0x802dd4acu, 0x802dd53cu, 0x802de870u, 0x80357150u};
constexpr std::uint32_t shared_source_func_802DEA00_6C67E0[] = {0x802deb00u, 0x802debb8u, 0x80357384u};
constexpr std::uint32_t shared_source_func_802DEB00_6C68E0[] = {0x80357384u};
constexpr std::uint32_t shared_source_func_802DEFC0_6C6DA0[] = {0x80357384u};
constexpr std::uint32_t shared_source_func_802E03C0_6C81A0[] = {0x802e01c8u, 0x802e01f4u, 0x802e0334u, 0x803570f0u, 0x803573bcu};
constexpr std::uint32_t shared_source_func_802E0900_6C86E0[] = {0x80357384u};
constexpr std::uint32_t shared_source_func_802E1D10_6C9AF0[] = {0x802e191cu, 0x802e1950u, 0x802e19a0u, 0x802e1a3cu, 0x802e1cbcu, 0x803573bcu};
constexpr std::uint32_t shared_source_func_802E20D0_6C9EB0[] = {0x803570b0u, 0x80357120u, 0x803571c4u};
constexpr std::uint32_t shared_source_func_802E2100_6C9EE0[] = {0x803572b0u};
constexpr std::uint32_t shared_source_func_802E2194_6C9F74[] = {0x802e2100u, 0x80357170u, 0x803573bcu};
constexpr std::uint32_t shared_actor_source_func_802DA674_6C2454[] = {0x8035e1d4u};
constexpr std::uint32_t shared_actor_source_func_802DAED4_6C2CB4[] = {0x8035e1d4u};
constexpr std::uint32_t shared_actor_source_func_802DB270_6C3050[] = {0x8035e174u};
constexpr std::uint32_t shared_actor_source_func_802DB93C_6C371C[] = {0x802db270u, 0x8035e174u};
constexpr std::uint32_t shared_actor_source_func_802DBDB0_6C3B90[] = {0x8035e174u};
constexpr std::uint32_t shared_actor_source_func_802DC820_6C4600[] = {0x8035e174u};
constexpr std::uint32_t shared_actor_source_func_802DCC6C_6C4A4C[] = {0x802dbdb0u, 0x8035e174u};
constexpr std::uint32_t shared_actor_source_func_802DD068_6C4E48[] = {0x8035e298u};
const ActorProviderDependency dependencies[] = {
    {0x802d8800u, "func_802D8800_6C05E0", "src/river/6C05E0.c", 25, 27, "void func_802D8800_6C05E0(WorldBlock* arg0, WorldBlock* arg1)", ActorSourceAbi::lifecycle_utility, {}, {}, {}, {}, {}, {}, {}, d_func_802D8800_6C05E0_lifecycle_services},
    {0x802d8824u, "func_802D8824_6C0604", "src/river/6C05E0.c", 29, 31, "void func_802D8824_6C0604(WorldBlock* arg0, WorldBlock* arg1)", ActorSourceAbi::lifecycle_utility, {}, {}, {}, {}, {}, {}, {}, d_func_802D8824_6C0604_lifecycle_services},
    {0x802d8848u, "func_802D8848_6C0628", "src/river/6C05E0.c", 33, 35, "void func_802D8848_6C0628(WorldBlock* arg0)", ActorSourceAbi::lifecycle_utility, {}, {}, {}, {}, {}, {}, {}, d_func_802D8848_6C0628_lifecycle_services},
    {0x802d886cu, "func_802D886C_6C064C", "src/river/6C05E0.c", 37, 42, "void func_802D886C_6C064C(void)", ActorSourceAbi::lifecycle_utility, d_37_func_802D886C_6C064C_functions, {}, {}, {}, {}, {}, {}, d_func_802D886C_6C064C_lifecycle_services},
    {0x802d88f8u, "func_802D88F8_6C06D8", "src/river/6C05E0.c", 44, 55, "void func_802D88F8_6C06D8(s32 arg0)", ActorSourceAbi::lifecycle_utility, {}, {}, {}, {}, {}, {}, {}, d_func_802D88F8_6C06D8_lifecycle_services},
    {0x802d895cu, "func_802D895C_6C073C", "src/river/6C05E0.c", 57, 64, "void func_802D895C_6C073C(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, {}, d_func_802D895C_6C073C_lifecycle_services},
    {0x802d89acu, "func_802D89AC_6C078C", "src/river/6C05E0.c", 66, 67, "void func_802D89AC_6C078C(s32 arg0)", ActorSourceAbi::lifecycle_utility, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802d89b4u, "func_802D89B4_6C0794", "src/river/6C05E0.c", 69, 83, "void func_802D89B4_6C0794(WorldBlock* arg0)", ActorSourceAbi::lifecycle_utility, d_69_func_802D89B4_6C0794_functions, {}, {}, {}, {}, {}, {}, d_func_802D89B4_6C0794_lifecycle_services},
    {0x802d8a48u, "func_802D8A48_6C0828", "src/river/6C05E0.c", 85, 114, "void func_802D8A48_6C0828(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, d_func_802D8A48_6C0828_child_factories, d_85_func_802D8A48_6C0828_services, {}},
    {0x802d8b34u, "func_802D8B34_6C0914", "src/river/6C05E0.c", 116, 144, "void func_802D8B34_6C0914(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, d_func_802D8B34_6C0914_child_factories, d_116_func_802D8B34_6C0914_services, {}},
    {0x802d8c24u, "func_802D8C24_6C0A04", "src/river/6C05E0.c", 146, 173, "void func_802D8C24_6C0A04(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, d_func_802D8C24_6C0A04_child_factories, d_146_func_802D8C24_6C0A04_services, {}},
    {0x802d8cf0u, "func_802D8CF0_6C0AD0", "src/river/6C05E0.c", 175, 203, "void func_802D8CF0_6C0AD0(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, d_func_802D8CF0_6C0AD0_child_factories, d_175_func_802D8CF0_6C0AD0_services, {}},
    {0x802d8dc4u, "func_802D8DC4_6C0BA4", "src/river/6C05E0.c", 205, 233, "void func_802D8DC4_6C0BA4(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, d_func_802D8DC4_6C0BA4_child_factories, d_205_func_802D8DC4_6C0BA4_services, {}},
    {0x802d8e98u, "func_802D8E98_6C0C78", "src/river/6C05E0.c", 235, 261, "void func_802D8E98_6C0C78(GObj* obj, GroundResult* ground)", ActorSourceAbi::item_collision, {}, {}, {}, d_235_func_802D8E98_6C0C78_counted_options, {}, {}, d_235_func_802D8E98_6C0C78_services, {}},
    {0x802d8fccu, "func_802D8FCC_6C0DAC", "src/river/6C05E0.c", 263, 280, "void func_802D8FCC_6C0DAC(void)", ActorSourceAbi::lifecycle_utility, {}, {}, {}, {}, {}, {}, {}, d_func_802D8FCC_6C0DAC_lifecycle_services},
    {0x802d90b8u, "func_802D90B8_6C0E98", "src/river/6C05E0.c", 282, 305, "void func_802D90B8_6C0E98(void)", ActorSourceAbi::lifecycle_utility, d_282_func_802D90B8_6C0E98_functions, {}, {}, {}, {}, {}, {}, d_func_802D90B8_6C0E98_lifecycle_services},
    {0x802d9208u, "func_802D9208_6C0FE8", "src/river/6C05E0.c", 307, 308, "void func_802D9208_6C0FE8(s32 arg0)", ActorSourceAbi::lifecycle_utility, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802d9210u, "func_802D9210_6C0FF0", "src/river/6C05E0.c", 310, 327, "s32 func_802D9210_6C0FF0(s32 sceneId)", ActorSourceAbi::lifecycle_utility, {}, {}, {}, {}, {}, {}, {}, d_func_802D9210_6C0FF0_lifecycle_services},
    {0x802d92b0u, "func_802D92B0_6C1090", "src/river/6C1090.c", 6, 49, "void func_802D92B0_6C1090(s32 arg0, f32 arg1)", ActorSourceAbi::lifecycle_utility, shared_source_func_802D92B0_6C1090, {}, {}, {}, {}, {}, {}, d_func_802D92B0_6C1090_lifecycle_services},
    {0x802d9520u, "func_802D9520_6C1300", "src/river/6C1300.c", 32, 63, "s32 func_802D9520_6C1300(GObj* obj)", ActorSourceAbi::returning_gobj_utility, {}, {}, {}, {}, {}, {}, d_32_func_802D9520_6C1300_services, {}},
    {0x802d9664u, "func_802D9664_6C1444", "src/river/6C1300.c", 65, 77, "void func_802D9664_6C1444(GObj* obj)", ActorSourceAbi::gobj_callback, d_65_func_802D9664_6C1444_functions, d_65_func_802D9664_6C1444_graphs, d_65_func_802D9664_6C1444_weighted_states, {}, d_65_func_802D9664_6C1444_headers, {}, d_65_func_802D9664_6C1444_services, {}},
    {0x802d96fcu, "func_802D96FC_6C14DC", "src/river/6C1300.c", 79, 88, "void func_802D96FC_6C14DC(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_79_func_802D96FC_6C14DC_graphs, d_79_func_802D96FC_6C14DC_weighted_states, {}, d_79_func_802D96FC_6C14DC_headers, {}, d_79_func_802D96FC_6C14DC_services, {}},
    {0x802d976cu, "func_802D976C_6C154C", "src/river/6C1300.c", 90, 102, "void func_802D976C_6C154C(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_90_func_802D976C_6C154C_graphs, d_90_func_802D976C_6C154C_weighted_states, {}, d_90_func_802D976C_6C154C_headers, {}, d_90_func_802D976C_6C154C_services, {}},
    {0x802d9808u, "func_802D9808_6C15E8", "src/river/6C1300.c", 104, 126, "void func_802D9808_6C15E8(GObj* obj)", ActorSourceAbi::gobj_callback, d_104_func_802D9808_6C15E8_functions, d_104_func_802D9808_6C15E8_graphs, {}, {}, d_104_func_802D9808_6C15E8_headers, {}, d_104_func_802D9808_6C15E8_services, {}},
    {0x802d990cu, "func_802D990C_6C16EC", "src/river/6C1300.c", 128, 139, "void func_802D990C_6C16EC(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, d_128_func_802D990C_6C16EC_headers, {}, {}, {}},
    {0x802d9990u, "func_802D9990_6C1770", "src/river/6C1300.c", 141, 151, "void func_802D9990_6C1770(GObj* obj)", ActorSourceAbi::gobj_callback, d_141_func_802D9990_6C1770_functions, d_141_func_802D9990_6C1770_graphs, {}, {}, d_141_func_802D9990_6C1770_headers, {}, d_141_func_802D9990_6C1770_services, {}},
    {0x802d9a0cu, "func_802D9A0C_6C17EC", "src/river/6C1300.c", 153, 161, "void func_802D9A0C_6C17EC(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802d9a58u, "func_802D9A58_6C1838", "src/river/6C1300.c", 163, 175, "void func_802D9A58_6C1838(GObj* obj)", ActorSourceAbi::gobj_callback, d_163_func_802D9A58_6C1838_functions, {}, {}, {}, {}, {}, d_163_func_802D9A58_6C1838_services, {}},
    {0x802d9ac4u, "func_802D9AC4_6C18A4", "src/river/6C1300.c", 177, 217, "void func_802D9AC4_6C18A4(GObj* obj)", ActorSourceAbi::gobj_callback, d_177_func_802D9AC4_6C18A4_functions, {}, {}, {}, d_177_func_802D9AC4_6C18A4_headers, {}, d_177_func_802D9AC4_6C18A4_services, {}},
    {0x802d9c8cu, "func_802D9C8C_6C1A6C", "src/river/6C1300.c", 219, 235, "void func_802D9C8C_6C1A6C(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, d_219_func_802D9C8C_6C1A6C_services, {}},
    {0x802d9d7cu, "func_802D9D7C_6C1B5C", "src/river/6C1300.c", 237, 246, "void func_802D9D7C_6C1B5C(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802d9dd8u, "func_802D9DD8_6C1BB8", "src/river/6C1300.c", 248, 270, "void func_802D9DD8_6C1BB8(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802d9f44u, "func_802D9F44_6C1D24", "src/river/6C1300.c", 272, 274, "GObj* func_802D9F44_6C1D24(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)", ActorSourceAbi::factory_six, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802d9f80u, "func_802D9F80_6C1D60", "src/river/6C1D60.c", 24, 32, "void func_802D9F80_6C1D60(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_24_func_802D9F80_6C1D60_graphs, d_24_func_802D9F80_6C1D60_weighted_states, {}, d_24_func_802D9F80_6C1D60_headers, {}, d_24_func_802D9F80_6C1D60_services, {}},
    {0x802d9fdcu, "func_802D9FDC_6C1DBC", "src/river/6C1D60.c", 34, 43, "void func_802D9FDC_6C1DBC(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_34_func_802D9FDC_6C1DBC_graphs, d_34_func_802D9FDC_6C1DBC_weighted_states, {}, d_34_func_802D9FDC_6C1DBC_headers, {}, d_34_func_802D9FDC_6C1DBC_services, {}},
    {0x802da04cu, "func_802DA04C_6C1E2C", "src/river/6C1D60.c", 45, 54, "void func_802DA04C_6C1E2C(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_45_func_802DA04C_6C1E2C_graphs, d_45_func_802DA04C_6C1E2C_weighted_states, {}, d_45_func_802DA04C_6C1E2C_headers, {}, d_45_func_802DA04C_6C1E2C_services, {}},
    {0x802da0bcu, "func_802DA0BC_6C1E9C", "src/river/6C1D60.c", 56, 65, "void func_802DA0BC_6C1E9C(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_56_func_802DA0BC_6C1E9C_graphs, d_56_func_802DA0BC_6C1E9C_weighted_states, {}, d_56_func_802DA0BC_6C1E9C_headers, {}, d_56_func_802DA0BC_6C1E9C_services, {}},
    {0x802da12cu, "func_802DA12C_6C1F0C", "src/river/6C1D60.c", 67, 77, "void func_802DA12C_6C1F0C(GObj* obj)", ActorSourceAbi::gobj_callback, d_67_func_802DA12C_6C1F0C_functions, d_67_func_802DA12C_6C1F0C_graphs, d_67_func_802DA12C_6C1F0C_weighted_states, {}, d_67_func_802DA12C_6C1F0C_headers, {}, d_67_func_802DA12C_6C1F0C_services, {}},
    {0x802da1acu, "func_802DA1AC_6C1F8C", "src/river/6C1D60.c", 79, 88, "void func_802DA1AC_6C1F8C(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802da208u, "func_802DA208_6C1FE8", "src/river/6C1D60.c", 90, 107, "POKEMON_FUNC(func_802DA208_6C1FE8)", ActorSourceAbi::gobj_callback, d_90_func_802DA208_6C1FE8_functions, d_90_func_802DA208_6C1FE8_graphs, {}, {}, d_90_func_802DA208_6C1FE8_headers, {}, d_90_func_802DA208_6C1FE8_services, {}},
    {0x802da30cu, "func_802DA30C_6C20EC", "src/river/6C1D60.c", 109, 119, "void func_802DA30C_6C20EC(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, d_109_func_802DA30C_6C20EC_headers, {}, {}, {}},
    {0x802da390u, "func_802DA390_6C2170", "src/river/6C1D60.c", 121, 130, "void func_802DA390_6C2170(GObj* obj)", ActorSourceAbi::gobj_callback, d_121_func_802DA390_6C2170_functions, {}, {}, {}, d_121_func_802DA390_6C2170_headers, {}, {}, {}},
    {0x802da3f8u, "func_802DA3F8_6C21D8", "src/river/6C1D60.c", 132, 141, "void func_802DA3F8_6C21D8(GObj* obj)", ActorSourceAbi::gobj_callback, d_132_func_802DA3F8_6C21D8_functions, d_132_func_802DA3F8_6C21D8_graphs, {}, {}, d_132_func_802DA3F8_6C21D8_headers, {}, d_132_func_802DA3F8_6C21D8_services, {}},
    {0x802da46cu, "func_802DA46C_6C224C", "src/river/6C1D60.c", 143, 151, "void func_802DA46C_6C224C(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802da4b8u, "func_802DA4B8_6C2298", "src/river/6C1D60.c", 153, 155, "GObj* func_802DA4B8_6C2298(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)", ActorSourceAbi::factory_six, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802da500u, "func_802DA500_6C22E0", "src/river/6C22E0.c", 17, 29, "void func_802DA500_6C22E0(GObj* obj)", ActorSourceAbi::gobj_callback, d_17_func_802DA500_6C22E0_functions, d_17_func_802DA500_6C22E0_graphs, {}, {}, d_17_func_802DA500_6C22E0_headers, {}, d_17_func_802DA500_6C22E0_services, {}},
    {0x802da590u, "func_802DA590_6C2370", "src/river/6C22E0.c", 31, 42, "void func_802DA590_6C2370(GObj* obj)", ActorSourceAbi::gobj_callback, d_31_func_802DA590_6C2370_functions, {}, {}, {}, {}, {}, {}, {}},
    {0x802da620u, "func_802DA620_6C2400", "src/river/6C22E0.c", 44, 53, "void func_802DA620_6C2400(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, d_44_func_802DA620_6C2400_services, {}},
    {0x802da674u, "func_802DA674_6C2454", "src/river/6C22E0.c", 56, 84, "void func_802DA674_6C2454(GObj* obj)", ActorSourceAbi::gobj_callback, shared_actor_source_func_802DA674_6C2454, {}, {}, {}, {}, {}, d_56_func_802DA674_6C2454_services, {}},
    {0x802da73cu, "func_802DA73C_6C251C", "src/river/6C22E0.c", 86, 89, "void func_802DA73C_6C251C(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802da768u, "func_802DA768_6C2548", "src/river/6C22E0.c", 91, 103, "void func_802DA768_6C2548(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, d_91_func_802DA768_6C2548_services, {}},
    {0x802da814u, "func_802DA814_6C25F4", "src/river/6C22E0.c", 105, 112, "void func_802DA814_6C25F4(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_105_func_802DA814_6C25F4_graphs, {}, {}, {}, {}, d_105_func_802DA814_6C25F4_services, {}},
    {0x802da864u, "func_802DA864_6C2644", "src/river/6C22E0.c", 114, 123, "void func_802DA864_6C2644(GObj* obj)", ActorSourceAbi::gobj_callback, d_114_func_802DA864_6C2644_functions, d_114_func_802DA864_6C2644_graphs, {}, {}, {}, {}, d_114_func_802DA864_6C2644_services, {}},
    {0x802da8ccu, "func_802DA8CC_6C26AC", "src/river/6C22E0.c", 125, 127, "GObj* func_802DA8CC_6C26AC(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)", ActorSourceAbi::factory_six, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802da910u, "func_802DA910_6C26F0", "src/river/6C26F0.c", 11, 23, "void func_802DA910_6C26F0(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_11_func_802DA910_6C26F0_graphs, {}, {}, d_11_func_802DA910_6C26F0_headers, {}, d_11_func_802DA910_6C26F0_services, {}},
    {0x802da9a4u, "func_802DA9A4_6C2784", "src/river/6C26F0.c", 25, 27, "GObj* func_802DA9A4_6C2784(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)", ActorSourceAbi::factory_six, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802da9e0u, "func_802DA9E0_6C27C0", "src/river/6C27C0.c", 20, 50, "void func_802DA9E0_6C27C0(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, d_func_802DA9E0_6C27C0_child_factories, d_20_func_802DA9E0_6C27C0_services, {}},
    {0x802daaccu, "func_802DAACC_6C28AC", "src/river/6C27C0.c", 52, 82, "void func_802DAACC_6C28AC(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, d_func_802DAACC_6C28AC_child_factories, d_52_func_802DAACC_6C28AC_services, {}},
    {0x802dabb8u, "func_802DABB8_6C2998", "src/river/6C27C0.c", 84, 91, "void func_802DABB8_6C2998(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_84_func_802DABB8_6C2998_graphs, {}, {}, {}, {}, d_84_func_802DABB8_6C2998_services, {}},
    {0x802dac04u, "func_802DAC04_6C29E4", "src/river/6C27C0.c", 93, 95, "void func_802DAC04_6C29E4(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, d_93_func_802DAC04_6C29E4_weighted_states, {}, {}, {}, d_93_func_802DAC04_6C29E4_services, {}},
    {0x802dac28u, "func_802DAC28_6C2A08", "src/river/6C27C0.c", 97, 107, "void func_802DAC28_6C2A08(GObj* obj)", ActorSourceAbi::gobj_callback, d_97_func_802DAC28_6C2A08_functions, {}, {}, {}, {}, {}, d_97_func_802DAC28_6C2A08_services, {}},
    {0x802daca8u, "func_802DACA8_6C2A88", "src/river/6C27C0.c", 109, 119, "void func_802DACA8_6C2A88(GObj* obj)", ActorSourceAbi::gobj_callback, d_109_func_802DACA8_6C2A88_functions, {}, {}, {}, {}, {}, d_109_func_802DACA8_6C2A88_services, {}},
    {0x802dad28u, "func_802DAD28_6C2B08", "src/river/6C27C0.c", 121, 123, "GObj* func_802DAD28_6C2B08(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)", ActorSourceAbi::factory_six, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802dad60u, "func_802DAD60_6C2B40", "src/river/6C27C0.c", 125, 135, "void func_802DAD60_6C2B40(GObj* obj)", ActorSourceAbi::gobj_callback, d_125_func_802DAD60_6C2B40_functions, d_125_func_802DAD60_6C2B40_graphs, {}, {}, d_125_func_802DAD60_6C2B40_headers, {}, d_125_func_802DAD60_6C2B40_services, {}},
    {0x802dadd8u, "func_802DADD8_6C2BB8", "src/river/6C27C0.c", 137, 148, "void func_802DADD8_6C2BB8(GObj* obj)", ActorSourceAbi::gobj_callback, d_137_func_802DADD8_6C2BB8_functions, {}, {}, {}, {}, {}, {}, {}},
    {0x802dae68u, "func_802DAE68_6C2C48", "src/river/6C27C0.c", 150, 160, "void func_802DAE68_6C2C48(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, d_150_func_802DAE68_6C2C48_services, {}},
    {0x802daed4u, "func_802DAED4_6C2CB4", "src/river/6C27C0.c", 163, 190, "void func_802DAED4_6C2CB4(GObj* obj)", ActorSourceAbi::gobj_callback, shared_actor_source_func_802DAED4_6C2CB4, {}, {}, {}, {}, {}, d_163_func_802DAED4_6C2CB4_services, {}},
    {0x802dafb8u, "func_802DAFB8_6C2D98", "src/river/6C27C0.c", 192, 204, "void func_802DAFB8_6C2D98(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, d_192_func_802DAFB8_6C2D98_services, {}},
    {0x802db064u, "func_802DB064_6C2E44", "src/river/6C27C0.c", 206, 208, "GObj* func_802DB064_6C2E44(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)", ActorSourceAbi::factory_six, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802db0a0u, "func_802DB0A0_6C2E80", "src/river/6C2E80.c", 38, 52, "void func_802DB0A0_6C2E80(GObj* obj)", ActorSourceAbi::gobj_callback, d_38_func_802DB0A0_6C2E80_functions, {}, d_38_func_802DB0A0_6C2E80_weighted_states, {}, {}, {}, d_38_func_802DB0A0_6C2E80_services, {}},
    {0x802db11cu, "func_802DB11C_6C2EFC", "src/river/6C2E80.c", 54, 57, "void func_802DB11C_6C2EFC(GObj* obj)", ActorSourceAbi::gobj_callback, d_54_func_802DB11C_6C2EFC_functions, {}, {}, {}, d_54_func_802DB11C_6C2EFC_headers, {}, {}, {}},
    {0x802db154u, "func_802DB154_6C2F34", "src/river/6C2E80.c", 59, 62, "void func_802DB154_6C2F34(GObj* obj)", ActorSourceAbi::gobj_callback, d_59_func_802DB154_6C2F34_functions, {}, {}, {}, d_59_func_802DB154_6C2F34_headers, {}, {}, {}},
    {0x802db18cu, "func_802DB18C_6C2F6C", "src/river/6C2E80.c", 64, 67, "void func_802DB18C_6C2F6C(GObj* obj)", ActorSourceAbi::gobj_callback, d_64_func_802DB18C_6C2F6C_functions, {}, {}, {}, d_64_func_802DB18C_6C2F6C_headers, {}, {}, {}},
    {0x802db1c4u, "func_802DB1C4_6C2FA4", "src/river/6C2E80.c", 69, 82, "void func_802DB1C4_6C2FA4(GObj* obj)", ActorSourceAbi::gobj_callback, d_69_func_802DB1C4_6C2FA4_functions, {}, {}, {}, {}, {}, d_69_func_802DB1C4_6C2FA4_services, {}},
    {0x802db270u, "func_802DB270_6C3050", "src/river/6C2E80.c", 84, 114, "void func_802DB270_6C3050(GObj* obj)", ActorSourceAbi::gobj_callback, shared_actor_source_func_802DB270_6C3050, {}, {}, {}, {}, {}, d_84_func_802DB270_6C3050_services, {}},
    {0x802db388u, "func_802DB388_6C3168", "src/river/6C2E80.c", 116, 125, "POKEMON_FUNC(func_802DB388_6C3168)", ActorSourceAbi::gobj_callback, d_116_func_802DB388_6C3168_functions, d_116_func_802DB388_6C3168_graphs, {}, {}, d_116_func_802DB388_6C3168_headers, {}, d_116_func_802DB388_6C3168_services, {}},
    {0x802db41cu, "func_802DB41C_6C31FC", "src/river/6C2E80.c", 127, 135, "void func_802DB41C_6C31FC(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802db468u, "func_802DB468_6C3248", "src/river/6C2E80.c", 137, 143, "void func_802DB468_6C3248(GObj* obj)", ActorSourceAbi::gobj_callback, d_137_func_802DB468_6C3248_functions, {}, {}, {}, {}, {}, {}, {}},
    {0x802db4a8u, "func_802DB4A8_6C3288", "src/river/6C2E80.c", 145, 159, "void func_802DB4A8_6C3288(GObj* obj)", ActorSourceAbi::gobj_callback, d_145_func_802DB4A8_6C3288_functions, d_145_func_802DB4A8_6C3288_graphs, {}, {}, d_145_func_802DB4A8_6C3288_headers, {}, d_145_func_802DB4A8_6C3288_services, {}},
    {0x802db554u, "func_802DB554_6C3334", "src/river/6C2E80.c", 161, 170, "void func_802DB554_6C3334(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802db5c0u, "func_802DB5C0_6C33A0", "src/river/6C2E80.c", 172, 181, "void func_802DB5C0_6C33A0(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802db630u, "func_802DB630_6C3410", "src/river/6C2E80.c", 183, 202, "void func_802DB630_6C3410(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, d_183_func_802DB630_6C3410_services, {}},
    {0x802db71cu, "func_802DB71C_6C34FC", "src/river/6C2E80.c", 204, 214, "void func_802DB71C_6C34FC(GObj* obj)", ActorSourceAbi::gobj_callback, d_204_func_802DB71C_6C34FC_functions, {}, {}, {}, d_204_func_802DB71C_6C34FC_headers, {}, {}, {}},
    {0x802db78cu, "func_802DB78C_6C356C", "src/river/6C2E80.c", 216, 235, "void func_802DB78C_6C356C(GObj* obj)", ActorSourceAbi::gobj_callback, d_216_func_802DB78C_6C356C_functions, {}, {}, {}, d_216_func_802DB78C_6C356C_headers, {}, d_216_func_802DB78C_6C356C_services, {}},
    {0x802db85cu, "func_802DB85C_6C363C", "src/river/6C2E80.c", 237, 252, "void func_802DB85C_6C363C(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, d_237_func_802DB85C_6C363C_services, {}},
    {0x802db8ecu, "func_802DB8EC_6C36CC", "src/river/6C2E80.c", 254, 261, "void func_802DB8EC_6C36CC(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_254_func_802DB8EC_6C36CC_graphs, {}, {}, {}, {}, d_254_func_802DB8EC_6C36CC_services, {}},
    {0x802db93cu, "func_802DB93C_6C371C", "src/river/6C2E80.c", 263, 294, "void func_802DB93C_6C371C(GObj* obj)", ActorSourceAbi::gobj_callback, shared_actor_source_func_802DB93C_6C371C, {}, {}, {}, d_263_func_802DB93C_6C371C_headers, {}, d_263_func_802DB93C_6C371C_services, {}},
    {0x802dbb54u, "func_802DBB54_6C3934", "src/river/6C2E80.c", 296, 298, "GObj* func_802DBB54_6C3934(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)", ActorSourceAbi::factory_six, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802dbba0u, "func_802DBBA0_6C3980", "src/river/6C3980.c", 52, 72, "POKEMON_FUNC(func_802DBBA0_6C3980)", ActorSourceAbi::gobj_callback, d_52_func_802DBBA0_6C3980_functions, {}, d_52_func_802DBBA0_6C3980_weighted_states, {}, {}, {}, d_52_func_802DBBA0_6C3980_services, {}},
    {0x802dbc5cu, "func_802DBC5C_6C3A3C", "src/river/6C3980.c", 74, 77, "void func_802DBC5C_6C3A3C(GObj* obj)", ActorSourceAbi::gobj_callback, d_74_func_802DBC5C_6C3A3C_functions, {}, {}, {}, d_74_func_802DBC5C_6C3A3C_headers, {}, {}, {}},
    {0x802dbc94u, "func_802DBC94_6C3A74", "src/river/6C3980.c", 79, 82, "void func_802DBC94_6C3A74(GObj* obj)", ActorSourceAbi::gobj_callback, d_79_func_802DBC94_6C3A74_functions, {}, {}, {}, d_79_func_802DBC94_6C3A74_headers, {}, {}, {}},
    {0x802dbcccu, "func_802DBCCC_6C3AAC", "src/river/6C3980.c", 84, 87, "void func_802DBCCC_6C3AAC(GObj* obj)", ActorSourceAbi::gobj_callback, d_84_func_802DBCCC_6C3AAC_functions, {}, {}, {}, d_84_func_802DBCCC_6C3AAC_headers, {}, {}, {}},
    {0x802dbd04u, "func_802DBD04_6C3AE4", "src/river/6C3980.c", 89, 102, "void func_802DBD04_6C3AE4(GObj* obj)", ActorSourceAbi::gobj_callback, d_89_func_802DBD04_6C3AE4_functions, {}, {}, {}, {}, {}, d_89_func_802DBD04_6C3AE4_services, {}},
    {0x802dbdb0u, "func_802DBDB0_6C3B90", "src/river/6C3980.c", 104, 130, "void func_802DBDB0_6C3B90(GObj* obj)", ActorSourceAbi::gobj_callback, shared_actor_source_func_802DBDB0_6C3B90, {}, {}, {}, {}, {}, d_104_func_802DBDB0_6C3B90_services, {}},
    {0x802dbeccu, "func_802DBECC_6C3CAC", "src/river/6C3980.c", 132, 141, "void func_802DBECC_6C3CAC(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_132_func_802DBECC_6C3CAC_graphs, d_132_func_802DBECC_6C3CAC_weighted_states, {}, d_132_func_802DBECC_6C3CAC_headers, {}, d_132_func_802DBECC_6C3CAC_services, {}},
    {0x802dbf3cu, "func_802DBF3C_6C3D1C", "src/river/6C3980.c", 143, 152, "void func_802DBF3C_6C3D1C(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_143_func_802DBF3C_6C3D1C_graphs, d_143_func_802DBF3C_6C3D1C_weighted_states, {}, d_143_func_802DBF3C_6C3D1C_headers, {}, d_143_func_802DBF3C_6C3D1C_services, {}},
    {0x802dbfacu, "func_802DBFAC_6C3D8C", "src/river/6C3980.c", 154, 166, "POKEMON_FUNC(func_802DBFAC_6C3D8C)", ActorSourceAbi::gobj_callback, d_154_func_802DBFAC_6C3D8C_functions, {}, {}, {}, d_154_func_802DBFAC_6C3D8C_headers, {}, d_154_func_802DBFAC_6C3D8C_services, {}},
    {0x802dc05cu, "func_802DC05C_6C3E3C", "src/river/6C3980.c", 168, 172, "POKEMON_FUNC(func_802DC05C_6C3E3C)", ActorSourceAbi::gobj_callback, d_168_func_802DC05C_6C3E3C_functions, {}, {}, {}, {}, {}, d_168_func_802DC05C_6C3E3C_services, {}},
    {0x802dc0a4u, "func_802DC0A4_6C3E84", "src/river/6C3980.c", 174, 185, "POKEMON_FUNC(func_802DC0A4_6C3E84)", ActorSourceAbi::gobj_callback, d_174_func_802DC0A4_6C3E84_functions, {}, {}, {}, {}, {}, d_174_func_802DC0A4_6C3E84_services, {}},
    {0x802dc150u, "func_802DC150_6C3F30", "src/river/6C3980.c", 187, 196, "void func_802DC150_6C3F30(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_187_func_802DC150_6C3F30_graphs, d_187_func_802DC150_6C3F30_weighted_states, {}, d_187_func_802DC150_6C3F30_headers, {}, d_187_func_802DC150_6C3F30_services, {}},
    {0x802dc1c0u, "func_802DC1C0_6C3FA0", "src/river/6C3980.c", 198, 207, "void func_802DC1C0_6C3FA0(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_198_func_802DC1C0_6C3FA0_graphs, d_198_func_802DC1C0_6C3FA0_weighted_states, {}, d_198_func_802DC1C0_6C3FA0_headers, {}, d_198_func_802DC1C0_6C3FA0_services, {}},
    {0x802dc230u, "func_802DC230_6C4010", "src/river/6C3980.c", 209, 222, "POKEMON_FUNC(func_802DC230_6C4010)", ActorSourceAbi::gobj_callback, d_209_func_802DC230_6C4010_functions, {}, {}, {}, d_209_func_802DC230_6C4010_headers, {}, d_209_func_802DC230_6C4010_services, {}},
    {0x802dc2f4u, "func_802DC2F4_6C40D4", "src/river/6C3980.c", 224, 229, "POKEMON_FUNC(func_802DC2F4_6C40D4)", ActorSourceAbi::gobj_callback, d_224_func_802DC2F4_6C40D4_functions, {}, {}, {}, {}, {}, d_224_func_802DC2F4_6C40D4_services, {}},
    {0x802dc358u, "func_802DC358_6C4138", "src/river/6C3980.c", 231, 242, "POKEMON_FUNC(func_802DC358_6C4138)", ActorSourceAbi::gobj_callback, d_231_func_802DC358_6C4138_functions, {}, {}, {}, {}, {}, d_231_func_802DC358_6C4138_services, {}},
    {0x802dc404u, "func_802DC404_6C41E4", "src/river/6C3980.c", 244, 253, "void func_802DC404_6C41E4(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_244_func_802DC404_6C41E4_graphs, d_244_func_802DC404_6C41E4_weighted_states, {}, d_244_func_802DC404_6C41E4_headers, {}, d_244_func_802DC404_6C41E4_services, {}},
    {0x802dc474u, "func_802DC474_6C4254", "src/river/6C3980.c", 255, 264, "void func_802DC474_6C4254(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_255_func_802DC474_6C4254_graphs, d_255_func_802DC474_6C4254_weighted_states, {}, d_255_func_802DC474_6C4254_headers, {}, d_255_func_802DC474_6C4254_services, {}},
    {0x802dc4e4u, "func_802DC4E4_6C42C4", "src/river/6C3980.c", 266, 282, "void func_802DC4E4_6C42C4(GObj* obj)", ActorSourceAbi::gobj_callback, d_266_func_802DC4E4_6C42C4_functions, {}, {}, {}, d_266_func_802DC4E4_6C42C4_headers, {}, d_266_func_802DC4E4_6C42C4_services, {}},
    {0x802dc5a8u, "func_802DC5A8_6C4388", "src/river/6C3980.c", 284, 292, "void func_802DC5A8_6C4388(GObj* obj)", ActorSourceAbi::gobj_callback, d_284_func_802DC5A8_6C4388_functions, {}, {}, {}, {}, {}, d_284_func_802DC5A8_6C4388_services, {}},
    {0x802dc60cu, "func_802DC60C_6C43EC", "src/river/6C3980.c", 294, 307, "POKEMON_FUNC(func_802DC60C_6C43EC)", ActorSourceAbi::gobj_callback, d_294_func_802DC60C_6C43EC_functions, {}, {}, {}, {}, {}, d_294_func_802DC60C_6C43EC_services, {}},
    {0x802dc6bcu, "func_802DC6BC_6C449C", "src/river/6C3980.c", 309, 332, "POKEMON_FUNC(func_802DC6BC_6C449C)", ActorSourceAbi::gobj_callback, d_309_func_802DC6BC_6C449C_functions, {}, {}, {}, {}, {}, d_309_func_802DC6BC_6C449C_services, {}},
    {0x802dc7acu, "func_802DC7AC_6C458C", "src/river/6C3980.c", 334, 339, "POKEMON_FUNC(func_802DC7AC_6C458C)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, d_334_func_802DC7AC_6C458C_services, {}},
    {0x802dc820u, "func_802DC820_6C4600", "src/river/6C3980.c", 341, 390, "void func_802DC820_6C4600(GObj* obj)", ActorSourceAbi::gobj_callback, shared_actor_source_func_802DC820_6C4600, {}, {}, {}, {}, {}, d_341_func_802DC820_6C4600_services, {}},
    {0x802dca7cu, "func_802DCA7C_6C485C", "src/river/6C3980.c", 392, 403, "POKEMON_FUNC(func_802DCA7C_6C485C)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, d_392_func_802DCA7C_6C485C_services, {}},
    {0x802dcb0cu, "func_802DCB0C_6C48EC", "src/river/6C3980.c", 405, 418, "POKEMON_FUNC(func_802DCB0C_6C48EC)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, d_405_func_802DCB0C_6C48EC_headers, {}, d_405_func_802DCB0C_6C48EC_services, {}},
    {0x802dcbb8u, "func_802DCBB8_6C4998", "src/river/6C3980.c", 420, 439, "void func_802DCBB8_6C4998(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_420_func_802DCBB8_6C4998_graphs, {}, {}, {}, {}, d_420_func_802DCBB8_6C4998_services, {}},
    {0x802dcc6cu, "func_802DCC6C_6C4A4C", "src/river/6C3980.c", 441, 478, "void func_802DCC6C_6C4A4C(GObj* obj)", ActorSourceAbi::gobj_callback, shared_actor_source_func_802DCC6C_6C4A4C, {}, {}, {}, d_441_func_802DCC6C_6C4A4C_headers, {}, d_441_func_802DCC6C_6C4A4C_services, {}},
    {0x802dceb0u, "func_802DCEB0_6C4C90", "src/river/6C3980.c", 480, 482, "GObj* func_802DCEB0_6C4C90(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)", ActorSourceAbi::factory_six, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802dcef0u, "func_802DCEF0_6C4CD0", "src/river/6C4CD0.c", 16, 18, "void func_802DCEF0_6C4CD0(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, d_16_func_802DCEF0_6C4CD0_weighted_states, {}, {}, {}, d_16_func_802DCEF0_6C4CD0_services, {}},
    {0x802dcf14u, "func_802DCF14_6C4CF4", "src/river/6C4CD0.c", 20, 23, "void func_802DCF14_6C4CF4(GObj* obj)", ActorSourceAbi::gobj_callback, d_20_func_802DCF14_6C4CF4_functions, {}, {}, {}, d_20_func_802DCF14_6C4CF4_headers, {}, {}, {}},
    {0x802dcf4cu, "func_802DCF4C_6C4D2C", "src/river/6C4CD0.c", 25, 28, "void func_802DCF4C_6C4D2C(GObj* obj)", ActorSourceAbi::gobj_callback, d_25_func_802DCF4C_6C4D2C_functions, {}, {}, {}, d_25_func_802DCF4C_6C4D2C_headers, {}, {}, {}},
    {0x802dcf84u, "func_802DCF84_6C4D64", "src/river/6C4CD0.c", 30, 33, "void func_802DCF84_6C4D64(GObj* obj)", ActorSourceAbi::gobj_callback, d_30_func_802DCF84_6C4D64_functions, {}, {}, {}, d_30_func_802DCF84_6C4D64_headers, {}, {}, {}},
    {0x802dcfbcu, "func_802DCFBC_6C4D9C", "src/river/6C4CD0.c", 35, 43, "POKEMON_FUNC(func_802DCFBC_6C4D9C)", ActorSourceAbi::gobj_callback, d_35_func_802DCFBC_6C4D9C_functions, {}, {}, {}, {}, {}, d_35_func_802DCFBC_6C4D9C_services, {}},
    {0x802dd068u, "func_802DD068_6C4E48", "src/river/6C4CD0.c", 45, 53, "void func_802DD068_6C4E48(GObj* obj)", ActorSourceAbi::gobj_callback, shared_actor_source_func_802DD068_6C4E48, {}, {}, {}, {}, {}, {}, {}},
    {0x802dd0a8u, "func_802DD0A8_6C4E88", "src/river/6C4CD0.c", 55, 57, "GObj* func_802DD0A8_6C4E88(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)", ActorSourceAbi::factory_six, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802dd0e0u, "func_802DD0E0_6C4EC0", "src/river/6C4EC0.c", 79, 107, "void func_802DD0E0_6C4EC0(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_79_func_802DD0E0_6C4EC0_graphs, {}, {}, d_79_func_802DD0E0_6C4EC0_headers, {}, d_79_func_802DD0E0_6C4EC0_services, {}},
    {0x802dd1d4u, "func_802DD1D4_6C4FB4", "src/river/6C4EC0.c", 109, 130, "void func_802DD1D4_6C4FB4(GObj* obj)", ActorSourceAbi::gobj_callback, d_109_func_802DD1D4_6C4FB4_functions, {}, {}, {}, {}, {}, d_109_func_802DD1D4_6C4FB4_services, {}},
    {0x802dd2bcu, "func_802DD2BC_6C509C", "src/river/6C4EC0.c", 132, 145, "POKEMON_FUNC(func_802DD2BC_6C509C)", ActorSourceAbi::gobj_callback, d_132_func_802DD2BC_6C509C_functions, d_132_func_802DD2BC_6C509C_graphs, {}, {}, d_132_func_802DD2BC_6C509C_headers, {}, d_132_func_802DD2BC_6C509C_services, {}},
    {0x802dd398u, "func_802DD398_6C5178", "src/river/6C4EC0.c", 147, 168, "POKEMON_FUNC(func_802DD398_6C5178)", ActorSourceAbi::gobj_callback, shared_source_func_802DD398_6C5178, {}, {}, {}, {}, {}, d_147_func_802DD398_6C5178_services, {}},
    {0x802dd4acu, "func_802DD4AC_6C528C", "src/river/6C4EC0.c", 170, 183, "void func_802DD4AC_6C528C(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, d_170_func_802DD4AC_6C528C_services, {}},
    {0x802dd53cu, "func_802DD53C_6C531C", "src/river/6C4EC0.c", 185, 229, "void func_802DD53C_6C531C(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, d_185_func_802DD53C_6C531C_services, {}},
    {0x802dd6f0u, "func_802DD6F0_6C54D0", "src/river/6C4EC0.c", 231, 239, "void func_802DD6F0_6C54D0(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_231_func_802DD6F0_6C54D0_graphs, d_231_func_802DD6F0_6C54D0_weighted_states, {}, d_231_func_802DD6F0_6C54D0_headers, {}, d_231_func_802DD6F0_6C54D0_services, {}},
    {0x802dd74cu, "func_802DD74C_6C552C", "src/river/6C4EC0.c", 241, 251, "void func_802DD74C_6C552C(GObj* obj)", ActorSourceAbi::gobj_callback, d_241_func_802DD74C_6C552C_functions, d_241_func_802DD74C_6C552C_graphs, d_241_func_802DD74C_6C552C_weighted_states, {}, d_241_func_802DD74C_6C552C_headers, {}, d_241_func_802DD74C_6C552C_services, {}},
    {0x802dd7ccu, "func_802DD7CC_6C55AC", "src/river/6C4EC0.c", 253, 262, "void func_802DD7CC_6C55AC(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802dd828u, "func_802DD828_6C5608", "src/river/6C4EC0.c", 264, 274, "void func_802DD828_6C5608(GObj* obj)", ActorSourceAbi::gobj_callback, d_264_func_802DD828_6C5608_functions, d_264_func_802DD828_6C5608_graphs, d_264_func_802DD828_6C5608_weighted_states, {}, d_264_func_802DD828_6C5608_headers, {}, d_264_func_802DD828_6C5608_services, {}},
    {0x802dd8a8u, "func_802DD8A8_6C5688", "src/river/6C4EC0.c", 276, 285, "void func_802DD8A8_6C5688(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802dd904u, "func_802DD904_6C56E4", "src/river/6C4EC0.c", 287, 296, "void func_802DD904_6C56E4(GObj* obj)", ActorSourceAbi::gobj_callback, d_287_func_802DD904_6C56E4_functions, {}, {}, {}, d_287_func_802DD904_6C56E4_headers, {}, {}, {}},
    {0x802dd96cu, "func_802DD96C_6C574C", "src/river/6C4EC0.c", 298, 307, "void func_802DD96C_6C574C(GObj* obj)", ActorSourceAbi::gobj_callback, d_298_func_802DD96C_6C574C_functions, d_298_func_802DD96C_6C574C_graphs, {}, {}, d_298_func_802DD96C_6C574C_headers, {}, d_298_func_802DD96C_6C574C_services, {}},
    {0x802dd9dcu, "func_802DD9DC_6C57BC", "src/river/6C4EC0.c", 309, 324, "void func_802DD9DC_6C57BC(GObj* obj)", ActorSourceAbi::gobj_callback, d_309_func_802DD9DC_6C57BC_functions, d_309_func_802DD9DC_6C57BC_graphs, {}, {}, d_309_func_802DD9DC_6C57BC_headers, {}, d_309_func_802DD9DC_6C57BC_services, {}},
    {0x802ddaa0u, "func_802DDAA0_6C5880", "src/river/6C4EC0.c", 326, 334, "void func_802DDAA0_6C5880(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802ddaecu, "func_802DDAEC_6C58CC", "src/river/6C4EC0.c", 336, 347, "void func_802DDAEC_6C58CC(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, d_336_func_802DDAEC_6C58CC_headers, {}, {}, {}},
    {0x802ddb70u, "func_802DDB70_6C5950", "src/river/6C4EC0.c", 349, 364, "void func_802DDB70_6C5950(GObj* obj)", ActorSourceAbi::gobj_callback, d_349_func_802DDB70_6C5950_functions, d_349_func_802DDB70_6C5950_graphs, {}, {}, d_349_func_802DDB70_6C5950_headers, {}, d_349_func_802DDB70_6C5950_services, {}},
    {0x802ddc28u, "func_802DDC28_6C5A08", "src/river/6C4EC0.c", 366, 371, "void func_802DDC28_6C5A08(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802ddc94u, "func_802DDC94_6C5A74", "src/river/6C4EC0.c", 373, 375, "void func_802DDC94_6C5A74(GObj* obj)", ActorSourceAbi::gobj_callback, d_373_func_802DDC94_6C5A74_functions, {}, {}, {}, {}, {}, {}, {}},
    {0x802ddcb8u, "func_802DDCB8_6C5A98", "src/river/6C4EC0.c", 377, 387, "void func_802DDCB8_6C5A98(GObj* obj)", ActorSourceAbi::gobj_callback, d_377_func_802DDCB8_6C5A98_functions, d_377_func_802DDCB8_6C5A98_graphs, d_377_func_802DDCB8_6C5A98_weighted_states, {}, d_377_func_802DDCB8_6C5A98_headers, {}, d_377_func_802DDCB8_6C5A98_services, {}},
    {0x802ddd38u, "func_802DDD38_6C5B18", "src/river/6C4EC0.c", 389, 399, "void func_802DDD38_6C5B18(GObj* obj)", ActorSourceAbi::gobj_callback, d_389_func_802DDD38_6C5B18_functions, d_389_func_802DDD38_6C5B18_graphs, d_389_func_802DDD38_6C5B18_weighted_states, {}, d_389_func_802DDD38_6C5B18_headers, {}, d_389_func_802DDD38_6C5B18_services, {}},
    {0x802dddb8u, "func_802DDDB8_6C5B98", "src/river/6C4EC0.c", 401, 410, "void func_802DDDB8_6C5B98(GObj* obj)", ActorSourceAbi::gobj_callback, d_401_func_802DDDB8_6C5B98_functions, {}, {}, {}, d_401_func_802DDDB8_6C5B98_headers, {}, {}, {}},
    {0x802dde20u, "func_802DDE20_6C5C00", "src/river/6C4EC0.c", 412, 421, "void func_802DDE20_6C5C00(GObj* obj)", ActorSourceAbi::gobj_callback, d_412_func_802DDE20_6C5C00_functions, d_412_func_802DDE20_6C5C00_graphs, {}, {}, d_412_func_802DDE20_6C5C00_headers, {}, d_412_func_802DDE20_6C5C00_services, {}},
    {0x802dde90u, "func_802DDE90_6C5C70", "src/river/6C4EC0.c", 423, 438, "void func_802DDE90_6C5C70(GObj* obj)", ActorSourceAbi::gobj_callback, d_423_func_802DDE90_6C5C70_functions, d_423_func_802DDE90_6C5C70_graphs, {}, {}, d_423_func_802DDE90_6C5C70_headers, {}, d_423_func_802DDE90_6C5C70_services, {}},
    {0x802ddf54u, "func_802DDF54_6C5D34", "src/river/6C4EC0.c", 440, 448, "void func_802DDF54_6C5D34(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802ddfa0u, "func_802DDFA0_6C5D80", "src/river/6C4EC0.c", 450, 461, "void func_802DDFA0_6C5D80(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, d_450_func_802DDFA0_6C5D80_headers, {}, {}, {}},
    {0x802de024u, "func_802DE024_6C5E04", "src/river/6C4EC0.c", 463, 478, "void func_802DE024_6C5E04(GObj* obj)", ActorSourceAbi::gobj_callback, d_463_func_802DE024_6C5E04_functions, d_463_func_802DE024_6C5E04_graphs, {}, {}, d_463_func_802DE024_6C5E04_headers, {}, d_463_func_802DE024_6C5E04_services, {}},
    {0x802de0dcu, "func_802DE0DC_6C5EBC", "src/river/6C4EC0.c", 480, 482, "void func_802DE0DC_6C5EBC(GObj* obj)", ActorSourceAbi::gobj_callback, d_480_func_802DE0DC_6C5EBC_functions, {}, {}, {}, {}, {}, {}, {}},
    {0x802de100u, "func_802DE100_6C5EE0", "src/river/6C4EC0.c", 484, 494, "void func_802DE100_6C5EE0(GObj* obj)", ActorSourceAbi::gobj_callback, d_484_func_802DE100_6C5EE0_functions, d_484_func_802DE100_6C5EE0_graphs, d_484_func_802DE100_6C5EE0_weighted_states, {}, d_484_func_802DE100_6C5EE0_headers, {}, d_484_func_802DE100_6C5EE0_services, {}},
    {0x802de180u, "func_802DE180_6C5F60", "src/river/6C4EC0.c", 496, 506, "void func_802DE180_6C5F60(GObj* obj)", ActorSourceAbi::gobj_callback, d_496_func_802DE180_6C5F60_functions, d_496_func_802DE180_6C5F60_graphs, d_496_func_802DE180_6C5F60_weighted_states, {}, d_496_func_802DE180_6C5F60_headers, {}, d_496_func_802DE180_6C5F60_services, {}},
    {0x802de200u, "func_802DE200_6C5FE0", "src/river/6C4EC0.c", 508, 517, "void func_802DE200_6C5FE0(GObj* obj)", ActorSourceAbi::gobj_callback, d_508_func_802DE200_6C5FE0_functions, {}, {}, {}, d_508_func_802DE200_6C5FE0_headers, {}, {}, {}},
    {0x802de268u, "func_802DE268_6C6048", "src/river/6C4EC0.c", 519, 528, "void func_802DE268_6C6048(GObj* obj)", ActorSourceAbi::gobj_callback, d_519_func_802DE268_6C6048_functions, d_519_func_802DE268_6C6048_graphs, {}, {}, d_519_func_802DE268_6C6048_headers, {}, d_519_func_802DE268_6C6048_services, {}},
    {0x802de2d8u, "func_802DE2D8_6C60B8", "src/river/6C4EC0.c", 530, 545, "void func_802DE2D8_6C60B8(GObj* obj)", ActorSourceAbi::gobj_callback, d_530_func_802DE2D8_6C60B8_functions, d_530_func_802DE2D8_6C60B8_graphs, {}, {}, d_530_func_802DE2D8_6C60B8_headers, {}, d_530_func_802DE2D8_6C60B8_services, {}},
    {0x802de39cu, "func_802DE39C_6C617C", "src/river/6C4EC0.c", 547, 555, "void func_802DE39C_6C617C(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802de3e8u, "func_802DE3E8_6C61C8", "src/river/6C4EC0.c", 557, 568, "void func_802DE3E8_6C61C8(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, d_557_func_802DE3E8_6C61C8_headers, {}, {}, {}},
    {0x802de46cu, "func_802DE46C_6C624C", "src/river/6C4EC0.c", 570, 585, "void func_802DE46C_6C624C(GObj* obj)", ActorSourceAbi::gobj_callback, d_570_func_802DE46C_6C624C_functions, d_570_func_802DE46C_6C624C_graphs, {}, {}, d_570_func_802DE46C_6C624C_headers, {}, d_570_func_802DE46C_6C624C_services, {}},
    {0x802de524u, "func_802DE524_6C6304", "src/river/6C4EC0.c", 587, 668, "void func_802DE524_6C6304(GObj* obj)", ActorSourceAbi::gobj_callback, shared_source_func_802DE524_6C6304, {}, {}, {}, d_587_func_802DE524_6C6304_headers, {}, d_587_func_802DE524_6C6304_services, {}},
    {0x802de870u, "func_802DE870_6C6650", "src/river/6C4EC0.c", 670, 676, "void func_802DE870_6C6650(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, d_670_func_802DE870_6C6650_services, {}},
    {0x802de8bcu, "func_802DE8BC_6C669C", "src/river/6C4EC0.c", 678, 680, "GObj* func_802DE8BC_6C669C(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)", ActorSourceAbi::factory_six, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802de900u, "func_802DE900_6C66E0", "src/river/6C66E0.c", 19, 24, "POKEMON_FUNC(func_802DE900_6C66E0)", ActorSourceAbi::gobj_callback, {}, d_19_func_802DE900_6C66E0_graphs, {}, {}, d_19_func_802DE900_6C66E0_headers, {}, d_19_func_802DE900_6C66E0_services, {}},
    {0x802de958u, "func_802DE958_6C6738", "src/river/6C66E0.c", 26, 32, "POKEMON_FUNC(func_802DE958_6C6738)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, d_26_func_802DE958_6C6738_headers, {}, d_26_func_802DE958_6C6738_services, {}},
    {0x802de9c0u, "func_802DE9C0_6C67A0", "src/river/6C66E0.c", 34, 38, "GObj* func_802DE9C0_6C67A0(s32 gObjID, u16 id, WorldBlock* roomA, WorldBlock* roomB, ObjectSpawn* spawn)", ActorSourceAbi::factory_five, {}, {}, {}, {}, {}, {}, d_34_func_802DE9C0_6C67A0_services, {}},
    {0x802dea00u, "func_802DEA00_6C67E0", "src/river/6C66E0.c", 40, 60, "POKEMON_FUNC(func_802DEA00_6C67E0)", ActorSourceAbi::gobj_callback, shared_source_func_802DEA00_6C67E0, {}, {}, {}, {}, {}, d_40_func_802DEA00_6C67E0_services, {}},
    {0x802deb00u, "func_802DEB00_6C68E0", "src/river/6C66E0.c", 62, 77, "POKEMON_FUNC(func_802DEB00_6C68E0)", ActorSourceAbi::gobj_callback, shared_source_func_802DEB00_6C68E0, {}, {}, {}, {}, {}, d_62_func_802DEB00_6C68E0_services, {}},
    {0x802debb8u, "func_802DEBB8_6C6998", "src/river/6C66E0.c", 79, 89, "POKEMON_FUNC(func_802DEBB8_6C6998)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, d_79_func_802DEBB8_6C6998_services, {}},
    {0x802dec50u, "func_802DEC50_6C6A30", "src/river/6C66E0.c", 91, 93, "GObj* func_802DEC50_6C6A30(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)", ActorSourceAbi::factory_six, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802dec90u, "func_802DEC90_6C6A70", "src/river/6C6A70.c", 33, 38, "POKEMON_FUNC(func_802DEC90_6C6A70)", ActorSourceAbi::gobj_callback, d_33_func_802DEC90_6C6A70_functions, {}, {}, {}, {}, {}, d_33_func_802DEC90_6C6A70_services, {}},
    {0x802ded0cu, "func_802DED0C_6C6AEC", "src/river/6C6A70.c", 40, 45, "POKEMON_FUNC(func_802DED0C_6C6AEC)", ActorSourceAbi::gobj_callback, d_40_func_802DED0C_6C6AEC_functions, d_40_func_802DED0C_6C6AEC_graphs, {}, {}, d_40_func_802DED0C_6C6AEC_headers, {}, d_40_func_802DED0C_6C6AEC_services, {}},
    {0x802ded68u, "func_802DED68_6C6B48", "src/river/6C6A70.c", 47, 53, "POKEMON_FUNC(func_802DED68_6C6B48)", ActorSourceAbi::gobj_callback, d_47_func_802DED68_6C6B48_functions, {}, {}, {}, d_47_func_802DED68_6C6B48_headers, {}, d_47_func_802DED68_6C6B48_services, {}},
    {0x802dedd4u, "func_802DEDD4_6C6BB4", "src/river/6C6A70.c", 55, 60, "POKEMON_FUNC(func_802DEDD4_6C6BB4)", ActorSourceAbi::gobj_callback, {}, d_55_func_802DEDD4_6C6BB4_graphs, {}, {}, d_55_func_802DEDD4_6C6BB4_headers, {}, d_55_func_802DEDD4_6C6BB4_services, {}},
    {0x802dee2cu, "func_802DEE2C_6C6C0C", "src/river/6C6A70.c", 62, 68, "POKEMON_FUNC(func_802DEE2C_6C6C0C)", ActorSourceAbi::gobj_callback, d_62_func_802DEE2C_6C6C0C_functions, d_62_func_802DEE2C_6C6C0C_graphs, {}, {}, d_62_func_802DEE2C_6C6C0C_headers, {}, d_62_func_802DEE2C_6C6C0C_services, {}},
    {0x802deea0u, "func_802DEEA0_6C6C80", "src/river/6C6A70.c", 70, 76, "POKEMON_FUNC(func_802DEEA0_6C6C80)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, d_70_func_802DEEA0_6C6C80_services, {}},
    {0x802def28u, "func_802DEF28_6C6D08", "src/river/6C6A70.c", 78, 84, "POKEMON_FUNC(func_802DEF28_6C6D08)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, d_78_func_802DEF28_6C6D08_services, {}},
    {0x802def88u, "func_802DEF88_6C6D68", "src/river/6C6A70.c", 86, 88, "GObj* func_802DEF88_6C6D68(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)", ActorSourceAbi::factory_six, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802defc0u, "func_802DEFC0_6C6DA0", "src/river/6C6A70.c", 90, 104, "POKEMON_FUNC(func_802DEFC0_6C6DA0)", ActorSourceAbi::gobj_callback, shared_source_func_802DEFC0_6C6DA0, {}, {}, {}, {}, {}, d_90_func_802DEFC0_6C6DA0_services, {}},
    {0x802df078u, "func_802DF078_6C6E58", "src/river/6C6A70.c", 106, 110, "void func_802DF078_6C6E58(GObj* obj)", ActorSourceAbi::gobj_callback, d_106_func_802DF078_6C6E58_functions, {}, {}, {}, {}, {}, {}, {}},
    {0x802df0b8u, "func_802DF0B8_6C6E98", "src/river/6C6A70.c", 112, 120, "POKEMON_FUNC(func_802DF0B8_6C6E98)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, d_112_func_802DF0B8_6C6E98_services, {}},
    {0x802df104u, "func_802DF104_6C6EE4", "src/river/6C6A70.c", 122, 130, "POKEMON_FUNC(func_802DF104_6C6EE4)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, d_122_func_802DF104_6C6EE4_services, {}},
    {0x802df154u, "func_802DF154_6C6F34", "src/river/6C6A70.c", 132, 142, "POKEMON_FUNC(func_802DF154_6C6F34)", ActorSourceAbi::gobj_callback, {}, d_132_func_802DF154_6C6F34_graphs, {}, {}, d_132_func_802DF154_6C6F34_headers, {}, d_132_func_802DF154_6C6F34_services, {}},
    {0x802df1f0u, "func_802DF1F0_6C6FD0", "src/river/6C6A70.c", 144, 163, "POKEMON_FUNC(func_802DF1F0_6C6FD0)", ActorSourceAbi::gobj_callback, d_144_func_802DF1F0_6C6FD0_functions, d_144_func_802DF1F0_6C6FD0_graphs, {}, {}, d_144_func_802DF1F0_6C6FD0_headers, {}, d_144_func_802DF1F0_6C6FD0_services, {}},
    {0x802df360u, "func_802DF360_6C7140", "src/river/6C6A70.c", 165, 175, "POKEMON_FUNC(func_802DF360_6C7140)", ActorSourceAbi::gobj_callback, d_165_func_802DF360_6C7140_functions, d_165_func_802DF360_6C7140_graphs, {}, {}, d_165_func_802DF360_6C7140_headers, {}, d_165_func_802DF360_6C7140_services, {}},
    {0x802df404u, "func_802DF404_6C71E4", "src/river/6C6A70.c", 177, 183, "POKEMON_FUNC(func_802DF404_6C71E4)", ActorSourceAbi::gobj_callback, d_177_func_802DF404_6C71E4_functions, d_177_func_802DF404_6C71E4_graphs, {}, {}, d_177_func_802DF404_6C71E4_headers, {}, d_177_func_802DF404_6C71E4_services, {}},
    {0x802df478u, "func_802DF478_6C7258", "src/river/6C6A70.c", 185, 191, "POKEMON_FUNC(func_802DF478_6C7258)", ActorSourceAbi::gobj_callback, d_185_func_802DF478_6C7258_functions, d_185_func_802DF478_6C7258_graphs, {}, {}, d_185_func_802DF478_6C7258_headers, {}, d_185_func_802DF478_6C7258_services, {}},
    {0x802df4ecu, "func_802DF4EC_6C72CC", "src/river/6C6A70.c", 193, 199, "POKEMON_FUNC(func_802DF4EC_6C72CC)", ActorSourceAbi::gobj_callback, d_193_func_802DF4EC_6C72CC_functions, d_193_func_802DF4EC_6C72CC_graphs, {}, {}, d_193_func_802DF4EC_6C72CC_headers, {}, d_193_func_802DF4EC_6C72CC_services, {}},
    {0x802df560u, "func_802DF560_6C7340", "src/river/6C6A70.c", 201, 207, "POKEMON_FUNC(func_802DF560_6C7340)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, d_201_func_802DF560_6C7340_services, {}},
    {0x802df5b8u, "func_802DF5B8_6C7398", "src/river/6C6A70.c", 209, 217, "POKEMON_FUNC(func_802DF5B8_6C7398)", ActorSourceAbi::gobj_callback, d_209_func_802DF5B8_6C7398_functions, {}, {}, {}, d_209_func_802DF5B8_6C7398_headers, {}, d_209_func_802DF5B8_6C7398_services, {}},
    {0x802df634u, "func_802DF634_6C7414", "src/river/6C6A70.c", 219, 221, "GObj* func_802DF634_6C7414(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)", ActorSourceAbi::factory_six, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802df670u, "func_802DF670_6C7450", "src/river/6C7450.c", 30, 37, "void func_802DF670_6C7450(GObj* obj)", ActorSourceAbi::gobj_callback, d_30_func_802DF670_6C7450_functions, {}, {}, {}, {}, {}, {}, {}},
    {0x802df6bcu, "func_802DF6BC_6C749C", "src/river/6C7450.c", 39, 47, "void func_802DF6BC_6C749C(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_39_func_802DF6BC_6C749C_graphs, {}, {}, {}, {}, d_39_func_802DF6BC_6C749C_services, {}},
    {0x802df710u, "func_802DF710_6C74F0", "src/river/6C7450.c", 49, 55, "void func_802DF710_6C74F0(GObj* obj)", ActorSourceAbi::gobj_callback, d_49_func_802DF710_6C74F0_functions, {}, {}, {}, {}, {}, {}, {}},
    {0x802df758u, "func_802DF758_6C7538", "src/river/6C7450.c", 57, 67, "void func_802DF758_6C7538(GObj* obj)", ActorSourceAbi::gobj_callback, d_57_func_802DF758_6C7538_functions, d_57_func_802DF758_6C7538_graphs, {}, {}, d_57_func_802DF758_6C7538_headers, {}, d_57_func_802DF758_6C7538_services, {}},
    {0x802df7d8u, "func_802DF7D8_6C75B8", "src/river/6C7450.c", 69, 78, "void func_802DF7D8_6C75B8(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802df844u, "func_802DF844_6C7624", "src/river/6C7450.c", 80, 91, "void func_802DF844_6C7624(GObj* obj)", ActorSourceAbi::gobj_callback, d_80_func_802DF844_6C7624_functions, d_80_func_802DF844_6C7624_graphs, {}, {}, d_80_func_802DF844_6C7624_headers, {}, d_80_func_802DF844_6C7624_services, {}},
    {0x802df8c4u, "func_802DF8C4_6C76A4", "src/river/6C7450.c", 93, 102, "void func_802DF8C4_6C76A4(GObj* obj)", ActorSourceAbi::gobj_callback, d_93_func_802DF8C4_6C76A4_functions, {}, {}, {}, {}, {}, {}, {}},
    {0x802df91cu, "func_802DF91C_6C76FC", "src/river/6C7450.c", 104, 116, "POKEMON_FUNC(func_802DF91C_6C76FC)", ActorSourceAbi::gobj_callback, d_104_func_802DF91C_6C76FC_functions, d_104_func_802DF91C_6C76FC_graphs, {}, {}, d_104_func_802DF91C_6C76FC_headers, {}, d_104_func_802DF91C_6C76FC_services, {}},
    {0x802df9ccu, "func_802DF9CC_6C77AC", "src/river/6C7450.c", 118, 130, "void func_802DF9CC_6C77AC(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802dfa30u, "func_802DFA30_6C7810", "src/river/6C7450.c", 132, 138, "void func_802DFA30_6C7810(GObj* obj)", ActorSourceAbi::gobj_callback, d_132_func_802DFA30_6C7810_functions, {}, {}, {}, {}, {}, d_132_func_802DFA30_6C7810_services, {}},
    {0x802dfa5cu, "func_802DFA5C_6C783C", "src/river/6C7450.c", 140, 162, "void func_802DFA5C_6C783C(GObj* obj)", ActorSourceAbi::gobj_callback, d_140_func_802DFA5C_6C783C_functions, d_140_func_802DFA5C_6C783C_graphs, {}, {}, d_140_func_802DFA5C_6C783C_headers, {}, d_140_func_802DFA5C_6C783C_services, {}},
    {0x802dfb74u, "func_802DFB74_6C7954", "src/river/6C7450.c", 164, 184, "void func_802DFB74_6C7954(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, d_164_func_802DFB74_6C7954_services, {}},
    {0x802dfc38u, "func_802DFC38_6C7A18", "src/river/6C7450.c", 186, 201, "void func_802DFC38_6C7A18(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, d_186_func_802DFC38_6C7A18_services, {}},
    {0x802dfcd0u, "func_802DFCD0_6C7AB0", "src/river/6C7450.c", 203, 216, "void func_802DFCD0_6C7AB0(GObj* obj)", ActorSourceAbi::gobj_callback, d_203_func_802DFCD0_6C7AB0_functions, d_203_func_802DFCD0_6C7AB0_graphs, {}, {}, d_203_func_802DFCD0_6C7AB0_headers, {}, d_203_func_802DFCD0_6C7AB0_services, {}},
    {0x802dfd78u, "func_802DFD78_6C7B58", "src/river/6C7450.c", 218, 230, "void func_802DFD78_6C7B58(GObj* obj)", ActorSourceAbi::gobj_callback, d_218_func_802DFD78_6C7B58_functions, {}, {}, {}, {}, {}, d_218_func_802DFD78_6C7B58_services, {}},
    {0x802dfdf8u, "func_802DFDF8_6C7BD8", "src/river/6C7450.c", 232, 240, "void func_802DFDF8_6C7BD8(GObj* obj)", ActorSourceAbi::gobj_callback, d_232_func_802DFDF8_6C7BD8_functions, {}, {}, {}, d_232_func_802DFDF8_6C7BD8_headers, {}, {}, {}},
    {0x802dfe4cu, "func_802DFE4C_6C7C2C", "src/river/6C7450.c", 242, 252, "void func_802DFE4C_6C7C2C(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, d_242_func_802DFE4C_6C7C2C_services, {}},
    {0x802dfeb0u, "func_802DFEB0_6C7C90", "src/river/6C7450.c", 254, 256, "GObj* func_802DFEB0_6C7C90(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)", ActorSourceAbi::factory_six, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802dfef0u, "func_802DFEF0_6C7CD0", "src/river/6C7CD0.c", 10, 17, "POKEMON_FUNC(func_802DFEF0_6C7CD0)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, d_10_func_802DFEF0_6C7CD0_headers, {}, d_10_func_802DFEF0_6C7CD0_services, {}},
    {0x802dff5cu, "func_802DFF5C_6C7D3C", "src/river/6C7CD0.c", 19, 21, "GObj* func_802DFF5C_6C7D3C(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)", ActorSourceAbi::factory_six, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802dff94u, "func_802DFF94_6C7D74", "src/river/6C7CD0.c", 23, 51, "void func_802DFF94_6C7D74(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, d_func_802DFF94_6C7D74_child_factories, d_23_func_802DFF94_6C7D74_services, {}},
    {0x802e0070u, "func_802E0070_6C7E50", "src/river/6C7E50.c", 28, 30, "void func_802E0070_6C7E50(GObj* obj)", ActorSourceAbi::gobj_callback, d_28_func_802E0070_6C7E50_functions, {}, {}, {}, {}, {}, {}, {}},
    {0x802e0094u, "func_802E0094_6C7E74", "src/river/6C7E50.c", 32, 40, "void func_802E0094_6C7E74(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_32_func_802E0094_6C7E74_graphs, {}, {}, d_32_func_802E0094_6C7E74_headers, {}, d_32_func_802E0094_6C7E74_services, {}},
    {0x802e00ecu, "func_802E00EC_6C7ECC", "src/river/6C7E50.c", 42, 60, "void func_802E00EC_6C7ECC(GObj* obj)", ActorSourceAbi::gobj_callback, d_42_func_802E00EC_6C7ECC_functions, {}, {}, {}, d_42_func_802E00EC_6C7ECC_headers, {}, d_42_func_802E00EC_6C7ECC_services, {}},
    {0x802e01c8u, "func_802E01C8_6C7FA8", "src/river/6C7E50.c", 62, 66, "void func_802E01C8_6C7FA8(DObj* arg0, s32 arg1, f32 arg2)", ActorSourceAbi::animation, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802e01f4u, "func_802E01F4_6C7FD4", "src/river/6C7E50.c", 68, 87, "void func_802E01F4_6C7FD4(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, d_68_func_802E01F4_6C7FD4_services, {}},
    {0x802e0334u, "func_802E0334_6C8114", "src/river/6C7E50.c", 89, 100, "void func_802E0334_6C8114(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, d_89_func_802E0334_6C8114_services, {}},
    {0x802e03c0u, "func_802E03C0_6C81A0", "src/river/6C7E50.c", 102, 142, "void func_802E03C0_6C81A0(GObj* obj)", ActorSourceAbi::gobj_callback, shared_source_func_802E03C0_6C81A0, {}, {}, {}, {}, {}, d_102_func_802E03C0_6C81A0_services, {}},
    {0x802e0584u, "func_802E0584_6C8364", "src/river/6C7E50.c", 144, 146, "GObj* func_802E0584_6C8364(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)", ActorSourceAbi::factory_six, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802e05c0u, "func_802E05C0_6C83A0", "src/river/6C83A0.c", 11, 19, "void func_802E05C0_6C83A0(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_11_func_802E05C0_6C83A0_graphs, {}, {}, d_11_func_802E05C0_6C83A0_headers, {}, d_11_func_802E05C0_6C83A0_services, {}},
    {0x802e0618u, "func_802E0618_6C83F8", "src/river/6C83A0.c", 21, 32, "void func_802E0618_6C83F8(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, d_21_func_802E0618_6C83F8_headers, {}, {}, {}},
    {0x802e0694u, "func_802E0694_6C8474", "src/river/6C83A0.c", 34, 36, "GObj* func_802E0694_6C8474(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)", ActorSourceAbi::factory_six, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802e06d0u, "func_802E06D0_6C84B0", "src/river/6C84B0.c", 11, 19, "void func_802E06D0_6C84B0(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_11_func_802E06D0_6C84B0_graphs, {}, {}, d_11_func_802E06D0_6C84B0_headers, {}, d_11_func_802E06D0_6C84B0_services, {}},
    {0x802e0728u, "func_802E0728_6C8508", "src/river/6C84B0.c", 21, 32, "void func_802E0728_6C8508(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, d_21_func_802E0728_6C8508_headers, {}, {}, {}},
    {0x802e07a4u, "func_802E07A4_6C8584", "src/river/6C84B0.c", 34, 36, "GObj* func_802E07A4_6C8584(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)", ActorSourceAbi::factory_six, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802e07f0u, "func_802E07F0_6C85D0", "src/river/6C85D0.c", 11, 19, "void func_802E07F0_6C85D0(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_11_func_802E07F0_6C85D0_graphs, {}, {}, d_11_func_802E07F0_6C85D0_headers, {}, d_11_func_802E07F0_6C85D0_services, {}},
    {0x802e0848u, "func_802E0848_6C8628", "src/river/6C85D0.c", 21, 32, "void func_802E0848_6C8628(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, d_21_func_802E0848_6C8628_headers, {}, {}, {}},
    {0x802e08c4u, "func_802E08C4_6C86A4", "src/river/6C85D0.c", 34, 36, "GObj* func_802E08C4_6C86A4(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)", ActorSourceAbi::factory_six, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802e0900u, "func_802E0900_6C86E0", "src/river/6C86E0.c", 46, 61, "void func_802E0900_6C86E0(GObj* obj)", ActorSourceAbi::gobj_callback, shared_source_func_802E0900_6C86E0, {}, {}, {}, {}, {}, {}, {}},
    {0x802e09a8u, "func_802E09A8_6C8788", "src/river/6C86E0.c", 63, 78, "void func_802E09A8_6C8788(GObj* obj)", ActorSourceAbi::gobj_callback, d_63_func_802E09A8_6C8788_functions, {}, {}, {}, {}, {}, {}, {}},
    {0x802e0a48u, "func_802E0A48_6C8828", "src/river/6C86E0.c", 80, 89, "void func_802E0A48_6C8828(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_80_func_802E0A48_6C8828_graphs, d_80_func_802E0A48_6C8828_weighted_states, {}, d_80_func_802E0A48_6C8828_headers, {}, d_80_func_802E0A48_6C8828_services, {}},
    {0x802e0ab8u, "func_802E0AB8_6C8898", "src/river/6C86E0.c", 91, 100, "void func_802E0AB8_6C8898(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_91_func_802E0AB8_6C8898_graphs, d_91_func_802E0AB8_6C8898_weighted_states, {}, d_91_func_802E0AB8_6C8898_headers, {}, d_91_func_802E0AB8_6C8898_services, {}},
    {0x802e0b28u, "func_802E0B28_6C8908", "src/river/6C86E0.c", 102, 111, "void func_802E0B28_6C8908(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_102_func_802E0B28_6C8908_graphs, d_102_func_802E0B28_6C8908_weighted_states, {}, d_102_func_802E0B28_6C8908_headers, {}, d_102_func_802E0B28_6C8908_services, {}},
    {0x802e0b98u, "func_802E0B98_6C8978", "src/river/6C86E0.c", 113, 126, "void func_802E0B98_6C8978(GObj* obj)", ActorSourceAbi::gobj_callback, d_113_func_802E0B98_6C8978_functions, d_113_func_802E0B98_6C8978_graphs, d_113_func_802E0B98_6C8978_weighted_states, {}, d_113_func_802E0B98_6C8978_headers, {}, d_113_func_802E0B98_6C8978_services, {}},
    {0x802e0c3cu, "func_802E0C3C_6C8A1C", "src/river/6C86E0.c", 128, 137, "void func_802E0C3C_6C8A1C(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802e0c98u, "func_802E0C98_6C8A78", "src/river/6C86E0.c", 140, 157, "void func_802E0C98_6C8A78(GObj* obj)", ActorSourceAbi::gobj_callback, d_140_func_802E0C98_6C8A78_functions, {}, {}, {}, d_140_func_802E0C98_6C8A78_headers, {}, {}, {}},
    {0x802e0d44u, "func_802E0D44_6C8B24", "src/river/6C86E0.c", 159, 168, "void func_802E0D44_6C8B24(GObj* obj)", ActorSourceAbi::gobj_callback, d_159_func_802E0D44_6C8B24_functions, {}, {}, {}, d_159_func_802E0D44_6C8B24_headers, {}, {}, {}},
    {0x802e0dacu, "func_802E0DAC_6C8B8C", "src/river/6C86E0.c", 170, 179, "void func_802E0DAC_6C8B8C(GObj* obj)", ActorSourceAbi::gobj_callback, d_170_func_802E0DAC_6C8B8C_functions, d_170_func_802E0DAC_6C8B8C_graphs, {}, {}, d_170_func_802E0DAC_6C8B8C_headers, {}, d_170_func_802E0DAC_6C8B8C_services, {}},
    {0x802e0e20u, "func_802E0E20_6C8C00", "src/river/6C86E0.c", 181, 190, "void func_802E0E20_6C8C00(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802e0e7cu, "func_802E0E7C_6C8C5C", "src/river/6C86E0.c", 192, 203, "void func_802E0E7C_6C8C5C(GObj* obj)", ActorSourceAbi::gobj_callback, d_192_func_802E0E7C_6C8C5C_functions, d_192_func_802E0E7C_6C8C5C_graphs, {}, {}, {}, {}, d_192_func_802E0E7C_6C8C5C_services, {}},
    {0x802e0f04u, "func_802E0F04_6C8CE4", "src/river/6C86E0.c", 206, 220, "void func_802E0F04_6C8CE4(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, d_206_func_802E0F04_6C8CE4_headers, {}, d_206_func_802E0F04_6C8CE4_services, {}},
    {0x802e0f9cu, "func_802E0F9C_6C8D7C", "src/river/6C86E0.c", 222, 234, "void func_802E0F9C_6C8D7C(GObj* obj)", ActorSourceAbi::gobj_callback, d_222_func_802E0F9C_6C8D7C_functions, d_222_func_802E0F9C_6C8D7C_graphs, d_222_func_802E0F9C_6C8D7C_weighted_states, {}, d_222_func_802E0F9C_6C8D7C_headers, {}, d_222_func_802E0F9C_6C8D7C_services, {}},
    {0x802e1030u, "func_802E1030_6C8E10", "src/river/6C86E0.c", 236, 241, "void func_802E1030_6C8E10(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802e1094u, "func_802E1094_6C8E74", "src/river/6C86E0.c", 243, 244, "void func_802E1094_6C8E74(void)", ActorSourceAbi::actor_utility, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802e109cu, "func_802E109C_6C8E7C", "src/river/6C86E0.c", 246, 255, "void func_802E109C_6C8E7C(GObj* obj)", ActorSourceAbi::gobj_callback, d_246_func_802E109C_6C8E7C_functions, d_246_func_802E109C_6C8E7C_graphs, {}, {}, d_246_func_802E109C_6C8E7C_headers, {}, d_246_func_802E109C_6C8E7C_services, {}},
    {0x802e1110u, "func_802E1110_6C8EF0", "src/river/6C86E0.c", 257, 265, "void func_802E1110_6C8EF0(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802e115cu, "func_802E115C_6C8F3C", "src/river/6C86E0.c", 267, 273, "void func_802E115C_6C8F3C(GObj* obj)", ActorSourceAbi::gobj_callback, d_267_func_802E115C_6C8F3C_functions, {}, {}, {}, {}, {}, {}, {}},
    {0x802e1194u, "func_802E1194_6C8F74", "src/river/6C86E0.c", 275, 282, "void func_802E1194_6C8F74(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_275_func_802E1194_6C8F74_graphs, {}, {}, {}, {}, d_275_func_802E1194_6C8F74_services, {}},
    {0x802e11d4u, "func_802E11D4_6C8FB4", "src/river/6C86E0.c", 284, 307, "void func_802E11D4_6C8FB4(GObj* obj)", ActorSourceAbi::gobj_callback, d_284_func_802E11D4_6C8FB4_functions, {}, {}, {}, {}, {}, d_284_func_802E11D4_6C8FB4_services, {}},
    {0x802e12d8u, "func_802E12D8_6C90B8", "src/river/6C86E0.c", 309, 322, "void func_802E12D8_6C90B8(GObj* obj)", ActorSourceAbi::gobj_callback, d_309_func_802E12D8_6C90B8_functions, {}, {}, {}, d_309_func_802E12D8_6C90B8_headers, {}, {}, {}},
    {0x802e1374u, "func_802E1374_6C9154", "src/river/6C86E0.c", 324, 333, "void func_802E1374_6C9154(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, d_324_func_802E1374_6C9154_services, {}},
    {0u, "nullsub2", "src/river/6C86E0.c", 335, 336, "static void nullsub2(void)", ActorSourceAbi::file_local_utility, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802e13f4u, "func_802E13F4_6C91D4", "src/river/6C86E0.c", 338, 341, "void func_802E13F4_6C91D4(GObj* obj)", ActorSourceAbi::gobj_callback, d_338_func_802E13F4_6C91D4_functions, {}, {}, {}, {}, {}, {}, {}},
    {0x802e142cu, "func_802E142C_6C920C", "src/river/6C86E0.c", 343, 351, "void func_802E142C_6C920C(GObj* obj)", ActorSourceAbi::gobj_callback, d_343_func_802E142C_6C920C_functions, d_343_func_802E142C_6C920C_graphs, {}, {}, d_343_func_802E142C_6C920C_headers, {}, d_343_func_802E142C_6C920C_services, {}},
    {0x802e1488u, "func_802E1488_6C9268", "src/river/6C86E0.c", 353, 362, "void func_802E1488_6C9268(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802e14f4u, "func_802E14F4_6C92D4", "src/river/6C86E0.c", 364, 374, "void func_802E14F4_6C92D4(GObj* obj)", ActorSourceAbi::gobj_callback, d_364_func_802E14F4_6C92D4_functions, d_364_func_802E14F4_6C92D4_graphs, {}, {}, d_364_func_802E14F4_6C92D4_headers, {}, d_364_func_802E14F4_6C92D4_services, {}},
    {0x802e1570u, "func_802E1570_6C9350", "src/river/6C86E0.c", 376, 378, "void func_802E1570_6C9350(GObj* obj)", ActorSourceAbi::gobj_callback, d_376_func_802E1570_6C9350_functions, {}, {}, {}, {}, {}, {}, {}},
    {0x802e1594u, "func_802E1594_6C9374", "src/river/6C86E0.c", 380, 389, "void func_802E1594_6C9374(GObj* obj)", ActorSourceAbi::gobj_callback, d_380_func_802E1594_6C9374_functions, d_380_func_802E1594_6C9374_graphs, {}, {}, d_380_func_802E1594_6C9374_headers, {}, d_380_func_802E1594_6C9374_services, {}},
    {0x802e1604u, "func_802E1604_6C93E4", "src/river/6C86E0.c", 391, 402, "void func_802E1604_6C93E4(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, d_391_func_802E1604_6C93E4_services, {}},
    {0x802e1678u, "func_802E1678_6C9458", "src/river/6C86E0.c", 404, 414, "void func_802E1678_6C9458(GObj* obj)", ActorSourceAbi::gobj_callback, d_404_func_802E1678_6C9458_functions, d_404_func_802E1678_6C9458_graphs, {}, {}, d_404_func_802E1678_6C9458_headers, {}, {}, {}},
    {0x802e171cu, "func_802E171C_6C94FC", "src/river/6C86E0.c", 416, 432, "void func_802E171C_6C94FC(GObj* obj)", ActorSourceAbi::gobj_callback, d_416_func_802E171C_6C94FC_functions, {}, {}, {}, d_416_func_802E171C_6C94FC_headers, {}, {}, {}},
    {0x802e17d4u, "func_802E17D4_6C95B4", "src/river/6C86E0.c", 434, 445, "void func_802E17D4_6C95B4(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, d_434_func_802E17D4_6C95B4_services, {}},
    {0u, "nullsub", "src/river/6C86E0.c", 447, 448, "static void nullsub()", ActorSourceAbi::file_local_utility, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802e1864u, "func_802E1864_6C9644", "src/river/6C86E0.c", 450, 452, "GObj* func_802E1864_6C9644(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)", ActorSourceAbi::factory_six, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802e18a0u, "func_802E18A0_6C9680", "src/river/6C9680.c", 32, 34, "void func_802E18A0_6C9680(GObj* obj)", ActorSourceAbi::gobj_callback, d_32_func_802E18A0_6C9680_functions, {}, {}, {}, {}, {}, {}, {}},
    {0x802e18c4u, "func_802E18C4_6C96A4", "src/river/6C9680.c", 36, 44, "void func_802E18C4_6C96A4(GObj* obj)", ActorSourceAbi::gobj_callback, {}, d_36_func_802E18C4_6C96A4_graphs, {}, {}, d_36_func_802E18C4_6C96A4_headers, {}, d_36_func_802E18C4_6C96A4_services, {}},
    {0x802e191cu, "func_802E191C_6C96FC", "src/river/6C9680.c", 46, 51, "void func_802E191C_6C96FC(f32 animSpeed)", ActorSourceAbi::actor_utility, {}, {}, {}, {}, {}, {}, d_46_func_802E191C_6C96FC_services, {}},
    {0x802e1950u, "func_802E1950_6C9730", "src/river/6C9680.c", 53, 58, "void func_802E1950_6C9730(f32 arg0)", ActorSourceAbi::actor_utility, {}, {}, {}, {}, {}, {}, d_53_func_802E1950_6C9730_services, {}},
    {0x802e19a0u, "func_802E19A0_6C9780", "src/river/6C9680.c", 60, 80, "void func_802E19A0_6C9780(DObj* arg0, s32 arg1, f32 arg2)", ActorSourceAbi::animation, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802e1a3cu, "func_802E1A3C_6C981C", "src/river/6C9680.c", 82, 85, "void func_802E1A3C_6C981C(DObj* arg0, s32 arg1, f32 arg2)", ActorSourceAbi::animation, {}, {}, {}, {}, {}, {}, {}, {}},
    {0x802e1a4cu, "func_802E1A4C_6C982C", "src/river/6C9680.c", 87, 132, "void func_802E1A4C_6C982C(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, d_87_func_802E1A4C_6C982C_services, {}},
    {0x802e1cbcu, "func_802E1CBC_6C9A9C", "src/river/6C9680.c", 134, 137, "void func_802E1CBC_6C9A9C(void)", ActorSourceAbi::actor_utility, d_134_func_802E1CBC_6C9A9C_functions, {}, {}, {}, {}, {}, d_134_func_802E1CBC_6C9A9C_services, {}},
    {0x802e1d10u, "func_802E1D10_6C9AF0", "src/river/6C9680.c", 139, 202, "void func_802E1D10_6C9AF0(GObj* obj)", ActorSourceAbi::gobj_callback, shared_source_func_802E1D10_6C9AF0, {}, {}, {}, {}, {}, d_139_func_802E1D10_6C9AF0_services, {}},
    {0x802e2084u, "func_802E2084_6C9E64", "src/river/6C9680.c", 204, 208, "GObj* func_802E2084_6C9E64(s32 gObjID, u16 id, WorldBlock* roomA, WorldBlock* roomB, ObjectSpawn* spawn)", ActorSourceAbi::factory_five, {}, {}, {}, {}, {}, {}, d_204_func_802E2084_6C9E64_services, {}},
    {0x802e20d0u, "func_802E20D0_6C9EB0", "src/river/6C9EB0.c", 20, 24, "void func_802E20D0_6C9EB0(GObj* obj)", ActorSourceAbi::gobj_callback, shared_source_func_802E20D0_6C9EB0, {}, {}, {}, {}, {}, {}, {}},
    {0x802e2100u, "func_802E2100_6C9EE0", "src/river/6C9EB0.c", 26, 28, "void func_802E2100_6C9EE0(void)", ActorSourceAbi::lifecycle_utility, shared_source_func_802E2100_6C9EE0, {}, {}, {}, {}, {}, {}, {}},
    {0x802e2120u, "func_802E2120_6C9F00", "src/river/6C9EB0.c", 30, 46, "void func_802E2120_6C9F00(DObj* dobj, s32 arg1, f32 arg2)", ActorSourceAbi::lifecycle_animation, {}, {}, {}, {}, {}, {}, {}, d_func_802E2120_6C9F00_lifecycle_services},
    {0x802e2194u, "func_802E2194_6C9F74", "src/river/6C9EB0.c", 48, 59, "void func_802E2194_6C9F74(void)", ActorSourceAbi::lifecycle_utility, shared_source_func_802E2194_6C9F74, {}, {}, {}, {}, {}, {}, d_func_802E2194_6C9F74_lifecycle_services},
    {0x802e222cu, "func_802E222C_6CA00C", "src/river/6C9EB0.c", 61, 74, "void func_802E222C_6CA00C(GObj* obj)", ActorSourceAbi::gobj_callback, {}, {}, {}, {}, {}, {}, {}, d_func_802E222C_6CA00C_lifecycle_services},
    {0x802e22e4u, "func_802E22E4_6CA0C4", "src/river/6C9EB0.c", 76, 140, "void func_802E22E4_6CA0C4(GObj* obj)", ActorSourceAbi::gobj_callback, d_76_func_802E22E4_6CA0C4_functions, {}, {}, {}, {}, {}, {}, d_func_802E22E4_6CA0C4_lifecycle_services},
    {0x802e25c4u, "func_802E25C4_6CA3A4", "src/river/6C9EB0.c", 142, 151, "void func_802E25C4_6CA3A4(void)", ActorSourceAbi::lifecycle_utility, d_142_func_802E25C4_6CA3A4_functions, {}, {}, {}, {}, {}, {}, d_func_802E25C4_6CA3A4_lifecycle_services},
};
const ActorFactoryBinding factories[] = {
    {79u,0x802d9f44u,0x802e2bdcu,0x802d9664u,ActorFactoryWrapper::ground,isle_river_adapter_func_802D9F44_6C1D24,nullptr,func_802D9F44_6C1D24},
    {80u,0x802da4b8u,0x802e2e04u,0x802d9f80u,ActorFactoryWrapper::ground,isle_river_adapter_func_802DA4B8_6C2298,nullptr,func_802DA4B8_6C2298},
    {90u,0x802da8ccu,0x802e2ec4u,0x802da500u,ActorFactoryWrapper::ground,isle_river_adapter_func_802DA8CC_6C26AC,nullptr,func_802DA8CC_6C26AC},
    {603u,0x802da9a4u,0x802e2f70u,0x802da910u,ActorFactoryWrapper::ground,isle_river_adapter_func_802DA9A4_6C2784,nullptr,func_802DA9A4_6C2784},
    {1020u,0x802dad28u,0x802e3074u,0x802dabb8u,ActorFactoryWrapper::ordinary_dl4,isle_river_adapter_func_802DAD28_6C2B08,nullptr,func_802DAD28_6C2B08},
    {91u,0x802db064u,0x802e312cu,0x802dad60u,ActorFactoryWrapper::ground,isle_river_adapter_func_802DB064_6C2E44,nullptr,func_802DB064_6C2E44},
    {54u,0x802dbb54u,0x802e3328u,0x802db0a0u,ActorFactoryWrapper::ground,isle_river_adapter_func_802DBB54_6C3934,nullptr,func_802DBB54_6C3934},
    {60u,0x802dceb0u,0x802e3684u,0x802dbba0u,ActorFactoryWrapper::ground,isle_river_adapter_func_802DCEB0_6C4C90,nullptr,func_802DCEB0_6C4C90},
    {129u,0x802dd0a8u,0x802e377cu,0x802dcef0u,ActorFactoryWrapper::ground,isle_river_adapter_func_802DD0A8_6C4E88,nullptr,func_802DD0A8_6C4E88},
    {137u,0x802de8bcu,0x802e3cbcu,0x802dd0e0u,ActorFactoryWrapper::ordinary,isle_river_adapter_func_802DE8BC_6C669C,nullptr,func_802DE8BC_6C669C},
    {1021u,0x802de9c0u,0x802e3d68u,0x802de900u,ActorFactoryWrapper::ordinary_dl4,func_802DE9C0_6C67A0,func_802DE9C0_6C67A0,nullptr},
    {1022u,0x802dec50u,0x802e3dccu,0x802dea00u,ActorFactoryWrapper::ordinary,isle_river_adapter_func_802DEC50_6C6A30,nullptr,func_802DEC50_6C6A30},
    {11u,0x802def88u,0x802e3ea4u,0x802dec90u,ActorFactoryWrapper::ordinary,isle_river_adapter_func_802DEF88_6C6D68,nullptr,func_802DEF88_6C6D68},
    {45u,0x802df634u,0x802e40b0u,0x802df078u,ActorFactoryWrapper::ground,isle_river_adapter_func_802DF634_6C7414,nullptr,func_802DF634_6C7414},
    {25u,0x802dfeb0u,0x802e4378u,0x802df670u,ActorFactoryWrapper::ordinary,isle_river_adapter_func_802DFEB0_6C7C90,nullptr,func_802DFEB0_6C7C90},
    {1002u,0x802dff5cu,0x802e43d8u,0x802dfef0u,ActorFactoryWrapper::ordinary_dl4,isle_river_adapter_func_802DFF5C_6C7D3C,nullptr,func_802DFF5C_6C7D3C},
    {1001u,0x802e0584u,0x802e44b4u,0x802e0070u,ActorFactoryWrapper::ordinary_dl4,isle_river_adapter_func_802E0584_6C8364,nullptr,func_802E0584_6C8364},
    {1023u,0x802e0694u,0x802e454cu,0x802e05c0u,ActorFactoryWrapper::ordinary_dl4,isle_river_adapter_func_802E0694_6C8474,nullptr,func_802E0694_6C8474},
    {1024u,0x802e07a4u,0x802e45dcu,0x802e06d0u,ActorFactoryWrapper::ordinary_dl4,isle_river_adapter_func_802E07A4_6C8584,nullptr,func_802E07A4_6C8584},
    {1025u,0x802e08c4u,0x802e466cu,0x802e07f0u,ActorFactoryWrapper::ordinary_dl4,isle_river_adapter_func_802E08C4_6C86A4,nullptr,func_802E08C4_6C86A4},
    {1u,0x802e1864u,0x802e4aa8u,0x802e09a8u,ActorFactoryWrapper::ground,isle_river_adapter_func_802E1864_6C9644,nullptr,func_802E1864_6C9644},
    {1019u,0x802e2084u,0x802e4b48u,0x802e18a0u,ActorFactoryWrapper::ordinary_dl4,func_802E2084_6C9E64,func_802E2084_6C9E64,nullptr},
    {1003u,0x8035e0d4u,0x80388348u,0x8035df20u,ActorFactoryWrapper::ground_dl4,func_8035E0D4_4FE4E4,func_8035E0D4_4FE4E4,nullptr},
};
const ActorChildFactoryDependency children[] = {
    {{0x802d8a48u,91u},0x802e28b8u,0x3ffu,{0u,603u,0x802da9a4u,0x80363eb4u,0x80364280u,0x802e2f70u,{0x802d8a48u,91u}}},
    {{0x802d8b34u,122u},0x802e28c8u,0x3ffu,{0u,80u,0x802da4b8u,0x80363eb4u,0x80364280u,0x802e2e04u,{0x802d8b34u,122u}}},
    {{0x802d8c24u,152u},0x802e28d8u,0x3ffu,{0u,129u,0x802dd0a8u,0x80363eb4u,0x80364280u,0x802e377cu,{0x802d8c24u,152u}}},
    {{0x802d8cf0u,181u},0x802e28e8u,0x3ffu,{0u,60u,0x802dceb0u,0x80363eb4u,0x80364280u,0x802e3684u,{0x802d8cf0u,181u}}},
    {{0x802d8dc4u,211u},0x802e28f8u,0x3ffu,{0u,54u,0x802dbb54u,0x80363eb4u,0x80364280u,0x802e3328u,{0x802d8dc4u,211u}}},
    {{0x802da9e0u,27u},0x802e3008u,0x7ffu,{0u,90u,0x802da8ccu,0x80363dbcu,0x80364280u,0x802e2ec4u,{0x802da9e0u,27u}}},
    {{0x802daaccu,59u},0x802e3018u,0x7ffu,{0u,91u,0x802db064u,0x80363dbcu,0x80364280u,0x802e312cu,{0x802daaccu,59u}}},
    {{0x802dff94u,29u},0x802e440cu,0x3ffu,{0u,1002u,0x802dff5cu,0x80363dbcu,0x80364280u,0x802e43d8u,{0x802dff94u,29u}}},
};
constexpr std::uint32_t graphs[] = {0x802e2ac4u, 0x802e2b3cu, 0x802e2b7cu, 0x802e2b9cu, 0x802e2ca8u, 0x802e2d40u, 0x802e2d80u, 0x802e2dc0u, 0x802e2e58u, 0x802e2e88u, 0x802e2f34u, 0x802e3028u, 0x802e30e0u, 0x802e324cu, 0x802e328cu, 0x802e32e8u, 0x802e3494u, 0x802e34d4u, 0x802e351cu, 0x802e3584u, 0x802e35ecu, 0x802e360cu, 0x802e362cu, 0x802e39b8u, 0x802e39e8u, 0x802e3a38u, 0x802e3a88u, 0x802e3ad8u, 0x802e3b28u, 0x802e3b78u, 0x802e3c28u, 0x802e3c58u, 0x802e3d34u, 0x802e3e40u, 0x802e3e70u, 0x802e3f6cu, 0x802e3fccu, 0x802e400cu, 0x802e403cu, 0x802e406cu, 0x802e4234u, 0x802e42a4u, 0x802e42c4u, 0x802e4314u, 0x802e4334u, 0x802e4478u, 0x802e4518u, 0x802e45a8u, 0x802e4638u, 0x802e4838u, 0x802e48b8u, 0x802e4908u, 0x802e4938u, 0x802e49a0u, 0x802e49e0u, 0x802e4a10u, 0x802e4a40u, 0x802e4b14u};
constexpr std::uint32_t weighted[] = {0x802e2b24u, 0x802e2d18u, 0x802e3048u, 0x802e32bcu, 0x802e3474u, 0x802e3504u, 0x802e356cu, 0x802e35d4u, 0x802e3748u, 0x802e3bc8u, 0x802e3be8u, 0x802e3c08u, 0x802e4978u};
constexpr std::uint32_t roots[] = {0x802d9664u, 0x802d9f80u, 0x802da500u, 0x802da910u, 0x802dabb8u, 0x802dad60u, 0x802db0a0u, 0x802dbba0u, 0x802dcef0u, 0x802dd0e0u, 0x802de900u, 0x802dea00u, 0x802dec90u, 0x802df078u, 0x802df670u, 0x802dfef0u, 0x802e0070u, 0x802e05c0u, 0x802e06d0u, 0x802e07f0u, 0x802e09a8u, 0x802e18a0u, 0x8035df20u};
constexpr ActorCountedOptionsBinding counted[] = {{0x802e2908u,3u}};
const ActorAnimationBinding animations[] = {
    {0x802e01c8u,func_802E01C8_6C7FA8},
    {0x802e19a0u,func_802E19A0_6C9780},
    {0x802e1a3cu,func_802E1A3C_6C981C},
};
const ActorLifecycleAnimationBinding entry_animations[] = {
    {0x802e2120u,func_802E2120_6C9F00},
};
const ActorItemCollisionBinding collisions[] = {{0x802d8e98u,func_802D8E98_6C0C78}};
const ActorReturningUtilityBinding returning_utilities[] = {{0x802d9520u,func_802D9520_6C1300}};
const ActorSequentialMaterialFamily sequential_materials[] = {
    {0x802e454cu,0x802e44f0u,0x802e44f0u,0x802e4504u,0x802e05c0u,0x802e0618u,0.7f,0.0f},
    {0x802e45dcu,0x802e4580u,0x802e4580u,0x802e4594u,0x802e06d0u,0x802e0728u,0.7f,0.0f},
    {0x802e466cu,0x802e4610u,0x802e4610u,0x802e4624u,0x802e07f0u,0x802e0848u,0.7f,0.0f},
};
constexpr ActorNativeService required_services[] = {ActorNativeService::reference_assign, ActorNativeService::model, ActorNativeService::model_read, ActorNativeService::model_store, ActorNativeService::root_position, ActorNativeService::root_store, ActorNativeService::node_descend, ActorNativeService::node_world, ActorNativeService::current_block, ActorNativeService::block_position, ActorNativeService::path_count, ActorNativeService::path_duration, ActorNativeService::path_parameter, ActorNativeService::path_point, ActorNativeService::item, ActorNativeService::player_object, ActorNativeService::player_model, ActorNativeService::rng_range, ActorNativeService::weighted, ActorNativeService::counted_options, ActorNativeService::graph, ActorNativeService::add_one, ActorNativeService::ground, ActorNativeService::sound, ActorNativeService::sound_params, ActorNativeService::sound_active, ActorNativeService::sound_stop, ActorNativeService::bgm_play, ActorNativeService::bgm_volume, ActorNativeService::bgm_volume_smooth, ActorNativeService::sound_volume_smooth, ActorNativeService::sound_global_volume, ActorNativeService::animation_speed, ActorNativeService::model_speed, ActorNativeService::camera, ActorNativeService::camera_read, ActorNativeService::camera_store, ActorNativeService::camera_speed, ActorNativeService::camera_animation, ActorNativeService::camera_process, ActorNativeService::player_model_init, ActorNativeService::player_animation, ActorNativeService::player_change_animation, ActorNativeService::player_animation_speed, ActorNativeService::animation_callback, ActorNativeService::pause_processes, ActorNativeService::resume_processes, ActorNativeService::sky_speed, ActorNativeService::notice, ActorNativeService::progress_read, ActorNativeService::progress_write, ActorNativeService::transition_prepare, ActorNativeService::transition_color, ActorNativeService::transition_fade, ActorNativeService::transition_active, ActorNativeService::create_helper, ActorNativeService::delete_object, ActorNativeService::send_command, ActorNativeService::send_command_link, ActorNativeService::distance, ActorNativeService::sound_stop_all, ActorNativeService::bgm_stop_all, ActorNativeService::remove_flying_items, ActorNativeService::reset_messages, ActorNativeService::audio_cutscene, ActorNativeService::source_process, ActorNativeService::environment_sound, ActorNativeService::definition, ActorNativeService::root_read};
}
std::span<const actor::BehaviorCallbackBinding> actor_callbacks() noexcept { return callbacks; }
std::span<const actor::BehaviorCallbackBinding> lifecycle_callbacks() noexcept { return entry_callbacks; }
std::span<const std::uint32_t> actor_graphs() noexcept { return graphs; }
std::span<const std::uint32_t> actor_weighted_states() noexcept { return weighted; }
std::span<const ActorCountedOptionsBinding> actor_counted_options() noexcept { return counted; }
std::span<const std::uint32_t> actor_roots() noexcept { return roots; }
std::span<const ActorFactoryBinding> actor_factories() noexcept { return factories; }
std::span<const ActorChildFactoryDependency> actor_child_factories() noexcept { return children; }
std::span<const ActorProviderDependency> actor_dependencies() noexcept { return dependencies; }
std::span<const ActorAnimationBinding> actor_animation_callbacks() noexcept { return animations; }
std::span<const ActorLifecycleAnimationBinding> lifecycle_animation_callbacks() noexcept { return entry_animations; }
std::span<const ActorItemCollisionBinding> actor_item_collisions() noexcept { return collisions; }
std::span<const ActorReturningUtilityBinding> actor_returning_utilities() noexcept { return returning_utilities; }
std::span<const ActorSequentialMaterialFamily> actor_sequential_material_families() noexcept { return sequential_materials; }
std::span<const ActorNativeService> actor_required_services() noexcept { return required_services; }
} // namespace isle::river

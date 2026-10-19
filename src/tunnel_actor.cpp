#include "tunnel_actor.hpp"

namespace isle::tunnel {
namespace {
constexpr std::uint32_t d_0_callbacks[] = {0x802e5d94u, 0x802e5f98u, 0x802e61dcu};
constexpr const char* d_0_native[] = {"HIDE_POKEMON", "Pokemon_SetState"};
constexpr const char* d_1_native[] = {"Pokemon_StopAuxProc"};
constexpr std::uint32_t d_2_callbacks[] = {0x802e5d94u};
constexpr std::uint32_t d_2_graphs[] = {0x802ee9f8u, 0x802eea18u};
constexpr std::uint32_t d_2_headers[] = {0x802ee994u, 0x802ee9a8u, 0x802ee9bcu, 0x802ee9d0u, 0x802ee9e4u};
constexpr const char* d_2_native[] = {"HIDE_POKEMON", "Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag", "SHOW_POKEMON", "cmdSendCommandToLink"};
constexpr std::uint32_t d_3_graphs[] = {0x802eea38u};
constexpr const char* d_3_native[] = {"Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_4_callbacks[] = {0x802e5f98u};
constexpr std::uint32_t d_4_graphs[] = {0x802ee9f8u};
constexpr std::uint32_t d_4_headers[] = {0x802ee994u, 0x802ee9a8u, 0x802ee9bcu, 0x802ee9d0u, 0x802ee9e4u};
constexpr const char* d_4_native[] = {"HIDE_POKEMON", "Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag", "SHOW_POKEMON", "cmdSendCommandToLink"};
constexpr std::uint32_t d_5_graphs[] = {0x802eea58u};
constexpr const char* d_5_native[] = {"Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_6_callbacks[] = {0x802e61dcu};
constexpr std::uint32_t d_6_graphs[] = {0x802ee9f8u};
constexpr std::uint32_t d_6_headers[] = {0x802ee994u, 0x802ee9a8u, 0x802ee9bcu, 0x802ee9d0u, 0x802ee9e4u};
constexpr const char* d_6_native[] = {"HIDE_POKEMON", "Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag", "SHOW_POKEMON", "cmdSendCommandToLink"};
constexpr const char* d_7_native[] = {"Pokemon_SpawnOnGround"};
constexpr const char* d_8_native[] = {"pokemonRemoveOne"};
constexpr std::uint32_t d_9_callbacks[] = {0x802e6574u, 0x802e67b8u, 0x802e6a30u};
constexpr const char* d_9_native[] = {"HIDE_POKEMON", "Pokemon_SetState"};
constexpr const char* d_10_native[] = {"Pokemon_StopAuxProc"};
constexpr std::uint32_t d_11_graphs[] = {0x802eeb48u};
constexpr const char* d_11_native[] = {"Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_12_callbacks[] = {0x802e6574u};
constexpr std::uint32_t d_12_graphs[] = {0x802eeb28u};
constexpr std::uint32_t d_12_headers[] = {0x802eeac4u, 0x802eead8u, 0x802eeaecu, 0x802eeb00u, 0x802eeb14u};
constexpr const char* d_12_native[] = {"HIDE_POKEMON", "Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag", "SHOW_POKEMON", "cmdSendCommandToLink"};
constexpr std::uint32_t d_13_graphs[] = {0x802eeb68u};
constexpr const char* d_13_native[] = {"Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_14_callbacks[] = {0x802e67b8u};
constexpr std::uint32_t d_14_graphs[] = {0x802eeb28u};
constexpr std::uint32_t d_14_headers[] = {0x802eeac4u, 0x802eead8u, 0x802eeaecu, 0x802eeb00u, 0x802eeb14u};
constexpr const char* d_14_native[] = {"HIDE_POKEMON", "Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag", "SHOW_POKEMON", "cmdSendCommandToLink"};
constexpr std::uint32_t d_15_graphs[] = {0x802eeb88u};
constexpr const char* d_15_native[] = {"Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_16_graphs[] = {0x802eeb28u};
constexpr std::uint32_t d_16_headers[] = {0x802eeac4u, 0x802eead8u, 0x802eeaecu, 0x802eeb00u, 0x802eeb14u};
constexpr const char* d_16_native[] = {"HIDE_POKEMON", "Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag", "SHOW_POKEMON", "cmdSendCommandToLink"};
constexpr const char* d_17_native[] = {"GET_POKEMON", "Pokemon_SpawnOnGround"};
constexpr const char* d_18_native[] = {"pokemonRemoveOne"};
constexpr std::uint32_t d_19_callbacks[] = {0x802e9c18u};
constexpr std::uint32_t d_19_graphs[] = {0x802ef464u};
constexpr const char* d_19_native[] = {"HIDE_POKEMON", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_20_callbacks[] = {0x802e9538u, 0x802e9becu, 0x802e9f80u};
constexpr const char* d_20_native[] = {"Pokemon_SetState", "SHOW_POKEMON"};
constexpr std::uint32_t d_21_graphs[] = {0x802ef3ecu, 0x802ef48cu};
constexpr std::uint32_t d_21_weighted_states[] = {0x802ef44cu};
constexpr std::uint32_t d_21_headers[] = {0x802ef338u};
constexpr const char* d_21_native[] = {"Pokemon_SetAnimation", "Pokemon_SetStateRandom", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_22_callbacks[] = {0x802e9538u, 0x802e968cu};
constexpr std::uint32_t d_22_graphs[] = {0x802ef3ecu};
constexpr std::uint32_t d_22_weighted_states[] = {0x802ef44cu};
constexpr std::uint32_t d_22_headers[] = {0x802ef34cu};
constexpr const char* d_22_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_SetStateRandom", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr const char* d_23_native[] = {"Pokemon_RunInCircles", "omEndProcess"};
constexpr std::uint32_t d_24_callbacks[] = {0x802e9538u, 0x802e9a48u};
constexpr std::uint32_t d_24_graphs[] = {0x802ef4ecu};
constexpr std::uint32_t d_24_headers[] = {0x802ef34cu, 0x802ef39cu, 0x802ef3b0u, 0x802ef3c4u, 0x802ef3d8u};
constexpr const char* d_24_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_25_callbacks[] = {0x802e9538u};
constexpr std::uint32_t d_25_headers[] = {0x802ef374u};
constexpr const char* d_25_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_26_callbacks[] = {0x802e9538u, 0x802e9a48u};
constexpr std::uint32_t d_26_graphs[] = {0x802ef51cu};
constexpr std::uint32_t d_26_headers[] = {0x802ef34cu, 0x802ef39cu};
constexpr const char* d_26_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_27_callbacks[] = {0x802e9538u, 0x802e9a48u, 0x802e9a94u};
constexpr std::uint32_t d_27_graphs[] = {0x802ef54cu, 0x802ef59cu};
constexpr std::uint32_t d_27_headers[] = {0x802ef34cu, 0x802ef360u, 0x802ef388u};
constexpr const char* d_27_native[] = {"Pokemon_EatApple", "Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr const char* d_28_native[] = {"Pokemon_TurnToTarget", "omEndProcess"};
constexpr const char* d_29_native[] = {"Pokemon_RunToTarget", "omEndProcess"};
constexpr std::uint32_t d_31_callbacks[] = {0x802e9538u, 0x802e9af8u, 0x802e9b90u};
constexpr std::uint32_t d_31_graphs[] = {0x802ef5ccu};
constexpr std::uint32_t d_31_headers[] = {0x802ef360u};
constexpr const char* d_31_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr const char* d_32_native[] = {"Pokemon_RunAwayFromTarget", "omEndProcess"};
constexpr const char* d_33_native[] = {"Pokemon_RunCleanup", "Pokemon_SetState"};
constexpr std::uint32_t d_34_callbacks[] = {0x802e9c3cu};
constexpr const char* d_34_native[] = {"Pokemon_SetState"};
constexpr std::uint32_t d_35_callbacks[] = {0x802e9c60u};
constexpr const char* d_35_native[] = {"Pokemon_SetState"};
constexpr std::uint32_t d_36_callbacks[] = {0x802e9c98u, 0x802e9d04u};
constexpr const char* d_36_native[] = {"Pokemon_SetState", "Pokemon_StartPathProc"};
constexpr const char* d_37_native[] = {"Pokemon_FollowPath", "Pokemon_ResetPathPos", "omEndProcess"};
constexpr std::uint32_t d_38_callbacks[] = {0x802e9d70u};
constexpr std::uint32_t d_38_graphs[] = {0x802ef61cu};
constexpr std::uint32_t d_38_headers[] = {0x802ef360u};
constexpr const char* d_38_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_39_callbacks[] = {0x802e9dd8u};
constexpr std::uint32_t d_39_graphs[] = {0x802ef64cu};
constexpr std::uint32_t d_39_headers[] = {0x802ef338u};
constexpr const char* d_39_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_40_headers[] = {0x802ef3b0u, 0x802ef3c4u};
constexpr const char* d_40_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_41_callbacks[] = {0x802e9d04u};
constexpr std::uint32_t d_41_graphs[] = {0x802ef68cu};
constexpr std::uint32_t d_41_headers[] = {0x802ef3b0u, 0x802ef3c4u, 0x802ef3d8u};
constexpr const char* d_41_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_42_callbacks[] = {0x802e9d04u};
constexpr std::uint32_t d_42_headers[] = {0x802ef374u};
constexpr const char* d_42_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_43_callbacks[] = {0x802ea030u, 0x802ea098u};
constexpr std::uint32_t d_43_headers[] = {0x802ef338u};
constexpr const char* d_43_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_44_callbacks[] = {0x802ea030u};
constexpr std::uint32_t d_44_graphs[] = {0x802ef6b0u};
constexpr std::uint32_t d_44_headers[] = {0x802ef338u};
constexpr const char* d_44_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr const char* d_45_native[] = {"Pokemon_FollowPath", "Pokemon_ResetPathPos", "omEndProcess"};
constexpr std::uint32_t d_46_callbacks[] = {0x802ea21cu};
constexpr std::uint32_t d_46_graphs[] = {0x802ef6b0u};
constexpr std::uint32_t d_46_headers[] = {0x802ef3b0u, 0x802ef3c4u, 0x802ef3d8u};
constexpr const char* d_46_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_47_callbacks[] = {0x802ea21cu};
constexpr std::uint32_t d_47_graphs[] = {0x802ef6b0u};
constexpr std::uint32_t d_47_headers[] = {0x802ef374u};
constexpr const char* d_47_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_48_callbacks[] = {0x802ea344u, 0x802ea3a0u};
constexpr std::uint32_t d_48_graphs[] = {0x802ef6b0u};
constexpr std::uint32_t d_48_headers[] = {0x802ef360u, 0x802ef39cu};
constexpr const char* d_48_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag", "cmdSendCommand"};
constexpr std::uint32_t d_49_callbacks[] = {0x802ea344u};
constexpr std::uint32_t d_49_graphs[] = {0x802ef6b0u};
constexpr std::uint32_t d_49_headers[] = {0x802ef338u};
constexpr const char* d_49_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr const char* d_50_native[] = {"Pokemon_TurnToTarget", "omEndProcess"};
constexpr const char* d_51_native[] = {"EnvSound_PlaySound", "Pokemon_StopAuxProc"};
constexpr const char* d_52_native[] = {"Pokemon_SpawnOnGround"};
constexpr const char* d_53_native[] = {"pokemonRemoveOne"};
constexpr const char* d_54_native[] = {"Pokemon_StopAuxProc", "ohWait"};
constexpr const char* d_55_native[] = {"Pokemon_RunCleanup", "Pokemon_SetState"};
constexpr std::uint32_t d_56_callbacks[] = {0x802e5244u, 0x802e53b8u, 0x802e54b4u, 0x802e55d8u, 0x802e5644u, 0x802e5818u};
constexpr const char* d_56_native[] = {"Pokemon_SetState"};
constexpr std::uint32_t d_57_graphs[] = {0x802ee83cu};
constexpr std::uint32_t d_57_headers[] = {0x802ee7ecu};
constexpr const char* d_57_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_58_callbacks[] = {0x802e50f0u};
constexpr std::uint32_t d_58_graphs[] = {0x802ee88cu};
constexpr std::uint32_t d_58_headers[] = {0x802ee800u, 0x802ee814u};
constexpr const char* d_58_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartAuxProc", "Pokemon_StartPathProc", "Pokemon_WaitForFlag", "cmdSendCommand", "cmdSendCommandToLink"};
constexpr std::uint32_t d_59_callbacks[] = {0x802e53dcu};
constexpr const char* d_59_native[] = {"Pokemon_SetState"};
constexpr std::uint32_t d_60_callbacks[] = {0x802e529cu, 0x802e5448u};
constexpr std::uint32_t d_60_headers[] = {0x802ee828u};
constexpr const char* d_60_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr const char* d_61_native[] = {"Pokemon_FollowPath", "Pokemon_ResetPathPos", "omEndProcess"};
constexpr std::uint32_t d_62_callbacks[] = {0x802e54f0u};
constexpr const char* d_62_native[] = {"Pokemon_SetState"};
constexpr std::uint32_t d_63_callbacks[] = {0x802e529cu, 0x802e556cu};
constexpr std::uint32_t d_63_headers[] = {0x802ee828u};
constexpr const char* d_63_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag", "cmdSendCommandToLink"};
constexpr const char* d_64_native[] = {"Pokemon_FollowPath", "Pokemon_ResetPathPos", "omEndProcess"};
constexpr std::uint32_t d_65_graphs[] = {0x802ee83cu};
constexpr std::uint32_t d_65_headers[] = {0x802ee7ecu};
constexpr const char* d_65_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_66_callbacks[] = {0x802e5668u};
constexpr const char* d_66_native[] = {"Pokemon_SetState"};
constexpr std::uint32_t d_67_callbacks[] = {0x802e56d8u};
constexpr std::uint32_t d_67_graphs[] = {0x802ee8b0u};
constexpr std::uint32_t d_67_headers[] = {0x802ee828u};
constexpr const char* d_67_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr const char* d_68_native[] = {"Pokemon_FollowPath", "Pokemon_ResetPathPos", "omEndProcess"};
constexpr std::uint32_t d_69_callbacks[] = {0x802e50f0u};
constexpr std::uint32_t d_69_graphs[] = {0x802ee88cu};
constexpr std::uint32_t d_69_headers[] = {0x802ee800u, 0x802ee814u};
constexpr const char* d_69_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartAuxProc", "Pokemon_StartPathProc", "Pokemon_WaitForFlag", "cmdSendCommandToLink"};
constexpr std::uint32_t d_70_callbacks[] = {0x802e583cu};
constexpr const char* d_70_native[] = {"Pokemon_SetState"};
constexpr std::uint32_t d_71_callbacks[] = {0x802e5244u, 0x802e58b0u};
constexpr std::uint32_t d_71_graphs[] = {0x802ee900u};
constexpr std::uint32_t d_71_headers[] = {0x802ee7ecu};
constexpr const char* d_71_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr const char* d_72_native[] = {"getLevelProgress", "ohWait", "omEndProcess"};
constexpr std::uint32_t d_73_headers[] = {0x802ee800u, 0x802ee814u};
constexpr const char* d_73_native[] = {"Pokemon_ForceAnimationAtTime", "Pokemon_RemovePokemons", "Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag", "SET_ANIM_FRAME", "auPlaySoundWithParams", "cmdSendCommand", "cmdSendCommandToLink", "getCurrentWorldBlock", "ohWait"};
constexpr const char* d_74_native[] = {"Pokemon_SpawnOnGround"};
constexpr std::uint32_t d_75_callbacks[] = {0x802ecae4u};
constexpr const char* d_75_native[] = {"Pokemon_SetState"};
constexpr std::uint32_t d_76_graphs[] = {0x802f0168u};
constexpr std::uint32_t d_76_headers[] = {0x802f0110u};
constexpr const char* d_76_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_77_callbacks[] = {0x802ece10u};
constexpr std::uint32_t d_77_headers[] = {0x802f0124u, 0x802f0138u, 0x802f014cu};
constexpr const char* d_77_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag", "auPlaySound", "cmdSendCommand"};
constexpr const char* d_79_native[] = {"Pokemon_StopAuxProc", "auSetBGMVolume", "ohWait"};
constexpr const char* d_80_native[] = {"Pokemon_StopAuxProc", "auPlaySound", "auStopSound", "ohWait"};
constexpr std::uint32_t d_81_callbacks[] = {0x802ecc44u, 0x802ecd84u};
constexpr std::uint32_t d_81_exact_abi_dependencies[] = {0x802ecc18u};
constexpr const char* d_81_native[] = {"Camera_StartCutScene", "Items_RemoveFlyingItems", "Msg_Reset", "PlayerModel_Init", "PlayerModel_SetAnimation", "Pokemon_RemovePokemons", "Pokemon_SetState", "Pokemon_StartAuxProc", "animSetCameraAnimation", "auStopAllSounds", "cmdSendCommand", "getMainCamera", "ohPauseObjectProcesses", "ohWait", "omCreateProcess", "omEndProcess", "resetMainCameraSettings", "setSkyBoxAnimationSpeed"};
constexpr const char* d_82_native[] = {"Pokemon_SpawnDlLink4"};
constexpr std::uint32_t d_83_callbacks[] = {0x802ebc04u};
constexpr const char* d_83_native[] = {"Pokemon_SetState"};
constexpr std::uint32_t d_84_callbacks[] = {0x802ebc78u};
constexpr std::uint32_t d_84_headers[] = {0x802efd58u};
constexpr const char* d_84_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr const char* d_85_native[] = {"Pokemon_FollowPath", "Pokemon_ResetPathPos", "omEndProcess"};
constexpr const char* d_86_native[] = {"Pokemon_SpawnOnGround"};
constexpr std::uint32_t d_87_callbacks[] = {0x802e70bcu, 0x802e70fcu};
constexpr std::uint32_t d_87_graphs[] = {0x802eec4cu};
constexpr const char* d_87_native[] = {"Pokemon_SetState", "Pokemon_WaitForFlag", "getGroundAt"};
constexpr std::uint32_t d_88_callbacks[] = {0x802e6e0cu};
constexpr std::uint32_t d_88_graphs[] = {0x802eec1cu};
constexpr std::uint32_t d_88_headers[] = {0x802eebf4u};
constexpr const char* d_88_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag", "randRange"};
constexpr std::uint32_t d_89_callbacks[] = {0x802e6f48u};
constexpr std::uint32_t d_89_graphs[] = {0x802eec1cu};
constexpr std::uint32_t d_89_headers[] = {0x802eec08u};
constexpr const char* d_89_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_90_callbacks[] = {0x802e6d90u, 0x802e6ee8u};
constexpr std::uint32_t d_90_headers[] = {0x802eec08u};
constexpr const char* d_90_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr const char* d_91_native[] = {"Pokemon_Jump", "omEndProcess"};
constexpr const char* d_92_native[] = {"Pokemon_FallToHeight", "omEndProcess"};
constexpr std::uint32_t d_93_callbacks[] = {0x802e6d90u, 0x802e7014u};
constexpr std::uint32_t d_93_headers[] = {0x802eec08u};
constexpr const char* d_93_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr const char* d_94_native[] = {"Pokemon_FallToHeight", "Pokemon_Jump", "omEndProcess"};
constexpr std::uint32_t d_95_graphs[] = {0x802eec6cu};
constexpr const char* d_95_native[] = {"Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_96_callbacks[] = {0x802e71d4u};
constexpr const char* d_96_native[] = {"HIDE_POKEMON", "Pokemon_SetState", "SHOW_POKEMON", "getLevelProgress", "ohWait"};
constexpr std::uint32_t d_97_callbacks[] = {0x802e71d4u, 0x802e729cu};
constexpr std::uint32_t d_97_graphs[] = {0x802eececu};
constexpr std::uint32_t d_97_headers[] = {0x802eebf4u};
constexpr const char* d_97_native[] = {"HIDE_POKEMON", "Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag", "SHOW_POKEMON", "randRange"};
constexpr const char* d_98_native[] = {"Pokemon_Fall", "Pokemon_Jump", "ohWait", "omEndProcess"};
constexpr std::uint32_t d_99_callbacks[] = {0x802e71d4u, 0x802e73d0u, 0x802e74a4u};
constexpr std::uint32_t d_99_graphs[] = {0x802eec8cu, 0x802eecccu};
constexpr std::uint32_t d_99_headers[] = {0x802eec08u};
constexpr const char* d_99_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag", "SHOW_POKEMON"};
constexpr const char* d_100_native[] = {"Pokemon_FallToHeight", "Pokemon_Jump", "Pokemon_WaitForFlag", "omEndProcess"};
constexpr const char* d_101_native[] = {"Pokemon_JumpToHeight", "omEndProcess"};
constexpr std::uint32_t d_102_callbacks[] = {0x802e71d4u, 0x802e74a4u, 0x802e7594u};
constexpr std::uint32_t d_102_graphs[] = {0x802eec8cu, 0x802eecccu};
constexpr const char* d_102_native[] = {"Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr const char* d_103_native[] = {"Pokemon_Jump", "omEndProcess"};
constexpr const char* d_104_native[] = {"Pokemon_RunCleanup", "Pokemon_SetState"};
constexpr const char* d_105_native[] = {"Pokemon_Spawn"};
constexpr std::uint32_t d_106_callbacks[] = {0x802ebd6cu, 0x802ebe0cu};
constexpr const char* d_106_native[] = {"Pokemon_SetState"};
constexpr std::uint32_t d_107_graphs[] = {0x802efe48u};
constexpr const char* d_107_native[] = {"HIDE_POKEMON", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_108_callbacks[] = {0x802ebe0cu};
constexpr const char* d_108_native[] = {"EnvSound_PlaySound", "Pokemon_SetState", "SHOW_POKEMON", "func_8035E1D4_4FE5E4"};
constexpr std::uint32_t d_109_callbacks[] = {0x802ebec4u};
constexpr std::uint32_t d_109_headers[] = {0x802efe20u};
constexpr const char* d_109_native[] = {"Pokemon_ForceAnimation", "Pokemon_RunCleanup", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag", "randRange"};
constexpr const char* d_110_native[] = {"func_8035E298_4FE6A8", "omEndProcess"};
constexpr const char* d_111_native[] = {"Pokemon_SpawnOnGround"};
constexpr const char* d_112_native[] = {"cmdSendCommandToLink", "ohWait"};
constexpr std::uint32_t d_113_callbacks[] = {0x802e39a0u, 0x802e3ac8u, 0x802e4234u};
constexpr const char* d_113_native[] = {"Pokemon_SetState", "Pokemon_StartAuxProc"};
constexpr std::uint32_t d_114_graphs[] = {0x802ee2a4u};
constexpr std::uint32_t d_114_weighted_states[] = {0x802ee344u};
constexpr std::uint32_t d_114_headers[] = {0x802ee220u};
constexpr const char* d_114_native[] = {"Pokemon_SetAnimation", "Pokemon_SetStateRandom", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_115_callbacks[] = {0x802e3ee4u};
constexpr std::uint32_t d_115_graphs[] = {0x802ee354u};
constexpr std::uint32_t d_115_headers[] = {0x802ee25cu};
constexpr const char* d_115_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_116_callbacks[] = {0x802e3ac8u};
constexpr std::uint32_t d_116_graphs[] = {0x802ee394u};
constexpr std::uint32_t d_116_headers[] = {0x802ee25cu};
constexpr const char* d_116_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_117_callbacks[] = {0x802e3b8cu};
constexpr const char* d_117_native[] = {"Pokemon_SetState"};
constexpr std::uint32_t d_118_callbacks[] = {0x802e3ac8u, 0x802e3d80u, 0x802e3e44u};
constexpr std::uint32_t d_118_graphs[] = {0x802ee3d4u};
constexpr std::uint32_t d_118_headers[] = {0x802ee20cu};
constexpr const char* d_118_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_119_headers[] = {0x802ee248u};
constexpr const char* d_119_native[] = {"Pokemon_RunToTarget", "Pokemon_SetAnimation", "Pokemon_TurnToTarget", "omEndProcess"};
constexpr std::uint32_t d_120_callbacks[] = {0x802e3cc8u};
constexpr const char* d_120_native[] = {"Pokemon_SetState"};
constexpr std::uint32_t d_121_callbacks[] = {0x802e3e44u};
constexpr std::uint32_t d_121_graphs[] = {0x802ee454u};
constexpr std::uint32_t d_121_weighted_states[] = {0x802ee344u};
constexpr std::uint32_t d_121_headers[] = {0x802ee270u};
constexpr const char* d_121_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_SetStateRandom", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_122_callbacks[] = {0x802e3ac8u, 0x802e3f8cu};
constexpr std::uint32_t d_122_graphs[] = {0x802ee2a4u};
constexpr std::uint32_t d_122_headers[] = {0x802ee234u};
constexpr const char* d_122_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr const char* d_123_native[] = {"Pokemon_TurnToTarget", "omEndProcess"};
constexpr std::uint32_t d_124_callbacks[] = {0x802e3ac8u, 0x802e40d4u};
constexpr std::uint32_t d_124_graphs[] = {0x802ee4c4u};
constexpr std::uint32_t d_124_headers[] = {0x802ee234u};
constexpr const char* d_124_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr const char* d_125_native[] = {"Pokemon_RunAwayDefault", "omEndProcess"};
constexpr std::uint32_t d_126_callbacks[] = {0x802e3ac8u, 0x802e3f8cu};
constexpr std::uint32_t d_126_graphs[] = {0x802ee4f4u, 0x802ee524u, 0x802ee564u};
constexpr std::uint32_t d_126_headers[] = {0x802ee234u};
constexpr const char* d_126_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr const char* d_127_native[] = {"ARRAY_COUNT", "GET_POKEMON", "Pokemon_GetDistance", "Pokemon_StopAuxProc", "cmdSendCommand", "ohWait"};
constexpr const char* d_128_native[] = {"fx_getPosVelDObj"};
constexpr std::uint32_t d_129_callbacks[] = {0x802e44dcu, 0x802e45b4u};
constexpr std::uint32_t d_129_headers[] = {0x802ee234u};
constexpr const char* d_129_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag", "cmdSendCommandToLink"};
constexpr std::uint32_t d_130_exact_abi_dependencies[] = {0x802e43b0u};
constexpr const char* d_130_native[] = {"Pokemon_SetTargetPos", "Pokemon_StepToTargetPos", "ohWait", "omEndProcess"};
constexpr std::uint32_t d_131_callbacks[] = {0x802e4668u, 0x802e4710u};
constexpr std::uint32_t d_131_graphs[] = {0x802ee5ccu};
constexpr const char* d_131_native[] = {"Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_132_callbacks[] = {0x802e4668u};
constexpr std::uint32_t d_132_graphs[] = {0x802ee5ccu};
constexpr const char* d_132_native[] = {"GET_POKEMON", "Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_133_exact_abi_dependencies[] = {0x802e43b0u};
constexpr const char* d_133_native[] = {"GET_TRANSFORM", "Pokemon_Turn", "ohWait"};
constexpr const char* d_134_native[] = {"Pokemon_StopAuxProc", "cmdSendCommand"};
constexpr const char* d_135_native[] = {"Pokemon_StopAuxProc", "cmdSendCommand"};
constexpr std::uint32_t d_136_callbacks[] = {0x802e4964u};
constexpr std::uint32_t d_136_headers[] = {0x802ee234u};
constexpr const char* d_136_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_137_callbacks[] = {0x802e4a84u, 0x802e5094u};
constexpr const char* d_137_native[] = {"Pokemon_SetState", "cmdSendCommandToLink"};
constexpr std::uint32_t d_138_callbacks[] = {0x802e49a4u, 0x802e4a84u};
constexpr std::uint32_t d_138_headers[] = {0x802ee234u};
constexpr const char* d_138_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr const char* d_139_native[] = {"Pokemon_RunCleanup", "Pokemon_SetState"};
constexpr const char* d_140_native[] = {"Pokemon_SpawnOnGround"};
constexpr std::uint32_t d_141_callbacks[] = {0x802e4b64u};
constexpr const char* d_141_native[] = {"Pokemon_SetState"};
constexpr std::uint32_t d_142_graphs[] = {0x802ee6b8u};
constexpr std::uint32_t d_142_weighted_states[] = {0x802ee6f8u};
constexpr std::uint32_t d_142_headers[] = {0x802ee6a4u};
constexpr const char* d_142_native[] = {"Pokemon_SetAnimation", "Pokemon_SetStateRandom", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_143_callbacks[] = {0x802e4b64u, 0x802e4c6cu};
constexpr std::uint32_t d_143_graphs[] = {0x802ee6b8u};
constexpr std::uint32_t d_143_weighted_states[] = {0x802ee6f8u};
constexpr std::uint32_t d_143_headers[] = {0x802ee6a4u};
constexpr const char* d_143_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_SetStateRandom", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr const char* d_144_native[] = {"Pokemon_RunInCirclesDefault", "omEndProcess"};
constexpr std::uint32_t d_145_callbacks[] = {0x802e4b64u, 0x802e4d54u};
constexpr std::uint32_t d_145_graphs[] = {0x802ee6b8u};
constexpr std::uint32_t d_145_weighted_states[] = {0x802ee6f8u};
constexpr std::uint32_t d_145_headers[] = {0x802ee6a4u};
constexpr const char* d_145_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_SetStateRandom", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr const char* d_146_native[] = {"Pokemon_RunInCirclesDefault", "omEndProcess"};
constexpr std::uint32_t d_147_callbacks[] = {0x802e4b64u, 0x802e4e2cu, 0x802e4ec8u};
constexpr std::uint32_t d_147_graphs[] = {0x802ee718u};
constexpr const char* d_147_native[] = {"Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr const char* d_148_native[] = {"Pokemon_RunToTarget", "Pokemon_TurnToTarget", "omEndProcess"};
constexpr std::uint32_t d_149_callbacks[] = {0x802e4da4u};
constexpr const char* d_149_native[] = {"Pokemon_SetState"};
constexpr std::uint32_t d_150_callbacks[] = {0x802e4ec8u};
constexpr std::uint32_t d_150_graphs[] = {0x802ee748u};
constexpr std::uint32_t d_150_weighted_states[] = {0x802ee6f8u};
constexpr std::uint32_t d_150_headers[] = {0x802ee6a4u};
constexpr const char* d_150_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_SetStateRandom", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_151_callbacks[] = {0x802e4b64u, 0x802e5008u};
constexpr std::uint32_t d_151_graphs[] = {0x802ee778u};
constexpr std::uint32_t d_151_headers[] = {0x802ee6a4u};
constexpr const char* d_151_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr const char* d_152_native[] = {"Pokemon_RunAwayDefault", "omEndProcess"};
constexpr const char* d_153_native[] = {"Pokemon_SpawnOnGround"};
constexpr std::uint32_t d_154_exact_abi_dependencies[] = {0x802e505cu};
constexpr const char* d_154_native[] = {"GET_POKEMON", "Pokemon_AddAtGeo"};
constexpr const char* d_155_native[] = {"GET_POKEMON", "GET_TRANSFORM", "getCurrentWorldBlock", "pokemonAddOne"};
constexpr const char* d_156_native[] = {"ARRAY_COUNT", "GET_ITEM", "GET_TRANSFORM", "Vec3fDistance", "func", "randRange"};
constexpr std::uint32_t d_157_callbacks[] = {0x802e7718u, 0x802e8008u, 0x802e853cu};
constexpr const char* d_157_native[] = {"Pokemon_SetState"};
constexpr std::uint32_t d_158_callbacks[] = {0x802e77b0u};
constexpr const char* d_158_native[] = {"HIDE_POKEMON", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_159_graphs[] = {0x802eeed8u};
constexpr std::uint32_t d_159_weighted_states[] = {0x802eef78u};
constexpr std::uint32_t d_159_headers[] = {0x802eedbcu};
constexpr const char* d_159_native[] = {"Pokemon_SetAnimation", "Pokemon_SetStateRandom", "Pokemon_StartPathProc", "Pokemon_WaitForFlag", "SHOW_POKEMON"};
constexpr std::uint32_t d_160_graphs[] = {0x802eeed8u};
constexpr std::uint32_t d_160_weighted_states[] = {0x802eef78u};
constexpr std::uint32_t d_160_headers[] = {0x802eedd0u};
constexpr const char* d_160_native[] = {"Pokemon_ForceAnimation", "Pokemon_SetStateRandom", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_161_callbacks[] = {0x802e782cu, 0x802e789cu};
constexpr std::uint32_t d_161_headers[] = {0x802eee20u};
constexpr const char* d_161_native[] = {"Pokemon_ForceAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_162_callbacks[] = {0x802e796cu};
constexpr const char* d_162_native[] = {"Pokemon_SetState"};
constexpr std::uint32_t d_163_callbacks[] = {0x802e77b0u, 0x802e782cu, 0x802e789cu, 0x802e796cu, 0x802e7ac8u, 0x802e7b3cu};
constexpr std::uint32_t d_163_headers[] = {0x802eee0cu, 0x802eee70u};
constexpr const char* d_163_native[] = {"Pokemon_EatApple", "Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr const char* d_164_native[] = {"Pokemon_RunToTarget", "Pokemon_TurnToTarget", "omEndProcess"};
constexpr std::uint32_t d_165_callbacks[] = {0x802e7c1cu, 0x802e7ca4u, 0x802e7d04u};
constexpr const char* d_165_native[] = {"Pokemon_GetDistance", "Pokemon_SetState", "Pokemon_StartAuxProc", "Pokemon_StartPathProc"};
constexpr std::uint32_t d_166_callbacks[] = {0x802e782cu, 0x802e7c1cu};
constexpr std::uint32_t d_166_headers[] = {0x802eede4u};
constexpr const char* d_166_native[] = {"Pokemon_ForceAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr const char* d_167_native[] = {"Items_GetPokeFluteCmd", "Pokemon_StopAuxProc", "ohWait"};
constexpr std::uint32_t d_168_callbacks[] = {0x802e782cu, 0x802e7ddcu, 0x802e7e94u};
constexpr std::uint32_t d_168_headers[] = {0x802eeeacu};
constexpr const char* d_168_native[] = {"Pokemon_ForceAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag", "cmdSendCommandToLink"};
constexpr const char* d_169_native[] = {"Pokemon_TurnToTarget", "ohWait"};
constexpr std::uint32_t d_170_callbacks[] = {0x802e7ddcu, 0x802e7e94u};
constexpr const char* d_170_native[] = {"Pokemon_SetState", "Pokemon_StartPathProc"};
constexpr std::uint32_t d_171_callbacks[] = {0x802e789cu, 0x802e7e94u};
constexpr std::uint32_t d_171_headers[] = {0x802eee34u};
constexpr const char* d_171_native[] = {"Pokemon_ForceAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr const char* d_172_native[] = {"Pokemon_RunCleanup", "Pokemon_StopAuxProc", "getLevelProgress", "ohWait"};
constexpr std::uint32_t d_173_callbacks[] = {0x802e7f64u, 0x802e8058u};
constexpr const char* d_173_native[] = {"Pokemon_SetState", "Pokemon_StartAuxProc"};
constexpr std::uint32_t d_174_callbacks[] = {0x802e8058u, 0x802e80ccu};
constexpr std::uint32_t d_174_graphs[] = {0x802ef0a0u};
constexpr std::uint32_t d_174_headers[] = {0x802eedd0u};
constexpr const char* d_174_native[] = {"Pokemon_ForceAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr const char* d_175_native[] = {"Pokemon_TurnToModelNode", "omEndProcess"};
constexpr std::uint32_t d_176_headers[] = {0x802eee20u, 0x802eee5cu};
constexpr const char* d_176_native[] = {"Pokemon_GetDistance", "Pokemon_SetAnimation", "Pokemon_StopAuxProc"};
constexpr std::uint32_t d_177_callbacks[] = {0x802e8058u};
constexpr std::uint32_t d_177_headers[] = {0x802eee20u};
constexpr const char* d_177_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_178_callbacks[] = {0x802e8058u, 0x802e8330u, 0x802e8398u};
constexpr const char* d_178_native[] = {"Pokemon_SetState", "Pokemon_StartPathProc"};
constexpr std::uint32_t d_179_callbacks[] = {0x802e8058u};
constexpr std::uint32_t d_179_graphs[] = {0x802ef150u};
constexpr std::uint32_t d_179_headers[] = {0x802eee0cu};
constexpr const char* d_179_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr const char* d_180_native[] = {"Pokemon_FollowPath", "Pokemon_ResetPathPos", "omEndProcess"};
constexpr std::uint32_t d_181_callbacks[] = {0x802e8330u};
constexpr std::uint32_t d_181_headers[] = {0x802eee20u};
constexpr const char* d_181_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr const char* d_182_native[] = {"Pokemon_RunCleanup", "Pokemon_StopAuxProc", "getLevelProgress", "ohWait"};
constexpr std::uint32_t d_183_callbacks[] = {0x802e849cu, 0x802e85bcu};
constexpr const char* d_183_native[] = {"Pokemon_SetState", "Pokemon_StartAuxProc", "ohWait"};
constexpr std::uint32_t d_184_graphs[] = {0x802ef190u};
constexpr std::uint32_t d_184_headers[] = {0x802eedbcu};
constexpr const char* d_184_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_185_callbacks[] = {0x802e85bcu};
constexpr std::uint32_t d_185_headers[] = {0x802eee20u};
constexpr const char* d_185_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_186_callbacks[] = {0x802e86c4u, 0x802e8754u};
constexpr const char* d_186_native[] = {"Pokemon_SetState", "Pokemon_StartPathProc"};
constexpr std::uint32_t d_187_callbacks[] = {0x802e85bcu, 0x802e8868u};
constexpr std::uint32_t d_187_graphs[] = {0x802ef1e0u};
constexpr std::uint32_t d_187_headers[] = {0x802eee0cu};
constexpr const char* d_187_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr const char* d_188_native[] = {"Pokemon_FollowPath", "Pokemon_ResetPathPos", "Pokemon_TurnToTarget", "omEndProcess"};
constexpr std::uint32_t d_189_callbacks[] = {0x802e86c4u};
constexpr std::uint32_t d_189_headers[] = {0x802eee20u};
constexpr const char* d_189_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_190_callbacks[] = {0x802e88ecu, 0x802e8990u, 0x802e8aa4u};
constexpr std::uint32_t d_190_graphs[] = {0x802ef220u};
constexpr std::uint32_t d_190_headers[] = {0x802eedbcu};
constexpr const char* d_190_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartAuxProc", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr const char* d_191_native[] = {"Pokemon_GetDistance", "ohWait", "omEndProcess"};
constexpr const char* d_192_native[] = {"Pokemon_StopAuxProc", "Pokemon_TurnToTarget", "ohWait"};
constexpr std::uint32_t d_193_callbacks[] = {0x802e8868u};
constexpr std::uint32_t d_193_headers[] = {0x802eee20u};
constexpr const char* d_193_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_194_callbacks[] = {0x802e8b1cu, 0x802e8c94u};
constexpr std::uint32_t d_194_headers[] = {0x802eee0cu};
constexpr const char* d_194_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr const char* d_195_native[] = {"ABS", "GET_TRANSFORM", "Pokemon_SetTargetPos", "Pokemon_StepToTargetPos", "ohWait", "omEndProcess"};
constexpr std::uint32_t d_196_callbacks[] = {0x802e8d38u, 0x802e8d84u};
constexpr std::uint32_t d_196_graphs[] = {0x802ef260u};
constexpr std::uint32_t d_196_headers[] = {0x802eee0cu};
constexpr const char* d_196_native[] = {"EnvSound_PlaySound", "Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartAuxProc", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr const char* d_197_native[] = {"Pokemon_TurnToTarget", "omEndProcess"};
constexpr const char* d_198_native[] = {"GET_TRANSFORM", "Pokemon_StopAuxProc", "ohWait"};
constexpr std::uint32_t d_199_callbacks[] = {0x802e8eacu};
constexpr std::uint32_t d_199_headers[] = {0x802eee5cu};
constexpr const char* d_199_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_200_callbacks[] = {0x802e8f44u, 0x802e9098u, 0x802e90dcu};
constexpr std::uint32_t d_200_headers[] = {0x802eee0cu};
constexpr const char* d_200_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartAuxProc", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr const char* d_201_native[] = {"GetInterpolatedPosition", "Pokemon_SetTargetPos", "Pokemon_StepToTargetPos", "getCurrentWorldBlock", "ohWait", "omEndProcess"};
constexpr const char* d_202_native[] = {"Pokemon_FallDownOnGround", "Pokemon_StopAuxProc"};
constexpr std::uint32_t d_203_callbacks[] = {0x802e90dcu, 0x802e9150u};
constexpr std::uint32_t d_203_graphs[] = {0x802ef280u};
constexpr std::uint32_t d_203_headers[] = {0x802eedd0u};
constexpr const char* d_203_native[] = {"Pokemon_ForceAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr const char* d_204_native[] = {"Pokemon_TurnToTarget", "omEndProcess"};
constexpr std::uint32_t d_205_callbacks[] = {0x802e91fcu};
constexpr std::uint32_t d_205_headers[] = {0x802eee20u};
constexpr const char* d_205_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_206_callbacks[] = {0x802e9288u};
constexpr std::uint32_t d_206_headers[] = {0x802eedbcu, 0x802eee0cu};
constexpr const char* d_206_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_TurnToTarget", "Pokemon_WaitForFlag"};
constexpr const char* d_207_native[] = {"GetInterpolatedPosition", "Pokemon_RunToTargetPos", "Pokemon_SetTargetPos", "getCurrentWorldBlock", "getGroundAt", "omEndProcess"};
constexpr const char* d_208_native[] = {"Pokemon_SpawnOnGround"};
constexpr const char* d_209_native[] = {"Pokemon_RunCleanup", "Pokemon_StopAuxProc", "getLevelProgress", "ohWait"};
constexpr std::uint32_t d_210_graphs[] = {0x802ef788u};
constexpr const char* d_210_native[] = {"HIDE_POKEMON", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_211_callbacks[] = {0x802ea490u, 0x802ea694u};
constexpr std::uint32_t d_211_graphs[] = {0x802ef778u};
constexpr const char* d_211_native[] = {"Pokemon_SetState", "Pokemon_StartAuxProc", "Pokemon_WaitForFlag", "SHOW_POKEMON"};
constexpr std::uint32_t d_212_graphs[] = {0x802ef778u};
constexpr std::uint32_t d_212_headers[] = {0x802ef750u};
constexpr const char* d_212_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr const char* d_213_native[] = {"Pokemon_SpawnDlLink4"};
constexpr std::uint32_t d_214_callbacks[] = {0x802ea79cu, 0x802ea970u};
constexpr const char* d_214_native[] = {"Pokemon_SetState"};
constexpr std::uint32_t d_215_headers[] = {0x802efa2cu, 0x802efa40u};
constexpr const char* d_215_native[] = {"Pokemon_RunCleanup", "Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag", "cmdSendCommandToLink", "getLevelProgress", "ohWait"};
constexpr std::uint32_t d_216_headers[] = {0x802efa2cu, 0x802efa40u};
constexpr const char* d_216_native[] = {"Pokemon_RunCleanup", "Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag", "cmdSendCommandToLink", "getLevelProgress", "ohWait"};
constexpr const char* d_217_native[] = {"GET_TRANSFORM", "StaticObject_CollideOne", "guMtxCatF", "hal_rotate_rpy_translate_scale_f"};
constexpr std::uint32_t d_218_exact_abi_dependencies[] = {0x802eab44u};
constexpr std::uint32_t d_219_exact_abi_dependencies[] = {0x802eadacu};
constexpr std::uint32_t d_220_exact_abi_dependencies[] = {0x802eae7cu};
constexpr const char* d_220_native[] = {"GET_POKEMON", "InitOneCollisionModel", "Items_func_8035CA1C", "Pokemon_SpawnDlLink4"};
constexpr std::uint32_t d_221_callbacks[] = {0x802eb014u};
constexpr const char* d_221_native[] = {"Pokemon_SetState", "ohWait"};
constexpr std::uint32_t d_222_callbacks[] = {0x802eb0ccu, 0x802eb28cu};
constexpr const char* d_222_native[] = {"Pokemon_SetState", "Pokemon_StartPathProc", "getLevelProgress", "ohWait"};
constexpr std::uint32_t d_223_callbacks[] = {0x802eb0ccu};
constexpr std::uint32_t d_223_graphs[] = {0x802efac8u};
constexpr std::uint32_t d_223_headers[] = {0x802efaa0u, 0x802efab4u};
constexpr const char* d_223_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag", "randRange"};
constexpr const char* d_224_native[] = {"cmdSendCommand", "getLevelProgress", "ohWait", "omEndProcess"};
constexpr std::uint32_t d_225_headers[] = {0x802efab4u};
constexpr const char* d_225_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState"};
constexpr const char* d_226_native[] = {"Pokemon_Spawn"};
constexpr std::uint32_t d_227_callbacks[] = {0x802ec400u, 0x802ec6ccu};
constexpr std::uint32_t d_227_graphs[] = {0x802efffcu};
constexpr std::uint32_t d_227_headers[] = {0x802effd4u};
constexpr const char* d_227_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_228_callbacks[] = {0x802ec4e8u};
constexpr std::uint32_t d_228_headers[] = {0x802effd4u, 0x802effe8u};
constexpr const char* d_228_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag", "randRange"};
constexpr std::uint32_t d_229_callbacks[] = {0x802ec6ccu};
constexpr std::uint32_t d_229_headers[] = {0x802effd4u, 0x802effe8u};
constexpr const char* d_229_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag", "cmdSendCommand", "randRange"};
constexpr std::uint32_t d_230_callbacks[] = {0x802ec6ccu};
constexpr std::uint32_t d_230_headers[] = {0x802effe8u};
constexpr const char* d_230_native[] = {"Pokemon_ForceAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr const char* d_231_native[] = {"Pokemon_SpawnDlLink4"};
constexpr std::uint32_t d_232_callbacks[] = {0x802ec760u, 0x802eca2cu};
constexpr std::uint32_t d_232_graphs[] = {0x802f009cu};
constexpr std::uint32_t d_232_headers[] = {0x802f0074u};
constexpr const char* d_232_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_233_callbacks[] = {0x802ec848u};
constexpr std::uint32_t d_233_headers[] = {0x802f0074u, 0x802f0088u};
constexpr const char* d_233_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag", "randRange"};
constexpr std::uint32_t d_234_callbacks[] = {0x802eca2cu};
constexpr std::uint32_t d_234_headers[] = {0x802f0074u, 0x802f0088u};
constexpr const char* d_234_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag", "cmdSendCommand", "randRange"};
constexpr std::uint32_t d_235_callbacks[] = {0x802eca2cu};
constexpr std::uint32_t d_235_headers[] = {0x802f0088u};
constexpr const char* d_235_native[] = {"Pokemon_ForceAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr const char* d_236_native[] = {"Pokemon_SpawnDlLink4"};
constexpr std::uint32_t d_237_callbacks[] = {0x802ed644u};
constexpr const char* d_237_native[] = {"Pokemon_SetState"};
constexpr std::uint32_t d_238_graphs[] = {0x802f0228u};
constexpr std::uint32_t d_238_headers[] = {0x802f01f0u};
constexpr const char* d_238_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr const char* d_239_native[] = {"PlayerModel_SetAnimationSpeed"};
constexpr const char* d_240_native[] = {"PlayerModel_ChangeAnimation", "animSetCameraAnimation"};
constexpr const char* d_241_native[] = {"ARRAY_COUNT"};
constexpr const char* d_243_native[] = {"auPlaySoundWithParams", "auSetBGMVolumeSmooth", "auStopSound", "func_800AB41C", "func_800AB700", "func_800AB9A4", "ohWait"};
constexpr std::uint32_t d_244_callbacks[] = {0x802ed7ccu};
constexpr const char* d_244_native[] = {"omAddGObj", "omCreateProcess"};
constexpr std::uint32_t d_245_exact_abi_dependencies[] = {0x802ed69cu, 0x802ed6d0u, 0x802ed720u, 0x802ed7bcu, 0x802edae8u};
constexpr const char* d_245_native[] = {"Items_RemoveFlyingItems", "Msg_Reset", "PlayerModel_Init", "Pokemon_SetState", "animSetModelAnimationSpeed", "auSetBGMVolumeSmooth", "auSetSoundGlobalVolume", "cmdSendCommand", "func_800A7460", "func_800A7470", "func_800A7918", "func_800A7F68", "func_800AAED0", "func_800BFC5C_5CAFC", "func_800C02A0_5D140", "getMainCamera", "ohPauseObjectProcesses", "ohResumeObjectProcesses", "ohWait", "omCreateProcess", "omDeleteGObj", "omEndProcess", "resetMainCameraSettings", "setSkyBoxAnimationSpeed"};
constexpr const char* d_246_native[] = {"Pokemon_SpawnDlLink4"};
constexpr const char* d_247_native[] = {"auPlaySong", "auPlaySoundWithParams", "auSetBGMChannelVolume", "auSetBGMExtraReverb", "auSetBGMVolumeSmooth", "auSetSoundGlobalReverb", "auSetSoundVolume", "auStopSong", "auStopSound", "setBackgroundMusic"};
constexpr std::uint32_t d_249_graphs[] = {0x802efbc8u};
constexpr const char* d_249_native[] = {"HIDE_POKEMON", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_250_callbacks[] = {0x802eb548u, 0x802eb664u, 0x802eb6d0u, 0x802eb750u};
constexpr std::uint32_t d_250_graphs[] = {0x802efba8u};
constexpr std::uint32_t d_250_headers[] = {0x802efb54u, 0x802efb7cu, 0x802efb90u};
constexpr const char* d_250_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartAuxProc", "Pokemon_StartPathProc", "Pokemon_WaitForFlag", "SHOW_POKEMON"};
constexpr const char* d_251_native[] = {"GET_TRANSFORM", "atan2f", "cosf", "ohWait", "omEndProcess", "sinf"};
constexpr const char* d_252_native[] = {"Pokemon_FollowPath", "Pokemon_ResetPathPos", "omEndProcess"};
constexpr const char* d_253_native[] = {"Pokemon_StopAuxProc", "ohWait"};
constexpr std::uint32_t d_254_callbacks[] = {0x802eb750u};
constexpr std::uint32_t d_254_graphs[] = {0x802efba8u};
constexpr std::uint32_t d_254_headers[] = {0x802efb68u};
constexpr const char* d_254_native[] = {"Pokemon_ForceAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_255_callbacks[] = {0x802eb7e4u};
constexpr const char* d_255_native[] = {"Pokemon_SetState"};
constexpr const char* d_256_native[] = {"Pokemon_RunCleanup", "Pokemon_SetState"};
constexpr const char* d_257_native[] = {"Pokemon_Spawn"};
constexpr std::uint32_t d_258_callbacks[] = {0x802ebfdcu};
constexpr std::uint32_t d_258_graphs[] = {0x802eff4cu};
constexpr const char* d_258_native[] = {"HIDE_POKEMON", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_259_graphs[] = {0x802eff2cu};
constexpr std::uint32_t d_259_headers[] = {0x802efebcu};
constexpr const char* d_259_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_WaitForFlag", "SHOW_POKEMON"};
constexpr std::uint32_t d_260_callbacks[] = {0x802ec078u};
constexpr std::uint32_t d_260_headers[] = {0x802efef8u};
constexpr const char* d_260_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState"};
constexpr std::uint32_t d_261_callbacks[] = {0x802ec178u, 0x802ec23cu, 0x802ec294u};
constexpr std::uint32_t d_261_headers[] = {0x802efed0u, 0x802efee4u};
constexpr const char* d_261_native[] = {"Pokemon_RunCleanup", "Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartAuxProc", "Pokemon_StartPathProc", "Pokemon_WaitForFlag", "cmdSendCommandToLink"};
constexpr const char* d_262_native[] = {"EnvSound_PlaySound", "Pokemon_StopAuxProc", "ohWait"};
constexpr std::uint32_t d_263_exact_abi_dependencies[] = {0x80362414u};
constexpr const char* d_263_native[] = {"func_80362414_502824", "omEndProcess"};
constexpr const char* d_264_native[] = {"GET_TRANSFORM", "Pokemon_SetTargetPos", "Pokemon_StepToTargetPos", "ohWait", "omEndProcess"};
constexpr const char* d_265_native[] = {"pokemonRemoveOne"};
constexpr const char* d_266_native[] = {"Pokemon_Spawn"};
constexpr std::uint32_t d_267_callbacks[] = {0x802eb908u, 0x802eb9f8u, 0x802eba48u};
constexpr std::uint32_t d_267_graphs[] = {0x802efc5cu};
constexpr const char* d_267_native[] = {"HIDE_POKEMON", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_268_callbacks[] = {0x802eb98cu};
constexpr std::uint32_t d_268_graphs[] = {0x802efc7cu};
constexpr std::uint32_t d_268_headers[] = {0x802efc48u};
constexpr const char* d_268_native[] = {"Pokemon_RunCleanup", "Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag", "SHOW_POKEMON"};
constexpr const char* d_269_native[] = {"Pokemon_FollowPath", "Pokemon_ResetPathPos", "omEndProcess"};
constexpr std::uint32_t d_270_graphs[] = {0x802efcacu};
constexpr const char* d_270_native[] = {"HIDE_POKEMON", "Pokemon_SetState", "Pokemon_WaitForFlag"};
constexpr std::uint32_t d_271_callbacks[] = {0x802eba48u, 0x802ebabcu};
constexpr std::uint32_t d_271_graphs[] = {0x802efcccu};
constexpr std::uint32_t d_271_headers[] = {0x802efc48u};
constexpr const char* d_271_native[] = {"Pokemon_SetAnimation", "Pokemon_SetState", "Pokemon_StartPathProc", "Pokemon_WaitForFlag"};
constexpr const char* d_272_native[] = {"Pokemon_FollowPath", "Pokemon_ResetPathPos", "omEndProcess"};
constexpr const char* d_273_native[] = {"Pokemon_Fall", "Pokemon_Jump", "Pokemon_StopAuxProc"};
constexpr const char* d_274_native[] = {"Pokemon_Spawn"};
const actor::BehaviorCallbackBinding callback_bindings[] = {
    {0x802e5cb0u, diglett_InitialState, d_0_callbacks, {}, {}},
    {0x802e5d6cu, diglett_PhotoTaken, {}, {}, {}},
    {0x802e5d94u, diglett_FirstRise, d_2_callbacks, d_2_graphs, {}},
    {0x802e5f98u, diglett_SecondIdle, {}, d_3_graphs, {}},
    {0x802e5fd8u, diglett_SecondRise, d_4_callbacks, d_4_graphs, {}},
    {0x802e61dcu, diglett_ThirdIdle, {}, d_5_graphs, {}},
    {0x802e621cu, diglett_ThirdRise, d_6_callbacks, d_6_graphs, {}},
    {0x802e6458u, diglett_Remove, {}, {}, {}},
    {0x802e6490u, dugtrio_InitialState, d_9_callbacks, {}, {}},
    {0x802e654cu, dugtrio_PhotoTaken, {}, {}, {}},
    {0x802e6574u, dugtrio_FirstIdle, {}, d_11_graphs, {}},
    {0x802e65b4u, dugtrio_FirstRise, d_12_callbacks, d_12_graphs, {}},
    {0x802e67b8u, dugtrio_SecondIdle, {}, d_13_graphs, {}},
    {0x802e67f8u, dugtrio_SecondRise, d_14_callbacks, d_14_graphs, {}},
    {0x802e6a30u, dugtrio_ThirdIdle, {}, d_15_graphs, {}},
    {0x802e6a70u, dugtrio_ThirdRise, {}, d_16_graphs, {}},
    {0x802e6c74u, dugtrio_Remove, {}, {}, {}},
    {0x802e93d0u, func_802E93D0_5E64A0, d_19_callbacks, d_19_graphs, {}},
    {0x802e944cu, func_802E944C_5E651C, d_20_callbacks, {}, {}},
    {0x802e9538u, func_802E9538_5E6608, {}, d_21_graphs, d_21_weighted_states},
    {0x802e95e8u, func_802E95E8_5E66B8, d_22_callbacks, d_22_graphs, d_22_weighted_states},
    {0x802e968cu, func_802E968C_5E675C, {}, {}, {}},
    {0x802e96e8u, func_802E96E8_5E67B8, d_24_callbacks, d_24_graphs, {}},
    {0x802e97ecu, func_802E97EC_5E68BC, d_25_callbacks, {}, {}},
    {0x802e9854u, func_802E9854_5E6924, d_26_callbacks, d_26_graphs, {}},
    {0x802e98f4u, func_802E98F4_5E69C4, d_27_callbacks, d_27_graphs, {}},
    {0x802e9a48u, func_802E9A48_5E6B18, {}, {}, {}},
    {0x802e9a94u, func_802E9A94_5E6B64, {}, {}, {}},
    {0x802e9af8u, func_802E9AF8_5E6BC8, d_31_callbacks, d_31_graphs, {}},
    {0x802e9b90u, func_802E9B90_5E6C60, {}, {}, {}},
    {0x802e9becu, func_802E9BEC_5E6CBC, {}, {}, {}},
    {0x802e9c18u, func_802E9C18_5E6CE8, d_34_callbacks, {}, {}},
    {0x802e9c3cu, func_802E9C3C_5E6D0C, d_35_callbacks, {}, {}},
    {0x802e9c60u, func_802E9C60_5E6D30, d_36_callbacks, {}, {}},
    {0x802e9c98u, func_802E9C98_5E6D68, {}, {}, {}},
    {0x802e9d04u, func_802E9D04_5E6DD4, d_38_callbacks, d_38_graphs, {}},
    {0x802e9d70u, func_802E9D70_5E6E40, d_39_callbacks, d_39_graphs, {}},
    {0x802e9dd8u, func_802E9DD8_5E6EA8, {}, {}, {}},
    {0x802e9e54u, func_802E9E54_5E6F24, d_41_callbacks, d_41_graphs, {}},
    {0x802e9f20u, func_802E9F20_5E6FF0, d_42_callbacks, {}, {}},
    {0x802e9f80u, func_802E9F80_5E7050, d_43_callbacks, {}, {}},
    {0x802ea030u, func_802EA030_5E7100, d_44_callbacks, d_44_graphs, {}},
    {0x802ea098u, func_802EA098_5E7168, {}, {}, {}},
    {0x802ea108u, func_802EA108_5E71D8, d_46_callbacks, d_46_graphs, {}},
    {0x802ea1b4u, func_802EA1B4_5E7284, d_47_callbacks, d_47_graphs, {}},
    {0x802ea21cu, func_802EA21C_5E72EC, d_48_callbacks, d_48_graphs, {}},
    {0x802ea344u, func_802EA344_5E7414, d_49_callbacks, d_49_graphs, {}},
    {0x802ea3a0u, func_802EA3A0_5E7470, {}, {}, {}},
    {0x802ea3ecu, func_802EA3EC_5E74BC, {}, {}, {}},
    {0x802ea45cu, func_802EA45C_5E752C, {}, {}, {}},
    {0x802e50f0u, electrode_ChangeCollisionOffset, {}, {}, {}},
    {0x802e5134u, electrode_Remove, {}, {}, {}},
    {0x802e5160u, electrode_InitialState, d_56_callbacks, {}, {}},
    {0x802e5244u, electrode_Idle, {}, d_57_graphs, {}},
    {0x802e529cu, electrode_Explode, d_58_callbacks, d_58_graphs, {}},
    {0x802e53b8u, electrode_Behavior1, d_59_callbacks, {}, {}},
    {0x802e53dcu, electrode_Behavior1_Move, d_60_callbacks, {}, {}},
    {0x802e5448u, electrode_Behavior1_FollowPath, {}, {}, {}},
    {0x802e54b4u, electrode_Behavior4, d_62_callbacks, {}, {}},
    {0x802e54f0u, electrode_Behavior4_Move, d_63_callbacks, {}, {}},
    {0x802e556cu, electrode_Behavior4_FollowPath, {}, {}, {}},
    {0x802e55d8u, electrode_Behavior2, {}, d_65_graphs, {}},
    {0x802e5644u, electrode_Behavior3, d_66_callbacks, {}, {}},
    {0x802e5668u, electrode_Behavior3_Move, d_67_callbacks, d_67_graphs, {}},
    {0x802e56d8u, electrode_Behavior3_FollowPath, {}, {}, {}},
    {0x802e5748u, electrode_Behavior3Hit, d_69_callbacks, d_69_graphs, {}},
    {0x802e5818u, electrode_Behavior5, d_70_callbacks, {}, {}},
    {0x802e583cu, electrode_HiddenPathGuardIdle, d_71_callbacks, d_71_graphs, {}},
    {0x802e58b0u, electrode_WaitForPlayer, {}, {}, {}},
    {0x802e5968u, electrode_RevealHiddenPath, {}, {}, {}},
    {0x802ecac0u, func_802ECAC0_5E9B90, d_75_callbacks, {}, {}},
    {0x802ecae4u, func_802ECAE4_5E9BB4, {}, d_76_graphs, {}},
    {0x802ecb3cu, func_802ECB3C_5E9C0C, d_77_callbacks, {}, {}},
    {0x802ecc44u, func_802ECC44_5E9D14, {}, {}, {}},
    {0x802ecd84u, func_802ECD84_5E9E54, {}, {}, {}},
    {0x802ece10u, func_802ECE10_5E9EE0, d_81_callbacks, {}, {}},
    {0x802ebbe0u, func_802EBBE0_5E8CB0, d_83_callbacks, {}, {}},
    {0x802ebc04u, func_802EBC04_5E8CD4, d_84_callbacks, {}, {}},
    {0x802ebc78u, func_802EBC78_5E8D48, {}, {}, {}},
    {0x802e6cb0u, kakuna_InitialState, d_87_callbacks, d_87_graphs, {}},
    {0x802e6d90u, func_802E6D90_5E3E60, d_88_callbacks, d_88_graphs, {}},
    {0x802e6e0cu, func_802E6E0C_5E3EDC, d_89_callbacks, d_89_graphs, {}},
    {0x802e6e7cu, func_802E6E7C_5E3F4C, d_90_callbacks, {}, {}},
    {0x802e6ee8u, func_802E6EE8_5E3FB8, {}, {}, {}},
    {0x802e6f48u, func_802E6F48_5E4018, {}, {}, {}},
    {0x802e6fa8u, func_802E6FA8_5E4078, d_93_callbacks, {}, {}},
    {0x802e7014u, func_802E7014_5E40E4, {}, {}, {}},
    {0x802e70bcu, func_802E70BC_5E418C, {}, d_95_graphs, {}},
    {0x802e70fcu, func_802E70FC_5E41CC, d_96_callbacks, {}, {}},
    {0x802e71d4u, func_802E71D4_5E42A4, d_97_callbacks, d_97_graphs, {}},
    {0x802e729cu, func_802E729C_5E436C, {}, {}, {}},
    {0x802e7324u, func_802E7324_5E43F4, d_99_callbacks, d_99_graphs, {}},
    {0x802e73d0u, func_802E73D0_5E44A0, {}, {}, {}},
    {0x802e74a4u, func_802E74A4_5E4574, {}, {}, {}},
    {0x802e7504u, func_802E7504_5E45D4, d_102_callbacks, d_102_graphs, {}},
    {0x802e7594u, func_802E7594_5E4664, {}, {}, {}},
    {0x802e75f4u, func_802E75F4_5E46C4, {}, {}, {}},
    {0x802ebd20u, func_802EBD20_5E8DF0, d_106_callbacks, {}, {}},
    {0x802ebd6cu, func_802EBD6C_5E8E3C, {}, d_107_graphs, {}},
    {0x802ebdbcu, func_802EBDBC_5E8E8C, d_108_callbacks, {}, {}},
    {0x802ebe0cu, func_802EBE0C_5E8EDC, d_109_callbacks, {}, {}},
    {0x802ebec4u, func_802EBEC4_5E8F94, {}, {}, {}},
    {0x802e39a0u, magnemite_ProcessZoomIn, {}, {}, {}},
    {0x802e3a14u, magnemite_InitialState, d_113_callbacks, {}, {}},
    {0x802e3ac8u, magnemite_Idle, {}, d_114_graphs, d_114_weighted_states},
    {0x802e3b8cu, magnemite_HitByPesterBall, d_115_callbacks, d_115_graphs, {}},
    {0x802e3c18u, magnemite_HitByApple, d_116_callbacks, d_116_graphs, {}},
    {0x802e3ca4u, magnemite_AffectedByPesterBall, d_117_callbacks, {}, {}},
    {0x802e3cc8u, magnemite_AppleNearby, d_118_callbacks, d_118_graphs, {}},
    {0x802e3d80u, magnemite_MoveToApple, {}, {}, {}},
    {0x802e3e20u, magnemite_SearchApple, d_120_callbacks, {}, {}},
    {0x802e3e44u, magnemite_WithApple, d_121_callbacks, d_121_graphs, d_121_weighted_states},
    {0x802e3ee4u, magnemite_RecoverFromHit, d_122_callbacks, d_122_graphs, {}},
    {0x802e3f8cu, magnemite_TurnToPlayer, {}, {}, {}},
    {0x802e3fd8u, magnemite_PlayerApproaching, d_124_callbacks, d_124_graphs, {}},
    {0x802e40d4u, magnemite_RunAwayFromPlayer, {}, {}, {}},
    {0x802e4124u, magnemite_ZoomedIn, d_126_callbacks, d_126_graphs, {}},
    {0x802e4234u, magnemite_FindNeighbours, {}, {}, {}},
    {0x802e4434u, magnemite_Attract, d_129_callbacks, {}, {}},
    {0x802e44dcu, magnemite_MoveToAttractedMagnemite, {}, {}, {}},
    {0x802e45b4u, magnemite_WaitForLeader, d_131_callbacks, d_131_graphs, {}},
    {0x802e4668u, magnemite_CopyLeaderBehavior, d_132_callbacks, d_132_graphs, {}},
    {0x802e4710u, magnemite_CopyLeaderMovement, {}, {}, {}},
    {0x802e480cu, magnemite_HitByPesterBall_SendToLeader, {}, {}, {}},
    {0x802e4844u, magnemite_HitByApple_SendToLeader, {}, {}, {}},
    {0x802e487cu, magnemite_AllMagnemitesCombined, d_136_callbacks, {}, {}},
    {0x802e4964u, magnemite_EvolveIntoMagneton, d_137_callbacks, {}, {}},
    {0x802e49a4u, magnemite_WaitToDelete, d_138_callbacks, {}, {}},
    {0x802e4a84u, magnemite_Delete, {}, {}, {}},
    {0x802e4b40u, magneton_InitialState, d_141_callbacks, {}, {}},
    {0x802e4b64u, magneton_Idle, {}, d_142_graphs, d_142_weighted_states},
    {0x802e4bd4u, magneton_Roam1, d_143_callbacks, d_143_graphs, d_143_weighted_states},
    {0x802e4c6cu, magneton_MoveRoam1, {}, {}, {}},
    {0x802e4cbcu, magneton_Roam2, d_145_callbacks, d_145_graphs, d_145_weighted_states},
    {0x802e4d54u, magneton_MoveRoam2, {}, {}, {}},
    {0x802e4da4u, magneton_AppleNearby, d_147_callbacks, d_147_graphs, {}},
    {0x802e4e2cu, magneton_MoveToApple, {}, {}, {}},
    {0x802e4ea4u, magneton_SearchApple, d_149_callbacks, {}, {}},
    {0x802e4ec8u, magneton_WithApple, d_150_callbacks, d_150_graphs, d_150_weighted_states},
    {0x802e4f4cu, magneton_RunAway, d_151_callbacks, d_151_graphs, {}},
    {0x802e5008u, magneton_MoveRunAway, {}, {}, {}},
    {0x802e5094u, tunnel_SpawnMagneton, {}, {}, {}},
    {0x802e2500u, tunnel_SpawnMagikarp, {}, {}, {}},
    {0x802e7680u, func_802E7680_5E4750, d_157_callbacks, {}, {}},
    {0x802e7718u, func_802E7718_5E47E8, d_158_callbacks, {}, {}},
    {0x802e77b0u, func_802E77B0_5E4880, {}, d_159_graphs, d_159_weighted_states},
    {0x802e782cu, func_802E782C_5E48FC, {}, d_160_graphs, d_160_weighted_states},
    {0x802e789cu, func_802E789C_5E496C, d_161_callbacks, {}, {}},
    {0x802e7948u, func_802E7948_5E4A18, d_162_callbacks, {}, {}},
    {0x802e796cu, func_802E796C_5E4A3C, d_163_callbacks, {}, {}},
    {0x802e7ac8u, func_802E7AC8_5E4B98, {}, {}, {}},
    {0x802e7b3cu, func_802E7B3C_5E4C0C, d_165_callbacks, {}, {}},
    {0x802e7c1cu, func_802E7C1C_5E4CEC, d_166_callbacks, {}, {}},
    {0x802e7ca4u, func_802E7CA4_5E4D74, {}, {}, {}},
    {0x802e7d04u, func_802E7D04_5E4DD4, d_168_callbacks, {}, {}},
    {0x802e7ddcu, func_802E7DDC_5E4EAC, {}, {}, {}},
    {0x802e7e5cu, func_802E7E5C_5E4F2C, d_170_callbacks, {}, {}},
    {0x802e7e94u, func_802E7E94_5E4F64, d_171_callbacks, {}, {}},
    {0x802e7f64u, func_802E7F64_5E5034, {}, {}, {}},
    {0x802e8008u, func_802E8008_5E50D8, d_173_callbacks, {}, {}},
    {0x802e8058u, func_802E8058_5E5128, d_174_callbacks, d_174_graphs, {}},
    {0x802e80ccu, func_802E80CC_5E519C, {}, {}, {}},
    {0x802e818cu, func_802E818C_5E525C, {}, {}, {}},
    {0x802e823cu, func_802E823C_5E530C, d_177_callbacks, {}, {}},
    {0x802e8290u, func_802E8290_5E5360, d_178_callbacks, {}, {}},
    {0x802e8330u, func_802E8330_5E5400, d_179_callbacks, d_179_graphs, {}},
    {0x802e8398u, func_802E8398_5E5468, {}, {}, {}},
    {0x802e8428u, func_802E8428_5E54F8, d_181_callbacks, {}, {}},
    {0x802e849cu, func_802E849C_5E556C, {}, {}, {}},
    {0x802e853cu, func_802E853C_5E560C, d_183_callbacks, {}, {}},
    {0x802e85bcu, func_802E85BC_5E568C, {}, d_184_graphs, {}},
    {0x802e8628u, func_802E8628_5E56F8, d_185_callbacks, {}, {}},
    {0x802e867cu, func_802E867C_5E574C, d_186_callbacks, {}, {}},
    {0x802e86c4u, func_802E86C4_5E5794, d_187_callbacks, d_187_graphs, {}},
    {0x802e8754u, func_802E8754_5E5824, {}, {}, {}},
    {0x802e87f4u, func_802E87F4_5E58C4, d_189_callbacks, {}, {}},
    {0x802e8868u, func_802E8868_5E5938, d_190_callbacks, d_190_graphs, {}},
    {0x802e88ecu, func_802E88EC_5E59BC, {}, {}, {}},
    {0x802e8990u, func_802E8990_5E5A60, {}, {}, {}},
    {0x802e8a24u, func_802E8A24_5E5AF4, d_193_callbacks, {}, {}},
    {0x802e8aa4u, func_802E8AA4_5E5B74, d_194_callbacks, {}, {}},
    {0x802e8b1cu, func_802E8B1C_5E5BEC, {}, {}, {}},
    {0x802e8c94u, func_802E8C94_5E5D64, d_196_callbacks, d_196_graphs, {}},
    {0x802e8d38u, func_802E8D38_5E5E08, {}, {}, {}},
    {0x802e8d84u, func_802E8D84_5E5E54, {}, {}, {}},
    {0x802e8e34u, func_802E8E34_5E5F04, d_199_callbacks, {}, {}},
    {0x802e8eacu, func_802E8EAC_5E5F7C, d_200_callbacks, {}, {}},
    {0x802e8f44u, func_802E8F44_5E6014, {}, {}, {}},
    {0x802e9098u, func_802E9098_5E6168, {}, {}, {}},
    {0x802e90dcu, func_802E90DC_5E61AC, d_203_callbacks, d_203_graphs, {}},
    {0x802e9150u, func_802E9150_5E6220, {}, {}, {}},
    {0x802e91a8u, func_802E91A8_5E6278, d_205_callbacks, {}, {}},
    {0x802e91fcu, func_802E91FC_5E62CC, d_206_callbacks, {}, {}},
    {0x802e9288u, func_802E9288_5E6358, {}, {}, {}},
    {0x802ea490u, func_802EA490_5E7560, {}, {}, {}},
    {0x802ea53cu, func_802EA53C_5E760C, {}, d_210_graphs, {}},
    {0x802ea5d0u, func_802EA5D0_5E76A0, d_211_callbacks, d_211_graphs, {}},
    {0x802ea694u, func_802EA694_5E7764, {}, d_212_graphs, {}},
    {0x802ea730u, func_802EA730_5E7800, d_214_callbacks, {}, {}},
    {0x802ea79cu, func_802EA79C_5E786C, {}, {}, {}},
    {0x802ea970u, func_802EA970_5E7A40, {}, {}, {}},
    {0x802eafb0u, func_802EAFB0_5E8080, d_221_callbacks, {}, {}},
    {0x802eb014u, func_802EB014_5E80E4, d_222_callbacks, {}, {}},
    {0x802eb0ccu, func_802EB0CC_5E819C, d_223_callbacks, d_223_graphs, {}},
    {0x802eb28cu, func_802EB28C_5E835C, {}, {}, {}},
    {0x802eb35cu, func_802EB35C_5E842C, {}, {}, {}},
    {0x802ec400u, func_802EC400_5E94D0, d_227_callbacks, d_227_graphs, {}},
    {0x802ec4e8u, func_802EC4E8_5E95B8, d_228_callbacks, {}, {}},
    {0x802ec5b0u, func_802EC5B0_5E9680, d_229_callbacks, {}, {}},
    {0x802ec6ccu, func_802EC6CC_5E979C, d_230_callbacks, {}, {}},
    {0x802ec760u, func_802EC760_5E9830, d_232_callbacks, d_232_graphs, {}},
    {0x802ec848u, func_802EC848_5E9918, d_233_callbacks, {}, {}},
    {0x802ec910u, func_802EC910_5E99E0, d_234_callbacks, {}, {}},
    {0x802eca2cu, func_802ECA2C_5E9AFC, d_235_callbacks, {}, {}},
    {0x802ed620u, func_802ED620_5EA6F0, d_237_callbacks, {}, {}},
    {0x802ed644u, func_802ED644_5EA714, {}, d_238_graphs, {}},
    {0x802ed7ccu, func_802ED7CC_5EA89C, {}, {}, {}},
    {0x802edb3cu, func_802EDB3C_5EAC0C, {}, {}, {}},
    {0x802eb3e0u, func_802EB3E0_5E84B0, {}, d_249_graphs, {}},
    {0x802eb430u, func_802EB430_5E8500, d_250_callbacks, d_250_graphs, {}},
    {0x802eb548u, func_802EB548_5E8618, {}, {}, {}},
    {0x802eb664u, func_802EB664_5E8734, {}, {}, {}},
    {0x802eb6d0u, func_802EB6D0_5E87A0, {}, {}, {}},
    {0x802eb750u, func_802EB750_5E8820, d_254_callbacks, d_254_graphs, {}},
    {0x802eb7acu, func_802EB7AC_5E887C, d_255_callbacks, {}, {}},
    {0x802eb7e4u, func_802EB7E4_5E88B4, {}, {}, {}},
    {0x802ebf40u, func_802EBF40_5E9010, d_258_callbacks, d_258_graphs, {}},
    {0x802ebfdcu, func_802EBFDC_5E90AC, {}, d_259_graphs, {}},
    {0x802ec040u, func_802EC040_5E9110, d_260_callbacks, {}, {}},
    {0x802ec078u, func_802EC078_5E9148, d_261_callbacks, {}, {}},
    {0x802ec178u, func_802EC178_5E9248, {}, {}, {}},
    {0x802ec23cu, func_802EC23C_5E930C, {}, {}, {}},
    {0x802ec294u, func_802EC294_5E9364, {}, {}, {}},
    {0x802ec398u, func_802EC398_5E9468, {}, {}, {}},
    {0x802eb860u, func_802EB860_5E8930, d_267_callbacks, d_267_graphs, {}},
    {0x802eb908u, func_802EB908_5E89D8, d_268_callbacks, d_268_graphs, {}},
    {0x802eb98cu, func_802EB98C_5E8A5C, {}, {}, {}},
    {0x802eb9f8u, func_802EB9F8_5E8AC8, {}, d_270_graphs, {}},
    {0x802eba48u, func_802EBA48_5E8B18, d_271_callbacks, d_271_graphs, {}},
    {0x802ebabcu, func_802EBABC_5E8B8C, {}, {}, {}},
    {0x802ebb28u, func_802EBB28_5E8BF8, {}, {}, {}},
};
constexpr ActorDependency dependencies[] = {
    {0x802e5cb0u, true, SourceAbi::gobj_callback, {"src/tunnel/diglett/code.c", "diglett_InitialState", 111, 127}, "POKEMON_FUNC(diglett_InitialState)", d_0_callbacks, {}, {}, {}, {}, d_0_native},
    {0x802e5d6cu, true, SourceAbi::gobj_callback, {"src/tunnel/diglett/code.c", "diglett_PhotoTaken", 129, 132}, "POKEMON_FUNC(diglett_PhotoTaken)", {}, {}, {}, {}, {}, d_1_native},
    {0x802e5d94u, true, SourceAbi::gobj_callback, {"src/tunnel/diglett/code.c", "diglett_FirstRise", 134, 186}, "POKEMON_FUNC(diglett_FirstRise)", d_2_callbacks, d_2_graphs, {}, d_2_headers, {}, d_2_native},
    {0x802e5f98u, true, SourceAbi::gobj_callback, {"src/tunnel/diglett/code.c", "diglett_SecondIdle", 188, 192}, "POKEMON_FUNC(diglett_SecondIdle)", {}, d_3_graphs, {}, {}, {}, d_3_native},
    {0x802e5fd8u, true, SourceAbi::gobj_callback, {"src/tunnel/diglett/code.c", "diglett_SecondRise", 194, 246}, "POKEMON_FUNC(diglett_SecondRise)", d_4_callbacks, d_4_graphs, {}, d_4_headers, {}, d_4_native},
    {0x802e61dcu, true, SourceAbi::gobj_callback, {"src/tunnel/diglett/code.c", "diglett_ThirdIdle", 248, 252}, "POKEMON_FUNC(diglett_ThirdIdle)", {}, d_5_graphs, {}, {}, {}, d_5_native},
    {0x802e621cu, true, SourceAbi::gobj_callback, {"src/tunnel/diglett/code.c", "diglett_ThirdRise", 254, 306}, "POKEMON_FUNC(diglett_ThirdRise)", d_6_callbacks, d_6_graphs, {}, d_6_headers, {}, d_6_native},
    {0x802e6420u, true, SourceAbi::factory_five_argument, {"src/tunnel/diglett/code.c", "diglett_Spawn", 308, 310}, "GObj* diglett_Spawn(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)", {}, {}, {}, {}, {}, d_7_native},
    {0x802e6458u, true, SourceAbi::gobj_callback, {"src/tunnel/diglett/code.c", "diglett_Remove", 312, 317}, "void diglett_Remove(GObj* obj)", {}, {}, {}, {}, {}, d_8_native},
    {0x802e6490u, true, SourceAbi::gobj_callback, {"src/tunnel/dugtrio/code.c", "dugtrio_InitialState", 110, 126}, "POKEMON_FUNC(dugtrio_InitialState)", d_9_callbacks, {}, {}, {}, {}, d_9_native},
    {0x802e654cu, true, SourceAbi::gobj_callback, {"src/tunnel/dugtrio/code.c", "dugtrio_PhotoTaken", 128, 131}, "POKEMON_FUNC(dugtrio_PhotoTaken)", {}, {}, {}, {}, {}, d_10_native},
    {0x802e6574u, true, SourceAbi::gobj_callback, {"src/tunnel/dugtrio/code.c", "dugtrio_FirstIdle", 133, 137}, "POKEMON_FUNC(dugtrio_FirstIdle)", {}, d_11_graphs, {}, {}, {}, d_11_native},
    {0x802e65b4u, true, SourceAbi::gobj_callback, {"src/tunnel/dugtrio/code.c", "dugtrio_FirstRise", 139, 191}, "POKEMON_FUNC(dugtrio_FirstRise)", d_12_callbacks, d_12_graphs, {}, d_12_headers, {}, d_12_native},
    {0x802e67b8u, true, SourceAbi::gobj_callback, {"src/tunnel/dugtrio/code.c", "dugtrio_SecondIdle", 193, 197}, "POKEMON_FUNC(dugtrio_SecondIdle)", {}, d_13_graphs, {}, {}, {}, d_13_native},
    {0x802e67f8u, true, SourceAbi::gobj_callback, {"src/tunnel/dugtrio/code.c", "dugtrio_SecondRise", 199, 256}, "POKEMON_FUNC(dugtrio_SecondRise)", d_14_callbacks, d_14_graphs, {}, d_14_headers, {}, d_14_native},
    {0x802e6a30u, true, SourceAbi::gobj_callback, {"src/tunnel/dugtrio/code.c", "dugtrio_ThirdIdle", 258, 262}, "POKEMON_FUNC(dugtrio_ThirdIdle)", {}, d_15_graphs, {}, {}, {}, d_15_native},
    {0x802e6a70u, true, SourceAbi::gobj_callback, {"src/tunnel/dugtrio/code.c", "dugtrio_ThirdRise", 264, 308}, "POKEMON_FUNC(dugtrio_ThirdRise)", {}, d_16_graphs, {}, d_16_headers, {}, d_16_native},
    {0x802e6c28u, true, SourceAbi::factory_five_argument, {"src/tunnel/dugtrio/code.c", "dugtrio_Spawn", 310, 314}, "GObj* dugtrio_Spawn(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)", {}, {}, {}, {}, {}, d_17_native},
    {0x802e6c74u, true, SourceAbi::gobj_callback, {"src/tunnel/dugtrio/code.c", "dugtrio_Remove", 316, 321}, "void dugtrio_Remove(GObj* obj)", {}, {}, {}, {}, {}, d_18_native},
    {0x802e93d0u, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802E93D0_5E64A0", 53, 64}, "POKEMON_FUNC(func_802E93D0_5E64A0)", d_19_callbacks, d_19_graphs, {}, {}, {}, d_19_native},
    {0x802e944cu, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802E944C_5E651C", 66, 88}, "POKEMON_FUNC(func_802E944C_5E651C)", d_20_callbacks, {}, {}, {}, {}, d_20_native},
    {0x802e9538u, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802E9538_5E6608", 90, 104}, "POKEMON_FUNC(func_802E9538_5E6608)", {}, d_21_graphs, d_21_weighted_states, d_21_headers, {}, d_21_native},
    {0x802e95e8u, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802E95E8_5E66B8", 106, 119}, "POKEMON_FUNC(func_802E95E8_5E66B8)", d_22_callbacks, d_22_graphs, d_22_weighted_states, d_22_headers, {}, d_22_native},
    {0x802e968cu, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802E968C_5E675C", 121, 127}, "POKEMON_FUNC(func_802E968C_5E675C)", {}, {}, {}, {}, {}, d_23_native},
    {0x802e96e8u, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802E96E8_5E67B8", 129, 153}, "POKEMON_FUNC(func_802E96E8_5E67B8)", d_24_callbacks, d_24_graphs, {}, d_24_headers, {}, d_24_native},
    {0x802e97ecu, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802E97EC_5E68BC", 155, 162}, "POKEMON_FUNC(func_802E97EC_5E68BC)", d_25_callbacks, {}, {}, d_25_headers, {}, d_25_native},
    {0x802e9854u, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802E9854_5E6924", 164, 175}, "POKEMON_FUNC(func_802E9854_5E6924)", d_26_callbacks, d_26_graphs, {}, d_26_headers, {}, d_26_native},
    {0x802e98f4u, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802E98F4_5E69C4", 177, 206}, "POKEMON_FUNC(func_802E98F4_5E69C4)", d_27_callbacks, d_27_graphs, {}, d_27_headers, {}, d_27_native},
    {0x802e9a48u, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802E9A48_5E6B18", 208, 213}, "POKEMON_FUNC(func_802E9A48_5E6B18)", {}, {}, {}, {}, {}, d_28_native},
    {0x802e9a94u, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802E9A94_5E6B64", 215, 221}, "POKEMON_FUNC(func_802E9A94_5E6B64)", {}, {}, {}, {}, {}, d_29_native},
    {0u, false, SourceAbi::file_local_empty, {"src/tunnel/electabuzz/code.c", "nullsub", 223, 224}, "static void nullsub(void)", {}, {}, {}, {}, {}, {}},
    {0x802e9af8u, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802E9AF8_5E6BC8", 226, 237}, "POKEMON_FUNC(func_802E9AF8_5E6BC8)", d_31_callbacks, d_31_graphs, {}, d_31_headers, {}, d_31_native},
    {0x802e9b90u, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802E9B90_5E6C60", 239, 245}, "POKEMON_FUNC(func_802E9B90_5E6C60)", {}, {}, {}, {}, {}, d_32_native},
    {0x802e9becu, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802E9BEC_5E6CBC", 247, 250}, "POKEMON_FUNC(func_802E9BEC_5E6CBC)", {}, {}, {}, {}, {}, d_33_native},
    {0x802e9c18u, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802E9C18_5E6CE8", 252, 254}, "POKEMON_FUNC(func_802E9C18_5E6CE8)", d_34_callbacks, {}, {}, {}, {}, d_34_native},
    {0x802e9c3cu, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802E9C3C_5E6D0C", 256, 258}, "POKEMON_FUNC(func_802E9C3C_5E6D0C)", d_35_callbacks, {}, {}, {}, {}, d_35_native},
    {0x802e9c60u, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802E9C60_5E6D30", 260, 263}, "POKEMON_FUNC(func_802E9C60_5E6D30)", d_36_callbacks, {}, {}, {}, {}, d_36_native},
    {0x802e9c98u, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802E9C98_5E6D68", 265, 271}, "POKEMON_FUNC(func_802E9C98_5E6D68)", {}, {}, {}, {}, {}, d_37_native},
    {0x802e9d04u, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802E9D04_5E6DD4", 273, 280}, "POKEMON_FUNC(func_802E9D04_5E6DD4)", d_38_callbacks, d_38_graphs, {}, d_38_headers, {}, d_38_native},
    {0x802e9d70u, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802E9D70_5E6E40", 282, 289}, "POKEMON_FUNC(func_802E9D70_5E6E40)", d_39_callbacks, d_39_graphs, {}, d_39_headers, {}, d_39_native},
    {0x802e9dd8u, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802E9DD8_5E6EA8", 291, 301}, "POKEMON_FUNC(func_802E9DD8_5E6EA8)", {}, {}, {}, d_40_headers, {}, d_40_native},
    {0x802e9e54u, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802E9E54_5E6F24", 303, 326}, "POKEMON_FUNC(func_802E9E54_5E6F24)", d_41_callbacks, d_41_graphs, {}, d_41_headers, {}, d_41_native},
    {0x802e9f20u, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802E9F20_5E6FF0", 328, 335}, "POKEMON_FUNC(func_802E9F20_5E6FF0)", d_42_callbacks, {}, {}, d_42_headers, {}, d_42_native},
    {0x802e9f80u, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802E9F80_5E7050", 337, 350}, "POKEMON_FUNC(func_802E9F80_5E7050)", d_43_callbacks, {}, {}, d_43_headers, {}, d_43_native},
    {0x802ea030u, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802EA030_5E7100", 352, 359}, "POKEMON_FUNC(func_802EA030_5E7100)", d_44_callbacks, d_44_graphs, {}, d_44_headers, {}, d_44_native},
    {0x802ea098u, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802EA098_5E7168", 361, 367}, "POKEMON_FUNC(func_802EA098_5E7168)", {}, {}, {}, {}, {}, d_45_native},
    {0x802ea108u, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802EA108_5E71D8", 369, 384}, "POKEMON_FUNC(func_802EA108_5E71D8)", d_46_callbacks, d_46_graphs, {}, d_46_headers, {}, d_46_native},
    {0x802ea1b4u, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802EA1B4_5E7284", 386, 393}, "POKEMON_FUNC(func_802EA1B4_5E7284)", d_47_callbacks, d_47_graphs, {}, d_47_headers, {}, d_47_native},
    {0x802ea21cu, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802EA21C_5E72EC", 395, 418}, "POKEMON_FUNC(func_802EA21C_5E72EC)", d_48_callbacks, d_48_graphs, {}, d_48_headers, {}, d_48_native},
    {0x802ea344u, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802EA344_5E7414", 420, 426}, "POKEMON_FUNC(func_802EA344_5E7414)", d_49_callbacks, d_49_graphs, {}, d_49_headers, {}, d_49_native},
    {0x802ea3a0u, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802EA3A0_5E7470", 428, 433}, "POKEMON_FUNC(func_802EA3A0_5E7470)", {}, {}, {}, {}, {}, d_50_native},
    {0x802ea3ecu, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802EA3EC_5E74BC", 435, 438}, "POKEMON_FUNC(func_802EA3EC_5E74BC)", {}, {}, {}, {}, {}, d_51_native},
    {0x802ea424u, true, SourceAbi::factory_five_argument, {"src/tunnel/electabuzz/code.c", "electabuzz_Spawn", 443, 445}, "GObj* electabuzz_Spawn(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)", {}, {}, {}, {}, {}, d_52_native},
    {0x802ea45cu, true, SourceAbi::gobj_callback, {"src/tunnel/electabuzz/code.c", "func_802EA45C_5E752C", 447, 451}, "POKEMON_FUNC(func_802EA45C_5E752C)", {}, {}, {}, {}, {}, d_53_native},
    {0x802e50f0u, true, SourceAbi::gobj_callback, {"src/tunnel/electrode/code.c", "electrode_ChangeCollisionOffset", 94, 98}, "POKEMON_FUNC(electrode_ChangeCollisionOffset)", {}, {}, {}, {}, {}, d_54_native},
    {0x802e5134u, true, SourceAbi::gobj_callback, {"src/tunnel/electrode/code.c", "electrode_Remove", 100, 103}, "POKEMON_FUNC(electrode_Remove)", {}, {}, {}, {}, {}, d_55_native},
    {0x802e5160u, true, SourceAbi::gobj_callback, {"src/tunnel/electrode/code.c", "electrode_InitialState", 105, 123}, "POKEMON_FUNC(electrode_InitialState)", d_56_callbacks, {}, {}, {}, {}, d_56_native},
    {0x802e5244u, true, SourceAbi::gobj_callback, {"src/tunnel/electrode/code.c", "electrode_Idle", 125, 131}, "POKEMON_FUNC(electrode_Idle)", {}, d_57_graphs, {}, d_57_headers, {}, d_57_native},
    {0x802e529cu, true, SourceAbi::gobj_callback, {"src/tunnel/electrode/code.c", "electrode_Explode", 133, 160}, "POKEMON_FUNC(electrode_Explode)", d_58_callbacks, d_58_graphs, {}, d_58_headers, {}, d_58_native},
    {0x802e53b8u, true, SourceAbi::gobj_callback, {"src/tunnel/electrode/code.c", "electrode_Behavior1", 162, 164}, "POKEMON_FUNC(electrode_Behavior1)", d_59_callbacks, {}, {}, {}, {}, d_59_native},
    {0x802e53dcu, true, SourceAbi::gobj_callback, {"src/tunnel/electrode/code.c", "electrode_Behavior1_Move", 166, 173}, "POKEMON_FUNC(electrode_Behavior1_Move)", d_60_callbacks, {}, {}, d_60_headers, {}, d_60_native},
    {0x802e5448u, true, SourceAbi::gobj_callback, {"src/tunnel/electrode/code.c", "electrode_Behavior1_FollowPath", 175, 181}, "POKEMON_FUNC(electrode_Behavior1_FollowPath)", {}, {}, {}, {}, {}, d_61_native},
    {0x802e54b4u, true, SourceAbi::gobj_callback, {"src/tunnel/electrode/code.c", "electrode_Behavior4", 183, 187}, "POKEMON_FUNC(electrode_Behavior4)", d_62_callbacks, {}, {}, {}, {}, d_62_native},
    {0x802e54f0u, true, SourceAbi::gobj_callback, {"src/tunnel/electrode/code.c", "electrode_Behavior4_Move", 189, 197}, "POKEMON_FUNC(electrode_Behavior4_Move)", d_63_callbacks, {}, {}, d_63_headers, {}, d_63_native},
    {0x802e556cu, true, SourceAbi::gobj_callback, {"src/tunnel/electrode/code.c", "electrode_Behavior4_FollowPath", 199, 205}, "POKEMON_FUNC(electrode_Behavior4_FollowPath)", {}, {}, {}, {}, {}, d_64_native},
    {0x802e55d8u, true, SourceAbi::gobj_callback, {"src/tunnel/electrode/code.c", "electrode_Behavior2", 207, 214}, "POKEMON_FUNC(electrode_Behavior2)", {}, d_65_graphs, {}, d_65_headers, {}, d_65_native},
    {0x802e5644u, true, SourceAbi::gobj_callback, {"src/tunnel/electrode/code.c", "electrode_Behavior3", 216, 218}, "POKEMON_FUNC(electrode_Behavior3)", d_66_callbacks, {}, {}, {}, {}, d_66_native},
    {0x802e5668u, true, SourceAbi::gobj_callback, {"src/tunnel/electrode/code.c", "electrode_Behavior3_Move", 220, 227}, "POKEMON_FUNC(electrode_Behavior3_Move)", d_67_callbacks, d_67_graphs, {}, d_67_headers, {}, d_67_native},
    {0x802e56d8u, true, SourceAbi::gobj_callback, {"src/tunnel/electrode/code.c", "electrode_Behavior3_FollowPath", 229, 235}, "POKEMON_FUNC(electrode_Behavior3_FollowPath)", {}, {}, {}, {}, {}, d_68_native},
    {0x802e5748u, true, SourceAbi::gobj_callback, {"src/tunnel/electrode/code.c", "electrode_Behavior3Hit", 237, 254}, "POKEMON_FUNC(electrode_Behavior3Hit)", d_69_callbacks, d_69_graphs, {}, d_69_headers, {}, d_69_native},
    {0x802e5818u, true, SourceAbi::gobj_callback, {"src/tunnel/electrode/code.c", "electrode_Behavior5", 256, 258}, "POKEMON_FUNC(electrode_Behavior5)", d_70_callbacks, {}, {}, {}, {}, d_70_native},
    {0x802e583cu, true, SourceAbi::gobj_callback, {"src/tunnel/electrode/code.c", "electrode_HiddenPathGuardIdle", 260, 267}, "POKEMON_FUNC(electrode_HiddenPathGuardIdle)", d_71_callbacks, d_71_graphs, {}, d_71_headers, {}, d_71_native},
    {0x802e58b0u, true, SourceAbi::gobj_callback, {"src/tunnel/electrode/code.c", "electrode_WaitForPlayer", 269, 284}, "POKEMON_FUNC(electrode_WaitForPlayer)", {}, {}, {}, {}, {}, d_72_native},
    {0x802e5968u, true, SourceAbi::gobj_callback, {"src/tunnel/electrode/code.c", "electrode_RevealHiddenPath", 292, 353}, "POKEMON_FUNC(electrode_RevealHiddenPath)", {}, {}, {}, d_73_headers, {}, d_73_native},
    {0x802e5c74u, true, SourceAbi::factory_five_argument, {"src/tunnel/electrode/code.c", "electrode_Spawn", 379, 381}, "GObj* electrode_Spawn(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)", {}, {}, {}, {}, {}, d_74_native},
    {0x802ecac0u, true, SourceAbi::gobj_callback, {"src/tunnel/gate/code.c", "func_802ECAC0_5E9B90", 24, 26}, "POKEMON_FUNC(func_802ECAC0_5E9B90)", d_75_callbacks, {}, {}, {}, {}, d_75_native},
    {0x802ecae4u, true, SourceAbi::gobj_callback, {"src/tunnel/gate/code.c", "func_802ECAE4_5E9BB4", 28, 34}, "POKEMON_FUNC(func_802ECAE4_5E9BB4)", {}, d_76_graphs, {}, d_76_headers, {}, d_76_native},
    {0x802ecb3cu, true, SourceAbi::gobj_callback, {"src/tunnel/gate/code.c", "func_802ECB3C_5E9C0C", 36, 56}, "POKEMON_FUNC(func_802ECB3C_5E9C0C)", d_77_callbacks, {}, {}, d_77_headers, {}, d_77_native},
    {0x802ecc18u, true, SourceAbi::animation_model_event, {"src/tunnel/gate/code.c", "func_802ECC18_5E9CE8", 58, 62}, "void func_802ECC18_5E9CE8(DObj* arg0, s32 arg1, f32 arg2)", {}, {}, {}, {}, {}, {}},
    {0x802ecc44u, true, SourceAbi::gobj_callback, {"src/tunnel/gate/code.c", "func_802ECC44_5E9D14", 64, 83}, "POKEMON_FUNC(func_802ECC44_5E9D14)", {}, {}, {}, {}, {}, d_79_native},
    {0x802ecd84u, true, SourceAbi::gobj_callback, {"src/tunnel/gate/code.c", "func_802ECD84_5E9E54", 85, 98}, "POKEMON_FUNC(func_802ECD84_5E9E54)", {}, {}, {}, {}, {}, d_80_native},
    {0x802ece10u, true, SourceAbi::gobj_callback, {"src/tunnel/gate/code.c", "func_802ECE10_5E9EE0", 100, 142}, "POKEMON_FUNC(func_802ECE10_5E9EE0)", d_81_callbacks, {}, {}, {}, d_81_exact_abi_dependencies, d_81_native},
    {0x802ecfc8u, true, SourceAbi::factory_five_argument, {"src/tunnel/gate/code.c", "func_802ECFC8_5EA098", 144, 146}, "GObj* func_802ECFC8_5EA098(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)", {}, {}, {}, {}, {}, d_82_native},
    {0x802ebbe0u, true, SourceAbi::gobj_callback, {"src/tunnel/haunter/code.c", "func_802EBBE0_5E8CB0", 9, 11}, "POKEMON_FUNC(func_802EBBE0_5E8CB0)", d_83_callbacks, {}, {}, {}, {}, d_83_native},
    {0x802ebc04u, true, SourceAbi::gobj_callback, {"src/tunnel/haunter/code.c", "func_802EBC04_5E8CD4", 13, 20}, "POKEMON_FUNC(func_802EBC04_5E8CD4)", d_84_callbacks, {}, {}, d_84_headers, {}, d_84_native},
    {0x802ebc78u, true, SourceAbi::gobj_callback, {"src/tunnel/haunter/code.c", "func_802EBC78_5E8D48", 22, 29}, "POKEMON_FUNC(func_802EBC78_5E8D48)", {}, {}, {}, {}, {}, d_85_native},
    {0x802ebce8u, true, SourceAbi::factory_five_argument, {"src/tunnel/haunter/code.c", "func_802EBCE8_5E8DB8", 31, 33}, "GObj* func_802EBCE8_5E8DB8(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)", {}, {}, {}, {}, {}, d_86_native},
    {0x802e6cb0u, true, SourceAbi::gobj_callback, {"src/tunnel/kakuna/code.c", "kakuna_InitialState", 107, 121}, "POKEMON_FUNC(kakuna_InitialState)", d_87_callbacks, d_87_graphs, {}, {}, {}, d_87_native},
    {0x802e6d90u, true, SourceAbi::gobj_callback, {"src/tunnel/kakuna/code.c", "func_802E6D90_5E3E60", 123, 130}, "POKEMON_FUNC(func_802E6D90_5E3E60)", d_88_callbacks, d_88_graphs, {}, d_88_headers, {}, d_88_native},
    {0x802e6e0cu, true, SourceAbi::gobj_callback, {"src/tunnel/kakuna/code.c", "func_802E6E0C_5E3EDC", 132, 138}, "POKEMON_FUNC(func_802E6E0C_5E3EDC)", d_89_callbacks, d_89_graphs, {}, d_89_headers, {}, d_89_native},
    {0x802e6e7cu, true, SourceAbi::gobj_callback, {"src/tunnel/kakuna/code.c", "func_802E6E7C_5E3F4C", 140, 147}, "POKEMON_FUNC(func_802E6E7C_5E3F4C)", d_90_callbacks, {}, {}, d_90_headers, {}, d_90_native},
    {0x802e6ee8u, true, SourceAbi::gobj_callback, {"src/tunnel/kakuna/code.c", "func_802E6EE8_5E3FB8", 149, 155}, "POKEMON_FUNC(func_802E6EE8_5E3FB8)", {}, {}, {}, {}, {}, d_91_native},
    {0x802e6f48u, true, SourceAbi::gobj_callback, {"src/tunnel/kakuna/code.c", "func_802E6F48_5E4018", 157, 163}, "POKEMON_FUNC(func_802E6F48_5E4018)", {}, {}, {}, {}, {}, d_92_native},
    {0x802e6fa8u, true, SourceAbi::gobj_callback, {"src/tunnel/kakuna/code.c", "func_802E6FA8_5E4078", 165, 172}, "POKEMON_FUNC(func_802E6FA8_5E4078)", d_93_callbacks, {}, {}, d_93_headers, {}, d_93_native},
    {0x802e7014u, true, SourceAbi::gobj_callback, {"src/tunnel/kakuna/code.c", "func_802E7014_5E40E4", 174, 186}, "POKEMON_FUNC(func_802E7014_5E40E4)", {}, {}, {}, {}, {}, d_94_native},
    {0x802e70bcu, true, SourceAbi::gobj_callback, {"src/tunnel/kakuna/code.c", "func_802E70BC_5E418C", 188, 192}, "POKEMON_FUNC(func_802E70BC_5E418C)", {}, d_95_graphs, {}, {}, {}, d_95_native},
    {0x802e70fcu, true, SourceAbi::gobj_callback, {"src/tunnel/kakuna/code.c", "func_802E70FC_5E41CC", 194, 214}, "POKEMON_FUNC(func_802E70FC_5E41CC)", d_96_callbacks, {}, {}, {}, {}, d_96_native},
    {0x802e71d4u, true, SourceAbi::gobj_callback, {"src/tunnel/kakuna/code.c", "func_802E71D4_5E42A4", 216, 230}, "POKEMON_FUNC(func_802E71D4_5E42A4)", d_97_callbacks, d_97_graphs, {}, d_97_headers, {}, d_97_native},
    {0x802e729cu, true, SourceAbi::gobj_callback, {"src/tunnel/kakuna/code.c", "func_802E729C_5E436C", 232, 241}, "POKEMON_FUNC(func_802E729C_5E436C)", {}, {}, {}, {}, {}, d_98_native},
    {0x802e7324u, true, SourceAbi::gobj_callback, {"src/tunnel/kakuna/code.c", "func_802E7324_5E43F4", 243, 255}, "POKEMON_FUNC(func_802E7324_5E43F4)", d_99_callbacks, d_99_graphs, {}, d_99_headers, {}, d_99_native},
    {0x802e73d0u, true, SourceAbi::gobj_callback, {"src/tunnel/kakuna/code.c", "func_802E73D0_5E44A0", 257, 271}, "POKEMON_FUNC(func_802E73D0_5E44A0)", {}, {}, {}, {}, {}, d_100_native},
    {0x802e74a4u, true, SourceAbi::gobj_callback, {"src/tunnel/kakuna/code.c", "func_802E74A4_5E4574", 273, 280}, "POKEMON_FUNC(func_802E74A4_5E4574)", {}, {}, {}, {}, {}, d_101_native},
    {0x802e7504u, true, SourceAbi::gobj_callback, {"src/tunnel/kakuna/code.c", "func_802E7504_5E45D4", 282, 292}, "POKEMON_FUNC(func_802E7504_5E45D4)", d_102_callbacks, d_102_graphs, {}, {}, {}, d_102_native},
    {0x802e7594u, true, SourceAbi::gobj_callback, {"src/tunnel/kakuna/code.c", "func_802E7594_5E4664", 294, 301}, "POKEMON_FUNC(func_802E7594_5E4664)", {}, {}, {}, {}, {}, d_103_native},
    {0x802e75f4u, true, SourceAbi::gobj_callback, {"src/tunnel/kakuna/code.c", "func_802E75F4_5E46C4", 303, 306}, "POKEMON_FUNC(func_802E75F4_5E46C4)", {}, {}, {}, {}, {}, d_104_native},
    {0x802e7620u, true, SourceAbi::factory_five_argument, {"src/tunnel/kakuna/code.c", "kakuna_Spawn", 308, 310}, "GObj* kakuna_Spawn(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)", {}, {}, {}, {}, {}, d_105_native},
    {0x802ebd20u, true, SourceAbi::gobj_callback, {"src/tunnel/magikarp/code.c", "func_802EBD20_5E8DF0", 11, 17}, "POKEMON_FUNC(func_802EBD20_5E8DF0)", d_106_callbacks, {}, {}, {}, {}, d_106_native},
    {0x802ebd6cu, true, SourceAbi::gobj_callback, {"src/tunnel/magikarp/code.c", "func_802EBD6C_5E8E3C", 19, 25}, "POKEMON_FUNC(func_802EBD6C_5E8E3C)", {}, d_107_graphs, {}, {}, {}, d_107_native},
    {0x802ebdbcu, true, SourceAbi::gobj_callback, {"src/tunnel/magikarp/code.c", "func_802EBDBC_5E8E8C", 27, 32}, "POKEMON_FUNC(func_802EBDBC_5E8E8C)", d_108_callbacks, {}, {}, {}, {}, d_108_native},
    {0x802ebe0cu, true, SourceAbi::gobj_callback, {"src/tunnel/magikarp/code.c", "func_802EBE0C_5E8EDC", 34, 44}, "POKEMON_FUNC(func_802EBE0C_5E8EDC)", d_109_callbacks, {}, {}, d_109_headers, {}, d_109_native},
    {0x802ebec4u, true, SourceAbi::gobj_callback, {"src/tunnel/magikarp/code.c", "func_802EBEC4_5E8F94", 46, 52}, "POKEMON_FUNC(func_802EBEC4_5E8F94)", {}, {}, {}, {}, {}, d_110_native},
    {0x802ebf04u, true, SourceAbi::factory_five_argument, {"src/tunnel/magikarp/code.c", "func_802EBF04_5E8FD4", 54, 56}, "GObj* func_802EBF04_5E8FD4(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)", {}, {}, {}, {}, {}, d_111_native},
    {0x802e39a0u, true, SourceAbi::gobj_callback, {"src/tunnel/magnemite/code.c", "magnemite_ProcessZoomIn", 188, 195}, "POKEMON_FUNC(magnemite_ProcessZoomIn)", {}, {}, {}, {}, {}, d_112_native},
    {0x802e3a14u, true, SourceAbi::gobj_callback, {"src/tunnel/magnemite/code.c", "magnemite_InitialState", 197, 218}, "POKEMON_FUNC(magnemite_InitialState)", d_113_callbacks, {}, {}, {}, {}, d_113_native},
    {0x802e3ac8u, true, SourceAbi::gobj_callback, {"src/tunnel/magnemite/code.c", "magnemite_Idle", 220, 236}, "POKEMON_FUNC(magnemite_Idle)", {}, d_114_graphs, d_114_weighted_states, d_114_headers, {}, d_114_native},
    {0x802e3b8cu, true, SourceAbi::gobj_callback, {"src/tunnel/magnemite/code.c", "magnemite_HitByPesterBall", 238, 248}, "POKEMON_FUNC(magnemite_HitByPesterBall)", d_115_callbacks, d_115_graphs, {}, d_115_headers, {}, d_115_native},
    {0x802e3c18u, true, SourceAbi::gobj_callback, {"src/tunnel/magnemite/code.c", "magnemite_HitByApple", 250, 260}, "POKEMON_FUNC(magnemite_HitByApple)", d_116_callbacks, d_116_graphs, {}, d_116_headers, {}, d_116_native},
    {0x802e3ca4u, true, SourceAbi::gobj_callback, {"src/tunnel/magnemite/code.c", "magnemite_AffectedByPesterBall", 262, 264}, "POKEMON_FUNC(magnemite_AffectedByPesterBall)", d_117_callbacks, {}, {}, {}, {}, d_117_native},
    {0x802e3cc8u, true, SourceAbi::gobj_callback, {"src/tunnel/magnemite/code.c", "magnemite_AppleNearby", 266, 280}, "POKEMON_FUNC(magnemite_AppleNearby)", d_118_callbacks, d_118_graphs, {}, d_118_headers, {}, d_118_native},
    {0x802e3d80u, true, SourceAbi::gobj_callback, {"src/tunnel/magnemite/code.c", "magnemite_MoveToApple", 282, 295}, "POKEMON_FUNC(magnemite_MoveToApple)", {}, {}, {}, d_119_headers, {}, d_119_native},
    {0x802e3e20u, true, SourceAbi::gobj_callback, {"src/tunnel/magnemite/code.c", "magnemite_SearchApple", 297, 299}, "POKEMON_FUNC(magnemite_SearchApple)", d_120_callbacks, {}, {}, {}, {}, d_120_native},
    {0x802e3e44u, true, SourceAbi::gobj_callback, {"src/tunnel/magnemite/code.c", "magnemite_WithApple", 301, 315}, "POKEMON_FUNC(magnemite_WithApple)", d_121_callbacks, d_121_graphs, d_121_weighted_states, d_121_headers, {}, d_121_native},
    {0x802e3ee4u, true, SourceAbi::gobj_callback, {"src/tunnel/magnemite/code.c", "magnemite_RecoverFromHit", 317, 328}, "POKEMON_FUNC(magnemite_RecoverFromHit)", d_122_callbacks, d_122_graphs, {}, d_122_headers, {}, d_122_native},
    {0x802e3f8cu, true, SourceAbi::gobj_callback, {"src/tunnel/magnemite/code.c", "magnemite_TurnToPlayer", 330, 335}, "POKEMON_FUNC(magnemite_TurnToPlayer)", {}, {}, {}, {}, {}, d_123_native},
    {0x802e3fd8u, true, SourceAbi::gobj_callback, {"src/tunnel/magnemite/code.c", "magnemite_PlayerApproaching", 337, 361}, "POKEMON_FUNC(magnemite_PlayerApproaching)", d_124_callbacks, d_124_graphs, {}, d_124_headers, {}, d_124_native},
    {0x802e40d4u, true, SourceAbi::gobj_callback, {"src/tunnel/magnemite/code.c", "magnemite_RunAwayFromPlayer", 363, 369}, "POKEMON_FUNC(magnemite_RunAwayFromPlayer)", {}, {}, {}, {}, {}, d_125_native},
    {0x802e4124u, true, SourceAbi::gobj_callback, {"src/tunnel/magnemite/code.c", "magnemite_ZoomedIn", 371, 392}, "POKEMON_FUNC(magnemite_ZoomedIn)", d_126_callbacks, d_126_graphs, {}, d_126_headers, {}, d_126_native},
    {0x802e4234u, true, SourceAbi::gobj_callback, {"src/tunnel/magnemite/code.c", "magnemite_FindNeighbours", 394, 424}, "POKEMON_FUNC(magnemite_FindNeighbours)", {}, {}, {}, {}, {}, d_127_native},
    {0x802e43b0u, true, SourceAbi::gobj_vector_utility, {"src/tunnel/magnemite/code.c", "magnemite_CalcTargetPos", 426, 437}, "void magnemite_CalcTargetPos(GObj* obj, Vec3f* targetPos)", {}, {}, {}, {}, {}, d_128_native},
    {0x802e4434u, true, SourceAbi::gobj_callback, {"src/tunnel/magnemite/code.c", "magnemite_Attract", 439, 453}, "POKEMON_FUNC(magnemite_Attract)", d_129_callbacks, {}, {}, d_129_headers, {}, d_129_native},
    {0x802e44dcu, true, SourceAbi::gobj_callback, {"src/tunnel/magnemite/code.c", "magnemite_MoveToAttractedMagnemite", 455, 470}, "POKEMON_FUNC(magnemite_MoveToAttractedMagnemite)", {}, {}, {}, {}, d_130_exact_abi_dependencies, d_130_native},
    {0x802e45b4u, true, SourceAbi::gobj_callback, {"src/tunnel/magnemite/code.c", "magnemite_WaitForLeader", 479, 488}, "POKEMON_FUNC(magnemite_WaitForLeader)", d_131_callbacks, d_131_graphs, {}, {}, {}, d_131_native},
    {0x802e4668u, true, SourceAbi::gobj_callback, {"src/tunnel/magnemite/code.c", "magnemite_CopyLeaderBehavior", 490, 502}, "POKEMON_FUNC(magnemite_CopyLeaderBehavior)", d_132_callbacks, d_132_graphs, {}, {}, {}, d_132_native},
    {0x802e4710u, true, SourceAbi::gobj_callback, {"src/tunnel/magnemite/code.c", "magnemite_CopyLeaderMovement", 504, 521}, "POKEMON_FUNC(magnemite_CopyLeaderMovement)", {}, {}, {}, {}, d_133_exact_abi_dependencies, d_133_native},
    {0x802e480cu, true, SourceAbi::gobj_callback, {"src/tunnel/magnemite/code.c", "magnemite_HitByPesterBall_SendToLeader", 523, 526}, "POKEMON_FUNC(magnemite_HitByPesterBall_SendToLeader)", {}, {}, {}, {}, {}, d_134_native},
    {0x802e4844u, true, SourceAbi::gobj_callback, {"src/tunnel/magnemite/code.c", "magnemite_HitByApple_SendToLeader", 528, 531}, "POKEMON_FUNC(magnemite_HitByApple_SendToLeader)", {}, {}, {}, {}, {}, d_135_native},
    {0x802e487cu, true, SourceAbi::gobj_callback, {"src/tunnel/magnemite/code.c", "magnemite_AllMagnemitesCombined", 533, 548}, "POKEMON_FUNC(magnemite_AllMagnemitesCombined)", d_136_callbacks, {}, {}, d_136_headers, {}, d_136_native},
    {0x802e4964u, true, SourceAbi::gobj_callback, {"src/tunnel/magnemite/code.c", "magnemite_EvolveIntoMagneton", 550, 554}, "POKEMON_FUNC(magnemite_EvolveIntoMagneton)", d_137_callbacks, {}, {}, {}, {}, d_137_native},
    {0x802e49a4u, true, SourceAbi::gobj_callback, {"src/tunnel/magnemite/code.c", "magnemite_WaitToDelete", 556, 570}, "POKEMON_FUNC(magnemite_WaitToDelete)", d_138_callbacks, {}, {}, d_138_headers, {}, d_138_native},
    {0x802e4a84u, true, SourceAbi::gobj_callback, {"src/tunnel/magnemite/code.c", "magnemite_Delete", 572, 586}, "POKEMON_FUNC(magnemite_Delete)", {}, {}, {}, {}, {}, d_139_native},
    {0x802e4afcu, true, SourceAbi::factory_five_argument, {"src/tunnel/magnemite/code.c", "magnemite_Spawn", 612, 614}, "GObj* magnemite_Spawn(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)", {}, {}, {}, {}, {}, d_140_native},
    {0x802e4b40u, true, SourceAbi::gobj_callback, {"src/tunnel/magneton/code.c", "magneton_InitialState", 82, 84}, "POKEMON_FUNC(magneton_InitialState)", d_141_callbacks, {}, {}, {}, {}, d_141_native},
    {0x802e4b64u, true, SourceAbi::gobj_callback, {"src/tunnel/magneton/code.c", "magneton_Idle", 86, 93}, "POKEMON_FUNC(magneton_Idle)", {}, d_142_graphs, d_142_weighted_states, d_142_headers, {}, d_142_native},
    {0x802e4bd4u, true, SourceAbi::gobj_callback, {"src/tunnel/magneton/code.c", "magneton_Roam1", 95, 106}, "POKEMON_FUNC(magneton_Roam1)", d_143_callbacks, d_143_graphs, d_143_weighted_states, d_143_headers, {}, d_143_native},
    {0x802e4c6cu, true, SourceAbi::gobj_callback, {"src/tunnel/magneton/code.c", "magneton_MoveRoam1", 108, 114}, "POKEMON_FUNC(magneton_MoveRoam1)", {}, {}, {}, {}, {}, d_144_native},
    {0x802e4cbcu, true, SourceAbi::gobj_callback, {"src/tunnel/magneton/code.c", "magneton_Roam2", 116, 127}, "POKEMON_FUNC(magneton_Roam2)", d_145_callbacks, d_145_graphs, d_145_weighted_states, d_145_headers, {}, d_145_native},
    {0x802e4d54u, true, SourceAbi::gobj_callback, {"src/tunnel/magneton/code.c", "magneton_MoveRoam2", 129, 135}, "POKEMON_FUNC(magneton_MoveRoam2)", {}, {}, {}, {}, {}, d_146_native},
    {0x802e4da4u, true, SourceAbi::gobj_callback, {"src/tunnel/magneton/code.c", "magneton_AppleNearby", 137, 147}, "POKEMON_FUNC(magneton_AppleNearby)", d_147_callbacks, d_147_graphs, {}, {}, {}, d_147_native},
    {0x802e4e2cu, true, SourceAbi::gobj_callback, {"src/tunnel/magneton/code.c", "magneton_MoveToApple", 149, 156}, "POKEMON_FUNC(magneton_MoveToApple)", {}, {}, {}, {}, {}, d_148_native},
    {0x802e4ea4u, true, SourceAbi::gobj_callback, {"src/tunnel/magneton/code.c", "magneton_SearchApple", 158, 160}, "POKEMON_FUNC(magneton_SearchApple)", d_149_callbacks, {}, {}, {}, {}, d_149_native},
    {0x802e4ec8u, true, SourceAbi::gobj_callback, {"src/tunnel/magneton/code.c", "magneton_WithApple", 162, 172}, "POKEMON_FUNC(magneton_WithApple)", d_150_callbacks, d_150_graphs, d_150_weighted_states, d_150_headers, {}, d_150_native},
    {0x802e4f4cu, true, SourceAbi::gobj_callback, {"src/tunnel/magneton/code.c", "magneton_RunAway", 174, 188}, "POKEMON_FUNC(magneton_RunAway)", d_151_callbacks, d_151_graphs, {}, d_151_headers, {}, d_151_native},
    {0x802e5008u, true, SourceAbi::gobj_callback, {"src/tunnel/magneton/code.c", "magneton_MoveRunAway", 190, 196}, "POKEMON_FUNC(magneton_MoveRunAway)", {}, {}, {}, {}, {}, d_152_native},
    {0x802e505cu, true, SourceAbi::factory_five_argument, {"src/tunnel/magneton/code.c", "magneton_Spawn", 198, 200}, "GObj* magneton_Spawn(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)", {}, {}, {}, {}, {}, d_153_native},
    {0x802e5094u, true, SourceAbi::gobj_callback, {"src/tunnel/magneton/code.c", "tunnel_SpawnMagneton", 202, 213}, "void tunnel_SpawnMagneton(GObj* obj)", {}, {}, {}, {}, d_154_exact_abi_dependencies, d_154_native},
    {0x802e2500u, true, SourceAbi::gobj_callback, {"src/tunnel/main.c", "tunnel_SpawnMagikarp", 208, 236}, "void tunnel_SpawnMagikarp(GObj* obj)", {}, {}, {}, {}, {}, d_155_native},
    {0x802e25d4u, true, SourceAbi::item_ground_callback, {"src/tunnel/main.c", "tunnel_HandleCollision", 238, 264}, "void tunnel_HandleCollision(GObj* obj, GroundResult* groundResult)", {}, {}, {}, {}, {}, d_156_native},
    {0x802e7680u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E7680_5E4750", 223, 234}, "POKEMON_FUNC(func_802E7680_5E4750)", d_157_callbacks, {}, {}, {}, {}, d_157_native},
    {0x802e7718u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E7718_5E47E8", 236, 246}, "POKEMON_FUNC(func_802E7718_5E47E8)", d_158_callbacks, {}, {}, {}, {}, d_158_native},
    {0x802e77b0u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E77B0_5E4880", 248, 256}, "POKEMON_FUNC(func_802E77B0_5E4880)", {}, d_159_graphs, d_159_weighted_states, d_159_headers, {}, d_159_native},
    {0x802e782cu, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E782C_5E48FC", 258, 265}, "POKEMON_FUNC(func_802E782C_5E48FC)", {}, d_160_graphs, d_160_weighted_states, d_160_headers, {}, d_160_native},
    {0x802e789cu, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E789C_5E496C", 267, 280}, "POKEMON_FUNC(func_802E789C_5E496C)", d_161_callbacks, {}, {}, d_161_headers, {}, d_161_native},
    {0x802e7948u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E7948_5E4A18", 282, 284}, "POKEMON_FUNC(func_802E7948_5E4A18)", d_162_callbacks, {}, {}, {}, {}, d_162_native},
    {0x802e796cu, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E796C_5E4A3C", 286, 322}, "POKEMON_FUNC(func_802E796C_5E4A3C)", d_163_callbacks, {}, {}, d_163_headers, {}, d_163_native},
    {0x802e7ac8u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E7AC8_5E4B98", 324, 332}, "POKEMON_FUNC(func_802E7AC8_5E4B98)", {}, {}, {}, {}, {}, d_164_native},
    {0x802e7b3cu, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E7B3C_5E4C0C", 334, 349}, "POKEMON_FUNC(func_802E7B3C_5E4C0C)", d_165_callbacks, {}, {}, {}, {}, d_165_native},
    {0x802e7c1cu, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E7C1C_5E4CEC", 351, 362}, "POKEMON_FUNC(func_802E7C1C_5E4CEC)", d_166_callbacks, {}, {}, d_166_headers, {}, d_166_native},
    {0x802e7ca4u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E7CA4_5E4D74", 364, 371}, "POKEMON_FUNC(func_802E7CA4_5E4D74)", {}, {}, {}, {}, {}, d_167_native},
    {0x802e7d04u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E7D04_5E4DD4", 373, 393}, "POKEMON_FUNC(func_802E7D04_5E4DD4)", d_168_callbacks, {}, {}, d_168_headers, {}, d_168_native},
    {0x802e7ddcu, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E7DDC_5E4EAC", 395, 403}, "POKEMON_FUNC(func_802E7DDC_5E4EAC)", {}, {}, {}, {}, {}, d_169_native},
    {0x802e7e5cu, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E7E5C_5E4F2C", 405, 408}, "POKEMON_FUNC(func_802E7E5C_5E4F2C)", d_170_callbacks, {}, {}, {}, {}, d_170_native},
    {0x802e7e94u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E7E94_5E4F64", 410, 428}, "POKEMON_FUNC(func_802E7E94_5E4F64)", d_171_callbacks, {}, {}, d_171_headers, {}, d_171_native},
    {0x802e7f64u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E7F64_5E5034", 430, 444}, "POKEMON_FUNC(func_802E7F64_5E5034)", {}, {}, {}, {}, {}, d_172_native},
    {0x802e8008u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E8008_5E50D8", 446, 451}, "POKEMON_FUNC(func_802E8008_5E50D8)", d_173_callbacks, {}, {}, {}, {}, d_173_native},
    {0x802e8058u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E8058_5E5128", 467, 474}, "POKEMON_FUNC(func_802E8058_5E5128)", d_174_callbacks, d_174_graphs, {}, d_174_headers, {}, d_174_native},
    {0x802e80ccu, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E80CC_5E519C", 476, 488}, "POKEMON_FUNC(func_802E80CC_5E519C)", {}, {}, {}, {}, {}, d_175_native},
    {0x802e818cu, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E818C_5E525C", 490, 502}, "POKEMON_FUNC(func_802E818C_5E525C)", {}, {}, {}, d_176_headers, {}, d_176_native},
    {0x802e823cu, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E823C_5E530C", 504, 510}, "POKEMON_FUNC(func_802E823C_5E530C)", d_177_callbacks, {}, {}, d_177_headers, {}, d_177_native},
    {0x802e8290u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E8290_5E5360", 512, 526}, "POKEMON_FUNC(func_802E8290_5E5360)", d_178_callbacks, {}, {}, {}, {}, d_178_native},
    {0x802e8330u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E8330_5E5400", 535, 542}, "POKEMON_FUNC(func_802E8330_5E5400)", d_179_callbacks, d_179_graphs, {}, d_179_headers, {}, d_179_native},
    {0x802e8398u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E8398_5E5468", 544, 552}, "POKEMON_FUNC(func_802E8398_5E5468)", {}, {}, {}, {}, {}, d_180_native},
    {0x802e8428u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E8428_5E54F8", 554, 562}, "POKEMON_FUNC(func_802E8428_5E54F8)", d_181_callbacks, {}, {}, d_181_headers, {}, d_181_native},
    {0x802e849cu, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E849C_5E556C", 564, 578}, "POKEMON_FUNC(func_802E849C_5E556C)", {}, {}, {}, {}, {}, d_182_native},
    {0x802e853cu, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E853C_5E560C", 580, 589}, "POKEMON_FUNC(func_802E853C_5E560C)", d_183_callbacks, {}, {}, {}, {}, d_183_native},
    {0x802e85bcu, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E85BC_5E568C", 599, 605}, "POKEMON_FUNC(func_802E85BC_5E568C)", {}, d_184_graphs, {}, d_184_headers, {}, d_184_native},
    {0x802e8628u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E8628_5E56F8", 607, 613}, "POKEMON_FUNC(func_802E8628_5E56F8)", d_185_callbacks, {}, {}, d_185_headers, {}, d_185_native},
    {0x802e867cu, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E867C_5E574C", 615, 619}, "POKEMON_FUNC(func_802E867C_5E574C)", d_186_callbacks, {}, {}, {}, {}, d_186_native},
    {0x802e86c4u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E86C4_5E5794", 628, 639}, "POKEMON_FUNC(func_802E86C4_5E5794)", d_187_callbacks, d_187_graphs, {}, d_187_headers, {}, d_187_native},
    {0x802e8754u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E8754_5E5824", 641, 650}, "POKEMON_FUNC(func_802E8754_5E5824)", {}, {}, {}, {}, {}, d_188_native},
    {0x802e87f4u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E87F4_5E58C4", 652, 660}, "POKEMON_FUNC(func_802E87F4_5E58C4)", d_189_callbacks, {}, {}, d_189_headers, {}, d_189_native},
    {0x802e8868u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E8868_5E5938", 669, 677}, "POKEMON_FUNC(func_802E8868_5E5938)", d_190_callbacks, d_190_graphs, {}, d_190_headers, {}, d_190_native},
    {0x802e88ecu, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E88EC_5E59BC", 679, 687}, "POKEMON_FUNC(func_802E88EC_5E59BC)", {}, {}, {}, {}, {}, d_191_native},
    {0x802e8990u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E8990_5E5A60", 689, 697}, "POKEMON_FUNC(func_802E8990_5E5A60)", {}, {}, {}, {}, {}, d_192_native},
    {0x802e8a24u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E8A24_5E5AF4", 699, 709}, "POKEMON_FUNC(func_802E8A24_5E5AF4)", d_193_callbacks, {}, {}, d_193_headers, {}, d_193_native},
    {0x802e8aa4u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E8AA4_5E5B74", 723, 731}, "POKEMON_FUNC(func_802E8AA4_5E5B74)", d_194_callbacks, {}, {}, d_194_headers, {}, d_194_native},
    {0x802e8b1cu, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E8B1C_5E5BEC", 733, 758}, "POKEMON_FUNC(func_802E8B1C_5E5BEC)", {}, {}, {}, {}, {}, d_195_native},
    {0x802e8c94u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E8C94_5E5D64", 760, 770}, "POKEMON_FUNC(func_802E8C94_5E5D64)", d_196_callbacks, d_196_graphs, {}, d_196_headers, {}, d_196_native},
    {0x802e8d38u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E8D38_5E5E08", 772, 777}, "POKEMON_FUNC(func_802E8D38_5E5E08)", {}, {}, {}, {}, {}, d_197_native},
    {0x802e8d84u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E8D84_5E5E54", 779, 789}, "POKEMON_FUNC(func_802E8D84_5E5E54)", {}, {}, {}, {}, {}, d_198_native},
    {0x802e8e34u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E8E34_5E5F04", 791, 799}, "POKEMON_FUNC(func_802E8E34_5E5F04)", d_199_callbacks, {}, {}, d_199_headers, {}, d_199_native},
    {0x802e8eacu, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E8EAC_5E5F7C", 801, 814}, "POKEMON_FUNC(func_802E8EAC_5E5F7C)", d_200_callbacks, {}, {}, d_200_headers, {}, d_200_native},
    {0x802e8f44u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E8F44_5E6014", 816, 839}, "POKEMON_FUNC(func_802E8F44_5E6014)", {}, {}, {}, {}, {}, d_201_native},
    {0x802e9098u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E9098_5E6168", 841, 845}, "POKEMON_FUNC(func_802E9098_5E6168)", {}, {}, {}, {}, {}, d_202_native},
    {0x802e90dcu, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E90DC_5E61AC", 847, 854}, "POKEMON_FUNC(func_802E90DC_5E61AC)", d_203_callbacks, d_203_graphs, {}, d_203_headers, {}, d_203_native},
    {0x802e9150u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E9150_5E6220", 856, 863}, "POKEMON_FUNC(func_802E9150_5E6220)", {}, {}, {}, {}, {}, d_204_native},
    {0x802e91a8u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E91A8_5E6278", 865, 871}, "POKEMON_FUNC(func_802E91A8_5E6278)", d_205_callbacks, {}, {}, d_205_headers, {}, d_205_native},
    {0x802e91fcu, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E91FC_5E62CC", 873, 883}, "POKEMON_FUNC(func_802E91FC_5E62CC)", d_206_callbacks, {}, {}, d_206_headers, {}, d_206_native},
    {0x802e9288u, true, SourceAbi::gobj_callback, {"src/tunnel/pikachu/code.c", "func_802E9288_5E6358", 885, 904}, "POKEMON_FUNC(func_802E9288_5E6358)", {}, {}, {}, {}, {}, d_207_native},
    {0x802e9390u, true, SourceAbi::factory_five_argument, {"src/tunnel/pikachu/code.c", "func_802E9390_5E6460", 930, 932}, "GObj* func_802E9390_5E6460(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)", {}, {}, {}, {}, {}, d_208_native},
    {0x802ea490u, true, SourceAbi::gobj_callback, {"src/tunnel/pkmn_1008/code.c", "func_802EA490_5E7560", 12, 26}, "POKEMON_FUNC(func_802EA490_5E7560)", {}, {}, {}, {}, {}, d_209_native},
    {0x802ea53cu, true, SourceAbi::gobj_callback, {"src/tunnel/pkmn_1008/code.c", "func_802EA53C_5E760C", 28, 35}, "POKEMON_FUNC(func_802EA53C_5E760C)", {}, d_210_graphs, {}, {}, {}, d_210_native},
    {0x802ea5d0u, true, SourceAbi::gobj_callback, {"src/tunnel/pkmn_1008/code.c", "func_802EA5D0_5E76A0", 37, 48}, "POKEMON_FUNC(func_802EA5D0_5E76A0)", d_211_callbacks, d_211_graphs, {}, {}, {}, d_211_native},
    {0x802ea694u, true, SourceAbi::gobj_callback, {"src/tunnel/pkmn_1008/code.c", "func_802EA694_5E7764", 50, 56}, "POKEMON_FUNC(func_802EA694_5E7764)", {}, d_212_graphs, {}, d_212_headers, {}, d_212_native},
    {0x802ea6f8u, true, SourceAbi::factory_five_argument, {"src/tunnel/pkmn_1008/code.c", "func_802EA6F8_5E77C8", 58, 60}, "GObj* func_802EA6F8_5E77C8(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)", {}, {}, {}, {}, {}, d_213_native},
    {0x802ea730u, true, SourceAbi::gobj_callback, {"src/tunnel/pkmn_1009/code.c", "func_802EA730_5E7800", 13, 22}, "POKEMON_FUNC(func_802EA730_5E7800)", d_214_callbacks, {}, {}, {}, {}, d_214_native},
    {0x802ea79cu, true, SourceAbi::gobj_callback, {"src/tunnel/pkmn_1009/code.c", "func_802EA79C_5E786C", 24, 68}, "POKEMON_FUNC(func_802EA79C_5E786C)", {}, {}, {}, d_215_headers, {}, d_215_native},
    {0x802ea970u, true, SourceAbi::gobj_callback, {"src/tunnel/pkmn_1009/code.c", "func_802EA970_5E7A40", 70, 114}, "POKEMON_FUNC(func_802EA970_5E7A40)", {}, {}, {}, d_216_headers, {}, d_216_native},
    {0x802eab44u, true, SourceAbi::collision_node, {"src/tunnel/pkmn_1009/code.c", "func_802EAB44_5E7C14", 116, 156}, "s32 func_802EAB44_5E7C14(GObj* obj, DObj* node, Vec3f* inPos, Vec3f* inVel, Vec3f* outPos, Vec3f* outVel)", {}, {}, {}, {}, {}, d_217_native},
    {0x802eadacu, true, SourceAbi::collision_object, {"src/tunnel/pkmn_1009/code.c", "func_802EADAC_5E7E7C", 158, 182}, "s32 func_802EADAC_5E7E7C(GObj* obj, Vec3f* inPos, Vec3f* inVel, Vec3f* outPos, Vec3f* outVel)", {}, {}, {}, {}, d_218_exact_abi_dependencies, {}},
    {0x802eae7cu, true, SourceAbi::collision_vectors, {"src/tunnel/pkmn_1009/code.c", "func_802EAE7C_5E7F4C", 184, 202}, "s32 func_802EAE7C_5E7F4C(Vec3f* inPos, Vec3f* inVel, Vec3f* outPos, Vec3f* outVel)", {}, {}, {}, {}, d_219_exact_abi_dependencies, {}},
    {0x802eaf18u, true, SourceAbi::factory_five_argument, {"src/tunnel/pkmn_1009/code.c", "func_802EAF18_5E7FE8", 204, 222}, "GObj* func_802EAF18_5E7FE8(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)", {}, {}, {}, {}, d_220_exact_abi_dependencies, d_220_native},
    {0x802eafb0u, true, SourceAbi::gobj_callback, {"src/tunnel/pkmn_1010/code.c", "func_802EAFB0_5E8080", 12, 18}, "POKEMON_FUNC(func_802EAFB0_5E8080)", d_221_callbacks, {}, {}, {}, {}, d_221_native},
    {0x802eb014u, true, SourceAbi::gobj_callback, {"src/tunnel/pkmn_1010/code.c", "func_802EB014_5E80E4", 20, 35}, "POKEMON_FUNC(func_802EB014_5E80E4)", d_222_callbacks, {}, {}, {}, {}, d_222_native},
    {0x802eb0ccu, true, SourceAbi::gobj_callback, {"src/tunnel/pkmn_1010/code.c", "func_802EB0CC_5E819C", 37, 66}, "POKEMON_FUNC(func_802EB0CC_5E819C)", d_223_callbacks, d_223_graphs, {}, d_223_headers, {}, d_223_native},
    {0x802eb28cu, true, SourceAbi::gobj_callback, {"src/tunnel/pkmn_1010/code.c", "func_802EB28C_5E835C", 68, 84}, "POKEMON_FUNC(func_802EB28C_5E835C)", {}, {}, {}, {}, {}, d_224_native},
    {0x802eb35cu, true, SourceAbi::gobj_callback, {"src/tunnel/pkmn_1010/code.c", "func_802EB35C_5E842C", 86, 90}, "POKEMON_FUNC(func_802EB35C_5E842C)", {}, {}, {}, d_225_headers, {}, d_225_native},
    {0x802eb3a8u, true, SourceAbi::factory_five_argument, {"src/tunnel/pkmn_1010/code.c", "func_802EB3A8_5E8478", 92, 94}, "GObj* func_802EB3A8_5E8478(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)", {}, {}, {}, {}, {}, d_226_native},
    {0x802ec400u, true, SourceAbi::gobj_callback, {"src/tunnel/pkmn_1012/code.c", "func_802EC400_5E94D0", 11, 24}, "POKEMON_FUNC(func_802EC400_5E94D0)", d_227_callbacks, d_227_graphs, {}, d_227_headers, {}, d_227_native},
    {0x802ec4e8u, true, SourceAbi::gobj_callback, {"src/tunnel/pkmn_1012/code.c", "func_802EC4E8_5E95B8", 26, 38}, "POKEMON_FUNC(func_802EC4E8_5E95B8)", d_228_callbacks, {}, {}, d_228_headers, {}, d_228_native},
    {0x802ec5b0u, true, SourceAbi::gobj_callback, {"src/tunnel/pkmn_1012/code.c", "func_802EC5B0_5E9680", 40, 61}, "POKEMON_FUNC(func_802EC5B0_5E9680)", d_229_callbacks, {}, {}, d_229_headers, {}, d_229_native},
    {0x802ec6ccu, true, SourceAbi::gobj_callback, {"src/tunnel/pkmn_1012/code.c", "func_802EC6CC_5E979C", 63, 68}, "POKEMON_FUNC(func_802EC6CC_5E979C)", d_230_callbacks, {}, {}, d_230_headers, {}, d_230_native},
    {0x802ec720u, true, SourceAbi::factory_five_argument, {"src/tunnel/pkmn_1012/code.c", "func_802EC720_5E97F0", 70, 74}, "GObj* func_802EC720_5E97F0(s32 gObjID, u16 id, WorldBlock* roomA, WorldBlock* roomB, ObjectSpawn* spawn)", {}, {}, {}, {}, {}, d_231_native},
    {0x802ec760u, true, SourceAbi::gobj_callback, {"src/tunnel/pkmn_1013/code.c", "func_802EC760_5E9830", 13, 26}, "POKEMON_FUNC(func_802EC760_5E9830)", d_232_callbacks, d_232_graphs, {}, d_232_headers, {}, d_232_native},
    {0x802ec848u, true, SourceAbi::gobj_callback, {"src/tunnel/pkmn_1013/code.c", "func_802EC848_5E9918", 28, 40}, "POKEMON_FUNC(func_802EC848_5E9918)", d_233_callbacks, {}, {}, d_233_headers, {}, d_233_native},
    {0x802ec910u, true, SourceAbi::gobj_callback, {"src/tunnel/pkmn_1013/code.c", "func_802EC910_5E99E0", 42, 63}, "POKEMON_FUNC(func_802EC910_5E99E0)", d_234_callbacks, {}, {}, d_234_headers, {}, d_234_native},
    {0x802eca2cu, true, SourceAbi::gobj_callback, {"src/tunnel/pkmn_1013/code.c", "func_802ECA2C_5E9AFC", 65, 70}, "POKEMON_FUNC(func_802ECA2C_5E9AFC)", d_235_callbacks, {}, {}, d_235_headers, {}, d_235_native},
    {0x802eca80u, true, SourceAbi::factory_five_argument, {"src/tunnel/pkmn_1013/code.c", "func_802ECA80_5E9B50", 72, 76}, "GObj* func_802ECA80_5E9B50(s32 gObjID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)", {}, {}, {}, {}, {}, d_236_native},
    {0x802ed620u, true, SourceAbi::gobj_callback, {"src/tunnel/pkmn_1014/code.c", "func_802ED620_5EA6F0", 29, 31}, "POKEMON_FUNC(func_802ED620_5EA6F0)", d_237_callbacks, {}, {}, {}, {}, d_237_native},
    {0x802ed644u, true, SourceAbi::gobj_callback, {"src/tunnel/pkmn_1014/code.c", "func_802ED644_5EA714", 33, 38}, "POKEMON_FUNC(func_802ED644_5EA714)", {}, d_238_graphs, {}, d_238_headers, {}, d_238_native},
    {0x802ed69cu, true, SourceAbi::float_utility, {"src/tunnel/pkmn_1014/code.c", "func_802ED69C_5EA76C", 40, 45}, "void func_802ED69C_5EA76C(f32 arg0)", {}, {}, {}, {}, {}, d_239_native},
    {0x802ed6d0u, true, SourceAbi::float_utility, {"src/tunnel/pkmn_1014/code.c", "func_802ED6D0_5EA7A0", 47, 52}, "void func_802ED6D0_5EA7A0(f32 arg0)", {}, {}, {}, {}, {}, d_240_native},
    {0x802ed720u, true, SourceAbi::animation_model_event, {"src/tunnel/pkmn_1014/code.c", "func_802ED720_5EA7F0", 54, 74}, "void func_802ED720_5EA7F0(DObj* arg0, s32 arg1, f32 arg2)", {}, {}, {}, {}, {}, d_241_native},
    {0x802ed7bcu, true, SourceAbi::animation_model_event, {"src/tunnel/pkmn_1014/code.c", "func_802ED7BC_5EA88C", 76, 79}, "void func_802ED7BC_5EA88C(DObj* arg0, s32 arg1, f32 arg2)", {}, {}, {}, {}, {}, {}},
    {0x802ed7ccu, true, SourceAbi::gobj_callback, {"src/tunnel/pkmn_1014/code.c", "func_802ED7CC_5EA89C", 81, 131}, "void func_802ED7CC_5EA89C(GObj* obj)", {}, {}, {}, {}, {}, d_243_native},
    {0x802edae8u, true, SourceAbi::no_argument_utility, {"src/tunnel/pkmn_1014/code.c", "func_802EDAE8_5EABB8", 133, 136}, "void func_802EDAE8_5EABB8(void)", d_244_callbacks, {}, {}, {}, {}, d_244_native},
    {0x802edb3cu, true, SourceAbi::gobj_callback, {"src/tunnel/pkmn_1014/code.c", "func_802EDB3C_5EAC0C", 138, 208}, "void func_802EDB3C_5EAC0C(GObj* obj)", {}, {}, {}, {}, d_245_exact_abi_dependencies, d_245_native},
    {0x802edec0u, true, SourceAbi::factory_five_argument, {"src/tunnel/pkmn_1014/code.c", "func_802EDEC0_5EAF90", 210, 214}, "GObj* func_802EDEC0_5EAF90(s32 gObjID, u16 id, WorldBlock* roomA, WorldBlock* roomB, ObjectSpawn* spawn)", {}, {}, {}, {}, {}, d_246_native},
    {0x802e2c70u, true, SourceAbi::movement_callback, {"src/tunnel/sounds.c", "tunnel_UpdateSounds", 17, 201}, "void tunnel_UpdateSounds(s32 blockCount, f32 blockPart)", {}, {}, {}, {}, {}, d_247_native},
    {0x8011cb50u, true, SourceAbi::old_style_no_argument, {"src/tunnel/world/world.c", "tunnel_stub", 574, 575}, "void tunnel_stub()", {}, {}, {}, {}, {}, {}},
    {0x802eb3e0u, true, SourceAbi::gobj_callback, {"src/tunnel/zapdos/code.c", "func_802EB3E0_5E84B0", 17, 22}, "POKEMON_FUNC(func_802EB3E0_5E84B0)", {}, d_249_graphs, {}, {}, {}, d_249_native},
    {0x802eb430u, true, SourceAbi::gobj_callback, {"src/tunnel/zapdos/code.c", "func_802EB430_5E8500", 24, 46}, "POKEMON_FUNC(func_802EB430_5E8500)", d_250_callbacks, d_250_graphs, {}, d_250_headers, {}, d_250_native},
    {0x802eb548u, true, SourceAbi::gobj_callback, {"src/tunnel/zapdos/code.c", "func_802EB548_5E8618", 48, 67}, "POKEMON_FUNC(func_802EB548_5E8618)", {}, {}, {}, {}, {}, d_251_native},
    {0x802eb664u, true, SourceAbi::gobj_callback, {"src/tunnel/zapdos/code.c", "func_802EB664_5E8734", 69, 75}, "POKEMON_FUNC(func_802EB664_5E8734)", {}, {}, {}, {}, {}, d_252_native},
    {0x802eb6d0u, true, SourceAbi::gobj_callback, {"src/tunnel/zapdos/code.c", "func_802EB6D0_5E87A0", 77, 85}, "POKEMON_FUNC(func_802EB6D0_5E87A0)", {}, {}, {}, {}, {}, d_253_native},
    {0x802eb750u, true, SourceAbi::gobj_callback, {"src/tunnel/zapdos/code.c", "func_802EB750_5E8820", 87, 93}, "POKEMON_FUNC(func_802EB750_5E8820)", d_254_callbacks, d_254_graphs, {}, d_254_headers, {}, d_254_native},
    {0x802eb7acu, true, SourceAbi::gobj_callback, {"src/tunnel/zapdos/code.c", "func_802EB7AC_5E887C", 95, 98}, "POKEMON_FUNC(func_802EB7AC_5E887C)", d_255_callbacks, {}, {}, {}, {}, d_255_native},
    {0x802eb7e4u, true, SourceAbi::gobj_callback, {"src/tunnel/zapdos/code.c", "func_802EB7E4_5E88B4", 100, 104}, "POKEMON_FUNC(func_802EB7E4_5E88B4)", {}, {}, {}, {}, {}, d_256_native},
    {0x802eb818u, true, SourceAbi::factory_five_argument, {"src/tunnel/zapdos/code.c", "func_802EB818_5E88E8", 106, 109}, "GObj* func_802EB818_5E88E8(s32 gObjID, u16 id, WorldBlock* roomA, WorldBlock* roomB, ObjectSpawn* spawn)", {}, {}, {}, {}, {}, d_257_native},
    {0x802ebf40u, true, SourceAbi::gobj_callback, {"src/tunnel/zapdos_egg/code.c", "func_802EBF40_5E9010", 25, 36}, "POKEMON_FUNC(func_802EBF40_5E9010)", d_258_callbacks, d_258_graphs, {}, {}, {}, d_258_native},
    {0x802ebfdcu, true, SourceAbi::gobj_callback, {"src/tunnel/zapdos_egg/code.c", "func_802EBFDC_5E90AC", 38, 44}, "POKEMON_FUNC(func_802EBFDC_5E90AC)", {}, d_259_graphs, {}, d_259_headers, {}, d_259_native},
    {0x802ec040u, true, SourceAbi::gobj_callback, {"src/tunnel/zapdos_egg/code.c", "func_802EC040_5E9110", 46, 49}, "POKEMON_FUNC(func_802EC040_5E9110)", d_260_callbacks, {}, {}, d_260_headers, {}, d_260_native},
    {0x802ec078u, true, SourceAbi::gobj_callback, {"src/tunnel/zapdos_egg/code.c", "func_802EC078_5E9148", 51, 72}, "POKEMON_FUNC(func_802EC078_5E9148)", d_261_callbacks, {}, {}, d_261_headers, {}, d_261_native},
    {0x802ec178u, true, SourceAbi::gobj_callback, {"src/tunnel/zapdos_egg/code.c", "func_802EC178_5E9248", 74, 86}, "POKEMON_FUNC(func_802EC178_5E9248)", {}, {}, {}, {}, {}, d_262_native},
    {0x802ec23cu, true, SourceAbi::gobj_callback, {"src/tunnel/zapdos_egg/code.c", "func_802EC23C_5E930C", 88, 94}, "POKEMON_FUNC(func_802EC23C_5E930C)", {}, {}, {}, {}, d_263_exact_abi_dependencies, d_263_native},
    {0x802ec294u, true, SourceAbi::gobj_callback, {"src/tunnel/zapdos_egg/code.c", "func_802EC294_5E9364", 96, 118}, "POKEMON_FUNC(func_802EC294_5E9364)", {}, {}, {}, {}, {}, d_264_native},
    {0x802ec398u, true, SourceAbi::gobj_callback, {"src/tunnel/zapdos_egg/code.c", "func_802EC398_5E9468", 120, 123}, "POKEMON_FUNC(func_802EC398_5E9468)", {}, {}, {}, {}, {}, d_265_native},
    {0x802ec3bcu, true, SourceAbi::factory_five_argument, {"src/tunnel/zapdos_egg/code.c", "func_802EC3BC_5E948C", 125, 129}, "GObj* func_802EC3BC_5E948C(s32 gObjID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)", {}, {}, {}, {}, {}, d_266_native},
    {0x802eb860u, true, SourceAbi::gobj_callback, {"src/tunnel/zubat/code.c", "func_802EB860_5E8930", 16, 28}, "POKEMON_FUNC(func_802EB860_5E8930)", d_267_callbacks, d_267_graphs, {}, {}, {}, d_267_native},
    {0x802eb908u, true, SourceAbi::gobj_callback, {"src/tunnel/zubat/code.c", "func_802EB908_5E89D8", 30, 39}, "POKEMON_FUNC(func_802EB908_5E89D8)", d_268_callbacks, d_268_graphs, {}, d_268_headers, {}, d_268_native},
    {0x802eb98cu, true, SourceAbi::gobj_callback, {"src/tunnel/zubat/code.c", "func_802EB98C_5E8A5C", 41, 48}, "POKEMON_FUNC(func_802EB98C_5E8A5C)", {}, {}, {}, {}, {}, d_269_native},
    {0x802eb9f8u, true, SourceAbi::gobj_callback, {"src/tunnel/zubat/code.c", "func_802EB9F8_5E8AC8", 50, 55}, "POKEMON_FUNC(func_802EB9F8_5E8AC8)", {}, d_270_graphs, {}, {}, {}, d_270_native},
    {0x802eba48u, true, SourceAbi::gobj_callback, {"src/tunnel/zubat/code.c", "func_802EBA48_5E8B18", 57, 64}, "POKEMON_FUNC(func_802EBA48_5E8B18)", d_271_callbacks, d_271_graphs, {}, d_271_headers, {}, d_271_native},
    {0x802ebabcu, true, SourceAbi::gobj_callback, {"src/tunnel/zubat/code.c", "func_802EBABC_5E8B8C", 66, 73}, "POKEMON_FUNC(func_802EBABC_5E8B8C)", {}, {}, {}, {}, {}, d_272_native},
    {0x802ebb28u, true, SourceAbi::gobj_callback, {"src/tunnel/zubat/code.c", "func_802EBB28_5E8BF8", 75, 82}, "POKEMON_FUNC(func_802EBB28_5E8BF8)", {}, {}, {}, {}, {}, d_273_native},
    {0x802ebba8u, true, SourceAbi::factory_five_argument, {"src/tunnel/zubat/code.c", "func_802EBBA8_5E8C78", 84, 86}, "GObj* func_802EBBA8_5E8C78(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)", {}, {}, {}, {}, {}, d_274_native},
    {0u, false, SourceAbi::file_local_empty, {"src/tunnel/electabuzz/code.c", "nullsub2", 440, 441}, "static void nullsub2(void)", {}, {}, {}, {}, {}, {}},
};
const ActorFactoryBinding factories[] = {
    {50, 0x802e6420u, 0x802eea8cu, 0x802e5cb0u, ActorFactoryWrapper::ground, diglett_Spawn},
    {51, 0x802e6c28u, 0x802eebbcu, 0x802e6490u, ActorFactoryWrapper::ground, dugtrio_Spawn},
    {125, 0x802ea424u, 0x802ef714u, 0x802e93d0u, ActorFactoryWrapper::ground, electabuzz_Spawn},
    {101, 0x802e5c74u, 0x802ee95cu, 0x802e5160u, ActorFactoryWrapper::ground, electrode_Spawn},
    {1001, 0x802ecfc8u, 0x802f01a0u, 0x802ecac0u, ActorFactoryWrapper::ordinary_dl4, func_802ECFC8_5EA098},
    {93, 0x802ebce8u, 0x802efd90u, 0x802ebbe0u, ActorFactoryWrapper::ground, func_802EBCE8_5E8DB8},
    {14, 0x802e7620u, 0x802eed40u, 0x802e6cb0u, ActorFactoryWrapper::ordinary, kakuna_Spawn},
    {129, 0x802ebf04u, 0x802efe7cu, 0x802ebd20u, ActorFactoryWrapper::ground, func_802EBF04_5E8FD4},
    {81, 0x802e4afcu, 0x802ee660u, 0x802e3a14u, ActorFactoryWrapper::ground, magnemite_Spawn},
    {82, 0x802e505cu, 0x802ee79cu, 0x802e4b40u, ActorFactoryWrapper::ground, magneton_Spawn},
    {25, 0x802e9390u, 0x802ef2d0u, 0x802e7680u, ActorFactoryWrapper::ground, func_802E9390_5E6460},
    {1008, 0x802ea6f8u, 0x802ef7bcu, 0x802ea53cu, ActorFactoryWrapper::ordinary_dl4, func_802EA6F8_5E77C8},
    {1009, 0x802eaf18u, 0x802efa68u, 0x802ea730u, ActorFactoryWrapper::ordinary_dl4, func_802EAF18_5E7FE8},
    {1010, 0x802eb3a8u, 0x802efafcu, 0x802eafb0u, ActorFactoryWrapper::ordinary, func_802EB3A8_5E8478},
    {1012, 0x802ec720u, 0x802f0030u, 0x802ec400u, ActorFactoryWrapper::ordinary_dl4, func_802EC720_5E97F0},
    {1013, 0x802eca80u, 0x802f00d0u, 0x802ec760u, ActorFactoryWrapper::ordinary_dl4, func_802ECA80_5E9B50},
    {1014, 0x802edec0u, 0x802f025cu, 0x802ed620u, ActorFactoryWrapper::ordinary_dl4, func_802EDEC0_5EAF90},
    {145, 0x802eb818u, 0x802efc0cu, 0x802eb3e0u, ActorFactoryWrapper::ordinary, func_802EB818_5E88E8},
    {602, 0x802ec3bcu, 0x802eff94u, 0x802ebf40u, ActorFactoryWrapper::ordinary, func_802EC3BC_5E948C},
    {41, 0x802ebba8u, 0x802efd10u, 0x802eb860u, ActorFactoryWrapper::ordinary, func_802EBBA8_5E8C78},
    /* Real imported common Source body, app_level/4FE330.c112-114. */
    {1003, 0x8035e0d4u, 0x80388348u, 0x8035df20u, ActorFactoryWrapper::ground_dl4, func_8035E0D4_4FE4E4},
};
constexpr std::uint32_t graphs[] = {0x802ee2a4u, 0x802ee354u, 0x802ee394u, 0x802ee3d4u, 0x802ee454u, 0x802ee4c4u, 0x802ee4f4u, 0x802ee524u, 0x802ee564u, 0x802ee5ccu, 0x802ee6b8u, 0x802ee718u, 0x802ee748u, 0x802ee778u, 0x802ee83cu, 0x802ee88cu, 0x802ee8b0u, 0x802ee900u, 0x802ee9f8u, 0x802eea18u, 0x802eea38u, 0x802eea58u, 0x802eeb28u, 0x802eeb48u, 0x802eeb68u, 0x802eeb88u, 0x802eec1cu, 0x802eec4cu, 0x802eec6cu, 0x802eec8cu, 0x802eecccu, 0x802eececu, 0x802eeed8u, 0x802ef0a0u, 0x802ef150u, 0x802ef190u, 0x802ef1e0u, 0x802ef220u, 0x802ef260u, 0x802ef280u, 0x802ef3ecu, 0x802ef464u, 0x802ef48cu, 0x802ef4ecu, 0x802ef51cu, 0x802ef54cu, 0x802ef59cu, 0x802ef5ccu, 0x802ef61cu, 0x802ef64cu, 0x802ef68cu, 0x802ef6b0u, 0x802ef778u, 0x802ef788u, 0x802efac8u, 0x802efba8u, 0x802efbc8u, 0x802efc5cu, 0x802efc7cu, 0x802efcacu, 0x802efcccu, 0x802efe48u, 0x802eff2cu, 0x802eff4cu, 0x802efffcu, 0x802f009cu, 0x802f0168u, 0x802f0228u};
constexpr std::uint32_t weighted[] = {0x802ee344u, 0x802ee6f8u, 0x802eef78u, 0x802ef44cu};
constexpr std::uint32_t roots[] = {0x802e3a14u, 0x802e4b40u, 0x802e5160u, 0x802e5cb0u, 0x802e6490u, 0x802e6cb0u, 0x802e7680u, 0x802e93d0u, 0x802ea53cu, 0x802ea730u, 0x802eafb0u, 0x802eb3e0u, 0x802eb860u, 0x802ebbe0u, 0x802ebd20u, 0x802ebf40u, 0x802ec400u, 0x802ec760u, 0x802ecac0u, 0x802ed620u, 0x8035df20u};
constexpr IsleTunnelCountedOption magikarp_options[] = {{3, tunnel_SpawnMagikarp}, {4, nullptr}};
constexpr ActorCountedOptions counted_options[] = {{0x802ee120u, magikarp_options}};
constexpr ActorChildFactoryDependency child_factories[] = {
    {{{"src/tunnel/magneton/code.c", "tunnel_SpawnMagneton", 204, 209}, 0x802e5094u, 204}, {0x00000000u, 0x802e5094u, 0x00000052u, 0x802e505cu, 0x80363eb4u, 0x80364280u, 0x802ee79cu}, true},
    {{{"src/tunnel/main.c", "tunnel_SpawnMagikarp", 214, 214}, 0x802e2500u, 214}, {0x802ee110u, 0x802e2500u, 0x00000081u, 0x802ebf04u, 0x80363eb4u, 0x80364280u, 0x802efe7cu}, false},
};
constexpr ActorAnimationBinding animation_callbacks[] = {
    {0x802ecc18u, func_802ECC18_5E9CE8},
    {0x802ed720u, func_802ED720_5EA7F0},
    {0x802ed7bcu, func_802ED7BC_5EA88C},
};
constexpr ActorCollisionBinding collision_callbacks[] = {{0x802eae7cu, func_802EAE7C_5E7F4C}};
/* Four initial-only headers are required in addition to the 66 explicitly
 * referenced by actor bodies. No disabled/NULL Source header is invented. */
constexpr std::uint32_t headers[] = {
    0x802ee20cu, 0x802ee220u, 0x802ee234u, 0x802ee248u, 0x802ee25cu, 0x802ee270u, 0x802ee6a4u, 0x802ee7ecu, 0x802ee800u, 0x802ee814u,
    0x802ee828u, 0x802ee994u, 0x802ee9a8u, 0x802ee9bcu, 0x802ee9d0u, 0x802ee9e4u, 0x802eeac4u, 0x802eead8u, 0x802eeaecu, 0x802eeb00u,
    0x802eeb14u, 0x802eebf4u, 0x802eec08u, 0x802eedbcu, 0x802eedd0u, 0x802eede4u, 0x802eee0cu, 0x802eee20u, 0x802eee34u, 0x802eee5cu,
    0x802eee70u, 0x802eeeacu, 0x802ef338u, 0x802ef34cu, 0x802ef360u, 0x802ef374u, 0x802ef388u, 0x802ef39cu, 0x802ef3b0u, 0x802ef3c4u,
    0x802ef3d8u, 0x802ef750u, 0x802ef764u, 0x802efa18u, 0x802efa2cu, 0x802efa40u, 0x802efaa0u, 0x802efab4u, 0x802efb54u, 0x802efb68u,
    0x802efb7cu, 0x802efb90u, 0x802efc48u, 0x802efd58u, 0x802efe0cu, 0x802efe20u, 0x802efebcu, 0x802efed0u, 0x802efee4u, 0x802efef8u,
    0x802effd4u, 0x802effe8u, 0x802f0074u, 0x802f0088u, 0x802f0110u, 0x802f0124u, 0x802f0138u, 0x802f014cu, 0x802f01f0u, 0x80388310u,
};
/* There is no course-local custom-render body in the pinned Tunnel scope.
 * These are genuine imported renderer dependencies, not simulated callbacks. */
constexpr ActorRenderDependency render_dependencies[] = {
    {0x800a15d8u, "renderModelTypeBFogged", "void renderModelTypeBFogged(GObj*)"},
    {0x800a1608u, "renderModelTypeDFogged", "void renderModelTypeDFogged(GObj*)"},
    {0x8035942cu, "renderPokemonModelTypeIFogged", "void renderPokemonModelTypeIFogged(GObj*)"},
    {0x80359484u, "renderPokemonModelTypeJFogged", "void renderPokemonModelTypeJFogged(GObj*)"},
    {0x803594dcu, "renderPokemonModelTypeBFogged", "void renderPokemonModelTypeBFogged(GObj*)"},
    {0x80359534u, "renderPokemonModelTypeDFogged", "void renderPokemonModelTypeDFogged(GObj*)"},
};
constexpr ActorAutomaticGraphDependency automatic_graphs[] = {
    {{{"src/tunnel/magnemite/code.c", "saved", 534, 537}, 0x802e487cu, 534}, 2, 0u},
    {{{"src/tunnel/magnemite/code.c", "saved", 557, 560}, 0x802e49a4u, 557}, 2, 0u},
    {{{"src/tunnel/pikachu/code.c", "saved", 237, 240}, 0x802e7718u, 237}, 2, 0u},
    {{{"src/tunnel/pikachu/code.c", "saved", 268, 272}, 0x802e789cu, 268}, 3, 0u},
    {{{"src/tunnel/pikachu/code.c", "saved", 287, 296}, 0x802e796cu, 287}, 8, 0u},
    {{{"src/tunnel/pikachu/code.c", "saved", 411, 416}, 0x802e7e94u, 411}, 4, 0u},
    {{{"src/tunnel/pkmn_1008/code.c", "saved", 29, 29}, 0x802ea53cu, 29}, 2, 0x802ef788u},
    {{{"src/tunnel/pkmn_1010/code.c", "saved", 38, 38}, 0x802eb0ccu, 38}, 2, 0x802efac8u},
    {{{"src/tunnel/pkmn_1012/code.c", "saved", 12, 12}, 0x802ec400u, 12}, 2, 0x802efffcu},
    {{{"src/tunnel/pkmn_1013/code.c", "saved", 14, 14}, 0x802ec760u, 14}, 2, 0x802f009cu},
    {{{"src/tunnel/zapdos_egg/code.c", "saved", 26, 26}, 0x802ebf40u, 26}, 2, 0x802eff4cu},
};
const ActorEggHelperBinding egg_helpers[] = {{0x80362414u, isle_egg_original_func_80362414_502824}};
} // namespace
std::span<const actor::BehaviorCallbackBinding> actor_callbacks() noexcept { return callback_bindings; }
std::span<const std::uint32_t> actor_graphs() noexcept { return graphs; }
std::span<const std::uint32_t> actor_weighted_states() noexcept { return weighted; }
std::span<const std::uint32_t> actor_roots() noexcept { return roots; }
std::span<const ActorFactoryBinding> actor_factories() noexcept { return factories; }
std::span<const ActorChildFactoryDependency> actor_child_factories() noexcept { return child_factories; }
std::span<const ActorDependency> actor_dependencies() noexcept { return dependencies; }
std::span<const ActorCountedOptions> actor_counted_options() noexcept { return counted_options; }
std::span<const ActorAnimationBinding> actor_animation_callbacks() noexcept { return animation_callbacks; }
std::span<const ActorCollisionBinding> actor_collision_callbacks() noexcept { return collision_callbacks; }
std::span<const std::uint32_t> actor_headers() noexcept { return headers; }
std::span<const ActorRenderDependency> actor_render_dependencies() noexcept { return render_dependencies; }
std::span<const ActorEggHelperBinding> actor_egg_helpers() noexcept { return egg_helpers; }
std::span<const ActorAutomaticGraphDependency> actor_automatic_graphs() noexcept { return automatic_graphs; }
} // namespace isle::tunnel

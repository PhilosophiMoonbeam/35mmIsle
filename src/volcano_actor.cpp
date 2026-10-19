#include "volcano_actor.hpp"
namespace isle::volcano {
namespace {
constexpr std::uint32_t d_209_0_callbacks[]={0x802db660u,0x802db6f8u};
constexpr std::uint32_t d_209_0_graphs[]={0x802e2984u};
constexpr std::uint32_t d_209_0_headers[]={0x802e27dcu,0x802e27f0u};
constexpr const char* d_209_0_services[]={"actor_current"};
const ActorHelperDependency d_228_1_helpers[]={{0x802d6e14u,"src/volcano/main.c",595,605,"void volcano_PokemonMove(IsleGameObject* obj)"}};
constexpr const char* d_228_1_services[]={"actor_current","actor_ground","actor_model","actor_position_component","actor_position_store"};
constexpr std::uint32_t d_242_2_graphs[]={0x802e2804u};
constexpr std::uint32_t d_242_2_weighted_states[]={0x802e2a64u};
constexpr std::uint32_t d_242_2_headers[]={0x802e2764u};
constexpr const char* d_242_2_services[]={"actor_current"};
constexpr std::uint32_t d_250_3_callbacks[]={0x802db6f8u,0x802db80cu};
constexpr std::uint32_t d_250_3_graphs[]={0x802e2804u};
constexpr std::uint32_t d_250_3_weighted_states[]={0x802e2a64u};
constexpr std::uint32_t d_250_3_headers[]={0x802e278cu};
constexpr const char* d_250_3_services[]={"actor_current"};
constexpr const char* d_264_4_services[]={"actor_current"};
constexpr std::uint32_t d_272_5_callbacks[]={0x802db6f8u,0x802db90cu};
constexpr std::uint32_t d_272_5_graphs[]={0x802e2804u};
constexpr std::uint32_t d_272_5_weighted_states[]={0x802e2a64u};
constexpr std::uint32_t d_272_5_headers[]={0x802e27a0u};
constexpr const char* d_272_5_services[]={"actor_current"};
constexpr const char* d_286_6_services[]={"actor_current"};
constexpr std::uint32_t d_294_7_graphs[]={0x802e2804u};
constexpr std::uint32_t d_294_7_weighted_states[]={0x802e2a64u};
constexpr std::uint32_t d_294_7_headers[]={0x802e2778u};
constexpr const char* d_294_7_services[]={"actor_current"};
constexpr std::uint32_t d_302_8_callbacks[]={0x802dbdb8u};
constexpr std::uint32_t d_302_8_graphs[]={0x802e2984u};
constexpr std::uint32_t d_302_8_headers[]={0x802e27b4u};
constexpr const char* d_302_8_services[]={"actor_current"};
constexpr std::uint32_t d_310_9_callbacks[]={0x802dbdb8u};
constexpr std::uint32_t d_310_9_graphs[]={0x802e2984u};
constexpr std::uint32_t d_310_9_headers[]={0x802e27b4u};
constexpr const char* d_310_9_services[]={"actor_current"};
constexpr std::uint32_t d_319_10_callbacks[]={0x802db6f8u,0x802dbb2cu};
constexpr std::uint32_t d_319_10_graphs[]={0x802e2984u};
constexpr std::uint32_t d_319_10_headers[]={0x802e27dcu};
constexpr const char* d_319_10_services[]={"actor_current"};
constexpr const char* d_328_11_services[]={"actor_current"};
constexpr std::uint32_t d_335_12_callbacks[]={0x802db6f8u,0x802dbc00u,0x802dbc84u};
constexpr std::uint32_t d_335_12_graphs[]={0x802e28b4u};
constexpr const char* d_335_12_services[]={"actor_current"};
constexpr std::uint32_t d_347_13_headers[]={0x802e27a0u};
constexpr const char* d_347_13_services[]={"actor_current"};
constexpr std::uint32_t d_357_14_callbacks[]={0x802db6f8u,0x802dbd34u,0x802dbdb8u};
constexpr std::uint32_t d_357_14_graphs[]={0x802e2934u};
constexpr std::uint32_t d_357_14_headers[]={0x802e27c8u};
constexpr const char* d_357_14_services[]={"actor_current"};
constexpr const char* d_374_15_services[]={"actor_current"};
constexpr std::uint32_t d_381_16_callbacks[]={0x802dbb78u};
constexpr const char* d_381_16_services[]={"actor_current"};
constexpr std::uint32_t d_385_17_callbacks[]={0x802db6f8u};
constexpr std::uint32_t d_385_17_graphs[]={0x802e29e4u};
constexpr const char* d_385_17_services[]={"actor_current","actor_reference_assign"};
constexpr std::uint32_t d_394_18_callbacks[]={0x802db6f8u,0x802dbe8cu};
constexpr std::uint32_t d_394_18_graphs[]={0x802e29b4u};
constexpr std::uint32_t d_394_18_headers[]={0x802e278cu};
constexpr const char* d_394_18_services[]={"actor_current"};
constexpr const char* d_403_19_services[]={"actor_current"};
constexpr std::uint32_t d_410_20_callbacks[]={0x802db6f8u,0x802dbf4cu};
constexpr std::uint32_t d_410_20_graphs[]={0x802e2a24u};
constexpr std::uint32_t d_410_20_headers[]={0x802e27a0u};
constexpr const char* d_410_20_services[]={"actor_current"};
constexpr const char* d_419_21_services[]={"actor_current"};
constexpr std::uint32_t d_427_22_graphs[]={0x802e2a24u};
constexpr std::uint32_t d_427_22_weighted_states[]={0x802e2a64u};
constexpr std::uint32_t d_427_22_headers[]={0x802e2778u};
constexpr const char* d_427_22_services[]={"actor_current"};
constexpr std::uint32_t d_436_23_child_factories[]={0x802dc018u};
constexpr std::uint32_t d_115_24_callbacks[]={0x802dd328u};
constexpr const char* d_115_24_services[]={"actor_current","actor_send_to_link"};
constexpr std::uint32_t d_121_25_callbacks[]={0x802dd3b0u,0x802dd500u};
constexpr std::uint32_t d_121_25_headers[]={0x802e2e64u};
constexpr const char* d_121_25_services[]={"actor_current","actor_player","actor_send_addressed"};
constexpr std::uint32_t d_132_26_callbacks[]={0x802dd4a4u};
constexpr const char* d_132_26_services[]={"actor_current","actor_ground","actor_model","actor_position_component","actor_position_store"};
constexpr std::uint32_t d_148_27_callbacks[]={0x802d6cfcu};
constexpr const char* d_148_27_services[]={"actor_current"};
constexpr std::uint32_t d_159_28_graphs[]={0x802e2ec8u};
constexpr std::uint32_t d_159_28_weighted_states[]={0x802e2f28u};
constexpr std::uint32_t d_159_28_headers[]={0x802e2e78u};
constexpr const char* d_159_28_services[]={"actor_current"};
constexpr std::uint32_t d_168_29_graphs[]={0x802e2ec8u};
constexpr std::uint32_t d_168_29_weighted_states[]={0x802e2f28u};
constexpr std::uint32_t d_168_29_headers[]={0x802e2e8cu};
constexpr const char* d_168_29_services[]={"actor_current"};
constexpr std::uint32_t d_177_30_callbacks[]={0x802dd67cu};
constexpr std::uint32_t d_177_30_weighted_states[]={0x802e2f28u};
constexpr std::uint32_t d_177_30_headers[]={0x802e2ea0u,0x802e2eb4u};
constexpr const char* d_177_30_services[]={"actor_current"};
constexpr const char* d_191_31_services[]={"actor_current"};
constexpr std::uint32_t d_198_32_callbacks[]={0x802dd67cu};
constexpr std::uint32_t d_198_32_weighted_states[]={0x802e2f28u};
constexpr std::uint32_t d_198_32_headers[]={0x802e2eb4u};
constexpr const char* d_198_32_services[]={"actor_current"};
constexpr std::uint32_t d_207_33_callbacks[]={0x802dd500u,0x802dd67cu};
constexpr std::uint32_t d_207_33_headers[]={0x802e2eb4u};
constexpr const char* d_207_33_services[]={"actor_current"};
constexpr std::uint32_t d_216_34_child_factories[]={0x802dd7acu};
constexpr const char* d_322_35_services[]={"actor_current"};
constexpr std::uint32_t d_350_36_callbacks[]={0x802d8aa0u,0x802d8ca4u,0x802d9900u,0x802d9950u};
constexpr const char* d_350_36_services[]={"actor_current","actor_reference_assign"};
constexpr std::uint32_t d_378_37_graphs[]={0x802e1a34u};
constexpr std::uint32_t d_378_37_weighted_states[]={0x802e1e64u};
constexpr std::uint32_t d_378_37_headers[]={0x802e1958u};
constexpr const char* d_378_37_services[]={"actor_current"};
constexpr std::uint32_t d_387_38_callbacks[]={0x802d8ca4u,0x802d8db8u};
constexpr std::uint32_t d_387_38_graphs[]={0x802e1a34u};
constexpr std::uint32_t d_387_38_weighted_states[]={0x802e1e64u};
constexpr std::uint32_t d_387_38_headers[]={0x802e1980u};
constexpr const char* d_387_38_services[]={"actor_current"};
constexpr const char* d_400_39_services[]={"actor_current"};
constexpr std::uint32_t d_408_40_callbacks[]={0x802d8ca4u,0x802d8eb8u};
constexpr std::uint32_t d_408_40_graphs[]={0x802e1a34u};
constexpr std::uint32_t d_408_40_weighted_states[]={0x802e1e64u};
constexpr std::uint32_t d_408_40_headers[]={0x802e1994u};
constexpr const char* d_408_40_services[]={"actor_current"};
constexpr const char* d_421_41_services[]={"actor_current"};
constexpr std::uint32_t d_429_42_graphs[]={0x802e1df4u};
constexpr std::uint32_t d_429_42_weighted_states[]={0x802e1e64u};
constexpr std::uint32_t d_429_42_headers[]={0x802e196cu};
constexpr const char* d_429_42_services[]={"actor_current","actor_send_to_link"};
constexpr std::uint32_t d_439_43_graphs[]={0x802e1df4u};
constexpr std::uint32_t d_439_43_weighted_states[]={0x802e1e64u};
constexpr std::uint32_t d_439_43_headers[]={0x802e196cu};
constexpr const char* d_439_43_services[]={"actor_current"};
constexpr const char* d_447_44_services[]={"actor_current","actor_send_addressed"};
constexpr std::uint32_t d_456_45_callbacks[]={0x802d8ca4u,0x802d9380u,0x802d9b8cu};
constexpr std::uint32_t d_456_45_graphs[]={0x802e1c44u,0x802e1da4u,0x802e1e90u,0x802e1eb0u};
constexpr std::uint32_t d_456_45_headers[]={0x802e1994u,0x802e19bcu,0x802e19d0u,0x802e19e4u};
constexpr const char* d_456_45_services[]={"actor_current","actor_send_to_link"};
constexpr const char* d_488_46_services[]={"actor_enter"};
constexpr std::uint32_t d_491_47_callbacks[]={0x802d8ca4u};
constexpr std::uint32_t d_491_47_graphs[]={0x802e1ed0u};
constexpr std::uint32_t d_491_47_headers[]={0x802e19a8u};
constexpr const char* d_491_47_services[]={"actor_current","actor_reference_assign"};
constexpr std::uint32_t d_503_48_callbacks[]={0x802d8ca4u,0x802d9380u};
constexpr std::uint32_t d_503_48_graphs[]={0x802e1da4u};
constexpr std::uint32_t d_503_48_headers[]={0x802e1994u};
constexpr const char* d_503_48_services[]={"actor_current"};
constexpr const char* d_512_49_services[]={"actor_current"};
constexpr std::uint32_t d_520_50_callbacks[]={0x802d8ca4u,0x802d9464u,0x802d94e8u};
constexpr std::uint32_t d_520_50_graphs[]={0x802e1b14u};
constexpr const char* d_520_50_services[]={"actor_current"};
constexpr std::uint32_t d_532_51_headers[]={0x802e1994u};
constexpr const char* d_532_51_services[]={"actor_current"};
constexpr std::uint32_t d_542_52_callbacks[]={0x802d8ca4u,0x802d95acu,0x802d97b8u};
constexpr std::uint32_t d_542_52_graphs[]={0x802e1ba4u};
constexpr std::uint32_t d_542_52_headers[]={0x802e19f8u};
constexpr const char* d_542_52_services[]={"actor_current"};
constexpr const char* d_558_53_services[]={"actor_current"};
constexpr std::uint32_t d_565_54_callbacks[]={0x802d93dcu};
constexpr const char* d_565_54_services[]={"actor_current"};
constexpr std::uint32_t d_569_55_callbacks[]={0x802d96acu};
constexpr std::uint32_t d_569_55_graphs[]={0x802e1d14u};
constexpr std::uint32_t d_569_55_weighted_states[]={0x802e1e64u};
constexpr std::uint32_t d_569_55_headers[]={0x802e1a0cu};
constexpr const char* d_569_55_services[]={"actor_current"};
constexpr const char* d_578_56_services[]={"actor_current"};
constexpr std::uint32_t d_585_57_callbacks[]={0x802d8ca4u,0x802d976cu};
constexpr std::uint32_t d_585_57_graphs[]={0x802e1c84u};
constexpr std::uint32_t d_585_57_headers[]={0x802e1980u};
constexpr const char* d_585_57_services[]={"actor_current"};
constexpr const char* d_594_58_services[]={"actor_current"};
constexpr std::uint32_t d_601_59_callbacks[]={0x802d8ca4u,0x802d95acu};
constexpr std::uint32_t d_601_59_graphs[]={0x802e1ba4u,0x802e1ef0u};
constexpr std::uint32_t d_601_59_headers[]={0x802e1a0cu};
constexpr const char* d_601_59_services[]={"actor_current","actor_reference_assign","actor_send_to_link"};
constexpr std::uint32_t d_628_60_graphs[]={0x802e1f40u};
constexpr const char* d_628_60_services[]={"actor_current","actor_visibility"};
constexpr std::uint32_t d_636_61_graphs[]={0x802e1f60u};
constexpr const char* d_636_61_services[]={"actor_current","actor_visibility"};
constexpr std::uint32_t d_644_62_callbacks[]={0x802d8ca4u,0x802d9a80u,0x802d9b44u};
constexpr std::uint32_t d_644_62_graphs[]={0x802e1a34u};
constexpr std::uint32_t d_644_62_headers[]={0x802e1994u};
constexpr const char* d_644_62_services[]={"actor_current","actor_visibility"};
constexpr const char* d_666_63_services[]={"actor_current","actor_model","actor_position_component"};
constexpr const char* d_682_64_services[]={"actor_current"};
constexpr std::uint32_t d_689_65_graphs[]={0x802e1f80u};
constexpr std::uint32_t d_689_65_headers[]={0x802e19d0u};
const ActorHelperDependency d_689_65_helpers[]={{0x802ded6cu,"src/volcano/evolution_controller/code.c",145,156,"void func_802DED6C_72FF6C(IsleGameObject* obj)"}};
constexpr const char* d_689_65_services[]={"actor_current"};
constexpr std::uint32_t d_698_66_callbacks[]={0x802d9c30u};
constexpr const char* d_698_66_services[]={"actor_current"};
constexpr std::uint32_t d_705_67_callbacks[]={0x802d9b8cu};
constexpr std::uint32_t d_705_67_headers[]={0x802e19d0u};
constexpr const char* d_705_67_services[]={"actor_current"};
const ActorHelperDependency d_713_68_helpers[]={{0x802dd24cu,"src/volcano/charmeleon/code.c",634,649,"void charmeleon_EvolvedSpawn(IsleGameObject* obj)"}};
constexpr const char* d_713_68_services[]={"actor_current"};
constexpr std::uint32_t d_719_69_child_factories[]={0x802d9cb8u};
constexpr const char* d_244_70_services[]={"actor_current"};
constexpr std::uint32_t d_259_71_callbacks[]={0x802dcb44u};
constexpr std::uint32_t d_259_71_weighted_states[]={0x802e2c40u};
constexpr const char* d_259_71_services[]={"actor_current"};
constexpr std::uint32_t d_268_72_callbacks[]={0x802dc280u};
constexpr std::uint32_t d_268_72_graphs[]={0x802e2c00u};
constexpr std::uint32_t d_268_72_weighted_states[]={0x802e2c40u};
constexpr std::uint32_t d_268_72_headers[]={0x802e2b28u};
constexpr const char* d_268_72_services[]={"actor_current"};
constexpr std::uint32_t d_279_73_callbacks[]={0x802dc280u};
constexpr std::uint32_t d_279_73_graphs[]={0x802e2c00u};
constexpr std::uint32_t d_279_73_weighted_states[]={0x802e2c40u};
constexpr std::uint32_t d_279_73_headers[]={0x802e2b3cu};
constexpr const char* d_279_73_services[]={"actor_current"};
const ActorHelperDependency d_290_74_helpers[]={{0x802d6f68u,"src/volcano/main.c",627,704,"void volcano_FollowPath(IsleGameObject* obj, float* pathParam, float pathEnd, float speedMult, int32_t flags)"}};
constexpr const char* d_290_74_services[]={"actor_current","actor_random_float"};
constexpr std::uint32_t d_317_75_graphs[]={0x802e2c00u};
constexpr std::uint32_t d_317_75_weighted_states[]={0x802e2c70u};
constexpr std::uint32_t d_317_75_headers[]={0x802e2b14u};
constexpr const char* d_317_75_services[]={"actor_current"};
constexpr std::uint32_t d_326_76_graphs[]={0x802e2c00u};
constexpr std::uint32_t d_326_76_weighted_states[]={0x802e2c70u};
constexpr std::uint32_t d_326_76_headers[]={0x802e2b00u};
constexpr const char* d_326_76_services[]={"actor_current"};
constexpr std::uint32_t d_335_77_graphs[]={0x802e2c00u};
constexpr std::uint32_t d_335_77_weighted_states[]={0x802e2c70u};
constexpr std::uint32_t d_335_77_headers[]={0x802e2aecu};
constexpr const char* d_335_77_services[]={"actor_current"};
constexpr std::uint32_t d_344_78_callbacks[]={0x802dc6b4u,0x802dc7f8u};
constexpr std::uint32_t d_344_78_graphs[]={0x802e2ba0u,0x802e2be0u};
constexpr std::uint32_t d_344_78_headers[]={0x802e2b50u};
constexpr const char* d_344_78_services[]={"actor_current"};
constexpr std::uint32_t d_360_79_callbacks[]={0x802dc6b4u,0x802dc7f8u};
constexpr std::uint32_t d_360_79_graphs[]={0x802e2ba0u,0x802e2be0u};
constexpr std::uint32_t d_360_79_headers[]={0x802e2b50u,0x802e2b64u,0x802e2b78u,0x802e2b8cu};
constexpr const char* d_360_79_services[]={"actor_current"};
constexpr const char* d_388_80_services[]={"actor_current","actor_item","actor_send_to_link"};
constexpr std::uint32_t d_404_81_callbacks[]={0x802dc590u};
constexpr const char* d_404_81_services[]={"actor_current","actor_model","actor_position_component"};
constexpr std::uint32_t d_412_82_callbacks[]={0x802dc4f0u};
constexpr const char* d_412_82_services[]={"actor_current","actor_model","actor_position_component"};
constexpr std::uint32_t d_420_83_callbacks[]={0x802dc850u,0x802dc924u};
constexpr std::uint32_t d_420_83_headers[]={0x802e2b3cu};
constexpr const char* d_420_83_services[]={"actor_current"};
constexpr std::uint32_t d_427_84_graphs[]={0x802e2ba0u};
constexpr std::uint32_t d_427_84_weighted_states[]={0x802e2c88u};
constexpr const char* d_427_84_services[]={"actor_current","actor_send_to_link"};
constexpr const char* d_448_85_services[]={"actor_current"};
constexpr std::uint32_t d_459_86_callbacks[]={0x802d6cc0u,0x802dca28u};
const ActorHelperDependency d_459_86_helpers[]={{0x802d6a5cu,"src/volcano/main.c",487,514,"void volcano_SpawnCharizard(IsleGameObject* obj)"}};
constexpr const char* d_459_86_services[]={"actor_current"};
constexpr const char* d_471_87_services[]={"actor_current","actor_model","actor_position_component","actor_position_store"};
constexpr std::uint32_t d_492_88_callbacks[]={0x802dc060u,0x802dcbd0u};
constexpr std::uint32_t d_492_88_headers[]={0x802e2b8cu};
constexpr const char* d_492_88_services[]={"actor_current"};
constexpr std::uint32_t d_504_89_graphs[]={0x802e2ca4u};
constexpr std::uint32_t d_504_89_weighted_states[]={0x802e2d04u};
constexpr std::uint32_t d_504_89_headers[]={0x802e2aecu};
constexpr const char* d_504_89_services[]={"actor_current"};
constexpr std::uint32_t d_512_90_callbacks[]={0x802dcbd0u,0x802dcce4u};
constexpr std::uint32_t d_512_90_graphs[]={0x802e2ca4u};
constexpr std::uint32_t d_512_90_weighted_states[]={0x802e2d04u};
constexpr std::uint32_t d_512_90_headers[]={0x802e2b28u};
constexpr const char* d_512_90_services[]={"actor_current"};
constexpr const char* d_526_91_services[]={"actor_current"};
constexpr std::uint32_t d_534_92_callbacks[]={0x802dcbd0u,0x802dcde4u};
constexpr std::uint32_t d_534_92_graphs[]={0x802e2ca4u};
constexpr std::uint32_t d_534_92_weighted_states[]={0x802e2d04u};
constexpr std::uint32_t d_534_92_headers[]={0x802e2b3cu};
constexpr const char* d_534_92_services[]={"actor_current"};
constexpr const char* d_548_93_services[]={"actor_current"};
constexpr std::uint32_t d_556_94_callbacks[]={0x802dcbd0u};
constexpr std::uint32_t d_556_94_graphs[]={0x802e2d24u};
constexpr std::uint32_t d_556_94_headers[]={0x802e2b64u,0x802e2b78u,0x802e2b8cu};
constexpr const char* d_556_94_services[]={"actor_current"};
constexpr std::uint32_t d_577_95_callbacks[]={0x802dcbd0u};
constexpr std::uint32_t d_577_95_headers[]={0x802e2b50u};
constexpr const char* d_577_95_services[]={"actor_current"};
constexpr std::uint32_t d_586_96_callbacks[]={0x802dcbd0u,0x802dd034u,0x802dd0e8u};
constexpr std::uint32_t d_586_96_graphs[]={0x802e2d54u};
constexpr const char* d_586_96_services[]={"actor_current"};
constexpr std::uint32_t d_598_97_headers[]={0x802e2b3cu};
constexpr const char* d_598_97_services[]={"actor_current","actor_random_float"};
constexpr std::uint32_t d_608_98_callbacks[]={0x802dcbd0u,0x802dd1c0u};
constexpr std::uint32_t d_608_98_graphs[]={0x802e2da4u};
constexpr std::uint32_t d_608_98_headers[]={0x802e2b00u};
constexpr const char* d_608_98_services[]={"actor_current"};
constexpr const char* d_623_99_services[]={"actor_current"};
constexpr std::uint32_t d_630_100_child_factories[]={0x802dd214u};
constexpr std::uint32_t d_634_101_child_factories[]={0x802dd214u};
constexpr const char* d_634_101_services[]={"actor_add_geo","actor_component","actor_component_store","actor_current","actor_model"};
constexpr std::uint32_t d_78_102_callbacks[]={0x802dec78u};
constexpr std::uint32_t d_78_102_graphs[]={0x802e3454u};
constexpr const char* d_78_102_services[]={"actor_current","actor_visibility"};
constexpr std::uint32_t d_91_103_callbacks[]={0x802dea80u,0x802debb0u};
constexpr const char* d_91_103_services[]={"actor_current","actor_model","actor_position_component","actor_send_addressed"};
constexpr std::uint32_t d_106_104_graphs[]={0x802e3474u};
constexpr const char* d_106_104_services[]={"actor_current"};
constexpr std::uint32_t d_112_105_headers[]={0x802e342cu};
constexpr const char* d_112_105_services[]={"actor_current","actor_send_to_link","actor_visibility"};
constexpr std::uint32_t d_124_106_headers[]={0x802e3440u};
constexpr const char* d_124_106_services[]={"actor_current","actor_environment_sound","actor_send_to_link","actor_visibility"};
constexpr std::uint32_t d_141_107_child_factories[]={0x802ded34u};
constexpr std::uint32_t d_145_108_child_factories[]={0x802ded34u};
constexpr const char* d_145_108_services[]={"actor_add_geo","actor_current"};
constexpr std::uint32_t d_97_109_callbacks[]={0x802df3d4u};
constexpr const char* d_97_109_services[]={"actor_current"};
constexpr std::uint32_t d_101_110_graphs[]={0x802e3708u};
constexpr std::uint32_t d_101_110_headers[]={0x802e36b0u};
constexpr const char* d_101_110_services[]={"actor_current"};
constexpr std::uint32_t d_109_111_callbacks[]={0x802df880u};
constexpr std::uint32_t d_109_111_headers[]={0x802e36c4u,0x802e36d8u,0x802e36ecu};
constexpr const char* d_109_111_services[]={"actor_current","actor_player","actor_send_addressed","actor_sound_play"};
constexpr const char* d_131_112_services[]={"actor_enter"};
constexpr const char* d_137_113_services[]={"actor_bgm_volume","actor_current","actor_sound_volume"};
constexpr const char* d_158_114_services[]={"actor_current","actor_sound_identity","actor_sound_play","actor_sound_stop"};
constexpr std::uint32_t d_173_115_callbacks[]={0x802df534u,0x802df7f4u};
constexpr const char* d_173_115_services[]={"actor_animation_callback","actor_camera_animation","actor_camera_object","actor_camera_process","actor_camera_speed","actor_current","actor_cutscene_start","actor_main_camera","actor_pause_processes","actor_player","actor_player_animation","actor_player_model_init","actor_remove_items","actor_reset_camera","actor_reset_messages","actor_send_addressed","actor_sky_speed","actor_sound_stop_all"};
constexpr std::uint32_t d_217_116_child_factories[]={0x802dfa38u};
constexpr std::uint32_t d_210_117_callbacks[]={0x802dab90u,0x802dac38u};
constexpr std::uint32_t d_210_117_graphs[]={0x802e25f4u};
constexpr std::uint32_t d_210_117_headers[]={0x802e241cu,0x802e2430u};
constexpr const char* d_210_117_services[]={"actor_current"};
const ActorHelperDependency d_227_118_helpers[]={{0x802d6e14u,"src/volcano/main.c",595,605,"void volcano_PokemonMove(IsleGameObject* obj)"}};
constexpr const char* d_227_118_services[]={"actor_current","actor_ground","actor_model","actor_position_component","actor_position_store"};
constexpr std::uint32_t d_241_119_graphs[]={0x802e2444u};
constexpr std::uint32_t d_241_119_weighted_states[]={0x802e26d4u};
constexpr std::uint32_t d_241_119_headers[]={0x802e23a4u};
constexpr const char* d_241_119_services[]={"actor_current"};
constexpr std::uint32_t d_250_120_callbacks[]={0x802dac38u,0x802dad4cu};
constexpr std::uint32_t d_250_120_graphs[]={0x802e2444u};
constexpr std::uint32_t d_250_120_weighted_states[]={0x802e26d4u};
constexpr std::uint32_t d_250_120_headers[]={0x802e23ccu};
constexpr const char* d_250_120_services[]={"actor_current"};
constexpr const char* d_264_121_services[]={"actor_current"};
constexpr std::uint32_t d_272_122_callbacks[]={0x802dac38u,0x802dae4cu};
constexpr std::uint32_t d_272_122_graphs[]={0x802e2444u};
constexpr std::uint32_t d_272_122_weighted_states[]={0x802e26d4u};
constexpr std::uint32_t d_272_122_headers[]={0x802e23e0u};
constexpr const char* d_272_122_services[]={"actor_current"};
constexpr const char* d_286_123_services[]={"actor_current"};
constexpr std::uint32_t d_294_124_graphs[]={0x802e2444u};
constexpr std::uint32_t d_294_124_weighted_states[]={0x802e26d4u};
constexpr std::uint32_t d_294_124_headers[]={0x802e23b8u};
constexpr const char* d_294_124_services[]={"actor_current"};
constexpr std::uint32_t d_303_125_callbacks[]={0x802db2f8u};
constexpr std::uint32_t d_303_125_graphs[]={0x802e25f4u};
constexpr std::uint32_t d_303_125_headers[]={0x802e23f4u};
constexpr const char* d_303_125_services[]={"actor_current"};
constexpr std::uint32_t d_312_126_callbacks[]={0x802db2f8u};
constexpr std::uint32_t d_312_126_graphs[]={0x802e25f4u};
constexpr std::uint32_t d_312_126_headers[]={0x802e23f4u};
constexpr const char* d_312_126_services[]={"actor_current"};
constexpr std::uint32_t d_321_127_callbacks[]={0x802dac38u,0x802db06cu};
constexpr std::uint32_t d_321_127_graphs[]={0x802e25f4u};
constexpr std::uint32_t d_321_127_headers[]={0x802e241cu};
constexpr const char* d_321_127_services[]={"actor_current"};
constexpr const char* d_330_128_services[]={"actor_current"};
constexpr std::uint32_t d_337_129_callbacks[]={0x802dac38u,0x802db140u,0x802db1c4u};
constexpr std::uint32_t d_337_129_graphs[]={0x802e24f4u};
constexpr const char* d_337_129_services[]={"actor_current"};
constexpr std::uint32_t d_349_130_headers[]={0x802e23e0u};
constexpr const char* d_349_130_services[]={"actor_current"};
constexpr std::uint32_t d_359_131_callbacks[]={0x802dac38u,0x802db274u,0x802db2f8u};
constexpr std::uint32_t d_359_131_graphs[]={0x802e2574u};
constexpr std::uint32_t d_359_131_headers[]={0x802e2408u};
constexpr const char* d_359_131_services[]={"actor_current"};
constexpr const char* d_376_132_services[]={"actor_current"};
constexpr std::uint32_t d_383_133_callbacks[]={0x802db0b8u};
constexpr const char* d_383_133_services[]={"actor_current"};
constexpr std::uint32_t d_387_134_callbacks[]={0x802dac38u};
constexpr std::uint32_t d_387_134_graphs[]={0x802e2654u};
constexpr const char* d_387_134_services[]={"actor_current","actor_reference_assign"};
constexpr std::uint32_t d_396_135_callbacks[]={0x802dac38u,0x802db3ccu};
constexpr std::uint32_t d_396_135_graphs[]={0x802e2624u};
constexpr std::uint32_t d_396_135_headers[]={0x802e23ccu};
constexpr const char* d_396_135_services[]={"actor_current"};
constexpr const char* d_405_136_services[]={"actor_current"};
constexpr std::uint32_t d_412_137_callbacks[]={0x802dac38u,0x802db48cu};
constexpr std::uint32_t d_412_137_graphs[]={0x802e2694u};
constexpr std::uint32_t d_412_137_headers[]={0x802e23e0u};
constexpr const char* d_412_137_services[]={"actor_current"};
constexpr const char* d_421_138_services[]={"actor_current"};
constexpr std::uint32_t d_429_139_graphs[]={0x802e2694u};
constexpr std::uint32_t d_429_139_weighted_states[]={0x802e26d4u};
constexpr std::uint32_t d_429_139_headers[]={0x802e23b8u};
constexpr const char* d_429_139_services[]={"actor_current"};
constexpr std::uint32_t d_438_140_child_factories[]={0x802db558u};
constexpr std::uint32_t d_54_141_callbacks[]={0x802d6b64u,0x802dd8f8u,0x802dda0cu};
constexpr const char* d_54_141_services[]={"actor_current","actor_visibility"};
constexpr std::uint32_t d_75_142_graphs[]={0x802e2fa4u};
constexpr std::uint32_t d_75_142_headers[]={0x802e2f90u};
constexpr const char* d_75_142_services[]={"actor_current"};
constexpr std::uint32_t d_83_143_callbacks[]={0x802d6b2cu,0x802dd8f8u};
constexpr std::uint32_t d_83_143_headers[]={0x802e2f90u};
const ActorHelperDependency d_83_143_helpers[]={{0x802d6d6cu,"src/volcano/main.c",574,593,"bool volcano_SpawnGrowlitheOrArcanine(IsleGameObject* obj)"}};
constexpr const char* d_83_143_services[]={"actor_current"};
constexpr const char* d_97_144_services[]={"actor_current","actor_misc_object_get","actor_random_range","actor_send_addressed"};
constexpr std::uint32_t d_109_145_child_factories[]={0x802dda98u};
constexpr const char* d_17_146_services[]={"lifecycle_current","lifecycle_cutscene_model","lifecycle_cutscene_prepare","lifecycle_cutscene_run"};
constexpr const char* d_23_147_services[]={"lifecycle_cutscene_stop"};
constexpr const char* d_27_148_services[]={"lifecycle_effect_attach","lifecycle_effect_create"};
const ActorHelperDependency d_38_149_helpers[]={{0x802e0690u,"src/volcano/intro/code.c",23,25,"void volcano_IntroStopCutscene(IsleVolcanoLifecycleSource* l)"}};
constexpr const char* d_38_149_services[]={"lifecycle_delete_intro","lifecycle_intro_restore","lifecycle_reset_camera","lifecycle_sound_identity","lifecycle_sound_stop"};
constexpr const char* d_57_150_services[]={"lifecycle_current","lifecycle_sound_play_params"};
const ActorHelperDependency d_76_151_helpers[]={{0x802e0724u,"src/volcano/intro/code.c",38,55,"void volcano_StopIntro(IsleVolcanoLifecycleSource* l)"},{0x802e081cu,"src/volcano/intro/code.c",57,74,"void volcano_PlayIntroSounds(IsleGameObject* obj)"}};
constexpr const char* d_76_151_services[]={"lifecycle_animation_callback","lifecycle_camera_animation","lifecycle_camera_object","lifecycle_camera_process","lifecycle_camera_read","lifecycle_camera_speed","lifecycle_camera_store","lifecycle_create_process","lifecycle_current","lifecycle_input_pressed","lifecycle_main_camera","lifecycle_player_animation"};
const ActorHelperDependency d_135_152_helpers[]={{0x802e0660u,"src/volcano/intro/code.c",17,21,"void volcano_IntroStartCutscene(IsleGameObject* obj)"},{0x802e0948u,"src/volcano/intro/code.c",76,133,"void volcano_UpdateIntro(IsleGameObject* obj)"}};
constexpr const char* d_135_152_services[]={"lifecycle_create_process","lifecycle_player_model_init","lifecycle_reference_assign"};
constexpr std::uint32_t d_75_153_callbacks[]={0x802de3fcu,0x802de4c0u};
constexpr std::uint32_t d_75_153_headers[]={0x802e31f4u};
constexpr const char* d_75_153_services[]={"actor_current"};
constexpr std::uint32_t d_84_154_callbacks[]={0x802de450u};
constexpr std::uint32_t d_84_154_headers[]={0x802e3208u};
constexpr const char* d_84_154_services[]={"actor_current"};
constexpr std::uint32_t d_92_155_headers[]={0x802e321cu};
constexpr const char* d_92_155_services[]={"actor_current","actor_send_to_link"};
constexpr const char* d_102_156_services[]={"actor_current"};
constexpr std::uint32_t d_110_157_child_factories[]={0x802de52cu};
constexpr std::uint32_t d_86_158_callbacks[]={0x802deee0u,0x802def4cu,0x802defb8u,0x802df0b4u,0x802df164u};
constexpr std::uint32_t d_86_158_headers[]={0x802e34fcu};
constexpr const char* d_86_158_services[]={"actor_animation_speed","actor_current"};
constexpr std::uint32_t d_113_159_headers[]={0x802e3510u};
constexpr const char* d_113_159_services[]={"actor_current"};
constexpr std::uint32_t d_123_160_headers[]={0x802e3524u};
constexpr const char* d_123_160_services[]={"actor_current"};
constexpr std::uint32_t d_133_161_callbacks[]={0x802df028u};
constexpr std::uint32_t d_133_161_headers[]={0x802e3538u};
constexpr const char* d_133_161_services[]={"actor_current"};
constexpr const char* d_143_162_services[]={"actor_component","actor_component_store","actor_current","actor_model"};
constexpr std::uint32_t d_152_163_headers[]={0x802e34fcu};
constexpr const char* d_152_163_services[]={"actor_current","actor_random_range","actor_visibility"};
constexpr std::uint32_t d_168_164_graphs[]={0x802e354cu};
constexpr const char* d_168_164_services[]={"actor_current","actor_visibility"};
constexpr std::uint32_t d_176_165_callbacks[]={0x802df164u};
constexpr std::uint32_t d_176_165_headers[]={0x802e34fcu};
constexpr const char* d_176_165_services[]={"actor_current","actor_visibility"};
constexpr const char* d_185_166_services[]={"actor_current"};
constexpr std::uint32_t d_190_167_child_factories[]={0x802df240u};
constexpr std::uint32_t d_83_168_callbacks[]={0x802df338u};
constexpr std::uint32_t d_83_168_headers[]={0x802e3634u};
constexpr const char* d_83_168_services[]={"actor_current","actor_model","actor_position_store","actor_random_range"};
constexpr std::uint32_t d_95_169_child_factories[]={0x8035e0d4u};
constexpr const char* d_95_169_services[]={"actor_current"};
constexpr std::uint32_t d_103_170_child_factories[]={0x802df378u};
constexpr const char* d_264_171_services[]={"actor_current"};
constexpr std::uint32_t d_294_172_callbacks[]={0x802d7290u,0x802d7458u};
constexpr std::uint32_t d_294_172_graphs[]={0x802e10a4u};
constexpr std::uint32_t d_294_172_headers[]={0x802e0fd8u};
constexpr const char* d_294_172_services[]={"actor_current"};
constexpr std::uint32_t d_309_173_callbacks[]={0x802d7510u};
constexpr std::uint32_t d_309_173_headers[]={0x802e0fd8u};
constexpr const char* d_309_173_services[]={"actor_current","actor_reference_assign"};
constexpr std::uint32_t d_325_174_graphs[]={0x802e10c4u};
constexpr std::uint32_t d_325_174_weighted_states[]={0x802e1484u};
constexpr std::uint32_t d_325_174_headers[]={0x802e0fd8u};
constexpr const char* d_325_174_services[]={"actor_current"};
constexpr std::uint32_t d_335_175_graphs[]={0x802e10c4u};
constexpr std::uint32_t d_335_175_weighted_states[]={0x802e1484u};
constexpr std::uint32_t d_335_175_headers[]={0x802e0fecu};
constexpr const char* d_335_175_services[]={"actor_current"};
constexpr std::uint32_t d_344_176_callbacks[]={0x802d7510u,0x802d7698u};
constexpr std::uint32_t d_344_176_graphs[]={0x802e1154u};
constexpr std::uint32_t d_344_176_weighted_states[]={0x802e1484u};
constexpr std::uint32_t d_344_176_headers[]={0x802e1000u};
constexpr const char* d_344_176_services[]={"actor_current"};
constexpr const char* d_357_177_services[]={"actor_current"};
constexpr std::uint32_t d_365_178_callbacks[]={0x802d7a34u};
constexpr std::uint32_t d_365_178_graphs[]={0x802e13f4u};
constexpr std::uint32_t d_365_178_headers[]={0x802e103cu,0x802e1050u,0x802e1064u};
constexpr const char* d_365_178_services[]={"actor_current"};
constexpr std::uint32_t d_387_179_graphs[]={0x802e13f4u};
constexpr std::uint32_t d_387_179_weighted_states[]={0x802e1484u};
constexpr std::uint32_t d_387_179_headers[]={0x802e1028u};
constexpr const char* d_387_179_services[]={"actor_current"};
constexpr std::uint32_t d_397_180_graphs[]={0x802e13f4u};
constexpr std::uint32_t d_397_180_weighted_states[]={0x802e1484u};
constexpr std::uint32_t d_397_180_headers[]={0x802e1028u};
constexpr const char* d_397_180_services[]={"actor_current"};
constexpr std::uint32_t d_407_181_callbacks[]={0x802d7510u,0x802d798cu,0x802d7a34u};
constexpr std::uint32_t d_407_181_graphs[]={0x802e11f4u,0x802e1294u,0x802e1324u};
constexpr std::uint32_t d_407_181_headers[]={0x802e1078u};
constexpr const char* d_407_181_services[]={"actor_current"};
constexpr std::uint32_t d_429_182_headers[]={0x802e1014u};
constexpr const char* d_429_182_services[]={"actor_current"};
constexpr std::uint32_t d_439_183_callbacks[]={0x802d78a4u};
constexpr const char* d_439_183_services[]={"actor_current"};
constexpr std::uint32_t d_443_184_callbacks[]={0x802d7510u};
constexpr std::uint32_t d_443_184_graphs[]={0x802e1374u};
constexpr const char* d_443_184_services[]={"actor_current","actor_reference_assign"};
constexpr std::uint32_t d_452_185_callbacks[]={0x802d7510u,0x802d7b08u};
constexpr std::uint32_t d_452_185_graphs[]={0x802e13c4u};
constexpr std::uint32_t d_452_185_headers[]={0x802e1000u};
constexpr const char* d_452_185_services[]={"actor_current"};
constexpr const char* d_461_186_services[]={"actor_current"};
constexpr std::uint32_t d_468_187_callbacks[]={0x802d7510u,0x802d7c30u,0x802d7d84u};
constexpr std::uint32_t d_468_187_graphs[]={0x802e1434u};
constexpr std::uint32_t d_468_187_headers[]={0x802e108cu};
constexpr const char* d_468_187_services[]={"actor_current","actor_send_addressed"};
constexpr const char* d_489_188_services[]={"actor_current"};
constexpr std::uint32_t d_505_189_callbacks[]={0x802d7a34u};
constexpr std::uint32_t d_505_189_graphs[]={0x802e13f4u};
constexpr std::uint32_t d_505_189_headers[]={0x802e103cu,0x802e1050u,0x802e1064u};
constexpr const char* d_505_189_services[]={"actor_current"};
constexpr std::uint32_t d_526_190_callbacks[]={0x802d7510u,0x802d7c30u};
constexpr std::uint32_t d_526_190_graphs[]={0x802e1434u,0x802e14a4u};
constexpr std::uint32_t d_526_190_headers[]={0x802e0fd8u,0x802e108cu};
constexpr const char* d_526_190_services[]={"actor_current","actor_send_addressed"};
constexpr std::uint32_t d_549_191_headers[]={0x802e103cu,0x802e1050u};
constexpr const char* d_549_191_services[]={"actor_current"};
constexpr std::uint32_t d_563_192_child_factories[]={0x802d7f28u};
constexpr std::uint32_t d_239_193_child_factories[]={0x802df378u};
constexpr const char* d_239_193_services[]={"actor_add_one","actor_component_store","actor_current","actor_current_block","actor_model","actor_position_component"};
constexpr const char* d_268_194_services[]={"actor_aux_read","actor_current","actor_item","actor_model","actor_player_model","actor_random_range","actor_root_read","actor_send_to_link","actor_vector_distance"};
constexpr const char* d_300_195_services[]={"lifecycle_world_add"};
constexpr const char* d_304_196_services[]={"lifecycle_world_change"};
constexpr const char* d_308_197_services[]={"lifecycle_world_remove"};
const ActorHelperDependency d_312_198_helpers[]={{0x802d6344u,"src/volcano/main.c",300,302,"void volcano_PokemonAdd(IsleVolcanoLifecycleSource* l, IsleVolcanoBlock* arg0, IsleVolcanoBlock* arg1)"},{0x802d6368u,"src/volcano/main.c",304,306,"void volcano_PokemonChangeBlock(IsleVolcanoLifecycleSource* l, IsleVolcanoBlock* arg0, IsleVolcanoBlock* arg1)"},{0x802d638cu,"src/volcano/main.c",308,310,"void volcano_PokemonRemove(IsleVolcanoLifecycleSource* l, IsleVolcanoBlock* arg0)"}};
constexpr const char* d_312_198_services[]={"lifecycle_actor_initialize_range","lifecycle_height_map","lifecycle_sky_follow","lifecycle_world_create"};
constexpr const char* d_319_199_services[]={"lifecycle_bgm_stop","lifecycle_camera_remove","lifecycle_detector_disable","lifecycle_end_level","lifecycle_environment_cleanup","lifecycle_player_stop","lifecycle_scene_end","lifecycle_sound_stop_all","lifecycle_world_destroy"};
constexpr const char* d_332_200_services[]={"lifecycle_current"};
const ActorHelperDependency d_338_202_helpers[]={{0x802d643cu,"src/volcano/main.c",319,330,"void volcano_EndLevel(IsleVolcanoLifecycleSource* l, int32_t endLevelReason)"}};
constexpr const char* d_338_202_services[]={"lifecycle_block_next","lifecycle_block_position","lifecycle_displace_items"};
constexpr const char* d_354_203_services[]={"lifecycle_fx_allocate","lifecycle_fx_bank","lifecycle_fx_camera","lifecycle_fx_dither","lifecycle_fx_reference","lifecycle_load_asset","lifecycle_main_camera"};
const ActorHelperDependency d_373_204_helpers[]={{0x802e0c28u,"src/volcano/intro/code.c",135,144,"void volcano_StartIntro(IsleVolcanoLifecycleSource* l)"},{0x802d61acu,"src/volcano/main.c",268,298,"void volcano_HandleCollision(IsleGameObject* obj, IsleActorGround* groundResult)"},{0x802d63b0u,"src/volcano/main.c",312,317,"void volcano_InitWorld(IsleVolcanoLifecycleSource* l)"},{0x802d643cu,"src/volcano/main.c",319,330,"void volcano_EndLevel(IsleVolcanoLifecycleSource* l, int32_t endLevelReason)"},{0x802d64a0u,"src/volcano/main.c",332,333,"void volcano_func_802D64A0_7276A0(IsleGameObject* obj)"},{0x802d64a8u,"src/volcano/main.c",335,336,"void volcano_Pause(IsleVolcanoLifecycleSource* l, int32_t arg0)"},{0x802d64b0u,"src/volcano/main.c",338,352,"void volcano_ExitBlock(IsleVolcanoLifecycleSource* l, IsleVolcanoBlock* arg0)"},{0x802d6544u,"src/volcano/main.c",354,371,"void volcano_LoadEffects(IsleVolcanoLifecycleSource* l)"},{0x802dfb80u,"src/volcano/sounds.c",14,123,"void volcano_UpdateSounds(IsleVolcanoActorSource* s, int32_t blockCount, float blockPart)"}};
constexpr const char* d_373_204_services[]={"lifecycle_actors_initialize","lifecycle_add_object","lifecycle_apply_screen","lifecycle_background_color","lifecycle_cameras_create","lifecycle_depth_allocate","lifecycle_detector_create","lifecycle_detector_enable","lifecycle_end_callback","lifecycle_engine_initialize","lifecycle_environment_initialize","lifecycle_fx_reset_bank","lifecycle_pause_callback","lifecycle_ui_initialize"};
constexpr const char* d_401_206_services[]={"lifecycle_illegal_copy","lifecycle_player_flag"};
const ActorHelperDependency d_407_207_helpers[]={{0x802d6788u,"src/volcano/main.c",401,405,"void volcano_CheckIllegalCopy(IsleVolcanoLifecycleSource* l)"}};
constexpr const char* d_407_207_services[]={"lifecycle_near_clipping","lifecycle_notice","lifecycle_photo_count","lifecycle_set_intervals","lifecycle_setup_scene"};
constexpr std::uint32_t d_423_208_child_factories[]={0x802dc018u};
constexpr const char* d_423_208_services[]={"actor_add_one","actor_component_store","actor_current","actor_current_block","actor_model","actor_position_component"};
constexpr std::uint32_t d_453_209_child_factories[]={0x802db558u};
constexpr const char* d_453_209_services[]={"actor_add_one","actor_component_store","actor_current","actor_current_block","actor_model","actor_position_component"};
constexpr const char* d_483_210_services[]={"actor_change_block_ground","actor_current"};
constexpr std::uint32_t d_487_211_child_factories[]={0x802dd7acu};
constexpr const char* d_487_211_services[]={"actor_add_one","actor_component_store","actor_current","actor_current_block","actor_model","actor_position_component"};
constexpr std::uint32_t d_516_212_child_factories[]={0x802df240u};
constexpr const char* d_516_212_services[]={"actor_add_geo","actor_current"};
constexpr std::uint32_t d_524_213_child_factories[]={0x802df240u};
constexpr const char* d_524_213_services[]={"actor_add_geo","actor_current","actor_misc_object_set"};
constexpr std::uint32_t d_533_214_child_factories[]={0x802df240u};
constexpr const char* d_533_214_services[]={"actor_add_geo","actor_component","actor_component_store","actor_current","actor_model"};
constexpr std::uint32_t d_544_215_child_factories[]={0x802df240u};
constexpr const char* d_544_215_services[]={"actor_add_geo","actor_component","actor_component_store","actor_current","actor_model"};
constexpr std::uint32_t d_555_216_child_factories[]={0x802df240u};
constexpr const char* d_555_216_services[]={"actor_add_geo","actor_current"};
constexpr std::uint32_t d_563_217_child_factories[]={0x802df240u};
constexpr const char* d_563_217_services[]={"actor_add_geo","actor_component","actor_component_store","actor_current","actor_model"};
constexpr std::uint32_t d_574_218_callbacks[]={0x802d6864u,0x802d693cu};
constexpr const char* d_574_218_services[]={"actor_current","actor_random_range"};
constexpr const char* d_595_219_services[]={"actor_component","actor_component_store","actor_current","actor_model"};
constexpr const char* d_607_220_services[]={"actor_component","actor_component_store","actor_current","actor_model"};
constexpr const char* d_618_221_services[]={"actor_enter","actor_ground"};
const ActorHelperDependency d_627_222_helpers[]={{0x802d6f38u,"src/volcano/main.c",618,625,"float volcano_GetHeightAt(IsleVolcanoActorSource* s, float x, float z)"}};
constexpr const char* d_627_222_services[]={"actor_component","actor_component_store","actor_current","actor_model"};
constexpr std::uint32_t d_106_223_callbacks[]={0x802de1b0u};
constexpr std::uint32_t d_106_223_graphs[]={0x802e314cu};
constexpr const char* d_106_223_services[]={"actor_current","actor_visibility"};
constexpr std::uint32_t d_117_224_callbacks[]={0x802de0dcu,0x802de144u};
constexpr std::uint32_t d_117_224_headers[]={0x802e30e8u,0x802e30fcu,0x802e3110u,0x802e3124u,0x802e3138u};
constexpr const char* d_117_224_services[]={"actor_component","actor_component_store","actor_current","actor_model","actor_visibility"};
constexpr const char* d_153_225_services[]={"actor_current"};
constexpr const char* d_161_226_services[]={"actor_current"};
constexpr std::uint32_t d_169_227_graphs[]={0x802e317cu};
constexpr const char* d_169_227_services[]={"actor_current"};
constexpr std::uint32_t d_175_228_callbacks[]={0x802de144u};
constexpr std::uint32_t d_175_228_headers[]={0x802e30fcu};
constexpr const char* d_175_228_services[]={"actor_current","actor_visibility"};
constexpr const char* d_186_229_services[]={"actor_current","actor_gb_cycle","actor_gb_pipe_sync","actor_gb_render_mode","actor_render_type_i"};
constexpr std::uint32_t d_198_230_child_factories[]={0x802de34cu};
constexpr std::uint32_t d_67_231_callbacks[]={0x802ddb04u};
constexpr const char* d_67_231_services[]={"actor_current"};
constexpr std::uint32_t d_72_232_graphs[]={0x802e3038u};
constexpr std::uint32_t d_72_232_headers[]={0x802e3010u};
constexpr const char* d_72_232_services[]={"actor_current"};
constexpr std::uint32_t d_80_233_callbacks[]={0x802ddb04u,0x802ddc30u,0x802ddca0u,0x802ddd04u};
constexpr std::uint32_t d_80_233_headers[]={0x802e3010u,0x802e3024u};
constexpr const char* d_80_233_services[]={"actor_current"};
constexpr const char* d_98_234_services[]={"actor_current"};
constexpr const char* d_108_235_services[]={"actor_current"};
constexpr std::uint32_t d_118_236_callbacks[]={0x802d6bb0u,0x802d6c38u,0x802dde00u};
constexpr std::uint32_t d_118_236_headers[]={0x802e3024u};
constexpr const char* d_118_236_services[]={"actor_current","actor_send_to_link"};
constexpr const char* d_140_237_services[]={"actor_current","actor_model","actor_position_component","actor_position_store"};
constexpr std::uint32_t d_154_238_child_factories[]={0x802ddec0u};
constexpr std::uint32_t d_180_239_callbacks[]={0x802d7fa8u,0x802d86e0u};
constexpr const char* d_180_239_services[]={"actor_current"};
constexpr std::uint32_t d_187_240_graphs[]={0x802e15b8u};
constexpr std::uint32_t d_187_240_weighted_states[]={0x802e17f8u};
constexpr std::uint32_t d_187_240_headers[]={0x802e1540u};
constexpr const char* d_187_240_services[]={"actor_current"};
constexpr std::uint32_t d_196_241_callbacks[]={0x802d7fa8u,0x802d80bcu};
constexpr std::uint32_t d_196_241_graphs[]={0x802e15b8u};
constexpr std::uint32_t d_196_241_weighted_states[]={0x802e17f8u};
constexpr std::uint32_t d_196_241_headers[]={0x802e1554u};
constexpr const char* d_196_241_services[]={"actor_current"};
constexpr const char* d_209_242_services[]={"actor_current"};
constexpr std::uint32_t d_217_243_callbacks[]={0x802d7fa8u,0x802d81bcu};
constexpr std::uint32_t d_217_243_graphs[]={0x802e15b8u};
constexpr std::uint32_t d_217_243_weighted_states[]={0x802e17f8u};
constexpr std::uint32_t d_217_243_headers[]={0x802e1568u};
constexpr const char* d_217_243_services[]={"actor_current"};
constexpr const char* d_230_244_services[]={"actor_current"};
constexpr std::uint32_t d_238_245_callbacks[]={0x802d85c0u};
constexpr std::uint32_t d_238_245_graphs[]={0x802e1718u};
constexpr std::uint32_t d_238_245_headers[]={0x802e157cu};
constexpr const char* d_238_245_services[]={"actor_current"};
constexpr std::uint32_t d_247_246_callbacks[]={0x802d85c0u};
constexpr std::uint32_t d_247_246_graphs[]={0x802e1718u};
constexpr std::uint32_t d_247_246_headers[]={0x802e157cu};
constexpr const char* d_247_246_services[]={"actor_current"};
constexpr std::uint32_t d_256_247_callbacks[]={0x802d7fa8u};
constexpr std::uint32_t d_256_247_graphs[]={0x802e1718u};
constexpr std::uint32_t d_256_247_headers[]={0x802e15a4u};
constexpr const char* d_256_247_services[]={"actor_current"};
constexpr std::uint32_t d_265_248_callbacks[]={0x802d7fa8u,0x802d842cu,0x802d8478u,0x802d84fcu};
constexpr std::uint32_t d_265_248_graphs[]={0x802e1648u,0x802e1718u};
constexpr std::uint32_t d_265_248_headers[]={0x802e15a4u};
constexpr const char* d_265_248_services[]={"actor_current"};
constexpr const char* d_281_249_services[]={"actor_current"};
constexpr std::uint32_t d_288_250_headers[]={0x802e1568u};
constexpr const char* d_288_250_services[]={"actor_current"};
constexpr std::uint32_t d_299_251_callbacks[]={0x802d7fa8u,0x802d85c0u};
constexpr std::uint32_t d_299_251_graphs[]={0x802e16c8u};
constexpr std::uint32_t d_299_251_headers[]={0x802e1590u};
constexpr const char* d_299_251_services[]={"actor_current"};
constexpr std::uint32_t d_314_252_callbacks[]={0x802d8368u};
constexpr const char* d_314_252_services[]={"actor_current"};
constexpr std::uint32_t d_318_253_callbacks[]={0x802d7fa8u};
constexpr std::uint32_t d_318_253_graphs[]={0x802e1778u};
constexpr const char* d_318_253_services[]={"actor_current","actor_reference_assign"};
constexpr std::uint32_t d_327_254_callbacks[]={0x802d7fa8u,0x802d8694u};
constexpr std::uint32_t d_327_254_graphs[]={0x802e1748u};
constexpr std::uint32_t d_327_254_headers[]={0x802e1554u};
constexpr const char* d_327_254_services[]={"actor_current"};
constexpr const char* d_336_255_services[]={"actor_current"};
constexpr std::uint32_t d_343_256_graphs[]={0x802e1818u};
constexpr std::uint32_t d_343_256_headers[]={0x802e1540u};
constexpr const char* d_343_256_services[]={"actor_current"};
constexpr std::uint32_t d_352_257_callbacks[]={0x802d8830u};
constexpr std::uint32_t d_352_257_graphs[]={0x802e1818u};
constexpr std::uint32_t d_352_257_headers[]={0x802e157cu};
constexpr const char* d_352_257_services[]={"actor_current"};
constexpr std::uint32_t d_361_258_callbacks[]={0x802d8830u};
constexpr std::uint32_t d_361_258_graphs[]={0x802e1818u};
constexpr std::uint32_t d_361_258_headers[]={0x802e15a4u};
constexpr const char* d_361_258_services[]={"actor_current"};
constexpr std::uint32_t d_370_259_callbacks[]={0x802d8868u,0x802d88d0u};
constexpr const char* d_370_259_services[]={"actor_current"};
constexpr std::uint32_t d_375_260_graphs[]={0x802e1878u};
constexpr std::uint32_t d_375_260_headers[]={0x802e1568u};
constexpr const char* d_375_260_services[]={"actor_current"};
constexpr const char* d_384_261_services[]={"actor_current"};
constexpr std::uint32_t d_392_262_callbacks[]={0x802d8868u};
constexpr std::uint32_t d_392_262_headers[]={0x802e157cu};
constexpr const char* d_392_262_services[]={"actor_current"};
constexpr std::uint32_t d_402_263_callbacks[]={0x802d8868u,0x802d893cu};
constexpr std::uint32_t d_402_263_headers[]={0x802e15a4u};
constexpr const char* d_402_263_services[]={"actor_current"};
constexpr std::uint32_t d_442_264_child_factories[]={0x802d8a5cu};
constexpr std::uint32_t d_49_265_callbacks[]={0x802de648u};
constexpr std::uint32_t d_49_265_graphs[]={0x802e32a4u};
constexpr std::uint32_t d_49_265_headers[]={0x802e3290u};
constexpr const char* d_49_265_services[]={"actor_current","actor_random_range","actor_send_to_link","actor_visibility"};
constexpr const char* d_67_266_services[]={"actor_current"};
constexpr std::uint32_t d_75_267_child_factories[]={0x802de6b4u};
constexpr std::uint32_t d_77_268_child_factories[]={0x802de6b4u};
constexpr const char* d_77_268_services[]={"actor_add_one","actor_component_store","actor_current","actor_current_block","actor_model","actor_position_component"};
constexpr std::uint32_t d_109_269_child_factories[]={0x802de52cu};
constexpr const char* d_109_269_services[]={"actor_add_one","actor_component_store","actor_current","actor_current_block","actor_model","actor_position_component"};
constexpr std::uint32_t d_141_270_callbacks[]={0x802de6f0u,0x802de91cu};
constexpr const char* d_141_270_services[]={"actor_current","actor_visibility"};
constexpr std::uint32_t d_148_271_graphs[]={0x802e3378u};
constexpr const char* d_148_271_services[]={"actor_current"};
constexpr std::uint32_t d_155_272_callbacks[]={0x802de91cu};
constexpr std::uint32_t d_155_272_weighted_states[]={0x802e33b8u};
constexpr const char* d_155_272_services[]={"actor_current","actor_ground","actor_model","actor_root_read"};
constexpr std::uint32_t d_167_273_callbacks[]={0x802de7dcu,0x802dea04u};
constexpr const char* d_167_273_services[]={"actor_current"};
constexpr std::uint32_t d_172_274_graphs[]={0x802e3398u};
constexpr const char* d_172_274_services[]={"actor_current"};
constexpr std::uint32_t d_179_275_child_factories[]={0x802dea44u};
constexpr const char* d_14_276_services[]={"actor_bgm_play","actor_enter","actor_sound_play_params","actor_sound_stop","actor_sound_volume"};
constexpr std::uint32_t d_50_277_callbacks[]={0x802dfa94u};
constexpr const char* d_50_277_services[]={"actor_current"};
constexpr std::uint32_t d_54_278_headers[]={0x802e3780u};
constexpr const char* d_54_278_services[]={"actor_current"};
constexpr const char* d_62_279_services[]={"actor_current","actor_gb_cycle","actor_gb_texture_lut","actor_render_type_d_fogged"};
constexpr std::uint32_t d_68_280_child_factories[]={0x802dfb44u};
constexpr const char* d_209_281_services[]={"actor_block_position","actor_component","actor_current","actor_current_block","actor_model"};
const ActorHelperDependency d_228_282_helpers[]={{0x802d9d00u,"src/volcano/vulpix/code.c",209,226,"int32_t vulpix_CloseToPathEnd(IsleGameObject* obj)"}};
constexpr const char* d_228_282_services[]={"actor_current","actor_send_addressed"};
constexpr const char* d_239_283_services[]={"actor_current"};
constexpr std::uint32_t d_254_284_callbacks[]={0x802d9dfcu,0x802d9e7cu,0x802d9f90u};
constexpr const char* d_254_284_services[]={"actor_current"};
constexpr const char* d_265_285_services[]={"actor_enter"};
constexpr std::uint32_t d_268_286_graphs[]={0x802e2098u};
constexpr std::uint32_t d_268_286_weighted_states[]={0x802e22d8u};
constexpr std::uint32_t d_268_286_headers[]={0x802e1ff8u};
constexpr const char* d_268_286_services[]={"actor_current"};
constexpr std::uint32_t d_277_287_callbacks[]={0x802d9f90u,0x802da0a4u};
constexpr std::uint32_t d_277_287_graphs[]={0x802e2098u};
constexpr std::uint32_t d_277_287_weighted_states[]={0x802e22d8u};
constexpr std::uint32_t d_277_287_headers[]={0x802e200cu};
constexpr const char* d_277_287_services[]={"actor_current"};
constexpr const char* d_291_288_services[]={"actor_current"};
constexpr std::uint32_t d_299_289_callbacks[]={0x802d9f90u,0x802da1a4u};
constexpr std::uint32_t d_299_289_graphs[]={0x802e2098u};
constexpr std::uint32_t d_299_289_weighted_states[]={0x802e22d8u};
constexpr std::uint32_t d_299_289_headers[]={0x802e2020u};
constexpr const char* d_299_289_services[]={"actor_current"};
constexpr const char* d_313_290_services[]={"actor_current"};
constexpr std::uint32_t d_321_291_callbacks[]={0x802da5d8u};
constexpr std::uint32_t d_321_291_headers[]={0x802e2048u,0x802e205cu,0x802e2070u};
constexpr const char* d_321_291_services[]={"actor_current"};
constexpr std::uint32_t d_340_292_callbacks[]={0x802da5d8u};
constexpr std::uint32_t d_340_292_headers[]={0x802e2034u};
constexpr const char* d_340_292_services[]={"actor_current"};
constexpr std::uint32_t d_349_293_callbacks[]={0x802d9f90u};
constexpr std::uint32_t d_349_293_graphs[]={0x802e21d8u};
constexpr std::uint32_t d_349_293_headers[]={0x802e2034u};
constexpr const char* d_349_293_services[]={"actor_current"};
constexpr std::uint32_t d_358_294_callbacks[]={0x802d9f90u,0x802da428u,0x802da4acu};
constexpr std::uint32_t d_358_294_graphs[]={0x802e2118u};
constexpr const char* d_358_294_services[]={"actor_current"};
constexpr std::uint32_t d_370_295_headers[]={0x802e2020u};
constexpr const char* d_370_295_services[]={"actor_current"};
constexpr std::uint32_t d_380_296_callbacks[]={0x802d9f90u,0x802da55cu,0x802da5d8u};
constexpr std::uint32_t d_380_296_graphs[]={0x802e2178u};
constexpr std::uint32_t d_380_296_headers[]={0x802e2084u};
constexpr const char* d_380_296_services[]={"actor_current"};
constexpr const char* d_397_297_services[]={"actor_current"};
constexpr std::uint32_t d_404_298_callbacks[]={0x802da3a0u};
constexpr const char* d_404_298_services[]={"actor_current"};
constexpr std::uint32_t d_408_299_callbacks[]={0x802d9f90u};
constexpr std::uint32_t d_408_299_graphs[]={0x802e2238u};
constexpr const char* d_408_299_services[]={"actor_current","actor_reference_assign"};
constexpr const char* d_417_300_services[]={"actor_enter"};
constexpr const char* d_420_301_services[]={"actor_current"};
constexpr std::uint32_t d_427_302_callbacks[]={0x802d9f90u,0x802da748u};
constexpr std::uint32_t d_427_302_graphs[]={0x802e2278u};
constexpr std::uint32_t d_427_302_headers[]={0x802e2020u};
constexpr const char* d_427_302_services[]={"actor_current"};
constexpr const char* d_443_303_services[]={"actor_current"};
constexpr std::uint32_t d_451_304_callbacks[]={0x802da848u,0x802da8a4u};
constexpr std::uint32_t d_451_304_graphs[]={0x802e2304u};
constexpr std::uint32_t d_451_304_headers[]={0x802e2020u};
constexpr const char* d_451_304_services[]={"actor_current"};
constexpr const char* d_465_305_services[]={"actor_current"};
constexpr std::uint32_t d_473_306_callbacks[]={0x802da930u};
constexpr std::uint32_t d_473_306_graphs[]={0x802e2304u};
constexpr std::uint32_t d_473_306_headers[]={0x802e2084u};
constexpr const char* d_473_306_services[]={"actor_current"};
constexpr const char* d_485_307_services[]={"actor_current"};
constexpr std::uint32_t d_492_308_callbacks[]={0x802da7a4u};
constexpr std::uint32_t d_492_308_graphs[]={0x802e2304u};
constexpr std::uint32_t d_492_308_headers[]={0x802e2048u,0x802e205cu,0x802e2070u};
constexpr const char* d_492_308_services[]={"actor_current"};
constexpr std::uint32_t d_509_309_callbacks[]={0x802da7a4u};
constexpr std::uint32_t d_509_309_headers[]={0x802e2034u};
constexpr const char* d_509_309_services[]={"actor_current"};
constexpr std::uint32_t d_518_310_child_factories[]={0x802daa9cu};
const actor::BehaviorCallbackBinding callbacks[]={
{0x802db5a0u,arcanine_InitialState,d_209_0_callbacks,d_209_0_graphs,{}},
{0x802db660u,arcanine_MoveJump,{},{},{}},
{0x802db6f8u,arcanine_Idle,{},d_242_2_graphs,d_242_2_weighted_states},
{0x802db768u,arcanine_Walk,d_250_3_callbacks,d_250_3_graphs,d_250_3_weighted_states},
{0x802db80cu,arcanine_MoveWalk,{},{},{}},
{0x802db868u,arcanine_Run,d_272_5_callbacks,d_272_5_graphs,d_272_5_weighted_states},
{0x802db90cu,arcanine_MoveRun,{},{},{}},
{0x802db968u,arcanine_Roar,{},d_294_7_graphs,d_294_7_weighted_states},
{0x802db9d8u,arcanine_HitByPesterBall,d_302_8_callbacks,d_302_8_graphs,{}},
{0x802dba48u,arcanine_HitByApple,d_310_9_callbacks,d_310_9_graphs,{}},
{0x802dbab8u,arcanine_AffectedByPesterBall,d_319_10_callbacks,d_319_10_graphs,{}},
{0x802dbb2cu,arcanine_TurnToFume,{},{},{}},
{0x802dbb78u,arcanine_AppleNearby,d_335_12_callbacks,d_335_12_graphs,{}},
{0x802dbc00u,arcanine_RunToApple,{},{},{}},
{0x802dbc84u,arcanine_WithApple,d_357_14_callbacks,d_357_14_graphs,{}},
{0x802dbd34u,arcanine_TurnToApple,{},{},{}},
{0x802dbd94u,arcanine_SearchApple,d_381_16_callbacks,{},{}},
{0x802dbdb8u,arcanine_Restore,d_385_17_callbacks,d_385_17_graphs,{}},
{0x802dbe18u,arcanine_Dance,d_394_18_callbacks,d_394_18_graphs,{}},
{0x802dbe8cu,arcanine_TurnToPlayer,{},{},{}},
{0x802dbed8u,arcanine_RunAwayFromPlayer,d_410_20_callbacks,d_410_20_graphs,{}},
{0x802dbf4cu,arcanine_RunAway,{},{},{}},
{0x802dbfa8u,arcanine_PhotoTaken,{},d_427_22_graphs,d_427_22_weighted_states},
{0x802dd2e0u,charizard_InititalState,d_115_24_callbacks,{},{}},
{0x802dd328u,charizard_Appear,d_121_25_callbacks,{},{}},
{0x802dd3b0u,charizard_MoveOutOfLava,d_132_26_callbacks,{},{}},
{0x802dd4a4u,charizard_CreateSplash,d_148_27_callbacks,{},{}},
{0x802dd500u,charizard_Idle,{},d_159_28_graphs,d_159_28_weighted_states},
{0x802dd570u,charizard_Roar,{},d_168_29_graphs,d_168_29_weighted_states},
{0x802dd5e0u,charizard_HitByPesterBall,d_177_30_callbacks,{},d_177_30_weighted_states},
{0x802dd67cu,charizard_TurnToPlayer,{},{},{}},
{0x802dd6d4u,charizard_HitByApple,d_198_32_callbacks,{},d_198_32_weighted_states},
{0x802dd740u,charizard_AffectedByPesterBall,d_207_33_callbacks,{},{}},
{0x802d8aa0u,charmander_DeleteLater,{},{},{}},
{0x802d8bb8u,charmander_InitialState,d_350_36_callbacks,{},{}},
{0x802d8ca4u,charmander_Idle,{},d_378_37_graphs,d_378_37_weighted_states},
{0x802d8d14u,charmander_Walk,d_387_38_callbacks,d_387_38_graphs,d_387_38_weighted_states},
{0x802d8db8u,charmander_MoveWalk,{},{},{}},
{0x802d8e14u,charmander_Run,d_408_40_callbacks,d_408_40_graphs,d_408_40_weighted_states},
{0x802d8eb8u,charmander_MoveRun,{},{},{}},
{0x802d8f14u,charmander_Howl,{},d_429_42_graphs,d_429_42_weighted_states},
{0x802d8f94u,charmander_HowlInResponse,{},d_439_43_graphs,d_439_43_weighted_states},
{0x802d9004u,charmander_HowlingEnded,{},{},{}},
{0x802d9074u,charmander_HitOrBurnt,d_456_45_callbacks,d_456_45_graphs,{}},
{0x802d924cu,charmander_HitByApple,d_491_47_callbacks,d_491_47_graphs,{}},
{0x802d930cu,charmander_RunAway,d_503_48_callbacks,d_503_48_graphs,{}},
{0x802d9380u,charmander_MoveRunAway,{},{},{}},
{0x802d93dcu,charmander_AppleNearby,d_520_50_callbacks,d_520_50_graphs,{}},
{0x802d9464u,charmander_RunToApple,{},{},{}},
{0x802d94e8u,charmander_WithApple,d_542_52_callbacks,d_542_52_graphs,{}},
{0x802d95acu,charmander_MoveTurnToTarget,{},{},{}},
{0x802d9614u,charmander_SearchApple,d_565_54_callbacks,{},{}},
{0x802d9638u,charmander_Roar,d_569_55_callbacks,d_569_55_graphs,d_569_55_weighted_states},
{0x802d96acu,charmander_MoveTurnToPlayer,{},{},{}},
{0x802d96f8u,charmander_Dance,d_585_57_callbacks,d_585_57_graphs,{}},
{0x802d976cu,charmander_MoveTurnToPlayer2,{},{},{}},
{0x802d97b8u,charmander_SummonFriend,d_601_59_callbacks,d_601_59_graphs,{}},
{0x802d9900u,charmander_InitGroup3,{},d_628_60_graphs,{}},
{0x802d9950u,charmander_InitGroup4,{},d_636_61_graphs,{}},
{0x802d99a0u,charmander_SpawnFriend,d_644_62_callbacks,d_644_62_graphs,{}},
{0x802d9a80u,charmander_RunToSummoner,{},{},{}},
{0x802d9b44u,charmander_RunToSummoner2,{},{},{}},
{0x802d9b8cu,charmander_PlayEvolveEffect,{},d_689_65_graphs,{}},
{0x802d9bf4u,charmander_BurntWhileFalling,d_698_66_callbacks,{},{}},
{0x802d9c30u,charmander_LieOnGround,d_705_67_callbacks,{},{}},
{0x802d9c84u,charmander_EvolveIntoCharmeleon,{},{},{}},
{0x802dc060u,charmeleon_DeleteLater,{},{},{}},
{0x802dc108u,charmeleon_InitialState,d_259_71_callbacks,{},d_259_71_weighted_states},
{0x802dc170u,charmeleon_Walk,d_268_72_callbacks,d_268_72_graphs,d_268_72_weighted_states},
{0x802dc1f8u,charmeleon_Run,d_279_73_callbacks,d_279_73_graphs,d_279_73_weighted_states},
{0x802dc280u,charmeleon_FollowPath,{},{},{}},
{0x802dc3a0u,charmeleon_LookAround,{},d_317_75_graphs,d_317_75_weighted_states},
{0x802dc410u,charmeleon_Roar,{},d_326_76_graphs,d_326_76_weighted_states},
{0x802dc480u,charmeleon_Idle,{},d_335_77_graphs,d_335_77_weighted_states},
{0x802dc4f0u,charmeleon_BounceOfApple,d_344_78_callbacks,d_344_78_graphs,{}},
{0x802dc590u,charmeleon_BounceOfPesterBall,d_360_79_callbacks,d_360_79_graphs,{}},
{0x802dc6b4u,charmeleon_BounceBack,{},{},{}},
{0x802dc758u,charmeleon_HitByPesterBall,d_404_81_callbacks,{},{}},
{0x802dc7a8u,charmeleon_HitByApple,d_412_82_callbacks,{},{}},
{0x802dc7f8u,charmeleon_Return,d_420_83_callbacks,{},{}},
{0x802dc850u,charmeleon_StepToOrigPos,{},d_427_84_graphs,d_427_84_weighted_states},
{0x802dc924u,charmeleon_RunToOrigPos,{},{},{}},
{0x802dc99cu,charmeleon_EvolveIntoCharizard,d_459_86_callbacks,{},{}},
{0x802dca28u,charmeleon_DiveInLava,{},{},{}},
{0x802dcb44u,charmeleon_EvolvedInit,d_492_88_callbacks,{},{}},
{0x802dcbd0u,charmeleon_EvolvedIdle,{},d_504_89_graphs,d_504_89_weighted_states},
{0x802dcc40u,charmeleon_EvolvedWalk,d_512_90_callbacks,d_512_90_graphs,d_512_90_weighted_states},
{0x802dcce4u,charmeleon_EvolvedMoveWalk,{},{},{}},
{0x802dcd40u,charmeleon_EvolvedRun,d_534_92_callbacks,d_534_92_graphs,d_534_92_weighted_states},
{0x802dcde4u,charmeleon_EvolvedMoveRun,{},{},{}},
{0x802dce40u,charmeleon_EvolvedHitByPesterBall,d_556_94_callbacks,d_556_94_graphs,{}},
{0x802dcf44u,charmeleon_EvolvedHit,d_577_95_callbacks,{},{}},
{0x802dcfacu,charmeleon_EvolvedSearchApple,d_586_96_callbacks,d_586_96_graphs,{}},
{0x802dd034u,charmeleon_EvolvedRunToApple,{},{},{}},
{0x802dd0e8u,charmeleon_EvolvedWithApple,d_608_98_callbacks,d_608_98_graphs,{}},
{0x802dd1c0u,charmeleon_EvolvedTurnToApple,{},{},{}},
{0x802dea80u,evolution_controller_InitialState,d_78_102_callbacks,d_78_102_graphs,{}},
{0x802deafcu,evolution_controller_CharmeleonFellInLava,d_91_103_callbacks,{},{}},
{0x802debb0u,evolution_controller_EvolveIntoCharizard,{},d_106_104_graphs,{}},
{0x802debf0u,evolution_controller_CharizardAppeared,{},{},{}},
{0x802dec78u,evolution_controller_EvolveIntoCharmeleon,{},{},{}},
{0x802df3b0u,volcano_gate_InitialState,d_97_109_callbacks,{},{}},
{0x802df3d4u,volcano_gate_Idle,{},d_101_110_graphs,{}},
{0x802df42cu,volcano_gate_PlayerApproaching,d_109_111_callbacks,{},{}},
{0x802df534u,volcano_gate_LowerVolume,{},{},{}},
{0x802df7f4u,volcano_gate_StopSounds,{},{},{}},
{0x802df880u,volcano_gate_Cutscene,d_173_115_callbacks,{},{}},
{0x802daae0u,growlithe_InitialState,d_210_117_callbacks,d_210_117_graphs,{}},
{0x802dab90u,growlithe_MoveJump,{},{},{}},
{0x802dac38u,growlithe_Idle,{},d_241_119_graphs,d_241_119_weighted_states},
{0x802daca8u,growlithe_Walk,d_250_120_callbacks,d_250_120_graphs,d_250_120_weighted_states},
{0x802dad4cu,growlithe_MoveWalk,{},{},{}},
{0x802dada8u,growlithe_Run,d_272_122_callbacks,d_272_122_graphs,d_272_122_weighted_states},
{0x802dae4cu,growlithe_MoveRun,{},{},{}},
{0x802daea8u,growlithe_Roar,{},d_294_124_graphs,d_294_124_weighted_states},
{0x802daf18u,growlithe_HitByPesterBall,d_303_125_callbacks,d_303_125_graphs,{}},
{0x802daf88u,growlithe_HitByApple,d_312_126_callbacks,d_312_126_graphs,{}},
{0x802daff8u,growlithe_AffectedByPesterBall,d_321_127_callbacks,d_321_127_graphs,{}},
{0x802db06cu,growlithe_TurnToFume,{},{},{}},
{0x802db0b8u,growlithe_AppleNearby,d_337_129_callbacks,d_337_129_graphs,{}},
{0x802db140u,growlithe_RunToApple,{},{},{}},
{0x802db1c4u,growlithe_WithApple,d_359_131_callbacks,d_359_131_graphs,{}},
{0x802db274u,growlithe_TurnToApple,{},{},{}},
{0x802db2d4u,growlithe_SearchApple,d_383_133_callbacks,{},{}},
{0x802db2f8u,growlithe_Restore,d_387_134_callbacks,d_387_134_graphs,{}},
{0x802db358u,growlithe_Dance,d_396_135_callbacks,d_396_135_graphs,{}},
{0x802db3ccu,growlithe_TurnToPlayer,{},{},{}},
{0x802db418u,growlithe_RunAwayFromPlayer,d_412_137_callbacks,d_412_137_graphs,{}},
{0x802db48cu,growlithe_RunAway,{},{},{}},
{0x802db4e8u,growlithe_PhotoTaken,{},d_429_139_graphs,d_429_139_weighted_states},
{0x802dd7f0u,growlithe_spawner_InitialState,d_54_141_callbacks,{},{}},
{0x802dd8f8u,growlithe_spawner_Idle,{},d_75_142_graphs,{}},
{0x802dd954u,growlithe_spawner_SpawnPokemon,d_83_143_callbacks,{},{}},
{0x802dda0cu,growlithe_spawner_SendCommands,{},{},{}},
{0x802de390u,koffing_smoke_InitialState,d_75_153_callbacks,{},{}},
{0x802de3fcu,koffing_smoke_RevealFace,d_84_154_callbacks,{},{}},
{0x802de450u,koffing_smoke_Hide,{},{},{}},
{0x802de4c0u,koffing_smoke_Rise,{},{},{}},
{0x802dede0u,lava_splash_InitialState,d_86_158_callbacks,{},{}},
{0x802deee0u,lava_splash_Type1,{},{},{}},
{0x802def4cu,lava_splash_Type5,{},{},{}},
{0x802defb8u,lava_splash_Type2,d_133_161_callbacks,{},{}},
{0x802df028u,lava_splash_Type2Inflate,{},{},{}},
{0x802df0b4u,lava_splash_Type3,{},{},{}},
{0x802df164u,lava_splash_Type4,{},d_168_164_graphs,{}},
{0x802df1b4u,lava_splash_StartType4,d_176_165_callbacks,{},{}},
{0x802df214u,lava_splash_EndType4,{},{},{}},
{0x802df280u,volcano_magikarp_InitialState,d_83_168_callbacks,{},{}},
{0x802df338u,func_802DF338_730538,{},{},{}},
{0x802d7290u,magmar_DeleteLater,{},{},{}},
{0x802d73a8u,magmar_InitialState,d_294_172_callbacks,d_294_172_graphs,{}},
{0x802d7458u,magmar_LoneInit,d_309_173_callbacks,{},{}},
{0x802d7510u,magmar_Idle,{},d_325_174_graphs,d_325_174_weighted_states},
{0x802d7584u,magmar_LookAround,{},d_335_175_graphs,d_335_175_weighted_states},
{0x802d75f4u,magmar_Walk,d_344_176_callbacks,d_344_176_graphs,d_344_176_weighted_states},
{0x802d7698u,magmar_MoveWalk,{},{},{}},
{0x802d76f4u,magmar_HitByPesterBall,d_365_178_callbacks,d_365_178_graphs,{}},
{0x802d77bcu,magmar_HitByApple,{},d_387_179_graphs,d_387_179_weighted_states},
{0x802d7830u,magmar_AffectedByPesterBall,{},d_397_180_graphs,d_397_180_weighted_states},
{0x802d78a4u,magmar_AppleNearby,d_407_181_callbacks,d_407_181_graphs,{}},
{0x802d798cu,magmar_RunToApple,{},{},{}},
{0x802d7a10u,magmar_SearchApple,d_439_183_callbacks,{},{}},
{0x802d7a34u,magmar_Recover,d_443_184_callbacks,d_443_184_graphs,{}},
{0x802d7a94u,magmar_HearsFlute,d_452_185_callbacks,d_452_185_graphs,{}},
{0x802d7b08u,magmar_TurnToPlayer,{},{},{}},
{0x802d7b54u,magmar_InteractWithPokemon,d_468_187_callbacks,d_468_187_graphs,{}},
{0x802d7c30u,magmar_TurnToRival,{},{},{}},
{0x802d7cb4u,magmar_BurnedDown,d_505_189_callbacks,d_505_189_graphs,{}},
{0x802d7d84u,magmar_BurnCharmander,d_526_190_callbacks,d_526_190_graphs,{}},
{0x802d7e90u,magmar_CharmanderEvolved,{},{},{}},
{0x802d60e0u,volcano_SpawnMagikarp,{},{},{}},
{0x802d6864u,volcano_SpawnArcanine,{},{},{}},
{0x802d693cu,volcano_SpawnGrowlithe,{},{},{}},
{0x802d6b2cu,volcano_CreateSplashFromGrowlitheSpawner,{},{},{}},
{0x802d6b64u,volcano_CreateSplashFromGrowlitheOrArcanine,{},{},{}},
{0x802d6bb0u,volcano_CreateSplashFromMoltres,{},{},{}},
{0x802d6c38u,volcano_CreateSplashFromMoltresEgg,{},{},{}},
{0x802d6cc0u,volcano_CreateSplashFromCharmeleon,{},{},{}},
{0x802d6cfcu,volcano_CreateSplashFromCharizard,{},{},{}},
{0x802ddf00u,moltres_InitialState,d_106_223_callbacks,d_106_223_graphs,{}},
{0x802ddf7cu,moltres_Appear,d_117_224_callbacks,{},{}},
{0x802de0dcu,moltres_FlyAway,{},{},{}},
{0x802de144u,moltres_Fly,{},{},{}},
{0x802de1b0u,moltres_UnusedBehavior,{},d_169_227_graphs,{}},
{0x802de1f0u,moltres_UnusedPlayerIsClose,d_175_228_callbacks,{},{}},
{0x802ddad0u,moltres_egg_InitialState,d_67_231_callbacks,{},{}},
{0x802ddb04u,moltres_egg_Idle,{},d_72_232_graphs,{}},
{0x802ddb60u,moltres_egg_Hit,d_80_233_callbacks,{},{}},
{0x802ddc30u,moltres_egg_FallIntoLava,{},{},{}},
{0x802ddca0u,moltres_egg_Jump,{},{},{}},
{0x802ddd04u,moltres_egg_SpawnMoltres,d_118_236_callbacks,{},{}},
{0x802dde00u,moltres_egg_DrownInLava,{},{},{}},
{0x802d7f60u,rapidash_InitialState,d_180_239_callbacks,{},{}},
{0x802d7fa8u,rapidash_Idle,{},d_187_240_graphs,d_187_240_weighted_states},
{0x802d8018u,rapidash_Walk,d_196_241_callbacks,d_196_241_graphs,d_196_241_weighted_states},
{0x802d80bcu,rapidash_MoveWalk,{},{},{}},
{0x802d8118u,rapidash_Run,d_217_243_callbacks,d_217_243_graphs,d_217_243_weighted_states},
{0x802d81bcu,rapidash_MoveRun,{},{},{}},
{0x802d8218u,rapidash_HitByPesterBall,d_238_245_callbacks,d_238_245_graphs,{}},
{0x802d8288u,rapidash_HitByApple,d_247_246_callbacks,d_247_246_graphs,{}},
{0x802d82f8u,rapidash_AffectedByPesterBall,d_256_247_callbacks,d_256_247_graphs,{}},
{0x802d8368u,rapidash_AppleNearby,d_265_248_callbacks,d_265_248_graphs,{}},
{0x802d842cu,rapidash_TurnToApple,{},{},{}},
{0x802d8478u,rapidash_RunToApple,{},{},{}},
{0x802d84fcu,rapidash_EatApple,d_299_251_callbacks,d_299_251_graphs,{}},
{0x802d859cu,rapidash_SearchApple,d_314_252_callbacks,{},{}},
{0x802d85c0u,rapidash_Recover,d_318_253_callbacks,d_318_253_graphs,{}},
{0x802d8620u,rapidash_HearsFlute,d_327_254_callbacks,d_327_254_graphs,{}},
{0x802d8694u,rapidash_TurnToPlayer,{},{},{}},
{0x802d86e0u,rapidash_IdleBeforeRun,{},d_343_256_graphs,{}},
{0x802d8750u,rapidash_HitBeforeRun,d_352_257_callbacks,d_352_257_graphs,{}},
{0x802d87c0u,rapidash_AffectedByItemBeforeRun,d_361_258_callbacks,d_361_258_graphs,{}},
{0x802d8830u,rapidash_StartRun,d_370_259_callbacks,{},{}},
{0x802d8868u,rapidash_RunAlongPath,{},d_375_260_graphs,{}},
{0x802d88d0u,rapidash_MoveRunAlongPath,{},{},{}},
{0x802d893cu,rapidash_RunningHit,d_392_262_callbacks,{},{}},
{0x802d89b0u,rapidash_RunningAffectedByItem,d_402_263_callbacks,{},{}},
{0x802de570u,smoke_puff_InitialState,d_49_265_callbacks,d_49_265_graphs,{}},
{0x802de648u,smoke_puff_Rise,{},{},{}},
{0x802de6f0u,smoke_spawner_CreateSmokePuff,{},{},{}},
{0x802de7dcu,smoke_spawner_CreateKoffingSmoke,{},{},{}},
{0x802de8c8u,smoke_spawner_InitialState,d_141_270_callbacks,{},{}},
{0x802de91cu,smoke_spawner_Idle,{},d_148_271_graphs,{}},
{0x802de95cu,smoke_spawner_PesterBallThrown,d_155_272_callbacks,{},d_155_272_weighted_states},
{0x802de9c4u,smoke_spawner_SpawnKoffingSmoke,d_167_273_callbacks,{},{}},
{0x802dea04u,smoke_spawner_WaitSmokeFade,{},d_172_274_graphs,{}},
{0x802dfa70u,volcano_smoke_InitialState,d_50_277_callbacks,{},{}},
{0x802dfa94u,volcano_smoke_Idle,{},{},{}},
{0x802d9dfcu,vulpix_SendRunCommand,{},{},{}},
{0x802d9e7cu,vulpix_DeleteLater,{},{},{}},
{0x802d9f24u,vulpix_InitialState,d_254_284_callbacks,{},{}},
{0x802d9f90u,vulpix_Idle,{},d_268_286_graphs,d_268_286_weighted_states},
{0x802da000u,vulpix_Walk,d_277_287_callbacks,d_277_287_graphs,d_277_287_weighted_states},
{0x802da0a4u,vulpix_MoveWalk,{},{},{}},
{0x802da100u,vulpix_Run,d_299_289_callbacks,d_299_289_graphs,d_299_289_weighted_states},
{0x802da1a4u,vulpix_MoveRun,{},{},{}},
{0x802da200u,vulpix_HitByPesterBall,d_321_291_callbacks,{},{}},
{0x802da2c8u,vulpix_HitByApple,d_340_292_callbacks,{},{}},
{0x802da330u,vulpix_AffectedByPesterBall,d_349_293_callbacks,d_349_293_graphs,{}},
{0x802da3a0u,vulpix_AppleNearby,d_358_294_callbacks,d_358_294_graphs,{}},
{0x802da428u,vulpix_RunToApple,{},{},{}},
{0x802da4acu,vulpix_EatApple,d_380_296_callbacks,d_380_296_graphs,{}},
{0x802da55cu,vulpix_TurnToApple,{},{},{}},
{0x802da5b4u,vulpix_SearchApple,d_404_298_callbacks,{},{}},
{0x802da5d8u,vulpix_Recover,d_408_299_callbacks,d_408_299_graphs,{}},
{0x802da640u,vulpix_unused_TurnToPlayer,{},{},{}},
{0x802da68cu,vulpix_RunAwayFromPlayer,d_427_302_callbacks,d_427_302_graphs,{}},
{0x802da748u,vulpix_MoveRunAway,{},{},{}},
{0x802da7a4u,vulpix_RunAlongPath,d_451_304_callbacks,d_451_304_graphs,{}},
{0x802da848u,vulpix_MoveRunToPathEnd,{},{},{}},
{0x802da8a4u,vulpix_IdleAtPathEnd,d_473_306_callbacks,d_473_306_graphs,{}},
{0x802da930u,vulpix_TurnToPlayer,{},{},{}},
{0x802da97cu,vulpix_RunningHitByPesterBall,d_492_308_callbacks,d_492_308_graphs,{}},
{0x802daa34u,vulpix_RunningHit,d_509_309_callbacks,{},{}}
};
const ActorProviderDependency dependencies[]={
{0x802db5a0u,"src/volcano/arcanine/code.c",209,226,"POKEMON_FUNC(arcanine_InitialState)","void arcanine_InitialState(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_209_0_callbacks,d_209_0_graphs,{},d_209_0_headers,{},{},d_209_0_services},
{0x802db660u,"src/volcano/arcanine/code.c",228,240,"POKEMON_FUNC(arcanine_MoveJump)","void arcanine_MoveJump(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},d_228_1_helpers,d_228_1_services},
{0x802db6f8u,"src/volcano/arcanine/code.c",242,248,"POKEMON_FUNC(arcanine_Idle)","void arcanine_Idle(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_242_2_graphs,d_242_2_weighted_states,d_242_2_headers,{},{},d_242_2_services},
{0x802db768u,"src/volcano/arcanine/code.c",250,262,"POKEMON_FUNC(arcanine_Walk)","void arcanine_Walk(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_250_3_callbacks,d_250_3_graphs,d_250_3_weighted_states,d_250_3_headers,{},{},d_250_3_services},
{0x802db80cu,"src/volcano/arcanine/code.c",264,270,"POKEMON_FUNC(arcanine_MoveWalk)","void arcanine_MoveWalk(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_264_4_services},
{0x802db868u,"src/volcano/arcanine/code.c",272,284,"POKEMON_FUNC(arcanine_Run)","void arcanine_Run(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_272_5_callbacks,d_272_5_graphs,d_272_5_weighted_states,d_272_5_headers,{},{},d_272_5_services},
{0x802db90cu,"src/volcano/arcanine/code.c",286,292,"POKEMON_FUNC(arcanine_MoveRun)","void arcanine_MoveRun(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_286_6_services},
{0x802db968u,"src/volcano/arcanine/code.c",294,300,"POKEMON_FUNC(arcanine_Roar)","void arcanine_Roar(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_294_7_graphs,d_294_7_weighted_states,d_294_7_headers,{},{},d_294_7_services},
{0x802db9d8u,"src/volcano/arcanine/code.c",302,308,"POKEMON_FUNC(arcanine_HitByPesterBall)","void arcanine_HitByPesterBall(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_302_8_callbacks,d_302_8_graphs,{},d_302_8_headers,{},{},d_302_8_services},
{0x802dba48u,"src/volcano/arcanine/code.c",310,317,"POKEMON_FUNC(arcanine_HitByApple)","void arcanine_HitByApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_310_9_callbacks,d_310_9_graphs,{},d_310_9_headers,{},{},d_310_9_services},
{0x802dbab8u,"src/volcano/arcanine/code.c",319,326,"POKEMON_FUNC(arcanine_AffectedByPesterBall)","void arcanine_AffectedByPesterBall(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_319_10_callbacks,d_319_10_graphs,{},d_319_10_headers,{},{},d_319_10_services},
{0x802dbb2cu,"src/volcano/arcanine/code.c",328,333,"POKEMON_FUNC(arcanine_TurnToFume)","void arcanine_TurnToFume(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_328_11_services},
{0x802dbb78u,"src/volcano/arcanine/code.c",335,345,"POKEMON_FUNC(arcanine_AppleNearby)","void arcanine_AppleNearby(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_335_12_callbacks,d_335_12_graphs,{},{},{},{},d_335_12_services},
{0x802dbc00u,"src/volcano/arcanine/code.c",347,355,"POKEMON_FUNC(arcanine_RunToApple)","void arcanine_RunToApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},d_347_13_headers,{},{},d_347_13_services},
{0x802dbc84u,"src/volcano/arcanine/code.c",357,372,"POKEMON_FUNC(arcanine_WithApple)","void arcanine_WithApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_357_14_callbacks,d_357_14_graphs,{},d_357_14_headers,{},{},d_357_14_services},
{0x802dbd34u,"src/volcano/arcanine/code.c",374,379,"POKEMON_FUNC(arcanine_TurnToApple)","void arcanine_TurnToApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_374_15_services},
{0x802dbd94u,"src/volcano/arcanine/code.c",381,383,"POKEMON_FUNC(arcanine_SearchApple)","void arcanine_SearchApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_381_16_callbacks,{},{},{},{},{},d_381_16_services},
{0x802dbdb8u,"src/volcano/arcanine/code.c",385,392,"POKEMON_FUNC(arcanine_Restore)","void arcanine_Restore(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_385_17_callbacks,d_385_17_graphs,{},{},{},{},d_385_17_services},
{0x802dbe18u,"src/volcano/arcanine/code.c",394,401,"POKEMON_FUNC(arcanine_Dance)","void arcanine_Dance(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_394_18_callbacks,d_394_18_graphs,{},d_394_18_headers,{},{},d_394_18_services},
{0x802dbe8cu,"src/volcano/arcanine/code.c",403,408,"POKEMON_FUNC(arcanine_TurnToPlayer)","void arcanine_TurnToPlayer(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_403_19_services},
{0x802dbed8u,"src/volcano/arcanine/code.c",410,417,"POKEMON_FUNC(arcanine_RunAwayFromPlayer)","void arcanine_RunAwayFromPlayer(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_410_20_callbacks,d_410_20_graphs,{},d_410_20_headers,{},{},d_410_20_services},
{0x802dbf4cu,"src/volcano/arcanine/code.c",419,425,"POKEMON_FUNC(arcanine_RunAway)","void arcanine_RunAway(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_419_21_services},
{0x802dbfa8u,"src/volcano/arcanine/code.c",427,434,"POKEMON_FUNC(arcanine_PhotoTaken)","void arcanine_PhotoTaken(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_427_22_graphs,d_427_22_weighted_states,d_427_22_headers,{},{},d_427_22_services},
{0x802dc018u,"src/volcano/arcanine/code.c",436,438,"GObj* arcanine_Spawn(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)","IsleGameObject* arcanine_Spawn(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn)","non_gobj_exact_Source_ABI",{},{},{},{},d_436_23_child_factories,{},{}},
{0x802dd2e0u,"src/volcano/charizard/code.c",115,119,"POKEMON_FUNC(charizard_InititalState)","void charizard_InititalState(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_115_24_callbacks,{},{},{},{},{},d_115_24_services},
{0x802dd328u,"src/volcano/charizard/code.c",121,130,"POKEMON_FUNC(charizard_Appear)","void charizard_Appear(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_121_25_callbacks,{},{},d_121_25_headers,{},{},d_121_25_services},
{0x802dd3b0u,"src/volcano/charizard/code.c",132,146,"POKEMON_FUNC(charizard_MoveOutOfLava)","void charizard_MoveOutOfLava(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_132_26_callbacks,{},{},{},{},{},d_132_26_services},
{0x802dd4a4u,"src/volcano/charizard/code.c",148,157,"POKEMON_FUNC(charizard_CreateSplash)","void charizard_CreateSplash(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_148_27_callbacks,{},{},{},{},{},d_148_27_services},
{0x802dd500u,"src/volcano/charizard/code.c",159,166,"POKEMON_FUNC(charizard_Idle)","void charizard_Idle(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_159_28_graphs,d_159_28_weighted_states,d_159_28_headers,{},{},d_159_28_services},
{0x802dd570u,"src/volcano/charizard/code.c",168,175,"POKEMON_FUNC(charizard_Roar)","void charizard_Roar(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_168_29_graphs,d_168_29_weighted_states,d_168_29_headers,{},{},d_168_29_services},
{0x802dd5e0u,"src/volcano/charizard/code.c",177,189,"POKEMON_FUNC(charizard_HitByPesterBall)","void charizard_HitByPesterBall(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_177_30_callbacks,{},d_177_30_weighted_states,d_177_30_headers,{},{},d_177_30_services},
{0x802dd67cu,"src/volcano/charizard/code.c",191,196,"POKEMON_FUNC(charizard_TurnToPlayer)","void charizard_TurnToPlayer(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_191_31_services},
{0x802dd6d4u,"src/volcano/charizard/code.c",198,205,"POKEMON_FUNC(charizard_HitByApple)","void charizard_HitByApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_198_32_callbacks,{},d_198_32_weighted_states,d_198_32_headers,{},{},d_198_32_services},
{0x802dd740u,"src/volcano/charizard/code.c",207,214,"POKEMON_FUNC(charizard_AffectedByPesterBall)","void charizard_AffectedByPesterBall(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_207_33_callbacks,{},{},d_207_33_headers,{},{},d_207_33_services},
{0x802dd7acu,"src/volcano/charizard/code.c",216,218,"GObj* charizard_Spawn(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)","IsleGameObject* charizard_Spawn(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn)","non_gobj_exact_Source_ABI",{},{},{},{},d_216_34_child_factories,{},{}},
{0x802d8aa0u,"src/volcano/charmander/code.c",322,348,"POKEMON_FUNC(charmander_DeleteLater)","void charmander_DeleteLater(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_322_35_services},
{0x802d8bb8u,"src/volcano/charmander/code.c",350,376,"POKEMON_FUNC(charmander_InitialState)","void charmander_InitialState(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_350_36_callbacks,{},{},{},{},{},d_350_36_services},
{0x802d8ca4u,"src/volcano/charmander/code.c",378,385,"POKEMON_FUNC(charmander_Idle)","void charmander_Idle(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_378_37_graphs,d_378_37_weighted_states,d_378_37_headers,{},{},d_378_37_services},
{0x802d8d14u,"src/volcano/charmander/code.c",387,398,"POKEMON_FUNC(charmander_Walk)","void charmander_Walk(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_387_38_callbacks,d_387_38_graphs,d_387_38_weighted_states,d_387_38_headers,{},{},d_387_38_services},
{0x802d8db8u,"src/volcano/charmander/code.c",400,406,"POKEMON_FUNC(charmander_MoveWalk)","void charmander_MoveWalk(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_400_39_services},
{0x802d8e14u,"src/volcano/charmander/code.c",408,419,"POKEMON_FUNC(charmander_Run)","void charmander_Run(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_408_40_callbacks,d_408_40_graphs,d_408_40_weighted_states,d_408_40_headers,{},{},d_408_40_services},
{0x802d8eb8u,"src/volcano/charmander/code.c",421,427,"POKEMON_FUNC(charmander_MoveRun)","void charmander_MoveRun(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_421_41_services},
{0x802d8f14u,"src/volcano/charmander/code.c",429,437,"POKEMON_FUNC(charmander_Howl)","void charmander_Howl(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_429_42_graphs,d_429_42_weighted_states,d_429_42_headers,{},{},d_429_42_services},
{0x802d8f94u,"src/volcano/charmander/code.c",439,445,"POKEMON_FUNC(charmander_HowlInResponse)","void charmander_HowlInResponse(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_439_43_graphs,d_439_43_weighted_states,d_439_43_headers,{},{},d_439_43_services},
{0x802d9004u,"src/volcano/charmander/code.c",447,454,"POKEMON_FUNC(charmander_HowlingEnded)","void charmander_HowlingEnded(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_447_44_services},
{0x802d9074u,"src/volcano/charmander/code.c",456,486,"POKEMON_FUNC(charmander_HitOrBurnt)","void charmander_HitOrBurnt(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_456_45_callbacks,d_456_45_graphs,{},d_456_45_headers,{},{},d_456_45_services},
{0,"src/volcano/charmander/code.c",488,489,"static void nullsub(void)","void volcano_charmander_nullsub(IsleVolcanoActorSource* s)","non_gobj_exact_Source_ABI",{},{},{},{},{},{},d_488_46_services},
{0x802d924cu,"src/volcano/charmander/code.c",491,501,"POKEMON_FUNC(charmander_HitByApple)","void charmander_HitByApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_491_47_callbacks,d_491_47_graphs,{},d_491_47_headers,{},{},d_491_47_services},
{0x802d930cu,"src/volcano/charmander/code.c",503,510,"POKEMON_FUNC(charmander_RunAway)","void charmander_RunAway(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_503_48_callbacks,d_503_48_graphs,{},d_503_48_headers,{},{},d_503_48_services},
{0x802d9380u,"src/volcano/charmander/code.c",512,518,"POKEMON_FUNC(charmander_MoveRunAway)","void charmander_MoveRunAway(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_512_49_services},
{0x802d93dcu,"src/volcano/charmander/code.c",520,530,"POKEMON_FUNC(charmander_AppleNearby)","void charmander_AppleNearby(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_520_50_callbacks,d_520_50_graphs,{},{},{},{},d_520_50_services},
{0x802d9464u,"src/volcano/charmander/code.c",532,540,"POKEMON_FUNC(charmander_RunToApple)","void charmander_RunToApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},d_532_51_headers,{},{},d_532_51_services},
{0x802d94e8u,"src/volcano/charmander/code.c",542,556,"POKEMON_FUNC(charmander_WithApple)","void charmander_WithApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_542_52_callbacks,d_542_52_graphs,{},d_542_52_headers,{},{},d_542_52_services},
{0x802d95acu,"src/volcano/charmander/code.c",558,563,"POKEMON_FUNC(charmander_MoveTurnToTarget)","void charmander_MoveTurnToTarget(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_558_53_services},
{0x802d9614u,"src/volcano/charmander/code.c",565,567,"POKEMON_FUNC(charmander_SearchApple)","void charmander_SearchApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_565_54_callbacks,{},{},{},{},{},d_565_54_services},
{0x802d9638u,"src/volcano/charmander/code.c",569,576,"POKEMON_FUNC(charmander_Roar)","void charmander_Roar(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_569_55_callbacks,d_569_55_graphs,d_569_55_weighted_states,d_569_55_headers,{},{},d_569_55_services},
{0x802d96acu,"src/volcano/charmander/code.c",578,583,"POKEMON_FUNC(charmander_MoveTurnToPlayer)","void charmander_MoveTurnToPlayer(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_578_56_services},
{0x802d96f8u,"src/volcano/charmander/code.c",585,592,"POKEMON_FUNC(charmander_Dance)","void charmander_Dance(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_585_57_callbacks,d_585_57_graphs,{},d_585_57_headers,{},{},d_585_57_services},
{0x802d976cu,"src/volcano/charmander/code.c",594,599,"POKEMON_FUNC(charmander_MoveTurnToPlayer2)","void charmander_MoveTurnToPlayer2(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_594_58_services},
{0x802d97b8u,"src/volcano/charmander/code.c",601,626,"POKEMON_FUNC(charmander_SummonFriend)","void charmander_SummonFriend(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_601_59_callbacks,d_601_59_graphs,{},d_601_59_headers,{},{},d_601_59_services},
{0x802d9900u,"src/volcano/charmander/code.c",628,634,"POKEMON_FUNC(charmander_InitGroup3)","void charmander_InitGroup3(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_628_60_graphs,{},{},{},{},d_628_60_services},
{0x802d9950u,"src/volcano/charmander/code.c",636,642,"POKEMON_FUNC(charmander_InitGroup4)","void charmander_InitGroup4(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_636_61_graphs,{},{},{},{},d_636_61_services},
{0x802d99a0u,"src/volcano/charmander/code.c",644,664,"POKEMON_FUNC(charmander_SpawnFriend)","void charmander_SpawnFriend(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_644_62_callbacks,d_644_62_graphs,{},d_644_62_headers,{},{},d_644_62_services},
{0x802d9a80u,"src/volcano/charmander/code.c",666,680,"POKEMON_FUNC(charmander_RunToSummoner)","void charmander_RunToSummoner(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_666_63_services},
{0x802d9b44u,"src/volcano/charmander/code.c",682,687,"POKEMON_FUNC(charmander_RunToSummoner2)","void charmander_RunToSummoner2(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_682_64_services},
{0x802d9b8cu,"src/volcano/charmander/code.c",689,696,"POKEMON_FUNC(charmander_PlayEvolveEffect)","void charmander_PlayEvolveEffect(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_689_65_graphs,{},d_689_65_headers,{},d_689_65_helpers,d_689_65_services},
{0x802d9bf4u,"src/volcano/charmander/code.c",698,703,"POKEMON_FUNC(charmander_BurntWhileFalling)","void charmander_BurntWhileFalling(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_698_66_callbacks,{},{},{},{},{},d_698_66_services},
{0x802d9c30u,"src/volcano/charmander/code.c",705,711,"POKEMON_FUNC(charmander_LieOnGround)","void charmander_LieOnGround(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_705_67_callbacks,{},{},d_705_67_headers,{},{},d_705_67_services},
{0x802d9c84u,"src/volcano/charmander/code.c",713,717,"POKEMON_FUNC(charmander_EvolveIntoCharmeleon)","void charmander_EvolveIntoCharmeleon(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},d_713_68_helpers,d_713_68_services},
{0x802d9cb8u,"src/volcano/charmander/code.c",719,721,"GObj* charmander_Spawn(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)","IsleGameObject* charmander_Spawn(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn)","non_gobj_exact_Source_ABI",{},{},{},{},d_719_69_child_factories,{},{}},
{0x802dc060u,"src/volcano/charmeleon/code.c",244,257,"POKEMON_FUNC(charmeleon_DeleteLater)","void charmeleon_DeleteLater(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_244_70_services},
{0x802dc108u,"src/volcano/charmeleon/code.c",259,266,"POKEMON_FUNC(charmeleon_InitialState)","void charmeleon_InitialState(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_259_71_callbacks,{},d_259_71_weighted_states,{},{},{},d_259_71_services},
{0x802dc170u,"src/volcano/charmeleon/code.c",268,277,"POKEMON_FUNC(charmeleon_Walk)","void charmeleon_Walk(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_268_72_callbacks,d_268_72_graphs,d_268_72_weighted_states,d_268_72_headers,{},{},d_268_72_services},
{0x802dc1f8u,"src/volcano/charmeleon/code.c",279,288,"POKEMON_FUNC(charmeleon_Run)","void charmeleon_Run(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_279_73_callbacks,d_279_73_graphs,d_279_73_weighted_states,d_279_73_headers,{},{},d_279_73_services},
{0x802dc280u,"src/volcano/charmeleon/code.c",290,315,"POKEMON_FUNC(charmeleon_FollowPath)","void charmeleon_FollowPath(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},d_290_74_helpers,d_290_74_services},
{0x802dc3a0u,"src/volcano/charmeleon/code.c",317,324,"POKEMON_FUNC(charmeleon_LookAround)","void charmeleon_LookAround(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_317_75_graphs,d_317_75_weighted_states,d_317_75_headers,{},{},d_317_75_services},
{0x802dc410u,"src/volcano/charmeleon/code.c",326,333,"POKEMON_FUNC(charmeleon_Roar)","void charmeleon_Roar(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_326_76_graphs,d_326_76_weighted_states,d_326_76_headers,{},{},d_326_76_services},
{0x802dc480u,"src/volcano/charmeleon/code.c",335,342,"POKEMON_FUNC(charmeleon_Idle)","void charmeleon_Idle(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_335_77_graphs,d_335_77_weighted_states,d_335_77_headers,{},{},d_335_77_services},
{0x802dc4f0u,"src/volcano/charmeleon/code.c",344,358,"POKEMON_FUNC(charmeleon_BounceOfApple)","void charmeleon_BounceOfApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_344_78_callbacks,d_344_78_graphs,{},d_344_78_headers,{},{},d_344_78_services},
{0x802dc590u,"src/volcano/charmeleon/code.c",360,386,"POKEMON_FUNC(charmeleon_BounceOfPesterBall)","void charmeleon_BounceOfPesterBall(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_360_79_callbacks,d_360_79_graphs,{},d_360_79_headers,{},{},d_360_79_services},
{0x802dc6b4u,"src/volcano/charmeleon/code.c",388,402,"POKEMON_FUNC(charmeleon_BounceBack)","void charmeleon_BounceBack(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_388_80_services},
{0x802dc758u,"src/volcano/charmeleon/code.c",404,410,"POKEMON_FUNC(charmeleon_HitByPesterBall)","void charmeleon_HitByPesterBall(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_404_81_callbacks,{},{},{},{},{},d_404_81_services},
{0x802dc7a8u,"src/volcano/charmeleon/code.c",412,418,"POKEMON_FUNC(charmeleon_HitByApple)","void charmeleon_HitByApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_412_82_callbacks,{},{},{},{},{},d_412_82_services},
{0x802dc7f8u,"src/volcano/charmeleon/code.c",420,425,"POKEMON_FUNC(charmeleon_Return)","void charmeleon_Return(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_420_83_callbacks,{},{},d_420_83_headers,{},{},d_420_83_services},
{0x802dc850u,"src/volcano/charmeleon/code.c",427,446,"POKEMON_FUNC(charmeleon_StepToOrigPos)","void charmeleon_StepToOrigPos(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_427_84_graphs,d_427_84_weighted_states,{},{},{},d_427_84_services},
{0x802dc924u,"src/volcano/charmeleon/code.c",448,457,"POKEMON_FUNC(charmeleon_RunToOrigPos)","void charmeleon_RunToOrigPos(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_448_85_services},
{0x802dc99cu,"src/volcano/charmeleon/code.c",459,469,"POKEMON_FUNC(charmeleon_EvolveIntoCharizard)","void charmeleon_EvolveIntoCharizard(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_459_86_callbacks,{},{},{},{},d_459_86_helpers,d_459_86_services},
{0x802dca28u,"src/volcano/charmeleon/code.c",471,490,"POKEMON_FUNC(charmeleon_DiveInLava)","void charmeleon_DiveInLava(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_471_87_services},
{0x802dcb44u,"src/volcano/charmeleon/code.c",492,502,"POKEMON_FUNC(charmeleon_EvolvedInit)","void charmeleon_EvolvedInit(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_492_88_callbacks,{},{},d_492_88_headers,{},{},d_492_88_services},
{0x802dcbd0u,"src/volcano/charmeleon/code.c",504,510,"POKEMON_FUNC(charmeleon_EvolvedIdle)","void charmeleon_EvolvedIdle(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_504_89_graphs,d_504_89_weighted_states,d_504_89_headers,{},{},d_504_89_services},
{0x802dcc40u,"src/volcano/charmeleon/code.c",512,524,"POKEMON_FUNC(charmeleon_EvolvedWalk)","void charmeleon_EvolvedWalk(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_512_90_callbacks,d_512_90_graphs,d_512_90_weighted_states,d_512_90_headers,{},{},d_512_90_services},
{0x802dcce4u,"src/volcano/charmeleon/code.c",526,532,"POKEMON_FUNC(charmeleon_EvolvedMoveWalk)","void charmeleon_EvolvedMoveWalk(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_526_91_services},
{0x802dcd40u,"src/volcano/charmeleon/code.c",534,546,"POKEMON_FUNC(charmeleon_EvolvedRun)","void charmeleon_EvolvedRun(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_534_92_callbacks,d_534_92_graphs,d_534_92_weighted_states,d_534_92_headers,{},{},d_534_92_services},
{0x802dcde4u,"src/volcano/charmeleon/code.c",548,554,"POKEMON_FUNC(charmeleon_EvolvedMoveRun)","void charmeleon_EvolvedMoveRun(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_548_93_services},
{0x802dce40u,"src/volcano/charmeleon/code.c",556,575,"POKEMON_FUNC(charmeleon_EvolvedHitByPesterBall)","void charmeleon_EvolvedHitByPesterBall(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_556_94_callbacks,d_556_94_graphs,{},d_556_94_headers,{},{},d_556_94_services},
{0x802dcf44u,"src/volcano/charmeleon/code.c",577,584,"POKEMON_FUNC(charmeleon_EvolvedHit)","void charmeleon_EvolvedHit(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_577_95_callbacks,{},{},d_577_95_headers,{},{},d_577_95_services},
{0x802dcfacu,"src/volcano/charmeleon/code.c",586,596,"POKEMON_FUNC(charmeleon_EvolvedSearchApple)","void charmeleon_EvolvedSearchApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_586_96_callbacks,d_586_96_graphs,{},{},{},{},d_586_96_services},
{0x802dd034u,"src/volcano/charmeleon/code.c",598,606,"POKEMON_FUNC(charmeleon_EvolvedRunToApple)","void charmeleon_EvolvedRunToApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},d_598_97_headers,{},{},d_598_97_services},
{0x802dd0e8u,"src/volcano/charmeleon/code.c",608,621,"POKEMON_FUNC(charmeleon_EvolvedWithApple)","void charmeleon_EvolvedWithApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_608_98_callbacks,d_608_98_graphs,{},d_608_98_headers,{},{},d_608_98_services},
{0x802dd1c0u,"src/volcano/charmeleon/code.c",623,628,"POKEMON_FUNC(charmeleon_EvolvedTurnToApple)","void charmeleon_EvolvedTurnToApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_623_99_services},
{0x802dd214u,"src/volcano/charmeleon/code.c",630,632,"GObj* charmeleon_Spawn(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)","IsleGameObject* charmeleon_Spawn(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn)","non_gobj_exact_Source_ABI",{},{},{},{},d_630_100_child_factories,{},{}},
{0x802dd24cu,"src/volcano/charmeleon/code.c",634,649,"void charmeleon_EvolvedSpawn(GObj* obj)","void charmeleon_EvolvedSpawn(IsleGameObject* obj)","gobj_void_single_argument",{},{},{},{},d_634_101_child_factories,{},d_634_101_services},
{0x802dea80u,"src/volcano/evolution_controller/code.c",78,88,"POKEMON_FUNC(evolution_controller_InitialState)","void evolution_controller_InitialState(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_78_102_callbacks,d_78_102_graphs,{},{},{},{},d_78_102_services},
{0x802deafcu,"src/volcano/evolution_controller/code.c",91,104,"void evolution_controller_CharmeleonFellInLava(GObj* obj)","void evolution_controller_CharmeleonFellInLava(IsleGameObject* obj)","gobj_void_single_argument",d_91_103_callbacks,{},{},{},{},{},d_91_103_services},
{0x802debb0u,"src/volcano/evolution_controller/code.c",106,110,"POKEMON_FUNC(evolution_controller_EvolveIntoCharizard)","void evolution_controller_EvolveIntoCharizard(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_106_104_graphs,{},{},{},{},d_106_104_services},
{0x802debf0u,"src/volcano/evolution_controller/code.c",112,122,"POKEMON_FUNC(evolution_controller_CharizardAppeared)","void evolution_controller_CharizardAppeared(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},d_112_105_headers,{},{},d_112_105_services},
{0x802dec78u,"src/volcano/evolution_controller/code.c",124,139,"POKEMON_FUNC(evolution_controller_EvolveIntoCharmeleon)","void evolution_controller_EvolveIntoCharmeleon(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},d_124_106_headers,{},{},d_124_106_services},
{0x802ded34u,"src/volcano/evolution_controller/code.c",141,143,"GObj* evolution_controller_Spawn(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)","IsleGameObject* evolution_controller_Spawn(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn)","non_gobj_exact_Source_ABI",{},{},{},{},d_141_107_child_factories,{},{}},
{0x802ded6cu,"src/volcano/evolution_controller/code.c",145,156,"void func_802DED6C_72FF6C(GObj* obj)","void func_802DED6C_72FF6C(IsleGameObject* obj)","gobj_void_single_argument",{},{},{},{},d_145_108_child_factories,{},d_145_108_services},
{0x802df3b0u,"src/volcano/gate/code.c",97,99,"POKEMON_FUNC(volcano_gate_InitialState)","void volcano_gate_InitialState(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_97_109_callbacks,{},{},{},{},{},d_97_109_services},
{0x802df3d4u,"src/volcano/gate/code.c",101,107,"POKEMON_FUNC(volcano_gate_Idle)","void volcano_gate_Idle(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_101_110_graphs,{},d_101_110_headers,{},{},d_101_110_services},
{0x802df42cu,"src/volcano/gate/code.c",109,129,"POKEMON_FUNC(volcano_gate_PlayerApproaching)","void volcano_gate_PlayerApproaching(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_109_111_callbacks,{},{},d_109_111_headers,{},{},d_109_111_services},
{0x802df508u,"src/volcano/gate/code.c",131,135,"void volcano_gate_PlayerAnimCallback(DObj* arg0, s32 arg1, f32 arg2)","void volcano_gate_PlayerAnimCallback(IsleVolcanoActorSource* s, const IsleGameModelRef* arg0, int32_t arg1, float arg2)","animation_void_DObj_ptr_s32_f32",{},{},{},{},{},{},d_131_112_services},
{0x802df534u,"src/volcano/gate/code.c",137,156,"POKEMON_FUNC(volcano_gate_LowerVolume)","void volcano_gate_LowerVolume(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_137_113_services},
{0x802df7f4u,"src/volcano/gate/code.c",158,171,"POKEMON_FUNC(volcano_gate_StopSounds)","void volcano_gate_StopSounds(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_158_114_services},
{0x802df880u,"src/volcano/gate/code.c",173,215,"POKEMON_FUNC(volcano_gate_Cutscene)","void volcano_gate_Cutscene(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_173_115_callbacks,{},{},{},{},{},d_173_115_services},
{0x802dfa38u,"src/volcano/gate/code.c",217,219,"GObj* volcano_gate_Spawn(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)","IsleGameObject* volcano_gate_Spawn(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn)","non_gobj_exact_Source_ABI",{},{},{},{},d_217_116_child_factories,{},{}},
{0x802daae0u,"src/volcano/growlithe/code.c",210,225,"POKEMON_FUNC(growlithe_InitialState)","void growlithe_InitialState(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_210_117_callbacks,d_210_117_graphs,{},d_210_117_headers,{},{},d_210_117_services},
{0x802dab90u,"src/volcano/growlithe/code.c",227,239,"POKEMON_FUNC(growlithe_MoveJump)","void growlithe_MoveJump(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},d_227_118_helpers,d_227_118_services},
{0x802dac38u,"src/volcano/growlithe/code.c",241,248,"POKEMON_FUNC(growlithe_Idle)","void growlithe_Idle(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_241_119_graphs,d_241_119_weighted_states,d_241_119_headers,{},{},d_241_119_services},
{0x802daca8u,"src/volcano/growlithe/code.c",250,262,"POKEMON_FUNC(growlithe_Walk)","void growlithe_Walk(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_250_120_callbacks,d_250_120_graphs,d_250_120_weighted_states,d_250_120_headers,{},{},d_250_120_services},
{0x802dad4cu,"src/volcano/growlithe/code.c",264,270,"POKEMON_FUNC(growlithe_MoveWalk)","void growlithe_MoveWalk(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_264_121_services},
{0x802dada8u,"src/volcano/growlithe/code.c",272,284,"POKEMON_FUNC(growlithe_Run)","void growlithe_Run(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_272_122_callbacks,d_272_122_graphs,d_272_122_weighted_states,d_272_122_headers,{},{},d_272_122_services},
{0x802dae4cu,"src/volcano/growlithe/code.c",286,292,"POKEMON_FUNC(growlithe_MoveRun)","void growlithe_MoveRun(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_286_123_services},
{0x802daea8u,"src/volcano/growlithe/code.c",294,301,"POKEMON_FUNC(growlithe_Roar)","void growlithe_Roar(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_294_124_graphs,d_294_124_weighted_states,d_294_124_headers,{},{},d_294_124_services},
{0x802daf18u,"src/volcano/growlithe/code.c",303,310,"POKEMON_FUNC(growlithe_HitByPesterBall)","void growlithe_HitByPesterBall(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_303_125_callbacks,d_303_125_graphs,{},d_303_125_headers,{},{},d_303_125_services},
{0x802daf88u,"src/volcano/growlithe/code.c",312,319,"POKEMON_FUNC(growlithe_HitByApple)","void growlithe_HitByApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_312_126_callbacks,d_312_126_graphs,{},d_312_126_headers,{},{},d_312_126_services},
{0x802daff8u,"src/volcano/growlithe/code.c",321,328,"POKEMON_FUNC(growlithe_AffectedByPesterBall)","void growlithe_AffectedByPesterBall(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_321_127_callbacks,d_321_127_graphs,{},d_321_127_headers,{},{},d_321_127_services},
{0x802db06cu,"src/volcano/growlithe/code.c",330,335,"POKEMON_FUNC(growlithe_TurnToFume)","void growlithe_TurnToFume(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_330_128_services},
{0x802db0b8u,"src/volcano/growlithe/code.c",337,347,"POKEMON_FUNC(growlithe_AppleNearby)","void growlithe_AppleNearby(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_337_129_callbacks,d_337_129_graphs,{},{},{},{},d_337_129_services},
{0x802db140u,"src/volcano/growlithe/code.c",349,357,"POKEMON_FUNC(growlithe_RunToApple)","void growlithe_RunToApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},d_349_130_headers,{},{},d_349_130_services},
{0x802db1c4u,"src/volcano/growlithe/code.c",359,374,"POKEMON_FUNC(growlithe_WithApple)","void growlithe_WithApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_359_131_callbacks,d_359_131_graphs,{},d_359_131_headers,{},{},d_359_131_services},
{0x802db274u,"src/volcano/growlithe/code.c",376,381,"POKEMON_FUNC(growlithe_TurnToApple)","void growlithe_TurnToApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_376_132_services},
{0x802db2d4u,"src/volcano/growlithe/code.c",383,385,"POKEMON_FUNC(growlithe_SearchApple)","void growlithe_SearchApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_383_133_callbacks,{},{},{},{},{},d_383_133_services},
{0x802db2f8u,"src/volcano/growlithe/code.c",387,394,"POKEMON_FUNC(growlithe_Restore)","void growlithe_Restore(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_387_134_callbacks,d_387_134_graphs,{},{},{},{},d_387_134_services},
{0x802db358u,"src/volcano/growlithe/code.c",396,403,"POKEMON_FUNC(growlithe_Dance)","void growlithe_Dance(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_396_135_callbacks,d_396_135_graphs,{},d_396_135_headers,{},{},d_396_135_services},
{0x802db3ccu,"src/volcano/growlithe/code.c",405,410,"POKEMON_FUNC(growlithe_TurnToPlayer)","void growlithe_TurnToPlayer(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_405_136_services},
{0x802db418u,"src/volcano/growlithe/code.c",412,419,"POKEMON_FUNC(growlithe_RunAwayFromPlayer)","void growlithe_RunAwayFromPlayer(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_412_137_callbacks,d_412_137_graphs,{},d_412_137_headers,{},{},d_412_137_services},
{0x802db48cu,"src/volcano/growlithe/code.c",421,427,"POKEMON_FUNC(growlithe_RunAway)","void growlithe_RunAway(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_421_138_services},
{0x802db4e8u,"src/volcano/growlithe/code.c",429,436,"POKEMON_FUNC(growlithe_PhotoTaken)","void growlithe_PhotoTaken(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_429_139_graphs,d_429_139_weighted_states,d_429_139_headers,{},{},d_429_139_services},
{0x802db558u,"src/volcano/growlithe/code.c",438,440,"GObj* growlithe_Spawn(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)","IsleGameObject* growlithe_Spawn(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn)","non_gobj_exact_Source_ABI",{},{},{},{},d_438_140_child_factories,{},{}},
{0x802dd7f0u,"src/volcano/growlithe_spawner/code.c",54,73,"POKEMON_FUNC(growlithe_spawner_InitialState)","void growlithe_spawner_InitialState(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_54_141_callbacks,{},{},{},{},{},d_54_141_services},
{0x802dd8f8u,"src/volcano/growlithe_spawner/code.c",75,81,"POKEMON_FUNC(growlithe_spawner_Idle)","void growlithe_spawner_Idle(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_75_142_graphs,{},d_75_142_headers,{},{},d_75_142_services},
{0x802dd954u,"src/volcano/growlithe_spawner/code.c",83,95,"POKEMON_FUNC(growlithe_spawner_SpawnPokemon)","void growlithe_spawner_SpawnPokemon(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_83_143_callbacks,{},{},d_83_143_headers,{},d_83_143_helpers,d_83_143_services},
{0x802dda0cu,"src/volcano/growlithe_spawner/code.c",97,107,"POKEMON_FUNC(growlithe_spawner_SendCommands)","void growlithe_spawner_SendCommands(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_97_144_services},
{0x802dda98u,"src/volcano/growlithe_spawner/code.c",109,111,"GObj* growlithe_spawner_Spawn(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)","IsleGameObject* growlithe_spawner_Spawn(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn)","non_gobj_exact_Source_ABI",{},{},{},{},d_109_145_child_factories,{},{}},
{0x802e0660u,"src/volcano/intro/code.c",17,21,"void volcano_IntroStartCutscene(GObj* obj)","void volcano_IntroStartCutscene(IsleGameObject* obj)","gobj_void_single_argument",{},{},{},{},{},{},d_17_146_services},
{0x802e0690u,"src/volcano/intro/code.c",23,25,"void volcano_IntroStopCutscene(void)","void volcano_IntroStopCutscene(IsleVolcanoLifecycleSource* l)","non_gobj_exact_Source_ABI",{},{},{},{},{},{},d_23_147_services},
{0x802e06b0u,"src/volcano/intro/code.c",27,36,"void volcano_IntroAnimationCallback(struct DObj* dobj, s32 param, f32 value)","void volcano_IntroAnimationCallback(IsleVolcanoLifecycleSource* l, const IslePlayerCartNode* dobj, int32_t param, float value)","animation_void_DObj_ptr_s32_f32",{},{},{},{},{},{},d_27_148_services},
{0x802e0724u,"src/volcano/intro/code.c",38,55,"void volcano_StopIntro(void)","void volcano_StopIntro(IsleVolcanoLifecycleSource* l)","non_gobj_exact_Source_ABI",{},{},{},{},{},d_38_149_helpers,d_38_149_services},
{0x802e081cu,"src/volcano/intro/code.c",57,74,"void volcano_PlayIntroSounds(GObj* obj)","void volcano_PlayIntroSounds(IsleGameObject* obj)","gobj_void_single_argument",{},{},{},{},{},{},d_57_150_services},
{0x802e0948u,"src/volcano/intro/code.c",76,133,"void volcano_UpdateIntro(GObj* obj)","void volcano_UpdateIntro(IsleGameObject* obj)","gobj_void_single_argument",{},{},{},{},{},d_76_151_helpers,d_76_151_services},
{0x802e0c28u,"src/volcano/intro/code.c",135,144,"void volcano_StartIntro(void)","void volcano_StartIntro(IsleVolcanoLifecycleSource* l)","non_gobj_exact_Source_ABI",{},{},{},{},{},d_135_152_helpers,d_135_152_services},
{0x802de390u,"src/volcano/koffing_smoke/code.c",75,82,"POKEMON_FUNC(koffing_smoke_InitialState)","void koffing_smoke_InitialState(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_75_153_callbacks,{},{},d_75_153_headers,{},{},d_75_153_services},
{0x802de3fcu,"src/volcano/koffing_smoke/code.c",84,90,"POKEMON_FUNC(koffing_smoke_RevealFace)","void koffing_smoke_RevealFace(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_84_154_callbacks,{},{},d_84_154_headers,{},{},d_84_154_services},
{0x802de450u,"src/volcano/koffing_smoke/code.c",92,100,"POKEMON_FUNC(koffing_smoke_Hide)","void koffing_smoke_Hide(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},d_92_155_headers,{},{},d_92_155_services},
{0x802de4c0u,"src/volcano/koffing_smoke/code.c",102,108,"POKEMON_FUNC(koffing_smoke_Rise)","void koffing_smoke_Rise(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_102_156_services},
{0x802de52cu,"src/volcano/koffing_smoke/code.c",110,112,"GObj* koffing_smoke_Spawn(s32 gObjID, u16 id, WorldBlock* roomA, WorldBlock* roomB, ObjectSpawn* spawn)","IsleGameObject* koffing_smoke_Spawn(int32_t gObjID, uint16_t id, IsleActorFactoryBlock* roomA, IsleActorFactoryBlock* roomB, IsleActorFactorySpawn* spawn)","non_gobj_exact_Source_ABI",{},{},{},{},d_110_157_child_factories,{},{}},
{0x802dede0u,"src/volcano/lava_splash/code.c",86,111,"POKEMON_FUNC(lava_splash_InitialState)","void lava_splash_InitialState(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_86_158_callbacks,{},{},d_86_158_headers,{},{},d_86_158_services},
{0x802deee0u,"src/volcano/lava_splash/code.c",113,121,"POKEMON_FUNC(lava_splash_Type1)","void lava_splash_Type1(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},d_113_159_headers,{},{},d_113_159_services},
{0x802def4cu,"src/volcano/lava_splash/code.c",123,131,"POKEMON_FUNC(lava_splash_Type5)","void lava_splash_Type5(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},d_123_160_headers,{},{},d_123_160_services},
{0x802defb8u,"src/volcano/lava_splash/code.c",133,141,"POKEMON_FUNC(lava_splash_Type2)","void lava_splash_Type2(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_133_161_callbacks,{},{},d_133_161_headers,{},{},d_133_161_services},
{0x802df028u,"src/volcano/lava_splash/code.c",143,150,"POKEMON_FUNC(lava_splash_Type2Inflate)","void lava_splash_Type2Inflate(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_143_162_services},
{0x802df0b4u,"src/volcano/lava_splash/code.c",152,166,"POKEMON_FUNC(lava_splash_Type3)","void lava_splash_Type3(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},d_152_163_headers,{},{},d_152_163_services},
{0x802df164u,"src/volcano/lava_splash/code.c",168,174,"POKEMON_FUNC(lava_splash_Type4)","void lava_splash_Type4(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_168_164_graphs,{},{},{},{},d_168_164_services},
{0x802df1b4u,"src/volcano/lava_splash/code.c",176,183,"POKEMON_FUNC(lava_splash_StartType4)","void lava_splash_StartType4(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_176_165_callbacks,{},{},d_176_165_headers,{},{},d_176_165_services},
{0x802df214u,"src/volcano/lava_splash/code.c",185,188,"POKEMON_FUNC(lava_splash_EndType4)","void lava_splash_EndType4(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_185_166_services},
{0x802df240u,"src/volcano/lava_splash/code.c",190,192,"GObj* lava_splash_Spawn(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)","IsleGameObject* lava_splash_Spawn(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn)","non_gobj_exact_Source_ABI",{},{},{},{},d_190_167_child_factories,{},{}},
{0x802df280u,"src/volcano/magikarp/code.c",83,93,"POKEMON_FUNC(volcano_magikarp_InitialState)","void volcano_magikarp_InitialState(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_83_168_callbacks,{},{},d_83_168_headers,{},{},d_83_168_services},
{0x802df338u,"src/volcano/magikarp/code.c",95,101,"POKEMON_FUNC(func_802DF338_730538)","void func_802DF338_730538(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},d_95_169_child_factories,{},d_95_169_services},
{0x802df378u,"src/volcano/magikarp/code.c",103,105,"GObj* volcano_magikarp_Spawn(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)","IsleGameObject* volcano_magikarp_Spawn(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn)","non_gobj_exact_Source_ABI",{},{},{},{},d_103_170_child_factories,{},{}},
{0x802d7290u,"src/volcano/magmar/code.c",264,292,"POKEMON_FUNC(magmar_DeleteLater)","void magmar_DeleteLater(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_264_171_services},
{0x802d73a8u,"src/volcano/magmar/code.c",294,307,"POKEMON_FUNC(magmar_InitialState)","void magmar_InitialState(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_294_172_callbacks,d_294_172_graphs,{},d_294_172_headers,{},{},d_294_172_services},
{0x802d7458u,"src/volcano/magmar/code.c",309,323,"POKEMON_FUNC(magmar_LoneInit)","void magmar_LoneInit(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_309_173_callbacks,{},{},d_309_173_headers,{},{},d_309_173_services},
{0x802d7510u,"src/volcano/magmar/code.c",325,333,"POKEMON_FUNC(magmar_Idle)","void magmar_Idle(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_325_174_graphs,d_325_174_weighted_states,d_325_174_headers,{},{},d_325_174_services},
{0x802d7584u,"src/volcano/magmar/code.c",335,342,"POKEMON_FUNC(magmar_LookAround)","void magmar_LookAround(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_335_175_graphs,d_335_175_weighted_states,d_335_175_headers,{},{},d_335_175_services},
{0x802d75f4u,"src/volcano/magmar/code.c",344,355,"POKEMON_FUNC(magmar_Walk)","void magmar_Walk(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_344_176_callbacks,d_344_176_graphs,d_344_176_weighted_states,d_344_176_headers,{},{},d_344_176_services},
{0x802d7698u,"src/volcano/magmar/code.c",357,363,"POKEMON_FUNC(magmar_MoveWalk)","void magmar_MoveWalk(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_357_177_services},
{0x802d76f4u,"src/volcano/magmar/code.c",365,385,"POKEMON_FUNC(magmar_HitByPesterBall)","void magmar_HitByPesterBall(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_365_178_callbacks,d_365_178_graphs,{},d_365_178_headers,{},{},d_365_178_services},
{0x802d77bcu,"src/volcano/magmar/code.c",387,395,"POKEMON_FUNC(magmar_HitByApple)","void magmar_HitByApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_387_179_graphs,d_387_179_weighted_states,d_387_179_headers,{},{},d_387_179_services},
{0x802d7830u,"src/volcano/magmar/code.c",397,405,"POKEMON_FUNC(magmar_AffectedByPesterBall)","void magmar_AffectedByPesterBall(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_397_180_graphs,d_397_180_weighted_states,d_397_180_headers,{},{},d_397_180_services},
{0x802d78a4u,"src/volcano/magmar/code.c",407,427,"POKEMON_FUNC(magmar_AppleNearby)","void magmar_AppleNearby(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_407_181_callbacks,d_407_181_graphs,{},d_407_181_headers,{},{},d_407_181_services},
{0x802d798cu,"src/volcano/magmar/code.c",429,437,"POKEMON_FUNC(magmar_RunToApple)","void magmar_RunToApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},d_429_182_headers,{},{},d_429_182_services},
{0x802d7a10u,"src/volcano/magmar/code.c",439,441,"POKEMON_FUNC(magmar_SearchApple)","void magmar_SearchApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_439_183_callbacks,{},{},{},{},{},d_439_183_services},
{0x802d7a34u,"src/volcano/magmar/code.c",443,450,"POKEMON_FUNC(magmar_Recover)","void magmar_Recover(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_443_184_callbacks,d_443_184_graphs,{},{},{},{},d_443_184_services},
{0x802d7a94u,"src/volcano/magmar/code.c",452,459,"POKEMON_FUNC(magmar_HearsFlute)","void magmar_HearsFlute(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_452_185_callbacks,d_452_185_graphs,{},d_452_185_headers,{},{},d_452_185_services},
{0x802d7b08u,"src/volcano/magmar/code.c",461,466,"POKEMON_FUNC(magmar_TurnToPlayer)","void magmar_TurnToPlayer(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_461_186_services},
{0x802d7b54u,"src/volcano/magmar/code.c",468,487,"POKEMON_FUNC(magmar_InteractWithPokemon)","void magmar_InteractWithPokemon(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_468_187_callbacks,d_468_187_graphs,{},d_468_187_headers,{},{},d_468_187_services},
{0x802d7c30u,"src/volcano/magmar/code.c",489,503,"POKEMON_FUNC(magmar_TurnToRival)","void magmar_TurnToRival(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_489_188_services},
{0x802d7cb4u,"src/volcano/magmar/code.c",505,524,"POKEMON_FUNC(magmar_BurnedDown)","void magmar_BurnedDown(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_505_189_callbacks,d_505_189_graphs,{},d_505_189_headers,{},{},d_505_189_services},
{0x802d7d84u,"src/volcano/magmar/code.c",526,547,"POKEMON_FUNC(magmar_BurnCharmander)","void magmar_BurnCharmander(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_526_190_callbacks,d_526_190_graphs,{},d_526_190_headers,{},{},d_526_190_services},
{0x802d7e90u,"src/volcano/magmar/code.c",549,561,"POKEMON_FUNC(magmar_CharmanderEvolved)","void magmar_CharmanderEvolved(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},d_549_191_headers,{},{},d_549_191_services},
{0x802d7f28u,"src/volcano/magmar/code.c",563,565,"GObj* magmar_Spawn(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)","IsleGameObject* magmar_Spawn(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn)","non_gobj_exact_Source_ABI",{},{},{},{},d_563_192_child_factories,{},{}},
{0x802d60e0u,"src/volcano/main.c",239,266,"void volcano_SpawnMagikarp(GObj* obj)","void volcano_SpawnMagikarp(IsleGameObject* obj)","gobj_void_single_argument",{},{},{},{},d_239_193_child_factories,{},d_239_193_services},
{0x802d61acu,"src/volcano/main.c",268,298,"void volcano_HandleCollision(GObj* obj, GroundResult* groundResult)","void volcano_HandleCollision(IsleGameObject* obj, IsleActorGround* groundResult)","non_gobj_exact_Source_ABI",{},{},{},{},{},{},d_268_194_services},
{0x802d6344u,"src/volcano/main.c",300,302,"void volcano_PokemonAdd(WorldBlock* arg0, WorldBlock* arg1)","void volcano_PokemonAdd(IsleVolcanoLifecycleSource* l, IsleVolcanoBlock* arg0, IsleVolcanoBlock* arg1)","non_gobj_exact_Source_ABI",{},{},{},{},{},{},d_300_195_services},
{0x802d6368u,"src/volcano/main.c",304,306,"void volcano_PokemonChangeBlock(WorldBlock* arg0, WorldBlock* arg1)","void volcano_PokemonChangeBlock(IsleVolcanoLifecycleSource* l, IsleVolcanoBlock* arg0, IsleVolcanoBlock* arg1)","non_gobj_exact_Source_ABI",{},{},{},{},{},{},d_304_196_services},
{0x802d638cu,"src/volcano/main.c",308,310,"void volcano_PokemonRemove(WorldBlock* arg0)","void volcano_PokemonRemove(IsleVolcanoLifecycleSource* l, IsleVolcanoBlock* arg0)","non_gobj_exact_Source_ABI",{},{},{},{},{},{},d_308_197_services},
{0x802d63b0u,"src/volcano/main.c",312,317,"void volcano_InitWorld(void)","void volcano_InitWorld(IsleVolcanoLifecycleSource* l)","non_gobj_exact_Source_ABI",{},{},{},{},{},d_312_198_helpers,d_312_198_services},
{0x802d643cu,"src/volcano/main.c",319,330,"void volcano_EndLevel(s32 endLevelReason)","void volcano_EndLevel(IsleVolcanoLifecycleSource* l, int32_t endLevelReason)","non_gobj_exact_Source_ABI",{},{},{},{},{},{},d_319_199_services},
{0x802d64a0u,"src/volcano/main.c",332,333,"void volcano_func_802D64A0_7276A0(GObj* obj)","void volcano_func_802D64A0_7276A0(IsleGameObject* obj)","gobj_void_single_argument",{},{},{},{},{},{},d_332_200_services},
{0x802d64a8u,"src/volcano/main.c",335,336,"void volcano_Pause(s32 arg0)","void volcano_Pause(IsleVolcanoLifecycleSource* l, int32_t arg0)","non_gobj_exact_Source_ABI",{},{},{},{},{},{},{}},
{0x802d64b0u,"src/volcano/main.c",338,352,"void volcano_ExitBlock(WorldBlock* arg0)","void volcano_ExitBlock(IsleVolcanoLifecycleSource* l, IsleVolcanoBlock* arg0)","non_gobj_exact_Source_ABI",{},{},{},{},{},d_338_202_helpers,d_338_202_services},
{0x802d6544u,"src/volcano/main.c",354,371,"void volcano_LoadEffects(void)","void volcano_LoadEffects(IsleVolcanoLifecycleSource* l)","non_gobj_exact_Source_ABI",{},{},{},{},{},{},d_354_203_services},
{0x802d6630u,"src/volcano/main.c",373,396,"void volcano_Init(void)","void volcano_Init(IsleVolcanoLifecycleSource* l)","non_gobj_exact_Source_ABI",{},{},{},{},{},d_373_204_helpers,d_373_204_services},
{0x802d6780u,"src/volcano/main.c",398,399,"void volcano_func_802D6780_727980(s32 arg0)","void volcano_func_802D6780_727980(IsleVolcanoLifecycleSource* l, int32_t arg0)","non_gobj_exact_Source_ABI",{},{},{},{},{},{},{}},
{0x802d6788u,"src/volcano/main.c",401,405,"void volcano_CheckIllegalCopy(void)","void volcano_CheckIllegalCopy(IsleVolcanoLifecycleSource* l)","non_gobj_exact_Source_ABI",{},{},{},{},{},{},d_401_206_services},
{0x802d67c4u,"src/volcano/main.c",407,421,"s32 volcano_Start(s32 arg0)","int32_t volcano_Start(IsleVolcanoLifecycleSource* l, int32_t arg0)","non_gobj_exact_Source_ABI",{},{},{},{},{},d_407_207_helpers,d_407_207_services},
{0x802d6864u,"src/volcano/main.c",423,451,"void volcano_SpawnArcanine(GObj* obj)","void volcano_SpawnArcanine(IsleGameObject* obj)","gobj_void_single_argument",{},{},{},{},d_423_208_child_factories,{},d_423_208_services},
{0x802d693cu,"src/volcano/main.c",453,481,"void volcano_SpawnGrowlithe(GObj* obj)","void volcano_SpawnGrowlithe(IsleGameObject* obj)","gobj_void_single_argument",{},{},{},{},d_453_209_child_factories,{},d_453_209_services},
{0x802d6a14u,"src/volcano/main.c",483,485,"void volcano_CharmeleonChangeBlock(GObj* arg0, f32 arg1, f32 arg2, f32 arg3, f32 arg4, f32 arg5, f32 arg6)","void volcano_CharmeleonChangeBlock(IsleGameObject* arg0, float arg1, float arg2, float arg3, float arg4, float arg5, float arg6)","non_gobj_exact_Source_ABI",{},{},{},{},{},{},d_483_210_services},
{0x802d6a5cu,"src/volcano/main.c",487,514,"void volcano_SpawnCharizard(GObj* obj)","void volcano_SpawnCharizard(IsleGameObject* obj)","gobj_void_single_argument",{},{},{},{},d_487_211_child_factories,{},d_487_211_services},
{0x802d6b2cu,"src/volcano/main.c",516,522,"void volcano_CreateSplashFromGrowlitheSpawner(GObj* obj)","void volcano_CreateSplashFromGrowlitheSpawner(IsleGameObject* obj)","gobj_void_single_argument",{},{},{},{},d_516_212_child_factories,{},d_516_212_services},
{0x802d6b64u,"src/volcano/main.c",524,531,"void volcano_CreateSplashFromGrowlitheOrArcanine(GObj* obj)","void volcano_CreateSplashFromGrowlitheOrArcanine(IsleGameObject* obj)","gobj_void_single_argument",{},{},{},{},d_524_213_child_factories,{},d_524_213_services},
{0x802d6bb0u,"src/volcano/main.c",533,542,"void volcano_CreateSplashFromMoltres(GObj* obj)","void volcano_CreateSplashFromMoltres(IsleGameObject* obj)","gobj_void_single_argument",{},{},{},{},d_533_214_child_factories,{},d_533_214_services},
{0x802d6c38u,"src/volcano/main.c",544,553,"void volcano_CreateSplashFromMoltresEgg(GObj* obj)","void volcano_CreateSplashFromMoltresEgg(IsleGameObject* obj)","gobj_void_single_argument",{},{},{},{},d_544_215_child_factories,{},d_544_215_services},
{0x802d6cc0u,"src/volcano/main.c",555,561,"void volcano_CreateSplashFromCharmeleon(GObj* obj)","void volcano_CreateSplashFromCharmeleon(IsleGameObject* obj)","gobj_void_single_argument",{},{},{},{},d_555_216_child_factories,{},d_555_216_services},
{0x802d6cfcu,"src/volcano/main.c",563,572,"void volcano_CreateSplashFromCharizard(GObj* obj)","void volcano_CreateSplashFromCharizard(IsleGameObject* obj)","gobj_void_single_argument",{},{},{},{},d_563_217_child_factories,{},d_563_217_services},
{0x802d6d6cu,"src/volcano/main.c",574,593,"bool volcano_SpawnGrowlitheOrArcanine(GObj* obj)","bool volcano_SpawnGrowlitheOrArcanine(IsleGameObject* obj)","non_gobj_exact_Source_ABI",d_574_218_callbacks,{},{},{},{},{},d_574_218_services},
{0x802d6e14u,"src/volcano/main.c",595,605,"void volcano_PokemonMove(GObj* obj)","void volcano_PokemonMove(IsleGameObject* obj)","gobj_void_single_argument",{},{},{},{},{},{},d_595_219_services},
{0x802d6ea8u,"src/volcano/main.c",607,616,"void volcano_PokemonMoveInDirection(GObj* obj, f32 heading)","void volcano_PokemonMoveInDirection(IsleGameObject* obj, float heading)","non_gobj_exact_Source_ABI",{},{},{},{},{},{},d_607_220_services},
{0x802d6f38u,"src/volcano/main.c",618,625,"f32 volcano_GetHeightAt(f32 x, f32 z)","float volcano_GetHeightAt(IsleVolcanoActorSource* s, float x, float z)","non_gobj_exact_Source_ABI",{},{},{},{},{},{},d_618_221_services},
{0x802d6f68u,"src/volcano/main.c",627,704,"void volcano_FollowPath(GObj* obj, f32* pathParam, f32 pathEnd, f32 speedMult, s32 flags)","void volcano_FollowPath(IsleGameObject* obj, float* pathParam, float pathEnd, float speedMult, int32_t flags)","non_gobj_exact_Source_ABI",{},{},{},{},{},d_627_222_helpers,d_627_222_services},
{0x802ddf00u,"src/volcano/moltres/code.c",106,115,"POKEMON_FUNC(moltres_InitialState)","void moltres_InitialState(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_106_223_callbacks,d_106_223_graphs,{},{},{},{},d_106_223_services},
{0x802ddf7cu,"src/volcano/moltres/code.c",117,151,"POKEMON_FUNC(moltres_Appear)","void moltres_Appear(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_117_224_callbacks,{},{},d_117_224_headers,{},{},d_117_224_services},
{0x802de0dcu,"src/volcano/moltres/code.c",153,159,"POKEMON_FUNC(moltres_FlyAway)","void moltres_FlyAway(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_153_225_services},
{0x802de144u,"src/volcano/moltres/code.c",161,167,"POKEMON_FUNC(moltres_Fly)","void moltres_Fly(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_161_226_services},
{0x802de1b0u,"src/volcano/moltres/code.c",169,173,"POKEMON_FUNC(moltres_UnusedBehavior)","void moltres_UnusedBehavior(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_169_227_graphs,{},{},{},{},d_169_227_services},
{0x802de1f0u,"src/volcano/moltres/code.c",175,184,"POKEMON_FUNC(moltres_UnusedPlayerIsClose)","void moltres_UnusedPlayerIsClose(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_175_228_callbacks,{},{},d_175_228_headers,{},{},d_175_228_services},
{0x802de26cu,"src/volcano/moltres/code.c",186,196,"POKEMON_FUNC(moltres_Render)","void moltres_Render(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_186_229_services},
{0x802de34cu,"src/volcano/moltres/code.c",198,200,"GObj* moltres_Spawn(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)","IsleGameObject* moltres_Spawn(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn)","non_gobj_exact_Source_ABI",{},{},{},{},d_198_230_child_factories,{},{}},
{0x802ddad0u,"src/volcano/moltres_egg/code.c",67,70,"POKEMON_FUNC(moltres_egg_InitialState)","void moltres_egg_InitialState(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_67_231_callbacks,{},{},{},{},{},d_67_231_services},
{0x802ddb04u,"src/volcano/moltres_egg/code.c",72,78,"POKEMON_FUNC(moltres_egg_Idle)","void moltres_egg_Idle(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_72_232_graphs,{},d_72_232_headers,{},{},d_72_232_services},
{0x802ddb60u,"src/volcano/moltres_egg/code.c",80,96,"POKEMON_FUNC(moltres_egg_Hit)","void moltres_egg_Hit(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_80_233_callbacks,{},{},d_80_233_headers,{},{},d_80_233_services},
{0x802ddc30u,"src/volcano/moltres_egg/code.c",98,106,"POKEMON_FUNC(moltres_egg_FallIntoLava)","void moltres_egg_FallIntoLava(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_98_234_services},
{0x802ddca0u,"src/volcano/moltres_egg/code.c",108,116,"POKEMON_FUNC(moltres_egg_Jump)","void moltres_egg_Jump(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_108_235_services},
{0x802ddd04u,"src/volcano/moltres_egg/code.c",118,138,"POKEMON_FUNC(moltres_egg_SpawnMoltres)","void moltres_egg_SpawnMoltres(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_118_236_callbacks,{},{},d_118_236_headers,{},{},d_118_236_services},
{0x802dde00u,"src/volcano/moltres_egg/code.c",140,152,"POKEMON_FUNC(moltres_egg_DrownInLava)","void moltres_egg_DrownInLava(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_140_237_services},
{0x802ddec0u,"src/volcano/moltres_egg/code.c",154,156,"GObj* moltres_egg_Spawn(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)","IsleGameObject* moltres_egg_Spawn(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn)","non_gobj_exact_Source_ABI",{},{},{},{},d_154_238_child_factories,{},{}},
{0x802d7f60u,"src/volcano/rapidash/code.c",180,185,"POKEMON_FUNC(rapidash_InitialState)","void rapidash_InitialState(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_180_239_callbacks,{},{},{},{},{},d_180_239_services},
{0x802d7fa8u,"src/volcano/rapidash/code.c",187,194,"POKEMON_FUNC(rapidash_Idle)","void rapidash_Idle(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_187_240_graphs,d_187_240_weighted_states,d_187_240_headers,{},{},d_187_240_services},
{0x802d8018u,"src/volcano/rapidash/code.c",196,207,"POKEMON_FUNC(rapidash_Walk)","void rapidash_Walk(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_196_241_callbacks,d_196_241_graphs,d_196_241_weighted_states,d_196_241_headers,{},{},d_196_241_services},
{0x802d80bcu,"src/volcano/rapidash/code.c",209,215,"POKEMON_FUNC(rapidash_MoveWalk)","void rapidash_MoveWalk(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_209_242_services},
{0x802d8118u,"src/volcano/rapidash/code.c",217,228,"POKEMON_FUNC(rapidash_Run)","void rapidash_Run(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_217_243_callbacks,d_217_243_graphs,d_217_243_weighted_states,d_217_243_headers,{},{},d_217_243_services},
{0x802d81bcu,"src/volcano/rapidash/code.c",230,236,"POKEMON_FUNC(rapidash_MoveRun)","void rapidash_MoveRun(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_230_244_services},
{0x802d8218u,"src/volcano/rapidash/code.c",238,245,"POKEMON_FUNC(rapidash_HitByPesterBall)","void rapidash_HitByPesterBall(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_238_245_callbacks,d_238_245_graphs,{},d_238_245_headers,{},{},d_238_245_services},
{0x802d8288u,"src/volcano/rapidash/code.c",247,254,"POKEMON_FUNC(rapidash_HitByApple)","void rapidash_HitByApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_247_246_callbacks,d_247_246_graphs,{},d_247_246_headers,{},{},d_247_246_services},
{0x802d82f8u,"src/volcano/rapidash/code.c",256,263,"POKEMON_FUNC(rapidash_AffectedByPesterBall)","void rapidash_AffectedByPesterBall(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_256_247_callbacks,d_256_247_graphs,{},d_256_247_headers,{},{},d_256_247_services},
{0x802d8368u,"src/volcano/rapidash/code.c",265,279,"POKEMON_FUNC(rapidash_AppleNearby)","void rapidash_AppleNearby(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_265_248_callbacks,d_265_248_graphs,{},d_265_248_headers,{},{},d_265_248_services},
{0x802d842cu,"src/volcano/rapidash/code.c",281,286,"POKEMON_FUNC(rapidash_TurnToApple)","void rapidash_TurnToApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_281_249_services},
{0x802d8478u,"src/volcano/rapidash/code.c",288,297,"POKEMON_FUNC(rapidash_RunToApple)","void rapidash_RunToApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},d_288_250_headers,{},{},d_288_250_services},
{0x802d84fcu,"src/volcano/rapidash/code.c",299,312,"POKEMON_FUNC(rapidash_EatApple)","void rapidash_EatApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_299_251_callbacks,d_299_251_graphs,{},d_299_251_headers,{},{},d_299_251_services},
{0x802d859cu,"src/volcano/rapidash/code.c",314,316,"POKEMON_FUNC(rapidash_SearchApple)","void rapidash_SearchApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_314_252_callbacks,{},{},{},{},{},d_314_252_services},
{0x802d85c0u,"src/volcano/rapidash/code.c",318,325,"POKEMON_FUNC(rapidash_Recover)","void rapidash_Recover(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_318_253_callbacks,d_318_253_graphs,{},{},{},{},d_318_253_services},
{0x802d8620u,"src/volcano/rapidash/code.c",327,334,"POKEMON_FUNC(rapidash_HearsFlute)","void rapidash_HearsFlute(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_327_254_callbacks,d_327_254_graphs,{},d_327_254_headers,{},{},d_327_254_services},
{0x802d8694u,"src/volcano/rapidash/code.c",336,341,"POKEMON_FUNC(rapidash_TurnToPlayer)","void rapidash_TurnToPlayer(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_336_255_services},
{0x802d86e0u,"src/volcano/rapidash/code.c",343,350,"POKEMON_FUNC(rapidash_IdleBeforeRun)","void rapidash_IdleBeforeRun(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_343_256_graphs,{},d_343_256_headers,{},{},d_343_256_services},
{0x802d8750u,"src/volcano/rapidash/code.c",352,359,"POKEMON_FUNC(rapidash_HitBeforeRun)","void rapidash_HitBeforeRun(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_352_257_callbacks,d_352_257_graphs,{},d_352_257_headers,{},{},d_352_257_services},
{0x802d87c0u,"src/volcano/rapidash/code.c",361,368,"POKEMON_FUNC(rapidash_AffectedByItemBeforeRun)","void rapidash_AffectedByItemBeforeRun(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_361_258_callbacks,d_361_258_graphs,{},d_361_258_headers,{},{},d_361_258_services},
{0x802d8830u,"src/volcano/rapidash/code.c",370,373,"POKEMON_FUNC(rapidash_StartRun)","void rapidash_StartRun(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_370_259_callbacks,{},{},{},{},{},d_370_259_services},
{0x802d8868u,"src/volcano/rapidash/code.c",375,382,"POKEMON_FUNC(rapidash_RunAlongPath)","void rapidash_RunAlongPath(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_375_260_graphs,{},d_375_260_headers,{},{},d_375_260_services},
{0x802d88d0u,"src/volcano/rapidash/code.c",384,390,"POKEMON_FUNC(rapidash_MoveRunAlongPath)","void rapidash_MoveRunAlongPath(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_384_261_services},
{0x802d893cu,"src/volcano/rapidash/code.c",392,400,"POKEMON_FUNC(rapidash_RunningHit)","void rapidash_RunningHit(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_392_262_callbacks,{},{},d_392_262_headers,{},{},d_392_262_services},
{0x802d89b0u,"src/volcano/rapidash/code.c",402,416,"POKEMON_FUNC(rapidash_RunningAffectedByItem)","void rapidash_RunningAffectedByItem(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_402_263_callbacks,{},{},d_402_263_headers,{},{},d_402_263_services},
{0x802d8a5cu,"src/volcano/rapidash/code.c",442,444,"GObj* rapidash_Spawn(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)","IsleGameObject* rapidash_Spawn(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn)","non_gobj_exact_Source_ABI",{},{},{},{},d_442_264_child_factories,{},{}},
{0x802de570u,"src/volcano/smoke_puff/code.c",49,65,"POKEMON_FUNC(smoke_puff_InitialState)","void smoke_puff_InitialState(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_49_265_callbacks,d_49_265_graphs,{},d_49_265_headers,{},{},d_49_265_services},
{0x802de648u,"src/volcano/smoke_puff/code.c",67,73,"POKEMON_FUNC(smoke_puff_Rise)","void smoke_puff_Rise(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_67_266_services},
{0x802de6b4u,"src/volcano/smoke_puff/code.c",75,77,"GObj* smoke_puff_Spawn(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)","IsleGameObject* smoke_puff_Spawn(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn)","non_gobj_exact_Source_ABI",{},{},{},{},d_75_267_child_factories,{},{}},
{0x802de6f0u,"src/volcano/smoke_spawner/code.c",77,107,"void smoke_spawner_CreateSmokePuff(GObj* obj)","void smoke_spawner_CreateSmokePuff(IsleGameObject* obj)","gobj_void_single_argument",{},{},{},{},d_77_268_child_factories,{},d_77_268_services},
{0x802de7dcu,"src/volcano/smoke_spawner/code.c",109,139,"void smoke_spawner_CreateKoffingSmoke(GObj* obj)","void smoke_spawner_CreateKoffingSmoke(IsleGameObject* obj)","gobj_void_single_argument",{},{},{},{},d_109_269_child_factories,{},d_109_269_services},
{0x802de8c8u,"src/volcano/smoke_spawner/code.c",141,146,"POKEMON_FUNC(smoke_spawner_InitialState)","void smoke_spawner_InitialState(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_141_270_callbacks,{},{},{},{},{},d_141_270_services},
{0x802de91cu,"src/volcano/smoke_spawner/code.c",148,153,"POKEMON_FUNC(smoke_spawner_Idle)","void smoke_spawner_Idle(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_148_271_graphs,{},{},{},{},d_148_271_services},
{0x802de95cu,"src/volcano/smoke_spawner/code.c",155,165,"POKEMON_FUNC(smoke_spawner_PesterBallThrown)","void smoke_spawner_PesterBallThrown(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_155_272_callbacks,{},d_155_272_weighted_states,{},{},{},d_155_272_services},
{0x802de9c4u,"src/volcano/smoke_spawner/code.c",167,170,"POKEMON_FUNC(smoke_spawner_SpawnKoffingSmoke)","void smoke_spawner_SpawnKoffingSmoke(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_167_273_callbacks,{},{},{},{},{},d_167_273_services},
{0x802dea04u,"src/volcano/smoke_spawner/code.c",172,177,"POKEMON_FUNC(smoke_spawner_WaitSmokeFade)","void smoke_spawner_WaitSmokeFade(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_172_274_graphs,{},{},{},{},d_172_274_services},
{0x802dea44u,"src/volcano/smoke_spawner/code.c",179,181,"GObj* smoke_spawner_Spawn(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)","IsleGameObject* smoke_spawner_Spawn(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn)","non_gobj_exact_Source_ABI",{},{},{},{},d_179_275_child_factories,{},{}},
{0x802dfb80u,"src/volcano/sounds.c",14,123,"void volcano_UpdateSounds(s32 blockCount, f32 blockPart)","void volcano_UpdateSounds(IsleVolcanoActorSource* s, int32_t blockCount, float blockPart)","non_gobj_exact_Source_ABI",{},{},{},{},{},{},d_14_276_services},
{0x802dfa70u,"src/volcano/volcano_smoke/code.c",50,52,"POKEMON_FUNC(volcano_smoke_InitialState)","void volcano_smoke_InitialState(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_50_277_callbacks,{},{},{},{},{},d_50_277_services},
{0x802dfa94u,"src/volcano/volcano_smoke/code.c",54,60,"POKEMON_FUNC(volcano_smoke_Idle)","void volcano_smoke_Idle(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},d_54_278_headers,{},{},d_54_278_services},
{0x802dfae4u,"src/volcano/volcano_smoke/code.c",62,66,"POKEMON_FUNC(volcano_smoke_Render)","void volcano_smoke_Render(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_62_279_services},
{0x802dfb44u,"src/volcano/volcano_smoke/code.c",68,70,"GObj* volcano_smoke_Spawn(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)","IsleGameObject* volcano_smoke_Spawn(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn)","non_gobj_exact_Source_ABI",{},{},{},{},d_68_280_child_factories,{},{}},
{0x802d9d00u,"src/volcano/vulpix/code.c",209,226,"s32 vulpix_CloseToPathEnd(GObj* obj)","int32_t vulpix_CloseToPathEnd(IsleGameObject* obj)","non_gobj_exact_Source_ABI",{},{},{},{},{},{},d_209_281_services},
{0x802d9dfcu,"src/volcano/vulpix/code.c",228,237,"POKEMON_FUNC(vulpix_SendRunCommand)","void vulpix_SendRunCommand(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},d_228_282_helpers,d_228_282_services},
{0x802d9e7cu,"src/volcano/vulpix/code.c",239,252,"POKEMON_FUNC(vulpix_DeleteLater)","void vulpix_DeleteLater(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_239_283_services},
{0x802d9f24u,"src/volcano/vulpix/code.c",254,263,"POKEMON_FUNC(vulpix_InitialState)","void vulpix_InitialState(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_254_284_callbacks,{},{},{},{},{},d_254_284_services},
{0,"src/volcano/vulpix/code.c",265,266,"static void nullsub()","void volcano_vulpix_nullsub(IsleVolcanoActorSource* s)","non_gobj_exact_Source_ABI",{},{},{},{},{},{},d_265_285_services},
{0x802d9f90u,"src/volcano/vulpix/code.c",268,275,"POKEMON_FUNC(vulpix_Idle)","void vulpix_Idle(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},d_268_286_graphs,d_268_286_weighted_states,d_268_286_headers,{},{},d_268_286_services},
{0x802da000u,"src/volcano/vulpix/code.c",277,289,"POKEMON_FUNC(vulpix_Walk)","void vulpix_Walk(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_277_287_callbacks,d_277_287_graphs,d_277_287_weighted_states,d_277_287_headers,{},{},d_277_287_services},
{0x802da0a4u,"src/volcano/vulpix/code.c",291,297,"POKEMON_FUNC(vulpix_MoveWalk)","void vulpix_MoveWalk(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_291_288_services},
{0x802da100u,"src/volcano/vulpix/code.c",299,311,"POKEMON_FUNC(vulpix_Run)","void vulpix_Run(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_299_289_callbacks,d_299_289_graphs,d_299_289_weighted_states,d_299_289_headers,{},{},d_299_289_services},
{0x802da1a4u,"src/volcano/vulpix/code.c",313,319,"POKEMON_FUNC(vulpix_MoveRun)","void vulpix_MoveRun(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_313_290_services},
{0x802da200u,"src/volcano/vulpix/code.c",321,338,"POKEMON_FUNC(vulpix_HitByPesterBall)","void vulpix_HitByPesterBall(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_321_291_callbacks,{},{},d_321_291_headers,{},{},d_321_291_services},
{0x802da2c8u,"src/volcano/vulpix/code.c",340,347,"POKEMON_FUNC(vulpix_HitByApple)","void vulpix_HitByApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_340_292_callbacks,{},{},d_340_292_headers,{},{},d_340_292_services},
{0x802da330u,"src/volcano/vulpix/code.c",349,356,"POKEMON_FUNC(vulpix_AffectedByPesterBall)","void vulpix_AffectedByPesterBall(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_349_293_callbacks,d_349_293_graphs,{},d_349_293_headers,{},{},d_349_293_services},
{0x802da3a0u,"src/volcano/vulpix/code.c",358,368,"POKEMON_FUNC(vulpix_AppleNearby)","void vulpix_AppleNearby(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_358_294_callbacks,d_358_294_graphs,{},{},{},{},d_358_294_services},
{0x802da428u,"src/volcano/vulpix/code.c",370,378,"POKEMON_FUNC(vulpix_RunToApple)","void vulpix_RunToApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},d_370_295_headers,{},{},d_370_295_services},
{0x802da4acu,"src/volcano/vulpix/code.c",380,395,"POKEMON_FUNC(vulpix_EatApple)","void vulpix_EatApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_380_296_callbacks,d_380_296_graphs,{},d_380_296_headers,{},{},d_380_296_services},
{0x802da55cu,"src/volcano/vulpix/code.c",397,402,"POKEMON_FUNC(vulpix_TurnToApple)","void vulpix_TurnToApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_397_297_services},
{0x802da5b4u,"src/volcano/vulpix/code.c",404,406,"POKEMON_FUNC(vulpix_SearchApple)","void vulpix_SearchApple(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_404_298_callbacks,{},{},{},{},{},d_404_298_services},
{0x802da5d8u,"src/volcano/vulpix/code.c",408,415,"POKEMON_FUNC(vulpix_Recover)","void vulpix_Recover(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_408_299_callbacks,d_408_299_graphs,{},{},{},{},d_408_299_services},
{0,"src/volcano/vulpix/code.c",417,418,"static void nullsub2()","void volcano_vulpix_nullsub2(IsleVolcanoActorSource* s)","unemitted-file-local-original-empty",{},{},{},{},{},{},d_417_300_services},
{0x802da640u,"src/volcano/vulpix/code.c",420,425,"POKEMON_FUNC(vulpix_unused_TurnToPlayer)","void vulpix_unused_TurnToPlayer(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_420_301_services},
{0x802da68cu,"src/volcano/vulpix/code.c",427,441,"POKEMON_FUNC(vulpix_RunAwayFromPlayer)","void vulpix_RunAwayFromPlayer(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_427_302_callbacks,d_427_302_graphs,{},d_427_302_headers,{},{},d_427_302_services},
{0x802da748u,"src/volcano/vulpix/code.c",443,449,"POKEMON_FUNC(vulpix_MoveRunAway)","void vulpix_MoveRunAway(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_443_303_services},
{0x802da7a4u,"src/volcano/vulpix/code.c",451,463,"POKEMON_FUNC(vulpix_RunAlongPath)","void vulpix_RunAlongPath(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_451_304_callbacks,d_451_304_graphs,{},d_451_304_headers,{},{},d_451_304_services},
{0x802da848u,"src/volcano/vulpix/code.c",465,471,"POKEMON_FUNC(vulpix_MoveRunToPathEnd)","void vulpix_MoveRunToPathEnd(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_465_305_services},
{0x802da8a4u,"src/volcano/vulpix/code.c",473,483,"POKEMON_FUNC(vulpix_IdleAtPathEnd)","void vulpix_IdleAtPathEnd(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_473_306_callbacks,d_473_306_graphs,{},d_473_306_headers,{},{},d_473_306_services},
{0x802da930u,"src/volcano/vulpix/code.c",485,490,"POKEMON_FUNC(vulpix_TurnToPlayer)","void vulpix_TurnToPlayer(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",{},{},{},{},{},{},d_485_307_services},
{0x802da97cu,"src/volcano/vulpix/code.c",492,507,"POKEMON_FUNC(vulpix_RunningHitByPesterBall)","void vulpix_RunningHitByPesterBall(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_492_308_callbacks,d_492_308_graphs,{},d_492_308_headers,{},{},d_492_308_services},
{0x802daa34u,"src/volcano/vulpix/code.c",509,516,"POKEMON_FUNC(vulpix_RunningHit)","void vulpix_RunningHit(IsleGameObject* obj)","gobj_process_state_aux_path_void_GObj_ptr",d_509_309_callbacks,{},{},d_509_309_headers,{},{},d_509_309_services},
{0x802daa9cu,"src/volcano/vulpix/code.c",518,520,"GObj* vulpix_Spawn(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn)","IsleGameObject* vulpix_Spawn(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn)","non_gobj_exact_Source_ABI",{},{},{},{},d_518_310_child_factories,{},{}},
{0x800fff90u,"src/volcano/world/world.c",29,30,"void volcano_stub()","void volcano_stub(IsleVolcanoLifecycleSource* l)","non_gobj_exact_Source_ABI",{},{},{},{},{},{},{}}
};
constexpr std::uint32_t graphs[]={0x802e2804u,0x802e28b4u,0x802e2934u,0x802e2984u,0x802e29b4u,0x802e29e4u,0x802e2a24u,0x802e2ec8u,0x802e2f08u,0x802e1a34u,0x802e1b14u,0x802e1ba4u,0x802e1c44u,0x802e1c84u,0x802e1cc4u,0x802e1d14u,0x802e1da4u,0x802e1df4u,0x802e1e90u,0x802e1eb0u,0x802e1ed0u,0x802e1ef0u,0x802e1f40u,0x802e1f60u,0x802e1f80u,0x802e2ba0u,0x802e2be0u,0x802e2c00u,0x802e2ca4u,0x802e2d24u,0x802e2d54u,0x802e2da4u,0x802e3454u,0x802e3474u,0x802e3708u,0x802e2444u,0x802e24f4u,0x802e2574u,0x802e25f4u,0x802e2624u,0x802e2654u,0x802e2694u,0x802e2fa4u,0x802e3230u,0x802e354cu,0x802e3648u,0x802e10a4u,0x802e10c4u,0x802e1154u,0x802e11f4u,0x802e1294u,0x802e1324u,0x802e1374u,0x802e13c4u,0x802e13f4u,0x802e1434u,0x802e14a4u,0x802e314cu,0x802e317cu,0x802e3038u,0x802e15b8u,0x802e1648u,0x802e16c8u,0x802e1718u,0x802e1748u,0x802e1778u,0x802e17b8u,0x802e1818u,0x802e1878u,0x802e32a4u,0x802e3378u,0x802e3398u,0x802e3794u,0x802e2098u,0x802e2118u,0x802e2178u,0x802e21d8u,0x802e2208u,0x802e2238u,0x802e2278u,0x802e2304u};
constexpr std::uint32_t weighted[]={0x802e2a64u,0x802e2f28u,0x802e1e64u,0x802e2c40u,0x802e2c70u,0x802e2c88u,0x802e2d04u,0x802e26d4u,0x802e1484u,0x802e17f8u,0x802e33b8u,0x802e22d8u};
constexpr std::uint32_t roots[]={0x802db5a0u,0x802dd2e0u,0x802d8bb8u,0x802dc108u,0x802dea80u,0x802df3b0u,0x802daae0u,0x802dd7f0u,0x802de390u,0x802dede0u,0x802df280u,0x802d73a8u,0x802ddf00u,0x802ddad0u,0x802d7f60u,0x802de570u,0x802de8c8u,0x802dfa70u,0x802d9f24u,0x8035df20u};
const ActorFactoryBinding factories[]={
{59,0x802dc018u,0x802e2aa8u,0x802db5a0u,ActorFactoryWrapper::ground,arcanine_Spawn},
{6,0x802dd7acu,0x802e2f54u,0x802dd2e0u,ActorFactoryWrapper::ground,charizard_Spawn},
{4,0x802d9cb8u,0x802e1fb4u,0x802d8bb8u,ActorFactoryWrapper::ground,charmander_Spawn},
{5,0x802dd214u,0x802e2e08u,0x802dc108u,ActorFactoryWrapper::ground,charmeleon_Spawn},
{1002,0x802ded34u,0x802e34a8u,0x802dea80u,ActorFactoryWrapper::ground_dl4,evolution_controller_Spawn},
{1001,0x802dfa38u,0x802e3740u,0x802df3b0u,ActorFactoryWrapper::ordinary_dl4,volcano_gate_Spawn},
{58,0x802db558u,0x802e2710u,0x802daae0u,ActorFactoryWrapper::ground,growlithe_Spawn},
{1026,0x802dda98u,0x802e2fd8u,0x802dd7f0u,ActorFactoryWrapper::ordinary_dl4,growlithe_spawner_Spawn},
{1028,0x802de52cu,0x802e3254u,0x802de390u,ActorFactoryWrapper::ordinary,koffing_smoke_Spawn},
{1030,0x802df240u,0x802e3590u,0x802dede0u,ActorFactoryWrapper::ordinary_dl4,lava_splash_Spawn},
{129,0x802df378u,0x802e3674u,0x802df280u,ActorFactoryWrapper::ground,volcano_magikarp_Spawn},
{126,0x802d7f28u,0x802e14d8u,0x802d73a8u,ActorFactoryWrapper::ground,magmar_Spawn},
{146,0x802de34cu,0x802e31b0u,0x802ddf00u,ActorFactoryWrapper::ground,moltres_Spawn},
{600,0x802ddec0u,0x802e3080u,0x802ddad0u,ActorFactoryWrapper::ground,moltres_egg_Spawn},
{78,0x802d8a5cu,0x802e190cu,0x802d7f60u,ActorFactoryWrapper::ground,rapidash_Spawn},
{1029,0x802de6b4u,0x802e32c8u,0x802de570u,ActorFactoryWrapper::ordinary_dl4,smoke_puff_Spawn},
{1027,0x802dea44u,0x802e33dcu,0x802de8c8u,ActorFactoryWrapper::ordinary_dl4,smoke_spawner_Spawn},
{1031,0x802dfb44u,0x802e37b8u,0x802dfa70u,ActorFactoryWrapper::ordinary_dl4,volcano_smoke_Spawn},
{37,0x802daa9cu,0x802e2358u,0x802d9f24u,ActorFactoryWrapper::ground,vulpix_Spawn},
{1003,0x8035e0d4u,0x80388348u,0x8035df20u,ActorFactoryWrapper::ground_dl4,func_8035E0D4_4FE4E4,true}
};
const ActorChildFactoryDependency children[]={
{0x802dd24cu,635,1030,{0,0x802dd24cu,635,0x00000000u,5,2150486548,2151038644,2151039616},0,false,true},
{0x802ded6cu,146,1030,{0,0x802ded6cu,146,0x00000000u,1002,2150493492,2151038644,2151039616},0,false,true},
{0x802d60e0u,245,129,{0,0x802d60e0u,245,0x802e0e94u,129,2150495096,2151038644,2151039616},0x3ff,false,false},
{0x802d6864u,429,59,{0,0x802d6864u,429,0x802e0f64u,59,2150481944,2151038644,2151039616},0x3ff,false,false},
{0x802d693cu,459,58,{0,0x802d693cu,459,0x802e0f74u,58,2150479192,2151038644,2151039616},0x3ff,false,false},
{0x802d6a5cu,493,6,{0,0x802d6a5cu,493,0x802e0f84u,6,2150487980,2151038644,2151039616},0x3ff,false,false},
{0x802de6f0u,84,1029,{0,0x802de6f0u,84,0x802e3358u,1029,2150491828,2151038396,2151039616},0x7ff,true,false},
{0x802de7dcu,116,1028,{0,0x802de7dcu,116,0x802e3368u,1028,2150491436,2151038396,2151039616},0x7ff,true,false},
{0x802d6b2cu,0,1030,{0x802e0f94u,0,0,0,1030,2150494784,2151038396,2151039616},0,false,true},
{0x802d6b64u,0,1030,{0x802e0f94u,0,0,0,1030,2150494784,2151038396,2151039616},0,false,true},
{0x802d6bb0u,0,1030,{0x802e0f94u,0,0,0,1030,2150494784,2151038396,2151039616},0,false,true},
{0x802d6c38u,0,1030,{0x802e0f94u,0,0,0,1030,2150494784,2151038396,2151039616},0,false,true},
{0x802d6cc0u,0,1030,{0x802e0f94u,0,0,0,1030,2150494784,2151038396,2151039616},0,false,true},
{0x802d6cfcu,0,1030,{0x802e0f94u,0,0,0,1030,2150494784,2151038396,2151039616},0,false,true}
};
const ActorAnimationBinding animation[]={{0x802df508u,volcano_gate_PlayerAnimCallback}};
const ActorRenderBinding renders[]={{0x802de26cu,moltres_Render,0},{0x802dfae4u,volcano_smoke_Render,1}};
const ActorMovementBinding movements[]={{0x802d6e14u,volcano_PokemonMove}};
const ActorPathBinding paths[]={{0x802d6f68u,volcano_FollowPath}};
const ActorBlockChangeBinding changes[]={{0x802d6a14u,volcano_CharmeleonChangeBlock}};
const ActorCountedOption magikarp_options[]={{3,volcano_SpawnMagikarp},{7,nullptr}};
const ActorCountedOptions counted[]={{0x802e0ea4u,magikarp_options}};
} // namespace
std::span<const actor::BehaviorCallbackBinding> actor_callbacks() noexcept { return callbacks; }
std::span<const std::uint32_t> actor_graphs() noexcept { return graphs; }
std::span<const std::uint32_t> actor_weighted_states() noexcept { return weighted; }
std::span<const std::uint32_t> actor_roots() noexcept { return roots; }
std::span<const ActorFactoryBinding> actor_factories() noexcept { return factories; }
std::span<const ActorChildFactoryDependency> actor_child_factories() noexcept { return children; }
std::span<const ActorProviderDependency> actor_dependencies() noexcept { return dependencies; }
std::span<const ActorAnimationBinding> actor_animation_callbacks() noexcept { return animation; }
std::span<const ActorRenderBinding> actor_render_callbacks() noexcept { return renders; }
std::span<const ActorMovementBinding> actor_movement_callbacks() noexcept { return movements; }
std::span<const ActorPathBinding> actor_path_callbacks() noexcept { return paths; }
std::span<const ActorBlockChangeBinding> actor_block_change_callbacks() noexcept { return changes; }
std::span<const ActorCountedOptions> actor_counted_options() noexcept { return counted; }
std::span<const IsleItemCollisionCallback> actor_item_collision_callbacks() noexcept { static constexpr IsleItemCollisionCallback rows[]={volcano_HandleCollision};return rows; }
} // namespace isle::volcano

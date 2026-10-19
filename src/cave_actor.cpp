#include "cave_actor.hpp"

namespace isle::cave {
namespace {
const actor::BehaviorCallbackBinding callback_bindings[] = {
    {0x802bdd00, func_802BDD00_6401B0},
    {0x802be820, func_802BE820_640CD0},
    {0x802be898, func_802BE898_640D48},
    {0x802be8d4, func_802BE8D4_640D84},
    {0x802be944, func_802BE944_640DF4},
    {0x802be9b4, func_802BE9B4_640E64},
    {0x802bea24, func_802BEA24_640ED4},
    {0x802beac8, func_802BEAC8_640F78},
    {0x802beb24, func_802BEB24_640FD4},
    {0x802bebbc, func_802BEBBC_64106C},
    {0x802bec34, func_802BEC34_6410E4},
    {0x802beca8, func_802BECA8_641158},
    {0x802bed04, func_802BED04_6411B4},
    {0x802bed8c, func_802BED8C_64123C},
    {0x802bee10, func_802BEE10_6412C0},
    {0x802beea4, func_802BEEA4_641354},
    {0x802beef4, func_802BEEF4_6413A4},
    {0x802bef18, func_802BEF18_6413C8},
    {0x802bef8c, func_802BEF8C_64143C},
    {0x802bf020, func_802BF020_6414D0},
    {0x802bf044, func_802BF044_6414F4},
    {0x802bf0b4, func_802BF0B4_641564},
    {0x802bf124, func_802BF124_6415D4},
    {0x802bf1c4, func_802BF1C4_641674},
    {0x802bf220, func_802BF220_6416D0},
    {0x802bf2d0, func_802BF2D0_641780},
    {0x802bf338, func_802BF338_6417E8},
    {0x802bf3d0, func_802BF3D0_641880},
    {0x802bf454, func_802BF454_641904},
    {0x802bf4e8, func_802BF4E8_641998},
    {0x802bf5a0, func_802BF5A0_641A50},
    {0x802bf68c, func_802BF68C_641B3C},
    {0x802bf750, func_802BF750_641C00},
    {0x802bf7b4, func_802BF7B4_641C64},
    {0x802bf824, func_802BF824_641CD4},
    {0x802bf894, func_802BF894_641D44},
    {0x802bf9b4, func_802BF9B4_641E64},
    {0x802bfa3c, func_802BFA3C_641EEC},
    {0x802bfa8c, func_802BFA8C_641F3C},
    {0x802bfb24, func_802BFB24_641FD4},
    {0x802bfb9c, func_802BFB9C_64204C},
    {0x802bfbf0, func_802BFBF0_6420A0},
    {0x802bfc84, func_802BFC84_642134},
    {0x802bfde0, func_802BFDE0_642290},
    {0x802bfe34, func_802BFE34_6422E4},
    {0x802bfef0, func_802BFEF0_6423A0},
    {0x802bff74, func_802BFF74_642424},
    {0x802c0058, func_802C0058_642508},
    {0x802c00b0, func_802C00B0_642560},
    {0x802c0220, func_802C0220_6426D0},
    {0x802c027c, func_802C027C_64272C},
    {0x802c02bc, func_802C02BC_64276C},
    {0x802c0330, func_802C0330_6427E0},
    {0x802c03e4, func_802C03E4_642894},
    {0x802c04a4, func_802C04A4_642954},
    {0x802c0510, func_802C0510_6429C0},
    {0x802c05d8, func_802C05D8_642A88},
    {0x802c0630, func_802C0630_642AE0},
    {0x802c06bc, func_802C06BC_642B6C},
    {0x802c0748, func_802C0748_642BF8},
    {0x802c0810, evolveIntoMuk},
    {0x802c0888, func_802C0888_642D38},
    {0x802c0960, func_802C0960_642E10},
    {0x802c09c4, func_802C09C4_642E74},
    {0x802c0a28, func_802C0A28_642ED8},
    {0x802c0a8c, func_802C0A8C_642F3C},
    {0x802c0af0, func_802C0AF0_642FA0},
    {0x802c0bb0, func_802C0BB0_643060},
    {0x802c0cd8, func_802C0CD8_643188},
    {0x802c0d34, func_802C0D34_6431E4},
    {0x802c0e28, func_802C0E28_6432D8},
    {0x802c0e80, func_802C0E80_643330},
    {0x802c0f30, func_802C0F30_6433E0},
    {0x802c1018, func_802C1018_6434C8},
    {0x802c1100, func_802C1100_6435B0},
    {0x802c1124, func_802C1124_6435D4},
    {0x802c1178, func_802C1178_643628},
    {0x802c11e8, func_802C11E8_643698},
    {0x802c1288, func_802C1288_643738},
    {0x802c12e4, func_802C12E4_643794},
    {0x802c134c, func_802C134C_6437FC},
    {0x802c14c4, func_802C14C4_643974},
    {0x802c157c, func_802C157C_643A2C},
    {0x802c1660, func_802C1660_643B10},
    {0x802c1728, func_802C1728_643BD8},
    {0x802c1794, func_802C1794_643C44},
    {0x802c1818, func_802C1818_643CC8},
    {0x802c189c, func_802C189C_643D4C},
    {0x802c1920, func_802C1920_643DD0},
    {0x802c19bc, func_802C19BC_643E6C},
    {0x802c1a20, func_802C1A20_643ED0},
    {0x802c1a7c, func_802C1A7C_643F2C},
    {0x802c1ac8, func_802C1AC8_643F78},
    {0x802c1b1c, func_802C1B1C_643FCC},
    {0x802c1b54, func_802C1B54_644004},
    {0x802c1c48, func_802C1C48_6440F8},
    {0x802c1e4c, func_802C1E4C_6442FC},
    {0x802c1eb0, func_802C1EB0_644360},
    {0x802c1f08, func_802C1F08_6443B8},
    {0x802c1f74, func_802C1F74_644424},
    {0x802c2058, func_802C2058_644508},
    {0x802c2308, func_802C2308_6447B8},
    {0x802c2504, func_802C2504_6449B4},
    {0x802c257c, func_802C257C_644A2C},
    {0x802c25d8, func_802C25D8_644A88},
    {0x802c2650, func_802C2650_644B00},
    {0x802c26ac, func_802C26AC_644B5C},
    {0x802c2710, func_802C2710_644BC0},
    {0x802c2758, func_802C2758_644C08},
    {0x802c27e4, func_802C27E4_644C94},
    {0x802c2854, func_802C2854_644D04},
    {0x802c28d0, func_802C28D0_644D80},
    {0x802c294c, func_802C294C_644DFC},
    {0x802c29d0, func_802C29D0_644E80},
    {0x802c2ac4, func_802C2AC4_644F74},
    {0x802c2b0c, func_802C2B0C_644FBC},
    {0x802c2b54, func_802C2B54_645004},
    {0x802c2b9c, func_802C2B9C_64504C},
    {0x802c2bdc, func_802C2BDC_64508C},
    {0x802c2c90, func_802C2C90_645140},
    {0x802c2d00, func_802C2D00_6451B0},
    {0x802c2d6c, func_802C2D6C_64521C},
    {0x802c2d98, func_802C2D98_645248},
    {0x802c2e24, func_802C2E24_6452D4},
    {0x802c2ebc, func_802C2EBC_64536C},
    {0x802c2f48, func_802C2F48_6453F8},
    {0x802c2fb4, func_802C2FB4_645464},
    {0x802c2ff4, func_802C2FF4_6454A4},
    {0x802c30b8, func_802C30B8_645568},
    {0x802c31d0, func_802C31D0_645680},
    {0x802c3290, func_802C3290_645740},
    {0x802c3348, func_802C3348_6457F8},
    {0x802c33c0, func_802C33C0_645870},
    {0x802c33e4, func_802C33E4_645894},
    {0x802c3494, func_802C3494_645944},
    {0x802c34e0, func_802C34E0_645990},
    {0x802c3580, func_802C3580_645A30},
    {0x802c3658, func_802C3658_645B08},
    {0x802c36f0, func_802C36F0_645BA0},
    {0x802c3784, func_802C3784_645C34},
    {0x802c3840, func_802C3840_645CF0},
    {0x802c3874, func_802C3874_645D24},
    {0x802c38e0, func_802C38E0_645D90},
    {0x802c39b8, func_802C39B8_645E68},
    {0x802c3a44, func_802C3A44_645EF4},
    {0x802c3b34, func_802C3B34_645FE4},
    {0x802c3b68, func_802C3B68_646018},
    {0x802c3c40, func_802C3C40_6460F0},
    {0x802c3d44, func_802C3D44_6461F4},
    {0x802c3dfc, func_802C3DFC_6462AC},
    {0x802c3e74, func_802C3E74_646324},
    {0x802c3edc, func_802C3EDC_64638C},
    {0x802c3f6c, func_802C3F6C_64641C},
    {0x802c3fd8, func_802C3FD8_646488},
    {0x802c4090, func_802C4090_646540},
    {0x802c413c, func_802C413C_6465EC},
    {0x802c41d0, func_802C41D0_646680},
    {0x802c4264, func_802C4264_646714},
    {0x802c433c, func_802C433C_6467EC},
    {0x802c4378, func_802C4378_646828},
    {0x802c45f4, func_802C45F4_646AA4},
    {0x802c46a0, func_802C46A0_646B50},
    {0x802c46f0, func_802C46F0_646BA0},
    {0x802c4820, func_802C4820_646CD0},
    {0x802c493c, func_802C493C_646DEC},
    {0x802c49f0, func_802C49F0_646EA0},
    {0x802c4a4c, func_802C4A4C_646EFC},
    {0x802c4aa8, func_802C4AA8_646F58},
    {0x802c4b04, func_802C4B04_646FB4},
    {0x802c4c18, func_802C4C18_6470C8},
    {0x802c4c70, func_802C4C70_647120},
    {0x802c4d60, func_802C4D60_647210},
    {0x802c4ef4, func_802C4EF4_6473A4},
    {0x802c4f5c, func_802C4F5C_64740C},
    {0x802c4fb8, func_802C4FB8_647468},
    {0x802c502c, func_802C502C_6474DC},
    {0x802c5188, func_802C5188_647638},
    {0x802c5230, func_802C5230_6476E0},
    {0x802c5380, func_802C5380_647830},
    {0x802c53f4, func_802C53F4_6478A4},
    {0x802c5468, func_802C5468_647918},
    {0x802c54dc, func_802C54DC_64798C},
    {0x802c5580, func_802C5580_647A30},
    {0x802c5620, func_802C5620_647AD0},
    {0x802c5644, func_802C5644_647AF4},
    {0x802c569c, func_802C569C_647B4C},
    {0x802c57a4, func_802C57A4_647C54},
    {0x802c58e4, func_802C58E4_647D94},
    {0x802c5970, func_802C5970_647E20},
};
const actor::BehaviorCallbackBinding lifecycle_callback_bindings[] = {
    {0x802be09c, func_802BE09C_64054C},
    {0x802c5b60, func_802C5B60_648010},
    {0x802c5cec, func_802C5CEC_64819C},
    {0x802c5d80, func_802C5D80_648230},
    {0x802c5dfc, func_802C5DFC_6482AC},
};
const ActorFactoryBinding factory_bindings[] = {
    {0x1,0x802befd8,0x802c6824,0x802be898,ActorFactoryWrapper::ground,isle_cave_actor_factory_func_802BEFD8_641488,nullptr,func_802BEFD8_641488,true},
    {0x84,0x802bf55c,0x802c69c0,0x802bf020,ActorFactoryWrapper::ground,isle_cave_actor_factory_func_802BF55C_641A0C,nullptr,func_802BF55C_641A0C,true},
    {0x46,0x802bfda0,0x802c6b5c,0x802bf5a0,ActorFactoryWrapper::ground,isle_cave_actor_factory_func_802BFDA0_642250,nullptr,func_802BFDA0_642250,true},
    {0x47,0x802c0104,0x802c6c64,0x802bfde0,ActorFactoryWrapper::ground,isle_cave_actor_factory_func_802C0104_6425B4,nullptr,func_802C0104_6425B4,true},
    {0x6d,0x802c07d4,0x802c6f14,0x802c0220,ActorFactoryWrapper::ordinary,isle_cave_actor_factory_func_802C07D4_642C84,nullptr,func_802C07D4_642C84,true},
    {0x58,0x802c10c8,0x802c7040,0x802c0888,ActorFactoryWrapper::ground,isle_cave_actor_factory_func_802C10C8_643578,nullptr,func_802C10C8_643578,true},
    {0x59,0x802c13b4,0x802c714c,0x802c1124,ActorFactoryWrapper::ground,isle_cave_actor_factory_func_802C13B4_643864,nullptr,func_802C13B4_643864,true},
    {0x59,0x802c13ec,0x802c7194,0x802c1100,ActorFactoryWrapper::ground,isle_cave_actor_factory_func_802C13EC_64389C,nullptr,func_802C13EC_64389C,true},
    {0x27,0x802c2994,0x802c7590,0x802c14c4,ActorFactoryWrapper::ordinary,isle_cave_actor_factory_func_802C2994_644E44,nullptr,func_802C2994_644E44,true},
    {0x29,0x802c3250,0x802c7728,0x802c29d0,ActorFactoryWrapper::ordinary,isle_cave_actor_factory_func_802C3250_645700,nullptr,func_802C3250_645700,true},
    {0x81,0x802c3388,0x802c77fc,0x802c3290,ActorFactoryWrapper::ground,isle_cave_actor_factory_func_802C3388_645838,nullptr,func_802C3388_645838,true},
    {0x19,0x802c465c,0x802c7ab4,0x802c33c0,ActorFactoryWrapper::ground,isle_cave_actor_factory_func_802C465C_646B0C,nullptr,func_802C465C_646B0C,true},
    {0x90,0x802c49a8,0x802c7b9c,0x802c46a0,ActorFactoryWrapper::ordinary,func_802C49A8_646E58,nullptr,nullptr,false},
    {0x259,0x802c4e64,0x802c7ccc,0x802c49f0,ActorFactoryWrapper::ground,func_802C4E64_647314,nullptr,nullptr,false},
    {0x7c,0x802c5548,0x802c7ec0,0x802c4ef4,ActorFactoryWrapper::ordinary,isle_cave_actor_factory_func_802C5548_6479F8,nullptr,func_802C5548_6479F8,true},
    {0x3fa,0x802c55d4,0x802c7f30,0x802c5580,ActorFactoryWrapper::ordinary,nullptr,func_802C55D4_647A84,nullptr,false},
    {0x3e9,0x802c5b28,0x802c8000,0x802c5620,ActorFactoryWrapper::ordinary_dl4,isle_cave_actor_factory_func_802C5B28_647FD8,nullptr,func_802C5B28_647FD8,true},
};
constexpr ActorHeaderBinding header_bindings[] = {
    {0x802c6824,0x802c6538},
    {0x802c6824,0x802c6574},
    {0x802c6824,0x802c654c},
    {0x802c6824,0x802c6560},
    {0x802c6824,0x802c65c4},
    {0x802c6824,0x802c659c},
    {0x802c6824,0x802c6588},
    {0x802c6824,0x802c65b0},
    {0x802c69c0,0x802c6870},
    {0x802c69c0,0x802c6910},
    {0x802c69c0,0x802c6884},
    {0x802c69c0,0x802c68c0},
    {0x802c69c0,0x802c68d4},
    {0x802c69c0,0x802c68e8},
    {0x802c69c0,0x802c68ac},
    {0x802c69c0,0x802c6898},
    {0x802c69c0,0x802c68fc},
    {0x802c6b5c,0x802c6a18},
    {0x802c6b5c,0x802c6a40},
    {0x802c6b5c,0x802c6a2c},
    {0x802c6b5c,0x802c6a54},
    {0x802c6b5c,0x802c6a68},
    {0x802c6b5c,0x802c6a7c},
    {0x802c6b5c,0x802c6a90},
    {0x802c6c64,0x802c6ba4},
    {0x802c6c64,0x802c6bb8},
    {0x802c6c64,0x802c6bcc},
    {0x802c6f14,0x802c6cd4},
    {0x802c6f14,0x802c6ce8},
    {0x802c6f14,0x802c6d38},
    {0x802c6f14,0x802c6cfc},
    {0x802c6f14,0x802c6d10},
    {0x802c6f14,0x802c6d24},
    {0x802c7040,0x802c6f60},
    {0x802c7040,0x802c6f74},
    {0x802c7040,0x802c6f88},
    {0x802c7040,0x802c6f9c},
    {0x802c7040,0x802c6fb0},
    {0x802c714c,0x802c708c},
    {0x802c714c,0x802c70dc},
    {0x802c714c,0x802c70a0},
    {0x802c714c,0x802c70b4},
    {0x802c714c,0x802c70c8},
    {0x802c7194,0x802c708c},
    {0x802c7194,0x802c70a0},
    {0x802c7194,0x802c70b4},
    {0x802c7194,0x802c70c8},
    {0x802c7590,0x802c71fc},
    {0x802c7590,0x802c7260},
    {0x802c7590,0x802c7238},
    {0x802c7590,0x802c724c},
    {0x802c7590,0x802c7210},
    {0x802c7590,0x802c7224},
    {0x802c7590,0x802c7274},
    {0x802c7590,0x802c7288},
    {0x802c7728,0x802c75d8},
    {0x802c77fc,0x802c779c},
    {0x802c7ab4,0x802c7880},
    {0x802c7ab4,0x802c78bc},
    {0x802c7ab4,0x802c78d0},
    {0x802c7ab4,0x802c7934},
    {0x802c7ab4,0x802c790c},
    {0x802c7ab4,0x802c7894},
    {0x802c7ab4,0x802c78e4},
    {0x802c7ab4,0x802c786c},
    {0x802c7b9c,0x802c7b10},
    {0x802c7b9c,0x802c7b38},
    {0x802c7b9c,0x802c7b24},
    {0x802c7ccc,0x802c7bd0},
    {0x802c7ccc,0x802c7c0c},
    {0x802c7ccc,0x802c7be4},
    {0x802c7ccc,0x802c7bf8},
    {0x802c7ec0,0x802c7d1c},
    {0x802c7ec0,0x802c7d44},
    {0x802c7ec0,0x802c7d30},
    {0x802c7ec0,0x802c7d58},
    {0x802c7ec0,0x802c7d6c},
    {0x802c7ec0,0x802c7d80},
    {0x802c7f30,0x802c7f08},
    {0x802c8000,0x802c7f70},
    {0x802c8000,0x802c7f84},
    {0x802c8000,0x802c7f98},
    {0x802c8000,0x802c7fac},
    {0x80388348,0x80388310},
};
const IsleActorRandomState weighted_802c67e8[] = {
    {100,func_802BE8D4_640D84},
    {100,func_802BEA24_640ED4},
    {100,func_802BE944_640DF4},
    {100,func_802BE9B4_640E64},
    {0,nullptr},
};
const IsleActorRandomState weighted_802c6964[] = {
    {100,func_802BF0B4_641564},
    {100,func_802BF124_6415D4},
    {0,nullptr},
};
const IsleActorRandomState weighted_802c6b28[] = {
    {100,func_802BF824_641CD4},
    {100,func_802BF7B4_641C64},
    {100,func_802BF68C_641B3C},
    {0,nullptr},
};
const IsleActorRandomState weighted_802c7014[] = {
    {100,func_802C0AF0_642FA0},
    {100,func_802C0BB0_643060},
    {0,nullptr},
};
const IsleActorRandomState weighted_802c7120[] = {
    {100,func_802C1178_643628},
    {100,func_802C11E8_643698},
    {0,nullptr},
};
const ActorWeightedBinding weighted_bindings[] = {
    {0x802c67e8,weighted_802c67e8},
    {0x802c6964,weighted_802c6964},
    {0x802c6b28,weighted_802c6b28},
    {0x802c7014,weighted_802c7014},
    {0x802c7120,weighted_802c7120},
};
const IsleCaveCountedOption counted_rows[] = {{3,func_802BDD00_6401B0},{7,nullptr}};
const ActorCountedOptionsBinding counted_options[] = {{0x802c6368,counted_rows}};
constexpr ActorAutomaticAggregate automatic_aggregates[] = {
    {0x802bdd00,29,1,"src/cave/6401B0.c","spawn","ObjectSpawn"},
    {0x802bdd00,31,1,"src/cave/6401B0.c","def","PokemonDef"},
    {0x802be820,36,1,"src/cave/640CD0.c","def","PokemonDef"},
    {0x802c0140,31,3,"src/cave/6425F0.c","sp38","InteractionHandler3"},
    {0x802c03e4,96,6,"src/cave/6425F0.c","sp20","InteractionHandler6"},
    {0x802c0630,158,5,"src/cave/6425F0.c","sp18","InteractionHandler5"},
    {0x802c06bc,168,5,"src/cave/6425F0.c","sp18","InteractionHandler5"},
    {0x802c0748,178,5,"src/cave/6425F0.c","sp18","InteractionHandler5"},
    {0x802c0810,38,1,"src/cave/642CC0.c","def","PokemonDef"},
    {0x802c1660,115,7,"src/cave/6438F0.c","sp20","InteractionHandler7"},
    {0x802c1794,141,4,"src/cave/6438F0.c","sp18","InteractionHandler4"},
    {0x802c1818,151,4,"src/cave/6438F0.c","sp18","InteractionHandler4"},
    {0x802c189c,161,4,"src/cave/6438F0.c","sp18","InteractionHandler4"},
    {0x802c1f74,335,6,"src/cave/6438F0.c","sp18","InteractionHandler6"},
    {0x802c2758,505,6,"src/cave/6438F0.c","sp18","InteractionHandler6"},
    {0x802c2854,526,3,"src/cave/6438F0.c","sp18","InteractionHandler3"},
    {0x802c28d0,535,3,"src/cave/6438F0.c","sp18","InteractionHandler3"},
    {0x802c2e24,147,2,"src/cave/644E80.c","sp18","InteractionHandler2"},
    {0x802c2ff4,194,4,"src/cave/644E80.c","sp20","InteractionHandler4"},
    {0x802c36f0,88,4,"src/cave/645870.c","sp18","InteractionHandler4"},
    {0x802c3d44,154,8,"src/cave/645D90.c","sp20","InteractionHandler8"},
    {0x802c3fd8,217,5,"src/cave/645D90.c","sp20","InteractionHandler5"},
};
const ActorChildFactoryDependency child_factories[] = {
    {0x802bdd00,{0x0,0x802bdd00,0x81,0x802c3388,0x80363eb4,0x80364280,0x802c77fc,0x802c6358,0x1f},31,0x3ff,"src/cave/6401B0.c"},
    {0x802be820,{0x0,0x802be820,0x84,0x802bf55c,0x80363eb4,0x80364280,0x802c69c0,0x802c65d8,0x24},36,0x3ff,"src/cave/640CD0.c"},
    {0x802c0810,{0x0,0x802c0810,0x59,0x802c13b4,0x80363dbc,0x80364280,0x802c714c,0x802c6fc4,0x26},38,0x3ff,"src/cave/642CC0.c"},
    {0x8035e1d4,{0x8038837c,0,1003,0x8035e0d4,0x80363dbc,0x80364280,0x80388348,0,0},51,0x3ff,"src/app_level/4FE330.c"},
};
constexpr ActorDependencyEvent d_802bdd00_calls[] = {
    {"getCurrentWorldBlock",0x800e2184,33,false},
    {"pokemonAddOne",0x80363c48,45,false},
    {"GET_TRANSFORM",0x0,49,false},
    {"GET_TRANSFORM",0x0,50,false},
    {"GET_TRANSFORM",0x0,51,false},
};
constexpr std::uint32_t d_802bdd00_child_factories[] = {0x802c3388};
constexpr ActorDependencyEvent d_802bddcc_calls[] = {
    {"GET_ITEM",0x0,56,false},
    {"ARRAY_COUNT",0x0,58,false},
    {"Vec3fDistance",0x8001af30,62,false},
    {"GET_TRANSFORM",0x0,62,false},
    {"randRange",0x80019e14,67,false},
    {"func",0x0,73,true},
};
constexpr ActorDependencyEvent d_802bdf34_calls[] = {
    {"pokemonAdd",0x80363a8c,83,false},
};
constexpr ActorDependencyEvent d_802bdf58_calls[] = {
    {"pokemonsChangeBlock",0x8036406c,87,false},
};
constexpr ActorDependencyEvent d_802bdf7c_calls[] = {
    {"pokemonRemove",0x803641b8,91,false},
};
constexpr ActorDependencyEvent d_802bdfa0_calls[] = {
    {"setHeightMap",0x800e4040,95,false},
    {"setCeilingMap",0x800e40a4,96,false},
    {"createWorld",0x800e2f38,97,false},
    {"setSkyBoxFollowPlayer",0x800e20b4,98,false},
    {"func_80363928_503D38",0x80363928,99,false},
};
constexpr ActorDependencyEvent d_802be038_calls[] = {
    {"func_800A19D8",0x800a19d8,104,false},
    {"PokemonDetector_Disable",0x80359074,105,false},
    {"EnvSound_Cleanup",0x8036650c,106,false},
    {"auStopBGM",0x80022334,107,false},
    {"auStopAllSounds",0x80022b14,108,false},
    {"destroyWorld",0x800e3064,109,false},
    {"func_80356FBC_4F73CC",0x80356fbc,110,false},
    {"removeMainCamera",0x803586c0,111,false},
    {"func_800067DC",0x800067dc,112,false},
};
constexpr ActorDependencyEvent d_802be0ac_calls[] = {
    {"Items_DisplaceAllItems",0x8035cb18,127,false},
    {"func_802BE038_6404E8",0x802be038,132,false},
};
constexpr ActorDependencyEvent d_802be140_calls[] = {
    {"func_800A73C0",0x800a73c0,141,false},
    {"func_800A73C0",0x800a73c0,142,false},
    {"fx_setupBankID",0x800a1ed0,144,false},
    {"func_800A73C0",0x800a73c0,146,false},
    {"func_800A73C0",0x800a73c0,147,false},
    {"fx_setupBankID",0x800a1ed0,149,false},
    {"func_800A2094",0x800a2094,151,false},
    {"getMainCamera",0x803586f8,151,false},
    {"fx_allocEffects",0x800a5e08,152,false},
    {"fx_setDitherModes",0x800a5df4,153,false},
};
constexpr ActorDependencyEvent d_802be22c_calls[] = {
    {"gtlMalloc",0x800054a0,160,false},
    {"viApplyScreenSettings",0x80007bc4,161,false},
    {"ARRAY_COUNT",0x0,163,false},
    {"func_8009CE00",0x8009ce00,165,false},
    {"omAddGObj",0x8000a410,166,false},
    {"Pokemons_Init",0x80363738,167,false},
    {"func_802BDFA0_640450",0x802bdfa0,168,false},
    {"getBackgroundColor",0x800a1758,169,false},
    {"createMainCameras",0x80358420,170,false},
    {"getMainCamera",0x803586f8,171,false},
    {"initUI",0x803563a0,172,false},
    {"setEndLevelCallback",0x80353d50,173,false},
    {"setPauseCallback",0x80353d5c,174,false},
    {"EnvSound_Init",0x80366470,175,false},
    {"func_802BE140_6405F0",0x802be140,176,false},
    {"PokemonDetector_Create",0x80358f14,177,false},
    {"PokemonDetector_Enable",0x80359064,178,false},
    {"func_802C60F4_6485A4",0x802c60f4,179,false},
    {"setPitchLimits",0x80357054,180,false},
};
constexpr std::uint32_t d_802be22c_callbacks[] = {0x802be09c};
constexpr ActorDependencyEvent d_802be3b0_calls[] = {
    {"gtlSetIntervals",0x800075fc,189,false},
    {"gtlDisableNearClipping",0x80005448,190,false},
    {"omSetupScene",0x800073ac,191,false},
    {"func_8009BC68",0x8009bc68,197,false},
    {"func_800AAED0",0x800aaed0,201,false},
};
constexpr ActorDependencyEvent d_802be450_calls[] = {
    {"auSetBGMExtraReverb",0x800226bc,19,false},
    {"auSetSoundGlobalReverb",0x80023880,20,false},
    {"auPlaySoundWithParams",0x80022a58,21,false},
    {"setBackgroundMusic",0x803507e0,27,false},
    {"auSetSoundVolume",0x80022c28,33,false},
    {"auSetSoundVolume",0x80022c28,35,false},
    {"auSetSoundVolume",0x80022c28,41,false},
    {"auStopSound",0x80022fd0,43,false},
    {"auPlaySong",0x80022374,49,false},
    {"auStopSong",0x800223f8,55,false},
};
constexpr ActorDependencyEvent d_802be820_calls[] = {
    {"Pokemon_AddAtGeo",0x8035fe24,38,false},
    {"GET_TRANSFORM",0x0,40,false},
    {"GET_TRANSFORM",0x0,40,false},
    {"omEndProcess",0x80008f2c,42,false},
};
constexpr std::uint32_t d_802be820_child_factories[] = {0x802bf55c};
constexpr ActorDependencyEvent d_802be898_calls[] = {
    {"GET_POKEMON",0x0,47,false},
    {"Pokemon_SetState",0x8035ec58,51,false},
};
constexpr std::uint32_t d_802be898_callbacks[] = {0x802be8d4};
constexpr ActorDependencyEvent d_802be8d4_calls[] = {
    {"GET_POKEMON",0x0,56,false},
    {"Pokemon_SetAnimation",0x8035f138,58,false},
    {"Pokemon_StartPathProc",0x8035edf8,59,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,61,false},
    {"Pokemon_SetStateRandom",0x8035ecac,62,false},
};
constexpr std::uint32_t d_802be8d4_graphs[] = {0x802c65e8};
constexpr std::uint32_t d_802be8d4_weighted_states[] = {0x802c67e8};
constexpr std::uint32_t d_802be8d4_headers[] = {0x802c6538};
constexpr ActorDependencyEvent d_802be944_calls[] = {
    {"GET_POKEMON",0x0,67,false},
    {"Pokemon_ForceAnimation",0x8035f15c,69,false},
    {"Pokemon_StartPathProc",0x8035edf8,70,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,72,false},
    {"Pokemon_SetStateRandom",0x8035ecac,73,false},
};
constexpr std::uint32_t d_802be944_graphs[] = {0x802c65e8};
constexpr std::uint32_t d_802be944_weighted_states[] = {0x802c67e8};
constexpr std::uint32_t d_802be944_headers[] = {0x802c654c};
constexpr ActorDependencyEvent d_802be9b4_calls[] = {
    {"GET_POKEMON",0x0,78,false},
    {"Pokemon_ForceAnimation",0x8035f15c,80,false},
    {"Pokemon_StartPathProc",0x8035edf8,81,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,83,false},
    {"Pokemon_SetStateRandom",0x8035ecac,84,false},
};
constexpr std::uint32_t d_802be9b4_graphs[] = {0x802c65e8};
constexpr std::uint32_t d_802be9b4_weighted_states[] = {0x802c67e8};
constexpr std::uint32_t d_802be9b4_headers[] = {0x802c6560};
constexpr ActorDependencyEvent d_802bea24_calls[] = {
    {"GET_POKEMON",0x0,89,false},
    {"Pokemon_SetAnimation",0x8035f138,91,false},
    {"Pokemon_StartPathProc",0x8035edf8,93,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,95,false},
    {"Pokemon_SetState",0x8035ec58,97,false},
    {"Pokemon_SetStateRandom",0x8035ecac,99,false},
};
constexpr std::uint32_t d_802bea24_callbacks[] = {0x802be8d4,0x802beac8};
constexpr std::uint32_t d_802bea24_graphs[] = {0x802c65e8};
constexpr std::uint32_t d_802bea24_weighted_states[] = {0x802c67e8};
constexpr std::uint32_t d_802bea24_headers[] = {0x802c6574};
constexpr ActorDependencyEvent d_802beac8_calls[] = {
    {"GET_POKEMON",0x0,104,false},
    {"Pokemon_RunInCircles",0x80361110,107,false},
    {"omEndProcess",0x80008f2c,110,false},
};
constexpr ActorDependencyEvent d_802beb24_calls[] = {
    {"GET_POKEMON",0x0,115,false},
    {"omCreateProcess",0x80008c28,118,false},
    {"Pokemon_StartPathProc",0x8035edf8,121,false},
    {"Pokemon_SetAnimation",0x8035f138,122,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,124,false},
    {"Pokemon_RunCleanup",0x8035fd70,125,false},
    {"Pokemon_SetState",0x8035ec58,126,false},
};
constexpr std::uint32_t d_802beb24_callbacks[] = {0x802be820,0x802be8d4};
constexpr std::uint32_t d_802beb24_headers[] = {0x802c65c4};
constexpr ActorDependencyEvent d_802bebbc_calls[] = {
    {"GET_POKEMON",0x0,131,false},
    {"Pokemon_StartPathProc",0x8035edf8,133,false},
    {"Pokemon_ForceAnimation",0x8035f15c,134,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,136,false},
    {"Pokemon_SetState",0x8035ec58,138,false},
};
constexpr std::uint32_t d_802bebbc_callbacks[] = {0x802be8d4};
constexpr std::uint32_t d_802bebbc_graphs[] = {0x802c66f8};
constexpr std::uint32_t d_802bebbc_headers[] = {0x802c659c};
constexpr ActorDependencyEvent d_802bec34_calls[] = {
    {"GET_POKEMON",0x0,143,false},
    {"Pokemon_SetAnimation",0x8035f138,145,false},
    {"Pokemon_StartPathProc",0x8035edf8,146,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,148,false},
    {"Pokemon_SetState",0x8035ec58,149,false},
};
constexpr std::uint32_t d_802bec34_callbacks[] = {0x802be8d4,0x802beca8};
constexpr std::uint32_t d_802bec34_graphs[] = {0x802c67a8};
constexpr std::uint32_t d_802bec34_headers[] = {0x802c6588};
constexpr ActorDependencyEvent d_802beca8_calls[] = {
    {"GET_POKEMON",0x0,154,false},
    {"Pokemon_RunAwayFromTarget",0x8036194c,157,false},
    {"omEndProcess",0x80008f2c,160,false},
};
constexpr ActorDependencyEvent d_802bed04_calls[] = {
    {"GET_POKEMON",0x0,165,false},
    {"Pokemon_StartPathProc",0x8035edf8,167,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,169,false},
    {"Pokemon_SetState",0x8035ec58,171,false},
    {"Pokemon_SetState",0x8035ec58,173,false},
};
constexpr std::uint32_t d_802bed04_callbacks[] = {0x802be8d4,0x802bed8c,0x802bee10};
constexpr std::uint32_t d_802bed04_graphs[] = {0x802c6678};
constexpr ActorDependencyEvent d_802bed8c_calls[] = {
    {"GET_POKEMON",0x0,177,false},
    {"Pokemon_SetAnimation",0x8035f138,179,false},
    {"Pokemon_RunToTarget",0x80361748,181,false},
    {"Pokemon_TurnToTarget",0x8036148c,182,false},
    {"omEndProcess",0x80008f2c,185,false},
};
constexpr std::uint32_t d_802bed8c_headers[] = {0x802c6588};
constexpr ActorDependencyEvent d_802bee10_calls[] = {
    {"GET_POKEMON",0x0,190,false},
    {"Pokemon_SetAnimation",0x8035f138,192,false},
    {"Pokemon_StartPathProc",0x8035edf8,193,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,195,false},
    {"Pokemon_SetState",0x8035ec58,197,false},
    {"Pokemon_SetStateRandom",0x8035ecac,199,false},
};
constexpr std::uint32_t d_802bee10_callbacks[] = {0x802beea4};
constexpr std::uint32_t d_802bee10_graphs[] = {0x802c6728};
constexpr std::uint32_t d_802bee10_weighted_states[] = {0x802c67e8};
constexpr std::uint32_t d_802bee10_headers[] = {0x802c65b0};
constexpr ActorDependencyEvent d_802beea4_calls[] = {
    {"Pokemon_TurnToTarget",0x8036148c,204,false},
    {"ohWait",0x8000bca8,205,false},
};
constexpr ActorDependencyEvent d_802beef4_calls[] = {
    {"Pokemon_SetState",0x8035ec58,210,false},
};
constexpr std::uint32_t d_802beef4_callbacks[] = {0x802bed04};
constexpr ActorDependencyEvent d_802bef18_calls[] = {
    {"GET_POKEMON",0x0,215,false},
    {"Pokemon_SetAnimation",0x8035f138,217,false},
    {"Pokemon_StartPathProc",0x8035edf8,218,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,220,false},
    {"Pokemon_SetState",0x8035ec58,221,false},
};
constexpr std::uint32_t d_802bef18_callbacks[] = {0x802be8d4,0x802bef8c};
constexpr std::uint32_t d_802bef18_graphs[] = {0x802c6778};
constexpr std::uint32_t d_802bef18_headers[] = {0x802c6574};
constexpr ActorDependencyEvent d_802bef8c_calls[] = {
    {"GET_POKEMON",0x0,226,false},
    {"Pokemon_TurnToTarget",0x8036148c,228,false},
    {"omEndProcess",0x80008f2c,231,false},
};
constexpr ActorDependencyEvent d_802befd8_calls[] = {
    {"Pokemon_SpawnOnGround",0x80362ee0,235,false},
};
constexpr ActorDependencyEvent d_802bf020_calls[] = {
    {"Pokemon_SetState",0x8035ec58,28,false},
};
constexpr std::uint32_t d_802bf020_callbacks[] = {0x802bf044};
constexpr ActorDependencyEvent d_802bf044_calls[] = {
    {"GET_POKEMON",0x0,33,false},
    {"Pokemon_SetAnimation",0x8035f138,36,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,38,false},
    {"Pokemon_SetState",0x8035ec58,40,false},
};
constexpr std::uint32_t d_802bf044_callbacks[] = {0x802bf0b4};
constexpr std::uint32_t d_802bf044_headers[] = {0x802c6910};
constexpr ActorDependencyEvent d_802bf0b4_calls[] = {
    {"GET_POKEMON",0x0,45,false},
    {"Pokemon_StartPathProc",0x8035edf8,47,false},
    {"Pokemon_SetAnimation",0x8035f138,48,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,50,false},
    {"Pokemon_SetStateRandom",0x8035ecac,51,false},
};
constexpr std::uint32_t d_802bf0b4_graphs[] = {0x802c6924};
constexpr std::uint32_t d_802bf0b4_weighted_states[] = {0x802c6964};
constexpr std::uint32_t d_802bf0b4_headers[] = {0x802c6870};
constexpr ActorDependencyEvent d_802bf124_calls[] = {
    {"GET_POKEMON",0x0,56,false},
    {"Pokemon_StartPathProc",0x8035edf8,58,false},
    {"Pokemon_SetAnimation",0x8035f138,59,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,62,false},
    {"Pokemon_SetState",0x8035ec58,64,false},
    {"Pokemon_SetStateRandom",0x8035ecac,66,false},
};
constexpr std::uint32_t d_802bf124_callbacks[] = {0x802bf0b4,0x802bf1c4};
constexpr std::uint32_t d_802bf124_graphs[] = {0x802c6924};
constexpr std::uint32_t d_802bf124_weighted_states[] = {0x802c6964};
constexpr std::uint32_t d_802bf124_headers[] = {0x802c6884};
constexpr ActorDependencyEvent d_802bf1c4_calls[] = {
    {"GET_POKEMON",0x0,71,false},
    {"Pokemon_RunInCircles",0x80361110,74,false},
    {"omEndProcess",0x80008f2c,77,false},
};
constexpr ActorDependencyEvent d_802bf220_calls[] = {
    {"GET_POKEMON",0x0,82,false},
    {"Pokemon_StartPathProc",0x8035edf8,84,false},
    {"Pokemon_SetAnimation",0x8035f138,85,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,87,false},
    {"Pokemon_SetAnimation",0x8035f138,88,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,90,false},
    {"Pokemon_SetAnimation",0x8035f138,91,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,93,false},
    {"Pokemon_SetState",0x8035ec58,94,false},
};
constexpr std::uint32_t d_802bf220_callbacks[] = {0x802bf0b4};
constexpr std::uint32_t d_802bf220_headers[] = {0x802c68c0,0x802c68d4,0x802c68e8};
constexpr ActorDependencyEvent d_802bf2d0_calls[] = {
    {"GET_POKEMON",0x0,99,false},
    {"Pokemon_StartPathProc",0x8035edf8,101,false},
    {"Pokemon_SetAnimation",0x8035f138,102,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,104,false},
    {"Pokemon_SetState",0x8035ec58,105,false},
};
constexpr std::uint32_t d_802bf2d0_callbacks[] = {0x802bf0b4};
constexpr std::uint32_t d_802bf2d0_headers[] = {0x802c68ac};
constexpr ActorDependencyEvent d_802bf338_calls[] = {
    {"GET_POKEMON",0x0,110,false},
    {"Pokemon_SetAnimation",0x8035f138,112,false},
    {"Pokemon_StartPathProc",0x8035edf8,113,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,115,false},
    {"Pokemon_SetState",0x8035ec58,117,false},
    {"Pokemon_SetState",0x8035ec58,119,false},
};
constexpr std::uint32_t d_802bf338_callbacks[] = {0x802bf0b4,0x802bf3d0,0x802bf454};
constexpr std::uint32_t d_802bf338_graphs[] = {0x802c6924};
constexpr std::uint32_t d_802bf338_headers[] = {0x802c6884};
constexpr ActorDependencyEvent d_802bf3d0_calls[] = {
    {"GET_POKEMON",0x0,124,false},
    {"Pokemon_TurnToTarget",0x8036148c,126,false},
    {"Pokemon_SetAnimation",0x8035f138,127,false},
    {"Pokemon_RunToTarget",0x80361748,129,false},
    {"omEndProcess",0x80008f2c,132,false},
};
constexpr std::uint32_t d_802bf3d0_headers[] = {0x802c6898};
constexpr ActorDependencyEvent d_802bf454_calls[] = {
    {"GET_POKEMON",0x0,137,false},
    {"Pokemon_SetAnimation",0x8035f138,139,false},
    {"Pokemon_StartPathProc",0x8035edf8,140,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,142,false},
    {"Pokemon_SetState",0x8035ec58,144,false},
    {"Pokemon_SetStateRandom",0x8035ecac,146,false},
};
constexpr std::uint32_t d_802bf454_callbacks[] = {0x802bf4e8};
constexpr std::uint32_t d_802bf454_graphs[] = {0x802c697c};
constexpr std::uint32_t d_802bf454_weighted_states[] = {0x802c6964};
constexpr std::uint32_t d_802bf454_headers[] = {0x802c68fc};
constexpr ActorDependencyEvent d_802bf4e8_calls[] = {
    {"Pokemon_TurnToTarget",0x8036148c,151,false},
    {"ohWait",0x8000bca8,152,false},
};
constexpr ActorDependencyEvent d_802bf55c_calls[] = {
    {"Pokemon_SpawnOnGround",0x80362ee0,160,false},
};
constexpr ActorDependencyEvent d_802bf5a0_calls[] = {
    {"GET_POKEMON",0x0,31,false},
    {"HIDE_POKEMON",0x0,35,false},
    {"getLevelProgress",0x80357384,38,false},
    {"ohWait",0x8000bca8,42,false},
    {"SHOW_POKEMON",0x0,45,false},
    {"Pokemon_SetState",0x8035ec58,51,false},
};
constexpr std::uint32_t d_802bf5a0_callbacks[] = {0x802bf68c};
constexpr ActorDependencyEvent d_802bf68c_calls[] = {
    {"GET_POKEMON",0x0,55,false},
    {"Pokemon_ResetPathPos",0x80362050,58,false},
    {"Pokemon_SetAnimation",0x8035f138,59,false},
    {"Pokemon_StartPathProc",0x8035edf8,60,false},
    {"randRange",0x80019e14,61,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,63,false},
    {"Pokemon_SetState",0x8035ec58,65,false},
    {"Pokemon_SetStateRandom",0x8035ecac,67,false},
};
constexpr std::uint32_t d_802bf68c_callbacks[] = {0x802bf750};
constexpr std::uint32_t d_802bf68c_graphs[] = {0x802c6ad8};
constexpr std::uint32_t d_802bf68c_weighted_states[] = {0x802c6b28};
constexpr std::uint32_t d_802bf68c_headers[] = {0x802c6a40};
constexpr ActorDependencyEvent d_802bf750_calls[] = {
    {"GET_POKEMON",0x0,76,false},
    {"Pokemon_FollowPath",0x803620c8,78,false},
    {"omEndProcess",0x80008f2c,82,false},
};
constexpr ActorDependencyEvent d_802bf7b4_calls[] = {
    {"GET_POKEMON",0x0,87,false},
    {"Pokemon_ForceAnimation",0x8035f15c,89,false},
    {"Pokemon_StartPathProc",0x8035edf8,90,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,92,false},
    {"Pokemon_SetState",0x8035ec58,93,false},
};
constexpr std::uint32_t d_802bf7b4_callbacks[] = {0x802bf68c};
constexpr std::uint32_t d_802bf7b4_graphs[] = {0x802c6ad8};
constexpr std::uint32_t d_802bf7b4_headers[] = {0x802c6a2c};
constexpr ActorDependencyEvent d_802bf824_calls[] = {
    {"GET_POKEMON",0x0,98,false},
    {"Pokemon_SetAnimation",0x8035f138,100,false},
    {"Pokemon_StartPathProc",0x8035edf8,101,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,103,false},
    {"Pokemon_SetState",0x8035ec58,104,false},
};
constexpr std::uint32_t d_802bf824_callbacks[] = {0x802bf68c};
constexpr std::uint32_t d_802bf824_graphs[] = {0x802c6ad8};
constexpr std::uint32_t d_802bf824_headers[] = {0x802c6a18};
constexpr ActorDependencyEvent d_802bf894_calls[] = {
    {"GET_POKEMON",0x0,109,false},
    {"Pokemon_SetAnimation",0x8035f138,112,false},
    {"Pokemon_StartPathProc",0x8035edf8,113,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,115,false},
    {"Pokemon_SetAnimation",0x8035f138,116,false},
    {"Pokemon_StartPathProc",0x8035edf8,117,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,119,false},
    {"Pokemon_SetAnimation",0x8035f138,120,false},
    {"cmdSendCommandToLink",0x8000b830,122,false},
    {"Pokemon_SetState",0x8035ec58,123,false},
    {"Pokemon_StartPathProc",0x8035edf8,125,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,127,false},
    {"Pokemon_SetAnimation",0x8035f138,128,false},
    {"Pokemon_StartPathProc",0x8035edf8,129,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,131,false},
    {"Pokemon_SetState",0x8035ec58,132,false},
};
constexpr std::uint32_t d_802bf894_callbacks[] = {0x802bf9b4,0x802bfa8c,0x802bfb9c};
constexpr std::uint32_t d_802bf894_graphs[] = {0x802c6aa8};
constexpr std::uint32_t d_802bf894_headers[] = {0x802c6a54,0x802c6a68,0x802c6a7c,0x802c6a90};
constexpr ActorDependencyEvent d_802bf9b4_calls[] = {
    {"GET_POKEMON",0x0,137,false},
    {"atan2f",0x80019abc,143,false},
    {"Pokemon_FallDownOnGround",0x80360ab8,144,false},
    {"omEndProcess",0x80008f2c,147,false},
};
constexpr ActorDependencyEvent d_802bfa3c_calls[] = {
    {"GET_POKEMON",0x0,152,false},
    {"GET_TRANSFORM",0x0,154,false},
    {"Pokemon_SetState",0x8035ec58,160,false},
};
constexpr std::uint32_t d_802bfa3c_callbacks[] = {0x802bf894};
constexpr ActorDependencyEvent d_802bfa8c_calls[] = {
    {"GET_POKEMON",0x0,165,false},
    {"Pokemon_SetAnimation",0x8035f138,167,false},
    {"Pokemon_StartPathProc",0x8035edf8,168,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,170,false},
    {"Pokemon_SetState",0x8035ec58,172,false},
    {"Pokemon_SetState",0x8035ec58,174,false},
};
constexpr std::uint32_t d_802bfa8c_callbacks[] = {0x802bf7b4,0x802bfb24};
constexpr std::uint32_t d_802bfa8c_graphs[] = {0x802c6aa8};
constexpr std::uint32_t d_802bfa8c_headers[] = {0x802c6a40};
constexpr ActorDependencyEvent d_802bfb24_calls[] = {
    {"GET_POKEMON",0x0,179,false},
    {"Pokemon_SetTargetPos",0x80361b50,182,false},
    {"Pokemon_RunToTargetPos",0x80361e58,185,false},
    {"omEndProcess",0x80008f2c,188,false},
};
constexpr ActorDependencyEvent d_802bfb9c_calls[] = {
    {"GET_POKEMON",0x0,193,false},
    {"Pokemon_StartPathProc",0x8035edf8,195,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,197,false},
    {"Pokemon_SetState",0x8035ec58,198,false},
};
constexpr std::uint32_t d_802bfb9c_graphs[] = {0x802c6b08};
constexpr ActorDependencyEvent d_802bfbf0_calls[] = {
    {"GET_POKEMON",0x0,203,false},
    {"Pokemon_StartPathProc",0x8035edf8,206,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,208,false},
    {"cmdSendCommand",0x8000b774,209,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,212,false},
    {"Pokemon_RunCleanup",0x8035fd70,213,false},
    {"Pokemon_SetState",0x8035ec58,214,false},
};
constexpr std::uint32_t d_802bfbf0_callbacks[] = {0x802bfc84};
constexpr ActorDependencyEvent d_802bfc84_calls[] = {
    {"GET_TRANSFORM",0x0,220,false},
    {"ohWait",0x8000bca8,229,false},
    {"omEndProcess",0x80008f2c,234,false},
};
constexpr ActorDependencyEvent d_802bfda0_calls[] = {
    {"Pokemon_SpawnOnGround",0x80362ee0,238,false},
};
constexpr ActorDependencyEvent d_802bfde0_calls[] = {
    {"GET_POKEMON",0x0,20,false},
    {"HIDE_POKEMON",0x0,22,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,24,false},
    {"Pokemon_SetState",0x8035ec58,25,false},
};
constexpr std::uint32_t d_802bfde0_graphs[] = {0x802c6c10};
constexpr ActorDependencyEvent d_802bfe34_calls[] = {
    {"GET_POKEMON",0x0,29,false},
    {"Pokemon_GetDistance",0x8035f2a4,31,false},
    {"Pokemon_SetState",0x8035ec58,32,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,36,false},
    {"cmdSendCommand",0x8000b774,37,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,39,false},
    {"Pokemon_SetState",0x8035ec58,40,false},
};
constexpr std::uint32_t d_802bfe34_callbacks[] = {0x802bfde0,0x802bfef0};
constexpr std::uint32_t d_802bfe34_graphs[] = {0x802c6c30};
constexpr ActorDependencyEvent d_802bfef0_calls[] = {
    {"GET_POKEMON",0x0,45,false},
    {"Pokemon_SetAnimation",0x8035f138,48,false},
    {"Pokemon_StartPathProc",0x8035edf8,49,false},
    {"SHOW_POKEMON",0x0,50,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,52,false},
    {"Pokemon_SetState",0x8035ec58,53,false},
};
constexpr std::uint32_t d_802bfef0_callbacks[] = {0x802bff74,0x802c0058};
constexpr std::uint32_t d_802bfef0_headers[] = {0x802c6bb8};
constexpr ActorDependencyEvent d_802bff74_calls[] = {
    {"GET_TRANSFORM",0x0,59,false},
    {"ohWait",0x8000bca8,60,false},
    {"GET_TRANSFORM",0x0,63,false},
    {"func_8035E1D4_4FE5E4",0x8035e1d4,63,false},
    {"Pokemon_FallDownOnGround",0x80360ab8,70,false},
    {"omEndProcess",0x80008f2c,74,false},
};
constexpr ActorDependencyEvent d_802c0058_calls[] = {
    {"GET_POKEMON",0x0,79,false},
    {"Pokemon_SetAnimation",0x8035f138,81,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,83,false},
    {"Pokemon_SetState",0x8035ec58,84,false},
};
constexpr std::uint32_t d_802c0058_graphs[] = {0x802c6be0};
constexpr std::uint32_t d_802c0058_headers[] = {0x802c6ba4};
constexpr ActorDependencyEvent d_802c00b0_calls[] = {
    {"GET_POKEMON",0x0,89,false},
    {"Pokemon_SetAnimation",0x8035f138,91,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,93,false},
    {"Pokemon_SetState",0x8035ec58,94,false},
};
constexpr std::uint32_t d_802c00b0_callbacks[] = {0x802c0058};
constexpr std::uint32_t d_802c00b0_headers[] = {0x802c6bcc};
constexpr ActorDependencyEvent d_802c0104_calls[] = {
    {"Pokemon_SpawnOnGround",0x80362ee0,98,false},
};
constexpr ActorDependencyEvent d_802c0140_calls[] = {
    {"GET_POKEMON",0x0,32,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,39,false},
    {"Pokemon_HearsPokeFlute",0x80361440,42,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,46,false},
};
constexpr std::uint32_t d_802c0140_graphs[] = {0x802c6d60};
constexpr ActorDependencyEvent d_802c0220_calls[] = {
    {"GET_POKEMON",0x0,54,false},
    {"HIDE_POKEMON",0x0,56,false},
    {"Pokemon_SetState",0x8035ec58,59,false},
    {"Pokemon_SetState",0x8035ec58,61,false},
};
constexpr std::uint32_t d_802c0220_callbacks[] = {0x802c027c,0x802c02bc};
constexpr ActorDependencyEvent d_802c027c_calls[] = {
    {"GET_POKEMON",0x0,65,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,68,false},
    {"Pokemon_SetState",0x8035ec58,69,false},
};
constexpr std::uint32_t d_802c027c_graphs[] = {0x802c6d90};
constexpr ActorDependencyEvent d_802c02bc_calls[] = {
    {"GET_POKEMON",0x0,73,false},
    {"ohWait",0x8000bca8,76,false},
    {"Pokemon_SetState",0x8035ec58,78,false},
};
constexpr std::uint32_t d_802c02bc_callbacks[] = {0x802c0330};
constexpr ActorDependencyEvent d_802c0330_calls[] = {
    {"GET_POKEMON",0x0,82,false},
    {"ohWait",0x8000bca8,86,false},
    {"SHOW_POKEMON",0x0,88,false},
    {"Pokemon_StartPathProc",0x8035edf8,89,false},
    {"Pokemon_SetState",0x8035ec58,90,false},
};
constexpr std::uint32_t d_802c0330_callbacks[] = {0x802c03e4,0x802c04a4};
constexpr ActorDependencyEvent d_802c03e4_calls[] = {
    {"GET_POKEMON",0x0,95,false},
    {"Pokemon_SetAnimation",0x8035f138,99,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,101,false},
    {"Pokemon_SetState",0x8035ec58,103,false},
    {"Pokemon_RunCleanup",0x8035fd70,105,false},
    {"Pokemon_SetState",0x8035ec58,106,false},
};
constexpr std::uint32_t d_802c03e4_graphs[] = {0x802c6db0};
constexpr std::uint32_t d_802c03e4_headers[] = {0x802c6cd4};
constexpr ActorDependencyEvent d_802c04a4_calls[] = {
    {"GET_POKEMON",0x0,111,false},
    {"Pokemon_ResetPathPos",0x80362050,113,false},
    {"Pokemon_FollowPath",0x803620c8,114,false},
    {"omEndProcess",0x80008f2c,117,false},
};
constexpr ActorDependencyEvent d_802c0510_calls[] = {
    {"GET_POKEMON",0x0,122,false},
    {"Pokemon_ForceAnimation",0x8035f15c,125,false},
    {"Pokemon_StartPathProc",0x8035edf8,126,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,128,false},
    {"cmdSendCommand",0x8000b774,129,false},
    {"Pokemon_SetAnimation",0x8035f138,133,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,135,false},
    {"Pokemon_RunCleanup",0x8035fd70,136,false},
    {"Pokemon_SetState",0x8035ec58,137,false},
};
constexpr std::uint32_t d_802c0510_callbacks[] = {0x802c05d8};
constexpr std::uint32_t d_802c0510_headers[] = {0x802c6ce8,0x802c6d38};
constexpr ActorDependencyEvent d_802c05d8_calls[] = {
    {"GET_POKEMON",0x0,142,false},
    {"Pokemon_FallDownOnGround",0x80360ab8,149,false},
    {"omEndProcess",0x80008f2c,152,false},
};
constexpr ActorDependencyEvent d_802c0630_calls[] = {
    {"GET_POKEMON",0x0,157,false},
    {"Pokemon_SetAnimation",0x8035f138,160,false},
    {"func_802C0140_6425F0",0x802c0140,161,false},
    {"Pokemon_SetState",0x8035ec58,162,false},
};
constexpr std::uint32_t d_802c0630_callbacks[] = {0x802c03e4};
constexpr std::uint32_t d_802c0630_graphs[] = {0x802c6e10};
constexpr std::uint32_t d_802c0630_headers[] = {0x802c6cfc};
constexpr ActorDependencyEvent d_802c06bc_calls[] = {
    {"GET_POKEMON",0x0,167,false},
    {"Pokemon_SetAnimation",0x8035f138,170,false},
    {"func_802C0140_6425F0",0x802c0140,171,false},
    {"Pokemon_SetState",0x8035ec58,172,false},
};
constexpr std::uint32_t d_802c06bc_callbacks[] = {0x802c03e4};
constexpr std::uint32_t d_802c06bc_graphs[] = {0x802c6e60};
constexpr std::uint32_t d_802c06bc_headers[] = {0x802c6d10};
constexpr ActorDependencyEvent d_802c0748_calls[] = {
    {"GET_POKEMON",0x0,177,false},
    {"Pokemon_SetAnimation",0x8035f138,180,false},
    {"func_802C0140_6425F0",0x802c0140,181,false},
    {"Pokemon_SetState",0x8035ec58,182,false},
};
constexpr std::uint32_t d_802c0748_callbacks[] = {0x802c03e4};
constexpr std::uint32_t d_802c0748_graphs[] = {0x802c6eb0};
constexpr std::uint32_t d_802c0748_headers[] = {0x802c6d24};
constexpr ActorDependencyEvent d_802c07d4_calls[] = {
    {"Pokemon_Spawn",0x80362e5c,186,false},
};
constexpr ActorDependencyEvent d_802c0810_calls[] = {
    {"Pokemon_AddAtGeo",0x8035fe24,40,false},
    {"GET_TRANSFORM",0x0,42,false},
    {"GET_TRANSFORM",0x0,42,false},
    {"omEndProcess",0x80008f2c,43,false},
};
constexpr std::uint32_t d_802c0810_child_factories[] = {0x802c13b4};
constexpr ActorDependencyEvent d_802c0888_calls[] = {
    {"GET_POKEMON",0x0,48,false},
    {"HIDE_POKEMON",0x0,52,false},
    {"Pokemon_SetState",0x8035ec58,55,false},
    {"Pokemon_SetState",0x8035ec58,58,false},
    {"Pokemon_SetState",0x8035ec58,61,false},
    {"Pokemon_SetState",0x8035ec58,64,false},
    {"Pokemon_SetState",0x8035ec58,66,false},
};
constexpr std::uint32_t d_802c0888_callbacks[] = {0x802c0960,0x802c09c4,0x802c0a28,0x802c0a8c,0x802c0af0};
constexpr ActorDependencyEvent d_802c0960_calls[] = {
    {"ohWait",0x8000bca8,71,false},
    {"Pokemon_SetState",0x8035ec58,73,false},
};
constexpr std::uint32_t d_802c0960_callbacks[] = {0x802c0af0};
constexpr ActorDependencyEvent d_802c09c4_calls[] = {
    {"ohWait",0x8000bca8,78,false},
    {"Pokemon_SetState",0x8035ec58,80,false},
};
constexpr std::uint32_t d_802c09c4_callbacks[] = {0x802c0af0};
constexpr ActorDependencyEvent d_802c0a28_calls[] = {
    {"ohWait",0x8000bca8,85,false},
    {"Pokemon_SetState",0x8035ec58,87,false},
};
constexpr std::uint32_t d_802c0a28_callbacks[] = {0x802c0af0};
constexpr ActorDependencyEvent d_802c0a8c_calls[] = {
    {"ohWait",0x8000bca8,92,false},
    {"Pokemon_SetState",0x8035ec58,94,false},
};
constexpr std::uint32_t d_802c0a8c_callbacks[] = {0x802c0af0};
constexpr ActorDependencyEvent d_802c0af0_calls[] = {
    {"GET_POKEMON",0x0,99,false},
    {"SHOW_POKEMON",0x0,101,false},
    {"Pokemon_StartPathProc",0x8035edf8,102,false},
    {"Pokemon_SetAnimation",0x8035f138,103,false},
    {"animSetTextureAnimationSpeed",0x8000c660,104,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,109,false},
    {"Pokemon_SetStateRandom",0x8035ecac,110,false},
};
constexpr std::uint32_t d_802c0af0_graphs[] = {0x802c6fd4};
constexpr std::uint32_t d_802c0af0_weighted_states[] = {0x802c7014};
constexpr std::uint32_t d_802c0af0_headers[] = {0x802c6f60};
constexpr ActorDependencyEvent d_802c0bb0_calls[] = {
    {"GET_POKEMON",0x0,116,false},
    {"Pokemon_StartPathProc",0x8035edf8,120,false},
    {"Pokemon_SetAnimation",0x8035f138,123,false},
    {"animSetTextureAnimationSpeed",0x8000c660,124,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,129,false},
    {"Pokemon_SetState",0x8035ec58,131,false},
    {"Pokemon_SetStateRandom",0x8035ecac,135,false},
};
constexpr std::uint32_t d_802c0bb0_callbacks[] = {0x802c0af0,0x802c0cd8};
constexpr std::uint32_t d_802c0bb0_graphs[] = {0x802c6fd4};
constexpr std::uint32_t d_802c0bb0_weighted_states[] = {0x802c7014};
constexpr std::uint32_t d_802c0bb0_headers[] = {0x802c6f74};
constexpr ActorDependencyEvent d_802c0cd8_calls[] = {
    {"GET_POKEMON",0x0,140,false},
    {"Pokemon_RunInCircles",0x80361110,143,false},
    {"omEndProcess",0x80008f2c,146,false},
};
constexpr ActorDependencyEvent d_802c0d34_calls[] = {
    {"GET_POKEMON",0x0,151,false},
    {"Pokemon_StartPathProc",0x8035edf8,153,false},
    {"Pokemon_SetAnimation",0x8035f138,154,false},
    {"animSetTextureAnimationSpeed",0x8000c660,155,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,160,false},
    {"Pokemon_StartAuxProc",0x8035ed90,163,false},
    {"Pokemon_SetState",0x8035ec58,167,false},
    {"Pokemon_SetState",0x8035ec58,169,false},
};
constexpr std::uint32_t d_802c0d34_callbacks[] = {0x802c0af0,0x802c0e28,0x802c0f30};
constexpr std::uint32_t d_802c0d34_headers[] = {0x802c6f88};
constexpr ActorDependencyEvent d_802c0e28_calls[] = {
    {"GET_POKEMON",0x0,174,false},
    {"ohWait",0x8000bca8,178,false},
    {"Pokemon_StopAuxProc",0x8035edc8,181,false},
};
constexpr ActorDependencyEvent d_802c0e80_calls[] = {
    {"GET_POKEMON",0x0,186,false},
    {"Pokemon_StartPathProc",0x8035edf8,188,false},
    {"Pokemon_SetAnimation",0x8035f138,189,false},
    {"animSetTextureAnimationSpeed",0x8000c660,190,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,195,false},
    {"Pokemon_SetState",0x8035ec58,196,false},
};
constexpr std::uint32_t d_802c0e80_callbacks[] = {0x802c0af0};
constexpr std::uint32_t d_802c0e80_headers[] = {0x802c6f9c};
constexpr ActorDependencyEvent d_802c0f30_calls[] = {
    {"GET_POKEMON",0x0,201,false},
    {"Pokemon_SetAnimation",0x8035f138,203,false},
    {"animSetTextureAnimationSpeed",0x8000c660,204,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,209,false},
    {"omCreateProcess",0x80008c28,210,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,213,false},
    {"Pokemon_RunCleanup",0x8035fd70,214,false},
    {"Pokemon_SetState",0x8035ec58,215,false},
};
constexpr std::uint32_t d_802c0f30_callbacks[] = {0x802c0810};
constexpr std::uint32_t d_802c0f30_headers[] = {0x802c6fb0};
constexpr ActorDependencyEvent d_802c1018_calls[] = {
    {"GET_POKEMON",0x0,219,false},
    {"Pokemon_StopAuxProc",0x8035edc8,235,false},
};
constexpr ActorDependencyEvent d_802c10c8_calls[] = {
    {"Pokemon_SpawnOnGround",0x80362ee0,239,false},
};
constexpr ActorDependencyEvent d_802c1100_calls[] = {
    {"Pokemon_SetState",0x8035ec58,243,false},
};
constexpr std::uint32_t d_802c1100_callbacks[] = {0x802c1178};
constexpr ActorDependencyEvent d_802c1124_calls[] = {
    {"GET_POKEMON",0x0,248,false},
    {"Pokemon_SetAnimation",0x8035f138,250,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,252,false},
    {"Pokemon_SetState",0x8035ec58,253,false},
};
constexpr std::uint32_t d_802c1124_callbacks[] = {0x802c1178};
constexpr std::uint32_t d_802c1124_headers[] = {0x802c70dc};
constexpr ActorDependencyEvent d_802c1178_calls[] = {
    {"GET_POKEMON",0x0,258,false},
    {"Pokemon_StartPathProc",0x8035edf8,260,false},
    {"Pokemon_SetAnimation",0x8035f138,261,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,263,false},
    {"Pokemon_SetStateRandom",0x8035ecac,264,false},
};
constexpr std::uint32_t d_802c1178_graphs[] = {0x802c70f0};
constexpr std::uint32_t d_802c1178_weighted_states[] = {0x802c7120};
constexpr std::uint32_t d_802c1178_headers[] = {0x802c708c};
constexpr ActorDependencyEvent d_802c11e8_calls[] = {
    {"GET_POKEMON",0x0,269,false},
    {"Pokemon_StartPathProc",0x8035edf8,271,false},
    {"Pokemon_SetAnimation",0x8035f138,272,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,275,false},
    {"Pokemon_SetState",0x8035ec58,277,false},
    {"Pokemon_SetStateRandom",0x8035ecac,279,false},
};
constexpr std::uint32_t d_802c11e8_callbacks[] = {0x802c1178,0x802c1288};
constexpr std::uint32_t d_802c11e8_graphs[] = {0x802c70f0};
constexpr std::uint32_t d_802c11e8_weighted_states[] = {0x802c7120};
constexpr std::uint32_t d_802c11e8_headers[] = {0x802c70a0};
constexpr ActorDependencyEvent d_802c1288_calls[] = {
    {"GET_POKEMON",0x0,284,false},
    {"Pokemon_RunInCircles",0x80361110,287,false},
    {"omEndProcess",0x80008f2c,290,false},
};
constexpr ActorDependencyEvent d_802c12e4_calls[] = {
    {"GET_POKEMON",0x0,295,false},
    {"Pokemon_StartPathProc",0x8035edf8,297,false},
    {"Pokemon_SetAnimation",0x8035f138,298,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,300,false},
    {"Pokemon_SetState",0x8035ec58,301,false},
};
constexpr std::uint32_t d_802c12e4_callbacks[] = {0x802c1178};
constexpr std::uint32_t d_802c12e4_headers[] = {0x802c70b4};
constexpr ActorDependencyEvent d_802c134c_calls[] = {
    {"GET_POKEMON",0x0,306,false},
    {"Pokemon_StartPathProc",0x8035edf8,308,false},
    {"Pokemon_SetAnimation",0x8035f138,309,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,311,false},
    {"Pokemon_SetState",0x8035ec58,312,false},
};
constexpr std::uint32_t d_802c134c_callbacks[] = {0x802c1178};
constexpr std::uint32_t d_802c134c_headers[] = {0x802c70c8};
constexpr ActorDependencyEvent d_802c13b4_calls[] = {
    {"Pokemon_SpawnOnGround",0x80362ee0,316,false},
};
constexpr ActorDependencyEvent d_802c13ec_calls[] = {
    {"Pokemon_SpawnOnGround",0x80362ee0,320,false},
};
constexpr ActorDependencyEvent d_802c1440_calls[] = {
    {"GET_POKEMON",0x0,62,false},
    {"Pokemon_HearsPokeFlute",0x80361440,65,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,71,false},
};
constexpr ActorDependencyEvent d_802c14c4_calls[] = {
    {"GET_POKEMON",0x0,76,false},
    {"HIDE_POKEMON",0x0,79,false},
    {"Pokemon_SetState",0x8035ec58,83,false},
    {"Pokemon_SetState",0x8035ec58,86,false},
    {"Pokemon_SetState",0x8035ec58,89,false},
    {"Pokemon_SetState",0x8035ec58,92,false},
};
constexpr std::uint32_t d_802c14c4_callbacks[] = {0x802c157c,0x802c1e4c,0x802c2650,0x802c26ac};
constexpr ActorDependencyEvent d_802c157c_calls[] = {
    {"GET_POKEMON",0x0,97,false},
    {"ohWait",0x8000bca8,100,false},
    {"GET_POKEMON",0x0,103,false},
    {"SHOW_POKEMON",0x0,104,false},
    {"Pokemon_SetAnimation",0x8035f138,105,false},
    {"Pokemon_StartPathProc",0x8035edf8,106,false},
    {"ohWait",0x8000bca8,107,false},
    {"Pokemon_SetState",0x8035ec58,109,false},
};
constexpr std::uint32_t d_802c157c_callbacks[] = {0x802c1660,0x802c1728};
constexpr std::uint32_t d_802c157c_headers[] = {0x802c71fc};
constexpr ActorDependencyEvent d_802c1660_calls[] = {
    {"GET_POKEMON",0x0,114,false},
    {"Pokemon_SetAnimation",0x8035f138,117,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,119,false},
    {"Pokemon_SetState",0x8035ec58,121,false},
    {"Pokemon_RunCleanup",0x8035fd70,123,false},
    {"Pokemon_SetState",0x8035ec58,124,false},
};
constexpr std::uint32_t d_802c1660_graphs[] = {0x802c72b0};
constexpr std::uint32_t d_802c1660_headers[] = {0x802c71fc};
constexpr ActorDependencyEvent d_802c1728_calls[] = {
    {"GET_POKEMON",0x0,129,false},
    {"Pokemon_ResetPathPos",0x80362050,131,false},
    {"Pokemon_FollowPath",0x803620c8,132,false},
    {"omEndProcess",0x80008f2c,135,false},
};
constexpr ActorDependencyEvent d_802c1794_calls[] = {
    {"GET_POKEMON",0x0,140,false},
    {"Pokemon_SetAnimation",0x8035f138,143,false},
    {"func_802C1440_6438F0",0x802c1440,144,false},
    {"Pokemon_SetState",0x8035ec58,145,false},
};
constexpr std::uint32_t d_802c1794_callbacks[] = {0x802c1660};
constexpr std::uint32_t d_802c1794_graphs[] = {0x802c7320};
constexpr std::uint32_t d_802c1794_headers[] = {0x802c7210};
constexpr ActorDependencyEvent d_802c1818_calls[] = {
    {"GET_POKEMON",0x0,150,false},
    {"Pokemon_SetAnimation",0x8035f138,153,false},
    {"func_802C1440_6438F0",0x802c1440,154,false},
    {"Pokemon_SetState",0x8035ec58,155,false},
};
constexpr std::uint32_t d_802c1818_callbacks[] = {0x802c1660};
constexpr std::uint32_t d_802c1818_graphs[] = {0x802c7360};
constexpr std::uint32_t d_802c1818_headers[] = {0x802c7224};
constexpr ActorDependencyEvent d_802c189c_calls[] = {
    {"GET_POKEMON",0x0,160,false},
    {"Pokemon_SetAnimation",0x8035f138,163,false},
    {"func_802C1440_6438F0",0x802c1440,164,false},
    {"Pokemon_SetState",0x8035ec58,165,false},
};
constexpr std::uint32_t d_802c189c_callbacks[] = {0x802c1660};
constexpr std::uint32_t d_802c189c_graphs[] = {0x802c73a0};
constexpr std::uint32_t d_802c189c_headers[] = {0x802c7238};
constexpr ActorDependencyEvent d_802c1920_calls[] = {
    {"GET_POKEMON",0x0,170,false},
    {"Pokemon_ForceAnimation",0x8035f15c,172,false},
    {"Pokemon_Fall",0x80360590,175,false},
    {"Pokemon_Jump",0x80360300,177,false},
    {"Pokemon_StopAuxProc",0x8035edc8,178,false},
};
constexpr std::uint32_t d_802c1920_headers[] = {0x802c724c};
constexpr ActorDependencyEvent d_802c19bc_calls[] = {
    {"GET_POKEMON",0x0,183,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,188,false},
    {"Pokemon_StartPathProc",0x8035edf8,189,false},
    {"Pokemon_SetState",0x8035ec58,190,false},
};
constexpr std::uint32_t d_802c19bc_callbacks[] = {0x802c1a20,0x802c1a7c};
constexpr ActorDependencyEvent d_802c1a20_calls[] = {
    {"GET_POKEMON",0x0,195,false},
    {"Pokemon_ForceAnimation",0x8035f15c,197,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,199,false},
    {"Pokemon_SetState",0x8035ec58,200,false},
};
constexpr std::uint32_t d_802c1a20_callbacks[] = {0x802c1b1c};
constexpr std::uint32_t d_802c1a20_graphs[] = {0x802c73e0};
constexpr std::uint32_t d_802c1a20_headers[] = {0x802c7288};
constexpr ActorDependencyEvent d_802c1a7c_calls[] = {
    {"GET_POKEMON",0x0,205,false},
    {"Pokemon_TurnToTarget",0x8036148c,207,false},
    {"omEndProcess",0x80008f2c,210,false},
};
constexpr ActorDependencyEvent d_802c1ac8_calls[] = {
    {"GET_POKEMON",0x0,215,false},
    {"Pokemon_SetAnimation",0x8035f138,217,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,219,false},
    {"Pokemon_SetState",0x8035ec58,220,false},
};
constexpr std::uint32_t d_802c1ac8_callbacks[] = {0x802c1a20};
constexpr std::uint32_t d_802c1ac8_headers[] = {0x802c7274};
constexpr ActorDependencyEvent d_802c1b1c_calls[] = {
    {"Pokemon_StartPathProc",0x8035edf8,224,false},
    {"Pokemon_SetState",0x8035ec58,225,false},
};
constexpr std::uint32_t d_802c1b1c_callbacks[] = {0x802c1b54,0x802c1c48};
constexpr ActorDependencyEvent d_802c1b54_calls[] = {
    {"GET_POKEMON",0x0,230,false},
    {"Pokemon_ForceAnimation",0x8035f15c,233,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,235,false},
    {"Pokemon_SetState",0x8035ec58,237,false},
    {"Pokemon_RunCleanup",0x8035fd70,252,false},
    {"Pokemon_SetState",0x8035ec58,253,false},
};
constexpr std::uint32_t d_802c1b54_headers[] = {0x802c7288};
constexpr ActorDependencyEvent d_802c1c48_calls[] = {
    {"GET_POKEMON",0x0,257,false},
    {"GET_TRANSFORM",0x0,268,false},
    {"GetInterpolatedPosition",0x8001fca4,272,false},
    {"getCurrentWorldBlock",0x800e2184,273,false},
    {"Pokemon_SetTargetPos",0x80361b50,279,false},
    {"Pokemon_StepToTargetPos",0x80361b68,281,false},
    {"ABS",0x0,282,false},
    {"ohWait",0x8000bca8,289,false},
    {"Pokemon_ResetPathPos",0x80362050,296,false},
    {"Pokemon_FollowPath",0x803620c8,297,false},
    {"omEndProcess",0x80008f2c,300,false},
};
constexpr ActorDependencyEvent d_802c1e4c_calls[] = {
    {"ohWait",0x8000bca8,305,false},
    {"Pokemon_SetState",0x8035ec58,307,false},
};
constexpr std::uint32_t d_802c1e4c_callbacks[] = {0x802c1eb0};
constexpr ActorDependencyEvent d_802c1eb0_calls[] = {
    {"GET_POKEMON",0x0,311,false},
    {"SHOW_POKEMON",0x0,313,false},
    {"Pokemon_StartAuxProc",0x8035ed90,314,false},
    {"Pokemon_StartPathProc",0x8035edf8,315,false},
    {"Pokemon_SetState",0x8035ec58,316,false},
};
constexpr std::uint32_t d_802c1eb0_callbacks[] = {0x802c1f08,0x802c1f74,0x802c2058};
constexpr ActorDependencyEvent d_802c1f08_calls[] = {
    {"GET_POKEMON",0x0,321,false},
    {"Pokemon_ResetPathPos",0x80362050,323,false},
    {"Pokemon_FollowPath",0x803620c8,324,false},
    {"omEndProcess",0x80008f2c,327,false},
};
constexpr ActorDependencyEvent d_802c1f74_calls[] = {
    {"GET_POKEMON",0x0,333,false},
    {"Pokemon_ForceAnimation",0x8035f15c,343,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,345,false},
    {"Pokemon_SetState",0x8035ec58,346,false},
};
constexpr std::uint32_t d_802c1f74_graphs[] = {0x802c741c};
constexpr std::uint32_t d_802c1f74_headers[] = {0x802c7260};
constexpr ActorDependencyEvent d_802c2058_calls[] = {
    {"GET_POKEMON",0x0,350,false},
    {"getLevelProgress",0x80357384,364,false},
    {"ohWait",0x8000bca8,368,false},
    {"Pokemon_StartAuxProc",0x8035ed90,371,false},
    {"Items_GetPokeFluteCmd",0x8035c834,372,false},
    {"ohWait",0x8000bca8,373,false},
    {"auPlaySong",0x80022374,380,false},
    {"ABS",0x0,393,false},
    {"ARRAY_COUNT",0x0,401,false},
    {"auSetBGMChannelVolume",0x800226f4,402,false},
    {"ohWait",0x8000bca8,405,false},
};
constexpr std::uint32_t d_802c2058_callbacks[] = {0x802c2308};
constexpr ActorDependencyEvent d_802c2308_calls[] = {
    {"GET_POKEMON",0x0,411,false},
    {"getLevelProgress",0x80357384,423,false},
    {"Items_GetPokeFluteCmd",0x8035c834,427,false},
    {"ARRAY_COUNT",0x0,428,false},
    {"auSetBGMChannelVolume",0x800226f4,429,false},
    {"ohWait",0x8000bca8,432,false},
    {"Items_GetPokeFluteCmd",0x8035c834,435,false},
    {"auStopSong",0x800223f8,436,false},
    {"setBackgroundMusic",0x803507e0,439,false},
    {"Pokemon_StopAuxProc",0x8035edc8,440,false},
};
constexpr ActorDependencyEvent d_802c2504_calls[] = {
    {"GET_POKEMON",0x0,445,false},
    {"Pokemon_SetAnimation",0x8035f138,449,false},
    {"Pokemon_StartPathProc",0x8035edf8,450,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,452,false},
    {"Pokemon_SetState",0x8035ec58,453,false},
};
constexpr std::uint32_t d_802c2504_callbacks[] = {0x802c1a7c,0x802c257c};
constexpr std::uint32_t d_802c2504_headers[] = {0x802c724c};
constexpr ActorDependencyEvent d_802c257c_calls[] = {
    {"GET_POKEMON",0x0,458,false},
    {"Pokemon_ForceAnimation",0x8035f15c,461,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,463,false},
    {"Pokemon_SetState",0x8035ec58,464,false},
};
constexpr std::uint32_t d_802c257c_callbacks[] = {0x802c1f74};
constexpr std::uint32_t d_802c257c_headers[] = {0x802c7274};
constexpr ActorDependencyEvent d_802c25d8_calls[] = {
    {"GET_POKEMON",0x0,469,false},
    {"Pokemon_SetAnimation",0x8035f138,474,false},
    {"func_802C1440_6438F0",0x802c1440,475,false},
    {"Pokemon_SetState",0x8035ec58,477,false},
};
constexpr std::uint32_t d_802c25d8_callbacks[] = {0x802c1f74};
constexpr std::uint32_t d_802c25d8_headers[] = {0x802c7274};
constexpr ActorDependencyEvent d_802c2650_calls[] = {
    {"ohWait",0x8000bca8,482,false},
    {"Pokemon_SetState",0x8035ec58,484,false},
};
constexpr std::uint32_t d_802c2650_callbacks[] = {0x802c2710};
constexpr ActorDependencyEvent d_802c26ac_calls[] = {
    {"ohWait",0x8000bca8,489,false},
    {"Pokemon_SetState",0x8035ec58,491,false},
};
constexpr std::uint32_t d_802c26ac_callbacks[] = {0x802c2710};
constexpr ActorDependencyEvent d_802c2710_calls[] = {
    {"GET_POKEMON",0x0,495,false},
    {"SHOW_POKEMON",0x0,497,false},
    {"Pokemon_StartPathProc",0x8035edf8,498,false},
    {"Pokemon_SetState",0x8035ec58,499,false},
};
constexpr std::uint32_t d_802c2710_callbacks[] = {0x802c2758,0x802c27e4};
constexpr ActorDependencyEvent d_802c2758_calls[] = {
    {"GET_POKEMON",0x0,504,false},
    {"Pokemon_SetAnimation",0x8035f138,507,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,509,false},
    {"Pokemon_SetState",0x8035ec58,510,false},
};
constexpr std::uint32_t d_802c2758_graphs[] = {0x802c748c};
constexpr std::uint32_t d_802c2758_headers[] = {0x802c7238};
constexpr ActorDependencyEvent d_802c27e4_calls[] = {
    {"GET_POKEMON",0x0,515,false},
    {"Pokemon_ResetPathPos",0x80362050,517,false},
    {"Pokemon_FollowPath",0x803620c8,518,false},
    {"omEndProcess",0x80008f2c,521,false},
};
constexpr ActorDependencyEvent d_802c2854_calls[] = {
    {"Pokemon_SetAnimation",0x8035f138,528,false},
    {"func_802C1440_6438F0",0x802c1440,529,false},
    {"Pokemon_SetState",0x8035ec58,530,false},
};
constexpr std::uint32_t d_802c2854_callbacks[] = {0x802c2758};
constexpr std::uint32_t d_802c2854_graphs[] = {0x802c74ec};
constexpr std::uint32_t d_802c2854_headers[] = {0x802c7210};
constexpr ActorDependencyEvent d_802c28d0_calls[] = {
    {"Pokemon_SetAnimation",0x8035f138,537,false},
    {"func_802C1440_6438F0",0x802c1440,538,false},
    {"Pokemon_SetState",0x8035ec58,539,false},
};
constexpr std::uint32_t d_802c28d0_callbacks[] = {0x802c2758};
constexpr std::uint32_t d_802c28d0_graphs[] = {0x802c751c};
constexpr std::uint32_t d_802c28d0_headers[] = {0x802c7224};
constexpr ActorDependencyEvent d_802c294c_calls[] = {
    {"Pokemon_SetAnimation",0x8035f138,543,false},
    {"func_802C1440_6438F0",0x802c1440,544,false},
    {"Pokemon_SetState",0x8035ec58,545,false},
};
constexpr std::uint32_t d_802c294c_callbacks[] = {0x802c2758};
constexpr std::uint32_t d_802c294c_graphs[] = {0x802c754c};
constexpr std::uint32_t d_802c294c_headers[] = {0x802c7238};
constexpr ActorDependencyEvent d_802c2994_calls[] = {
    {"Pokemon_Spawn",0x80362e5c,549,false},
};
constexpr ActorDependencyEvent d_802c29d0_calls[] = {
    {"GET_POKEMON",0x0,31,false},
    {"HIDE_POKEMON",0x0,33,false},
    {"Pokemon_SetState",0x8035ec58,36,false},
    {"Pokemon_SetState",0x8035ec58,39,false},
    {"Pokemon_SetState",0x8035ec58,42,false},
    {"Pokemon_SetState",0x8035ec58,45,false},
    {"Pokemon_SetState",0x8035ec58,48,false},
    {"Pokemon_SetState",0x8035ec58,50,false},
};
constexpr std::uint32_t d_802c29d0_callbacks[] = {0x802c2ac4,0x802c2b0c,0x802c2b54,0x802c2bdc,0x802c2e24,0x802c2fb4};
constexpr ActorDependencyEvent d_802c2ac4_calls[] = {
    {"Pokemon_SetState",0x8035ec58,55,false},
    {"Pokemon_SetState",0x8035ec58,57,false},
};
constexpr std::uint32_t d_802c2ac4_callbacks[] = {0x802c2b9c,0x802c2d6c};
constexpr ActorDependencyEvent d_802c2b0c_calls[] = {
    {"Pokemon_SetState",0x8035ec58,62,false},
    {"Pokemon_SetState",0x8035ec58,64,false},
};
constexpr std::uint32_t d_802c2b0c_callbacks[] = {0x802c2b9c,0x802c2d6c};
constexpr ActorDependencyEvent d_802c2b54_calls[] = {
    {"Pokemon_SetState",0x8035ec58,69,false},
    {"Pokemon_SetState",0x8035ec58,71,false},
};
constexpr std::uint32_t d_802c2b54_callbacks[] = {0x802c2b9c,0x802c2d6c};
constexpr ActorDependencyEvent d_802c2b9c_calls[] = {
    {"GET_POKEMON",0x0,75,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,78,false},
    {"Pokemon_SetState",0x8035ec58,79,false},
};
constexpr std::uint32_t d_802c2b9c_graphs[] = {0x802c7614};
constexpr ActorDependencyEvent d_802c2bdc_calls[] = {
    {"GET_POKEMON",0x0,84,false},
    {"SHOW_POKEMON",0x0,86,false},
    {"Pokemon_SetAnimation",0x8035f138,87,false},
    {"Pokemon_StartPathProc",0x8035edf8,90,false},
    {"Pokemon_StartPathProc",0x8035edf8,92,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,96,false},
    {"Pokemon_SetState",0x8035ec58,97,false},
};
constexpr std::uint32_t d_802c2bdc_callbacks[] = {0x802c2c90,0x802c2d00,0x802c2d6c};
constexpr std::uint32_t d_802c2bdc_graphs[] = {0x802c7634};
constexpr std::uint32_t d_802c2bdc_headers[] = {0x802c75d8};
constexpr ActorDependencyEvent d_802c2c90_calls[] = {
    {"GET_POKEMON",0x0,102,false},
    {"Pokemon_ResetPathPos",0x80362050,104,false},
    {"Pokemon_FollowPath",0x803620c8,105,false},
    {"omEndProcess",0x80008f2c,108,false},
};
constexpr ActorDependencyEvent d_802c2d00_calls[] = {
    {"GET_POKEMON",0x0,113,false},
    {"Pokemon_ResetPathPos",0x80362050,115,false},
    {"Pokemon_FollowPath",0x803620c8,116,false},
    {"omEndProcess",0x80008f2c,119,false},
};
constexpr ActorDependencyEvent d_802c2d6c_calls[] = {
    {"Pokemon_RunCleanup",0x8035fd70,123,false},
    {"Pokemon_SetState",0x8035ec58,124,false},
};
constexpr ActorDependencyEvent d_802c2d98_calls[] = {
    {"GET_POKEMON",0x0,128,false},
    {"Pokemon_StopAuxProc",0x8035edc8,141,false},
};
constexpr ActorDependencyEvent d_802c2e24_calls[] = {
    {"GET_POKEMON",0x0,146,false},
    {"HIDE_POKEMON",0x0,149,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,151,false},
    {"Pokemon_SetState",0x8035ec58,152,false},
};
constexpr std::uint32_t d_802c2e24_callbacks[] = {0x802c2ebc};
constexpr std::uint32_t d_802c2e24_graphs[] = {0x802c7674};
constexpr ActorDependencyEvent d_802c2ebc_calls[] = {
    {"GET_POKEMON",0x0,157,false},
    {"SHOW_POKEMON",0x0,160,false},
    {"Pokemon_SetAnimation",0x8035f138,161,false},
    {"Pokemon_StartPathProc",0x8035edf8,162,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,164,false},
    {"Pokemon_RunCleanup",0x8035fd70,166,false},
    {"Pokemon_SetState",0x8035ec58,167,false},
};
constexpr std::uint32_t d_802c2ebc_callbacks[] = {0x802c2f48};
constexpr std::uint32_t d_802c2ebc_headers[] = {0x802c75d8};
constexpr ActorDependencyEvent d_802c2f48_calls[] = {
    {"GET_POKEMON",0x0,172,false},
    {"Pokemon_ResetPathPos",0x80362050,174,false},
    {"Pokemon_FollowPath",0x803620c8,175,false},
    {"omEndProcess",0x80008f2c,178,false},
};
constexpr ActorDependencyEvent d_802c2fb4_calls[] = {
    {"GET_POKEMON",0x0,182,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,185,false},
    {"Pokemon_SetState",0x8035ec58,186,false},
};
constexpr std::uint32_t d_802c2fb4_graphs[] = {0x802c7694};
constexpr ActorDependencyEvent d_802c2ff4_calls[] = {
    {"GET_POKEMON",0x0,192,false},
    {"SHOW_POKEMON",0x0,197,false},
    {"Pokemon_SetAnimation",0x8035f138,198,false},
    {"Pokemon_StartPathProc",0x8035edf8,199,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,201,false},
    {"Pokemon_SetState",0x8035ec58,202,false},
};
constexpr std::uint32_t d_802c2ff4_callbacks[] = {0x802c2c90,0x802c2d6c};
constexpr std::uint32_t d_802c2ff4_graphs[] = {0x802c76d4};
constexpr std::uint32_t d_802c2ff4_headers[] = {0x802c75d8};
constexpr ActorDependencyEvent d_802c30b8_calls[] = {
    {"GET_TRANSFORM",0x0,211,false},
    {"EnvSound_PlaySound",0x803667c0,212,false},
    {"getGroundAt",0x800e41d8,214,false},
    {"ohWait",0x8000bca8,216,false},
    {"cmdSendCommandToLink",0x8000b830,219,false},
    {"getGroundAt",0x800e41d8,222,false},
    {"ohWait",0x8000bca8,224,false},
    {"Pokemon_RunCleanup",0x8035fd70,227,false},
    {"Pokemon_SetState",0x8035ec58,228,false},
};
constexpr ActorDependencyEvent d_802c31d0_calls[] = {
    {"GET_POKEMON",0x0,234,false},
    {"Pokemon_Fall",0x80360590,239,false},
    {"Pokemon_Jump",0x80360300,241,false},
    {"Pokemon_StopAuxProc",0x8035edc8,242,false},
};
constexpr ActorDependencyEvent d_802c3250_calls[] = {
    {"Pokemon_Spawn",0x80362e5c,246,false},
};
constexpr ActorDependencyEvent d_802c3290_calls[] = {
    {"Pokemon_SetAnimation",0x8035f138,11,false},
    {"randRange",0x80019e14,12,false},
    {"Pokemon_StartPathProc",0x8035edf8,13,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,15,false},
    {"Pokemon_RunCleanup",0x8035fd70,17,false},
    {"Pokemon_SetState",0x8035ec58,18,false},
};
constexpr std::uint32_t d_802c3290_callbacks[] = {0x802c3348};
constexpr std::uint32_t d_802c3290_headers[] = {0x802c779c};
constexpr ActorDependencyEvent d_802c3348_calls[] = {
    {"func_8035E298_4FE6A8",0x8035e298,22,false},
    {"omEndProcess",0x80008f2c,25,false},
};
constexpr ActorDependencyEvent d_802c3388_calls[] = {
    {"Pokemon_SpawnOnGround",0x80362ee0,29,false},
};
constexpr ActorDependencyEvent d_802c33c0_calls[] = {
    {"Pokemon_SetState",0x8035ec58,22,false},
};
constexpr std::uint32_t d_802c33c0_callbacks[] = {0x802c33e4};
constexpr ActorDependencyEvent d_802c33e4_calls[] = {
    {"Pokemon_SetAnimation",0x8035f138,27,false},
    {"Pokemon_StartPathProc",0x8035edf8,28,false},
    {"Pokemon_GetDistance",0x8035f2a4,32,false},
    {"ohWait",0x8000bca8,36,false},
    {"Pokemon_SetState",0x8035ec58,38,false},
};
constexpr std::uint32_t d_802c33e4_callbacks[] = {0x802c3494,0x802c34e0};
constexpr std::uint32_t d_802c33e4_headers[] = {0x802c7880};
constexpr ActorDependencyEvent d_802c3494_calls[] = {
    {"Pokemon_TurnToTarget",0x8036148c,42,false},
    {"omEndProcess",0x80008f2c,45,false},
};
constexpr ActorDependencyEvent d_802c34e0_calls[] = {
    {"Pokemon_SetAnimation",0x8035f138,49,false},
    {"Pokemon_StartPathProc",0x8035edf8,50,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,53,false},
    {"EnvSound_PlaySound",0x803667c0,54,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,56,false},
    {"Pokemon_SetState",0x8035ec58,57,false},
};
constexpr std::uint32_t d_802c34e0_callbacks[] = {0x802c3580,0x802c3658};
constexpr std::uint32_t d_802c34e0_headers[] = {0x802c78bc};
constexpr ActorDependencyEvent d_802c3580_calls[] = {
    {"GET_TRANSFORM",0x0,61,false},
    {"GET_TRANSFORM",0x0,62,false},
    {"ohWait",0x8000bca8,69,false},
    {"omEndProcess",0x80008f2c,74,false},
};
constexpr ActorDependencyEvent d_802c3658_calls[] = {
    {"HIDE_POKEMON",0x0,78,false},
    {"ohWait",0x8000bca8,80,false},
    {"SHOW_POKEMON",0x0,82,false},
    {"Pokemon_StartPathProc",0x8035edf8,83,false},
    {"Pokemon_SetState",0x8035ec58,84,false},
};
constexpr std::uint32_t d_802c3658_callbacks[] = {0x802c36f0,0x802c3784};
constexpr ActorDependencyEvent d_802c36f0_calls[] = {
    {"Pokemon_ForceAnimation",0x8035f15c,90,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,92,false},
    {"Pokemon_SetState",0x8035ec58,93,false},
};
constexpr std::uint32_t d_802c36f0_graphs[] = {0x802c7984};
constexpr std::uint32_t d_802c36f0_headers[] = {0x802c7880};
constexpr ActorDependencyEvent d_802c3784_calls[] = {
    {"GET_TRANSFORM",0x0,97,false},
    {"GET_TRANSFORM",0x0,98,false},
    {"ohWait",0x8000bca8,105,false},
};
constexpr ActorDependencyEvent d_802c3840_calls[] = {
    {"cmdSendCommandToLink",0x8000b830,110,false},
    {"Pokemon_StopAuxProc",0x8035edc8,111,false},
};
constexpr ActorDependencyEvent d_802c3874_calls[] = {
    {"Pokemon_SetAnimation",0x8035f138,115,false},
    {"Pokemon_StartPathProc",0x8035edf8,116,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,118,false},
    {"Pokemon_SetState",0x8035ec58,119,false},
};
constexpr std::uint32_t d_802c3874_callbacks[] = {0x802c38e0,0x802c39b8};
constexpr std::uint32_t d_802c3874_headers[] = {0x802c78d0};
constexpr ActorDependencyEvent d_802c38e0_calls[] = {
    {"GET_POKEMON",0x0,37,false},
    {"GET_TRANSFORM",0x0,42,false},
    {"getGroundAt",0x800e41d8,47,false},
    {"ohWait",0x8000bca8,49,false},
    {"omEndProcess",0x80008f2c,53,false},
};
constexpr ActorDependencyEvent d_802c39b8_calls[] = {
    {"GET_POKEMON",0x0,58,false},
    {"Pokemon_SetAnimation",0x8035f138,61,false},
    {"Pokemon_StartAuxProc",0x8035ed90,62,false},
    {"Pokemon_StartPathProc",0x8035edf8,63,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,65,false},
    {"Pokemon_SetState",0x8035ec58,67,false},
};
constexpr std::uint32_t d_802c39b8_callbacks[] = {0x802c3a44,0x802c3b34,0x802c3b68};
constexpr std::uint32_t d_802c39b8_headers[] = {0x802c7934};
constexpr ActorDependencyEvent d_802c3a44_calls[] = {
    {"GET_POKEMON",0x0,73,false},
    {"GET_TRANSFORM",0x0,78,false},
    {"ohWait",0x8000bca8,86,false},
    {"getGroundAt",0x800e41d8,87,false},
    {"omEndProcess",0x80008f2c,94,false},
};
constexpr ActorDependencyEvent d_802c3b34_calls[] = {
    {"Pokemon_TurnToTarget",0x8036148c,98,false},
    {"Pokemon_StopAuxProc",0x8035edc8,99,false},
};
constexpr ActorDependencyEvent d_802c3b68_calls[] = {
    {"GET_POKEMON",0x0,104,false},
    {"Pokemon_SetAnimation",0x8035f138,107,false},
    {"Pokemon_StartPathProc",0x8035edf8,108,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,110,false},
    {"Pokemon_SetAnimation",0x8035f138,111,false},
    {"Pokemon_StartPathProc",0x8035edf8,112,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,114,false},
    {"Pokemon_SetAnimation",0x8035f138,117,false},
    {"Pokemon_TurnToTarget",0x8036148c,118,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,120,false},
    {"Pokemon_SetState",0x8035ec58,122,false},
};
constexpr std::uint32_t d_802c3b68_callbacks[] = {0x802c3c40,0x802c3d44};
constexpr std::uint32_t d_802c3b68_headers[] = {0x802c7880,0x802c78bc,0x802c790c};
constexpr ActorDependencyEvent d_802c3c40_calls[] = {
    {"GET_TRANSFORM",0x0,127,false},
    {"GET_POKEMON",0x0,131,false},
    {"GetInterpolatedPosition",0x8001fca4,137,false},
    {"getCurrentWorldBlock",0x800e2184,138,false},
    {"Pokemon_SetTargetPos",0x80361b50,142,false},
    {"Pokemon_RunToTargetPos",0x80361e58,143,false},
    {"omEndProcess",0x80008f2c,148,false},
};
constexpr ActorDependencyEvent d_802c3d44_calls[] = {
    {"GET_POKEMON",0x0,153,false},
    {"Pokemon_ForceAnimation",0x8035f15c,157,false},
    {"Pokemon_StartPathProc",0x8035edf8,158,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,160,false},
    {"Pokemon_SetState",0x8035ec58,161,false},
};
constexpr std::uint32_t d_802c3d44_callbacks[] = {0x802c3dfc};
constexpr std::uint32_t d_802c3d44_graphs[] = {0x802c79d0};
constexpr std::uint32_t d_802c3d44_headers[] = {0x802c7880};
constexpr ActorDependencyEvent d_802c3dfc_calls[] = {
    {"cmdSendCommand",0x8000b774,167,false},
    {"ohWait",0x8000bca8,169,false},
};
constexpr ActorDependencyEvent d_802c3e74_calls[] = {
    {"GET_POKEMON",0x0,175,false},
    {"Pokemon_SetAnimation",0x8035f138,177,false},
    {"Pokemon_StartPathProc",0x8035edf8,178,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,180,false},
    {"Pokemon_SetState",0x8035ec58,181,false},
};
constexpr std::uint32_t d_802c3e74_callbacks[] = {0x802c3d44};
constexpr std::uint32_t d_802c3e74_headers[] = {0x802c78d0};
constexpr ActorDependencyEvent d_802c3edc_calls[] = {
    {"GET_POKEMON",0x0,186,false},
    {"Pokemon_ForceAnimation",0x8035f15c,188,false},
    {"Pokemon_StartPathProc",0x8035edf8,189,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,191,false},
    {"Pokemon_SetState",0x8035ec58,193,false},
    {"Pokemon_SetState",0x8035ec58,195,false},
};
constexpr std::uint32_t d_802c3edc_callbacks[] = {0x802c3d44,0x802c3f6c};
constexpr std::uint32_t d_802c3edc_headers[] = {0x802c7894};
constexpr ActorDependencyEvent d_802c3f6c_calls[] = {
    {"GET_POKEMON",0x0,200,false},
    {"Pokemon_HearsPokeFlute",0x80361440,203,false},
    {"ohWait",0x8000bca8,206,false},
    {"omEndProcess",0x80008f2c,211,false},
};
constexpr ActorDependencyEvent d_802c3fd8_calls[] = {
    {"GET_POKEMON",0x0,216,false},
    {"Pokemon_ForceAnimation",0x8035f15c,219,false},
    {"Pokemon_StartPathProc",0x8035edf8,220,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,222,false},
    {"Pokemon_SetState",0x8035ec58,223,false},
};
constexpr std::uint32_t d_802c3fd8_callbacks[] = {0x802c4090};
constexpr std::uint32_t d_802c3fd8_graphs[] = {0x802c7a50};
constexpr std::uint32_t d_802c3fd8_headers[] = {0x802c78e4};
constexpr ActorDependencyEvent d_802c4090_calls[] = {
    {"GET_POKEMON",0x0,228,false},
    {"Pokemon_GetDistance",0x8035f2a4,231,false},
    {"ohWait",0x8000bca8,234,false},
    {"cmdSendCommand",0x8000b774,237,false},
    {"omEndProcess",0x80008f2c,240,false},
};
constexpr ActorDependencyEvent d_802c413c_calls[] = {
    {"GET_POKEMON",0x0,245,false},
    {"Pokemon_SetAnimation",0x8035f138,248,false},
    {"Pokemon_StartPathProc",0x8035edf8,249,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,251,false},
    {"Pokemon_StartPathProc",0x8035edf8,253,false},
    {"Pokemon_SetState",0x8035ec58,254,false},
};
constexpr std::uint32_t d_802c413c_callbacks[] = {0x802c41d0,0x802c4264,0x802c4378};
constexpr std::uint32_t d_802c413c_headers[] = {0x802c78bc};
constexpr ActorDependencyEvent d_802c41d0_calls[] = {
    {"GET_POKEMON",0x0,259,false},
    {"Pokemon_ForceAnimation",0x8035f15c,261,false},
    {"Pokemon_StartAuxProc",0x8035ed90,262,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,264,false},
    {"Pokemon_SetState",0x8035ec58,266,false},
    {"Pokemon_RunCleanup",0x8035fd70,268,false},
    {"Pokemon_SetState",0x8035ec58,269,false},
};
constexpr std::uint32_t d_802c41d0_callbacks[] = {0x802c433c};
constexpr std::uint32_t d_802c41d0_headers[] = {0x802c790c};
constexpr ActorDependencyEvent d_802c4264_calls[] = {
    {"GET_TRANSFORM",0x0,273,false},
    {"GET_TRANSFORM",0x0,274,false},
    {"ohWait",0x8000bca8,281,false},
    {"omEndProcess",0x80008f2c,286,false},
};
constexpr ActorDependencyEvent d_802c433c_calls[] = {
    {"ohWait",0x8000bca8,290,false},
    {"EnvSound_PlaySound",0x803667c0,291,false},
    {"Pokemon_StopAuxProc",0x8035edc8,292,false},
};
constexpr ActorDependencyEvent d_802c4378_calls[] = {
    {"GET_TRANSFORM",0x0,296,false},
    {"ABS",0x0,304,false},
    {"ABS",0x0,312,false},
    {"ABS",0x0,320,false},
    {"ohWait",0x8000bca8,331,false},
    {"omEndProcess",0x80008f2c,340,false},
};
constexpr ActorDependencyEvent d_802c45f4_calls[] = {
    {"GET_POKEMON",0x0,348,false},
    {"Pokemon_ResetPathPos",0x80362050,350,false},
    {"Pokemon_FollowPath",0x803620c8,351,false},
    {"omEndProcess",0x80008f2c,354,false},
};
constexpr ActorDependencyEvent d_802c465c_calls[] = {
    {"Pokemon_SpawnOnGround",0x80362ee0,358,false},
};
constexpr ActorDependencyEvent d_802c46a0_calls[] = {
    {"GET_POKEMON",0x0,18,false},
    {"HIDE_POKEMON",0x0,20,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,22,false},
    {"Pokemon_SetState",0x8035ec58,23,false},
};
constexpr std::uint32_t d_802c46a0_graphs[] = {0x802c7b50};
constexpr ActorDependencyEvent d_802c46f0_calls[] = {
    {"GET_POKEMON",0x0,28,false},
    {"GET_TRANSFORM",0x0,32,false},
    {"GET_TRANSFORM",0x0,32,false},
    {"GET_TRANSFORM",0x0,33,false},
    {"GET_TRANSFORM",0x0,33,false},
    {"GET_TRANSFORM",0x0,34,false},
    {"GET_TRANSFORM",0x0,34,false},
    {"SHOW_POKEMON",0x0,36,false},
    {"Pokemon_ForceAnimation",0x8035f15c,37,false},
    {"Pokemon_TurnToTarget",0x8036148c,38,false},
    {"Pokemon_StartPathProc",0x8035edf8,39,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,41,false},
    {"Pokemon_SetAnimation",0x8035f138,42,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,44,false},
    {"Pokemon_SetAnimation",0x8035f138,45,false},
    {"Pokemon_StartPathProc",0x8035edf8,46,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,48,false},
    {"Pokemon_RunCleanup",0x8035fd70,50,false},
    {"Pokemon_SetState",0x8035ec58,51,false},
};
constexpr std::uint32_t d_802c46f0_callbacks[] = {0x802c4820,0x802c493c};
constexpr std::uint32_t d_802c46f0_headers[] = {0x802c7b10,0x802c7b24,0x802c7b38};
constexpr ActorDependencyEvent d_802c4820_calls[] = {
    {"GET_POKEMON",0x0,56,false},
    {"GET_TRANSFORM",0x0,64,false},
    {"GET_TRANSFORM",0x0,65,false},
    {"atan2f",0x80019abc,70,false},
    {"__sinf",0x80032a20,71,false},
    {"__cosf",0x80038de0,72,false},
    {"ohWait",0x8000bca8,74,false},
    {"omEndProcess",0x80008f2c,79,false},
};
constexpr ActorDependencyEvent d_802c493c_calls[] = {
    {"GET_POKEMON",0x0,84,false},
    {"Pokemon_ResetPathPos",0x80362050,86,false},
    {"Pokemon_FollowPath",0x803620c8,87,false},
    {"omEndProcess",0x80008f2c,90,false},
};
constexpr ActorDependencyEvent d_802c49a8_calls[] = {
    {"Pokemon_Spawn",0x80362e5c,94,false},
};
constexpr ActorDependencyEvent d_802c49f0_calls[] = {
    {"GET_POKEMON",0x0,24,false},
    {"Pokemon_SetAnimation",0x8035f138,26,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,28,false},
    {"Pokemon_SetState",0x8035ec58,29,false},
};
constexpr std::uint32_t d_802c49f0_callbacks[] = {0x802c4a4c};
constexpr std::uint32_t d_802c49f0_graphs[] = {0x802c7c40};
constexpr std::uint32_t d_802c49f0_headers[] = {0x802c7bd0};
constexpr ActorDependencyEvent d_802c4a4c_calls[] = {
    {"GET_POKEMON",0x0,34,false},
    {"Pokemon_SetAnimation",0x8035f138,36,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,38,false},
    {"Pokemon_SetState",0x8035ec58,39,false},
};
constexpr std::uint32_t d_802c4a4c_graphs[] = {0x802c7c40};
constexpr std::uint32_t d_802c4a4c_headers[] = {0x802c7bd0};
constexpr ActorDependencyEvent d_802c4aa8_calls[] = {
    {"GET_POKEMON",0x0,44,false},
    {"Pokemon_SetAnimation",0x8035f138,46,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,48,false},
    {"Pokemon_SetState",0x8035ec58,49,false},
};
constexpr std::uint32_t d_802c4aa8_callbacks[] = {0x802c4a4c};
constexpr std::uint32_t d_802c4aa8_graphs[] = {0x802c7c40};
constexpr std::uint32_t d_802c4aa8_headers[] = {0x802c7c0c};
constexpr ActorDependencyEvent d_802c4b04_calls[] = {
    {"GET_POKEMON",0x0,54,false},
    {"Pokemon_SetAnimation",0x8035f138,57,false},
    {"Pokemon_StartPathProc",0x8035edf8,58,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,60,false},
    {"Pokemon_SetState",0x8035ec58,62,false},
    {"Pokemon_SetAnimation",0x8035f138,64,false},
    {"Pokemon_StartPathProc",0x8035edf8,65,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,67,false},
    {"cmdSendCommandToLink",0x8000b830,68,false},
    {"Pokemon_SetAnimation",0x8035f138,69,false},
    {"Pokemon_StartPathProc",0x8035edf8,70,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,72,false},
    {"HIDE_POKEMON",0x0,73,false},
    {"Pokemon_RunCleanup",0x8035fd70,75,false},
    {"Pokemon_SetState",0x8035ec58,76,false},
};
constexpr std::uint32_t d_802c4b04_callbacks[] = {0x802c4a4c,0x802c4c18,0x802c4c70,0x802c4d60};
constexpr std::uint32_t d_802c4b04_headers[] = {0x802c7be4,0x802c7bf8};
constexpr ActorDependencyEvent d_802c4c18_calls[] = {
    {"GET_POKEMON",0x0,81,false},
    {"func_80362414_502824",0x80362414,83,false},
    {"omEndProcess",0x80008f2c,86,false},
};
constexpr ActorDependencyEvent d_802c4c70_calls[] = {
    {"GET_POKEMON",0x0,91,false},
    {"animSetModelAndTextureAnimationSpeed",0x8000c5fc,102,false},
    {"Pokemon_HearsPokeFlute",0x80361440,103,false},
    {"ohWait",0x8000bca8,106,false},
    {"omEndProcess",0x80008f2c,118,false},
};
constexpr std::uint32_t d_802c4c70_headers[] = {0x802c7be4};
constexpr ActorDependencyEvent d_802c4d60_calls[] = {
    {"GET_TRANSFORM",0x0,122,false},
    {"Pokemon_SetTargetPos",0x80361b50,128,false},
    {"Pokemon_StepToTargetPos",0x80361b68,131,false},
    {"ohWait",0x8000bca8,138,false},
    {"omEndProcess",0x80008f2c,143,false},
};
constexpr ActorDependencyEvent d_802c4e64_calls[] = {
    {"Pokemon_SpawnOnGround",0x80362ee0,147,false},
};
constexpr ActorDependencyEvent d_802c4eb0_calls[] = {
    {"getGroundAt",0x800e41d8,61,false},
};
constexpr ActorDependencyEvent d_802c4ef4_calls[] = {
    {"GET_TRANSFORM",0x0,74,false},
    {"GET_TRANSFORM",0x0,75,false},
    {"GET_POKEMON",0x0,76,false},
    {"func_802C4EB0_647360",0x802c4eb0,81,false},
    {"Pokemon_SetState",0x8035ec58,85,false},
};
constexpr std::uint32_t d_802c4ef4_callbacks[] = {0x802c4f5c};
constexpr ActorDependencyEvent d_802c4f5c_calls[] = {
    {"GET_POKEMON",0x0,90,false},
    {"Pokemon_SetAnimation",0x8035f138,92,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,94,false},
    {"Pokemon_SetState",0x8035ec58,95,false},
};
constexpr std::uint32_t d_802c4f5c_graphs[] = {0x802c7d94};
constexpr std::uint32_t d_802c4f5c_headers[] = {0x802c7d1c};
constexpr ActorDependencyEvent d_802c4fb8_calls[] = {
    {"GET_POKEMON",0x0,100,false},
    {"Pokemon_StartPathProc",0x8035edf8,102,false},
    {"Pokemon_SetAnimation",0x8035f138,103,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,105,false},
    {"Pokemon_SetState",0x8035ec58,106,false},
};
constexpr std::uint32_t d_802c4fb8_callbacks[] = {0x802c4f5c,0x802c502c};
constexpr std::uint32_t d_802c4fb8_graphs[] = {0x802c7d94};
constexpr std::uint32_t d_802c4fb8_headers[] = {0x802c7d30};
constexpr ActorDependencyEvent d_802c502c_calls[] = {
    {"GET_POKEMON",0x0,110,false},
    {"GET_TRANSFORM",0x0,112,false},
    {"GET_TRANSFORM",0x0,113,false},
    {"func_802C4EB0_647360",0x802c4eb0,121,false},
    {"ohWait",0x8000bca8,130,false},
    {"omEndProcess",0x80008f2c,134,false},
};
constexpr ActorDependencyEvent d_802c5188_calls[] = {
    {"GET_POKEMON",0x0,139,false},
    {"Pokemon_SetAnimation",0x8035f138,142,false},
    {"Pokemon_StartPathProc",0x8035edf8,143,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,145,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,147,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,150,false},
    {"Pokemon_SetState",0x8035ec58,151,false},
};
constexpr std::uint32_t d_802c5188_callbacks[] = {0x802c4fb8,0x802c5230};
constexpr std::uint32_t d_802c5188_graphs[] = {0x802c7ddc};
constexpr std::uint32_t d_802c5188_headers[] = {0x802c7d44};
constexpr ActorDependencyEvent d_802c5230_calls[] = {
    {"GET_POKEMON",0x0,155,false},
    {"GET_TRANSFORM",0x0,157,false},
    {"GET_TRANSFORM",0x0,158,false},
    {"func_802C4EB0_647360",0x802c4eb0,166,false},
    {"ohWait",0x8000bca8,175,false},
    {"omEndProcess",0x80008f2c,179,false},
};
constexpr ActorDependencyEvent d_802c5380_calls[] = {
    {"GET_POKEMON",0x0,184,false},
    {"Pokemon_SetAnimation",0x8035f138,186,false},
    {"Pokemon_StartPathProc",0x8035edf8,187,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,189,false},
    {"Pokemon_SetState",0x8035ec58,190,false},
};
constexpr std::uint32_t d_802c5380_callbacks[] = {0x802c4fb8,0x802c54dc};
constexpr std::uint32_t d_802c5380_graphs[] = {0x802c7e1c};
constexpr std::uint32_t d_802c5380_headers[] = {0x802c7d58};
constexpr ActorDependencyEvent d_802c53f4_calls[] = {
    {"GET_POKEMON",0x0,195,false},
    {"Pokemon_SetAnimation",0x8035f138,197,false},
    {"Pokemon_StartPathProc",0x8035edf8,198,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,200,false},
    {"Pokemon_SetState",0x8035ec58,201,false},
};
constexpr std::uint32_t d_802c53f4_callbacks[] = {0x802c4fb8,0x802c54dc};
constexpr std::uint32_t d_802c53f4_graphs[] = {0x802c7e4c};
constexpr std::uint32_t d_802c53f4_headers[] = {0x802c7d6c};
constexpr ActorDependencyEvent d_802c5468_calls[] = {
    {"GET_POKEMON",0x0,206,false},
    {"Pokemon_SetAnimation",0x8035f138,208,false},
    {"Pokemon_StartPathProc",0x8035edf8,209,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,211,false},
    {"Pokemon_SetState",0x8035ec58,212,false},
};
constexpr std::uint32_t d_802c5468_callbacks[] = {0x802c4fb8,0x802c54dc};
constexpr std::uint32_t d_802c5468_graphs[] = {0x802c7e7c};
constexpr std::uint32_t d_802c5468_headers[] = {0x802c7d80};
constexpr ActorDependencyEvent d_802c54dc_calls[] = {
    {"GET_POKEMON",0x0,217,false},
    {"Pokemon_HearsPokeFlute",0x80361440,220,false},
    {"ohWait",0x8000bca8,223,false},
    {"omEndProcess",0x80008f2c,228,false},
};
constexpr ActorDependencyEvent d_802c5548_calls[] = {
    {"Pokemon_Spawn",0x80362e5c,232,false},
};
constexpr ActorDependencyEvent d_802c5580_calls[] = {
    {"GET_POKEMON",0x0,237,false},
    {"Pokemon_SetAnimation",0x8035f138,239,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,241,false},
    {"Pokemon_SetState",0x8035ec58,242,false},
};
constexpr std::uint32_t d_802c5580_headers[] = {0x802c7f08};
constexpr ActorDependencyEvent d_802c55d4_calls[] = {
    {"Pokemon_Spawn",0x80362e5c,246,false},
};
constexpr ActorDependencyEvent d_802c5620_calls[] = {
    {"Pokemon_SetState",0x8035ec58,250,false},
};
constexpr std::uint32_t d_802c5620_callbacks[] = {0x802c5644};
constexpr ActorDependencyEvent d_802c5644_calls[] = {
    {"GET_POKEMON",0x0,255,false},
    {"Pokemon_SetAnimation",0x8035f138,257,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,259,false},
    {"Pokemon_SetState",0x8035ec58,260,false},
};
constexpr std::uint32_t d_802c5644_graphs[] = {0x802c7fc8};
constexpr std::uint32_t d_802c5644_headers[] = {0x802c7f70};
constexpr ActorDependencyEvent d_802c569c_calls[] = {
    {"GET_POKEMON",0x0,265,false},
    {"cmdSendCommand",0x8000b774,267,false},
    {"auPlaySound",0x800228e4,268,false},
    {"Pokemon_SetAnimation",0x8035f138,269,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,271,false},
    {"auPlaySound",0x800228e4,272,false},
    {"auPlaySound",0x800228e4,273,false},
    {"Pokemon_SetAnimation",0x8035f138,274,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,276,false},
    {"Pokemon_SetAnimation",0x8035f138,277,false},
    {"Pokemon_WaitForFlag",0x8035fbf0,279,false},
    {"Pokemon_SetState",0x8035ec58,280,false},
};
constexpr std::uint32_t d_802c569c_callbacks[] = {0x802c5970};
constexpr std::uint32_t d_802c569c_headers[] = {0x802c7f84,0x802c7f98,0x802c7fac};
constexpr ActorDependencyEvent d_802c57a4_calls[] = {
    {"auSetBGMVolume",0x80022454,300,false},
    {"auSetBGMVolume",0x80022454,301,false},
    {"ohWait",0x8000bca8,302,false},
    {"auSetBGMVolume",0x80022454,305,false},
    {"auSetBGMVolume",0x80022454,306,false},
    {"Pokemon_StopAuxProc",0x8035edc8,307,false},
};
constexpr ActorDependencyEvent d_802c58e4_calls[] = {
    {"ohWait",0x8000bca8,314,false},
    {"auPlaySound",0x800228e4,317,false},
    {"auStopSound",0x80022fd0,319,false},
    {"Pokemon_StopAuxProc",0x8035edc8,321,false},
};
constexpr ActorDependencyEvent d_802c5970_calls[] = {
    {"Pokemon_RemovePokemons",0x8035fd9c,333,false},
    {"ohWait",0x8000bca8,334,false},
    {"Pokemon_StartAuxProc",0x8035ed90,335,false},
    {"Pokemon_StartAuxProc",0x8035ed90,336,false},
    {"Items_RemoveFlyingItems",0x8035cab0,337,false},
    {"resetMainCameraSettings",0x803573bc,338,false},
    {"Msg_Reset",0x80365e34,339,false},
    {"Camera_StartCutScene",0x803570f0,340,false},
    {"getMainCamera",0x803586f8,341,false},
    {"ohPauseObjectProcesses",0x8000bd44,343,false},
    {"animSetCameraAnimation",0x8001054c,345,false},
    {"omCreateProcess",0x80008c28,346,false},
    {"setSkyBoxAnimationSpeed",0x800e1a78,347,false},
    {"PlayerModel_Init",0x80365e80,348,false},
    {"cmdSendCommand",0x8000b774,350,false},
    {"omEndProcess",0x80008f2c,351,false},
    {"PlayerModel_SetAnimation",0x80365f38,353,false},
    {"ohWait",0x8000bca8,358,false},
    {"auStopAllSounds",0x80022b14,361,false},
    {"cmdSendCommand",0x8000b774,362,false},
    {"omEndProcess",0x80008f2c,363,false},
    {"Pokemon_SetState",0x8035ec58,364,false},
};
constexpr std::uint32_t d_802c5970_callbacks[] = {0x802c57a4,0x802c58e4};
constexpr ActorDependencyEvent d_802c5b28_calls[] = {
    {"Pokemon_SpawnDlLink4",0x80362dc4,368,false},
};
constexpr ActorDependencyEvent d_802c5b60_calls[] = {
    {"func_803571C4_4F75D4",0x803571c4,372,false},
    {"func_80357120_4F7530",0x80357120,373,false},
    {"func_803570B0_4F74C0",0x803570b0,374,false},
};
constexpr ActorDependencyEvent d_802c5b90_calls[] = {
    {"func_803572B0_4F76C0",0x803572b0,378,false},
};
constexpr ActorDependencyEvent d_802c5bb0_calls[] = {
    {"fx_createEffect",0x800a6c48,392,false},
};
constexpr ActorDependencyEvent d_802c5c24_calls[] = {
    {"auStopSound",0x80022fd0,401,false},
    {"auStopSound",0x80022fd0,404,false},
    {"auStopSound",0x80022fd0,407,false},
    {"func_80357170_4F7580",0x80357170,409,false},
    {"resetMainCameraSettings",0x803573bc,410,false},
    {"func_802C5B90_648040",0x802c5b90,411,false},
    {"omDeleteGObj",0x8000a52c,412,false},
};
constexpr ActorDependencyEvent d_802c5cec_calls[] = {
    {"ohWait",0x8000bca8,419,false},
    {"auPlaySoundWithParams",0x80022a58,422,false},
    {"auPlaySoundWithParams",0x80022a58,423,false},
    {"omEndProcess",0x80008f2c,424,false},
};
constexpr ActorDependencyEvent d_802c5d80_calls[] = {
    {"ohWait",0x8000bca8,431,false},
    {"auPlaySoundWithParams",0x80022a58,434,false},
    {"auStopSound",0x80022fd0,435,false},
    {"omEndProcess",0x80008f2c,436,false},
};
constexpr ActorDependencyEvent d_802c5dfc_calls[] = {
    {"getMainCamera",0x803586f8,452,false},
    {"getMainCamera",0x803586f8,453,false},
    {"animSetCameraAnimation",0x8001054c,461,false},
    {"omCreateProcess",0x80008c28,462,false},
    {"PlayerModel_SetAnimation",0x80365f38,463,false},
    {"omCreateProcess",0x80008c28,466,false},
    {"omCreateProcess",0x80008c28,467,false},
    {"omEndProcess",0x80008f2c,471,false},
    {"func_802C5C24_6480D4",0x802c5c24,472,false},
    {"ohWait",0x8000bca8,474,false},
    {"omEndProcess",0x80008f2c,477,false},
    {"func_802C5C24_6480D4",0x802c5c24,491,false},
    {"ohWait",0x8000bca8,499,false},
    {"func_802C5C24_6480D4",0x802c5c24,501,false},
    {"ohWait",0x8000bca8,502,false},
};
constexpr std::uint32_t d_802c5dfc_callbacks[] = {0x802c5cec,0x802c5d80};
constexpr ActorDependencyEvent d_802c60f4_calls[] = {
    {"PlayerModel_Init",0x80365e80,508,false},
    {"omCreateProcess",0x80008c28,511,false},
    {"func_802C5B60_648010",0x802c5b60,512,false},
};
constexpr std::uint32_t d_802c60f4_callbacks[] = {0x802c5b60,0x802c5dfc};
constexpr ActorTypedDependency d_802bdfa0_typed[] = {
    {0x802bdf34,ActorSourceAbi::utility,"void func_802BDF34_6403E4(WorldBlock* arg0, WorldBlock* arg1)"},
    {0x802bdf58,ActorSourceAbi::utility,"void func_802BDF58_640408(WorldBlock* arg0, WorldBlock* arg1)"},
    {0x802bdf7c,ActorSourceAbi::utility,"void func_802BDF7C_64042C(WorldBlock* arg0)"},
};
constexpr ActorTypedDependency d_802be0ac_typed[] = {
    {0x802be038,ActorSourceAbi::utility,"void func_802BE038_6404E8(s32 arg0)"},
};
constexpr ActorTypedDependency d_802be22c_typed[] = {
    {0x802bddcc,ActorSourceAbi::object_utility,"void func_802BDDCC_64027C(GObj* obj, GroundResult* groundResult)"},
    {0x802bdfa0,ActorSourceAbi::utility,"void func_802BDFA0_640450(void)"},
    {0x802be038,ActorSourceAbi::utility,"void func_802BE038_6404E8(s32 arg0)"},
    {0x802be0a4,ActorSourceAbi::utility,"void func_802BE0A4_640554(s32 arg0)"},
    {0x802be0ac,ActorSourceAbi::utility,"void func_802BE0AC_64055C(WorldBlock* arg0)"},
    {0x802be140,ActorSourceAbi::utility,"void func_802BE140_6405F0(void)"},
    {0x802be450,ActorSourceAbi::utility,"void func_802BE450_640900(s32 arg0, f32 arg1)"},
    {0x802c60f4,ActorSourceAbi::utility,"void func_802C60F4_6485A4(void)"},
};
constexpr ActorTypedDependency d_802c0630_typed[] = {
    {0x802c0140,ActorSourceAbi::object_utility,"void func_802C0140_6425F0(GObj* obj, InteractionHandler* arg1)"},
};
constexpr ActorTypedDependency d_802c06bc_typed[] = {
    {0x802c0140,ActorSourceAbi::object_utility,"void func_802C0140_6425F0(GObj* obj, InteractionHandler* arg1)"},
};
constexpr ActorTypedDependency d_802c0748_typed[] = {
    {0x802c0140,ActorSourceAbi::object_utility,"void func_802C0140_6425F0(GObj* obj, InteractionHandler* arg1)"},
};
constexpr ActorTypedDependency d_802c1794_typed[] = {
    {0x802c1440,ActorSourceAbi::object_utility,"void func_802C1440_6438F0(GObj* obj, InteractionHandler* arg1)"},
};
constexpr ActorTypedDependency d_802c1818_typed[] = {
    {0x802c1440,ActorSourceAbi::object_utility,"void func_802C1440_6438F0(GObj* obj, InteractionHandler* arg1)"},
};
constexpr ActorTypedDependency d_802c189c_typed[] = {
    {0x802c1440,ActorSourceAbi::object_utility,"void func_802C1440_6438F0(GObj* obj, InteractionHandler* arg1)"},
};
constexpr ActorTypedDependency d_802c25d8_typed[] = {
    {0x802c1440,ActorSourceAbi::object_utility,"void func_802C1440_6438F0(GObj* obj, InteractionHandler* arg1)"},
};
constexpr ActorTypedDependency d_802c2854_typed[] = {
    {0x802c1440,ActorSourceAbi::object_utility,"void func_802C1440_6438F0(GObj* obj, InteractionHandler* arg1)"},
};
constexpr ActorTypedDependency d_802c28d0_typed[] = {
    {0x802c1440,ActorSourceAbi::object_utility,"void func_802C1440_6438F0(GObj* obj, InteractionHandler* arg1)"},
};
constexpr ActorTypedDependency d_802c294c_typed[] = {
    {0x802c1440,ActorSourceAbi::object_utility,"void func_802C1440_6438F0(GObj* obj, InteractionHandler* arg1)"},
};
constexpr ActorTypedDependency d_802c4ef4_typed[] = {
    {0x802c4eb0,ActorSourceAbi::model_utility,"void func_802C4EB0_647360(PokemonTransform* transform, f32 arg1)"},
};
constexpr ActorTypedDependency d_802c502c_typed[] = {
    {0x802c4eb0,ActorSourceAbi::model_utility,"void func_802C4EB0_647360(PokemonTransform* transform, f32 arg1)"},
};
constexpr ActorTypedDependency d_802c5230_typed[] = {
    {0x802c4eb0,ActorSourceAbi::model_utility,"void func_802C4EB0_647360(PokemonTransform* transform, f32 arg1)"},
};
constexpr ActorTypedDependency d_802c5970_typed[] = {
    {0x802c5778,ActorSourceAbi::animation,"void func_802C5778_647C28(DObj* arg0, s32 arg1, f32 arg2)"},
};
constexpr ActorTypedDependency d_802c5c24_typed[] = {
    {0x802c5b90,ActorSourceAbi::utility,"void func_802C5B90_648040(void)"},
};
constexpr ActorTypedDependency d_802c5dfc_typed[] = {
    {0x802c5bb0,ActorSourceAbi::animation,"void func_802C5BB0_648060(DObj* dobj, s32 arg1, f32 arg2)"},
    {0x802c5c24,ActorSourceAbi::utility,"void func_802C5C24_6480D4(void)"},
};
constexpr ActorTypedDependency d_802c4c18_common_egg_typed[] = {{0x80362414,ActorSourceAbi::common_egg_driver,"void func_80362414_502824(Pokemon* obj, EggStruct* egg, f32 arg2)"}};
constexpr ActorDependencyEvent shared_8035e1d4_calls[] = {
    {"Pokemon_AddAtGeo",0x8035fe24,148,false},
    {"GET_TRANSFORM",0x0,149,false},
    {"getGroundAt",0x800e41d8,150,false},
    {"GET_POKEMON",0x0,152,false},
};
constexpr std::uint32_t shared_8035e1d4_child_factories[] = {0x8035e0d4};
constexpr ActorTypedDependency d_802bff74_shared_typed[] = {{0x8035e1d4,ActorSourceAbi::object_utility,"GObj* func_8035E1D4_4FE5E4(GObj* obj)"}};
const ActorProviderDependency provider_dependencies[] = {
    {0x8012a0c0,ActorSourceAbi::utility,true,"src/cave/1F5E70.c",6,7,"void func_8012A0C0_1F5E70(void)",{},{},{},{},{},{},{}},
    {0x802bdd00,ActorSourceAbi::callback,false,"src/cave/6401B0.c",25,52,"void func_802BDD00_6401B0(GObj* obj)",d_802bdd00_calls,{},{},{},{},{},d_802bdd00_child_factories},
    {0x802bddcc,ActorSourceAbi::object_utility,false,"src/cave/6401B0.c",54,80,"void func_802BDDCC_64027C(GObj* obj, GroundResult* groundResult)",d_802bddcc_calls,{},{},{},{},{},{}},
    {0x802bdf34,ActorSourceAbi::utility,true,"src/cave/6401B0.c",82,84,"void func_802BDF34_6403E4(WorldBlock* arg0, WorldBlock* arg1)",d_802bdf34_calls,{},{},{},{},{},{}},
    {0x802bdf58,ActorSourceAbi::utility,true,"src/cave/6401B0.c",86,88,"void func_802BDF58_640408(WorldBlock* arg0, WorldBlock* arg1)",d_802bdf58_calls,{},{},{},{},{},{}},
    {0x802bdf7c,ActorSourceAbi::utility,true,"src/cave/6401B0.c",90,92,"void func_802BDF7C_64042C(WorldBlock* arg0)",d_802bdf7c_calls,{},{},{},{},{},{}},
    {0x802bdfa0,ActorSourceAbi::utility,true,"src/cave/6401B0.c",94,100,"void func_802BDFA0_640450(void)",d_802bdfa0_calls,d_802bdfa0_typed,{},{},{},{},{}},
    {0x802be038,ActorSourceAbi::utility,true,"src/cave/6401B0.c",102,113,"void func_802BE038_6404E8(s32 arg0)",d_802be038_calls,{},{},{},{},{},{}},
    {0x802be09c,ActorSourceAbi::callback,true,"src/cave/6401B0.c",115,116,"void func_802BE09C_64054C(GObj* arg0)",{},{},{},{},{},{},{}},
    {0x802be0a4,ActorSourceAbi::utility,true,"src/cave/6401B0.c",118,119,"void func_802BE0A4_640554(s32 arg0)",{},{},{},{},{},{},{}},
    {0x802be0ac,ActorSourceAbi::utility,true,"src/cave/6401B0.c",121,135,"void func_802BE0AC_64055C(WorldBlock* arg0)",d_802be0ac_calls,d_802be0ac_typed,{},{},{},{},{}},
    {0x802be140,ActorSourceAbi::utility,true,"src/cave/6401B0.c",137,154,"void func_802BE140_6405F0(void)",d_802be140_calls,{},{},{},{},{},{}},
    {0x802be22c,ActorSourceAbi::utility,true,"src/cave/6401B0.c",156,182,"void func_802BE22C_6406DC(void)",d_802be22c_calls,d_802be22c_typed,d_802be22c_callbacks,{},{},{},{}},
    {0x802be3a8,ActorSourceAbi::utility,true,"src/cave/6401B0.c",184,185,"void func_802BE3A8_640858(s32 arg0)",{},{},{},{},{},{},{}},
    {0x802be3b0,ActorSourceAbi::utility,true,"src/cave/6401B0.c",187,204,"s32 func_802BE3B0_640860(s32 arg0)",d_802be3b0_calls,{},{},{},{},{},{}},
    {0x802be450,ActorSourceAbi::utility,false,"src/cave/640900.c",15,62,"void func_802BE450_640900(s32 arg0, f32 arg1)",d_802be450_calls,{},{},{},{},{},{}},
    {0x802be820,ActorSourceAbi::callback,false,"src/cave/640CD0.c",33,43,"void func_802BE820_640CD0(GObj* obj)",d_802be820_calls,{},{},{},{},{},d_802be820_child_factories},
    {0x802be898,ActorSourceAbi::callback,false,"src/cave/640CD0.c",45,52,"void func_802BE898_640D48(GObj* obj)",d_802be898_calls,{},d_802be898_callbacks,{},{},{},{}},
    {0x802be8d4,ActorSourceAbi::callback,false,"src/cave/640CD0.c",54,63,"void func_802BE8D4_640D84(GObj* obj)",d_802be8d4_calls,{},{},d_802be8d4_graphs,d_802be8d4_weighted_states,d_802be8d4_headers,{}},
    {0x802be944,ActorSourceAbi::callback,false,"src/cave/640CD0.c",65,74,"void func_802BE944_640DF4(GObj* obj)",d_802be944_calls,{},{},d_802be944_graphs,d_802be944_weighted_states,d_802be944_headers,{}},
    {0x802be9b4,ActorSourceAbi::callback,false,"src/cave/640CD0.c",76,85,"void func_802BE9B4_640E64(GObj* obj)",d_802be9b4_calls,{},{},d_802be9b4_graphs,d_802be9b4_weighted_states,d_802be9b4_headers,{}},
    {0x802bea24,ActorSourceAbi::callback,false,"src/cave/640CD0.c",87,100,"void func_802BEA24_640ED4(GObj* obj)",d_802bea24_calls,{},d_802bea24_callbacks,d_802bea24_graphs,d_802bea24_weighted_states,d_802bea24_headers,{}},
    {0x802beac8,ActorSourceAbi::callback,false,"src/cave/640CD0.c",102,111,"void func_802BEAC8_640F78(GObj* obj)",d_802beac8_calls,{},{},{},{},{},{}},
    {0x802beb24,ActorSourceAbi::callback,false,"src/cave/640CD0.c",114,127,"void func_802BEB24_640FD4(GObj* obj)",d_802beb24_calls,{},d_802beb24_callbacks,{},{},d_802beb24_headers,{}},
    {0x802bebbc,ActorSourceAbi::callback,false,"src/cave/640CD0.c",129,139,"void func_802BEBBC_64106C(GObj* obj)",d_802bebbc_calls,{},d_802bebbc_callbacks,d_802bebbc_graphs,{},d_802bebbc_headers,{}},
    {0x802bec34,ActorSourceAbi::callback,false,"src/cave/640CD0.c",141,150,"void func_802BEC34_6410E4(GObj* obj)",d_802bec34_calls,{},d_802bec34_callbacks,d_802bec34_graphs,{},d_802bec34_headers,{}},
    {0x802beca8,ActorSourceAbi::callback,false,"src/cave/640CD0.c",152,161,"void func_802BECA8_641158(GObj* obj)",d_802beca8_calls,{},{},{},{},{},{}},
    {0x802bed04,ActorSourceAbi::callback,false,"src/cave/640CD0.c",163,174,"void func_802BED04_6411B4(GObj* obj)",d_802bed04_calls,{},d_802bed04_callbacks,d_802bed04_graphs,{},{},{}},
    {0x802bed8c,ActorSourceAbi::callback,false,"src/cave/640CD0.c",176,186,"void func_802BED8C_64123C(GObj* obj)",d_802bed8c_calls,{},{},{},{},d_802bed8c_headers,{}},
    {0x802bee10,ActorSourceAbi::callback,false,"src/cave/640CD0.c",188,200,"void func_802BEE10_6412C0(GObj* obj)",d_802bee10_calls,{},d_802bee10_callbacks,d_802bee10_graphs,d_802bee10_weighted_states,d_802bee10_headers,{}},
    {0x802beea4,ActorSourceAbi::callback,false,"src/cave/640CD0.c",202,207,"void func_802BEEA4_641354(GObj* obj)",d_802beea4_calls,{},{},{},{},{},{}},
    {0x802beef4,ActorSourceAbi::callback,false,"src/cave/640CD0.c",209,211,"void func_802BEEF4_6413A4(GObj* obj)",d_802beef4_calls,{},d_802beef4_callbacks,{},{},{},{}},
    {0x802bef18,ActorSourceAbi::callback,false,"src/cave/640CD0.c",213,222,"void func_802BEF18_6413C8(GObj* obj)",d_802bef18_calls,{},d_802bef18_callbacks,d_802bef18_graphs,{},d_802bef18_headers,{}},
    {0x802bef8c,ActorSourceAbi::callback,false,"src/cave/640CD0.c",224,232,"void func_802BEF8C_64143C(GObj* obj)",d_802bef8c_calls,{},{},{},{},{},{}},
    {0x802befd8,ActorSourceAbi::factory6,false,"src/cave/640CD0.c",234,236,"GObj* func_802BEFD8_641488(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)",d_802befd8_calls,{},{},{},{},{},{}},
    {0x802bf020,ActorSourceAbi::callback,false,"src/cave/6414D0.c",27,29,"void func_802BF020_6414D0(GObj* arg0)",d_802bf020_calls,{},d_802bf020_callbacks,{},{},{},{}},
    {0x802bf044,ActorSourceAbi::callback,false,"src/cave/6414D0.c",31,41,"void func_802BF044_6414F4(GObj* obj)",d_802bf044_calls,{},d_802bf044_callbacks,{},{},d_802bf044_headers,{}},
    {0x802bf0b4,ActorSourceAbi::callback,false,"src/cave/6414D0.c",43,52,"void func_802BF0B4_641564(GObj* obj)",d_802bf0b4_calls,{},{},d_802bf0b4_graphs,d_802bf0b4_weighted_states,d_802bf0b4_headers,{}},
    {0x802bf124,ActorSourceAbi::callback,false,"src/cave/6414D0.c",54,67,"void func_802BF124_6415D4(GObj* obj)",d_802bf124_calls,{},d_802bf124_callbacks,d_802bf124_graphs,d_802bf124_weighted_states,d_802bf124_headers,{}},
    {0x802bf1c4,ActorSourceAbi::callback,false,"src/cave/6414D0.c",69,78,"void func_802BF1C4_641674(GObj* obj)",d_802bf1c4_calls,{},{},{},{},{},{}},
    {0x802bf220,ActorSourceAbi::callback,false,"src/cave/6414D0.c",80,95,"void func_802BF220_6416D0(GObj* obj)",d_802bf220_calls,{},d_802bf220_callbacks,{},{},d_802bf220_headers,{}},
    {0x802bf2d0,ActorSourceAbi::callback,false,"src/cave/6414D0.c",97,106,"void func_802BF2D0_641780(GObj* obj)",d_802bf2d0_calls,{},d_802bf2d0_callbacks,{},{},d_802bf2d0_headers,{}},
    {0x802bf338,ActorSourceAbi::callback,false,"src/cave/6414D0.c",108,120,"void func_802BF338_6417E8(GObj* obj)",d_802bf338_calls,{},d_802bf338_callbacks,d_802bf338_graphs,{},d_802bf338_headers,{}},
    {0x802bf3d0,ActorSourceAbi::callback,false,"src/cave/6414D0.c",122,133,"void func_802BF3D0_641880(GObj* obj)",d_802bf3d0_calls,{},{},{},{},d_802bf3d0_headers,{}},
    {0x802bf454,ActorSourceAbi::callback,false,"src/cave/6414D0.c",135,147,"void func_802BF454_641904(GObj* obj)",d_802bf454_calls,{},d_802bf454_callbacks,d_802bf454_graphs,d_802bf454_weighted_states,d_802bf454_headers,{}},
    {0x802bf4e8,ActorSourceAbi::callback,false,"src/cave/6414D0.c",149,154,"void func_802BF4E8_641998(GObj* obj)",d_802bf4e8_calls,{},{},{},{},{},{}},
    {0x802bf554,ActorSourceAbi::utility,false,"src/cave/6414D0.c",156,157,"void func_802BF554_641A04(void)",{},{},{},{},{},{},{}},
    {0x802bf55c,ActorSourceAbi::factory6,false,"src/cave/6414D0.c",159,161,"GObj* func_802BF55C_641A0C(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)",d_802bf55c_calls,{},{},{},{},{},{}},
    {0x802bf5a0,ActorSourceAbi::callback,false,"src/cave/641A50.c",29,52,"void func_802BF5A0_641A50(GObj* obj)",d_802bf5a0_calls,{},d_802bf5a0_callbacks,{},{},{},{}},
    {0x802bf68c,ActorSourceAbi::callback,false,"src/cave/641A50.c",54,72,"void func_802BF68C_641B3C(GObj* obj)",d_802bf68c_calls,{},d_802bf68c_callbacks,d_802bf68c_graphs,d_802bf68c_weighted_states,d_802bf68c_headers,{}},
    {0x802bf750,ActorSourceAbi::callback,false,"src/cave/641A50.c",74,83,"void func_802BF750_641C00(GObj* obj)",d_802bf750_calls,{},{},{},{},{},{}},
    {0x802bf7b4,ActorSourceAbi::callback,false,"src/cave/641A50.c",85,94,"void func_802BF7B4_641C64(GObj* obj)",d_802bf7b4_calls,{},d_802bf7b4_callbacks,d_802bf7b4_graphs,{},d_802bf7b4_headers,{}},
    {0x802bf824,ActorSourceAbi::callback,false,"src/cave/641A50.c",96,105,"void func_802BF824_641CD4(GObj* obj)",d_802bf824_calls,{},d_802bf824_callbacks,d_802bf824_graphs,{},d_802bf824_headers,{}},
    {0x802bf894,ActorSourceAbi::callback,false,"src/cave/641A50.c",108,133,"void func_802BF894_641D44(GObj* obj)",d_802bf894_calls,{},d_802bf894_callbacks,d_802bf894_graphs,{},d_802bf894_headers,{}},
    {0x802bf9b4,ActorSourceAbi::callback,false,"src/cave/641A50.c",135,148,"void func_802BF9B4_641E64(GObj* obj)",d_802bf9b4_calls,{},{},{},{},{},{}},
    {0x802bfa3c,ActorSourceAbi::callback,false,"src/cave/641A50.c",150,161,"void func_802BFA3C_641EEC(GObj* obj)",d_802bfa3c_calls,{},d_802bfa3c_callbacks,{},{},{},{}},
    {0x802bfa8c,ActorSourceAbi::callback,false,"src/cave/641A50.c",163,175,"void func_802BFA8C_641F3C(GObj* obj)",d_802bfa8c_calls,{},d_802bfa8c_callbacks,d_802bfa8c_graphs,{},d_802bfa8c_headers,{}},
    {0x802bfb24,ActorSourceAbi::callback,false,"src/cave/641A50.c",177,189,"void func_802BFB24_641FD4(GObj* obj)",d_802bfb24_calls,{},{},{},{},{},{}},
    {0x802bfb9c,ActorSourceAbi::callback,false,"src/cave/641A50.c",191,199,"void func_802BFB9C_64204C(GObj* obj)",d_802bfb9c_calls,{},{},d_802bfb9c_graphs,{},{},{}},
    {0x802bfbf0,ActorSourceAbi::callback,false,"src/cave/641A50.c",202,215,"void func_802BFBF0_6420A0(GObj* obj)",d_802bfbf0_calls,{},d_802bfbf0_callbacks,{},{},{},{}},
    {0x802bfc84,ActorSourceAbi::callback,false,"src/cave/641A50.c",217,235,"POKEMON_FUNC(func_802BFC84_642134)",d_802bfc84_calls,{},{},{},{},{},{}},
    {0x802bfda0,ActorSourceAbi::factory6,false,"src/cave/641A50.c",237,239,"GObj* func_802BFDA0_642250(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)",d_802bfda0_calls,{},{},{},{},{},{}},
    {0x802bfde0,ActorSourceAbi::callback,false,"src/cave/642290.c",18,26,"void func_802BFDE0_642290(GObj* obj)",d_802bfde0_calls,{},{},d_802bfde0_graphs,{},{},{}},
    {0x802bfe34,ActorSourceAbi::callback,false,"src/cave/642290.c",28,41,"void func_802BFE34_6422E4(GObj* obj)",d_802bfe34_calls,{},d_802bfe34_callbacks,d_802bfe34_graphs,{},{},{}},
    {0x802bfef0,ActorSourceAbi::callback,false,"src/cave/642290.c",43,54,"void func_802BFEF0_6423A0(GObj* obj)",d_802bfef0_calls,{},d_802bfef0_callbacks,{},{},d_802bfef0_headers,{}},
    {0x802bff74,ActorSourceAbi::callback,false,"src/cave/642290.c",56,75,"POKEMON_FUNC(func_802BFF74_642424)",d_802bff74_calls,d_802bff74_shared_typed,{},{},{},{},{}},
    {0x802c0058,ActorSourceAbi::callback,false,"src/cave/642290.c",77,85,"void func_802C0058_642508(GObj* obj)",d_802c0058_calls,{},{},d_802c0058_graphs,{},d_802c0058_headers,{}},
    {0x802c00b0,ActorSourceAbi::callback,false,"src/cave/642290.c",87,95,"void func_802C00B0_642560(GObj* obj)",d_802c00b0_calls,{},d_802c00b0_callbacks,{},{},d_802c00b0_headers,{}},
    {0x802c0104,ActorSourceAbi::factory6,false,"src/cave/642290.c",97,99,"GObj* func_802C0104_6425B4(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)",d_802c0104_calls,{},{},{},{},{},{}},
    {0x802c0140,ActorSourceAbi::object_utility,false,"src/cave/6425F0.c",30,51,"void func_802C0140_6425F0(GObj* obj, InteractionHandler* arg1)",d_802c0140_calls,{},{},d_802c0140_graphs,{},{},{}},
    {0x802c0220,ActorSourceAbi::callback,false,"src/cave/6425F0.c",53,62,"void func_802C0220_6426D0(GObj* obj)",d_802c0220_calls,{},d_802c0220_callbacks,{},{},{},{}},
    {0x802c027c,ActorSourceAbi::callback,false,"src/cave/6425F0.c",64,70,"void func_802C027C_64272C(GObj* obj)",d_802c027c_calls,{},{},d_802c027c_graphs,{},{},{}},
    {0x802c02bc,ActorSourceAbi::callback,false,"src/cave/6425F0.c",72,79,"void func_802C02BC_64276C(GObj* obj)",d_802c02bc_calls,{},d_802c02bc_callbacks,{},{},{},{}},
    {0x802c0330,ActorSourceAbi::callback,false,"src/cave/6425F0.c",81,91,"void func_802C0330_6427E0(GObj* obj)",d_802c0330_calls,{},d_802c0330_callbacks,{},{},{},{}},
    {0x802c03e4,ActorSourceAbi::callback,false,"src/cave/6425F0.c",93,107,"void func_802C03E4_642894(GObj* obj)",d_802c03e4_calls,{},{},d_802c03e4_graphs,{},d_802c03e4_headers,{}},
    {0x802c04a4,ActorSourceAbi::callback,false,"src/cave/6425F0.c",109,118,"void func_802C04A4_642954(GObj* obj)",d_802c04a4_calls,{},{},{},{},{},{}},
    {0x802c0510,ActorSourceAbi::callback,false,"src/cave/6425F0.c",121,138,"void func_802C0510_6429C0(GObj* obj)",d_802c0510_calls,{},d_802c0510_callbacks,{},{},d_802c0510_headers,{}},
    {0x802c05d8,ActorSourceAbi::callback,false,"src/cave/6425F0.c",140,153,"void func_802C05D8_642A88(GObj* obj)",d_802c05d8_calls,{},{},{},{},{},{}},
    {0x802c0630,ActorSourceAbi::callback,false,"src/cave/6425F0.c",155,163,"void func_802C0630_642AE0(GObj* obj)",d_802c0630_calls,d_802c0630_typed,d_802c0630_callbacks,d_802c0630_graphs,{},d_802c0630_headers,{}},
    {0x802c06bc,ActorSourceAbi::callback,false,"src/cave/6425F0.c",165,173,"void func_802C06BC_642B6C(GObj* obj)",d_802c06bc_calls,d_802c06bc_typed,d_802c06bc_callbacks,d_802c06bc_graphs,{},d_802c06bc_headers,{}},
    {0x802c0748,ActorSourceAbi::callback,false,"src/cave/6425F0.c",175,183,"void func_802C0748_642BF8(GObj* obj)",d_802c0748_calls,d_802c0748_typed,d_802c0748_callbacks,d_802c0748_graphs,{},d_802c0748_headers,{}},
    {0x802c07d4,ActorSourceAbi::factory6,false,"src/cave/6425F0.c",185,187,"GObj* func_802C07D4_642C84(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)",d_802c07d4_calls,{},{},{},{},{},{}},
    {0x802c0810,ActorSourceAbi::callback,false,"src/cave/642CC0.c",36,44,"void evolveIntoMuk(GObj* obj)",d_802c0810_calls,{},{},{},{},{},d_802c0810_child_factories},
    {0x802c0888,ActorSourceAbi::callback,false,"src/cave/642CC0.c",46,67,"void func_802C0888_642D38(GObj* obj)",d_802c0888_calls,{},d_802c0888_callbacks,{},{},{},{}},
    {0x802c0960,ActorSourceAbi::callback,false,"src/cave/642CC0.c",69,74,"void func_802C0960_642E10(GObj* obj)",d_802c0960_calls,{},d_802c0960_callbacks,{},{},{},{}},
    {0x802c09c4,ActorSourceAbi::callback,false,"src/cave/642CC0.c",76,81,"void func_802C09C4_642E74(GObj* obj)",d_802c09c4_calls,{},d_802c09c4_callbacks,{},{},{},{}},
    {0x802c0a28,ActorSourceAbi::callback,false,"src/cave/642CC0.c",83,88,"void func_802C0A28_642ED8(GObj* obj)",d_802c0a28_calls,{},d_802c0a28_callbacks,{},{},{},{}},
    {0x802c0a8c,ActorSourceAbi::callback,false,"src/cave/642CC0.c",90,95,"void func_802C0A8C_642F3C(GObj* obj)",d_802c0a8c_calls,{},d_802c0a8c_callbacks,{},{},{},{}},
    {0x802c0af0,ActorSourceAbi::callback,false,"src/cave/642CC0.c",97,111,"void func_802C0AF0_642FA0(GObj* obj)",d_802c0af0_calls,{},{},d_802c0af0_graphs,d_802c0af0_weighted_states,d_802c0af0_headers,{}},
    {0x802c0bb0,ActorSourceAbi::callback,false,"src/cave/642CC0.c",114,136,"void func_802C0BB0_643060(GObj* obj)",d_802c0bb0_calls,{},d_802c0bb0_callbacks,d_802c0bb0_graphs,d_802c0bb0_weighted_states,d_802c0bb0_headers,{}},
    {0x802c0cd8,ActorSourceAbi::callback,false,"src/cave/642CC0.c",138,147,"void func_802C0CD8_643188(GObj* obj)",d_802c0cd8_calls,{},{},{},{},{},{}},
    {0x802c0d34,ActorSourceAbi::callback,false,"src/cave/642CC0.c",149,170,"void func_802C0D34_6431E4(GObj* obj)",d_802c0d34_calls,{},d_802c0d34_callbacks,{},{},d_802c0d34_headers,{}},
    {0x802c0e28,ActorSourceAbi::callback,false,"src/cave/642CC0.c",172,182,"void func_802C0E28_6432D8(GObj* obj)",d_802c0e28_calls,{},{},{},{},{},{}},
    {0x802c0e80,ActorSourceAbi::callback,false,"src/cave/642CC0.c",184,197,"void func_802C0E80_643330(GObj* obj)",d_802c0e80_calls,{},d_802c0e80_callbacks,{},{},d_802c0e80_headers,{}},
    {0x802c0f30,ActorSourceAbi::callback,false,"src/cave/642CC0.c",199,216,"void func_802C0F30_6433E0(GObj* obj)",d_802c0f30_calls,{},d_802c0f30_callbacks,{},{},d_802c0f30_headers,{}},
    {0x802c1018,ActorSourceAbi::callback,false,"src/cave/642CC0.c",218,236,"void func_802C1018_6434C8(GObj* obj)",d_802c1018_calls,{},{},{},{},{},{}},
    {0x802c10c8,ActorSourceAbi::factory6,false,"src/cave/642CC0.c",238,240,"GObj* func_802C10C8_643578(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)",d_802c10c8_calls,{},{},{},{},{},{}},
    {0x802c1100,ActorSourceAbi::callback,false,"src/cave/642CC0.c",242,244,"void func_802C1100_6435B0(GObj* obj)",d_802c1100_calls,{},d_802c1100_callbacks,{},{},{},{}},
    {0x802c1124,ActorSourceAbi::callback,false,"src/cave/642CC0.c",246,254,"void func_802C1124_6435D4(GObj* obj)",d_802c1124_calls,{},d_802c1124_callbacks,{},{},d_802c1124_headers,{}},
    {0x802c1178,ActorSourceAbi::callback,false,"src/cave/642CC0.c",256,265,"void func_802C1178_643628(GObj* obj)",d_802c1178_calls,{},{},d_802c1178_graphs,d_802c1178_weighted_states,d_802c1178_headers,{}},
    {0x802c11e8,ActorSourceAbi::callback,false,"src/cave/642CC0.c",267,280,"void func_802C11E8_643698(GObj* obj)",d_802c11e8_calls,{},d_802c11e8_callbacks,d_802c11e8_graphs,d_802c11e8_weighted_states,d_802c11e8_headers,{}},
    {0x802c1288,ActorSourceAbi::callback,false,"src/cave/642CC0.c",282,291,"void func_802C1288_643738(GObj* obj)",d_802c1288_calls,{},{},{},{},{},{}},
    {0x802c12e4,ActorSourceAbi::callback,false,"src/cave/642CC0.c",293,302,"void func_802C12E4_643794(GObj* obj)",d_802c12e4_calls,{},d_802c12e4_callbacks,{},{},d_802c12e4_headers,{}},
    {0x802c134c,ActorSourceAbi::callback,false,"src/cave/642CC0.c",304,313,"void func_802C134C_6437FC(GObj* obj)",d_802c134c_calls,{},d_802c134c_callbacks,{},{},d_802c134c_headers,{}},
    {0x802c13b4,ActorSourceAbi::factory6,false,"src/cave/642CC0.c",315,317,"GObj* func_802C13B4_643864(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)",d_802c13b4_calls,{},{},{},{},{},{}},
    {0x802c13ec,ActorSourceAbi::factory6,false,"src/cave/642CC0.c",319,321,"GObj* func_802C13EC_64389C(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)",d_802c13ec_calls,{},{},{},{},{},{}},
    {0x802c1440,ActorSourceAbi::object_utility,false,"src/cave/6438F0.c",61,73,"void func_802C1440_6438F0(GObj* obj, InteractionHandler* arg1)",d_802c1440_calls,{},{},{},{},{},{}},
    {0x802c14c4,ActorSourceAbi::callback,false,"src/cave/6438F0.c",75,93,"void func_802C14C4_643974(GObj* obj)",d_802c14c4_calls,{},d_802c14c4_callbacks,{},{},{},{}},
    {0x802c157c,ActorSourceAbi::callback,false,"src/cave/6438F0.c",95,110,"void func_802C157C_643A2C(GObj* obj)",d_802c157c_calls,{},d_802c157c_callbacks,{},{},d_802c157c_headers,{}},
    {0x802c1660,ActorSourceAbi::callback,false,"src/cave/6438F0.c",112,125,"void func_802C1660_643B10(GObj* obj)",d_802c1660_calls,{},{},d_802c1660_graphs,{},d_802c1660_headers,{}},
    {0x802c1728,ActorSourceAbi::callback,false,"src/cave/6438F0.c",127,136,"void func_802C1728_643BD8(GObj* obj)",d_802c1728_calls,{},{},{},{},{},{}},
    {0x802c1794,ActorSourceAbi::callback,false,"src/cave/6438F0.c",138,146,"void func_802C1794_643C44(GObj* obj)",d_802c1794_calls,d_802c1794_typed,d_802c1794_callbacks,d_802c1794_graphs,{},d_802c1794_headers,{}},
    {0x802c1818,ActorSourceAbi::callback,false,"src/cave/6438F0.c",148,156,"void func_802C1818_643CC8(GObj* obj)",d_802c1818_calls,d_802c1818_typed,d_802c1818_callbacks,d_802c1818_graphs,{},d_802c1818_headers,{}},
    {0x802c189c,ActorSourceAbi::callback,false,"src/cave/6438F0.c",158,166,"void func_802C189C_643D4C(GObj* obj)",d_802c189c_calls,d_802c189c_typed,d_802c189c_callbacks,d_802c189c_graphs,{},d_802c189c_headers,{}},
    {0x802c1920,ActorSourceAbi::callback,false,"src/cave/6438F0.c",168,179,"void func_802C1920_643DD0(GObj* obj)",d_802c1920_calls,{},{},{},{},d_802c1920_headers,{}},
    {0x802c19bc,ActorSourceAbi::callback,false,"src/cave/6438F0.c",181,191,"void func_802C19BC_643E6C(GObj* obj)",d_802c19bc_calls,{},d_802c19bc_callbacks,{},{},{},{}},
    {0x802c1a20,ActorSourceAbi::callback,false,"src/cave/6438F0.c",193,201,"void func_802C1A20_643ED0(GObj* obj)",d_802c1a20_calls,{},d_802c1a20_callbacks,d_802c1a20_graphs,{},d_802c1a20_headers,{}},
    {0x802c1a7c,ActorSourceAbi::callback,false,"src/cave/6438F0.c",203,211,"void func_802C1A7C_643F2C(GObj* obj)",d_802c1a7c_calls,{},{},{},{},{},{}},
    {0x802c1ac8,ActorSourceAbi::callback,false,"src/cave/6438F0.c",213,221,"void func_802C1AC8_643F78(GObj* obj)",d_802c1ac8_calls,{},d_802c1ac8_callbacks,{},{},d_802c1ac8_headers,{}},
    {0x802c1b1c,ActorSourceAbi::callback,false,"src/cave/6438F0.c",223,226,"void func_802C1B1C_643FCC(GObj* obj)",d_802c1b1c_calls,{},d_802c1b1c_callbacks,{},{},{},{}},
    {0x802c1b54,ActorSourceAbi::callback,false,"src/cave/6438F0.c",228,254,"void func_802C1B54_644004(GObj* obj)",d_802c1b54_calls,{},{},{},{},d_802c1b54_headers,{}},
    {0x802c1c48,ActorSourceAbi::callback,false,"src/cave/6438F0.c",256,301,"void func_802C1C48_6440F8(GObj* obj)",d_802c1c48_calls,{},{},{},{},{},{}},
    {0x802c1e4c,ActorSourceAbi::callback,false,"src/cave/6438F0.c",303,308,"void func_802C1E4C_6442FC(GObj* obj)",d_802c1e4c_calls,{},d_802c1e4c_callbacks,{},{},{},{}},
    {0x802c1eb0,ActorSourceAbi::callback,false,"src/cave/6438F0.c",310,317,"void func_802C1EB0_644360(GObj* obj)",d_802c1eb0_calls,{},d_802c1eb0_callbacks,{},{},{},{}},
    {0x802c1f08,ActorSourceAbi::callback,false,"src/cave/6438F0.c",319,328,"void func_802C1F08_6443B8(GObj* obj)",d_802c1f08_calls,{},{},{},{},{},{}},
    {0x802c1f74,ActorSourceAbi::callback,false,"src/cave/6438F0.c",331,347,"void func_802C1F74_644424(GObj* obj)",d_802c1f74_calls,{},{},d_802c1f74_graphs,{},d_802c1f74_headers,{}},
    {0x802c2058,ActorSourceAbi::callback,false,"src/cave/6438F0.c",348,407,"void func_802C2058_644508(GObj* obj)",d_802c2058_calls,{},d_802c2058_callbacks,{},{},{},{}},
    {0x802c2308,ActorSourceAbi::callback,false,"src/cave/6438F0.c",409,441,"void func_802C2308_6447B8(GObj* obj)",d_802c2308_calls,{},{},{},{},{},{}},
    {0x802c2504,ActorSourceAbi::callback,false,"src/cave/6438F0.c",443,454,"void func_802C2504_6449B4(GObj* obj)",d_802c2504_calls,{},d_802c2504_callbacks,{},{},d_802c2504_headers,{}},
    {0x802c257c,ActorSourceAbi::callback,false,"src/cave/6438F0.c",456,465,"void func_802C257C_644A2C(GObj* obj)",d_802c257c_calls,{},d_802c257c_callbacks,{},{},d_802c257c_headers,{}},
    {0x802c25d8,ActorSourceAbi::callback,false,"src/cave/6438F0.c",467,478,"void func_802C25D8_644A88(GObj* obj)",d_802c25d8_calls,d_802c25d8_typed,d_802c25d8_callbacks,{},{},d_802c25d8_headers,{}},
    {0x802c2650,ActorSourceAbi::callback,false,"src/cave/6438F0.c",480,485,"void func_802C2650_644B00(GObj* arg0)",d_802c2650_calls,{},d_802c2650_callbacks,{},{},{},{}},
    {0x802c26ac,ActorSourceAbi::callback,false,"src/cave/6438F0.c",487,492,"void func_802C26AC_644B5C(GObj* obj)",d_802c26ac_calls,{},d_802c26ac_callbacks,{},{},{},{}},
    {0x802c2710,ActorSourceAbi::callback,false,"src/cave/6438F0.c",494,500,"void func_802C2710_644BC0(GObj* obj)",d_802c2710_calls,{},d_802c2710_callbacks,{},{},{},{}},
    {0x802c2758,ActorSourceAbi::callback,false,"src/cave/6438F0.c",502,511,"void func_802C2758_644C08(GObj* obj)",d_802c2758_calls,{},{},d_802c2758_graphs,{},d_802c2758_headers,{}},
    {0x802c27e4,ActorSourceAbi::callback,false,"src/cave/6438F0.c",513,522,"void func_802C27E4_644C94(GObj* obj)",d_802c27e4_calls,{},{},{},{},{},{}},
    {0x802c2854,ActorSourceAbi::callback,false,"src/cave/6438F0.c",524,531,"void func_802C2854_644D04(GObj* obj)",d_802c2854_calls,d_802c2854_typed,d_802c2854_callbacks,d_802c2854_graphs,{},d_802c2854_headers,{}},
    {0x802c28d0,ActorSourceAbi::callback,false,"src/cave/6438F0.c",533,540,"void func_802C28D0_644D80(GObj* obj)",d_802c28d0_calls,d_802c28d0_typed,d_802c28d0_callbacks,d_802c28d0_graphs,{},d_802c28d0_headers,{}},
    {0x802c294c,ActorSourceAbi::callback,false,"src/cave/6438F0.c",542,546,"void func_802C294C_644DFC(GObj* obj)",d_802c294c_calls,d_802c294c_typed,d_802c294c_callbacks,d_802c294c_graphs,{},d_802c294c_headers,{}},
    {0x802c2994,ActorSourceAbi::factory6,false,"src/cave/6438F0.c",548,550,"GObj* func_802C2994_644E44(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)",d_802c2994_calls,{},{},{},{},{},{}},
    {0x802c29d0,ActorSourceAbi::callback,false,"src/cave/644E80.c",29,51,"void func_802C29D0_644E80(GObj* obj)",d_802c29d0_calls,{},d_802c29d0_callbacks,{},{},{},{}},
    {0x802c2ac4,ActorSourceAbi::callback,false,"src/cave/644E80.c",53,58,"void func_802C2AC4_644F74(GObj* obj)",d_802c2ac4_calls,{},d_802c2ac4_callbacks,{},{},{},{}},
    {0x802c2b0c,ActorSourceAbi::callback,false,"src/cave/644E80.c",60,65,"void func_802C2B0C_644FBC(GObj* obj)",d_802c2b0c_calls,{},d_802c2b0c_callbacks,{},{},{},{}},
    {0x802c2b54,ActorSourceAbi::callback,false,"src/cave/644E80.c",67,72,"void func_802C2B54_645004(GObj* obj)",d_802c2b54_calls,{},d_802c2b54_callbacks,{},{},{},{}},
    {0x802c2b9c,ActorSourceAbi::callback,false,"src/cave/644E80.c",74,80,"void func_802C2B9C_64504C(GObj* obj)",d_802c2b9c_calls,{},{},d_802c2b9c_graphs,{},{},{}},
    {0x802c2bdc,ActorSourceAbi::callback,false,"src/cave/644E80.c",82,98,"void func_802C2BDC_64508C(GObj* obj)",d_802c2bdc_calls,{},d_802c2bdc_callbacks,d_802c2bdc_graphs,{},d_802c2bdc_headers,{}},
    {0x802c2c90,ActorSourceAbi::callback,false,"src/cave/644E80.c",100,109,"void func_802C2C90_645140(GObj* obj)",d_802c2c90_calls,{},{},{},{},{},{}},
    {0x802c2d00,ActorSourceAbi::callback,false,"src/cave/644E80.c",111,120,"void func_802C2D00_6451B0(GObj* obj)",d_802c2d00_calls,{},{},{},{},{},{}},
    {0x802c2d6c,ActorSourceAbi::callback,false,"src/cave/644E80.c",122,125,"void func_802C2D6C_64521C(GObj* obj)",d_802c2d6c_calls,{},{},{},{},{},{}},
    {0x802c2d98,ActorSourceAbi::callback,false,"src/cave/644E80.c",127,142,"void func_802C2D98_645248(GObj* obj)",d_802c2d98_calls,{},{},{},{},{},{}},
    {0x802c2e24,ActorSourceAbi::callback,false,"src/cave/644E80.c",144,153,"void func_802C2E24_6452D4(GObj* obj)",d_802c2e24_calls,{},d_802c2e24_callbacks,d_802c2e24_graphs,{},{},{}},
    {0x802c2ebc,ActorSourceAbi::callback,false,"src/cave/644E80.c",155,168,"void func_802C2EBC_64536C(GObj* obj)",d_802c2ebc_calls,{},d_802c2ebc_callbacks,{},{},d_802c2ebc_headers,{}},
    {0x802c2f48,ActorSourceAbi::callback,false,"src/cave/644E80.c",170,179,"void func_802C2F48_6453F8(GObj* obj)",d_802c2f48_calls,{},{},{},{},{},{}},
    {0x802c2fb4,ActorSourceAbi::callback,false,"src/cave/644E80.c",181,187,"void func_802C2FB4_645464(GObj* obj)",d_802c2fb4_calls,{},{},d_802c2fb4_graphs,{},{},{}},
    {0x802c2ff4,ActorSourceAbi::callback,false,"src/cave/644E80.c",190,203,"void func_802C2FF4_6454A4(GObj* obj)",d_802c2ff4_calls,{},d_802c2ff4_callbacks,d_802c2ff4_graphs,{},d_802c2ff4_headers,{}},
    {0x802c30b8,ActorSourceAbi::callback,false,"src/cave/644E80.c",205,229,"void func_802C30B8_645568(GObj* obj)",d_802c30b8_calls,{},{},{},{},{},{}},
    {0x802c31d0,ActorSourceAbi::callback,false,"src/cave/644E80.c",232,243,"void func_802C31D0_645680(GObj* obj)",d_802c31d0_calls,{},{},{},{},{},{}},
    {0x802c3250,ActorSourceAbi::factory6,false,"src/cave/644E80.c",245,247,"GObj* func_802C3250_645700(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)",d_802c3250_calls,{},{},{},{},{},{}},
    {0x802c3290,ActorSourceAbi::callback,false,"src/cave/645740.c",10,19,"POKEMON_FUNC(func_802C3290_645740)",d_802c3290_calls,{},d_802c3290_callbacks,{},{},d_802c3290_headers,{}},
    {0x802c3348,ActorSourceAbi::callback,false,"src/cave/645740.c",21,26,"POKEMON_FUNC(func_802C3348_6457F8)",d_802c3348_calls,{},{},{},{},{},{}},
    {0x802c3388,ActorSourceAbi::factory6,false,"src/cave/645740.c",28,30,"GObj* func_802C3388_645838(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)",d_802c3388_calls,{},{},{},{},{},{}},
    {0x802c33c0,ActorSourceAbi::callback,false,"src/cave/645870.c",21,23,"void func_802C33C0_645870(GObj* arg0)",d_802c33c0_calls,{},d_802c33c0_callbacks,{},{},{},{}},
    {0x802c33e4,ActorSourceAbi::callback,false,"src/cave/645870.c",25,39,"POKEMON_FUNC(func_802C33E4_645894)",d_802c33e4_calls,{},d_802c33e4_callbacks,{},{},d_802c33e4_headers,{}},
    {0x802c3494,ActorSourceAbi::callback,false,"src/cave/645870.c",41,46,"POKEMON_FUNC(func_802C3494_645944)",d_802c3494_calls,{},{},{},{},{},{}},
    {0x802c34e0,ActorSourceAbi::callback,false,"src/cave/645870.c",48,58,"POKEMON_FUNC(func_802C34E0_645990)",d_802c34e0_calls,{},d_802c34e0_callbacks,{},{},d_802c34e0_headers,{}},
    {0x802c3580,ActorSourceAbi::callback,false,"src/cave/645870.c",60,75,"POKEMON_FUNC(func_802C3580_645A30)",d_802c3580_calls,{},{},{},{},{},{}},
    {0x802c3658,ActorSourceAbi::callback,false,"src/cave/645870.c",77,85,"POKEMON_FUNC(func_802C3658_645B08)",d_802c3658_calls,{},d_802c3658_callbacks,{},{},{},{}},
    {0x802c36f0,ActorSourceAbi::callback,false,"src/cave/645870.c",87,94,"POKEMON_FUNC(func_802C36F0_645BA0)",d_802c36f0_calls,{},{},d_802c36f0_graphs,{},d_802c36f0_headers,{}},
    {0x802c3784,ActorSourceAbi::callback,false,"src/cave/645870.c",96,107,"POKEMON_FUNC(func_802C3784_645C34)",d_802c3784_calls,{},{},{},{},{},{}},
    {0x802c3840,ActorSourceAbi::callback,false,"src/cave/645870.c",109,112,"void func_802C3840_645CF0(GObj* obj)",d_802c3840_calls,{},{},{},{},{},{}},
    {0x802c3874,ActorSourceAbi::callback,false,"src/cave/645870.c",114,120,"POKEMON_FUNC(func_802C3874_645D24)",d_802c3874_calls,{},d_802c3874_callbacks,{},{},d_802c3874_headers,{}},
    {0x802c38e0,ActorSourceAbi::callback,false,"src/cave/645D90.c",34,54,"void func_802C38E0_645D90(GObj* obj)",d_802c38e0_calls,{},{},{},{},{},{}},
    {0x802c39b8,ActorSourceAbi::callback,false,"src/cave/645D90.c",56,68,"void func_802C39B8_645E68(GObj* obj)",d_802c39b8_calls,{},d_802c39b8_callbacks,{},{},d_802c39b8_headers,{}},
    {0x802c3a44,ActorSourceAbi::callback,false,"src/cave/645D90.c",70,95,"void func_802C3A44_645EF4(GObj* obj)",d_802c3a44_calls,{},{},{},{},{},{}},
    {0x802c3b34,ActorSourceAbi::callback,false,"src/cave/645D90.c",97,100,"void func_802C3B34_645FE4(GObj* obj)",d_802c3b34_calls,{},{},{},{},{},{}},
    {0x802c3b68,ActorSourceAbi::callback,false,"src/cave/645D90.c",103,123,"void func_802C3B68_646018(GObj* obj)",d_802c3b68_calls,{},d_802c3b68_callbacks,{},{},d_802c3b68_headers,{}},
    {0x802c3c40,ActorSourceAbi::callback,false,"src/cave/645D90.c",125,149,"void func_802C3C40_6460F0(GObj* obj)",d_802c3c40_calls,{},{},{},{},{},{}},
    {0x802c3d44,ActorSourceAbi::callback,false,"src/cave/645D90.c",151,162,"void func_802C3D44_6461F4(GObj* obj)",d_802c3d44_calls,{},d_802c3d44_callbacks,d_802c3d44_graphs,{},d_802c3d44_headers,{}},
    {0x802c3dfc,ActorSourceAbi::callback,false,"src/cave/645D90.c",164,171,"void func_802C3DFC_6462AC(GObj* obj)",d_802c3dfc_calls,{},{},{},{},{},{}},
    {0x802c3e74,ActorSourceAbi::callback,false,"src/cave/645D90.c",173,182,"void func_802C3E74_646324(GObj* obj)",d_802c3e74_calls,{},d_802c3e74_callbacks,{},{},d_802c3e74_headers,{}},
    {0x802c3edc,ActorSourceAbi::callback,false,"src/cave/645D90.c",184,196,"void func_802C3EDC_64638C(GObj* obj)",d_802c3edc_calls,{},d_802c3edc_callbacks,{},{},d_802c3edc_headers,{}},
    {0x802c3f6c,ActorSourceAbi::callback,false,"src/cave/645D90.c",198,212,"void func_802C3F6C_64641C(GObj* obj)",d_802c3f6c_calls,{},{},{},{},{},{}},
    {0x802c3fd8,ActorSourceAbi::callback,false,"src/cave/645D90.c",214,224,"void func_802C3FD8_646488(GObj* obj)",d_802c3fd8_calls,{},d_802c3fd8_callbacks,d_802c3fd8_graphs,{},d_802c3fd8_headers,{}},
    {0x802c4090,ActorSourceAbi::callback,false,"src/cave/645D90.c",226,241,"void func_802C4090_646540(GObj* obj)",d_802c4090_calls,{},{},{},{},{},{}},
    {0x802c413c,ActorSourceAbi::callback,false,"src/cave/645D90.c",243,255,"void func_802C413C_6465EC(GObj* obj)",d_802c413c_calls,{},d_802c413c_callbacks,{},{},d_802c413c_headers,{}},
    {0x802c41d0,ActorSourceAbi::callback,false,"src/cave/645D90.c",257,270,"void func_802C41D0_646680(GObj* obj)",d_802c41d0_calls,{},d_802c41d0_callbacks,{},{},d_802c41d0_headers,{}},
    {0x802c4264,ActorSourceAbi::callback,false,"src/cave/645D90.c",272,287,"POKEMON_FUNC(func_802C4264_646714)",d_802c4264_calls,{},{},{},{},{},{}},
    {0x802c433c,ActorSourceAbi::callback,false,"src/cave/645D90.c",289,293,"void func_802C433C_6467EC(GObj* obj)",d_802c433c_calls,{},{},{},{},{},{}},
    {0x802c4378,ActorSourceAbi::callback,false,"src/cave/645D90.c",295,341,"POKEMON_FUNC(func_802C4378_646828)",d_802c4378_calls,{},{},{},{},{},{}},
    {0x802c45ec,ActorSourceAbi::utility,false,"src/cave/645D90.c",343,344,"void func_802C45EC_646A9C(void)",{},{},{},{},{},{},{}},
    {0x802c45f4,ActorSourceAbi::callback,false,"src/cave/645D90.c",346,355,"void func_802C45F4_646AA4(GObj* obj)",d_802c45f4_calls,{},{},{},{},{},{}},
    {0x802c465c,ActorSourceAbi::factory6,false,"src/cave/645D90.c",357,359,"GObj* func_802C465C_646B0C(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)",d_802c465c_calls,{},{},{},{},{},{}},
    {0x802c46a0,ActorSourceAbi::callback,false,"src/cave/646B50.c",17,24,"void func_802C46A0_646B50(GObj* obj)",d_802c46a0_calls,{},{},d_802c46a0_graphs,{},{},{}},
    {0x802c46f0,ActorSourceAbi::callback,false,"src/cave/646B50.c",26,52,"void func_802C46F0_646BA0(GObj* obj)",d_802c46f0_calls,{},d_802c46f0_callbacks,{},{},d_802c46f0_headers,{}},
    {0x802c4820,ActorSourceAbi::callback,false,"src/cave/646B50.c",54,80,"void func_802C4820_646CD0(GObj* obj)",d_802c4820_calls,{},{},{},{},{},{}},
    {0x802c493c,ActorSourceAbi::callback,false,"src/cave/646B50.c",82,91,"void func_802C493C_646DEC(GObj* obj)",d_802c493c_calls,{},{},{},{},{},{}},
    {0x802c49a8,ActorSourceAbi::factory5,false,"src/cave/646B50.c",93,97,"GObj* func_802C49A8_646E58(s32 gObjID, u16 id, WorldBlock* roomA, WorldBlock* roomB, ObjectSpawn* spawn)",d_802c49a8_calls,{},{},{},{},{},{}},
    {0x802c49f0,ActorSourceAbi::callback,false,"src/cave/646EA0.c",22,30,"void func_802C49F0_646EA0(GObj* obj)",d_802c49f0_calls,{},d_802c49f0_callbacks,d_802c49f0_graphs,{},d_802c49f0_headers,{}},
    {0x802c4a4c,ActorSourceAbi::callback,false,"src/cave/646EA0.c",32,40,"void func_802C4A4C_646EFC(GObj* obj)",d_802c4a4c_calls,{},{},d_802c4a4c_graphs,{},d_802c4a4c_headers,{}},
    {0x802c4aa8,ActorSourceAbi::callback,false,"src/cave/646EA0.c",42,50,"void func_802C4AA8_646F58(GObj* obj)",d_802c4aa8_calls,{},d_802c4aa8_callbacks,d_802c4aa8_graphs,{},d_802c4aa8_headers,{}},
    {0x802c4b04,ActorSourceAbi::callback,false,"src/cave/646EA0.c",53,77,"void func_802C4B04_646FB4(GObj* obj)",d_802c4b04_calls,{},d_802c4b04_callbacks,{},{},d_802c4b04_headers,{}},
    {0x802c4c18,ActorSourceAbi::callback,false,"src/cave/646EA0.c",79,87,"void func_802C4C18_6470C8(GObj* obj)",d_802c4c18_calls,d_802c4c18_common_egg_typed,{},{},{},{},{}},
    {0x802c4c70,ActorSourceAbi::callback,false,"src/cave/646EA0.c",90,119,"void func_802C4C70_647120(GObj* obj)",d_802c4c70_calls,{},{},{},{},d_802c4c70_headers,{}},
    {0x802c4d60,ActorSourceAbi::callback,false,"src/cave/646EA0.c",121,144,"POKEMON_FUNC(func_802C4D60_647210)",d_802c4d60_calls,{},{},{},{},{},{}},
    {0x802c4e64,ActorSourceAbi::factory5,false,"src/cave/646EA0.c",146,150,"GObj* func_802C4E64_647314(s32 gObjID, u16 id, WorldBlock* roomA, WorldBlock* roomB, ObjectSpawn* spawn)",d_802c4e64_calls,{},{},{},{},{},{}},
    {0x802c4eb0,ActorSourceAbi::model_utility,false,"src/cave/647360.c",58,63,"void func_802C4EB0_647360(PokemonTransform* transform, f32 arg1)",d_802c4eb0_calls,{},{},{},{},{},{}},
    {0x802c4ef4,ActorSourceAbi::callback,false,"src/cave/647360.c",65,86,"void func_802C4EF4_6473A4(GObj* obj)",d_802c4ef4_calls,d_802c4ef4_typed,d_802c4ef4_callbacks,{},{},{},{}},
    {0x802c4f5c,ActorSourceAbi::callback,false,"src/cave/647360.c",88,96,"void func_802C4F5C_64740C(GObj* obj)",d_802c4f5c_calls,{},{},d_802c4f5c_graphs,{},d_802c4f5c_headers,{}},
    {0x802c4fb8,ActorSourceAbi::callback,false,"src/cave/647360.c",98,107,"void func_802C4FB8_647468(GObj* obj)",d_802c4fb8_calls,{},d_802c4fb8_callbacks,d_802c4fb8_graphs,{},d_802c4fb8_headers,{}},
    {0x802c502c,ActorSourceAbi::callback,false,"src/cave/647360.c",109,135,"void func_802C502C_6474DC(GObj* obj)",d_802c502c_calls,d_802c502c_typed,{},{},{},{},{}},
    {0x802c5188,ActorSourceAbi::callback,false,"src/cave/647360.c",138,152,"void func_802C5188_647638(GObj* obj)",d_802c5188_calls,{},d_802c5188_callbacks,d_802c5188_graphs,{},d_802c5188_headers,{}},
    {0x802c5230,ActorSourceAbi::callback,false,"src/cave/647360.c",154,180,"void func_802C5230_6476E0(GObj* obj)",d_802c5230_calls,d_802c5230_typed,{},{},{},{},{}},
    {0x802c5380,ActorSourceAbi::callback,false,"src/cave/647360.c",182,191,"void func_802C5380_647830(GObj* obj)",d_802c5380_calls,{},d_802c5380_callbacks,d_802c5380_graphs,{},d_802c5380_headers,{}},
    {0x802c53f4,ActorSourceAbi::callback,false,"src/cave/647360.c",193,202,"void func_802C53F4_6478A4(GObj* obj)",d_802c53f4_calls,{},d_802c53f4_callbacks,d_802c53f4_graphs,{},d_802c53f4_headers,{}},
    {0x802c5468,ActorSourceAbi::callback,false,"src/cave/647360.c",204,213,"void func_802C5468_647918(GObj* obj)",d_802c5468_calls,{},d_802c5468_callbacks,d_802c5468_graphs,{},d_802c5468_headers,{}},
    {0x802c54dc,ActorSourceAbi::callback,false,"src/cave/647360.c",215,229,"void func_802C54DC_64798C(GObj* obj)",d_802c54dc_calls,{},{},{},{},{},{}},
    {0x802c5548,ActorSourceAbi::factory6,false,"src/cave/647360.c",231,233,"GObj* func_802C5548_6479F8(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)",d_802c5548_calls,{},{},{},{},{},{}},
    {0x802c5580,ActorSourceAbi::callback,false,"src/cave/647360.c",235,243,"void func_802C5580_647A30(GObj* obj)",d_802c5580_calls,{},{},{},{},d_802c5580_headers,{}},
    {0x802c55d4,ActorSourceAbi::void_factory5,false,"src/cave/647360.c",245,247,"void func_802C55D4_647A84(s32 gObjID, u16 id, WorldBlock* roomA, WorldBlock* roomB, ObjectSpawn* spawn)",d_802c55d4_calls,{},{},{},{},{},{}},
    {0x802c5620,ActorSourceAbi::callback,false,"src/cave/647360.c",249,251,"void func_802C5620_647AD0(GObj* obj)",d_802c5620_calls,{},d_802c5620_callbacks,{},{},{},{}},
    {0x802c5644,ActorSourceAbi::callback,false,"src/cave/647360.c",253,261,"void func_802C5644_647AF4(GObj* obj)",d_802c5644_calls,{},{},d_802c5644_graphs,{},d_802c5644_headers,{}},
    {0x802c569c,ActorSourceAbi::callback,false,"src/cave/647360.c",263,281,"void func_802C569C_647B4C(GObj* obj)",d_802c569c_calls,{},d_802c569c_callbacks,{},{},d_802c569c_headers,{}},
    {0x802c5778,ActorSourceAbi::animation,false,"src/cave/647360.c",283,287,"void func_802C5778_647C28(DObj* arg0, s32 arg1, f32 arg2)",{},{},{},{},{},{},{}},
    {0x802c57a4,ActorSourceAbi::callback,false,"src/cave/647360.c",289,308,"void func_802C57A4_647C54(GObj* obj)",d_802c57a4_calls,{},{},{},{},{},{}},
    {0x802c58e4,ActorSourceAbi::callback,false,"src/cave/647360.c",310,322,"void func_802C58E4_647D94(GObj* obj)",d_802c58e4_calls,{},{},{},{},{},{}},
    {0x802c5970,ActorSourceAbi::callback,false,"src/cave/647360.c",324,365,"void func_802C5970_647E20(GObj* obj)",d_802c5970_calls,d_802c5970_typed,d_802c5970_callbacks,{},{},{},{}},
    {0x802c5b28,ActorSourceAbi::factory6,false,"src/cave/647360.c",367,369,"GObj* func_802C5B28_647FD8(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)",d_802c5b28_calls,{},{},{},{},{},{}},
    {0x802c5b60,ActorSourceAbi::callback,true,"src/cave/647360.c",371,375,"void func_802C5B60_648010(GObj* arg0)",d_802c5b60_calls,{},{},{},{},{},{}},
    {0x802c5b90,ActorSourceAbi::utility,true,"src/cave/647360.c",377,379,"void func_802C5B90_648040(void)",d_802c5b90_calls,{},{},{},{},{},{}},
    {0x802c5bb0,ActorSourceAbi::animation,true,"src/cave/647360.c",381,397,"void func_802C5BB0_648060(DObj* dobj, s32 arg1, f32 arg2)",d_802c5bb0_calls,{},{},{},{},{},{}},
    {0x802c5c24,ActorSourceAbi::utility,true,"src/cave/647360.c",399,413,"void func_802C5C24_6480D4(void)",d_802c5c24_calls,d_802c5c24_typed,{},{},{},{},{}},
    {0x802c5cec,ActorSourceAbi::callback,true,"src/cave/647360.c",415,425,"void func_802C5CEC_64819C(GObj* obj)",d_802c5cec_calls,{},{},{},{},{},{}},
    {0x802c5d80,ActorSourceAbi::callback,true,"src/cave/647360.c",427,437,"void func_802C5D80_648230(GObj* obj)",d_802c5d80_calls,{},{},{},{},{},{}},
    {0x802c5dfc,ActorSourceAbi::callback,true,"src/cave/647360.c",439,503,"void func_802C5DFC_6482AC(GObj* obj)",d_802c5dfc_calls,d_802c5dfc_typed,d_802c5dfc_callbacks,{},{},{},{}},
    {0x802c60f4,ActorSourceAbi::utility,true,"src/cave/647360.c",505,514,"void func_802C60F4_6485A4(void)",d_802c60f4_calls,{},d_802c60f4_callbacks,{},{},{},{}},
    {0x8035e1d4,ActorSourceAbi::object_utility,false,"src/app_level/4FE330.c",143,154,"GObj* func_8035E1D4_4FE5E4(GObj* obj)",shared_8035e1d4_calls,{},{},{},{},{},shared_8035e1d4_child_factories},
};
constexpr std::uint32_t graphs[] = {0x802c65e8,0x802c6678,0x802c66f8,0x802c6728,0x802c6778,0x802c67a8,0x802c6924,0x802c697c,0x802c6aa8,0x802c6ad8,0x802c6b08,0x802c6be0,0x802c6c10,0x802c6c30,0x802c6d60,0x802c6d90,0x802c6db0,0x802c6e10,0x802c6e60,0x802c6eb0,0x802c6fd4,0x802c70f0,0x802c72b0,0x802c7320,0x802c7360,0x802c73a0,0x802c73e0,0x802c741c,0x802c748c,0x802c74ec,0x802c751c,0x802c754c,0x802c7614,0x802c7634,0x802c7674,0x802c7694,0x802c76d4,0x802c7984,0x802c79d0,0x802c7a50,0x802c7b50,0x802c7c40,0x802c7d94,0x802c7ddc,0x802c7e1c,0x802c7e4c,0x802c7e7c,0x802c7fc8};
constexpr std::uint32_t weighted[] = {0x802c67e8,0x802c6964,0x802c6b28,0x802c7014,0x802c7120};
constexpr std::uint32_t roots[] = {0x802bdd00,0x802be898,0x802bf020,0x802bf5a0,0x802bfde0,0x802c0220,0x802c0888,0x802c1100,0x802c1124,0x802c14c4,0x802c29d0,0x802c3290,0x802c33c0,0x802c46a0,0x802c49f0,0x802c4ef4,0x802c5580,0x802c5620,0x8035df20};
const ActorAnimationBinding animation_callbacks[] = {{0x802c5778,func_802C5778_647C28}};
const LifecycleAnimationBinding lifecycle_animation_bindings[] = {{0x802c5bb0,func_802C5BB0_648060}};
const ActorEggHelperBinding egg_helpers[] = {{0x80362414,isle_egg_original_func_80362414_502824}};
const ActorObjectHelperBinding object_helpers[] = {{0x8035e1d4,isle_cave_actor_original_func_8035E1D4_4FE5E4}};
const ActorItemBinding item_callbacks[] = {{0x802bddcc,func_802BDDCC_64027C}};
const ActorMovementBinding movement_callbacks[] = {{0x802be450,func_802BE450_640900}};
const ActorGraphHelperBinding graph_helpers[] = {{0x802c0140,func_802C0140_6425F0},{0x802c1440,func_802C1440_6438F0}};
const ActorTransformBinding transform_helpers[] = {{0x802c4eb0,func_802C4EB0_647360}};
const ActorUtilityBinding utilities[] = {{0x802bf554,func_802BF554_641A04},{0x802c45ec,func_802C45EC_646A9C}};
} // namespace
std::span<const actor::BehaviorCallbackBinding> actor_callbacks() noexcept { return callback_bindings; }
std::span<const actor::BehaviorCallbackBinding> lifecycle_callbacks() noexcept { return lifecycle_callback_bindings; }
std::span<const std::uint32_t> actor_graphs() noexcept { return graphs; }
std::span<const std::uint32_t> actor_weighted_states() noexcept { return weighted; }
std::span<const std::uint32_t> actor_roots() noexcept { return roots; }
std::span<const ActorFactoryBinding> actor_factories() noexcept { return factory_bindings; }
std::span<const ActorChildFactoryDependency> actor_child_factories() noexcept { return child_factories; }
std::span<const ActorProviderDependency> actor_dependencies() noexcept { return provider_dependencies; }
std::span<const ActorHeaderBinding> actor_headers() noexcept { return header_bindings; }
std::span<const ActorWeightedBinding> actor_weighted_bindings() noexcept { return weighted_bindings; }
std::span<const ActorCountedOptionsBinding> actor_counted_options() noexcept { return counted_options; }
std::span<const ActorAutomaticAggregate> actor_automatic_aggregates() noexcept { return automatic_aggregates; }
std::span<const ActorAnimationBinding> actor_animation_callbacks() noexcept { return animation_callbacks; }
std::span<const ActorItemBinding> actor_item_callbacks() noexcept { return item_callbacks; }
std::span<const ActorMovementBinding> actor_movement_callbacks() noexcept { return movement_callbacks; }
std::span<const ActorGraphHelperBinding> actor_graph_helpers() noexcept { return graph_helpers; }
std::span<const ActorTransformBinding> actor_transform_helpers() noexcept { return transform_helpers; }
std::span<const ActorUtilityBinding> actor_utilities() noexcept { return utilities; }
std::span<const LifecycleAnimationBinding> lifecycle_animation_callbacks() noexcept { return lifecycle_animation_bindings; }
std::span<const ActorEggHelperBinding> actor_egg_helpers() noexcept { return egg_helpers; }
std::span<const ActorObjectHelperBinding> actor_object_helpers() noexcept { return object_helpers; }
} // namespace isle::cave

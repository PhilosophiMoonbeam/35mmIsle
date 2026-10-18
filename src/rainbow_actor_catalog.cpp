#include "actor_resource_catalog.hpp"

namespace isle::actor::detail {
namespace {
// Own GAME pin df8815e75f3d79618466e01e0827c6165b93d11c, never a dirty worktree:
// rainbow/main.c blob bee9dab9845cd7ceb6d55c08d623b82ba7a775cf:19-41,
// 49-159: five table families and three genuine automatic local definitions.
// rainbow/mew/code.c blob 40455edb782421b6add1b1107a61586854989633:
// 56-118,633-659: eight headers and actual initialization/setup/ground factory.
// rainbow/mew/model.c blob 8da0ffa9a1388dac1dddb19e56c016190b182b9e:
// 31-75,77-124,247-418: material allocation and 33-node initialized tree.
// rainbow/pkmn_1037/code.c blob 03f323a788a58896889f78aed3e1efaa207711f7:
// 16-44,322-324; gate/code.c blob ee5d0b94feada6dcafa0df3a8b07c36baa5b10d1:
// 17-28,171-173,193-224; evolution_controller/code.c blob
// 603722c803dea37983a783ca5200f8a8c2d96bca:3-5,33-65; pkmn_1038/code.c
// blob b0519e04b178d5ed8b2096713045a6948e342bc6:3-4,63-65. Extern data
// values below come from the admitted ROM, not invented C initializers.
//
// ROM SHA256 a1d5d816db7f8557ee04c35a011326d058b2c1fbca76b57b352b1d705a1ec1cc.
// Pinned splat.yaml blob d2690bd3d2cb0aeac5c648db20666c5a42e5ba45:
// initialized boundaries 5219-5247,9827-9865; world data shares the unchanged
// course_resources.cpp mapping. Source pointers bind exact typed ROM records:
// init/setup ROM pairs: 82A600/82A5EC,82ACC4/82ACB0,82A6E8/82A6D4,
//                      82AD28/82AD14,82ADA8/82AD94 (52/20 bytes respectively).
// Initialization SHA256 in that order:
// d556dc28bd5b73856cf84807dd6f21938241baa80524224975ec490cfc82342c
// ce77e724e081828eaeb53a27f5b09e218c0a75e0bbaefc88778f4612b3e41d98
// 4a60d2df112dd445da0a44409e15d5fdc242ef7d21534a44191ceb13ffc8d386
// 0070f52f56c45ced53b993daf6594eba8641f08f4f1c904d5aff7518f5326f90
// cd004100f8cf0ab37017ed6697ab4d98f2151bceee93c9d7222f902cbb6fe7bf
constexpr std::array<InitializationSpec,5> initializations{{
    {151,0x8034ae90,0x8034fb98,0x8034b740,0x8035942c,0x8034ae7c,0x803470cc,0x8034acb0,33,0,8,0,0,0},
    {1037,0x8034b554,0x80116ef0,0x80116cf0,0x800a1608,0x8034b540,0x80349180,0x8034b098,4,8,12,0,0,0},
    {1001,0x8034af78,0x800f1d10,0x800f07a0,0x800a1608,0x8034af64,0x80348970,0x8034aed0,4,20,4,0,0,0},
    {1002,0x8034b5b8,0x800f5490,0x800f41d0,0x80359534,0x8034b5a4,0x80349f00,0x8034b590,7,24,1,0,0,0},
    {1038,0x8034b638,0x80118210,0x80118160,0x800a1608,0x8034b624,0x8034a190,0x8034b600,2,25,1,0,0,0},
}};
// Include all twelve initialized 1037 header records, not merely its ten
// referenced extern declarations: B0D4/B14C are real 20-byte records at ROM
// 82A844/82A8BC. The whole B098..B188 typed range is ROM82A808..82A8F8,
// SHA256 f5590d197ebe7631d38c17c4729ccb74317aff2ed062e60e2dd0c79bfa5f306d.
// Other header ranges: ROM82A420..82A4C0,82A640..82A690,82AD00..82AD14,
// 82AD70..82AD84. All model/material table pointers are decoded from these.
constexpr std::array<std::uint32_t,26> headers{
    0x8034acb0,0x8034acc4,0x8034acd8,0x8034acec,
    0x8034ad00,0x8034ad14,0x8034ad28,0x8034ad3c,
    0x8034b098,0x8034b0ac,0x8034b0c0,0x8034b0d4,
    0x8034b0e8,0x8034b0fc,0x8034b110,0x8034b124,
    0x8034b138,0x8034b14c,0x8034b160,0x8034b174,
    0x8034aed0,0x8034aee4,0x8034aef8,0x8034af0c,
    0x8034b590,0x8034b600,
};
// World gate allocations: world/gate/materials.c blob
// 79538be4866a51a0c6b0e4bd3e42e90a879d8d77:10-25; controller allocations:
// world/evolution_controller/materials.c blob
// 355bccb14c85d52cba183f18c3938a0318197b3e:12-26. Mew's six+NULL image
// allocation is explicitly defined at rainbow/mew/model.c:67-75.
// The initialized assets binary's remaining pointer ranges are delimited by
// the next exact typed table/material identity, not a mapping-size or NULL-scan
// guess: 1037 tables [116D00,116D14),[116D14,116D2C),[116D2C,116D38),
// with the last slot NULL; 1038 [118168,118180), six actual image pointers.
// ROM4C90D0..4C9108 SHA256
// 75c733aa3b1b86f5f43b6e3deb34d91323b8bb81d43c1e8423eea7fcf18321dd;
// ROM4CA538..4CA550 SHA256
// 4c329cb38e834bf12b36062cd8e8a9f605b6bb761afe182860f361d8b620dae8.
// No palette pointer tables are present in these fourteen Texture records.
constexpr std::array<model_assets::MaterialExtent,14> material_extents{{
    {0x800f07b8,0x800f07b0,1,1},
    {0x800f0830,0x800f07b4,1,1},
    {0x800f08a8,0,0,0},
    {0x800f41f0,0x800f41ec,1,1},
    {0x800f4268,0,0,0},
    {0x800f42e0,0,0,0},
    {0x800f4358,0,0,0},
    {0x800f43d0,0,0,0},
    {0x800f4448,0,0,0},
    {0x80116d38,0x80116d00,5,5},
    {0x80116db0,0x80116d14,6,6},
    {0x80116e28,0x80116d2c,2,3},
    {0x80118180,0x80118168,6,6},
    {0x8034b7e0,0x8034b7c4,6,7},
}};
// Select only Rainbow and shared initialized World data. Overlay VRAM ranges
// are never merged with Beach or another course, and no code/BSS is readable.
// In particular, World ROM[64280,731B0) maps to [800E6AD0,800F5A00)
// under pinned splat.yaml:420-445,520-533. The controller modelanim allocation
// is ROM[72DA0,73070), not a reason to extend any initialized mapping.
constexpr std::array<model_assets::Mapping,4> mappings{{
    {0x800e6ad0,0x64280,0x731b0-0x64280},
    {0x800f5da0,0x4a8170,0x4ec000-0x4a8170},
    {0x80139c50,0x4ec000,0x4f0610-0x4ec000},
    {0x8034ab10,0x82a280,0x82f8e0-0x82a280},
}};
// Five live table rows plus two real initialized singletons. The zero row is
// validated separately, not exposed as a Rainbow executable definition.
// Table ROM82A2A4..82A304; singleton ROM82A71C/82AD5C, each 16 bytes.
// Helpers copy these exact singletons at gate/code.c:199 and
// evolution_controller/code.c:43; neither is a 1037 singleton.
constexpr std::array<DefinitionSpec,7> definitions{{
    {0x8034ab34,151,0x80348938,0x80363dbc,0x80364280,0,false},
    {0x8034ab44,1037,0x80349eb8,0x80363dbc,0x80364280,1,false},
    {0x8034ab54,1001,0x80348fb8,0x80363dbc,0x80364280,2,false},
    {0x8034ab64,1002,0x8034a064,0x80363dbc,0x80364280,3,false},
    {0x8034ab74,1038,0x8034a4e8,0x80363dbc,0x80364280,4,false},
    {0x8034afac,1001,0x80348fb8,0x80363dbc,0x80364280,2,true},
    {0x8034b5ec,1002,0x8034a064,0x80363dbc,0x80364280,3,true},
}};
constexpr std::array<DefinitionTableSpec,1> tables{{{0x8034ab34,0x8034ab84,0,5}}};
// Literal automatic initializers in pinned rainbow/main.c, not ROM records.
// Exact value tuples match the actual admitted initialized family; origins
// retain the containing Source function identity and initializer line.
constexpr std::array<AutomaticActorDefinition,3> automatic_definitions{{
    {{0x803466c0,56},151,0x80348938,0x80363dbc,0x80364280,0},
    {{0x803467a4,94},1037,0x80349eb8,0x80363dbc,0x80364280,1},
    {{0x80346898,132},1038,0x8034a4e8,0x80363dbc,0x80364280,4},
}};
// The complete header table/attachment closure has 311 distinct model stream
// identities and 63 texture stream identities (264+32+8+6+1 and 8+36+12+6+1).
// Counts are pinned numeric extraction facts, not an empty/nonempty shortcut.
// Following every JUMP and SET_ANIMATION (170 opcode14 occurrences across all
// 374 roots) finds exactly one opcode13/path: header8034B590, controller1002,
// DFS node1, stream/command800F567C -> InterpData800F5664 (ROM72E14,24 bytes).
// Pinned include/sys/interpolation.h blob bb9e129275a30ce093c7b154167948a6be6a74d7:11-19;
// sys/anim.c blob477cd1e92ef0bd257caa2f6eb61b2e70955f8cb1:422-467 follows
// opcode14 and binds opcode13; src/1F960.c blobcaf0e25e5ba3f1f0b313d46eb6a3b3cc6ff8b1d3:
// 128-181,312-314 consumes this position path. The shared decoder/interpolator
// owns that implementation; these are provenance facts, not a second decoder.
// Descriptor words: 03000002 3F000000 800F5618 40A33335 800F5648 800F5650.
// Thus CATROM kind3, numPoints2, tension0.5, duration bits40A33335:
// four Vec3f controls800F5618 -> ROM[72DC8,72DF8), timestamps800F5648 ->
// ROM[72DF8,72E00), one five-float speed polynomial800F5650 ->
// ROM[72E00,72E14). Controls have X/Z=0 and Y words
// 43FF0003,00000000,C3FF0003,C47F0003; timestamps are 0/1; speed words
// 00000000,00000000,00000000,00000000,41D0147F. Every scalar is finite.
// Descriptor SHA256 baf15ab5104fbb91dc1f14d95f6f1c12ef6999604ccfdeff27f97c2c1001cc30;
// complete control/time/speed storage is bounded by the typed descriptor and
// entirely inside the unchanged World initialized mapping above.
const CourseCatalog catalog{6,0x800f5da0,mappings,material_extents,initializations,headers,definitions,tables,
                            automatic_definitions,{5,14,311,63,26,7,1}};
static_assert(initializations.size()<=max_catalog_models);
} // namespace
const CourseCatalog& rainbow_catalog() noexcept { return catalog; }
} // namespace isle::actor::detail

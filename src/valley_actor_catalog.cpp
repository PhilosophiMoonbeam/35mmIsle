#include "actor_resource_catalog.hpp"

namespace isle::actor::detail {
namespace {
// Own GAME df8815e75f3d79618466e01e0827c6165b93d11c; matching map SHA256
// 545866faf5fb44abf47d2f455ff7cad75580dc15320c357ac8dadc3359cb1a00.
// Numeric authority: qualified638k typed initialization/setup/header/definition
// records and qualified638n Valley real-consumer material extents/counts.
// Source/blob/span provenance is retained in native-valley-actor-catalog-receipt.json.
// These are immutable data descriptors, not callback or gameplay admission.
constexpr std::array<InitializationSpec,20> initializations{{
    {75,0x802d2cb4,0x80323200,0x8031f9d0,0x803594dc,0x802d2ca0,0x802c82c0,0x802d2b10,17,0,8,0,0,0},
    {130,0x802d2df8,0x80326f68,0x80323520,0x803594dc,0x802d2de4,0x802c95c0,0x802d2d24,18,8,5,0,0,0},
    {74,0x802d2f2c,0x8032f328,0x8032b580,0x8035942c,0x802d2f18,0x802c9958,0x802d2e98,14,13,5,0,0,0},
    {149,0x802d302c,0x80333440,0,0x8035942c,0x802d3018,0x802c9fd0,0x802d2f8c,24,18,5,0,0,0},
    {147,0x802d3100,0x80339e20,0x80337d80,0x8035942c,0x802d30ec,0x802ca270,0x802d3084,14,23,3,0,0,0},
    {27,0x802d33f8,0x8033d148,0x8033a0c0,0x8035942c,0x802d33e4,0x802ca768,0x802d315c,13,26,10,0,0,0},
    {7,0x802d37c0,0x8034c6b8,0x80349640,0x803594dc,0x802d37ac,0x802cb080,0x802d3450,10,36,14,0,0,0},
    {120,0x802d3914,0x8032b2e0,0x803272b0,0x8035942c,0x802d3900,0x802cca70,0x802d3808,14,50,2,0,0,0},
    {121,0x802d3a44,0x80346628,0x80341910,0x8035942c,0x802d3a30,0x802cd7c0,0x802d3958,20,52,3,0,0,0},
    {1033,0x802d3c0c,0x80119330,0x80119210,0x800a15d8,0x802d3bf8,0x802cde80,0x802d3a80,3,55,9,0,0,0},
    {28,0x802d3f38,0x80341618,0x8033d3f8,0x8035942c,0x802d3f24,0x802ce200,0x802d3c5c,16,64,10,0,0,0},
    {56,0x802d4124,0x80337a68,0x80333890,0x8035942c,0x802d4110,0x802ceaa0,0x802d3f8c,17,74,5,0,0,0},
    {1034,0x802d41e8,0x80118820,0x80118780,0x800a15d8,0x802d41d4,0x802cf840,0x802d4168,3,79,3,0,0,0},
    {1032,0x802ec68c,0x8011ac50,0,0x800a15d8,0x802ec678,0x802d0320,0x802d4220,10,82,1,0,0,0},
    {1001,0x802ec750,0x800f1d10,0x800f07a0,0x800a1608,0x802ec73c,0x802d0b80,0x802ec6c0,4,83,4,0,0,0},
    {1035,0x802ec7b8,0x80119bb0,0,0x803594dc,0x802ec7a4,0x802d1190,0x802ec790,2,87,1,0,0,0},
    {129,0x802ec934,0x8034feb8,0,0x803594dc,0x802ec920,0x802d1200,0x802ec82c,18,88,4,0,0,0},
    {118,0x802eca30,0x803493a0,0x803469d0,0x8035942c,0x802eca1c,0x802d2230,0x802ec994,14,92,3,0,0,0x802ec9d0},
    {1036,0x802ecb2c,0x802eca70,0,0x80014f98,0x802ecb18,0x802d25e0,0,1,95,0,0,0,0},
    {1003,0x80388348,0x800eb430,0x800eb510,0x800a1608,0x80388334,0x8035df20,0x80388310,4,95,1,0,0,0},
}};
// Per-initialization receiver-proven slices only. The setup-typed header
// 802ec790 is a real pointer/record, not an invented named Source symbol.
// The common1003 binding is the existing closed Beach family, retained last.
constexpr std::array<std::uint32_t,96> headers{
    0x802d2ac0,0x802d2ad4,0x802d2afc,0x802d2b10,
    0x802d2b24,0x802d2b38,0x802d2b4c,0x802d2b60,
    0x802d2d10,0x802d2d24,0x802d2d38,0x802d2d4c,
    0x802d2d60,0x802d2e48,0x802d2e5c,0x802d2e84,
    0x802d2e98,0x802d2eac,0x802d2f8c,0x802d2fa0,
    0x802d2fb4,0x802d2fc8,0x802d2fdc,0x802d3084,
    0x802d3098,0x802d30ac,0x802d315c,0x802d3170,
    0x802d3184,0x802d3198,0x802d31ac,0x802d31c0,
    0x802d31d4,0x802d31e8,0x802d31fc,0x802d3210,
    0x802d3450,0x802d3464,0x802d3478,0x802d348c,
    0x802d34a0,0x802d34b4,0x802d34c8,0x802d34dc,
    0x802d34f0,0x802d3504,0x802d3518,0x802d352c,
    0x802d3540,0x802d3554,0x802d3808,0x802d381c,
    0x802d3958,0x802d396c,0x802d3980,0x802d3a80,
    0x802d3a94,0x802d3aa8,0x802d3abc,0x802d3ad0,
    0x802d3ae4,0x802d3af8,0x802d3b0c,0x802d3b20,
    0x802d3c5c,0x802d3c70,0x802d3c84,0x802d3c98,
    0x802d3cac,0x802d3cc0,0x802d3cd4,0x802d3ce8,
    0x802d3cfc,0x802d3d10,0x802d3f8c,0x802d3fa0,
    0x802d3fb4,0x802d3fc8,0x802d3fdc,0x802d4168,
    0x802d417c,0x802d4190,0x802d4220,0x802ec6c0,
    0x802ec6d4,0x802ec6e8,0x802ec6fc,0x802ec790,
    0x802ec818,0x802ec82c,0x802ec840,0x802ec854,
    0x802ec994,0x802ec9a8,0x802ec9bc,0x80388310,
};
// Source-bounded material allocation domains plus qualified638n all-program
// consumed prefixes; image and optional palette extents stay separate.
constexpr std::array<model_assets::MaterialExtent,23> material_extents{{
    {0x800eb520,0,0,0,0,0,0},
    {0x800f07b8,0x800f07b0,1,1,0,0,0},
    {0x800f0830,0x800f07b4,1,1,0,0,0},
    {0x800f08a8,0,0,0,0,0,0},
    {0x80118798,0x8011878c,2,2,0,0,0},
    {0x80119230,0x8011921c,2,2,0,0,0},
    {0x801192a8,0x80119224,2,2,0,0,0},
    {0x8031fa20,0x8031fa14,2,3,0,0,0},
    {0x80323568,0,0,0,0,0,0},
    {0x803235e0,0,0,0,0,0,0},
    {0x803272f8,0x803272e8,3,4,0,0,0},
    {0x8032b5c0,0x8032b5b8,2,2,0,0,0},
    {0x80333900,0x803338d4,5,5,0,0,0},
    {0x80333978,0x803338e8,5,6,0,0,0},
    {0x80337dc8,0x80337db8,3,4,0,0,0},
    {0x8033a110,0x8033a0f4,6,7,0,0,0},
    {0x8033d448,0x8033d438,3,4,0,0,0},
    {0x80341978,0x80341960,3,3,0,0,0},
    {0x803419f0,0x8034196c,3,3,0,0,0},
    {0x80346a20,0x80346a08,3,3,0,0,0},
    {0x80346a98,0x80346a14,3,3,0,0,0},
    {0x80349690,0x80349668,5,5,0,0,0},
    {0x80349708,0x8034967c,5,5,0,0,0},
}};
// Exact course::layout(5).mappings, common1003 initialized data and the
// loaded Magikarp initialized model data. No code/BSS or other-course union.
constexpr std::array<model_assets::Mapping,8> mappings{{
    {0x800fffa0,0x003f63e0,0x00086b50},
    {0x80186b10,0x0047cf30,0x0002b230},
    {0x802d2740,0x007abcd0,0x0007a160},
    {0x800e6ad0,0x00064280,0x0000ef30},
    {0x800f5d90,0x000731b0,0x0000a200},
    {0x803b1f80,0x0082f8e0,0x00003080},
    {0x80388310,0x00528720,0x00000090},
    {0x8034e130,0x0054b5d0,0x000020d0},
}};
// Twenty live registry rows in Source/initialized order, then six exact
// singleton records. The all-zero terminator is validated by tables below.
// Distinct ordinary Staryu/Starmie factory identities survive their shared
// initialization: registry802cd704/802cde04, singleton802cd73c/802cde3c.
// Magikarp factory802d21c8 selects ground for behavior!=3, ordinary otherwise;
// both Source calls bind InitData802ec934, never a fabricated second family.
constexpr std::array<DefinitionSpec,26> definitions{{
    {0x802d282c,75,0x802c957c,0x80363eb4,0x80364280,0,false},
    {0x802d283c,130,0x802c97f8,0x80363dbc,0x80364280,1,false},
    {0x802d284c,74,0x802c9f90,0x80363eb4,0x80364280,2,false},
    {0x802d285c,129,0x802d21c8,0x80363dbc,0x80364280,16,false},
    {0x802d286c,149,0x802ca224,0x80363eb4,0x80364280,3,false},
    {0x802d287c,147,0x802ca688,0x80363eb4,0x80364280,4,false},
    {0x802d288c,27,0x802cb02c,0x80363eb4,0x80364280,5,false},
    {0x802d289c,118,0x802d2540,0x80363eb4,0x80364280,17,false},
    {0x802d28ac,7,0x802cc8a8,0x802cc8e0,0x80364280,6,false},
    {0x802d28bc,28,0x802cea58,0x80363eb4,0x80364280,10,false},
    {0x802d28cc,56,0x802cf808,0x80363eb4,0x80364280,11,false},
    {0x802d28dc,120,0x802cd704,0x80363dbc,0x802cd774,7,false},
    {0x802d28ec,121,0x802cde04,0x80363dbc,0x80364280,8,false},
    {0x802d28fc,1036,0x802d2704,0x80363dbc,0x80364280,18,false},
    {0x802d290c,1003,0x8035e0d4,0x80363dbc,0x80364280,19,false},
    {0x802d291c,1033,0x802ce1b8,0x80363dbc,0x80364280,9,false},
    {0x802d292c,1034,0x802cfa00,0x80363dbc,0x80364280,12,false},
    {0x802d293c,1032,0x802d0b3c,0x80363dbc,0x80364280,13,false},
    {0x802d294c,1001,0x802d1154,0x80363dbc,0x80364280,14,false},
    {0x802d295c,1035,0x802d11b0,0x80363dbc,0x80364280,15,false},
    {0x802d2984,118,0x802d2540,0x80363eb4,0x80364280,17,true},
    {0x802d2994,147,0x802ca688,0x80363eb4,0x80364280,4,true},
    {0x802d29a4,129,0x802d21c8,0x80363eb4,0x80364280,16,true},
    {0x802ecae0,120,0x802cd73c,0x80363dbc,0x80364280,7,true},
    {0x802ecaf0,121,0x802cde3c,0x80363dbc,0x80364280,8,true},
    {0x8038837c,1003,0x8035e0d4,0x80363dbc,0x80364280,19,true},
}};
constexpr std::array<DefinitionTableSpec,1> tables{{{0x802d282c,0x802d296c,0,20}}};
// Exact automatic Source copies in valley/79F1B0.c blob
// 63430b57b90aa93384eb5862f19ef398be207194:84,120,153. Origins are
// containing function identities plus declaration lines, never guest storage.
// Copy three retains change80363eb4, unlike the registry Magikarp row
// change80363dbc; do not normalize the exact copied Source values.
constexpr std::array<AutomaticActorDefinition,3> automatic_definitions{{
    {{0x802c5e68,84},118,0x802d2540,0x80363eb4,0x80364280,17},
    {{0x802c5f70,120},147,0x802ca688,0x80363eb4,0x80364280,4},
    {{0x802c606c,153},129,0x802d21c8,0x80363eb4,0x80364280,16},
}};
// Qualified638n: 20 models,23 materials,1136 model streams,85 texture streams,
// 96 receiver bindings,0 paths; complete GBI closure had zero obligations.
// Extra Source-used signed list802ecb00 belongs to BehaviorResources, not
// this immutable actor catalog. No callback execution is claimed here.
const CourseCatalog catalog{5,0x80100720,mappings,material_extents,initializations,headers,definitions,tables,
                            automatic_definitions,{20,23,1136,85,96,26,0}};
static_assert(initializations.size()<=max_catalog_models);
} // namespace
const CourseCatalog& valley_catalog() noexcept { return catalog; }
} // namespace isle::actor::detail

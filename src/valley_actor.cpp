#include "valley_actor.hpp"
#include <array>
namespace isle::valley {
namespace {
constexpr std::array<std::uint32_t,62> graphs{{
    0x802d2b94,
    0x802d2bb4,
    0x802d2bd4,
    0x802d2c10,
    0x802d2c40,
    0x802d2c70,
    0x802d2d7c,
    0x802d2dac,
    0x802d2ed8,
    0x802d2ef8,
    0x802d2ff8,
    0x802d3224,
    0x802d3294,
    0x802d32b4,
    0x802d32d4,
    0x802d3304,
    0x802d3334,
    0x802d3374,
    0x802d33b4,
    0x802d3568,
    0x802d363c,
    0x802d366c,
    0x802d369c,
    0x802d36cc,
    0x802d370c,
    0x802d372c,
    0x802d374c,
    0x802d376c,
    0x802d378c,
    0x802d3850,
    0x802d3880,
    0x802d38a0,
    0x802d38e0,
    0x802d3994,
    0x802d39d0,
    0x802d39f0,
    0x802d3a10,
    0x802d3b38,
    0x802d3b68,
    0x802d3b98,
    0x802d3bc8,
    0x802d3d24,
    0x802d3da4,
    0x802d3dc4,
    0x802d3de4,
    0x802d3e14,
    0x802d3e44,
    0x802d3e94,
    0x802d3ee4,
    0x802d3ffc,
    0x802d401c,
    0x802d404c,
    0x802d4070,
    0x802d40b0,
    0x802d40d0,
    0x802d41a8,
    0x802ec658,
    0x802ec718,
    0x802ec898,
    0x802ec8b8,
    0x802ec8e4,
    0x802ec9d0,
}};
constexpr std::array<std::uint32_t,12> weighted{{
    0x802d2b74,
    0x802d2dcc,
    0x802d2ec0,
    0x802d30c0,
    0x802d3274,
    0x802d3608,
    0x802d39b4,
    0x802d3d84,
    0x802d40a0,
    0x802ec870,
    0x802ec9f0,
    0x802ecb00,
}};
constexpr std::array<std::uint32_t,255> roots{{
    0x802c5d7c,
    0x802c5e68,
    0x802c5f70,
    0x802c606c,
    0x802c65e0,
    0x802c673c,
    0x802c67f4,
    0x802c82c0,
    0x802c8354,
    0x802c840c,
    0x802c8458,
    0x802c84d0,
    0x802c853c,
    0x802c8594,
    0x802c8650,
    0x802c8704,
    0x802c8764,
    0x802c8828,
    0x802c88c4,
    0x802c892c,
    0x802c8998,
    0x802c8b38,
    0x802c8c08,
    0x802c8c70,
    0x802c8d30,
    0x802c8e54,
    0x802c8ea8,
    0x802c8f50,
    0x802c903c,
    0x802c90e4,
    0x802c91ac,
    0x802c931c,
    0x802c9408,
    0x802c94a4,
    0x802c95c0,
    0x802c95e4,
    0x802c9634,
    0x802c96a0,
    0x802c96fc,
    0x802c9750,
    0x802c97a4,
    0x802c9840,
    0x802c9958,
    0x802c9990,
    0x802c9a48,
    0x802c9a94,
    0x802c9b0c,
    0x802c9b78,
    0x802c9bd0,
    0x802c9c84,
    0x802c9d00,
    0x802c9dc4,
    0x802c9e60,
    0x802c9ec8,
    0x802c9f34,
    0x802c9fd0,
    0x802ca020,
    0x802ca1b0,
    0x802ca270,
    0x802ca2bc,
    0x802ca2f4,
    0x802ca32c,
    0x802ca364,
    0x802ca434,
    0x802ca574,
    0x802ca5b4,
    0x802ca65c,
    0x802ca6c0,
    0x802ca768,
    0x802ca7a0,
    0x802ca858,
    0x802ca8a4,
    0x802ca8f8,
    0x802ca948,
    0x802ca9bc,
    0x802caa2c,
    0x802caad0,
    0x802cab2c,
    0x802cabd0,
    0x802cac2c,
    0x802cace4,
    0x802cad54,
    0x802cadec,
    0x802cae70,
    0x802caf04,
    0x802caf54,
    0x802cb008,
    0x802cb080,
    0x802cb110,
    0x802cb194,
    0x802cb238,
    0x802cb294,
    0x802cb338,
    0x802cb394,
    0x802cb404,
    0x802cb474,
    0x802cb550,
    0x802cb5ac,
    0x802cb614,
    0x802cb6f8,
    0x802cb79c,
    0x802cb7f4,
    0x802cb818,
    0x802cb88c,
    0x802cb8d8,
    0x802cb92c,
    0x802cb9bc,
    0x802cba24,
    0x802cba90,
    0x802cbbdc,
    0x802cbc74,
    0x802cbcdc,
    0x802cbd74,
    0x802cbe14,
    0x802cbe9c,
    0x802cbf18,
    0x802cbf64,
    0x802cbfdc,
    0x802cc068,
    0x802cc08c,
    0x802cc104,
    0x802cc1e0,
    0x802cc250,
    0x802cc2b4,
    0x802cc320,
    0x802cc3b4,
    0x802cc424,
    0x802cc4dc,
    0x802cc514,
    0x802cc5c0,
    0x802cca70,
    0x802ccab4,
    0x802ccb34,
    0x802ccbf8,
    0x802ccc90,
    0x802cccfc,
    0x802ccd80,
    0x802ccddc,
    0x802cce70,
    0x802cd0b8,
    0x802cd118,
    0x802cd1ac,
    0x802cd2a8,
    0x802cd2fc,
    0x802cd4f4,
    0x802cd5d8,
    0x802cd774,
    0x802cd7c0,
    0x802cd84c,
    0x802cd8a4,
    0x802cd8dc,
    0x802cd914,
    0x802cd98c,
    0x802cd9f8,
    0x802cda94,
    0x802cdb40,
    0x802cdbd4,
    0x802cdc68,
    0x802cdcfc,
    0x802cde80,
    0x802cdea4,
    0x802cdefc,
    0x802cdf68,
    0x802cdffc,
    0x802ce090,
    0x802ce124,
    0x802ce200,
    0x802ce224,
    0x802ce2dc,
    0x802ce328,
    0x802ce37c,
    0x802ce3cc,
    0x802ce440,
    0x802ce4b0,
    0x802ce554,
    0x802ce5b0,
    0x802ce654,
    0x802ce6b0,
    0x802ce768,
    0x802ce7d8,
    0x802ce870,
    0x802ce8f4,
    0x802ce978,
    0x802cea34,
    0x802ceaa0,
    0x802ceb4c,
    0x802ceb9c,
    0x802ced44,
    0x802cefb8,
    0x802cf07c,
    0x802cf114,
    0x802cf388,
    0x802cf3f8,
    0x802cf48c,
    0x802cf500,
    0x802cf524,
    0x802cf594,
    0x802cf638,
    0x802cf65c,
    0x802cf6cc,
    0x802cf720,
    0x802cf794,
    0x802cf840,
    0x802cf864,
    0x802cf8bc,
    0x802cfa40,
    0x802cfc6c,
    0x802d0320,
    0x802d0344,
    0x802d04cc,
    0x802d07bc,
    0x802d0b80,
    0x802d0ba4,
    0x802d0bfc,
    0x802d0d0c,
    0x802d0f0c,
    0x802d0f98,
    0x802d1190,
    0x802d1200,
    0x802d129c,
    0x802d12d4,
    0x802d130c,
    0x802d1344,
    0x802d1414,
    0x802d1454,
    0x802d156c,
    0x802d1598,
    0x802d15bc,
    0x802d16cc,
    0x802d1734,
    0x802d17d8,
    0x802d18d8,
    0x802d19dc,
    0x802d1acc,
    0x802d1b48,
    0x802d1bc8,
    0x802d1c40,
    0x802d1d4c,
    0x802d1e3c,
    0x802d1eb8,
    0x802d1f18,
    0x802d1fc0,
    0x802d2128,
    0x802d2230,
    0x802d22a0,
    0x802d22d4,
    0x802d230c,
    0x802d2344,
    0x802d237c,
    0x802d2428,
    0x802d2580,
    0x802d25b0,
    0x802d25e0,
    0x802d2604,
    0x802d2684,
}};
constexpr std::array<ActorDependencyEdge,1> calls_0{{
    {0x80363a8c,"pokemonAdd","void pokemonAdd(struct WorldBlock*, struct WorldBlock*, PokemonDef* def);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_0{{
    {0x802d282c,"D_802D282C_7ABDBC","PokemonDef"},
}};
constexpr std::array<ActorNativeService,0> services_0{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_1{{
    {0x8036406c,"pokemonsChangeBlock","void pokemonsChangeBlock(struct WorldBlock*, struct WorldBlock*, PokemonDef*);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_1{{
    {0x802d282c,"D_802D282C_7ABDBC","PokemonDef"},
}};
constexpr std::array<ActorNativeService,0> services_1{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_2{{
    {0x803641b8,"pokemonRemove","void pokemonRemove(struct WorldBlock*, PokemonDef*);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_2{{
    {0x802d282c,"D_802D282C_7ABDBC","PokemonDef"},
}};
constexpr std::array<ActorNativeService,0> services_2{{
}};
constexpr std::array<ActorDependencyEdge,7> calls_3{{
    {0x800e4040,"setHeightMap","s32 setHeightMap(HeightMap* arg0);"},
    {0x800e2f38,"createWorld","s32 createWorld(WorldSetup* arg0, s32 skyBoxObjId, s32 blockMinObjId, s32 blockMaxObjId, s32 link, s32 dllink, BlockFunc2 arg6, BlockFunc arg7, BlockFunc2 arg8);"},
    {0x800e20b4,"setSkyBoxFollowPlayer","void setSkyBoxFollowPlayer(void);"},
    {0x80363928,"func_80363928_503D38","void func_80363928_503D38(s32 minObjId, s32 maxObjId, s32 link, s32 arg3);"},
    {0x802c5c20,"func_802C5C20_79F1B0","void func_802C5C20_79F1B0(WorldBlock* arg0, WorldBlock* arg1)"},
    {0x802c5c68,"func_802C5C68_79F1F8","void func_802C5C68_79F1F8(WorldBlock* arg0)"},
    {0x802c5c44,"func_802C5C44_79F1D4","void func_802C5C44_79F1D4(WorldBlock* arg0, WorldBlock* arg1)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_3{{
    {0x8031f9c0,"D_8031F9C0_7F8F50","HeightMap"},
}};
constexpr std::array<ActorNativeService,0> services_3{{
}};
constexpr std::array<ActorDependencyEdge,9> calls_4{{
    {0x800a19d8,"func_800A19D8","void func_800A19D8(s32);"},
    {0x80359074,"PokemonDetector_Disable","void PokemonDetector_Disable(void);"},
    {0x8036650c,"EnvSound_Cleanup","void EnvSound_Cleanup(void);"},
    {0x80022334,"auStopBGM","void auStopBGM(void);"},
    {0x80022b14,"auStopAllSounds","void auStopAllSounds(void);"},
    {0x800e3064,"destroyWorld","void destroyWorld(void);"},
    {0x80356fbc,"func_80356FBC_4F73CC","void func_80356FBC_4F73CC(void);"},
    {0x803586c0,"removeMainCamera","void removeMainCamera(void);"},
    {0x800067dc,"func_800067DC","void func_800067DC(void);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_4{{
    {0x802d297c,"D_802D297C_7ABF0C","s32"},
}};
constexpr std::array<ActorNativeService,2> services_4{{
    ActorNativeService::sound_stop_all,
    ActorNativeService::bgm_stop,
}};
constexpr std::array<ActorDependencyEdge,0> calls_5{{
}};
constexpr std::array<ActorResourceDependency,1> resources_5{{
    {0x802d2828,"D_802D2828_7ABDB8","f32"},
}};
constexpr std::array<ActorNativeService,0> services_5{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_5{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_5{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_5{{
}};
constexpr std::array<ActorDependencyEdge,0> calls_6{{
}};
constexpr std::array<ActorResourceDependency,0> resources_6{{
}};
constexpr std::array<ActorNativeService,0> services_6{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_7{{
    {0x8035cb18,"Items_DisplaceAllItems","void Items_DisplaceAllItems(f32 deltaX, f32 deltaY, f32 deltaZ);"},
    {0x802c5d18,"func_802C5D18_79F2A8","void func_802C5D18_79F2A8(s32 arg0)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_7{{
    {0x802d297c,"D_802D297C_7ABF0C","s32"},
}};
constexpr std::array<ActorNativeService,0> services_7{{
}};
constexpr std::array<ActorDependencyEdge,6> calls_8{{
    {0x800e2184,"getCurrentWorldBlock","WorldBlock* getCurrentWorldBlock(void);"},
    {0x80363c48,"pokemonAddOne","GObj* pokemonAddOne(struct WorldBlock* roomA, struct WorldBlock* roomB, ObjectSpawn* spawn, PokemonDef* def);"},
    {0x8000b774,"cmdSendCommand","s32 cmdSendCommand(struct GObj* obj, s32 cmd, struct GObj* source);"},
    {0x0,"GET_TRANSFORM","Source intrinsic/designator: GET_TRANSFORM"},
    {0x0,"GET_TRANSFORM","Source intrinsic/designator: GET_TRANSFORM"},
    {0x0,"GET_TRANSFORM","Source intrinsic/designator: GET_TRANSFORM"},
}};
constexpr std::array<ActorResourceDependency,4> resources_8{{
    {0x802d2980,"D_802D2980_7ABF10","GObj*"},
    {0x802d2984,"D_802D2984_7ABF14","PokemonDef"},
    {0x0,"def","Source-qualified resource"},
    {0x0,"spawn","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,8> services_8{{
    ActorNativeService::reference_assign,
    ActorNativeService::model,
    ActorNativeService::vector,
    ActorNativeService::vector_read,
    ActorNativeService::vector_store,
    ActorNativeService::current_block,
    ActorNativeService::add_one,
    ActorNativeService::command,
}};
constexpr std::array<std::uint32_t,0> callback_deps_8{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_8{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_8{{
}};
constexpr std::array<ActorDependencyEdge,6> calls_9{{
    {0x800e2184,"getCurrentWorldBlock","WorldBlock* getCurrentWorldBlock(void);"},
    {0x80363c48,"pokemonAddOne","GObj* pokemonAddOne(struct WorldBlock* roomA, struct WorldBlock* roomB, ObjectSpawn* spawn, PokemonDef* def);"},
    {0x0,"GET_POKEMON","Source intrinsic/designator: GET_POKEMON"},
    {0x0,"GET_TRANSFORM","Source intrinsic/designator: GET_TRANSFORM"},
    {0x0,"GET_TRANSFORM","Source intrinsic/designator: GET_TRANSFORM"},
    {0x0,"GET_TRANSFORM","Source intrinsic/designator: GET_TRANSFORM"},
}};
constexpr std::array<ActorResourceDependency,4> resources_9{{
    {0x802d2980,"D_802D2980_7ABF10","GObj*"},
    {0x802d2994,"D_802D2994_7ABF24","PokemonDef"},
    {0x0,"def","Source-qualified resource"},
    {0x0,"spawn","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,7> services_9{{
    ActorNativeService::reference_assign,
    ActorNativeService::model,
    ActorNativeService::vector,
    ActorNativeService::vector_read,
    ActorNativeService::vector_store,
    ActorNativeService::current_block,
    ActorNativeService::add_one,
}};
constexpr std::array<std::uint32_t,0> callback_deps_9{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_9{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_9{{
}};
constexpr std::array<ActorDependencyEdge,6> calls_10{{
    {0x800e2184,"getCurrentWorldBlock","WorldBlock* getCurrentWorldBlock(void);"},
    {0x80363c48,"pokemonAddOne","GObj* pokemonAddOne(struct WorldBlock* roomA, struct WorldBlock* roomB, ObjectSpawn* spawn, PokemonDef* def);"},
    {0x0,"GET_POKEMON","Source intrinsic/designator: GET_POKEMON"},
    {0x0,"GET_TRANSFORM","Source intrinsic/designator: GET_TRANSFORM"},
    {0x0,"GET_TRANSFORM","Source intrinsic/designator: GET_TRANSFORM"},
    {0x0,"GET_TRANSFORM","Source intrinsic/designator: GET_TRANSFORM"},
}};
constexpr std::array<ActorResourceDependency,4> resources_10{{
    {0x802d2980,"D_802D2980_7ABF10","GObj*"},
    {0x802d29a4,"D_802D29A4_7ABF34","PokemonDef"},
    {0x0,"def","Source-qualified resource"},
    {0x0,"spawn","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,7> services_10{{
    ActorNativeService::reference_assign,
    ActorNativeService::model,
    ActorNativeService::vector,
    ActorNativeService::vector_read,
    ActorNativeService::vector_store,
    ActorNativeService::current_block,
    ActorNativeService::add_one,
}};
constexpr std::array<std::uint32_t,0> callback_deps_10{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_10{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_10{{
}};
constexpr std::array<ActorDependencyEdge,7> calls_11{{
    {0x0,"GET_ITEM","Source intrinsic/designator: GET_ITEM"},
    {0x0,"ARRAY_COUNT","Source intrinsic/designator: ARRAY_COUNT"},
    {0x8000b774,"cmdSendCommand","s32 cmdSendCommand(struct GObj* obj, s32 cmd, struct GObj* source);"},
    {0x8001af30,"Vec3fDistance","Source intrinsic/designator: Vec3fDistance"},
    {0x0,"GET_TRANSFORM","Source intrinsic/designator: GET_TRANSFORM"},
    {0x80019e14,"randRange","Source intrinsic/designator: randRange"},
    {0x0,"func","Source intrinsic/designator: func"},
}};
constexpr std::array<ActorResourceDependency,2> resources_11{{
    {0x802d29b4,"D_802D29B4_7ABF44","RandomState2"},
    {0x802d3b34,"D_802D3B34_7AD0C4","GObj*"},
}};
constexpr std::array<ActorNativeService,8> services_11{{
    ActorNativeService::model,
    ActorNativeService::player_model,
    ActorNativeService::vector,
    ActorNativeService::vector_read,
    ActorNativeService::item,
    ActorNativeService::rng_range,
    ActorNativeService::command,
    ActorNativeService::vector_distance,
}};
constexpr std::array<ActorDependencyEdge,10> calls_12{{
    {0x800a73c0,"func_800A73C0","void* func_800A73C0(u32 romSrcStart, u32 romSrcEnd);"},
    {0x800a73c0,"func_800A73C0","void* func_800A73C0(u32 romSrcStart, u32 romSrcEnd);"},
    {0x800a1ed0,"fx_setupBankID","void fx_setupBankID(s32 bankID, UNK_PTR scriptDesc, UNK_PTR spritesDesc);"},
    {0x800a73c0,"func_800A73C0","void* func_800A73C0(u32 romSrcStart, u32 romSrcEnd);"},
    {0x800a73c0,"func_800A73C0","void* func_800A73C0(u32 romSrcStart, u32 romSrcEnd);"},
    {0x800a1ed0,"fx_setupBankID","void fx_setupBankID(s32 bankID, UNK_PTR scriptDesc, UNK_PTR spritesDesc);"},
    {0x800a2094,"func_800A2094","GObj* func_800A2094(s32, s32, OMCamera*);"},
    {0x803586f8,"getMainCamera","OMCamera* getMainCamera(void);"},
    {0x800a5e08,"fx_allocEffects","GObj* fx_allocEffects(s32 num);"},
    {0x800a5df4,"fx_setDitherModes","void fx_setDitherModes(s32 color, s32 alpha);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_12{{
}};
constexpr std::array<ActorNativeService,1> services_12{{
    ActorNativeService::camera,
}};
constexpr std::array<ActorDependencyEdge,24> calls_13{{
    {0x800054a0,"gtlMalloc","void* gtlMalloc(s32 size, s32 alignment);"},
    {0x80007bc4,"viApplyScreenSettings","void viApplyScreenSettings(ScreenSettings* settings);"},
    {0x0,"ARRAY_COUNT","Source intrinsic/designator: ARRAY_COUNT"},
    {0x8009ce00,"func_8009CE00","void func_8009CE00(void);"},
    {0x8000a410,"omAddGObj","GObj* omAddGObj(u32 id, void (*fnUpdate)(GObj*), u8 link, u32 priority);"},
    {0x80363738,"Pokemons_Init","void Pokemons_Init(void);"},
    {0x802c5c8c,"func_802C5C8C_79F21C","void func_802C5C8C_79F21C(void)"},
    {0x800a1758,"getBackgroundColor","Source intrinsic/designator: getBackgroundColor"},
    {0x80358420,"createMainCameras","GObj* createMainCameras(s32 bgColor);"},
    {0x803563a0,"initUI","GObj* initUI(void (*exitBlockCB)(struct WorldBlock*), void (*updateMovementCB)(s32, f32), GObjFunc fnUpdateItems, u8 fnUpdateItemsKind, void (*fnCollide)(GObj*, GroundResult*));"},
    {0x80353d50,"setEndLevelCallback","void setEndLevelCallback(void (*cb)(s32));"},
    {0x80353d5c,"setPauseCallback","void setPauseCallback(void (*cb)(s32));"},
    {0x80366470,"EnvSound_Init","void EnvSound_Init(EnvSoundData* data, s32 numEntries);"},
    {0x0,"ARRAY_COUNT","Source intrinsic/designator: ARRAY_COUNT"},
    {0x802c6300,"func_802C6300_79F890","void func_802C6300_79F890(void)"},
    {0x80358f14,"PokemonDetector_Create","void PokemonDetector_Create(void);"},
    {0x80359064,"PokemonDetector_Enable","void PokemonDetector_Enable(void);"},
    {0x802c6ad4,"func_802C6AD4_7A0064","void func_802C6AD4_7A0064(void)"},
    {0x802c5d7c,"func_802C5D7C_79F30C","void func_802C5D7C_79F30C(GObj* arg0)"},
    {0x802c5dd4,"func_802C5DD4_79F364","void func_802C5DD4_79F364(WorldBlock* arg0)"},
    {0x802c6b30,"func_802C6B30_7A00C0","void func_802C6B30_7A00C0(s32 arg0, f32 arg1)"},
    {0x802c6168,"func_802C6168_79F6F8","void func_802C6168_79F6F8(GObj* obj, GroundResult* ground)"},
    {0x802c5d18,"func_802C5D18_79F2A8","void func_802C5D18_79F2A8(s32 arg0)"},
    {0x802c5dcc,"func_802C5DCC_79F35C","void func_802C5DCC_79F35C(s32 arg0)"},
}};
constexpr std::array<ActorResourceDependency,2> resources_13{{
    {0x802d2740,"D_802D2740_7ABCD0","EnvSoundData"},
    {0x802d29d4,"D_802D29D4_7ABF64","ScreenSettings"},
}};
constexpr std::array<ActorNativeService,1> services_13{{
    ActorNativeService::object_add,
}};
constexpr std::array<ActorDependencyEdge,0> calls_14{{
}};
constexpr std::array<ActorResourceDependency,0> resources_14{{
}};
constexpr std::array<ActorNativeService,0> services_14{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_15{{
    {0x800075fc,"gtlSetIntervals","void gtlSetIntervals(u16 updateInterval, u16 drawInterval);"},
    {0x80005448,"gtlDisableNearClipping","void gtlDisableNearClipping(s32);"},
    {0x800073ac,"omSetupScene","void omSetupScene(SceneSetup* arg);"},
    {0x8009bc68,"func_8009BC68","s32 func_8009BC68(void);"},
    {0x800aaed0,"func_800AAED0","void func_800AAED0(s32);"},
}};
constexpr std::array<ActorResourceDependency,3> resources_15{{
    {0x802d297c,"D_802D297C_7ABF0C","s32"},
    {0x802d29f0,"D_802D29F0_7ABF80","SceneSetup"},
    {0x802d2a00,"D_802D2A00_7ABF90","UNK_TYPE"},
}};
constexpr std::array<ActorNativeService,1> services_15{{
    ActorNativeService::application_notice,
}};
constexpr std::array<ActorDependencyEdge,3> calls_16{{
    {0x803571c4,"func_803571C4_4F75D4","void func_803571C4_4F75D4(void);"},
    {0x80357120,"func_80357120_4F7530","void func_80357120_4F7530(GObj*);"},
    {0x803570b0,"func_803570B0_4F74C0","void func_803570B0_4F74C0(void);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_16{{
}};
constexpr std::array<ActorNativeService,0> services_16{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_16{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_16{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_16{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_17{{
    {0x803572b0,"func_803572B0_4F76C0","void func_803572B0_4F76C0(void);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_17{{
}};
constexpr std::array<ActorNativeService,0> services_17{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_18{{
    {0x800a6c48,"fx_createEffect","Effect* fx_createEffect(s32 bankID, s32 scriptID);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_18{{
    {0x802d2a80,"D_802D2A80_7AC010","bool"},
}};
constexpr std::array<ActorNativeService,2> services_18{{
    ActorNativeService::effect_create,
    ActorNativeService::effect_attach,
}};
constexpr std::array<ActorDependencyEdge,6> calls_19{{
    {0x80022fd0,"auStopSound","void auStopSound(u32 handle);"},
    {0x80022fd0,"auStopSound","void auStopSound(u32 handle);"},
    {0x80357170,"func_80357170_4F7580","void func_80357170_4F7580(void);"},
    {0x803573bc,"resetMainCameraSettings","void resetMainCameraSettings(void);"},
    {0x802c6610,"func_802C6610_79FBA0","void func_802C6610_79FBA0(void)"},
    {0x8000a52c,"omDeleteGObj","void omDeleteGObj(GObj* obj);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_19{{
    {0x8034c8b8,"D_8034C8B8_825E48","u8"},
    {0x8034c8b9,"D_8034C8B9_825E49","u8"},
}};
constexpr std::array<ActorNativeService,3> services_19{{
    ActorNativeService::sound_active,
    ActorNativeService::sound_stop,
    ActorNativeService::reset_camera,
}};
constexpr std::array<ActorDependencyEdge,5> calls_20{{
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x80022a58,"auPlaySoundWithParams","s32 auPlaySoundWithParams(u32 soundID, s32 volume, s32 pan, f32 pitch, s32 reverbAmt);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x80022a58,"auPlaySoundWithParams","s32 auPlaySoundWithParams(u32 soundID, s32 volume, s32 pan, f32 pitch, s32 reverbAmt);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_20{{
    {0x8034c8b8,"D_8034C8B8_825E48","u8"},
    {0x8034c8b9,"D_8034C8B9_825E49","u8"},
}};
constexpr std::array<ActorNativeService,1> services_20{{
    ActorNativeService::sound_params,
}};
constexpr std::array<std::uint32_t,0> callback_deps_20{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_20{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_20{{
}};
constexpr std::array<ActorDependencyEdge,16> calls_21{{
    {0x803586f8,"getMainCamera","OMCamera* getMainCamera(void);"},
    {0x803586f8,"getMainCamera","OMCamera* getMainCamera(void);"},
    {0x8001054c,"animSetCameraAnimation","void animSetCameraAnimation(OMCamera* cam, AnimCmd* animList, f32 skipFrames);"},
    {0x80008c28,"omCreateProcess","GObjProcess* omCreateProcess(GObj* obj, void (*func)(struct GObj*), u8 kind, u32 pri);"},
    {0x80365f38,"PlayerModel_SetAnimation","void PlayerModel_SetAnimation(AnimCmd** modelAnim, AnimCmd*** matAnim, f32 startTime, f32 speed);"},
    {0x80008c28,"omCreateProcess","GObjProcess* omCreateProcess(GObj* obj, void (*func)(struct GObj*), u8 kind, u32 pri);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
    {0x802c66a4,"valley_stopIntro","void valley_stopIntro(void)"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
    {0x802c66a4,"valley_stopIntro","void valley_stopIntro(void)"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x802c66a4,"valley_stopIntro","void valley_stopIntro(void)"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x802c6630,"func_802C6630_79FBC0","void func_802C6630_79FBC0(DObj* dobj, s32 arg1, f32 arg2)"},
    {0x802c673c,"func_802C673C_79FCCC","void func_802C673C_79FCCC(GObj* arg0)"},
}};
constexpr std::array<ActorResourceDependency,4> resources_21{{
    {0x80120520,"D_80120520_416960","AnimCmd*"},
    {0x801212a0,"D_801212A0_4176E0","AnimCmd**"},
    {0x80121d30,"D_80121D30_418170","AnimCmd"},
    {0x802d2a80,"D_802D2A80_7AC010","bool"},
}};
constexpr std::array<ActorNativeService,9> services_21{{
    ActorNativeService::camera,
    ActorNativeService::camera_speed,
    ActorNativeService::camera_read,
    ActorNativeService::camera_component,
    ActorNativeService::camera_object,
    ActorNativeService::camera_animation,
    ActorNativeService::camera_animation_process,
    ActorNativeService::player_model_animation,
    ActorNativeService::input_pressed,
}};
constexpr std::array<std::uint32_t,1> callback_deps_21{{
    0x802c673c,
}};
constexpr std::array<std::uint32_t,0> graph_deps_21{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_21{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_22{{
    {0x80365e80,"PlayerModel_Init","GObj* PlayerModel_Init(void);"},
    {0x80008c28,"omCreateProcess","GObjProcess* omCreateProcess(GObj* obj, void (*func)(struct GObj*), u8 kind, u32 pri);"},
    {0x802c65e0,"func_802C65E0_79FB70","void func_802C65E0_79FB70(GObj* arg0)"},
    {0x802c67f4,"func_802C67F4_79FD84","void func_802C67F4_79FD84(GObj* obj)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_22{{
    {0x802d2a84,"D_802D2A84_7AC014","GObj*"},
}};
constexpr std::array<ActorNativeService,2> services_22{{
    ActorNativeService::reference_assign,
    ActorNativeService::player_model_init,
}};
constexpr std::array<ActorDependencyEdge,41> calls_23{{
    {0x80022a58,"auPlaySoundWithParams","s32 auPlaySoundWithParams(u32 soundID, s32 volume, s32 pan, f32 pitch, s32 reverbAmt);"},
    {0x803507e0,"setBackgroundMusic","void setBackgroundMusic(s32);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x80022a58,"auPlaySoundWithParams","s32 auPlaySoundWithParams(u32 soundID, s32 volume, s32 pan, f32 pitch, s32 reverbAmt);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x80022fd0,"auStopSound","void auStopSound(u32 handle);"},
    {0x80022a58,"auPlaySoundWithParams","s32 auPlaySoundWithParams(u32 soundID, s32 volume, s32 pan, f32 pitch, s32 reverbAmt);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x80022fd0,"auStopSound","void auStopSound(u32 handle);"},
    {0x80022a58,"auPlaySoundWithParams","s32 auPlaySoundWithParams(u32 soundID, s32 volume, s32 pan, f32 pitch, s32 reverbAmt);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x80022fd0,"auStopSound","void auStopSound(u32 handle);"},
    {0x80022a58,"auPlaySoundWithParams","s32 auPlaySoundWithParams(u32 soundID, s32 volume, s32 pan, f32 pitch, s32 reverbAmt);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x80022fd0,"auStopSound","void auStopSound(u32 handle);"},
    {0x80022a58,"auPlaySoundWithParams","s32 auPlaySoundWithParams(u32 soundID, s32 volume, s32 pan, f32 pitch, s32 reverbAmt);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x80022fd0,"auStopSound","void auStopSound(u32 handle);"},
    {0x80022a58,"auPlaySoundWithParams","s32 auPlaySoundWithParams(u32 soundID, s32 volume, s32 pan, f32 pitch, s32 reverbAmt);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x80022fd0,"auStopSound","void auStopSound(u32 handle);"},
}};
constexpr std::array<ActorResourceDependency,3> resources_23{{
    {0x802d2a90,"D_802D2A90_7AC020","u32"},
    {0x802d2a94,"D_802D2A94_7AC024","u32"},
    {0x8034c8c8,"D_8034C8C8_825E58","u8"},
}};
constexpr std::array<ActorNativeService,4> services_23{{
    ActorNativeService::sound_params,
    ActorNativeService::sound_stop,
    ActorNativeService::sound_volume,
    ActorNativeService::background_music,
}};
constexpr std::array<ActorDependencyEdge,7> calls_24{{
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802c8354,"func_802C8354_7A18E4","POKEMON_FUNC(func_802C8354_7A18E4)"},
    {0x802c8b38,"func_802C8B38_7A20C8","POKEMON_FUNC(func_802C8B38_7A20C8)"},
    {0x802c8c08,"func_802C8C08_7A2198","POKEMON_FUNC(func_802C8C08_7A2198)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_24{{
    {0x802d2b8c,"D_802D2B8C_7AC11C","GObj*"},
}};
constexpr std::array<ActorNativeService,1> services_24{{
    ActorNativeService::reference_assign,
}};
constexpr std::array<std::uint32_t,3> callback_deps_24{{
    0x802c8354,
    0x802c8b38,
    0x802c8c08,
}};
constexpr std::array<std::uint32_t,0> graph_deps_24{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_24{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_25{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802c840c,"func_802C840C_7A199C","POKEMON_FUNC(func_802C840C_7A199C)"},
}};
constexpr std::array<ActorResourceDependency,3> resources_25{{
    {0x802d2b10,"D_802D2B10_7AC0A0","AnimationHeader"},
    {0x802d2b94,"D_802D2B94_7AC124","InteractionHandler2"},
    {0x0,"sp20","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,1> services_25{{
    ActorNativeService::graph_copy,
}};
constexpr std::array<std::uint32_t,1> callback_deps_25{{
    0x802c840c,
}};
constexpr std::array<std::uint32_t,1> graph_deps_25{{
    0x802d2b94,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_25{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_26{{
    {0x8036148c,"Pokemon_TurnToTarget","void Pokemon_TurnToTarget(GObj*, f32, u32);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_26{{
}};
constexpr std::array<ActorNativeService,0> services_26{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_26{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_26{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_26{{
}};
constexpr std::array<ActorDependencyEdge,6> calls_27{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802c84d0,"func_802C84D0_7A1A60","POKEMON_FUNC(func_802C84D0_7A1A60)"},
    {0x802c853c,"func_802C853C_7A1ACC","POKEMON_FUNC(func_802C853C_7A1ACC)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_27{{
    {0x802d2b24,"D_802D2B24_7AC0B4","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_27{{
}};
constexpr std::array<std::uint32_t,2> callback_deps_27{{
    0x802c84d0,
    0x802c853c,
}};
constexpr std::array<std::uint32_t,0> graph_deps_27{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_27{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_28{{
    {0x80362050,"Pokemon_ResetPathPos","void Pokemon_ResetPathPos(GObj*);"},
    {0x803620c8,"Pokemon_FollowPath","void Pokemon_FollowPath(GObj* obj, f32 start, f32 end, f32 dt, f32 yawStep, u32 flags);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_28{{
}};
constexpr std::array<ActorNativeService,0> services_28{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_28{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_28{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_28{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_29{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_29{{
    {0x802d2ac0,"D_802D2AC0_7AC050","AnimationHeader"},
    {0x802d2bb4,"D_802D2BB4_7AC144","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_29{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,0> callback_deps_29{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_29{{
    0x802d2bb4,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_29{{
}};
constexpr std::array<ActorDependencyEdge,8> calls_30{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802c853c,"func_802C853C_7A1ACC","POKEMON_FUNC(func_802C853C_7A1ACC)"},
}};
constexpr std::array<ActorResourceDependency,4> resources_30{{
    {0x802d2b38,"D_802D2B38_7AC0C8","AnimationHeader"},
    {0x802d2b4c,"D_802D2B4C_7AC0DC","AnimationHeader"},
    {0x802d2b60,"D_802D2B60_7AC0F0","AnimationHeader"},
    {0x802d2bd4,"D_802D2BD4_7AC164","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_30{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,1> callback_deps_30{{
    0x802c853c,
}};
constexpr std::array<std::uint32_t,1> graph_deps_30{{
    0x802d2bd4,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_30{{
}};
constexpr std::array<ActorDependencyEdge,11> calls_31{{
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035ed90,"Pokemon_StartAuxProc","void Pokemon_StartAuxProc(GObj*, void (*)(GObj*));"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802c8704,"func_802C8704_7A1C94","POKEMON_FUNC(func_802C8704_7A1C94)"},
    {0x802c8764,"func_802C8764_7A1CF4","POKEMON_FUNC(func_802C8764_7A1CF4)"},
    {0x802c8828,"func_802C8828_7A1DB8","POKEMON_FUNC(func_802C8828_7A1DB8)"},
}};
constexpr std::array<ActorResourceDependency,2> resources_31{{
    {0x802d2ad4,"D_802D2AD4_7AC064","AnimationHeader"},
    {0x802d2afc,"D_802D2AFC_7AC08C","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_31{{
}};
constexpr std::array<std::uint32_t,3> callback_deps_31{{
    0x802c8704,
    0x802c8764,
    0x802c8828,
}};
constexpr std::array<std::uint32_t,0> graph_deps_31{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_31{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_32{{
    {0x8036082c,"Pokemon_JumpAndBounceFromGround","void Pokemon_JumpAndBounceFromGround(GObj* obj, f32 speed, f32 jumpBackwards, f32 arg3);"},
    {0x8000b830,"cmdSendCommandToLink","void cmdSendCommandToLink(s32 llIndex, s32, struct GObj*);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_32{{
}};
constexpr std::array<ActorNativeService,2> services_32{{
    ActorNativeService::jump_bounce,
    ActorNativeService::command_link,
}};
constexpr std::array<std::uint32_t,0> callback_deps_32{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_32{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_32{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_33{{
    {0x80360fc8,"Pokemon_Turn","bool Pokemon_Turn(DObj* model, f32 targetYaw, f32 turnSpeed);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x8035edc8,"Pokemon_StopAuxProc","void Pokemon_StopAuxProc(GObj*);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_33{{
}};
constexpr std::array<ActorNativeService,3> services_33{{
    ActorNativeService::model,
    ActorNativeService::vector,
    ActorNativeService::vector_read,
}};
constexpr std::array<std::uint32_t,0> callback_deps_33{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_33{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_33{{
}};
constexpr std::array<ActorDependencyEdge,9> calls_34{{
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802c88c4,"func_802C88C4_7A1E54","POKEMON_FUNC(func_802C88C4_7A1E54)"},
    {0x802c8458,"func_802C8458_7A19E8","POKEMON_FUNC(func_802C8458_7A19E8)"},
}};
constexpr std::array<ActorResourceDependency,2> resources_34{{
    {0x802d2b10,"D_802D2B10_7AC0A0","AnimationHeader"},
    {0x802d2b24,"D_802D2B24_7AC0B4","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_34{{
}};
constexpr std::array<std::uint32_t,2> callback_deps_34{{
    0x802c8458,
    0x802c88c4,
}};
constexpr std::array<std::uint32_t,0> graph_deps_34{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_34{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_35{{
    {0x80361b50,"Pokemon_SetTargetPos","void Pokemon_SetTargetPos(GObj*, f32, f32);"},
    {0x80361e58,"Pokemon_RunToTargetPos","void Pokemon_RunToTargetPos(GObj*, f32);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_35{{
}};
constexpr std::array<ActorNativeService,0> services_35{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_35{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_35{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_35{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_36{{
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ecac,"Pokemon_SetStateRandom","void Pokemon_SetStateRandom(GObj* obj, RandomState* nextStates);"},
    {0x802c8998,"func_802C8998_7A1F28","POKEMON_FUNC(func_802C8998_7A1F28)"},
}};
constexpr std::array<ActorResourceDependency,2> resources_36{{
    {0x802d2b24,"D_802D2B24_7AC0B4","AnimationHeader"},
    {0x802d2b74,"D_802D2B74_7AC104","RandomState"},
}};
constexpr std::array<ActorNativeService,1> services_36{{
    ActorNativeService::weighted,
}};
constexpr std::array<std::uint32_t,1> callback_deps_36{{
    0x802c8998,
}};
constexpr std::array<std::uint32_t,0> graph_deps_36{{
}};
constexpr std::array<std::uint32_t,1> weighted_deps_36{{
    0x802d2b74,
}};
constexpr std::array<ActorDependencyEdge,2> calls_37{{
    {0x80361110,"Pokemon_RunInCircles","void Pokemon_RunInCircles(GObj*, f32, f32, UNK_TYPE);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_37{{
}};
constexpr std::array<ActorNativeService,0> services_37{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_37{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_37{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_37{{
}};
constexpr std::array<ActorDependencyEdge,0> calls_38{{
}};
constexpr std::array<ActorResourceDependency,1> resources_38{{
    {0x802d2bf4,"D_802D2BF4_7AC184","f32"},
}};
constexpr std::array<ActorNativeService,0> services_38{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_39{{
    {0x802c89f4,"func_802C89F4_7A1F84","f32 func_802C89F4_7A1F84(s32 behavior)"},
    {0x80032a20,"__sinf","f32 __sinf(f32);"},
    {0x80038de0,"__cosf","f32 __cosf(f32);"},
    {0x800e41d8,"getGroundAt","s32 getGroundAt(f32 arg0, f32 arg1, GroundResult* arg2);"},
    {0x80360fc8,"Pokemon_Turn","bool Pokemon_Turn(DObj* model, f32 targetYaw, f32 turnSpeed);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_39{{
    {0x802d2bf8,"D_802D2BF8_7AC188","f32"},
}};
constexpr std::array<ActorNativeService,3> services_39{{
    ActorNativeService::vector_read,
    ActorNativeService::vector_store,
    ActorNativeService::ground,
}};
constexpr std::array<ActorDependencyEdge,6> calls_40{{
    {0x80357384,"getLevelProgress","void getLevelProgress(s32* blockCount, f32* blockPart);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x8035c834,"Items_GetPokeFluteCmd","s32 Items_GetPokeFluteCmd(void);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802c8c70,"func_802C8C70_7A2200","POKEMON_FUNC(func_802C8C70_7A2200)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_40{{
}};
constexpr std::array<ActorNativeService,2> services_40{{
    ActorNativeService::level_progress,
    ActorNativeService::flute,
}};
constexpr std::array<std::uint32_t,1> callback_deps_40{{
    0x802c8c70,
}};
constexpr std::array<std::uint32_t,0> graph_deps_40{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_40{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_41{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802c8d30,"func_802C8D30_7A22C0","POKEMON_FUNC(func_802C8D30_7A22C0)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_41{{
}};
constexpr std::array<ActorNativeService,1> services_41{{
    ActorNativeService::reference_assign,
}};
constexpr std::array<std::uint32_t,1> callback_deps_41{{
    0x802c8d30,
}};
constexpr std::array<std::uint32_t,0> graph_deps_41{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_41{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_42{{
    {0x8001fca4,"GetInterpolatedPosition","void GetInterpolatedPosition(Vec3f*, InterpData*, f32);"},
    {0x800e2184,"getCurrentWorldBlock","WorldBlock* getCurrentWorldBlock(void);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802c8d30,"func_802C8D30_7A22C0","POKEMON_FUNC(func_802C8D30_7A22C0)"},
}};
constexpr std::array<ActorResourceDependency,4> resources_42{{
    {0x802d2b90,"D_802D2B90_7AC120","GObj*"},
    {0x802d2bfc,"D_802D2BFC_7AC18C","Vec3f"},
    {0x8034c8d8,"D_8034C8D8_825E68","f32"},
    {0x8034c8dc,"D_8034C8DC_825E6C","f32"},
}};
constexpr std::array<ActorNativeService,3> services_42{{
    ActorNativeService::reference_assign,
    ActorNativeService::current_block,
    ActorNativeService::path_position,
}};
constexpr std::array<std::uint32_t,1> callback_deps_42{{
    0x802c8d30,
}};
constexpr std::array<std::uint32_t,0> graph_deps_42{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_42{{
}};
constexpr std::array<ActorDependencyEdge,11> calls_43{{
    {0x802c8a4c,"func_802C8A4C_7A1FDC","void func_802C8A4C_7A1FDC(DObj* model, Pokemon* pokemon, Vec3f* arg2)"},
    {0x80361b50,"Pokemon_SetTargetPos","void Pokemon_SetTargetPos(GObj*, f32, f32);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ed90,"Pokemon_StartAuxProc","void Pokemon_StartAuxProc(GObj*, void (*)(GObj*));"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802c8e54,"func_802C8E54_7A23E4","POKEMON_FUNC(func_802C8E54_7A23E4)"},
    {0x802c94a4,"func_802C94A4_7A2A34","POKEMON_FUNC(func_802C94A4_7A2A34)"},
    {0x802c8ea8,"func_802C8EA8_7A2438","POKEMON_FUNC(func_802C8EA8_7A2438)"},
}};
constexpr std::array<ActorResourceDependency,2> resources_43{{
    {0x802d2bf8,"D_802D2BF8_7AC188","f32"},
    {0x802d2c08,"D_802D2C08_7AC198","s32"},
}};
constexpr std::array<ActorNativeService,2> services_43{{
    ActorNativeService::model,
    ActorNativeService::forbidden_ground,
}};
constexpr std::array<std::uint32_t,3> callback_deps_43{{
    0x802c8e54,
    0x802c8ea8,
    0x802c94a4,
}};
constexpr std::array<std::uint32_t,0> graph_deps_43{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_43{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_44{{
    {0x80361e58,"Pokemon_RunToTargetPos","void Pokemon_RunToTargetPos(GObj*, f32);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_44{{
}};
constexpr std::array<ActorNativeService,0> services_44{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_44{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_44{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_44{{
}};
constexpr std::array<ActorDependencyEdge,6> calls_45{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802c8f50,"func_802C8F50_7A24E0","POKEMON_FUNC(func_802C8F50_7A24E0)"},
    {0x802c8ea8,"func_802C8EA8_7A2438","POKEMON_FUNC(func_802C8EA8_7A2438)"},
}};
constexpr std::array<ActorResourceDependency,3> resources_45{{
    {0x802d2b24,"D_802D2B24_7AC0B4","AnimationHeader"},
    {0x802d2c10,"D_802D2C10_7AC1A0","InteractionHandler3"},
    {0x0,"sp20","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,1> services_45{{
    ActorNativeService::graph_copy,
}};
constexpr std::array<std::uint32_t,2> callback_deps_45{{
    0x802c8ea8,
    0x802c8f50,
}};
constexpr std::array<std::uint32_t,1> graph_deps_45{{
    0x802d2c10,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_45{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_46{{
    {0x802c8a4c,"func_802C8A4C_7A1FDC","void func_802C8A4C_7A1FDC(DObj* model, Pokemon* pokemon, Vec3f* arg2)"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_46{{
    {0x802d2bf4,"D_802D2BF4_7AC184","f32"},
    {0x802d2bf8,"D_802D2BF8_7AC188","f32"},
}};
constexpr std::array<ActorNativeService,2> services_46{{
    ActorNativeService::model,
    ActorNativeService::vector,
}};
constexpr std::array<std::uint32_t,0> callback_deps_46{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_46{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_46{{
}};
constexpr std::array<ActorDependencyEdge,6> calls_47{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802c90e4,"func_802C90E4_7A2674","POKEMON_FUNC(func_802C90E4_7A2674)"},
    {0x802c903c,"func_802C903C_7A25CC","POKEMON_FUNC(func_802C903C_7A25CC)"},
}};
constexpr std::array<ActorResourceDependency,3> resources_47{{
    {0x802d2b24,"D_802D2B24_7AC0B4","AnimationHeader"},
    {0x802d2c40,"D_802D2C40_7AC1D0","InteractionHandler3"},
    {0x0,"sp20","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,1> services_47{{
    ActorNativeService::graph_copy,
}};
constexpr std::array<std::uint32_t,2> callback_deps_47{{
    0x802c903c,
    0x802c90e4,
}};
constexpr std::array<std::uint32_t,1> graph_deps_47{{
    0x802d2c40,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_47{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_48{{
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_48{{
}};
constexpr std::array<ActorNativeService,4> services_48{{
    ActorNativeService::model,
    ActorNativeService::vector,
    ActorNativeService::vector_read,
    ActorNativeService::vector_store,
}};
constexpr std::array<std::uint32_t,0> callback_deps_48{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_48{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_48{{
}};
constexpr std::array<ActorDependencyEdge,12> calls_49{{
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802c9408,"func_802C9408_7A2998","POKEMON_FUNC(func_802C9408_7A2998)"},
    {0x802c931c,"func_802C931C_7A28AC","POKEMON_FUNC(func_802C931C_7A28AC)"},
    {0x802c91ac,"func_802C91AC_7A273C","POKEMON_FUNC(func_802C91AC_7A273C)"},
}};
constexpr std::array<ActorResourceDependency,3> resources_49{{
    {0x802d2b24,"D_802D2B24_7AC0B4","AnimationHeader"},
    {0x802d2b38,"D_802D2B38_7AC0C8","AnimationHeader"},
    {0x0,"sp44","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,1> services_49{{
    ActorNativeService::graph_copy,
}};
constexpr std::array<std::uint32_t,3> callback_deps_49{{
    0x802c91ac,
    0x802c931c,
    0x802c9408,
}};
constexpr std::array<std::uint32_t,0> graph_deps_49{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_49{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_50{{
    {0x802c8a4c,"func_802C8A4C_7A1FDC","void func_802C8A4C_7A1FDC(DObj* model, Pokemon* pokemon, Vec3f* arg2)"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_50{{
    {0x802d2bf4,"D_802D2BF4_7AC184","f32"},
    {0x802d2bf8,"D_802D2BF8_7AC188","f32"},
}};
constexpr std::array<ActorNativeService,2> services_50{{
    ActorNativeService::model,
    ActorNativeService::vector,
}};
constexpr std::array<std::uint32_t,0> callback_deps_50{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_50{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_50{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_51{{
    {0x802c8a4c,"func_802C8A4C_7A1FDC","void func_802C8A4C_7A1FDC(DObj* model, Pokemon* pokemon, Vec3f* arg2)"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_51{{
    {0x802d2bf8,"D_802D2BF8_7AC188","f32"},
}};
constexpr std::array<ActorNativeService,2> services_51{{
    ActorNativeService::model,
    ActorNativeService::vector,
}};
constexpr std::array<std::uint32_t,0> callback_deps_51{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_51{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_51{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_52{{
    {0x8035c834,"Items_GetPokeFluteCmd","s32 Items_GetPokeFluteCmd(void);"},
    {0x8000b830,"cmdSendCommandToLink","void cmdSendCommandToLink(s32 llIndex, s32, struct GObj*);"},
    {0x8000b830,"cmdSendCommandToLink","void cmdSendCommandToLink(s32 llIndex, s32, struct GObj*);"},
    {0x8000b830,"cmdSendCommandToLink","void cmdSendCommandToLink(s32 llIndex, s32, struct GObj*);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_52{{
}};
constexpr std::array<ActorNativeService,2> services_52{{
    ActorNativeService::flute,
    ActorNativeService::command_link,
}};
constexpr std::array<std::uint32_t,0> callback_deps_52{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_52{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_52{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_53{{
    {0x80362ee0,"Pokemon_SpawnOnGround","GObj* Pokemon_SpawnOnGround(s32 gObjID, u16 id, struct WorldBlock* roomA, struct WorldBlock* roomB, ObjectSpawn* spawn, PokemonInitData* initData);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_53{{
    {0x802d2cb4,"D_802D2CB4_7AC244","PokemonInitData"},
    {0x0,"spawn","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,0> services_53{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_54{{
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802c95e4,"func_802C95E4_7A2B74","POKEMON_FUNC(func_802C95E4_7A2B74)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_54{{
}};
constexpr std::array<ActorNativeService,0> services_54{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_54{{
    0x802c95e4,
}};
constexpr std::array<std::uint32_t,0> graph_deps_54{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_54{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_55{{
    {0x0,"HIDE_POKEMON","Source intrinsic/designator: HIDE_POKEMON"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_55{{
    {0x802d2dac,"D_802D2DAC_7AC33C","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_55{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,0> callback_deps_55{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_55{{
    0x802d2dac,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_55{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_56{{
    {0x0,"SHOW_POKEMON","Source intrinsic/designator: SHOW_POKEMON"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802c9750,"func_802C9750_7A2CE0","POKEMON_FUNC(func_802C9750_7A2CE0)"},
}};
constexpr std::array<ActorResourceDependency,2> resources_56{{
    {0x802d2d10,"D_802D2D10_7AC2A0","AnimationHeader"},
    {0x802d2d78,"D_802D2D78_7AC308","u16"},
}};
constexpr std::array<ActorNativeService,0> services_56{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_56{{
    0x802c9750,
}};
constexpr std::array<std::uint32_t,0> graph_deps_56{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_56{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_57{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ecac,"Pokemon_SetStateRandom","void Pokemon_SetStateRandom(GObj* obj, RandomState* nextStates);"},
}};
constexpr std::array<ActorResourceDependency,3> resources_57{{
    {0x802d2d24,"D_802D2D24_7AC2B4","AnimationHeader"},
    {0x802d2d7c,"D_802D2D7C_7AC30C","InteractionHandler"},
    {0x802d2dcc,"D_802D2DCC_7AC35C","RandomState"},
}};
constexpr std::array<ActorNativeService,2> services_57{{
    ActorNativeService::graph,
    ActorNativeService::weighted,
}};
constexpr std::array<std::uint32_t,0> callback_deps_57{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_57{{
    0x802d2d7c,
}};
constexpr std::array<std::uint32_t,1> weighted_deps_57{{
    0x802d2dcc,
}};
constexpr std::array<ActorDependencyEdge,4> calls_58{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802c9750,"func_802C9750_7A2CE0","POKEMON_FUNC(func_802C9750_7A2CE0)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_58{{
    {0x802d2d60,"D_802D2D60_7AC2F0","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_58{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_58{{
    0x802c9750,
}};
constexpr std::array<std::uint32_t,0> graph_deps_58{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_58{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_59{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802c96a0,"func_802C96A0_7A2C30","POKEMON_FUNC(func_802C96A0_7A2C30)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_59{{
    {0x802d2d4c,"D_802D2D4C_7AC2DC","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_59{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_59{{
    0x802c96a0,
}};
constexpr std::array<std::uint32_t,0> graph_deps_59{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_59{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_60{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802c96a0,"func_802C96A0_7A2C30","POKEMON_FUNC(func_802C96A0_7A2C30)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_60{{
    {0x802d2d38,"D_802D2D38_7AC2C8","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_60{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_60{{
    0x802c96a0,
}};
constexpr std::array<std::uint32_t,0> graph_deps_60{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_60{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_61{{
    {0x80362e5c,"Pokemon_Spawn","GObj* Pokemon_Spawn(s32 gObjID, u16 id, struct WorldBlock* roomA, struct WorldBlock* roomB, ObjectSpawn* spawn, PokemonInitData* initData);"},
}};
constexpr std::array<ActorResourceDependency,3> resources_61{{
    {0x802d2d74,"D_802D2D74_7AC304","GObj*"},
    {0x802d2df8,"D_802D2DF8_7AC388","PokemonInitData"},
    {0x0,"spawn","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,1> services_61{{
    ActorNativeService::reference_assign,
}};
constexpr std::array<ActorDependencyEdge,6> calls_62{{
    {0x80357384,"getLevelProgress","void getLevelProgress(s32* blockCount, f32* blockPart);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x80357384,"getLevelProgress","void getLevelProgress(s32* blockCount, f32* blockPart);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x8035fd70,"Pokemon_RunCleanup","void Pokemon_RunCleanup(GObj* obj);"},
    {0x8035edc8,"Pokemon_StopAuxProc","void Pokemon_StopAuxProc(GObj*);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_62{{
}};
constexpr std::array<ActorNativeService,1> services_62{{
    ActorNativeService::level_progress,
}};
constexpr std::array<std::uint32_t,0> callback_deps_62{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_62{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_62{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_63{{
    {0x8035ed90,"Pokemon_StartAuxProc","void Pokemon_StartAuxProc(GObj*, void (*)(GObj*));"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802c9840,"func_802C9840_7A2DD0","POKEMON_FUNC(func_802C9840_7A2DD0)"},
    {0x802c9990,"func_802C9990_7A2F20","POKEMON_FUNC(func_802C9990_7A2F20)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_63{{
}};
constexpr std::array<ActorNativeService,0> services_63{{
}};
constexpr std::array<std::uint32_t,2> callback_deps_63{{
    0x802c9840,
    0x802c9990,
}};
constexpr std::array<std::uint32_t,0> graph_deps_63{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_63{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_64{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802c9a48,"func_802C9A48_7A2FD8","POKEMON_FUNC(func_802C9A48_7A2FD8)"},
}};
constexpr std::array<ActorResourceDependency,3> resources_64{{
    {0x802d2e98,"D_802D2E98_7AC428","AnimationHeader"},
    {0x802d2ed8,"D_802D2ED8_7AC468","InteractionHandler2"},
    {0x0,"sp20","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,1> services_64{{
    ActorNativeService::graph_copy,
}};
constexpr std::array<std::uint32_t,1> callback_deps_64{{
    0x802c9a48,
}};
constexpr std::array<std::uint32_t,1> graph_deps_64{{
    0x802d2ed8,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_64{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_65{{
    {0x8036148c,"Pokemon_TurnToTarget","void Pokemon_TurnToTarget(GObj*, f32, u32);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_65{{
}};
constexpr std::array<ActorNativeService,0> services_65{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_65{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_65{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_65{{
}};
constexpr std::array<ActorDependencyEdge,6> calls_66{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802c9b0c,"func_802C9B0C_7A309C","POKEMON_FUNC(func_802C9B0C_7A309C)"},
    {0x802c9b78,"func_802C9B78_7A3108","POKEMON_FUNC(func_802C9B78_7A3108)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_66{{
    {0x802d2eac,"D_802D2EAC_7AC43C","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_66{{
}};
constexpr std::array<std::uint32_t,2> callback_deps_66{{
    0x802c9b0c,
    0x802c9b78,
}};
constexpr std::array<std::uint32_t,0> graph_deps_66{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_66{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_67{{
    {0x80362050,"Pokemon_ResetPathPos","void Pokemon_ResetPathPos(GObj*);"},
    {0x803620c8,"Pokemon_FollowPath","void Pokemon_FollowPath(GObj* obj, f32 start, f32 end, f32 dt, f32 yawStep, u32 flags);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_67{{
}};
constexpr std::array<ActorNativeService,0> services_67{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_67{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_67{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_67{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_68{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_68{{
    {0x802d2e48,"D_802D2E48_7AC3D8","AnimationHeader"},
    {0x802d2ef8,"D_802D2EF8_7AC488","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_68{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,0> callback_deps_68{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_68{{
    0x802d2ef8,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_68{{
}};
constexpr std::array<ActorDependencyEdge,11> calls_69{{
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035ed90,"Pokemon_StartAuxProc","void Pokemon_StartAuxProc(GObj*, void (*)(GObj*));"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802c9c84,"func_802C9C84_7A3214","POKEMON_FUNC(func_802C9C84_7A3214)"},
    {0x802c9d00,"func_802C9D00_7A3290","POKEMON_FUNC(func_802C9D00_7A3290)"},
    {0x802c9dc4,"func_802C9DC4_7A3354","POKEMON_FUNC(func_802C9DC4_7A3354)"},
}};
constexpr std::array<ActorResourceDependency,2> resources_69{{
    {0x802d2e5c,"D_802D2E5C_7AC3EC","AnimationHeader"},
    {0x802d2e84,"D_802D2E84_7AC414","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_69{{
}};
constexpr std::array<std::uint32_t,3> callback_deps_69{{
    0x802c9c84,
    0x802c9d00,
    0x802c9dc4,
}};
constexpr std::array<std::uint32_t,0> graph_deps_69{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_69{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_70{{
    {0x8036082c,"Pokemon_JumpAndBounceFromGround","void Pokemon_JumpAndBounceFromGround(GObj* obj, f32 speed, f32 jumpBackwards, f32 arg3);"},
    {0x8000b774,"cmdSendCommand","s32 cmdSendCommand(struct GObj* obj, s32 cmd, struct GObj* source);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_70{{
    {0x802d2b8c,"D_802D2B8C_7AC11C","GObj*"},
}};
constexpr std::array<ActorNativeService,2> services_70{{
    ActorNativeService::jump_bounce,
    ActorNativeService::command,
}};
constexpr std::array<std::uint32_t,0> callback_deps_70{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_70{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_70{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_71{{
    {0x80360fc8,"Pokemon_Turn","bool Pokemon_Turn(DObj* model, f32 targetYaw, f32 turnSpeed);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x8035edc8,"Pokemon_StopAuxProc","void Pokemon_StopAuxProc(GObj*);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_71{{
}};
constexpr std::array<ActorNativeService,3> services_71{{
    ActorNativeService::model,
    ActorNativeService::vector,
    ActorNativeService::vector_read,
}};
constexpr std::array<std::uint32_t,0> callback_deps_71{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_71{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_71{{
}};
constexpr std::array<ActorDependencyEdge,9> calls_72{{
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802c9e60,"func_802C9E60_7A33F0","POKEMON_FUNC(func_802C9E60_7A33F0)"},
    {0x802c9a94,"func_802C9A94_7A3024","POKEMON_FUNC(func_802C9A94_7A3024)"},
}};
constexpr std::array<ActorResourceDependency,2> resources_72{{
    {0x802d2e98,"D_802D2E98_7AC428","AnimationHeader"},
    {0x802d2eac,"D_802D2EAC_7AC43C","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_72{{
}};
constexpr std::array<std::uint32_t,2> callback_deps_72{{
    0x802c9a94,
    0x802c9e60,
}};
constexpr std::array<std::uint32_t,0> graph_deps_72{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_72{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_73{{
    {0x80361b50,"Pokemon_SetTargetPos","void Pokemon_SetTargetPos(GObj*, f32, f32);"},
    {0x80361e58,"Pokemon_RunToTargetPos","void Pokemon_RunToTargetPos(GObj*, f32);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_73{{
}};
constexpr std::array<ActorNativeService,0> services_73{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_73{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_73{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_73{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_74{{
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ecac,"Pokemon_SetStateRandom","void Pokemon_SetStateRandom(GObj* obj, RandomState* nextStates);"},
    {0x802c9f34,"func_802C9F34_7A34C4","POKEMON_FUNC(func_802C9F34_7A34C4)"},
}};
constexpr std::array<ActorResourceDependency,2> resources_74{{
    {0x802d2eac,"D_802D2EAC_7AC43C","AnimationHeader"},
    {0x802d2ec0,"D_802D2EC0_7AC450","RandomState"},
}};
constexpr std::array<ActorNativeService,1> services_74{{
    ActorNativeService::weighted,
}};
constexpr std::array<std::uint32_t,1> callback_deps_74{{
    0x802c9f34,
}};
constexpr std::array<std::uint32_t,0> graph_deps_74{{
}};
constexpr std::array<std::uint32_t,1> weighted_deps_74{{
    0x802d2ec0,
}};
constexpr std::array<ActorDependencyEdge,2> calls_75{{
    {0x80361110,"Pokemon_RunInCircles","void Pokemon_RunInCircles(GObj*, f32, f32, UNK_TYPE);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_75{{
}};
constexpr std::array<ActorNativeService,0> services_75{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_75{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_75{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_75{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_76{{
    {0x80362ee0,"Pokemon_SpawnOnGround","GObj* Pokemon_SpawnOnGround(s32 gObjID, u16 id, struct WorldBlock* roomA, struct WorldBlock* roomB, ObjectSpawn* spawn, PokemonInitData* initData);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_76{{
    {0x802d2f2c,"D_802D2F2C_7AC4BC","PokemonInitData"},
    {0x0,"spawn","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,0> services_76{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_77{{
    {0x0,"HIDE_POKEMON","Source intrinsic/designator: HIDE_POKEMON"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_77{{
    {0x802d2ff8,"D_802D2FF8_7AC588","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_77{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,0> callback_deps_77{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_77{{
    0x802d2ff8,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_77{{
}};
constexpr std::array<ActorDependencyEdge,16> calls_78{{
    {0x0,"GET_TRANSFORM","Source intrinsic/designator: GET_TRANSFORM"},
    {0x8035e1d4,"func_8035E1D4_4FE5E4","GObj* func_8035E1D4_4FE5E4(GObj*);"},
    {0x8035f15c,"Pokemon_ForceAnimation","void Pokemon_ForceAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035fd70,"Pokemon_RunCleanup","void Pokemon_RunCleanup(GObj* obj);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802ca1b0,"func_802CA1B0_7A3740","POKEMON_FUNC(func_802CA1B0_7A3740)"},
}};
constexpr std::array<ActorResourceDependency,2> resources_78{{
    {0x802d2fb4,"D_802D2FB4_7AC544","AnimationHeader"},
    {0x802d2fdc,"D_802D2FDC_7AC56C","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,4> services_78{{
    ActorNativeService::vector,
    ActorNativeService::vector_read,
    ActorNativeService::vector_store,
    ActorNativeService::common_child1,
}};
constexpr std::array<std::uint32_t,1> callback_deps_78{{
    0x802ca1b0,
}};
constexpr std::array<std::uint32_t,0> graph_deps_78{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_78{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_79{{
    {0x80362050,"Pokemon_ResetPathPos","void Pokemon_ResetPathPos(GObj*);"},
    {0x803620c8,"Pokemon_FollowPath","void Pokemon_FollowPath(GObj* obj, f32 start, f32 end, f32 dt, f32 yawStep, u32 flags);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_79{{
}};
constexpr std::array<ActorNativeService,0> services_79{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_79{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_79{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_79{{
}};
constexpr std::array<ActorDependencyEdge,0> calls_80{{
}};
constexpr std::array<ActorResourceDependency,0> resources_80{{
}};
constexpr std::array<ActorNativeService,0> services_80{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_81{{
    {0x80362ee0,"Pokemon_SpawnOnGround","GObj* Pokemon_SpawnOnGround(s32 gObjID, u16 id, struct WorldBlock* roomA, struct WorldBlock* roomB, ObjectSpawn* spawn, PokemonInitData* initData);"},
}};
constexpr std::array<ActorResourceDependency,3> resources_81{{
    {0x802d2ff0,"D_802D2FF0_7AC580","GObj*"},
    {0x802d302c,"D_802D302C_7AC5BC","PokemonInitData"},
    {0x0,"spawn","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,1> services_81{{
    ActorNativeService::reference_assign,
}};
constexpr std::array<ActorDependencyEdge,3> calls_82{{
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ecac,"Pokemon_SetStateRandom","void Pokemon_SetStateRandom(GObj* obj, RandomState* nextStates);"},
    {0x802ca574,"func_802CA574_7A3B04","POKEMON_FUNC(func_802CA574_7A3B04)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_82{{
    {0x802d30c0,"D_802D30C0_7AC650","RandomState"},
}};
constexpr std::array<ActorNativeService,1> services_82{{
    ActorNativeService::weighted,
}};
constexpr std::array<std::uint32_t,1> callback_deps_82{{
    0x802ca574,
}};
constexpr std::array<std::uint32_t,0> graph_deps_82{{
}};
constexpr std::array<std::uint32_t,1> weighted_deps_82{{
    0x802d30c0,
}};
constexpr std::array<ActorDependencyEdge,3> calls_83{{
    {0x8035f15c,"Pokemon_ForceAnimation","void Pokemon_ForceAnimation(GObj*, AnimationHeader*);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802ca364,"func_802CA364_7A38F4","POKEMON_FUNC(func_802CA364_7A38F4)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_83{{
    {0x802d3084,"D_802D3084_7AC614","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_83{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_83{{
    0x802ca364,
}};
constexpr std::array<std::uint32_t,0> graph_deps_83{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_83{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_84{{
    {0x8035f15c,"Pokemon_ForceAnimation","void Pokemon_ForceAnimation(GObj*, AnimationHeader*);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802ca364,"func_802CA364_7A38F4","POKEMON_FUNC(func_802CA364_7A38F4)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_84{{
    {0x802d3098,"D_802D3098_7AC628","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_84{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_84{{
    0x802ca364,
}};
constexpr std::array<std::uint32_t,0> graph_deps_84{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_84{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_85{{
    {0x8035f15c,"Pokemon_ForceAnimation","void Pokemon_ForceAnimation(GObj*, AnimationHeader*);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802ca364,"func_802CA364_7A38F4","POKEMON_FUNC(func_802CA364_7A38F4)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_85{{
    {0x802d30ac,"D_802D30AC_7AC63C","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_85{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_85{{
    0x802ca364,
}};
constexpr std::array<std::uint32_t,0> graph_deps_85{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_85{{
}};
constexpr std::array<ActorDependencyEdge,8> calls_86{{
    {0x80019e14,"randRange","Source intrinsic/designator: randRange"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035fd70,"Pokemon_RunCleanup","void Pokemon_RunCleanup(GObj* obj);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802ca434,"func_802CA434_7A39C4","POKEMON_FUNC(func_802CA434_7A39C4)"},
    {0x802ca5b4,"func_802CA5B4_7A3B44","POKEMON_FUNC(func_802CA5B4_7A3B44)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_86{{
    {0x802d2980,"D_802D2980_7ABF10","GObj*"},
}};
constexpr std::array<ActorNativeService,5> services_86{{
    ActorNativeService::reference_assign,
    ActorNativeService::model,
    ActorNativeService::vector,
    ActorNativeService::vector_store,
    ActorNativeService::rng_range,
}};
constexpr std::array<std::uint32_t,2> callback_deps_86{{
    0x802ca434,
    0x802ca5b4,
}};
constexpr std::array<std::uint32_t,0> graph_deps_86{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_86{{
}};
constexpr std::array<ActorDependencyEdge,7> calls_87{{
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x800a5e98,"fx_getPosVelDObj","void fx_getPosVelDObj(Vec3f* pos, Vec3f* vel, DObj* dobj);"},
    {0x800e41d8,"getGroundAt","s32 getGroundAt(f32 arg0, f32 arg1, GroundResult* arg2);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x8035e174,"func_8035E174_4FE584","void func_8035E174_4FE584(GObj* obj, Vec3f*);"},
    {0x8035e264,"func_8035E264_4FE674","void func_8035E264_4FE674(GObj* obj, Vec3f*);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_87{{
}};
constexpr std::array<ActorNativeService,5> services_87{{
    ActorNativeService::model,
    ActorNativeService::node_path,
    ActorNativeService::ground,
    ActorNativeService::node_world,
    ActorNativeService::misc_object_get,
}};
constexpr std::array<std::uint32_t,0> callback_deps_87{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_87{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_87{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_88{{
    {0x8035e238,"func_8035E238_4FE648","GObj* func_8035E238_4FE648(GObj* obj);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802ca5b4,"func_802CA5B4_7A3B44","POKEMON_FUNC(func_802CA5B4_7A3B44)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_88{{
}};
constexpr std::array<ActorNativeService,2> services_88{{
    ActorNativeService::common_child2,
    ActorNativeService::misc_object_set,
}};
constexpr std::array<std::uint32_t,1> callback_deps_88{{
    0x802ca5b4,
}};
constexpr std::array<std::uint32_t,0> graph_deps_88{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_88{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_89{{
    {0x80019e14,"randRange","Source intrinsic/designator: randRange"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ecac,"Pokemon_SetStateRandom","void Pokemon_SetStateRandom(GObj* obj, RandomState* nextStates);"},
    {0x802ca65c,"func_802CA65C_7A3BEC","POKEMON_FUNC(func_802CA65C_7A3BEC)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_89{{
    {0x802d30c0,"D_802D30C0_7AC650","RandomState"},
}};
constexpr std::array<ActorNativeService,2> services_89{{
    ActorNativeService::weighted,
    ActorNativeService::rng_range,
}};
constexpr std::array<std::uint32_t,1> callback_deps_89{{
    0x802ca65c,
}};
constexpr std::array<std::uint32_t,0> graph_deps_89{{
}};
constexpr std::array<std::uint32_t,1> weighted_deps_89{{
    0x802d30c0,
}};
constexpr std::array<ActorDependencyEdge,2> calls_90{{
    {0x8035fd70,"Pokemon_RunCleanup","void Pokemon_RunCleanup(GObj* obj);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_90{{
}};
constexpr std::array<ActorNativeService,0> services_90{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_90{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_90{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_90{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_91{{
    {0x80362ee0,"Pokemon_SpawnOnGround","GObj* Pokemon_SpawnOnGround(s32 gObjID, u16 id, struct WorldBlock* roomA, struct WorldBlock* roomB, ObjectSpawn* spawn, PokemonInitData* initData);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_91{{
    {0x802d3100,"D_802D3100_7AC690","PokemonInitData"},
    {0x0,"spawn","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,0> services_91{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_92{{
    {0x80357384,"getLevelProgress","void getLevelProgress(s32* blockCount, f32* blockPart);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x8035fd70,"Pokemon_RunCleanup","void Pokemon_RunCleanup(GObj* obj);"},
    {0x8035edc8,"Pokemon_StopAuxProc","void Pokemon_StopAuxProc(GObj*);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_92{{
}};
constexpr std::array<ActorNativeService,1> services_92{{
    ActorNativeService::level_progress,
}};
constexpr std::array<std::uint32_t,0> callback_deps_92{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_92{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_92{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_93{{
    {0x8035ed90,"Pokemon_StartAuxProc","void Pokemon_StartAuxProc(GObj*, void (*)(GObj*));"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802ca6c0,"func_802CA6C0_7A3C50","POKEMON_FUNC(func_802CA6C0_7A3C50)"},
    {0x802ca7a0,"func_802CA7A0_7A3D30","POKEMON_FUNC(func_802CA7A0_7A3D30)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_93{{
}};
constexpr std::array<ActorNativeService,0> services_93{{
}};
constexpr std::array<std::uint32_t,2> callback_deps_93{{
    0x802ca6c0,
    0x802ca7a0,
}};
constexpr std::array<std::uint32_t,0> graph_deps_93{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_93{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_94{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802ca858,"func_802CA858_7A3DE8","POKEMON_FUNC(func_802CA858_7A3DE8)"},
}};
constexpr std::array<ActorResourceDependency,3> resources_94{{
    {0x802d315c,"D_802D315C_7AC6EC","AnimationHeader"},
    {0x802d3294,"D_802D3294_7AC824","InteractionHandler2"},
    {0x0,"sp20","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,1> services_94{{
    ActorNativeService::graph_copy,
}};
constexpr std::array<std::uint32_t,1> callback_deps_94{{
    0x802ca858,
}};
constexpr std::array<std::uint32_t,1> graph_deps_94{{
    0x802d3294,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_94{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_95{{
    {0x8036148c,"Pokemon_TurnToTarget","void Pokemon_TurnToTarget(GObj*, f32, u32);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_95{{
}};
constexpr std::array<ActorNativeService,0> services_95{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_95{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_95{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_95{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_96{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802ca8f8,"func_802CA8F8_7A3E88","POKEMON_FUNC(func_802CA8F8_7A3E88)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_96{{
    {0x802d3210,"D_802D3210_7AC7A0","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_96{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_96{{
    0x802ca8f8,
}};
constexpr std::array<std::uint32_t,0> graph_deps_96{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_96{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_97{{
    {0x0,"HIDE_POKEMON","Source intrinsic/designator: HIDE_POKEMON"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_97{{
    {0x802d32b4,"D_802D32B4_7AC844","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_97{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,0> callback_deps_97{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_97{{
    0x802d32b4,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_97{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_98{{
    {0x0,"SHOW_POKEMON","Source intrinsic/designator: SHOW_POKEMON"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802ca9bc,"func_802CA9BC_7A3F4C","POKEMON_FUNC(func_802CA9BC_7A3F4C)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_98{{
    {0x802d31fc,"D_802D31FC_7AC78C","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_98{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_98{{
    0x802ca9bc,
}};
constexpr std::array<std::uint32_t,0> graph_deps_98{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_98{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_99{{
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ecac,"Pokemon_SetStateRandom","void Pokemon_SetStateRandom(GObj* obj, RandomState* nextStates);"},
}};
constexpr std::array<ActorResourceDependency,3> resources_99{{
    {0x802d315c,"D_802D315C_7AC6EC","AnimationHeader"},
    {0x802d3224,"D_802D3224_7AC7B4","InteractionHandler"},
    {0x802d3274,"D_802D3274_7AC804","RandomState"},
}};
constexpr std::array<ActorNativeService,2> services_99{{
    ActorNativeService::graph,
    ActorNativeService::weighted,
}};
constexpr std::array<std::uint32_t,0> callback_deps_99{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_99{{
    0x802d3224,
}};
constexpr std::array<std::uint32_t,1> weighted_deps_99{{
    0x802d3274,
}};
constexpr std::array<ActorDependencyEdge,7> calls_100{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ecac,"Pokemon_SetStateRandom","void Pokemon_SetStateRandom(GObj* obj, RandomState* nextStates);"},
    {0x802caad0,"func_802CAAD0_7A4060","POKEMON_FUNC(func_802CAAD0_7A4060)"},
    {0x802ca9bc,"func_802CA9BC_7A3F4C","POKEMON_FUNC(func_802CA9BC_7A3F4C)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_100{{
    {0x802d3274,"D_802D3274_7AC804","RandomState"},
}};
constexpr std::array<ActorNativeService,2> services_100{{
    ActorNativeService::graph,
    ActorNativeService::weighted,
}};
constexpr std::array<std::uint32_t,2> callback_deps_100{{
    0x802ca9bc,
    0x802caad0,
}};
constexpr std::array<std::uint32_t,0> graph_deps_100{{
}};
constexpr std::array<std::uint32_t,1> weighted_deps_100{{
    0x802d3274,
}};
constexpr std::array<ActorDependencyEdge,2> calls_101{{
    {0x80361110,"Pokemon_RunInCircles","void Pokemon_RunInCircles(GObj*, f32, f32, UNK_TYPE);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_101{{
}};
constexpr std::array<ActorNativeService,0> services_101{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_101{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_101{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_101{{
}};
constexpr std::array<ActorDependencyEdge,7> calls_102{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ecac,"Pokemon_SetStateRandom","void Pokemon_SetStateRandom(GObj* obj, RandomState* nextStates);"},
    {0x802cabd0,"func_802CABD0_7A4160","POKEMON_FUNC(func_802CABD0_7A4160)"},
    {0x802ca9bc,"func_802CA9BC_7A3F4C","POKEMON_FUNC(func_802CA9BC_7A3F4C)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_102{{
    {0x802d3274,"D_802D3274_7AC804","RandomState"},
}};
constexpr std::array<ActorNativeService,2> services_102{{
    ActorNativeService::graph,
    ActorNativeService::weighted,
}};
constexpr std::array<std::uint32_t,2> callback_deps_102{{
    0x802ca9bc,
    0x802cabd0,
}};
constexpr std::array<std::uint32_t,0> graph_deps_102{{
}};
constexpr std::array<std::uint32_t,1> weighted_deps_102{{
    0x802d3274,
}};
constexpr std::array<ActorDependencyEdge,2> calls_103{{
    {0x80361110,"Pokemon_RunInCircles","void Pokemon_RunInCircles(GObj*, f32, f32, UNK_TYPE);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_103{{
}};
constexpr std::array<ActorNativeService,0> services_103{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_103{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_103{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_103{{
}};
constexpr std::array<ActorDependencyEdge,9> calls_104{{
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802ca9bc,"func_802CA9BC_7A3F4C","POKEMON_FUNC(func_802CA9BC_7A3F4C)"},
}};
constexpr std::array<ActorResourceDependency,4> resources_104{{
    {0x802d31ac,"D_802D31AC_7AC73C","AnimationHeader"},
    {0x802d31c0,"D_802D31C0_7AC750","AnimationHeader"},
    {0x802d31d4,"D_802D31D4_7AC764","AnimationHeader"},
    {0x802d32d4,"D_802D32D4_7AC864","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_104{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,1> callback_deps_104{{
    0x802ca9bc,
}};
constexpr std::array<std::uint32_t,1> graph_deps_104{{
    0x802d32d4,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_104{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_105{{
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802ca9bc,"func_802CA9BC_7A3F4C","POKEMON_FUNC(func_802CA9BC_7A3F4C)"},
}};
constexpr std::array<ActorResourceDependency,2> resources_105{{
    {0x802d3198,"D_802D3198_7AC728","AnimationHeader"},
    {0x802d3304,"D_802D3304_7AC894","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_105{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,1> callback_deps_105{{
    0x802ca9bc,
}};
constexpr std::array<std::uint32_t,1> graph_deps_105{{
    0x802d3304,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_105{{
}};
constexpr std::array<ActorDependencyEdge,8> calls_106{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cadec,"func_802CADEC_7A437C","POKEMON_FUNC(func_802CADEC_7A437C)"},
    {0x802ca9bc,"func_802CA9BC_7A3F4C","POKEMON_FUNC(func_802CA9BC_7A3F4C)"},
    {0x802cae70,"func_802CAE70_7A4400","POKEMON_FUNC(func_802CAE70_7A4400)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_106{{
}};
constexpr std::array<ActorNativeService,1> services_106{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,3> callback_deps_106{{
    0x802ca9bc,
    0x802cadec,
    0x802cae70,
}};
constexpr std::array<std::uint32_t,0> graph_deps_106{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_106{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_107{{
    {0x8036148c,"Pokemon_TurnToTarget","void Pokemon_TurnToTarget(GObj*, f32, u32);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x80361748,"Pokemon_RunToTarget","void Pokemon_RunToTarget(GObj*, f32, f32, u32);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_107{{
    {0x802d3184,"D_802D3184_7AC714","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_107{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_107{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_107{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_107{{
}};
constexpr std::array<ActorDependencyEdge,7> calls_108{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ecac,"Pokemon_SetStateRandom","void Pokemon_SetStateRandom(GObj* obj, RandomState* nextStates);"},
    {0x802caf04,"func_802CAF04_7A4494","POKEMON_FUNC(func_802CAF04_7A4494)"},
    {0x802cae70,"func_802CAE70_7A4400","POKEMON_FUNC(func_802CAE70_7A4400)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_108{{
    {0x802d3274,"D_802D3274_7AC804","RandomState"},
}};
constexpr std::array<ActorNativeService,2> services_108{{
    ActorNativeService::graph,
    ActorNativeService::weighted,
}};
constexpr std::array<std::uint32_t,2> callback_deps_108{{
    0x802cae70,
    0x802caf04,
}};
constexpr std::array<std::uint32_t,0> graph_deps_108{{
}};
constexpr std::array<std::uint32_t,1> weighted_deps_108{{
    0x802d3274,
}};
constexpr std::array<ActorDependencyEdge,2> calls_109{{
    {0x8036148c,"Pokemon_TurnToTarget","void Pokemon_TurnToTarget(GObj*, f32, u32);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_109{{
}};
constexpr std::array<ActorNativeService,0> services_109{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_109{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_109{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_109{{
}};
constexpr std::array<ActorDependencyEdge,6> calls_110{{
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802ca9bc,"func_802CA9BC_7A3F4C","POKEMON_FUNC(func_802CA9BC_7A3F4C)"},
    {0x802cad54,"func_802CAD54_7A42E4","POKEMON_FUNC(func_802CAD54_7A42E4)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_110{{
}};
constexpr std::array<ActorNativeService,1> services_110{{
    ActorNativeService::graph_copy,
}};
constexpr std::array<std::uint32_t,2> callback_deps_110{{
    0x802ca9bc,
    0x802cad54,
}};
constexpr std::array<std::uint32_t,0> graph_deps_110{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_110{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_111{{
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cad54,"func_802CAD54_7A42E4","POKEMON_FUNC(func_802CAD54_7A42E4)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_111{{
}};
constexpr std::array<ActorNativeService,0> services_111{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_111{{
    0x802cad54,
}};
constexpr std::array<std::uint32_t,0> graph_deps_111{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_111{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_112{{
    {0x80362ee0,"Pokemon_SpawnOnGround","GObj* Pokemon_SpawnOnGround(s32 gObjID, u16 id, struct WorldBlock* roomA, struct WorldBlock* roomB, ObjectSpawn* spawn, PokemonInitData* initData);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_112{{
    {0x802d33f8,"D_802D33F8_7AC988","PokemonInitData"},
    {0x0,"spawn","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,0> services_112{{
}};
constexpr std::array<ActorDependencyEdge,8> calls_113{{
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cb8d8,"func_802CB8D8_7A4E68","POKEMON_FUNC(func_802CB8D8_7A4E68)"},
    {0x802cc068,"func_802CC068_7A55F8","POKEMON_FUNC(func_802CC068_7A55F8)"},
    {0x802cc2b4,"func_802CC2B4_7A5844","POKEMON_FUNC(func_802CC2B4_7A5844)"},
    {0x802cb110,"func_802CB110_7A46A0","POKEMON_FUNC(func_802CB110_7A46A0)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_113{{
}};
constexpr std::array<ActorNativeService,0> services_113{{
}};
constexpr std::array<std::uint32_t,4> callback_deps_113{{
    0x802cb110,
    0x802cb8d8,
    0x802cc068,
    0x802cc2b4,
}};
constexpr std::array<std::uint32_t,0> graph_deps_113{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_113{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_114{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ecac,"Pokemon_SetStateRandom","void Pokemon_SetStateRandom(GObj* obj, RandomState* nextStates);"},
}};
constexpr std::array<ActorResourceDependency,3> resources_114{{
    {0x802d3450,"D_802D3450_7AC9E0","AnimationHeader"},
    {0x802d3568,"D_802D3568_7ACAF8","InteractionHandler"},
    {0x802d3608,"D_802D3608_7ACB98","RandomState"},
}};
constexpr std::array<ActorNativeService,2> services_114{{
    ActorNativeService::graph,
    ActorNativeService::weighted,
}};
constexpr std::array<std::uint32_t,0> callback_deps_114{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_114{{
    0x802d3568,
}};
constexpr std::array<std::uint32_t,1> weighted_deps_114{{
    0x802d3608,
}};
constexpr std::array<ActorDependencyEdge,7> calls_115{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ecac,"Pokemon_SetStateRandom","void Pokemon_SetStateRandom(GObj* obj, RandomState* nextStates);"},
    {0x802cb238,"func_802CB238_7A47C8","POKEMON_FUNC(func_802CB238_7A47C8)"},
    {0x802cb110,"func_802CB110_7A46A0","POKEMON_FUNC(func_802CB110_7A46A0)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_115{{
    {0x802d3608,"D_802D3608_7ACB98","RandomState"},
}};
constexpr std::array<ActorNativeService,2> services_115{{
    ActorNativeService::graph,
    ActorNativeService::weighted,
}};
constexpr std::array<std::uint32_t,2> callback_deps_115{{
    0x802cb110,
    0x802cb238,
}};
constexpr std::array<std::uint32_t,0> graph_deps_115{{
}};
constexpr std::array<std::uint32_t,1> weighted_deps_115{{
    0x802d3608,
}};
constexpr std::array<ActorDependencyEdge,2> calls_116{{
    {0x80361110,"Pokemon_RunInCircles","void Pokemon_RunInCircles(GObj*, f32, f32, UNK_TYPE);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_116{{
}};
constexpr std::array<ActorNativeService,0> services_116{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_116{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_116{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_116{{
}};
constexpr std::array<ActorDependencyEdge,7> calls_117{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ecac,"Pokemon_SetStateRandom","void Pokemon_SetStateRandom(GObj* obj, RandomState* nextStates);"},
    {0x802cb338,"func_802CB338_7A48C8","POKEMON_FUNC(func_802CB338_7A48C8)"},
    {0x802cb110,"func_802CB110_7A46A0","POKEMON_FUNC(func_802CB110_7A46A0)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_117{{
    {0x802d3608,"D_802D3608_7ACB98","RandomState"},
}};
constexpr std::array<ActorNativeService,2> services_117{{
    ActorNativeService::graph,
    ActorNativeService::weighted,
}};
constexpr std::array<std::uint32_t,2> callback_deps_117{{
    0x802cb110,
    0x802cb338,
}};
constexpr std::array<std::uint32_t,0> graph_deps_117{{
}};
constexpr std::array<std::uint32_t,1> weighted_deps_117{{
    0x802d3608,
}};
constexpr std::array<ActorDependencyEdge,2> calls_118{{
    {0x80361110,"Pokemon_RunInCircles","void Pokemon_RunInCircles(GObj*, f32, f32, UNK_TYPE);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_118{{
}};
constexpr std::array<ActorNativeService,0> services_118{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_118{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_118{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_118{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_119{{
    {0x8035f15c,"Pokemon_ForceAnimation","void Pokemon_ForceAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ecac,"Pokemon_SetStateRandom","void Pokemon_SetStateRandom(GObj* obj, RandomState* nextStates);"},
}};
constexpr std::array<ActorResourceDependency,3> resources_119{{
    {0x802d3464,"D_802D3464_7AC9F4","AnimationHeader"},
    {0x802d3568,"D_802D3568_7ACAF8","InteractionHandler"},
    {0x802d3608,"D_802D3608_7ACB98","RandomState"},
}};
constexpr std::array<ActorNativeService,2> services_119{{
    ActorNativeService::graph,
    ActorNativeService::weighted,
}};
constexpr std::array<std::uint32_t,0> callback_deps_119{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_119{{
    0x802d3568,
}};
constexpr std::array<std::uint32_t,1> weighted_deps_119{{
    0x802d3608,
}};
constexpr std::array<ActorDependencyEdge,4> calls_120{{
    {0x8035f15c,"Pokemon_ForceAnimation","void Pokemon_ForceAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ecac,"Pokemon_SetStateRandom","void Pokemon_SetStateRandom(GObj* obj, RandomState* nextStates);"},
}};
constexpr std::array<ActorResourceDependency,3> resources_120{{
    {0x802d3478,"D_802D3478_7ACA08","AnimationHeader"},
    {0x802d3568,"D_802D3568_7ACAF8","InteractionHandler"},
    {0x802d3608,"D_802D3608_7ACB98","RandomState"},
}};
constexpr std::array<ActorNativeService,2> services_120{{
    ActorNativeService::graph,
    ActorNativeService::weighted,
}};
constexpr std::array<std::uint32_t,0> callback_deps_120{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_120{{
    0x802d3568,
}};
constexpr std::array<std::uint32_t,1> weighted_deps_120{{
    0x802d3608,
}};
constexpr std::array<ActorDependencyEdge,12> calls_121{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cb550,"func_802CB550_7A4AE0","POKEMON_FUNC(func_802CB550_7A4AE0)"},
    {0x802cb110,"func_802CB110_7A46A0","POKEMON_FUNC(func_802CB110_7A46A0)"},
}};
constexpr std::array<ActorResourceDependency,4> resources_121{{
    {0x802d34c8,"D_802D34C8_7ACA58","AnimationHeader"},
    {0x802d34dc,"D_802D34DC_7ACA6C","AnimationHeader"},
    {0x802d34f0,"D_802D34F0_7ACA80","AnimationHeader"},
    {0x802d363c,"D_802D363C_7ACBCC","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_121{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,2> callback_deps_121{{
    0x802cb110,
    0x802cb550,
}};
constexpr std::array<std::uint32_t,1> graph_deps_121{{
    0x802d363c,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_121{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_122{{
    {0x8036194c,"Pokemon_RunAwayFromTarget","void Pokemon_RunAwayFromTarget(GObj* obj, f32 targetDistance, f32 turnSpeed, u32 flags);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_122{{
}};
constexpr std::array<ActorNativeService,0> services_122{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_122{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_122{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_122{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_123{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cb110,"func_802CB110_7A46A0","POKEMON_FUNC(func_802CB110_7A46A0)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_123{{
    {0x802d34b4,"D_802D34B4_7ACA44","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_123{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_123{{
    0x802cb110,
}};
constexpr std::array<std::uint32_t,0> graph_deps_123{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_123{{
}};
constexpr std::array<ActorDependencyEdge,13> calls_124{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8036010c,"Pokemon_EatApple","void Pokemon_EatApple(GObj*);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cb6f8,"func_802CB6F8_7A4C88","POKEMON_FUNC(func_802CB6F8_7A4C88)"},
    {0x802cb110,"func_802CB110_7A46A0","POKEMON_FUNC(func_802CB110_7A46A0)"},
    {0x802cb79c,"func_802CB79C_7A4D2C","POKEMON_FUNC(func_802CB79C_7A4D2C)"},
    {0x802cb110,"func_802CB110_7A46A0","POKEMON_FUNC(func_802CB110_7A46A0)"},
}};
constexpr std::array<ActorResourceDependency,2> resources_124{{
    {0x802d3518,"D_802D3518_7ACAA8","AnimationHeader"},
    {0x802d366c,"D_802D366C_7ACBFC","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_124{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,3> callback_deps_124{{
    0x802cb110,
    0x802cb6f8,
    0x802cb79c,
}};
constexpr std::array<std::uint32_t,1> graph_deps_124{{
    0x802d366c,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_124{{
}};
constexpr std::array<ActorDependencyEdge,6> calls_125{{
    {0x8036148c,"Pokemon_TurnToTarget","void Pokemon_TurnToTarget(GObj*, f32, u32);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fc54,"Pokemon_WaitForFlagNoInteraction","void Pokemon_WaitForFlagNoInteraction(GObj*, u32);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x80361748,"Pokemon_RunToTarget","void Pokemon_RunToTarget(GObj*, f32, f32, u32);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_125{{
    {0x802d34a0,"D_802D34A0_7ACA30","AnimationHeader"},
    {0x802d3504,"D_802D3504_7ACA94","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_125{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_125{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_125{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_125{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_126{{
    {0x8036148c,"Pokemon_TurnToTarget","void Pokemon_TurnToTarget(GObj*, f32, u32);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_126{{
}};
constexpr std::array<ActorNativeService,0> services_126{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_126{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_126{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_126{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_127{{
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cb614,"func_802CB614_7A4BA4","POKEMON_FUNC(func_802CB614_7A4BA4)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_127{{
}};
constexpr std::array<ActorNativeService,0> services_127{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_127{{
    0x802cb614,
}};
constexpr std::array<std::uint32_t,0> graph_deps_127{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_127{{
}};
constexpr std::array<ActorDependencyEdge,6> calls_128{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cb88c,"func_802CB88C_7A4E1C","POKEMON_FUNC(func_802CB88C_7A4E1C)"},
    {0x802cb110,"func_802CB110_7A46A0","POKEMON_FUNC(func_802CB110_7A46A0)"},
}};
constexpr std::array<ActorResourceDependency,2> resources_128{{
    {0x802d348c,"D_802D348C_7ACA1C","AnimationHeader"},
    {0x802d369c,"D_802D369C_7ACC2C","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_128{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,2> callback_deps_128{{
    0x802cb110,
    0x802cb88c,
}};
constexpr std::array<std::uint32_t,1> graph_deps_128{{
    0x802d369c,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_128{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_129{{
    {0x8036148c,"Pokemon_TurnToTarget","void Pokemon_TurnToTarget(GObj*, f32, u32);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_129{{
}};
constexpr std::array<ActorNativeService,0> services_129{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_129{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_129{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_129{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_130{{
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_130{{
    {0x802d370c,"D_802D370C_7ACC9C","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_130{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,0> callback_deps_130{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_130{{
    0x802d370c,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_130{{
}};
constexpr std::array<ActorDependencyEdge,8> calls_131{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035ed90,"Pokemon_StartAuxProc","void Pokemon_StartAuxProc(GObj*, void (*)(GObj*));"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cba24,"func_802CBA24_7A4FB4","POKEMON_FUNC(func_802CBA24_7A4FB4)"},
    {0x802cba90,"func_802CBA90_7A5020","POKEMON_FUNC(func_802CBA90_7A5020)"},
    {0x802cb9bc,"func_802CB9BC_7A4F4C","POKEMON_FUNC(func_802CB9BC_7A4F4C)"},
}};
constexpr std::array<ActorResourceDependency,2> resources_131{{
    {0x802d3540,"D_802D3540_7ACAD0","AnimationHeader"},
    {0x802d36cc,"D_802D36CC_7ACC5C","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_131{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,3> callback_deps_131{{
    0x802cb9bc,
    0x802cba24,
    0x802cba90,
}};
constexpr std::array<std::uint32_t,1> graph_deps_131{{
    0x802d36cc,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_131{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_132{{
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035fd70,"Pokemon_RunCleanup","void Pokemon_RunCleanup(GObj* obj);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cbbdc,"func_802CBBDC_7A516C","POKEMON_FUNC(func_802CBBDC_7A516C)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_132{{
}};
constexpr std::array<ActorNativeService,0> services_132{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_132{{
    0x802cbbdc,
}};
constexpr std::array<std::uint32_t,0> graph_deps_132{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_132{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_133{{
    {0x80362050,"Pokemon_ResetPathPos","void Pokemon_ResetPathPos(GObj*);"},
    {0x803620c8,"Pokemon_FollowPath","void Pokemon_FollowPath(GObj* obj, f32 start, f32 end, f32 dt, f32 yawStep, u32 flags);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_133{{
}};
constexpr std::array<ActorNativeService,0> services_133{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_133{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_133{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_133{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_134{{
    {0x800e41d8,"getGroundAt","s32 getGroundAt(f32 arg0, f32 arg1, GroundResult* arg2);"},
    {0x80032a20,"__sinf","f32 __sinf(f32);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x8035edc8,"Pokemon_StopAuxProc","void Pokemon_StopAuxProc(GObj*);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_134{{
}};
constexpr std::array<ActorNativeService,5> services_134{{
    ActorNativeService::model,
    ActorNativeService::vector,
    ActorNativeService::vector_read,
    ActorNativeService::vector_store,
    ActorNativeService::ground,
}};
constexpr std::array<std::uint32_t,0> callback_deps_134{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_134{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_134{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_135{{
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_135{{
}};
constexpr std::array<ActorNativeService,4> services_135{{
    ActorNativeService::model,
    ActorNativeService::vector,
    ActorNativeService::vector_read,
    ActorNativeService::vector_store,
}};
constexpr std::array<std::uint32_t,0> callback_deps_135{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_135{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_135{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_136{{
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035fd70,"Pokemon_RunCleanup","void Pokemon_RunCleanup(GObj* obj);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cbcdc,"func_802CBCDC_7A526C","POKEMON_FUNC(func_802CBCDC_7A526C)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_136{{
}};
constexpr std::array<ActorNativeService,0> services_136{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_136{{
    0x802cbcdc,
}};
constexpr std::array<std::uint32_t,0> graph_deps_136{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_136{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_137{{
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_137{{
}};
constexpr std::array<ActorNativeService,4> services_137{{
    ActorNativeService::model,
    ActorNativeService::vector,
    ActorNativeService::vector_read,
    ActorNativeService::vector_store,
}};
constexpr std::array<std::uint32_t,0> callback_deps_137{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_137{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_137{{
}};
constexpr std::array<ActorDependencyEdge,7> calls_138{{
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cbe14,"func_802CBE14_7A53A4","POKEMON_FUNC(func_802CBE14_7A53A4)"},
    {0x802cbf64,"func_802CBF64_7A54F4","POKEMON_FUNC(func_802CBF64_7A54F4)"},
    {0x802cbe9c,"func_802CBE9C_7A542C","POKEMON_FUNC(func_802CBE9C_7A542C)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_138{{
}};
constexpr std::array<ActorNativeService,1> services_138{{
    ActorNativeService::forbidden_ground,
}};
constexpr std::array<std::uint32_t,3> callback_deps_138{{
    0x802cbe14,
    0x802cbe9c,
    0x802cbf64,
}};
constexpr std::array<std::uint32_t,0> graph_deps_138{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_138{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_139{{
    {0x80019abc,"atan2f","Source intrinsic/designator: atan2f"},
    {0x80360ab8,"Pokemon_FallDownOnGround","void Pokemon_FallDownOnGround(GObj* obj, f32 acceleration, s32);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_139{{
}};
constexpr std::array<ActorNativeService,1> services_139{{
    ActorNativeService::item_direction,
}};
constexpr std::array<std::uint32_t,0> callback_deps_139{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_139{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_139{{
}};
constexpr std::array<ActorDependencyEdge,6> calls_140{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cbf18,"func_802CBF18_7A54A8","POKEMON_FUNC(func_802CBF18_7A54A8)"},
    {0x802cb110,"func_802CB110_7A46A0","POKEMON_FUNC(func_802CB110_7A46A0)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_140{{
    {0x802d3554,"D_802D3554_7ACAE4","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,1> services_140{{
    ActorNativeService::forbidden_ground,
}};
constexpr std::array<std::uint32_t,2> callback_deps_140{{
    0x802cb110,
    0x802cbf18,
}};
constexpr std::array<std::uint32_t,0> graph_deps_140{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_140{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_141{{
    {0x8036148c,"Pokemon_TurnToTarget","void Pokemon_TurnToTarget(GObj*, f32, u32);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_141{{
}};
constexpr std::array<ActorNativeService,0> services_141{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_141{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_141{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_141{{
}};
constexpr std::array<ActorDependencyEdge,7> calls_142{{
    {0x803667c0,"EnvSound_PlaySound","void EnvSound_PlaySound(GObj* obj, u8 category, s32 soundID);"},
    {0x8035e1d4,"func_8035E1D4_4FE5E4","GObj* func_8035E1D4_4FE5E4(GObj*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035fd70,"Pokemon_RunCleanup","void Pokemon_RunCleanup(GObj* obj);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cbfdc,"func_802CBFDC_7A556C","POKEMON_FUNC(func_802CBFDC_7A556C)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_142{{
}};
constexpr std::array<ActorNativeService,2> services_142{{
    ActorNativeService::environment_sound,
    ActorNativeService::common_child1,
}};
constexpr std::array<std::uint32_t,1> callback_deps_142{{
    0x802cbfdc,
}};
constexpr std::array<std::uint32_t,0> graph_deps_142{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_142{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_143{{
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_143{{
}};
constexpr std::array<ActorNativeService,4> services_143{{
    ActorNativeService::model,
    ActorNativeService::vector,
    ActorNativeService::vector_read,
    ActorNativeService::vector_store,
}};
constexpr std::array<std::uint32_t,0> callback_deps_143{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_143{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_143{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_144{{
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cc08c,"func_802CC08C_7A561C","POKEMON_FUNC(func_802CC08C_7A561C)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_144{{
}};
constexpr std::array<ActorNativeService,0> services_144{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_144{{
    0x802cc08c,
}};
constexpr std::array<std::uint32_t,0> graph_deps_144{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_144{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_145{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,3> resources_145{{
    {0x802d3450,"D_802D3450_7AC9E0","AnimationHeader"},
    {0x802d3638,"D_802D3638_7ACBC8","f32"},
    {0x802d372c,"D_802D372C_7ACCBC","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,2> services_145{{
    ActorNativeService::graph,
    ActorNativeService::forbidden_ground,
}};
constexpr std::array<std::uint32_t,0> callback_deps_145{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_145{{
    0x802d372c,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_145{{
}};
constexpr std::array<ActorDependencyEdge,13> calls_146{{
    {0x8036148c,"Pokemon_TurnToTarget","void Pokemon_TurnToTarget(GObj*, f32, u32);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035ed90,"Pokemon_StartAuxProc","void Pokemon_StartAuxProc(GObj*, void (*)(GObj*));"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x803667c0,"EnvSound_PlaySound","void EnvSound_PlaySound(GObj* obj, u8 category, s32 soundID);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035fd70,"Pokemon_RunCleanup","void Pokemon_RunCleanup(GObj* obj);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cc1e0,"func_802CC1E0_7A5770","POKEMON_FUNC(func_802CC1E0_7A5770)"},
    {0x802cc250,"func_802CC250_7A57E0","POKEMON_FUNC(func_802CC250_7A57E0)"},
}};
constexpr std::array<ActorResourceDependency,2> resources_146{{
    {0x802d3504,"D_802D3504_7ACA94","AnimationHeader"},
    {0x802d352c,"D_802D352C_7ACABC","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,1> services_146{{
    ActorNativeService::environment_sound,
}};
constexpr std::array<std::uint32_t,2> callback_deps_146{{
    0x802cc1e0,
    0x802cc250,
}};
constexpr std::array<std::uint32_t,0> graph_deps_146{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_146{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_147{{
    {0x80362050,"Pokemon_ResetPathPos","void Pokemon_ResetPathPos(GObj*);"},
    {0x803620c8,"Pokemon_FollowPath","void Pokemon_FollowPath(GObj* obj, f32 start, f32 end, f32 dt, f32 yawStep, u32 flags);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_147{{
}};
constexpr std::array<ActorNativeService,0> services_147{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_147{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_147{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_147{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_148{{
    {0x800e41d8,"getGroundAt","s32 getGroundAt(f32 arg0, f32 arg1, GroundResult* arg2);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_148{{
}};
constexpr std::array<ActorNativeService,5> services_148{{
    ActorNativeService::model,
    ActorNativeService::vector,
    ActorNativeService::vector_read,
    ActorNativeService::vector_store,
    ActorNativeService::ground,
}};
constexpr std::array<std::uint32_t,0> callback_deps_148{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_148{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_148{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_149{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_149{{
    {0x802d3450,"D_802D3450_7AC9E0","AnimationHeader"},
    {0x802d376c,"D_802D376C_7ACCFC","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_149{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,0> callback_deps_149{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_149{{
    0x802d376c,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_149{{
}};
constexpr std::array<ActorDependencyEdge,6> calls_150{{
    {0x8036148c,"Pokemon_TurnToTarget","void Pokemon_TurnToTarget(GObj*, f32, u32);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8036148c,"Pokemon_TurnToTarget","void Pokemon_TurnToTarget(GObj*, f32, u32);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cc424,"func_802CC424_7A59B4","POKEMON_FUNC(func_802CC424_7A59B4)"},
}};
constexpr std::array<ActorResourceDependency,2> resources_150{{
    {0x802d3504,"D_802D3504_7ACA94","AnimationHeader"},
    {0x802d3ff0,"D_802D3FF0_7AD580","GObj*"},
}};
constexpr std::array<ActorNativeService,0> services_150{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_150{{
    0x802cc424,
}};
constexpr std::array<std::uint32_t,0> graph_deps_150{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_150{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_151{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cc424,"func_802CC424_7A59B4","POKEMON_FUNC(func_802CC424_7A59B4)"},
}};
constexpr std::array<ActorResourceDependency,2> resources_151{{
    {0x802d3450,"D_802D3450_7AC9E0","AnimationHeader"},
    {0x802d374c,"D_802D374C_7ACCDC","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_151{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,1> callback_deps_151{{
    0x802cc424,
}};
constexpr std::array<std::uint32_t,1> graph_deps_151{{
    0x802d374c,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_151{{
}};
constexpr std::array<ActorDependencyEdge,7> calls_152{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cc3b4,"func_802CC3B4_7A5944","POKEMON_FUNC(func_802CC3B4_7A5944)"},
}};
constexpr std::array<ActorResourceDependency,4> resources_152{{
    {0x802d352c,"D_802D352C_7ACABC","AnimationHeader"},
    {0x802d3554,"D_802D3554_7ACAE4","AnimationHeader"},
    {0x802d374c,"D_802D374C_7ACCDC","InteractionHandler"},
    {0x802d378c,"D_802D378C_7ACD1C","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_152{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,1> callback_deps_152{{
    0x802cc3b4,
}};
constexpr std::array<std::uint32_t,2> graph_deps_152{{
    0x802d374c,
    0x802d378c,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_152{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_153{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cc514,"func_802CC514_7A5AA4","POKEMON_FUNC(func_802CC514_7A5AA4)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_153{{
    {0x802d352c,"D_802D352C_7ACABC","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_153{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_153{{
    0x802cc514,
}};
constexpr std::array<std::uint32_t,0> graph_deps_153{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_153{{
}};
constexpr std::array<ActorDependencyEdge,8> calls_154{{
    {0x803667c0,"EnvSound_PlaySound","void EnvSound_PlaySound(GObj* obj, u8 category, s32 soundID);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035fd70,"Pokemon_RunCleanup","void Pokemon_RunCleanup(GObj* obj);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cc5c0,"func_802CC5C0_7A5B50","POKEMON_FUNC(func_802CC5C0_7A5B50)"},
    {0x802cbf64,"func_802CBF64_7A54F4","POKEMON_FUNC(func_802CBF64_7A54F4)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_154{{
}};
constexpr std::array<ActorNativeService,2> services_154{{
    ActorNativeService::forbidden_ground,
    ActorNativeService::environment_sound,
}};
constexpr std::array<std::uint32_t,2> callback_deps_154{{
    0x802cbf64,
    0x802cc5c0,
}};
constexpr std::array<std::uint32_t,0> graph_deps_154{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_154{{
}};
constexpr std::array<ActorDependencyEdge,8> calls_155{{
    {0x0,"GET_TRANSFORM","Source intrinsic/designator: GET_TRANSFORM"},
    {0x80019abc,"atan2f","Source intrinsic/designator: atan2f"},
    {0x80019abc,"atan2f","Source intrinsic/designator: atan2f"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x80360f1c,"Pokemon_StepWalkInDirectionFacing","bool Pokemon_StepWalkInDirectionFacing(GObj*, u32);"},
    {0x8035f2a4,"Pokemon_GetDistance","f32 Pokemon_GetDistance(GObj* obj1, GObj* obj2);"},
    {0x8000b830,"cmdSendCommandToLink","void cmdSendCommandToLink(s32 llIndex, s32, struct GObj*);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_155{{
    {0x802d3ff0,"D_802D3FF0_7AD580","GObj*"},
}};
constexpr std::array<ActorNativeService,6> services_155{{
    ActorNativeService::model,
    ActorNativeService::vector,
    ActorNativeService::vector_read,
    ActorNativeService::vector_store,
    ActorNativeService::item_direction,
    ActorNativeService::command_link,
}};
constexpr std::array<std::uint32_t,0> callback_deps_155{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_155{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_155{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_156{{
    {0x80362ee0,"Pokemon_SpawnOnGround","GObj* Pokemon_SpawnOnGround(s32 gObjID, u16 id, struct WorldBlock* roomA, struct WorldBlock* roomB, ObjectSpawn* spawn, PokemonInitData* initData);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_156{{
    {0x802d37c0,"D_802D37C0_7ACD50","PokemonInitData"},
    {0x0,"spawn","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,0> services_156{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_157{{
    {0x0,"GET_POKEMON","Source intrinsic/designator: GET_POKEMON"},
    {0x80363dbc,"pokemonChangeBlock","void pokemonChangeBlock(GObj* obj, f32 arg1, f32 arg2, f32 arg3, f32 arg4, f32 arg5, f32 arg6);"},
    {0x80363eb4,"pokemonChangeBlockOnGround","void pokemonChangeBlockOnGround(GObj* arg0, f32 arg1, f32 arg2, f32 arg3, f32 arg4, f32 arg5, f32 arg6);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_157{{
}};
constexpr std::array<ActorNativeService,2> services_157{{
    ActorNativeService::block_change,
    ActorNativeService::block_change_ground,
}};
constexpr std::array<ActorDependencyEdge,4> calls_158{{
    {0x80032a20,"__sinf","f32 __sinf(f32);"},
    {0x0,"GET_TRANSFORM","Source intrinsic/designator: GET_TRANSFORM"},
    {0x80038de0,"__cosf","f32 __cosf(f32);"},
    {0x0,"GET_TRANSFORM","Source intrinsic/designator: GET_TRANSFORM"},
}};
constexpr std::array<ActorResourceDependency,2> resources_158{{
    {0x802d2828,"D_802D2828_7ABDB8","f32"},
    {0x802d3844,"D_802D3844_7ACDD4","f32"},
}};
constexpr std::array<ActorNativeService,4> services_158{{
    ActorNativeService::player_model,
    ActorNativeService::vector,
    ActorNativeService::vector_read,
    ActorNativeService::vector_store,
}};
constexpr std::array<ActorDependencyEdge,3> calls_159{{
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802ccb34,"func_802CCB34_7A60C4","POKEMON_FUNC(func_802CCB34_7A60C4)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_159{{
}};
constexpr std::array<ActorNativeService,0> services_159{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_159{{
    0x802ccb34,
}};
constexpr std::array<std::uint32_t,0> graph_deps_159{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_159{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_160{{
    {0x80019db0,"randFloat","Source intrinsic/designator: randFloat"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_160{{
}};
constexpr std::array<ActorNativeService,5> services_160{{
    ActorNativeService::model,
    ActorNativeService::vector,
    ActorNativeService::vector_read,
    ActorNativeService::vector_store,
    ActorNativeService::rng_float,
}};
constexpr std::array<std::uint32_t,0> callback_deps_160{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_160{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_160{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_161{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035ed90,"Pokemon_StartAuxProc","void Pokemon_StartAuxProc(GObj*, void (*)(GObj*));"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802ccab4,"func_802CCAB4_7A6044","POKEMON_FUNC(func_802CCAB4_7A6044)"},
}};
constexpr std::array<ActorResourceDependency,4> resources_161{{
    {0x802d3808,"D_802D3808_7ACD98","AnimationHeader"},
    {0x802d384c,"D_802D384C_7ACDDC","f32"},
    {0x802d3880,"D_802D3880_7ACE10","InteractionHandler2"},
    {0x0,"sp20","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,2> services_161{{
    ActorNativeService::graph_copy,
    ActorNativeService::forbidden_ground,
}};
constexpr std::array<std::uint32_t,1> callback_deps_161{{
    0x802ccab4,
}};
constexpr std::array<std::uint32_t,1> graph_deps_161{{
    0x802d3880,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_161{{
}};
constexpr std::array<ActorDependencyEdge,6> calls_162{{
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802ccc90,"func_802CCC90_7A6220","POKEMON_FUNC(func_802CCC90_7A6220)"},
    {0x802cccfc,"func_802CCCFC_7A628C","POKEMON_FUNC(func_802CCCFC_7A628C)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_162{{
    {0x802d38a0,"D_802D38A0_7ACE30","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_162{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,2> callback_deps_162{{
    0x802ccc90,
    0x802cccfc,
}};
constexpr std::array<std::uint32_t,1> graph_deps_162{{
    0x802d38a0,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_162{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_163{{
    {0x80362050,"Pokemon_ResetPathPos","void Pokemon_ResetPathPos(GObj*);"},
    {0x803620c8,"Pokemon_FollowPath","void Pokemon_FollowPath(GObj* obj, f32 start, f32 end, f32 dt, f32 yawStep, u32 flags);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_163{{
}};
constexpr std::array<ActorNativeService,0> services_163{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_163{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_163{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_163{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_164{{
    {0x8035ed90,"Pokemon_StartAuxProc","void Pokemon_StartAuxProc(GObj*, void (*)(GObj*));"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035fd70,"Pokemon_RunCleanup","void Pokemon_RunCleanup(GObj* obj);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cd5d8,"func_802CD5D8_7A6B68","POKEMON_FUNC(func_802CD5D8_7A6B68)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_164{{
    {0x802d38e0,"D_802D38E0_7ACE70","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_164{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,1> callback_deps_164{{
    0x802cd5d8,
}};
constexpr std::array<std::uint32_t,1> graph_deps_164{{
    0x802d38e0,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_164{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_165{{
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035fd70,"Pokemon_RunCleanup","void Pokemon_RunCleanup(GObj* obj);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_165{{
}};
constexpr std::array<ActorNativeService,0> services_165{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_165{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_165{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_165{{
}};
constexpr std::array<ActorDependencyEdge,8> calls_166{{
    {0x8035ed90,"Pokemon_StartAuxProc","void Pokemon_StartAuxProc(GObj*, void (*)(GObj*));"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cd5d8,"func_802CD5D8_7A6B68","POKEMON_FUNC(func_802CD5D8_7A6B68)"},
    {0x802cce70,"func_802CCE70_7A6400","POKEMON_FUNC(func_802CCE70_7A6400)"},
    {0x802cd118,"func_802CD118_7A66A8","POKEMON_FUNC(func_802CD118_7A66A8)"},
}};
constexpr std::array<ActorResourceDependency,2> resources_166{{
    {0x802d381c,"D_802D381C_7ACDAC","AnimationHeader"},
    {0x802d3850,"D_802D3850_7ACDE0","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_166{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,3> callback_deps_166{{
    0x802cce70,
    0x802cd118,
    0x802cd5d8,
}};
constexpr std::array<std::uint32_t,1> graph_deps_166{{
    0x802d3850,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_166{{
}};
constexpr std::array<ActorDependencyEdge,15> calls_167{{
    {0x0,"GET_TRANSFORM","Source intrinsic/designator: GET_TRANSFORM"},
    {0x80361b50,"Pokemon_SetTargetPos","void Pokemon_SetTargetPos(GObj*, f32, f32);"},
    {0x80361b68,"Pokemon_StepToTargetPos","s32 Pokemon_StepToTargetPos(GObj* obj, f32 turnSpeed, u32 flags);"},
    {0x0,"SQ","Source intrinsic/designator: SQ"},
    {0x0,"SQ","Source intrinsic/designator: SQ"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x80019abc,"atan2f","Source intrinsic/designator: atan2f"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x80032a20,"__sinf","f32 __sinf(f32);"},
    {0x80038de0,"__cosf","f32 __cosf(f32);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x802cc980,"func_802CC980_7A5F10","void func_802CC980_7A5F10(s32 arg0, Vec3f* arg1, f32* arg2)"},
    {0x80361b50,"Pokemon_SetTargetPos","void Pokemon_SetTargetPos(GObj*, f32, f32);"},
    {0x80361b68,"Pokemon_StepToTargetPos","s32 Pokemon_StepToTargetPos(GObj* obj, f32 turnSpeed, u32 flags);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_167{{
    {0x802d2828,"D_802D2828_7ABDB8","f32"},
}};
constexpr std::array<ActorNativeService,5> services_167{{
    ActorNativeService::model,
    ActorNativeService::player_model,
    ActorNativeService::vector,
    ActorNativeService::vector_read,
    ActorNativeService::vector_store,
}};
constexpr std::array<std::uint32_t,0> callback_deps_167{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_167{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_167{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_168{{
    {0x80019e14,"randRange","Source intrinsic/designator: randRange"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cd118,"func_802CD118_7A66A8","POKEMON_FUNC(func_802CD118_7A66A8)"},
}};
constexpr std::array<ActorResourceDependency,2> resources_168{{
    {0x802d3808,"D_802D3808_7ACD98","AnimationHeader"},
    {0x802d381c,"D_802D381C_7ACDAC","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,1> services_168{{
    ActorNativeService::rng_range,
}};
constexpr std::array<std::uint32_t,1> callback_deps_168{{
    0x802cd118,
}};
constexpr std::array<std::uint32_t,0> graph_deps_168{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_168{{
}};
constexpr std::array<ActorDependencyEdge,7> calls_169{{
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cd1ac,"func_802CD1AC_7A673C","POKEMON_FUNC(func_802CD1AC_7A673C)"},
    {0x802cd0b8,"func_802CD0B8_7A6648","POKEMON_FUNC(func_802CD0B8_7A6648)"},
    {0x802cd2a8,"func_802CD2A8_7A6838","POKEMON_FUNC(func_802CD2A8_7A6838)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_169{{
}};
constexpr std::array<ActorNativeService,1> services_169{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,3> callback_deps_169{{
    0x802cd0b8,
    0x802cd1ac,
    0x802cd2a8,
}};
constexpr std::array<std::uint32_t,0> graph_deps_169{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_169{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_170{{
    {0x802cc980,"func_802CC980_7A5F10","void func_802CC980_7A5F10(s32 arg0, Vec3f* arg1, f32* arg2)"},
    {0x80357384,"getLevelProgress","void getLevelProgress(s32* blockCount, f32* blockPart);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_170{{
    {0x802d3b34,"D_802D3B34_7AD0C4","GObj*"},
}};
constexpr std::array<ActorNativeService,3> services_170{{
    ActorNativeService::model,
    ActorNativeService::vector,
    ActorNativeService::level_progress,
}};
constexpr std::array<std::uint32_t,0> callback_deps_170{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_170{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_170{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_171{{
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cd2fc,"func_802CD2FC_7A688C","POKEMON_FUNC(func_802CD2FC_7A688C)"},
    {0x802cd4f4,"func_802CD4F4_7A6A84","POKEMON_FUNC(func_802CD4F4_7A6A84)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_171{{
}};
constexpr std::array<ActorNativeService,0> services_171{{
}};
constexpr std::array<std::uint32_t,2> callback_deps_171{{
    0x802cd2fc,
    0x802cd4f4,
}};
constexpr std::array<std::uint32_t,0> graph_deps_171{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_171{{
}};
constexpr std::array<ActorDependencyEdge,10> calls_172{{
    {0x0,"GET_TRANSFORM","Source intrinsic/designator: GET_TRANSFORM"},
    {0x0,"GET_TRANSFORM","Source intrinsic/designator: GET_TRANSFORM"},
    {0x802cc980,"func_802CC980_7A5F10","void func_802CC980_7A5F10(s32 arg0, Vec3f* arg1, f32* arg2)"},
    {0x80019abc,"atan2f","Source intrinsic/designator: atan2f"},
    {0x0,"ABS","Source intrinsic/designator: ABS"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x80361b50,"Pokemon_SetTargetPos","void Pokemon_SetTargetPos(GObj*, f32, f32);"},
    {0x80361b68,"Pokemon_StepToTargetPos","s32 Pokemon_StepToTargetPos(GObj* obj, f32 turnSpeed, u32 flags);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_172{{
    {0x802d3844,"D_802D3844_7ACDD4","f32"},
}};
constexpr std::array<ActorNativeService,4> services_172{{
    ActorNativeService::model,
    ActorNativeService::player_model,
    ActorNativeService::vector,
    ActorNativeService::vector_read,
}};
constexpr std::array<std::uint32_t,0> callback_deps_172{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_172{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_172{{
}};
constexpr std::array<ActorDependencyEdge,7> calls_173{{
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8000b830,"cmdSendCommandToLink","void cmdSendCommandToLink(s32 llIndex, s32, struct GObj*);"},
    {0x8000b830,"cmdSendCommandToLink","void cmdSendCommandToLink(s32 llIndex, s32, struct GObj*);"},
    {0x8000b830,"cmdSendCommandToLink","void cmdSendCommandToLink(s32 llIndex, s32, struct GObj*);"},
    {0x8000b774,"cmdSendCommand","s32 cmdSendCommand(struct GObj* obj, s32 cmd, struct GObj* source);"},
    {0x8035fd70,"Pokemon_RunCleanup","void Pokemon_RunCleanup(GObj* obj);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_173{{
    {0x802d3848,"D_802D3848_7ACDD8","s32"},
    {0x802d3b34,"D_802D3B34_7AD0C4","GObj*"},
}};
constexpr std::array<ActorNativeService,2> services_173{{
    ActorNativeService::command,
    ActorNativeService::command_link,
}};
constexpr std::array<std::uint32_t,0> callback_deps_173{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_173{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_173{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_174{{
    {0x800e41d8,"getGroundAt","s32 getGroundAt(f32 arg0, f32 arg1, GroundResult* arg2);"},
    {0x0,"ABS","Source intrinsic/designator: ABS"},
    {0x800e41d8,"getGroundAt","s32 getGroundAt(f32 arg0, f32 arg1, GroundResult* arg2);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_174{{
}};
constexpr std::array<ActorNativeService,5> services_174{{
    ActorNativeService::model,
    ActorNativeService::vector,
    ActorNativeService::vector_read,
    ActorNativeService::vector_store,
    ActorNativeService::ground,
}};
constexpr std::array<std::uint32_t,0> callback_deps_174{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_174{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_174{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_175{{
    {0x80362e5c,"Pokemon_Spawn","GObj* Pokemon_Spawn(s32 gObjID, u16 id, struct WorldBlock* roomA, struct WorldBlock* roomB, ObjectSpawn* spawn, PokemonInitData* initData);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_175{{
    {0x802d3914,"D_802D3914_7ACEA4","PokemonInitData"},
    {0x0,"spawn","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,0> services_175{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_176{{
    {0x80362e5c,"Pokemon_Spawn","GObj* Pokemon_Spawn(s32 gObjID, u16 id, struct WorldBlock* roomA, struct WorldBlock* roomB, ObjectSpawn* spawn, PokemonInitData* initData);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_176{{
    {0x802d3914,"D_802D3914_7ACEA4","PokemonInitData"},
    {0x0,"spawn","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,0> services_176{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_177{{
    {0x80364280,"pokemonRemoveOne","void pokemonRemoveOne(GObj* obj);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_177{{
}};
constexpr std::array<ActorNativeService,1> services_177{{
    ActorNativeService::common_remove,
}};
constexpr std::array<std::uint32_t,0> callback_deps_177{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_177{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_177{{
}};
constexpr std::array<ActorDependencyEdge,7> calls_178{{
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cdb40,"func_802CDB40_7A70D0","POKEMON_FUNC(func_802CDB40_7A70D0)"},
    {0x802cdbd4,"func_802CDBD4_7A7164","POKEMON_FUNC(func_802CDBD4_7A7164)"},
    {0x802cdc68,"func_802CDC68_7A71F8","POKEMON_FUNC(func_802CDC68_7A71F8)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_178{{
}};
constexpr std::array<ActorNativeService,0> services_178{{
}};
constexpr std::array<std::uint32_t,3> callback_deps_178{{
    0x802cdb40,
    0x802cdbd4,
    0x802cdc68,
}};
constexpr std::array<std::uint32_t,0> graph_deps_178{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_178{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_179{{
    {0x8035ecac,"Pokemon_SetStateRandom","void Pokemon_SetStateRandom(GObj* obj, RandomState* nextStates);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_179{{
    {0x802d39b4,"D_802D39B4_7ACF44","RandomStates3"},
    {0x0,"sp18","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,1> services_179{{
    ActorNativeService::weighted_copy,
}};
constexpr std::array<std::uint32_t,0> callback_deps_179{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_179{{
}};
constexpr std::array<std::uint32_t,1> weighted_deps_179{{
    0x802d39b4,
}};
constexpr std::array<ActorDependencyEdge,3> calls_180{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cd914,"func_802CD914_7A6EA4","POKEMON_FUNC(func_802CD914_7A6EA4)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_180{{
    {0x802d3958,"D_802D3958_7ACEE8","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_180{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_180{{
    0x802cd914,
}};
constexpr std::array<std::uint32_t,0> graph_deps_180{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_180{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_181{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cd914,"func_802CD914_7A6EA4","POKEMON_FUNC(func_802CD914_7A6EA4)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_181{{
    {0x802d396c,"D_802D396C_7ACEFC","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_181{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_181{{
    0x802cd914,
}};
constexpr std::array<std::uint32_t,0> graph_deps_181{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_181{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_182{{
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035fd70,"Pokemon_RunCleanup","void Pokemon_RunCleanup(GObj* obj);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cd84c,"func_802CD84C_7A6DDC","POKEMON_FUNC(func_802CD84C_7A6DDC)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_182{{
}};
constexpr std::array<ActorNativeService,1> services_182{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,1> callback_deps_182{{
    0x802cd84c,
}};
constexpr std::array<std::uint32_t,0> graph_deps_182{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_182{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_183{{
    {0x80362050,"Pokemon_ResetPathPos","void Pokemon_ResetPathPos(GObj*);"},
    {0x803620c8,"Pokemon_FollowPath","void Pokemon_FollowPath(GObj* obj, f32 start, f32 end, f32 dt, f32 yawStep, u32 flags);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_183{{
}};
constexpr std::array<ActorNativeService,0> services_183{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_183{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_183{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_183{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_184{{
    {0x80019db0,"randFloat","Source intrinsic/designator: randFloat"},
    {0x80019db0,"randFloat","Source intrinsic/designator: randFloat"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_184{{
}};
constexpr std::array<ActorNativeService,5> services_184{{
    ActorNativeService::model,
    ActorNativeService::vector,
    ActorNativeService::vector_read,
    ActorNativeService::vector_store,
    ActorNativeService::rng_float,
}};
constexpr std::array<std::uint32_t,0> callback_deps_184{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_184{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_184{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_185{{
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x8035edc8,"Pokemon_StopAuxProc","void Pokemon_StopAuxProc(GObj*);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_185{{
}};
constexpr std::array<ActorNativeService,4> services_185{{
    ActorNativeService::model,
    ActorNativeService::vector,
    ActorNativeService::vector_read,
    ActorNativeService::vector_store,
}};
constexpr std::array<std::uint32_t,0> callback_deps_185{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_185{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_185{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_186{{
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_186{{
    {0x802d39d0,"D_802D39D0_7ACF60","InteractionHandler2"},
    {0x0,"sp18","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,1> services_186{{
    ActorNativeService::graph_copy,
}};
constexpr std::array<std::uint32_t,0> callback_deps_186{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_186{{
    0x802d39d0,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_186{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_187{{
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_187{{
    {0x802d39f0,"D_802D39F0_7ACF80","InteractionHandler2"},
    {0x0,"sp18","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,1> services_187{{
    ActorNativeService::graph_copy,
}};
constexpr std::array<std::uint32_t,0> callback_deps_187{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_187{{
    0x802d39f0,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_187{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_188{{
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_188{{
    {0x802d3a10,"D_802D3A10_7ACFA0","InteractionHandler2"},
    {0x0,"sp18","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,1> services_188{{
    ActorNativeService::graph_copy,
}};
constexpr std::array<std::uint32_t,0> callback_deps_188{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_188{{
    0x802d3a10,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_188{{
}};
constexpr std::array<ActorDependencyEdge,11> calls_189{{
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8000b774,"cmdSendCommand","s32 cmdSendCommand(struct GObj* obj, s32 cmd, struct GObj* source);"},
    {0x803667c0,"EnvSound_PlaySound","void EnvSound_PlaySound(GObj* obj, u8 category, s32 soundID);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035ed90,"Pokemon_StartAuxProc","void Pokemon_StartAuxProc(GObj*, void (*)(GObj*));"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cd9f8,"func_802CD9F8_7A6F88","POKEMON_FUNC(func_802CD9F8_7A6F88)"},
    {0x802cd98c,"func_802CD98C_7A6F1C","POKEMON_FUNC(func_802CD98C_7A6F1C)"},
    {0x802cd84c,"func_802CD84C_7A6DDC","POKEMON_FUNC(func_802CD84C_7A6DDC)"},
}};
constexpr std::array<ActorResourceDependency,4> resources_189{{
    {0x802d3980,"D_802D3980_7ACF10","AnimationHeader"},
    {0x802d3994,"D_802D3994_7ACF24","InteractionHandler"},
    {0x802d39cc,"D_802D39CC_7ACF5C","s32"},
    {0x802d3b34,"D_802D3B34_7AD0C4","GObj*"},
}};
constexpr std::array<ActorNativeService,3> services_189{{
    ActorNativeService::graph,
    ActorNativeService::environment_sound,
    ActorNativeService::command,
}};
constexpr std::array<std::uint32_t,3> callback_deps_189{{
    0x802cd84c,
    0x802cd98c,
    0x802cd9f8,
}};
constexpr std::array<std::uint32_t,1> graph_deps_189{{
    0x802d3994,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_189{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_190{{
    {0x80362e5c,"Pokemon_Spawn","GObj* Pokemon_Spawn(s32 gObjID, u16 id, struct WorldBlock* roomA, struct WorldBlock* roomB, ObjectSpawn* spawn, PokemonInitData* initData);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_190{{
    {0x802d3a44,"D_802D3A44_7ACFD4","PokemonInitData"},
    {0x0,"spawn","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,0> services_190{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_191{{
    {0x80362e5c,"Pokemon_Spawn","GObj* Pokemon_Spawn(s32 gObjID, u16 id, struct WorldBlock* roomA, struct WorldBlock* roomB, ObjectSpawn* spawn, PokemonInitData* initData);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_191{{
    {0x802d3a44,"D_802D3A44_7ACFD4","PokemonInitData"},
    {0x0,"spawn","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,0> services_191{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_192{{
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cdea4,"func_802CDEA4_7A7434","POKEMON_FUNC(func_802CDEA4_7A7434)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_192{{
}};
constexpr std::array<ActorNativeService,0> services_192{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_192{{
    0x802cdea4,
}};
constexpr std::array<std::uint32_t,0> graph_deps_192{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_192{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_193{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_193{{
    {0x802d3a80,"D_802D3A80_7AD010","AnimationHeader"},
    {0x802d3b38,"D_802D3B38_7AD0C8","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_193{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,0> callback_deps_193{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_193{{
    0x802d3b38,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_193{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_194{{
    {0x803667c0,"EnvSound_PlaySound","void EnvSound_PlaySound(GObj* obj, u8 category, s32 soundID);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cdea4,"func_802CDEA4_7A7434","POKEMON_FUNC(func_802CDEA4_7A7434)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_194{{
    {0x802d3ae4,"D_802D3AE4_7AD074","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,1> services_194{{
    ActorNativeService::environment_sound,
}};
constexpr std::array<std::uint32_t,1> callback_deps_194{{
    0x802cdea4,
}};
constexpr std::array<std::uint32_t,0> graph_deps_194{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_194{{
}};
constexpr std::array<ActorDependencyEdge,6> calls_195{{
    {0x803667c0,"EnvSound_PlaySound","void EnvSound_PlaySound(GObj* obj, u8 category, s32 soundID);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,3> resources_195{{
    {0x802d3a94,"D_802D3A94_7AD024","AnimationHeader"},
    {0x802d3ae4,"D_802D3AE4_7AD074","AnimationHeader"},
    {0x802d3b68,"D_802D3B68_7AD0F8","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,2> services_195{{
    ActorNativeService::graph,
    ActorNativeService::environment_sound,
}};
constexpr std::array<std::uint32_t,0> callback_deps_195{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_195{{
    0x802d3b68,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_195{{
}};
constexpr std::array<ActorDependencyEdge,6> calls_196{{
    {0x803667c0,"EnvSound_PlaySound","void EnvSound_PlaySound(GObj* obj, u8 category, s32 soundID);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,3> resources_196{{
    {0x802d3aa8,"D_802D3AA8_7AD038","AnimationHeader"},
    {0x802d3af8,"D_802D3AF8_7AD088","AnimationHeader"},
    {0x802d3b98,"D_802D3B98_7AD128","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,2> services_196{{
    ActorNativeService::graph,
    ActorNativeService::environment_sound,
}};
constexpr std::array<std::uint32_t,0> callback_deps_196{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_196{{
    0x802d3b98,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_196{{
}};
constexpr std::array<ActorDependencyEdge,6> calls_197{{
    {0x803667c0,"EnvSound_PlaySound","void EnvSound_PlaySound(GObj* obj, u8 category, s32 soundID);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,3> resources_197{{
    {0x802d3abc,"D_802D3ABC_7AD04C","AnimationHeader"},
    {0x802d3b0c,"D_802D3B0C_7AD09C","AnimationHeader"},
    {0x802d3bc8,"D_802D3BC8_7AD158","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,2> services_197{{
    ActorNativeService::graph,
    ActorNativeService::environment_sound,
}};
constexpr std::array<std::uint32_t,0> callback_deps_197{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_197{{
    0x802d3bc8,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_197{{
}};
constexpr std::array<ActorDependencyEdge,6> calls_198{{
    {0x803667c0,"EnvSound_PlaySound","void EnvSound_PlaySound(GObj* obj, u8 category, s32 soundID);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8000b774,"cmdSendCommand","s32 cmdSendCommand(struct GObj* obj, s32 cmd, struct GObj* source);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_198{{
    {0x802d2ff0,"D_802D2FF0_7AC580","GObj*"},
}};
constexpr std::array<ActorNativeService,2> services_198{{
    ActorNativeService::environment_sound,
    ActorNativeService::command,
}};
constexpr std::array<std::uint32_t,0> callback_deps_198{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_198{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_198{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_199{{
    {0x80362dc4,"Pokemon_SpawnDlLink4","GObj* Pokemon_SpawnDlLink4(s32 gObjID, u16 id, struct WorldBlock* roomA, struct WorldBlock* roomB, ObjectSpawn* spawn, PokemonInitData* initData);"},
}};
constexpr std::array<ActorResourceDependency,3> resources_199{{
    {0x802d3b34,"D_802D3B34_7AD0C4","GObj*"},
    {0x802d3c0c,"D_802D3C0C_7AD19C","PokemonInitData"},
    {0x0,"spawn","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,1> services_199{{
    ActorNativeService::reference_assign,
}};
constexpr std::array<ActorDependencyEdge,2> calls_200{{
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802ce224,"func_802CE224_7A77B4","POKEMON_FUNC(func_802CE224_7A77B4)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_200{{
}};
constexpr std::array<ActorNativeService,0> services_200{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_200{{
    0x802ce224,
}};
constexpr std::array<std::uint32_t,0> graph_deps_200{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_200{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_201{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802ce2dc,"func_802CE2DC_7A786C","POKEMON_FUNC(func_802CE2DC_7A786C)"},
}};
constexpr std::array<ActorResourceDependency,3> resources_201{{
    {0x802d3c5c,"D_802D3C5C_7AD1EC","AnimationHeader"},
    {0x802d3da4,"D_802D3DA4_7AD334","InteractionHandler2"},
    {0x0,"sp20","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,1> services_201{{
    ActorNativeService::graph_copy,
}};
constexpr std::array<std::uint32_t,1> callback_deps_201{{
    0x802ce2dc,
}};
constexpr std::array<std::uint32_t,1> graph_deps_201{{
    0x802d3da4,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_201{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_202{{
    {0x8036148c,"Pokemon_TurnToTarget","void Pokemon_TurnToTarget(GObj*, f32, u32);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_202{{
}};
constexpr std::array<ActorNativeService,0> services_202{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_202{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_202{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_202{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_203{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802ce37c,"func_802CE37C_7A790C","POKEMON_FUNC(func_802CE37C_7A790C)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_203{{
    {0x802d3d10,"D_802D3D10_7AD2A0","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_203{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_203{{
    0x802ce37c,
}};
constexpr std::array<std::uint32_t,0> graph_deps_203{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_203{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_204{{
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_204{{
    {0x802d3dc4,"D_802D3DC4_7AD354","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_204{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,0> callback_deps_204{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_204{{
    0x802d3dc4,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_204{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_205{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802ce440,"func_802CE440_7A79D0","POKEMON_FUNC(func_802CE440_7A79D0)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_205{{
    {0x802d3cfc,"D_802D3CFC_7AD28C","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_205{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_205{{
    0x802ce440,
}};
constexpr std::array<std::uint32_t,0> graph_deps_205{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_205{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_206{{
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ecac,"Pokemon_SetStateRandom","void Pokemon_SetStateRandom(GObj* obj, RandomState* nextStates);"},
}};
constexpr std::array<ActorResourceDependency,3> resources_206{{
    {0x802d3c5c,"D_802D3C5C_7AD1EC","AnimationHeader"},
    {0x802d3d24,"D_802D3D24_7AD2B4","InteractionHandler"},
    {0x802d3d84,"D_802D3D84_7AD314","RandomState"},
}};
constexpr std::array<ActorNativeService,2> services_206{{
    ActorNativeService::graph,
    ActorNativeService::weighted,
}};
constexpr std::array<std::uint32_t,0> callback_deps_206{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_206{{
    0x802d3d24,
}};
constexpr std::array<std::uint32_t,1> weighted_deps_206{{
    0x802d3d84,
}};
constexpr std::array<ActorDependencyEdge,7> calls_207{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ecac,"Pokemon_SetStateRandom","void Pokemon_SetStateRandom(GObj* obj, RandomState* nextStates);"},
    {0x802ce554,"func_802CE554_7A7AE4","POKEMON_FUNC(func_802CE554_7A7AE4)"},
    {0x802ce440,"func_802CE440_7A79D0","POKEMON_FUNC(func_802CE440_7A79D0)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_207{{
    {0x802d3d84,"D_802D3D84_7AD314","RandomState"},
}};
constexpr std::array<ActorNativeService,2> services_207{{
    ActorNativeService::graph,
    ActorNativeService::weighted,
}};
constexpr std::array<std::uint32_t,2> callback_deps_207{{
    0x802ce440,
    0x802ce554,
}};
constexpr std::array<std::uint32_t,0> graph_deps_207{{
}};
constexpr std::array<std::uint32_t,1> weighted_deps_207{{
    0x802d3d84,
}};
constexpr std::array<ActorDependencyEdge,2> calls_208{{
    {0x80361110,"Pokemon_RunInCircles","void Pokemon_RunInCircles(GObj*, f32, f32, UNK_TYPE);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_208{{
}};
constexpr std::array<ActorNativeService,0> services_208{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_208{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_208{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_208{{
}};
constexpr std::array<ActorDependencyEdge,7> calls_209{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ecac,"Pokemon_SetStateRandom","void Pokemon_SetStateRandom(GObj* obj, RandomState* nextStates);"},
    {0x802ce654,"func_802CE654_7A7BE4","POKEMON_FUNC(func_802CE654_7A7BE4)"},
    {0x802ce440,"func_802CE440_7A79D0","POKEMON_FUNC(func_802CE440_7A79D0)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_209{{
    {0x802d3d84,"D_802D3D84_7AD314","RandomState"},
}};
constexpr std::array<ActorNativeService,2> services_209{{
    ActorNativeService::graph,
    ActorNativeService::weighted,
}};
constexpr std::array<std::uint32_t,2> callback_deps_209{{
    0x802ce440,
    0x802ce654,
}};
constexpr std::array<std::uint32_t,0> graph_deps_209{{
}};
constexpr std::array<std::uint32_t,1> weighted_deps_209{{
    0x802d3d84,
}};
constexpr std::array<ActorDependencyEdge,2> calls_210{{
    {0x80361110,"Pokemon_RunInCircles","void Pokemon_RunInCircles(GObj*, f32, f32, UNK_TYPE);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_210{{
}};
constexpr std::array<ActorNativeService,0> services_210{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_210{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_210{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_210{{
}};
constexpr std::array<ActorDependencyEdge,9> calls_211{{
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802ce440,"func_802CE440_7A79D0","POKEMON_FUNC(func_802CE440_7A79D0)"},
}};
constexpr std::array<ActorResourceDependency,4> resources_211{{
    {0x802d3cac,"D_802D3CAC_7AD23C","AnimationHeader"},
    {0x802d3cc0,"D_802D3CC0_7AD250","AnimationHeader"},
    {0x802d3cd4,"D_802D3CD4_7AD264","AnimationHeader"},
    {0x802d3de4,"D_802D3DE4_7AD374","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_211{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,1> callback_deps_211{{
    0x802ce440,
}};
constexpr std::array<std::uint32_t,1> graph_deps_211{{
    0x802d3de4,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_211{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_212{{
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802ce440,"func_802CE440_7A79D0","POKEMON_FUNC(func_802CE440_7A79D0)"},
}};
constexpr std::array<ActorResourceDependency,2> resources_212{{
    {0x802d3c98,"D_802D3C98_7AD228","AnimationHeader"},
    {0x802d3e14,"D_802D3E14_7AD3A4","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_212{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,1> callback_deps_212{{
    0x802ce440,
}};
constexpr std::array<std::uint32_t,1> graph_deps_212{{
    0x802d3e14,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_212{{
}};
constexpr std::array<ActorDependencyEdge,8> calls_213{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802ce870,"func_802CE870_7A7E00","POKEMON_FUNC(func_802CE870_7A7E00)"},
    {0x802ce440,"func_802CE440_7A79D0","POKEMON_FUNC(func_802CE440_7A79D0)"},
    {0x802ce8f4,"func_802CE8F4_7A7E84","POKEMON_FUNC(func_802CE8F4_7A7E84)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_213{{
}};
constexpr std::array<ActorNativeService,1> services_213{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,3> callback_deps_213{{
    0x802ce440,
    0x802ce870,
    0x802ce8f4,
}};
constexpr std::array<std::uint32_t,0> graph_deps_213{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_213{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_214{{
    {0x8036148c,"Pokemon_TurnToTarget","void Pokemon_TurnToTarget(GObj*, f32, u32);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x80361748,"Pokemon_RunToTarget","void Pokemon_RunToTarget(GObj*, f32, f32, u32);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_214{{
    {0x802d3c84,"D_802D3C84_7AD214","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_214{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_214{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_214{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_214{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_215{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ecac,"Pokemon_SetStateRandom","void Pokemon_SetStateRandom(GObj* obj, RandomState* nextStates);"},
    {0x802ce8f4,"func_802CE8F4_7A7E84","POKEMON_FUNC(func_802CE8F4_7A7E84)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_215{{
    {0x802d3d84,"D_802D3D84_7AD314","RandomState"},
}};
constexpr std::array<ActorNativeService,2> services_215{{
    ActorNativeService::graph,
    ActorNativeService::weighted,
}};
constexpr std::array<std::uint32_t,1> callback_deps_215{{
    0x802ce8f4,
}};
constexpr std::array<std::uint32_t,0> graph_deps_215{{
}};
constexpr std::array<std::uint32_t,1> weighted_deps_215{{
    0x802d3d84,
}};
constexpr std::array<ActorDependencyEdge,6> calls_216{{
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802ce440,"func_802CE440_7A79D0","POKEMON_FUNC(func_802CE440_7A79D0)"},
    {0x802ce7d8,"func_802CE7D8_7A7D68","POKEMON_FUNC(func_802CE7D8_7A7D68)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_216{{
}};
constexpr std::array<ActorNativeService,1> services_216{{
    ActorNativeService::graph_copy,
}};
constexpr std::array<std::uint32_t,2> callback_deps_216{{
    0x802ce440,
    0x802ce7d8,
}};
constexpr std::array<std::uint32_t,0> graph_deps_216{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_216{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_217{{
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802ce7d8,"func_802CE7D8_7A7D68","POKEMON_FUNC(func_802CE7D8_7A7D68)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_217{{
}};
constexpr std::array<ActorNativeService,0> services_217{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_217{{
    0x802ce7d8,
}};
constexpr std::array<std::uint32_t,0> graph_deps_217{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_217{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_218{{
    {0x80362ee0,"Pokemon_SpawnOnGround","GObj* Pokemon_SpawnOnGround(s32 gObjID, u16 id, struct WorldBlock* roomA, struct WorldBlock* roomB, ObjectSpawn* spawn, PokemonInitData* initData);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_218{{
    {0x802d3f38,"D_802D3F38_7AD4C8","PokemonInitData"},
    {0x0,"spawn","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,0> services_218{{
}};
constexpr std::array<ActorDependencyEdge,8> calls_219{{
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cf388,"func_802CF388_7A8918","POKEMON_FUNC(func_802CF388_7A8918)"},
    {0x802cf500,"func_802CF500_7A8A90","POKEMON_FUNC(func_802CF500_7A8A90)"},
    {0x802cf638,"func_802CF638_7A8BC8","POKEMON_FUNC(func_802CF638_7A8BC8)"},
    {0x802ceb4c,"func_802CEB4C_7A80DC","POKEMON_FUNC(func_802CEB4C_7A80DC)"},
}};
constexpr std::array<ActorResourceDependency,2> resources_219{{
    {0x802d3ff0,"D_802D3FF0_7AD580","GObj*"},
    {0x802d3ff4,"D_802D3FF4_7AD584","GObj*"},
}};
constexpr std::array<ActorNativeService,1> services_219{{
    ActorNativeService::reference_assign,
}};
constexpr std::array<std::uint32_t,4> callback_deps_219{{
    0x802ceb4c,
    0x802cf388,
    0x802cf500,
    0x802cf638,
}};
constexpr std::array<std::uint32_t,0> graph_deps_219{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_219{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_220{{
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_220{{
    {0x802d3ffc,"D_802D3FFC_7AD58C","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_220{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,0> callback_deps_220{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_220{{
    0x802d3ffc,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_220{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_221{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802ced44,"func_802CED44_7A82D4","POKEMON_FUNC(func_802CED44_7A82D4)"},
}};
constexpr std::array<ActorResourceDependency,2> resources_221{{
    {0x802d3f8c,"D_802D3F8C_7AD51C","AnimationHeader"},
    {0x802d401c,"D_802D401C_7AD5AC","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_221{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,1> callback_deps_221{{
    0x802ced44,
}};
constexpr std::array<std::uint32_t,1> graph_deps_221{{
    0x802d401c,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_221{{
}};
constexpr std::array<ActorDependencyEdge,6> calls_222{{
    {0x0,"ABS","Source intrinsic/designator: ABS"},
    {0x80036090,"sqrtf","Source intrinsic/designator: sqrtf"},
    {0x0,"SQ","Source intrinsic/designator: SQ"},
    {0x0,"SQ","Source intrinsic/designator: SQ"},
    {0x0,"SQ","Source intrinsic/designator: SQ"},
    {0x80019abc,"atan2f","Source intrinsic/designator: atan2f"},
}};
constexpr std::array<ActorResourceDependency,0> resources_222{{
}};
constexpr std::array<ActorNativeService,0> services_222{{
}};
constexpr std::array<ActorDependencyEdge,14> calls_223{{
    {0x0,"GET_TRANSFORM","Source intrinsic/designator: GET_TRANSFORM"},
    {0x0,"GET_TRANSFORM","Source intrinsic/designator: GET_TRANSFORM"},
    {0x802cec18,"func_802CEC18_7A81A8","f32 func_802CEC18_7A81A8(Vec3f* arg0, Vec3f* arg1)"},
    {0x0,"GET_TRANSFORM","Source intrinsic/designator: GET_TRANSFORM"},
    {0x0,"ABS","Source intrinsic/designator: ABS"},
    {0x0,"ABS","Source intrinsic/designator: ABS"},
    {0x0,"SIGN","Source intrinsic/designator: SIGN"},
    {0x0,"SIGN","Source intrinsic/designator: SIGN"},
    {0x0,"GET_TRANSFORM","Source intrinsic/designator: GET_TRANSFORM"},
    {0x80032a20,"sinf","Source intrinsic/designator: sinf"},
    {0x80038de0,"cosf","Source intrinsic/designator: cosf"},
    {0x0,"GET_TRANSFORM","Source intrinsic/designator: GET_TRANSFORM"},
    {0x0,"GET_TRANSFORM","Source intrinsic/designator: GET_TRANSFORM"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_223{{
}};
constexpr std::array<ActorNativeService,5> services_223{{
    ActorNativeService::model,
    ActorNativeService::player_model,
    ActorNativeService::vector,
    ActorNativeService::vector_read,
    ActorNativeService::vector_store,
}};
constexpr std::array<std::uint32_t,0> callback_deps_223{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_223{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_223{{
}};
constexpr std::array<ActorDependencyEdge,6> calls_224{{
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x803601fc,"Pokemon_KickItem","void Pokemon_KickItem(GObj* obj, f32 speed);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802ceb9c,"func_802CEB9C_7A812C","POKEMON_FUNC(func_802CEB9C_7A812C)"},
}};
constexpr std::array<ActorResourceDependency,3> resources_224{{
    {0x802d3fa0,"D_802D3FA0_7AD530","AnimationHeader"},
    {0x802d404c,"D_802D404C_7AD5DC","InteractionHandler2"},
    {0x0,"sp20","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,2> services_224{{
    ActorNativeService::graph_copy,
    ActorNativeService::kick_item,
}};
constexpr std::array<std::uint32_t,1> callback_deps_224{{
    0x802ceb9c,
}};
constexpr std::array<std::uint32_t,1> graph_deps_224{{
    0x802d404c,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_224{{
}};
constexpr std::array<ActorDependencyEdge,7> calls_225{{
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cf114,"func_802CF114_7A86A4","POKEMON_FUNC(func_802CF114_7A86A4)"},
}};
constexpr std::array<ActorResourceDependency,2> resources_225{{
    {0x802d3fc8,"D_802D3FC8_7AD558","AnimationHeader"},
    {0x802d3fdc,"D_802D3FDC_7AD56C","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_225{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_225{{
    0x802cf114,
}};
constexpr std::array<std::uint32_t,0> graph_deps_225{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_225{{
}};
constexpr std::array<ActorDependencyEdge,16> calls_226{{
    {0x8000b774,"cmdSendCommand","s32 cmdSendCommand(struct GObj* obj, s32 cmd, struct GObj* source);"},
    {0x8035f1a8,"Pokemon_ForceAnimationAtTime","void Pokemon_ForceAnimationAtTime(GObj* obj, AnimationHeader* header, f32 time);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x803570f0,"Camera_StartCutScene","void Camera_StartCutScene(GObj* pokemon, AnimCmd* camAnim, f32 time);"},
    {0x800e1a78,"setSkyBoxAnimationSpeed","void setSkyBoxAnimationSpeed(f32 arg0);"},
    {0x80357150,"func_80357150_4F7560","void func_80357150_4F7560(GObj*);"},
    {0x8000b774,"cmdSendCommand","s32 cmdSendCommand(struct GObj* obj, s32 cmd, struct GObj* source);"},
    {0x8035f1a8,"Pokemon_ForceAnimationAtTime","void Pokemon_ForceAnimationAtTime(GObj* obj, AnimationHeader* header, f32 time);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x80357150,"func_80357150_4F7560","void func_80357150_4F7560(GObj*);"},
    {0x8000b774,"cmdSendCommand","s32 cmdSendCommand(struct GObj* obj, s32 cmd, struct GObj* source);"},
    {0x8035f1a8,"Pokemon_ForceAnimationAtTime","void Pokemon_ForceAnimationAtTime(GObj* obj, AnimationHeader* header, f32 time);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8000bd78,"ohResumeObjectProcesses","void ohResumeObjectProcesses(GObj* obj);"},
    {0x8000b774,"cmdSendCommand","s32 cmdSendCommand(struct GObj* obj, s32 cmd, struct GObj* source);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,6> resources_226{{
    {0x80100730,"D_80100730_3F6B70","f32"},
    {0x8011c550,"D_8011C550_412990","AnimCmd"},
    {0x802d3fb4,"D_802D3FB4_7AD544","AnimationHeader"},
    {0x802d406c,"D_802D406C_7AD5FC","f32"},
    {0x802d41a4,"D_802D41A4_7AD734","GObj*"},
    {0x802ec644,"D_802EC644_7C5BD4","GObj*"},
}};
constexpr std::array<ActorNativeService,7> services_226{{
    ActorNativeService::camera,
    ActorNativeService::camera_component,
    ActorNativeService::resume_processes,
    ActorNativeService::cutscene_start,
    ActorNativeService::sky_speed,
    ActorNativeService::player_cutscene_process,
    ActorNativeService::command,
}};
constexpr std::array<std::uint32_t,0> callback_deps_226{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_226{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_226{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_227{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ecac,"Pokemon_SetStateRandom","void Pokemon_SetStateRandom(GObj* obj, RandomState* nextStates);"},
}};
constexpr std::array<ActorResourceDependency,3> resources_227{{
    {0x802d3f8c,"D_802D3F8C_7AD51C","AnimationHeader"},
    {0x802d4070,"D_802D4070_7AD600","InteractionHandler"},
    {0x802d40a0,"D_802D40A0_7AD630","RandomState"},
}};
constexpr std::array<ActorNativeService,2> services_227{{
    ActorNativeService::graph,
    ActorNativeService::weighted,
}};
constexpr std::array<std::uint32_t,0> callback_deps_227{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_227{{
    0x802d4070,
}};
constexpr std::array<std::uint32_t,1> weighted_deps_227{{
    0x802d40a0,
}};
constexpr std::array<ActorDependencyEdge,7> calls_228{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_228{{
    {0x802d3fc8,"D_802D3FC8_7AD558","AnimationHeader"},
    {0x802d3fdc,"D_802D3FDC_7AD56C","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_228{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_228{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_228{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_228{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_229{{
    {0x803601fc,"Pokemon_KickItem","void Pokemon_KickItem(GObj* obj, f32 speed);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ecac,"Pokemon_SetStateRandom","void Pokemon_SetStateRandom(GObj* obj, RandomState* nextStates);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_229{{
    {0x802d3fa0,"D_802D3FA0_7AD530","AnimationHeader"},
    {0x802d40a0,"D_802D40A0_7AD630","RandomState"},
}};
constexpr std::array<ActorNativeService,2> services_229{{
    ActorNativeService::weighted,
    ActorNativeService::kick_item,
}};
constexpr std::array<std::uint32_t,0> callback_deps_229{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_229{{
}};
constexpr std::array<std::uint32_t,1> weighted_deps_229{{
    0x802d40a0,
}};
constexpr std::array<ActorDependencyEdge,2> calls_230{{
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cf524,"func_802CF524_7A8AB4","POKEMON_FUNC(func_802CF524_7A8AB4)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_230{{
}};
constexpr std::array<ActorNativeService,0> services_230{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_230{{
    0x802cf524,
}};
constexpr std::array<std::uint32_t,0> graph_deps_230{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_230{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_231{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cf524,"func_802CF524_7A8AB4","POKEMON_FUNC(func_802CF524_7A8AB4)"},
}};
constexpr std::array<ActorResourceDependency,2> resources_231{{
    {0x802d3f8c,"D_802D3F8C_7AD51C","AnimationHeader"},
    {0x802d40b0,"D_802D40B0_7AD640","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_231{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,1> callback_deps_231{{
    0x802cf524,
}};
constexpr std::array<std::uint32_t,1> graph_deps_231{{
    0x802d40b0,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_231{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_232{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x80360ab8,"Pokemon_FallDownOnGround","void Pokemon_FallDownOnGround(GObj* obj, f32 acceleration, s32);"},
    {0x8035fd70,"Pokemon_RunCleanup","void Pokemon_RunCleanup(GObj* obj);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_232{{
    {0x802d3fc8,"D_802D3FC8_7AD558","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,3> services_232{{
    ActorNativeService::model,
    ActorNativeService::vector,
    ActorNativeService::vector_read,
}};
constexpr std::array<std::uint32_t,0> callback_deps_232{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_232{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_232{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_233{{
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cf65c,"func_802CF65C_7A8BEC","POKEMON_FUNC(func_802CF65C_7A8BEC)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_233{{
}};
constexpr std::array<ActorNativeService,0> services_233{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_233{{
    0x802cf65c,
}};
constexpr std::array<std::uint32_t,0> graph_deps_233{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_233{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_234{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cf65c,"func_802CF65C_7A8BEC","POKEMON_FUNC(func_802CF65C_7A8BEC)"},
}};
constexpr std::array<ActorResourceDependency,2> resources_234{{
    {0x802d3f8c,"D_802D3F8C_7AD51C","AnimationHeader"},
    {0x802d40d0,"D_802D40D0_7AD660","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_234{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,1> callback_deps_234{{
    0x802cf65c,
}};
constexpr std::array<std::uint32_t,1> graph_deps_234{{
    0x802d40d0,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_234{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_235{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cf65c,"func_802CF65C_7A8BEC","POKEMON_FUNC(func_802CF65C_7A8BEC)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_235{{
    {0x802d3fa0,"D_802D3FA0_7AD530","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_235{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_235{{
    0x802cf65c,
}};
constexpr std::array<std::uint32_t,0> graph_deps_235{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_235{{
}};
constexpr std::array<ActorDependencyEdge,6> calls_236{{
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x803601fc,"Pokemon_KickItem","void Pokemon_KickItem(GObj* obj, f32 speed);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cf65c,"func_802CF65C_7A8BEC","POKEMON_FUNC(func_802CF65C_7A8BEC)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_236{{
    {0x802d3fa0,"D_802D3FA0_7AD530","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,1> services_236{{
    ActorNativeService::kick_item,
}};
constexpr std::array<std::uint32_t,1> callback_deps_236{{
    0x802cf65c,
}};
constexpr std::array<std::uint32_t,0> graph_deps_236{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_236{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_237{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,3> resources_237{{
    {0x802d3fc8,"D_802D3FC8_7AD558","AnimationHeader"},
    {0x802d3fdc,"D_802D3FDC_7AD56C","AnimationHeader"},
    {0x802d3ff8,"D_802D3FF8_7AD588","s32"},
}};
constexpr std::array<ActorNativeService,0> services_237{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_237{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_237{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_237{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_238{{
    {0x80362ee0,"Pokemon_SpawnOnGround","GObj* Pokemon_SpawnOnGround(s32 gObjID, u16 id, struct WorldBlock* roomA, struct WorldBlock* roomB, ObjectSpawn* spawn, PokemonInitData* initData);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_238{{
    {0x802d4124,"D_802D4124_7AD6B4","PokemonInitData"},
    {0x0,"spawn","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,0> services_238{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_239{{
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802cf864,"func_802CF864_7A8DF4","POKEMON_FUNC(func_802CF864_7A8DF4)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_239{{
}};
constexpr std::array<ActorNativeService,0> services_239{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_239{{
    0x802cf864,
}};
constexpr std::array<std::uint32_t,0> graph_deps_239{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_239{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_240{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_240{{
    {0x802d4168,"D_802D4168_7AD6F8","AnimationHeader"},
    {0x802d41a8,"D_802D41A8_7AD738","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_240{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,0> callback_deps_240{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_240{{
    0x802d41a8,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_240{{
}};
constexpr std::array<ActorDependencyEdge,8> calls_241{{
    {0x800a5e98,"fx_getPosVelDObj","void fx_getPosVelDObj(Vec3f* pos, Vec3f* vel, DObj* dobj);"},
    {0x0,"SQ","Source intrinsic/designator: SQ"},
    {0x0,"SQ","Source intrinsic/designator: SQ"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_241{{
    {0x802d417c,"D_802D417C_7AD70C","AnimationHeader"},
    {0x802d4190,"D_802D4190_7AD720","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,4> services_241{{
    ActorNativeService::model,
    ActorNativeService::vector,
    ActorNativeService::vector_read,
    ActorNativeService::node_world,
}};
constexpr std::array<std::uint32_t,0> callback_deps_241{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_241{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_241{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_242{{
    {0x80362dc4,"Pokemon_SpawnDlLink4","GObj* Pokemon_SpawnDlLink4(s32 gObjID, u16 id, struct WorldBlock* roomA, struct WorldBlock* roomB, ObjectSpawn* spawn, PokemonInitData* initData);"},
}};
constexpr std::array<ActorResourceDependency,3> resources_242{{
    {0x802d41a4,"D_802D41A4_7AD734","GObj*"},
    {0x802d41e8,"D_802D41E8_7AD778","PokemonInitData"},
    {0x0,"spawn","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,1> services_242{{
    ActorNativeService::reference_assign,
}};
constexpr std::array<ActorDependencyEdge,4> calls_243{{
    {0x0,"spMove","Source intrinsic/designator: spMove"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x0,"spMove","Source intrinsic/designator: spMove"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
}};
constexpr std::array<ActorResourceDependency,4> resources_243{{
    {0x802ec5ec,"D_802EC5EC_7C5B7C","s32"},
    {0x802ec5f0,"D_802EC5F0_7C5B80","Sprite*"},
    {0x802ec608,"D_802EC608_7C5B98","Sprite*"},
    {0x8034e0a0,"D_8034E0A0_827630","SObj*"},
}};
constexpr std::array<ActorNativeService,2> services_243{{
    ActorNativeService::sprite_list,
    ActorNativeService::sprite_move,
}};
constexpr std::array<std::uint32_t,0> callback_deps_243{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_243{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_243{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_244{{
    {0x80026188,"spX2Move","Source intrinsic/designator: spX2Move"},
    {0x0,"ARRAY_COUNT","Source intrinsic/designator: ARRAY_COUNT"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
}};
constexpr std::array<ActorResourceDependency,3> resources_244{{
    {0x802ec5e8,"D_802EC5E8_7C5B78","s32"},
    {0x802ec620,"D_802EC620_7C5BB0","Sprite*"},
    {0x8034e0a4,"D_8034E0A4_827634","SObj*"},
}};
constexpr std::array<ActorNativeService,2> services_244{{
    ActorNativeService::sprite_list,
    ActorNativeService::sprite_x2_move,
}};
constexpr std::array<std::uint32_t,0> callback_deps_244{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_244{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_244{{
}};
constexpr std::array<ActorDependencyEdge,25> calls_245{{
    {0x8000a410,"omAddGObj","GObj* omAddGObj(u32 id, void (*fnUpdate)(GObj*), u8 link, u32 priority);"},
    {0x8000a8a4,"omLinkGObjDL","void omLinkGObjDL(GObj* obj, void (*arg1)(GObj*), u8 dlLink, u32 dlPriority, s32 cameraTag);"},
    {0x80026188,"spX2Move","Source intrinsic/designator: spX2Move"},
    {0x8000a0b4,"omGObjAddSprite","SObj* omGObjAddSprite(GObj* obj, Sprite* sprite);"},
    {0x8000a0b4,"omGObjAddSprite","SObj* omGObjAddSprite(GObj* obj, Sprite* sprite);"},
    {0x80008c28,"omCreateProcess","GObjProcess* omCreateProcess(GObj* obj, void (*func)(struct GObj*), u8 kind, u32 pri);"},
    {0x8000a0b4,"omGObjAddSprite","SObj* omGObjAddSprite(GObj* obj, Sprite* sprite);"},
    {0x80008c28,"omCreateProcess","GObjProcess* omCreateProcess(GObj* obj, void (*func)(struct GObj*), u8 kind, u32 pri);"},
    {0x8000a0b4,"omGObjAddSprite","SObj* omGObjAddSprite(GObj* obj, Sprite* sprite);"},
    {0x80026188,"spX2Move","Source intrinsic/designator: spX2Move"},
    {0x80028850,"spScale","Source intrinsic/designator: spScale"},
    {0x80026140,"spColor","Source intrinsic/designator: spColor"},
    {0x800bf498,"getPlayerName","char* getPlayerName(void);"},
    {0x80365e60,"Msg_SetInteractive","void Msg_SetInteractive(void);"},
    {0x800374d4,"sprintf","Source intrinsic/designator: sprintf"},
    {0x80365bb0,"Msg_ShowMessage","void Msg_ShowMessage(u8* message, s32 posX, s32 posY, u8 colorR, u8 colorG, u8 colorB, u8 colorA, s32 delayBetweenChars, u8 alignment);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x80365e70,"Msg_IsMessagePrinted","bool Msg_IsMessagePrinted(void);"},
    {0x800228e4,"auPlaySound","s32 auPlaySound(u32 soundID);"},
    {0x800228e4,"auPlaySound","s32 auPlaySound(u32 soundID);"},
    {0x800228e4,"auPlaySound","s32 auPlaySound(u32 soundID);"},
    {0x80365e34,"Msg_Reset","void Msg_Reset(void);"},
    {0x80365e54,"Msg_SetNonInteractive","void Msg_SetNonInteractive(void);"},
    {0x802cfa40,"func_802CFA40_7A8FD0","void func_802CFA40_7A8FD0(GObj* obj)"},
    {0x802cfc6c,"func_802CFC6C_7A91FC","void func_802CFC6C_7A91FC(GObj* arg0)"},
}};
constexpr std::array<ActorResourceDependency,8> resources_245{{
    {0x802e6b18,"D_802E6B18_7C00A8","Sprite"},
    {0x802ec4d8,"D_802EC4D8_7C5A68","Sprite"},
    {0x802ec51c,"D_802EC51C_7C5AAC","SomeC"},
    {0x802ec5e8,"D_802EC5E8_7C5B78","s32"},
    {0x802ec5ec,"D_802EC5EC_7C5B7C","s32"},
    {0x8034e080,"D_8034E080_827610","char"},
    {0x8034e0a0,"D_8034E0A0_827630","SObj*"},
    {0x8034e0a4,"D_8034E0A4_827634","SObj*"},
}};
constexpr std::array<ActorNativeService,17> services_245{{
    ActorNativeService::sound,
    ActorNativeService::message_reset,
    ActorNativeService::message_interactive,
    ActorNativeService::message_noninteractive,
    ActorNativeService::message_printed,
    ActorNativeService::message_show,
    ActorNativeService::dialogue,
    ActorNativeService::sprite,
    ActorNativeService::sprite_add,
    ActorNativeService::sprite_x2_move,
    ActorNativeService::sprite_scale,
    ActorNativeService::sprite_color,
    ActorNativeService::sprite_link,
    ActorNativeService::object_default_add,
    ActorNativeService::input_pressed,
    ActorNativeService::player_name,
    ActorNativeService::dialogue_format,
}};
constexpr std::array<ActorDependencyEdge,14> calls_246{{
    {0x8000a410,"omAddGObj","GObj* omAddGObj(u32 id, void (*fnUpdate)(GObj*), u8 link, u32 priority);"},
    {0x8000a8a4,"omLinkGObjDL","void omLinkGObjDL(GObj* obj, void (*arg1)(GObj*), u8 dlLink, u32 dlPriority, s32 cameraTag);"},
    {0x8000a0b4,"omGObjAddSprite","SObj* omGObjAddSprite(GObj* obj, Sprite* sprite);"},
    {0x80026188,"spX2Move","Source intrinsic/designator: spX2Move"},
    {0x80028850,"spScale","Source intrinsic/designator: spScale"},
    {0x80026140,"spColor","Source intrinsic/designator: spColor"},
    {0x80365e60,"Msg_SetInteractive","void Msg_SetInteractive(void);"},
    {0x800374d4,"sprintf","Source intrinsic/designator: sprintf"},
    {0x80365bb0,"Msg_ShowMessage","void Msg_ShowMessage(u8* message, s32 posX, s32 posY, u8 colorR, u8 colorG, u8 colorB, u8 colorA, s32 delayBetweenChars, u8 alignment);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x80365e70,"Msg_IsMessagePrinted","bool Msg_IsMessagePrinted(void);"},
    {0x800228e4,"auPlaySound","s32 auPlaySound(u32 soundID);"},
    {0x80365e34,"Msg_Reset","void Msg_Reset(void);"},
    {0x80365e54,"Msg_SetNonInteractive","void Msg_SetNonInteractive(void);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_246{{
    {0x802ec4d8,"D_802EC4D8_7C5A68","Sprite"},
    {0x802ec5d0,"D_802EC5D0_7C5B60","SomeC"},
}};
constexpr std::array<ActorNativeService,16> services_246{{
    ActorNativeService::sound,
    ActorNativeService::message_reset,
    ActorNativeService::message_interactive,
    ActorNativeService::message_noninteractive,
    ActorNativeService::message_printed,
    ActorNativeService::message_show,
    ActorNativeService::dialogue,
    ActorNativeService::sprite,
    ActorNativeService::sprite_add,
    ActorNativeService::sprite_x2_move,
    ActorNativeService::sprite_scale,
    ActorNativeService::sprite_color,
    ActorNativeService::sprite_link,
    ActorNativeService::object_default_add,
    ActorNativeService::input_pressed,
    ActorNativeService::dialogue_format,
}};
constexpr std::array<ActorDependencyEdge,2> calls_247{{
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802d0344,"func_802D0344_7A98D4","POKEMON_FUNC(func_802D0344_7A98D4)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_247{{
}};
constexpr std::array<ActorNativeService,0> services_247{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_247{{
    0x802d0344,
}};
constexpr std::array<std::uint32_t,0> graph_deps_247{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_247{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_248{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_248{{
    {0x802d4220,"D_802D4220_7AD7B0","AnimationHeader"},
    {0x802ec658,"D_802EC658_7C5BE8","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_248{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,0> callback_deps_248{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_248{{
    0x802ec658,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_248{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_249{{
    {0x80366070,"PlayerModel_SetAnimationSpeed","void PlayerModel_SetAnimationSpeed(f32 speed);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_249{{
    {0x802ec648,"D_802EC648_7C5BD8","OMCamera*"},
}};
constexpr std::array<ActorNativeService,2> services_249{{
    ActorNativeService::camera_speed,
    ActorNativeService::player_model_speed,
}};
constexpr std::array<ActorDependencyEdge,2> calls_250{{
    {0x8001054c,"animSetCameraAnimation","void animSetCameraAnimation(OMCamera* cam, AnimCmd* animList, f32 skipFrames);"},
    {0x80365fec,"PlayerModel_ChangeAnimation","void PlayerModel_ChangeAnimation(AnimCmd** modelAnim, AnimCmd*** matAnim, f32 startTime);"},
}};
constexpr std::array<ActorResourceDependency,4> resources_250{{
    {0x8011da60,"D_8011DA60_413EA0","AnimCmd*"},
    {0x8011ea20,"D_8011EA20_414E60","AnimCmd**"},
    {0x801204a0,"D_801204A0_4168E0","AnimCmd"},
    {0x802ec648,"D_802EC648_7C5BD8","OMCamera*"},
}};
constexpr std::array<ActorNativeService,2> services_250{{
    ActorNativeService::camera_animation,
    ActorNativeService::player_model_change,
}};
constexpr std::array<ActorDependencyEdge,1> calls_251{{
    {0x0,"ARRAY_COUNT","Source intrinsic/designator: ARRAY_COUNT"},
}};
constexpr std::array<ActorResourceDependency,4> resources_251{{
    {0x802ec638,"D_802EC638_7C5BC8","f32"},
    {0x802ec64c,"D_802EC64C_7C5BDC","s32"},
    {0x802ec650,"D_802EC650_7C5BE0","f32"},
    {0x802ec654,"D_802EC654_7C5BE4","f32"},
}};
constexpr std::array<ActorNativeService,0> services_251{{
}};
constexpr std::array<ActorDependencyEdge,0> calls_252{{
}};
constexpr std::array<ActorResourceDependency,0> resources_252{{
}};
constexpr std::array<ActorNativeService,0> services_252{{
}};
constexpr std::array<ActorDependencyEdge,19> calls_253{{
    {0x800ab9a4,"func_800AB9A4","void func_800AB9A4(void);"},
    {0x80022a58,"auPlaySoundWithParams","s32 auPlaySoundWithParams(u32 soundID, s32 volume, s32 pan, f32 pitch, s32 reverbAmt);"},
    {0x80022334,"auStopBGM","void auStopBGM(void);"},
    {0x80022a58,"auPlaySoundWithParams","s32 auPlaySoundWithParams(u32 soundID, s32 volume, s32 pan, f32 pitch, s32 reverbAmt);"},
    {0x80022fd0,"auStopSound","void auStopSound(u32 handle);"},
    {0x80022a58,"auPlaySoundWithParams","s32 auPlaySoundWithParams(u32 soundID, s32 volume, s32 pan, f32 pitch, s32 reverbAmt);"},
    {0x800ab700,"func_800AB700","s32 func_800AB700(s32, u32, u32, s32);"},
    {0x80022454,"auSetBGMVolume","void auSetBGMVolume(s32 playerID, u32 vol);"},
    {0x80022374,"auPlaySong","u32 auPlaySong(s32 playerID, u32 songID);"},
    {0x800ab700,"func_800AB700","s32 func_800AB700(s32, u32, u32, s32);"},
    {0x80022454,"auSetBGMVolume","void auSetBGMVolume(s32 playerID, u32 vol);"},
    {0x80022374,"auPlaySong","u32 auPlaySong(s32 playerID, u32 songID);"},
    {0x800ab700,"func_800AB700","s32 func_800AB700(s32, u32, u32, s32);"},
    {0x800ab700,"func_800AB700","s32 func_800AB700(s32, u32, u32, s32);"},
    {0x800224dc,"auSetBGMVolumeSmooth","void auSetBGMVolumeSmooth(s32 playerID, u32 vol, u32 time);"},
    {0x800bfca0,"checkPlayerFlag","s32 checkPlayerFlag(s32 pfid);"},
    {0x800224dc,"auSetBGMVolumeSmooth","void auSetBGMVolumeSmooth(s32 playerID, u32 vol, u32 time);"},
    {0x800224dc,"auSetBGMVolumeSmooth","void auSetBGMVolumeSmooth(s32 playerID, u32 vol, u32 time);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
}};
constexpr std::array<ActorResourceDependency,5> resources_253{{
    {0x802ec64c,"D_802EC64C_7C5BDC","s32"},
    {0x8034c8c8,"D_8034C8C8_825E58","u8"},
    {0x8034e0ac,"D_8034E0AC_82763C","s32"},
    {0x8034e0b0,"D_8034E0B0_827640","u8"},
    {0x8034e0b1,"D_8034E0B1_827641","u8"},
}};
constexpr std::array<ActorNativeService,9> services_253{{
    ActorNativeService::sound_params,
    ActorNativeService::sound_stop,
    ActorNativeService::sound_volume_smooth,
    ActorNativeService::bgm_play,
    ActorNativeService::bgm_stop,
    ActorNativeService::bgm_volume,
    ActorNativeService::bgm_volume_smooth,
    ActorNativeService::check_player_flag,
    ActorNativeService::audio_cutscene_reset,
}};
constexpr std::array<std::uint32_t,0> callback_deps_253{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_253{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_253{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_254{{
    {0x8000a410,"omAddGObj","GObj* omAddGObj(u32 id, void (*fnUpdate)(GObj*), u8 link, u32 priority);"},
    {0x80008c28,"omCreateProcess","GObjProcess* omCreateProcess(GObj* obj, void (*func)(struct GObj*), u8 kind, u32 pri);"},
    {0x802d04cc,"func_802D04CC_7A9A5C","POKEMON_FUNC(func_802D04CC_7A9A5C)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_254{{
    {0x8034e0a8,"D_8034E0A8_827638","GObj*"},
}};
constexpr std::array<ActorNativeService,2> services_254{{
    ActorNativeService::reference_assign,
    ActorNativeService::object_default_add,
}};
constexpr std::array<ActorDependencyEdge,1> calls_255{{
    {0x80022a58,"auPlaySoundWithParams","s32 auPlaySoundWithParams(u32 soundID, s32 volume, s32 pan, f32 pitch, s32 reverbAmt);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_255{{
}};
constexpr std::array<ActorNativeService,1> services_255{{
    ActorNativeService::sound_params,
}};
constexpr std::array<ActorDependencyEdge,42> calls_256{{
    {0x800224dc,"auSetBGMVolumeSmooth","void auSetBGMVolumeSmooth(s32 playerID, u32 vol, u32 time);"},
    {0x8035cab0,"Items_RemoveFlyingItems","void Items_RemoveFlyingItems(void);"},
    {0x803573bc,"resetMainCameraSettings","void resetMainCameraSettings(void);"},
    {0x80365e34,"Msg_Reset","void Msg_Reset(void);"},
    {0x8000b774,"cmdSendCommand","s32 cmdSendCommand(struct GObj* obj, s32 cmd, struct GObj* source);"},
    {0x800e1a78,"setSkyBoxAnimationSpeed","void setSkyBoxAnimationSpeed(f32 arg0);"},
    {0x803586f8,"getMainCamera","OMCamera* getMainCamera(void);"},
    {0x8000bd44,"ohPauseObjectProcesses","void ohPauseObjectProcesses(GObj* obj);"},
    {0x80008c28,"omCreateProcess","GObjProcess* omCreateProcess(GObj* obj, void (*func)(struct GObj*), u8 kind, u32 pri);"},
    {0x80365e80,"PlayerModel_Init","GObj* PlayerModel_Init(void);"},
    {0x8000c5b4,"animSetModelAnimationSpeed","void animSetModelAnimationSpeed(GObj* obj, f32 speed);"},
    {0x802d03d0,"func_802D03D0_7A9960","void func_802D03D0_7A9960(f32 skipFrames)"},
    {0x802d039c,"func_802D039C_7A992C","void func_802D039C_7A992C(f32 speed)"},
    {0x802d0700,"func_802D0700_7A9C90","void func_802D0700_7A9C90(void)"},
    {0x802d039c,"func_802D039C_7A992C","void func_802D039C_7A992C(f32 speed)"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x802d039c,"func_802D039C_7A992C","void func_802D039C_7A992C(f32 speed)"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x80365e48,"Msg_SetSpecialHandler","void Msg_SetSpecialHandler(void (*handler)(u8));"},
    {0x0,"void","typedef void (*GObjFunc)(struct GObj*);"},
    {0x800bfca0,"checkPlayerFlag","s32 checkPlayerFlag(s32 pfid);"},
    {0x802cfe8c,"func_802CFE8C_7A941C","void func_802CFE8C_7A941C(void)"},
    {0x800aaed0,"func_800AAED0","void func_800AAED0(s32);"},
    {0x800bfebc,"setPlayerFlag","void setPlayerFlag(s32, s32);"},
    {0x802d017c,"func_802D017C_7A970C","void func_802D017C_7A970C(void)"},
    {0x80365e48,"Msg_SetSpecialHandler","void Msg_SetSpecialHandler(void (*handler)(u8));"},
    {0x800a7f68,"func_800A7F68","void func_800A7F68(s32, s32);"},
    {0x800a7f40,"func_800A7F40","Source intrinsic/designator: func_800A7F40"},
    {0x800224dc,"auSetBGMVolumeSmooth","void auSetBGMVolumeSmooth(s32 playerID, u32 vol, u32 time);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x800a7460,"func_800A7460","s32 func_800A7460(void);"},
    {0x80023474,"auSetSoundGlobalVolume","void auSetSoundGlobalVolume(u8 vol);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x80023474,"auSetSoundGlobalVolume","void auSetSoundGlobalVolume(u8 vol);"},
    {0x8000b774,"cmdSendCommand","s32 cmdSendCommand(struct GObj* obj, s32 cmd, struct GObj* source);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
    {0x8000a52c,"omDeleteGObj","void omDeleteGObj(GObj* obj);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802d04bc,"func_802D04BC_7A9A4C","void func_802D04BC_7A9A4C(DObj* arg0, s32 arg1, f32 arg2)"},
    {0x802d0420,"func_802D0420_7A99B0","void func_802D0420_7A99B0(DObj* arg0, s32 arg1, f32 arg2)"},
    {0x802d0754,"func_802D0754_7A9CE4","void func_802D0754_7A9CE4(s32 arg0)"},
}};
constexpr std::array<ActorResourceDependency,6> resources_256{{
    {0x80100730,"D_80100730_3F6B70","f32"},
    {0x802ec648,"D_802EC648_7C5BD8","OMCamera*"},
    {0x802ec64c,"D_802EC64C_7C5BDC","s32"},
    {0x802ec650,"D_802EC650_7C5BE0","f32"},
    {0x802ec654,"D_802EC654_7C5BE4","f32"},
    {0x8034e0a8,"D_8034E0A8_827638","GObj*"},
}};
constexpr std::array<ActorNativeService,22> services_256{{
    ActorNativeService::animation_callback,
    ActorNativeService::model_animation_speed,
    ActorNativeService::camera,
    ActorNativeService::camera_object,
    ActorNativeService::camera_animation_process,
    ActorNativeService::pause_processes,
    ActorNativeService::player_model_init,
    ActorNativeService::sound_global_volume,
    ActorNativeService::bgm_volume_smooth,
    ActorNativeService::check_player_flag,
    ActorNativeService::set_player_flag,
    ActorNativeService::remove_flying_items,
    ActorNativeService::reset_camera,
    ActorNativeService::sky_speed,
    ActorNativeService::message_reset,
    ActorNativeService::message_handler,
    ActorNativeService::player_object,
    ActorNativeService::application_notice,
    ActorNativeService::transition_begin,
    ActorNativeService::transition_color,
    ActorNativeService::transition_busy,
    ActorNativeService::command,
}};
constexpr std::array<std::uint32_t,0> callback_deps_256{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_256{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_256{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_257{{
    {0x80362dc4,"Pokemon_SpawnDlLink4","GObj* Pokemon_SpawnDlLink4(s32 gObjID, u16 id, struct WorldBlock* roomA, struct WorldBlock* roomB, ObjectSpawn* spawn, PokemonInitData* initData);"},
}};
constexpr std::array<ActorResourceDependency,3> resources_257{{
    {0x802ec644,"D_802EC644_7C5BD4","GObj*"},
    {0x802ec68c,"D_802EC68C_7C5C1C","PokemonInitData"},
    {0x0,"spawn","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,1> services_257{{
    ActorNativeService::reference_assign,
}};
constexpr std::array<ActorDependencyEdge,2> calls_258{{
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802d0ba4,"func_802D0BA4_7AA134","POKEMON_FUNC(func_802D0BA4_7AA134)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_258{{
}};
constexpr std::array<ActorNativeService,0> services_258{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_258{{
    0x802d0ba4,
}};
constexpr std::array<std::uint32_t,0> graph_deps_258{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_258{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_259{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_259{{
    {0x802ec6c0,"D_802EC6C0_7C5C50","AnimationHeader"},
    {0x802ec718,"D_802EC718_7C5CA8","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_259{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,0> callback_deps_259{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_259{{
    0x802ec718,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_259{{
}};
constexpr std::array<ActorDependencyEdge,12> calls_260{{
    {0x8000b774,"cmdSendCommand","s32 cmdSendCommand(struct GObj* obj, s32 cmd, struct GObj* source);"},
    {0x800228e4,"auPlaySound","s32 auPlaySound(u32 soundID);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x800228e4,"auPlaySound","s32 auPlaySound(u32 soundID);"},
    {0x800228e4,"auPlaySound","s32 auPlaySound(u32 soundID);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802d0f98,"func_802D0F98_7AA528","POKEMON_FUNC(func_802D0F98_7AA528)"},
}};
constexpr std::array<ActorResourceDependency,4> resources_260{{
    {0x802ec6d4,"D_802EC6D4_7C5C64","AnimationHeader"},
    {0x802ec6e8,"D_802EC6E8_7C5C78","AnimationHeader"},
    {0x802ec6fc,"D_802EC6FC_7C5C8C","AnimationHeader"},
    {0x802ec710,"D_802EC710_7C5CA0","s32"},
}};
constexpr std::array<ActorNativeService,3> services_260{{
    ActorNativeService::sound,
    ActorNativeService::player_object,
    ActorNativeService::command,
}};
constexpr std::array<std::uint32_t,1> callback_deps_260{{
    0x802d0f98,
}};
constexpr std::array<std::uint32_t,0> graph_deps_260{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_260{{
}};
constexpr std::array<ActorDependencyEdge,0> calls_261{{
}};
constexpr std::array<ActorResourceDependency,1> resources_261{{
    {0x802ec714,"D_802EC714_7C5CA4","s32"},
}};
constexpr std::array<ActorNativeService,1> services_261{{
    ActorNativeService::model_time_left,
}};
constexpr std::array<ActorDependencyEdge,8> calls_262{{
    {0x80022454,"auSetBGMVolume","void auSetBGMVolume(s32 playerID, u32 vol);"},
    {0x80022454,"auSetBGMVolume","void auSetBGMVolume(s32 playerID, u32 vol);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x80022454,"auSetBGMVolume","void auSetBGMVolume(s32 playerID, u32 vol);"},
    {0x80022454,"auSetBGMVolume","void auSetBGMVolume(s32 playerID, u32 vol);"},
    {0x80022c28,"auSetSoundVolume","void auSetSoundVolume(s32 handle, u32 vol);"},
    {0x8035edc8,"Pokemon_StopAuxProc","void Pokemon_StopAuxProc(GObj*);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_262{{
    {0x8034c8c8,"D_8034C8C8_825E58","u8"},
}};
constexpr std::array<ActorNativeService,2> services_262{{
    ActorNativeService::sound_volume,
    ActorNativeService::bgm_volume,
}};
constexpr std::array<std::uint32_t,0> callback_deps_262{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_262{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_262{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_263{{
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x800228e4,"auPlaySound","s32 auPlaySound(u32 soundID);"},
    {0x80022fd0,"auStopSound","void auStopSound(u32 handle);"},
    {0x8035edc8,"Pokemon_StopAuxProc","void Pokemon_StopAuxProc(GObj*);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_263{{
    {0x802ec710,"D_802EC710_7C5CA0","s32"},
}};
constexpr std::array<ActorNativeService,3> services_263{{
    ActorNativeService::sound,
    ActorNativeService::sound_active,
    ActorNativeService::sound_stop,
}};
constexpr std::array<std::uint32_t,0> callback_deps_263{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_263{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_263{{
}};
constexpr std::array<ActorDependencyEdge,25> calls_264{{
    {0x8035ed90,"Pokemon_StartAuxProc","void Pokemon_StartAuxProc(GObj*, void (*)(GObj*));"},
    {0x8035ed90,"Pokemon_StartAuxProc","void Pokemon_StartAuxProc(GObj*, void (*)(GObj*));"},
    {0x8035cab0,"Items_RemoveFlyingItems","void Items_RemoveFlyingItems(void);"},
    {0x803573bc,"resetMainCameraSettings","void resetMainCameraSettings(void);"},
    {0x80365e34,"Msg_Reset","void Msg_Reset(void);"},
    {0x8035fd9c,"Pokemon_RemovePokemons","void Pokemon_RemovePokemons(u16*);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x803570f0,"Camera_StartCutScene","void Camera_StartCutScene(GObj* pokemon, AnimCmd* camAnim, f32 time);"},
    {0x803586f8,"getMainCamera","OMCamera* getMainCamera(void);"},
    {0x8000bd44,"ohPauseObjectProcesses","void ohPauseObjectProcesses(GObj* obj);"},
    {0x8001054c,"animSetCameraAnimation","void animSetCameraAnimation(OMCamera* cam, AnimCmd* animList, f32 skipFrames);"},
    {0x80008c28,"omCreateProcess","GObjProcess* omCreateProcess(GObj* obj, void (*func)(struct GObj*), u8 kind, u32 pri);"},
    {0x800e1a78,"setSkyBoxAnimationSpeed","void setSkyBoxAnimationSpeed(f32 arg0);"},
    {0x80365e80,"PlayerModel_Init","GObj* PlayerModel_Init(void);"},
    {0x8000b774,"cmdSendCommand","s32 cmdSendCommand(struct GObj* obj, s32 cmd, struct GObj* source);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
    {0x80365f38,"PlayerModel_SetAnimation","void PlayerModel_SetAnimation(AnimCmd** modelAnim, AnimCmd*** matAnim, f32 startTime, f32 speed);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x80022b14,"auStopAllSounds","void auStopAllSounds(void);"},
    {0x8000b774,"cmdSendCommand","s32 cmdSendCommand(struct GObj* obj, s32 cmd, struct GObj* source);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802d0d0c,"func_802D0D0C_7AA29C","POKEMON_FUNC(func_802D0D0C_7AA29C)"},
    {0x802d0f0c,"func_802D0F0C_7AA49C","POKEMON_FUNC(func_802D0F0C_7AA49C)"},
    {0x802d0cd8,"func_802D0CD8_7AA268","void func_802D0CD8_7AA268(DObj* obj, s32 arg1, f32 arg2)"},
}};
constexpr std::array<ActorResourceDependency,3> resources_264{{
    {0x8011c5a0,"D_8011C5A0_4129E0","AnimCmd*"},
    {0x8011cfb0,"D_8011CFB0_4133F0","AnimCmd**"},
    {0x802ec714,"D_802EC714_7C5CA4","s32"},
}};
constexpr std::array<ActorNativeService,17> services_264{{
    ActorNativeService::animation_callback,
    ActorNativeService::camera,
    ActorNativeService::camera_speed,
    ActorNativeService::camera_object,
    ActorNativeService::camera_animation,
    ActorNativeService::camera_animation_process,
    ActorNativeService::pause_processes,
    ActorNativeService::player_model_init,
    ActorNativeService::player_model_animation,
    ActorNativeService::cutscene_start,
    ActorNativeService::sound_stop_all,
    ActorNativeService::remove_flying_items,
    ActorNativeService::reset_camera,
    ActorNativeService::sky_speed,
    ActorNativeService::message_reset,
    ActorNativeService::player_object,
    ActorNativeService::command,
}};
constexpr std::array<std::uint32_t,2> callback_deps_264{{
    0x802d0d0c,
    0x802d0f0c,
}};
constexpr std::array<std::uint32_t,0> graph_deps_264{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_264{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_265{{
    {0x80362dc4,"Pokemon_SpawnDlLink4","GObj* Pokemon_SpawnDlLink4(s32 gObjID, u16 id, struct WorldBlock* roomA, struct WorldBlock* roomB, ObjectSpawn* spawn, PokemonInitData* initData);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_265{{
    {0x802ec750,"D_802EC750_7C5CE0","PokemonInitData"},
    {0x0,"spawn","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,0> services_265{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_266{{
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_266{{
}};
constexpr std::array<ActorNativeService,0> services_266{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_266{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_266{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_266{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_267{{
    {0x80362e5c,"Pokemon_Spawn","GObj* Pokemon_Spawn(s32 gObjID, u16 id, struct WorldBlock* roomA, struct WorldBlock* roomB, ObjectSpawn* spawn, PokemonInitData* initData);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_267{{
    {0x0,"spawn","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,0> services_267{{
}};
constexpr std::array<ActorDependencyEdge,7> calls_268{{
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ecac,"Pokemon_SetStateRandom","void Pokemon_SetStateRandom(GObj* obj, RandomState* nextStates);"},
    {0x802d1454,"func_802D1454_7AA9E4","POKEMON_FUNC(func_802D1454_7AA9E4)"},
    {0x802d1598,"func_802D1598_7AAB28","POKEMON_FUNC(func_802D1598_7AAB28)"},
    {0x802d1bc8,"func_802D1BC8_7AB158","POKEMON_FUNC(func_802D1BC8_7AB158)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_268{{
    {0x802ec870,"D_802EC870_7C5E00","RandomState"},
}};
constexpr std::array<ActorNativeService,1> services_268{{
    ActorNativeService::weighted,
}};
constexpr std::array<std::uint32_t,3> callback_deps_268{{
    0x802d1454,
    0x802d1598,
    0x802d1bc8,
}};
constexpr std::array<std::uint32_t,0> graph_deps_268{{
}};
constexpr std::array<std::uint32_t,1> weighted_deps_268{{
    0x802ec870,
}};
constexpr std::array<ActorDependencyEdge,3> calls_269{{
    {0x8035f15c,"Pokemon_ForceAnimation","void Pokemon_ForceAnimation(GObj*, AnimationHeader*);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802d1344,"func_802D1344_7AA8D4","POKEMON_FUNC(func_802D1344_7AA8D4)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_269{{
    {0x802ec82c,"D_802EC82C_7C5DBC","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_269{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_269{{
    0x802d1344,
}};
constexpr std::array<std::uint32_t,0> graph_deps_269{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_269{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_270{{
    {0x8035f15c,"Pokemon_ForceAnimation","void Pokemon_ForceAnimation(GObj*, AnimationHeader*);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802d1344,"func_802D1344_7AA8D4","POKEMON_FUNC(func_802D1344_7AA8D4)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_270{{
    {0x802ec840,"D_802EC840_7C5DD0","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_270{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_270{{
    0x802d1344,
}};
constexpr std::array<std::uint32_t,0> graph_deps_270{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_270{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_271{{
    {0x8035f15c,"Pokemon_ForceAnimation","void Pokemon_ForceAnimation(GObj*, AnimationHeader*);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802d1344,"func_802D1344_7AA8D4","POKEMON_FUNC(func_802D1344_7AA8D4)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_271{{
    {0x802ec854,"D_802EC854_7C5DE4","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_271{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_271{{
    0x802d1344,
}};
constexpr std::array<std::uint32_t,0> graph_deps_271{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_271{{
}};
constexpr std::array<ActorDependencyEdge,8> calls_272{{
    {0x80019e14,"randRange","Source intrinsic/designator: randRange"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035fd70,"Pokemon_RunCleanup","void Pokemon_RunCleanup(GObj* obj);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802d1414,"func_802D1414_7AA9A4","POKEMON_FUNC(func_802D1414_7AA9A4)"},
    {0x802d1454,"func_802D1454_7AA9E4","POKEMON_FUNC(func_802D1454_7AA9E4)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_272{{
    {0x802d2980,"D_802D2980_7ABF10","GObj*"},
}};
constexpr std::array<ActorNativeService,5> services_272{{
    ActorNativeService::reference_assign,
    ActorNativeService::model,
    ActorNativeService::vector,
    ActorNativeService::vector_store,
    ActorNativeService::rng_range,
}};
constexpr std::array<std::uint32_t,2> callback_deps_272{{
    0x802d1414,
    0x802d1454,
}};
constexpr std::array<std::uint32_t,0> graph_deps_272{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_272{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_273{{
    {0x8035e298,"func_8035E298_4FE6A8","void func_8035E298_4FE6A8(GObj* obj);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_273{{
}};
constexpr std::array<ActorNativeService,0> services_273{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_273{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_273{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_273{{
}};
constexpr std::array<ActorDependencyEdge,8> calls_274{{
    {0x80019e14,"randRange","Source intrinsic/designator: randRange"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x800e41d8,"getGroundAt","s32 getGroundAt(f32 arg0, f32 arg1, GroundResult* arg2);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035e174,"func_8035E174_4FE584","void func_8035E174_4FE584(GObj* obj, Vec3f*);"},
    {0x8035ecac,"Pokemon_SetStateRandom","void Pokemon_SetStateRandom(GObj* obj, RandomState* nextStates);"},
    {0x802d156c,"func_802D156C_7AAAFC","POKEMON_FUNC(func_802D156C_7AAAFC)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_274{{
    {0x802ec870,"D_802EC870_7C5E00","RandomState"},
}};
constexpr std::array<ActorNativeService,6> services_274{{
    ActorNativeService::model,
    ActorNativeService::vector,
    ActorNativeService::vector_read,
    ActorNativeService::weighted,
    ActorNativeService::ground,
    ActorNativeService::rng_range,
}};
constexpr std::array<std::uint32_t,1> callback_deps_274{{
    0x802d156c,
}};
constexpr std::array<std::uint32_t,0> graph_deps_274{{
}};
constexpr std::array<std::uint32_t,1> weighted_deps_274{{
    0x802ec870,
}};
constexpr std::array<ActorDependencyEdge,2> calls_275{{
    {0x8035fd70,"Pokemon_RunCleanup","void Pokemon_RunCleanup(GObj* obj);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_275{{
}};
constexpr std::array<ActorNativeService,0> services_275{{
}};
constexpr std::array<std::uint32_t,0> callback_deps_275{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_275{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_275{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_276{{
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802d15bc,"func_802D15BC_7AAB4C","POKEMON_FUNC(func_802D15BC_7AAB4C)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_276{{
}};
constexpr std::array<ActorNativeService,0> services_276{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_276{{
    0x802d15bc,
}};
constexpr std::array<std::uint32_t,0> graph_deps_276{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_276{{
}};
constexpr std::array<ActorDependencyEdge,11> calls_277{{
    {0x80019e14,"randRange","Source intrinsic/designator: randRange"},
    {0x8035f15c,"Pokemon_ForceAnimation","void Pokemon_ForceAnimation(GObj*, AnimationHeader*);"},
    {0x8035f15c,"Pokemon_ForceAnimation","void Pokemon_ForceAnimation(GObj*, AnimationHeader*);"},
    {0x8035f15c,"Pokemon_ForceAnimation","void Pokemon_ForceAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x80019e14,"randRange","Source intrinsic/designator: randRange"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802d1414,"func_802D1414_7AA9A4","POKEMON_FUNC(func_802D1414_7AA9A4)"},
    {0x802d15bc,"func_802D15BC_7AAB4C","POKEMON_FUNC(func_802D15BC_7AAB4C)"},
}};
constexpr std::array<ActorResourceDependency,4> resources_277{{
    {0x802ec82c,"D_802EC82C_7C5DBC","AnimationHeader"},
    {0x802ec840,"D_802EC840_7C5DD0","AnimationHeader"},
    {0x802ec854,"D_802EC854_7C5DE4","AnimationHeader"},
    {0x802ec898,"D_802EC898_7C5E28","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,2> services_277{{
    ActorNativeService::graph,
    ActorNativeService::rng_range,
}};
constexpr std::array<std::uint32_t,2> callback_deps_277{{
    0x802d1414,
    0x802d15bc,
}};
constexpr std::array<std::uint32_t,1> graph_deps_277{{
    0x802ec898,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_277{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_278{{
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_278{{
    {0x802ec8b8,"D_802EC8B8_7C5E48","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_278{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,0> callback_deps_278{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_278{{
    0x802ec8b8,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_278{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_279{{
    {0x80357384,"getLevelProgress","void getLevelProgress(s32* blockCount, f32* blockPart);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802d16cc,"func_802D16CC_7AAC5C","POKEMON_FUNC(func_802D16CC_7AAC5C)"},
    {0x802d17d8,"func_802D17D8_7AAD68","POKEMON_FUNC(func_802D17D8_7AAD68)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_279{{
}};
constexpr std::array<ActorNativeService,1> services_279{{
    ActorNativeService::level_progress,
}};
constexpr std::array<std::uint32_t,2> callback_deps_279{{
    0x802d16cc,
    0x802d17d8,
}};
constexpr std::array<std::uint32_t,0> graph_deps_279{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_279{{
}};
constexpr std::array<ActorDependencyEdge,11> calls_280{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035ed90,"Pokemon_StartAuxProc","void Pokemon_StartAuxProc(GObj*, void (*)(GObj*));"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802d18d8,"func_802D18D8_7AAE68","POKEMON_FUNC(func_802D18D8_7AAE68)"},
    {0x802d19dc,"func_802D19DC_7AAF6C","POKEMON_FUNC(func_802D19DC_7AAF6C)"},
    {0x802d1acc,"func_802D1ACC_7AB05C","POKEMON_FUNC(func_802D1ACC_7AB05C)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_280{{
    {0x802ec890,"D_802EC890_7C5E20","s32"},
}};
constexpr std::array<ActorNativeService,1> services_280{{
    ActorNativeService::forbidden_ground,
}};
constexpr std::array<std::uint32_t,3> callback_deps_280{{
    0x802d18d8,
    0x802d19dc,
    0x802d1acc,
}};
constexpr std::array<std::uint32_t,0> graph_deps_280{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_280{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_281{{
    {0x0,"GET_TRANSFORM","Source intrinsic/designator: GET_TRANSFORM"},
    {0x80019abc,"atan2f","Source intrinsic/designator: atan2f"},
    {0x80360ab8,"Pokemon_FallDownOnGround","void Pokemon_FallDownOnGround(GObj* obj, f32 acceleration, s32);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_281{{
}};
constexpr std::array<ActorNativeService,3> services_281{{
    ActorNativeService::model,
    ActorNativeService::vector,
    ActorNativeService::vector_read,
}};
constexpr std::array<std::uint32_t,0> callback_deps_281{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_281{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_281{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_282{{
    {0x8035f2a4,"Pokemon_GetDistance","f32 Pokemon_GetDistance(GObj* obj1, GObj* obj2);"},
    {0x8000b774,"cmdSendCommand","s32 cmdSendCommand(struct GObj* obj, s32 cmd, struct GObj* source);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x8035f2a4,"Pokemon_GetDistance","f32 Pokemon_GetDistance(GObj* obj1, GObj* obj2);"},
    {0x8035edc8,"Pokemon_StopAuxProc","void Pokemon_StopAuxProc(GObj*);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_282{{
    {0x802d3ff4,"D_802D3FF4_7AD584","GObj*"},
    {0x802ec868,"D_802EC868_7C5DF8","u16"},
}};
constexpr std::array<ActorNativeService,1> services_282{{
    ActorNativeService::command,
}};
constexpr std::array<std::uint32_t,0> callback_deps_282{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_282{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_282{{
}};
constexpr std::array<ActorDependencyEdge,6> calls_283{{
    {0x803667c0,"EnvSound_PlaySound","void EnvSound_PlaySound(GObj* obj, u8 category, s32 soundID);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035fd70,"Pokemon_RunCleanup","void Pokemon_RunCleanup(GObj* obj);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802d1b48,"func_802D1B48_7AB0D8","POKEMON_FUNC(func_802D1B48_7AB0D8)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_283{{
    {0x802ec86c,"D_802EC86C_7C5DFC","u16"},
}};
constexpr std::array<ActorNativeService,1> services_283{{
    ActorNativeService::environment_sound,
}};
constexpr std::array<std::uint32_t,1> callback_deps_283{{
    0x802d1b48,
}};
constexpr std::array<std::uint32_t,0> graph_deps_283{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_283{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_284{{
    {0x80360ab8,"Pokemon_FallDownOnGround","void Pokemon_FallDownOnGround(GObj* obj, f32 acceleration, s32);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_284{{
}};
constexpr std::array<ActorNativeService,3> services_284{{
    ActorNativeService::model,
    ActorNativeService::vector,
    ActorNativeService::vector_store,
}};
constexpr std::array<std::uint32_t,0> callback_deps_284{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_284{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_284{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_285{{
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802d1c40,"func_802D1C40_7AB1D0","POKEMON_FUNC(func_802D1C40_7AB1D0)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_285{{
}};
constexpr std::array<ActorNativeService,0> services_285{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_285{{
    0x802d1c40,
}};
constexpr std::array<std::uint32_t,0> graph_deps_285{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_285{{
}};
constexpr std::array<ActorDependencyEdge,10> calls_286{{
    {0x80357384,"getLevelProgress","void getLevelProgress(s32* blockCount, f32* blockPart);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035ed90,"Pokemon_StartAuxProc","void Pokemon_StartAuxProc(GObj*, void (*)(GObj*));"},
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802d1d4c,"func_802D1D4C_7AB2DC","POKEMON_FUNC(func_802D1D4C_7AB2DC)"},
    {0x802d1e3c,"func_802D1E3C_7AB3CC","POKEMON_FUNC(func_802D1E3C_7AB3CC)"},
    {0x802d1eb8,"func_802D1EB8_7AB448","POKEMON_FUNC(func_802D1EB8_7AB448)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_286{{
    {0x802ec818,"D_802EC818_7C5DA8","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,1> services_286{{
    ActorNativeService::level_progress,
}};
constexpr std::array<std::uint32_t,3> callback_deps_286{{
    0x802d1d4c,
    0x802d1e3c,
    0x802d1eb8,
}};
constexpr std::array<std::uint32_t,0> graph_deps_286{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_286{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_287{{
    {0x800e2184,"getCurrentWorldBlock","WorldBlock* getCurrentWorldBlock(void);"},
    {0x8001fca4,"GetInterpolatedPosition","void GetInterpolatedPosition(Vec3f*, InterpData*, f32);"},
    {0x80360ab8,"Pokemon_FallDownOnGround","void Pokemon_FallDownOnGround(GObj* obj, f32 acceleration, s32);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_287{{
    {0x802ec8d8,"D_802EC8D8_7C5E68","Vec3f"},
}};
constexpr std::array<ActorNativeService,6> services_287{{
    ActorNativeService::model,
    ActorNativeService::vector,
    ActorNativeService::vector_store,
    ActorNativeService::current_block,
    ActorNativeService::path_position,
    ActorNativeService::path_start,
}};
constexpr std::array<std::uint32_t,0> callback_deps_287{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_287{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_287{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_288{{
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x803667c0,"EnvSound_PlaySound","void EnvSound_PlaySound(GObj* obj, u8 category, s32 soundID);"},
    {0x8035edc8,"Pokemon_StopAuxProc","void Pokemon_StopAuxProc(GObj*);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_288{{
}};
constexpr std::array<ActorNativeService,1> services_288{{
    ActorNativeService::environment_sound,
}};
constexpr std::array<std::uint32_t,0> callback_deps_288{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_288{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_288{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_289{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_289{{
    {0x802ec818,"D_802EC818_7C5DA8","AnimationHeader"},
    {0x802ec8e4,"D_802EC8E4_7C5E74","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_289{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,0> callback_deps_289{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_289{{
    0x802ec8e4,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_289{{
}};
constexpr std::array<ActorDependencyEdge,0> calls_290{{
}};
constexpr std::array<ActorResourceDependency,0> resources_290{{
}};
constexpr std::array<ActorNativeService,0> services_290{{
}};
constexpr std::array<ActorDependencyEdge,10> calls_291{{
    {0x8035f138,"Pokemon_SetAnimation","void Pokemon_SetAnimation(GObj*, AnimationHeader*);"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035ed90,"Pokemon_StartAuxProc","void Pokemon_StartAuxProc(GObj*, void (*)(GObj*));"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x8000b774,"cmdSendCommand","s32 cmdSendCommand(struct GObj* obj, s32 cmd, struct GObj* source);"},
    {0x8035fd70,"Pokemon_RunCleanup","void Pokemon_RunCleanup(GObj* obj);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802d1fc0,"func_802D1FC0_7AB550","POKEMON_FUNC(func_802D1FC0_7AB550)"},
    {0x802d2128,"func_802D2128_7AB6B8","POKEMON_FUNC(func_802D2128_7AB6B8)"},
}};
constexpr std::array<ActorResourceDependency,3> resources_291{{
    {0x802d2d74,"D_802D2D74_7AC304","GObj*"},
    {0x802ec818,"D_802EC818_7C5DA8","AnimationHeader"},
    {0x802ec894,"D_802EC894_7C5E24","f32"},
}};
constexpr std::array<ActorNativeService,2> services_291{{
    ActorNativeService::forbidden_ground,
    ActorNativeService::command,
}};
constexpr std::array<std::uint32_t,2> callback_deps_291{{
    0x802d1fc0,
    0x802d2128,
}};
constexpr std::array<std::uint32_t,0> graph_deps_291{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_291{{
}};
constexpr std::array<ActorDependencyEdge,6> calls_292{{
    {0x800e2184,"getCurrentWorldBlock","WorldBlock* getCurrentWorldBlock(void);"},
    {0x8001fca4,"GetInterpolatedPosition","void GetInterpolatedPosition(Vec3f*, InterpData*, f32);"},
    {0x80019abc,"atan2f","Source intrinsic/designator: atan2f"},
    {0x80360ab8,"Pokemon_FallDownOnGround","void Pokemon_FallDownOnGround(GObj* obj, f32 acceleration, s32);"},
    {0x803667c0,"EnvSound_PlaySound","void EnvSound_PlaySound(GObj* obj, u8 category, s32 soundID);"},
    {0x8035e174,"func_8035E174_4FE584","void func_8035E174_4FE584(GObj* obj, Vec3f*);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_292{{
}};
constexpr std::array<ActorNativeService,6> services_292{{
    ActorNativeService::model,
    ActorNativeService::vector,
    ActorNativeService::vector_read,
    ActorNativeService::current_block,
    ActorNativeService::path_position,
    ActorNativeService::environment_sound,
}};
constexpr std::array<std::uint32_t,0> callback_deps_292{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_292{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_292{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_293{{
    {0x0,"GET_TRANSFORM","Source intrinsic/designator: GET_TRANSFORM"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x8035edc8,"Pokemon_StopAuxProc","void Pokemon_StopAuxProc(GObj*);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_293{{
}};
constexpr std::array<ActorNativeService,3> services_293{{
    ActorNativeService::model,
    ActorNativeService::vector,
    ActorNativeService::vector_read,
}};
constexpr std::array<std::uint32_t,0> callback_deps_293{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_293{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_293{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_294{{
    {0x80362ee0,"Pokemon_SpawnOnGround","GObj* Pokemon_SpawnOnGround(s32 gObjID, u16 id, struct WorldBlock* roomA, struct WorldBlock* roomB, ObjectSpawn* spawn, PokemonInitData* initData);"},
    {0x80362e5c,"Pokemon_Spawn","GObj* Pokemon_Spawn(s32 gObjID, u16 id, struct WorldBlock* roomA, struct WorldBlock* roomB, ObjectSpawn* spawn, PokemonInitData* initData);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_294{{
    {0x802ec934,"D_802EC934_7C5EC4","PokemonInitData"},
}};
constexpr std::array<ActorNativeService,0> services_294{{
}};
constexpr std::array<ActorDependencyEdge,4> calls_295{{
    {0x0,"HIDE_POKEMON","Source intrinsic/designator: HIDE_POKEMON"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035fd70,"Pokemon_RunCleanup","void Pokemon_RunCleanup(GObj* obj);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_295{{
    {0x802ec9d0,"D_802EC9D0_7C5F60","InteractionHandler"},
}};
constexpr std::array<ActorNativeService,1> services_295{{
    ActorNativeService::graph,
}};
constexpr std::array<std::uint32_t,0> callback_deps_295{{
}};
constexpr std::array<std::uint32_t,1> graph_deps_295{{
    0x802ec9d0,
}};
constexpr std::array<std::uint32_t,0> weighted_deps_295{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_296{{
    {0x0,"SHOW_POKEMON","Source intrinsic/designator: SHOW_POKEMON"},
    {0x8035ecac,"Pokemon_SetStateRandom","void Pokemon_SetStateRandom(GObj* obj, RandomState* nextStates);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_296{{
    {0x802ec9f0,"D_802EC9F0_7C5F80","RandomState"},
}};
constexpr std::array<ActorNativeService,1> services_296{{
    ActorNativeService::weighted,
}};
constexpr std::array<std::uint32_t,0> callback_deps_296{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_296{{
}};
constexpr std::array<std::uint32_t,1> weighted_deps_296{{
    0x802ec9f0,
}};
constexpr std::array<ActorDependencyEdge,3> calls_297{{
    {0x8035f15c,"Pokemon_ForceAnimation","void Pokemon_ForceAnimation(GObj*, AnimationHeader*);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802d237c,"func_802D237C_7AB90C","POKEMON_FUNC(func_802D237C_7AB90C)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_297{{
    {0x802ec994,"D_802EC994_7C5F24","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_297{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_297{{
    0x802d237c,
}};
constexpr std::array<std::uint32_t,0> graph_deps_297{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_297{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_298{{
    {0x8035f15c,"Pokemon_ForceAnimation","void Pokemon_ForceAnimation(GObj*, AnimationHeader*);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802d237c,"func_802D237C_7AB90C","POKEMON_FUNC(func_802D237C_7AB90C)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_298{{
    {0x802ec9a8,"D_802EC9A8_7C5F38","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_298{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_298{{
    0x802d237c,
}};
constexpr std::array<std::uint32_t,0> graph_deps_298{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_298{{
}};
constexpr std::array<ActorDependencyEdge,3> calls_299{{
    {0x8035f15c,"Pokemon_ForceAnimation","void Pokemon_ForceAnimation(GObj*, AnimationHeader*);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802d237c,"func_802D237C_7AB90C","POKEMON_FUNC(func_802D237C_7AB90C)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_299{{
    {0x802ec9bc,"D_802EC9BC_7C5F4C","AnimationHeader"},
}};
constexpr std::array<ActorNativeService,0> services_299{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_299{{
    0x802d237c,
}};
constexpr std::array<std::uint32_t,0> graph_deps_299{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_299{{
}};
constexpr std::array<ActorDependencyEdge,6> calls_300{{
    {0x80019e14,"randRange","Source intrinsic/designator: randRange"},
    {0x8035edf8,"Pokemon_StartPathProc","void Pokemon_StartPathProc(GObj* obj, GObjFunc func);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035fd70,"Pokemon_RunCleanup","void Pokemon_RunCleanup(GObj* obj);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802d2428,"func_802D2428_7AB9B8","POKEMON_FUNC(func_802D2428_7AB9B8)"},
}};
constexpr std::array<ActorResourceDependency,1> resources_300{{
    {0x802d2980,"D_802D2980_7ABF10","GObj*"},
}};
constexpr std::array<ActorNativeService,5> services_300{{
    ActorNativeService::reference_assign,
    ActorNativeService::model,
    ActorNativeService::vector,
    ActorNativeService::vector_store,
    ActorNativeService::rng_range,
}};
constexpr std::array<std::uint32_t,1> callback_deps_300{{
    0x802d2428,
}};
constexpr std::array<std::uint32_t,0> graph_deps_300{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_300{{
}};
constexpr std::array<ActorDependencyEdge,6> calls_301{{
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x800a5e98,"fx_getPosVelDObj","void fx_getPosVelDObj(Vec3f* pos, Vec3f* vel, DObj* dobj);"},
    {0x800e41d8,"getGroundAt","s32 getGroundAt(f32 arg0, f32 arg1, GroundResult* arg2);"},
    {0x8000bca8,"ohWait","void ohWait(s32 numFrames);"},
    {0x8035e174,"func_8035E174_4FE584","void func_8035E174_4FE584(GObj* obj, Vec3f*);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_301{{
}};
constexpr std::array<ActorNativeService,4> services_301{{
    ActorNativeService::model,
    ActorNativeService::node_path,
    ActorNativeService::ground,
    ActorNativeService::node_world,
}};
constexpr std::array<std::uint32_t,0> callback_deps_301{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_301{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_301{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_302{{
    {0x80362ee0,"Pokemon_SpawnOnGround","GObj* Pokemon_SpawnOnGround(s32 gObjID, u16 id, struct WorldBlock* roomA, struct WorldBlock* roomB, ObjectSpawn* spawn, PokemonInitData* initData);"},
}};
constexpr std::array<ActorResourceDependency,2> resources_302{{
    {0x802eca30,"D_802ECA30_7C5FC0","PokemonInitData"},
    {0x0,"spawn","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,0> services_302{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_303{{
    {0x8035fe24,"Pokemon_AddAtGeo","GObj* Pokemon_AddAtGeo(GObj* obj, u16 PokemonID, PokemonDef* def);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_303{{
}};
constexpr std::array<ActorNativeService,1> services_303{{
    ActorNativeService::add_geo,
}};
constexpr std::array<std::uint32_t,0> callback_deps_303{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_303{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_303{{
}};
constexpr std::array<ActorDependencyEdge,2> calls_304{{
    {0x8035fe24,"Pokemon_AddAtGeo","GObj* Pokemon_AddAtGeo(GObj* obj, u16 PokemonID, PokemonDef* def);"},
    {0x80008f2c,"omEndProcess","void omEndProcess(GObjProcess* proc);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_304{{
}};
constexpr std::array<ActorNativeService,1> services_304{{
    ActorNativeService::add_geo,
}};
constexpr std::array<std::uint32_t,0> callback_deps_304{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_304{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_304{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_305{{
    {0x8035ecac,"Pokemon_SetStateRandom","void Pokemon_SetStateRandom(GObj* obj, RandomState* nextStates);"},
}};
constexpr std::array<ActorResourceDependency,0> resources_305{{
}};
constexpr std::array<ActorNativeService,1> services_305{{
    ActorNativeService::weighted,
}};
constexpr std::array<std::uint32_t,0> callback_deps_305{{
}};
constexpr std::array<std::uint32_t,0> graph_deps_305{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_305{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_306{{
    {0x80008c28,"omCreateProcess","GObjProcess* omCreateProcess(GObj* obj, void (*func)(struct GObj*), u8 kind, u32 pri);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035fd70,"Pokemon_RunCleanup","void Pokemon_RunCleanup(GObj* obj);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802d2580,"spawnStaryuAtGeo","POKEMON_FUNC(spawnStaryuAtGeo)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_306{{
}};
constexpr std::array<ActorNativeService,0> services_306{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_306{{
    0x802d2580,
}};
constexpr std::array<std::uint32_t,0> graph_deps_306{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_306{{
}};
constexpr std::array<ActorDependencyEdge,5> calls_307{{
    {0x80008c28,"omCreateProcess","GObjProcess* omCreateProcess(GObj* obj, void (*func)(struct GObj*), u8 kind, u32 pri);"},
    {0x8035fbf0,"Pokemon_WaitForFlag","void Pokemon_WaitForFlag(GObj* obj, s32 flags);"},
    {0x8035fd70,"Pokemon_RunCleanup","void Pokemon_RunCleanup(GObj* obj);"},
    {0x8035ec58,"Pokemon_SetState","void Pokemon_SetState(GObj* obj, GObjFunc state);"},
    {0x802d25b0,"spawnStarmieAtGeo","POKEMON_FUNC(spawnStarmieAtGeo)"},
}};
constexpr std::array<ActorResourceDependency,0> resources_307{{
}};
constexpr std::array<ActorNativeService,0> services_307{{
}};
constexpr std::array<std::uint32_t,1> callback_deps_307{{
    0x802d25b0,
}};
constexpr std::array<std::uint32_t,0> graph_deps_307{{
}};
constexpr std::array<std::uint32_t,0> weighted_deps_307{{
}};
constexpr std::array<ActorDependencyEdge,1> calls_308{{
    {0x80362e5c,"Pokemon_Spawn","GObj* Pokemon_Spawn(s32 gObjID, u16 id, struct WorldBlock* roomA, struct WorldBlock* roomB, ObjectSpawn* spawn, PokemonInitData* initData);"},
}};
constexpr std::array<ActorResourceDependency,1> resources_308{{
    {0x0,"spawn","Source-qualified resource"},
}};
constexpr std::array<ActorNativeService,0> services_308{{
}};
constexpr std::array<actor::BehaviorCallbackBinding,255> callbacks{{
    {0x802c5d7c,func_802C5D7C_79F30C,callback_deps_5,graph_deps_5,weighted_deps_5},
    {0x802c5e68,func_802C5E68_79F3F8,callback_deps_8,graph_deps_8,weighted_deps_8},
    {0x802c5f70,func_802C5F70_79F500,callback_deps_9,graph_deps_9,weighted_deps_9},
    {0x802c606c,func_802C606C_79F5FC,callback_deps_10,graph_deps_10,weighted_deps_10},
    {0x802c65e0,func_802C65E0_79FB70,callback_deps_16,graph_deps_16,weighted_deps_16},
    {0x802c673c,func_802C673C_79FCCC,callback_deps_20,graph_deps_20,weighted_deps_20},
    {0x802c67f4,func_802C67F4_79FD84,callback_deps_21,graph_deps_21,weighted_deps_21},
    {0x802c82c0,func_802C82C0_7A1850,callback_deps_24,graph_deps_24,weighted_deps_24},
    {0x802c8354,func_802C8354_7A18E4,callback_deps_25,graph_deps_25,weighted_deps_25},
    {0x802c840c,func_802C840C_7A199C,callback_deps_26,graph_deps_26,weighted_deps_26},
    {0x802c8458,func_802C8458_7A19E8,callback_deps_27,graph_deps_27,weighted_deps_27},
    {0x802c84d0,func_802C84D0_7A1A60,callback_deps_28,graph_deps_28,weighted_deps_28},
    {0x802c853c,func_802C853C_7A1ACC,callback_deps_29,graph_deps_29,weighted_deps_29},
    {0x802c8594,func_802C8594_7A1B24,callback_deps_30,graph_deps_30,weighted_deps_30},
    {0x802c8650,func_802C8650_7A1BE0,callback_deps_31,graph_deps_31,weighted_deps_31},
    {0x802c8704,func_802C8704_7A1C94,callback_deps_32,graph_deps_32,weighted_deps_32},
    {0x802c8764,func_802C8764_7A1CF4,callback_deps_33,graph_deps_33,weighted_deps_33},
    {0x802c8828,func_802C8828_7A1DB8,callback_deps_34,graph_deps_34,weighted_deps_34},
    {0x802c88c4,func_802C88C4_7A1E54,callback_deps_35,graph_deps_35,weighted_deps_35},
    {0x802c892c,func_802C892C_7A1EBC,callback_deps_36,graph_deps_36,weighted_deps_36},
    {0x802c8998,func_802C8998_7A1F28,callback_deps_37,graph_deps_37,weighted_deps_37},
    {0x802c8b38,func_802C8B38_7A20C8,callback_deps_40,graph_deps_40,weighted_deps_40},
    {0x802c8c08,func_802C8C08_7A2198,callback_deps_41,graph_deps_41,weighted_deps_41},
    {0x802c8c70,func_802C8C70_7A2200,callback_deps_42,graph_deps_42,weighted_deps_42},
    {0x802c8d30,func_802C8D30_7A22C0,callback_deps_43,graph_deps_43,weighted_deps_43},
    {0x802c8e54,func_802C8E54_7A23E4,callback_deps_44,graph_deps_44,weighted_deps_44},
    {0x802c8ea8,func_802C8EA8_7A2438,callback_deps_45,graph_deps_45,weighted_deps_45},
    {0x802c8f50,func_802C8F50_7A24E0,callback_deps_46,graph_deps_46,weighted_deps_46},
    {0x802c903c,func_802C903C_7A25CC,callback_deps_47,graph_deps_47,weighted_deps_47},
    {0x802c90e4,func_802C90E4_7A2674,callback_deps_48,graph_deps_48,weighted_deps_48},
    {0x802c91ac,func_802C91AC_7A273C,callback_deps_49,graph_deps_49,weighted_deps_49},
    {0x802c931c,func_802C931C_7A28AC,callback_deps_50,graph_deps_50,weighted_deps_50},
    {0x802c9408,func_802C9408_7A2998,callback_deps_51,graph_deps_51,weighted_deps_51},
    {0x802c94a4,func_802C94A4_7A2A34,callback_deps_52,graph_deps_52,weighted_deps_52},
    {0x802c95c0,func_802C95C0_7A2B50,callback_deps_54,graph_deps_54,weighted_deps_54},
    {0x802c95e4,func_802C95E4_7A2B74,callback_deps_55,graph_deps_55,weighted_deps_55},
    {0x802c9634,func_802C9634_7A2BC4,callback_deps_56,graph_deps_56,weighted_deps_56},
    {0x802c96a0,func_802C96A0_7A2C30,callback_deps_57,graph_deps_57,weighted_deps_57},
    {0x802c96fc,func_802C96FC_7A2C8C,callback_deps_58,graph_deps_58,weighted_deps_58},
    {0x802c9750,func_802C9750_7A2CE0,callback_deps_59,graph_deps_59,weighted_deps_59},
    {0x802c97a4,func_802C97A4_7A2D34,callback_deps_60,graph_deps_60,weighted_deps_60},
    {0x802c9840,func_802C9840_7A2DD0,callback_deps_62,graph_deps_62,weighted_deps_62},
    {0x802c9958,func_802C9958_7A2EE8,callback_deps_63,graph_deps_63,weighted_deps_63},
    {0x802c9990,func_802C9990_7A2F20,callback_deps_64,graph_deps_64,weighted_deps_64},
    {0x802c9a48,func_802C9A48_7A2FD8,callback_deps_65,graph_deps_65,weighted_deps_65},
    {0x802c9a94,func_802C9A94_7A3024,callback_deps_66,graph_deps_66,weighted_deps_66},
    {0x802c9b0c,func_802C9B0C_7A309C,callback_deps_67,graph_deps_67,weighted_deps_67},
    {0x802c9b78,func_802C9B78_7A3108,callback_deps_68,graph_deps_68,weighted_deps_68},
    {0x802c9bd0,func_802C9BD0_7A3160,callback_deps_69,graph_deps_69,weighted_deps_69},
    {0x802c9c84,func_802C9C84_7A3214,callback_deps_70,graph_deps_70,weighted_deps_70},
    {0x802c9d00,func_802C9D00_7A3290,callback_deps_71,graph_deps_71,weighted_deps_71},
    {0x802c9dc4,func_802C9DC4_7A3354,callback_deps_72,graph_deps_72,weighted_deps_72},
    {0x802c9e60,func_802C9E60_7A33F0,callback_deps_73,graph_deps_73,weighted_deps_73},
    {0x802c9ec8,func_802C9EC8_7A3458,callback_deps_74,graph_deps_74,weighted_deps_74},
    {0x802c9f34,func_802C9F34_7A34C4,callback_deps_75,graph_deps_75,weighted_deps_75},
    {0x802c9fd0,func_802C9FD0_7A3560,callback_deps_77,graph_deps_77,weighted_deps_77},
    {0x802ca020,func_802CA020_7A35B0,callback_deps_78,graph_deps_78,weighted_deps_78},
    {0x802ca1b0,func_802CA1B0_7A3740,callback_deps_79,graph_deps_79,weighted_deps_79},
    {0x802ca270,func_802CA270_7A3800,callback_deps_82,graph_deps_82,weighted_deps_82},
    {0x802ca2bc,func_802CA2BC_7A384C,callback_deps_83,graph_deps_83,weighted_deps_83},
    {0x802ca2f4,func_802CA2F4_7A3884,callback_deps_84,graph_deps_84,weighted_deps_84},
    {0x802ca32c,func_802CA32C_7A38BC,callback_deps_85,graph_deps_85,weighted_deps_85},
    {0x802ca364,func_802CA364_7A38F4,callback_deps_86,graph_deps_86,weighted_deps_86},
    {0x802ca434,func_802CA434_7A39C4,callback_deps_87,graph_deps_87,weighted_deps_87},
    {0x802ca574,func_802CA574_7A3B04,callback_deps_88,graph_deps_88,weighted_deps_88},
    {0x802ca5b4,func_802CA5B4_7A3B44,callback_deps_89,graph_deps_89,weighted_deps_89},
    {0x802ca65c,func_802CA65C_7A3BEC,callback_deps_90,graph_deps_90,weighted_deps_90},
    {0x802ca6c0,func_802CA6C0_7A3C50,callback_deps_92,graph_deps_92,weighted_deps_92},
    {0x802ca768,func_802CA768_7A3CF8,callback_deps_93,graph_deps_93,weighted_deps_93},
    {0x802ca7a0,func_802CA7A0_7A3D30,callback_deps_94,graph_deps_94,weighted_deps_94},
    {0x802ca858,func_802CA858_7A3DE8,callback_deps_95,graph_deps_95,weighted_deps_95},
    {0x802ca8a4,func_802CA8A4_7A3E34,callback_deps_96,graph_deps_96,weighted_deps_96},
    {0x802ca8f8,func_802CA8F8_7A3E88,callback_deps_97,graph_deps_97,weighted_deps_97},
    {0x802ca948,func_802CA948_7A3ED8,callback_deps_98,graph_deps_98,weighted_deps_98},
    {0x802ca9bc,func_802CA9BC_7A3F4C,callback_deps_99,graph_deps_99,weighted_deps_99},
    {0x802caa2c,func_802CAA2C_7A3FBC,callback_deps_100,graph_deps_100,weighted_deps_100},
    {0x802caad0,func_802CAAD0_7A4060,callback_deps_101,graph_deps_101,weighted_deps_101},
    {0x802cab2c,func_802CAB2C_7A40BC,callback_deps_102,graph_deps_102,weighted_deps_102},
    {0x802cabd0,func_802CABD0_7A4160,callback_deps_103,graph_deps_103,weighted_deps_103},
    {0x802cac2c,func_802CAC2C_7A41BC,callback_deps_104,graph_deps_104,weighted_deps_104},
    {0x802cace4,func_802CACE4_7A4274,callback_deps_105,graph_deps_105,weighted_deps_105},
    {0x802cad54,func_802CAD54_7A42E4,callback_deps_106,graph_deps_106,weighted_deps_106},
    {0x802cadec,func_802CADEC_7A437C,callback_deps_107,graph_deps_107,weighted_deps_107},
    {0x802cae70,func_802CAE70_7A4400,callback_deps_108,graph_deps_108,weighted_deps_108},
    {0x802caf04,func_802CAF04_7A4494,callback_deps_109,graph_deps_109,weighted_deps_109},
    {0x802caf54,func_802CAF54_7A44E4,callback_deps_110,graph_deps_110,weighted_deps_110},
    {0x802cb008,func_802CB008_7A4598,callback_deps_111,graph_deps_111,weighted_deps_111},
    {0x802cb080,func_802CB080_7A4610,callback_deps_113,graph_deps_113,weighted_deps_113},
    {0x802cb110,func_802CB110_7A46A0,callback_deps_114,graph_deps_114,weighted_deps_114},
    {0x802cb194,func_802CB194_7A4724,callback_deps_115,graph_deps_115,weighted_deps_115},
    {0x802cb238,func_802CB238_7A47C8,callback_deps_116,graph_deps_116,weighted_deps_116},
    {0x802cb294,func_802CB294_7A4824,callback_deps_117,graph_deps_117,weighted_deps_117},
    {0x802cb338,func_802CB338_7A48C8,callback_deps_118,graph_deps_118,weighted_deps_118},
    {0x802cb394,func_802CB394_7A4924,callback_deps_119,graph_deps_119,weighted_deps_119},
    {0x802cb404,func_802CB404_7A4994,callback_deps_120,graph_deps_120,weighted_deps_120},
    {0x802cb474,func_802CB474_7A4A04,callback_deps_121,graph_deps_121,weighted_deps_121},
    {0x802cb550,func_802CB550_7A4AE0,callback_deps_122,graph_deps_122,weighted_deps_122},
    {0x802cb5ac,func_802CB5AC_7A4B3C,callback_deps_123,graph_deps_123,weighted_deps_123},
    {0x802cb614,func_802CB614_7A4BA4,callback_deps_124,graph_deps_124,weighted_deps_124},
    {0x802cb6f8,func_802CB6F8_7A4C88,callback_deps_125,graph_deps_125,weighted_deps_125},
    {0x802cb79c,func_802CB79C_7A4D2C,callback_deps_126,graph_deps_126,weighted_deps_126},
    {0x802cb7f4,func_802CB7F4_7A4D84,callback_deps_127,graph_deps_127,weighted_deps_127},
    {0x802cb818,func_802CB818_7A4DA8,callback_deps_128,graph_deps_128,weighted_deps_128},
    {0x802cb88c,func_802CB88C_7A4E1C,callback_deps_129,graph_deps_129,weighted_deps_129},
    {0x802cb8d8,func_802CB8D8_7A4E68,callback_deps_130,graph_deps_130,weighted_deps_130},
    {0x802cb92c,func_802CB92C_7A4EBC,callback_deps_131,graph_deps_131,weighted_deps_131},
    {0x802cb9bc,func_802CB9BC_7A4F4C,callback_deps_132,graph_deps_132,weighted_deps_132},
    {0x802cba24,func_802CBA24_7A4FB4,callback_deps_133,graph_deps_133,weighted_deps_133},
    {0x802cba90,func_802CBA90_7A5020,callback_deps_134,graph_deps_134,weighted_deps_134},
    {0x802cbbdc,func_802CBBDC_7A516C,callback_deps_135,graph_deps_135,weighted_deps_135},
    {0x802cbc74,func_802CBC74_7A5204,callback_deps_136,graph_deps_136,weighted_deps_136},
    {0x802cbcdc,func_802CBCDC_7A526C,callback_deps_137,graph_deps_137,weighted_deps_137},
    {0x802cbd74,func_802CBD74_7A5304,callback_deps_138,graph_deps_138,weighted_deps_138},
    {0x802cbe14,func_802CBE14_7A53A4,callback_deps_139,graph_deps_139,weighted_deps_139},
    {0x802cbe9c,func_802CBE9C_7A542C,callback_deps_140,graph_deps_140,weighted_deps_140},
    {0x802cbf18,func_802CBF18_7A54A8,callback_deps_141,graph_deps_141,weighted_deps_141},
    {0x802cbf64,func_802CBF64_7A54F4,callback_deps_142,graph_deps_142,weighted_deps_142},
    {0x802cbfdc,func_802CBFDC_7A556C,callback_deps_143,graph_deps_143,weighted_deps_143},
    {0x802cc068,func_802CC068_7A55F8,callback_deps_144,graph_deps_144,weighted_deps_144},
    {0x802cc08c,func_802CC08C_7A561C,callback_deps_145,graph_deps_145,weighted_deps_145},
    {0x802cc104,func_802CC104_7A5694,callback_deps_146,graph_deps_146,weighted_deps_146},
    {0x802cc1e0,func_802CC1E0_7A5770,callback_deps_147,graph_deps_147,weighted_deps_147},
    {0x802cc250,func_802CC250_7A57E0,callback_deps_148,graph_deps_148,weighted_deps_148},
    {0x802cc2b4,func_802CC2B4_7A5844,callback_deps_149,graph_deps_149,weighted_deps_149},
    {0x802cc320,func_802CC320_7A58B0,callback_deps_150,graph_deps_150,weighted_deps_150},
    {0x802cc3b4,func_802CC3B4_7A5944,callback_deps_151,graph_deps_151,weighted_deps_151},
    {0x802cc424,func_802CC424_7A59B4,callback_deps_152,graph_deps_152,weighted_deps_152},
    {0x802cc4dc,func_802CC4DC_7A5A6C,callback_deps_153,graph_deps_153,weighted_deps_153},
    {0x802cc514,func_802CC514_7A5AA4,callback_deps_154,graph_deps_154,weighted_deps_154},
    {0x802cc5c0,func_802CC5C0_7A5B50,callback_deps_155,graph_deps_155,weighted_deps_155},
    {0x802cca70,func_802CCA70_7A6000,callback_deps_159,graph_deps_159,weighted_deps_159},
    {0x802ccab4,func_802CCAB4_7A6044,callback_deps_160,graph_deps_160,weighted_deps_160},
    {0x802ccb34,func_802CCB34_7A60C4,callback_deps_161,graph_deps_161,weighted_deps_161},
    {0x802ccbf8,func_802CCBF8_7A6188,callback_deps_162,graph_deps_162,weighted_deps_162},
    {0x802ccc90,func_802CCC90_7A6220,callback_deps_163,graph_deps_163,weighted_deps_163},
    {0x802cccfc,func_802CCCFC_7A628C,callback_deps_164,graph_deps_164,weighted_deps_164},
    {0x802ccd80,func_802CCD80_7A6310,callback_deps_165,graph_deps_165,weighted_deps_165},
    {0x802ccddc,func_802CCDDC_7A636C,callback_deps_166,graph_deps_166,weighted_deps_166},
    {0x802cce70,func_802CCE70_7A6400,callback_deps_167,graph_deps_167,weighted_deps_167},
    {0x802cd0b8,func_802CD0B8_7A6648,callback_deps_168,graph_deps_168,weighted_deps_168},
    {0x802cd118,func_802CD118_7A66A8,callback_deps_169,graph_deps_169,weighted_deps_169},
    {0x802cd1ac,func_802CD1AC_7A673C,callback_deps_170,graph_deps_170,weighted_deps_170},
    {0x802cd2a8,func_802CD2A8_7A6838,callback_deps_171,graph_deps_171,weighted_deps_171},
    {0x802cd2fc,func_802CD2FC_7A688C,callback_deps_172,graph_deps_172,weighted_deps_172},
    {0x802cd4f4,func_802CD4F4_7A6A84,callback_deps_173,graph_deps_173,weighted_deps_173},
    {0x802cd5d8,func_802CD5D8_7A6B68,callback_deps_174,graph_deps_174,weighted_deps_174},
    {0x802cd774,func_802CD774_7A6D04,callback_deps_177,graph_deps_177,weighted_deps_177},
    {0x802cd7c0,func_802CD7C0_7A6D50,callback_deps_178,graph_deps_178,weighted_deps_178},
    {0x802cd84c,func_802CD84C_7A6DDC,callback_deps_179,graph_deps_179,weighted_deps_179},
    {0x802cd8a4,func_802CD8A4_7A6E34,callback_deps_180,graph_deps_180,weighted_deps_180},
    {0x802cd8dc,func_802CD8DC_7A6E6C,callback_deps_181,graph_deps_181,weighted_deps_181},
    {0x802cd914,func_802CD914_7A6EA4,callback_deps_182,graph_deps_182,weighted_deps_182},
    {0x802cd98c,func_802CD98C_7A6F1C,callback_deps_183,graph_deps_183,weighted_deps_183},
    {0x802cd9f8,func_802CD9F8_7A6F88,callback_deps_184,graph_deps_184,weighted_deps_184},
    {0x802cda94,func_802CDA94_7A7024,callback_deps_185,graph_deps_185,weighted_deps_185},
    {0x802cdb40,func_802CDB40_7A70D0,callback_deps_186,graph_deps_186,weighted_deps_186},
    {0x802cdbd4,func_802CDBD4_7A7164,callback_deps_187,graph_deps_187,weighted_deps_187},
    {0x802cdc68,func_802CDC68_7A71F8,callback_deps_188,graph_deps_188,weighted_deps_188},
    {0x802cdcfc,func_802CDCFC_7A728C,callback_deps_189,graph_deps_189,weighted_deps_189},
    {0x802cde80,func_802CDE80_7A7410,callback_deps_192,graph_deps_192,weighted_deps_192},
    {0x802cdea4,func_802CDEA4_7A7434,callback_deps_193,graph_deps_193,weighted_deps_193},
    {0x802cdefc,func_802CDEFC_7A748C,callback_deps_194,graph_deps_194,weighted_deps_194},
    {0x802cdf68,func_802CDF68_7A74F8,callback_deps_195,graph_deps_195,weighted_deps_195},
    {0x802cdffc,func_802CDFFC_7A758C,callback_deps_196,graph_deps_196,weighted_deps_196},
    {0x802ce090,func_802CE090_7A7620,callback_deps_197,graph_deps_197,weighted_deps_197},
    {0x802ce124,func_802CE124_7A76B4,callback_deps_198,graph_deps_198,weighted_deps_198},
    {0x802ce200,func_802CE200_7A7790,callback_deps_200,graph_deps_200,weighted_deps_200},
    {0x802ce224,func_802CE224_7A77B4,callback_deps_201,graph_deps_201,weighted_deps_201},
    {0x802ce2dc,func_802CE2DC_7A786C,callback_deps_202,graph_deps_202,weighted_deps_202},
    {0x802ce328,func_802CE328_7A78B8,callback_deps_203,graph_deps_203,weighted_deps_203},
    {0x802ce37c,func_802CE37C_7A790C,callback_deps_204,graph_deps_204,weighted_deps_204},
    {0x802ce3cc,func_802CE3CC_7A795C,callback_deps_205,graph_deps_205,weighted_deps_205},
    {0x802ce440,func_802CE440_7A79D0,callback_deps_206,graph_deps_206,weighted_deps_206},
    {0x802ce4b0,func_802CE4B0_7A7A40,callback_deps_207,graph_deps_207,weighted_deps_207},
    {0x802ce554,func_802CE554_7A7AE4,callback_deps_208,graph_deps_208,weighted_deps_208},
    {0x802ce5b0,func_802CE5B0_7A7B40,callback_deps_209,graph_deps_209,weighted_deps_209},
    {0x802ce654,func_802CE654_7A7BE4,callback_deps_210,graph_deps_210,weighted_deps_210},
    {0x802ce6b0,func_802CE6B0_7A7C40,callback_deps_211,graph_deps_211,weighted_deps_211},
    {0x802ce768,func_802CE768_7A7CF8,callback_deps_212,graph_deps_212,weighted_deps_212},
    {0x802ce7d8,func_802CE7D8_7A7D68,callback_deps_213,graph_deps_213,weighted_deps_213},
    {0x802ce870,func_802CE870_7A7E00,callback_deps_214,graph_deps_214,weighted_deps_214},
    {0x802ce8f4,func_802CE8F4_7A7E84,callback_deps_215,graph_deps_215,weighted_deps_215},
    {0x802ce978,func_802CE978_7A7F08,callback_deps_216,graph_deps_216,weighted_deps_216},
    {0x802cea34,func_802CEA34_7A7FC4,callback_deps_217,graph_deps_217,weighted_deps_217},
    {0x802ceaa0,func_802CEAA0_7A8030,callback_deps_219,graph_deps_219,weighted_deps_219},
    {0x802ceb4c,func_802CEB4C_7A80DC,callback_deps_220,graph_deps_220,weighted_deps_220},
    {0x802ceb9c,func_802CEB9C_7A812C,callback_deps_221,graph_deps_221,weighted_deps_221},
    {0x802ced44,func_802CED44_7A82D4,callback_deps_223,graph_deps_223,weighted_deps_223},
    {0x802cefb8,func_802CEFB8_7A8548,callback_deps_224,graph_deps_224,weighted_deps_224},
    {0x802cf07c,func_802CF07C_7A860C,callback_deps_225,graph_deps_225,weighted_deps_225},
    {0x802cf114,func_802CF114_7A86A4,callback_deps_226,graph_deps_226,weighted_deps_226},
    {0x802cf388,func_802CF388_7A8918,callback_deps_227,graph_deps_227,weighted_deps_227},
    {0x802cf3f8,func_802CF3F8_7A8988,callback_deps_228,graph_deps_228,weighted_deps_228},
    {0x802cf48c,func_802CF48C_7A8A1C,callback_deps_229,graph_deps_229,weighted_deps_229},
    {0x802cf500,func_802CF500_7A8A90,callback_deps_230,graph_deps_230,weighted_deps_230},
    {0x802cf524,func_802CF524_7A8AB4,callback_deps_231,graph_deps_231,weighted_deps_231},
    {0x802cf594,func_802CF594_7A8B24,callback_deps_232,graph_deps_232,weighted_deps_232},
    {0x802cf638,func_802CF638_7A8BC8,callback_deps_233,graph_deps_233,weighted_deps_233},
    {0x802cf65c,func_802CF65C_7A8BEC,callback_deps_234,graph_deps_234,weighted_deps_234},
    {0x802cf6cc,func_802CF6CC_7A8C5C,callback_deps_235,graph_deps_235,weighted_deps_235},
    {0x802cf720,func_802CF720_7A8CB0,callback_deps_236,graph_deps_236,weighted_deps_236},
    {0x802cf794,func_802CF794_7A8D24,callback_deps_237,graph_deps_237,weighted_deps_237},
    {0x802cf840,func_802CF840_7A8DD0,callback_deps_239,graph_deps_239,weighted_deps_239},
    {0x802cf864,func_802CF864_7A8DF4,callback_deps_240,graph_deps_240,weighted_deps_240},
    {0x802cf8bc,func_802CF8BC_7A8E4C,callback_deps_241,graph_deps_241,weighted_deps_241},
    {0x802cfa40,func_802CFA40_7A8FD0,callback_deps_243,graph_deps_243,weighted_deps_243},
    {0x802cfc6c,func_802CFC6C_7A91FC,callback_deps_244,graph_deps_244,weighted_deps_244},
    {0x802d0320,func_802D0320_7A98B0,callback_deps_247,graph_deps_247,weighted_deps_247},
    {0x802d0344,func_802D0344_7A98D4,callback_deps_248,graph_deps_248,weighted_deps_248},
    {0x802d04cc,func_802D04CC_7A9A5C,callback_deps_253,graph_deps_253,weighted_deps_253},
    {0x802d07bc,func_802D07BC_7A9D4C,callback_deps_256,graph_deps_256,weighted_deps_256},
    {0x802d0b80,func_802D0B80_7AA110,callback_deps_258,graph_deps_258,weighted_deps_258},
    {0x802d0ba4,func_802D0BA4_7AA134,callback_deps_259,graph_deps_259,weighted_deps_259},
    {0x802d0bfc,func_802D0BFC_7AA18C,callback_deps_260,graph_deps_260,weighted_deps_260},
    {0x802d0d0c,func_802D0D0C_7AA29C,callback_deps_262,graph_deps_262,weighted_deps_262},
    {0x802d0f0c,func_802D0F0C_7AA49C,callback_deps_263,graph_deps_263,weighted_deps_263},
    {0x802d0f98,func_802D0F98_7AA528,callback_deps_264,graph_deps_264,weighted_deps_264},
    {0x802d1190,func_802D1190,callback_deps_266,graph_deps_266,weighted_deps_266},
    {0x802d1200,func_802D1200_7AA790,callback_deps_268,graph_deps_268,weighted_deps_268},
    {0x802d129c,func_802D129C_7AA82C,callback_deps_269,graph_deps_269,weighted_deps_269},
    {0x802d12d4,func_802D12D4_7AA864,callback_deps_270,graph_deps_270,weighted_deps_270},
    {0x802d130c,func_802D130C_7AA89C,callback_deps_271,graph_deps_271,weighted_deps_271},
    {0x802d1344,func_802D1344_7AA8D4,callback_deps_272,graph_deps_272,weighted_deps_272},
    {0x802d1414,func_802D1414_7AA9A4,callback_deps_273,graph_deps_273,weighted_deps_273},
    {0x802d1454,func_802D1454_7AA9E4,callback_deps_274,graph_deps_274,weighted_deps_274},
    {0x802d156c,func_802D156C_7AAAFC,callback_deps_275,graph_deps_275,weighted_deps_275},
    {0x802d1598,func_802D1598_7AAB28,callback_deps_276,graph_deps_276,weighted_deps_276},
    {0x802d15bc,func_802D15BC_7AAB4C,callback_deps_277,graph_deps_277,weighted_deps_277},
    {0x802d16cc,func_802D16CC_7AAC5C,callback_deps_278,graph_deps_278,weighted_deps_278},
    {0x802d1734,func_802D1734_7AACC4,callback_deps_279,graph_deps_279,weighted_deps_279},
    {0x802d17d8,func_802D17D8_7AAD68,callback_deps_280,graph_deps_280,weighted_deps_280},
    {0x802d18d8,func_802D18D8_7AAE68,callback_deps_281,graph_deps_281,weighted_deps_281},
    {0x802d19dc,func_802D19DC_7AAF6C,callback_deps_282,graph_deps_282,weighted_deps_282},
    {0x802d1acc,func_802D1ACC_7AB05C,callback_deps_283,graph_deps_283,weighted_deps_283},
    {0x802d1b48,func_802D1B48_7AB0D8,callback_deps_284,graph_deps_284,weighted_deps_284},
    {0x802d1bc8,func_802D1BC8_7AB158,callback_deps_285,graph_deps_285,weighted_deps_285},
    {0x802d1c40,func_802D1C40_7AB1D0,callback_deps_286,graph_deps_286,weighted_deps_286},
    {0x802d1d4c,func_802D1D4C_7AB2DC,callback_deps_287,graph_deps_287,weighted_deps_287},
    {0x802d1e3c,func_802D1E3C_7AB3CC,callback_deps_288,graph_deps_288,weighted_deps_288},
    {0x802d1eb8,func_802D1EB8_7AB448,callback_deps_289,graph_deps_289,weighted_deps_289},
    {0x802d1f18,func_802D1F18_7AB4A8,callback_deps_291,graph_deps_291,weighted_deps_291},
    {0x802d1fc0,func_802D1FC0_7AB550,callback_deps_292,graph_deps_292,weighted_deps_292},
    {0x802d2128,func_802D2128_7AB6B8,callback_deps_293,graph_deps_293,weighted_deps_293},
    {0x802d2230,func_802D2230_7AB7C0,callback_deps_295,graph_deps_295,weighted_deps_295},
    {0x802d22a0,func_802D22A0_7AB830,callback_deps_296,graph_deps_296,weighted_deps_296},
    {0x802d22d4,func_802D22D4_7AB864,callback_deps_297,graph_deps_297,weighted_deps_297},
    {0x802d230c,func_802D230C_7AB89C,callback_deps_298,graph_deps_298,weighted_deps_298},
    {0x802d2344,func_802D2344_7AB8D4,callback_deps_299,graph_deps_299,weighted_deps_299},
    {0x802d237c,func_802D237C_7AB90C,callback_deps_300,graph_deps_300,weighted_deps_300},
    {0x802d2428,func_802D2428_7AB9B8,callback_deps_301,graph_deps_301,weighted_deps_301},
    {0x802d2580,spawnStaryuAtGeo,callback_deps_303,graph_deps_303,weighted_deps_303},
    {0x802d25b0,spawnStarmieAtGeo,callback_deps_304,graph_deps_304,weighted_deps_304},
    {0x802d25e0,func_802D25E0,callback_deps_305,graph_deps_305,weighted_deps_305},
    {0x802d2604,func_802D2604,callback_deps_306,graph_deps_306,weighted_deps_306},
    {0x802d2684,func_802D2684,callback_deps_307,graph_deps_307,weighted_deps_307},
}};
constexpr std::array<ActorProviderDependency,309> dependencies{{
    {0x802c5c20,"src/valley/79F1B0.c:20-22","void func_802C5C20_79F1B0(WorldBlock* arg0, WorldBlock* arg1)","void func_802C5C20_79F1B0(IsleValleyLifecycleSource* s, IsleValleyWorldBlock* arg0, IsleValleyWorldBlock* arg1)",SourceAbi::utility,SourceDomain::lifecycle,calls_0,resources_0,services_0},
    {0x802c5c44,"src/valley/79F1B0.c:24-26","void func_802C5C44_79F1D4(WorldBlock* arg0, WorldBlock* arg1)","void func_802C5C44_79F1D4(IsleValleyLifecycleSource* s, IsleValleyWorldBlock* arg0, IsleValleyWorldBlock* arg1)",SourceAbi::utility,SourceDomain::lifecycle,calls_1,resources_1,services_1},
    {0x802c5c68,"src/valley/79F1B0.c:28-30","void func_802C5C68_79F1F8(WorldBlock* arg0)","void func_802C5C68_79F1F8(IsleValleyLifecycleSource* s, IsleValleyWorldBlock* arg0)",SourceAbi::utility,SourceDomain::lifecycle,calls_2,resources_2,services_2},
    {0x802c5c8c,"src/valley/79F1B0.c:32-37","void func_802C5C8C_79F21C(void)","void func_802C5C8C_79F21C(IsleValleyLifecycleSource* s)",SourceAbi::utility,SourceDomain::lifecycle,calls_3,resources_3,services_3},
    {0x802c5d18,"src/valley/79F1B0.c:39-50","void func_802C5D18_79F2A8(s32 arg0)","void func_802C5D18_79F2A8(IsleValleyLifecycleSource* s, int32_t arg0)",SourceAbi::utility,SourceDomain::lifecycle,calls_4,resources_4,services_4},
    {0x802c5d7c,"src/valley/79F1B0.c:52-57","void func_802C5D7C_79F30C(GObj* arg0)","void func_802C5D7C_79F30C(IsleGameObject* arg0)",SourceAbi::gobj_callback,SourceDomain::actor,calls_5,resources_5,services_5},
    {0x802c5dcc,"src/valley/79F1B0.c:59-60","void func_802C5DCC_79F35C(s32 arg0)","void func_802C5DCC_79F35C(IsleValleyLifecycleSource* s, int32_t arg0)",SourceAbi::utility,SourceDomain::lifecycle,calls_6,resources_6,services_6},
    {0x802c5dd4,"src/valley/79F1B0.c:62-76","void func_802C5DD4_79F364(WorldBlock* arg0)","void func_802C5DD4_79F364(IsleValleyLifecycleSource* s, IsleValleyWorldBlock* arg0)",SourceAbi::utility,SourceDomain::lifecycle,calls_7,resources_7,services_7},
    {0x802c5e68,"src/valley/79F1B0.c:78-112","void func_802C5E68_79F3F8(GObj* obj)","void func_802C5E68_79F3F8(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_8,resources_8,services_8},
    {0x802c5f70,"src/valley/79F1B0.c:114-146","void func_802C5F70_79F500(GObj* obj)","void func_802C5F70_79F500(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_9,resources_9,services_9},
    {0x802c606c,"src/valley/79F1B0.c:148-180","void func_802C606C_79F5FC(GObj* obj)","void func_802C606C_79F5FC(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_10,resources_10,services_10},
    {0x802c6168,"src/valley/79F1B0.c:182-218","void func_802C6168_79F6F8(GObj* obj, GroundResult* ground)","void func_802C6168_79F6F8(IsleGameObject* obj, IsleActorGround* ground)",SourceAbi::item_collision,SourceDomain::actor,calls_11,resources_11,services_11},
    {0x802c6300,"src/valley/79F1B0.c:220-237","void func_802C6300_79F890(void)","void func_802C6300_79F890(IsleValleyLifecycleSource* s)",SourceAbi::utility,SourceDomain::lifecycle,calls_12,resources_12,services_12},
    {0x802c63ec,"src/valley/79F1B0.c:239-262","void func_802C63EC_79F97C(void)","void func_802C63EC_79F97C(IsleValleyLifecycleSource* s)",SourceAbi::utility,SourceDomain::lifecycle,calls_13,resources_13,services_13},
    {0x802c653c,"src/valley/79F1B0.c:264-265","void func_802C653C_79FACC(s32 arg0)","void func_802C653C_79FACC(IsleValleyLifecycleSource* s, int32_t arg0)",SourceAbi::utility,SourceDomain::lifecycle,calls_14,resources_14,services_14},
    {0x802c6544,"src/valley/79F1B0.c:267-281","s32 func_802C6544_79FAD4(s32 arg0)","int32_t func_802C6544_79FAD4(IsleValleyLifecycleSource* s, int32_t arg0)",SourceAbi::utility,SourceDomain::lifecycle,calls_15,resources_15,services_15},
    {0x802c65e0,"src/valley/79FB70.c:15-19","void func_802C65E0_79FB70(GObj* arg0)","void func_802C65E0_79FB70(IsleGameObject* arg0)",SourceAbi::gobj_callback,SourceDomain::lifecycle,calls_16,resources_16,services_16},
    {0x802c6610,"src/valley/79FB70.c:21-23","void func_802C6610_79FBA0(void)","void func_802C6610_79FBA0(IsleValleyLifecycleSource* s)",SourceAbi::utility,SourceDomain::lifecycle,calls_17,resources_17,services_17},
    {0x802c6630,"src/valley/79FB70.c:25-40","void func_802C6630_79FBC0(DObj* dobj, s32 arg1, f32 arg2)","void func_802C6630_79FBC0(IsleValleyLifecycleSource* s, const IsleActorNodeRef* dobj, int32_t arg1, float arg2)",SourceAbi::lifecycle_animation,SourceDomain::lifecycle,calls_18,resources_18,services_18},
    {0x802c66a4,"src/valley/79FB70.c:42-53","void valley_stopIntro(void)","void valley_stopIntro(IsleValleyLifecycleSource* s)",SourceAbi::utility,SourceDomain::lifecycle,calls_19,resources_19,services_19},
    {0x802c673c,"src/valley/79FB70.c:55-68","void func_802C673C_79FCCC(GObj* arg0)","void func_802C673C_79FCCC(IsleGameObject* arg0)",SourceAbi::gobj_callback,SourceDomain::lifecycle,calls_20,resources_20,services_20},
    {0x802c67f4,"src/valley/79FB70.c:70-127","void func_802C67F4_79FD84(GObj* obj)","void func_802C67F4_79FD84(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::lifecycle,calls_21,resources_21,services_21},
    {0x802c6ad4,"src/valley/79FB70.c:129-138","void func_802C6AD4_7A0064(void)","void func_802C6AD4_7A0064(IsleValleyLifecycleSource* s)",SourceAbi::utility,SourceDomain::lifecycle,calls_22,resources_22,services_22},
    {0x802c6b30,"src/valley/7A00C0.c:10-204","void func_802C6B30_7A00C0(s32 arg0, f32 arg1)","void func_802C6B30_7A00C0(IsleValleyLifecycleSource* s, int32_t arg0, float arg1)",SourceAbi::utility,SourceDomain::lifecycle,calls_23,resources_23,services_23},
    {0x802c82c0,"src/valley/7A1850.c:54-68","POKEMON_FUNC(func_802C82C0_7A1850)","void func_802C82C0_7A1850(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_24,resources_24,services_24},
    {0x802c8354,"src/valley/7A1850.c:70-78","POKEMON_FUNC(func_802C8354_7A18E4)","void func_802C8354_7A18E4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_25,resources_25,services_25},
    {0x802c840c,"src/valley/7A1850.c:80-85","POKEMON_FUNC(func_802C840C_7A199C)","void func_802C840C_7A199C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_26,resources_26,services_26},
    {0x802c8458,"src/valley/7A1850.c:87-94","POKEMON_FUNC(func_802C8458_7A19E8)","void func_802C8458_7A19E8(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_27,resources_27,services_27},
    {0x802c84d0,"src/valley/7A1850.c:96-102","POKEMON_FUNC(func_802C84D0_7A1A60)","void func_802C84D0_7A1A60(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_28,resources_28,services_28},
    {0x802c853c,"src/valley/7A1850.c:104-109","POKEMON_FUNC(func_802C853C_7A1ACC)","void func_802C853C_7A1ACC(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_29,resources_29,services_29},
    {0x802c8594,"src/valley/7A1850.c:111-122","POKEMON_FUNC(func_802C8594_7A1B24)","void func_802C8594_7A1B24(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_30,resources_30,services_30},
    {0x802c8650,"src/valley/7A1850.c:124-136","POKEMON_FUNC(func_802C8650_7A1BE0)","void func_802C8650_7A1BE0(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_31,resources_31,services_31},
    {0x802c8704,"src/valley/7A1850.c:138-144","POKEMON_FUNC(func_802C8704_7A1C94)","void func_802C8704_7A1C94(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_32,resources_32,services_32},
    {0x802c8764,"src/valley/7A1850.c:146-155","POKEMON_FUNC(func_802C8764_7A1CF4)","void func_802C8764_7A1CF4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_33,resources_33,services_33},
    {0x802c8828,"src/valley/7A1850.c:157-167","POKEMON_FUNC(func_802C8828_7A1DB8)","void func_802C8828_7A1DB8(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_34,resources_34,services_34},
    {0x802c88c4,"src/valley/7A1850.c:169-176","POKEMON_FUNC(func_802C88C4_7A1E54)","void func_802C88C4_7A1E54(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_35,resources_35,services_35},
    {0x802c892c,"src/valley/7A1850.c:178-184","POKEMON_FUNC(func_802C892C_7A1EBC)","void func_802C892C_7A1EBC(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_36,resources_36,services_36},
    {0x802c8998,"src/valley/7A1850.c:186-192","POKEMON_FUNC(func_802C8998_7A1F28)","void func_802C8998_7A1F28(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_37,resources_37,services_37},
    {0x802c89f4,"src/valley/7A1850.c:194-210","f32 func_802C89F4_7A1F84(s32 behavior)","float func_802C89F4_7A1F84(IsleValleyActorSource* s, int32_t behavior)",SourceAbi::utility,SourceDomain::actor,calls_38,resources_38,services_38},
    {0x802c8a4c,"src/valley/7A1850.c:212-225","void func_802C8A4C_7A1FDC(DObj* model, Pokemon* pokemon, Vec3f* arg2)","void func_802C8A4C_7A1FDC(IsleValleyActorSource* s, IsleActorNodeRef model, IsleActorSource* pokemon, IsleValleyVectorRef arg2)",SourceAbi::utility,SourceDomain::actor,calls_39,resources_39,services_39},
    {0x802c8b38,"src/valley/7A1850.c:227-244","POKEMON_FUNC(func_802C8B38_7A20C8)","void func_802C8B38_7A20C8(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_40,resources_40,services_40},
    {0x802c8c08,"src/valley/7A1850.c:246-252","POKEMON_FUNC(func_802C8C08_7A2198)","void func_802C8C08_7A2198(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_41,resources_41,services_41},
    {0x802c8c70,"src/valley/7A1850.c:254-264","POKEMON_FUNC(func_802C8C70_7A2200)","void func_802C8C70_7A2200(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_42,resources_42,services_42},
    {0x802c8d30,"src/valley/7A1850.c:266-288","POKEMON_FUNC(func_802C8D30_7A22C0)","void func_802C8D30_7A22C0(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_43,resources_43,services_43},
    {0x802c8e54,"src/valley/7A1850.c:290-296","POKEMON_FUNC(func_802C8E54_7A23E4)","void func_802C8E54_7A23E4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_44,resources_44,services_44},
    {0x802c8ea8,"src/valley/7A1850.c:298-306","POKEMON_FUNC(func_802C8EA8_7A2438)","void func_802C8EA8_7A2438(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_45,resources_45,services_45},
    {0x802c8f50,"src/valley/7A1850.c:308-323","POKEMON_FUNC(func_802C8F50_7A24E0)","void func_802C8F50_7A24E0(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_46,resources_46,services_46},
    {0x802c903c,"src/valley/7A1850.c:325-333","POKEMON_FUNC(func_802C903C_7A25CC)","void func_802C903C_7A25CC(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_47,resources_47,services_47},
    {0x802c90e4,"src/valley/7A1850.c:335-346","POKEMON_FUNC(func_802C90E4_7A2674)","void func_802C90E4_7A2674(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_48,resources_48,services_48},
    {0x802c91ac,"src/valley/7A1850.c:348-372","POKEMON_FUNC(func_802C91AC_7A273C)","void func_802C91AC_7A273C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_49,resources_49,services_49},
    {0x802c931c,"src/valley/7A1850.c:374-388","POKEMON_FUNC(func_802C931C_7A28AC)","void func_802C931C_7A28AC(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_50,resources_50,services_50},
    {0x802c9408,"src/valley/7A1850.c:390-398","POKEMON_FUNC(func_802C9408_7A2998)","void func_802C9408_7A2998(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_51,resources_51,services_51},
    {0x802c94a4,"src/valley/7A1850.c:400-424","POKEMON_FUNC(func_802C94A4_7A2A34)","void func_802C94A4_7A2A34(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_52,resources_52,services_52},
    {0x802c957c,"src/valley/7A1850.c:426-428","GObj* func_802C957C_7A2B0C(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)","IsleGameObject* func_802C957C_7A2B0C(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn, IsleActorFactoryInitialization* initData)",SourceAbi::factory_returning6,SourceDomain::actor,calls_53,resources_53,services_53},
    {0x802c95c0,"src/valley/7A2B50.c:18-20","POKEMON_FUNC(func_802C95C0_7A2B50)","void func_802C95C0_7A2B50(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_54,resources_54,services_54},
    {0x802c95e4,"src/valley/7A2B50.c:22-27","POKEMON_FUNC(func_802C95E4_7A2B74)","void func_802C95E4_7A2B74(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_55,resources_55,services_55},
    {0x802c9634,"src/valley/7A2B50.c:29-36","POKEMON_FUNC(func_802C9634_7A2BC4)","void func_802C9634_7A2BC4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_56,resources_56,services_56},
    {0x802c96a0,"src/valley/7A2B50.c:38-43","POKEMON_FUNC(func_802C96A0_7A2C30)","void func_802C96A0_7A2C30(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_57,resources_57,services_57},
    {0x802c96fc,"src/valley/7A2B50.c:45-50","POKEMON_FUNC(func_802C96FC_7A2C8C)","void func_802C96FC_7A2C8C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_58,resources_58,services_58},
    {0x802c9750,"src/valley/7A2B50.c:52-57","POKEMON_FUNC(func_802C9750_7A2CE0)","void func_802C9750_7A2CE0(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_59,resources_59,services_59},
    {0x802c97a4,"src/valley/7A2B50.c:59-64","POKEMON_FUNC(func_802C97A4_7A2D34)","void func_802C97A4_7A2D34(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_60,resources_60,services_60},
    {0x802c97f8,"src/valley/7A2B50.c:66-70","GObj* func_802C97F8_7A2D88(s32 gObjID, u16 id, WorldBlock* roomA, WorldBlock* roomB, ObjectSpawn* spawn)","IsleGameObject* func_802C97F8_7A2D88(int32_t gObjID, uint16_t id, IsleActorFactoryBlock* roomA, IsleActorFactoryBlock* roomB, IsleActorFactorySpawn* spawn)",SourceAbi::factory_returning5,SourceDomain::actor,calls_61,resources_61,services_61},
    {0x802c9840,"src/valley/7A2DD0.c:25-53","POKEMON_FUNC(func_802C9840_7A2DD0)","void func_802C9840_7A2DD0(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_62,resources_62,services_62},
    {0x802c9958,"src/valley/7A2DD0.c:55-58","POKEMON_FUNC(func_802C9958_7A2EE8)","void func_802C9958_7A2EE8(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_63,resources_63,services_63},
    {0x802c9990,"src/valley/7A2DD0.c:60-68","POKEMON_FUNC(func_802C9990_7A2F20)","void func_802C9990_7A2F20(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_64,resources_64,services_64},
    {0x802c9a48,"src/valley/7A2DD0.c:70-75","POKEMON_FUNC(func_802C9A48_7A2FD8)","void func_802C9A48_7A2FD8(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_65,resources_65,services_65},
    {0x802c9a94,"src/valley/7A2DD0.c:77-84","POKEMON_FUNC(func_802C9A94_7A3024)","void func_802C9A94_7A3024(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_66,resources_66,services_66},
    {0x802c9b0c,"src/valley/7A2DD0.c:86-92","POKEMON_FUNC(func_802C9B0C_7A309C)","void func_802C9B0C_7A309C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_67,resources_67,services_67},
    {0x802c9b78,"src/valley/7A2DD0.c:94-99","POKEMON_FUNC(func_802C9B78_7A3108)","void func_802C9B78_7A3108(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_68,resources_68,services_68},
    {0x802c9bd0,"src/valley/7A2DD0.c:101-113","POKEMON_FUNC(func_802C9BD0_7A3160)","void func_802C9BD0_7A3160(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_69,resources_69,services_69},
    {0x802c9c84,"src/valley/7A2DD0.c:115-124","POKEMON_FUNC(func_802C9C84_7A3214)","void func_802C9C84_7A3214(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_70,resources_70,services_70},
    {0x802c9d00,"src/valley/7A2DD0.c:126-135","POKEMON_FUNC(func_802C9D00_7A3290)","void func_802C9D00_7A3290(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_71,resources_71,services_71},
    {0x802c9dc4,"src/valley/7A2DD0.c:137-147","POKEMON_FUNC(func_802C9DC4_7A3354)","void func_802C9DC4_7A3354(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_72,resources_72,services_72},
    {0x802c9e60,"src/valley/7A2DD0.c:149-156","POKEMON_FUNC(func_802C9E60_7A33F0)","void func_802C9E60_7A33F0(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_73,resources_73,services_73},
    {0x802c9ec8,"src/valley/7A2DD0.c:158-164","POKEMON_FUNC(func_802C9EC8_7A3458)","void func_802C9EC8_7A3458(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_74,resources_74,services_74},
    {0x802c9f34,"src/valley/7A2DD0.c:166-172","POKEMON_FUNC(func_802C9F34_7A34C4)","void func_802C9F34_7A34C4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_75,resources_75,services_75},
    {0x802c9f90,"src/valley/7A2DD0.c:174-176","GObj* func_802C9F90_7A3520(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)","IsleGameObject* func_802C9F90_7A3520(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn, IsleActorFactoryInitialization* initData)",SourceAbi::factory_returning6,SourceDomain::actor,calls_76,resources_76,services_76},
    {0x802c9fd0,"src/valley/7A3560.c:15-20","POKEMON_FUNC(func_802C9FD0_7A3560)","void func_802C9FD0_7A3560(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_77,resources_77,services_77},
    {0x802ca020,"src/valley/7A3560.c:22-61","POKEMON_FUNC(func_802CA020_7A35B0)","void func_802CA020_7A35B0(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_78,resources_78,services_78},
    {0x802ca1b0,"src/valley/7A3560.c:63-69","POKEMON_FUNC(func_802CA1B0_7A3740)","void func_802CA1B0_7A3740(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_79,resources_79,services_79},
    {0x801e04a8,"src/valley/7A3560.c:71-73","static void nullsub(void)","void isle_valley_7A3560_nullsub(void)",SourceAbi::file_local_empty,SourceDomain::actor,calls_80,resources_80,services_80},
    {0x802ca224,"src/valley/7A3560.c:75-79","GObj* func_802CA224_7A37B4(s32 gObjID, u16 id, WorldBlock* roomA, WorldBlock* roomB, ObjectSpawn* spawn)","IsleGameObject* func_802CA224_7A37B4(int32_t gObjID, uint16_t id, IsleActorFactoryBlock* roomA, IsleActorFactoryBlock* roomB, IsleActorFactorySpawn* spawn)",SourceAbi::factory_returning5,SourceDomain::actor,calls_81,resources_81,services_81},
    {0x802ca270,"src/valley/7A3800.c:18-23","POKEMON_FUNC(func_802CA270_7A3800)","void func_802CA270_7A3800(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_82,resources_82,services_82},
    {0x802ca2bc,"src/valley/7A3800.c:25-28","POKEMON_FUNC(func_802CA2BC_7A384C)","void func_802CA2BC_7A384C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_83,resources_83,services_83},
    {0x802ca2f4,"src/valley/7A3800.c:30-33","POKEMON_FUNC(func_802CA2F4_7A3884)","void func_802CA2F4_7A3884(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_84,resources_84,services_84},
    {0x802ca32c,"src/valley/7A3800.c:35-38","POKEMON_FUNC(func_802CA32C_7A38BC)","void func_802CA32C_7A38BC(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_85,resources_85,services_85},
    {0x802ca364,"src/valley/7A3800.c:40-51","POKEMON_FUNC(func_802CA364_7A38F4)","void func_802CA364_7A38F4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_86,resources_86,services_86},
    {0x802ca434,"src/valley/7A3800.c:53-82","POKEMON_FUNC(func_802CA434_7A39C4)","void func_802CA434_7A39C4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_87,resources_87,services_87},
    {0x802ca574,"src/valley/7A3800.c:84-87","POKEMON_FUNC(func_802CA574_7A3B04)","void func_802CA574_7A3B04(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_88,resources_88,services_88},
    {0x802ca5b4,"src/valley/7A3800.c:89-104","POKEMON_FUNC(func_802CA5B4_7A3B44)","void func_802CA5B4_7A3B44(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_89,resources_89,services_89},
    {0x802ca65c,"src/valley/7A3800.c:106-109","POKEMON_FUNC(func_802CA65C_7A3BEC)","void func_802CA65C_7A3BEC(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_90,resources_90,services_90},
    {0x802ca688,"src/valley/7A3800.c:111-113","GObj* func_802CA688_7A3C18(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)","IsleGameObject* func_802CA688_7A3C18(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn, IsleActorFactoryInitialization* initData)",SourceAbi::factory_returning6,SourceDomain::actor,calls_91,resources_91,services_91},
    {0x802ca6c0,"src/valley/7A3C50.c:34-47","POKEMON_FUNC(func_802CA6C0_7A3C50)","void func_802CA6C0_7A3C50(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_92,resources_92,services_92},
    {0x802ca768,"src/valley/7A3C50.c:49-52","POKEMON_FUNC(func_802CA768_7A3CF8)","void func_802CA768_7A3CF8(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_93,resources_93,services_93},
    {0x802ca7a0,"src/valley/7A3C50.c:54-62","POKEMON_FUNC(func_802CA7A0_7A3D30)","void func_802CA7A0_7A3D30(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_94,resources_94,services_94},
    {0x802ca858,"src/valley/7A3C50.c:64-69","POKEMON_FUNC(func_802CA858_7A3DE8)","void func_802CA858_7A3DE8(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_95,resources_95,services_95},
    {0x802ca8a4,"src/valley/7A3C50.c:71-76","POKEMON_FUNC(func_802CA8A4_7A3E34)","void func_802CA8A4_7A3E34(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_96,resources_96,services_96},
    {0x802ca8f8,"src/valley/7A3C50.c:78-83","POKEMON_FUNC(func_802CA8F8_7A3E88)","void func_802CA8F8_7A3E88(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_97,resources_97,services_97},
    {0x802ca948,"src/valley/7A3C50.c:85-93","POKEMON_FUNC(func_802CA948_7A3ED8)","void func_802CA948_7A3ED8(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_98,resources_98,services_98},
    {0x802ca9bc,"src/valley/7A3C50.c:95-101","POKEMON_FUNC(func_802CA9BC_7A3F4C)","void func_802CA9BC_7A3F4C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_99,resources_99,services_99},
    {0x802caa2c,"src/valley/7A3C50.c:103-113","POKEMON_FUNC(func_802CAA2C_7A3FBC)","void func_802CAA2C_7A3FBC(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_100,resources_100,services_100},
    {0x802caad0,"src/valley/7A3C50.c:115-121","POKEMON_FUNC(func_802CAAD0_7A4060)","void func_802CAAD0_7A4060(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_101,resources_101,services_101},
    {0x802cab2c,"src/valley/7A3C50.c:123-133","POKEMON_FUNC(func_802CAB2C_7A40BC)","void func_802CAB2C_7A40BC(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_102,resources_102,services_102},
    {0x802cabd0,"src/valley/7A3C50.c:135-141","POKEMON_FUNC(func_802CABD0_7A4160)","void func_802CABD0_7A4160(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_103,resources_103,services_103},
    {0x802cac2c,"src/valley/7A3C50.c:143-155","POKEMON_FUNC(func_802CAC2C_7A41BC)","void func_802CAC2C_7A41BC(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_104,resources_104,services_104},
    {0x802cace4,"src/valley/7A3C50.c:157-163","POKEMON_FUNC(func_802CACE4_7A4274)","void func_802CACE4_7A4274(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_105,resources_105,services_105},
    {0x802cad54,"src/valley/7A3C50.c:165-174","POKEMON_FUNC(func_802CAD54_7A42E4)","void func_802CAD54_7A42E4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_106,resources_106,services_106},
    {0x802cadec,"src/valley/7A3C50.c:176-184","POKEMON_FUNC(func_802CADEC_7A437C)","void func_802CADEC_7A437C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_107,resources_107,services_107},
    {0x802cae70,"src/valley/7A3C50.c:186-195","POKEMON_FUNC(func_802CAE70_7A4400)","void func_802CAE70_7A4400(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_108,resources_108,services_108},
    {0x802caf04,"src/valley/7A3C50.c:197-202","POKEMON_FUNC(func_802CAF04_7A4494)","void func_802CAF04_7A4494(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_109,resources_109,services_109},
    {0x802caf54,"src/valley/7A3C50.c:204-214","POKEMON_FUNC(func_802CAF54_7A44E4)","void func_802CAF54_7A44E4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_110,resources_110,services_110},
    {0x802cb008,"src/valley/7A3C50.c:216-218","POKEMON_FUNC(func_802CB008_7A4598)","void func_802CB008_7A4598(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_111,resources_111,services_111},
    {0x802cb02c,"src/valley/7A3C50.c:220-222","void func_802CB02C_7A45BC(s32 gObjID, u16 id, WorldBlock* roomA, WorldBlock* roomB, ObjectSpawn* spawn)","void func_802CB02C_7A45BC(int32_t gObjID, uint16_t id, IsleActorFactoryBlock* roomA, IsleActorFactoryBlock* roomB, IsleActorFactorySpawn* spawn)",SourceAbi::factory_void5,SourceDomain::actor,calls_112,resources_112,services_112},
    {0x802cb080,"src/valley/7A4610.c:66-79","POKEMON_FUNC(func_802CB080_7A4610)","void func_802CB080_7A4610(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_113,resources_113,services_113},
    {0x802cb110,"src/valley/7A4610.c:81-90","POKEMON_FUNC(func_802CB110_7A46A0)","void func_802CB110_7A46A0(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_114,resources_114,services_114},
    {0x802cb194,"src/valley/7A4610.c:92-102","POKEMON_FUNC(func_802CB194_7A4724)","void func_802CB194_7A4724(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_115,resources_115,services_115},
    {0x802cb238,"src/valley/7A4610.c:104-110","POKEMON_FUNC(func_802CB238_7A47C8)","void func_802CB238_7A47C8(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_116,resources_116,services_116},
    {0x802cb294,"src/valley/7A4610.c:112-122","POKEMON_FUNC(func_802CB294_7A4824)","void func_802CB294_7A4824(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_117,resources_117,services_117},
    {0x802cb338,"src/valley/7A4610.c:124-130","POKEMON_FUNC(func_802CB338_7A48C8)","void func_802CB338_7A48C8(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_118,resources_118,services_118},
    {0x802cb394,"src/valley/7A4610.c:132-138","POKEMON_FUNC(func_802CB394_7A4924)","void func_802CB394_7A4924(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_119,resources_119,services_119},
    {0x802cb404,"src/valley/7A4610.c:140-146","POKEMON_FUNC(func_802CB404_7A4994)","void func_802CB404_7A4994(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_120,resources_120,services_120},
    {0x802cb474,"src/valley/7A4610.c:148-164","POKEMON_FUNC(func_802CB474_7A4A04)","void func_802CB474_7A4A04(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_121,resources_121,services_121},
    {0x802cb550,"src/valley/7A4610.c:166-172","POKEMON_FUNC(func_802CB550_7A4AE0)","void func_802CB550_7A4AE0(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_122,resources_122,services_122},
    {0x802cb5ac,"src/valley/7A4610.c:174-180","POKEMON_FUNC(func_802CB5AC_7A4B3C)","void func_802CB5AC_7A4B3C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_123,resources_123,services_123},
    {0x802cb614,"src/valley/7A4610.c:182-196","POKEMON_FUNC(func_802CB614_7A4BA4)","void func_802CB614_7A4BA4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_124,resources_124,services_124},
    {0x802cb6f8,"src/valley/7A4610.c:198-208","POKEMON_FUNC(func_802CB6F8_7A4C88)","void func_802CB6F8_7A4C88(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_125,resources_125,services_125},
    {0x802cb79c,"src/valley/7A4610.c:210-215","POKEMON_FUNC(func_802CB79C_7A4D2C)","void func_802CB79C_7A4D2C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_126,resources_126,services_126},
    {0x802cb7f4,"src/valley/7A4610.c:217-219","POKEMON_FUNC(func_802CB7F4_7A4D84)","void func_802CB7F4_7A4D84(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_127,resources_127,services_127},
    {0x802cb818,"src/valley/7A4610.c:221-227","POKEMON_FUNC(func_802CB818_7A4DA8)","void func_802CB818_7A4DA8(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_128,resources_128,services_128},
    {0x802cb88c,"src/valley/7A4610.c:229-234","POKEMON_FUNC(func_802CB88C_7A4E1C)","void func_802CB88C_7A4E1C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_129,resources_129,services_129},
    {0x802cb8d8,"src/valley/7A4610.c:236-243","POKEMON_FUNC(func_802CB8D8_7A4E68)","void func_802CB8D8_7A4E68(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_130,resources_130,services_130},
    {0x802cb92c,"src/valley/7A4610.c:245-254","POKEMON_FUNC(func_802CB92C_7A4EBC)","void func_802CB92C_7A4EBC(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_131,resources_131,services_131},
    {0x802cb9bc,"src/valley/7A4610.c:256-263","POKEMON_FUNC(func_802CB9BC_7A4F4C)","void func_802CB9BC_7A4F4C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_132,resources_132,services_132},
    {0x802cba24,"src/valley/7A4610.c:265-271","POKEMON_FUNC(func_802CBA24_7A4FB4)","void func_802CBA24_7A4FB4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_133,resources_133,services_133},
    {0x802cba90,"src/valley/7A4610.c:273-296","POKEMON_FUNC(func_802CBA90_7A5020)","void func_802CBA90_7A5020(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_134,resources_134,services_134},
    {0x802cbbdc,"src/valley/7A4610.c:298-308","POKEMON_FUNC(func_802CBBDC_7A516C)","void func_802CBBDC_7A516C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_135,resources_135,services_135},
    {0x802cbc74,"src/valley/7A4610.c:310-317","POKEMON_FUNC(func_802CBC74_7A5204)","void func_802CBC74_7A5204(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_136,resources_136,services_136},
    {0x802cbcdc,"src/valley/7A4610.c:319-329","POKEMON_FUNC(func_802CBCDC_7A526C)","void func_802CBCDC_7A526C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_137,resources_137,services_137},
    {0x802cbd74,"src/valley/7A4610.c:331-341","POKEMON_FUNC(func_802CBD74_7A5304)","void func_802CBD74_7A5304(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_138,resources_138,services_138},
    {0x802cbe14,"src/valley/7A4610.c:343-354","POKEMON_FUNC(func_802CBE14_7A53A4)","void func_802CBE14_7A53A4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_139,resources_139,services_139},
    {0x802cbe9c,"src/valley/7A4610.c:356-364","POKEMON_FUNC(func_802CBE9C_7A542C)","void func_802CBE9C_7A542C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_140,resources_140,services_140},
    {0x802cbf18,"src/valley/7A4610.c:366-371","POKEMON_FUNC(func_802CBF18_7A54A8)","void func_802CBF18_7A54A8(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_141,resources_141,services_141},
    {0x802cbf64,"src/valley/7A4610.c:373-381","POKEMON_FUNC(func_802CBF64_7A54F4)","void func_802CBF64_7A54F4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_142,resources_142,services_142},
    {0x802cbfdc,"src/valley/7A4610.c:383-393","POKEMON_FUNC(func_802CBFDC_7A556C)","void func_802CBFDC_7A556C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_143,resources_143,services_143},
    {0x802cc068,"src/valley/7A4610.c:395-397","POKEMON_FUNC(func_802CC068_7A55F8)","void func_802CC068_7A55F8(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_144,resources_144,services_144},
    {0x802cc08c,"src/valley/7A4610.c:399-406","POKEMON_FUNC(func_802CC08C_7A561C)","void func_802CC08C_7A561C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_145,resources_145,services_145},
    {0x802cc104,"src/valley/7A4610.c:408-423","POKEMON_FUNC(func_802CC104_7A5694)","void func_802CC104_7A5694(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_146,resources_146,services_146},
    {0x802cc1e0,"src/valley/7A4610.c:425-431","POKEMON_FUNC(func_802CC1E0_7A5770)","void func_802CC1E0_7A5770(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_147,resources_147,services_147},
    {0x802cc250,"src/valley/7A4610.c:433-441","POKEMON_FUNC(func_802CC250_7A57E0)","void func_802CC250_7A57E0(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_148,resources_148,services_148},
    {0x802cc2b4,"src/valley/7A4610.c:443-449","POKEMON_FUNC(func_802CC2B4_7A5844)","void func_802CC2B4_7A5844(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_149,resources_149,services_149},
    {0x802cc320,"src/valley/7A4610.c:451-459","POKEMON_FUNC(func_802CC320_7A58B0)","void func_802CC320_7A58B0(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_150,resources_150,services_150},
    {0x802cc3b4,"src/valley/7A4610.c:461-467","POKEMON_FUNC(func_802CC3B4_7A5944)","void func_802CC3B4_7A5944(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_151,resources_151,services_151},
    {0x802cc424,"src/valley/7A4610.c:469-480","POKEMON_FUNC(func_802CC424_7A59B4)","void func_802CC424_7A59B4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_152,resources_152,services_152},
    {0x802cc4dc,"src/valley/7A4610.c:482-485","POKEMON_FUNC(func_802CC4DC_7A5A6C)","void func_802CC4DC_7A5A6C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_153,resources_153,services_153},
    {0x802cc514,"src/valley/7A4610.c:487-501","POKEMON_FUNC(func_802CC514_7A5AA4)","void func_802CC514_7A5AA4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_154,resources_154,services_154},
    {0x802cc5c0,"src/valley/7A4610.c:503-561","POKEMON_FUNC(func_802CC5C0_7A5B50)","void func_802CC5C0_7A5B50(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_155,resources_155,services_155},
    {0x802cc8a8,"src/valley/7A4610.c:563-565","GObj* func_802CC8A8_7A5E38(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)","IsleGameObject* func_802CC8A8_7A5E38(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn, IsleActorFactoryInitialization* initData)",SourceAbi::factory_returning6,SourceDomain::actor,calls_156,resources_156,services_156},
    {0x802cc8e0,"src/valley/7A4610.c:567-573","void func_802CC8E0_7A5E70(GObj* obj, f32 arg1, f32 arg2, f32 arg3, f32 arg4, f32 arg5, f32 arg6)","void func_802CC8E0_7A5E70(IsleGameObject* obj, float arg1, float arg2, float arg3, float arg4, float arg5, float arg6)",SourceAbi::block_change,SourceDomain::actor,calls_157,resources_157,services_157},
    {0x802cc980,"src/valley/7A5F10.c:27-42","void func_802CC980_7A5F10(s32 arg0, Vec3f* arg1, f32* arg2)","void func_802CC980_7A5F10(IsleValleyActorSource* s, int32_t arg0, IsleValleyVectorRef arg1, float* arg2)",SourceAbi::utility,SourceDomain::actor,calls_158,resources_158,services_158},
    {0x802cca70,"src/valley/7A5F10.c:44-49","POKEMON_FUNC(func_802CCA70_7A6000)","void func_802CCA70_7A6000(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_159,resources_159,services_159},
    {0x802ccab4,"src/valley/7A5F10.c:51-58","POKEMON_FUNC(func_802CCAB4_7A6044)","void func_802CCAB4_7A6044(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_160,resources_160,services_160},
    {0x802ccb34,"src/valley/7A5F10.c:60-69","POKEMON_FUNC(func_802CCB34_7A60C4)","void func_802CCB34_7A60C4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_161,resources_161,services_161},
    {0x802ccbf8,"src/valley/7A5F10.c:71-79","POKEMON_FUNC(func_802CCBF8_7A6188)","void func_802CCBF8_7A6188(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_162,resources_162,services_162},
    {0x802ccc90,"src/valley/7A5F10.c:81-87","POKEMON_FUNC(func_802CCC90_7A6220)","void func_802CCC90_7A6220(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_163,resources_163,services_163},
    {0x802cccfc,"src/valley/7A5F10.c:89-97","POKEMON_FUNC(func_802CCCFC_7A628C)","void func_802CCCFC_7A628C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_164,resources_164,services_164},
    {0x802ccd80,"src/valley/7A5F10.c:99-106","POKEMON_FUNC(func_802CCD80_7A6310)","void func_802CCD80_7A6310(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_165,resources_165,services_165},
    {0x802ccddc,"src/valley/7A5F10.c:108-116","POKEMON_FUNC(func_802CCDDC_7A636C)","void func_802CCDDC_7A636C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_166,resources_166,services_166},
    {0x802cce70,"src/valley/7A5F10.c:118-169","POKEMON_FUNC(func_802CCE70_7A6400)","void func_802CCE70_7A6400(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_167,resources_167,services_167},
    {0x802cd0b8,"src/valley/7A5F10.c:171-178","POKEMON_FUNC(func_802CD0B8_7A6648)","void func_802CD0B8_7A6648(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_168,resources_168,services_168},
    {0x802cd118,"src/valley/7A5F10.c:180-189","POKEMON_FUNC(func_802CD118_7A66A8)","void func_802CD118_7A66A8(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_169,resources_169,services_169},
    {0x802cd1ac,"src/valley/7A5F10.c:191-207","POKEMON_FUNC(func_802CD1AC_7A673C)","void func_802CD1AC_7A673C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_170,resources_170,services_170},
    {0x802cd2a8,"src/valley/7A5F10.c:209-214","POKEMON_FUNC(func_802CD2A8_7A6838)","void func_802CD2A8_7A6838(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_171,resources_171,services_171},
    {0x802cd2fc,"src/valley/7A5F10.c:216-253","POKEMON_FUNC(func_802CD2FC_7A688C)","void func_802CD2FC_7A688C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_172,resources_172,services_172},
    {0x802cd4f4,"src/valley/7A5F10.c:255-275","POKEMON_FUNC(func_802CD4F4_7A6A84)","void func_802CD4F4_7A6A84(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_173,resources_173,services_173},
    {0x802cd5d8,"src/valley/7A5F10.c:277-300","POKEMON_FUNC(func_802CD5D8_7A6B68)","void func_802CD5D8_7A6B68(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_174,resources_174,services_174},
    {0x802cd704,"src/valley/7A5F10.c:302-304","GObj* func_802CD704_7A6C94(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)","IsleGameObject* func_802CD704_7A6C94(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn, IsleActorFactoryInitialization* initData)",SourceAbi::factory_returning6,SourceDomain::actor,calls_175,resources_175,services_175},
    {0x802cd73c,"src/valley/7A5F10.c:306-308","GObj* func_802CD73C_7A6CCC(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)","IsleGameObject* func_802CD73C_7A6CCC(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn, IsleActorFactoryInitialization* initData)",SourceAbi::factory_returning6,SourceDomain::actor,calls_176,resources_176,services_176},
    {0x802cd774,"src/valley/7A5F10.c:310-314","POKEMON_FUNC(func_802CD774_7A6D04)","void func_802CD774_7A6D04(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_177,resources_177,services_177},
    {0x802cd7c0,"src/valley/7A6D50.c:20-33","POKEMON_FUNC(func_802CD7C0_7A6D50)","void func_802CD7C0_7A6D50(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_178,resources_178,services_178},
    {0x802cd84c,"src/valley/7A6D50.c:35-39","POKEMON_FUNC(func_802CD84C_7A6DDC)","void func_802CD84C_7A6DDC(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_179,resources_179,services_179},
    {0x802cd8a4,"src/valley/7A6D50.c:41-44","POKEMON_FUNC(func_802CD8A4_7A6E34)","void func_802CD8A4_7A6E34(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_180,resources_180,services_180},
    {0x802cd8dc,"src/valley/7A6D50.c:46-49","POKEMON_FUNC(func_802CD8DC_7A6E6C)","void func_802CD8DC_7A6E6C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_181,resources_181,services_181},
    {0x802cd914,"src/valley/7A6D50.c:51-59","POKEMON_FUNC(func_802CD914_7A6EA4)","void func_802CD914_7A6EA4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_182,resources_182,services_182},
    {0x802cd98c,"src/valley/7A6D50.c:61-67","POKEMON_FUNC(func_802CD98C_7A6F1C)","void func_802CD98C_7A6F1C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_183,resources_183,services_183},
    {0x802cd9f8,"src/valley/7A6D50.c:69-78","POKEMON_FUNC(func_802CD9F8_7A6F88)","void func_802CD9F8_7A6F88(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_184,resources_184,services_184},
    {0x802cda94,"src/valley/7A6D50.c:80-88","POKEMON_FUNC(func_802CDA94_7A7024)","void func_802CDA94_7A7024(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_185,resources_185,services_185},
    {0x802cdb40,"src/valley/7A6D50.c:90-98","POKEMON_FUNC(func_802CDB40_7A70D0)","void func_802CDB40_7A70D0(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_186,resources_186,services_186},
    {0x802cdbd4,"src/valley/7A6D50.c:100-108","POKEMON_FUNC(func_802CDBD4_7A7164)","void func_802CDBD4_7A7164(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_187,resources_187,services_187},
    {0x802cdc68,"src/valley/7A6D50.c:110-118","POKEMON_FUNC(func_802CDC68_7A71F8)","void func_802CDC68_7A71F8(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_188,resources_188,services_188},
    {0x802cdcfc,"src/valley/7A6D50.c:120-136","POKEMON_FUNC(func_802CDCFC_7A728C)","void func_802CDCFC_7A728C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_189,resources_189,services_189},
    {0x802cde04,"src/valley/7A6D50.c:138-140","GObj* func_802CDE04_7A7394(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)","IsleGameObject* func_802CDE04_7A7394(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn, IsleActorFactoryInitialization* initData)",SourceAbi::factory_returning6,SourceDomain::actor,calls_190,resources_190,services_190},
    {0x802cde3c,"src/valley/7A6D50.c:142-144","GObj* func_802CDE3C_7A73CC(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)","IsleGameObject* func_802CDE3C_7A73CC(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn, IsleActorFactoryInitialization* initData)",SourceAbi::factory_returning6,SourceDomain::actor,calls_191,resources_191,services_191},
    {0x802cde80,"src/valley/7A7410.c:22-24","POKEMON_FUNC(func_802CDE80_7A7410)","void func_802CDE80_7A7410(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_192,resources_192,services_192},
    {0x802cdea4,"src/valley/7A7410.c:26-31","POKEMON_FUNC(func_802CDEA4_7A7434)","void func_802CDEA4_7A7434(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_193,resources_193,services_193},
    {0x802cdefc,"src/valley/7A7410.c:33-39","POKEMON_FUNC(func_802CDEFC_7A748C)","void func_802CDEFC_7A748C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_194,resources_194,services_194},
    {0x802cdf68,"src/valley/7A7410.c:41-50","POKEMON_FUNC(func_802CDF68_7A74F8)","void func_802CDF68_7A74F8(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_195,resources_195,services_195},
    {0x802cdffc,"src/valley/7A7410.c:52-61","POKEMON_FUNC(func_802CDFFC_7A758C)","void func_802CDFFC_7A758C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_196,resources_196,services_196},
    {0x802ce090,"src/valley/7A7410.c:63-72","POKEMON_FUNC(func_802CE090_7A7620)","void func_802CE090_7A7620(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_197,resources_197,services_197},
    {0x802ce124,"src/valley/7A7410.c:74-84","POKEMON_FUNC(func_802CE124_7A76B4)","void func_802CE124_7A76B4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_198,resources_198,services_198},
    {0x802ce1b8,"src/valley/7A7410.c:86-90","GObj* func_802CE1B8_7A7748(s32 gObjID, u16 id, WorldBlock* roomA, WorldBlock* roomB, ObjectSpawn* spawn)","IsleGameObject* func_802CE1B8_7A7748(int32_t gObjID, uint16_t id, IsleActorFactoryBlock* roomA, IsleActorFactoryBlock* roomB, IsleActorFactorySpawn* spawn)",SourceAbi::factory_returning5,SourceDomain::actor,calls_199,resources_199,services_199},
    {0x802ce200,"src/valley/7A7790.c:34-36","POKEMON_FUNC(func_802CE200_7A7790)","void func_802CE200_7A7790(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_200,resources_200,services_200},
    {0x802ce224,"src/valley/7A7790.c:38-46","POKEMON_FUNC(func_802CE224_7A77B4)","void func_802CE224_7A77B4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_201,resources_201,services_201},
    {0x802ce2dc,"src/valley/7A7790.c:48-53","POKEMON_FUNC(func_802CE2DC_7A786C)","void func_802CE2DC_7A786C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_202,resources_202,services_202},
    {0x802ce328,"src/valley/7A7790.c:55-60","POKEMON_FUNC(func_802CE328_7A78B8)","void func_802CE328_7A78B8(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_203,resources_203,services_203},
    {0x802ce37c,"src/valley/7A7790.c:62-68","POKEMON_FUNC(func_802CE37C_7A790C)","void func_802CE37C_7A790C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_204,resources_204,services_204},
    {0x802ce3cc,"src/valley/7A7790.c:70-79","POKEMON_FUNC(func_802CE3CC_7A795C)","void func_802CE3CC_7A795C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_205,resources_205,services_205},
    {0x802ce440,"src/valley/7A7790.c:81-87","POKEMON_FUNC(func_802CE440_7A79D0)","void func_802CE440_7A79D0(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_206,resources_206,services_206},
    {0x802ce4b0,"src/valley/7A7790.c:89-99","POKEMON_FUNC(func_802CE4B0_7A7A40)","void func_802CE4B0_7A7A40(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_207,resources_207,services_207},
    {0x802ce554,"src/valley/7A7790.c:101-107","POKEMON_FUNC(func_802CE554_7A7AE4)","void func_802CE554_7A7AE4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_208,resources_208,services_208},
    {0x802ce5b0,"src/valley/7A7790.c:109-119","POKEMON_FUNC(func_802CE5B0_7A7B40)","void func_802CE5B0_7A7B40(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_209,resources_209,services_209},
    {0x802ce654,"src/valley/7A7790.c:121-127","POKEMON_FUNC(func_802CE654_7A7BE4)","void func_802CE654_7A7BE4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_210,resources_210,services_210},
    {0x802ce6b0,"src/valley/7A7790.c:129-141","POKEMON_FUNC(func_802CE6B0_7A7C40)","void func_802CE6B0_7A7C40(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_211,resources_211,services_211},
    {0x802ce768,"src/valley/7A7790.c:143-149","POKEMON_FUNC(func_802CE768_7A7CF8)","void func_802CE768_7A7CF8(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_212,resources_212,services_212},
    {0x802ce7d8,"src/valley/7A7790.c:151-160","POKEMON_FUNC(func_802CE7D8_7A7D68)","void func_802CE7D8_7A7D68(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_213,resources_213,services_213},
    {0x802ce870,"src/valley/7A7790.c:162-170","POKEMON_FUNC(func_802CE870_7A7E00)","void func_802CE870_7A7E00(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_214,resources_214,services_214},
    {0x802ce8f4,"src/valley/7A7790.c:172-180","POKEMON_FUNC(func_802CE8F4_7A7E84)","void func_802CE8F4_7A7E84(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_215,resources_215,services_215},
    {0x802ce978,"src/valley/7A7790.c:182-192","POKEMON_FUNC(func_802CE978_7A7F08)","void func_802CE978_7A7F08(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_216,resources_216,services_216},
    {0x802cea34,"src/valley/7A7790.c:194-196","POKEMON_FUNC(func_802CEA34_7A7FC4)","void func_802CEA34_7A7FC4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_217,resources_217,services_217},
    {0x802cea58,"src/valley/7A7790.c:198-200","GObj* func_802CEA58_7A7FE8(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)","IsleGameObject* func_802CEA58_7A7FE8(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn, IsleActorFactoryInitialization* initData)",SourceAbi::factory_returning6,SourceDomain::actor,calls_218,resources_218,services_218},
    {0x802ceaa0,"src/valley/7A8030.c:32-47","POKEMON_FUNC(func_802CEAA0_7A8030)","void func_802CEAA0_7A8030(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_219,resources_219,services_219},
    {0x802ceb4c,"src/valley/7A8030.c:49-55","POKEMON_FUNC(func_802CEB4C_7A80DC)","void func_802CEB4C_7A80DC(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_220,resources_220,services_220},
    {0x802ceb9c,"src/valley/7A8030.c:57-65","POKEMON_FUNC(func_802CEB9C_7A812C)","void func_802CEB9C_7A812C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_221,resources_221,services_221},
    {0x802cec18,"src/valley/7A8030.c:67-90","f32 func_802CEC18_7A81A8(Vec3f* arg0, Vec3f* arg1)","float func_802CEC18_7A81A8(IsleValleyActorSource* s, const IsleGameVec3* arg0, const IsleGameVec3* arg1)",SourceAbi::utility,SourceDomain::actor,calls_222,resources_222,services_222},
    {0x802ced44,"src/valley/7A8030.c:92-138","POKEMON_FUNC(func_802CED44_7A82D4)","void func_802CED44_7A82D4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_223,resources_223,services_223},
    {0x802cefb8,"src/valley/7A8030.c:140-149","POKEMON_FUNC(func_802CEFB8_7A8548)","void func_802CEFB8_7A8548(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_224,resources_224,services_224},
    {0x802cf07c,"src/valley/7A8030.c:151-161","POKEMON_FUNC(func_802CF07C_7A860C)","void func_802CF07C_7A860C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_225,resources_225,services_225},
    {0x802cf114,"src/valley/7A8030.c:163-209","POKEMON_FUNC(func_802CF114_7A86A4)","void func_802CF114_7A86A4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_226,resources_226,services_226},
    {0x802cf388,"src/valley/7A8030.c:211-217","POKEMON_FUNC(func_802CF388_7A8918)","void func_802CF388_7A8918(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_227,resources_227,services_227},
    {0x802cf3f8,"src/valley/7A8030.c:219-229","POKEMON_FUNC(func_802CF3F8_7A8988)","void func_802CF3F8_7A8988(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_228,resources_228,services_228},
    {0x802cf48c,"src/valley/7A8030.c:231-238","POKEMON_FUNC(func_802CF48C_7A8A1C)","void func_802CF48C_7A8A1C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_229,resources_229,services_229},
    {0x802cf500,"src/valley/7A8030.c:240-242","POKEMON_FUNC(func_802CF500_7A8A90)","void func_802CF500_7A8A90(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_230,resources_230,services_230},
    {0x802cf524,"src/valley/7A8030.c:244-250","POKEMON_FUNC(func_802CF524_7A8AB4)","void func_802CF524_7A8AB4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_231,resources_231,services_231},
    {0x802cf594,"src/valley/7A8030.c:252-260","POKEMON_FUNC(func_802CF594_7A8B24)","void func_802CF594_7A8B24(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_232,resources_232,services_232},
    {0x802cf638,"src/valley/7A8030.c:262-264","POKEMON_FUNC(func_802CF638_7A8BC8)","void func_802CF638_7A8BC8(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_233,resources_233,services_233},
    {0x802cf65c,"src/valley/7A8030.c:266-272","POKEMON_FUNC(func_802CF65C_7A8BEC)","void func_802CF65C_7A8BEC(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_234,resources_234,services_234},
    {0x802cf6cc,"src/valley/7A8030.c:274-279","POKEMON_FUNC(func_802CF6CC_7A8C5C)","void func_802CF6CC_7A8C5C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_235,resources_235,services_235},
    {0x802cf720,"src/valley/7A8030.c:281-288","POKEMON_FUNC(func_802CF720_7A8CB0)","void func_802CF720_7A8CB0(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_236,resources_236,services_236},
    {0x802cf794,"src/valley/7A8030.c:290-297","POKEMON_FUNC(func_802CF794_7A8D24)","void func_802CF794_7A8D24(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_237,resources_237,services_237},
    {0x802cf808,"src/valley/7A8030.c:299-301","GObj* func_802CF808_7A8D98(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)","IsleGameObject* func_802CF808_7A8D98(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn, IsleActorFactoryInitialization* initData)",SourceAbi::factory_returning6,SourceDomain::actor,calls_238,resources_238,services_238},
    {0x802cf840,"src/valley/7A8DD0.c:54-56","void func_802CF840_7A8DD0(GObj* obj)","void func_802CF840_7A8DD0(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_239,resources_239,services_239},
    {0x802cf864,"src/valley/7A8DD0.c:58-63","POKEMON_FUNC(func_802CF864_7A8DF4)","void func_802CF864_7A8DF4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_240,resources_240,services_240},
    {0x802cf8bc,"src/valley/7A8DD0.c:65-83","POKEMON_FUNC(func_802CF8BC_7A8E4C)","void func_802CF8BC_7A8E4C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_241,resources_241,services_241},
    {0x802cfa00,"src/valley/7A8DD0.c:85-89","GObj* func_802CFA00_7A8F90(s32 gObjID, u16 id, WorldBlock* roomA, WorldBlock* roomB, ObjectSpawn* spawn)","IsleGameObject* func_802CFA00_7A8F90(int32_t gObjID, uint16_t id, IsleActorFactoryBlock* roomA, IsleActorFactoryBlock* roomB, IsleActorFactorySpawn* spawn)",SourceAbi::factory_returning5,SourceDomain::actor,calls_242,resources_242,services_242},
    {0x802cfa40,"src/valley/7A8DD0.c:91-125","void func_802CFA40_7A8FD0(GObj* obj)","void func_802CFA40_7A8FD0(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_243,resources_243,services_243},
    {0x802cfc6c,"src/valley/7A8DD0.c:127-159","void func_802CFC6C_7A91FC(GObj* arg0)","void func_802CFC6C_7A91FC(IsleGameObject* arg0)",SourceAbi::gobj_callback,SourceDomain::actor,calls_244,resources_244,services_244},
    {0x802cfe8c,"src/valley/7A8DD0.c:161-237","void func_802CFE8C_7A941C(void)","void func_802CFE8C_7A941C(IsleValleyActorSource* s)",SourceAbi::utility,SourceDomain::actor,calls_245,resources_245,services_245},
    {0x802d017c,"src/valley/7A8DD0.c:239-278","void func_802D017C_7A970C(void)","void func_802D017C_7A970C(IsleValleyActorSource* s)",SourceAbi::utility,SourceDomain::actor,calls_246,resources_246,services_246},
    {0x802d0320,"src/valley/7A8DD0.c:280-282","void func_802D0320_7A98B0(GObj* obj)","void func_802D0320_7A98B0(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_247,resources_247,services_247},
    {0x802d0344,"src/valley/7A8DD0.c:284-289","POKEMON_FUNC(func_802D0344_7A98D4)","void func_802D0344_7A98D4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_248,resources_248,services_248},
    {0x802d039c,"src/valley/7A8DD0.c:291-296","void func_802D039C_7A992C(f32 speed)","void func_802D039C_7A992C(IsleValleyActorSource* s, float speed)",SourceAbi::utility,SourceDomain::actor,calls_249,resources_249,services_249},
    {0x802d03d0,"src/valley/7A8DD0.c:298-303","void func_802D03D0_7A9960(f32 skipFrames)","void func_802D03D0_7A9960(IsleValleyActorSource* s, float skipFrames)",SourceAbi::utility,SourceDomain::actor,calls_250,resources_250,services_250},
    {0x802d0420,"src/valley/7A8DD0.c:305-325","void func_802D0420_7A99B0(DObj* arg0, s32 arg1, f32 arg2)","void func_802D0420_7A99B0(IsleValleyActorSource* s, const IsleActorNodeRef* arg0, int32_t arg1, float arg2)",SourceAbi::animation,SourceDomain::actor,calls_251,resources_251,services_251},
    {0x802d04bc,"src/valley/7A8DD0.c:327-333","void func_802D04BC_7A9A4C(DObj* arg0, s32 arg1, f32 arg2)","void func_802D04BC_7A9A4C(IsleValleyActorSource* s, const IsleActorNodeRef* arg0, int32_t arg1, float arg2)",SourceAbi::animation,SourceDomain::actor,calls_252,resources_252,services_252},
    {0x802d04cc,"src/valley/7A8DD0.c:335-382","POKEMON_FUNC(func_802D04CC_7A9A5C)","void func_802D04CC_7A9A5C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_253,resources_253,services_253},
    {0x802d0700,"src/valley/7A8DD0.c:384-387","void func_802D0700_7A9C90(void)","void func_802D0700_7A9C90(IsleValleyActorSource* s)",SourceAbi::utility,SourceDomain::actor,calls_254,resources_254,services_254},
    {0x802d0754,"src/valley/7A8DD0.c:389-403","void func_802D0754_7A9CE4(s32 arg0)","void func_802D0754_7A9CE4(IsleValleyActorSource* s, int32_t arg0)",SourceAbi::utility,SourceDomain::actor,calls_255,resources_255,services_255},
    {0x802d07bc,"src/valley/7A8DD0.c:405-469","void func_802D07BC_7A9D4C(GObj* obj)","void func_802D07BC_7A9D4C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_256,resources_256,services_256},
    {0x802d0b3c,"src/valley/7A8DD0.c:471-475","GObj* func_802D0B3C_7AA0CC(s32 gObjID, u16 id, WorldBlock* roomA, WorldBlock* roomB, ObjectSpawn* spawn)","IsleGameObject* func_802D0B3C_7AA0CC(int32_t gObjID, uint16_t id, IsleActorFactoryBlock* roomA, IsleActorFactoryBlock* roomB, IsleActorFactorySpawn* spawn)",SourceAbi::factory_returning5,SourceDomain::actor,calls_257,resources_257,services_257},
    {0x802d0b80,"src/valley/7AA110.c:26-28","POKEMON_FUNC(func_802D0B80_7AA110)","void func_802D0B80_7AA110(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_258,resources_258,services_258},
    {0x802d0ba4,"src/valley/7AA110.c:30-35","POKEMON_FUNC(func_802D0BA4_7AA134)","void func_802D0BA4_7AA134(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_259,resources_259,services_259},
    {0x802d0bfc,"src/valley/7AA110.c:37-52","POKEMON_FUNC(func_802D0BFC_7AA18C)","void func_802D0BFC_7AA18C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_260,resources_260,services_260},
    {0x802d0cd8,"src/valley/7AA110.c:54-59","void func_802D0CD8_7AA268(DObj* obj, s32 arg1, f32 arg2)","void func_802D0CD8_7AA268(IsleValleyActorSource* s, const IsleActorNodeRef* obj, int32_t arg1, float arg2)",SourceAbi::animation,SourceDomain::actor,calls_261,resources_261,services_261},
    {0x802d0d0c,"src/valley/7AA110.c:61-81","POKEMON_FUNC(func_802D0D0C_7AA29C)","void func_802D0D0C_7AA29C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_262,resources_262,services_262},
    {0x802d0f0c,"src/valley/7AA110.c:83-94","POKEMON_FUNC(func_802D0F0C_7AA49C)","void func_802D0F0C_7AA49C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_263,resources_263,services_263},
    {0x802d0f98,"src/valley/7AA110.c:96-136","POKEMON_FUNC(func_802D0F98_7AA528)","void func_802D0F98_7AA528(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_264,resources_264,services_264},
    {0x802d1154,"src/valley/7AA110.c:138-140","GObj* func_802D1154_7AA6E4(s32 objID, u16 id, WorldBlock* block, WorldBlock* blockB, ObjectSpawn* spawn, PokemonInitData* initData)","IsleGameObject* func_802D1154_7AA6E4(int32_t objID, uint16_t id, IsleActorFactoryBlock* block, IsleActorFactoryBlock* blockB, IsleActorFactorySpawn* spawn, IsleActorFactoryInitialization* initData)",SourceAbi::factory_returning6,SourceDomain::actor,calls_265,resources_265,services_265},
    {0x802d1190,"src/valley/7AA720.c:3-5","void func_802D1190(GObj* obj)","void func_802D1190(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_266,resources_266,services_266},
    {0x802d11b0,"src/valley/7AA720.c:7-9","void func_802D11B0(s32 gObjID, u16 id, WorldBlock* roomA, WorldBlock* roomB, ObjectSpawn* spawn)","void func_802D11B0(int32_t gObjID, uint16_t id, IsleActorFactoryBlock* roomA, IsleActorFactoryBlock* roomB, IsleActorFactorySpawn* spawn)",SourceAbi::factory_void5,SourceDomain::actor,calls_267,resources_267,services_267},
    {0x802d1200,"src/valley/7AA790.c:43-55","POKEMON_FUNC(func_802D1200_7AA790)","void func_802D1200_7AA790(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_268,resources_268,services_268},
    {0x802d129c,"src/valley/7AA790.c:57-60","POKEMON_FUNC(func_802D129C_7AA82C)","void func_802D129C_7AA82C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_269,resources_269,services_269},
    {0x802d12d4,"src/valley/7AA790.c:62-65","POKEMON_FUNC(func_802D12D4_7AA864)","void func_802D12D4_7AA864(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_270,resources_270,services_270},
    {0x802d130c,"src/valley/7AA790.c:67-70","POKEMON_FUNC(func_802D130C_7AA89C)","void func_802D130C_7AA89C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_271,resources_271,services_271},
    {0x802d1344,"src/valley/7AA790.c:72-83","POKEMON_FUNC(func_802D1344_7AA8D4)","void func_802D1344_7AA8D4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_272,resources_272,services_272},
    {0x802d1414,"src/valley/7AA790.c:85-90","POKEMON_FUNC(func_802D1414_7AA9A4)","void func_802D1414_7AA9A4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_273,resources_273,services_273},
    {0x802d1454,"src/valley/7AA790.c:92-115","POKEMON_FUNC(func_802D1454_7AA9E4)","void func_802D1454_7AA9E4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_274,resources_274,services_274},
    {0x802d156c,"src/valley/7AA790.c:117-120","POKEMON_FUNC(func_802D156C_7AAAFC)","void func_802D156C_7AAAFC(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_275,resources_275,services_275},
    {0x802d1598,"src/valley/7AA790.c:122-124","POKEMON_FUNC(func_802D1598_7AAB28)","void func_802D1598_7AAB28(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_276,resources_276,services_276},
    {0x802d15bc,"src/valley/7AA790.c:126-150","POKEMON_FUNC(func_802D15BC_7AAB4C)","void func_802D15BC_7AAB4C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_277,resources_277,services_277},
    {0x802d16cc,"src/valley/7AA790.c:152-161","POKEMON_FUNC(func_802D16CC_7AAC5C)","void func_802D16CC_7AAC5C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_278,resources_278,services_278},
    {0x802d1734,"src/valley/7AA790.c:163-173","POKEMON_FUNC(func_802D1734_7AACC4)","void func_802D1734_7AACC4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_279,resources_279,services_279},
    {0x802d17d8,"src/valley/7AA790.c:175-196","POKEMON_FUNC(func_802D17D8_7AAD68)","void func_802D17D8_7AAD68(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_280,resources_280,services_280},
    {0x802d18d8,"src/valley/7AA790.c:198-213","POKEMON_FUNC(func_802D18D8_7AAE68)","void func_802D18D8_7AAE68(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_281,resources_281,services_281},
    {0x802d19dc,"src/valley/7AA790.c:215-228","POKEMON_FUNC(func_802D19DC_7AAF6C)","void func_802D19DC_7AAF6C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_282,resources_282,services_282},
    {0x802d1acc,"src/valley/7AA790.c:230-238","POKEMON_FUNC(func_802D1ACC_7AB05C)","void func_802D1ACC_7AB05C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_283,resources_283,services_283},
    {0x802d1b48,"src/valley/7AA790.c:240-249","POKEMON_FUNC(func_802D1B48_7AB0D8)","void func_802D1B48_7AB0D8(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_284,resources_284,services_284},
    {0x802d1bc8,"src/valley/7AA790.c:251-258","POKEMON_FUNC(func_802D1BC8_7AB158)","void func_802D1BC8_7AB158(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_285,resources_285,services_285},
    {0x802d1c40,"src/valley/7AA790.c:260-280","POKEMON_FUNC(func_802D1C40_7AB1D0)","void func_802D1C40_7AB1D0(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_286,resources_286,services_286},
    {0x802d1d4c,"src/valley/7AA790.c:282-295","POKEMON_FUNC(func_802D1D4C_7AB2DC)","void func_802D1D4C_7AB2DC(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_287,resources_287,services_287},
    {0x802d1e3c,"src/valley/7AA790.c:297-303","POKEMON_FUNC(func_802D1E3C_7AB3CC)","void func_802D1E3C_7AB3CC(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_288,resources_288,services_288},
    {0x802d1eb8,"src/valley/7AA790.c:305-310","POKEMON_FUNC(func_802D1EB8_7AB448)","void func_802D1EB8_7AB448(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_289,resources_289,services_289},
    {0x801e04a8,"src/valley/7AA790.c:312-313","static void nullsub()","void isle_valley_7AA790_nullsub()",SourceAbi::file_local_empty,SourceDomain::actor,calls_290,resources_290,services_290},
    {0x802d1f18,"src/valley/7AA790.c:315-326","POKEMON_FUNC(func_802D1F18_7AB4A8)","void func_802D1F18_7AB4A8(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_291,resources_291,services_291},
    {0x802d1fc0,"src/valley/7AA790.c:328-350","POKEMON_FUNC(func_802D1FC0_7AB550)","void func_802D1FC0_7AB550(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_292,resources_292,services_292},
    {0x802d2128,"src/valley/7AA790.c:352-359","POKEMON_FUNC(func_802D2128_7AB6B8)","void func_802D2128_7AB6B8(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_293,resources_293,services_293},
    {0x802d21c8,"src/valley/7AA790.c:361-367","void func_802D21C8_7AB758(s32 arg0, u16 arg1, WorldBlock* arg2, WorldBlock* arg3, ObjectSpawn* arg4)","void func_802D21C8_7AB758(int32_t arg0, uint16_t arg1, IsleActorFactoryBlock* arg2, IsleActorFactoryBlock* arg3, IsleActorFactorySpawn* arg4)",SourceAbi::factory_void5,SourceDomain::actor,calls_294,resources_294,services_294},
    {0x802d2230,"src/valley/7AB7C0.c:15-23","POKEMON_FUNC(func_802D2230_7AB7C0)","void func_802D2230_7AB7C0(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_295,resources_295,services_295},
    {0x802d22a0,"src/valley/7AB7C0.c:25-28","POKEMON_FUNC(func_802D22A0_7AB830)","void func_802D22A0_7AB830(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_296,resources_296,services_296},
    {0x802d22d4,"src/valley/7AB7C0.c:30-33","POKEMON_FUNC(func_802D22D4_7AB864)","void func_802D22D4_7AB864(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_297,resources_297,services_297},
    {0x802d230c,"src/valley/7AB7C0.c:35-38","POKEMON_FUNC(func_802D230C_7AB89C)","void func_802D230C_7AB89C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_298,resources_298,services_298},
    {0x802d2344,"src/valley/7AB7C0.c:40-43","POKEMON_FUNC(func_802D2344_7AB8D4)","void func_802D2344_7AB8D4(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_299,resources_299,services_299},
    {0x802d237c,"src/valley/7AB7C0.c:45-53","POKEMON_FUNC(func_802D237C_7AB90C)","void func_802D237C_7AB90C(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_300,resources_300,services_300},
    {0x802d2428,"src/valley/7AB7C0.c:55-81","POKEMON_FUNC(func_802D2428_7AB9B8)","void func_802D2428_7AB9B8(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_301,resources_301,services_301},
    {0x802d2540,"src/valley/7AB7C0.c:83-85","void func_802D2540_7ABAD0(s32 gObjID, u16 id, WorldBlock* roomA, WorldBlock* roomB, ObjectSpawn* spawn)","void func_802D2540_7ABAD0(int32_t gObjID, uint16_t id, IsleActorFactoryBlock* roomA, IsleActorFactoryBlock* roomB, IsleActorFactorySpawn* spawn)",SourceAbi::factory_void5,SourceDomain::actor,calls_302,resources_302,services_302},
    {0x802d2580,"src/valley/7ABB10.c:3-6","POKEMON_FUNC(spawnStaryuAtGeo)","void spawnStaryuAtGeo(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_303,resources_303,services_303},
    {0x802d25b0,"src/valley/7ABB10.c:8-11","POKEMON_FUNC(spawnStarmieAtGeo)","void spawnStarmieAtGeo(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_304,resources_304,services_304},
    {0x802d25e0,"src/valley/7ABB10.c:13-15","POKEMON_FUNC(func_802D25E0)","void func_802D25E0(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_305,resources_305,services_305},
    {0x802d2604,"src/valley/7ABB10.c:17-25","POKEMON_FUNC(func_802D2604)","void func_802D2604(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_306,resources_306,services_306},
    {0x802d2684,"src/valley/7ABB10.c:27-34","POKEMON_FUNC(func_802D2684)","void func_802D2684(IsleGameObject* obj)",SourceAbi::gobj_callback,SourceDomain::actor,calls_307,resources_307,services_307},
    {0x802d2704,"src/valley/7ABB10.c:36-38","void func_802D2704(s32 gObjID, u16 id, WorldBlock* roomA, WorldBlock* roomB, ObjectSpawn* spawn)","void func_802D2704(int32_t gObjID, uint16_t id, IsleActorFactoryBlock* roomA, IsleActorFactoryBlock* roomB, IsleActorFactorySpawn* spawn)",SourceAbi::factory_void5,SourceDomain::actor,calls_308,resources_308,services_308},
}};
constexpr ActorUtilityBindings utilities{func_802C5C20_79F1B0,func_802C5C44_79F1D4,func_802C5C68_79F1F8,func_802C5C8C_79F21C,func_802C5D18_79F2A8,func_802C5DCC_79F35C,func_802C5DD4_79F364,func_802C6300_79F890,func_802C63EC_79F97C,func_802C653C_79FACC,func_802C6544_79FAD4,func_802C6610_79FBA0,valley_stopIntro,func_802C6AD4_7A0064,func_802C6B30_7A00C0,func_802C89F4_7A1F84,func_802C8A4C_7A1FDC,isle_valley_7A3560_nullsub,func_802CC980_7A5F10,func_802CEC18_7A81A8,func_802CFE8C_7A941C,func_802D017C_7A970C,func_802D039C_7A992C,func_802D03D0_7A9960,func_802D0700_7A9C90,func_802D0754_7A9CE4,isle_valley_7AA790_nullsub};
constexpr std::array<ActorFactoryBinding,21> factories{{
    {75,0x802c957c,0x802d2cb4,0x802c82c0,ActorFactoryWrapper::ground,nullptr,func_802C957C_7A2B0C,nullptr},
    {130,0x802c97f8,0x802d2df8,0x802c95c0,ActorFactoryWrapper::ordinary,func_802C97F8_7A2D88,nullptr,nullptr},
    {74,0x802c9f90,0x802d2f2c,0x802c9958,ActorFactoryWrapper::ground,nullptr,func_802C9F90_7A3520,nullptr},
    {149,0x802ca224,0x802d302c,0x802c9fd0,ActorFactoryWrapper::ground,func_802CA224_7A37B4,nullptr,nullptr},
    {147,0x802ca688,0x802d3100,0x802ca270,ActorFactoryWrapper::ground,nullptr,func_802CA688_7A3C18,nullptr},
    {27,0x802cb02c,0x802d33f8,0x802ca768,ActorFactoryWrapper::ground,nullptr,nullptr,func_802CB02C_7A45BC},
    {7,0x802cc8a8,0x802d37c0,0x802cb080,ActorFactoryWrapper::ground,nullptr,func_802CC8A8_7A5E38,nullptr},
    {120,0x802cd704,0x802d3914,0x802cca70,ActorFactoryWrapper::ordinary,nullptr,func_802CD704_7A6C94,nullptr},
    {120,0x802cd73c,0x802d3914,0x802cca70,ActorFactoryWrapper::ordinary,nullptr,func_802CD73C_7A6CCC,nullptr},
    {121,0x802cde04,0x802d3a44,0x802cd7c0,ActorFactoryWrapper::ordinary,nullptr,func_802CDE04_7A7394,nullptr},
    {121,0x802cde3c,0x802d3a44,0x802cd7c0,ActorFactoryWrapper::ordinary,nullptr,func_802CDE3C_7A73CC,nullptr},
    {1033,0x802ce1b8,0x802d3c0c,0x802cde80,ActorFactoryWrapper::ordinary_dl4,func_802CE1B8_7A7748,nullptr,nullptr},
    {28,0x802cea58,0x802d3f38,0x802ce200,ActorFactoryWrapper::ground,nullptr,func_802CEA58_7A7FE8,nullptr},
    {56,0x802cf808,0x802d4124,0x802ceaa0,ActorFactoryWrapper::ground,nullptr,func_802CF808_7A8D98,nullptr},
    {1034,0x802cfa00,0x802d41e8,0x802cf840,ActorFactoryWrapper::ordinary_dl4,func_802CFA00_7A8F90,nullptr,nullptr},
    {1032,0x802d0b3c,0x802ec68c,0x802d0320,ActorFactoryWrapper::ordinary_dl4,func_802D0B3C_7AA0CC,nullptr,nullptr},
    {1001,0x802d1154,0x802ec750,0x802d0b80,ActorFactoryWrapper::ordinary_dl4,nullptr,func_802D1154_7AA6E4,nullptr},
    {1035,0x802d11b0,0x802ec7b8,0x802d1190,ActorFactoryWrapper::ordinary,nullptr,nullptr,func_802D11B0},
    {129,0x802d21c8,0x802ec934,0x802d1200,ActorFactoryWrapper::behavior_not3_ground_else_ordinary,nullptr,nullptr,func_802D21C8_7AB758},
    {118,0x802d2540,0x802eca30,0x802d2230,ActorFactoryWrapper::ground,nullptr,nullptr,func_802D2540_7ABAD0},
    {1036,0x802d2704,0x802ecb2c,0x802d25e0,ActorFactoryWrapper::ordinary,nullptr,nullptr,func_802D2704},
}};
constexpr std::array<ActorChildFactoryDependency,5> children{{
    {0x802c5e68,82,84,0x802d2984,118,118,0x802d2540,0x802eca30,0x80363eb4,0x80364280,0x3ff},
    {0x802c5f70,118,120,0x802d2994,147,147,0x802ca688,0x802d3100,0x80363eb4,0x80364280,0x3ff},
    {0x802c606c,151,153,0x802d29a4,129,129,0x802d21c8,0x802ec934,0x80363eb4,0x80364280,0x3ff},
    {0x802d2580,0,0,0x802ecae0,120,120,0x802cd73c,0x802d3914,0x80363dbc,0x80364280,0x0},
    {0x802d25b0,0,0,0x802ecaf0,121,121,0x802cde3c,0x802d3a44,0x80363dbc,0x80364280,0x0},
}};
constexpr std::array<ActorCountedOption,4> counted_rows{{
    {10,0x802c5e68},
    {10,0x802c5f70},
    {10,0x802c606c},
    {70,0},
}};
constexpr std::array<ActorCountedOptions,1> counted{{
    {0x802d29b4,counted_rows},
}};
constexpr std::array<ActorAnimationBinding,3> animation{{
    {0x802d0420,func_802D0420_7A99B0},
    {0x802d04bc,func_802D04BC_7A9A4C},
    {0x802d0cd8,func_802D0CD8_7AA268},
}};
constexpr std::array<ActorLifecycleAnimationBinding,1> lifecycle_animation{{
    {0x802c6630,func_802C6630_79FBC0},
}};
constexpr std::array<ActorItemCollisionBinding,1> item_collision{{
    {0x802c6168,func_802C6168_79F6F8},
}};
constexpr std::array<ActorBlockChangeBinding,1> block_change{{
    {0x802cc8e0,func_802CC8E0_7A5E70},
}};
constexpr std::array<ActorNativeService,102> required_services{{
    ActorNativeService::reference_assign,
    ActorNativeService::model,
    ActorNativeService::player_model,
    ActorNativeService::node_path,
    ActorNativeService::vector,
    ActorNativeService::vector_read,
    ActorNativeService::vector_store,
    ActorNativeService::item,
    ActorNativeService::item_direction,
    ActorNativeService::current_block,
    ActorNativeService::graph,
    ActorNativeService::graph_copy,
    ActorNativeService::weighted,
    ActorNativeService::weighted_copy,
    ActorNativeService::forbidden_ground,
    ActorNativeService::add_one,
    ActorNativeService::add_geo,
    ActorNativeService::ground,
    ActorNativeService::node_world,
    ActorNativeService::level_progress,
    ActorNativeService::path_position,
    ActorNativeService::path_start,
    ActorNativeService::animation_callback,
    ActorNativeService::model_animation_speed,
    ActorNativeService::camera,
    ActorNativeService::camera_speed,
    ActorNativeService::camera_read,
    ActorNativeService::camera_component,
    ActorNativeService::camera_object,
    ActorNativeService::camera_animation,
    ActorNativeService::camera_animation_process,
    ActorNativeService::pause_processes,
    ActorNativeService::resume_processes,
    ActorNativeService::player_model_init,
    ActorNativeService::player_model_animation,
    ActorNativeService::player_model_change,
    ActorNativeService::player_model_speed,
    ActorNativeService::cutscene_start,
    ActorNativeService::sound,
    ActorNativeService::sound_params,
    ActorNativeService::sound_active,
    ActorNativeService::sound_stop,
    ActorNativeService::sound_stop_all,
    ActorNativeService::sound_volume,
    ActorNativeService::sound_global_volume,
    ActorNativeService::sound_volume_smooth,
    ActorNativeService::bgm_play,
    ActorNativeService::bgm_stop,
    ActorNativeService::bgm_volume,
    ActorNativeService::bgm_volume_smooth,
    ActorNativeService::background_music,
    ActorNativeService::check_player_flag,
    ActorNativeService::set_player_flag,
    ActorNativeService::remove_flying_items,
    ActorNativeService::reset_camera,
    ActorNativeService::sky_speed,
    ActorNativeService::message_reset,
    ActorNativeService::message_interactive,
    ActorNativeService::message_noninteractive,
    ActorNativeService::message_printed,
    ActorNativeService::message_show,
    ActorNativeService::message_handler,
    ActorNativeService::dialogue,
    ActorNativeService::sprite,
    ActorNativeService::sprite_list,
    ActorNativeService::sprite_add,
    ActorNativeService::sprite_move,
    ActorNativeService::sprite_x2_move,
    ActorNativeService::sprite_scale,
    ActorNativeService::sprite_color,
    ActorNativeService::sprite_link,
    ActorNativeService::object_add,
    ActorNativeService::object_default_add,
    ActorNativeService::input_pressed,
    ActorNativeService::player_object,
    ActorNativeService::player_name,
    ActorNativeService::effect_create,
    ActorNativeService::effect_attach,
    ActorNativeService::application_notice,
    ActorNativeService::transition_begin,
    ActorNativeService::transition_color,
    ActorNativeService::transition_busy,
    ActorNativeService::audio_cutscene_reset,
    ActorNativeService::environment_sound,
    ActorNativeService::kick_item,
    ActorNativeService::jump_bounce,
    ActorNativeService::common_child1,
    ActorNativeService::common_child2,
    ActorNativeService::block_change,
    ActorNativeService::block_change_ground,
    ActorNativeService::common_remove,
    ActorNativeService::misc_object_get,
    ActorNativeService::misc_object_set,
    ActorNativeService::model_time_left,
    ActorNativeService::rng_range,
    ActorNativeService::rng_float,
    ActorNativeService::player_cutscene_process,
    ActorNativeService::flute,
    ActorNativeService::command,
    ActorNativeService::command_link,
    ActorNativeService::vector_distance,
    ActorNativeService::dialogue_format,
}};
} // namespace
std::span<const actor::BehaviorCallbackBinding> actor_callbacks() noexcept { return callbacks; }
std::span<const std::uint32_t> actor_graphs() noexcept { return graphs; }
std::span<const std::uint32_t> actor_weighted_states() noexcept { return weighted; }
std::span<const std::uint32_t> actor_roots() noexcept { return roots; }
std::span<const ActorFactoryBinding> actor_factories() noexcept { return factories; }
std::span<const ActorChildFactoryDependency> actor_child_factories() noexcept { return children; }
std::span<const ActorProviderDependency> actor_dependencies() noexcept { return dependencies; }
std::span<const ActorCountedOptions> actor_counted_options() noexcept { return counted; }
std::span<const ActorAnimationBinding> actor_animation_callbacks() noexcept { return animation; }
std::span<const ActorLifecycleAnimationBinding> lifecycle_animation_callbacks() noexcept { return lifecycle_animation; }
std::span<const ActorItemCollisionBinding> actor_item_collision_callbacks() noexcept { return item_collision; }
std::span<const ActorBlockChangeBinding> actor_block_change_callbacks() noexcept { return block_change; }
std::span<const ActorNativeService> actor_required_services() noexcept { return required_services; }
const ActorUtilityBindings& actor_utilities() noexcept { return utilities; }
} // namespace isle::valley

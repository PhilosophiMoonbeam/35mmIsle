#ifndef ISLE_PLAYER_PHOTO_SOURCE_H
#define ISLE_PLAYER_PHOTO_SOURCE_H
#include "player_source.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Semantic values only. Wire records are encoded explicitly, never overlaid. */
typedef struct IslePlayerPhotoTicket {
    uint16_t context;
    uint64_t generation, submission;
#ifdef __cplusplus
    bool operator==(const IslePlayerPhotoTicket&) const = default;
#endif
} IslePlayerPhotoTicket;
/* Transient published Detector value, never a retained focus authority. */
typedef struct IslePlayerPhotoFocus {
    int32_t present;
    IsleGameObject* object;
    int32_t pokemon_id;
    uint16_t flags, context;
} IslePlayerPhotoFocus;
typedef struct IslePlayerPokemonPhoto {
    int32_t pokemonID;
    uint8_t unk_00_13, specialID, poseID;
    float animationTime;
    IsleGameVec3 position;
    float yaw;
} IslePlayerPokemonPhoto;
typedef struct IslePlayerItemPhoto {
    int8_t itemType;
    uint8_t animationTime;
    IsleGameVec3 pos;
} IslePlayerItemPhoto;
typedef struct IslePlayerEffectPhoto {
    int8_t textureID;
    uint8_t dataID, bankID, unk_03;
    int16_t posX, posY, posZ;
    uint16_t size;
    uint8_t primColor[4];
} IslePlayerEffectPhoto;
typedef struct IslePlayerPhotoData {
    int8_t levelID;
    uint8_t unk_00_24, unk_00_16;
    union { int32_t s32; float f32; } unk_04;
    IsleGameVec3 unk_08, unk_14;
    IslePlayerPokemonPhoto pokemons[12];
    IslePlayerItemPhoto items[6];
    IslePlayerEffectPhoto effects[32];
} IslePlayerPhotoData;
typedef struct IslePlayerPhotoContext {
    IslePlayerPhotoData main;
    IsleGameObject* pokemonInFocus;
    IsleGameObject* pokemonObjects[12];
} IslePlayerPhotoContext;
typedef struct IslePlayerPhotoParticle {
    struct IslePlayerPhotoParticle* next;
    IsleGameVec3 pos;
    float size;
    uint16_t flags, bankID;
    uint8_t textureID, dataID;
    struct { uint8_t r,g,b,a; } primColor;
} IslePlayerPhotoParticle;
typedef struct IslePlayerPhotoItemState {
    IsleGameVec3 position;
    float model_time, material_time, scale;
    int32_t family, hidden;
} IslePlayerPhotoItemState;
typedef struct IslePlayerPhotoPokemonState {
    int32_t id, tangible, hidden, culled, animation_index, default_pose;
    uint16_t specialPoseID, poseID;
    float animationTime, yaw;
    IsleGameVec3 position;
} IslePlayerPhotoPokemonState;
/* Application owned: this roll is not a member of the course Player owner. */
struct IslePlayerPhotoSource {
    IslePlayerPhotoData D_800B0598[60];
    IsleGameResult status;
    IslePlayerPhotoContext PokemonDetector_Photo[2];
    int32_t gPhotoCount, D_800AE27C, D_800AE280, D_800BDF20[3];
    int32_t gPokemonIdInFocus, PokemonDetector_TakenPhotoIndex;
    void* owner;
    float D_803B1518_551928[4][4];
    int32_t qsortItemSize;
    int32_t (*qsortCompareFunc)(const void*,const void*);
    const int8_t* D_800AE4E4;
    const char* const* D_800AE284;
    void* save;
};
int32_t isle_player_photo_func_8009BB4C(IslePlayerPhotoSource*, int32_t);
const char* isle_player_photo_getPokemonName(IslePlayerPhotoSource*, int32_t);
int32_t isle_player_photo_checkPlayerFlag(IslePlayerPhotoSource*, int32_t);
int32_t isle_player_photo_func_800BF3D4_5C274(IslePlayerPhotoSource*, int32_t);
int32_t isle_player_photo_report_photo_present(IslePlayerPhotoSource*, int32_t);
int32_t isle_player_photo_func_8009BBF4(IslePlayerPhotoSource*);
int32_t isle_player_photo_func_8009BC00(IslePlayerPhotoSource*);
int32_t isle_player_photo_func_8009BC0C(IslePlayerPhotoSource*, int32_t);
int32_t isle_player_photo_func_8009BC68(IslePlayerPhotoSource*);
IslePlayerPhotoData* isle_player_photo_func_8009BC74(IslePlayerPhotoSource*);
IslePlayerPhotoData* isle_player_photo_func_8009BC80(IslePlayerPhotoSource*, int32_t);
int32_t isle_player_photo_func_8009BCC4(IslePlayerPhotoSource*, IslePlayerPhotoContext*);
int32_t isle_player_photo_makePhoto(IslePlayerPhotoSource*, IsleGameObject*);
void isle_player_photo_func_8009CDC0(IslePlayerPhotoSource*, uint8_t*, int32_t);
int32_t isle_player_photo_func_8009CDE4(IslePlayerPhotoSource*, uint8_t*);
void isle_player_photo_func_8009CE00(IslePlayerPhotoSource*);
/* Parent render boundary: explicit status, no wait/fault across C++ owners. */
IsleGameResult isle_player_photo_func_8009C8E4(IslePlayerPhotoSource*, uint16_t context,
    uint64_t generation);
IsleGameResult isle_player_photo_publish_completed(IslePlayerPhotoSource*);
IsleGameResult isle_player_photo_require_taken_context(IslePlayerPhotoSource*);
/* Returning owner-thread leaves. Failed borrows clear the output ticket.
 * Release remains available for real cleanup after a sticky source failure. */
IsleGameResult isle_player_photo_published_focus(IslePlayerPhotoSource*, IslePlayerPhotoFocus*);
IsleGameResult isle_player_photo_clear_published_presence(IslePlayerPhotoSource*);
IsleGameResult isle_player_photo_borrow_taken(IslePlayerPhotoSource*, IslePlayerPhotoTicket*);
IsleGameResult isle_player_photo_release_taken(IslePlayerPhotoSource*, const IslePlayerPhotoTicket*);
/* Pure C: borrow, original makePhoto, unconditional release, then return.
 * Caller checks Photo.status before any source HUD/cooldown continuation. */
int32_t isle_player_photo_take_photo(IslePlayerPhotoSource*, IsleGameObject*);
void isle_player_photo_PokemonDetector_CopyInfo(IsleGameObject*);
int32_t isle_player_photo_func_8009C304(IslePlayerPhotoSource*, IslePlayerItemPhoto*,
    const IslePlayerPhotoItemState*);
int32_t isle_player_photo_func_8009BF48(IslePlayerPhotoSource*, IslePlayerPokemonPhoto*,
    const IslePlayerPhotoPokemonState*);
IsleGameResult isle_player_photo_capture_context(IslePlayerPhotoSource*, IslePlayerPhotoContext*);
IsleGameResult isle_player_photo_capture_camera(IslePlayerPhotoSource*, IslePlayerPhotoContext*);
IsleGameResult isle_player_photo_capture_pokemon(IslePlayerPhotoSource*, IslePlayerPhotoContext*);
IsleGameResult isle_player_photo_capture_items(IslePlayerPhotoSource*, IslePlayerPhotoContext*);
IsleGameResult isle_player_photo_capture_fx(IslePlayerPhotoSource*, IslePlayerPhotoContext*);
void isle_player_photo_capture_effects(IslePlayerPhotoSource*, IslePlayerPhotoContext*,
    IslePlayerPhotoParticle* particles);
#ifdef __cplusplus
}
#endif
#endif

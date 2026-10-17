#ifndef ISLE_ENGINE_INTERNAL_H
#define ISLE_ENGINE_INTERNAL_H

#include "35mmisle/engine.h"
#include "35mmisle/continuation.h"
#include "engine_math.h"
#include <stdbool.h>
#include <setjmp.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t s32;
typedef struct GObj GObj;
typedef struct GObjProcess GObjProcess;
typedef struct SObj SObj;
typedef struct OMCamera OMCamera;
typedef struct OMMtx OMMtx;
typedef IsleSprite Sprite;
typedef void (*GObjFunc)(GObj*);

/* Independently declared native interfaces for the selected OM definitions.
 * These are deliberately not N64 layouts and contain no embedded SDK thread,
 * stack-node ABI, executable address or serialized pointer field.
 */
struct GObjProcess {
    GObjProcess* next;
    GObjProcess* prev;
    GObjProcess* nextInPriorityList;
    GObjProcess* prevInPriorityList;
    s32 priority;
    u8 kind;
    u8 paused;
    GObj* object;
    union { GObjFunc cb; } unk_1C;
    IsleContinuation* continuation;
    IsleGameCallback entry;
    IsleGameSourceCallback function; /* Actual GAME entry, never a dispatch adapter. */
    void* user;
    IsleGame* game;
    bool alive;
    SObj* fade_sprite;
    u16 fade_updates;
    unsigned callbackDepth; /* Saved exclusion context across real continuation waits. */
    bool retirementObserver;
    bool factoryTargetHeld; /* Unpublished process owns only an explicit old identity. */
};

enum {
    ISLE_SOURCE_PAYLOAD_NONE = 0,
    ISLE_SOURCE_PAYLOAD_ACTOR = 1,
    ISLE_SOURCE_PAYLOAD_ITEM = 2
};

struct GObj {
    u32 id;
    GObj* next;
    GObj* prev;
    u8 link;
    u8 type;
    u32 priority;
    GObjFunc fnUpdate;
    GObjProcess* processListHead;
    GObjProcess* processListTail;
    union { SObj* sobj; OMCamera* cam; IsleGameModel* model; void* any; } data;
    u32 flags;
    IsleGameCallback update;
    void* updateUser; /* Native callback context, never GAME actor data. */
    void* userData;   /* GAME entity payload; allocation/reuse clears it. */
    IsleGameCallback retiring;
    void* preRetireUser;
    IsleGameCallback retired;
    void* retireUser;
    u32 identityHolds; /* Physical address only; never payload/process liveness. */
    IsleGame* game;
    bool alive;
    bool retirementStarted;
    u8 sourcePayloadKind; /* Native discriminator before interpreting entity userdata. */
    struct IsleSourceFactoryResources* factoryResources;
    GObj* nextFactory;
    size_t factoryHolds; /* Private admission pins, never public identity credits. */
};

struct SObj {
    SObj* nextFree;
    GObj* obj;
    SObj* next;
    SObj* prev;
    Sprite sprite;
    s32 unk_54;
    SObj* unk_58;
    SObj* unk_5C;
    IsleGame* game;
    bool alive;
};

/* Native ownership seam follows GAME omGetCamera/omGetMtx and their free lists.
 * No unadmitted animation, render-list or SDK viewport ownership is present.
 */
struct OMMtx {
    OMMtx* nextFree;
    IsleGameCameraMatrix result;
    IsleGameCameraReflection reflection;
    bool cached;
};
struct OMCamera {
    OMCamera* nextFree;
    GObj* obj;
    IsleGame* game;
    IsleGameCameraParameters parameters;
    OMMtx* matrices[2];
    size_t numMatrices;
    u16 normalization;
    float roll;
    bool alive;
};

/* Original native ownership, not an imported console DObj layout. A single
 * arena block contains the header, capacity nodes and capacity dirty bytes.
 */
struct IsleGameModel {
    IsleGameModel* nextFree;
    GObj* obj;
    IsleGame* game;
    size_t count, capacity;
    bool alive;
    IsleGameModelPlacement placement;
    IsleGameModelNode nodes[];
};

struct IsleGame {
    isle_arena* arena;
    IsleContinuationDomain* domain;
    GObj* free_objects;
    GObjProcess* free_processes;
    SObj* free_sprites;
    OMCamera* free_cameras;
    OMMtx* free_matrices;
    IsleGameModel* free_models;
    size_t held_identities;
    GObj* factory_objects; /* Stable caller-owned tickets, rooted by actual raw objects. */
    unsigned callback_depth;
    bool retirement_observer; /* Also covers read-only successful-enqueue observers. */
    /* Only the owned synchronous command drain may arm this returning cleanup. */
    void (*command_fault_abort)(void*);
    void* command_fault_context;
    IsleGameCounts counts;
    IsleGameResult fault;
    char error[512];
    jmp_buf fault_frame;
    bool ticking;
    bool alive;
};

#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))
#define GOBJ_FLAG_40 UINT32_C(0x40)

extern GObj* omGObjListHead[32];
extern GObj* omGObjListTail[32];
extern GObjProcess* omGProcessList[12];
extern GObj* omCurrentObject;
extern GObjProcess* omCurrentProcess;
extern s32 omD_8004AC54;

/* Definitions below are compiled from exact, allowlisted game source in a
 * build-private translation unit. No SDK implementation is an input.
 */
void omLinkProcess(GObjProcess* process);
void omUnlinkProcessFromPriorityList(GObjProcess* process);
void omUnlinkProcess(GObjProcess* process);
void omLinkGObjAfter(GObj* object, GObj* previous);
void omLinkGObjAfterSamePriority(GObj* object);
void omLinkGObjBeforeSamePriority(GObj* object);
void omUnlinkGObj(GObj* object);
GObj* omUpdateGObj(GObj* object);
GObjProcess* omRunProcess(GObjProcess* process);
void omUpdateAll(void);
SObj* omGObjAddSprite(GObj* object, const Sprite* sprite);
void omGObjRemoveSprite(SObj* sprite);
void func_800E1958_AA0708(SObj* sprite, u16 updates);

/* Original native replacements consumed by the selected game definitions. */
void isle_native_execution_fault(IsleGame* game, const char* text);
void isle_native_resume_process(GObjProcess* process);
void omEndProcess(GObjProcess* process);
void omDeleteGObj(GObj* object);
SObj* omGetSObj(void);
void omFreeSObj(SObj* sprite);
void ohWait(s32 updates);

#endif

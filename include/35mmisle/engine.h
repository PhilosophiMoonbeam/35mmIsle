#ifndef ISLE_ENGINE_H
#define ISLE_ENGINE_H

#include "35mmisle/native.h"
#include "35mmisle/sprite.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct IsleGame IsleGame;
typedef struct GObj IsleGameObject;
typedef struct GObjProcess IsleGameProcess;
typedef struct SObj IsleGameSprite;
typedef struct OMCamera IsleGameCamera;
typedef struct IsleGameModel IsleGameModel;
typedef void (*IsleGameCallback)(IsleGameObject* object, void* user);
typedef void (*IsleGameSourceCallback)(IsleGameObject* object);

typedef enum IsleGameResult {
    ISLE_GAME_OK = 0,
    ISLE_GAME_INVALID_ARGUMENT,
    ISLE_GAME_WRONG_OWNER,
    ISLE_GAME_INVALID_STATE,
    ISLE_GAME_EXHAUSTED,
    ISLE_GAME_EXECUTION_FAILURE
} IsleGameResult;

typedef enum IsleGameProcessKind {
    ISLE_GAME_STACKFUL = 0,
    ISLE_GAME_SYNCHRONOUS = 1
} IsleGameProcessKind;

typedef struct IsleGameCounts {
    size_t objects;
    size_t processes;
    size_t sprites;
    size_t cameras;
    size_t matrices;
    size_t models;
    size_t model_nodes;
} IsleGameCounts;

/* One active authoritative OM world, one gameplay timeline. Caller-owned arena
 * metadata/storage must remain valid and cannot be reset until destroy joins
 * all processes. Native sizeof/alignment, not N64 pool strides, determine
 * allocations. Empty free lists grow from this bounded arena. The C++ process
 * domain owns actual continuation stacks separately. Creation publishes only
 * on success; failed allocations never publish partially linked objects.
 */
IsleGameResult isle_game_create(isle_arena* arena, IsleGame** output);
IsleGameResult isle_game_destroy(IsleGame* game);
IsleGameResult isle_game_tick(IsleGame* game);
IsleGameResult isle_game_counts(IsleGame* game, IsleGameCounts* output);
const char* isle_game_error(const IsleGame* game);

/* Updates traverse links 0..31, descending object priority with insertion after
 * equal priorities, then processes 11..0 in the authoritative list order.
 * Callbacks may create/end processes and delete objects synchronously. User
 * storage must outlive every callback using it. No implicit ownership transfer.
 * Callbacks must not throw C++ exceptions.
 * Stackful callbacks use the C continuation fence: intervening callback frames
 * must not retain owning C++ automatic objects across wait, self-end or failure.
 * C-owned resources need explicit object/scene ownership, not stack cleanup.
 */
IsleGameResult isle_game_add_object(IsleGame* game, uint32_t id, uint8_t link,
                                   uint32_t priority, IsleGameCallback update,
                                   void* user, IsleGameObject** output);
IsleGameResult isle_game_delete_object(IsleGame* game, IsleGameObject* object);
IsleGameResult isle_game_pause_updates(IsleGame* game, IsleGameObject* object, int paused);
IsleGameResult isle_game_object_id(IsleGame* game, IsleGameObject* object, uint32_t* output);
/* Native ownership metadata, never a console record layout. Retirement callback
 * runs once after every process is joined and payload is removed, before node
 * reuse; it must not throw, wait or mutate OM. Its userdata survives that fence. */
typedef struct IsleGameObjectMetadata {
    uint32_t id, priority, flags;
    uint8_t update_link;
} IsleGameObjectMetadata;
IsleGameResult isle_game_object_metadata(IsleGame*, IsleGameObject*, IsleGameObjectMetadata*);
/* Immediate source flag assignment (e.g. setWorldBlocksVisibility), not queued
 * command delivery. Caller preserves unrelated flag bits using metadata(). */
IsleGameResult isle_game_object_set_flags(IsleGame*, IsleGameObject*, uint32_t flags);
IsleGameResult isle_game_object_set_retire_callback(IsleGame*, IsleGameObject*, IsleGameCallback, void*);

/* One source FIFO per object, arena-backed recycled nodes. Command userdata and
 * queues must survive actual deferred/joined object retirement. Handler receives
 * only custom commands; -1 deletes, -2/-3 pause/resume all object processes,
 * -4/-5 hide/show. NULL custom handler drains unknown commands. */
typedef struct IsleGameCommand IsleGameCommand;
typedef struct IsleGameCommandPool { isle_arena* arena; IsleGameCommand* free; } IsleGameCommandPool;
typedef struct IsleGameCommandQueue {
    IsleGameCommand* head;
    IsleGameCommand* tail;
    size_t count;
    uint8_t processing, cancelled;
} IsleGameCommandQueue;
typedef void (*IsleGameCommandHandler)(IsleGameObject*, int32_t command, IsleGameObject* source, void* user);
IsleGameResult isle_game_command_send(IsleGameCommandPool*, IsleGameCommandQueue*, int32_t, IsleGameObject*);
/* delete_requested stops traversal; caller performs whole-object deletion. */
IsleGameResult isle_game_commands(IsleGame*, IsleGameObject*, IsleGameCommandPool*,
    IsleGameCommandQueue*, IsleGameCommandHandler, void*, int* delete_requested);
void isle_game_commands_release(IsleGameCommandPool*, IsleGameCommandQueue*);
IsleGameResult isle_game_add_process(IsleGame* game, IsleGameObject* object,
                                    IsleGameCallback entry, void* user,
                                    IsleGameProcessKind kind, uint32_t priority,
                                    IsleGameProcess** output);
/* GAME one-argument entries retain their actual callable typed identity,
 * separately from native dispatch/continuation adapters. They read GAME actor
 * userData through their object, independently of native callback contexts.
 * Both entry forms share OM priorities, kind semantics and joined retirement.
 * No ownership is transferred.
 */
IsleGameResult isle_game_add_source_process(IsleGame* game, IsleGameObject* object,
    IsleGameSourceCallback function, IsleGameProcessKind kind, uint32_t priority,
    IsleGameProcess** output);
/* Native two-argument processes report NULL. Output clears on every error. */
IsleGameResult isle_game_process_source_function(IsleGame* game, IsleGameProcess* process,
    IsleGameSourceCallback* output);
/* Exact typed identity comparison in object insertion order, as GAME
 * Items_EndProcessByFunction (items.c:69-85). NULL object selects the current
 * object. Non-current matches are joined immediately. A matching stackful
 * current process self-ends without returning or visiting later matches;
 * synchronous self-end returns and traversal continues, like the source helper.
 */
IsleGameResult isle_game_end_source_processes(IsleGame* game, IsleGameObject* object,
    IsleGameSourceCallback function);
IsleGameResult isle_game_end_process(IsleGame* game, IsleGameProcess* process);
IsleGameResult isle_game_pause_process(IsleGame* game, IsleGameProcess* process, int paused);
/* Active stackful callback only. Paused processes consume no waits. */
IsleGameResult isle_game_wait(IsleGame* game, uint32_t updates);

/* NULL object/process selects the current callback's object/process where one
 * exists. Self-object deletion is deferred until callback yield/return. A
 * synchronous self-end returns to its callback; a stackful self-end does not.
 * Every non-current process is joined before its node can be recycled.
 * Destroyed raw handles cannot be used, including after arena slot reuse.
 */

/* Sprite scalar state is copied into the game object; immutable bitmap/palette
 * views are borrowed. Their resident owner must outlive the object AND renderer
 * completion. NULL initial creates a zeroed sprite. Sprite handles expire when
 * removed or their object is deleted. Quiesce callbacks before removing a
 * sprite they directly retain. Native RSP display-list caches are not imported.
 */
IsleGameResult isle_game_add_sprite(IsleGame* game, IsleGameObject* object,
                                   const IsleSprite* initial, IsleGameSprite** output);
IsleGameResult isle_game_remove_sprite(IsleGame* game, IsleGameSprite* sprite);
IsleGameResult isle_game_sprite_view(IsleGame* game, IsleGameSprite* sprite,
                                    IsleSprite** output);
/* Runs the selected authoritative C fade helper as a genuine stackful process;
 * this is not full intro entry, persistence initialization, audio or scene boot.
 */
IsleGameResult isle_game_add_intro_fade(IsleGame* game, IsleGameObject* object,
                                       IsleGameSprite* sprite, uint16_t updates,
                                       uint32_t priority, IsleGameProcess** output);

typedef struct IsleGamePerspective {
    float fovy, aspect, near_plane, far_plane, scale;
} IsleGamePerspective;
typedef struct IsleGameLookAt {
    float eye[3], at[3], up[3];
} IsleGameLookAt;
/* Public viewport ABI: quarter-pixel X/Y, half-depth Z; fourth lanes reserved.
 * Initial viewport is 320x240, depth 0..1023, as in GAME initialization.
 */
typedef struct IsleGameViewport {
    int16_t scale[4], translate[4];
} IsleGameViewport;
typedef struct IsleGameCameraParameters {
    IsleGamePerspective perspective;
    IsleGameLookAt look_at;
    IsleGameViewport viewport;
} IsleGameCameraParameters;
typedef enum IsleGameCameraMatrixKind {
    ISLE_GAME_CAMERA_PERSPECTIVE_FAST = 3,
    ISLE_GAME_CAMERA_LOOK_AT = 6,
    ISLE_GAME_CAMERA_LOOK_AT_ROLL = 8,
    ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL = 14
} IsleGameCameraMatrixKind;
/* Native numeric words, NOT a host-endian serialized SDK structure. First
 * eight words pair signed integral halves, last eight pair fractional halves.
 * Copy numeric words directly into aligned word-swapped canonical RDRAM.
 * floating preserves the actual GAME f32 result (row-vector convention).
 */
typedef struct IsleGameFixedMatrix { uint32_t m[4][4]; } IsleGameFixedMatrix;
typedef struct IsleGameCameraMatrix {
    float floating[4][4];
    IsleGameFixedMatrix fixed;
    uint64_t revision;
    IsleGameCameraMatrixKind kind;
} IsleGameCameraMatrix;

/* Numeric RCP LookAt light values, not a serialized SDK structure. Axis 0 is
 * screen-right, axis 1 screen-up. Directions are signed 1.7 (positive unity
 * saturates to 127, negative unity is -128). Colors are actual GAME outputs.
 */
typedef struct IsleGameCameraReflection {
    int8_t direction[2][3];
    uint8_t color[2][3], color_copy[2][3];
    uint64_t revision;
} IsleGameCameraReflection;

/* Nonanimated camera payload, type 3, attached only to an empty object. Like
 * GAME omGObjSetCamera it initially has no matrices. Add up to two of any
 * supported kind, as GAME omCameraAddMtx permits; defaults are GAME parameters.
 * Roll defaults to zero finite radians and affects kinds 8 and 14, not kind 6.
 * Kind 8 uses GAME's fixed Y-up axis; kind 6/14 use look_at.up. A kind-8 pose
 * must not look along Y. Up-only edits do not dirty kind 8.
 * No renderer, command, animation or callback interface is implied.
 * All handles/borrows expire on removal or object deletion, including reuse.
 * Borrow addresses stay stable until removal; contents change only on compute
 * after the relevant parameters change. Viewport changes do not dirty matrices.
 * Projection normalization is computed with the projection. No update allocates.
 * Nonfinite/degenerate inputs are rejected; an otherwise valid pose/projection
 * outside signed 16.16 range is rejected by compute, never wrapped or clamped.
 * Pointer outputs are cleared on every error, including wrong-owner errors.
 */
IsleGameResult isle_game_attach_camera(IsleGame* game, IsleGameObject* object,
                                      IsleGameCamera** output);
IsleGameResult isle_game_camera_add_matrix(IsleGame* game, IsleGameCamera* camera,
                                          IsleGameCameraMatrixKind kind);
IsleGameResult isle_game_camera_set_perspective(IsleGame* game, IsleGameCamera* camera,
                                               const IsleGamePerspective* parameters);
IsleGameResult isle_game_camera_set_look_at(IsleGame* game, IsleGameCamera* camera,
                                           const IsleGameLookAt* parameters);
IsleGameResult isle_game_camera_set_roll(IsleGame* game, IsleGameCamera* camera, float radians);
IsleGameResult isle_game_camera_roll(IsleGame* game, IsleGameCamera* camera, float* output);
IsleGameResult isle_game_camera_set_viewport(IsleGame* game, IsleGameCamera* camera,
                                            const IsleGameViewport* parameters);
IsleGameResult isle_game_camera_parameters(IsleGame* game, IsleGameCamera* camera,
                                          const IsleGameCameraParameters** output);
/* Compute every dirty matrix/reflection as one transaction. Failure leaves all
 * matrix/reflection payloads, revisions and normalization unpublished; success
 * publishes only dirty entries, with unchanged stable borrow addresses.
 */
IsleGameResult isle_game_camera_compute_all(IsleGame* game, IsleGameCamera* camera);
IsleGameResult isle_game_camera_matrix(IsleGame* game, IsleGameCamera* camera,
                                      size_t index, const IsleGameCameraMatrix** output);
/* Same index/cache/revision and lifetime as matrix(); kinds 3/6/8 return
 * INVALID_STATE, never fabricated reflection directions. */
IsleGameResult isle_game_camera_reflection(IsleGame* game, IsleGameCamera* camera,
                                          size_t index, const IsleGameCameraReflection** output);
IsleGameResult isle_game_camera_matrix_count(IsleGame* game, IsleGameCamera* camera,
                                            size_t* output);
IsleGameResult isle_game_camera_perspective_normalization(IsleGame* game,
                                                         IsleGameCamera* camera,
                                                         uint16_t* output);
IsleGameResult isle_game_remove_camera(IsleGame* game, IsleGameCamera* camera);

typedef struct IsleGameVec3 { float x, y, z; } IsleGameVec3;
typedef struct IsleGameModelTransform {
    IsleGameVec3 position, rotation, scale;
} IsleGameModelTransform;
/* Independent actor placement, prepended to the ordinary source root matrices.
 * Disabled consumes no transform. RT consumes only position/rotation and stores
 * unit scale; RTS also consumes scale. Neither changes any node-local cache.
 */
typedef enum IsleGameModelPlacementMode {
    ISLE_GAME_MODEL_PLACEMENT_DISABLED = 0,
    ISLE_GAME_MODEL_PLACEMENT_RT,
    ISLE_GAME_MODEL_PLACEMENT_RTS
} IsleGameModelPlacementMode;
typedef struct IsleGameModelPlacement {
    IsleGameModelPlacementMode mode;
    IsleGameModelTransform transform;
    IsleGameFixedMatrix fixed;
    uint64_t revision;
} IsleGameModelPlacement;
/* Native choices, not serialized console matrix-kind numbers. Selection is
 * immutable for a node's lifetime; both use the same stable revision cache.
 */
typedef enum IsleGameModelMatrixKind {
    ISLE_GAME_MODEL_ROTATE_RPY_TRANSLATE_SCALE = 0,
    ISLE_GAME_MODEL_TRANSLATE = 1
} IsleGameModelMatrixKind;
typedef struct IsleGameModelNodeInit {
    uint32_t parent; /* UINT32_MAX root; otherwise an earlier node index. */
    IsleGameModelTransform transform;
    IsleGameModelMatrixKind matrix_kind;
} IsleGameModelNodeInit;
typedef struct IsleGameModelNode {
    IsleGameModelTransform transform;
    uint32_t parent, first_child, next_sibling; /* UINT32_MAX absent. */
    uint32_t flags;
    IsleGameFixedMatrix fixed; /* Direct selected GAME integer computation. */
    uint64_t revision;
    IsleGameModelMatrixKind matrix_kind;
} IsleGameModelNode;

/* Native type-1 payload: exactly one root, parent-before-child node order.
 * Actual-size arena storage is reused after removal. No mutation/compute
 * allocates. Local matrices compute lazily, only after transform changes;
 * flags do not dirty matrices, and parent changes do not dirty child locals.
 * Node borrows stay stable until model removal/object deletion. Handles expire
 * at removal, including slot reuse. Pointer/count outputs clear on errors.
 * Finite angle, scale*256 and translation*65536 must fit signed int32 before
 * GAME casts; MIPS low-word products and arithmetic shifts are defined explicitly.
 * Translate-only nodes require zero rotation and unit scale; their positions
 * use exact GAME hal_translate without sine-table work or quantization.
 */
IsleGameResult isle_game_attach_model(IsleGame* game, IsleGameObject* object,
    const IsleGameModelNodeInit* initial, size_t count, IsleGameModel** output);
IsleGameResult isle_game_remove_model(IsleGame* game, IsleGameModel* model);
/* Pure preflight: owner/index/kind/scalar checks only, no state/cache mutation.
 * With no intervening mutation/callback/thread access, the same setter input
 * cannot subsequently fail. The caller owns that exclusion.
 */
IsleGameResult isle_game_model_validate_transform(IsleGame* game, IsleGameModel* model,
    size_t index, const IsleGameModelTransform* transform);
IsleGameResult isle_game_model_set_transform(IsleGame* game, IsleGameModel* model,
    size_t index, const IsleGameModelTransform* transform);
IsleGameResult isle_game_model_set_flags(IsleGame* game, IsleGameModel* model,
    size_t index, uint32_t flags);
IsleGameResult isle_game_model_nodes(IsleGame* game, IsleGameModel* model,
    const IsleGameModelNode** output, size_t* count);
/* Pure consumed-field preflight, with the same exclusion as validate_transform.
 * Setter eagerly computes once, then atomically publishes the complete record
 * and live-matrix count. Identical consumed input preserves revision. Disabled
 * stores neutral metadata/zero fixed bytes without computing an identity.
 * The getter is nonmutating; its stable const borrow expires at model removal.
 */
IsleGameResult isle_game_model_validate_placement(IsleGame*, IsleGameModel*,
    IsleGameModelPlacementMode, const IsleGameModelTransform*);
IsleGameResult isle_game_model_set_placement(IsleGame*, IsleGameModel*,
    IsleGameModelPlacementMode, const IsleGameModelTransform*);
IsleGameResult isle_game_model_placement(IsleGame*, IsleGameModel*,
    const IsleGameModelPlacement**);
/* Alias-safe, noncumulative root-local × photo-world bake. Always pass the
 * unbaked animated root. Photo-world uniform scale is game_model_scale*0.1f.
 * Output is cleared on failure; this pure calculation has no game ownership.
 */
IsleGameResult isle_game_model_bake_photo_transform(
    const IsleGameModelTransform* unbaked_root, const IsleGameVec3* position,
    float yaw, float game_model_scale, IsleGameModelTransform* output);

#ifdef __cplusplus
}
#endif

#endif

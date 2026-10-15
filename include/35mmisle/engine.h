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
typedef void (*IsleGameCallback)(IsleGameObject* object, void* user);

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
IsleGameResult isle_game_add_process(IsleGame* game, IsleGameObject* object,
                                    IsleGameCallback entry, void* user,
                                    IsleGameProcessKind kind, uint32_t priority,
                                    IsleGameProcess** output);
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
    ISLE_GAME_CAMERA_LOOK_AT = 6
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

/* Nonanimated camera payload, type 3, attached only to an empty object. Like
 * GAME omGObjSetCamera it initially has no matrices. Add up to two of either
 * supported kind, as GAME omCameraAddMtx permits; defaults are GAME parameters.
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
IsleGameResult isle_game_camera_set_viewport(IsleGame* game, IsleGameCamera* camera,
                                            const IsleGameViewport* parameters);
IsleGameResult isle_game_camera_parameters(IsleGame* game, IsleGameCamera* camera,
                                          const IsleGameCameraParameters** output);
IsleGameResult isle_game_camera_matrix(IsleGame* game, IsleGameCamera* camera,
                                      size_t index, const IsleGameCameraMatrix** output);
IsleGameResult isle_game_camera_matrix_count(IsleGame* game, IsleGameCamera* camera,
                                            size_t* output);
IsleGameResult isle_game_camera_perspective_normalization(IsleGame* game,
                                                         IsleGameCamera* camera,
                                                         uint16_t* output);
IsleGameResult isle_game_remove_camera(IsleGame* game, IsleGameCamera* camera);

#ifdef __cplusplus
}
#endif

#endif

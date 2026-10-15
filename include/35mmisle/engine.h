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

#ifdef __cplusplus
}
#endif

#endif

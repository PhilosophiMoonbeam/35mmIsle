#ifndef ISLE_PLAYER_COMMANDS_SOURCE_H
#define ISLE_PLAYER_COMMANDS_SOURCE_H
#include "player_source.h"
#include "source_command_scope.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Nonowning link to the actual retained Scene Services binding. This is not a
 * second command queue, Player state, failure latch or native object payload. */
typedef struct IslePlayerCommandSource {
    IslePlayerSource* player;
    void* owner;
} IslePlayerCommandSource;

typedef struct IslePlayerCommandData {
    int32_t cmd;
    IsleGameObject* source;
} IslePlayerCommandData;
typedef void (*IslePlayerCommandHandler)(IslePlayerSource*, IslePlayerCommandData);

/* Ordinary C helpers require a retained prepared Source, its active canonical
 * per-game context and an original C OM callback or source_call's local fence.
 * process additionally requires the real synchronous native object-update
 * boundary. Handlers are nonwaiting C calls; checked failures are caught locally
 * so the actual FIFO scope closes before the first failure propagates. No owning
 * C++ frame encloses handlers, and no borrow outlives actual native joins. */
void isle_player_commands_process(IslePlayerSource*, IsleGameObject*, IslePlayerCommandHandler);
void isle_player_commands_send(IslePlayerSource*, IsleGameObject*, int32_t command, IsleGameObject* sender);
void isle_player_commands_send_to_link(IslePlayerSource*, uint8_t link, int32_t command, IsleGameObject* sender);

/* Returning C++ leaves. end remains callable after Source failure and attempts
 * actual scope cleanup before reporting failure; none invokes original C. */
IsleGameResult isle_player_commands_backend_begin(IslePlayerCommandSource*, IsleGameObject*, IsleSourceCommandScope*);
IsleGameResult isle_player_commands_backend_end(IslePlayerCommandSource*, IsleSourceCommandScope*, IsleGameResult, int delete_requested);
IsleGameResult isle_player_commands_backend_send(IslePlayerCommandSource*, IsleGameObject*, int32_t command, IsleGameObject* sender);
IsleGameResult isle_player_commands_backend_send_to_link(IslePlayerCommandSource*, uint8_t link, int32_t command, IsleGameObject* sender);

#ifdef __cplusplus
}
#endif
#endif

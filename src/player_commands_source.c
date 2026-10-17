#include "player_commands_source.h"
#include "engine_internal.h"

static IslePlayerCommandSource* command_source(IslePlayerSource* player) {
    IsleGame* game = player->game;
    IslePlayerSource* canonical = game->playerSource;
    IslePlayerSource* active = NULL;
    /* A mismatched caller cannot nominate its own local failure fence. Match
     * the canonical Source borrow preconditions used by other checked C leaves. */
    IslePlayerSource* failure_source = canonical != NULL ? canonical : player;
    isle_player_source_checked(failure_source, isle_player_source_context(game, &active));
    if (active != player || (player->nonwaiting_call_frame == NULL &&
        (!game->ticking || omCurrentObject == NULL)))
        isle_player_source_checked(failure_source, ISLE_GAME_INVALID_STATE);
    if (player->commands == NULL || player->commands->player != player ||
        player->commands->owner == NULL)
        isle_player_source_checked(failure_source, ISLE_GAME_INVALID_STATE);
    return player->commands;
}

typedef struct PlayerCommandCall {
    IslePlayerSource* player;
    IsleGameObject* object;
    IslePlayerCommandHandler handler;
    IsleSourceCommandScope* scope;
    IsleGameResult result;
    int delete_requested;
} PlayerCommandCall;

static void command_handler(IsleGameObject* object, int32_t command,
    IsleGameObject* sender, void* user) {
    PlayerCommandCall* call = user;
    IslePlayerCommandData data = {command, sender};
    (void) object;
    call->handler(call->player, data);
}

static void command_process(IslePlayerSource* player, void* user) {
    PlayerCommandCall* call = user;
    /* The interpreter and typed handler are C. A checked handler failure lands
     * in source_call below, never across a returning native C++ backend. */
    call->result = isle_game_commands_checked(player->game, call->object,
        call->scope->pool, call->scope->queue,
        call->handler != NULL ? command_handler : NULL, call,
        NULL, NULL, NULL, &call->delete_requested);
}

void isle_player_commands_process(IslePlayerSource* player, IsleGameObject* object,
    IslePlayerCommandHandler handler) {
    IslePlayerCommandSource* source = command_source(player);
    IsleSourceCommandScope scope = {0};
    PlayerCommandCall call = {player, object, handler, &scope, ISLE_GAME_OK, 0};
    IsleGameResult first, released;
    isle_player_source_checked(player,
        isle_player_commands_backend_begin(source, object, &scope));
    first = isle_player_source_call(player, command_process, &call);
    if (first == ISLE_GAME_OK) first = call.result;
    /* ALWAYS close the actual native scope after the local C fence returns,
     * including checked Source failures with an open custom callback/current
     * sender credit. Cleanup must not reject canonical sticky Source status. */
    released = isle_player_commands_backend_end(source, &scope, first,
        call.delete_requested);
    if (first == ISLE_GAME_OK) first = released;
    isle_player_source_checked(player, first);
}

void isle_player_commands_send(IslePlayerSource* player, IsleGameObject* object,
    int32_t command, IsleGameObject* sender) {
    IslePlayerCommandSource* source = command_source(player);
    isle_player_source_checked(player,
        isle_player_commands_backend_send(source, object, command, sender));
}

void isle_player_commands_send_to_link(IslePlayerSource* player, uint8_t link,
    int32_t command, IsleGameObject* sender) {
    IslePlayerCommandSource* source = command_source(player);
    isle_player_source_checked(player,
        isle_player_commands_backend_send_to_link(source, link, command, sender));
}

#include "player_commands_rom_fixture.h"
#include "player_command_reference.h"
#include "engine_internal.h"
#include "player_command_reference.inc"

/* The native runtime admits one active game. This is borrowed observation
 * storage only, attached outside OM and detached after every actual join. No
 * object userdata, Source scalar, queue or gameplay state is substituted. */
static PlayerCommandsRomTrace* active_trace;

IsleGameResult player_commands_rom_attach(PlayerCommandsRomTrace* trace) {
    IslePlayerSource* canonical = NULL;
    IsleGameResult result;
    if (trace == NULL || trace->player == NULL || active_trace != NULL)
        return ISLE_GAME_INVALID_ARGUMENT;
    result = isle_player_source_context(trace->player->game, &canonical);
    if (result != ISLE_GAME_OK) return result;
    if (canonical != trace->player || canonical->game->ticking ||
        canonical->game->callback_depth != 0 || canonical->nonwaiting_call_frame != NULL)
        return ISLE_GAME_INVALID_STATE;
    active_trace = trace;
    return ISLE_GAME_OK;
}
IsleGameResult player_commands_rom_detach(PlayerCommandsRomTrace* trace) {
    IsleGameCounts counts;
    IsleGameResult result;
    if (trace == NULL || trace != active_trace) return ISLE_GAME_INVALID_ARGUMENT;
    result = isle_game_counts(trace->player->game, &counts);
    if (result != ISLE_GAME_OK) return result;
    if (counts.objects != 0 || counts.processes != 0 || trace->player->game->ticking ||
        trace->player->game->callback_depth != 0 || trace->player->nonwaiting_call_frame != NULL)
        return ISLE_GAME_INVALID_STATE;
    active_trace = NULL;
    return ISLE_GAME_OK;
}
static void record(PlayerCommandsRomTrace* trace, PlayerCommandsRomEvent* entries,
    size_t* count, IsleGameObject* receiver, int32_t command, IsleGameObject* sender) {
    PlayerCommandsRomEvent* event;
    if (*count == PLAYER_COMMANDS_ROM_TRACE_CAPACITY) { ++trace->invalid; return; }
    event = &entries[(*count)++];
    *event = (PlayerCommandsRomEvent){0};
    event->receiver = receiver;
    event->sender = sender;
    event->command = command;
    event->flags = receiver->flags;
    if (receiver->processListHead != NULL) event->paused = receiver->processListHead->paused;
    if (sender != NULL) {
        event->sender_id = sender->id;
        event->retired_sender = !sender->alive && sender->retirementStarted && sender->identityHolds != 0;
    }
}
void player_commands_rom_queued(IsleGameObject* object, int32_t command,
    IsleGameObject* sender, void* user) {
    PlayerCommandsRomTrace* trace = user;
    record(trace, trace->queued, &trace->queued_count, object, command, sender);
}
void player_commands_rom_child(IsleGameObject* object) {
    IslePlayerSource* player = isle_player_source_entry(object);
    if (active_trace == NULL || active_trace->player != player)
        isle_player_source_checked(player, ISLE_GAME_INVALID_STATE);
    ++active_trace->child_entered;
    isle_player_source_wait(player, 2);
    ++active_trace->child_after_wait;
    isle_player_source_checked(player, isle_game_end_process(player->game, NULL));
    ++active_trace->child_after_end;
}
static void wrong_payload(IslePlayerSource* player, void* camera) {
    ++active_trace->fault_entered;
    /* This is the real native wrong-payload leaf on the actual camera, not a
     * fabricated status or a C++ exception. The returning leaf fails before
     * Source.checked transfers to its local C fence. */
    (void)isle_player_source_first_sprite(player, camera);
    ++active_trace->fault_returned;
}
static void observe_empty(IslePlayerSource* player, IslePlayerCommandData command) {
    PlayerCommandsRomTrace* trace = active_trace;
    if (trace == NULL || trace->player != player)
        isle_player_source_checked(player, ISLE_GAME_INVALID_STATE);
    player_command_reference_empty_handle(player, command);
    record(trace, trace->delivered, &trace->delivered_count, omCurrentObject, command.cmd, command.source);
    switch (command.cmd) {
    case PLAYER_COMMANDS_ROM_EMIT:
        isle_player_commands_send(player, trace->camera, ISLE_PLAYER_CAMERA_BLINK, NULL);
        isle_player_commands_send_to_link(player, trace->broadcast_link, PLAYER_COMMANDS_ROM_BROADCAST, NULL);
        break;
    case PLAYER_COMMANDS_ROM_DELETE:
        isle_player_source_checked(player, isle_game_delete_object(player->game, NULL));
        break;
    case PLAYER_COMMANDS_ROM_FAULT:
        wrong_payload(player, trace->camera);
        break;
    case PLAYER_COMMANDS_ROM_NESTED_FAULT: {
        void* previous = player->nonwaiting_call_frame;
        trace->nested_result = isle_player_source_call(player, wrong_payload, trace->camera);
        trace->nested_restored = previous != NULL && player->nonwaiting_call_frame == previous;
        isle_player_source_checked(player, trace->nested_result);
        break;
    }
    case PLAYER_COMMANDS_ROM_APPEND_FAULT:
        isle_player_commands_send(player, omCurrentObject, PLAYER_COMMANDS_ROM_UNKNOWN, NULL);
        wrong_payload(player, trace->camera);
        break;
    case PLAYER_COMMANDS_ROM_FOREIGN_SOURCE:
        ++trace->fault_entered;
        isle_player_commands_send(trace->inactive_source, trace->camera,
            ISLE_PLAYER_CAMERA_BLINK, NULL);
        ++trace->fault_returned;
        break;
    case PLAYER_COMMANDS_ROM_CREATE:
    case PLAYER_COMMANDS_ROM_CREATE_FAULT:
        trace->child = isle_player_source_add_process(player, omCurrentObject,
            player_commands_rom_child, ISLE_GAME_STACKFUL, 2);
        ++trace->child_created;
        if (command.cmd == PLAYER_COMMANDS_ROM_CREATE_FAULT)
            wrong_payload(player, trace->camera);
        break;
    case PLAYER_COMMANDS_ROM_FOREIGN_CREATE:
        ++trace->fault_entered;
        (void)isle_player_source_add_process(trace->inactive_source, omCurrentObject,
            player_commands_rom_child, ISLE_GAME_STACKFUL, 2);
        ++trace->fault_returned;
        break;
    default:
        break;
    }
    ++trace->handlers_returned;
}
void player_commands_rom_sprite_update(IsleGameObject* object) {
    IslePlayerSource* player = isle_player_source_entry(object);
    if (active_trace == NULL || active_trace->player != player)
        isle_player_source_checked(player, ISLE_GAME_INVALID_STATE);
    ++active_trace->updates_entered;
    if (active_trace->probe_update_admission && !active_trace->null_update_probed) {
        ++active_trace->null_update_probed;
        active_trace->null_during_update = isle_game_object_set_source_update(player->game, NULL, NULL);
    }
    (void)isle_player_source_first_sprite(player, object);
    isle_player_commands_process(player, object, observe_empty);
    ++active_trace->updates_returned;
}
void player_commands_rom_process(IsleGameObject* object) {
    IslePlayerSource* player = isle_player_source_entry(object);
    if (active_trace == NULL || active_trace->player != player)
        isle_player_source_checked(player, ISLE_GAME_INVALID_STATE);
    ++active_trace->process_runs;
}
void player_commands_rom_retiring(IsleGameObject* object, void* user) {
    PlayerCommandsRomTrace* trace = user;
    ++trace->retiring;
    if (object->type != 2 || object->data.sobj == NULL || object->userData != NULL ||
        object->sourcePayloadKind != ISLE_SOURCE_PAYLOAD_NONE) ++trace->join_invalid;
    if (trace->probe_update_admission) {
        trace->live_during_retirement =
            isle_game_object_set_source_update(trace->player->game, trace->camera, NULL);
        trace->retiring_update =
            isle_game_object_set_source_update(trace->player->game, object, NULL);
    }
}
void player_commands_rom_joined(IsleGameObject* object, void* user) {
    PlayerCommandsRomTrace* trace = user;
    ++trace->joined;
    if (object->processListHead != NULL || object->processListTail != NULL ||
        object->type != 0 || object->data.any != NULL || !object->retirementStarted)
        ++trace->join_invalid;
}
IsleGameResult player_commands_rom_topology(IslePlayerSource* player,
    IsleGameObject* camera, PlayerCommandsRomTopology* out) {
    IsleGameResult result;
    if (player == NULL || out == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    *out = (PlayerCommandsRomTopology){0};
    result = isle_game_counts(player->game, &out->counts);
    if (result != ISLE_GAME_OK) return result;
    out->held_identities = player->game->held_identities;
    out->callback_depth = player->game->callback_depth;
    out->command_fault_armed = player->game->command_fault_abort != NULL || player->game->command_fault_context != NULL;
    out->source_frame_open = player->nonwaiting_call_frame != NULL;
    out->traversing = player->game->ticking || omCurrentObject != NULL || omCurrentProcess != NULL;
    if (camera != NULL) {
        IsleGameObjectMetadata metadata;
        result = isle_game_object_metadata(player->game, camera, &metadata);
        if (result != ISLE_GAME_OK) return result;
        out->camera_original_update = camera->fnUpdate == isle_player_mainCameraUpdate &&
            camera->update == NULL && camera->updateUser == NULL;
        out->camera_none_payload = camera->sourcePayloadKind == ISLE_SOURCE_PAYLOAD_NONE && camera->userData == NULL;
        out->camera_attached = camera->type == 3 && camera->data.cam != NULL && camera->data.cam == player->camera;
    }
    return ISLE_GAME_OK;
}

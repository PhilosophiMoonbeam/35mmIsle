#ifndef ISLE_PLAYER_COMMANDS_ROM_FIXTURE_H
#define ISLE_PLAYER_COMMANDS_ROM_FIXTURE_H
#include "player_commands_source.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Fixture-only controls/observations around the exact original empty handler.
 * They are not Player commands or a gameplay receiver implementation. */
enum {
    PLAYER_COMMANDS_ROM_UNKNOWN = 700,
    PLAYER_COMMANDS_ROM_EMIT = 701,
    PLAYER_COMMANDS_ROM_DELETE = 702,
    PLAYER_COMMANDS_ROM_FAULT = 703,
    PLAYER_COMMANDS_ROM_NESTED_FAULT = 704,
    PLAYER_COMMANDS_ROM_BROADCAST = 705,
    PLAYER_COMMANDS_ROM_APPEND_FAULT = 706,
    PLAYER_COMMANDS_ROM_FOREIGN_SOURCE = 707,
    PLAYER_COMMANDS_ROM_TRACE_CAPACITY = 64
};
typedef struct PlayerCommandsRomEvent {
    IsleGameObject* receiver;
    IsleGameObject* sender;
    int32_t command;
    uint32_t flags;
    uint32_t sender_id;
    unsigned paused, retired_sender;
} PlayerCommandsRomEvent;
typedef struct PlayerCommandsRomTrace {
    IslePlayerSource* player;
    IsleGameObject* camera;
    IslePlayerSource* inactive_source;
    uint8_t broadcast_link;
    PlayerCommandsRomEvent delivered[PLAYER_COMMANDS_ROM_TRACE_CAPACITY];
    PlayerCommandsRomEvent queued[PLAYER_COMMANDS_ROM_TRACE_CAPACITY];
    size_t delivered_count, queued_count;
    unsigned invalid, updates_entered, updates_returned, handlers_returned;
    unsigned process_runs, fault_entered, fault_returned, nested_restored;
    unsigned retiring, joined, join_invalid;
    unsigned probe_update_admission, null_update_probed;
    IsleGameResult null_during_update, live_during_retirement, retiring_update;
    IsleGameResult nested_result;
} PlayerCommandsRomTrace;
typedef struct PlayerCommandsRomTopology {
    IsleGameCounts counts;
    size_t held_identities;
    unsigned callback_depth, command_fault_armed, source_frame_open, traversing;
    unsigned camera_original_update, camera_none_payload, camera_attached;
} PlayerCommandsRomTopology;

IsleGameResult player_commands_rom_attach(PlayerCommandsRomTrace*);
IsleGameResult player_commands_rom_detach(PlayerCommandsRomTrace*);
void player_commands_rom_sprite_update(IsleGameObject*);
void player_commands_rom_process(IsleGameObject*);
void player_commands_rom_queued(IsleGameObject*, int32_t, IsleGameObject*, void*);
void player_commands_rom_retiring(IsleGameObject*, void*);
void player_commands_rom_joined(IsleGameObject*, void*);
IsleGameResult player_commands_rom_topology(IslePlayerSource*, IsleGameObject*, PlayerCommandsRomTopology*);
#ifdef __cplusplus
}
#endif
#endif

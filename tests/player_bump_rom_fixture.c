#include "player_bump_rom_fixture.h"
#include "actor_owner_source.h"
#include "engine_internal.h"

void player_bump_rom_call(IslePlayerSource* player, void* user) {
    const struct PlayerBumpRomCall* call = user;
    IsleGameResult result = ISLE_GAME_INVALID_ARGUMENT;
    /* Actual cold C context entry and local nonwaiting fault fence. */
    if (isle_player_source_entry(player->object) != player)
        isle_player_source_checked(player, ISLE_GAME_INVALID_STATE);
    switch (call->operation) {
    case PLAYER_BUMP_ROM_INIT: result = isle_player_bump_BumpDetector_Init(player->bump); break;
    case PLAYER_BUMP_ROM_DISABLE: result = isle_player_bump_BumpDetector_Disable(player->bump); break;
    case PLAYER_BUMP_ROM_ENABLE: result = isle_player_bump_BumpDetector_Enable(player->bump); break;
    case PLAYER_BUMP_ROM_DELETE: result = isle_player_bump_BumpDetector_Delete(player->bump, 123); break;
    }
    /* Every native C++ leaf has returned before the C-only transfer. */
    isle_player_source_checked(player, result);
}

struct ReceiverCall {
    struct PlayerBumpRomFixture* fixture;
    IsleGameObject* receiver;
    IsleGameObject* sender;
    int32_t command;
};
static void receiver_call(IslePlayerSource* player, void* user) {
    struct ReceiverCall* call = user;
    struct PlayerBumpRomFixture* fixture = call->fixture;
    IslePlayerSource* bound = NULL;
    struct PlayerBumpRomObservation* observation;
    isle_player_source_checked(player,
        isle_player_source_get(player->game, call->receiver, &bound));
    if (bound != player || call->receiver != player->object ||
        fixture->delivered_count == PLAYER_BUMP_ROM_CAPACITY)
        isle_player_source_checked(player, ISLE_GAME_INVALID_STATE);
    observation = &fixture->delivered[fixture->delivered_count++];
    *observation = (struct PlayerBumpRomObservation){
        call->command, fixture->bump->BumpDetector_ProximityLevel,
        call->receiver, call->sender
    };
    if (call->command == 4) {
        if (call->sender != fixture->expected_sender || !fixture->borrower)
            isle_player_source_checked(player, ISLE_GAME_INVALID_STATE);
        /* Explicit harness interpretation: adopt in a real canonical Actor
         * field. Only real Actor retirement will close this stored borrow. */
        isle_player_source_checked(player, isle_actor_owner_reference_assign(
            fixture->borrower, &fixture->borrower->interactionTarget, call->sender));
    } else if (call->command != 100 && call->command != 101) {
        isle_player_source_checked(player, ISLE_GAME_INVALID_ARGUMENT);
    }
}
void player_bump_rom_receiver(IsleGameObject* receiver, int32_t command,
    IsleGameObject* sender, void* user) {
    struct PlayerBumpRomFixture* fixture = user;
    struct ReceiverCall call = {fixture, receiver, sender, command};
    /* Never jump out through Services' C++ dispatcher. Check the exact
     * returned source status only after its local C fence has returned. */
    IsleGameResult result = isle_player_source_call(fixture->player, receiver_call, &call);
    if (fixture->callback_status == ISLE_GAME_OK) fixture->callback_status = result;
}
void player_bump_rom_queued(IsleGameObject* receiver, int32_t command,
    IsleGameObject* sender, void* user) {
    struct PlayerBumpRomFixture* fixture = user;
    struct PlayerBumpRomObservation* observation;
    if (command != 4) return;
    if (fixture->queued_count == PLAYER_BUMP_ROM_CAPACITY ||
        receiver != fixture->player->object || sender != fixture->expected_sender ||
        fixture->bump->BumpDetector_Object != sender || omCurrentObject != sender ||
        !omCurrentProcess || omCurrentProcess->object != sender ||
        omCurrentProcess->kind != 0 || omCurrentProcess->priority != 9 ||
        omCurrentProcess->function != isle_player_bump_BumpDetector_Update ||
        sender->sourcePayloadKind != ISLE_SOURCE_PAYLOAD_NONE || sender->userData != NULL)
    {
        if (fixture->callback_status == ISLE_GAME_OK)
            fixture->callback_status = ISLE_GAME_INVALID_STATE;
        return;
    }
    observation = &fixture->queued[fixture->queued_count++];
    *observation = (struct PlayerBumpRomObservation){
        command, fixture->bump->BumpDetector_ProximityLevel, receiver, sender
    };
    return;
}
IsleGameResult player_bump_rom_inspect(IslePlayerBumpSource* bump, int paused) {
    GObjProcess* process;
    IsleGameObjectMetadata metadata;
    IsleGameResult result = isle_game_object_metadata(bump->player->game,
        bump->BumpDetector_Object, &metadata);
    if (result != ISLE_GAME_OK) return result;
    process = bump->BumpDetector_Object->processListHead;
    if (metadata.id != 8 || metadata.update_link != 0 || metadata.priority != 0x80000000u ||
        !bump->BumpDetector_Object->update ||
        bump->BumpDetector_Object->sourcePayloadKind != ISLE_SOURCE_PAYLOAD_NONE ||
        bump->BumpDetector_Object->userData != NULL || !process || process->next ||
        process->function != isle_player_bump_BumpDetector_Update ||
        process->kind != 0 || process->priority != 9 || process->paused != !!paused)
        return ISLE_GAME_INVALID_STATE;
    return ISLE_GAME_OK;
}
void player_bump_rom_delete_self(IsleGameObject* object, void* user) {
    struct PlayerBumpRomFixture* fixture = user;
    IslePlayerSource* player = isle_player_source_current(object);
    IslePlayerBumpSource* bump = player->bump;
    IsleGameResult result;
    fixture->self_delete_proximity = bump->BumpDetector_ProximityLevel;
    fixture->self_delete_enabled = bump->BumpDetector_IsEnabled;
    result = isle_player_bump_BumpDetector_Delete(bump, -42);
    fixture->self_delete_handle_preserved = bump->BumpDetector_Object == object;
    isle_player_source_checked(player, result);
    fixture->self_delete_returned = 1;
    /* Return normally. The actual native callback retirement joins Update's
     * suspended C stack before Services' composed joined observer clears it. */
}

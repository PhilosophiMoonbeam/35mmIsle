#ifndef ISLE_TEST_PLAYER_BUMP_ROM_FIXTURE_H
#define ISLE_TEST_PLAYER_BUMP_ROM_FIXTURE_H
#include "player_bump_source.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Bounded receiver/observations only. This is not the production Player FSM.
 * The retained sender is the actual ActorOwner interactionTarget field, never
 * a harness-held identity or an invented borrow credit. */
enum { PLAYER_BUMP_ROM_CAPACITY = 96 };
enum PlayerBumpRomOperation {
    PLAYER_BUMP_ROM_INIT, PLAYER_BUMP_ROM_DISABLE,
    PLAYER_BUMP_ROM_ENABLE, PLAYER_BUMP_ROM_DELETE
};
struct PlayerBumpRomCall { enum PlayerBumpRomOperation operation; };
struct PlayerBumpRomObservation {
    int32_t command, proximity;
    IsleGameObject* receiver;
    IsleGameObject* sender;
};
struct PlayerBumpRomFixture {
    IslePlayerSource* player;
    IslePlayerBumpSource* bump;
    IsleActorSource* borrower;
    IsleGameObject* expected_sender;
    IsleGameResult callback_status;
    size_t queued_count, delivered_count;
    struct PlayerBumpRomObservation queued[PLAYER_BUMP_ROM_CAPACITY];
    struct PlayerBumpRomObservation delivered[PLAYER_BUMP_ROM_CAPACITY];
    int self_delete_returned, self_delete_handle_preserved;
    int32_t self_delete_proximity;
    int8_t self_delete_enabled;
};
void player_bump_rom_call(IslePlayerSource*, void*);
void player_bump_rom_receiver(IsleGameObject*, int32_t, IsleGameObject*, void*);
void player_bump_rom_queued(IsleGameObject*, int32_t, IsleGameObject*, void*);
IsleGameResult player_bump_rom_inspect(IslePlayerBumpSource*, int paused);
void player_bump_rom_delete_self(IsleGameObject*, void*);
#ifdef __cplusplus
}
#endif
#endif

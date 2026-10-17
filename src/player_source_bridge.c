#include "player_source.h"
#include "engine_internal.h"

/* Same explicitly qualified modern PI policy as the common C importer; this is
 * not a claim about original SDK macro types or numerical equivalence. */
#define PLAYER_NATIVE_PI 3.14159265358979323846

typedef struct IslePlayerCallFrame {
    jmp_buf failure;
    void* previous;
} IslePlayerCallFrame;

IsleGameResult isle_player_source_prepare(IsleGame* game, IslePlayerSource* source,
    isle_rng* rng, void* owner, int32_t minimum_photo_interval) {
    IsleGameResult result;
    if (source == NULL || rng == NULL || owner == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    result = isle_game_status(game);
    if (result != ISLE_GAME_OK) return result;
    if (source->game != NULL || source->object != NULL || source->owner != NULL || source->rng != NULL)
        return ISLE_GAME_INVALID_STATE;
    /* All other fields start in the caller's required zero state. Set only the
     * consumed nonzero original defaults and actual native ownership links. */
    source->game = game;
    source->owner = owner;
    source->rng = rng;
    source->movement = (IsleWorldMovementState)ISLE_WORLD_MOVEMENT_STATE_INITIALIZER;
    source->cam_target.z = -500.0f;
    source->gDirectionIndex = 2;
    source->TotalSpeedMult = 0.0005f;
    source->CameraVibrationAmplitude = 20.0f;
    source->CameraVibrationSpeed = 3.0f;
    source->DashEngineSpeedMult = 1.0f;
    source->ThrowItemTimeout = 3;
    source->PressPokeFluteTimeout = 3;
    source->DashEngineSoundTimeout = 3;
    source->DirectionsList[0] = -PLAYER_NATIVE_PI;
    source->DirectionsList[1] = -PLAYER_NATIVE_PI / 2;
    source->DirectionsList[3] = PLAYER_NATIVE_PI / 2;
    source->DirectionsList[4] = PLAYER_NATIVE_PI;
    source->MinPitch = -PLAYER_NATIVE_PI / 8;
    source->MaxPitch = PLAYER_NATIVE_PI / 4;
    source->D_80382D00_523110 = 2;
    source->D_80382D28_523138 = 153;
    source->D_80382D38_523148 = 360;
    source->D_80382D3C_52314C = 1.0f;
    source->LevelBGMSongID = -1;
    source->sMinTimeBetweenPhotos = minimum_photo_interval;
    source->D_80382D54_523164 = 100;
    source->dashEngineSoundHandle = -1;
    source->D_80382D5C_52316C = 50.0f;
    source->D_80382D64_523174 = 50.0f;
    source->sTimerAfterPhotoTaken = -1;
    return ISLE_GAME_OK;
}

IsleGameResult isle_player_source_activate(IslePlayerSource* source) {
    IsleGameResult result;
    IsleGame* game;
    if (source == NULL || source->game == NULL || source->owner == NULL || source->rng == NULL)
        return ISLE_GAME_INVALID_ARGUMENT;
    game = source->game;
    result = isle_game_status(game);
    if (result != ISLE_GAME_OK) return result;
    if (source->status != ISLE_GAME_OK) return source->status;
    if (game->ticking || game->callback_depth != 0 || game->playerSource != NULL ||
        source->nonwaiting_call_frame != NULL) return ISLE_GAME_INVALID_STATE;
    game->playerSource = source;
    return ISLE_GAME_OK;
}

IsleGameResult isle_player_source_deactivate(IslePlayerSource* source) {
    IsleGameCounts counts;
    IsleGameResult result;
    IsleGame* game;
    if (source == NULL || source->game == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    game = source->game;
    /* Cleanup remains possible after a sticky execution fault. This ordinary
     * owner/handle check does not clear or reinterpret that fault. */
    result = isle_game_counts(game, &counts);
    if (result != ISLE_GAME_OK) return result;
    if (game->ticking || game->callback_depth != 0 || omCurrentObject != NULL ||
        omCurrentProcess != NULL || game->playerSource != source ||
        source->nonwaiting_call_frame != NULL) return ISLE_GAME_INVALID_STATE;
    game->playerSource = NULL;
    return game->fault;
}

IsleGameResult isle_player_source_bind(IsleGame* game, IsleGameObject* object,
    IslePlayerSource* source) {
    IsleGameObjectMetadata metadata;
    IsleGameResult result = isle_game_object_metadata(game, object, &metadata);
    if (result != ISLE_GAME_OK) return result;
    if (source == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    if (game->fault != ISLE_GAME_OK) return game->fault;
    if (source->status != ISLE_GAME_OK) return source->status;
    if (source->game != game || source->owner == NULL || source->rng == NULL ||
        game->playerSource != source || source->object != NULL || object->retirementStarted ||
        object->sourcePayloadKind != ISLE_SOURCE_PAYLOAD_NONE || object->userData != NULL)
        return ISLE_GAME_INVALID_STATE;
    source->object = object;
    object->userData = source;
    object->sourcePayloadKind = ISLE_SOURCE_PAYLOAD_PLAYER;
    return ISLE_GAME_OK;
}

IsleGameResult isle_player_source_get(IsleGame* game, IsleGameObject* object,
    IslePlayerSource** output) {
    IsleGameObjectMetadata metadata;
    IslePlayerSource* source;
    IsleGameResult result;
    if (output != NULL) *output = NULL;
    result = isle_game_object_metadata(game, object, &metadata);
    if (result != ISLE_GAME_OK) return result;
    if (output == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    if (game->fault != ISLE_GAME_OK) return game->fault;
    if (object->retirementStarted || object->sourcePayloadKind != ISLE_SOURCE_PAYLOAD_PLAYER ||
        object->userData == NULL) return ISLE_GAME_INVALID_STATE;
    source = object->userData;
    if (source->game != game || source->object != object || source->owner == NULL ||
        source->rng == NULL || game->playerSource != source) return ISLE_GAME_INVALID_STATE;
    if (source->status != ISLE_GAME_OK) return source->status;
    *output = source;
    return ISLE_GAME_OK;
}

IsleGameResult isle_player_source_context(IsleGame* game, IslePlayerSource** output) {
    IslePlayerSource* source;
    IsleGameResult result;
    if (output != NULL) *output = NULL;
    if (output == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    result = isle_game_status(game);
    if (result != ISLE_GAME_OK) return result;
    source = game->playerSource;
    if (source == NULL) return ISLE_GAME_OK;
    if (source->game != game || source->owner == NULL || source->rng == NULL)
        return ISLE_GAME_INVALID_STATE;
    if (source->status != ISLE_GAME_OK) return source->status;
    *output = source;
    return ISLE_GAME_OK;
}

IslePlayerSource* isle_player_source_current(IsleGameObject* object) {
    IsleGameObjectMetadata metadata;
    IsleGame* game = object->game;
    IslePlayerSource* source = game->playerSource;
    if (game->fault != ISLE_GAME_OK || isle_game_object_metadata(game, object, &metadata) != ISLE_GAME_OK ||
        source == NULL || source->game != game || source->owner == NULL || source->rng == NULL ||
        source->status != ISLE_GAME_OK || !game->ticking || omCurrentObject == NULL)
        isle_native_execution_fault(game, "original Player callback requires its live per-game C context");
    return source;
}

IslePlayerSource* isle_player_source_entry(IsleGameObject* object) {
    IsleGame* game = object->game;
    IslePlayerSource* source = game->playerSource;
    if (source != NULL && source->nonwaiting_call_frame != NULL) {
        IsleGameObjectMetadata metadata;
        isle_player_source_checked(source, source->status);
        isle_player_source_checked(source, game->fault);
        isle_player_source_checked(source, isle_game_object_metadata(game, object, &metadata));
        if (source->game != game || source->owner == NULL || source->rng == NULL)
            isle_player_source_checked(source, ISLE_GAME_INVALID_STATE);
        return source;
    }
    return isle_player_source_current(object);
}

IsleGameResult isle_player_source_call(IslePlayerSource* source,
    IslePlayerNonwaitingCall callback, void* user) {
    IslePlayerCallFrame frame;
    IsleGameResult result;
    if (source == NULL || source->game == NULL || callback == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    result = isle_game_status(source->game);
    if (result != ISLE_GAME_OK) return result;
    if (source->game->playerSource != source || source->owner == NULL || source->rng == NULL)
        return ISLE_GAME_INVALID_STATE;
    if (source->status != ISLE_GAME_OK) return source->status;
    frame.previous = source->nonwaiting_call_frame;
    source->nonwaiting_call_frame = &frame;
    if (setjmp(frame.failure) == 0) callback(source, user);
    source->nonwaiting_call_frame = frame.previous;
    return source->status;
}

void isle_player_source_checked(IslePlayerSource* source, IsleGameResult result) {
    if (result == ISLE_GAME_OK) return;
    if (source->status == ISLE_GAME_OK) source->status = result;
    if (source->nonwaiting_call_frame != NULL) {
        IslePlayerCallFrame* frame = source->nonwaiting_call_frame;
        longjmp(frame->failure, 1);
    }
    isle_native_execution_fault(source->game, "original Player native source leaf failed");
}

void isle_player_source_wait(IslePlayerSource* source, int32_t updates) {
    if (updates < 0 || (updates > 0 && source->nonwaiting_call_frame != NULL)) {
        isle_player_source_checked(source, ISLE_GAME_INVALID_STATE);
        return;
    }
    isle_player_source_checked(source, isle_game_wait(source->game, (uint32_t)updates));
}

static void check_callback_source(IslePlayerSource* source) {
    IsleGame* game = source->game;
    IslePlayerSource* canonical = game->playerSource;
    IslePlayerSource* active = NULL;
    /* Only the actual canonical record may supply a local C failure fence.
     * With no active context, an admitted OM callback still has its native C
     * fault target; a merely prepared record cannot authorize a borrow. */
    IslePlayerSource* failure_source = canonical != NULL ? canonical : source;
    isle_player_source_checked(failure_source, isle_player_source_context(game, &active));
    if (active != source || (source->nonwaiting_call_frame == NULL &&
        (!game->ticking || omCurrentObject == NULL)))
        isle_player_source_checked(failure_source, ISLE_GAME_INVALID_STATE);
}

IsleGameProcess* isle_player_source_add_process(IslePlayerSource* source,
    IsleGameObject* object, IsleGameSourceCallback function, uint8_t kind,
    uint32_t priority) {
    IsleGameProcess* process = NULL;
    check_callback_source(source);
    isle_player_source_checked(source, isle_game_add_source_process(source->game,
        object, function, (IsleGameProcessKind)kind, priority, &process));
    return process;
}

IsleGameSprite* isle_player_source_first_sprite(IslePlayerSource* source,
    IsleGameObject* object) {
    IsleGameSprite* sprite = NULL;
    check_callback_source(source);
    isle_player_source_checked(source, isle_game_object_sprite(source->game, object, &sprite));
    return sprite;
}

IsleSprite* isle_player_source_sprite_view(IslePlayerSource* source,
    IsleGameSprite* sprite) {
    IsleSprite* view = NULL;
    check_callback_source(source);
    isle_player_source_checked(source, isle_game_sprite_view(source->game, sprite, &view));
    return view;
}

#include "screen_transition_source.h"
#include "engine_internal.h"
#include "engine_math.h"

IsleGameResult isle_screen_transition_bind(IsleScreenTransitionObject* r) {
    IsleGameCounts counts;
    IsleGameResult result;
    if (!r || !r->source || !r->object) return ISLE_GAME_INVALID_ARGUMENT;
    result = isle_game_counts(r->source->game, &counts);
    if (result != ISLE_GAME_OK) return result;
    if (r->object->game != r->source->game || !r->object->alive ||
        r->object->retirementStarted || r->object->userData != NULL ||
        r->object->sourcePayloadKind != ISLE_SOURCE_PAYLOAD_NONE) return ISLE_GAME_INVALID_STATE;
    r->object->userData = r;
    return ISLE_GAME_OK;
}
IsleScreenTransitionObject* isle_screen_transition_current(IsleGameObject* object) {
    IsleScreenTransitionObject* r;
    IsleGameSourceCallback callback = NULL;
    if (!object || !object->alive || object->retirementStarted ||
        object->sourcePayloadKind != ISLE_SOURCE_PAYLOAD_NONE ||
        !object->game->ticking || omCurrentObject != object || omCurrentProcess == NULL)
        isle_native_execution_fault(object ? object->game : NULL, "transition callback requires its real native process");
    r = object->userData;
    if (!r || r->object != object || !r->source || r->source->game != object->game ||
        isle_game_process_source_function(object->game, omCurrentProcess, &callback) != ISLE_GAME_OK ||
        callback != r->callback)
        isle_native_execution_fault(object->game, "transition callback owner or typed native identity mismatch");
    return r;
}
void isle_screen_transition_checked(IsleScreenTransitionSource* s, IsleGameResult result) {
    if (result == ISLE_GAME_OK) return;
    if (s && s->status == ISLE_GAME_OK) s->status = result;
    isle_native_execution_fault(s ? s->game : NULL, "transition source native operation failed");
}
void isle_screen_transition_delete(IsleScreenTransitionSource* s, IsleGameObject* object) {
    (void)s;
    omDeleteGObj(object);
}

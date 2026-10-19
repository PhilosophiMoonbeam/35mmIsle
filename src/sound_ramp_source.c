#include "sound_ramp_source.h"
#include "engine_internal.h"

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

/* The actual Game owns only a nonowning lifetime association. Canonical rows
 * exist once in Services; no global calendar or current-course router exists. */
typedef struct SoundRampCallFrame { jmp_buf failure; void* previous; } SoundRampCallFrame;

typedef enum SoundRampOperation {
    SOUND_RAMP_START_PAN, SOUND_RAMP_START_VOLUME, SOUND_RAMP_FINISH_KIND,
    SOUND_RAMP_FINISH_ALL, SOUND_RAMP_FADE_ALL
} SoundRampOperation;

static int complete_native(const IsleSoundRampNative* n) {
    return n->publish_object && n->reconcile_field && n->erase_object &&
        n->object_joined && n->release_object && n->sound_pan && n->sound_volume;
}
static int valid_kind(IsleSoundRampKind kind) {
    return kind == ISLE_SOUND_RAMP_VOLUME || kind == ISLE_SOUND_RAMP_PAN;
}
static IsleSoundRampRow* row(IsleSoundRampSource* s, IsleSoundRampKind kind, uint32_t index) {
    if (!valid_kind(kind) || index >= 5) return NULL;
    return kind == ISLE_SOUND_RAMP_PAN ? &s->D_800BEFD8[index] : &s->D_800BEF60[index];
}
IsleSoundRampSource* isle_sound_ramp_source_current(IsleGame* game) {
    return game ? game->soundRampSource : NULL;
}
static IsleGameResult access_source(IsleSoundRampSource* s, int healthy) {
    IsleGameCounts counts;
    IsleGameResult result;
    if (!s || !s->game || !s->owner || !s->bindings || !s->binding_capacity ||
        !s->source_incarnation || !complete_native(&s->native)) return ISLE_GAME_INVALID_ARGUMENT;
    result = isle_game_counts(s->game, &counts);
    if (result != ISLE_GAME_OK) return result;
    if (s->game->soundRampSource != s || s->game->retirement_observer) return ISLE_GAME_INVALID_STATE;
    if (healthy && s->status != ISLE_GAME_OK) return s->status;
    if (healthy && s->game->fault != ISLE_GAME_OK) return s->game->fault;
    return ISLE_GAME_OK;
}
static IsleSoundRampBinding* find_binding(IsleSoundRampSource* s, IsleSoundRampKind kind,
    uint32_t index, IsleGameObject* object) {
    size_t i;
    IsleSoundRampRow* r = row(s,kind,index);
    if (!r || !object) return NULL;
    for (i = 0; i < s->binding_capacity; ++i) {
        IsleSoundRampBinding* b = &s->bindings[i];
        if (b->object == object && b->source == s && b->source_incarnation == s->source_incarnation &&
            b->incarnation && b->incarnation <= s->next_incarnation && b->kind == kind &&
            b->index == index && b->field == &r->unk_00) return b;
    }
    return NULL;
}
void isle_sound_ramp_checked(IsleSoundRampSource* s, IsleGameResult result) {
    if (result == ISLE_GAME_OK) return;
    if (!s || !s->game) abort();
    if (s->status == ISLE_GAME_OK) s->status = result;
    if (!s->failure[0])
        (void)snprintf(s->failure,sizeof(s->failure),"original sound ramp native operation failed (%d)",(int)result);
    if (s->nonwaiting_call_frame)
        longjmp(((SoundRampCallFrame*)s->nonwaiting_call_frame)->failure,1);
    /* All returning Services/audio leaves have finished. This is the real
     * current Game's C-only process/update execution frame, never a C++ loop. */
    isle_native_execution_fault(s->game,s->failure);
    abort();
}
void isle_sound_ramp_require(IsleSoundRampSource* s) {
    isle_sound_ramp_checked(s,access_source(s,1));
}
void isle_sound_ramp_require_reset(IsleSoundRampSource* s) {
    size_t i;
    isle_sound_ramp_checked(s,access_source(s,0));
    if (isle_sound_ramp_source_outstanding(s))
        isle_sound_ramp_checked(s,ISLE_GAME_INVALID_STATE);
    for (i = 0; i < 5; ++i)
        if (s->D_800BEF60[i].unk_00 || s->D_800BEFD8[i].unk_00)
            isle_sound_ramp_checked(s,ISLE_GAME_INVALID_STATE);
}
IsleGameResult isle_sound_ramp_source_initialize(IsleSoundRampSource* s, IsleGame* game,
    void* owner, uint64_t incarnation, IsleSoundRampNative native,
    IsleSoundRampBinding* bindings, size_t capacity) {
    IsleGameResult result;
    size_t i;
    if (!s || !game || !owner || !incarnation || !bindings || capacity < 10 ||
        capacity > SIZE_MAX / sizeof(*bindings) || !complete_native(&native)) return ISLE_GAME_INVALID_ARGUMENT;
    result = isle_game_require_idle(game);
    if (result != ISLE_GAME_OK) return result;
    result = isle_game_status(game);
    if (result != ISLE_GAME_OK) return result;
    if (game->soundRampSource || s->game || s->owner || s->source_incarnation || s->nonwaiting_call_frame)
        return ISLE_GAME_INVALID_STATE;
    for (i = 0; i < capacity; ++i)
        if (bindings[i].object || bindings[i].source || bindings[i].process || bindings[i].incarnation)
            return ISLE_GAME_INVALID_STATE;
    *s = (IsleSoundRampSource){0};
    s->game = game;
    s->owner = owner;
    s->native = native;
    s->bindings = bindings;
    s->binding_capacity = capacity;
    s->source_incarnation = incarnation;
    game->soundRampSource = s;
    /* BSS is native zero storage once; all later reset scalar stores are the
     * actual imported AB9A4, not a second native row-reset implementation. */
    return isle_sound_ramp_source_reset(s);
}
IsleSoundRampBinding* isle_sound_ramp_current(IsleGameObject* object, IsleSoundRampKind kind) {
    IsleSoundRampSource* s = object && object->game ? object->game->soundRampSource : NULL;
    IsleSoundRampBinding* b = NULL;
    IsleGameSourceCallback expected, actual = NULL;
    IsleGameObjectMetadata metadata;
    uintptr_t base, address, offset;
    if (!object || !object->game) abort();
    if (!s || object->game != s->game) {
        isle_native_execution_fault(object->game,"sound callback requires its actual Game lifetime association");
        abort();
    }
    isle_sound_ramp_require(s);
    if (!valid_kind(kind) || !s->game->ticking || s->nonwaiting_call_frame ||
        s->game->callback_depth || s->game->retirement_observer ||
        omCurrentObject != object || !omCurrentProcess || omCurrentProcess->object != object ||
        omCurrentProcess->game != s->game || !omCurrentProcess->alive ||
        omCurrentProcess->kind != ISLE_GAME_STACKFUL || omCurrentProcess->priority != 1 ||
        object->retirementStarted || object->sourcePayloadKind != ISLE_SOURCE_PAYLOAD_NONE ||
        object->userData != NULL)
        isle_sound_ramp_checked(s,ISLE_GAME_INVALID_STATE);
    expected = kind == ISLE_SOUND_RAMP_PAN ? isle_sound_ramp_func_800AB240 : isle_sound_ramp_func_800AB518;
    isle_sound_ramp_checked(s,isle_game_process_source_function(s->game,omCurrentProcess,&actual));
    if (actual != expected) isle_sound_ramp_checked(s,ISLE_GAME_INVALID_STATE);
    /* Admit the exact native array element in constant time. Construct the
     * pointer from our owned array only after bounds/alignment validation;
     * never dereference an arbitrary process user pointer. */
    base = (uintptr_t)(void*)s->bindings;
    address = (uintptr_t)omCurrentProcess->user;
    if (address >= base) {
        offset = address - base;
        if (offset < s->binding_capacity * sizeof(*s->bindings) &&
            offset % sizeof(*s->bindings) == 0)
            b = &s->bindings[offset / sizeof(*s->bindings)];
    }
    if (!b || b->source != s || b->source_incarnation != s->source_incarnation ||
        !b->incarnation || b->incarnation > s->next_incarnation || b->object != object ||
        b->process != omCurrentProcess || b->kind != kind || b->index >= 5 ||
        b->field != &row(s,kind,b->index)->unk_00 || *b->field != object)
        isle_sound_ramp_checked(s,ISLE_GAME_INVALID_STATE);
    isle_sound_ramp_checked(s,isle_game_object_metadata(s->game,object,&metadata));
    if (metadata.id != 14 || metadata.update_link != 0 || metadata.priority != UINT32_C(0x80000000))
        isle_sound_ramp_checked(s,ISLE_GAME_INVALID_STATE);
    return b;
}
/* Native publication errors do not roll back Original scalar stores. They do
 * request actual object teardown while preserving the registered identity and
 * callback metadata until real join plus the outside-OM release. */
static void failed_publication(IsleSoundRampSource* s, IsleSoundRampBinding* b, IsleGameResult failure) {
    if (b->object) {
        IsleGameResult cleanup = s->native.erase_object(s,b);
        if (failure == ISLE_GAME_OK) failure = cleanup;
    } else {
        *b = (IsleSoundRampBinding){0};
    }
    isle_sound_ramp_checked(s,failure == ISLE_GAME_OK ? ISLE_GAME_INVALID_STATE : failure);
}
IsleGameObject* isle_sound_ramp_add_object(IsleSoundRampSource* s, IsleSoundRampKind kind, uint32_t index) {
    IsleSoundRampBinding* b = NULL;
    IsleSoundRampRow* r;
    IsleGameResult result;
    size_t i;
    isle_sound_ramp_require(s);
    r = row(s,kind,index);
    if (!r || r->unk_00) isle_sound_ramp_checked(s,ISLE_GAME_INVALID_STATE);
    for (i = 0; i < s->binding_capacity; ++i)
        if (!s->bindings[i].object && !s->bindings[i].source) { b = &s->bindings[i]; break; }
    if (!b || s->next_incarnation == UINT64_MAX) isle_sound_ramp_checked(s,ISLE_GAME_EXHAUSTED);
    *b = (IsleSoundRampBinding){0};
    b->source = s;
    b->source_incarnation = s->source_incarnation;
    b->incarnation = ++s->next_incarnation;
    b->kind = kind;
    b->index = index;
    b->field = &r->unk_00;
    result = s->native.publish_object(s,b);
    if (result != ISLE_GAME_OK) failed_publication(s,b,result);
    if (!b->object || b->object->game != s->game || !b->object->alive ||
        b->object->retirementStarted || b->object->sourcePayloadKind != ISLE_SOURCE_PAYLOAD_NONE ||
        b->object->userData || b->object->fnUpdate || b->object->update || b->object->processListHead)
        failed_publication(s,b,ISLE_GAME_INVALID_STATE);
    return b->object;
}
void isle_sound_ramp_reconcile(IsleSoundRampSource* s, IsleSoundRampKind kind, uint32_t index,
    IsleGameObject* object, IsleGameObject* previous) {
    IsleSoundRampBinding* b;
    IsleGameResult result;
    /* In particular, this is NOT require(s): teardown is legal after failure
     * and after the current object's deletion has been requested. */
    isle_sound_ramp_checked(s,access_source(s,0));
    b = find_binding(s,kind,index,object);
    if (!b || (*b->field != NULL && *b->field != object) ||
        (previous != NULL && previous != object)) isle_sound_ramp_checked(s,ISLE_GAME_INVALID_STATE);
    result = s->native.reconcile_field(s,b,previous);
    if (result != ISLE_GAME_OK) failed_publication(s,b,result);
}
void isle_sound_ramp_create_process(IsleSoundRampSource* s, IsleSoundRampKind kind, uint32_t index,
    IsleGameObject* object, IsleGameSourceCallback callback) {
    IsleSoundRampBinding* b;
    IsleGameProcess* process = NULL;
    IsleGameResult result;
    IsleGameSourceCallback expected;
    isle_sound_ramp_require(s);
    b = find_binding(s,kind,index,object);
    expected = kind == ISLE_SOUND_RAMP_PAN ? isle_sound_ramp_func_800AB240 : isle_sound_ramp_func_800AB518;
    if (!b || b->process || *b->field != object || callback != expected)
        isle_sound_ramp_checked(s,ISLE_GAME_INVALID_STATE);
    result = isle_game_add_source_process(s->game,object,callback,ISLE_GAME_STACKFUL,1,&process);
    if (result != ISLE_GAME_OK) failed_publication(s,b,result);
    /* No scheduler transfer/callback occurs between publication and this typed
     * native context assignment. The actual C function remains process entry. */
    process->user = b;
    b->process = process;
}
void isle_sound_ramp_delete(IsleSoundRampSource* s, IsleSoundRampBinding* b) {
    if (!b || b->source != s || b->object != omCurrentObject || b->process != omCurrentProcess)
        isle_sound_ramp_checked(s,ISLE_GAME_INVALID_STATE);
    isle_sound_ramp_checked(s,s->native.erase_object(s,b));
}
void isle_sound_ramp_wait(IsleSoundRampSource* s, uint32_t updates) {
    isle_sound_ramp_checked(s,isle_game_source_wait(s->game,updates));
}
static IsleGameResult call(IsleSoundRampSource* s, SoundRampOperation operation, int32_t slot,
    uint32_t from, uint32_t to, int32_t duration, int32_t* output) {
    SoundRampCallFrame frame;
    IsleGameResult result = access_source(s,1);
    if (result != ISLE_GAME_OK) return result;
    frame.previous = s->nonwaiting_call_frame;
    s->nonwaiting_call_frame = &frame;
    if (setjmp(frame.failure) == 0) {
        switch (operation) {
        case SOUND_RAMP_START_PAN:
            *output = isle_sound_ramp_func_800AB41C(s,slot,from,to,duration); break;
        case SOUND_RAMP_START_VOLUME:
            *output = isle_sound_ramp_func_800AB700(s,slot,from,to,duration); break;
        case SOUND_RAMP_FINISH_KIND: isle_sound_ramp_func_800AB7FC(s,slot); break;
        case SOUND_RAMP_FINISH_ALL: isle_sound_ramp_func_800AB874(s); break;
        case SOUND_RAMP_FADE_ALL: isle_sound_ramp_func_800AB918(s,duration); break;
        }
    }
    s->nonwaiting_call_frame = frame.previous;
    return s->status;
}
IsleGameResult isle_sound_ramp_start_pan(IsleSoundRampSource* s, int32_t slot, uint32_t from,
    uint32_t to, int32_t duration, int32_t* result) {
    if (!result) return ISLE_GAME_INVALID_ARGUMENT;
    *result = -1;
    return call(s,SOUND_RAMP_START_PAN,slot,from,to,duration,result);
}
IsleGameResult isle_sound_ramp_start_volume(IsleSoundRampSource* s, int32_t slot, uint32_t from,
    uint32_t to, int32_t duration, int32_t* result) {
    if (!result) return ISLE_GAME_INVALID_ARGUMENT;
    *result = -1;
    return call(s,SOUND_RAMP_START_VOLUME,slot,from,to,duration,result);
}
IsleGameResult isle_sound_ramp_finish_kind(IsleSoundRampSource* s, IsleSoundRampKind kind) {
    if (!valid_kind(kind)) return ISLE_GAME_INVALID_ARGUMENT;
    return call(s,SOUND_RAMP_FINISH_KIND,(int32_t)kind,0,0,0,NULL);
}
IsleGameResult isle_sound_ramp_finish_all(IsleSoundRampSource* s) {
    return call(s,SOUND_RAMP_FINISH_ALL,0,0,0,0,NULL);
}
IsleGameResult isle_sound_ramp_fade_all(IsleSoundRampSource* s, int32_t duration) {
    return call(s,SOUND_RAMP_FADE_ALL,0,0,0,duration,NULL);
}
size_t isle_sound_ramp_source_outstanding(const IsleSoundRampSource* s) {
    size_t i, count = 0;
    if (!s || !s->bindings) return 0;
    for (i = 0; i < s->binding_capacity; ++i)
        if (s->bindings[i].object || s->bindings[i].source) ++count;
    return count;
}
IsleGameResult isle_sound_ramp_source_stop(IsleSoundRampSource* s) {
    size_t i;
    IsleGameResult result = access_source(s,0), first = ISLE_GAME_OK;
    if (result != ISLE_GAME_OK) return result;
    for (i = 0; i < s->binding_capacity; ++i) {
        IsleSoundRampBinding* b = &s->bindings[i];
        if (!b->object || s->native.object_joined(s,b)) continue;
        result = s->native.erase_object(s,b);
        if (first == ISLE_GAME_OK && result != ISLE_GAME_OK) first = result;
    }
    return first;
}
IsleGameResult isle_sound_ramp_source_collect(IsleSoundRampSource* s) {
    size_t i;
    IsleGameResult result = access_source(s,0);
    if (result != ISLE_GAME_OK) return result;
    result = isle_game_require_idle(s->game);
    if (result != ISLE_GAME_OK) return result;
    if (s->nonwaiting_call_frame) return ISLE_GAME_INVALID_STATE;
    for (i = 0; i < s->binding_capacity; ++i) {
        IsleSoundRampBinding* b = &s->bindings[i];
        if (!b->object || !s->native.object_joined(s,b)) continue;
        /* A joined previous incarnation may share its field address with a
         * newer live one; only the previous identity must have lost its credit. */
        if (*b->field == b->object) return ISLE_GAME_INVALID_STATE;
        result = s->native.release_object(s,b);
        if (result != ISLE_GAME_OK) return result;
        *b = (IsleSoundRampBinding){0};
    }
    return ISLE_GAME_OK;
}
IsleGameResult isle_sound_ramp_source_reset(IsleSoundRampSource* s) {
    SoundRampCallFrame frame;
    size_t i;
    IsleGameResult result = access_source(s,0);
    if (result != ISLE_GAME_OK) return result;
    if (isle_sound_ramp_source_outstanding(s)) return ISLE_GAME_INVALID_STATE;
    for (i = 0; i < 5; ++i)
        if (s->D_800BEF60[i].unk_00 || s->D_800BEFD8[i].unk_00) return ISLE_GAME_INVALID_STATE;
    /* AB9A4 is also a real stackful cutscene operation, not an idle-only
     * teardown API. Its checked guard stays inside this returning C fence. */
    frame.previous = s->nonwaiting_call_frame;
    s->nonwaiting_call_frame = &frame;
    if (setjmp(frame.failure) == 0) {
        isle_sound_ramp_func_800AB9A4(s);
        result = ISLE_GAME_OK;
    } else result = s->status != ISLE_GAME_OK ? s->status : ISLE_GAME_EXECUTION_FAILURE;
    s->nonwaiting_call_frame = frame.previous;
    return result; /* Successful cleanup never clears the first Source fault. */
}
IsleGameResult isle_sound_ramp_source_close(IsleSoundRampSource* s) {
    IsleGameResult result = access_source(s,0);
    if (result != ISLE_GAME_OK) return result;
    result = isle_game_require_idle(s->game);
    if (result != ISLE_GAME_OK) return result;
    result = isle_sound_ramp_source_reset(s);
    if (result != ISLE_GAME_OK) return result;
    s->game->soundRampSource = NULL;
    s->game = NULL;
    s->owner = NULL;
    return ISLE_GAME_OK;
}

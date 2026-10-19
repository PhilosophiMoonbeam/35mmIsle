#ifndef ISLE_SOUND_RAMP_SOURCE_H
#define ISLE_SOUND_RAMP_SOURCE_H

#include "35mmisle/engine.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct IsleSoundRampSource IsleSoundRampSource;
typedef struct IsleSoundRampBinding IsleSoundRampBinding;

typedef enum IsleSoundRampKind {
    ISLE_SOUND_RAMP_VOLUME = 0,
    ISLE_SOUND_RAMP_PAN = 1
} IsleSoundRampKind;

/* The one canonical native representation of original Struct_800BEFD8. These
 * are not console offsets: unk_00 is an actual native GObj* field. */
typedef struct IsleSoundRampRow {
    IsleGameObject* unk_00;
    int32_t unk_04;
    uint32_t unk_08, unk_0C, unk_10, unk_14;
} IsleSoundRampRow;

/* One physical native incarnation, never a second ramp/calendar state. The
 * supplied cold storage must cover held, actually joined incarnations as well
 * as the ten live rows. process is identity-only after native join. */
struct IsleSoundRampBinding {
    IsleSoundRampSource* source;
    IsleGameObject* object;
    IsleGameProcess* process;
    IsleGameObject** field;
    uint64_t source_incarnation, incarnation;
    uint32_t index;
    IsleSoundRampKind kind;
};

/* REQUIRED returning leaves, implemented by the actual Services owner. None
 * may wait, call Source checked(), invoke an Original callback, throw, transfer
 * continuations, or retain stack-owned state. No audio scheduler lives here. */
typedef struct IsleSoundRampNative {
    /* Create exactly id14/link0/priority0x80000000 with no updater. Before
     * exposing b->object, register it with Services::source_owner_register and
     * the stable one-element span &b->field, whose *field is currently NULL.
     * All pre-registration failure must really delete/join the unpublished
     * object and leave b->object NULL. On post-registration failure retain its
     * actual registered b->object for erase/join/collect; never hide publication.
     * Do not create a process: the private C helper does that afterwards. */
    IsleGameResult (*publish_object)(IsleSoundRampSource*, IsleSoundRampBinding*);
    /* Account the immediately preceding exact original C field store, using
     * Services' real canonical-field credits. This teardown-capable operation
     * must also work after self-delete was requested and after sticky failure;
     * it may not require healthy new-work admission. Never write *b->field a
     * second time. Self-delete still has an actual live current object here. */
    IsleGameResult (*reconcile_field)(IsleSoundRampSource*, IsleSoundRampBinding*, IsleGameObject* previous);
    /* Request real whole-object deletion through Services/native OM, including
     * partial publication and sticky-failure cleanup. Non-current deletion
     * actually joins; current deletion remains deferred until original C wait.
     * Pre-retirement closes any still-published canonical field and its credit.
     * Return errors without dropping b->object, identity hold or field credit. */
    IsleGameResult (*erase_object)(IsleSoundRampSource*, IsleSoundRampBinding*);
    /* Query Services::source_owner_joined, which only actual native join sets.
     * This must remain valid after sticky failure; do not dereference process. */
    int (*object_joined)(IsleSoundRampSource*, const IsleSoundRampBinding*);
    /* Outside ALL OM/callback/graphics borrowers only. Require actual joined,
     * source_owner_collectable and source_owner_release. Success releases the
     * registered native identity; caller then clears the binding exactly once. */
    IsleGameResult (*release_object)(IsleSoundRampSource*, IsleSoundRampBinding*);
    /* Call the existing Session::sound_pan / Session::sound_volume exactly
     * once, with (unsigned)slot and the original u32 value. Translate the actual
     * audio::Error to IsleGameResult, preserving failure in Services. No new
     * audio API, voice proxy, retrace, drain, interpolation or cached audio FSM. */
    IsleGameResult (*sound_pan)(IsleSoundRampSource*, int32_t slot, uint32_t value);
    IsleGameResult (*sound_volume)(IsleSoundRampSource*, int32_t slot, uint32_t value);
} IsleSoundRampNative;

struct IsleSoundRampSource {
    IsleGame* game;
    void* owner; /* actual Services, or its stable Services-owned implementation */
    IsleSoundRampNative native;
    IsleSoundRampRow D_800BEF60[5];
    IsleSoundRampRow D_800BEFD8[5];
    IsleSoundRampBinding* bindings;
    size_t binding_capacity;
    uint64_t source_incarnation, next_incarnation;
    void* nonwaiting_call_frame;
    IsleGameResult status;
    char failure[256];
};

IsleSoundRampSource* isle_sound_ramp_source_current(IsleGame*);
/* Initialize once, before any OM callback. Source, rows and binding storage
 * stay at fixed addresses for this one Services/Game lifetime. Pass zeroed
 * caller-owned storage, all required leaves and a nonzero lifetime identity.
 * Never reuse one universal course record or reinitialize an active Source. */
IsleGameResult isle_sound_ramp_source_initialize(IsleSoundRampSource*, IsleGame*, void* owner,
    uint64_t source_incarnation, IsleSoundRampNative, IsleSoundRampBinding*, size_t capacity);
/* These returning C fences are the only C++ calling surface. A full original
 * five-row calendar is status OK/result -1, not a native failure. The provider
 * adds no slot/duration/clamp policy; the imported Source owns its operations. */
IsleGameResult isle_sound_ramp_start_pan(IsleSoundRampSource*, int32_t slot, uint32_t from,
    uint32_t to, int32_t duration, int32_t* source_result);
IsleGameResult isle_sound_ramp_start_volume(IsleSoundRampSource*, int32_t slot, uint32_t from,
    uint32_t to, int32_t duration, int32_t* source_result);
IsleGameResult isle_sound_ramp_finish_kind(IsleSoundRampSource*, IsleSoundRampKind);
IsleGameResult isle_sound_ramp_finish_all(IsleSoundRampSource*);
IsleGameResult isle_sound_ramp_fade_all(IsleSoundRampSource*, int32_t duration);
/* Stop only requests genuine deletion. A current callback can still be live.
 * collect is outside OM and releases only independently proven joined owners.
 * reset refuses outstanding bindings/fields, then runs exact AB9A4 through a
 * returning C fence; legitimate stackful Source callers need not be idle. */
IsleGameResult isle_sound_ramp_source_stop(IsleSoundRampSource*);
IsleGameResult isle_sound_ramp_source_collect(IsleSoundRampSource*);
IsleGameResult isle_sound_ramp_source_reset(IsleSoundRampSource*);
size_t isle_sound_ramp_source_outstanding(const IsleSoundRampSource*);
IsleGameResult isle_sound_ramp_source_close(IsleSoundRampSource*);

/* Materialized exact Original C bodies. Native C Source callers may use them;
 * C++ callers use the returning fences above, never these checked functions. */
void isle_sound_ramp_func_800AB240(IsleGameObject*);
int32_t isle_sound_ramp_func_800AB41C(IsleSoundRampSource*, int32_t, uint32_t, uint32_t, int32_t);
void isle_sound_ramp_func_800AB518(IsleGameObject*);
int32_t isle_sound_ramp_func_800AB700(IsleSoundRampSource*, int32_t, uint32_t, uint32_t, int32_t);
void isle_sound_ramp_func_800AB7FC(IsleSoundRampSource*, int32_t);
void isle_sound_ramp_func_800AB874(IsleSoundRampSource*);
void isle_sound_ramp_func_800AB918(IsleSoundRampSource*, int32_t);
void isle_sound_ramp_func_800AB9A4(IsleSoundRampSource*);

/* Private C mechanics consumed by positive-count materializer rewrites. */
void isle_sound_ramp_require(IsleSoundRampSource*);
void isle_sound_ramp_require_reset(IsleSoundRampSource*);
void isle_sound_ramp_checked(IsleSoundRampSource*, IsleGameResult);
IsleSoundRampBinding* isle_sound_ramp_current(IsleGameObject*, IsleSoundRampKind);
IsleGameObject* isle_sound_ramp_add_object(IsleSoundRampSource*, IsleSoundRampKind, uint32_t index);
void isle_sound_ramp_reconcile(IsleSoundRampSource*, IsleSoundRampKind, uint32_t index,
    IsleGameObject*, IsleGameObject* previous);
void isle_sound_ramp_create_process(IsleSoundRampSource*, IsleSoundRampKind, uint32_t index,
    IsleGameObject*, IsleGameSourceCallback);
void isle_sound_ramp_delete(IsleSoundRampSource*, IsleSoundRampBinding*);
void isle_sound_ramp_wait(IsleSoundRampSource*, uint32_t);

#ifdef __cplusplus
}
#endif
#endif

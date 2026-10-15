#include "engine_internal.h"

#include <stddef.h>
#include <string.h>

GObj* omGObjListHead[32];
GObj* omGObjListTail[32];
GObjProcess* omGProcessList[12];
GObj* omCurrentObject;
GObjProcess* omCurrentProcess;
s32 omD_8004AC54;
static IsleGame* active_game;

static void diagnostic(IsleGame* game, const char* text) {
    size_t i = 0;
    if (text == game->error) return;
    if (text == NULL) text = "native game operation failed";
    while (i + 1 < sizeof(game->error) && text[i] != '\0') {
        game->error[i] = text[i];
        ++i;
    }
    game->error[i] = '\0';
}

static IsleGameResult access_game(IsleGame* game) {
    IsleContinuationResult result;
    if (game == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    if (!game->alive) return ISLE_GAME_INVALID_STATE;
    result = isle_continuation_domain_check_access(game->domain);
    if (result == ISLE_CONT_WRONG_OWNER) return ISLE_GAME_WRONG_OWNER;
    if (result != ISLE_CONT_OK || active_game != game) return ISLE_GAME_INVALID_STATE;
    return ISLE_GAME_OK;
}

/* On a worker, fail exits its own saved C frame before C++ transports failure.
 * On the owner, that API returns INVALID_STATE and the C-only tick frame is the
 * abort target. Never jump directly from a worker to the owner's environment.
 */
static void execution_fault(IsleGame* game, const char* text) {
    IsleContinuationResult result;
    diagnostic(game, text);
    game->fault = ISLE_GAME_EXECUTION_FAILURE;
    result = isle_continuation_fail(game->domain, game->error);
    if (result == ISLE_CONT_INVALID_STATE && game->ticking) {
        longjmp(game->fault_frame, 1);
    }
}

static void* allocate(IsleGame* game, size_t bytes, size_t alignment) {
    isle_span allocation;
    if (isle_arena_allocate(game->arena, bytes, alignment, &allocation) != ISLE_OK) {
        diagnostic(game, "native game arena exhausted");
        return NULL;
    }
    return allocation.data;
}

static bool valid_object(IsleGame* game, GObj* object) {
    return object != NULL && object->game == game && object->alive;
}

static bool valid_process(IsleGame* game, GObjProcess* process) {
    return process != NULL && process->game == game && process->alive;
}

static bool valid_sprite(IsleGame* game, SObj* sprite) {
    return sprite != NULL && sprite->game == game && sprite->alive;
}

static void update_entry(GObj* object) {
    object->update(object, object->userData);
}

static void synchronous_entry(GObj* object) {
    GObjProcess* process = omCurrentProcess;
    process->entry(object, process->user);
}

static void stackful_entry(void* user) {
    GObjProcess* process = user;
    process->entry(process->object, process->user);
}

IsleGameResult isle_game_create(isle_arena* arena, IsleGame** output) {
    isle_arena candidate;
    isle_span storage;
    IsleContinuationDomain* domain = NULL;
    IsleGame* game;
    if (arena == NULL || output == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    if (active_game != NULL) return ISLE_GAME_INVALID_STATE;
    candidate = *arena;
    if (isle_arena_allocate(&candidate, sizeof(IsleGame), _Alignof(IsleGame), &storage) != ISLE_OK) {
        return ISLE_GAME_EXHAUSTED;
    }
    if (isle_continuation_domain_create(&domain) != ISLE_CONT_OK) return ISLE_GAME_EXHAUSTED;
    game = (void*) storage.data;
    *game = (IsleGame){0};
    game->arena = arena;
    game->domain = domain;
    game->alive = true;
    for (size_t i = 0; i < ARRAY_COUNT(omGObjListHead); ++i) {
        omGObjListHead[i] = NULL;
        omGObjListTail[i] = NULL;
    }
    for (size_t i = 0; i < ARRAY_COUNT(omGProcessList); ++i) omGProcessList[i] = NULL;
    omCurrentObject = NULL;
    omCurrentProcess = NULL;
    omD_8004AC54 = 0;
    *arena = candidate;
    active_game = game;
    *output = game;
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_tick(IsleGame* game) {
    IsleGameResult result = access_game(game);
    if (result != ISLE_GAME_OK) return result;
    if (game->ticking) return ISLE_GAME_INVALID_STATE;
    if (game->fault != ISLE_GAME_OK) return game->fault;
    game->ticking = true;
    if (setjmp(game->fault_frame) == 0) omUpdateAll();
    game->ticking = false;
    omCurrentObject = NULL;
    omCurrentProcess = NULL;
    omD_8004AC54 = 0;
    return game->fault;
}

IsleGameResult isle_game_counts(IsleGame* game, IsleGameCounts* output) {
    IsleGameResult result = access_game(game);
    if (result != ISLE_GAME_OK) return result;
    if (output == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    *output = game->counts;
    return ISLE_GAME_OK;
}

const char* isle_game_error(const IsleGame* game) {
    if (game == NULL) return "null native game";
    if (!game->alive) return "native game destroyed";
    if (isle_continuation_domain_check_access(game->domain) != ISLE_CONT_OK) return "wrong native game owner";
    return game->error;
}

IsleGameResult isle_game_add_object(IsleGame* game, uint32_t id, uint8_t link,
                                   uint32_t priority, IsleGameCallback update,
                                   void* user, IsleGameObject** output) {
    GObj* object;
    IsleGameResult result = access_game(game);
    if (result != ISLE_GAME_OK) return result;
    if (output == NULL || link >= ARRAY_COUNT(omGObjListHead)) return ISLE_GAME_INVALID_ARGUMENT;
    if (game->fault != ISLE_GAME_OK) return game->fault;
    object = game->free_objects;
    if (object != NULL) game->free_objects = object->next;
    else object = allocate(game, sizeof(*object), _Alignof(GObj));
    if (object == NULL) return ISLE_GAME_EXHAUSTED;
    *object = (GObj){0};
    object->id = id;
    object->link = link;
    object->priority = priority;
    object->update = update;
    object->fnUpdate = update != NULL ? update_entry : NULL;
    object->userData = user;
    object->game = game;
    object->alive = true;
    omLinkGObjAfterSamePriority(object);
    ++game->counts.objects;
    *output = object;
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_pause_updates(IsleGame* game, IsleGameObject* object, int paused) {
    IsleGameResult result = access_game(game);
    if (result != ISLE_GAME_OK) return result;
    if (object == NULL) object = omCurrentObject;
    if (!valid_object(game, object)) return ISLE_GAME_INVALID_ARGUMENT;
    if (paused) object->flags |= GOBJ_FLAG_40;
    else object->flags &= ~GOBJ_FLAG_40;
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_object_id(IsleGame* game, IsleGameObject* object, uint32_t* output) {
    IsleGameResult result = access_game(game);
    if (result != ISLE_GAME_OK) return result;
    if (object == NULL) object = omCurrentObject;
    if (!valid_object(game, object) || output == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    *output = object->id;
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_add_process(IsleGame* game, IsleGameObject* object,
                                    IsleGameCallback entry, void* user,
                                    IsleGameProcessKind kind, uint32_t priority,
                                    IsleGameProcess** output) {
    GObjProcess* process;
    IsleContinuationResult created;
    IsleGameResult result = access_game(game);
    if (result != ISLE_GAME_OK) return result;
    if (object == NULL) object = omCurrentObject;
    if (!valid_object(game, object) || entry == NULL || output == NULL || priority >= ARRAY_COUNT(omGProcessList)
        || (kind != ISLE_GAME_STACKFUL && kind != ISLE_GAME_SYNCHRONOUS)) return ISLE_GAME_INVALID_ARGUMENT;
    if (game->fault != ISLE_GAME_OK) return game->fault;
    process = game->free_processes;
    if (process != NULL) game->free_processes = process->next;
    else process = allocate(game, sizeof(*process), _Alignof(GObjProcess));
    if (process == NULL) return ISLE_GAME_EXHAUSTED;
    *process = (GObjProcess){0};
    process->game = game;
    process->object = object;
    process->entry = entry;
    process->user = user;
    process->kind = (u8) kind;
    process->priority = (s32) priority;
    if (kind == ISLE_GAME_STACKFUL) {
        created = isle_continuation_create(game->domain, stackful_entry, process, &process->continuation);
        if (created != ISLE_CONT_OK) {
            diagnostic(game, isle_continuation_error(game->domain));
            process->next = game->free_processes;
            game->free_processes = process;
            return created == ISLE_CONT_RESOURCE_FAILURE ? ISLE_GAME_EXHAUSTED : ISLE_GAME_EXECUTION_FAILURE;
        }
    } else process->unk_1C.cb = synchronous_entry;
    process->alive = true;
    omLinkProcess(process);
    ++game->counts.processes;
    *output = process;
    return ISLE_GAME_OK;
}

void omEndProcess(GObjProcess* process) {
    IsleGame* game = active_game;
    if (process == NULL || process == omCurrentProcess) {
        omD_8004AC54 = 1;
        if (omCurrentProcess != NULL && omCurrentProcess->kind == ISLE_GAME_STACKFUL) {
            if (isle_continuation_finish(game->domain) != ISLE_CONT_OK) {
                execution_fault(game, "stackful self-end outside its active C continuation");
            }
        }
        return;
    }
    if (process->continuation != NULL) {
        if (isle_continuation_destroy(process->continuation) != ISLE_CONT_OK) {
            execution_fault(game, isle_continuation_error(game->domain));
            return;
        }
        process->continuation = NULL;
    }
    omUnlinkProcess(process);
    process->alive = false;
    process->next = game->free_processes;
    game->free_processes = process;
    --game->counts.processes;
}

IsleGameResult isle_game_end_process(IsleGame* game, IsleGameProcess* process) {
    IsleGameResult result = access_game(game);
    if (result != ISLE_GAME_OK) return result;
    if (process == NULL) process = omCurrentProcess;
    if (!valid_process(game, process)) return ISLE_GAME_INVALID_ARGUMENT;
    omEndProcess(process);
    return game->fault;
}

IsleGameResult isle_game_pause_process(IsleGame* game, IsleGameProcess* process, int paused) {
    IsleGameResult result = access_game(game);
    if (result != ISLE_GAME_OK) return result;
    if (process == NULL) process = omCurrentProcess;
    if (!valid_process(game, process)) return ISLE_GAME_INVALID_ARGUMENT;
    process->paused = paused ? 1 : 0;
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_wait(IsleGame* game, uint32_t updates) {
    IsleGameResult result = access_game(game);
    if (result != ISLE_GAME_OK) return result;
    if (!game->ticking || omCurrentProcess == NULL || omCurrentProcess->kind != ISLE_GAME_STACKFUL) return ISLE_GAME_INVALID_STATE;
    if (isle_continuation_wait(game->domain, updates) != ISLE_CONT_OK) return ISLE_GAME_EXECUTION_FAILURE;
    return ISLE_GAME_OK;
}

void ohWait(s32 updates) {
    if (updates < 0 || isle_game_wait(active_game, (uint32_t) updates) != ISLE_GAME_OK) {
        execution_fault(active_game, "invalid wait from selected C game code");
    }
}

void isle_native_resume_process(GObjProcess* process) {
    IsleContinuationState state;
    if (isle_continuation_resume(process->continuation) != ISLE_CONT_OK
        || isle_continuation_state(process->continuation, &state) != ISLE_CONT_OK) {
        execution_fault(process->game, isle_continuation_error(process->game->domain));
        return;
    }
    if (state == ISLE_CONT_FAILED || state == ISLE_CONT_CANCELLED) {
        execution_fault(process->game, "unexpected terminal state of selected C game process");
    } else if (state == ISLE_CONT_COMPLETED && omD_8004AC54 == 0) omD_8004AC54 = 1;
}

SObj* omGetSObj(void) {
    IsleGame* game = active_game;
    SObj* sprite = game->free_sprites;
    if (sprite != NULL) game->free_sprites = sprite->nextFree;
    else sprite = allocate(game, sizeof(*sprite), _Alignof(SObj));
    if (sprite == NULL) return NULL;
    *sprite = (SObj){0};
    sprite->game = game;
    sprite->alive = true;
    ++game->counts.sprites;
    return sprite;
}

void omFreeSObj(SObj* sprite) {
    IsleGame* game = sprite->game;
    sprite->alive = false;
    sprite->nextFree = game->free_sprites;
    game->free_sprites = sprite;
    --game->counts.sprites;
}

IsleGameResult isle_game_add_sprite(IsleGame* game, IsleGameObject* object,
                                   const IsleSprite* initial, IsleGameSprite** output) {
    SObj* sprite;
    IsleGameResult result = access_game(game);
    if (result != ISLE_GAME_OK) return result;
    if (object == NULL) object = omCurrentObject;
    if (!valid_object(game, object) || output == NULL || (object->type != 0 && object->type != 2)) return ISLE_GAME_INVALID_ARGUMENT;
    if (game->fault != ISLE_GAME_OK) return game->fault;
    sprite = omGObjAddSprite(object, initial);
    if (sprite == NULL) return ISLE_GAME_EXHAUSTED;
    *output = sprite;
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_remove_sprite(IsleGame* game, IsleGameSprite* sprite) {
    IsleGameResult result = access_game(game);
    if (result != ISLE_GAME_OK) return result;
    if (!valid_sprite(game, sprite)) return ISLE_GAME_INVALID_ARGUMENT;
    omGObjRemoveSprite(sprite);
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_sprite_view(IsleGame* game, IsleGameSprite* sprite, IsleSprite** output) {
    IsleGameResult result = access_game(game);
    if (result != ISLE_GAME_OK) return result;
    if (!valid_sprite(game, sprite) || output == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    *output = &sprite->sprite;
    return ISLE_GAME_OK;
}

void omDeleteGObj(GObj* object) {
    IsleGame* game = active_game;
    if (object == NULL || object == omCurrentObject) {
        omD_8004AC54 = 2;
        return;
    }
    while (object->processListHead != NULL) {
        GObjProcess* before = object->processListHead;
        omEndProcess(before);
        if (before == object->processListHead) return;
    }
    while (object->data.sobj != NULL) omGObjRemoveSprite(object->data.sobj);
    omUnlinkGObj(object);
    object->alive = false;
    object->next = game->free_objects;
    game->free_objects = object;
    --game->counts.objects;
}

IsleGameResult isle_game_delete_object(IsleGame* game, IsleGameObject* object) {
    IsleGameResult result = access_game(game);
    if (result != ISLE_GAME_OK) return result;
    if (object == NULL) object = omCurrentObject;
    if (!valid_object(game, object)) return ISLE_GAME_INVALID_ARGUMENT;
    omDeleteGObj(object);
    return game->fault;
}

static void fade_entry(GObj* object, void* user) {
    GObjProcess* process = user;
    (void) object;
    func_800E1958_AA0708(process->fade_sprite, process->fade_updates);
}

IsleGameResult isle_game_add_intro_fade(IsleGame* game, IsleGameObject* object,
                                       IsleGameSprite* sprite, uint16_t updates,
                                       uint32_t priority, IsleGameProcess** output) {
    GObjProcess* process;
    IsleGameResult result = access_game(game);
    if (result != ISLE_GAME_OK) return result;
    if (object == NULL) object = omCurrentObject;
    if (!valid_object(game, object) || !valid_sprite(game, sprite) || sprite->obj != object || output == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    result = isle_game_add_process(game, object, fade_entry, NULL, ISLE_GAME_STACKFUL, priority, &process);
    if (result != ISLE_GAME_OK) return result;
    process->user = process;
    process->fade_sprite = sprite;
    process->fade_updates = updates;
    *output = process;
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_destroy(IsleGame* game) {
    IsleGameResult result = access_game(game);
    if (result != ISLE_GAME_OK) return result;
    if (game->ticking) return ISLE_GAME_INVALID_STATE;
    for (size_t link = 0; link < ARRAY_COUNT(omGObjListHead); ++link) {
        while (omGObjListHead[link] != NULL) {
            GObj* before = omGObjListHead[link];
            omDeleteGObj(before);
            if (before == omGObjListHead[link]) return ISLE_GAME_EXECUTION_FAILURE;
        }
    }
    if (isle_continuation_domain_destroy(game->domain) != ISLE_CONT_OK) {
        diagnostic(game, isle_continuation_error(game->domain));
        return ISLE_GAME_EXECUTION_FAILURE;
    }
    game->domain = NULL;
    game->alive = false;
    active_game = NULL;
    return ISLE_GAME_OK;
}

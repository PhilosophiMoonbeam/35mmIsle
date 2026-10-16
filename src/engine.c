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
static void remove_camera(OMCamera* camera);

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
    object->update(object, object->updateUser);
}

static void synchronous_entry(GObj* object) {
    GObjProcess* process = omCurrentProcess;
    if (process->function != NULL) process->function(object);
    else process->entry(object, process->user);
}

static void stackful_entry(void* user) {
    GObjProcess* process = user;
    if (process->function != NULL) process->function(process->object);
    else process->entry(process->object, process->user);
}

IsleGameResult isle_game_create(isle_arena* arena, IsleGame** output) {
    isle_arena candidate;
    isle_span storage;
    IsleContinuationDomain* domain = NULL;
    IsleGame* game;
    if (output != NULL) *output = NULL;
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
    if (output != NULL) *output = NULL;
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
    object->updateUser = user;
    object->game = game;
    object->alive = true;
    omLinkGObjAfterSamePriority(object);
    ++game->counts.objects;
    *output = object;
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_object_metadata(IsleGame* game, IsleGameObject* object, IsleGameObjectMetadata* output) {
    IsleGameResult result = access_game(game);
    if (output != NULL) *output = (IsleGameObjectMetadata){0};
    if (result != ISLE_GAME_OK) return result;
    if (object == NULL) object = omCurrentObject;
    if (!valid_object(game, object) || output == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    *output = (IsleGameObjectMetadata){object->id, object->priority, object->flags, object->link};
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_object_set_flags(IsleGame* game, IsleGameObject* object, uint32_t flags) {
    IsleGameResult result = access_game(game);
    if (result != ISLE_GAME_OK) return result;
    if (object == NULL) object = omCurrentObject;
    if (!valid_object(game, object)) return ISLE_GAME_INVALID_ARGUMENT;
    object->flags = flags;
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_object_set_retire_callback(IsleGame* game, IsleGameObject* object,
    IsleGameCallback callback, void* user) {
    IsleGameResult result = access_game(game);
    if (result != ISLE_GAME_OK) return result;
    if (object == NULL) object = omCurrentObject;
    if (!valid_object(game, object)) return ISLE_GAME_INVALID_ARGUMENT;
    object->retired = callback;
    object->retireUser = user;
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

static IsleGameResult add_process(IsleGame* game, IsleGameObject* object,
                                 IsleGameCallback entry, void* user,
                                 IsleGameSourceCallback function,
                                 IsleGameProcessKind kind, uint32_t priority,
                                 IsleGameProcess** output) {
    GObjProcess* process;
    IsleContinuationResult created;
    if (output != NULL) *output = NULL;
    IsleGameResult result = access_game(game);
    if (result != ISLE_GAME_OK) return result;
    if (object == NULL) object = omCurrentObject;
    if (!valid_object(game, object) || (entry == NULL && function == NULL) || output == NULL || priority >= ARRAY_COUNT(omGProcessList)
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
    process->function = function;
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

IsleGameResult isle_game_add_process(IsleGame* game, IsleGameObject* object,
                                    IsleGameCallback entry, void* user,
                                    IsleGameProcessKind kind, uint32_t priority,
                                    IsleGameProcess** output) {
    return add_process(game, object, entry, user, NULL, kind, priority, output);
}

IsleGameResult isle_game_add_source_process(IsleGame* game, IsleGameObject* object,
    IsleGameSourceCallback function, IsleGameProcessKind kind, uint32_t priority,
    IsleGameProcess** output) {
    return add_process(game, object, NULL, NULL, function, kind, priority, output);
}

IsleGameResult isle_game_process_source_function(IsleGame* game, IsleGameProcess* process,
    IsleGameSourceCallback* output) {
    IsleGameResult result = access_game(game);
    if (output != NULL) *output = NULL;
    if (result != ISLE_GAME_OK) return result;
    if (process == NULL) process = omCurrentProcess;
    if (!valid_process(game, process) || output == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    *output = process->function;
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

IsleGameResult isle_game_end_source_processes(IsleGame* game, IsleGameObject* object,
    IsleGameSourceCallback function) {
    GObjProcess* process;
    IsleGameResult result = access_game(game);
    if (result != ISLE_GAME_OK) return result;
    if (object == NULL) object = omCurrentObject;
    if (!valid_object(game, object) || function == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    /* GAME items.c:77-84 saves the next object-local link before termination.
     * A stackful self-end deliberately never returns into this C frame. */
    process = object->processListHead;
    while (process != NULL) {
        GObjProcess* next = process->next;
        if (process->function == function) {
            omEndProcess(process);
            if (game->fault != ISLE_GAME_OK) return game->fault;
        }
        process = next;
    }
    return ISLE_GAME_OK;
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
    if (output != NULL) *output = NULL;
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
    if (output != NULL) *output = NULL;
    IsleGameResult result = access_game(game);
    if (result != ISLE_GAME_OK) return result;
    if (!valid_sprite(game, sprite) || output == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    *output = &sprite->sprite;
    return ISLE_GAME_OK;
}

static IsleGameResult access_camera(IsleGame* game, OMCamera* camera) {
    IsleGameResult result = access_game(game);
    if (result != ISLE_GAME_OK) return result;
    if (camera == NULL || !camera->alive || camera->game != game ||
        !valid_object(game, camera->obj) || camera->obj->type != 3 ||
        camera->obj->data.cam != camera) return ISLE_GAME_INVALID_ARGUMENT;
    return game->fault;
}

static void remove_camera(OMCamera* camera) {
    IsleGame* game = camera->game;
    camera->obj->type = 0;
    camera->obj->data.any = NULL;
    for (size_t i = 0; i < camera->numMatrices; ++i) {
        OMMtx* matrix = camera->matrices[i];
        matrix->nextFree = game->free_matrices;
        game->free_matrices = matrix;
        --game->counts.matrices;
    }
    camera->numMatrices = 0;
    camera->alive = false;
    camera->nextFree = game->free_cameras;
    game->free_cameras = camera;
    --game->counts.cameras;
}

IsleGameResult isle_game_attach_camera(IsleGame* game, IsleGameObject* object,
                                      IsleGameCamera** output) {
    OMCamera* camera;
    IsleGameResult result;
    if (output != NULL) *output = NULL;
    result = access_game(game);
    if (result != ISLE_GAME_OK) return result;
    if (object == NULL) object = omCurrentObject;
    if (output == NULL || !valid_object(game, object)) return ISLE_GAME_INVALID_ARGUMENT;
    if (game->fault != ISLE_GAME_OK) return game->fault;
    if (object->type != 0 || object->data.any != NULL) return ISLE_GAME_INVALID_STATE;
    camera = game->free_cameras;
    if (camera != NULL) game->free_cameras = camera->nextFree;
    else camera = allocate(game, sizeof(*camera), _Alignof(OMCamera));
    if (camera == NULL) return ISLE_GAME_EXHAUSTED;
    *camera = (OMCamera){0};
    camera->obj = object;
    camera->game = game;
    camera->alive = true;
    /* Native ownership initialization, following pinned GAME om.c:63,65 and
     * rdp_reset.c:58-62. These are not admitted GAME ownership/render bodies.
     */
    camera->parameters.perspective = (IsleGamePerspective){30.0f, 4.0f / 3.0f, 100.0f, 12800.0f, 1.0f};
    camera->parameters.look_at = (IsleGameLookAt){{1500.0f, 0, 0}, {0, 0, 0}, {0, 0, 1.0f}};
    camera->parameters.viewport = (IsleGameViewport){{640, 480, 511, 0}, {640, 480, 511, 0}};
    object->type = 3;
    object->data.cam = camera;
    ++game->counts.cameras;
    *output = camera;
    return ISLE_GAME_OK;
}

static bool valid_camera_pose(const IsleGameLookAt* p, bool require_y_up, bool require_supplied_up) {
    float look[3], right[3], length = 0, cross_length = 0, inverse_length;
    if (p == NULL) return false;
    for (size_t i = 0; i < 3; ++i) {
        if (!isfinite(p->eye[i]) || !isfinite(p->at[i]) || !isfinite(p->up[i])) return false;
        look[i] = p->at[i] - p->eye[i];
        length += look[i] * look[i];
    }
    if (!isfinite(length) || length <= 0) return false;
    inverse_length = -1.0f / sqrtf(length);
    if (!isfinite(inverse_length)) return false;
    for (size_t i = 0; i < 3; ++i) look[i] *= inverse_length;
    /* render.c kind 8 supplies (0,1,0), independently of look_at.up. Keep
     * its normalization away from a zero/underflowed horizontal basis. */
    if (require_y_up) {
        float horizontal = look[0] * look[0] + look[2] * look[2];
        if (!isfinite(horizontal) || horizontal <= 0 ||
            !isfinite(1.0f / sqrtf(horizontal))) return false;
    }
    /* Kind 8 never consumes look_at.up. A valid fixed-Y basis must not be
     * rejected merely because that separate, unused vector is collinear. */
    if (!require_supplied_up) return true;
    right[0] = p->up[1] * look[2] - p->up[2] * look[1];
    right[1] = p->up[2] * look[0] - p->up[0] * look[2];
    right[2] = p->up[0] * look[1] - p->up[1] * look[0];
    for (size_t i = 0; i < 3; ++i) cross_length += right[i] * right[i];
    return isfinite(cross_length) && cross_length > 0 &&
        isfinite(1.0f / sqrtf(cross_length));
}

IsleGameResult isle_game_camera_add_matrix(IsleGame* game, IsleGameCamera* camera,
                                          IsleGameCameraMatrixKind kind) {
    OMMtx* matrix;
    IsleGameResult result = access_camera(game, camera);
    if (result != ISLE_GAME_OK) return result;
    if (kind != ISLE_GAME_CAMERA_PERSPECTIVE_FAST && kind != ISLE_GAME_CAMERA_LOOK_AT &&
        kind != ISLE_GAME_CAMERA_LOOK_AT_ROLL && kind != ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL)
        return ISLE_GAME_INVALID_ARGUMENT;
    if (kind == ISLE_GAME_CAMERA_LOOK_AT_ROLL &&
        !valid_camera_pose(&camera->parameters.look_at, true, false)) return ISLE_GAME_INVALID_ARGUMENT;
    if ((kind == ISLE_GAME_CAMERA_LOOK_AT || kind == ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL) &&
        !valid_camera_pose(&camera->parameters.look_at, false, true)) return ISLE_GAME_INVALID_ARGUMENT;
    if (camera->numMatrices == ARRAY_COUNT(camera->matrices)) return ISLE_GAME_EXHAUSTED;
    matrix = game->free_matrices;
    if (matrix != NULL) game->free_matrices = matrix->nextFree;
    else matrix = allocate(game, sizeof(*matrix), _Alignof(OMMtx));
    if (matrix == NULL) return ISLE_GAME_EXHAUSTED;
    *matrix = (OMMtx){0};
    matrix->result.kind = kind;
    camera->matrices[camera->numMatrices++] = matrix;
    ++game->counts.matrices;
    return ISLE_GAME_OK;
}

static void invalidate_camera(OMCamera* camera, IsleGameCameraMatrixKind kind) {
    for (size_t i = 0; i < camera->numMatrices; ++i)
        if (camera->matrices[i]->result.kind == kind) camera->matrices[i]->cached = false;
}

IsleGameResult isle_game_camera_set_perspective(IsleGame* game, IsleGameCamera* camera,
                                               const IsleGamePerspective* p) {
    IsleGamePerspective* old;
    IsleGameResult result = access_camera(game, camera);
    if (result != ISLE_GAME_OK) return result;
    if (p == NULL || !isfinite(p->fovy) || !isfinite(p->aspect) ||
        !isfinite(p->near_plane) || !isfinite(p->far_plane) || !isfinite(p->scale) ||
        p->fovy <= 0 || p->fovy >= 180 || p->aspect <= 0 || p->near_plane <= 0 ||
        p->far_plane <= p->near_plane || p->scale <= 0 ||
        (s32)(p->fovy * 0.008726646f * 651.8986f) == 0)
        return ISLE_GAME_INVALID_ARGUMENT;
    old = &camera->parameters.perspective;
    if (old->fovy == p->fovy && old->aspect == p->aspect &&
        old->near_plane == p->near_plane && old->far_plane == p->far_plane &&
        old->scale == p->scale) return ISLE_GAME_OK;
    *old = *p;
    invalidate_camera(camera, ISLE_GAME_CAMERA_PERSPECTIVE_FAST);
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_camera_set_look_at(IsleGame* game, IsleGameCamera* camera,
                                           const IsleGameLookAt* p) {
    bool pose_changed = false, up_changed = false, require_y_up = false, require_supplied_up = false;
    IsleGameResult result = access_camera(game, camera);
    if (result != ISLE_GAME_OK) return result;
    if (p == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    for (size_t i = 0; i < 3; ++i) {
        if (p->eye[i] != camera->parameters.look_at.eye[i] ||
            p->at[i] != camera->parameters.look_at.at[i]) pose_changed = true;
        if (p->up[i] != camera->parameters.look_at.up[i]) up_changed = true;
    }
    if (!pose_changed && !up_changed) return ISLE_GAME_OK;
    for (size_t i = 0; i < camera->numMatrices; ++i) {
        const IsleGameCameraMatrixKind kind = camera->matrices[i]->result.kind;
        if (kind == ISLE_GAME_CAMERA_LOOK_AT_ROLL) require_y_up = true;
        if (kind == ISLE_GAME_CAMERA_LOOK_AT || kind == ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL)
            require_supplied_up = true;
    }
    if (!valid_camera_pose(p, require_y_up, require_supplied_up || !require_y_up))
        return ISLE_GAME_INVALID_ARGUMENT;
    camera->parameters.look_at = *p;
    invalidate_camera(camera, ISLE_GAME_CAMERA_LOOK_AT);
    if (pose_changed) invalidate_camera(camera, ISLE_GAME_CAMERA_LOOK_AT_ROLL);
    invalidate_camera(camera, ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL);
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_camera_set_roll(IsleGame* game, IsleGameCamera* camera, float radians) {
    IsleGameResult result = access_camera(game, camera);
    if (result != ISLE_GAME_OK) return result;
    if (!isfinite(radians)) return ISLE_GAME_INVALID_ARGUMENT;
    if (camera->roll == radians) return ISLE_GAME_OK;
    camera->roll = radians;
    invalidate_camera(camera, ISLE_GAME_CAMERA_LOOK_AT_ROLL);
    invalidate_camera(camera, ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL);
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_camera_roll(IsleGame* game, IsleGameCamera* camera, float* output) {
    IsleGameResult result;
    if (output != NULL) *output = 0;
    result = access_camera(game, camera);
    if (result != ISLE_GAME_OK) return result;
    if (output == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    *output = camera->roll;
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_camera_set_viewport(IsleGame* game, IsleGameCamera* camera,
                                            const IsleGameViewport* p) {
    IsleGameResult result = access_camera(game, camera);
    if (result != ISLE_GAME_OK) return result;
    if (p == NULL || p->scale[0] <= 0 || p->scale[1] <= 0 || p->scale[2] < 0 ||
        p->translate[2] < p->scale[2] || (int)p->translate[2] + p->scale[2] > 1023 ||
        p->scale[3] != 0 || p->translate[3] != 0) return ISLE_GAME_INVALID_ARGUMENT;
    camera->parameters.viewport = *p;
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_camera_parameters(IsleGame* game, IsleGameCamera* camera,
                                          const IsleGameCameraParameters** output) {
    IsleGameResult result;
    if (output != NULL) *output = NULL;
    result = access_camera(game, camera);
    if (result != ISLE_GAME_OK) return result;
    if (output == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    *output = &camera->parameters;
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_camera_matrix_count(IsleGame* game, IsleGameCamera* camera,
                                            size_t* output) {
    IsleGameResult result;
    if (output != NULL) *output = 0;
    result = access_camera(game, camera);
    if (result != ISLE_GAME_OK) return result;
    if (output == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    *output = camera->numMatrices;
    return ISLE_GAME_OK;
}

/* All source math and finite checks finish before any output/cache publication.
 * The same computation serves individual getters and atomic camera batches.
 */
static IsleGameResult compute_camera_matrix(const OMCamera* camera, const OMMtx* matrix,
    IsleGameCameraMatrix* output, IsleGameCameraReflection* reflected, u16* norm) {
    Mtx4f floating;
    LookAt reflection;
    u16 normalization = *norm;
    if (matrix->result.kind == ISLE_GAME_CAMERA_PERSPECTIVE_FAST) {
        const IsleGamePerspective* p = &camera->parameters.perspective;
        /* Keep invalid huge inputs away from GAME's float-to-u16 norm cast. */
        float sum = p->near_plane + p->far_plane;
        if (!isfinite(sum)) return ISLE_GAME_INVALID_ARGUMENT;
        hal_perspective_fast_f(floating, &normalization,
                              p->fovy, p->aspect, p->near_plane, p->far_plane, p->scale);
    } else {
        const IsleGameLookAt* p = &camera->parameters.look_at;
        if (matrix->result.kind == ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL) {
            if (!hal_look_at_reflect_roll_f(floating, &reflection,
                    p->eye[0], p->eye[1], p->eye[2], p->at[0], p->at[1], p->at[2],
                    camera->roll, p->up[0], p->up[1], p->up[2]))
                return ISLE_GAME_INVALID_ARGUMENT;
        } else if (matrix->result.kind == ISLE_GAME_CAMERA_LOOK_AT_ROLL) {
            /* GAME render.c:2294-2300, kind 8 (not kind 10/Z-up). */
            hal_look_at_roll_f(floating,
                p->eye[0], p->eye[1], p->eye[2], p->at[0], p->at[1], p->at[2],
                camera->roll, 0.0f, 1.0f, 0.0f);
        } else {
            hal_look_at_f(floating, p->eye[0], p->eye[1], p->eye[2],
                          p->at[0], p->at[1], p->at[2], p->up[0], p->up[1], p->up[2]);
        }
    }
    for (size_t row = 0; row < 4; ++row) {
        for (size_t col = 0; col < 4; ++col) {
            float value = floating[row][col] * 65536.0f;
            if (!isfinite(value) || value < -2147483648.0f || value >= 2147483648.0f)
                return ISLE_GAME_INVALID_ARGUMENT;
        }
    }
    output->kind = matrix->result.kind;
    output->revision = matrix->result.revision + 1;
    memcpy(output->floating, floating, sizeof(floating));
    hal_mtx_f2l(floating, &output->fixed);
    if (output->kind == ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL) {
        for (size_t axis = 0; axis < 2; ++axis) {
            memcpy(reflected->direction[axis], reflection.l[axis].l.dir, 3);
            memcpy(reflected->color[axis], reflection.l[axis].l.col, 3);
            memcpy(reflected->color_copy[axis], reflection.l[axis].l.colc, 3);
        }
        reflected->revision = output->revision;
    }
    *norm = normalization;
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_camera_compute_all(IsleGame* game, IsleGameCamera* camera) {
    struct {
        IsleGameCameraMatrix matrix;
        IsleGameCameraReflection reflection;
    } staged[2];
    IsleGameResult result = access_camera(game, camera);
    if (result != ISLE_GAME_OK) return result;
    u16 normalization = camera->normalization;
    for (size_t i = 0; i < camera->numMatrices; ++i) {
        const OMMtx* matrix = camera->matrices[i];
        if (matrix->cached) continue;
        result = compute_camera_matrix(camera, matrix, &staged[i].matrix, &staged[i].reflection, &normalization);
        if (result != ISLE_GAME_OK) return result;
    }
    for (size_t i = 0; i < camera->numMatrices; ++i) {
        OMMtx* matrix = camera->matrices[i];
        if (matrix->cached) continue;
        matrix->result = staged[i].matrix;
        if (matrix->result.kind == ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL)
            matrix->reflection = staged[i].reflection;
        matrix->cached = true;
    }
    camera->normalization = normalization;
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_camera_matrix(IsleGame* game, IsleGameCamera* camera,
                                      size_t index, const IsleGameCameraMatrix** output) {
    OMMtx* matrix;
    IsleGameResult result;
    if (output != NULL) *output = NULL;
    result = access_camera(game, camera);
    if (result != ISLE_GAME_OK) return result;
    if (output == NULL || index >= camera->numMatrices) return ISLE_GAME_INVALID_ARGUMENT;
    matrix = camera->matrices[index];
    if (!matrix->cached) {
        result = compute_camera_matrix(camera, matrix, &matrix->result, &matrix->reflection, &camera->normalization);
        if (result != ISLE_GAME_OK) return result;
        matrix->cached = true;
    }
    *output = &matrix->result;
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_camera_reflection(IsleGame* game, IsleGameCamera* camera,
                                          size_t index, const IsleGameCameraReflection** output) {
    const IsleGameCameraMatrix* computed;
    IsleGameResult result;
    if (output != NULL) *output = NULL;
    result = access_camera(game, camera);
    if (result != ISLE_GAME_OK) return result;
    if (output == NULL || index >= camera->numMatrices) return ISLE_GAME_INVALID_ARGUMENT;
    if (camera->matrices[index]->result.kind != ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL)
        return ISLE_GAME_INVALID_STATE;
    result = isle_game_camera_matrix(game, camera, index, &computed);
    if (result != ISLE_GAME_OK) return result;
    *output = &camera->matrices[index]->reflection;
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_camera_perspective_normalization(IsleGame* game,
                                                         IsleGameCamera* camera,
                                                         uint16_t* output) {
    const IsleGameCameraMatrix* matrix;
    IsleGameResult result;
    if (output != NULL) *output = 0;
    result = access_camera(game, camera);
    if (result != ISLE_GAME_OK) return result;
    if (output == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    for (size_t i = 0; i < camera->numMatrices; ++i) {
        if (camera->matrices[i]->result.kind != ISLE_GAME_CAMERA_PERSPECTIVE_FAST) continue;
        result = isle_game_camera_matrix(game, camera, i, &matrix);
        if (result == ISLE_GAME_OK) *output = camera->normalization;
        return result;
    }
    return ISLE_GAME_INVALID_STATE;
}

IsleGameResult isle_game_remove_camera(IsleGame* game, IsleGameCamera* camera) {
    IsleGameResult result = access_camera(game, camera);
    if (result != ISLE_GAME_OK) return result;
    remove_camera(camera);
    return ISLE_GAME_OK;
}

static unsigned char* model_dirty(IsleGameModel* model) {
    return (unsigned char*) (model->nodes + model->capacity);
}

static bool signed_word_range(double value) {
    return isfinite(value) && value >= -2147483648.0 && value < 2147483648.0;
}

static bool valid_model_rt_transform(const IsleGameModelTransform* t) {
    if (t == NULL) return false;
    /* Exact GAME ROM kind-28 MUL.S constant, observed at 0x80043DB0:
     * binary32 0x4422F983. Validate the same binary32 products as its casts.
     */
    const float angle_factor = 0x1.45f306p+9f;
    return signed_word_range(t->position.x * 65536.0f) &&
        signed_word_range(t->position.y * 65536.0f) &&
        signed_word_range(t->position.z * 65536.0f) &&
        signed_word_range(t->rotation.x * angle_factor) &&
        signed_word_range(t->rotation.y * angle_factor) &&
        signed_word_range(t->rotation.z * angle_factor);
}
static bool valid_model_transform(const IsleGameModelTransform* t) {
    return valid_model_rt_transform(t) &&
        signed_word_range(t->scale.x * 256.0f) &&
        signed_word_range(t->scale.y * 256.0f) &&
        signed_word_range(t->scale.z * 256.0f);
}
static bool valid_model_matrix_transform(const IsleGameModelTransform* t, IsleGameModelMatrixKind kind) {
    if (kind == ISLE_GAME_MODEL_ROTATE_RPY_TRANSLATE_SCALE) return valid_model_transform(t);
    if (kind != ISLE_GAME_MODEL_TRANSLATE || t == NULL) return false;
    return signed_word_range(t->position.x * 65536.0f) &&
        signed_word_range(t->position.y * 65536.0f) &&
        signed_word_range(t->position.z * 65536.0f) &&
        t->rotation.x == 0.0f && t->rotation.y == 0.0f && t->rotation.z == 0.0f &&
        t->scale.x == 1.0f && t->scale.y == 1.0f && t->scale.z == 1.0f;
}

static bool same_vec3(IsleGameVec3 a, IsleGameVec3 b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

static IsleGameResult access_model(IsleGame* game, IsleGameModel* model) {
    IsleGameResult result = access_game(game);
    if (result != ISLE_GAME_OK) return result;
    if (model == NULL || !model->alive || model->game != game ||
        !valid_object(game, model->obj) || model->obj->type != 1 ||
        model->obj->data.model != model) return ISLE_GAME_INVALID_ARGUMENT;
    return game->fault;
}

static void remove_model(IsleGameModel* model) {
    IsleGame* game = model->game;
    model->obj->type = 0;
    model->obj->data.any = NULL;
    --game->counts.models;
    game->counts.model_nodes -= model->count;
    game->counts.matrices -= model->count +
        (model->placement.mode != ISLE_GAME_MODEL_PLACEMENT_DISABLED);
    model->count = 0;
    model->alive = false;
    model->nextFree = game->free_models;
    game->free_models = model;
}

IsleGameResult isle_game_attach_model(IsleGame* game, IsleGameObject* object,
    const IsleGameModelNodeInit* initial, size_t count, IsleGameModel** output) {
    IsleGameModel* model;
    IsleGameModel** best = NULL;
    IsleGameResult result;
    if (output != NULL) *output = NULL;
    result = access_game(game);
    if (result != ISLE_GAME_OK) return result;
    if (object == NULL) object = omCurrentObject;
    if (output == NULL || !valid_object(game, object) || initial == NULL ||
        count == 0 || count > UINT32_MAX ||
        count > (SIZE_MAX - sizeof(IsleGameModel)) / (sizeof(IsleGameModelNode) + 1))
        return ISLE_GAME_INVALID_ARGUMENT;
    if (game->fault != ISLE_GAME_OK) return game->fault;
    if (object->type != 0 || object->data.any != NULL) return ISLE_GAME_INVALID_STATE;
    for (size_t i = 0; i < count; ++i) {
        if ((i == 0 ? initial[i].parent != UINT32_MAX : initial[i].parent >= i) ||
            !valid_model_matrix_transform(&initial[i].transform, initial[i].matrix_kind))
            return ISLE_GAME_INVALID_ARGUMENT;
    }
    /* Best-fit released block, or one actual-size allocation. All rejection
     * precedes free-list/arena mutation and object publication.
     */
    for (IsleGameModel** slot = &game->free_models; *slot != NULL; slot = &(*slot)->nextFree) {
        if ((*slot)->capacity >= count &&
            (best == NULL || (*slot)->capacity < (*best)->capacity)) best = slot;
    }
    if (best != NULL) {
        model = *best;
        *best = model->nextFree;
    } else {
        model = allocate(game, sizeof(*model) + count * (sizeof(model->nodes[0]) + 1),
                         _Alignof(IsleGameModel));
        if (model == NULL) return ISLE_GAME_EXHAUSTED;
        model->capacity = count;
    }
    model->nextFree = NULL;
    model->obj = object;
    model->game = game;
    model->count = count;
    model->alive = true;
    model->placement = (IsleGameModelPlacement){
        .mode = ISLE_GAME_MODEL_PLACEMENT_DISABLED, .transform.scale = {1, 1, 1}
    };
    for (size_t i = 0; i < count; ++i) {
        model->nodes[i] = (IsleGameModelNode){
            .transform = initial[i].transform, .parent = initial[i].parent,
            .first_child = UINT32_MAX, .next_sibling = UINT32_MAX,
            .matrix_kind = initial[i].matrix_kind
        };
        model_dirty(model)[i] = 1;
    }
    /* Reverse prepend preserves the original order, including geometry-less
     * nodes, in linear time without a second allocation of child-tail indices.
     */
    for (size_t i = count; i-- > 1;) {
        IsleGameModelNode* parent = &model->nodes[model->nodes[i].parent];
        model->nodes[i].next_sibling = parent->first_child;
        parent->first_child = (uint32_t) i;
    }
    object->type = 1;
    object->data.model = model;
    ++game->counts.models;
    game->counts.model_nodes += count;
    game->counts.matrices += count;
    *output = model;
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_remove_model(IsleGame* game, IsleGameModel* model) {
    IsleGameResult result = access_model(game, model);
    if (result != ISLE_GAME_OK) return result;
    remove_model(model);
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_model_validate_transform(IsleGame* game, IsleGameModel* model,
    size_t index, const IsleGameModelTransform* transform) {
    IsleGameResult result = access_model(game, model);
    if (result != ISLE_GAME_OK) return result;
    if (index >= model->count || !valid_model_matrix_transform(transform, model->nodes[index].matrix_kind))
        return ISLE_GAME_INVALID_ARGUMENT;
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_model_set_transform(IsleGame* game, IsleGameModel* model,
    size_t index, const IsleGameModelTransform* transform) {
    IsleGameResult result = isle_game_model_validate_transform(game, model, index, transform);
    if (result != ISLE_GAME_OK) return result;
    IsleGameModelTransform* old = &model->nodes[index].transform;
    if (same_vec3(old->position, transform->position) &&
        same_vec3(old->rotation, transform->rotation) &&
        same_vec3(old->scale, transform->scale)) return ISLE_GAME_OK;
    *old = *transform;
    model_dirty(model)[index] = 1;
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_model_validate_placement(IsleGame* game, IsleGameModel* model,
    IsleGameModelPlacementMode mode, const IsleGameModelTransform* transform) {
    IsleGameResult result = access_model(game, model);
    if (result != ISLE_GAME_OK) return result;
    if (mode == ISLE_GAME_MODEL_PLACEMENT_DISABLED) return ISLE_GAME_OK;
    if (mode == ISLE_GAME_MODEL_PLACEMENT_RTS)
        return valid_model_transform(transform) ? ISLE_GAME_OK : ISLE_GAME_INVALID_ARGUMENT;
    if (mode != ISLE_GAME_MODEL_PLACEMENT_RT || transform == NULL)
        return ISLE_GAME_INVALID_ARGUMENT;
    /* Never read RT's unused scale, including indeterminate source fields. */
    return valid_model_rt_transform(transform) ? ISLE_GAME_OK : ISLE_GAME_INVALID_ARGUMENT;
}

IsleGameResult isle_game_model_set_placement(IsleGame* game, IsleGameModel* model,
    IsleGameModelPlacementMode mode, const IsleGameModelTransform* transform) {
    IsleGameResult result = isle_game_model_validate_placement(game, model, mode, transform);
    if (result != ISLE_GAME_OK) return result;
    IsleGameModelPlacement next = {
        .mode = mode, .transform.scale = {1, 1, 1}
    };
    if (mode != ISLE_GAME_MODEL_PLACEMENT_DISABLED) {
        next.transform.position = transform->position;
        next.transform.rotation = transform->rotation;
        if (mode == ISLE_GAME_MODEL_PLACEMENT_RTS) next.transform.scale = transform->scale;
    }
    const IsleGameModelPlacement* old = &model->placement;
    if (old->mode == mode && same_vec3(old->transform.position, next.transform.position) &&
        same_vec3(old->transform.rotation, next.transform.rotation) &&
        same_vec3(old->transform.scale, next.transform.scale)) return ISLE_GAME_OK;
    const IsleGameModelTransform* t = &next.transform;
    if (mode == ISLE_GAME_MODEL_PLACEMENT_RT)
        hal_rotate_rpy_translate(&next.fixed, t->position.x, t->position.y, t->position.z,
            t->rotation.x, t->rotation.y, t->rotation.z);
    else if (mode == ISLE_GAME_MODEL_PLACEMENT_RTS)
        hal_rotate_rpy_translate_scale(&next.fixed, t->position.x, t->position.y, t->position.z,
            t->rotation.x, t->rotation.y, t->rotation.z, t->scale.x, t->scale.y, t->scale.z);
    next.revision = old->revision + 1;
    if (old->mode == ISLE_GAME_MODEL_PLACEMENT_DISABLED) ++game->counts.matrices;
    if (mode == ISLE_GAME_MODEL_PLACEMENT_DISABLED) --game->counts.matrices;
    model->placement = next;
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_model_placement(IsleGame* game, IsleGameModel* model,
    const IsleGameModelPlacement** output) {
    if (output != NULL) *output = NULL;
    IsleGameResult result = access_model(game, model);
    if (result != ISLE_GAME_OK) return result;
    if (output == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    *output = &model->placement;
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_model_set_flags(IsleGame* game, IsleGameModel* model,
    size_t index, uint32_t flags) {
    IsleGameResult result = access_model(game, model);
    if (result != ISLE_GAME_OK) return result;
    if (index >= model->count) return ISLE_GAME_INVALID_ARGUMENT;
    model->nodes[index].flags = flags;
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_model_nodes(IsleGame* game, IsleGameModel* model,
    const IsleGameModelNode** output, size_t* count) {
    IsleGameResult result;
    if (output != NULL) *output = NULL;
    if (count != NULL) *count = 0;
    result = access_model(game, model);
    if (result != ISLE_GAME_OK) return result;
    if (output == NULL || count == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    for (size_t i = 0; i < model->count; ++i) {
        if (!model_dirty(model)[i]) continue;
        IsleGameModelNode* node = &model->nodes[i];
        const IsleGameModelTransform* t = &node->transform;
        if (node->matrix_kind == ISLE_GAME_MODEL_TRANSLATE)
            hal_translate(&node->fixed, t->position.x, t->position.y, t->position.z);
        else
            hal_rotate_rpy_translate_scale(&node->fixed, t->position.x, t->position.y, t->position.z,
                t->rotation.x, t->rotation.y, t->rotation.z, t->scale.x, t->scale.y, t->scale.z);
        ++node->revision;
        model_dirty(model)[i] = 0;
    }
    *output = model->nodes;
    *count = model->count;
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_model_bake_photo_transform(
    const IsleGameModelTransform* unbaked_root, const IsleGameVec3* position,
    float yaw, float game_model_scale, IsleGameModelTransform* output) {
    /* Copy before clearing: output may alias the root or its position. */
    IsleGameModelTransform root = {0}, baked;
    IsleGameVec3 world_position = {0};
    Mtx4f local, world, concatenated;
    if (unbaked_root != NULL) root = *unbaked_root;
    if (position != NULL) world_position = *position;
    if (output != NULL) *output = (IsleGameModelTransform){0};
    if (unbaked_root == NULL || position == NULL || output == NULL ||
        !valid_model_transform(&root) || !isfinite(yaw) || !isfinite(game_model_scale) ||
        !isfinite(world_position.x) || !isfinite(world_position.y) || !isfinite(world_position.z))
        return ISLE_GAME_INVALID_ARGUMENT;
    float scale = game_model_scale * 0.1f;
    if (!isfinite(scale)) return ISLE_GAME_INVALID_ARGUMENT;
    hal_rotate_rpy_translate_scale_f(local, root.position.x, root.position.y, root.position.z,
        root.rotation.x, root.rotation.y, root.rotation.z, root.scale.x, root.scale.y, root.scale.z);
    hal_rotate_rpy_translate_scale_f(world, world_position.x, world_position.y, world_position.z,
        0.0f, yaw, 0.0f, scale, scale, scale);
    /* Original native row-vector concatenation, public guMtxCatF contract;
     * no SDK implementation. Explicit left-associated binary32 operations,
     * with contraction/reassociation disabled by the engine target.
     */
    for (size_t row = 0; row < 4; ++row) {
        for (size_t col = 0; col < 4; ++col) {
            concatenated[row][col] = ((local[row][0] * world[0][col] +
                local[row][1] * world[1][col]) + local[row][2] * world[2][col]) +
                local[row][3] * world[3][col];
            if (!isfinite(concatenated[row][col])) return ISLE_GAME_INVALID_ARGUMENT;
        }
    }
    baked = root;
    baked.position = (IsleGameVec3){concatenated[3][0], concatenated[3][1], concatenated[3][2]};
    baked.rotation.y += yaw;
    baked.scale.x *= scale;
    baked.scale.y *= scale;
    baked.scale.z *= scale;
    if (!valid_model_transform(&baked)) return ISLE_GAME_INVALID_ARGUMENT;
    *output = baked;
    return ISLE_GAME_OK;
}

void omDeleteGObj(GObj* object) {
    /* Reject an unadmitted/corrupt payload before ending processes or unlinking. */
    IsleGame* game = active_game;
    if (object == NULL || object == omCurrentObject) {
        omD_8004AC54 = 2;
        return;
    }
    if (!valid_object(game, object) ||
        (object->type != 0 && object->type != 1 && object->type != 2 && object->type != 3) ||
        (object->type == 0 && object->data.any != NULL) ||
        (object->type == 1 && (object->data.model == NULL ||
          !object->data.model->alive || object->data.model->game != game ||
          object->data.model->obj != object)) ||
        (object->type == 3 && (object->data.cam == NULL ||
          !object->data.cam->alive || object->data.cam->game != game ||
          object->data.cam->obj != object))) {
        execution_fault(game, "unsupported or invalid native object payload");
        return;
    }
    while (object->processListHead != NULL) {
        GObjProcess* before = object->processListHead;
        omEndProcess(before);
        if (before == object->processListHead) return;
    }
    switch (object->type) {
        case 1:
            remove_model(object->data.model);
            break;
        case 2:
            while (object->data.sobj != NULL) omGObjRemoveSprite(object->data.sobj);
            break;
        case 3:
            remove_camera(object->data.cam);
            break;
        case 0:
            break;
    }
    omUnlinkGObj(object);
    object->alive = false;
    if (object->retired != NULL) {
        IsleGameCallback callback = object->retired;
        void* user = object->retireUser;
        object->retired = NULL;
        object->retireUser = NULL;
        callback(object, user);
    }
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
    if (output != NULL) *output = NULL;
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

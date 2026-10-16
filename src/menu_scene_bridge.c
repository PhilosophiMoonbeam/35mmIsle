#include "menu_scene_bridge.h"
#include "engine_internal.h"
#include "engine_math.h"
#include <string.h>

struct IsleGameCommand {
    struct IsleGameCommand* next;
    int32_t command;
    IsleGameObject* source;
};

static IsleGameResult access(IsleGame* game) {
    IsleGameCounts counts;
    return isle_game_counts(game, &counts);
}

static int reserve(isle_arena* arena, size_t count, size_t stride, size_t alignment, void** out) {
    size_t bytes;
    isle_span storage;
    *out = NULL;
    if (count == 0) return 1;
    if (isle_checked_multiply(count, stride, &bytes) != ISLE_OK ||
        isle_arena_allocate(arena, bytes, alignment, &storage) != ISLE_OK) return 0;
    memset(storage.data, 0, bytes);
    *out = storage.data;
    return 1;
}

IsleGameResult isle_menu_reserve_pools(IsleGame* game, isle_arena* arena,
    size_t objects, size_t processes, size_t sprites, size_t cameras, size_t matrices) {
    isle_arena candidate;
    GObj* object;
    GObjProcess* process;
    SObj* sprite;
    OMCamera* camera;
    OMMtx* matrix;
    IsleGameResult result = access(game);
    if (result != ISLE_GAME_OK) return result;
    if (arena == NULL || arena != game->arena) return ISLE_GAME_INVALID_ARGUMENT;
    candidate = *arena;
    if (!reserve(&candidate, objects, sizeof(*object), _Alignof(GObj), (void**)&object) ||
        !reserve(&candidate, processes, sizeof(*process), _Alignof(GObjProcess), (void**)&process) ||
        !reserve(&candidate, sprites, sizeof(*sprite), _Alignof(SObj), (void**)&sprite) ||
        !reserve(&candidate, cameras, sizeof(*camera), _Alignof(OMCamera), (void**)&camera) ||
        !reserve(&candidate, matrices, sizeof(*matrix), _Alignof(OMMtx), (void**)&matrix)) return ISLE_GAME_EXHAUSTED;
    for (size_t i = 0; i < objects; ++i) { object[i].next = game->free_objects; game->free_objects = &object[i]; }
    for (size_t i = 0; i < processes; ++i) { process[i].next = game->free_processes; game->free_processes = &process[i]; }
    for (size_t i = 0; i < sprites; ++i) { sprite[i].nextFree = game->free_sprites; game->free_sprites = &sprite[i]; }
    for (size_t i = 0; i < cameras; ++i) { camera[i].nextFree = game->free_cameras; game->free_cameras = &camera[i]; }
    for (size_t i = 0; i < matrices; ++i) { matrix[i].nextFree = game->free_matrices; game->free_matrices = &matrix[i]; }
    *arena = candidate;
    return ISLE_GAME_OK;
}

IsleGameObject* isle_menu_current_object(IsleGame* game) {
    return access(game) == ISLE_GAME_OK && omCurrentObject != NULL && omCurrentObject->game == game ? omCurrentObject : NULL;
}
IsleGameObject* isle_menu_first_object(IsleGame* game, uint8_t link) {
    return access(game) == ISLE_GAME_OK && link < 32 ? omGObjListHead[link] : NULL;
}
IsleGameObject* isle_menu_next_object(IsleGame* game, IsleGameObject* object) {
    return access(game) == ISLE_GAME_OK && object != NULL && object->alive && object->game == game ? object->next : NULL;
}

IsleGameResult isle_menu_end_processes(IsleGame* game, IsleGameObject* object) {
    GObjProcess* current = NULL;
    IsleGameResult result = access(game);
    if (result != ISLE_GAME_OK) return result;
    if (object == NULL) object = omCurrentObject;
    if (object == NULL || !object->alive || object->game != game) return ISLE_GAME_INVALID_ARGUMENT;
    for (GObjProcess* process = object->processListHead; process != NULL;) {
        GObjProcess* next = process->next;
        if (process == omCurrentProcess) current = process;
        else if ((result = isle_game_end_process(game, process)) != ISLE_GAME_OK) return result;
        process = next;
    }
    return current != NULL ? isle_game_end_process(game, current) : ISLE_GAME_OK;
}

static void update(IsleGameObject* object) { object->update(object, object->updateUser); }
IsleGameResult isle_menu_set_update(IsleGame* game, IsleGameObject* object, IsleGameCallback callback, void* user) {
    IsleGameResult result = access(game);
    if (result != ISLE_GAME_OK) return result;
    if (object == NULL || !object->alive || object->game != game) return ISLE_GAME_INVALID_ARGUMENT;
    object->update = callback;
    object->updateUser = user;
    object->fnUpdate = callback != NULL ? update : NULL;
    return ISLE_GAME_OK;
}

IsleGameResult isle_menu_place_before(IsleGame* game, IsleGameObject* object, IsleGameObject* before) {
    IsleGameResult result = access(game);
    if (result != ISLE_GAME_OK) return result;
    if (object == NULL || before == NULL || object == before || !object->alive || !before->alive ||
        object->game != game || before->game != game || object->link != before->link ||
        object->priority != before->priority || object->processListHead != NULL) return ISLE_GAME_INVALID_ARGUMENT;
    omUnlinkGObj(object);
    omLinkGObjAfter(object, before->prev);
    return ISLE_GAME_OK;
}

IsleGameResult isle_game_command_send(IsleGameCommandPool* pool, IsleGameCommandQueue* queue,
    int32_t command, IsleGameObject* source) {
    IsleGameCommand* entry;
    isle_span storage;
    if (pool == NULL || pool->arena == NULL || queue == NULL || queue->cancelled) return ISLE_GAME_INVALID_ARGUMENT;
    entry = pool->free;
    if (entry != NULL) pool->free = entry->next;
    else {
        if (isle_arena_allocate(pool->arena, sizeof(*entry), _Alignof(IsleGameCommand), &storage) != ISLE_OK) return ISLE_GAME_EXHAUSTED;
        entry = (void*)storage.data;
    }
    entry->next = NULL;
    entry->command = command;
    entry->source = source != NULL ? source : omCurrentObject;
    if (queue->tail != NULL) queue->tail->next = entry;
    else queue->head = entry;
    queue->tail = entry;
    ++queue->count;
    return ISLE_GAME_OK;
}

void isle_game_commands_release(IsleGameCommandPool* pool, IsleGameCommandQueue* queue) {
    if (pool == NULL || queue == NULL) return;
    if (queue->processing) { queue->cancelled = 1; return; }
    if (queue->tail != NULL) { queue->tail->next = pool->free; pool->free = queue->head; }
    *queue = (IsleGameCommandQueue){0};
}

IsleGameResult isle_menu_observe_command_queued(IsleGame* game, IsleGameObject* object,
    int32_t command, IsleGameObject* source, IsleGameCommandHandler observer, void* user) {
    IsleGameResult result = access(game);
    if (result != ISLE_GAME_OK) return result;
    if (object == NULL || !object->alive || object->game != game || observer == NULL)
        return ISLE_GAME_INVALID_ARGUMENT;
    bool previous_observer = game->retirement_observer;
    ++game->callback_depth;
    game->retirement_observer = true;
    observer(object, command, source, user);
    game->retirement_observer = previous_observer;
    --game->callback_depth;
    return game->fault;
}

IsleGameResult isle_game_commands(IsleGame* game, IsleGameObject* object,
    IsleGameCommandPool* pool, IsleGameCommandQueue* queue,
    IsleGameCommandHandler handler, void* user, int* delete_requested) {
    IsleGameResult result = access(game);
    if (delete_requested != NULL) *delete_requested = 0;
    if (result != ISLE_GAME_OK) return result;
    if (object == NULL) object = omCurrentObject;
    if (object == NULL || !object->alive || object->game != game || pool == NULL ||
        queue == NULL || delete_requested == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    if (queue->processing) return ISLE_GAME_INVALID_STATE;
    queue->processing = 1;
    for (IsleGameCommand* command = queue->head; command != NULL; command = command->next) {
        switch (command->command) {
        case -1:
            queue->processing = 0;
            *delete_requested = 1;
            return ISLE_GAME_OK;
        case -2: case -3:
            for (GObjProcess* process = object->processListHead; process != NULL; process = process->next)
                process->paused = command->command == -2;
            break;
        case -4: object->flags |= 1; break;
        case -5: object->flags &= ~UINT32_C(1); break;
        default:
            if (handler != NULL) {
                ++game->callback_depth;
                handler(object, command->command, command->source, user);
                --game->callback_depth;
            }
            break;
        }
        if (queue->cancelled || !object->alive || (object == omCurrentObject && omD_8004AC54 == 2)) {
            *delete_requested = object->alive && object == omCurrentObject && omD_8004AC54 == 2;
            break;
        }
    }
    queue->processing = 0;
    isle_game_commands_release(pool, queue);
    return game->fault;
}

int isle_menu_object_hidden(IsleGame* game, const IsleGameObject* object) {
    return object != NULL && object->game == game && object->alive && (object->flags & 1) != 0;
}

IsleGameResult isle_menu_logo_fade(IsleGame* game, IsleGameSprite* sprite, uint16_t updates) {
    IsleSprite* view = NULL;
    IsleGameResult result = isle_game_sprite_view(game, sprite, &view);
    if (result != ISLE_GAME_OK) return result;
    if (omCurrentProcess == NULL || omCurrentProcess->kind != ISLE_GAME_STACKFUL ||
        omCurrentProcess->object != omCurrentObject || omCurrentObject->game != game) {
        return ISLE_GAME_INVALID_STATE;
    }
    func_800E1958_AA0708(sprite, updates);
    return ISLE_GAME_OK;
}

IsleGameResult isle_menu_scale_matrix(const IsleGameVec3* scale, IsleGameFixedMatrix* output) {
    Mtx4f matrix = {{0}};
    if (scale == NULL || output == NULL ||
        !isfinite(scale->x) || !isfinite(scale->y) || !isfinite(scale->z) ||
        scale->x < -32768.0f || scale->x >= 32768.0f ||
        scale->y < -32768.0f || scale->y >= 32768.0f ||
        scale->z < -32768.0f || scale->z >= 32768.0f) return ISLE_GAME_INVALID_ARGUMENT;
    matrix[0][0] = scale->x;
    matrix[1][1] = scale->y;
    matrix[2][2] = scale->z;
    matrix[3][3] = 1.0f;
    hal_mtx_f2l(matrix,output);
    return ISLE_GAME_OK;
}

#include "item_owner_source.h"
#include "actor_owner_source.h"
#include "engine_internal.h"

void isle_item_source_checked(IsleItemGlobals* g, IsleGameResult result) {
    if (result == ISLE_GAME_OK) return;
    if (g == NULL) isle_native_execution_fault(NULL, "missing native Item context");
    if (g->status == ISLE_GAME_OK) g->status = result;
    /* Release only unpublished admission ownership before the C fault boundary.
     * Published objects remain retained for real Owner/Services retirement. */
    if (g->owner != NULL) (void)isle_item_owner_finish(g);
    if (!g->nonwaiting)
        isle_native_execution_fault(g->game, "native Item source leaf failed");
}
IsleItemGlobals* isle_item_source_globals(IsleGameObject* object) {
    IsleItemGlobals* g;
    if (object == NULL) return NULL;
    g = isle_item_owner_globals(object);
    if (g == NULL) isle_native_execution_fault(object->game, "Item callback has no real owner context");
    return g;
}
IsleGameModelTransform* isle_item_source_root(IsleItemGlobals* g, IsleGameObject* object) {
    IsleGameModelTransform* root = NULL;
    isle_item_source_checked(g, isle_item_owner_root(g, object, &root));
    return root;
}
void isle_item_source_publish(IsleItemGlobals* g, IsleGameObject* object) {
    isle_item_source_checked(g, isle_item_owner_publish(g, object));
}
void isle_item_source_texture(IsleItemGlobals* g, IsleGameObject* object, float speed) {
    isle_item_source_checked(g, isle_item_owner_texture(g, object, speed));
}
float isle_item_source_animation_time(IsleItemGlobals* g, IsleGameObject* object) {
    float time = 0;
    isle_item_source_checked(g, isle_item_owner_animation_time(g, object, &time));
    return time;
}
void isle_item_source_animation_process(IsleGameObject* object) {
    IsleItemGlobals* g = isle_item_source_globals(object);
    isle_item_source_checked(g, isle_item_owner_step(g, object));
}
void isle_item_source_update(IsleGameObject* object, void* user) {
    (void)user;
    Items_UpdateObject(object);
}
void isle_item_source_default_update(IsleGameObject* object, void* user) {
    IsleItemGlobals* g = isle_item_source_globals(object);
    (void)user;
    isle_item_source_checked(g, isle_item_owner_commands(g, object));
}
void isle_item_source_command(IsleGameObject* object, int32_t command, IsleGameObject* sender, void* user) {
    IsleItemGlobals* g = user;
    (void)sender;
    /* Services dispatch is a nonwaiting C++ frame. Accumulate errors here;
     * the outer C UpdateObject bridge faults only after Services has returned. */
    if (g->status != ISLE_GAME_OK) return;
    if (isle_item_owner_globals(object) != g) {
        g->status = ISLE_GAME_INVALID_STATE;
        return;
    }
    ++g->nonwaiting;
    Items_ProcessCommand(object, command);
    --g->nonwaiting;
}
void isle_item_source_beach(IsleGameObject* object, IsleActorGround* ground) {
    IsleItemGlobals* g = isle_item_source_globals(object);
    isle_item_source_checked(g, isle_item_owner_beach(g, object, ground));
}
void isle_item_source_ground(IsleItemGlobals* g, float x, float z, IsleActorGround* ground) {
    isle_item_source_checked(g, isle_item_owner_ground(g, x, z, ground));
}
int isle_item_source_ceiling(IsleItemGlobals* g, float x, float z, IsleActorGround* ground) {
    int present = 0;
    isle_item_source_checked(g, isle_item_owner_ceiling(g, x, z, ground, &present));
    return present;
}
int32_t isle_item_source_collide(IsleItemGlobals* g, IsleGameVec3* position, IsleGameVec3* velocity,
                                IsleGameVec3* out_position, IsleGameVec3* out_velocity) {
    int32_t id = -1;
    isle_item_source_checked(g, isle_item_owner_collide(g, position, velocity, out_position, out_velocity, &id));
    return id;
}
int32_t isle_item_source_extra(IsleItemGlobals* g, IsleGameObject* object, IsleGameVec3* position,
                              IsleGameVec3* velocity, IsleGameVec3* out_position, IsleGameVec3* out_velocity) {
    int32_t hit = -1;
    IsleGameResult result;
    if (g == NULL || g->extra == NULL || g->extra_context == NULL) {
        isle_item_source_checked(g, ISLE_GAME_INVALID_STATE);
        return -1;
    }
    /* This direct C dispatch has no owning C++ frame. The borrowed context
     * cannot be replaced while active; the provider returns before C checks
     * its status. A failed operation is never a Source hit/miss decision. */
    ++g->extra_active;
    result = g->extra(g->extra_context, position, velocity, out_position, out_velocity, &hit);
    --g->extra_active;
    isle_item_source_checked(g, result);
    if (result != ISLE_GAME_OK) return -1;
    /* Input mutation survives a legitimate negative hit, and is visible in the
     * Runtime/native root before any subsequent Source hook/sound/FX/wait. */
    isle_item_source_publish(g, object);
    return hit;
}
void isle_item_source_effect(IsleItemGlobals* g, IsleGameObject* object) {
    isle_item_source_checked(g, isle_item_owner_effect(g, object));
}
void isle_item_source_send(IsleItemGlobals* g, IsleGameObject* recipient, int32_t command, IsleGameObject* sender) {
    isle_item_source_checked(g, isle_item_owner_send(g, recipient, command, sender));
}
void isle_item_source_broadcast(IsleItemGlobals* g, int32_t command, IsleGameObject* sender) {
    isle_item_source_checked(g, isle_item_owner_broadcast(g, command, sender));
}
void isle_item_source_sound(IsleItemGlobals* g, int32_t id, int32_t volume) {
    isle_item_source_checked(g, isle_item_owner_sound(g, id, volume));
}
void isle_item_source_delete_native(IsleItemGlobals* g, IsleGameObject* object) {
    /* Delete may abandon a stackful C continuation. Never call it in C++. */
    if (g->status != ISLE_GAME_OK) isle_item_source_checked(g, g->status);
    omDeleteGObj(object);
}
void isle_item_source_pause(IsleItemGlobals* g, IsleGameObject* object, int paused) {
    GObjProcess* process;
    for (process = object->processListHead; process != NULL; process = process->next)
        isle_item_source_checked(g, isle_game_pause_process(g->game, process, paused));
}
IsleGameObject* isle_item_source_link_next(IsleItemGlobals* g, IsleGameObject* previous, uint8_t link) {
    IsleGameObjectMetadata metadata;
    GObj* next;
    if (link >= ARRAY_COUNT(omGObjListHead)) {
        isle_item_source_checked(g, ISLE_GAME_INVALID_ARGUMENT);
        return NULL;
    }
    if (previous != NULL) {
        isle_item_source_checked(g, isle_game_object_metadata(g->game, previous, &metadata));
        if (previous->retirementStarted || previous->link != link) {
            isle_item_source_checked(g, ISLE_GAME_INVALID_STATE);
            return NULL;
        }
    }
    next = previous ? previous->next : omGObjListHead[link];
    if (next != NULL) {
        isle_item_source_checked(g, isle_game_object_metadata(g->game, next, &metadata));
        if (next->retirementStarted || next->link != link)
            isle_item_source_checked(g, ISLE_GAME_INVALID_STATE);
    }
    return next;
}
IsleActorSource* isle_item_source_actor(IsleItemGlobals* g, IsleGameObject* object) {
    IsleActorSource* actor = NULL;
    isle_item_source_checked(g, isle_actor_source_get(g->game, object, &actor));
    return actor;
}
float isle_item_source_actor_yaw(IsleItemGlobals* g, IsleGameObject* actor) {
    float yaw = 0;
    isle_item_source_checked(g, isle_item_owner_actor_yaw(g, actor, &yaw));
    return yaw;
}
void isle_item_source_create_process(IsleItemGlobals* g, IsleGameObject* object,
    IsleGameSourceCallback callback, uint8_t kind, uint32_t priority) {
    IsleGameProcess* process = NULL;
    isle_item_source_checked(g, isle_item_owner_process(g, object, callback, kind, priority, &process));
}
void isle_item_source_hidden(IsleItemGlobals* g, IsleGameObject* object, int hidden) {
    IsleGameObjectMetadata metadata;
    IsleGameResult result = isle_game_object_metadata(g->game, object, &metadata);
    if (result == ISLE_GAME_OK)
        result = isle_game_object_set_flags(g->game, object, hidden ? metadata.flags | 1u : metadata.flags & ~1u);
    isle_item_source_checked(g, result);
}
IsleItemSource* isle_item_source_nullable(IsleItemGlobals* g, IsleGameObject* object) {
    IsleItemSource* source = NULL;
    if (object != NULL && object->sourcePayloadKind == ISLE_SOURCE_PAYLOAD_NONE && object->userData == NULL)
        return NULL; /* genuine fume, never an impersonated Item */
    isle_item_source_checked(g, isle_item_source_get(g->game, object, &source));
    return source;
}

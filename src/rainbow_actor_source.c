#include "rainbow_actor_source.h"
#include "engine_internal.h"
#include "player_source.h"
#include <stdlib.h>
#include <string.h>

typedef struct RainbowCallFrame {
    jmp_buf failure;
    void* previous;
    void* previous_player_frame;
} RainbowCallFrame;
static uint32_t be32(const uint8_t* p) {
    return (uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | p[3];
}
static float befloat(const uint8_t* p) {
    uint32_t bits = be32(p);
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}
static int services_complete(const IsleRainbowActorServices* p) {
    return p && p->reference_assign && p->current_block && p->add_one &&
        p->sound_play && p->sound_active && p->sound_stop && p->sound_stop_all &&
        p->bgm_play && p->bgm_stop && p->bgm_volume && p->bgm_channel_volume &&
        p->remove_flying_items && p->reset_camera && p->reset_messages &&
        p->remove_pokemon && p->cutscene_start && p->resume_processes &&
        p->main_camera && p->pause_processes && p->camera_speed &&
        p->camera_animation && p->camera_animation_process && p->sky_speed &&
        p->player_model_init && p->player_model_animation && p->animation_callback &&
        p->collision_initialize && p->collision_query && p->collision_callback;
}
IsleGameResult isle_rainbow_actor_prepare(IsleRainbowActorSource* s, void* owner,
    const uint8_t* rom, size_t bytes) {
    const uint8_t* globals;
    size_t i;
    if (!s || !owner || !rom || bytes != 0x1000000u)
        return ISLE_GAME_INVALID_ARGUMENT;
    if (s->phase!=ISLE_RAINBOW_ACTOR_UNPREPARED || s->nativeGame || s->owner ||
        s->services || s->nonwaiting_call_frame)
        return ISLE_GAME_INVALID_STATE;
    globals = rom + 0x82A304;
    if (be32(globals) || be32(globals+4) || be32(globals+8) != 0x761FFu ||
        be32(globals+12) != 0x761FFu || be32(globals+16) ||
        be32(rom+0x82A5B8) != 3 || be32(rom+0x82A5BC) != 3 ||
        be32(rom+0x82A69C) || be32(rom+0x82A6A0) != UINT32_MAX ||
        be32(rom+0x82A6A4) || be32(rom+0x82A6A8) ||
        be32(rom+0x82A690) != 1001 || be32(rom+0x82A694) != 0x800EDF78u ||
        be32(rom+0x82A698) != 0x3F800000u || be32(rom+0x4A8180) != 0x3F800000u ||
        be32(rom+0x82A6CC) != 0x009703E9u ||
        ((uint16_t)rom[0x82A6D0]<<8 | rom[0x82A6D1]) != 0)
        return ISLE_GAME_INVALID_STATE;
    for (i=0; i<4; ++i) if (be32(rom+0x82A5C0+i*4)) return ISLE_GAME_INVALID_STATE;
    for (i=0; i<6; ++i) if (be32(rom+0x82AC98+i*4)) return ISLE_GAME_INVALID_STATE;
    s->owner=owner; s->status=ISLE_GAME_OK;
    s->D_8034AB94_82A304=NULL; s->D_8034AB98_82A308=NULL;
    s->D_8034AB9C_82A30C=(int32_t)be32(globals+8);
    s->D_8034ABA0_82A310=(int32_t)be32(globals+12);
    s->D_8034ABA4_82A314=(int32_t)be32(globals+16);
    s->D_8034AE48_82A5B8=(int32_t)be32(rom+0x82A5B8);
    s->D_8034AE4C_82A5BC=(int32_t)be32(rom+0x82A5BC);
    s->D_8034AE50_82A5C0=befloat(rom+0x82A5C0);
    s->D_8034AE54_82A5C4=befloat(rom+0x82A5C4);
    s->D_8034AE58_82A5C8=befloat(rom+0x82A5C8);
    s->D_8034AE5C_82A5CC=(int32_t)be32(rom+0x82A5CC);
    /* These alone are Source BSS, proven by mew/code.c159-167 + splat9951. */
    memset(s->D_80350170_82F8E0,0,sizeof(s->D_80350170_82F8E0));
    s->D_80350188_82F8F8=0; s->D_8035018C_82F8FC=0;
    s->D_80350190_82F900=0; s->D_80350194_82F904=0; s->D_80350198_82F908=0;
    s->D_8035019C_82F90C=0;
    memset(s->D_803501A0_82F910,0,sizeof(s->D_803501A0_82F910));
    s->D_8034AF2C_82A69C=NULL;
    s->mew_model=(IsleGameModelRef){0}; s->shield_model=(IsleGameModelRef){0};
    s->gate_model=(IsleGameModelRef){0};
    s->D_8034AF30_82A6A0=-1; s->D_8034AF34_82A6A4=0; s->D_8034AF38_82A6A8=0;
    s->D_8034AF5C_82A6CC[0]=151; s->D_8034AF5C_82A6CC[1]=1001; s->D_8034AF5C_82A6CC[2]=0;
    s->D_8034AF20_82A690=(IsleRainbowCollision){0x8034AF20u,be32(rom+0x82A694),
        (int32_t)be32(rom+0x82A690),befloat(rom+0x82A698),NULL};
    s->D_8034B528_82AC98=(IsleGameVec3){befloat(rom+0x82AC98),befloat(rom+0x82AC9C),befloat(rom+0x82ACA0)};
    s->D_8034B534_82ACA4=(IsleGameVec3){befloat(rom+0x82ACA4),befloat(rom+0x82ACA8),befloat(rom+0x82ACAC)};
    s->D_800F5DB0_4A8180=befloat(rom+0x4A8180);
    s->phase=ISLE_RAINBOW_ACTOR_PREPARED;
    return ISLE_GAME_OK;
}
static IsleGameResult native_status(IsleGame* game) {
    IsleGameResult result=isle_game_status(game);
    /* status() checks actual Game/domain access, but intentionally reports the
     * sticky fault only. An abandoned update is terminal for this Source entry. */
    return result==ISLE_GAME_OK && game->update_stopped ? ISLE_GAME_STOPPED : result;
}
IsleGameResult isle_rainbow_actor_activate(IsleRainbowActorSource* s, IsleGame* game,
    const IsleRainbowActorServices* services) {
    IsleRainbowActorSource* canonical=NULL;
    IsleGameResult result;
    if (!s || !game || !services_complete(services)) return ISLE_GAME_INVALID_ARGUMENT;
    if (s->phase!=ISLE_RAINBOW_ACTOR_PREPARED || !s->owner || s->nativeGame ||
        s->services || s->nonwaiting_call_frame) return ISLE_GAME_INVALID_STATE;
    if (s->status!=ISLE_GAME_OK) return s->status;
    result=isle_game_require_idle(game);
    if (result!=ISLE_GAME_OK) return result;
    result=native_status(game);
    if (result!=ISLE_GAME_OK) return result;
    /* The canonical Root/ActorOwner course context already borrows this exact
     * record. PREPARED is recognized only for this nonexecuting binding check. */
    s->nativeGame=game; s->services=services;
    result=isle_rainbow_actor_native_context(game,&canonical);
    if (result==ISLE_GAME_OK && canonical!=s) result=ISLE_GAME_INVALID_STATE;
    if (result!=ISLE_GAME_OK) {
        s->nativeGame=NULL; s->services=NULL;
        return result;
    }
    s->phase=ISLE_RAINBOW_ACTOR_ACTIVE;
    return ISLE_GAME_OK;
}
static int model_reference_empty(const IsleGameModelRef* reference) {
    return !reference->object && !reference->model && !reference->incarnation;
}
IsleGameResult isle_rainbow_actor_deactivate(IsleRainbowActorSource* s) {
    IsleGameCounts counts;
    IsleGameResult result;
    if (!s) return ISLE_GAME_INVALID_ARGUMENT;
    if (s->phase!=ISLE_RAINBOW_ACTOR_ACTIVE || !s->nativeGame || !s->owner ||
        !services_complete(s->services) || s->nonwaiting_call_frame)
        return ISLE_GAME_INVALID_STATE;
    result=isle_game_require_idle(s->nativeGame);
    if (result!=ISLE_GAME_OK) return result;
    result=isle_actor_source_retirement_status(s->nativeGame);
    if (s->status==ISLE_GAME_OK) s->status=result;
    result=isle_game_counts(s->nativeGame,&counts);
    if (result!=ISLE_GAME_OK) return result;
    if (counts.processes || s->D_8034AB94_82A304 || s->D_8034AB98_82A308 ||
        s->D_8034AF2C_82A69C || !model_reference_empty(&s->mew_model) ||
        !model_reference_empty(&s->shield_model) || !model_reference_empty(&s->gate_model) ||
        s->D_8034AF20_82A690.native)
        return ISLE_GAME_INVALID_STATE;
    /* These observable guards supplement, never replace, the host's real
     * GPU/Photo/audio/semantic/model-credit joins documented in the C contract. */
    s->nativeGame=NULL; s->services=NULL;
    s->phase=ISLE_RAINBOW_ACTOR_DETACHED;
    return ISLE_GAME_OK;
}
static IsleGameResult active_binding(IsleRainbowActorSource* s) {
    IsleGameResult result;
    if (!s) return ISLE_GAME_INVALID_ARGUMENT;
    if (s->phase!=ISLE_RAINBOW_ACTOR_ACTIVE || !s->nativeGame || !s->owner ||
        !services_complete(s->services)) return ISLE_GAME_INVALID_STATE;
    if (s->status!=ISLE_GAME_OK) return s->status;
    result=native_status(s->nativeGame);
    if (result!=ISLE_GAME_OK) s->status=result;
    return result;
}
static IsleGameResult active_context(IsleRainbowActorSource* s) {
    IsleRainbowActorSource* canonical=NULL;
    IsleGameResult result=active_binding(s);
    if (result!=ISLE_GAME_OK) return result;
    result=isle_rainbow_actor_native_context(s->nativeGame,&canonical);
    if (result!=ISLE_GAME_OK) return result;
    if (canonical!=s) return ISLE_GAME_INVALID_STATE;
    return isle_rainbow_actor_native_admission(s);
}
void isle_rainbow_actor_checked(IsleRainbowActorSource* s, IsleGameResult result) {
    IslePlayerSource* player=NULL;
    RainbowCallFrame* frame=s ? s->nonwaiting_call_frame : NULL;
    if (result==ISLE_GAME_OK) return;
    if (s && s->status==ISLE_GAME_OK) s->status=result;
    if (s && s->nativeGame) player=s->nativeGame->playerSource;
    /* An inner Player fence must restore its link before this Source fence.
     * An enclosing Player fence, however, must not skip our restoration. */
    if (player && player->nonwaiting_call_frame &&
        (!frame || player->nonwaiting_call_frame!=frame->previous_player_frame))
        isle_player_source_checked(player,result);
    if (frame) longjmp(frame->failure,1);
    isle_native_execution_fault(s ? s->nativeGame : NULL,"original Rainbow native leaf failed");
}
IsleRainbowActorSource* isle_rainbow_actor_require(IsleGame* game) {
    IsleRainbowActorSource* s=NULL;
    IsleGameResult result=isle_rainbow_actor_native_context(game,&s);
    if (result!=ISLE_GAME_OK || !s || s->nativeGame!=game)
        isle_native_execution_fault(game,"original Rainbow requires its canonical scene record");
    isle_rainbow_actor_checked(s,active_binding(s));
    isle_rainbow_actor_checked(s,isle_rainbow_actor_native_admission(s));
    return s;
}
/* Private generated-C entry guard for explicit-context helpers and callbacks.
 * The returning invoke fences own failure recovery before native C++ calls. */
void isle_rainbow_actor_enter(IsleRainbowActorSource* s) {
    isle_rainbow_actor_checked(s,active_context(s));
}
IsleActorSource* isle_rainbow_actor_source(IsleRainbowActorSource* s, IsleGameObject* object) {
    IsleActorSource* actor=NULL;
    isle_rainbow_actor_checked(s,isle_actor_source_get(s->nativeGame,object,&actor));
    return actor;
}
IsleGameModelRef isle_rainbow_actor_model(IsleRainbowActorSource* s, IsleGameObject* object) {
    IsleGameModelRef reference;
    if (!object || object->game!=s->nativeGame)
        isle_rainbow_actor_checked(s,ISLE_GAME_INVALID_ARGUMENT);
    isle_rainbow_actor_checked(s,isle_game_model_reference(s->nativeGame,object,object->data.model,&reference));
    return reference;
}
const IsleGameModelTransform* isle_rainbow_actor_aux(IsleRainbowActorSource* s, const IsleGameModelRef* model) {
    const IsleGameModelTransform* value=NULL;
    IsleActorSource* actor=isle_rainbow_actor_source(s,model->object);
    isle_rainbow_actor_checked(s,isle_actor_owner_aux_read(actor,model,&value));
    return value;
}
void isle_rainbow_actor_store(IsleRainbowActorSource* s, const IsleGameModelRef* model,
    uint32_t mask, float value) {
    IsleGameModelTransform values;
    IsleActorSource* actor=isle_rainbow_actor_source(s,model->object);
    switch(mask) {
    case ISLE_ACTOR_PX: values.position.x=value; break;
    case ISLE_ACTOR_PY: values.position.y=value; break;
    case ISLE_ACTOR_PZ: values.position.z=value; break;
    case ISLE_ACTOR_RX: values.rotation.x=value; break;
    case ISLE_ACTOR_RY: values.rotation.y=value; break;
    case ISLE_ACTOR_RZ: values.rotation.z=value; break;
    case ISLE_ACTOR_SX: values.scale.x=value; break;
    case ISLE_ACTOR_SY: values.scale.y=value; break;
    case ISLE_ACTOR_SZ: values.scale.z=value; break;
    default: isle_rainbow_actor_checked(s,ISLE_GAME_INVALID_ARGUMENT);
    }
    isle_rainbow_actor_checked(s,isle_actor_owner_aux_store(actor,model,mask,&values));
}
void isle_rainbow_actor_reference(IsleRainbowActorSource* s, IsleGameObject** field, IsleGameObject* object) {
    IsleGameModelRef model=isle_rainbow_actor_model(s,object);
    IsleGameModelRef* destination=NULL;
    if (field==&s->D_8034AB94_82A304) destination=&s->mew_model;
    else if (field==&s->D_8034AB98_82A308) destination=&s->shield_model;
    else if (field==&s->D_8034AF2C_82A69C) destination=&s->gate_model;
    else isle_rainbow_actor_checked(s,ISLE_GAME_INVALID_ARGUMENT);
    isle_rainbow_actor_checked(s,s->services->reference_assign(s,field,object));
    *destination=model;
}
void isle_rainbow_actor_visibility(IsleRainbowActorSource* s, IsleActorSource* pokemon,
    IsleGameObject* object, int hidden) {
    IsleGameObjectMetadata metadata;
    if (!pokemon || isle_rainbow_actor_source(s,pokemon->object)!=pokemon)
        isle_rainbow_actor_checked(s,ISLE_GAME_INVALID_STATE);
    /* HIDE_POKEMON's two operands differ in main's 1038 helper: tangible is
     * Mew's, while obj flags belong to the newly spawned 1038. */
    pokemon->tangible=hidden ? 0 : 1;
    isle_rainbow_actor_checked(s,isle_game_object_metadata(s->nativeGame,object,&metadata));
    isle_rainbow_actor_checked(s,isle_game_object_set_flags(s->nativeGame,object,
        hidden ? metadata.flags|3u : 0u));
}
IsleActorNodeRef isle_rainbow_actor_mew_node(IsleRainbowActorSource* s) {
    IsleActorNodeRef reference={s->mew_model,0};
    IsleGameModel* model=NULL;
    const IsleGameModelNode* nodes=NULL;
    size_t count=0;
    unsigned depth;
    isle_rainbow_actor_checked(s,isle_game_model_resolve(s->nativeGame,&reference.model,&model));
    isle_rainbow_actor_checked(s,isle_game_model_peek_nodes(s->nativeGame,model,&nodes,&count));
    for (depth=0;depth<2;++depth) {
        if (reference.node>=count || nodes[reference.node].first_child==UINT32_MAX)
            isle_rainbow_actor_checked(s,ISLE_GAME_INVALID_STATE);
        reference.node=nodes[reference.node].first_child;
    }
    if (reference.node>=count) isle_rainbow_actor_checked(s,ISLE_GAME_INVALID_STATE);
    return reference;
}
void isle_rainbow_actor_node_world(IsleRainbowActorSource* s, const IsleActorNodeRef* reference,
    IsleGameVec3* position, IsleGameVec3* velocity) {
    IsleActorSource* actor=isle_rainbow_actor_source(s,reference->model.object);
    isle_rainbow_actor_checked(s,isle_actor_owner_node_world_ref(actor,reference,position,velocity));
}
void* isle_rainbow_actor_current_block(IsleRainbowActorSource* s) {
    void* block=NULL;
    isle_rainbow_actor_checked(s,s->services->current_block(s,&block));
    return block;
}
IsleGameObject* isle_rainbow_actor_add_one(IsleRainbowActorSource* s, void* block,
    void* reference, const IsleRainbowAutomaticSpawn* spawn, const IsleRainbowDefinition* definition) {
    IsleGameObject* object=NULL;
    isle_rainbow_actor_checked(s,s->services->add_one(s,block,reference,spawn,definition,&object));
    if (!object) isle_rainbow_actor_checked(s,ISLE_GAME_EXHAUSTED);
    return object;
}
int32_t isle_rainbow_actor_sound(IsleRainbowActorSource* s, int32_t sound) {
    int32_t handle=-1;
    isle_rainbow_actor_checked(s,s->services->sound_play(s,sound,&handle));
    return handle;
}
int32_t isle_rainbow_actor_sound_active(IsleRainbowActorSource* s, int32_t handle) {
    int32_t active=0;
    isle_rainbow_actor_checked(s,s->services->sound_active(s,handle,&active));
    return active;
}
IsleGameObject* isle_rainbow_actor_main_camera(IsleRainbowActorSource* s) {
    IsleGameObject* object=NULL;
    isle_rainbow_actor_checked(s,s->services->main_camera(s,&object));
    return object;
}
IsleGameObject* isle_rainbow_actor_player_model(IsleRainbowActorSource* s) {
    IsleGameObject* object=NULL;
    isle_rainbow_actor_checked(s,s->services->player_model_init(s,&object));
    return object;
}
int32_t isle_rainbow_actor_collide(IsleRainbowActorSource* s, IsleGameVec3* a, IsleGameVec3* b,
    IsleGameVec3* c, IsleGameVec3* d, const IsleGameModelRef* model) {
    const IsleGameModelTransform* transform=isle_rainbow_actor_aux(s,model);
    int32_t hit=0;
    isle_rainbow_actor_checked(s,s->services->collision_query(s,a,b,c,d,&s->D_8034AF20_82A690,
        &transform->position,&transform->rotation,&hit));
    return hit;
}
const IsleActorRandomState* isle_rainbow_actor_weighted(IsleGameObject* object, uint32_t identity) {
    IsleRainbowActorSource* s=isle_rainbow_actor_require(object->game);
    const IsleActorRandomState* rows=NULL;
    isle_rainbow_actor_checked(s,isle_rainbow_actor_native_weighted(s,identity,&rows));
    if (!rows) isle_rainbow_actor_checked(s,ISLE_GAME_INVALID_STATE);
    return rows;
}
typedef struct RainbowSpawnInvocation {
    IsleRainbowActorSource* source;
    uint32_t identity;
} RainbowSpawnInvocation;
typedef struct RainbowAnimationInvocation {
    IsleRainbowActorSource* source;
    const IsleGameModelRef* model;
    int32_t event;
    float value;
} RainbowAnimationInvocation;
typedef struct RainbowCollisionInvocation {
    IsleRainbowActorSource* source;
    IsleGameVec3 *a, *b, *c, *d;
    int32_t* output;
} RainbowCollisionInvocation;
static void spawn_call(IslePlayerSource* player, void* user) {
    RainbowSpawnInvocation* invocation=user;
    (void)player;
    if (invocation->identity==0x803466C0u) func_803466C0_825E30(invocation->source);
    else func_80346898_826008(invocation->source);
}
static void animation_call(IslePlayerSource* player, void* user) {
    RainbowAnimationInvocation* invocation=user;
    (void)player;
    func_80348BDC_82834C(invocation->source,invocation->model,invocation->event,invocation->value);
}
static void collision_call(IslePlayerSource* player, void* user) {
    RainbowCollisionInvocation* invocation=user;
    (void)player;
    *invocation->output=func_80348FF0_828760(invocation->source,
        invocation->a,invocation->b,invocation->c,invocation->d);
}
static IsleGameResult actor_call(IsleRainbowActorSource* s,
    IslePlayerNonwaitingCall callback, void* argument) {
    RainbowCallFrame frame;
    IslePlayerSource* player=NULL;
    IsleGameResult result=isle_player_source_context(s->nativeGame,&player);
    if (result!=ISLE_GAME_OK) return result;
    if (!player) return ISLE_GAME_INVALID_STATE;
    frame.previous=s->nonwaiting_call_frame;
    frame.previous_player_frame=player->nonwaiting_call_frame;
    s->nonwaiting_call_frame=&frame;
    if (setjmp(frame.failure)==0) {
        result=isle_player_source_call(player,callback,argument);
        if (result!=ISLE_GAME_OK && s->status==ISLE_GAME_OK) s->status=result;
    }
    s->nonwaiting_call_frame=frame.previous;
    return s->status;
}
IsleGameResult isle_rainbow_actor_invoke_spawn(IsleRainbowActorSource* s, uint32_t identity) {
    RainbowSpawnInvocation invocation={s,identity};
    IsleGameResult result;
    if (identity!=0x803466C0u && identity!=0x80346898u) return ISLE_GAME_INVALID_ARGUMENT;
    result=active_context(s);
    if (result!=ISLE_GAME_OK) return result;
    return actor_call(s,spawn_call,&invocation);
}
IsleGameResult isle_rainbow_actor_invoke_animation(IsleRainbowActorSource* s,
    const IsleGameModelRef* model, int32_t event, float value) {
    RainbowAnimationInvocation invocation={s,model,event,value};
    IsleGameModel* native_model=NULL;
    IsleGameResult result=active_context(s);
    if (result!=ISLE_GAME_OK) return result;
    result=isle_game_model_resolve(s->nativeGame,model,&native_model);
    if (result!=ISLE_GAME_OK) return result;
    return actor_call(s,animation_call,&invocation);
}
IsleGameResult isle_rainbow_actor_invoke_collision(IsleRainbowActorSource* s, IsleGameVec3* a,
    IsleGameVec3* b, IsleGameVec3* c, IsleGameVec3* d, int32_t* output) {
    RainbowCollisionInvocation invocation={s,a,b,c,d,output};
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output=-1;
    if (!a || !b || !c || !d) return ISLE_GAME_INVALID_ARGUMENT;
    IsleGameResult result=active_context(s);
    if (result!=ISLE_GAME_OK) return result;
    return actor_call(s,collision_call,&invocation);
}

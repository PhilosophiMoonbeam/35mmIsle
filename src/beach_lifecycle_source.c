#include "beach_lifecycle_source.h"
#include "engine_internal.h"
#include "menu_scene_bridge.h"
#include <math.h>
#include <stdlib.h>

typedef struct LifecycleCallFrame { jmp_buf failure; void* previous; } LifecycleCallFrame;
typedef void (*LifecycleCall)(IsleBeachLifecycleSource*, void*);
typedef struct LifecycleInvocation {
    IsleBeachLifecycleSource* source;
    LifecycleCall callback;
    void* argument;
} LifecycleInvocation;

static uint16_t be16(const uint8_t* p) {
    return (uint16_t)((uint16_t)p[0] << 8 | p[1]);
}
static uint32_t be32(const uint8_t* p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}
static int32_t signed32(uint32_t value) {
    return value <= INT32_MAX ? (int32_t)value : -1 - (int32_t)(UINT32_MAX - value);
}
IsleGameResult isle_beach_lifecycle_prepare(IsleBeachLifecycleSource* s, void* owner,
    IsleContinuationDomain* domain, const uint8_t* rom, size_t bytes, int32_t scene) {
    const uint8_t *setup, *screen, *random, *def;
    size_t i;
    if (!s || !owner || !domain || !rom || bytes != 0x1000000u)
        return ISLE_GAME_INVALID_ARGUMENT;
    if (s->owner || s->domain || s->player || s->music || s->running || s->completed ||
        s->nonwaiting_call_frame || s->D_beach_802CC0E4) return ISLE_GAME_INVALID_STATE;
    if (isle_continuation_domain_check_access(domain) != ISLE_CONT_OK)
        return ISLE_GAME_INVALID_STATE;
    /* Own-GAME declaration + admitted ROM mapping provenance is enumerated in
     * beach-lifecycle-inputs.json. These are wire reads, not console struct casts. */
    setup = rom + 0x5640A8;
    screen = rom + 0x56408C;
    random = rom + 0x564074;
    def = rom + 0x564064;
    if ((be32(random+4) != 0 && be32(random+4) != 0x802C40A0u) ||
        (be32(random+12) != 0 && be32(random+12) != 0x802C40A0u) ||
        be32(rom+0x564154) != 0) return ISLE_GAME_INVALID_STATE;
    s->owner = owner;
    s->domain = domain;
    s->input_scene = scene;
    s->scene_setup.unknown_00 = be16(setup);
    s->scene_setup.update_identity = be32(setup+4);
    s->scene_setup.draw_identity = be32(setup+8);
    s->scene_setup.heap_base = be32(setup+12);
    s->scene_setup.heap_size = be32(setup+16);
    s->scene_setup.unknown_14 = be32(setup+20);
    s->scene_setup.contexts = signed32(be32(setup+24));
    for (i=0;i<4;i++) s->scene_setup.display_list_bytes[i] = be32(setup+28+i*4);
    s->scene_setup.graphics_heap_bytes = be32(setup+44);
    s->scene_setup.unknown_30 = be16(setup+48);
    s->scene_setup.rdp_output_bytes = signed32(be32(setup+52));
    s->scene_setup.prerender_identity = be32(setup+56);
    s->scene_setup.input_identity = be32(setup+60);
    s->scene_setup.threads = be32(setup+64);
    s->scene_setup.thread_stack_bytes = be32(setup+68);
    s->scene_setup.stacks = be32(setup+72);
    s->scene_setup.unknown_4c = signed32(be32(setup+76));
    s->scene_setup.processes = be32(setup+80);
    s->scene_setup.objects = be32(setup+84);
    s->scene_setup.object_bytes = be32(setup+88);
    s->scene_setup.matrices = be32(setup+92);
    s->scene_setup.matrix_callback_identity = be32(setup+96);
    s->scene_setup.dynamic_store_identity = be32(setup+100);
    s->scene_setup.animations = be32(setup+104);
    s->scene_setup.materials = be32(setup+108);
    s->scene_setup.models = be32(setup+112);
    s->scene_setup.model_bytes = be32(setup+116);
    s->scene_setup.sprites = be32(setup+120);
    s->scene_setup.sprite_bytes = be32(setup+124);
    s->scene_setup.cameras = be32(setup+128);
    s->scene_setup.camera_bytes = be32(setup+132);
    s->scene_setup.initialize_identity = be32(setup+136);
    for (i=0;i<3;i++) s->screen.framebuffer_identity[i] = be32(screen+i*4);
    s->screen.initial_depth_identity = be32(screen+12);
    s->screen.width = be32(screen+16);
    s->screen.height = be32(screen+20);
    s->screen.flags = be32(screen+24);
    for (i=0;i<53;i++) {
        const uint8_t* row = rom + 0x563E80 + i*4;
        s->environment[i].sound = be16(row);
        s->environment[i].pitch_modifier = row[2];
        s->environment[i].hearing_range = row[3];
    }
    s->magikarp.id = be32(def);
    s->magikarp.initialize_identity = be32(def+4);
    s->magikarp.update_identity = be32(def+8);
    s->magikarp.remove_identity = be32(def+12);
    for (i=0;i<2;i++) {
        s->D_beach_802CC004[i].weight = random[i*8];
        s->D_beach_802CC004[i].func = be32(random+i*8+4) ?
            isle_beach_lifecycle_beachSpawnMagikarpAtGObj : NULL;
    }
    s->D_beach_802CC014 = be16(rom+0x564084);
    s->D_beach_802CC018 = signed32(be32(rom+0x564088));
    s->D_beach_802CC0E0 = signed32(be32(rom+0x564150));
    return ISLE_GAME_OK;
}

void isle_beach_lifecycle_checked(IsleBeachLifecycleSource* s, IsleGameResult result) {
    if (result == ISLE_GAME_OK) return;
    if (s->status == ISLE_GAME_OK) s->status = result;
    /* Prefer the inner Root fence: jumping past it would leave its frame link
     * dangling. Native leaves have already returned before either transfer. */
    if (s->player && s->player->nonwaiting_call_frame)
        isle_player_source_checked(s->player,result);
    if (s->nonwaiting_call_frame)
        longjmp(((LifecycleCallFrame*)s->nonwaiting_call_frame)->failure,1);
    if (s->player) isle_player_source_checked(s->player,result);
    if (s->running) {
        (void)isle_continuation_fail(s->domain,"original Beach lifecycle leaf failed");
        return;
    }
    abort(); /* No authorized C execution fence: programming error, not success. */
}
typedef struct LifecycleTransitionCall {
    IsleBeachLifecycleSource* source;
    int32_t reason;
    IsleGameResult result;
} LifecycleTransitionCall;
static void lifecycle_transition_on_owner(void* user) {
    LifecycleTransitionCall* call = user;
    call->result = isle_beach_lifecycle_owner_transition_end(call->source,call->reason);
}
IsleGameResult isle_beach_lifecycle_native_transition_end(IsleBeachLifecycleSource* s,int32_t reason) {
    LifecycleTransitionCall call = {s,reason,ISLE_GAME_INVALID_STATE};
    IsleContinuationResult handoff;
    if (!s || !s->owner) return ISLE_GAME_INVALID_ARGUMENT;
    handoff = isle_continuation_on_owner(lifecycle_transition_on_owner,&call);
    if (handoff == ISLE_CONT_INVALID_STATE) {
        /* No active compiled-C callback: the native leaf still checks the
         * actual Manager owner. A foreign caller cannot authorize a write. */
        lifecycle_transition_on_owner(&call);
    } else if (handoff != ISLE_CONT_OK) {
        return handoff == ISLE_CONT_WRONG_OWNER ? ISLE_GAME_WRONG_OWNER :
            handoff == ISLE_CONT_RESOURCE_FAILURE ? ISLE_GAME_EXHAUSTED :
            ISLE_GAME_EXECUTION_FAILURE;
    }
    return call.result;
}
IsleGameResult isle_beach_lifecycle_native_context(IsleGameObject* object, IsleBeachLifecycleSource** output) {
    IslePlayerSource* player = NULL;
    IsleBeachLifecycleSource* source;
    IsleGameObjectMetadata metadata;
    IsleGameResult result;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = NULL;
    if (!object) return ISLE_GAME_INVALID_ARGUMENT;
    result = isle_game_object_metadata(object->game, object, &metadata);
    if (result != ISLE_GAME_OK) return result;
    result = isle_player_source_context(object->game, &player);
    if (result != ISLE_GAME_OK) return result;
    /* Original setup installs these exact callbacks before intro processes.
     * Borrow their retained course context; helper/Cart/camera payloads are
     * unrelated and must never be interpreted as a Player or Beach record. */
    if (!player || player->EndLevelCb != isle_beach_lifecycle_end_event ||
        player->PauseCb != isle_beach_lifecycle_pause_event ||
        !player->end_level_user || player->end_level_user != player->pause_user)
        return ISLE_GAME_INVALID_STATE;
    source = player->end_level_user;
    if (source->player != player || !source->owner || !source->domain)
        return ISLE_GAME_INVALID_STATE;
    if (source->status != ISLE_GAME_OK) return source->status;
    *output = source;
    return ISLE_GAME_OK;
}
IsleBeachLifecycleSource* isle_beach_lifecycle_current(IsleGameObject* object) {
    IsleBeachLifecycleSource* source = NULL;
    IsleGameResult result = isle_beach_lifecycle_native_context(object,&source);
    if (result != ISLE_GAME_OK || !source || !source->player || !source->owner)
        isle_native_execution_fault(object->game,"Beach callback requires its live retained context");
    isle_beach_lifecycle_checked(source,source->status);
    return source;
}
static void player_call(IslePlayerSource* player, void* user) {
    LifecycleInvocation* invocation = user;
    (void)player;
    invocation->callback(invocation->source,invocation->argument);
}
static IsleGameResult lifecycle_call(IsleBeachLifecycleSource* s, LifecycleCall call, void* argument) {
    LifecycleCallFrame frame;
    LifecycleInvocation invocation = {s,call,argument};
    if (!s || !call || !s->owner || s->status != ISLE_GAME_OK)
        return s ? (s->status != ISLE_GAME_OK ? s->status : ISLE_GAME_INVALID_STATE) : ISLE_GAME_INVALID_ARGUMENT;
    frame.previous = s->nonwaiting_call_frame;
    s->nonwaiting_call_frame = &frame;
    if (setjmp(frame.failure) == 0) {
        if (s->player) {
            IsleGameResult result = isle_player_source_call(s->player,player_call,&invocation);
            if (result != ISLE_GAME_OK && s->status == ISLE_GAME_OK) s->status = result;
        } else call(s,argument);
    }
    s->nonwaiting_call_frame = frame.previous;
    return s->status;
}
void isle_beach_lifecycle_run(void* user) {
    IsleBeachLifecycleSource* s = user;
    if (!s || !s->owner || !s->domain || s->running || s->completed) return;
    s->running = 1;
    s->output_scene = isle_beach_lifecycle_func_802C4740(s,s->input_scene);
    s->running = 0;
    s->completed = 1;
}
void isle_beach_lifecycle_setup_scene(IsleBeachLifecycleSource* s,
    const IsleBeachLifecycleSceneSetup* setup) {
    if (!s->running || s->nonwaiting_call_frame || s->scene_requested)
        isle_beach_lifecycle_checked(s,ISLE_GAME_INVALID_STATE);
    isle_beach_lifecycle_checked(s,isle_beach_lifecycle_native_request_scene(s,setup));
    s->scene_requested = 1;
    /* The independent lifecycle continuation remains live while the host drives
     * and drains the unrelated native scene domain. This is not an OM process. */
    while (!s->scene_completion_ready) {
        if (isle_continuation_wait(s->domain,1) != ISLE_CONT_OK)
            isle_beach_lifecycle_checked(s,ISLE_GAME_INVALID_STATE);
    }
    isle_beach_lifecycle_checked(s,s->scene_completion_status);
}
IsleGameResult isle_beach_lifecycle_complete_scene(IsleBeachLifecycleSource* s, IsleGameResult result) {
    if (!s || !s->domain || !s->running || !s->scene_requested || s->scene_completion_ready)
        return ISLE_GAME_INVALID_STATE;
    if (isle_continuation_domain_check_access(s->domain) != ISLE_CONT_OK ||
        s->nonwaiting_call_frame || s->D_beach_802CC0E4 != NULL)
        return ISLE_GAME_INVALID_STATE;
    /* Host has closed all real scene borrows, not just set an exit boolean. */
    s->player = NULL;
    s->music = NULL;
    s->scene_completion_status = result;
    s->scene_completion_ready = 1;
    return ISLE_GAME_OK;
}
static void init_call(IsleBeachLifecycleSource* s, void* ignored) {
    (void)ignored;
    isle_beach_lifecycle_func_802C45C0(s);
}
IsleGameResult isle_beach_lifecycle_invoke_init(IsleBeachLifecycleSource* s) {
    if (!s || !s->player || !s->music || s->music->player != s->player)
        return ISLE_GAME_INVALID_STATE;
    return lifecycle_call(s,init_call,NULL);
}
static void end_call(IsleBeachLifecycleSource* s, void* argument) {
    isle_beach_lifecycle_func_802C43CC(s,*(int32_t*)argument);
}
IsleGameResult isle_beach_lifecycle_invoke_end(IsleBeachLifecycleSource* s, int32_t code) {
    return lifecycle_call(s,end_call,&code);
}
IsleGameResult isle_beach_lifecycle_end_event(void* user, int32_t value) {
    return isle_beach_lifecycle_invoke_end(user,value);
}
static void pause_call(IsleBeachLifecycleSource* s, void* argument) {
    isle_beach_lifecycle_func_802C4438(s,*(int32_t*)argument);
}
IsleGameResult isle_beach_lifecycle_pause_event(void* user, int32_t value) {
    return lifecycle_call(user,pause_call,&value);
}
typedef struct BlockCall { IsleBeachLifecycleBlock *first, *second; } BlockCall;
static void world_add_call(IsleBeachLifecycleSource* s, void* argument) {
    BlockCall* call = argument;
    isle_beach_lifecycle_beachPokemonAdd(s,call->first,call->second);
}
static void world_change_call(IsleBeachLifecycleSource* s, void* argument) {
    BlockCall* call = argument;
    isle_beach_lifecycle_func_802C42F8(s,call->first,call->second);
}
static void world_remove_call(IsleBeachLifecycleSource* s, void* argument) {
    isle_beach_lifecycle_func_802C431C(s,argument);
}
static void block_exit_call(IsleBeachLifecycleSource* s, void* argument) {
    isle_beach_lifecycle_func_802C4440(s,argument);
}
IsleGameResult isle_beach_lifecycle_world_add(void* user, IsleBeachLifecycleBlock* first, IsleBeachLifecycleBlock* second) {
    BlockCall call = {first,second};
    return lifecycle_call(user,world_add_call,&call);
}
IsleGameResult isle_beach_lifecycle_world_change(void* user, IsleBeachLifecycleBlock* first, IsleBeachLifecycleBlock* second) {
    BlockCall call = {first,second};
    return lifecycle_call(user,world_change_call,&call);
}
IsleGameResult isle_beach_lifecycle_world_remove(void* user, IsleBeachLifecycleBlock* block) {
    return lifecycle_call(user,world_remove_call,block);
}
IsleGameResult isle_beach_lifecycle_exit_block(void* user, IsleBeachLifecycleBlock* block) {
    return lifecycle_call(user,block_exit_call,block);
}
typedef struct MovementCall { int32_t block; float part; } MovementCall;
static void movement_call(IsleBeachLifecycleSource* s, void* argument) {
    MovementCall* call = argument;
    isle_beach_lifecycle_checked(s,isle_beach_music_invoke(s->music,call->block,call->part));
}
IsleGameResult isle_beach_lifecycle_movement(void* user, int32_t block, float part) {
    MovementCall call = {block,part};
    return lifecycle_call(user,movement_call,&call);
}
typedef struct ItemCall { IsleGameObject* object; const IsleActorGround* ground; } ItemCall;
static void collide_call(IsleBeachLifecycleSource* s, void* argument) {
    ItemCall* call = argument;
    isle_beach_lifecycle_func_802C416C(s,call->object,call->ground);
}
IsleGameResult isle_beach_lifecycle_collide_item(void* user, IsleGameObject* object, const IsleActorGround* ground) {
    ItemCall call = {object,ground};
    if (!ground) return ISLE_GAME_INVALID_ARGUMENT;
    return lifecycle_call(user,collide_call,&call);
}
void isle_beach_lifecycle_item_collision(IsleGameObject* object, IsleActorGround* ground) {
    IsleBeachLifecycleSource* s = isle_beach_lifecycle_current(object);
    IsleGameResult result = isle_beach_lifecycle_collide_item(s,object,ground);
    isle_beach_lifecycle_checked(s,result);
}
void isle_beach_lifecycle_delete_intro_cart(IsleBeachLifecycleSource* s) {
    IsleGameObject* object = isle_menu_current_object(s->player->game);
    isle_beach_lifecycle_checked(s,isle_beach_lifecycle_native_retire_intro_cart(s,object));
    /* Both returning owner frames are gone before the genuine C processes end.
     * Preserve original whole-object deletion's delayed-sound cancellation. */
    isle_beach_lifecycle_checked(s,isle_game_end_source_processes(s->player->game,
        object,isle_beach_lifecycle_func_802C527C));
    isle_beach_lifecycle_checked(s,isle_game_end_process(s->player->game,NULL));
}
typedef struct CartEventCall { const IslePlayerCartNode* node; int32_t event; float argument; } CartEventCall;
static void cart_event_call(IsleBeachLifecycleSource* s, void* argument) {
    CartEventCall* call = argument;
    isle_beach_lifecycle_func_802C51A0(s,call->node,call->event,call->argument);
}
void isle_beach_lifecycle_cart_event(void* user, const IslePlayerCartNode* node, int32_t event, float argument) {
    IsleBeachLifecycleSource* s = user;
    CartEventCall call = {node,event,argument};
    IsleGameResult result = lifecycle_call(s,cart_event_call,&call);
    /* Cart events dispatch from canonical C after its returning step leaf. */
    isle_player_source_checked(s->player,result);
}
void isle_beach_lifecycle_camera_animation_update(IsleGameObject* object) {
    IsleBeachLifecycleSource* s = isle_beach_lifecycle_current(object);
    isle_beach_lifecycle_checked(s,isle_player_root_native_camera_animation_update(s->player,object));
}

#define SCALAR_ADAPTER(name, type) \
type isle_beach_lifecycle_##name(IsleBeachLifecycleSource* s) { \
    type value = 0; \
    isle_beach_lifecycle_checked(s,isle_beach_lifecycle_native_##name(s,&value)); \
    return value; \
}
SCALAR_ADAPTER(fx_bank_count,size_t)
SCALAR_ADAPTER(idle_enabled,int32_t)
SCALAR_ADAPTER(photo_count,int32_t)
#undef SCALAR_ADAPTER
IsleBeachLifecycleBlock* isle_beach_lifecycle_current_block(IsleBeachLifecycleSource* s) {
    IsleBeachLifecycleBlock* value = NULL;
    isle_beach_lifecycle_checked(s,isle_beach_lifecycle_native_current_block(s,&value));
    return value;
}
IsleGameObject* isle_beach_lifecycle_add_one(IsleBeachLifecycleSource* s, IsleBeachLifecycleBlock* first,
    IsleBeachLifecycleBlock* second, const IsleBeachLifecycleSpawn* spawn, const IsleBeachLifecyclePokemonDef* definition) {
    IsleGameObject* value = NULL;
    isle_beach_lifecycle_checked(s,isle_beach_lifecycle_native_add_one(s,first,second,spawn,definition,&value));
    return value;
}
IsleGameModel* isle_beach_lifecycle_model(IsleBeachLifecycleSource* s, IsleGameObject* object) {
    IsleGameModel* value = NULL;
    isle_beach_lifecycle_checked(s,isle_beach_lifecycle_native_model(s,object,&value));
    return value;
}
IsleGameVec3 isle_beach_lifecycle_model_position(IsleBeachLifecycleSource* s, IsleGameModel* model, int32_t transformed) {
    IsleGameVec3 value = {0,0,0};
    isle_beach_lifecycle_checked(s,isle_beach_lifecycle_native_model_position(s,model,transformed,&value));
    return value;
}
void isle_beach_lifecycle_set_position_component(IsleBeachLifecycleSource* s, IsleGameModel* model, uint32_t axis, float value) {
    isle_beach_lifecycle_checked(s,isle_beach_lifecycle_native_set_position_component(s,model,axis,value));
}
IsleBeachLifecycleBlock* isle_beach_lifecycle_block_next(IsleBeachLifecycleSource* s, IsleBeachLifecycleBlock* block) {
    IsleBeachLifecycleBlock* value = NULL;
    isle_beach_lifecycle_checked(s,isle_beach_lifecycle_native_block_next(s,block,&value));
    return value;
}
IsleGameVec3 isle_beach_lifecycle_block_position(IsleBeachLifecycleSource* s, IsleBeachLifecycleBlock* block) {
    IsleGameVec3 value;
    isle_beach_lifecycle_checked(s,isle_beach_lifecycle_native_block_position(s,block,&value));
    return value;
}
void* isle_beach_lifecycle_depth_buffer(IsleBeachLifecycleSource* s, uint32_t bytes, uint32_t alignment) {
    void* value = NULL;
    isle_beach_lifecycle_checked(s,isle_beach_lifecycle_native_depth_buffer(s,bytes,alignment,&value));
    return value;
}
const IsleBeachLifecycleAsset* isle_beach_lifecycle_load(IsleBeachLifecycleSource* s, IsleBeachLifecycleAssetId id) {
    static const uint32_t ranges[][2] = {
        {0xAB5860,0xAB5980},{0xAB5980,0xAB85E0},
        {0xAB85E0,0xAB8780},{0xAB8780,0xABE7A0},{0xAAA660,0xAB1470}
    };
    const IsleBeachLifecycleAsset* value = NULL;
    if ((unsigned)id >= sizeof(ranges)/sizeof(ranges[0]))
        isle_beach_lifecycle_checked(s,ISLE_GAME_INVALID_ARGUMENT);
    isle_beach_lifecycle_checked(s,isle_beach_lifecycle_native_load(s,id,ranges[id][0],ranges[id][1],&value));
    return value;
}
void isle_beach_lifecycle_init_ui(IsleBeachLifecycleSource* s) {
    const IsleBeachLifecycleUiCallbacks callbacks = {s,isle_beach_lifecycle_exit_block,
        isle_beach_lifecycle_movement,NULL,0,isle_beach_lifecycle_collide_item};
    isle_beach_lifecycle_checked(s,isle_beach_lifecycle_native_ui_bind(s,&callbacks));
    IsleGameObject* object=isle_player_initUI(s->player);
    isle_beach_lifecycle_checked(s,isle_beach_lifecycle_native_ui_initialized(s,object));
}
IsleGameObject* isle_beach_lifecycle_fx_camera(IsleBeachLifecycleSource* s, int32_t a, int32_t b, IsleGameCamera* camera) {
    IsleGameObject* value = NULL;
    isle_beach_lifecycle_checked(s,isle_beach_lifecycle_native_fx_camera(s,a,b,camera,&value));
    return value;
}
IsleGameObject* isle_beach_lifecycle_fx_effects(IsleBeachLifecycleSource* s, int32_t count) {
    IsleGameObject* value = NULL;
    isle_beach_lifecycle_checked(s,isle_beach_lifecycle_native_fx_effects(s,count,&value));
    return value;
}
int32_t isle_beach_lifecycle_sound_identity(IsleBeachLifecycleSource* s, uint32_t handle) {
    int32_t value = 0;
    isle_beach_lifecycle_checked(s,isle_beach_lifecycle_native_sound_identity(s,handle,&value));
    return value;
}
IsleBeachLifecycleEffect* isle_beach_lifecycle_create_effect(IsleBeachLifecycleSource* s, int32_t bank, int32_t index) {
    IsleBeachLifecycleEffect* value = NULL;
    isle_beach_lifecycle_checked(s,isle_beach_lifecycle_native_create_effect(s,bank,index,&value));
    return value;
}
#define CLIP_ADAPTER(name, type) \
const type* isle_beach_lifecycle_##name(IsleBeachLifecycleSource* s, uint32_t identity) { \
    const type* value = NULL; \
    isle_beach_lifecycle_checked(s,isle_beach_lifecycle_native_##name(s,identity,&value)); \
    return value; \
}
CLIP_ADAPTER(camera_clip,IslePlayerCameraAnimationClip)
CLIP_ADAPTER(cart_model_clip,IslePlayerCartModelAnimation)
CLIP_ADAPTER(cart_material_clip,IslePlayerCartMaterialAnimation)
#undef CLIP_ADAPTER
IsleGameCamera* isle_beach_lifecycle_camera(IsleBeachLifecycleSource* s) {
    if (!s->player || !s->player->camera) isle_beach_lifecycle_checked(s,ISLE_GAME_INVALID_STATE);
    return s->player->camera;
}
IsleGameObject* isle_beach_lifecycle_camera_object(IsleBeachLifecycleSource* s) {
    IsleGameObject* value = NULL;
    isle_beach_lifecycle_checked(s,isle_game_camera_object(s->player->game,isle_beach_lifecycle_camera(s),&value));
    return value;
}
float isle_beach_lifecycle_distance_from_player(IsleBeachLifecycleSource* s, IsleGameModel* model) {
    IsleGameVec3 player = isle_beach_lifecycle_model_position(s,s->player->model,1);
    IsleGameVec3 item = isle_beach_lifecycle_model_position(s,model,0);
    float x = player.x-item.x, y = player.y-item.y, z = player.z-item.z;
    return sqrtf(x*x+y*y+z*z);
}

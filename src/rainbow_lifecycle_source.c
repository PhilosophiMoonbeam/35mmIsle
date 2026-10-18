#include "rainbow_lifecycle_source.h"
#include "engine_internal.h"
#include "menu_scene_bridge.h"
#include <setjmp.h>
#include <stdlib.h>

typedef struct LifecycleCallFrame { jmp_buf failure; void* previous; } LifecycleCallFrame;
typedef void (*LifecycleCall)(IsleRainbowLifecycleSource*, void*);
typedef struct LifecycleInvocation {
    IsleRainbowLifecycleSource* source;
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
/* Only ACTIVE permits borrowing scene storage. PREPARED and DETACHED belong
 * to the independent entry owner and never authorize a Player/Game access. */
static IsleGameResult lifecycle_active(IsleRainbowLifecycleSource* s) {
    IsleRainbowActorSource* actors = NULL;
    IslePlayerSource* player = NULL;
    IsleGameResult result;
    if (!s || !s->owner || !s->actors ||
        s->actors->phase != ISLE_RAINBOW_ACTOR_ACTIVE ||
        s->actors->owner != s->owner || !s->actors->nativeGame ||
        !s->actors->services || !s->player)
        return ISLE_GAME_INVALID_STATE;
    if (s->status != ISLE_GAME_OK) return s->status;
    if (s->actors->status != ISLE_GAME_OK) return s->actors->status;
    result = isle_player_source_context(s->actors->nativeGame,&player);
    if (result != ISLE_GAME_OK) return result;
    if (player != s->player || !player->cart || !player->cart->owner ||
        player->cart->player != player)
        return ISLE_GAME_INVALID_STATE;
    if (s->actors->nativeGame->update_stopped) return ISLE_GAME_STOPPED;
    result = isle_rainbow_actor_native_context(s->actors->nativeGame,&actors);
    if (result != ISLE_GAME_OK) return result;
    return actors == s->actors ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
}
static IsleGameResult entry_joined(IsleRainbowLifecycleSource* s) {
    if (!s || !s->owner || !s->running || !s->scene_requested ||
        !s->scene_completion_ready || !s->actors ||
        s->actors->phase != ISLE_RAINBOW_ACTOR_DETACHED ||
        s->actors->owner != s->owner || s->actors->nativeGame || s->actors->services ||
        s->actors->nonwaiting_call_frame || s->player)
        return ISLE_GAME_INVALID_STATE;
    if (s->status != ISLE_GAME_OK) return s->status;
    if (s->actors->status != ISLE_GAME_OK) return s->actors->status;
    return s->scene_completion_status;
}
IsleGameResult isle_rainbow_lifecycle_prepare(IsleRainbowLifecycleSource* s, void* owner,
    IsleContinuationDomain* domain, IsleRainbowActorSource* actors,
    const uint8_t* rom, size_t bytes, int32_t scene) {
    static const uint32_t setup_words[35] = {
        0,0x8000AFFC,0x8000ADA0,0x80139C50,0,1,2,0x5000,0x1000,0x400,0,0xC800,
        0x20000,0x4000,0x800A1A50,0x80004860,0,768,0,0,0,0,0x5C,0,0,
        0x80346EE8,0,0,0,0x88,0,0x58,0,0x90,0x80346D50
    };
    static const uint32_t screen_words[7] = {0x803B5000,0x803DA800,0,0,320,240,0xA99};
    static const uint32_t defs[6][4] = {
        {151,0x80348938,0x80363DBC,0x80364280},
        {1037,0x80349EB8,0x80363DBC,0x80364280},
        {1001,0x80348FB8,0x80363DBC,0x80364280},
        {1002,0x8034A064,0x80363DBC,0x80364280},
        {1038,0x8034A4E8,0x80363DBC,0x80364280},{0,0,0,0}
    };
    static const uint16_t sounds[9] = {349,350,351,352,147,145,148,154,155};
    static const uint8_t pitch[9] = {1,1,1,1,0,0,0,2,2};
    static const uint8_t range[9] = {22,22,22,22,25,22,35,22,22};
    const uint8_t *setup, *screen;
    size_t i, j;
    if (!s || !owner || !domain || !actors || !rom || bytes != 0x1000000u)
        return ISLE_GAME_INVALID_ARGUMENT;
    if (s->owner || s->domain || s->actors || s->player || s->running || s->completed ||
        s->scene_requested || s->scene_completion_ready || s->screen.depth_buffer ||
        s->scene_completion_status != ISLE_GAME_OK || s->nonwaiting_call_frame ||
        s->D_8034B674_82ADE4 || s->status != ISLE_GAME_OK)
        return ISLE_GAME_INVALID_STATE;
    if (actors->phase != ISLE_RAINBOW_ACTOR_PREPARED || actors->owner != owner ||
        actors->status != ISLE_GAME_OK || actors->nativeGame || actors->services ||
        actors->nonwaiting_call_frame)
        return ISLE_GAME_INVALID_STATE;
    if (isle_continuation_domain_check_access(domain) != ISLE_CONT_OK)
        return ISLE_GAME_INVALID_STATE;
    /* Locked numeric facts are independently derived from pinned own-GAME
     * declarations and the admitted ROM. No guest layout cast or payload copy. */
    setup = rom + 0x82A368;
    screen = rom + 0x82A34C;
    for (i=0;i<35;i++) if (be32(setup+i*4) != setup_words[i]) return ISLE_GAME_INVALID_STATE;
    for (i=0;i<7;i++) if (be32(screen+i*4) != screen_words[i]) return ISLE_GAME_INVALID_STATE;
    for (i=0;i<6;i++) for (j=0;j<4;j++)
        if (be32(rom+0x82A2A4+i*16+j*4) != defs[i][j]) return ISLE_GAME_INVALID_STATE;
    for (i=0;i<9;i++) {
        const uint8_t* row = rom+0x82A280+i*4;
        if (be16(row) != sounds[i] || row[2] != pitch[i] || row[3] != range[i])
            return ISLE_GAME_INVALID_STATE;
    }
    if (be32(rom+0x82A348) || be32(rom+0x82ADE0) || be32(rom+0x82ADE4))
        return ISLE_GAME_INVALID_STATE;
    for (i=0;i<8;i++) if (s->D_803501E0_82F8E0[i]) return ISLE_GAME_INVALID_STATE;
    if (s->D_803501E8_82F958 || s->D_803501E9_82F959 || s->D_803501EA_82F95A)
        return ISLE_GAME_INVALID_STATE;
    s->owner = owner;
    s->domain = domain;
    s->actors = actors;
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
    for (i=0;i<9;i++) {
        const uint8_t* row = rom+0x82A280+i*4;
        s->environment[i].sound = be16(row);
        s->environment[i].pitch_modifier = row[2];
        s->environment[i].hearing_range = row[3];
    }
    for (i=0;i<6;i++) {
        const uint8_t* row = rom+0x82A2A4+i*16;
        s->pokemon_defs[i].id = be32(row);
        s->pokemon_defs[i].initialize_identity = be32(row+4);
        s->pokemon_defs[i].update_identity = be32(row+8);
        s->pokemon_defs[i].remove_identity = be32(row+12);
    }
    s->rainbow_endLevelReason = signed32(be32(rom+0x82A348));
    s->D_8034B670_82ADE0 = signed32(be32(rom+0x82ADE0));
    return ISLE_GAME_OK;
}

void isle_rainbow_lifecycle_checked(IsleRainbowLifecycleSource* s, IsleGameResult result) {
    IslePlayerSource* player = NULL;
    if (result == ISLE_GAME_OK) return;
    if (!s) abort();
    if (s->status == ISLE_GAME_OK) s->status = result;
    result = s->status;
    /* Deactivation can precede sceneStorage destruction and complete_scene.
     * Never inspect the old Player borrow after the actor binding detaches. */
    if (s->actors && s->actors->phase == ISLE_RAINBOW_ACTOR_ACTIVE &&
        s->actors->nativeGame && s->player &&
        s->actors->nativeGame->playerSource == s->player)
        player = s->player;
    /* Never jump over an inner Player C fence and leave its stack link stale. */
    if (player && player->nonwaiting_call_frame)
        isle_player_source_checked(player,result);
    if (s->nonwaiting_call_frame)
        longjmp(((LifecycleCallFrame*)s->nonwaiting_call_frame)->failure,1);
    if (player) isle_player_source_checked(player,result);
    if (s->running) (void)isle_continuation_fail(s->domain,"original Rainbow lifecycle leaf failed");
    abort(); /* Missing authorized C execution fence is never successful return. */
}
IsleRainbowLifecycleSource* isle_rainbow_lifecycle_current(IsleGameObject* object) {
    IsleRainbowLifecycleSource* s = NULL;
    IsleGameResult result = isle_rainbow_lifecycle_native_context(object ? object->game : NULL,object,&s);
    if (result != ISLE_GAME_OK || !s || !s->actors ||
        s->actors->phase != ISLE_RAINBOW_ACTOR_ACTIVE ||
        !object || object->game != s->actors->nativeGame)
        isle_native_execution_fault(object ? object->game : NULL,"Rainbow callback requires its live retained context");
    isle_rainbow_lifecycle_checked(s,lifecycle_active(s));
    return s;
}
static void player_call(IslePlayerSource* player, void* user) {
    LifecycleInvocation* invocation = user;
    (void)player;
    invocation->callback(invocation->source,invocation->argument);
}
static IsleGameResult lifecycle_call(IsleRainbowLifecycleSource* s, LifecycleCall call, void* argument) {
    LifecycleCallFrame frame;
    LifecycleInvocation invocation = {s,call,argument};
    IsleGameResult result;
    if (!call) return ISLE_GAME_INVALID_ARGUMENT;
    result = lifecycle_active(s);
    if (result != ISLE_GAME_OK) return result;
    frame.previous = s->nonwaiting_call_frame;
    s->nonwaiting_call_frame = &frame;
    if (setjmp(frame.failure) == 0) {
        result = isle_player_source_call(s->player,player_call,&invocation);
        if (result != ISLE_GAME_OK && s->status == ISLE_GAME_OK) s->status = result;
    }
    s->nonwaiting_call_frame = frame.previous;
    return s->status;
}
void isle_rainbow_lifecycle_run(void* user) {
    IsleRainbowLifecycleSource* s = user;
    int32_t output;
    if (!s || !s->owner || !s->domain || !s->actors || s->running || s->completed) return;
    s->running = 1;
    if (s->actors->phase != ISLE_RAINBOW_ACTOR_PREPARED ||
        s->actors->owner != s->owner || s->player || s->scene_requested ||
        s->scene_completion_ready)
        isle_rainbow_lifecycle_checked(s,ISLE_GAME_INVALID_STATE);
    isle_rainbow_lifecycle_checked(s,s->status);
    isle_rainbow_lifecycle_checked(s,s->actors->status);
    output = isle_rainbow_lifecycle_func_80346EF0_826660(s,s->input_scene);
    isle_rainbow_lifecycle_checked(s,entry_joined(s));
    s->output_scene = output;
    s->running = 0;
    s->completed = 1;
}
void isle_rainbow_lifecycle_setup_scene(IsleRainbowLifecycleSource* s,
    const IsleRainbowLifecycleSceneSetup* setup) {
    if (!s->running || s->nonwaiting_call_frame || s->scene_requested || s->player ||
        !s->actors || s->actors->phase != ISLE_RAINBOW_ACTOR_PREPARED ||
        s->actors->owner != s->owner || s->actors->nativeGame || s->actors->services)
        isle_rainbow_lifecycle_checked(s,ISLE_GAME_INVALID_STATE);
    isle_rainbow_lifecycle_checked(s,s->actors->status);
    isle_rainbow_lifecycle_checked(s,isle_rainbow_lifecycle_native_request_scene(s,setup));
    s->scene_requested = 1;
    /* Independent entry domain: NOT an OM process, NOT cancelled with scene OM. */
    while (!s->scene_completion_ready) {
        if (isle_continuation_wait(s->domain,1) != ISLE_CONT_OK)
            isle_rainbow_lifecycle_checked(s,ISLE_GAME_INVALID_STATE);
    }
    /* Completion_ready is published only after the real owner join receipt.
     * This return path sees independent scalar state, never dead scene storage. */
    isle_rainbow_lifecycle_checked(s,entry_joined(s));
}
IsleGameResult isle_rainbow_lifecycle_complete_scene(IsleRainbowLifecycleSource* s, IsleGameResult result) {
    IsleGameResult joined;
    if (!s || !s->owner || !s->domain || !s->running || !s->scene_requested ||
        s->scene_completion_ready || s->nonwaiting_call_frame || s->D_8034B674_82ADE4 ||
        !s->actors || s->actors->phase != ISLE_RAINBOW_ACTOR_DETACHED ||
        s->actors->owner != s->owner || s->actors->nativeGame || s->actors->services ||
        s->actors->nonwaiting_call_frame)
        return ISLE_GAME_INVALID_STATE;
    if (isle_continuation_domain_check_access(s->domain) != ISLE_CONT_OK)
        return ISLE_GAME_INVALID_STATE;
    joined = isle_rainbow_lifecycle_native_scene_joined(s);
    if (joined != ISLE_GAME_OK) return joined;
    /* The native hook reads the independent owner's retained join receipt,
     * not s->player or an already destroyed Game/sceneStorage. */
    s->player = NULL;
    s->screen.depth_buffer = NULL;
    if (s->status != ISLE_GAME_OK) result = s->status;
    else if (s->actors->status != ISLE_GAME_OK) result = s->actors->status;
    if (result != ISLE_GAME_OK && s->status == ISLE_GAME_OK) s->status = result;
    s->scene_completion_status = result;
    s->scene_completion_ready = 1;
    /* STOPPED/cancellation drains the borrow but never acknowledges success or
     * permits an original entry result, even if the host resumes this domain. */
    return result;
}
typedef struct TransitionCall {
    IsleRainbowLifecycleSource* source;
    int32_t reason;
    IsleGameResult result;
} TransitionCall;
static void transition_on_owner(void* user) {
    TransitionCall* call = user;
    call->result = isle_rainbow_lifecycle_owner_transition_end(call->source,call->reason);
}
IsleGameResult isle_rainbow_lifecycle_native_transition_end(IsleRainbowLifecycleSource* s, int32_t reason) {
    TransitionCall call = {s,reason,ISLE_GAME_INVALID_STATE};
    IsleContinuationResult handoff;
    IsleGameResult result;
    if (!s || !s->owner) return ISLE_GAME_INVALID_ARGUMENT;
    result = lifecycle_active(s);
    if (result != ISLE_GAME_OK) return result;
    handoff = isle_continuation_on_owner(transition_on_owner,&call);
    if (handoff == ISLE_CONT_INVALID_STATE) transition_on_owner(&call);
    else if (handoff != ISLE_CONT_OK)
        return handoff == ISLE_CONT_WRONG_OWNER ? ISLE_GAME_WRONG_OWNER :
            handoff == ISLE_CONT_RESOURCE_FAILURE ? ISLE_GAME_EXHAUSTED : ISLE_GAME_EXECUTION_FAILURE;
    return call.result;
}
static void init_call(IsleRainbowLifecycleSource* s, void* ignored) {
    (void)ignored;
    isle_rainbow_lifecycle_func_80346D50_8264C0(s);
}
IsleGameResult isle_rainbow_lifecycle_invoke_init(IsleRainbowLifecycleSource* s) {
    if (!s || !s->running || !s->scene_requested || s->scene_completion_ready)
        return ISLE_GAME_INVALID_STATE;
    return lifecycle_call(s,init_call,NULL);
}
static void end_call(IsleRainbowLifecycleSource* s, void* argument) {
    isle_rainbow_lifecycle_func_80346AA8_826218(s,*(int32_t*)argument);
}
IsleGameResult isle_rainbow_lifecycle_invoke_end(IsleRainbowLifecycleSource* s, int32_t reason) {
    return lifecycle_call(s,end_call,&reason);
}
IsleGameResult isle_rainbow_lifecycle_end_event(void* user, int32_t reason) {
    return isle_rainbow_lifecycle_invoke_end(user,reason);
}
static void pause_call(IsleRainbowLifecycleSource* s, void* argument) {
    isle_rainbow_lifecycle_func_80346B94_826304(s,*(int32_t*)argument);
}
IsleGameResult isle_rainbow_lifecycle_pause_event(void* user, int32_t value) {
    return lifecycle_call(user,pause_call,&value);
}
typedef struct BlockCall { IsleRainbowLifecycleBlock *first, *second; } BlockCall;
static void add_call(IsleRainbowLifecycleSource* s, void* argument) {
    BlockCall* call = argument;
    isle_rainbow_lifecycle_func_80346994_826104(s,call->first,call->second);
}
static void change_call(IsleRainbowLifecycleSource* s, void* argument) {
    BlockCall* call = argument;
    isle_rainbow_lifecycle_func_803469E0_826150(s,call->first,call->second);
}
static void remove_call(IsleRainbowLifecycleSource* s, void* argument) {
    isle_rainbow_lifecycle_func_80346A04_826174(s,argument);
}
static void exit_call(IsleRainbowLifecycleSource* s, void* argument) {
    isle_rainbow_lifecycle_func_80346BD0_826340(s,argument);
}
IsleGameResult isle_rainbow_lifecycle_world_add(void* user, IsleRainbowLifecycleBlock* first, IsleRainbowLifecycleBlock* second) {
    BlockCall call = {first,second};
    return lifecycle_call(user,add_call,&call);
}
IsleGameResult isle_rainbow_lifecycle_world_change(void* user, IsleRainbowLifecycleBlock* first, IsleRainbowLifecycleBlock* second) {
    BlockCall call = {first,second};
    return lifecycle_call(user,change_call,&call);
}
IsleGameResult isle_rainbow_lifecycle_world_remove(void* user, IsleRainbowLifecycleBlock* block) {
    return lifecycle_call(user,remove_call,block);
}
IsleGameResult isle_rainbow_lifecycle_exit_block(void* user, IsleRainbowLifecycleBlock* block) {
    return lifecycle_call(user,exit_call,block);
}
typedef struct CartEventCall { const IslePlayerCartNode* node; int32_t event; float argument; } CartEventCall;
static void cart_event_call(IsleRainbowLifecycleSource* s, void* argument) {
    CartEventCall* call = argument;
    isle_rainbow_lifecycle_func_8034A578_829CE8(s,call->node,call->event,call->argument);
}
void isle_rainbow_lifecycle_cart_event(void* user, const IslePlayerCartNode* node, int32_t event, float argument) {
    IsleRainbowLifecycleSource* s = user;
    CartEventCall call = {node,event,argument};
    IsleGameResult result = lifecycle_call(s,cart_event_call,&call);
    /* Canonical Cart C dispatch invokes this after returning native step. */
    isle_rainbow_lifecycle_checked(s,result);
}
void isle_rainbow_lifecycle_actor_cart_event(void* user, const IslePlayerCartNode* node,
    int32_t event, float argument) {
    IsleRainbowLifecycleSource* s = user;
    IsleGameResult result = lifecycle_active(s);
    if (result == ISLE_GAME_OK && !node) result = ISLE_GAME_INVALID_ARGUMENT;
    if (result == ISLE_GAME_OK)
        result = isle_rainbow_actor_invoke_animation(s->actors,&node->binding,event,argument);
    /* The real Cart C event lease is still open. The returning Actor C fence
     * catches its Source faults before this C-only checked transfer. */
    isle_rainbow_lifecycle_checked(s,result);
}
void isle_rainbow_lifecycle_input_update(IsleGameObject* object) {
    IsleRainbowLifecycleSource* s = isle_rainbow_lifecycle_current(object);
    for (;;) {
        isle_rainbow_lifecycle_checked(s,isle_rainbow_lifecycle_native_invalidate_input(s));
        isle_player_source_wait(s->player,1);
    }
}
void isle_rainbow_lifecycle_camera_animation_update(IsleGameObject* object) {
    IsleRainbowLifecycleSource* s = isle_rainbow_lifecycle_current(object);
    isle_rainbow_lifecycle_checked(s,isle_player_root_native_camera_animation_update(s->player,object));
}
void isle_rainbow_lifecycle_delete_intro_cart(IsleRainbowLifecycleSource* s) {
    isle_rainbow_lifecycle_checked(s,lifecycle_active(s));
    IsleGameObject* object = isle_menu_current_object(s->player->game);
    isle_rainbow_lifecycle_checked(s,isle_rainbow_lifecycle_native_retire_intro_cart(s,object));
    /* Original whole-object deletion cancels BOTH delayed sound processes.
     * End self only after returning leaves; no C++ stack spans stop/fault. */
    isle_rainbow_lifecycle_checked(s,isle_game_end_source_processes(s->player->game,object,
        isle_rainbow_lifecycle_func_8034A6B4_829E24));
    isle_rainbow_lifecycle_checked(s,isle_game_end_source_processes(s->player->game,object,
        isle_rainbow_lifecycle_func_8034A748_829EB8));
    isle_rainbow_lifecycle_checked(s,isle_game_end_process(s->player->game,NULL));
}

#define SCALAR_ADAPTER(name, type) \
type isle_rainbow_lifecycle_##name(IsleRainbowLifecycleSource* s) { \
    type value = 0; \
    isle_rainbow_lifecycle_checked(s,isle_rainbow_lifecycle_native_##name(s,&value)); \
    return value; \
}
SCALAR_ADAPTER(fx_bank_count,size_t)
SCALAR_ADAPTER(flute_command,int32_t)
#undef SCALAR_ADAPTER
int32_t isle_rainbow_lifecycle_photo_count(IsleRainbowLifecycleSource* s) {
    int32_t value = 0;
    isle_rainbow_lifecycle_checked(s,entry_joined(s));
    isle_rainbow_lifecycle_checked(s,isle_rainbow_lifecycle_native_photo_count(s,&value));
    return value;
}
IsleRainbowLifecycleBlock* isle_rainbow_lifecycle_block_next(IsleRainbowLifecycleSource* s, IsleRainbowLifecycleBlock* block) {
    IsleRainbowLifecycleBlock* value = NULL;
    isle_rainbow_lifecycle_checked(s,isle_rainbow_lifecycle_native_block_next(s,block,&value));
    return value;
}
IsleGameVec3 isle_rainbow_lifecycle_block_position(IsleRainbowLifecycleSource* s, IsleRainbowLifecycleBlock* block) {
    IsleGameVec3 value;
    isle_rainbow_lifecycle_checked(s,isle_rainbow_lifecycle_native_block_position(s,block,&value));
    return value;
}
void isle_rainbow_lifecycle_displace_items(IsleRainbowLifecycleSource* s, float x, float y, float z) {
    isle_rainbow_lifecycle_checked(s,isle_rainbow_lifecycle_native_displace_items(s,x,y,z));
}
void* isle_rainbow_lifecycle_depth_buffer(IsleRainbowLifecycleSource* s, uint32_t bytes, uint32_t alignment) {
    void* value = NULL;
    isle_rainbow_lifecycle_checked(s,isle_rainbow_lifecycle_native_depth_buffer(s,bytes,alignment,&value));
    return value;
}
int32_t isle_rainbow_lifecycle_play_sound(IsleRainbowLifecycleSource* s, int32_t sound,
    int32_t volume, int32_t pan, float pitch, int32_t reverb) {
    int32_t value = 0;
    isle_rainbow_lifecycle_checked(s,isle_rainbow_lifecycle_native_play_sound(s,sound,volume,pan,pitch,reverb,&value));
    return value;
}
int32_t isle_rainbow_lifecycle_sound_identity(IsleRainbowLifecycleSource* s, uint32_t handle) {
    int32_t value = 0;
    isle_rainbow_lifecycle_checked(s,isle_rainbow_lifecycle_native_sound_identity(s,handle,&value));
    return value;
}
const IsleRainbowLifecycleAsset* isle_rainbow_lifecycle_load(IsleRainbowLifecycleSource* s, IsleRainbowLifecycleAssetId id) {
    static const uint32_t ranges[][2] = {
        {0xAB5860,0xAB5980},{0xAB5980,0xAB85E0},
        {0xADD310,0xADD5D0},{0xADD5D0,0xADEC60}
    };
    const IsleRainbowLifecycleAsset* value = NULL;
    if ((unsigned)id >= sizeof(ranges)/sizeof(ranges[0]))
        isle_rainbow_lifecycle_checked(s,ISLE_GAME_INVALID_ARGUMENT);
    isle_rainbow_lifecycle_checked(s,isle_rainbow_lifecycle_native_load(s,id,ranges[id][0],ranges[id][1],&value));
    return value;
}
void isle_rainbow_lifecycle_init_ui(IsleRainbowLifecycleSource* s) {
    const IsleRainbowLifecycleUiCallbacks callbacks = {s,isle_rainbow_lifecycle_exit_block,NULL,NULL,0,NULL};
    IsleGameObject* object;
    isle_rainbow_lifecycle_checked(s,isle_rainbow_lifecycle_native_ui_bind(s,&callbacks));
    object = isle_player_initUI(s->player);
    isle_rainbow_lifecycle_checked(s,isle_rainbow_lifecycle_native_ui_initialized(s,object));
}
IsleGameObject* isle_rainbow_lifecycle_fx_camera(IsleRainbowLifecycleSource* s, int32_t a, int32_t b, IsleGameCamera* camera) {
    IsleGameObject* value = NULL;
    isle_rainbow_lifecycle_checked(s,isle_rainbow_lifecycle_native_fx_camera(s,a,b,camera,&value));
    return value;
}
IsleGameObject* isle_rainbow_lifecycle_fx_effects(IsleRainbowLifecycleSource* s, int32_t count) {
    IsleGameObject* value = NULL;
    isle_rainbow_lifecycle_checked(s,isle_rainbow_lifecycle_native_fx_effects(s,count,&value));
    return value;
}
IsleRainbowLifecycleEffect* isle_rainbow_lifecycle_create_effect(IsleRainbowLifecycleSource* s, int32_t bank, int32_t index) {
    IsleRainbowLifecycleEffect* value = NULL;
    isle_rainbow_lifecycle_checked(s,isle_rainbow_lifecycle_native_create_effect(s,bank,index,&value));
    return value;
}
#define CLIP_ADAPTER(name, type) \
const type* isle_rainbow_lifecycle_##name(IsleRainbowLifecycleSource* s, uint32_t identity) { \
    const type* value = NULL; \
    isle_rainbow_lifecycle_checked(s,isle_rainbow_lifecycle_native_##name(s,identity,&value)); \
    return value; \
}
CLIP_ADAPTER(camera_clip,IslePlayerCameraAnimationClip)
CLIP_ADAPTER(cart_model_clip,IslePlayerCartModelAnimation)
CLIP_ADAPTER(cart_material_clip,IslePlayerCartMaterialAnimation)
#undef CLIP_ADAPTER
IsleGameCamera* isle_rainbow_lifecycle_camera(IsleRainbowLifecycleSource* s) {
    isle_rainbow_lifecycle_checked(s,lifecycle_active(s));
    if (!s->player->camera) isle_rainbow_lifecycle_checked(s,ISLE_GAME_INVALID_STATE);
    return s->player->camera;
}
IsleGameObject* isle_rainbow_lifecycle_camera_object(IsleRainbowLifecycleSource* s) {
    IsleGameObject* value = NULL;
    IsleGameCamera* camera = isle_rainbow_lifecycle_camera(s);
    isle_rainbow_lifecycle_checked(s,isle_game_camera_object(s->player->game,camera,&value));
    return value;
}

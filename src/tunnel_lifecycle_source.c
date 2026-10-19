#include "tunnel_lifecycle_source.h"
#include "engine_internal.h"
#include "menu_scene_bridge.h"
#include <setjmp.h>
#include <stdlib.h>

typedef struct LifecycleCallFrame { jmp_buf failure; void* previous; } LifecycleCallFrame;
typedef void (*LifecycleCall)(IsleTunnelLifecycleSource*, void*);
typedef struct LifecycleInvocation {
    IsleTunnelLifecycleSource* source;
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

IsleGameResult isle_tunnel_lifecycle_prepare(IsleTunnelLifecycleSource* s, void* owner,
    IsleContinuationDomain* domain, IsleTunnelActorSource* actors,
    const uint8_t* rom, size_t bytes, int32_t scene) {
    static const uint32_t setup_words[35] = {
        0x00000000u,0x8000AFFCu,0x8000ADA0u,0x8018BC50u,0x00000000u,0x00000001u,0x00000002u,
        0x00005400u,0x00000C00u,0x00000400u,0x00000000u,0x0000C800u,0x00020000u,0x00004000u,
        0x800A1A50u,0x80004860u,0x00000000u,0x00000440u,0x00000000u,0x00000000u,0x00000000u,
        0x00000000u,0x0000005Cu,0x00000000u,0x00000000u,0x802E2BB0u,0x00000000u,0x00000000u,
        0x00000000u,0x00000088u,0x00000000u,0x00000058u,0x00000000u,0x00000090u,0x802E2A34u,
    };
    static const uint32_t screen_words[7] = {
        0x803B5000u,0x803DA800u,0x00000000u,0x00000000u,0x00000140u,0x000000F0u,0x00000A99u,
    };
    static const uint32_t intro_words[2] = {
        0x00000000u,0x00000000u,
    };
    static const uint32_t definitions[22][4] = {
        {0x51u,0x802E4AFCu,0x80363EB4u,0x80364280u},
        {0x52u,0x802E505Cu,0x80363EB4u,0x80364280u},
        {0x65u,0x802E5C74u,0x80363EB4u,0x80364280u},
        {0x32u,0x802E6420u,0x80363EB4u,0x802E6458u},
        {0x33u,0x802E6C28u,0x80363EB4u,0x802E6C74u},
        {0xEu,0x802E7620u,0x80363DBCu,0x80364280u},
        {0x19u,0x802E9390u,0x80363DBCu,0x80364280u},
        {0x7Du,0x802EA424u,0x80363DBCu,0x802EA45Cu},
        {0x91u,0x802EB818u,0x80363DBCu,0x80364280u},
        {0x25Au,0x802EC3BCu,0x80363DBCu,0x802EC398u},
        {0x29u,0x802EBBA8u,0x80363DBCu,0x80364280u},
        {0x5Du,0x802EBCE8u,0x80363DBCu,0x80364280u},
        {0x81u,0x802EBF04u,0x80363EB4u,0x80364280u},
        {0x3F1u,0x802EAF18u,0x80363DBCu,0x0u},
        {0x3EBu,0x8035E0D4u,0x80363DBCu,0x80364280u},
        {0x3F0u,0x802EA6F8u,0x80363DBCu,0x80364280u},
        {0x3F2u,0x802EB3A8u,0x80363DBCu,0x80364280u},
        {0x3F4u,0x802EC720u,0x80363DBCu,0x80364280u},
        {0x3F5u,0x802ECA80u,0x80363DBCu,0x80364280u},
        {0x3F6u,0x802EDEC0u,0x80363DBCu,0x80364280u},
        {0x3E9u,0x802ECFC8u,0x80363DBCu,0x80364280u},
        {0x0u,0x0u,0x0u,0x0u},
    };
    static const uint16_t sounds[43] = {361,362,51,50,49,327,220,221,222,219,218,204,209,207,205,206,208,105,393,104,151,210,211,376,114,135,60,263,264,265,266,267,268,371,127,360,353,119,137,134,139,309,133};
    static const uint8_t pitch[43] = {1,1,4,4,4,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,2,1,1,0,0,0,0,1,1,1,1,2,2,0,0,0,1,0,0,0,0,1,0};
    static const uint8_t ranges[43] = {20,30,30,30,30,7,9,9,9,10,6,10,10,10,10,10,10,5,15,10,8,8,8,8,8,14,14,8,10,8,8,8,8,8,10,10,10,15,15,15,15,5,5};
    const uint8_t *setup, *screen, *state;
    size_t i, j;
    if (!s || !owner || !domain || !actors || !rom || bytes != 0x1000000u)
        return ISLE_GAME_INVALID_ARGUMENT;
    /* The caller supplies fixed-address zero storage, exactly once. No Game,
     * service binding, allocation, reference fabrication or activation occurs. */
    state = (const uint8_t*)s;
    for (i = 0; i < sizeof(*s); ++i)
        if (state[i] != 0) return ISLE_GAME_INVALID_STATE;
    if (actors->base.phase != ISLE_COURSE_ACTOR_PREPARED ||
        actors->base.course_id != 1 || actors->base.owner != owner ||
        actors->base.status != ISLE_GAME_OK || actors->base.nativeGame ||
        actors->services || actors->base.nonwaiting_call_frame)
        return ISLE_GAME_INVALID_STATE;
    if (isle_continuation_domain_check_access(domain) != ISLE_CONT_OK)
        return ISLE_GAME_INVALID_STATE;
    /* Exact mapped Tunnel segment relation, never symbol-suffix arithmetic.
     * Read actual fields from this SAME identity-admitted canonical Snapshot.
     * Qualified639a exact words retain both high-halfword padding words too.
     * They match pinned Source/map invariants; host sizeof/layout is never used.
     * This numeric admission does not claim native owners or Source execution. */
    setup = rom + 0x5EB220u;
    screen = rom + 0x5EB204u;
    for (i = 0; i < 35; ++i)
        if (be32(setup + i * 4) != setup_words[i]) return ISLE_GAME_INVALID_STATE;
    for (i = 0; i < 7; ++i)
        if (be32(screen + i * 4) != screen_words[i]) return ISLE_GAME_INVALID_STATE;
    for (i = 0; i < 22; ++i) for (j = 0; j < 4; ++j)
        if (be32(rom + 0x5EB07Cu + i * 16 + j * 4) != definitions[i][j])
            return ISLE_GAME_INVALID_STATE;
    for (i = 0; i < 43; ++i) {
        const uint8_t* row = rom + 0x5EAFD0u + i * 4;
        if (be16(row) != sounds[i] || row[2] != pitch[i] || row[3] != ranges[i])
            return ISLE_GAME_INVALID_STATE;
    }
    if (be32(rom + 0x5EB200u) != 0)
        return ISLE_GAME_INVALID_STATE;
    /* Nonzero cannot denote a bound native object before Game admission.
     * Reject it, never cast a guest address or fabricate a semantic GObj. */
    if (be32(rom + 0x5ED2B0u) != intro_words[0] ||
        be32(rom + 0x5ED2B4u) != intro_words[1])
        return ISLE_GAME_INVALID_STATE;
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
    for (i = 0; i < 43; ++i) {
        const uint8_t* row = rom + 0x5EAFD0u + i * 4;
        s->environment[i].sound = be16(row);
        s->environment[i].pitch_modifier = row[2];
        s->environment[i].hearing_range = row[3];
    }
    for (i = 0; i < 22; ++i) {
        const uint8_t* row = rom + 0x5EB07Cu + i * 16;
        s->pokemon_defs[i].id = be32(row);
        s->pokemon_defs[i].initialize_identity = be32(row + 4);
        s->pokemon_defs[i].update_identity = be32(row + 8);
        s->pokemon_defs[i].remove_identity = be32(row + 12);
    }
    s->tunnel_EndLevelReason = signed32(be32(rom + 0x5EB200u));
    /* Preserve the actual Source bool word, not a guessed/normalized default.
     * The qualified zero object word above maps to this canonical NULL field;
     * native registry admission retains the field before later original stores. */
    s->D_802F01E0_5ED2B0 = signed32(be32(rom + 0x5ED2B0u));
    s->D_802F01E4_5ED2B4 = NULL;
    s->owner = owner; /* Publish once, only after every check/decode succeeded. */
    return ISLE_GAME_OK;
}

/* Only ACTIVE permits borrowing scene storage. PREPARED and DETACHED belong
 * to the independent entry owner and never authorize a Player/Game access. */
static IsleGameResult lifecycle_active(IsleTunnelLifecycleSource* s) {
    IslePlayerSource* player = NULL;
    IsleTunnelLifecycleSource* canonical = NULL;
    IsleGameResult result;
    if (!s || !s->owner || !s->actors ||
        s->actors->base.phase != ISLE_COURSE_ACTOR_ACTIVE ||
        s->actors->base.course_id != 1 || s->actors->base.owner != s->owner ||
        !s->actors->base.nativeGame || !s->actors->services || !s->player)
        return ISLE_GAME_INVALID_STATE;
    if (s->status != ISLE_GAME_OK) return s->status;
    if (s->actors->base.status != ISLE_GAME_OK) return s->actors->base.status;
    /* Explicit non-object entry/event preflight, using the actual active Game.
     * Native context validates the retained lifecycle alternative, not userdata. */
    result = isle_tunnel_lifecycle_native_context(s->actors->base.nativeGame, NULL, &canonical);
    if (result != ISLE_GAME_OK) return result;
    if (canonical != s) return ISLE_GAME_INVALID_STATE;
    result = isle_player_source_context(s->actors->base.nativeGame, &player);
    if (result != ISLE_GAME_OK) return result;
    if (player != s->player || player->game != s->actors->base.nativeGame ||
        !player->cart || !player->cart->owner || player->cart->player != player)
        return ISLE_GAME_INVALID_STATE;
    result = isle_actor_source_execution_status(s->actors->base.nativeGame);
    if (result != ISLE_GAME_OK) return result;
    return isle_tunnel_actor_native_admission(s->actors,
        s->actors->base.nativeGame, s->actors->services);
}
static IsleGameResult entry_joined(IsleTunnelLifecycleSource* s) {
    if (!s || !s->owner || !s->running || !s->scene_requested ||
        !s->scene_completion_ready || !s->actors ||
        s->actors->base.phase != ISLE_COURSE_ACTOR_DETACHED ||
        s->actors->base.owner != s->owner || s->actors->base.nativeGame || s->actors->services ||
        s->actors->base.nonwaiting_call_frame || s->player)
        return ISLE_GAME_INVALID_STATE;
    if (s->status != ISLE_GAME_OK) return s->status;
    if (s->actors->base.status != ISLE_GAME_OK) return s->actors->base.status;
    return s->scene_completion_status;
}

void isle_tunnel_lifecycle_checked(IsleTunnelLifecycleSource* s, IsleGameResult result) {
    IslePlayerSource* player = NULL;
    if (result == ISLE_GAME_OK) return;
    if (!s) abort();
    if (s->status == ISLE_GAME_OK) s->status = result;
    result = s->status;
    /* Deactivation can precede sceneStorage destruction and complete_scene.
     * Never inspect the old Player borrow after the actor binding detaches. */
    if (s->actors && s->actors->base.phase == ISLE_COURSE_ACTOR_ACTIVE &&
        s->actors->base.nativeGame && s->player &&
        s->actors->base.nativeGame->playerSource == s->player)
        player = s->player;
    /* Never jump over an inner Player C fence and leave its stack link stale. */
    if (player && player->nonwaiting_call_frame)
        isle_player_source_checked(player,result);
    if (s->nonwaiting_call_frame)
        longjmp(((LifecycleCallFrame*)s->nonwaiting_call_frame)->failure,1);
    if (player) isle_player_source_checked(player,result);
    if (s->running) (void)isle_continuation_fail(s->domain,"original Tunnel lifecycle leaf failed");
    abort(); /* Missing authorized C execution fence is never successful return. */
}
IsleTunnelLifecycleSource* isle_tunnel_lifecycle_current(IsleGameObject* object) {
    IsleTunnelLifecycleSource* s = NULL;
    IsleGameResult result = isle_tunnel_lifecycle_native_context(object ? object->game : NULL,object,&s);
    if (result != ISLE_GAME_OK || !s || !s->actors ||
        s->actors->base.phase != ISLE_COURSE_ACTOR_ACTIVE ||
        !object || object->game != s->actors->base.nativeGame)
        isle_native_execution_fault(object ? object->game : NULL,"Tunnel callback requires its live retained context");
    isle_tunnel_lifecycle_checked(s,lifecycle_active(s));
    return s;
}
static void player_call(IslePlayerSource* player, void* user) {
    LifecycleInvocation* invocation = user;
    (void)player;
    invocation->callback(invocation->source,invocation->argument);
}
static IsleGameResult lifecycle_call(IsleTunnelLifecycleSource* s, LifecycleCall call, void* argument) {
    LifecycleCallFrame frame;
    LifecycleInvocation invocation = {s,call,argument};
    IsleGameResult result;
    if (!call) return ISLE_GAME_INVALID_ARGUMENT;
    result = lifecycle_active(s);
    if (result != ISLE_GAME_OK) {
        if (s && s->status == ISLE_GAME_OK) s->status = result;
        return result;
    }
    frame.previous = s->nonwaiting_call_frame;
    s->nonwaiting_call_frame = &frame;
    if (setjmp(frame.failure) == 0) {
        result = isle_player_source_call(s->player,player_call,&invocation);
        if (result != ISLE_GAME_OK && s->status == ISLE_GAME_OK) s->status = result;
    }
    s->nonwaiting_call_frame = frame.previous;
    return s->status;
}
void isle_tunnel_lifecycle_run(void* user) {
    IsleTunnelLifecycleSource* s = user;
    int32_t output;
    if (!s || !s->owner || !s->domain || !s->actors || s->running || s->completed) return;
    s->running = 1;
    if (s->actors->base.phase != ISLE_COURSE_ACTOR_PREPARED ||
        s->actors->base.owner != s->owner || s->player || s->scene_requested ||
        s->scene_completion_ready)
        isle_tunnel_lifecycle_checked(s,ISLE_GAME_INVALID_STATE);
    isle_tunnel_lifecycle_checked(s,s->status);
    isle_tunnel_lifecycle_checked(s,s->actors->base.status);
    output = isle_tunnel_lifecycle_tunnel_Start(s,s->input_scene);
    isle_tunnel_lifecycle_checked(s,entry_joined(s));
    s->output_scene = output;
    s->running = 0;
    s->completed = 1;
}
void isle_tunnel_lifecycle_setup_scene(IsleTunnelLifecycleSource* s,
    const IsleTunnelLifecycleSceneSetup* setup) {
    if (!s->running || s->nonwaiting_call_frame || s->scene_requested || s->player ||
        !s->actors || s->actors->base.phase != ISLE_COURSE_ACTOR_PREPARED ||
        s->actors->base.owner != s->owner || s->actors->base.nativeGame || s->actors->services)
        isle_tunnel_lifecycle_checked(s,ISLE_GAME_INVALID_STATE);
    isle_tunnel_lifecycle_checked(s,s->actors->base.status);
    isle_tunnel_lifecycle_checked(s,isle_tunnel_lifecycle_native_request_scene(s,setup));
    s->scene_requested = 1;
    /* Independent entry domain: NOT an OM process, NOT cancelled with scene OM. */
    while (!s->scene_completion_ready) {
        if (isle_continuation_wait(s->domain,1) != ISLE_CONT_OK)
            isle_tunnel_lifecycle_checked(s,ISLE_GAME_INVALID_STATE);
    }
    /* Completion_ready is published only after the real owner join receipt.
     * This return path sees independent scalar state, never dead scene storage. */
    isle_tunnel_lifecycle_checked(s,entry_joined(s));
}
IsleGameResult isle_tunnel_lifecycle_complete_scene(IsleTunnelLifecycleSource* s, IsleGameResult result) {
    IsleGameResult joined;
    if (!s || !s->owner || !s->domain || !s->running || !s->scene_requested ||
        s->scene_completion_ready || s->nonwaiting_call_frame || s->D_802F01E4_5ED2B4 ||
        !s->actors || s->actors->base.phase != ISLE_COURSE_ACTOR_DETACHED ||
        s->actors->base.owner != s->owner || s->actors->base.nativeGame || s->actors->services ||
        s->actors->base.nonwaiting_call_frame)
        return ISLE_GAME_INVALID_STATE;
    if (isle_continuation_domain_check_access(s->domain) != ISLE_CONT_OK)
        return ISLE_GAME_INVALID_STATE;
    joined = isle_tunnel_lifecycle_native_scene_joined(s);
    if (joined != ISLE_GAME_OK) {
        if (s->status == ISLE_GAME_OK)
            s->status = s->actors->base.status != ISLE_GAME_OK ?
                s->actors->base.status : joined;
        return s->status;
    }
    /* The native hook reads the independent owner's retained join receipt,
     * not s->player or an already destroyed Game/sceneStorage. */
    s->player = NULL;
    s->screen.depth_buffer = NULL;
    if (s->status != ISLE_GAME_OK) result = s->status;
    else if (s->actors->base.status != ISLE_GAME_OK) result = s->actors->base.status;
    if (result != ISLE_GAME_OK && s->status == ISLE_GAME_OK) s->status = result;
    s->scene_completion_status = result;
    s->scene_completion_ready = 1;
    /* STOPPED/cancellation drains the borrow but never acknowledges success or
     * permits an original entry result, even if the host resumes this domain. */
    return result;
}
typedef struct TransitionCall {
    IsleTunnelLifecycleSource* source;
    int32_t reason;
    IsleGameResult result;
} TransitionCall;
static void transition_on_owner(void* user) {
    TransitionCall* call = user;
    call->result = isle_tunnel_lifecycle_owner_transition_end(call->source,call->reason);
}
IsleGameResult isle_tunnel_lifecycle_native_transition_end(IsleTunnelLifecycleSource* s, int32_t reason) {
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
static void init_call(IsleTunnelLifecycleSource* s, void* ignored) {
    (void)ignored;
    isle_tunnel_lifecycle_tunnel_Init(s);
}
IsleGameResult isle_tunnel_lifecycle_invoke_init(IsleTunnelLifecycleSource* s) {
    if (!s || !s->running || !s->scene_requested || s->scene_completion_ready)
        return ISLE_GAME_INVALID_STATE;
    return lifecycle_call(s,init_call,NULL);
}
static void end_call(IsleTunnelLifecycleSource* s, void* argument) {
    isle_tunnel_lifecycle_tunnel_EndLevel(s,*(int32_t*)argument);
}
IsleGameResult isle_tunnel_lifecycle_invoke_end(IsleTunnelLifecycleSource* s, int32_t reason) {
    return lifecycle_call(s,end_call,&reason);
}
IsleGameResult isle_tunnel_lifecycle_end_event(void* user, int32_t reason) {
    return isle_tunnel_lifecycle_invoke_end(user,reason);
}
static void pause_call(IsleTunnelLifecycleSource* s, void* argument) {
    isle_tunnel_lifecycle_tunnel_Pause(s,*(int32_t*)argument);
}
IsleGameResult isle_tunnel_lifecycle_pause_event(void* user, int32_t value) {
    return lifecycle_call(user,pause_call,&value);
}
typedef struct BlockCall { IsleTunnelLifecycleBlock *first, *second; } BlockCall;
static void add_call(IsleTunnelLifecycleSource* s, void* argument) {
    BlockCall* call = argument;
    isle_tunnel_lifecycle_tunnel_PokemonAdd(s,call->first,call->second);
}
static void change_call(IsleTunnelLifecycleSource* s, void* argument) {
    BlockCall* call = argument;
    isle_tunnel_lifecycle_tunnel_PokemonChangeBlock(s,call->first,call->second);
}
static void remove_call(IsleTunnelLifecycleSource* s, void* argument) {
    isle_tunnel_lifecycle_tunnel_PokemonRemove(s,argument);
}
static void exit_call(IsleTunnelLifecycleSource* s, void* argument) {
    isle_tunnel_lifecycle_tunnel_ExitBlock(s,argument);
}
IsleGameResult isle_tunnel_lifecycle_world_add(void* user, IsleTunnelLifecycleBlock* first, IsleTunnelLifecycleBlock* second) {
    BlockCall call = {first,second};
    return lifecycle_call(user,add_call,&call);
}
IsleGameResult isle_tunnel_lifecycle_world_change(void* user, IsleTunnelLifecycleBlock* first, IsleTunnelLifecycleBlock* second) {
    BlockCall call = {first,second};
    return lifecycle_call(user,change_call,&call);
}
IsleGameResult isle_tunnel_lifecycle_world_remove(void* user, IsleTunnelLifecycleBlock* block) {
    return lifecycle_call(user,remove_call,block);
}
IsleGameResult isle_tunnel_lifecycle_exit_block(void* user, IsleTunnelLifecycleBlock* block) {
    return lifecycle_call(user,exit_call,block);
}
typedef struct MovementCall { IsleTunnelLifecycleSource* source; int32_t block; float fraction; } MovementCall;
static void actor_movement_call(void* argument) {
    MovementCall* call = argument;
    tunnel_UpdateSounds(call->source->actors, call->block, call->fraction);
}
static void movement_call(IsleTunnelLifecycleSource* s, void* argument) {
    MovementCall* call = argument;
    isle_tunnel_lifecycle_checked(s, isle_tunnel_actor_invoke(s->actors, actor_movement_call, call));
}
IsleGameResult isle_tunnel_lifecycle_movement(void* user, int32_t block, float fraction) {
    MovementCall call = {user, block, fraction};
    return lifecycle_call(user, movement_call, &call);
}
typedef struct CollisionCall { IsleGameObject* object; const IsleActorGround* ground; } CollisionCall;
static void actor_collision_call(void* argument) {
    CollisionCall* call = argument;
    tunnel_HandleCollision(call->object, call->ground);
}
static void collision_call(IsleTunnelLifecycleSource* s, void* argument) {
    CollisionCall* call = argument;
    if (!call->object || !call->ground || call->object->game != s->actors->base.nativeGame)
        isle_tunnel_lifecycle_checked(s, ISLE_GAME_INVALID_ARGUMENT);
    isle_tunnel_lifecycle_checked(s, isle_tunnel_actor_invoke(s->actors, actor_collision_call, call));
}
IsleGameResult isle_tunnel_lifecycle_collide_item(void* user, IsleGameObject* object, const IsleActorGround* ground) {
    CollisionCall call = {object, ground};
    return lifecycle_call(user, collision_call, &call);
}
typedef struct CartEventCall { const IslePlayerCartNode* node; int32_t event; float argument; } CartEventCall;
static void cart_event_call(IsleTunnelLifecycleSource* s, void* argument) {
    CartEventCall* call = argument;
    isle_tunnel_lifecycle_func_802ED050_5EA120(s,call->node,call->event,call->argument);
}
void isle_tunnel_lifecycle_cart_event(void* user, const IslePlayerCartNode* node, int32_t event, float argument) {
    IsleTunnelLifecycleSource* s = user;
    CartEventCall call = {node,event,argument};
    IsleGameResult result = lifecycle_call(s,cart_event_call,&call);
    /* Canonical Cart C dispatch invokes this after returning native step. */
    isle_tunnel_lifecycle_checked(s,result);
}
void isle_tunnel_lifecycle_input_update(IsleGameObject* object) {
    IsleTunnelLifecycleSource* s = isle_tunnel_lifecycle_current(object);
    for (;;) {
        isle_tunnel_lifecycle_checked(s,isle_tunnel_lifecycle_native_invalidate_input(s));
        isle_player_source_wait(s->player,1);
    }
}
void isle_tunnel_lifecycle_camera_animation_update(IsleGameObject* object) {
    IsleTunnelLifecycleSource* s = isle_tunnel_lifecycle_current(object);
    isle_tunnel_lifecycle_checked(s,isle_player_root_native_camera_animation_update(s->player,object));
}
void isle_tunnel_lifecycle_delete_intro_cart(IsleTunnelLifecycleSource* s) {
    isle_tunnel_lifecycle_checked(s,lifecycle_active(s));
    IsleGameObject* object = isle_menu_current_object(s->player->game);
    isle_tunnel_lifecycle_checked(s,isle_tunnel_lifecycle_native_retire_intro_cart(s,object));
    /* Original whole-object deletion cancels its delayed sound process.
     * End self only after returning leaves; no C++ stack spans stop/fault. */
    isle_tunnel_lifecycle_checked(s,isle_game_end_source_processes(s->player->game,object,
        isle_tunnel_lifecycle_func_802ED1BC_5EA28C));
    isle_tunnel_lifecycle_checked(s,isle_game_end_process(s->player->game,NULL));
}

#define SCALAR_ADAPTER(name, type) \
type isle_tunnel_lifecycle_##name(IsleTunnelLifecycleSource* s) { \
    type value = 0; \
    isle_tunnel_lifecycle_checked(s,isle_tunnel_lifecycle_native_##name(s,&value)); \
    return value; \
}
SCALAR_ADAPTER(fx_bank_count,size_t)
SCALAR_ADAPTER(idle_enabled,int32_t)
#undef SCALAR_ADAPTER
int32_t isle_tunnel_lifecycle_photo_count(IsleTunnelLifecycleSource* s) {
    int32_t value = 0;
    isle_tunnel_lifecycle_checked(s,entry_joined(s));
    isle_tunnel_lifecycle_checked(s,isle_tunnel_lifecycle_native_photo_count(s,&value));
    return value;
}
IsleTunnelLifecycleBlock* isle_tunnel_lifecycle_block_next(IsleTunnelLifecycleSource* s, IsleTunnelLifecycleBlock* block) {
    IsleTunnelLifecycleBlock* value = NULL;
    isle_tunnel_lifecycle_checked(s,isle_tunnel_lifecycle_native_block_next(s,block,&value));
    return value;
}
IsleGameVec3 isle_tunnel_lifecycle_block_position(IsleTunnelLifecycleSource* s, IsleTunnelLifecycleBlock* block) {
    IsleGameVec3 value;
    isle_tunnel_lifecycle_checked(s,isle_tunnel_lifecycle_native_block_position(s,block,&value));
    return value;
}
void isle_tunnel_lifecycle_displace_items(IsleTunnelLifecycleSource* s, float x, float y, float z) {
    isle_tunnel_lifecycle_checked(s,isle_tunnel_lifecycle_native_displace_items(s,x,y,z));
}
void* isle_tunnel_lifecycle_depth_buffer(IsleTunnelLifecycleSource* s, uint32_t bytes, uint32_t alignment) {
    void* value = NULL;
    isle_tunnel_lifecycle_checked(s,isle_tunnel_lifecycle_native_depth_buffer(s,bytes,alignment,&value));
    return value;
}
int32_t isle_tunnel_lifecycle_play_sound(IsleTunnelLifecycleSource* s, int32_t sound,
    int32_t volume, int32_t pan, float pitch, int32_t reverb) {
    int32_t value = 0;
    isle_tunnel_lifecycle_checked(s,isle_tunnel_lifecycle_native_play_sound(s,sound,volume,pan,pitch,reverb,&value));
    return value;
}
int32_t isle_tunnel_lifecycle_sound_identity(IsleTunnelLifecycleSource* s, uint32_t handle) {
    int32_t value = 0;
    isle_tunnel_lifecycle_checked(s,isle_tunnel_lifecycle_native_sound_identity(s,handle,&value));
    return value;
}
const IsleTunnelLifecycleAsset* isle_tunnel_lifecycle_load(IsleTunnelLifecycleSource* s, IsleTunnelLifecycleAssetId id) {
    static const uint32_t ranges[5][2] = {
        {0xAB5860,0xAB5980},{0xAB5980,0xAB85E0},
        {0xABE7A0,0xABEBD0},{0xABEBD0,0xAC6890},{0xAB1470,0xAB5860}
    };
    const IsleTunnelLifecycleAsset* value = NULL;
    if ((unsigned)id >= sizeof(ranges)/sizeof(ranges[0]))
        isle_tunnel_lifecycle_checked(s,ISLE_GAME_INVALID_ARGUMENT);
    isle_tunnel_lifecycle_checked(s,isle_tunnel_lifecycle_native_load(s,id,ranges[id][0],ranges[id][1],&value));
    return value;
}
void isle_tunnel_lifecycle_init_ui(IsleTunnelLifecycleSource* s) {
    const IsleTunnelLifecycleUiCallbacks callbacks = {s,isle_tunnel_lifecycle_exit_block,
        isle_tunnel_lifecycle_movement,NULL,0,isle_tunnel_lifecycle_collide_item};
    IsleGameObject* object;
    isle_tunnel_lifecycle_checked(s,isle_tunnel_lifecycle_native_ui_bind(s,&callbacks));
    object = isle_player_initUI(s->player);
    isle_tunnel_lifecycle_checked(s,isle_tunnel_lifecycle_native_ui_initialized(s,object));
}
IsleGameObject* isle_tunnel_lifecycle_fx_camera(IsleTunnelLifecycleSource* s, int32_t a, int32_t b, IsleGameCamera* camera) {
    IsleGameObject* value = NULL;
    isle_tunnel_lifecycle_checked(s,isle_tunnel_lifecycle_native_fx_camera(s,a,b,camera,&value));
    return value;
}
IsleGameObject* isle_tunnel_lifecycle_fx_effects(IsleTunnelLifecycleSource* s, int32_t count) {
    IsleGameObject* value = NULL;
    isle_tunnel_lifecycle_checked(s,isle_tunnel_lifecycle_native_fx_effects(s,count,&value));
    return value;
}
IsleTunnelLifecycleEffect* isle_tunnel_lifecycle_create_effect(IsleTunnelLifecycleSource* s, int32_t bank, int32_t index) {
    IsleTunnelLifecycleEffect* value = NULL;
    isle_tunnel_lifecycle_checked(s,isle_tunnel_lifecycle_native_create_effect(s,bank,index,&value));
    return value;
}
#define CLIP_ADAPTER(name, type) \
const type* isle_tunnel_lifecycle_##name(IsleTunnelLifecycleSource* s, uint32_t identity) { \
    const type* value = NULL; \
    isle_tunnel_lifecycle_checked(s,isle_tunnel_lifecycle_native_##name(s,identity,&value)); \
    return value; \
}
CLIP_ADAPTER(camera_clip,IslePlayerCameraAnimationClip)
CLIP_ADAPTER(cart_model_clip,IslePlayerCartModelAnimation)
CLIP_ADAPTER(cart_material_clip,IslePlayerCartMaterialAnimation)
#undef CLIP_ADAPTER
IsleGameCamera* isle_tunnel_lifecycle_camera(IsleTunnelLifecycleSource* s) {
    isle_tunnel_lifecycle_checked(s,lifecycle_active(s));
    if (!s->player->camera) isle_tunnel_lifecycle_checked(s,ISLE_GAME_INVALID_STATE);
    return s->player->camera;
}
IsleGameObject* isle_tunnel_lifecycle_camera_object(IsleTunnelLifecycleSource* s) {
    IsleGameObject* value = NULL;
    IsleGameCamera* camera = isle_tunnel_lifecycle_camera(s);
    isle_tunnel_lifecycle_checked(s,isle_game_camera_object(s->player->game,camera,&value));
    return value;
}

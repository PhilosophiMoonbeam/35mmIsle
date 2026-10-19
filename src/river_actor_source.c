#include "river_actor_source.h"
#include "engine_internal.h"
#include "player_source.h"
#include <stdlib.h>
#include <string.h>
static uint16_t river_be16(const uint8_t* p) { return (uint16_t)((uint16_t)p[0] << 8 | p[1]); }
static uint32_t river_be32(const uint8_t* p) { return (uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | p[3]; }
static int32_t river_signed32(uint32_t value) { return value <= INT32_MAX ? (int32_t)value : -1-(int32_t)(UINT32_MAX-value); }
static float river_float(const uint8_t* p) { uint32_t bits=river_be32(p); float value; memcpy(&value,&bits,sizeof(value)); return value; }
static const uint8_t* river_wire(const uint8_t* rom, size_t bytes, uint32_t guest, size_t count) {
    static const uint32_t mappings[][3] = {
        {UINT32_C(0x8012a0d0), UINT32_C(0x0029a1a0), UINT32_C(0x70df0)},
        {UINT32_C(0x8019aee0), UINT32_C(0x0030af90), UINT32_C(0x1bc80)},
        {UINT32_C(0x802e2620), UINT32_C(0x006ca400), UINT32_C(0x5cee0)},
    };
    size_t i;
    for (i=0;i<sizeof(mappings)/sizeof(mappings[0]);++i) {
        const uint32_t* map=mappings[i];
        if (guest >= map[0] && guest-map[0] <= map[2] && count <= map[2]-(guest-map[0]) &&
            map[1] <= bytes && guest-map[0] <= bytes-map[1] && count <= bytes-map[1]-(guest-map[0]))
            return rom+map[1]+(guest-map[0]);
    }
    return NULL;
}
IsleGameResult isle_river_actor_prepare(IsleRiverActorSource* s, void* owner, const uint8_t* rom, size_t bytes) {
    const uint8_t* p;
    size_t i;
    IsleGameResult validation;
    if (!s || !owner || !rom || bytes != UINT32_C(0x1000000)) return ISLE_GAME_INVALID_ARGUMENT;
    if (s->base.phase != ISLE_COURSE_ACTOR_UNPREPARED || s->base.owner || s->base.nativeGame ||
        s->base.nonwaiting_call_frame || s->services || s->base.status != ISLE_GAME_OK)
        return ISLE_GAME_INVALID_STATE;
    validation=isle_river_actor_validate_initial_fields(rom,bytes);
    if (validation != ISLE_GAME_OK) return validation;
    if (!river_wire(rom, bytes, UINT32_C(0x802e2620), 1)) return ISLE_GAME_INVALID_STATE;
    if (!river_wire(rom, bytes, UINT32_C(0x802e28b4), 4)) return ISLE_GAME_INVALID_STATE;
    if (!river_wire(rom, bytes, UINT32_C(0x802e4b90), 4)) return ISLE_GAME_INVALID_STATE;
    if (!river_wire(rom, bytes, UINT32_C(0x802e29d0), 4)) return ISLE_GAME_INVALID_STATE;
    if (!river_wire(rom, bytes, UINT32_C(0x802e2bbc), 12)) return ISLE_GAME_INVALID_STATE;
    if (!river_wire(rom, bytes, UINT32_C(0x802e32dc), 12)) return ISLE_GAME_INVALID_STATE;
    if (!river_wire(rom, bytes, UINT32_C(0x802e3308), 12)) return ISLE_GAME_INVALID_STATE;
    if (!river_wire(rom, bytes, UINT32_C(0x802e364c), 12)) return ISLE_GAME_INVALID_STATE;
    if (!river_wire(rom, bytes, UINT32_C(0x802e3658), 12)) return ISLE_GAME_INVALID_STATE;
    if (!river_wire(rom, bytes, UINT32_C(0x802e3664), 12)) return ISLE_GAME_INVALID_STATE;
    if (!river_wire(rom, bytes, UINT32_C(0x802e3c88), 12)) return ISLE_GAME_INVALID_STATE;
    if (!river_wire(rom, bytes, UINT32_C(0x802e3c94), 18)) return ISLE_GAME_INVALID_STATE;
    if (!river_wire(rom, bytes, UINT32_C(0x802e4b00), 4)) return ISLE_GAME_INVALID_STATE;
    if (!river_wire(rom, bytes, UINT32_C(0x802e3da0), 4)) return ISLE_GAME_INVALID_STATE;
    if (!river_wire(rom, bytes, UINT32_C(0x802e4230), 4)) return ISLE_GAME_INVALID_STATE;
    if (!river_wire(rom, bytes, UINT32_C(0x802e4470), 4)) return ISLE_GAME_INVALID_STATE;
    if (!river_wire(rom, bytes, UINT32_C(0x802e4474), 4)) return ISLE_GAME_INVALID_STATE;
    if (!river_wire(rom, bytes, UINT32_C(0x802e4498), 6)) return ISLE_GAME_INVALID_STATE;
    if (!river_wire(rom, bytes, UINT32_C(0x802e4a60), 4)) return ISLE_GAME_INVALID_STATE;
    if (!river_wire(rom, bytes, UINT32_C(0x802e4af4), 12)) return ISLE_GAME_INVALID_STATE;
    if (!river_wire(rom, bytes, UINT32_C(0x802e4b04), 4)) return ISLE_GAME_INVALID_STATE;
    if (!river_wire(rom, bytes, UINT32_C(0x802e4b08), 4)) return ISLE_GAME_INVALID_STATE;
    if (!river_wire(rom, bytes, UINT32_C(0x802e4b0c), 4)) return ISLE_GAME_INVALID_STATE;
    if (!river_wire(rom, bytes, UINT32_C(0x802e4b10), 4)) return ISLE_GAME_INVALID_STATE;
    if (!river_wire(rom, bytes, UINT32_C(0x802e4b80), 4)) return ISLE_GAME_INVALID_STATE;
    if (!river_wire(rom, bytes, UINT32_C(0x802e4b84), 4)) return ISLE_GAME_INVALID_STATE;
    if (!river_wire(rom, bytes, UINT32_C(0x8012aca0), 4)) return ISLE_GAME_INVALID_STATE;
    if (!river_wire(rom, bytes, UINT32_C(0x802e28ac), 2)) return ISLE_GAME_INVALID_STATE;
    if (!river_wire(rom, bytes, UINT32_C(0x802e28b0), 2)) return ISLE_GAME_INVALID_STATE;
    p=river_wire(rom, bytes, UINT32_C(0x802e2620), 1);
    s->D_802E2620_6CA400=*p;
    p=river_wire(rom, bytes, UINT32_C(0x802e28b4), 4);
    s->D_802E28B4_6CA694=river_signed32(river_be32(p));
    p=river_wire(rom, bytes, UINT32_C(0x802e4b90), 4);
    s->D_802E4B90_6CC970=river_float(p);
    p=river_wire(rom, bytes, UINT32_C(0x802e29d0), 4);
    s->D_802E29D0_6CA7B0=river_be32(p);
    p=river_wire(rom, bytes, UINT32_C(0x802e2bbc), 12);
    s->D_802E2BBC_6CA99C=(IsleGameVec3){river_float(p),river_float(p+4),river_float(p+8)};
    p=river_wire(rom, bytes, UINT32_C(0x802e32dc), 12);
    s->D_802E32DC_6CB0BC=(IsleGameVec3){river_float(p),river_float(p+4),river_float(p+8)};
    p=river_wire(rom, bytes, UINT32_C(0x802e3308), 12);
    s->D_802E3308_6CB0E8=(IsleGameVec3){river_float(p),river_float(p+4),river_float(p+8)};
    p=river_wire(rom, bytes, UINT32_C(0x802e364c), 12);
    s->D_802E364C_6CB42C=(IsleGameVec3){river_float(p),river_float(p+4),river_float(p+8)};
    p=river_wire(rom, bytes, UINT32_C(0x802e3658), 12);
    s->D_802E3658_6CB438=(IsleGameVec3){river_float(p),river_float(p+4),river_float(p+8)};
    p=river_wire(rom, bytes, UINT32_C(0x802e3664), 12);
    s->D_802E3664_6CB444=(IsleGameVec3){river_float(p),river_float(p+4),river_float(p+8)};
    p=river_wire(rom, bytes, UINT32_C(0x802e3c88), 12);
    s->D_802E3C88_6CBA68=(IsleGameVec3){river_float(p),river_float(p+4),river_float(p+8)};
    p=river_wire(rom, bytes, UINT32_C(0x802e3c94), 18);
    for (i=0;i<9;++i) s->D_802E3C94_6CBA74.data[i]=river_be16(p+i*2);
    p=river_wire(rom, bytes, UINT32_C(0x802e4b00), 4);
    s->D_802E4B00_6CC8E0=NULL;
    p=river_wire(rom, bytes, UINT32_C(0x802e3da0), 4);
    s->D_802E3DA0_6CBB80=river_signed32(river_be32(p));
    p=river_wire(rom, bytes, UINT32_C(0x802e4230), 4);
    s->D_802E4230_6CC010=NULL;
    p=river_wire(rom, bytes, UINT32_C(0x802e4470), 4);
    s->D_802E4470_6CC250=river_signed32(river_be32(p));
    p=river_wire(rom, bytes, UINT32_C(0x802e4474), 4);
    s->D_802E4474_6CC254=river_be32(p);
    p=river_wire(rom, bytes, UINT32_C(0x802e4498), 6);
    for (i=0;i<3;++i) s->D_802E4498_6CC278.data[i]=river_be16(p+i*2);
    p=river_wire(rom, bytes, UINT32_C(0x802e4a60), 4);
    s->D_802E4A60_6CC840=river_float(p);
    p=river_wire(rom, bytes, UINT32_C(0x802e4af4), 12);
    for (i=0;i<3;++i) s->D_802E4AF4_6CC8D4[i]=river_float(p+i*4);
    p=river_wire(rom, bytes, UINT32_C(0x802e4b04), 4);
    s->D_802E4B04_6CC8E4=NULL;
    p=river_wire(rom, bytes, UINT32_C(0x802e4b08), 4);
    s->D_802E4B08_6CC8E8=river_signed32(river_be32(p));
    p=river_wire(rom, bytes, UINT32_C(0x802e4b0c), 4);
    s->D_802E4B0C_6CC8EC=river_float(p);
    p=river_wire(rom, bytes, UINT32_C(0x802e4b10), 4);
    s->D_802E4B10_6CC8F0=river_float(p);
    p=river_wire(rom, bytes, UINT32_C(0x802e4b80), 4);
    s->D_802E4B80_6CC960=river_signed32(river_be32(p));
    p=river_wire(rom, bytes, UINT32_C(0x802e4b84), 4);
    s->D_802E4B84_6CC964=NULL;
    p=river_wire(rom, bytes, UINT32_C(0x8012aca0), 4);
    s->D_8012ACA0_29AD70=river_float(p);
    p=river_wire(rom, bytes, UINT32_C(0x802e28ac), 2);
    s->D_802E28AC_6CA68C=river_be16(p);
    p=river_wire(rom, bytes, UINT32_C(0x802e28b0), 2);
    s->D_802E28B0_6CA690=river_be16(p);
    memset(s->D_8033F500_7272E0,0,sizeof(s->D_8033F500_7272E0));
    s->D_8033F5C8_7273A8=NULL;
    s->D_8033F5CC_7273AC=0;
    memset(s->D_8033F5D0_7273B0,0,sizeof(s->D_8033F5D0_7273B0));
    memset(s->D_8033F670_7272E0,0,sizeof(s->D_8033F670_7272E0));
    s->D_8033F678_727458=NULL;
    s->D_8033F67C_72745C=0;
    s->D_8033F680_727460=0;
    s->D_8033F681_727461=0;
    memset(s->D_8033F690_7272E0,0,sizeof(s->D_8033F690_7272E0));
    s->D_8033F698_727478=0;
    s->D_8033F699_727479=0;
    s->D_8033F69A_72747A=0;
    s->D_8033F69C_72747C=0;
    memset(s->D_8033F6A0_727480,0,sizeof(s->D_8033F6A0_727480));
    s->base.course_id=3;
    s->base.owner=owner;
    s->base.status=ISLE_GAME_OK;
    s->base.phase=ISLE_COURSE_ACTOR_PREPARED;
    return ISLE_GAME_OK;
}
static int river_services_complete(const IsleRiverActorServices* p) {
    return p && p->reference_assign && p->model && p->model_read && p->model_store && p->root_position && p->root_store && p->node_descend && p->node_world && p->current_block && p->block_position && p->path_count && p->path_duration && p->path_parameter && p->path_point && p->item && p->player_object && p->player_model && p->rng_range && p->weighted && p->counted_options && p->graph && p->add_one && p->ground && p->sound && p->sound_params && p->sound_active && p->sound_stop && p->bgm_play && p->bgm_volume && p->bgm_volume_smooth && p->sound_volume_smooth && p->sound_global_volume && p->animation_speed && p->model_speed && p->camera && p->camera_read && p->camera_store && p->camera_speed && p->camera_animation && p->camera_process && p->player_model_init && p->player_animation && p->player_change_animation && p->player_animation_speed && p->animation_callback && p->pause_processes && p->resume_processes && p->sky_speed && p->notice && p->progress_read && p->progress_write && p->transition_prepare && p->transition_color && p->transition_fade && p->transition_active && p->create_helper && p->delete_object && p->send_command && p->send_command_link && p->distance && p->sound_stop_all && p->bgm_stop_all && p->remove_flying_items && p->reset_messages && p->audio_cutscene && p->source_process && p->environment_sound && p->definition && p->root_read;
}
IsleGameResult isle_river_actor_activate(IsleRiverActorSource* s, IsleGame* game, const IsleRiverActorServices* services) {
    IsleGameResult result;
    if (!s || !game || !river_services_complete(services)) return ISLE_GAME_INVALID_ARGUMENT;
    if (s->base.phase == ISLE_COURSE_ACTOR_ACTIVE) {
        if (s->base.nativeGame != game || s->services != services) return ISLE_GAME_INVALID_STATE;
        if (s->base.status != ISLE_GAME_OK) return s->base.status;
        return isle_river_actor_native_admission(s, game, services);
    }
    if (s->base.phase != ISLE_COURSE_ACTOR_PREPARED || s->base.course_id != 3 ||
        !s->base.owner || s->base.nativeGame || s->services || s->base.nonwaiting_call_frame)
        return ISLE_GAME_INVALID_STATE;
    if (s->base.status != ISLE_GAME_OK) return s->base.status;
    result=isle_river_actor_native_admission(s,game,services);
    if (result != ISLE_GAME_OK) return result;
    s->base.nativeGame=game;
    s->services=services;
    s->base.phase=ISLE_COURSE_ACTOR_ACTIVE;
    return ISLE_GAME_OK;
}
IsleGameResult isle_river_actor_deactivate(IsleRiverActorSource* s) {
    IsleGameResult result, status;
    if (!s || s->base.phase != ISLE_COURSE_ACTOR_ACTIVE || !s->base.nativeGame ||
        !s->base.owner || !river_services_complete(s->services) || s->base.nonwaiting_call_frame)
        return ISLE_GAME_INVALID_STATE;
    status=isle_actor_source_retirement_status(s->base.nativeGame);
    if (s->base.status == ISLE_GAME_OK) s->base.status=status;
    result=isle_river_actor_native_retirement(s);
    if (result != ISLE_GAME_OK) return result;
    if (s->D_802E4B00_6CC8E0 || s->D_802E4230_6CC010 || s->D_802E4B04_6CC8E4 || s->D_802E4B84_6CC964 || s->D_8033F5C8_7273A8 || s->D_8033F678_727458) return ISLE_GAME_INVALID_STATE;
    s->base.nativeGame=NULL;
    s->services=NULL;
    s->base.phase=ISLE_COURSE_ACTOR_DETACHED;
    return ISLE_GAME_OK;
}
void isle_river_actor_checked(IsleRiverActorSource* s, IsleGameResult result) {
    isle_course_actor_checked(s ? &s->base : NULL, result);
}
void isle_river_actor_enter(IsleRiverActorSource* s) {
    if (!s || s->base.course_id != 3 || s->base.phase != ISLE_COURSE_ACTOR_ACTIVE ||
        !s->base.owner || !s->base.nativeGame || !river_services_complete(s->services))
        isle_river_actor_checked(s,ISLE_GAME_INVALID_STATE);
    isle_river_actor_checked(s,s->base.status);
    isle_river_actor_checked(s,isle_river_actor_native_admission(s,s->base.nativeGame,s->services));
}
IsleRiverActorSource* isle_river_actor_current(IsleGameObject* object) {
    IsleRiverActorSource* s=NULL;
    IsleGameResult result=isle_river_actor_native_context(object ? object->game : NULL,object,&s);
    if (result != ISLE_GAME_OK || !s || !object || s->base.nativeGame != object->game)
        isle_native_execution_fault(object ? object->game : NULL,"River callback requires its canonical receiver context");
    isle_river_actor_enter(s);
    return s;
}
IsleGameResult isle_river_actor_invoke(IsleRiverActorSource* s, IsleCourseActorCall callback, void* argument) {
    IsleGameResult result;
    if (!s || !callback || s->base.phase != ISLE_COURSE_ACTOR_ACTIVE || !river_services_complete(s->services)) return ISLE_GAME_INVALID_STATE;
    result=isle_river_actor_native_admission(s,s->base.nativeGame,s->services);
    if (result != ISLE_GAME_OK) return result;
    return isle_course_actor_invoke(&s->base,callback,argument);
}
typedef struct RiverAnimationCall { IsleRiverActorSource* s; IsleRiverAnimationCallback callback; const IsleActorNodeRef* receiver; int32_t event; float value; } RiverAnimationCall;
static void river_animation_call(void* argument) {
    RiverAnimationCall* call=argument;
    call->callback(call->s,call->receiver,call->event,call->value);
}
IsleGameResult isle_river_actor_invoke_animation(IsleRiverActorSource* s, IsleRiverAnimationCallback callback, const IsleActorNodeRef* receiver, int32_t event, float value) {
    RiverAnimationCall call={s,callback,receiver,event,value};
    if (!callback || !receiver) return ISLE_GAME_INVALID_ARGUMENT;
    return isle_river_actor_invoke(s,river_animation_call,&call);
}
typedef struct RiverCollisionCall { IsleGameObject* object; IsleActorGround* ground; } RiverCollisionCall;
static void river_collision_call(void* argument) { RiverCollisionCall* call=argument; func_802D8E98_6C0C78(call->object,call->ground); }
IsleGameResult isle_river_actor_invoke_item_collision(IsleRiverActorSource* s, IsleGameObject* object, IsleActorGround* ground) {
    RiverCollisionCall call={object,ground};
    if (!object || !ground) return ISLE_GAME_INVALID_ARGUMENT;
    return isle_river_actor_invoke(s,river_collision_call,&call);
}
void isle_river_actor_reference_assign(IsleRiverActorSource* s, IsleGameObject** field, IsleGameObject* value) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->reference_assign(s, field, value));
}

IsleGameModelRef isle_river_actor_model(IsleRiverActorSource* s, IsleGameObject* object) {
    IsleGameModelRef value = {0};
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->model(s, object, &value));
    return value;
}

const IsleGameModelTransform* isle_river_actor_model_read(IsleRiverActorSource* s, IsleGameModelRef model) {
    const IsleGameModelTransform* value = NULL;
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->model_read(s, model, &value));
    return value;
}

void isle_river_actor_model_store(IsleRiverActorSource* s, IsleGameModelRef model, uint32_t component, float value) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->model_store(s, model, component, value));
}

IsleGameVec3 isle_river_actor_root_position(IsleRiverActorSource* s, IsleGameModelRef model) {
    IsleGameVec3 value = {0};
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->root_position(s, model, &value));
    return value;
}

void isle_river_actor_root_store(IsleRiverActorSource* s, IsleGameModelRef model, uint32_t component, float value) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->root_store(s, model, component, value));
}

IsleActorNodeRef isle_river_actor_node_descend(IsleRiverActorSource* s, IsleGameModelRef model, const uint8_t* steps, size_t count) {
    IsleActorNodeRef value = {0};
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->node_descend(s, model, steps, count, &value));
    return value;
}

void isle_river_actor_node_world(IsleRiverActorSource* s, IsleActorNodeRef node, IsleGameVec3* position, IsleGameVec3* velocity) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->node_world(s, node, position, velocity));
}

IsleRiverBlock* isle_river_actor_current_block(IsleRiverActorSource* s) {
    IsleRiverBlock* value = NULL;
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->current_block(s, &value));
    return value;
}

IsleGameVec3 isle_river_actor_block_position(IsleRiverActorSource* s, IsleRiverBlock* block) {
    IsleGameVec3 value = {0};
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->block_position(s, block, &value));
    return value;
}

int32_t isle_river_actor_path_count(IsleRiverActorSource* s, const void* path) {
    int32_t value = {0};
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->path_count(s, path, &value));
    return value;
}

float isle_river_actor_path_duration(IsleRiverActorSource* s, const void* path) {
    float value = {0};
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->path_duration(s, path, &value));
    return value;
}

float isle_river_actor_path_parameter(IsleRiverActorSource* s, const void* path, int32_t index) {
    float value = {0};
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->path_parameter(s, path, index, &value));
    return value;
}

const IsleGameVec3* isle_river_actor_path_point(IsleRiverActorSource* s, const void* path, int32_t index) {
    const IsleGameVec3* value = NULL;
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->path_point(s, path, index, &value));
    return value;
}

const IsleRiverItemView* isle_river_actor_item(IsleRiverActorSource* s, IsleGameObject* object) {
    const IsleRiverItemView* value = NULL;
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->item(s, object, &value));
    return value;
}

IsleGameObject* isle_river_actor_player_object(IsleRiverActorSource* s) {
    IsleGameObject* value = NULL;
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->player_object(s, &value));
    return value;
}

IsleGameModelRef isle_river_actor_player_model(IsleRiverActorSource* s) {
    IsleGameModelRef value = {0};
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->player_model(s, &value));
    return value;
}

int32_t isle_river_actor_rng_range(IsleRiverActorSource* s, int32_t range) {
    int32_t value = {0};
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->rng_range(s, range, &value));
    return value;
}

const IsleActorRandomState* isle_river_actor_weighted(IsleRiverActorSource* s, IsleGameObject* receiver, uint32_t identity) {
    const IsleActorRandomState* value = NULL;
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->weighted(s, receiver, identity, &value));
    return value;
}

const IsleRiverCountedOption* isle_river_actor_counted_options(IsleRiverActorSource* s, uint32_t identity, size_t* count) {
    const IsleRiverCountedOption* value = NULL;
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->counted_options(s, identity, count, &value));
    return value;
}

const IsleActorInteractionGraph* isle_river_actor_graph(IsleRiverActorSource* s, IsleGameObject* receiver, uint32_t identity) {
    const IsleActorInteractionGraph* value = NULL;
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->graph(s, receiver, identity, &value));
    return value;
}

IsleGameObject* isle_river_actor_add_one(IsleRiverActorSource* s, IsleRiverBlock* a, IsleRiverBlock* b, const IsleRiverAutomaticSpawn* spawn, const IsleRiverDefinition* definition) {
    IsleGameObject* value = NULL;
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->add_one(s, a, b, spawn, definition, &value));
    return value;
}

void isle_river_actor_ground(IsleRiverActorSource* s, float x, float z, IsleActorGround* ground) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->ground(s, x, z, ground));
}


int32_t isle_river_actor_sound(IsleRiverActorSource* s, int32_t id) {
    int32_t value = {0};
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->sound(s, id, &value));
    return value;
}

int32_t isle_river_actor_sound_params(IsleRiverActorSource* s, int32_t id, int32_t volume, int32_t pan, float pitch, int32_t priority) {
    int32_t value = {0};
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->sound_params(s, id, volume, pan, pitch, priority, &value));
    return value;
}

int32_t isle_river_actor_sound_active(IsleRiverActorSource* s, int32_t handle) {
    int32_t value = {0};
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->sound_active(s, handle, &value));
    return value;
}

void isle_river_actor_sound_stop(IsleRiverActorSource* s, int32_t handle) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->sound_stop(s, handle));
}

void isle_river_actor_bgm_play(IsleRiverActorSource* s, int32_t player, int32_t song) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->bgm_play(s, player, song));
}

void isle_river_actor_bgm_volume(IsleRiverActorSource* s, int32_t player, uint32_t volume) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->bgm_volume(s, player, volume));
}

void isle_river_actor_bgm_volume_smooth(IsleRiverActorSource* s, int32_t player, int32_t volume, int32_t frames) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->bgm_volume_smooth(s, player, volume, frames));
}

void isle_river_actor_sound_volume_smooth(IsleRiverActorSource* s, int32_t handle, int32_t from, int32_t to, int32_t frames) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->sound_volume_smooth(s, handle, from, to, frames));
}

void isle_river_actor_sound_global_volume(IsleRiverActorSource* s, int32_t volume) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->sound_global_volume(s, volume));
}

void isle_river_actor_animation_speed(IsleRiverActorSource* s, IsleGameObject* object, float speed, int32_t materials) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->animation_speed(s, object, speed, materials));
}

float isle_river_actor_model_speed(IsleRiverActorSource* s, IsleGameModelRef model) {
    float value = {0};
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->model_speed(s, model, &value));
    return value;
}

IsleGameObject* isle_river_actor_camera(IsleRiverActorSource* s) {
    IsleGameObject* value = NULL;
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->camera(s, &value));
    return value;
}

const IsleRiverCameraView* isle_river_actor_camera_read(IsleRiverActorSource* s, IsleGameObject* camera) {
    const IsleRiverCameraView* value = NULL;
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->camera_read(s, camera, &value));
    return value;
}

void isle_river_actor_camera_store(IsleRiverActorSource* s, IsleGameObject* camera, uint32_t component, float value) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->camera_store(s, camera, component, value));
}

void isle_river_actor_camera_speed(IsleRiverActorSource* s, IsleGameObject* camera, float speed) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->camera_speed(s, camera, speed));
}

void isle_river_actor_camera_animation(IsleRiverActorSource* s, IsleGameObject* camera, uint32_t identity, float start) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->camera_animation(s, camera, identity, start));
}

IsleGameProcess* isle_river_actor_camera_process(IsleRiverActorSource* s, IsleGameObject* camera, int32_t kind, int32_t priority) {
    IsleGameProcess* value = NULL;
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->camera_process(s, camera, kind, priority, &value));
    return value;
}

IsleGameObject* isle_river_actor_player_model_init(IsleRiverActorSource* s) {
    IsleGameObject* value = NULL;
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->player_model_init(s, &value));
    return value;
}

void isle_river_actor_player_animation(IsleRiverActorSource* s, uint32_t model, uint32_t material, float start, float speed) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->player_animation(s, model, material, start, speed));
}

void isle_river_actor_player_change_animation(IsleRiverActorSource* s, uint32_t model, uint32_t material, float start) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->player_change_animation(s, model, material, start));
}

void isle_river_actor_player_animation_speed(IsleRiverActorSource* s, float speed) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->player_animation_speed(s, speed));
}

void isle_river_actor_animation_callback(IsleRiverActorSource* s, IsleGameObject* receiver, IsleRiverAnimationCallback callback) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->animation_callback(s, receiver, callback));
}

void isle_river_actor_pause_processes(IsleRiverActorSource* s, IsleGameObject* object) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->pause_processes(s, object));
}

void isle_river_actor_resume_processes(IsleRiverActorSource* s, IsleGameObject* object) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->resume_processes(s, object));
}

void isle_river_actor_sky_speed(IsleRiverActorSource* s, float speed) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->sky_speed(s, speed));
}


void isle_river_actor_notice(IsleRiverActorSource* s, uint32_t bits) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->notice(s, bits));
}

int32_t isle_river_actor_progress_read(IsleRiverActorSource* s) {
    int32_t value = {0};
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->progress_read(s, &value));
    return value;
}

void isle_river_actor_progress_write(IsleRiverActorSource* s, int32_t value) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->progress_write(s, value));
}

void isle_river_actor_transition_prepare(IsleRiverActorSource* s, int32_t a, int32_t b) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->transition_prepare(s, a, b));
}

void isle_river_actor_transition_color(IsleRiverActorSource* s, int32_t r, int32_t g, int32_t b) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->transition_color(s, r, g, b));
}

void isle_river_actor_transition_fade(IsleRiverActorSource* s, int32_t direction, float duration) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->transition_fade(s, direction, duration));
}

int32_t isle_river_actor_transition_active(IsleRiverActorSource* s) {
    int32_t value = {0};
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->transition_active(s, &value));
    return value;
}

IsleGameObject* isle_river_actor_create_helper(IsleRiverActorSource* s, int32_t id, IsleGameSourceCallback update, int32_t link, uint32_t priority) {
    IsleGameObject* value = NULL;
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->create_helper(s, id, update, link, priority, &value));
    return value;
}

void isle_river_actor_delete_object(IsleRiverActorSource* s, IsleGameObject* object) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->delete_object(s, object));
    if (!object) omEndProcess(NULL);
}


void isle_river_actor_send_command(IsleRiverActorSource* s, IsleGameObject* destination, int32_t command, IsleGameObject* sender) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->send_command(s, destination, command, sender));
}

void isle_river_actor_send_command_link(IsleRiverActorSource* s, int32_t link, int32_t command, IsleGameObject* sender) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->send_command_link(s, link, command, sender));
}

float isle_river_actor_distance(IsleRiverActorSource* s, IsleGameObject* a, IsleGameObject* b) {
    float value = {0};
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->distance(s, a, b, &value));
    return value;
}

void isle_river_actor_sound_stop_all(IsleRiverActorSource* s) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->sound_stop_all(s));
}

void isle_river_actor_bgm_stop_all(IsleRiverActorSource* s) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->bgm_stop_all(s));
}

void isle_river_actor_remove_flying_items(IsleRiverActorSource* s) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->remove_flying_items(s));
}


void isle_river_actor_reset_messages(IsleRiverActorSource* s) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->reset_messages(s));
}

void isle_river_actor_audio_cutscene(IsleRiverActorSource* s) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->audio_cutscene(s));
}


IsleGameProcess* isle_river_actor_source_process(IsleRiverActorSource* s, IsleGameObject* object, IsleGameSourceCallback callback, int32_t kind, int32_t priority) {
    IsleGameProcess* value = NULL;
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->source_process(s, object, callback, kind, priority, &value));
    return value;
}

void isle_river_actor_environment_sound(IsleRiverActorSource* s, IsleGameObject* object, uint8_t category, int32_t sound) {
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->environment_sound(s, object, category, sound));
}

IsleRiverDefinition isle_river_actor_definition(IsleRiverActorSource* s, uint32_t identity, uint32_t function, uint32_t line) {
    IsleRiverDefinition value = {0};
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->definition(s, identity, function, line, &value));
    return value;
}


const IsleGameVec3* isle_river_actor_root_read(IsleRiverActorSource* s, IsleGameModelRef model) {
    const IsleGameVec3* value = NULL;
    isle_river_actor_enter(s);
    isle_river_actor_checked(s, s->services->root_read(s, model, &value));
    return value;
}
IsleActorSource* isle_river_actor_source(IsleRiverActorSource* s, IsleGameObject* object) {
    IsleActorSource* value = NULL;
    isle_river_actor_checked(s, isle_actor_source_get(s->base.nativeGame, object, &value));
    return value;
}
const IsleGameVec3* isle_river_actor_vector_read(IsleRiverActorSource* s, IsleRiverVectorRef ref) {
    const IsleGameModelTransform* transform;
    if (ref.root) {
        if (ref.component) isle_river_actor_checked(s, ISLE_GAME_INVALID_ARGUMENT);
        return isle_river_actor_root_read(s, ref.model);
    }
    transform = isle_river_actor_model_read(s, ref.model);
    switch (ref.component) {
    case 0: return &transform->position;
    case 1: return &transform->rotation;
    case 2: return &transform->scale;
    default: isle_river_actor_checked(s, ISLE_GAME_INVALID_ARGUMENT); abort();
    }
}
void isle_river_actor_vector_store(IsleRiverActorSource* s, IsleRiverVectorRef ref, uint32_t axis, float value) {
    if (axis > 2 || ref.component > 2) isle_river_actor_checked(s, ISLE_GAME_INVALID_ARGUMENT);
    if (ref.root) {
        if (ref.component) isle_river_actor_checked(s, ISLE_GAME_INVALID_ARGUMENT);
        isle_river_actor_root_store(s, ref.model, axis, value);
    } else isle_river_actor_model_store(s, ref.model, ref.component * 3u + axis, value);
}
IsleRiverGraph2 isle_river_actor_graph2(IsleRiverActorSource* s, IsleGameObject* object, uint32_t identity) {
    IsleRiverGraph2 value;
    const IsleActorInteractionGraph* graph = isle_river_actor_graph(s, object, identity);
    if (!graph || graph->count != 2) isle_river_actor_checked(s, ISLE_GAME_INVALID_STATE);
    memcpy(value.data, graph->rows, sizeof(value.data));
    return value;
}
IsleRiverGraph3 isle_river_actor_graph3(IsleRiverActorSource* s, IsleGameObject* object, uint32_t identity) {
    IsleRiverGraph3 value;
    const IsleActorInteractionGraph* graph = isle_river_actor_graph(s, object, identity);
    if (!graph || graph->count != 3) isle_river_actor_checked(s, ISLE_GAME_INVALID_STATE);
    memcpy(value.data, graph->rows, sizeof(value.data));
    return value;
}
const IsleRiverCountedOption* isle_river_actor_counted(IsleRiverActorSource* s, uint32_t identity, uint16_t index) {
    size_t count = 0;
    const IsleRiverCountedOption* options = isle_river_actor_counted_options(s, identity, &count);
    if (!options || count != 3 || index >= count) isle_river_actor_checked(s, ISLE_GAME_INVALID_STATE);
    return &options[index];
}
IslePlayerSource* isle_river_actor_player(IsleRiverActorSource* s) {
    IslePlayerSource* player=NULL;
    isle_river_actor_enter(s);
    isle_river_actor_checked(s,isle_player_source_context(s->base.nativeGame,&player));
    if (!player) isle_river_actor_checked(s,ISLE_GAME_INVALID_STATE);
    return player;
}

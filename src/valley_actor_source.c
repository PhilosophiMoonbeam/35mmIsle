
#include "valley_actor_source.h"
#include "valley_actor_native_source.h"
#include "engine_internal.h"
#include <stdlib.h>
#include <string.h>
static uint32_t read32(const uint8_t* p) { return (uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | p[3]; }
static uint16_t read16(const uint8_t* p) { return (uint16_t)((uint16_t)p[0]<<8 | p[1]); }
static float readfloat(const uint8_t* p) { uint32_t bits=read32(p); float value; memcpy(&value,&bits,4); return value; }
static int services_complete(const IsleValleyActorServices* p) { return p && p->reference_assign && p->model && p->player_model && p->node_path && p->vector && p->vector_read && p->vector_store && p->item && p->item_direction && p->current_block && p->graph && p->graph_copy && p->weighted && p->weighted_copy && p->forbidden_ground && p->add_one && p->add_geo && p->ground && p->node_world && p->level_progress && p->path_position && p->path_start && p->animation_callback && p->model_animation_speed && p->camera && p->camera_speed && p->camera_read && p->camera_component && p->camera_object && p->camera_animation && p->camera_animation_process && p->pause_processes && p->resume_processes && p->player_model_init && p->player_model_animation && p->player_model_change && p->player_model_speed && p->cutscene_start && p->sound && p->sound_params && p->sound_active && p->sound_stop && p->sound_stop_all && p->sound_volume && p->sound_global_volume && p->sound_volume_smooth && p->bgm_play && p->bgm_stop && p->bgm_volume && p->bgm_volume_smooth && p->background_music && p->check_player_flag && p->set_player_flag && p->remove_flying_items && p->reset_camera && p->sky_speed && p->message_reset && p->message_interactive && p->message_noninteractive && p->message_printed && p->message_show && p->message_handler && p->dialogue && p->sprite && p->sprite_list && p->sprite_add && p->sprite_move && p->sprite_x2_move && p->sprite_scale && p->sprite_color && p->sprite_link && p->object_add && p->object_default_add && p->input_pressed && p->player_object && p->player_name && p->effect_create && p->effect_attach && p->application_notice && p->transition_begin && p->transition_color && p->transition_busy && p->audio_cutscene_reset && p->environment_sound && p->kick_item && p->jump_bounce && p->common_child1 && p->common_child2 && p->block_change && p->block_change_ground && p->common_remove && p->misc_object_get && p->misc_object_set && p->model_time_left && p->rng_range && p->rng_float && p->player_cutscene_process && p->flute && p->command && p->command_link && p->vector_distance && p->dialogue_format; }
IsleGameResult isle_valley_actor_prepare(IsleValleyActorSource* s, void* owner, const uint8_t* rom, size_t bytes) {
    if (!s || !owner || !rom || bytes!=0x1000000u) return ISLE_GAME_INVALID_ARGUMENT;
    if (s->base.phase!=ISLE_COURSE_ACTOR_UNPREPARED || s->base.owner || s->base.nativeGame || s->services || s->base.nonwaiting_call_frame || s->base.status!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7abf10)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x7abf44]!=10 || read32(rom+0x7abf48)!=UINT32_C(0x802c5e68)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x7abf4c]!=10 || read32(rom+0x7abf50)!=UINT32_C(0x802c5f70)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x7abf54]!=10 || read32(rom+0x7abf58)!=UINT32_C(0x802c606c)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x7abf5c]!=70 || read32(rom+0x7abf60)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ad0c4)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ac014)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ac11c)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ac120)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ac304)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ac580)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ad580)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ad584)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ad734)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5bd4)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5bd8)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7abdb8)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7abf0c)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7abf10)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ad0c4)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ac010)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ac014)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ac020)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ac024)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ac11c)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ac120)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ac184)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ac188)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ac18c)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ac190)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ac194)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ac198)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ac19c)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ac304)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read16(rom+0x7ac308)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ac580)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read16(rom+0x7ac584)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ac670)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ac674)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ac678)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7acbc8)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ad580)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7acdd4)!=UINT32_C(0x3f800000)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7acdd8)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7acddc)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7acf5c)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ad584)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ad588)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ad5fc)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ad734)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5bd4)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ad758)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ad75c)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7ad760)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5b78)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5b7c)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5bc8)!=UINT32_C(0x3dcccccd)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5bcc)!=UINT32_C(0x3f000000)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5bd0)!=UINT32_C(0x3f800000)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5bd8)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5bdc)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5be0)!=UINT32_C(0xbf800000)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5be4)!=UINT32_C(0xbf800000)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x3f6b70)!=UINT32_C(0x40000000)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5ca0)!=UINT32_C(0xffffffff)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5ca4)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read16(rom+0x7c5cc8)!=UINT32_C(0x3e9)) return ISLE_GAME_INVALID_STATE;
    if (read16(rom+0x7c5cca)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read16(rom+0x7c5df8)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read16(rom+0x7c5dfc)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5e20)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5e24)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5e68)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5e6c)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5e70)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5ea4)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5ea8)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5eac)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5fa0)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5fa4)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5fa8)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    /* Qualified639b: actual consumed prefixes, not neighboring allocations.
     * Terminal SomeC rows consume ONLY their NULL pointer. BSS text operand
     * 8034e080 is asserted as an identity, never read through this ROM view. */
    if (read32(rom+0x7c5aac)!=UINT32_C(0x802ece00) || rom[0x7c5ab0]!=0 || rom[0x7c5ab1]!=0 || read32(rom+0x7c5ab4)!=UINT32_C(0x8034e080)) return ISLE_GAME_INVALID_STATE;
    if (memcmp(rom+0x7c6390,"\x5c\x42\x4f\x68\x21\x20\x48\x69\x2c\x20\x25\x73\x21\x0a\x57\x68\x61\x74\x20\x61\x20\x73\x75\x72\x70\x72\x69\x73\x65\x20\x74\x6f\x20\x73\x65\x65\x20\x79\x6f\x75\x20\x68\x65\x72\x65\x21\x00",47u)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5ab8)!=UINT32_C(0x802ece30) || rom[0x7c5abc]!=0 || rom[0x7c5abd]!=0 || read32(rom+0x7c5ac0)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (memcmp(rom+0x7c63c0,"\x49\x20\x73\x75\x70\x70\x6f\x73\x65\x20\x79\x6f\x75\x27\x64\x20\x6c\x69\x6b\x65\x20\x74\x6f\x0a\x6b\x6e\x6f\x77\x20\x77\x68\x79\x20\x49\x20\x61\x6d\x20\x68\x65\x72\x65\x3f\x0a\x41\x68\x68\x2c\x20\x77\x65\x6c\x6c\x2e\x2e\x2e\x00",57u)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5ac4)!=UINT32_C(0x802ece6c) || rom[0x7c5ac8]!=0 || rom[0x7c5ac9]!=0 || read32(rom+0x7c5acc)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (memcmp(rom+0x7c63fc,"\x49\x20\x72\x65\x63\x65\x69\x76\x65\x64\x20\x73\x6f\x6d\x65\x20\x69\x6e\x74\x65\x72\x65\x73\x74\x69\x6e\x67\x0a\x69\x6e\x66\x6f\x72\x6d\x61\x74\x69\x6f\x6e\x20\x61\x74\x0a\x74\x68\x65\x20\x50\x6f\x6b\xa6\xc5\x6d\x6f\x6e\x20\x4c\x61\x62\x2e\x00",61u)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5ad0)!=UINT32_C(0x802eceac) || rom[0x7c5ad4]!=0 || rom[0x7c5ad5]!=1 || read32(rom+0x7c5ad8)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (memcmp(rom+0x7c643c,"\x49\x74\x20\x73\x65\x65\x6d\x73\x20\x74\x68\x61\x74\x20\x74\x68\x65\x72\x65\x20\x61\x72\x65\x20\x73\x69\x78\x0a\x50\x4b\x4d\x4e\x20\x53\x69\x67\x6e\x73\x20\x68\x69\x64\x64\x65\x6e\x20\x6f\x6e\x20\x74\x68\x69\x73\x0a\x69\x73\x6c\x61\x6e\x64\x2e\x00",62u)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5adc)!=UINT32_C(0x802eceec) || rom[0x7c5ae0]!=0 || rom[0x7c5ae1]!=0 || read32(rom+0x7c5ae4)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (memcmp(rom+0x7c647c,"\x54\x68\x65\x79\x20\x68\x61\x76\x65\x20\x73\x6f\x6d\x65\x74\x68\x69\x6e\x67\x20\x74\x6f\x20\x64\x6f\x0a\x77\x69\x74\x68\x20\x69\x6d\x70\x6f\x72\x74\x61\x6e\x74\x20\x50\x6f\x6b\xa6\xc5\x6d\x6f\x6e\x0a\x74\x68\x61\x74\x20\x61\x72\x65\x20\x68\x65\x72\x65\x2e\x00",65u)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5ae8)!=UINT32_C(0x802ecf30) || rom[0x7c5aec]!=0 || rom[0x7c5aed]!=0 || read32(rom+0x7c5af0)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (memcmp(rom+0x7c64c0,"\x54\x68\x65\x20\x53\x69\x67\x6e\x73\x20\x61\x72\x65\x20\x73\x63\x65\x6e\x65\x72\x79\x20\x77\x68\x69\x63\x68\x0a\x73\x65\x65\x6d\x20\x74\x6f\x20\x72\x65\x73\x65\x6d\x62\x6c\x65\x20\x50\x6f\x6b\xa6\xc5\x6d\x6f\x6e\x2e\x00",55u)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5af4)!=UINT32_C(0x802ecf68) || rom[0x7c5af8]!=0 || rom[0x7c5af9]!=0 || read32(rom+0x7c5afc)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (memcmp(rom+0x7c64f8,"\x4f\x6e\x65\x20\x53\x69\x67\x6e\x20\x69\x73\x20\x61\x20\x6d\x6f\x75\x6e\x74\x61\x69\x6e\x20\x74\x68\x61\x74\x0a\x6c\x6f\x6f\x6b\x73\x20\x6c\x69\x6b\x65\x20\x44\x55\x47\x54\x52\x49\x4f\x2e\x20\x49\x20\x63\x61\x6d\x65\x0a\x68\x65\x72\x65\x20\x74\x6f\x20\x6c\x6f\x6f\x6b\x20\x66\x6f\x72\x20\x69\x74\x2e\x00",76u)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5b00)!=UINT32_C(0x802ecfb4) || rom[0x7c5b04]!=0 || rom[0x7c5b05]!=1 || read32(rom+0x7c5b08)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (memcmp(rom+0x7c6544,"\x48\x6f\x77\x65\x76\x65\x72\x2c\x20\x49\x20\x63\x61\x6e\x27\x74\x20\x67\x6f\x20\x61\x6e\x79\x0a\x64\x65\x65\x70\x65\x72\x20\x69\x6e\x74\x6f\x20\x74\x68\x65\x20\x69\x73\x6c\x61\x6e\x64\x2c\x0a\x73\x6f\x20\x49\x20\x63\x61\x6e\x27\x74\x20\x66\x69\x6e\x64\x20\x69\x74\x2e\x00",68u)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5b0c)!=UINT32_C(0x802ecff8) || rom[0x7c5b10]!=0 || rom[0x7c5b11]!=0 || read32(rom+0x7c5b14)!=UINT32_C(0x8034e080)) return ISLE_GAME_INVALID_STATE;
    if (memcmp(rom+0x7c6588,"\x48\x61\x76\x65\x20\x79\x6f\x75\x20\x73\x65\x65\x6e\x20\x61\x20\x6d\x6f\x75\x6e\x74\x61\x69\x6e\x20\x74\x68\x61\x74\x0a\x6c\x6f\x6f\x6b\x73\x20\x6c\x69\x6b\x65\x20\x44\x55\x47\x54\x52\x49\x4f\x3f\x0a\x4f\x68\x21\x20\x49\x20\x68\x61\x76\x65\x20\x61\x6e\x20\x69\x64\x65\x61\x21\x00",70u)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5b18)!=UINT32_C(0x802ed040) || rom[0x7c5b1c]!=0 || rom[0x7c5b1d]!=0 || read32(rom+0x7c5b20)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (memcmp(rom+0x7c65d0,"\x49\x66\x20\x79\x6f\x75\x20\x66\x69\x6e\x64\x20\x74\x68\x69\x73\x20\x50\x4b\x4d\x4e\x20\x53\x69\x67\x6e\x2c\x0a\x74\x61\x6b\x65\x20\x61\x20\x70\x69\x63\x74\x75\x72\x65\x20\x6f\x66\x20\x69\x74\x20\x6c\x69\x6b\x65\x20\x79\x6f\x75\x0a\x77\x6f\x75\x6c\x64\x20\x61\x20\x50\x6f\x6b\xa6\xc5\x6d\x6f\x6e\x2e\x00",76u)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5b24)!=UINT32_C(0x802ed08c) || rom[0x7c5b28]!=0 || rom[0x7c5b29]!=0 || read32(rom+0x7c5b2c)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (memcmp(rom+0x7c661c,"\x57\x65\x20\x6d\x69\x67\x68\x74\x20\x64\x69\x73\x63\x6f\x76\x65\x72\x20\x63\x6c\x75\x65\x73\x20\x66\x72\x6f\x6d\x0a\x74\x68\x65\x20\x70\x69\x63\x74\x75\x72\x65\x73\x20\x61\x6e\x64\x20\x74\x68\x65\x20\x70\x6c\x61\x63\x65\x73\x0a\x77\x68\x65\x72\x65\x20\x79\x6f\x75\x20\x74\x6f\x6f\x6b\x20\x74\x68\x65\x6d\x2e\x00",78u)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5b30)!=UINT32_C(0x802ed0dc) || rom[0x7c5b34]!=1 || rom[0x7c5b35]!=0 || read32(rom+0x7c5b38)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (memcmp(rom+0x7c666c,"\x53\x68\x61\x6c\x6c\x20\x49\x20\x72\x65\x70\x65\x61\x74\x20\x74\x68\x69\x73\x20\x69\x6e\x66\x6f\x72\x6d\x61\x74\x69\x6f\x6e\x3f\x0a\x5c\x61\x20\x59\x65\x73\x2c\x20\x70\x6c\x65\x61\x73\x65\x2e\x0a\x5c\x62\x20\x4e\x6f\x2c\x20\x74\x68\x61\x6e\x6b\x20\x79\x6f\x75\x2e\x00",67u)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5b3c)!=UINT32_C(0x802ed120) || rom[0x7c5b40]!=0 || rom[0x7c5b41]!=0 || read32(rom+0x7c5b44)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (memcmp(rom+0x7c66b0,"\x5c\x51\x57\x61\x69\x74\x2e\x2e\x2e\x0a\x49\x20\x77\x69\x6c\x6c\x20\x61\x64\x64\x20\x61\x20\x70\x61\x67\x65\x20\x66\x6f\x72\x20\x50\x4b\x4d\x4e\x20\x53\x69\x67\x6e\x73\x0a\x69\x6e\x20\x74\x68\x65\x20\x50\x4b\x4d\x4e\x20\x52\x65\x70\x6f\x72\x74\x2e\x00",63u)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5b48)!=UINT32_C(0x802ed160) || rom[0x7c5b4c]!=0 || rom[0x7c5b4d]!=0 || read32(rom+0x7c5b50)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (memcmp(rom+0x7c66f0,"\x53\x65\x65\x20\x79\x6f\x75\x20\x6c\x61\x74\x65\x72\x21\x00",15u)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5b54)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5b60)!=UINT32_C(0x802ed170) || rom[0x7c5b64]!=0 || rom[0x7c5b65]!=0 || read32(rom+0x7c5b68)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (memcmp(rom+0x7c6700,"\x54\x68\x65\x72\x65\x27\x73\x20\x6e\x6f\x20\x6f\x6e\x65\x20\x74\x68\x65\x72\x65\x2e\x2e\x2e\x00",24u)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5b6c)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5b80)!=UINT32_C(0x802e8318) || read32(rom+0x7c5b84)!=UINT32_C(0x802e8918) || read32(rom+0x7c5b88)!=UINT32_C(0x802e8f18) || read32(rom+0x7c5b8c)!=UINT32_C(0x802e8918) || read32(rom+0x7c5b90)!=UINT32_C(0x802e8318) || read32(rom+0x7c5b94)) return ISLE_GAME_INVALID_STATE;
    if (read32(rom+0x7c5b98)!=UINT32_C(0x802e7118) || read32(rom+0x7c5b9c)!=UINT32_C(0x802e7718) || read32(rom+0x7c5ba0)!=UINT32_C(0x802e7d18) || read32(rom+0x7c5ba4)!=UINT32_C(0x802e7718) || read32(rom+0x7c5ba8)!=UINT32_C(0x802e7118) || read32(rom+0x7c5bac)) return ISLE_GAME_INVALID_STATE;
    s->D_802D2828_7ABDB8=readfloat(rom+0x7abdb8);
    s->D_802D297C_7ABF0C=(int32_t)read32(rom+0x7abf0c);
    s->D_802D2980_7ABF10=NULL;
    s->D_802D2984_7ABF14=(IsleValleyDefinition){UINT32_C(0x802d2984),UINT32_C(0x802d2984),0,0,read32(rom+0x7abf14),read32(rom+0x7abf14+4),read32(rom+0x7abf14+8),read32(rom+0x7abf14+12)};
    s->D_802D2994_7ABF24=(IsleValleyDefinition){UINT32_C(0x802d2994),UINT32_C(0x802d2994),0,0,read32(rom+0x7abf24),read32(rom+0x7abf24+4),read32(rom+0x7abf24+8),read32(rom+0x7abf24+12)};
    s->D_802D29A4_7ABF34=(IsleValleyDefinition){UINT32_C(0x802d29a4),UINT32_C(0x802d29a4),0,0,read32(rom+0x7abf34),read32(rom+0x7abf34+4),read32(rom+0x7abf34+8),read32(rom+0x7abf34+12)};
    s->D_802D29B4_7ABF44[0]=(IsleValleyCountedOption){10,func_802C5E68_79F3F8};
    s->D_802D29B4_7ABF44[1]=(IsleValleyCountedOption){10,func_802C5F70_79F500};
    s->D_802D29B4_7ABF44[2]=(IsleValleyCountedOption){10,func_802C606C_79F5FC};
    s->D_802D29B4_7ABF44[3]=(IsleValleyCountedOption){70,NULL};
    s->D_802D3B34_7AD0C4=NULL;
    s->D_802D2A80_7AC010=read32(rom+0x7ac010);
    s->D_802D2A84_7AC014=NULL;
    memset(&s->D_8034C8A0_825E30,0,sizeof(s->D_8034C8A0_825E30));
    memset(&s->D_8034C8B8_825E48,0,sizeof(s->D_8034C8B8_825E48));
    memset(&s->D_8034C8B9_825E49,0,sizeof(s->D_8034C8B9_825E49));
    s->D_802D2A90_7AC020=read32(rom+0x7ac020);
    s->D_802D2A94_7AC024=read32(rom+0x7ac024);
    memset(&s->D_8034C8C0_825E30,0,sizeof(s->D_8034C8C0_825E30));
    memset(&s->D_8034C8C8_825E58,0,sizeof(s->D_8034C8C8_825E58));
    s->D_802D2B8C_7AC11C=NULL;
    s->D_802D2B90_7AC120=NULL;
    s->D_802D2BF4_7AC184=readfloat(rom+0x7ac184);
    s->D_802D2BF8_7AC188=readfloat(rom+0x7ac188);
    s->D_802D2BFC_7AC18C=(IsleGameVec3){readfloat(rom+0x7ac18c),readfloat(rom+0x7ac18c+4),readfloat(rom+0x7ac18c+8)};
    s->D_802D2C08_7AC198=(int32_t)read32(rom+0x7ac198);
    s->D_802D2C0C_7AC19C=readfloat(rom+0x7ac19c);
    memset(&s->D_8034C8D0_825E30,0,sizeof(s->D_8034C8D0_825E30));
    memset(&s->D_8034C8D8_825E68,0,sizeof(s->D_8034C8D8_825E68));
    memset(&s->D_8034C8DC_825E6C,0,sizeof(s->D_8034C8DC_825E6C));
    memset(&s->D_8034C8E0_825E70,0,sizeof(s->D_8034C8E0_825E70));
    s->D_802D2D74_7AC304=NULL;
    s->D_802D2D78_7AC308=read16(rom+0x7ac308);
    s->D_802D2FF0_7AC580=NULL;
    s->D_802D2FF4_7AC584=read16(rom+0x7ac584);
    s->D_802D30E0_7AC670=(IsleGameVec3){readfloat(rom+0x7ac670),readfloat(rom+0x7ac670+4),readfloat(rom+0x7ac670+8)};
    s->D_802D3638_7ACBC8=readfloat(rom+0x7acbc8);
    s->D_802D3FF0_7AD580=NULL;
    s->D_802D3844_7ACDD4=readfloat(rom+0x7acdd4);
    s->D_802D3848_7ACDD8=(int32_t)read32(rom+0x7acdd8);
    s->D_802D384C_7ACDDC=readfloat(rom+0x7acddc);
    s->D_802D39CC_7ACF5C=(int32_t)read32(rom+0x7acf5c);
    s->D_802D3FF4_7AD584=NULL;
    s->D_802D3FF8_7AD588=(int32_t)read32(rom+0x7ad588);
    s->D_802D406C_7AD5FC=readfloat(rom+0x7ad5fc);
    s->D_802D41A4_7AD734=NULL;
    s->D_802EC644_7C5BD4=NULL;
    s->D_802D41C8_7AD758=(IsleGameVec3){readfloat(rom+0x7ad758),readfloat(rom+0x7ad758+4),readfloat(rom+0x7ad758+8)};
    s->D_802EC5E8_7C5B78=(int32_t)read32(rom+0x7c5b78);
    s->D_802EC5EC_7C5B7C=(int32_t)read32(rom+0x7c5b7c);
    s->D_802EC638_7C5BC8[0]=readfloat(rom+0x7c5bc8);
    s->D_802EC638_7C5BC8[1]=readfloat(rom+0x7c5bcc);
    s->D_802EC638_7C5BC8[2]=readfloat(rom+0x7c5bd0);
    s->D_802EC648_7C5BD8=NULL;
    s->D_802EC64C_7C5BDC=(int32_t)read32(rom+0x7c5bdc);
    s->D_802EC650_7C5BE0=readfloat(rom+0x7c5be0);
    s->D_802EC654_7C5BE4=readfloat(rom+0x7c5be4);
    memset(&s->D_8034E080_827610,0,sizeof(s->D_8034E080_827610));
    memset(&s->D_8034E0A0_827630,0,sizeof(s->D_8034E0A0_827630));
    memset(&s->D_8034E0A4_827634,0,sizeof(s->D_8034E0A4_827634));
    memset(&s->D_8034E0A8_827638,0,sizeof(s->D_8034E0A8_827638));
    memset(&s->D_8034E0AC_82763C,0,sizeof(s->D_8034E0AC_82763C));
    memset(&s->D_8034E0B0_827640,0,sizeof(s->D_8034E0B0_827640));
    memset(&s->D_8034E0B1_827641,0,sizeof(s->D_8034E0B1_827641));
    memset(&s->D_8034E0B2_827642,0,sizeof(s->D_8034E0B2_827642));
    memset(&s->D_8034E0B4_827644,0,sizeof(s->D_8034E0B4_827644));
    memset(&s->D_8034E0B8_827648,0,sizeof(s->D_8034E0B8_827648));
    s->D_80100730_3F6B70=readfloat(rom+0x3f6b70);
    s->D_802EC710_7C5CA0=(int32_t)read32(rom+0x7c5ca0);
    s->D_802EC714_7C5CA4=(int32_t)read32(rom+0x7c5ca4);
    s->D_802EC738_7C5CC8=(IsleValleyS16Pair){{(int16_t)read16(rom+0x7c5cc8),(int16_t)read16(rom+0x7c5cc8+2)}};
    s->D_802EC868_7C5DF8=read16(rom+0x7c5df8);
    s->D_802EC86C_7C5DFC=read16(rom+0x7c5dfc);
    s->D_802EC890_7C5E20=(int32_t)read32(rom+0x7c5e20);
    s->D_802EC894_7C5E24=readfloat(rom+0x7c5e24);
    s->D_802EC8D8_7C5E68=(IsleGameVec3){readfloat(rom+0x7c5e68),readfloat(rom+0x7c5e68+4),readfloat(rom+0x7c5e68+8)};
    s->D_802EC914_7C5EA4=(IsleGameVec3){readfloat(rom+0x7c5ea4),readfloat(rom+0x7c5ea4+4),readfloat(rom+0x7c5ea4+8)};
    s->D_802ECA10_7C5FA0=(IsleGameVec3){readfloat(rom+0x7c5fa0),readfloat(rom+0x7c5fa0+4),readfloat(rom+0x7c5fa0+8)};
    s->extraStaryuDef=(IsleValleyDefinition){UINT32_C(0x802ecae0),UINT32_C(0x802ecae0),0,0,read32(rom+0x7c6070),read32(rom+0x7c6070+4),read32(rom+0x7c6070+8),read32(rom+0x7c6070+12)};
    s->extraStarmieDef=(IsleValleyDefinition){UINT32_C(0x802ecaf0),UINT32_C(0x802ecaf0),0,0,read32(rom+0x7c6080),read32(rom+0x7c6080+4),read32(rom+0x7c6080+8),read32(rom+0x7c6080+12)};
    s->base.course_id=5; s->base.owner=owner;
    s->base.phase=ISLE_COURSE_ACTOR_PREPARED;
    return ISLE_GAME_OK;
}
static IsleGameResult active_admission(IsleValleyActorSource* s) {
    if (!s || s->base.phase!=ISLE_COURSE_ACTOR_ACTIVE || s->base.course_id!=5 || !s->base.owner || !s->base.nativeGame || !services_complete(s->services)) return ISLE_GAME_INVALID_STATE;
    if (s->base.status!=ISLE_GAME_OK) return s->base.status;
    return isle_valley_actor_native_admission(s,s->base.nativeGame,s->services);
}
IsleGameResult isle_valley_actor_activate(IsleValleyActorSource* s, IsleGame* game, const IsleValleyActorServices* services) {
    IsleGameResult result;
    if (!s || !game || !services_complete(services)) return ISLE_GAME_INVALID_ARGUMENT;
    if (s->base.phase!=ISLE_COURSE_ACTOR_PREPARED || s->base.course_id!=5 || !s->base.owner || s->base.nativeGame || s->services || s->base.nonwaiting_call_frame) return ISLE_GAME_INVALID_STATE;
    if (s->base.status!=ISLE_GAME_OK) return s->base.status;
    result=isle_game_require_idle(game);
    if (result==ISLE_GAME_OK) result=isle_actor_source_execution_status(game);
    if (result==ISLE_GAME_OK) result=isle_valley_actor_native_admission(s,game,services);
    if (result!=ISLE_GAME_OK) { if (s->base.status==ISLE_GAME_OK) s->base.status=result; return result; }
    /* Admission saw PREPARED plus the actual proposed arguments. Never fake
     * ACTIVE, publish provisional pointers, rehydrate or reset Source words. */
    s->base.nativeGame=game; s->services=services;
    s->base.phase=ISLE_COURSE_ACTOR_ACTIVE;
    return ISLE_GAME_OK;
}
IsleGameResult isle_valley_actor_deactivate(IsleValleyActorSource* s) {
    IsleGameResult result;
    if (!s || s->base.phase!=ISLE_COURSE_ACTOR_ACTIVE || !s->base.nativeGame || !s->base.owner || !services_complete(s->services) || s->base.nonwaiting_call_frame) return ISLE_GAME_INVALID_STATE;
    result=isle_actor_source_retirement_status(s->base.nativeGame);
    if(result!=ISLE_GAME_OK && s->base.status==ISLE_GAME_OK) s->base.status=result;
    result=isle_valley_actor_native_retirement(s);
    if (result!=ISLE_GAME_OK) { if(s->base.status==ISLE_GAME_OK) s->base.status=result; return result; }
    /* Native retirement has the real retained queue/process/reference/model,
     * FX/GPU/readback/Photo and audio joins. Counts/idle are not that proof. */
    s->base.nativeGame=NULL; s->services=NULL;
    s->base.phase=ISLE_COURSE_ACTOR_DETACHED;
    return ISLE_GAME_OK;
}
void isle_valley_actor_checked(IsleValleyActorSource* s, IsleGameResult result) { isle_course_actor_checked(s ? &s->base : NULL,result); }
IsleValleyActorSource* isle_valley_actor_current(IsleGameObject* object) {
    IsleValleyActorSource* s=NULL;
    IsleGameResult result=isle_valley_actor_native_context(object ? object->game : NULL,object,&s);
    if (result==ISLE_GAME_OK && (!s || !object || s->base.nativeGame!=object->game)) result=ISLE_GAME_INVALID_STATE;
    if (result==ISLE_GAME_OK) result=active_admission(s);
    if (result!=ISLE_GAME_OK) {
        if (s) isle_valley_actor_checked(s,result);
        isle_native_execution_fault(object ? object->game : NULL,"Valley receiver requires canonical typed native context");
    }
    return s;
}
IsleValleyActorSource* isle_valley_actor_factory_current(const IsleActorFactoryBlock* block, const IsleActorFactoryBlock* reference, const IsleActorFactorySpawn* spawn) {
    IsleValleyActorSource* s=NULL;
    IsleGameResult result=isle_valley_actor_native_factory_context(block,reference,spawn,&s);
    if (result==ISLE_GAME_OK) result=s ? active_admission(s) : ISLE_GAME_INVALID_STATE;
    if (s && result!=ISLE_GAME_OK && s->base.status==ISLE_GAME_OK) s->base.status=result;
    /* Returning native lookup is over before the shared C Player factory fence
     * handles failure. Genuine successful NULL spawn results remain untouched. */
    isle_actor_source_factory_checked(block,reference,spawn,result);
    return s;
}
IsleGameResult isle_valley_actor_invoke(IsleValleyActorSource* s, IsleCourseActorCall callback, void* argument) {
    IsleGameResult result=active_admission(s);
    if (result!=ISLE_GAME_OK) { if(s && s->base.status==ISLE_GAME_OK) s->base.status=result; return result; }
    return isle_course_actor_invoke(&s->base,callback,argument);
}
IsleGameResult isle_valley_actor_validate_animation_node(IsleValleyActorSource* s, const IsleActorNodeRef* node) {
    IsleValleyActorSource* canonical=NULL;
    IsleGameModel* model=NULL;
    const IsleGameModelNode* nodes=NULL;
    size_t count=0;
    IsleGameResult result=active_admission(s);
    if(result==ISLE_GAME_OK && (!node || !node->model.object)) result=ISLE_GAME_INVALID_ARGUMENT;
    if(result==ISLE_GAME_OK) result=isle_valley_actor_native_context(s->base.nativeGame,node->model.object,&canonical);
    if(result==ISLE_GAME_OK && canonical!=s) result=ISLE_GAME_INVALID_STATE;
    if(result==ISLE_GAME_OK) result=isle_game_model_resolve(s->base.nativeGame,&node->model,&model);
    if(result==ISLE_GAME_OK) result=isle_game_model_peek_nodes(s->base.nativeGame,model,&nodes,&count);
    if(result==ISLE_GAME_OK && node->node>=count) result=ISLE_GAME_INVALID_ARGUMENT;
    if(s && result!=ISLE_GAME_OK && s->base.status==ISLE_GAME_OK) s->base.status=result;
    return result;
}
typedef struct ValleyAnimationCall { IsleValleyActorSource* source; IsleValleyAnimationCallback callback; const IsleActorNodeRef* node; int32_t event; float value; } ValleyAnimationCall;
static void animation_call(void* user) { ValleyAnimationCall* p=user; p->callback(p->source,p->node,p->event,p->value); }
IsleGameResult isle_valley_actor_invoke_animation(IsleValleyActorSource* s, IsleValleyAnimationCallback callback, const IsleActorNodeRef* node, int32_t event, float value) {
    IsleGameResult result;
    ValleyAnimationCall call={s,callback,node,event,value};
    if (!s || !node || !node->model.object || !callback || (callback!=func_802D0420_7A99B0 && callback!=func_802D04BC_7A9A4C && callback!=func_802D0CD8_7AA268)) return ISLE_GAME_INVALID_ARGUMENT;
    result=isle_valley_actor_validate_animation_node(s,node);
    if(result!=ISLE_GAME_OK) return result;
    return isle_valley_actor_invoke(s,animation_call,&call);
}
typedef struct ValleyCollisionCall { IsleGameObject* object; IsleActorGround* ground; } ValleyCollisionCall;
static void collision_call(void* user) { ValleyCollisionCall* p=user; func_802C6168_79F6F8(p->object,p->ground); }
IsleGameResult isle_valley_actor_invoke_collision(IsleValleyActorSource* s, IsleGameObject* object, IsleActorGround* ground) {
    IsleValleyActorSource* canonical=NULL; IsleGameResult result;
    ValleyCollisionCall call={object,ground};
    if (!s || !object || !ground) return ISLE_GAME_INVALID_ARGUMENT;
    result=isle_valley_actor_native_context(s->base.nativeGame,object,&canonical);
    if (result!=ISLE_GAME_OK || canonical!=s) return result!=ISLE_GAME_OK ? result : ISLE_GAME_INVALID_STATE;
    return isle_valley_actor_invoke(s,collision_call,&call);
}
void isle_valley_actor_message_event(IsleValleyActorSource* s, uint8_t value) { func_802D0754_7A9CE4(s,(int32_t)value); }
typedef struct ValleyMessageCall { IsleValleyActorSource* source; uint8_t value; } ValleyMessageCall;
static void message_call(void* user) { ValleyMessageCall* p=user; isle_valley_actor_message_event(p->source,p->value); }
IsleGameResult isle_valley_actor_invoke_message(IsleValleyActorSource* s, uint8_t value) { ValleyMessageCall call={s,value}; return isle_valley_actor_invoke(s,message_call,&call); }
IsleGameProcess* isle_valley_actor_process(IsleValleyActorSource* s, IsleGameObject* object, IsleGameSourceCallback callback, int32_t kind, uint32_t priority) {
    IsleGameProcess* output=NULL;
    isle_valley_actor_checked(s,isle_game_add_source_process(s->base.nativeGame,object,callback,(IsleGameProcessKind)kind,priority,&output));
    return output;
}
void isle_valley_actor_visibility(IsleGameObject* object, int hidden) {
    IsleActorSource* actor=isle_actor_source_require(object);
    actor->tangible=!hidden;
    isle_actor_source_object_flags(object,hidden ? 3u : UINT32_MAX,hidden ? 3u : 0u);
}
void isle_valley_actor_spawn_at(IsleValleyActorSource* s, IsleGameObject* object, const IsleGameVec3* position) { IsleGameVec3 value=*position; (void)s; func_8035E174_4FE584(object,&value); }
void isle_valley_actor_trigger_at(IsleValleyActorSource* s, IsleGameObject* object, const IsleGameVec3* position) { IsleGameVec3 value=*position; isle_valley_actor_checked(s,func_8035E264_4FE674(object,&value)); }
void isle_valley_actor_remove_pair(IsleValleyActorSource* s, IsleGameObject* object, const IsleValleyS16Pair* pair) { uint16_t ids[2]={(uint16_t)pair->data[0],(uint16_t)pair->data[1]}; (void)s; isle_actor_remove_pokemons(object,ids); }
IsleValleyVectorRef isle_valley_actor_local_vector(IsleGameVec3* value) { IsleValleyVectorRef ref={0}; ref.local=value; return ref; }
void isle_valley_actor_reference_assign(IsleValleyActorSource* s, IsleGameObject** field, IsleGameObject* value) {
    isle_valley_actor_checked(s, s->services->reference_assign(s, field, value));
}

IsleActorNodeRef isle_valley_actor_model(IsleValleyActorSource* s, IsleGameObject* object) {
    IsleActorNodeRef value = {0};
    isle_valley_actor_checked(s, s->services->model(s, object, &value));
    return value;
}

IsleActorNodeRef isle_valley_actor_player_model(IsleValleyActorSource* s) {
    IsleActorNodeRef value = {0};
    isle_valley_actor_checked(s, s->services->player_model(s, &value));
    return value;
}

IsleActorNodeRef isle_valley_actor_node_path(IsleValleyActorSource* s, IsleActorNodeRef model, const char* steps) {
    IsleActorNodeRef value = {0};
    isle_valley_actor_checked(s, s->services->node_path(s, model, steps, &value));
    return value;
}

IsleValleyVectorRef isle_valley_actor_vector(IsleValleyActorSource* s, IsleActorNodeRef model, int32_t space, int32_t member) {
    IsleValleyVectorRef value = {0};
    isle_valley_actor_checked(s, s->services->vector(s, model, space, member, &value));
    return value;
}

const IsleGameVec3* isle_valley_actor_vector_read(IsleValleyActorSource* s, IsleValleyVectorRef reference) {
    if (reference.local) return reference.local;
    const IsleGameVec3* value = 0;
    isle_valley_actor_checked(s, s->services->vector_read(s, reference, &value));
    return value;
}

void isle_valley_actor_vector_store(IsleValleyActorSource* s, IsleValleyVectorRef reference, uint32_t component, float value) {
    if (reference.local) {
        if (component==0) reference.local->x=value;
        else if(component==1) reference.local->y=value;
        else if(component==2) reference.local->z=value;
        else isle_valley_actor_checked(s,ISLE_GAME_INVALID_ARGUMENT);
        return;
    }
    isle_valley_actor_checked(s, s->services->vector_store(s, reference, component, value));
}

IsleItemSource* isle_valley_actor_item(IsleValleyActorSource* s, IsleGameObject* object) {
    IsleItemSource* value = 0;
    isle_valley_actor_checked(s, s->services->item(s, object, &value));
    return value;
}

IsleGameVec3 isle_valley_actor_item_direction(IsleValleyActorSource* s, IsleGameObject* object) {
    IsleGameVec3 value = {0};
    isle_valley_actor_checked(s, s->services->item_direction(s, object, &value));
    return value;
}

IsleValleyWorldBlock* isle_valley_actor_current_block(IsleValleyActorSource* s) {
    IsleValleyWorldBlock* value = 0;
    isle_valley_actor_checked(s, s->services->current_block(s, &value));
    return value;
}

const IsleActorInteractionGraph* isle_valley_actor_graph(IsleValleyActorSource* s, uint32_t identity) {
    const IsleActorInteractionGraph* value = 0;
    isle_valley_actor_checked(s, s->services->graph(s, identity, &value));
    return value;
}

void isle_valley_actor_graph_copy(IsleValleyActorSource* s, uint32_t identity, IsleActorInteraction* rows, size_t count) {
    isle_valley_actor_checked(s, s->services->graph_copy(s, identity, rows, count));
}

const IsleActorRandomState* isle_valley_actor_weighted(IsleValleyActorSource* s, uint32_t identity) {
    const IsleActorRandomState* value = 0;
    isle_valley_actor_checked(s, s->services->weighted(s, identity, &value));
    return value;
}

void isle_valley_actor_weighted_copy(IsleValleyActorSource* s, uint32_t identity, IsleActorRandomState* rows, size_t count) {
    isle_valley_actor_checked(s, s->services->weighted_copy(s, identity, rows, count));
}

void isle_valley_actor_forbidden_ground(IsleValleyActorSource* s, IsleActorSource* actor, uint32_t identity) {
    isle_valley_actor_checked(s, s->services->forbidden_ground(s, actor, identity));
}

IsleGameObject* isle_valley_actor_add_one(IsleValleyActorSource* s, IsleValleyWorldBlock* a, IsleValleyWorldBlock* b, const IsleValleyAutomaticSpawn* spawn, const IsleValleyDefinition* definition) {
    IsleGameObject* value = 0;
    isle_valley_actor_checked(s, s->services->add_one(s, a, b, spawn, definition, &value));
    return value;
}

IsleGameObject* isle_valley_actor_add_geo(IsleValleyActorSource* s, IsleGameObject* object, uint16_t requested_id, const IsleValleyDefinition* definition) {
    IsleGameObject* value = 0;
    isle_valley_actor_checked(s, s->services->add_geo(s, object, requested_id, definition, &value));
    return value;
}

int32_t isle_valley_actor_ground(IsleValleyActorSource* s, float x, float z, IsleActorGround* ground) {
    int32_t value = 0;
    isle_valley_actor_checked(s, s->services->ground(s, x, z, ground, &value));
    return value;
}

void isle_valley_actor_node_world(IsleValleyActorSource* s, IsleGameVec3* position, IsleGameVec3* velocity, IsleActorNodeRef node) {
    isle_valley_actor_checked(s, s->services->node_world(s, position, velocity, node));
}

void isle_valley_actor_level_progress(IsleValleyActorSource* s, int32_t* block, float* part) {
    isle_valley_actor_checked(s, s->services->level_progress(s, block, part));
}

void isle_valley_actor_path_position(IsleValleyActorSource* s, IsleGameVec3* position, const void* path, float parameter) {
    isle_valley_actor_checked(s, s->services->path_position(s, position, path, parameter));
}

float isle_valley_actor_path_start(IsleValleyActorSource* s, const void* path) {
    float value = 0;
    isle_valley_actor_checked(s, s->services->path_start(s, path, &value));
    return value;
}

void isle_valley_actor_animation_callback(IsleValleyActorSource* s, IsleGameObject* object, IsleValleyAnimationCallback callback) {
    isle_valley_actor_checked(s, s->services->animation_callback(s, object, callback));
}

void isle_valley_actor_model_animation_speed(IsleValleyActorSource* s, IsleGameObject* object, float speed) {
    isle_valley_actor_checked(s, s->services->model_animation_speed(s, object, speed));
}

IsleValleyCamera* isle_valley_actor_camera(IsleValleyActorSource* s) {
    IsleValleyCamera* value = 0;
    isle_valley_actor_checked(s, s->services->camera(s, &value));
    return value;
}

void isle_valley_actor_camera_speed(IsleValleyActorSource* s, IsleValleyCamera* camera, float speed) {
    isle_valley_actor_checked(s, s->services->camera_speed(s, camera, speed));
}

const IsleGameLookAt* isle_valley_actor_camera_read(IsleValleyActorSource* s, IsleValleyCamera* camera) {
    const IsleGameLookAt* value = 0;
    isle_valley_actor_checked(s, s->services->camera_read(s, camera, &value));
    return value;
}

void isle_valley_actor_camera_component(IsleValleyActorSource* s, IsleValleyCamera* camera, int32_t target, uint32_t component, float value) {
    isle_valley_actor_checked(s, s->services->camera_component(s, camera, target, component, value));
}

IsleGameObject* isle_valley_actor_camera_object(IsleValleyActorSource* s, IsleValleyCamera* camera) {
    IsleGameObject* value = 0;
    isle_valley_actor_checked(s, s->services->camera_object(s, camera, &value));
    return value;
}

void isle_valley_actor_camera_animation(IsleValleyActorSource* s, IsleValleyCamera* camera, uint32_t identity, float time) {
    isle_valley_actor_checked(s, s->services->camera_animation(s, camera, identity, time));
}

IsleGameProcess* isle_valley_actor_camera_animation_process(IsleValleyActorSource* s, IsleGameObject* object, int32_t kind, uint32_t priority) {
    IsleGameProcess* value = 0;
    isle_valley_actor_checked(s, s->services->camera_animation_process(s, object, kind, priority, &value));
    return value;
}

void isle_valley_actor_pause_processes(IsleValleyActorSource* s, IsleGameObject* object) {
    isle_valley_actor_checked(s, s->services->pause_processes(s, object));
}

void isle_valley_actor_resume_processes(IsleValleyActorSource* s, IsleGameObject* object) {
    isle_valley_actor_checked(s, s->services->resume_processes(s, object));
}

IsleGameObject* isle_valley_actor_player_model_init(IsleValleyActorSource* s) {
    IsleGameObject* value = 0;
    isle_valley_actor_checked(s, s->services->player_model_init(s, &value));
    return value;
}

void isle_valley_actor_player_model_animation(IsleValleyActorSource* s, uint32_t model, uint32_t material, float time, float speed) {
    isle_valley_actor_checked(s, s->services->player_model_animation(s, model, material, time, speed));
}

void isle_valley_actor_player_model_change(IsleValleyActorSource* s, uint32_t model, uint32_t material, float time) {
    isle_valley_actor_checked(s, s->services->player_model_change(s, model, material, time));
}

void isle_valley_actor_player_model_speed(IsleValleyActorSource* s, float speed) {
    isle_valley_actor_checked(s, s->services->player_model_speed(s, speed));
}

void isle_valley_actor_cutscene_start(IsleValleyActorSource* s, IsleGameObject* object, uint32_t animation, float time) {
    isle_valley_actor_checked(s, s->services->cutscene_start(s, object, animation, time));
}

int32_t isle_valley_actor_sound(IsleValleyActorSource* s, uint32_t sound) {
    int32_t value = 0;
    isle_valley_actor_checked(s, s->services->sound(s, sound, &value));
    return value;
}

int32_t isle_valley_actor_sound_params(IsleValleyActorSource* s, uint32_t sound, int32_t volume, int32_t pan, float pitch, int32_t reverb) {
    int32_t value = 0;
    isle_valley_actor_checked(s, s->services->sound_params(s, sound, volume, pan, pitch, reverb, &value));
    return value;
}

int32_t isle_valley_actor_sound_active(IsleValleyActorSource* s, int32_t handle) {
    int32_t value = 0;
    isle_valley_actor_checked(s, s->services->sound_active(s, handle, &value));
    return value;
}

void isle_valley_actor_sound_stop(IsleValleyActorSource* s, uint32_t handle) {
    isle_valley_actor_checked(s, s->services->sound_stop(s, handle));
}

void isle_valley_actor_sound_stop_all(IsleValleyActorSource* s) {
    isle_valley_actor_checked(s, s->services->sound_stop_all(s));
}

void isle_valley_actor_sound_volume(IsleValleyActorSource* s, int32_t handle, uint32_t volume) {
    isle_valley_actor_checked(s, s->services->sound_volume(s, handle, volume));
}

void isle_valley_actor_sound_global_volume(IsleValleyActorSource* s, uint8_t volume) {
    isle_valley_actor_checked(s, s->services->sound_global_volume(s, volume));
}

int32_t isle_valley_actor_sound_volume_smooth(IsleValleyActorSource* s, int32_t handle, uint32_t from, uint32_t to, int32_t frames) {
    int32_t value = 0;
    isle_valley_actor_checked(s, s->services->sound_volume_smooth(s, handle, from, to, frames, &value));
    return value;
}

uint32_t isle_valley_actor_bgm_play(IsleValleyActorSource* s, int32_t player, uint32_t song) {
    uint32_t value = 0;
    isle_valley_actor_checked(s, s->services->bgm_play(s, player, song, &value));
    return value;
}

void isle_valley_actor_bgm_stop(IsleValleyActorSource* s) {
    isle_valley_actor_checked(s, s->services->bgm_stop(s));
}

void isle_valley_actor_bgm_volume(IsleValleyActorSource* s, int32_t player, uint32_t volume) {
    isle_valley_actor_checked(s, s->services->bgm_volume(s, player, volume));
}

void isle_valley_actor_bgm_volume_smooth(IsleValleyActorSource* s, int32_t player, uint32_t volume, uint32_t frames) {
    isle_valley_actor_checked(s, s->services->bgm_volume_smooth(s, player, volume, frames));
}

void isle_valley_actor_background_music(IsleValleyActorSource* s, int32_t song) {
    isle_valley_actor_checked(s, s->services->background_music(s, song));
}

int32_t isle_valley_actor_check_player_flag(IsleValleyActorSource* s, int32_t flag) {
    int32_t value = 0;
    isle_valley_actor_checked(s, s->services->check_player_flag(s, flag, &value));
    return value;
}

void isle_valley_actor_set_player_flag(IsleValleyActorSource* s, int32_t flag, int32_t value) {
    isle_valley_actor_checked(s, s->services->set_player_flag(s, flag, value));
}

void isle_valley_actor_remove_flying_items(IsleValleyActorSource* s) {
    isle_valley_actor_checked(s, s->services->remove_flying_items(s));
}

void isle_valley_actor_reset_camera(IsleValleyActorSource* s) {
    isle_valley_actor_checked(s, s->services->reset_camera(s));
}

void isle_valley_actor_sky_speed(IsleValleyActorSource* s, float speed) {
    isle_valley_actor_checked(s, s->services->sky_speed(s, speed));
}

void isle_valley_actor_message_reset(IsleValleyActorSource* s) {
    isle_valley_actor_checked(s, s->services->message_reset(s));
}

void isle_valley_actor_message_interactive(IsleValleyActorSource* s) {
    isle_valley_actor_checked(s, s->services->message_interactive(s));
}

void isle_valley_actor_message_noninteractive(IsleValleyActorSource* s) {
    isle_valley_actor_checked(s, s->services->message_noninteractive(s));
}

bool isle_valley_actor_message_printed(IsleValleyActorSource* s) {
    bool value = 0;
    isle_valley_actor_checked(s, s->services->message_printed(s, &value));
    return value;
}

void isle_valley_actor_message_show(IsleValleyActorSource* s, const char* text, int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a, int32_t delay, uint8_t alignment) {
    isle_valley_actor_checked(s, s->services->message_show(s, text, x, y, r, g, b, a, delay, alignment));
}

void isle_valley_actor_message_handler(IsleValleyActorSource* s, IsleValleyMessageCallback callback) {
    isle_valley_actor_checked(s, s->services->message_handler(s, callback));
}

const IsleValleyDialogue* isle_valley_actor_dialogue(IsleValleyActorSource* s, uint32_t identity, size_t index) {
    const IsleValleyDialogue* value = 0;
    isle_valley_actor_checked(s, s->services->dialogue(s, identity, index, &value));
    if(!value || (identity!=UINT32_C(0x802ec51c) && identity!=UINT32_C(0x802ec5d0)) ||
        (identity==UINT32_C(0x802ec51c) && index>14) ||
        (identity==UINT32_C(0x802ec5d0) && index>1))
        isle_valley_actor_checked(s,ISLE_GAME_INVALID_STATE);
    if((identity==UINT32_C(0x802ec51c) && index==14) ||
        (identity==UINT32_C(0x802ec5d0) && index==1)) {
        if(value->unk_00) isle_valley_actor_checked(s,ISLE_GAME_INVALID_STATE);
        return value; /* No read of terminal row's unconsumed fields. */
    }
    if(!value->unk_00) isle_valley_actor_checked(s,ISLE_GAME_INVALID_STATE);
    if(identity==UINT32_C(0x802ec51c) && index==0) {
        if(value->argument_kind!=ISLE_VALLEY_DIALOGUE_TEXT ||
            value->argument.text!=s->D_8034E080_827610)
            isle_valley_actor_checked(s,ISLE_GAME_INVALID_STATE);
    } else if(value->argument_kind!=ISLE_VALLEY_DIALOGUE_NO_ARGUMENT) {
        /* Row8 contains the same raw8034e080 word but has NO conversion: the
         * original extra variadic operand is ignored, not dereferenced. */
        isle_valley_actor_checked(s,ISLE_GAME_INVALID_STATE);
    }
    return value;
}

const IsleSprite* isle_valley_actor_sprite(IsleValleyActorSource* s, uint32_t identity) {
    const IsleSprite* value = 0;
    isle_valley_actor_checked(s, s->services->sprite(s, identity, &value));
    return value;
}

const IsleSprite* const* isle_valley_actor_sprite_list(IsleValleyActorSource* s, uint32_t identity) {
    const IsleSprite* const* value = 0;
    isle_valley_actor_checked(s, s->services->sprite_list(s, identity, &value));
    return value;
}

IsleGameSprite* isle_valley_actor_sprite_add(IsleValleyActorSource* s, IsleGameObject* object, const IsleSprite* sprite) {
    IsleGameSprite* value = 0;
    isle_valley_actor_checked(s, s->services->sprite_add(s, object, sprite, &value));
    return value;
}

void isle_valley_actor_sprite_move(IsleValleyActorSource* s, IsleSprite* sprite, int32_t x, int32_t y) {
    isle_valley_actor_checked(s, s->services->sprite_move(s, sprite, x, y));
}

void isle_valley_actor_sprite_x2_move(IsleValleyActorSource* s, IsleSprite* sprite, int32_t x, int32_t y) {
    isle_valley_actor_checked(s, s->services->sprite_x2_move(s, sprite, x, y));
}

void isle_valley_actor_sprite_scale(IsleValleyActorSource* s, IsleSprite* sprite, float x, float y) {
    isle_valley_actor_checked(s, s->services->sprite_scale(s, sprite, x, y));
}

void isle_valley_actor_sprite_color(IsleValleyActorSource* s, IsleSprite* sprite, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    isle_valley_actor_checked(s, s->services->sprite_color(s, sprite, r, g, b, a));
}

void isle_valley_actor_sprite_link(IsleValleyActorSource* s, IsleGameObject* object, uint8_t link, uint32_t priority, int32_t camera_tag) {
    isle_valley_actor_checked(s, s->services->sprite_link(s, object, link, priority, camera_tag));
}

IsleGameObject* isle_valley_actor_object_add(IsleValleyActorSource* s, uint32_t id, IsleGameSourceCallback update, uint8_t link, uint32_t priority) {
    IsleGameObject* value = 0;
    isle_valley_actor_checked(s, s->services->object_add(s, id, update, link, priority, &value));
    return value;
}

IsleGameObject* isle_valley_actor_object_default_add(IsleValleyActorSource* s, uint32_t id, uint8_t link, uint32_t priority) {
    IsleGameObject* value = 0;
    isle_valley_actor_checked(s, s->services->object_default_add(s, id, link, priority, &value));
    return value;
}

uint32_t isle_valley_actor_input_pressed(IsleValleyActorSource* s) {
    uint32_t value = 0;
    isle_valley_actor_checked(s, s->services->input_pressed(s, &value));
    return value;
}

IsleGameObject* isle_valley_actor_player_object(IsleValleyActorSource* s) {
    IsleGameObject* value = 0;
    isle_valley_actor_checked(s, s->services->player_object(s, &value));
    return value;
}

const char* isle_valley_actor_player_name(IsleValleyActorSource* s) {
    const char* value = 0;
    isle_valley_actor_checked(s, s->services->player_name(s, &value));
    return value;
}

IsleValleyEffect* isle_valley_actor_effect_create(IsleValleyActorSource* s, int32_t bank, int32_t id) {
    IsleValleyEffect* value = 0;
    isle_valley_actor_checked(s, s->services->effect_create(s, bank, id, &value));
    return value;
}

void isle_valley_actor_effect_attach(IsleValleyActorSource* s, IsleValleyEffect* effect, IsleActorNodeRef node) {
    isle_valley_actor_checked(s, s->services->effect_attach(s, effect, node));
}

void isle_valley_actor_application_notice(IsleValleyActorSource* s, int32_t bits) {
    isle_valley_actor_checked(s, s->services->application_notice(s, bits));
}

void isle_valley_actor_transition_begin(IsleValleyActorSource* s, int32_t frames, int32_t mode) {
    isle_valley_actor_checked(s, s->services->transition_begin(s, frames, mode));
}

void isle_valley_actor_transition_color(IsleValleyActorSource* s, float r, float g, float b, float a) {
    isle_valley_actor_checked(s, s->services->transition_color(s, r, g, b, a));
}

int32_t isle_valley_actor_transition_busy(IsleValleyActorSource* s) {
    int32_t value = 0;
    isle_valley_actor_checked(s, s->services->transition_busy(s, &value));
    return value;
}

void isle_valley_actor_audio_cutscene_reset(IsleValleyActorSource* s) {
    isle_valley_actor_checked(s, s->services->audio_cutscene_reset(s));
}

void isle_valley_actor_environment_sound(IsleValleyActorSource* s, IsleGameObject* object, uint8_t category, int32_t sound) {
    isle_valley_actor_checked(s, s->services->environment_sound(s, object, category, sound));
}

void isle_valley_actor_kick_item(IsleValleyActorSource* s, IsleGameObject* object, float speed) {
    isle_valley_actor_checked(s, s->services->kick_item(s, object, speed));
}

void isle_valley_actor_jump_bounce(IsleValleyActorSource* s, IsleGameObject* object, float speed, float backwards, float parameter) {
    isle_valley_actor_checked(s, s->services->jump_bounce(s, object, speed, backwards, parameter));
    /* The native preflight returned; only this C continuation encloses ohWait. */
    Pokemon_JumpAndBounceFromGround(object, speed, backwards, parameter);
}

IsleGameObject* isle_valley_actor_common_child1(IsleValleyActorSource* s, IsleGameObject* object) {
    IsleGameObject* value = 0;
    isle_valley_actor_checked(s, s->services->common_child1(s, object, &value));
    return value;
}

IsleGameObject* isle_valley_actor_common_child2(IsleValleyActorSource* s, IsleGameObject* object) {
    IsleGameObject* value = 0;
    isle_valley_actor_checked(s, s->services->common_child2(s, object, &value));
    return value;
}

void isle_valley_actor_block_change(IsleValleyActorSource* s, IsleGameObject* object, float x, float y, float z, float rx, float ry, float rz) {
    isle_valley_actor_checked(s, s->services->block_change(s, object, x, y, z, rx, ry, rz));
}

void isle_valley_actor_block_change_ground(IsleValleyActorSource* s, IsleGameObject* object, float x, float y, float z, float rx, float ry, float rz) {
    isle_valley_actor_checked(s, s->services->block_change_ground(s, object, x, y, z, rx, ry, rz));
}

void isle_valley_actor_common_remove(IsleValleyActorSource* s, IsleGameObject* object) {
    isle_valley_actor_checked(s, s->services->common_remove(s, object));
}

IsleGameObject* isle_valley_actor_misc_object_get(IsleValleyActorSource* s, IsleGameObject* object, uint32_t index) {
    IsleGameObject* value = 0;
    isle_valley_actor_checked(s, s->services->misc_object_get(s, object, index, &value));
    return value;
}

void isle_valley_actor_misc_object_set(IsleValleyActorSource* s, IsleGameObject* object, uint32_t index, IsleGameObject* target) {
    isle_valley_actor_checked(s, s->services->misc_object_set(s, object, index, target));
}

void isle_valley_actor_model_time_left(IsleValleyActorSource* s, IsleActorNodeRef node, float value) {
    isle_valley_actor_checked(s, s->services->model_time_left(s, node, value));
}

int32_t isle_valley_actor_rng_range(IsleValleyActorSource* s, int32_t range) {
    int32_t value = 0;
    isle_valley_actor_checked(s, s->services->rng_range(s, range, &value));
    return value;
}

float isle_valley_actor_rng_float(IsleValleyActorSource* s) {
    float value = 0;
    isle_valley_actor_checked(s, s->services->rng_float(s, &value));
    return value;
}

void isle_valley_actor_player_cutscene_process(IsleValleyActorSource* s, IsleGameObject* object) {
    isle_valley_actor_checked(s, s->services->player_cutscene_process(s, object));
}

int32_t isle_valley_actor_flute(IsleValleyActorSource* s) {
    int32_t value = 0;
    isle_valley_actor_checked(s, s->services->flute(s, &value));
    return value;
}

void isle_valley_actor_command(IsleValleyActorSource* s, IsleGameObject* destination, int32_t command, IsleGameObject* sender) {
    isle_valley_actor_checked(s, s->services->command(s, destination, command, sender));
}

void isle_valley_actor_command_link(IsleValleyActorSource* s, uint8_t link, int32_t command, IsleGameObject* sender) {
    isle_valley_actor_checked(s, s->services->command_link(s, link, command, sender));
}

float isle_valley_actor_vector_distance(IsleValleyActorSource* s, const IsleGameVec3* a, const IsleGameVec3* b) {
    float value = 0;
    isle_valley_actor_checked(s, s->services->vector_distance(s, a, b, &value));
    return value;
}

int32_t isle_valley_actor_dialogue_format(IsleValleyActorSource* s, char* destination, size_t capacity, const IsleValleyDialogue* row) {
    int32_t value = 0;
    isle_valley_actor_checked(s, s->services->dialogue_format(s, destination, capacity, row, &value));
    return value;
}


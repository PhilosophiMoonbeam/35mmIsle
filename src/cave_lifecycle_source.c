#include "cave_lifecycle_source.h"
#include "engine_internal.h"
#include "player_source.h"
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
static uint16_t be16(const uint8_t* p) { return (uint16_t)((uint16_t)p[0]<<8 | p[1]); }
static uint32_t be32(const uint8_t* p) { return (uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | p[3]; }
static int32_t signed32(uint32_t v) { return v<=INT32_MAX ? (int32_t)v : -1-(int32_t)(UINT32_MAX-v); }
static int services_complete(const IsleCaveLifecycleServices* p) { return p && p->scene_joined &&
    p->reference &&
    p->block_position &&
    p->sound_params &&
    p->sound_active &&
    p->sound_stop &&
    p->sound_stop_all &&
    p->bgm_stop_all &&
    p->camera_reset &&
    p->camera_speed &&
    p->camera_animation &&
    p->player_animation &&
    p->animation_callback &&
    p->main_camera &&
    p->player_model_init &&
    p->camera_process &&
    p->effect_create &&
    p->effect_model &&
    p->camera_read &&
    p->camera_store &&
    p->input_pressed &&
    p->camera_object &&
    p->height_map &&
    p->ceiling_map &&
    p->world_create &&
    p->sky_follow &&
    p->source_80363928 &&
    p->source_800A19D8 &&
    p->detector_disable &&
    p->environment_cleanup &&
    p->world_destroy &&
    p->source_80356FBC &&
    p->camera_remove &&
    p->source_800067DC &&
    p->items_displace &&
    p->fx_bank &&
    p->fx_dither &&
    p->screen_apply &&
    p->fx_reset_bank &&
    p->source_8009CE00 &&
    p->pokemon_initialize &&
    p->background_color &&
    p->camera_create &&
    p->camera_flags &&
    p->ui_initialize &&
    p->end_callback &&
    p->pause_callback &&
    p->environment_initialize &&
    p->detector_create &&
    p->detector_enable &&
    p->pitch_limits &&
    p->intervals &&
    p->near_clipping &&
    p->source_800AAED0 &&
    p->source_803570B0 &&
    p->source_80357120 &&
    p->source_80357170 &&
    p->source_803571C4 &&
    p->source_803572B0 &&
    p->block_next &&
    p->asset_load &&
    p->fx_pool &&
    p->fx_manager &&
    p->allocate &&
    p->add_helper &&
    p->source_8009BC68 &&
    p->scene_request &&
    p->pokemon_add &&
    p->pokemon_change &&
    p->pokemon_remove &&
    p->fx_manager_assign &&
    p->fx_pool_assign; }
IsleGameResult isle_cave_lifecycle_prepare(IsleCaveLifecycleSource* s,void* owner,IsleContinuationDomain* domain,IsleCaveActorSource* actors,const IsleCaveLifecycleServices* services,const uint8_t* rom,size_t bytes,int32_t scene) {
    size_t i; const uint8_t* setup; const uint8_t* screen;
    if (!s || !owner || !domain || !actors || !services_complete(services) || !rom || bytes!=0x1000000u) return ISLE_GAME_INVALID_ARGUMENT;
    if (s->owner || s->domain || s->actors || s->services || s->running || s->completed || s->scene_requested || s->scene_completion_ready || s->entry_executing || s->nonwaiting_call_frame || s->status!=ISLE_GAME_OK || s->scene_completion_status!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE;
    if (actors->base.phase!=ISLE_COURSE_ACTOR_PREPARED || actors->base.owner!=owner || actors->base.nativeGame || actors->services || actors->base.nonwaiting_call_frame || actors->base.status!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE;
    if (isle_continuation_domain_check_access(domain)!=ISLE_CONT_OK) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648600)!=UINT32_C(0x91)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648602]!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648603]!=UINT32_C(0x16)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648604)!=UINT32_C(0xd5)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648606]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648607]!=UINT32_C(0x14)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648608)!=UINT32_C(0xd6)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64860a]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64860b]!=UINT32_C(0x14)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x64860c)!=UINT32_C(0x98)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64860e]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64860f]!=UINT32_C(0x1e)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648610)!=UINT32_C(0xd8)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648612]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648613]!=UINT32_C(0x1c)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648614)!=UINT32_C(0xd9)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648616]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648617]!=UINT32_C(0x1c)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648618)!=UINT32_C(0x14c)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64861a]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64861b]!=UINT32_C(0xe)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x64861c)!=UINT32_C(0x14d)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64861e]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64861f]!=UINT32_C(0xe)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648620)!=UINT32_C(0x14e)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648622]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648623]!=UINT32_C(0xe)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648624)!=UINT32_C(0x95)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648626]!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648627]!=UINT32_C(0xe)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648628)!=UINT32_C(0x96)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64862a]!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64862b]!=UINT32_C(0xe)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x64862c)!=UINT32_C(0x67)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64862e]!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64862f]!=UINT32_C(0xe)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648630)!=UINT32_C(0xd2)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648632]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648633]!=UINT32_C(0x8)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648634)!=UINT32_C(0xd3)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648636]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648637]!=UINT32_C(0x8)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648638)!=UINT32_C(0x178)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64863a]!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64863b]!=UINT32_C(0x8)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x64863c)!=UINT32_C(0x72)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64863e]!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64863f]!=UINT32_C(0x8)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648640)!=UINT32_C(0xc4)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648642]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648643]!=UINT32_C(0xc)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648644)!=UINT32_C(0xc6)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648646]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648647]!=UINT32_C(0xc)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648648)!=UINT32_C(0xc5)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64864a]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64864b]!=UINT32_C(0xc)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x64864c)!=UINT32_C(0x84)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64864e]!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64864f]!=UINT32_C(0xf)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648650)!=UINT32_C(0x16e)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648652]!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648653]!=UINT32_C(0x8)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648654)!=UINT32_C(0x86)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648656]!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648657]!=UINT32_C(0x12)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648658)!=UINT32_C(0x8b)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64865a]!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64865b]!=UINT32_C(0x12)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x64865c)!=UINT32_C(0x77)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64865e]!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64865f]!=UINT32_C(0xf)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648660)!=UINT32_C(0x132)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648662]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648663]!=UINT32_C(0xc)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648664)!=UINT32_C(0x133)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648666]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648667]!=UINT32_C(0xc)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648668)!=UINT32_C(0x134)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64866a]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64866b]!=UINT32_C(0xc)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x64866c)!=UINT32_C(0x174)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64866e]!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64866f]!=UINT32_C(0x8)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648670)!=UINT32_C(0x107)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648672]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648673]!=UINT32_C(0x12)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648674)!=UINT32_C(0x162)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648676]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648677]!=UINT32_C(0xf)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648678)!=UINT32_C(0x108)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64867a]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64867b]!=UINT32_C(0xc)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x64867c)!=UINT32_C(0x109)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64867e]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64867f]!=UINT32_C(0xa)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648680)!=UINT32_C(0x10a)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648682]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648683]!=UINT32_C(0xa)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648684)!=UINT32_C(0x7f)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648686]!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648687]!=UINT32_C(0xc)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648688)!=UINT32_C(0x168)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64868a]!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64868b]!=UINT32_C(0xc)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x64868c)!=UINT32_C(0x173)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64868e]!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64868f]!=UINT32_C(0xa)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648690)!=UINT32_C(0x14f)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648692]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648693]!=UINT32_C(0xc)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648694)!=UINT32_C(0x150)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648696]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x648697]!=UINT32_C(0xc)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648698)!=UINT32_C(0x151)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64869a]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64869b]!=UINT32_C(0xc)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x64869c)!=UINT32_C(0x152)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64869e]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x64869f]!=UINT32_C(0xc)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x6486a0)!=UINT32_C(0x153)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486a2]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486a3]!=UINT32_C(0xc)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x6486a4)!=UINT32_C(0x154)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486a6]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486a7]!=UINT32_C(0xc)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x6486a8)!=UINT32_C(0x155)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486aa]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486ab]!=UINT32_C(0xc)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x6486ac)!=UINT32_C(0x141)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486ae]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486af]!=UINT32_C(0xc)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x6486b0)!=UINT32_C(0x142)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486b2]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486b3]!=UINT32_C(0xc)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x6486b4)!=UINT32_C(0x143)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486b6]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486b7]!=UINT32_C(0xc)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x6486b8)!=UINT32_C(0x88)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486ba]!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486bb]!=UINT32_C(0xc)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x6486bc)!=UINT32_C(0xbc)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486be]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486bf]!=UINT32_C(0x1a)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x6486c0)!=UINT32_C(0x6a)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486c2]!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486c3]!=UINT32_C(0x1a)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x6486c4)!=UINT32_C(0x6b)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486c6]!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486c7]!=UINT32_C(0x1a)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x6486c8)!=UINT32_C(0x99)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486ca]!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486cb]!=UINT32_C(0x1a)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x6486cc)!=UINT32_C(0x148)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486ce]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486cf]!=UINT32_C(0x12)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x6486d0)!=UINT32_C(0x149)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486d2]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486d3]!=UINT32_C(0x16)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x6486d4)!=UINT32_C(0x14a)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486d6]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486d7]!=UINT32_C(0x16)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x6486d8)!=UINT32_C(0x14b)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486da]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486db]!=UINT32_C(0x16)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x6486dc)!=UINT32_C(0x135)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486de]!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486df]!=UINT32_C(0x8)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x6486e0)!=UINT32_C(0x85)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486e2]!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (rom[0x6486e3]!=UINT32_C(0x8)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x64882c)!=UINT32_C(0x803b5000)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x648830)!=UINT32_C(0x803da800)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x648834)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x648838)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x64883c)!=UINT32_C(0x140)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x648840)!=UINT32_C(0xf0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x648844)!=UINT32_C(0xa99)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648848)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x64884c)!=UINT32_C(0x8000affc)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x648850)!=UINT32_C(0x8000ada0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x648854)!=UINT32_C(0x801aedf0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x648858)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x64885c)!=UINT32_C(0x1)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x648860)!=UINT32_C(0x2)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x648864)!=UINT32_C(0x5000)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x648868)!=UINT32_C(0x2000)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x64886c)!=UINT32_C(0x400)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x648870)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x648874)!=UINT32_C(0xc800)) return ISLE_GAME_INVALID_STATE;
    if (be16(rom+0x648878)!=UINT32_C(0x2)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x64887c)!=UINT32_C(0x4000)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x648880)!=UINT32_C(0x800a1a50)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x648884)!=UINT32_C(0x80004860)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x648888)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x64888c)!=UINT32_C(0x440)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x648890)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x648894)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x648898)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x64889c)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x6488a0)!=UINT32_C(0x5c)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x6488a4)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x6488a8)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x6488ac)!=UINT32_C(0x802be3a8)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x6488b0)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x6488b4)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x6488b8)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x6488bc)!=UINT32_C(0x88)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x6488c0)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x6488c4)!=UINT32_C(0x58)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x6488c8)!=UINT32_C(0x0)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x6488cc)!=UINT32_C(0x90)) return ISLE_GAME_INVALID_STATE;
    if (be32(rom+0x6488d0)!=UINT32_C(0x802be22c)) return ISLE_GAME_INVALID_STATE;
    setup=rom+0x648848; screen=rom+0x64882c;
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
    for (i=0;i<57;i++) {
        const uint8_t* row=rom+0x648600+i*4;
        s->environment[i].sound=be16(row); s->environment[i].pitch_modifier=row[2]; s->environment[i].hearing_range=row[3];
    }
    for (i=0;i<18;i++) {
        const uint8_t* row=rom+0x6486e4+i*16;
        s->definitions[i].id=be32(row); s->definitions[i].initialize_identity=be32(row+4);
        s->definitions[i].update_identity=be32(row+8); s->definitions[i].remove_identity=be32(row+12);
    }
    s->owner=owner; s->domain=domain; s->actors=actors; s->services=services; s->input_scene=scene;
    return ISLE_GAME_OK;
}
static IsleGameResult lifecycle_return(IsleCaveLifecycleSource* s,IsleGameResult result) {
    if (s && s->status==ISLE_GAME_OK && result!=ISLE_GAME_OK) s->status=result;
    return s && s->status!=ISLE_GAME_OK ? s->status : result;
}

typedef struct CaveLifecycleFrame { jmp_buf failure; void* previous; void* previous_player_frame; } CaveLifecycleFrame;
typedef struct CaveLifecycleInvocation { IsleCaveLifecycleSource* source; IsleCaveLifecycleCall callback; void* argument; } CaveLifecycleInvocation;
static void lifecycle_player_call(IslePlayerSource* player,void* payload) { CaveLifecycleInvocation* c=payload; (void)player; c->callback(c->source,c->argument); }
void isle_cave_lifecycle_checked(IsleCaveLifecycleSource* s,IsleGameResult r) {
    if (r==ISLE_GAME_OK && (!s || s->status==ISLE_GAME_OK)) return;
    if (!s) abort();
    if (s->status==ISLE_GAME_OK) s->status=r;
    {
        CaveLifecycleFrame* frame=s->nonwaiting_call_frame;
        IslePlayerSource* player=NULL;
        if (!s->entry_executing && s->actors && s->actors->base.phase==ISLE_COURSE_ACTOR_ACTIVE && s->actors->base.nativeGame) player=s->actors->base.nativeGame->playerSource;
        if (player && player->nonwaiting_call_frame && (!frame || player->nonwaiting_call_frame!=frame->previous_player_frame)) isle_player_source_checked(player,s->status);
        if (frame) longjmp(frame->failure,1);
    }
    if (s->entry_executing) {
        s->entry_executing=0;
        (void)isle_continuation_fail(s->domain,"Cave independent entry failed");
        abort();
    }
    /* Only ACTIVE may inspect the actual Game. Detached failure and original
     * scene return belong solely to the independent continuation/owner. */
    if (s->actors && s->actors->base.phase==ISLE_COURSE_ACTOR_ACTIVE && s->actors->base.nativeGame)
        isle_native_execution_fault(s->actors->base.nativeGame,"Cave lifecycle native leaf failed");
    if (s->running) (void)isle_continuation_fail(s->domain,"Cave lifecycle native leaf failed");
    abort();
}
IsleCaveLifecycleSource* isle_cave_lifecycle_require(IsleGame* game,IsleGameObject* receiver) {
    IsleCaveLifecycleSource* s=NULL; IsleGameResult r=isle_cave_lifecycle_native_context(game,receiver,&s);
    if (r!=ISLE_GAME_OK || !s) { isle_native_execution_fault(game,"Cave lifecycle receiver context failed"); abort(); }
    if (!s->actors || s->actors->base.phase!=ISLE_COURSE_ACTOR_ACTIVE || s->actors->base.nativeGame!=game || s->actors->base.owner!=s->owner || !services_complete(s->services))
        isle_cave_lifecycle_checked(s,ISLE_GAME_INVALID_STATE);
    isle_cave_lifecycle_checked(s,s->actors->base.status);
    isle_cave_lifecycle_checked(s,isle_cave_actor_native_admission(s->actors,game,s->actors->services));
    return s;
}
IsleGameResult isle_cave_lifecycle_invoke(IsleCaveLifecycleSource* s,IsleCaveLifecycleCall callback,void* argument) {
    CaveLifecycleFrame frame;
    CaveLifecycleInvocation invocation={s,callback,argument};
    IslePlayerSource* player=NULL; IsleGameResult result;
    if (!s || !callback || !s->owner || !s->domain || !services_complete(s->services)) return ISLE_GAME_INVALID_ARGUMENT;
    if (s->status!=ISLE_GAME_OK) return s->status;
    if (isle_continuation_domain_check_access(s->domain)!=ISLE_CONT_OK) return ISLE_GAME_INVALID_STATE;
    if (s->actors && s->actors->base.phase==ISLE_COURSE_ACTOR_ACTIVE) {
        result=isle_player_source_context(s->actors->base.nativeGame,&player);
        if (result!=ISLE_GAME_OK) return lifecycle_return(s,result);
        if (!player) return ISLE_GAME_INVALID_STATE;
    }
    frame.previous=s->nonwaiting_call_frame;
    frame.previous_player_frame=player ? player->nonwaiting_call_frame : NULL;
    s->nonwaiting_call_frame=&frame;
    if (setjmp(frame.failure)==0) {
        if (player) {
            result=isle_player_source_call(player,lifecycle_player_call,&invocation);
            if (s->status==ISLE_GAME_OK && result!=ISLE_GAME_OK) s->status=result;
        } else callback(s,argument);
    }
    s->nonwaiting_call_frame=frame.previous;
    return s->status;
}
void isle_cave_lifecycle_setup_scene(IsleCaveLifecycleSource* s,const IsleCaveLifecycleSceneSetup* setup) {
    if (!s || !s->running || s->scene_requested || !s->actors || s->actors->base.phase!=ISLE_COURSE_ACTOR_PREPARED || s->actors->base.owner!=s->owner)
        isle_cave_lifecycle_checked(s,ISLE_GAME_INVALID_STATE);
    isle_cave_lifecycle_checked(s,s->actors->base.status);
    isle_cave_lifecycle_scene_request(s,setup);
    s->scene_requested=1;
    while (!s->scene_completion_ready) {
        IsleContinuationResult waited;
        isle_cave_lifecycle_checked(s,s->status);
        s->entry_executing=0;
        waited=isle_continuation_wait(s->domain,1);
        s->entry_executing=1;
        if (waited!=ISLE_CONT_OK) isle_cave_lifecycle_checked(s,ISLE_GAME_STOPPED);
    }
    isle_cave_lifecycle_checked(s,s->status);
    isle_cave_lifecycle_checked(s,s->scene_completion_status);
    isle_cave_lifecycle_checked(s,s->actors->base.status);
}
IsleGameResult isle_cave_lifecycle_complete_scene(IsleCaveLifecycleSource* s,IsleGameResult status) {
    IsleGameResult r;
    if (!s || !s->running || !s->scene_requested || s->scene_completion_ready || s->entry_executing || s->nonwaiting_call_frame || !s->actors || s->actors->base.phase!=ISLE_COURSE_ACTOR_DETACHED || s->actors->base.owner!=s->owner || s->actors->base.nativeGame || s->actors->services || s->actors->base.nonwaiting_call_frame)
        return ISLE_GAME_INVALID_STATE;
    if (isle_continuation_domain_check_access(s->domain)!=ISLE_CONT_OK) return ISLE_GAME_INVALID_STATE;
    r=s->services->scene_joined(s);
    if (r!=ISLE_GAME_OK) { if (s->status==ISLE_GAME_OK) s->status=r; return r; }
    if (s->status==ISLE_GAME_OK && s->actors->base.status!=ISLE_GAME_OK) s->status=s->actors->base.status;
    if (s->status==ISLE_GAME_OK && status!=ISLE_GAME_OK) s->status=status;
    s->screen.depth_buffer=NULL;
    s->scene_completion_status=s->status;
    s->scene_completion_ready=1;
    return s->status;
}
void isle_cave_lifecycle_run(void* context) {
    IsleCaveLifecycleSource* s=context; int32_t output;
    if (!s || !s->owner || !s->domain || s->running || s->completed || s->scene_requested || s->status!=ISLE_GAME_OK) {
        if (s) isle_cave_lifecycle_checked(s,ISLE_GAME_INVALID_STATE);
        abort();
    }
    s->running=1; s->entry_executing=1;
    output=func_802BE3B0_640860(s,s->input_scene);
    isle_cave_lifecycle_checked(s,s->status);
    if (!s->scene_completion_ready || !s->actors || s->actors->base.phase!=ISLE_COURSE_ACTOR_DETACHED)
        isle_cave_lifecycle_checked(s,ISLE_GAME_INVALID_STATE);
    s->output_scene=output; s->completed=1; s->running=0; s->entry_executing=0;
}
static void init_call(IsleCaveLifecycleSource* s,void* payload) { (void)payload; func_802BE22C_6406DC(s); }
IsleGameResult isle_cave_lifecycle_invoke_init(IsleCaveLifecycleSource* s) {
    IsleGameResult r;
    if (!s || !s->scene_requested || !s->actors || s->actors->base.phase!=ISLE_COURSE_ACTOR_ACTIVE) return ISLE_GAME_INVALID_STATE;
    r=isle_cave_actor_native_admission(s->actors,s->actors->base.nativeGame,s->actors->services);
    return lifecycle_return(s,r==ISLE_GAME_OK ? isle_cave_lifecycle_invoke(s,init_call,NULL) : r);
}
static void end_call(IsleCaveLifecycleSource* s,void* payload) { const int32_t* reason=payload; func_802BE038_6404E8(s,*reason); }
IsleGameResult isle_cave_lifecycle_invoke_end(IsleCaveLifecycleSource* s,int32_t reason) {
    if (!s || !s->actors || s->actors->base.phase!=ISLE_COURSE_ACTOR_ACTIVE) return ISLE_GAME_INVALID_STATE;
    return isle_cave_lifecycle_invoke(s,end_call,&reason);
}
typedef struct CaveLifecycleAnimationCall { const IsleActorNodeRef* model; int32_t event; float value; } CaveLifecycleAnimationCall;
static void animation_call(IsleCaveLifecycleSource* s,void* payload) { CaveLifecycleAnimationCall* c=payload; func_802C5BB0_648060(s,c->model,c->event,c->value); }
IsleGameResult isle_cave_lifecycle_invoke_animation(IsleCaveLifecycleSource* s,const IsleActorNodeRef* model,int32_t event,float value) {
    IsleGameModel* resolved=NULL;
    IsleCaveLifecycleSource* canonical=NULL; IsleGameResult r; CaveLifecycleAnimationCall call={model,event,value};
    if (!s || !model || !model->model.object || !s->actors || s->actors->base.phase!=ISLE_COURSE_ACTOR_ACTIVE) return ISLE_GAME_INVALID_STATE;
    r=isle_game_model_resolve(s->actors->base.nativeGame,&model->model,&resolved);
    if (r!=ISLE_GAME_OK) return lifecycle_return(s,r);
    r=isle_cave_lifecycle_native_context(s->actors->base.nativeGame,model->model.object,&canonical);
    if (r!=ISLE_GAME_OK) return lifecycle_return(s,r);
    if (canonical!=s) return lifecycle_return(s,ISLE_GAME_INVALID_STATE);
    r=isle_cave_lifecycle_native_animation_node(s,model);
    if (r!=ISLE_GAME_OK) return lifecycle_return(s,r);
    return isle_cave_lifecycle_invoke(s,animation_call,&call);
}

typedef struct CaveWorldPairCall { IsleCaveWorldPairCallback callback; IsleCaveWorldBlock* a; IsleCaveWorldBlock* b; } CaveWorldPairCall;
static void world_pair_call(IsleCaveLifecycleSource* s,void* payload) { CaveWorldPairCall* c=payload; c->callback(s,c->a,c->b); }
IsleGameResult isle_cave_lifecycle_invoke_world_pair(IsleCaveLifecycleSource* s,IsleCaveWorldPairCallback callback,IsleCaveWorldBlock* a,IsleCaveWorldBlock* b) {
    CaveWorldPairCall call={callback,a,b};
    if (!callback || !s || !s->actors || s->actors->base.phase!=ISLE_COURSE_ACTOR_ACTIVE) return ISLE_GAME_INVALID_STATE;
    return isle_cave_lifecycle_invoke(s,world_pair_call,&call);
}
typedef struct CaveWorldSingleCall { IsleCaveWorldSingleCallback callback; IsleCaveWorldBlock* block; } CaveWorldSingleCall;
static void world_single_call(IsleCaveLifecycleSource* s,void* payload) { CaveWorldSingleCall* c=payload; c->callback(s,c->block); }
IsleGameResult isle_cave_lifecycle_invoke_world_single(IsleCaveLifecycleSource* s,IsleCaveWorldSingleCallback callback,IsleCaveWorldBlock* block) {
    CaveWorldSingleCall call={callback,block};
    if (!callback || !s || !s->actors || s->actors->base.phase!=ISLE_COURSE_ACTOR_ACTIVE) return ISLE_GAME_INVALID_STATE;
    return isle_cave_lifecycle_invoke(s,world_single_call,&call);
}
typedef struct CaveEndCall { IsleCaveEndCallback callback; int32_t reason; } CaveEndCall;
static void end_callback_call(IsleCaveLifecycleSource* s,void* payload) { CaveEndCall* c=payload; c->callback(s,c->reason); }
IsleGameResult isle_cave_lifecycle_invoke_end_callback(IsleCaveLifecycleSource* s,IsleCaveEndCallback callback,int32_t reason) {
    CaveEndCall call={callback,reason};
    if (!callback || !s || !s->actors || s->actors->base.phase!=ISLE_COURSE_ACTOR_ACTIVE) return ISLE_GAME_INVALID_STATE;
    return isle_cave_lifecycle_invoke(s,end_callback_call,&call);
}

IsleGameResult isle_cave_lifecycle_report_failure(IsleCaveLifecycleSource* s,IsleGameResult result) {
    if (!s || !s->owner || !s->domain || result==ISLE_GAME_OK) return ISLE_GAME_INVALID_ARGUMENT;
    if (s->entry_executing || s->nonwaiting_call_frame || s->completed) return ISLE_GAME_INVALID_STATE;
    if (isle_continuation_domain_check_access(s->domain)!=ISLE_CONT_OK) return ISLE_GAME_INVALID_STATE;
    /* Owner-only, outside Source/native traversals. No Game/Player access,
     * no invented join/completion receipt, no transfer across a C++ frame. */
    if (s->status==ISLE_GAME_OK) s->status=result;
    return s->status;
}
void isle_cave_lifecycle_reference(IsleCaveLifecycleSource* s, IsleGameObject** field, IsleGameObject* object) {
    isle_cave_lifecycle_checked(s, s->services->reference(s, field, object));
}
IsleGameVec3 isle_cave_lifecycle_block_position(IsleCaveLifecycleSource* s, IsleCaveWorldBlock* block) {
    IsleGameVec3 output = {0};
    isle_cave_lifecycle_checked(s, s->services->block_position(s, block, &output));
    return output;
}
int32_t isle_cave_lifecycle_sound_params(IsleCaveLifecycleSource* s, int32_t id, int32_t volume, int32_t pan, float pitch, int32_t reverb) {
    int32_t output = {0};
    isle_cave_lifecycle_checked(s, s->services->sound_params(s, id, volume, pan, pitch, reverb, &output));
    return output;
}
int32_t isle_cave_lifecycle_sound_active(IsleCaveLifecycleSource* s, int32_t handle) {
    int32_t output = {0};
    isle_cave_lifecycle_checked(s, s->services->sound_active(s, handle, &output));
    return output;
}
void isle_cave_lifecycle_sound_stop(IsleCaveLifecycleSource* s, int32_t handle) {
    isle_cave_lifecycle_checked(s, s->services->sound_stop(s, handle));
}
void isle_cave_lifecycle_sound_stop_all(IsleCaveLifecycleSource* s) {
    isle_cave_lifecycle_checked(s, s->services->sound_stop_all(s));
}
void isle_cave_lifecycle_bgm_stop_all(IsleCaveLifecycleSource* s) {
    isle_cave_lifecycle_checked(s, s->services->bgm_stop_all(s));
}
void isle_cave_lifecycle_camera_reset(IsleCaveLifecycleSource* s) {
    isle_cave_lifecycle_checked(s, s->services->camera_reset(s));
}
void isle_cave_lifecycle_camera_speed(IsleCaveLifecycleSource* s, IsleCaveCamera* camera, float speed) {
    isle_cave_lifecycle_checked(s, s->services->camera_speed(s, camera, speed));
}
void isle_cave_lifecycle_camera_animation(IsleCaveLifecycleSource* s, IsleCaveCamera* camera, uint32_t animation, float start) {
    isle_cave_lifecycle_checked(s, s->services->camera_animation(s, camera, animation, start));
}
void isle_cave_lifecycle_player_animation(IsleCaveLifecycleSource* s, uint32_t models, uint32_t materials, float start, float speed) {
    isle_cave_lifecycle_checked(s, s->services->player_animation(s, models, materials, start, speed));
}
void isle_cave_lifecycle_animation_callback(IsleCaveLifecycleSource* s, IsleGameObject* object, IsleCaveLifecycleAnimationCallback callback) {
    isle_cave_lifecycle_checked(s, s->services->animation_callback(s, object, callback));
}
IsleCaveCamera* isle_cave_lifecycle_main_camera(IsleCaveLifecycleSource* s) {
    IsleCaveCamera* output = {0};
    isle_cave_lifecycle_checked(s, s->services->main_camera(s, &output));
    return output;
}
IsleGameObject* isle_cave_lifecycle_player_model_init(IsleCaveLifecycleSource* s) {
    IsleGameObject* output = {0};
    isle_cave_lifecycle_checked(s, s->services->player_model_init(s, &output));
    return output;
}
IsleGameProcess* isle_cave_lifecycle_camera_process(IsleCaveLifecycleSource* s, IsleGameObject* object, int32_t kind, int32_t priority) {
    IsleGameProcess* output = {0};
    isle_cave_lifecycle_checked(s, s->services->camera_process(s, object, kind, priority, &output));
    return output;
}
IsleCaveEffect* isle_cave_lifecycle_effect_create(IsleCaveLifecycleSource* s, int32_t bank, int32_t id) {
    IsleCaveEffect* output = {0};
    isle_cave_lifecycle_checked(s, s->services->effect_create(s, bank, id, &output));
    return output;
}
void isle_cave_lifecycle_effect_model(IsleCaveLifecycleSource* s, IsleCaveEffect* effect, IsleActorNodeRef model) {
    isle_cave_lifecycle_checked(s, s->services->effect_model(s, effect, model));
}
float isle_cave_lifecycle_camera_read(IsleCaveLifecycleSource* s, IsleCaveCamera* camera, uint32_t component) {
    float output = {0};
    isle_cave_lifecycle_checked(s, s->services->camera_read(s, camera, component, &output));
    return output;
}
void isle_cave_lifecycle_camera_store(IsleCaveLifecycleSource* s, IsleCaveCamera* camera, uint32_t component, float value) {
    isle_cave_lifecycle_checked(s, s->services->camera_store(s, camera, component, value));
}
uint32_t isle_cave_lifecycle_input_pressed(IsleCaveLifecycleSource* s) {
    uint32_t output = {0};
    isle_cave_lifecycle_checked(s, s->services->input_pressed(s, &output));
    return output;
}
IsleGameObject* isle_cave_lifecycle_camera_object(IsleCaveLifecycleSource* s, IsleCaveCamera* camera) {
    IsleGameObject* output = {0};
    isle_cave_lifecycle_checked(s, s->services->camera_object(s, camera, &output));
    return output;
}
void isle_cave_lifecycle_height_map(IsleCaveLifecycleSource* s, uint32_t identity) {
    isle_cave_lifecycle_checked(s, s->services->height_map(s, identity));
}
void isle_cave_lifecycle_ceiling_map(IsleCaveLifecycleSource* s, uint32_t identity) {
    isle_cave_lifecycle_checked(s, s->services->ceiling_map(s, identity));
}
void isle_cave_lifecycle_world_create(IsleCaveLifecycleSource* s, uint32_t descriptor, int32_t sky, int32_t ground, int32_t camera, int32_t blocks, int32_t objects, IsleCaveWorldPairCallback add, IsleCaveWorldSingleCallback remove, IsleCaveWorldPairCallback change) {
    isle_cave_lifecycle_checked(s, s->services->world_create(s, descriptor, sky, ground, camera, blocks, objects, add, remove, change));
}
void isle_cave_lifecycle_sky_follow(IsleCaveLifecycleSource* s) {
    isle_cave_lifecycle_checked(s, s->services->sky_follow(s));
}
void isle_cave_lifecycle_source_80363928(IsleCaveLifecycleSource* s, int32_t a, int32_t b, int32_t c, int32_t d) {
    isle_cave_lifecycle_checked(s, s->services->source_80363928(s, a, b, c, d));
}
void isle_cave_lifecycle_source_800A19D8(IsleCaveLifecycleSource* s, int32_t reason) {
    isle_cave_lifecycle_checked(s, s->services->source_800A19D8(s, reason));
}
void isle_cave_lifecycle_detector_disable(IsleCaveLifecycleSource* s) {
    isle_cave_lifecycle_checked(s, s->services->detector_disable(s));
}
void isle_cave_lifecycle_environment_cleanup(IsleCaveLifecycleSource* s) {
    isle_cave_lifecycle_checked(s, s->services->environment_cleanup(s));
}
void isle_cave_lifecycle_world_destroy(IsleCaveLifecycleSource* s) {
    isle_cave_lifecycle_checked(s, s->services->world_destroy(s));
}
void isle_cave_lifecycle_source_80356FBC(IsleCaveLifecycleSource* s) {
    isle_cave_lifecycle_checked(s, s->services->source_80356FBC(s));
}
void isle_cave_lifecycle_camera_remove(IsleCaveLifecycleSource* s) {
    isle_cave_lifecycle_checked(s, s->services->camera_remove(s));
}
void isle_cave_lifecycle_source_800067DC(IsleCaveLifecycleSource* s) {
    isle_cave_lifecycle_checked(s, s->services->source_800067DC(s));
}
void isle_cave_lifecycle_items_displace(IsleCaveLifecycleSource* s, float x, float y, float z) {
    isle_cave_lifecycle_checked(s, s->services->items_displace(s, x, y, z));
}
void isle_cave_lifecycle_fx_bank(IsleCaveLifecycleSource* s, int32_t bank, IsleCaveLifecycleAsset* script, IsleCaveLifecycleAsset* sprites) {
    isle_cave_lifecycle_checked(s, s->services->fx_bank(s, bank, script, sprites));
}
void isle_cave_lifecycle_fx_dither(IsleCaveLifecycleSource* s, int32_t color, int32_t alpha) {
    isle_cave_lifecycle_checked(s, s->services->fx_dither(s, color, alpha));
}
void isle_cave_lifecycle_screen_apply(IsleCaveLifecycleSource* s, const IsleCaveLifecycleScreen* screen) {
    isle_cave_lifecycle_checked(s, s->services->screen_apply(s, screen));
}
void isle_cave_lifecycle_fx_reset_bank(IsleCaveLifecycleSource* s, size_t index) {
    isle_cave_lifecycle_checked(s, s->services->fx_reset_bank(s, index));
}
void isle_cave_lifecycle_source_8009CE00(IsleCaveLifecycleSource* s) {
    isle_cave_lifecycle_checked(s, s->services->source_8009CE00(s));
}
void isle_cave_lifecycle_pokemon_initialize(IsleCaveLifecycleSource* s) {
    isle_cave_lifecycle_checked(s, s->services->pokemon_initialize(s));
}
void isle_cave_lifecycle_background_color(IsleCaveLifecycleSource* s, uint8_t* r, uint8_t* g, uint8_t* b) {
    isle_cave_lifecycle_checked(s, s->services->background_color(s, r, g, b));
}
void isle_cave_lifecycle_camera_create(IsleCaveLifecycleSource* s, uint32_t color) {
    isle_cave_lifecycle_checked(s, s->services->camera_create(s, color));
}
void isle_cave_lifecycle_camera_flags(IsleCaveLifecycleSource* s, uint32_t mask, uint32_t value) {
    isle_cave_lifecycle_checked(s, s->services->camera_flags(s, mask, value));
}
void isle_cave_lifecycle_ui_initialize(IsleCaveLifecycleSource* s, IsleCaveWorldSingleCallback exit, IsleCaveMovementCallback movement, IsleGameSourceCallback update_items, uint8_t item_kind, IsleCaveItemCallback collision) {
    isle_cave_lifecycle_checked(s, s->services->ui_initialize(s, exit, movement, update_items, item_kind, collision));
}
void isle_cave_lifecycle_end_callback(IsleCaveLifecycleSource* s, IsleCaveEndCallback callback) {
    isle_cave_lifecycle_checked(s, s->services->end_callback(s, callback));
}
void isle_cave_lifecycle_pause_callback(IsleCaveLifecycleSource* s, IsleCaveEndCallback callback) {
    isle_cave_lifecycle_checked(s, s->services->pause_callback(s, callback));
}
void isle_cave_lifecycle_environment_initialize(IsleCaveLifecycleSource* s, const IsleCaveLifecycleEnvironment* rows, size_t count) {
    isle_cave_lifecycle_checked(s, s->services->environment_initialize(s, rows, count));
}
void isle_cave_lifecycle_detector_create(IsleCaveLifecycleSource* s) {
    isle_cave_lifecycle_checked(s, s->services->detector_create(s));
}
void isle_cave_lifecycle_detector_enable(IsleCaveLifecycleSource* s) {
    isle_cave_lifecycle_checked(s, s->services->detector_enable(s));
}
void isle_cave_lifecycle_pitch_limits(IsleCaveLifecycleSource* s, float minimum, float maximum) {
    isle_cave_lifecycle_checked(s, s->services->pitch_limits(s, minimum, maximum));
}
void isle_cave_lifecycle_intervals(IsleCaveLifecycleSource* s, int32_t update, int32_t draw) {
    isle_cave_lifecycle_checked(s, s->services->intervals(s, update, draw));
}
void isle_cave_lifecycle_near_clipping(IsleCaveLifecycleSource* s, int32_t disabled) {
    isle_cave_lifecycle_checked(s, s->services->near_clipping(s, disabled));
}
void isle_cave_lifecycle_source_800AAED0(IsleCaveLifecycleSource* s, int32_t flags) {
    isle_cave_lifecycle_checked(s, s->services->source_800AAED0(s, flags));
}
void isle_cave_lifecycle_source_803570B0(IsleCaveLifecycleSource* s) {
    isle_cave_lifecycle_checked(s, s->services->source_803570B0(s));
}
void isle_cave_lifecycle_source_80357120(IsleCaveLifecycleSource* s, IsleGameObject* object) {
    isle_cave_lifecycle_checked(s, s->services->source_80357120(s, object));
}
void isle_cave_lifecycle_source_80357170(IsleCaveLifecycleSource* s) {
    isle_cave_lifecycle_checked(s, s->services->source_80357170(s));
}
void isle_cave_lifecycle_source_803571C4(IsleCaveLifecycleSource* s) {
    isle_cave_lifecycle_checked(s, s->services->source_803571C4(s));
}
void isle_cave_lifecycle_source_803572B0(IsleCaveLifecycleSource* s) {
    isle_cave_lifecycle_checked(s, s->services->source_803572B0(s));
}
IsleCaveWorldBlock* isle_cave_lifecycle_block_next(IsleCaveLifecycleSource* s, IsleCaveWorldBlock* block) {
    IsleCaveWorldBlock* output = {0};
    isle_cave_lifecycle_checked(s, s->services->block_next(s, block, &output));
    return output;
}
IsleCaveLifecycleAsset* isle_cave_lifecycle_asset_load(IsleCaveLifecycleSource* s, IsleCaveLifecycleAssetId id) {
    IsleCaveLifecycleAsset* output = {0};
    isle_cave_lifecycle_checked(s, s->services->asset_load(s, id, &output));
    return output;
}
IsleGameObject* isle_cave_lifecycle_fx_pool(IsleCaveLifecycleSource* s, int32_t count) {
    IsleGameObject* output = {0};
    isle_cave_lifecycle_checked(s, s->services->fx_pool(s, count, &output));
    return output;
}
IsleGameObject* isle_cave_lifecycle_fx_manager(IsleCaveLifecycleSource* s, int32_t link, int32_t count, IsleCaveCamera* camera) {
    IsleGameObject* output = {0};
    isle_cave_lifecycle_checked(s, s->services->fx_manager(s, link, count, camera, &output));
    return output;
}
void* isle_cave_lifecycle_allocate(IsleCaveLifecycleSource* s, size_t bytes, size_t alignment) {
    void* output = {0};
    isle_cave_lifecycle_checked(s, s->services->allocate(s, bytes, alignment, &output));
    return output;
}
IsleGameObject* isle_cave_lifecycle_add_helper(IsleCaveLifecycleSource* s, int32_t id, IsleGameSourceCallback callback, uint8_t link, uint32_t priority) {
    IsleGameObject* output = {0};
    isle_cave_lifecycle_checked(s, s->services->add_helper(s, id, callback, link, priority, &output));
    return output;
}
int32_t isle_cave_lifecycle_source_8009BC68(IsleCaveLifecycleSource* s) {
    int32_t output = {0};
    isle_cave_lifecycle_checked(s, s->services->source_8009BC68(s, &output));
    return output;
}
void isle_cave_lifecycle_scene_request(IsleCaveLifecycleSource* s, const IsleCaveLifecycleSceneSetup* setup) {
    isle_cave_lifecycle_checked(s, s->services->scene_request(s, setup));
}
void isle_cave_lifecycle_pokemon_add(IsleCaveLifecycleSource* s, IsleCaveWorldBlock* a, IsleCaveWorldBlock* b, const IsleCaveLifecycleDefinition* definitions, size_t count) {
    isle_cave_lifecycle_checked(s, s->services->pokemon_add(s, a, b, definitions, count));
}
void isle_cave_lifecycle_pokemon_change(IsleCaveLifecycleSource* s, IsleCaveWorldBlock* a, IsleCaveWorldBlock* b, const IsleCaveLifecycleDefinition* definitions, size_t count) {
    isle_cave_lifecycle_checked(s, s->services->pokemon_change(s, a, b, definitions, count));
}
void isle_cave_lifecycle_pokemon_remove(IsleCaveLifecycleSource* s, IsleCaveWorldBlock* block, const IsleCaveLifecycleDefinition* definitions, size_t count) {
    isle_cave_lifecycle_checked(s, s->services->pokemon_remove(s, block, definitions, count));
}
void isle_cave_lifecycle_fx_manager_assign(IsleCaveLifecycleSource* s, IsleGameObject* object) {
    isle_cave_lifecycle_checked(s, s->services->fx_manager_assign(s, object));
}
void isle_cave_lifecycle_fx_pool_assign(IsleCaveLifecycleSource* s, IsleGameObject* object) {
    isle_cave_lifecycle_checked(s, s->services->fx_pool_assign(s, object));
}

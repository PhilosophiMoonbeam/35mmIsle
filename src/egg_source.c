#include "egg_source.h"
#include <stdlib.h>
IsleGameResult isle_egg_source_prepare(IsleEgg* egg, IsleCourseActorBase* base,
    const void* source, void* owner, const uint8_t* rom, size_t bytes,
    IsleEggVertex* frame0, size_t count, const IsleEggPreparation* p) {
    uint32_t data, table, zero, endpoint;
    if (!egg || !base || !source || !owner || !rom || !frame0 || !p)
        return ISLE_GAME_INVALID_ARGUMENT;
    if (base->course_id==1) { data=0x80188d28u; table=0x802eff0cu; zero=0x803431f0u; endpoint=0x8018a6f0u; if (count!=336) return ISLE_GAME_INVALID_STATE; }
    else if (base->course_id==4) { data=0x8017a780u; table=0x802c7c20u; zero=0x8033e210u; endpoint=0x8017c090u; if (count!=324) return ISLE_GAME_INVALID_STATE; }
    else return ISLE_GAME_INVALID_STATE;
    if (base->phase!=ISLE_COURSE_ACTOR_UNPREPARED || base->owner!=owner ||
        base->nativeGame || base->nonwaiting_call_frame || base->status!=ISLE_GAME_OK ||
        p->source!=source || p->owner!=owner || p->rom!=rom || p->rom_bytes!=bytes || bytes!=0x1000000u ||
        p->course_id!=base->course_id || p->data_identity!=data || p->temp_buffer_identity!=table ||
        p->frame0_identity!=zero || p->frame1_identity!=endpoint || p->vertex_count!=(int32_t)count ||
        egg->data_identity!=data || egg->temp_buffer_identity!=table || egg->vertex_count!=(int32_t)count || egg->end!=2 || egg->timer!=0.0f ||
        egg->data || egg->temp_buffer || egg->course || egg->native_geometry || egg->bind || egg->revision ||
        !p->data || !p->frames || p->frames[0]!=frame0 || !p->frames[1] ||
        p->data==frame0 || p->data==p->frames[1] || frame0==p->frames[1] || !p->native_geometry || !p->bind)
        return ISLE_GAME_INVALID_STATE;
    egg->data=p->data; egg->temp_buffer=p->frames;
    egg->course=base; egg->native_geometry=p->native_geometry; egg->bind=p->bind;
    egg->revision=1;
    return ISLE_GAME_OK;
}
void isle_egg_source_bind(IsleActorSource* actor, IsleEgg* egg) {
    IsleGameResult result=ISLE_GAME_INVALID_STATE;
    if (actor && egg && egg->course && egg->bind && actor->eggGeo==egg)
        result=egg->bind(egg,actor);
    if (egg && egg->course) isle_course_actor_checked(egg->course,result);
    else abort();
}
void isle_egg_source_commit(IsleEgg* egg) {
    if (!egg || !egg->course) abort();
    isle_course_actor_checked(egg->course, egg->revision==UINT64_MAX ? ISLE_GAME_INVALID_STATE : ISLE_GAME_OK);
    ++egg->revision;
}

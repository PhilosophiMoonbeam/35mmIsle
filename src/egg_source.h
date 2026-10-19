#ifndef ISLE_EGG_SOURCE_H
#define ISLE_EGG_SOURCE_H
#include "course_actor_source.h"
#include "actor_source.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Semantic Source-width vertices. Never overlay canonical guest RAM. */
typedef struct IsleEggVertex {
    int16_t unk_00, unk_02, unk_04;
    uint8_t unk_06[6];
    int8_t unk_0C, unk_0D, unk_0E;
    uint8_t unk_0F;
} IsleEggVertex;
typedef struct IsleEgg IsleEgg;
struct IsleEgg {
    uint32_t data_identity, temp_buffer_identity;
    IsleEggVertex* data;
    IsleEggVertex** temp_buffer;
    int32_t vertex_count, end;
    float timer;
    uint64_t revision;
    IsleCourseActorBase* course;
    void* native_geometry;
    IsleGameResult (*bind)(IsleEgg*, IsleActorSource*);
};
/* Explicit, fixed-address pre-Game capability, consumed after Source memset.
 * Adapter and native geometry owner retain every pointed-to array until joins. */
typedef struct IsleEggPreparation {
    const void* source;
    void* owner;
    const uint8_t* rom;
    size_t rom_bytes;
    int32_t course_id;
    uint32_t data_identity, temp_buffer_identity, frame0_identity, frame1_identity;
    int32_t vertex_count;
    IsleEggVertex* data;
    IsleEggVertex** frames;
    void* native_geometry;
    IsleGameResult (*bind)(IsleEgg*, IsleActorSource*);
} IsleEggPreparation;
IsleGameResult isle_egg_source_prepare(IsleEgg*, IsleCourseActorBase*, const void*,
    void*, const uint8_t*, size_t, IsleEggVertex*, size_t, const IsleEggPreparation*);
void isle_egg_source_bind(IsleActorSource*, IsleEgg*);
void isle_egg_source_commit(IsleEgg*);
int8_t isle_egg_original_func_800AAFB0(int16_t, int16_t, float);
int16_t isle_egg_original_func_800AB000(int16_t, int16_t, float);
void isle_egg_original_func_800AB050(float, int32_t, IsleEggVertex*, IsleEggVertex*, IsleEggVertex*);
int32_t isle_egg_original_func_800AB138(IsleEgg*, float);
void isle_egg_original_func_800AB1F8(IsleEgg*);
void isle_egg_original_func_80362414_502824(IsleActorSource*, IsleEgg*, float);
#ifdef __cplusplus
}
static_assert(sizeof(IsleEggVertex)==16);
static_assert(offsetof(IsleEggVertex,unk_06)==6 && offsetof(IsleEggVertex,unk_0C)==12 && offsetof(IsleEggVertex,unk_0F)==15);
#else
_Static_assert(sizeof(IsleEggVertex)==16,"Source vertex width");
_Static_assert(offsetof(IsleEggVertex,unk_06)==6 && offsetof(IsleEggVertex,unk_0C)==12 && offsetof(IsleEggVertex,unk_0F)==15,"Source vertex offsets");
#endif
#endif

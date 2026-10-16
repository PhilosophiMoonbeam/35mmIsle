#ifndef ISLE_MAGIKARP_OWNER_PROBE_H
#define ISLE_MAGIKARP_OWNER_PROBE_H
#include "actor_owner_source.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Observation-only C callback: the selected source helper must run inside a
 * genuine native callback/fault fence, never through a waiting C++ frame. */
typedef struct IsleMagikarpGeoProbe {
    IsleGameObject* child;
    unsigned calls;
} IsleMagikarpGeoProbe;
void isle_magikarp_geo_probe(IsleGameObject*, void*);
#ifdef __cplusplus
}
#endif
#endif

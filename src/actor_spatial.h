#ifndef ISLE_ACTOR_SPATIAL_H
#define ISLE_ACTOR_SPATIAL_H
#include "actor_source.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Nonwaiting own-GAME pokemon.c444-484/590-628 leaves. Neither reports native
 * execution faults nor transfers control; the C caller handles returned errors.
 * Caller retains object/model/source lifetimes and excludes native list, binding
 * and pose mutation until return. Retired handles reject, even while identity
 * held; released/recycled raw handles are not supported. No pose/cache update,
 * allocation, visibility/tangibility/species/pause filtering or collision query.
 * Outputs are published only on success and must designate distinct storage.
 */
IsleGameResult isle_actor_spatial_distance(IsleGame*, IsleGameObject* first,
    IsleGameObject* second, float* out_distance);
/* Distance reads AUX position when present, otherwise the actual root position.
 * NULL argument or a live model-less second object yields FLT_MAX. A model-less
 * first object, a non-model payload or an invalid/retired non-NULL object errors.
 * NULL arguments win before the other object's model/liveness is consumed.
 */
IsleGameResult isle_actor_spatial_neighbors(IsleActorSource*,
    IsleGameVec3* out_direction, float* out_magnitude);
/* Neighbors require actual AUX placement, including on every non-self link3
 * model. Native link3 order across all owners is authoritative. Only self is
 * excluded. For distance < 100, add distance * normalized(self - neighbor), then
 * normalize the ordered sum and return its pre-normalization magnitude. Output
 * direction may be &actor->dirToNeighbors; canonical state is not otherwise
 * written. Zero, cancellation, underflow and overflow use GAME vector semantics.
 */

/* Checked native bridge query used by the leaves, not a model/identity cache.
 * NULL object validates game access and returns NULL model. An actual live
 * payload-less object likewise returns NULL; all non-NULL models are checked
 * using the existing source-model validator. Output clears on every error.
 */
IsleGameResult isle_actor_spatial_object_model(IsleGame*, IsleGameObject*, IsleGameModel**);

#ifdef __cplusplus
}
#endif
#endif

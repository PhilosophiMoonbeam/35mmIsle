#ifndef ISLE_ACTOR_PHOTO_ELIGIBILITY_HPP
#define ISLE_ACTOR_PHOTO_ELIGIBILITY_HPP

#include "actor_source.h"

namespace isle::actor::photo {
// Independent native values, not an SDK matrix/camera layout. Row-vector math.
// This camera deliberately uses explicit eye/at/up, never the effect-camera roll
// or viewport. Preparation publishes both matrices as one transaction.
struct Camera {
    float view[4][4]{};
    float view_projection[4][4]{};
};
IsleGameResult prepare_camera(const IsleGameCameraParameters&, Camera&) noexcept;
IsleGameResult prepare_camera(IsleGame*, IsleGameCamera*, Camera&) noexcept;

// Reads the live bound source record and its actual attached native model.
// AUX placement must be present (RT or RTS); absent AUX is an invalid state,
// not an identity fallback. Only flag1 consumes root position and AUX scale;
// root rotation/scale and renderer fixed/contextual caches are never consumed.
IsleGameResult collision_position(const IsleActorSource&, IsleGameModel*,
                                  IsleGameVec3& output) noexcept;

// GAME 5047F0.c60-83: camera-space depth, then signed32-truncated X, then Y.
// No projection/viewport/fixed-cache constraints apply. Out-of-domain consumed
// arithmetic/casts return INVALID_ARGUMENT, never a fabricated invisible result.
IsleGameResult point_visible(const Camera&, IsleGameVec3, bool& output) noexcept;

// The caller supplies the canonical collision position, not a rendered origin.
// Only flag100 is changed. Flag40 consumes neither distance, point nor camera;
// distance>10000 consumes neither point nor camera. Errors leave flags/output
// unchanged. No collision-update scheduling or snapshot collection is implied.
IsleGameResult update_visibility(IsleActorSource&, const Camera&,
                                 IsleGameVec3 collision, bool& output) noexcept;
bool species_allowed(int32_t species) noexcept;
// Hidden/!tangible reject before any visibility mutation; species follows it.
IsleGameResult admitted(IsleActorSource&, const Camera&, IsleGameVec3 collision,
                         bool& output) noexcept;

// All functions are nonwaiting and allocation-free. Caller retains native
// objects/models/camera/record and excludes mutation/removal during each call.
// Every output remains unchanged on errors. Stale/recycled raw handles are not
// supported by the existing native lifetime contract. Numeric native scope only:
// binary32/host libm is not a claim of Original numeric or full-photo parity.
} // namespace isle::actor::photo
#endif

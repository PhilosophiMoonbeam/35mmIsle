#ifndef ISLE_ACTOR_PHOTO_MEMBERSHIP_HPP
#define ISLE_ACTOR_PHOTO_MEMBERSHIP_HPP

#include "actor_photo_eligibility.hpp"
#include "35mmisle/focus_detector.hpp"

namespace isle::actor::photo {
// Focus-only projection of GAME 47380.c's native link3 snapshot collection.
// Prepares the actual camera, then admits the first twelve qualifying live
// native identities in native priority/insertion order using each record's
// canonical collPosition. No collision refresh or payload serialization occurs.
// Success replaces the photo, clears focus and nulls every unused identity.
// Errors preserve output, NOT visibility sideeffects on already visited records:
// a caller must abort the draw rather than publish stale/partial membership.
// Stops immediately after the twelfth admission, without visiting its tail.
// Nonwaiting/allocation-free. Caller retains camera/objects/models/records and
// excludes mutation/removal throughout the call and identity reuse throughout
// any downstream photo/history/graphics borrow. No raw stale-handle recovery.
IsleGameResult collect_membership(IsleGame*, IsleGameCamera*, focus::Photo&) noexcept;
} // namespace isle::actor::photo
#endif

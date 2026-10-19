#pragma once
#include "35mmisle/engine.h"
#include "menu_effects.hpp"

namespace isle::menu::model {
// Pure borrowed accepted-pose traversal. Caller excludes mutation/removal for
// this call and retains the model's lifetime; stale handles are not supported.
// Raw row-vector world: callers own column normalization, not this helper.
// Native placement represents root auxiliary RT/RTS, after the root local pose.
// Flags and renderer/contextual matrix selections do not suppress source FX math.
// On any error output is unchanged. No allocation or native cache evaluation.
EffectsStatus source_effect_world(IsleGame*, IsleGameModel*, std::size_t node,
                                 menu::EffectWorld& output) noexcept;
// Actual Source fx_getPosVelDObj output primitive. The caller resolves/retains
// the true model incarnation first. Item/Player callback requesters need no
// fabricated Actor payload. Velocity is an input direction and output direction.
// On any error both outputs are unchanged.
IsleGameResult source_effect_position_velocity(IsleGame*, IsleGameModel*, std::size_t,
                                              IsleGameVec3*, IsleGameVec3*) noexcept;
}

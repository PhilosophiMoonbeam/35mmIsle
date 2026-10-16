#pragma once
#include "35mmisle/engine.h"
#include "menu_effects.hpp"

namespace isle::menu::model {
// Pure borrowed accepted-pose traversal. Caller excludes mutation/removal for
// this call and retains the model's lifetime; stale handles are not supported.
// Raw row-vector world: EffectRuntime owns column normalization, not this helper.
// Native placement represents root auxiliary RT/RTS, after the root local pose.
// Flags and renderer/contextual matrix selections do not suppress source FX math.
// On any error output is unchanged. No allocation or native cache evaluation.
EffectsStatus source_effect_world(IsleGame*, IsleGameModel*, std::size_t node,
                                 menu::EffectWorld& output) noexcept;
}

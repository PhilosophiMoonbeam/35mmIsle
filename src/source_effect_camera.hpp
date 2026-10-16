#pragma once
#include "35mmisle/engine.h"
#include "menu_effects.hpp"

namespace isle::menu {
// GAME effect.c998-1012 evaluates perspective and roll view from actual camera
// parameters with fixed up=(0,1,0), independently of renderer matrix kinds/cache.
// No allocation/cache evaluation; output is unchanged on failure. Caller retains
// the native camera and excludes parameter mutation for this borrowed traversal.
bool source_effect_camera(IsleGame*, IsleGameCamera*, EffectCamera&) noexcept;
}

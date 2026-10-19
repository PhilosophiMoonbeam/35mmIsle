#ifndef ISLE_VOLCANO_ACTOR_NATIVE_HPP
#define ISLE_VOLCANO_ACTOR_NATIVE_HPP
#include "volcano_actor_source.h"
#include "player_cart_source.h"
#include <cstdint>
namespace isle::course {
// Installed cold on the actual Actor Producer by NativeHost, never an OM hook.
IsleGameResult volcano_actor_source_render(void*, IsleGameObject*, std::uint32_t) noexcept;
}
extern "C" void isle_volcano_actor_cart_event(void*, const IslePlayerCartNode*, int32_t, float);
#endif

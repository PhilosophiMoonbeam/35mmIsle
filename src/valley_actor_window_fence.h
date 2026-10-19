#ifndef ISLE_VALLEY_ACTOR_WINDOW_FENCE_H
#define ISLE_VALLEY_ACTOR_WINDOW_FENCE_H
#include "valley_actor_source.h"
#include "player_ui_source.h"
#ifdef __cplusplus
extern "C" {
#endif
IsleGameResult isle_valley_window_invoke_show(IsleValleyActorSource*,IslePlayerUiSource*,IsleValleyMessageCallback,const char*,int32_t,int32_t,uint8_t,uint8_t,uint8_t,uint8_t,int32_t,uint8_t);
IsleGameResult isle_valley_window_install_handler(IslePlayerUiSource*,int enabled);
#ifdef __cplusplus
}
#endif
#endif

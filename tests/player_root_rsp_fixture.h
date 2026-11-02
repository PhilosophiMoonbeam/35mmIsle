#ifndef ISLE_PLAYER_ROOT_RSP_FIXTURE_H
#define ISLE_PLAYER_ROOT_RSP_FIXTURE_H
#include "player_root_source.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Explicit public native inputs, not detector observations or earned progress. */
typedef struct PlayerRootRspLabel {
    int32_t species, visible, hidden;
} PlayerRootRspLabel;
void player_root_rsp_label(IslePlayerSource*, void*);
void player_root_rsp_tutorial_message(IslePlayerSource*, void*);
#ifdef __cplusplus
}
#endif
#endif

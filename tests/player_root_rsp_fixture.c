#include "player_root_rsp_fixture.h"

/* Checked Source transfers must stay inside plain-C frames. These thunks call
 * the actual pinned Root bodies; they do not reproduce their branch logic. */
void player_root_rsp_label(IslePlayerSource* player, void* user) {
    PlayerRootRspLabel* call = user;
    isle_player_showPokemonLabel(player, call->species, call->visible, &call->hidden);
}

void player_root_rsp_tutorial_message(IslePlayerSource* player, void* user) {
    isle_player_Tutorial_ShowMessage(player, *(const int32_t*)user);
}

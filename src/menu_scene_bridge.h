#ifndef ISLE_MENU_SCENE_BRIDGE_H
#define ISLE_MENU_SCENE_BRIDGE_H

#include "35mmisle/engine.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Private native metadata only; never a console layout or GPU allocation. */
IsleGameResult isle_menu_reserve_pools(IsleGame*, isle_arena*, size_t objects,
    size_t processes, size_t sprites, size_t cameras, size_t matrices);
IsleGameObject* isle_menu_current_object(IsleGame*);
IsleGameObject* isle_menu_first_object(IsleGame*, uint8_t update_link);
IsleGameObject* isle_menu_next_object(IsleGame*, IsleGameObject*);
IsleGameResult isle_menu_end_processes(IsleGame*, IsleGameObject*);
IsleGameResult isle_menu_set_update(IsleGame*, IsleGameObject*, IsleGameCallback, void*);
/* Reinsert a newly created process-free equal-priority object before a peer. */
IsleGameResult isle_menu_place_before(IsleGame*, IsleGameObject*, IsleGameObject*);
int isle_menu_object_hidden(IsleGame*, const IsleGameObject*);
/* Active stackful callback only; reuses the retained exact GAME RGB fade. */
IsleGameResult isle_menu_logo_fade(IsleGame*, IsleGameSprite*, uint16_t updates);
/* Exact GAME float-to-fixed packing; native scale, no kind28 quantization. */
IsleGameResult isle_menu_scale_matrix(const IsleGameVec3*, IsleGameFixedMatrix*);

#ifdef __cplusplus
}
#endif
#endif

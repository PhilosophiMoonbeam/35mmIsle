#ifndef ISLE_PLAYER_PHOTO_RSP_FIXTURE_H
#define ISLE_PLAYER_PHOTO_RSP_FIXTURE_H
#include "player_photo_source.h"
#ifdef __cplusplus
extern "C" {
#endif
/* C-only entries keep native source failure transfers out of C++ frames. */
typedef struct PlayerPhotoRspMake {
    IsleGameObject* subject;
    int32_t value;
    IsleGameResult status;
    unsigned entered, returned;
} PlayerPhotoRspMake;
typedef struct PlayerPhotoRspCopies {
    unsigned entered, returned;
} PlayerPhotoRspCopies;
void player_photo_rsp_make(IslePlayerSource*, void*);
void player_photo_rsp_copy_info(IsleGameObject*, void*);
#ifdef __cplusplus
}
#endif
#endif

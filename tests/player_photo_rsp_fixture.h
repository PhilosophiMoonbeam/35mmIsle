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
typedef struct PlayerPhotoRspRelease {
    IslePlayerPhotoTicket ticket;
    IsleGameResult released, status;
    unsigned entered, returned;
} PlayerPhotoRspRelease;
typedef struct PlayerPhotoRspRetiredSubject {
    IsleGameObject* subject;
    IslePlayerPhotoFocus before, after;
    IsleGameResult lookup;
    unsigned entered, returned, missing, cleared;
} PlayerPhotoRspRetiredSubject;
/* Diagnostic C suffix after a real Source camera callback; the observer is a
 * returning leaf. Its failure is checked only after its C++ frames unwind. */
typedef struct PlayerPhotoRspBoundary {
    void (*render)(IslePlayerSource*, IsleGameObject*);
    IsleGameResult (*observe)(void*);
    void* context;
    uint32_t suffix_lanes;
    unsigned entered, returned;
} PlayerPhotoRspBoundary;
void player_photo_rsp_boundary_bind(PlayerPhotoRspBoundary*);
void player_photo_rsp_boundary_render(IslePlayerSource*, IsleGameObject*);
void player_photo_rsp_make(IslePlayerSource*, void*);
void player_photo_rsp_make_raw(IslePlayerSource*, void*);
void player_photo_rsp_make_checked(IslePlayerSource*, void*);
void player_photo_rsp_release_checked(IslePlayerSource*, void*);
void player_photo_rsp_consume_retired_subject(IslePlayerSource*, void*);
void player_photo_rsp_copy_info(IsleGameObject*, void*);
#ifdef __cplusplus
}
#endif
#endif

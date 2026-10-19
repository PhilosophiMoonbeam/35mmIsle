#ifndef ISLE_PHOTO_PRESENTATION_H
#define ISLE_PHOTO_PRESENTATION_H
#include "application_photos_source.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct IslePhotoPresentation IslePhotoPresentation;
typedef enum IslePhotoPresentationStatus {
    ISLE_PHOTO_PRESENTATION_IDLE = 0,
    ISLE_PHOTO_PRESENTATION_PENDING = 1,
    ISLE_PHOTO_PRESENTATION_READY = 2,
    ISLE_PHOTO_PRESENTATION_FAILED = 3,
    ISLE_PHOTO_PRESENTATION_RETIRED = 4
} IslePhotoPresentationStatus;
/* These returning leaves never hide/unhide a camera, dispatch Source, wait,
 * advance OM/GTL, or fabricate graphics completion. Source owns that order.
 * begin rejects until clear has joined the previous retained photo. */
int isle_photo_presentation_begin(IslePhotoPresentation*, IsleGameCamera*, const IslePhotoRecord*);
int isle_photo_presentation_begin_working(IslePhotoPresentation*, IsleGameCamera*, IslePhotoWorking*);
IslePhotoPresentationStatus isle_photo_presentation_poll(IslePhotoPresentation*);
IslePhotoPresentationStatus isle_photo_presentation_clear(IslePhotoPresentation*);
IslePhotoPresentationStatus isle_photo_presentation_release_to_replay(IslePhotoPresentation*);
IslePhotoPresentationStatus isle_photo_presentation_retire(IslePhotoPresentation*);
int isle_photo_presentation_pause_links(IslePhotoPresentation*, uint64_t mask, int paused);
/* Exact 8009FC38 course preparation, delegated to the existing thumbnail
 * Replay owner: no photo reconstruction, destination write or readback here. */
IslePhotoPresentationStatus isle_photo_presentation_prepare_course(IslePhotoPresentation*, const IslePhotoRecord*);
#ifdef __cplusplus
}
#endif
#endif

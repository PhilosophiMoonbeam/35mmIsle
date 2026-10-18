#ifndef ISLE_APPLICATION_PHOTOS_SOURCE_H
#define ISLE_APPLICATION_PHOTOS_SOURCE_H
#include "player_photo_source.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Opaque application owner. No guest record/bitfield layout is imported. */
typedef struct IsleApplicationPhotos IsleApplicationPhotos;
typedef enum IslePhotoStorage {
    ISLE_PHOTO_ROLL, ISLE_PHOTO_REPORT, ISLE_PHOTO_TEMPORARY, ISLE_PHOTO_ALBUM
} IslePhotoStorage;
enum {
    ISLE_PHOTO_ROLL_CAPACITY = 60, ISLE_PHOTO_REPORT_CAPACITY = 69,
    ISLE_PHOTO_TEMPORARY_CAPACITY = 4, ISLE_PHOTO_ALBUM_CAPACITY = 60
};
typedef struct IsleSelectedPhoto {
    IslePlayerPhotoData* photo; /* Actual application roll record, never a scene copy. */
    bool for_oak;
    bool source_second_flag;
    bool for_album;
} IsleSelectedPhoto;
/* Read-only semantic projection/roll reference. The owner outlives borrowers;
 * data's address survives waits, replacement, removal and session reset. Its
 * VALUE follows canonical mutations. Revision detects replacement, not GPU
 * completion: this is NOT a replay/submission capability. A replay owner must
 * retain its actual request/submission and serialize against record mutation.
 * Scorers write flags through apply_photo_flags, never through a const cast. */
typedef struct IslePhotoRecord {
    IsleApplicationPhotos* owner;
    const IslePlayerPhotoData* photo; /* NULL is an absent source record. */
    IslePhotoStorage storage;
    int32_t index; /* Canonical slot, NOT a species ID. */
    uint64_t revision;
} IslePhotoRecord;
/* All leaves return normally; no Source callbacks, ticks, waits or transfers.
 * Exceptions are contained and reported as ISLE_GAME_INVALID_STATE. Outputs are
 * cleared on failure. No operation here commits to durable storage. */
IsleGameResult isle_application_photos_select(IsleApplicationPhotos*, IslePlayerPhotoData*,
    int for_oak, int source_second_flag, int for_album);
IsleGameResult isle_application_photos_selected_count(IsleApplicationPhotos*, int32_t*);
IsleGameResult isle_application_photos_selected(IsleApplicationPhotos*, int32_t, const IsleSelectedPhoto**);
/* Checked generic lookup: invalid enum/index is an argument error. Missing
 * saved records return OK with record.photo=NULL. Roll bounds use actual count. */
IsleGameResult isle_application_photos_record(IsleApplicationPhotos*, IslePhotoStorage, int32_t, IslePhotoRecord*);
/* Retained original pointer -> owner/set/slot identity (NOT authorization).
 * Unknown pointers fail; NULL is the genuine source NULL input. */
IsleGameResult isle_application_photos_identify(IsleApplicationPhotos*, const IslePlayerPhotoData*, IslePhotoRecord*);
IsleGameResult isle_application_photos_apply_photo_flags(IsleApplicationPhotos*, const IslePhotoRecord*, uint8_t);
/* Source func_8009BB4C mapping, including 603->90 and all six signs. An unknown
 * source ID maps to -1; original report getters return NULL/0 and setters ignore
 * it. Photo and score operations remain deliberately separate. */
IsleGameResult isle_application_photos_report_index(IsleApplicationPhotos*, int32_t source_id, int32_t*);
IsleGameResult isle_application_photos_report(IsleApplicationPhotos*, int32_t source_id, IslePhotoRecord*);
IsleGameResult isle_application_photos_report_present(IsleApplicationPhotos*, int32_t source_id, int*);
/* Original count/ordinal walk ordinary IDs 1..151 in source order, not signs. */
IsleGameResult isle_application_photos_report_count(IsleApplicationPhotos*, int32_t*);
IsleGameResult isle_application_photos_report_ordinal(IsleApplicationPhotos*, int32_t ordinal, IslePhotoRecord*);
IsleGameResult isle_application_photos_set_report(IsleApplicationPhotos*, int32_t source_id, const IslePlayerPhotoData*);
IsleGameResult isle_application_photos_remove_report(IsleApplicationPhotos*, int32_t source_id);
IsleGameResult isle_application_photos_report_score(IsleApplicationPhotos*, int32_t source_id, int32_t*);
IsleGameResult isle_application_photos_set_report_score(IsleApplicationPhotos*, int32_t source_id, int32_t);
/* Original temporary/album helpers: invalid signed slot returns NULL or ignores
 * a write/remove. Temporary has FOUR slots (0..3); append/placement stays C. */
IsleGameResult isle_application_photos_temporary(IsleApplicationPhotos*, int32_t, IslePhotoRecord*);
IsleGameResult isle_application_photos_set_temporary(IsleApplicationPhotos*, int32_t, const IslePlayerPhotoData*);
IsleGameResult isle_application_photos_remove_temporary(IsleApplicationPhotos*, int32_t);
IsleGameResult isle_application_photos_album(IsleApplicationPhotos*, int32_t, IslePhotoRecord*);
/* comment is 32 native numeric source characters, explicitly encoded BE16.
 * NULL photo and NULL comment are independently optional; NULL comment retains
 * the old comment. No text transcoding or native-struct serialization. */
IsleGameResult isle_application_photos_set_album(IsleApplicationPhotos*, int32_t,
    const IslePlayerPhotoData*, const uint16_t* comment);
IsleGameResult isle_application_photos_remove_album(IsleApplicationPhotos*, int32_t);
/* Stable read-only 32-character projection, including for an absent photo.
 * Invalid slot returns NULL. Removing a photo clears only character zero. */
IsleGameResult isle_application_photos_album_comment(IsleApplicationPhotos*, int32_t, const uint16_t**);
IsleGameResult isle_application_photos_set_album_comment(IsleApplicationPhotos*, int32_t, const uint16_t*);
IsleGameResult isle_application_photos_pending_item_grant(IsleApplicationPhotos*, int source_index, int*);
IsleGameResult isle_application_photos_set_pending_item_grant(IsleApplicationPhotos*, int source_index, int value);
#ifdef __cplusplus
}
#endif
#endif

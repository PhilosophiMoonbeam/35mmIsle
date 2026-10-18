#ifndef ISLE_APPLICATION_SOURCE_H
#define ISLE_APPLICATION_SOURCE_H

#include "35mmisle/continuation.h"
#include "player_source.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The sole application-lifetime storage for 46270.c globals. Embed this record
 * in game::State; migrate its existing manager latch, never mirror it. The two
 * words have explicit =0 data initializers (lines41–42); idle_script is BSS
 * (line45). Initialize once per application, never on a scene-storage reset. */
typedef struct IsleApplicationManagerState {
    int32_t latch;                 /* D_800AC00C */
    int32_t attract_alternator;    /* D_800AC010 */
    const IslePlayerIdleScript* idle_script; /* D_800B0578; exact existing type */
} IsleApplicationManagerState;
#define ISLE_APPLICATION_MANAGER_STATE_INITIALIZER { 0, 0, NULL }

/* These operations are NOT raw Source scene IDs. In particular Boot has no
 * Source scene argument; Title receives raw8 and Name receives raw0. The same
 * OaksLab/Gallery/Credits operation retains its exact raw argument variant. */
typedef enum IsleApplicationEntryOperation {
    ISLE_APPLICATION_ENTRY_BOOT,
    ISLE_APPLICATION_ENTRY_TITLE,
    ISLE_APPLICATION_ENTRY_NAME,
    ISLE_APPLICATION_ENTRY_BEACH,
    ISLE_APPLICATION_ENTRY_TUNNEL,
    ISLE_APPLICATION_ENTRY_VOLCANO,
    ISLE_APPLICATION_ENTRY_RIVER,
    ISLE_APPLICATION_ENTRY_CAVE,
    ISLE_APPLICATION_ENTRY_VALLEY,
    ISLE_APPLICATION_ENTRY_RAINBOW,
    ISLE_APPLICATION_ENTRY_CAMERA_CHECK,
    ISLE_APPLICATION_ENTRY_OAKS_LAB,
    ISLE_APPLICATION_ENTRY_ALBUM,
    ISLE_APPLICATION_ENTRY_REPORT,
    ISLE_APPLICATION_ENTRY_PHOTO_CHECK,
    ISLE_APPLICATION_ENTRY_GALLERY,
    ISLE_APPLICATION_ENTRY_END_LEVEL_8,
    ISLE_APPLICATION_ENTRY_CREDITS,
    ISLE_APPLICATION_ENTRY_PRINTER_DIAGNOSTIC
} IsleApplicationEntryOperation;

/* Typed resource identities, not console Overlay layouts, guest addresses,
 * relocated function pointers, or synthetic Source scene numbers. */
typedef enum IsleApplicationOverlay {
    ISLE_APPLICATION_OVERLAY_UNK_SEGMENT_AA18E0,
    ISLE_APPLICATION_OVERLAY_WINDOW,
    ISLE_APPLICATION_OVERLAY_CAMERA_CHECK,
    ISLE_APPLICATION_OVERLAY_OAKS_LAB,
    ISLE_APPLICATION_OVERLAY_POKEMON_ALBUM,
    ISLE_APPLICATION_OVERLAY_POKEMON_REPORT,
    ISLE_APPLICATION_OVERLAY_PHOTO_CHECK,
    ISLE_APPLICATION_OVERLAY_GALLERY,
    ISLE_APPLICATION_OVERLAY_UNK_END_LEVEL_8,
    ISLE_APPLICATION_OVERLAY_MORE_FUNCS,
    ISLE_APPLICATION_OVERLAY_MAIN_MENU,
    ISLE_APPLICATION_OVERLAY_MENU_NEW_GAME,
    ISLE_APPLICATION_OVERLAY_CREDITS,
    ISLE_APPLICATION_OVERLAY_INTRO_CODE,
    ISLE_APPLICATION_OVERLAY_APP_LEVEL,
    ISLE_APPLICATION_OVERLAY_WORLD,
    ISLE_APPLICATION_OVERLAY_BEACH_ASSETS,
    ISLE_APPLICATION_OVERLAY_BEACH_CODE,
    ISLE_APPLICATION_OVERLAY_TUNNEL_ASSETS,
    ISLE_APPLICATION_OVERLAY_TUNNEL_CODE,
    ISLE_APPLICATION_OVERLAY_CAVE_ASSETS,
    ISLE_APPLICATION_OVERLAY_CAVE_CODE,
    ISLE_APPLICATION_OVERLAY_RIVER_ASSETS,
    ISLE_APPLICATION_OVERLAY_RIVER_CODE,
    ISLE_APPLICATION_OVERLAY_VOLCANO_ASSETS,
    ISLE_APPLICATION_OVERLAY_VOLCANO_CODE,
    ISLE_APPLICATION_OVERLAY_VALLEY_ASSETS,
    ISLE_APPLICATION_OVERLAY_VALLEY_CODE,
    ISLE_APPLICATION_OVERLAY_RAINBOW_ASSETS,
    ISLE_APPLICATION_OVERLAY_RAINBOW_CODE,
} IsleApplicationOverlay;

typedef enum IsleApplicationRom {
    ISLE_APPLICATION_ROM_BEACH_EXTRA,
    ISLE_APPLICATION_ROM_MAGIKARP_TEXTURES,
    ISLE_APPLICATION_ROM_MAGIKARP_MODEL_HD,
    ISLE_APPLICATION_ROM_PIKACHU1,
    ISLE_APPLICATION_ROM_PIKACHU_MODEL_HD,
    ISLE_APPLICATION_ROM_TUNNEL_EXTRA,
    ISLE_APPLICATION_ROM_ZUBAT1,
    ISLE_APPLICATION_ROM_ZUBAT_MODEL_HD,
    ISLE_APPLICATION_ROM_CAVE_EXTRA,
    ISLE_APPLICATION_ROM_BULBASAUR1,
    ISLE_APPLICATION_ROM_BULBASAUR_MODEL_HD,
    ISLE_APPLICATION_ROM_RIVER_EXTRA,
    ISLE_APPLICATION_ROM_VOLCANO_EXTRA,
    ISLE_APPLICATION_ROM_VALLEY_EXTRA,
    ISLE_APPLICATION_ROM_RAINBOW_EXTRA,
    ISLE_APPLICATION_ROM_MAGIKARP_MODEL,
    ISLE_APPLICATION_ROM_PIKACHU_MODEL,
    ISLE_APPLICATION_ROM_ZUBAT_MODEL,
    ISLE_APPLICATION_ROM_BULBASAUR_MODEL,
} IsleApplicationRom;

typedef enum IsleApplicationVpk {
    ISLE_APPLICATION_VPK_INTRO_CODE_VPK0,
    ISLE_APPLICATION_VPK_MAIN_MENU_VPK0,
} IsleApplicationVpk;

typedef enum IsleApplicationCacheOperation {
    ISLE_APPLICATION_CACHE_WRITEBACK,
    ISLE_APPLICATION_CACHE_INVALIDATE
} IsleApplicationCacheOperation;

/* Printer memory is a real native-owned writable allocation, not a borrowed
 * console address. The pinned test touches0xffffc words and synchronizes the
 * complete0x400000-byte region. Its allocation survives the entire C test. */
typedef struct IsleApplicationPrinterMemory {
    uint32_t* words;
    size_t word_count;
    size_t byte_count;
} IsleApplicationPrinterMemory;

typedef struct IsleApplicationSource {
    IsleApplicationManagerState* state; /* Borrowed canonical application state. */
    IsleContinuationDomain* domain;     /* Independent of every sceneStorage. */
    void* owner;                        /* Retained real application backend. */
    IsleGameResult status;              /* First failure, never reset by scene exit. */
    int32_t manager_argument;
    int started;
} IsleApplicationSource;

/* Prepare a zero-initialized execution record. This never resets state or owns
 * a domain. Pass source_run directly to isle_continuation_create on the separate
 * application domain. Join/cancel that continuation before releasing any link.
 * Only run is a positive-wait entry: NEVER enclose it in player_source_call or
 * any C++ callback/RAII frame. Cancellation never returns to retired C frames. */
IsleGameResult isle_application_source_prepare(IsleApplicationSource*,
    IsleApplicationManagerState*, IsleContinuationDomain*, void* owner,
    int32_t manager_argument);
void isle_application_source_run(void* source);

/* Complete materialized Source API. Simple state accessors borrow the same
 * record; algorithmic helpers and manager require the active compiled-C domain.
 * These are not alternate native dispatchers. */
void isle_application_setIdleScript(IsleApplicationSource*, const IslePlayerIdleScript*);
const IslePlayerIdleScript* isle_application_getIdleScript(IsleApplicationSource*);
void isle_application_func_8009A8D8(IsleApplicationSource*, int32_t);
int32_t isle_application_func_8009A8E4(IsleApplicationSource*);
void isle_application_func_8009A8F0(IsleApplicationSource*, int32_t level_id);
void isle_application_func_8009AE0C(IsleApplicationSource*, uint32_t scene_id);
int32_t isle_application_func_8009B2BC(IsleApplicationSource*);
void isle_application_func_8009B40C(IsleApplicationSource*);
void isle_application_func_800AAF20(IsleApplicationSource*);
void isle_application_start_scene_manager(IsleApplicationSource*, int32_t);

/* Returning native leaves ONLY. Every leaf must return before C checks its
 * status, waits, fails or invokes any Source callback. No successful placeholder
 * implementation is supplied. Resource requests retain their original order;
 * the backend resolves the named pinned resources through real native owners.
 * No leaf recursively constructs, drives, resumes or destroys a scene. */
IsleGameResult isle_application_native_reverb_custom(IsleApplicationSource*);
IsleGameResult isle_application_native_sound_global_reverb(IsleApplicationSource*, int32_t);
IsleGameResult isle_application_native_bgm_extra_reverb(IsleApplicationSource*, int32_t player, int32_t);
IsleGameResult isle_application_native_stop_bgm(IsleApplicationSource*);
IsleGameResult isle_application_native_bgm_volume(IsleApplicationSource*, int32_t player, int32_t);
IsleGameResult isle_application_native_sound_global_volume(IsleApplicationSource*, int32_t);
IsleGameResult isle_application_native_bgm_channel_volume(IsleApplicationSource*, int32_t player, int32_t channel, int32_t);
IsleGameResult isle_application_native_overlay(IsleApplicationSource*, IsleApplicationOverlay);
IsleGameResult isle_application_native_rom(IsleApplicationSource*, IsleApplicationRom);
IsleGameResult isle_application_native_vpk(IsleApplicationSource*, IsleApplicationVpk);
IsleGameResult isle_application_native_printer_available(IsleApplicationSource*, int32_t* available);
IsleGameResult isle_application_native_printer_memory(IsleApplicationSource*, IsleApplicationPrinterMemory*);
IsleGameResult isle_application_native_cache(IsleApplicationSource*, IsleApplicationCacheOperation,
    const IsleApplicationPrinterMemory*, size_t bytes);
IsleGameResult isle_application_native_check_flag(IsleApplicationSource*, int32_t flag, int32_t*);
/* Reuse the existing canonical GameSnapshot::screen_offsets decoder. */
IsleGameResult isle_application_native_saved_offsets(IsleApplicationSource*, int32_t* horizontal, int32_t* vertical);
IsleGameResult isle_application_native_sound_quality(IsleApplicationSource*, int32_t high_quality);
IsleGameResult isle_application_native_screen_offsets(IsleApplicationSource*, int32_t h_start, int32_t h_end, int32_t v_start, int32_t v_end);
IsleGameResult isle_application_native_intervals(IsleApplicationSource*, uint32_t update, uint32_t draw);
IsleGameResult isle_application_native_set_level(IsleApplicationSource*, int32_t source_level);

/* Request publishes exactly operation/argument into retained application-owned
 * intent storage and returns. It cannot complete synchronously. After resume
 * has returned, the host constructs/drives/retires the actual native scene,
 * including independent graphics/photo/PCM fences and a fresh cold per-course
 * Frame, then publishes its real signed Source result. entry_result reads that
 * same completion once; ready must be0 until real outside-owner completion.
 * Boot's ignored result and diagnostic's checked result are not scene aliases.
 * One outstanding request per manager; cancellation joins before intent storage
 * or canonical state is released, and must cancel/retire the native borrower. */
IsleGameResult isle_application_native_request_entry(IsleApplicationSource*,
    IsleApplicationEntryOperation, uint32_t argument);
IsleGameResult isle_application_native_entry_result(IsleApplicationSource*,
    int32_t* ready, int32_t* source_result);

#ifdef __cplusplus
}
#endif
#endif

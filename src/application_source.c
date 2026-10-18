#include "application_source.h"

/* Native C++ leaves have returned before any of these C-only transfers. The
 * application continuation has no dependency on a scene's Game/arena/domain. */
static IsleGameResult application_continuation_result(IsleContinuationResult result) {
    switch (result) {
        case ISLE_CONT_OK: return ISLE_GAME_OK;
        case ISLE_CONT_INVALID_ARGUMENT: return ISLE_GAME_INVALID_ARGUMENT;
        case ISLE_CONT_WRONG_OWNER: return ISLE_GAME_WRONG_OWNER;
        case ISLE_CONT_INVALID_STATE: return ISLE_GAME_INVALID_STATE;
        case ISLE_CONT_RESOURCE_FAILURE: return ISLE_GAME_EXHAUSTED;
        case ISLE_CONT_EXECUTION_FAILURE: return ISLE_GAME_EXECUTION_FAILURE;
    }
    return ISLE_GAME_EXECUTION_FAILURE;
}

static void application_checked(IsleApplicationSource* source, IsleGameResult result) {
    if (result == ISLE_GAME_OK) return;
    if (source->status == ISLE_GAME_OK) source->status = result;
    /* source_run admits only this domain's active callback. Under that
     * prerequisite fail never returns and resume reports EXECUTION_FAILURE. */
    (void)isle_continuation_fail(source->domain, "original application Source leaf failed");
}

static void application_wait(IsleApplicationSource* source) {
    IsleContinuationResult result = isle_continuation_wait(source->domain, 1);
    application_checked(source, application_continuation_result(result));
}

/* This is the ONLY blocking scene-entry seam. In particular it is not an OM
 * process in sceneStorage, an owner rendezvous, or a nonwaiting Player call. */
static int32_t application_enter(IsleApplicationSource* source,
    IsleApplicationEntryOperation operation, uint32_t argument) {
    int32_t ready = 0;
    int32_t result = 0;
    IsleGameResult requested = isle_application_native_request_entry(source, operation, argument);
    application_checked(source, requested);
    do {
        application_wait(source);
        application_checked(source,
            isle_application_native_entry_result(source, &ready, &result));
        if (ready != 0 && ready != 1)
            application_checked(source, ISLE_GAME_INVALID_STATE);
    } while (!ready);
    return result;
}

static void application_overlay(IsleApplicationSource* source, IsleApplicationOverlay overlay) {
    application_checked(source, isle_application_native_overlay(source, overlay));
}

static void application_rom(IsleApplicationSource* source, IsleApplicationRom resource) {
    application_checked(source, isle_application_native_rom(source, resource));
}

static void application_vpk(IsleApplicationSource* source, IsleApplicationVpk resource) {
    application_checked(source, isle_application_native_vpk(source, resource));
}

static void application_reverb_custom(IsleApplicationSource* source) {
    application_checked(source, isle_application_native_reverb_custom(source));
}

static void application_sound_global_reverb(IsleApplicationSource* source, int32_t value) {
    application_checked(source, isle_application_native_sound_global_reverb(source, value));
}

static void application_bgm_extra_reverb(IsleApplicationSource* source, int32_t player, int32_t value) {
    application_checked(source, isle_application_native_bgm_extra_reverb(source, player, value));
}

static void application_stop_bgm(IsleApplicationSource* source) {
    application_checked(source, isle_application_native_stop_bgm(source));
}

static void application_bgm_volume(IsleApplicationSource* source, int32_t player, int32_t value) {
    application_checked(source, isle_application_native_bgm_volume(source, player, value));
}

static void application_sound_global_volume(IsleApplicationSource* source, int32_t value) {
    application_checked(source, isle_application_native_sound_global_volume(source, value));
}

static void application_bgm_channel_volume(IsleApplicationSource* source,
    int32_t player, int32_t channel, int32_t value) {
    application_checked(source,
        isle_application_native_bgm_channel_volume(source, player, channel, value));
}

static int32_t application_printer_available(IsleApplicationSource* source) {
    int32_t available = 0;
    application_checked(source, isle_application_native_printer_available(source, &available));
    return available;
}

static IsleApplicationPrinterMemory application_printer_memory(IsleApplicationSource* source) {
    IsleApplicationPrinterMemory memory = {0};
    application_checked(source, isle_application_native_printer_memory(source, &memory));
    /* 46270.c188–189: the original end is exclusive, sixteen bytes before the
     * end of the cache-synchronized four-MiB region. Do not round the loop up. */
    if (memory.words == NULL || memory.word_count != 0xFFFFCu ||
        memory.byte_count != 0x400000u)
        application_checked(source, ISLE_GAME_INVALID_STATE);
    return memory;
}

static void application_cache(IsleApplicationSource* source,
    IsleApplicationCacheOperation operation, const IsleApplicationPrinterMemory* memory,
    size_t bytes) {
    application_checked(source, isle_application_native_cache(source, operation, memory, bytes));
}

static void application_panic(IsleApplicationSource* source) {
    application_checked(source, ISLE_GAME_EXECUTION_FAILURE);
}

static int32_t application_check_flag(IsleApplicationSource* source, int32_t flag) {
    int32_t value = 0;
    application_checked(source, isle_application_native_check_flag(source, flag, &value));
    return value;
}

static void application_saved_offsets(IsleApplicationSource* source,
    int32_t* horizontal, int32_t* vertical) {
    application_checked(source,
        isle_application_native_saved_offsets(source, horizontal, vertical));
}

static void application_sound_quality(IsleApplicationSource* source, int32_t high_quality) {
    application_checked(source, isle_application_native_sound_quality(source, high_quality));
}

static void application_screen_offsets(IsleApplicationSource* source,
    int32_t h_start, int32_t h_end, int32_t v_start, int32_t v_end) {
    application_checked(source,
        isle_application_native_screen_offsets(source, h_start, h_end, v_start, v_end));
}

static void application_intervals(IsleApplicationSource* source, uint32_t update, uint32_t draw) {
    application_checked(source, isle_application_native_intervals(source, update, draw));
}

static void application_set_level(IsleApplicationSource* source, int32_t level) {
    application_checked(source, isle_application_native_set_level(source, level));
}

/* Ten complete counted own-GAME definitions, generated exclusively into private
 * build storage. No original C or SDK layout/implementation is redistributed. */
#include "application_original.inc"

IsleGameResult isle_application_source_prepare(IsleApplicationSource* source,
    IsleApplicationManagerState* state, IsleContinuationDomain* domain, void* owner,
    int32_t manager_argument) {
    IsleContinuationResult access;
    if (source == NULL || state == NULL || domain == NULL || owner == NULL)
        return ISLE_GAME_INVALID_ARGUMENT;
    access = isle_continuation_domain_check_access(domain);
    if (access != ISLE_CONT_OK) return application_continuation_result(access);
    if (source->state != NULL || source->domain != NULL || source->owner != NULL ||
        source->started || source->status != ISLE_GAME_OK)
        return ISLE_GAME_INVALID_STATE;
    source->state = state;
    source->domain = domain;
    source->owner = owner;
    source->manager_argument = manager_argument;
    return ISLE_GAME_OK;
}

void isle_application_source_run(void* opaque) {
    IsleApplicationSource* source = opaque;
    IsleContinuationResult access;
    if (source == NULL) return;
    if (source->state == NULL || source->domain == NULL || source->owner == NULL ||
        source->started || source->status != ISLE_GAME_OK) {
        if (source->status == ISLE_GAME_OK) source->status = ISLE_GAME_INVALID_STATE;
        return;
    }
    /* wait(0) is a side-effect-free active-callback admission, not a positive
     * wait or manufactured scene completion. Native callers cannot run the
     * manager inline from C++: they receive an explicit retained failure. */
    access = isle_continuation_wait(source->domain, 0);
    if (access != ISLE_CONT_OK) {
        source->status = application_continuation_result(access);
        return;
    }
    source->started = 1;
    isle_application_start_scene_manager(source, source->manager_argument);
}

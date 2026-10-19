#ifndef ISLE_CREDITS_SCENE_NATIVE_HPP
#define ISLE_CREDITS_SCENE_NATIVE_HPP
#include "credits_source.h"
#include "credits_worker_native.hpp"
#include <array>
#include <cstdint>
#include <memory>

namespace isle::application { class Photos; }
namespace isle::scene::graphics { class SourceControl; }
namespace isle::post_course { struct CreditsSceneNative; }

/* A cursor is valid only during the Parent's actual pre-render dispatch. Its
 * metadata names the real, still-owned Native allocation, not a guest Gfx or a
 * process-global scene. The second entry is the one-command terminal cursor. */
struct IsleCreditsDisplayListCursor {
    void* allocation = nullptr;
    IsleCreditsSource* source = nullptr;
    isle::post_course::CreditsSceneNative* state = nullptr;
    std::uint64_t generation = 0;
    isle::scene::graphics::SourceControl* control = nullptr;
};

namespace isle::post_course {
struct CreditsSceneNative final {
    explicit CreditsSceneNative(application::Photos&) noexcept;
    ~CreditsSceneNative();
    CreditsSceneNative(const CreditsSceneNative&) = delete;
    CreditsSceneNative& operator=(const CreditsSceneNative&) = delete;

    IsleCreditsSource* source = nullptr;
    std::unique_ptr<IsleCreditsWorker> worker;
    IsleGameObject* preview_object = nullptr;
    IsleGameCamera* preview_camera = nullptr;
    IsleGameObject* clear_object = nullptr;
    IsleGameCamera* clear_camera = nullptr;
    IsleGameObject* sprite_object = nullptr;
    IsleGameCamera* sprite_camera = nullptr;
    std::array<IsleCreditsDisplayListCursor,2> display{};
    IsleCreditsDisplayListCursor* display_current = nullptr;
    std::size_t logical_heap_bytes = 0;
    IsleGameResult failure = ISLE_GAME_OK;
    bool prepared = false;
    bool screen_applied = false;
    bool window_initialized = false;
    bool cameras_initialized = false;
    bool transition_initialized = false;
    bool cancel_requested = false;
    bool native_drained = false;
    bool display_active = false;

    /* Owner-only, outside another worker pump. This resumes the existing real
     * C continuation, never another menu controller. Parent services actual
     * Frame completion first; main must not dispatch while main_excluded(). */
    IsleGameResult pump() noexcept;
    bool main_excluded() const noexcept;
    IsleGameResult request_cancel() noexcept;
    IsleGameResult join(int* done) noexcept;
    bool joined() const noexcept;

    /* Parent supplies its actual allocation generation and SourceControl only
     * for the duration of Original func_credits_801DC8A0. It validates those same
     * fields in SceneAccess::credits_display_context. End invalidates both
     * entries, including failure paths; no cursor survives GPU preparation. */
    IsleCreditsDisplayListCursor** begin_display(void* allocation,
        std::uint64_t generation,scene::graphics::SourceControl&) noexcept;
    IsleGameResult end_display() noexcept;
};
const IsleCreditsNative& credits_scene_native_table() noexcept;
/* Source initialize precedes prepare: initialize requires worker==NULL. Parent
 * has already cold-admitted Native.menu, real Game/Window and selected bank. */
IsleGameResult credits_scene_native_prepare(IsleCreditsSource*,CreditsSceneNative&) noexcept;
}
#endif

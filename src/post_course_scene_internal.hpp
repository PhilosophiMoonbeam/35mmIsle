#ifndef ISLE_POST_COURSE_SCENE_INTERNAL_HPP
#define ISLE_POST_COURSE_SCENE_INTERNAL_HPP
#include "post_course_scene.hpp"
#include "post_course_window.hpp"
#include "scene_storage.hpp"
#include <array>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <thread>
#include <variant>
#include <vector>

#include <utility>

namespace isle::post_course {
struct Scene::Impl {
    const rom::Snapshot& snapshot;
    application::Manager& manager;
    game::State& state;
    game_input::Service& input;
    game_audio::Session& audio;
    host::GtlClock& clock;
    application::Photos& photos;
    host::Progress& progress;
    ReplayNativeProgress outer_progress;
    Modules& modules;
    Resources::Kind kind;
    std::int32_t argument;
    bool require_cpu;
    const std::thread::id creator = std::this_thread::get_id();
    std::variant<std::monostate,IsleCameraCheckSource,IslePhotoCheckSource> context;
    IslePhotoCheckEntry photo_entry{};
    IsleContinuationDomain* domain = nullptr;
    IsleContinuation* entry = nullptr;
    SceneStatus phase = SceneStatus::ready;
    IsleGameResult failure = ISLE_GAME_OK;
    std::array<char,512> diagnostic{};
    bool requested = false, native_retired = false, cancelling = false;
    bool near_clipping_disabled = false;
    std::uint32_t requested_screen = 0, requested_setup = 0;
    std::array<std::uint32_t,7> screen_words{};
    std::array<std::uint32_t,35> setup_words{};
    std::int32_t output_scene = 0;

    struct Native {
        Impl& owner;
        scene::Storage storage;
        std::shared_ptr<const window::Resources> common;
        std::shared_ptr<const Resources> resources;
        std::unique_ptr<sprite::Producer> producer;
        std::unique_ptr<scene::Services> services;
        std::shared_ptr<window::Heap> heap;
        const std::size_t heap_bytes;
        std::unique_ptr<window::Text> text;
        std::unique_ptr<window::Frame> ui_frame;
        std::unique_ptr<window::Focus> focus;
        std::unique_ptr<window::Layout> layout;
        std::unique_ptr<window::Buttons> buttons;
        std::unique_ptr<Window> window;
        std::unique_ptr<scene::graphics::Frame> graphics;
        std::unique_ptr<photo::Replay> pipeline;
        std::unique_ptr<Replay> replay;
        std::array<WindowRasterSlot,4> element_slots{};
        std::vector<WindowRasterSlot> thumbnail_slots;
        std::vector<sprite::RamRange> reservations;
        std::vector<sprite::AssetSpan> immutable;
        std::array<scene::SpriteDraw,1024> draws{};
        std::array<scene::render::Pass,8> passes{};
        std::size_t pass_count = 0;
        std::vector<IsleGameObject*> source_objects;
        std::array<IsleCameraCheckInput,4> camera_inputs{};
        std::array<bool,4> camera_input_observed{};
        std::array<char,17> player_name{};
        bool activated = false, heap_admitted = false, input_initialized = false;
        bool replay_initialized = false, callbacks_joined = false, retired = false;
        Native(Impl&);
        ~Native();
        scene::render::Input render_input();
        std::uint64_t released_ordinal = 0;
        // No Source dispatch here. Registers actual native camera rendering
        // metadata; every draw refreshes flags/background from the native CObj.
        void add_camera(IsleGameObject*,IsleGameCamera*,std::uint32_t priority,
                        std::uint64_t link_mask,std::uint32_t camera_tag);
        bool retire() noexcept;
    };
    std::unique_ptr<Native> native;

    Impl(const rom::Snapshot&,application::Manager&,game::State&,game_input::Service&,
         game_audio::Session&,host::GtlClock&,application::Photos&,host::Progress&,
         ReplayNativeProgress,Modules&,Resources::Kind,std::int32_t,bool);
    ~Impl();
    IslePostCourseSceneSource& source() noexcept;
    const IslePostCourseSceneSource& source() const noexcept;
    Native& live();
    IsleGameResult request_stop() noexcept;
    IsleGameResult fail(IsleGameResult,const char*) noexcept;
    IsleGameResult prepare_entry() noexcept;
    // Returning intent publication only; original C remains on its independent
    // entry stack. Camera applies intervals1/2; Photo preserves inherited1/1.
    IsleGameResult setup_begin(std::int32_t,std::uint32_t screen,std::uint32_t setup,
                               std::uint32_t flags,int near_disabled,
                               int update_interval,int draw_interval) noexcept;
    IsleGameResult setup_poll(int*,std::int32_t*) noexcept;
    IsleGameResult initialize_replay() noexcept;
    IsleGameResult cleanup_replay() noexcept;
    IsleGameResult request_exit(std::int32_t) noexcept;
    // Actual GPU/readback/display/native service only. The outer callback owns
    // real Runtime FrameToken/Host presentation; never call Runtime::pump here.
    static IsleGameResult native_progress(void*) noexcept;
    static photo::ReplayAcquire acquire_gpu(void*,photo::ReplayGpuBinding&) noexcept;
    static bool release_gpu(void*,const photo::ReplayGpuBinding&) noexcept;
};

struct SceneAccess {
    template<class F>
    static IsleGameResult invoke(IslePostCourseSceneSource* source,F&& function) noexcept {
        if (!source || !source->owner) return ISLE_GAME_INVALID_ARGUMENT;
        auto& owner = *static_cast<Scene::Impl*>(source->owner);
        if (&owner.source() != source || owner.domain != source->domain)
            return ISLE_GAME_INVALID_STATE;
        const auto access = source->game ? isle_game_status(source->game) :
            (isle_continuation_domain_check_access(source->domain)==ISLE_CONT_OK ? ISLE_GAME_OK : ISLE_GAME_WRONG_OWNER);
        if (access != ISLE_GAME_OK) return access;
        try { return function(owner); }
        catch (const std::exception& error) { return owner.fail(ISLE_GAME_INVALID_STATE,error.what()); }
        catch (...) { return owner.fail(ISLE_GAME_INVALID_STATE,"post-course native leaf threw"); }
    }
    template<class F>
    static IsleGameResult invoke_owner(void* raw,F&& function) noexcept {
        if (!raw) return ISLE_GAME_INVALID_ARGUMENT;
        auto& owner = *static_cast<Scene::Impl*>(raw);
        return invoke(&owner.source(),std::forward<F>(function));
    }
};
}
#endif

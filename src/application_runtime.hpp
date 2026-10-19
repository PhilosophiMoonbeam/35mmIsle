#ifndef ISLE_APPLICATION_RUNTIME_HPP
#define ISLE_APPLICATION_RUNTIME_HPP
#include "application_manager.hpp"
#include "beach_lifecycle.hpp"
#include "rainbow_lifecycle.hpp"
#include <memory>

namespace isle::startup { class Runtime; }
namespace isle::new_game { class Scene; }
namespace isle::oaks_lab { class Scene; }
namespace isle::post_course { class Scene; struct ReplayNativeProgress; }
namespace isle::host { class Progress; }
namespace isle::course { template<class Adapter> class Lifecycle; }
namespace isle::tunnel { struct Adapter; }
namespace isle::volcano { struct Adapter; }
namespace isle::river { struct Adapter; }
namespace isle::cave { struct Adapter; }
namespace isle::valley { struct Adapter; }
namespace isle::application {
struct RuntimeCreateResult;
struct SceneRetirementView {
    const EntryIntent* entry = nullptr;
    std::int32_t source_result = 0;
    const beach::Lifecycle* beach = nullptr;
    const post_course::Scene* post = nullptr;
    const rainbow::Lifecycle* rainbow = nullptr;
    const course::Lifecycle<tunnel::Adapter>* tunnel = nullptr;
    const course::Lifecycle<volcano::Adapter>* volcano = nullptr;
    const course::Lifecycle<river::Adapter>* river = nullptr;
    const course::Lifecycle<cave::Adapter>* cave = nullptr;
    const course::Lifecycle<valley::Adapter>* valley = nullptr;
};
struct RuntimeConfig {
    bool require_cpu_renderer = false;
    // Outside traversal, after actual scene retirement/result, before owner
    // release and Manager acknowledgement. Borrowed observation only; never
    // retain pointers, mutate Source or reenter Runtime. Not called on cancel.
    void* retirement_user = nullptr;
    void (*scene_retired)(void*, SceneRetirementView) = nullptr;
};
// Borrowed owner-phase inspection. Pointers expire at the next pump/retirement;
// no preparation, GPU, Photo or native identity credit is transferred here.
struct RuntimeView {
    const EntryIntent* entry = nullptr;
    const startup::Runtime* startup = nullptr;
    const new_game::Scene* name = nullptr;
    const oaks_lab::Scene* oak = nullptr;
    const beach::Lifecycle* beach = nullptr;
    const post_course::Scene* post = nullptr;
    const rainbow::Lifecycle* rainbow = nullptr;
    const course::Lifecycle<tunnel::Adapter>* tunnel = nullptr;
    const course::Lifecycle<volcano::Adapter>* volcano = nullptr;
    const course::Lifecycle<river::Adapter>* river = nullptr;
    const course::Lifecycle<cave::Adapter>* cave = nullptr;
    const course::Lifecycle<valley::Adapter>* valley = nullptr;
};
// Application scheduling/ownership only. Original C Manager chooses every
// entry/result; the actual scene owners implement their native Source bodies.
// Dependencies outlive Runtime. Main independently acquires real input, supplies
// retraces to Input/Audio/GtlClock and pumps Audio/events/presentation throughout
// graphics, input backpressure and scene retirement.
class Runtime final {
public:
    static RuntimeCreateResult create(const rom::Snapshot&, const menu::Catalog&,
        game::State&, game_input::Service&, game_audio::Session&, host::GtlClock&,
        RuntimeConfig = {}) noexcept;
    ~Runtime();
    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;
    // One bounded host iteration. No manufactured retrace, blocking device read,
    // audio task or presentation pacing is hidden here. False retains failure
    // while the caller continues pumping actual shutdown obligations.
    bool pump(std::uint32_t raw_game_time) noexcept;
    // Forward the actual Service::complete_read acknowledgement, including when
    // a Source update is waiting for its real acquisition/publication.
    IsleGameResult acknowledge_input(const game_input::EventResult&) noexcept;
    // Bind once before first pump. Shared real Host progress and display/native
    // service outlive Runtime; the service never pumps Source, OM or GTL.
    IsleGameResult bind_post_course_progress(host::Progress&, post_course::ReplayNativeProgress) noexcept;
    scene::graphics::Completed completed_frame() const noexcept;
    // After the caller synchronously copies or declines the optional display.
    // Closes caller image access and offers the real GTL terminal. Completed
    // CPU Photo/history remains live for original CopyInfo/shutter updates
    // until the next actual draw preparation or scene retirement.
    IsleGameResult release_completed() noexcept;
    // Cancellation joins the actual Frame/course/Replay/Window/native/entry
    // owners. A Source STOPPED abandons the current GTL update and never
    // acknowledges an entry result or invokes scene_retired; faults set error().
    void request_stop() noexcept;
    bool stopped() const noexcept;
    bool scene_active() const noexcept;
    IsleGameResult error() const noexcept;
    const char* message() const noexcept;
    RuntimeView view() const noexcept;
private:
    struct Impl;
    explicit Runtime(std::unique_ptr<Impl>) noexcept;
    std::unique_ptr<Impl> impl_;
};
struct RuntimeCreateResult {
    IsleGameResult error = ISLE_GAME_OK;
    std::unique_ptr<Runtime> runtime;
    std::array<char,512> message{};
    explicit operator bool() const noexcept { return error == ISLE_GAME_OK && bool(runtime); }
};
}
#endif

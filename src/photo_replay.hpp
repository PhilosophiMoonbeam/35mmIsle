#ifndef ISLE_PHOTO_REPLAY_HPP
#define ISLE_PHOTO_REPLAY_HPP

#include "photo_world.hpp"
#include "world_producer.hpp"
#include "borrowed_model_producer.hpp"
#include "graphics_task.hpp"
#include "rsp_render.hpp"
#include "player_photo_source.h"
#include "35mmisle/photo_score.hpp"
#include "scene_graphics.hpp"

namespace isle::photo {
// The Source owner, not Replay, issues and retains this capability. Every field
// of data (including inactive records) and the revision must stay unchanged until
// acknowledge_result or completed cancellation. Revision alone is not a fence.
struct RecordCapability {
    const IslePlayerPhotoData* data = nullptr;
    const std::uint64_t* revision = nullptr;
    std::uint64_t expected_revision = 0;
};
// Every admitted preview or score binding names its actual owning Frame.
// Null Frame is only an uninitialized/pending output, never executor-only access.
struct ReplayGpuBinding {
    rsp::CanonicalExecutor* executor = nullptr;
    const graphics::SourcePrograms* programs = nullptr;
    std::uint64_t epoch = 0, admission = 0;
    scene::graphics::Frame* frame = nullptr;
};
enum class ReplayAcquire { pending, admitted, failed };
struct ReplayGpuAccess {
    void* context = nullptr;
    ReplayAcquire (*acquire)(void*, ReplayGpuBinding&) noexcept = nullptr;
    bool (*release)(void*, const ReplayGpuBinding&) noexcept = nullptr;
};
// Explicit retained physical storage, disjoint from the ordinary display's
// reservations and every immutable resource. These reservations stay protected
// across Source waits; the executor itself is borrowed ONLY per GPU stage.
// workspace: stack[0,4000), yield[4000,6000), task[6000,7000), boot[7000,7100).
// commands begins with a 0x400-byte reset/root/viewport payload reservation.
// Source storage aliases intentionally: feedback={color.offset,0x27000},
// color={base,0x20d00}, depth={base+0x20d00,0x20d00}. Score overwrites color and
// the first 0x6300 depth bytes. reservations() exports their one 0x41a00 union;
// do not plan three independent buffers or restore an older preview afterward.
struct ReplayLayout {
    menu::RamRange commands{}, workspace{}, output{};
    world::ProducerLayout world{};
    menu::model::PacketLayout subjects{}, items{};
    menu::RamRange feedback{}, preview_color{}, preview_depth{}, effects{}, effect_assets{};
};
struct ReplayConfig {
    ReplayLayout memory;
    // Ordinary display targets/workspace/producers, excluding cold main/BSS.
    // Copied cold; caller need not retain the span's storage.
    std::span<const menu::RamRange> external_reservations;
    graphics::SourceProgram program = graphics::SourceProgram::f3dex2_non_fifo;
    // Source's actual D_803A6660/64 backing buffer dimensions, not a crop size.
    std::uint32_t buffer_width = 320, buffer_height = 210;
    std::uint32_t dl_link = 19, world_object_link = 18, photo_object_link = 17;
    // Actual application-lifetime GAME color bytes, when this component is
    // hosted by the application. Standalone GPU consumers own no such state.
    std::array<std::uint8_t,3>* background_rgb = nullptr;
};
enum class ReplayKind { score, preview };
struct ReplayRequest {
    ReplayKind kind = ReplayKind::score;
    RecordCapability record;
    bool has_dash_engine = false;
    // window/847B60.c73-95 clamps the requested viewport, never resamples it.
    std::int32_t width = 320, height = 210;
    float preview_scale = 1;
};
enum class ReplaySignal {
    idle, pending, play_sound85, wait60, wait1, result, cancelled, failed
};
struct ReplayStep {
    ReplaySignal signal = ReplaySignal::idle;
    // An operation-local obligation identity, never a GTL/GPU completion token.
    std::uint64_t obligation = 0;
};
struct ReplayResult {
    ReplayKind kind = ReplayKind::score;
    const photo_score::Result* score = nullptr;
    // Canonical big-endian RGBA5551 bytes, copied ONLY from actually completed
    // RDRAM. Includes the backing stride; no screenshot/RGBA8 conversion.
    // Result borrows expire on acknowledgment, cancellation or the next begin.
    std::span<const std::byte> rgba5551;
    std::uint32_t width = 0, height = 0, stride_bytes = 0;
};
// Actual natural score_Render boundary evidence, copied from the shared Frame
// after Original's post10 returned and restored the native camera fields.
struct ScoreExecution {
    scene::graphics::TaskReceipt task;
    bool wide = false, source_returned = false, camera_restored = false;
    std::uint16_t pokemon_count = 0;
};
// The same authoritative Game. The admitted complete score_Render C producer
// dispatches only inside its plain-C fence and the borrowed Frame transaction;
// ordinary OM/GTL ticks, Source waits and gameplay AI are never dispatched here.
// Preview borrows the same quiescent executor; score uses its exact Frame's
// SourceControl submit/drain path. Both release only after mapped read retirement.
// Snapshot, Game, world::ModuleState and access.context outlive Replay, including
// all GPU/readback and native cleanup. Destruction requires retire()==true:
// silently destroying native callback storage is illegal.
struct ReplayWorldHandoff {
    // Immutable Resources may also be retained by admitted cold record caches.
    // The sole World still moves without moving its native callback storage;
    // member order destroys it before releasing its resource hold.
    std::shared_ptr<world::Resources> resources;
    std::unique_ptr<World> world;
};
class Replay final {
public:
    Replay(IsleGame*, const rom::Snapshot&, world::ModuleState&, ReplayGpuAccess, ReplayConfig);
    // Actual Source80374D40 creates this same owned camera before any record.
    // Its ordinary display callback remains disabled outside real Replay work.
    bool initialize_preview_camera(std::uint32_t background_rgba,
        IsleGameObject**,IsleGameCamera**) noexcept;
    ~Replay() noexcept;
    Replay(const Replay&) = delete;
    Replay& operator=(const Replay&) = delete;
    bool begin(const ReplayRequest&) noexcept;
    ReplayStep poll() noexcept;
    // Caller acknowledges sound only after the actual Source audio call, and
    // waits only after the original C ohWait returns. Never call Source under
    // this C++ frame. Wrong/stale acknowledgments are sticky failures.
    bool acknowledge(std::uint64_t obligation) noexcept;
    ReplayResult result() const noexcept;
    bool acknowledge_result() noexcept;
    void cancel() noexcept;
    // Nonblocking true native retirement. Call again after genuine native join
    // progress if false; poll continues draining a submitted GPU on cancellation.
    bool retire() noexcept;
    // Returning native half of Source 8009FC38; caller owns real prior-GFX
    // join and canonical record lease. Only idle requests may prepare a course.
    // False is pending native retirement unless error() is nonempty.
    bool prepare_course(std::int32_t level) noexcept;
    // Closed mechanical ownership handoff to the one retained Presentation.
    // Original thumbnail and full-photo paths must never own duplicate Worlds.
    ReplayAcquire take_presentation_world(IsleGame*, const void* borrower,
        std::int32_t level, std::uint32_t dl_link, std::uint32_t world_link,
        std::uint32_t photo_link, ReplayWorldHandoff&) noexcept;
    bool return_presentation_world(const void* borrower, ReplayWorldHandoff&&) noexcept;
    bool busy() const noexcept;
    bool gpu_busy() const noexcept;
    const char* error() const noexcept;
    std::span<const menu::RamRange> reservations() const noexcept;
    std::span<const ScoreExecution> score_executions() const noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace isle::photo
#endif

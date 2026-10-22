#ifndef ISLE_SCENE_GRAPHICS_HPP
#define ISLE_SCENE_GRAPHICS_HPP

#include "startup_runtime.hpp"
#include "rsp_render.hpp"
#include "scene_render.hpp"

namespace isle::graphics { class SourcePrograms; enum class SourceProgram : std::uint32_t; }
namespace isle::scene::graphics {
enum class Error { none, invalid, resource, native, preparation, execution, retired };
enum class Poll { idle, pending, completed, failed };
// Physical task ownership, independent of immutable course banks. Commands
// reserve their first 0x100 bytes for the generic task reset prefix and final
// 0x400 for identity/viewport payloads. Workspace contains stack, yield, task
// and boot at offsets 0/0x4000/0x6000/0x7000.
struct Layout {
    sprite::RamRange commands{0x100000, 0x10000};
    sprite::RamRange workspace{0x280000, 0x10000};
    sprite::RamRange output{0x200000, 0x40000};
    std::uint32_t depth = 0x600000;
};
struct Config {
    bool require_cpu = false;
    rsp::Limits limits{12000000, 4 * 1024 * 1024};
    Layout memory{};
    // Retained transition packets are relocatable runtime data, not Source ROM.
    // Snapshot composition may relocate; the Catalog adapter keeps its plan.
    sprite::RamRange transition_memory{0x1f0000,0x10000};
};
// Native source-screen semantics, not a console VI/ScreenSettings layout.
// The supported 0xa99 mode scans two 320x240 RGBA16 source buffers. Guest
// identities are preserved here; only their validated physical spans reach RDP.
struct SourceScreen {
    std::uint32_t width = 320, height = 240, flags = 0xa99;
    std::array<std::uint32_t, 3> framebuffer_identity{0x803b5000, 0x803da800, 0};
};
// A capability naming this Frame's actual protected RDRAM depth storage.
// Never a host pixel allocation, a GPU completion token or a console pointer.
// Its address remains stable until Frame destruction; copies are not admitted.
struct DepthReservation {
    sprite::RamRange memory;
    std::uint32_t alignment;
};
// The two actual source RGBA16 contexts; this is not a presentation image index.
struct FrameContext { unsigned buffer_index = 0; };
struct Completed {
    std::span<const std::byte> rgba;
    std::uint32_t width = 320, height = 240, stride = 1280;
    save::ScreenOffsets screen_offsets{};
    std::uint64_t ordinal = 0;
    explicit operator bool() const noexcept { return !rgba.empty(); }
};
// Invoked once under the actual completed GPU mapping, before RGBA8 copying.
// Context outlives poll completion. Never release/move the lease or retain its
// spans. A writer must finish_postprocess before return; abandonment poisons.
struct CompletedWork {
    void* context = nullptr;
    void (*function)(void*, rsp::CanonicalExecutor::MemoryLease&, std::uint32_t framebuffer) = nullptr;
};
// Cold upper bounds for source-controlled composition. The pass-order storage
// is already fixed at 32; each pass may add one sprite-command reservation.
struct RenderCapacity {
    std::size_t sprite_draws = 0;
    std::size_t resource_reservations = 0;
    // Total retained real receipts and Source descriptor slots between resets.
    // Empty processing consumes neither. Both are cold, bounded admission limits.
    std::size_t task_receipts = 33;
    std::size_t source_task_slots = 33;
};
struct CreateResult;
// Synchronous nonowning source capability. Never retain it past its control
// callback; a temporary borrower must restore its guard before normal return.
// RAM is available only inside returning scoped writer callbacks, never across
// a task boundary or after the control callback returns.
// C callers enter through returning C++ leaves and a nested plain-C fence;
// no checked C transfer may cross these command/preparation frames.
class SourceControl final {
public:
    SourceControl(const SourceControl&) = delete;
    SourceControl& operator=(const SourceControl&) = delete;
    void with_ram(void*, void (*)(void*, std::span<std::uint32_t>));
    std::uint64_t epoch() const noexcept;
    std::uint32_t framebuffer() const noexcept;
    std::uint32_t depth() const noexcept;
    std::uint32_t camera_flags() const noexcept;
    void command(std::uint32_t, std::uint32_t);
    void call(render::List);
    // Emit the same genuine reset body as the default renderer into a caller's
    // bounded native cursor, using this actual Frame's targets and payloads.
    // The sink returns normally; it must not perform checked C transfers.
    void reset_program_state(void*, void (*)(void*, std::uint32_t, std::uint32_t)) const;
    // Emits only current native camera/viewport/scissor/flags/targets. No
    // producer, object or prefix preparation occurs. At most once per dispatch.
    void initialize_camera();
    // Restore a populated secondary lane into the caller's bounded cursor.
    // Reuses the current native viewport/targets and camera encoder, suppressing
    // depth/background clears. Returns the actual advanced physical cursor.
    std::uint32_t restore_camera(unsigned lane,std::uint32_t cursor,std::uint32_t end);
    std::uint32_t initialize_camera_into(std::uint32_t cursor,std::uint32_t end);
    std::uint32_t draw_into(std::uint32_t cursor,std::uint32_t end);
    // Cold admission check before a caller emits task roots or terminal tails.
    // Does not consume a slot; submit_task performs the same check and consumes.
    void reserve_task();
    // Publish one actual task after every scoped writer has returned. Submission
    // does not reset Source cursors/allocators. The next writer joins the GPU.
    void submit_task(render::List root, render::List terminal,
                     ::isle::graphics::SourceProgram);
    // Actual completion before a Source reset; no synthetic reset-only task.
    void drain_tasks();
    // Reset Source task accounting only after drain, alongside allocator reset.
    void reset_task_slots();
    // Generic callback-owned outer commands use the same actual task boundary.
    // A reset joins completion before resetting Source task accounting.
    void process_commands(bool reset);
    // Compiled passes only: prepares real producer lists without emitting any
    // camera or lane commands. The borrow expires with this control callback.
    // At most once; cannot mix either explicit phase with draw().
    const render::Prepared& prepare();
    // Initializes/prepares/draws the current pass at most once, reading its
    // actual camera and sprite views now, not before preceding source dispatch.
    void draw();
private:
    struct State;
    explicit SourceControl(State& state) noexcept : state_(&state) {}
    State* state_;
    friend struct SourceControlAccess;
};
enum class SourceGpuAcquire { pending, admitted, failed };
struct SourceGpuBinding {
    rsp::CanonicalExecutor* executor = nullptr;
    const ::isle::graphics::SourcePrograms* programs = nullptr;
    std::uint64_t epoch = 0, admission = 0;
};
struct TaskReceipt {
    // Zero token with failed=true records a synchronous RSP attempt rejected
    // before GPU acceptance, never a manufactured submitted identity.
    std::uint64_t token = 0;
    ::isle::graphics::SourceProgram program{};
    std::uint32_t root = 0, terminal = 0;
    bool completed = false, failed = false;
    // Copied scalar evidence from an actual returned RSP attempt, including its
    // failure. GPU completion remains separate; no RAM mapping is implied.
    rsp::Result execution{};
    bool has_execution = false;
    // Actual task fields captured before the shared descriptor workspace is
    // reused. This is the native transport contract, never an SDK layout.
    rsp::Task task{};
};

// Owner-thread source compositor. Catalog and unmoved Snapshot outlive Frame.
// begin borrows View through synchronous logical dispatch. Each scoped writer
// ends before real task submission; the GPU receives only task/RAM packets,
// never native views. The logical resource owners remain pinned across every
// inter-task gap. Poll until !busy() before the next runtime tick, transition,
// producer retirement or native object destruction.
class Frame final {
public:
    static CreateResult create(const rom::Snapshot&, Config = {}) noexcept;
    // Typed startup preparation adapter; the renderer/executor remains shared.
    // Reject core aliases with fixed startup producers/targets before executor
    // creation. Snapshot-only composition retains independently planned storage.
    static CreateResult create(const menu::Catalog&, const rom::Snapshot&, Config = {}) noexcept;
    ~Frame();
    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;
    // Optional explicit cold plan, before the first begin (including a failed
    // begin). Source-only callers reserve before dispatch, not by warm-up.
    // Once installed, excess input fails closed rather than growing storage.
    // Failure is retained in error()/message(), as for begin().
    bool reserve_render_capacity(RenderCapacity) noexcept;
    // Original Source depth allocation and screen application are separate
    // cold operations. A non-NULL capability names this Frame's actual target;
    // NULL explicitly preserves Source viZBuffer=NULL, not private core depth.
    const DepthReservation* reserve_source_depth(std::uint32_t bytes,
                                                std::uint32_t alignment) noexcept;
    bool configure_source_screen(const SourceScreen&, const DepthReservation*) noexcept;
    // Cold persistent Source replay storage, independent of this Frame's own
    // reservations(). Protected from normal display writes across Source waits.
    // Reserve before first begin; caller includes it in other producer plans.
    bool reserve_source_gpu_storage(std::span<const sprite::RamRange>) noexcept;
    // Cold subset of the existing Replay plan, lent only by the actual unique
    // World handoff. The retained owner must end its loan before Replay can
    // acquire the executor again. No target/workspace protection is removed.
    bool register_source_display_storage(const void* owner,
        std::span<const sprite::RamRange>) noexcept;
    bool begin_source_display_storage(const void* owner) noexcept;
    bool end_source_display_storage(const void* owner) noexcept;
    // Use this Frame's retained transition resources with the actual Services
    // owner; callers must include its immutable bytes and workspace in Input.
    render::Pass transition_pass(::isle::screen_transition::Owner&) noexcept;
    std::span<const sprite::AssetSpan> transition_immutable() const noexcept;
    sprite::RamRange transition_workspace() const noexcept;
    // One exclusive real executor/RAM stage, never a display/GTL token.
    // Release only after actual GPU, readback and mapped-RAM leases have ended.
    // Retained semantic replay ownership must not hold this across Source waits.
    SourceGpuAcquire acquire_source_gpu(SourceGpuBinding&) noexcept;
    bool release_source_gpu(const SourceGpuBinding&) noexcept;
    // Exact admitted Replay/score owner only. Runs a returning Source callback
    // through the same task boundary/lease machinery, synchronously closes its
    // final real result, and invokes work once. No display image, GTL token,
    // presentation ordinal or framebuffer/FIFO completion is manufactured.
    bool run_source_control(const SourceGpuBinding&, void*,
        bool (*)(void*,SourceControl&), CompletedWork = {}) noexcept;
    bool begin(render::Input, FrameContext = {}, CompletedWork = {}) noexcept;
    bool begin(startup::View, FrameContext = {}) noexcept;
    // Returning owner-side failure latch, never a GPU cancellation or terminal.
    // Covers admission/consumer failures after a real task was submitted. The
    // owner must still poll/drain; successful presentation is suppressed.
    bool fail_draw(Error, const char*) noexcept;
    Poll poll() noexcept;
    bool busy() const noexcept;
    bool quiescent() const noexcept;
    // Last real task while the logical draw remains live, not a busy predicate.
    std::uint64_t submission() const noexcept;
    // Stable actual final-task identity until next begin, including already
    // drained flag10 tasks. It is independent of presentation ordinal/FIFO IDs.
    std::uint64_t presentation_submission() const noexcept;
    std::span<const TaskReceipt> task_receipts() const noexcept;
    // Dispatch authority, including CPU-only gaps after a drained Source10.
    // Neither a GPU token nor permission to retain a mapped RAM span.
    bool preparing() const noexcept;
    unsigned submission_context() const noexcept;
    // A submitted task belongs to this failed logical draw only after all real
    // execution/readback/writer leases have drained. Earlier successful tasks
    // remain independently truthful completed receipts.
    bool failed_submission(std::uint64_t) const noexcept;
    // Actual protected cold/task/command/output/depth ranges and, after explicit
    // source-screen setup, its two physical framebuffer spans. Not a GPU fence.
    std::span<const sprite::RamRange> reservations() const noexcept;
    Error error() const noexcept;
    const char* message() const noexcept;
    // Copied opaque RGBA8, never a RAM/native borrow. Expires at next begin or
    // retire; a host that presents asynchronously must copy before returning.
    Completed completed() const noexcept;
    bool retire() noexcept; // Rejects outstanding real execution/readback.
private:
    struct Impl;
    explicit Frame(std::unique_ptr<Impl>) noexcept;
    std::unique_ptr<Impl> impl_;
};
struct CreateResult {
    Error error = Error::none;
    std::unique_ptr<Frame> frame;
    std::array<char, 512> message{};
    explicit operator bool() const noexcept { return error == Error::none && bool(frame); }
};
} // namespace isle::scene::graphics
#endif

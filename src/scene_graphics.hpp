#ifndef ISLE_SCENE_GRAPHICS_HPP
#define ISLE_SCENE_GRAPHICS_HPP

#include "startup_runtime.hpp"
#include "rsp_render.hpp"
#include "scene_render.hpp"

namespace isle::scene::graphics {
enum class Error { none, invalid, resource, native, preparation, execution, retired };
enum class Poll { idle, pending, completed, failed };
// Physical task ownership, independent of immutable course banks. Commands
// reserve their final 0x400 bytes for identity/viewport payloads. Workspace
// contains stack, yield, task and boot at offsets 0/0x4000/0x6000/0x7000.
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
struct CreateResult;
// Owner-thread source compositor. Catalog and unmoved Snapshot outlive Frame.
// begin borrows View ONLY while preparing actual RAM. No native/producer view is
// retained during execution. The owner must poll until !busy() before the next
// Runtime tick, transition, producer retirement, or native object destruction.
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
    bool begin(render::Input, FrameContext = {}, CompletedWork = {}) noexcept;
    bool begin(startup::View, FrameContext = {}) noexcept;
    Poll poll() noexcept;
    bool busy() const noexcept;
    bool quiescent() const noexcept;
    // Actual protected cold/task/command/output/depth ranges; not a GPU fence.
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

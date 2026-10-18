#ifndef ISLE_PHOTO_EFFECTS_RENDERER_HPP
#define ISLE_PHOTO_EFFECTS_RENDERER_HPP

#include "effect_bank_resources.hpp"
#include "effect_graphics_resources.hpp"
#include "player_photo_source.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace isle::photo_effects {
enum class Error { none, course, resource, extent, arithmetic, storage, capacity, borrowed };
struct Status {
    Error error = Error::none;
    std::size_t row = menu::no_index;
    std::uint32_t address = 0;
    explicit operator bool() const noexcept { return error == Error::none; }
};
// Numeric canonical-RDRAM words, as in scene::render::List; includes ENDDL.
struct List { std::uint32_t address = 0; std::size_t words = 0; };
struct Storage {
    std::span<std::uint32_t> ram;
    sprite::RamRange commands;
    // Nonzero caller-owned RAM installation identity, NOT a GPU capability.
    std::uint64_t epoch = 0;
    // Additional caller-owned targets/workspaces/lists which commands may not
    // overwrite. Immutable FX reservations are checked independently.
    std::span<const sprite::RamRange> protected_ranges;
};
class Renderer;
// This is a CPU/storage borrow, never proof of submission or completion. The
// actual Replay request must retain it, Renderer, canonical RAM, immutable FX
// installation and both resource owners through real GPU retirement. Destroy
// only once no CPU/GPU consumer can access either list or its texture payloads.
class Packet final {
public:
    Packet() noexcept = default;
    ~Packet();
    Packet(Packet&&) noexcept;
    Packet& operator=(Packet&&) noexcept;
    Packet(const Packet&) = delete;
    Packet& operator=(const Packet&) = delete;
    explicit operator bool() const noexcept { return owner_ != nullptr; }
    List setup() const noexcept { return setup_; }
    List body() const noexcept { return body_; }
    std::span<const std::uint32_t> ram() const noexcept { return ram_; }
    std::uint64_t epoch() const noexcept { return epoch_; }
private:
    friend class Renderer;
    Packet(Renderer&, Storage, List, List) noexcept;
    void release() noexcept;
    Renderer* owner_ = nullptr;
    std::span<const std::uint32_t> ram_;
    std::uint64_t epoch_ = 0;
    List setup_, body_;
};
struct Result {
    Status status;
    // Exact total capacity, including the two ENDDL commands. On capacity
    // rejection these and effects remain available, and RAM is unchanged.
    std::size_t words = 0, effects = 0;
    Packet packet;
};

// Own GAME df8815e75f3d79618466e01e0827c6165b93d11c:
// app_render/47380.c:1345-1652, course colors:965-1035, capture:663-711.
// No live particle pool, scene/Application owner, callback, submission or wait.
// Cold construction admits exactly common(slot3) plus this course(slot0), with
// immutable source-byte provenance checked against EffectGraphicsResources.
// Admitted formats: I4/I8, IA8, CI4 and RGBA16/RGBA32. CI4 always selects
// palette0: its 16 reachable RGBA16 entries are loaded without Source's
// 512-byte adjacent-palette read. This legalization makes no equivalence claim
// for CI8, other palette selectors or any unadmitted image state.
// Snapshot, both resource owners and their views must remain live, unmoved and
// immutable through Renderer and all Packet lifetimes. Install the existing
// graphics installation_spans() into the supplied canonical RAM beforehand;
// the caller's exclusive RAM/epoch lease keeps those reservations immutable.
class Renderer final {
public:
    static constexpr std::size_t effect_count = 32;
    // Setup9+END, body setup8, 32*(TLUT6+LUT1+load7+prim1+combine1+
    // blend1+alpha1+depth1+rect3), restore4+END, in two-word commands.
    static constexpr std::size_t max_words = 1454;
    Renderer(const menu::EffectResources&, const menu::EffectGraphicsResources&,
             std::int32_t course) noexcept;
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;
    Status status() const noexcept { return status_; }
    bool borrowed() const noexcept { return borrowed_; }
    // camera.projection is the exact GAME row-vector lookAt * fast-perspective
    // matrix; viewport values are raw quarter-pixel vscale/vtrans (Y is negated
    // here). The recorded recipe always depth-tests and ignores live-FX list
    // masks/depth_test. Existing framebuffer, depth and scissor stay installed.
    // Parent must preserve Source's ordering boundary: call packet.setup(),
    // perform the real combine-all-display-lists ordering, then call body().
    // Each range is independently callable. Do not execute setup and body as
    // one flat list: setup ends at Source's gtlCombineAllDLists (line1404).
    // All validation/projection/counting precedes writes; rejection changes no
    // canonical word and acquires no borrow. Bounded, allocation-free leaf.
    Result prepare(const IslePlayerPhotoData&, const menu::EffectCamera&, Storage) noexcept;
    Result prepare(std::span<const IslePlayerEffectPhoto, effect_count>,
                   const menu::EffectCamera&, Storage) noexcept;
private:
    friend class Packet;
    Status validate_bindings() const noexcept;
    std::array<const menu::EffectBank*, 8> banks_{};
    std::array<const menu::EffectBankBinding*, 8> bindings_{};
    std::span<const sprite::AssetSpan> installation_;
    std::span<const menu::RamRange> reservations_;
    std::int32_t course_ = -1;
    std::uint32_t environment_ = 0;
    Status status_;
    bool borrowed_ = false;
};
} // namespace isle::photo_effects
#endif

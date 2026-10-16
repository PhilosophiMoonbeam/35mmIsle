#ifndef ISLE_MENU_MODEL_PRODUCER_HPP
#define ISLE_MENU_MODEL_PRODUCER_HPP

#include "borrowed_model_kernel.hpp"

namespace isle::menu::model {
struct Layout {
    RamRange payloads{0x110000, 0x10000};
    RamRange lists{0x120000, 0x10000};
    RamRange materials{0x130000, 0x10000};
};
struct Frame {
    ListRange camera; // Projection LOAD, normalization, kind8 projection MUL. No LookAt lights.
    // Four GAME gMainGfxPos lanes, each END-terminated. Each lane contains all
    // five renderlink4 objects in source creation order D/J/D/I/B (same priority
    // inserts AFTER existing objects). Parent invokes camera before each lane;
    // its target/scissor/identity/pre-render state remains the parent's concern.
    std::array<ListRange, 4> lanes{};
    std::span<const RamRange> reservations;
    std::uint64_t generation = 0;
};
struct PrepareResult {
    Status status;
    Frame frame;
    explicit operator bool() const noexcept { return static_cast<bool>(status); }
};
struct CreateResult;
struct Binding {
    animation::InstanceHandle instance;
    IsleGameObject* object = nullptr;
    IsleGameModel* model = nullptr;
};
// Title-only native attachment helper. The scene owns the returned model and
// removes it with its real object lifetime, after graphics/effect retirement.
Status attach_title_model(IsleGame*, IsleGameObject*, const Model&,
                          animation::ModelState, IsleGameModel**) noexcept;
// Title facade over the borrowed kernel. Bindings are actual source render
// order; neither this facade nor the kernel owns native attachment lifetime.
class Producer final {
public:
    static CreateResult create(IsleGame*, IsleGameCamera*, const Catalog&,
        const animation::Runtime&, std::span<const Binding> bindings,
        Layout = {}, std::span<const RamRange> other_reservations = {}) noexcept;
    ~Producer();
    Producer(const Producer&) = delete;
    Producer& operator=(const Producer&) = delete;
    Producer(Producer&&) = delete;
    Producer& operator=(Producer&&) = delete;
    // Caller holds exclusive preparation/reuse access: no outstanding GPU RAM
    // borrow, native mutation, callbacks, or concurrent Runtime step. A new RAM
    // installation/replacement requires a new nonzero epoch. Resources must
    // already be installed; this never copies a cold image or immutable assets.
    // Rejection changes neither RAM nor producer-published native/cache/world
    // values. Independently accepted engine placement setters are not rolled back.
    PrepareResult prepare(std::span<std::uint32_t> word_swapped_rdram,
                          std::uint64_t exclusive_epoch) noexcept;
    // Update the native pose/world attachment view WITHOUT writing RAM, for
    // ordered opcode16 delivery before subsequent Runtime updates. Same owner
    // exclusion as prepare. Uses actual source floating RTS, never fixed decode.
    Status synchronize() noexcept;
    std::span<const WorldMatrix> world_nodes(animation::InstanceHandle) const noexcept;
    std::span<const IsleGameModelNode> native_nodes(animation::InstanceHandle) const noexcept;
    IsleGameModel* native_model(animation::InstanceHandle) const noexcept;
    std::span<const RamRange> reservations() const noexcept;
    void retire() noexcept; // After GPU retirement; subsequent prepares reject.
private:
    struct Impl;
    explicit Producer(std::unique_ptr<Impl>) noexcept;
    std::unique_ptr<Impl> impl_;
};
struct CreateResult {
    Status status;
    std::unique_ptr<Producer> producer;
    explicit operator bool() const noexcept { return static_cast<bool>(status); }
};
} // namespace isle::menu::model
#endif

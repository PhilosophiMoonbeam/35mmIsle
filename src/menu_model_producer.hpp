#ifndef ISLE_MENU_MODEL_PRODUCER_HPP
#define ISLE_MENU_MODEL_PRODUCER_HPP

#include "35mmisle/engine.h"
#include "menu_animation.hpp"

namespace isle::menu::model {
struct Command { std::uint32_t w0 = 0, w1 = 0; };
struct ListRange { std::uint32_t address = 0, commands = 0; };
struct WorldMatrix { float m[4][4]{}; };
struct Layout {
    RamRange payloads{0x110000, 0x10000};
    RamRange lists{0x120000, 0x10000};
    RamRange materials{0x130000, 0x10000};
};
enum class Error { none, invalid_input, extent, reservation, relocation_required, native, allocation, retired };
struct Status {
    Error error = Error::none;
    std::uint32_t address = 0;
    explicit operator bool() const noexcept { return error == Error::none; }
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
// Exclusive native scene owner. Caller-created objects correspond to Catalog
// model order and must be empty. Owns their attached models, NOT the objects,
// camera, game, Catalog, Runtime or RAM. All borrowed owners stay live/unmoved.
// Destructor requires GPU retirement and removes models before object deletion.
class Producer final {
public:
    static CreateResult create(IsleGame*, IsleGameCamera*, const Catalog&,
        const animation::Runtime&, std::span<IsleGameObject* const> objects,
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
    std::span<const WorldMatrix> world_nodes(std::size_t model) const noexcept;
    std::span<const IsleGameModelNode> native_nodes(std::size_t model) const noexcept;
    // Borrowed attached model; same lifetime as this producer, no synthetic node.
    IsleGameModel* native_model(std::size_t model) const noexcept;
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
// Bounded source renLoadTextures encoder, also useful for consumer boundary
// tests. No writes/publication on rejection. Branch-table addresses are supplied
// by Producer; this body ends with ENDDL and contains no native/guest pointers.
struct MaterialCommands { std::array<Command, 32> words{}; std::size_t count = 0; };
Status encode_material(const Material&, const animation::MaterialState&,
                       MaterialCommands& output) noexcept;
// Shared bounded GAME hierarchy encoder used by Producer. Buffers are scratch:
// failure may change their contents/counts, never native state or canonical RAM.
// Caller admits child/sibling spans once: valid source indices, parent-before-
// child and forward siblings, as Producer::create already does. No tree copies.
struct HierarchyInput {
    const Model& source;
    std::span<const animation::NodeState> nodes;
    std::span<const std::uint32_t> first_child, next_sibling, material_bases;
    const IsleGameModelPlacement& placement;
    const WorldMatrix& projection;
    std::uint32_t matrices = 0, placement_address = 0;
};
struct HierarchyBuffer {
    std::array<std::span<Command>,4> lanes;
    std::array<std::size_t,4>& counts;
    std::span<Command> pending;
};
Status encode_hierarchy(const HierarchyInput&, HierarchyBuffer&) noexcept;
} // namespace isle::menu::model
#endif

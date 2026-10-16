#ifndef ISLE_WORLD_PRODUCER_HPP
#define ISLE_WORLD_PRODUCER_HPP
#include "world_runtime.hpp"
#include "borrowed_model_kernel.hpp"
#include "scene_render.hpp"

namespace isle::world {
struct ProducerLayout {
    menu::RamRange payloads{0x170000,0x10000};
    menu::RamRange lists{0x180000,0x10000};
    menu::RamRange materials{0x190000,0x10000};
};
struct ProducerFrame {
    menu::model::ListRange camera;
    std::array<menu::model::ListRange,4> lanes;
    std::span<const menu::RamRange> reservations;
    std::uint64_t revision = 0;
};
// Reusable synchronous scene pass, NOT a Beach handler. Borrows actual World
// models/process state and the admitted full Resources. No duplicate objects,
// player/actor substitutes, cold-image writes or independent GPU submission.
// Ordinary poses/materials/uploads use the same borrowed kernel as title.
// Camera3/14/reflection, direct sky, fog and pass policy stay in this adapter.
// Public World model handles may enable native RT/RTS placement independently
// of ordinary terrain/road/sky locals. Active placements append exactly 64 bytes
// each after unchanged local payload addresses, admitted before writes. Root
// placement PUSH + local NOPUSH share the existing single root pop on lane 0;
// unused lanes remain empty. Pointer/epoch changes republish active payloads.
class Producer final {
public:
    Producer(IsleGame*, IsleGameCamera*, menu::model::CameraBasisState&, const Resources&, const World&,
             ProducerLayout = {}, std::span<const menu::RamRange> other_reservations = {});
    ~Producer();
    Producer(const Producer&) = delete;
    Producer& operator=(const Producer&) = delete;
    ProducerFrame prepare(std::span<std::uint32_t> word_swapped_rdram, std::uint64_t exclusive_epoch);
    ProducerFrame prepare(std::span<std::uint32_t>, std::uint64_t, const scene::render::Pass&);
    static bool prepare_pass(void*, std::span<std::uint32_t>, std::uint64_t,
                             const scene::render::Pass&, scene::render::Prepared&);
    std::span<const menu::RamRange> reservations() const noexcept;
    void retire() noexcept;
private:
    struct Impl;
    ProducerFrame prepare_filtered(std::span<std::uint32_t>, std::uint64_t, const scene::render::Pass*);
    std::unique_ptr<Impl> impl_;
};
} // namespace isle::world
#endif

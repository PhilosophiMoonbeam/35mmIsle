#ifndef ISLE_WORLD_PRODUCER_HPP
#define ISLE_WORLD_PRODUCER_HPP
#include "world_runtime.hpp"
#include "menu_model_producer.hpp"
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
class Producer final {
public:
    Producer(IsleGame*, IsleGameCamera*, const Resources&, const World&, ProducerLayout = {},
             std::span<const menu::RamRange> other_reservations = {});
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

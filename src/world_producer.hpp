#ifndef ISLE_WORLD_PRODUCER_HPP
#define ISLE_WORLD_PRODUCER_HPP
#include "world_runtime.hpp"
#include "borrowed_model_kernel.hpp"
#include "scene_render.hpp"
#include "scene_services.hpp"

namespace isle::world {
struct ProducerLayout {
    menu::RamRange payloads{0x170000,0x10000};
    menu::RamRange lists{0x180000,0x10000};
    menu::RamRange materials{0x190000,0x10000};
};
struct ProducerObject {
    IsleGameObject* object = nullptr;
    scene::RenderMetadata render;
    // Complete callback ranges, including fog setup/restore; no lane EndDL.
    std::array<menu::model::ListRange,4> lanes{};
    // Absolute byte addresses of owned NOP tails, outside raw 64-bit counts.
    std::array<std::uint32_t,4> continuation_slots{};
    bool resets_camera = false; // Direct sky requires camera restoration afterward.
};
struct ProducerFrame {
    menu::model::ListRange camera;
    std::array<menu::model::ListRange,4> lanes;
    std::span<const menu::RamRange> reservations;
    std::uint64_t revision = 0;
    // Borrowed until the next successful preparation or producer destruction.
    // Rejected preparation does not modify the last published object records.
    std::span<const ProducerObject> objects;
};
// World-specific policy over stable actual resources, Runtime and native model
// records. The owner invalidates RenderView before releasing any of its storage;
// producer/graphics consumers must end before native deletion. Camera3/12 or3/14,
// Type-A one/two-cycle sky wrappers, fog and restoration remain in this adapter.
class Producer final {
public:
    Producer(IsleGame*, IsleGameCamera*, menu::model::CameraBasisState&, const Resources&, RenderView,
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

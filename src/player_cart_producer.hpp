#ifndef ISLE_PLAYER_CART_PRODUCER_HPP
#define ISLE_PLAYER_CART_PRODUCER_HPP
#include "player_cart_render_binding.hpp"
#include "scene_render.hpp"

namespace isle::player::cart {
struct ProducerLayout {
    menu::RamRange payloads,lists,materials;
    std::array<menu::RamRange,2> framebuffers;
};
struct ProducerObject {
    IsleGameObject* object=nullptr;
    scene::RenderMetadata render;
    std::array<menu::model::ListRange,4> lanes{};
    // Absolute byte addresses of owned NOP tails, outside raw 64-bit counts.
    std::array<std::uint32_t,4> continuation_slots{};
};
struct ProducerFrame {
    menu::model::ListRange camera;
    std::array<menu::model::ListRange,4> lanes;
    std::span<const menu::RamRange> reservations;
    std::uint64_t revision=0;
    std::span<const ProducerObject> objects;
};
// Genuine source J-fogged deferred hierarchy, through the sole BorrowedKernel.
// Owner bindings are supplied in actual Services DL order. Objects expose raw
// callback ranges (without EndDL) for course-wide native-order composition.
// All active bindings are validated before hidden/pass filtering. No source
// clock, pose, lifecycle, callback policy or GPU completion is synthesized here.
class Producer final {
public:
    Producer(IsleGame*,IsleGameCamera*,menu::model::CameraBasisState&,const CartResources&,
        std::span<const menu::animation::SlotCapacity>,std::span<const RenderBinding>,
        ProducerLayout,std::span<const menu::RamRange> complete_other_reservations);
    ~Producer();
    Producer(const Producer&)=delete;
    Producer& operator=(const Producer&)=delete;
    ProducerFrame prepare(std::span<std::uint32_t>,std::uint64_t exclusive_epoch);
    ProducerFrame prepare(std::span<std::uint32_t>,std::uint64_t,const scene::render::Pass&);
    static bool prepare_pass(void*,std::span<std::uint32_t>,std::uint64_t,
        const scene::render::Pass&,scene::render::Prepared&);
    std::span<const menu::RamRange> reservations() const noexcept;
    std::span<const menu::model::WorldMatrix> world_nodes(menu::animation::InstanceHandle) const noexcept;
    std::span<const IsleGameModelNode> native_nodes(menu::animation::InstanceHandle) const noexcept;
    void retire() noexcept;
private:
    struct Impl;
    ProducerFrame prepare_filtered(std::span<std::uint32_t>,std::uint64_t,const scene::render::Pass*);
    std::unique_ptr<Impl> impl_;
};
} // namespace isle::player::cart
#endif

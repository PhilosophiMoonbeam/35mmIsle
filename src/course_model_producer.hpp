#ifndef ISLE_COURSE_MODEL_PRODUCER_HPP
#define ISLE_COURSE_MODEL_PRODUCER_HPP
#include "actor_producer.hpp"
#include "item_producer.hpp"
#include "world_producer.hpp"
#include "player_cart_producer.hpp"

namespace isle::course {
struct ItemComposition {
    scene::Services& services;
    items::Producer& producer;
    // Explicit projection capacity: DL3 for diagnostic composition, total
    // DL3/4/5 for source composition, including hidden/foreign owners.
    std::size_t render_order_capacity;
    player::cart::Producer* cart = nullptr;
    // Actual PhotoScene object represented by the NULL detector-prefix fragment.
    IsleGameObject* prefix_object = nullptr;
    // Also enables all-provider order for source scenes without Cart/Photo.
    bool global_source_order = false;
};
// Source callbacks are projected in Services link/priority/insertion order.
// Raw callback commands remain in provider RAM; only owned continuation slots
// are patched into cold-bounded unpushed continuation blocks. The no-composition
// diagnostic path retains the original aggregate World/Actor camera join.
// Caller admits the complete immutable/task/framebuffer/capture union and owns
// the one real exclusive preparation/GPU/readback lease. Downstream rejection
// fails that lease; no cross-producer rollback or fake completion is supplied.
class ModelProducer final {
public:
    ModelProducer(world::Producer&, actor::Producer&, menu::RamRange lists,
                  const ItemComposition* = nullptr);
    ModelProducer(const ModelProducer&) = delete;
    ModelProducer& operator=(const ModelProducer&) = delete;
    scene::render::Prepared prepare(std::span<std::uint32_t>, std::uint64_t,
                                    const scene::render::Pass&);
    static bool prepare_pass(void*, std::span<std::uint32_t>, std::uint64_t,
                             const scene::render::Pass&, scene::render::Prepared&);
    std::span<const menu::RamRange> reservations() const noexcept { return {&lists_,1}; }
private:
    world::Producer& world_;
    actor::Producer& actors_;
    menu::RamRange lists_;
    scene::Services* services_ = nullptr;
    items::Producer* items_ = nullptr;
    std::vector<scene::RenderOrderEntry> source_order_;
    player::cart::Producer* cart_ = nullptr;
    IsleGameObject* prefix_object_ = nullptr;
    bool global_source_order_ = false;
    struct Patch { std::uint32_t slot, target; };
    std::vector<Patch> patches_;
    std::vector<menu::model::Command> commands_;
};
} // namespace isle::course
#endif

#ifndef ISLE_COURSE_MODEL_PRODUCER_HPP
#define ISLE_COURSE_MODEL_PRODUCER_HPP
#include "actor_producer.hpp"
#include "borrowed_model_producer.hpp"
#include "world_producer.hpp"
#include "player_cart_producer.hpp"
#include <limits>
#include <stdexcept>

namespace isle::course {
// Four lanes, each with at most a camera call and an unpushed callback branch
// per Services entry, then one EndDL. Provider callback bodies/tails are separate.
constexpr std::size_t source_join_bytes(std::size_t render_order_capacity) {
    constexpr std::size_t entry_bytes = 4*2*8, terminal_bytes = 4*8;
    if (render_order_capacity > (std::numeric_limits<std::size_t>::max()-terminal_bytes)/entry_bytes)
        throw std::overflow_error("course source join size overflows");
    return render_order_capacity*entry_bytes+terminal_bytes;
}
struct ItemComposition {
    scene::Services& services;
    menu::model::PacketProducer& producer;
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
    menu::model::PacketProducer* items_ = nullptr;
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

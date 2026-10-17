#ifndef ISLE_COURSE_MODEL_PRODUCER_HPP
#define ISLE_COURSE_MODEL_PRODUCER_HPP
#include "actor_producer.hpp"
#include "item_producer.hpp"
#include "world_producer.hpp"

namespace isle::course {
struct ItemComposition {
    scene::Services& services;
    items::Producer& producer;
    // Explicit maximum of the actual Services DL3 projection, including hidden
    // owners. This is scratch capacity, not an Item count or ordering registry.
    std::size_t render_order_capacity;
};
// Thin command composition, not another model renderer/owner. Source World and
// Items share DL3: their complete callback commands are merged using Services'
// current global priority/insertion order, including later World/fume insertion.
// Actors retain DL4/5 prefix/history policy; scene graphics executes lanes0,2,1,3.
// Without ItemComposition the legitimate World+Actor diagnostic path is exactly
// the original camera/sublist join. FX retains its separate source camera pass.
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
    std::vector<menu::model::Command> commands_;
};
} // namespace isle::course
#endif

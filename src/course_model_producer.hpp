#ifndef ISLE_COURSE_MODEL_PRODUCER_HPP
#define ISLE_COURSE_MODEL_PRODUCER_HPP
#include "actor_producer.hpp"
#include "world_producer.hpp"

namespace isle::course {
// Thin list composition, not another model renderer/owner. Source World DL3
// precedes actor DL4/5 WITHIN each lane; scene::graphics preserves 0,2,1,3.
// FX uses its separate source camera pass (priority4, below main priority5).
// Caller admits the complete immutable/task/framebuffer/capture union and owns
// real exclusive preparation. A downstream producer rejection fails that whole
// actual scene lease; no cross-producer rollback or retry fence is fabricated.
class ModelProducer final {
public:
    ModelProducer(world::Producer&, actor::Producer&, menu::RamRange lists);
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
};
} // namespace isle::course
#endif

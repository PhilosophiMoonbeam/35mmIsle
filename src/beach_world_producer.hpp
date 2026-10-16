#ifndef ISLE_BEACH_WORLD_PRODUCER_HPP
#define ISLE_BEACH_WORLD_PRODUCER_HPP

#include "beach_world_resources.hpp"
#include "graphics_task.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace isle::beach {
struct FrameTargets {
    std::uint32_t color = 0x300000, depth = 0x313000;
    std::uint16_t width = 64, height = 48;
};
struct Fog {
    std::uint16_t minimum, maximum;
    std::array<std::uint8_t, 3> rgb;
};
enum class Layers : unsigned { terrain = 1, sky = 2, terrain_and_sky = 3 };
struct FrameSettings {
    // GAME positions the block at (descriptor.worldPos - current.worldPos)*100.
    // Caller may select a diagnostic reference; this does not advance a road.
    Vec3 current_block_world_position;
    Fog fog;
    // Diagnostic layer isolation renders the same genuine lists and wrappers.
    Layers layers = Layers::terrain_and_sky;
};

// Real first static Beach block + sky, NOT Beach gameplay or a scene service.
// Borrows one native main camera (kinds 3,14) and immutable admitted resources.
// Snapshot/storage, resources, game and camera must remain alive/unmoved until
// destruction AND final canonical RSP/RDP/completed-memory retirement. Caller
// calls install/prepare only inside the executor's exclusive preparation lease;
// returned task/boot borrow does not acquire or replace that lease.
// Construction owns two native single-root model records. No frame allocation,
// list/texture/vertex recopy, whole-memory reset or hidden-memory reset occurs.
// Rejected preparation preserves native model records and every published
// camera matrix/reflection cache as well as RAM. All request/model preflight
// precedes atomic dirty-camera computation, then same-thread model commits.
class StartProducer final {
public:
    StartProducer(IsleGame* game, IsleGameCamera* camera,
                  const rom::Snapshot& snapshot, const StartResources& resources);
    ~StartProducer();
    StartProducer(const StartProducer&) = delete;
    StartProducer& operator=(const StartProducer&) = delete;
    StartProducer(StartProducer&&) = delete;
    StartProducer& operator=(StartProducer&&) = delete;

    FrameSettings source_settings() const noexcept;
    void install_inputs(std::span<std::uint32_t> word_swapped_rdram, std::uint64_t epoch);
    graphics::PreparedTask prepare(std::span<std::uint32_t> word_swapped_rdram,
                                   std::uint64_t epoch, FrameTargets targets,
                                   const FrameSettings& settings);
    // Stable native borrows; contents update only when the corresponding input
    // changes. Terrain is translate-only, unity scale; sky follows camera eye.
    const IsleGameModelNode& terrain_node() const;
    const IsleGameModelNode& sky_node() const;

    static constexpr std::uint32_t commands_address = 0x100000, commands_end = 0x101000;
    static constexpr std::uint32_t matrices_address = 0x101000;
    static constexpr std::uint32_t reflection_address = 0x101200, viewport_address = 0x102000;
    static constexpr std::uint32_t stack_address = 0x104000, yield_address = 0x105000;
    static constexpr std::uint32_t task_address = 0x106000, boot_address = 0x107000;
    static constexpr std::uint32_t runtime_end = 0x110000;
    static constexpr std::uint32_t output_address = 0x200000, output_end = 0x210000;
private:
    void validate_owner() const;
    const IsleGameModelNode& node(std::size_t index) const;
    IsleGame* game_;
    IsleGameCamera* camera_;
    const rom::Snapshot& snapshot_;
    const StartResources& resources_;
    const std::byte* source_data_;
    std::array<IsleGameObject*, 2> objects_{};
    std::array<IsleGameModel*, 2> models_{};
    std::uint32_t* resident_memory_ = nullptr;
    std::uint64_t resident_epoch_ = 0;
    std::array<std::uint64_t, 4> matrix_revisions_{};
    std::uint64_t reflection_revision_ = 0;
    IsleGameViewport resident_viewport_{};
    bool payloads_resident_ = false;
};
} // namespace isle::beach
#endif

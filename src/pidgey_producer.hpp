#ifndef ISLE_PIDGEY_PRODUCER_HPP
#define ISLE_PIDGEY_PRODUCER_HPP

#include "35mmisle/engine.h"
#include "35mmisle/rsp.hpp"
#include "pidgey_resources.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace isle::pidgey {
struct PhotoTargets {
    std::uint32_t color, depth;
    std::uint16_t width = 64, height = 48;
};
struct PreparedPidgeyTask {
    rsp::Task task;
    std::span<const std::byte> boot;
};

// One bounded pose-0, score-style model pass, not the complete photo scorer.
// Game, camera, immutable Snapshot AND HdResources must remain alive/unmoved
// through this object's destruction AND final canonical request retirement.
// No API may overlap an executing request or its completed-memory borrow.
class PidgeyProducer final {
public:
    PidgeyProducer(IsleGame* game, IsleGameCamera* camera,
                   const rom::Snapshot& snapshot, const HdResources& resources);
    ~PidgeyProducer();
    PidgeyProducer(const PidgeyProducer&) = delete;
    PidgeyProducer& operator=(const PidgeyProducer&) = delete;
    PidgeyProducer(PidgeyProducer&&) = delete;
    PidgeyProducer& operator=(PidgeyProducer&&) = delete;

    void set_photo_pose(float animation_time, IsleGameVec3 position, float yaw);
    std::span<const IsleGameModelNode> model_nodes() const;
    std::span<const IsleGameModelTransform> unbaked_pose() const noexcept { return unbaked_; }

    // Explicit COLD input installation. Copies only actual main image, clears
    // actual main BSS, installs IPL boot and the exact 42 immutable resource
    // spans. Never clears whole RAM. Call once per exclusive canonical session;
    // repeated pointer+epoch is rejected, not a hidden per-frame reinstallation.
    // A new epoch is REQUIRED after external RAM replacement/reset/overwrite.
    void install_inputs(std::span<std::uint32_t> word_swapped_rdram,
                        std::uint64_t canonical_epoch);

    // Requires successful install_inputs on this exact pointer+epoch. Caller
    // retains the canonical map/lease and every input owner through retirement.
    // Writes only runtime payloads (revision cached) and the new command/task
    // stream. Real RDP commands clear RGBA16 color and Z; no CPU target reset.
    PreparedPidgeyTask prepare(std::span<std::uint32_t> word_swapped_rdram,
                              std::uint64_t canonical_epoch, PhotoTargets targets);

    static constexpr std::uint32_t commands_address = 0x100000;
    static constexpr std::uint32_t commands_end = 0x101000;
    static constexpr std::uint32_t matrices_address = 0x101000;
    static constexpr std::uint32_t material_address = 0x101500;
    static constexpr std::uint32_t viewport_address = 0x102000;
    static constexpr std::uint32_t stack_address = 0x104000;
    static constexpr std::uint32_t yield_address = 0x105000;
    static constexpr std::uint32_t task_address = 0x106000;
    static constexpr std::uint32_t boot_address = 0x107000;
    static constexpr std::uint32_t output_address = 0x200000;
    static constexpr std::uint32_t output_end = 0x210000;

private:
    IsleGame* game_;
    IsleGameModelTransform baked_root_{};
    IsleGameCamera* camera_;
    const rom::Snapshot& snapshot_;
    const HdResources& resources_;
    IsleGameObject* object_ = nullptr;
    IsleGameModel* model_ = nullptr;
    std::array<IsleGameModelTransform, node_count> unbaked_{};
    std::array<std::uint32_t, node_count> flags_{};
    std::array<std::uint64_t, node_count + 2> resident_revisions_{};
    std::uint32_t* resident_memory_ = nullptr;
    std::uint64_t resident_epoch_ = 0;
    IsleGameViewport resident_viewport_{};
    bool resident_payloads_ = false, have_pose_ = false;
    float animation_time_ = 0, yaw_ = 0;
    IsleGameVec3 position_{};
};
} // namespace isle::pidgey
#endif

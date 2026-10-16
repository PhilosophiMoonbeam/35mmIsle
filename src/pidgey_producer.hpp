#ifndef ISLE_PIDGEY_PRODUCER_HPP
#define ISLE_PIDGEY_PRODUCER_HPP

#include "35mmisle/engine.h"
#include "35mmisle/rsp.hpp"
#include "pidgey_resources.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace isle::photo { class LightingInputs; }

namespace isle::pidgey {
struct PhotoTargets {
    std::uint32_t color, depth;
    std::uint16_t width = 64, height = 48;
};
struct PreparedPidgeyTask {
    rsp::Task task;
    std::span<const std::byte> boot;
};
inline constexpr std::size_t max_instances = 12;
enum class FeedbackPass { wide, narrow };
struct FeedbackActor {
    std::size_t instance;
    std::optional<std::uint8_t> score_slot; // nullopt: genuine cumulative-only occluder.
    float animation_time;
    IsleGameVec3 position;
    float yaw;
};
struct FeedbackRequest {
    IsleGameLookAt look_at; // GAME photograph uses world-up (0,1,0).
    std::span<const FeedbackActor> actors; // Source order, never sorted/compacted.
    std::uint32_t planes_base;
};
class FrozenFeedbackRequest final {
public:
    std::uint32_t planes_base() const noexcept { return planes_base_; }
private:
    friend class PidgeyProducer;
    FrozenFeedbackRequest() = default;
    const class PidgeyProducer* owner_ = nullptr;
    std::array<FeedbackActor, max_instances> actors_{};
    std::size_t count_ = 0;
    std::uint32_t planes_base_ = 0;
    IsleGameCameraParameters camera_{};
};

// One scene owner with independently owned native pose-0 model instances.
// Game, camera, immutable Snapshot AND HdResources remain alive/unmoved through
// destruction AND final canonical retirement. Calls must not overlap execution
// or a completed-memory borrow; the caller owns that exclusion.
class PidgeyProducer final {
public:
    PidgeyProducer(IsleGame* game, IsleGameCamera* camera,
                   const rom::Snapshot& snapshot, const HdResources& resources,
                   std::size_t capacity);
    ~PidgeyProducer();
    PidgeyProducer(const PidgeyProducer&) = delete;
    PidgeyProducer& operator=(const PidgeyProducer&) = delete;
    PidgeyProducer(PidgeyProducer&&) = delete;
    PidgeyProducer& operator=(PidgeyProducer&&) = delete;

    std::size_t capacity() const noexcept { return capacity_; }
    void set_photo_pose(std::size_t instance, float animation_time, IsleGameVec3 position, float yaw);
    std::span<const IsleGameModelNode> model_nodes(std::size_t instance) const;
    std::span<const IsleGameModelTransform> unbaked_pose(std::size_t instance) const;

    // Explicit COLD installation: main image/BSS, boot, exact immutable model
    // spans and original GAME score copy list. Never resets whole RAM/hidden.
    // Repeated pointer+epoch rejected; external replacement requires new epoch.
    void install_inputs(std::span<std::uint32_t> word_swapped_rdram,
                        std::uint64_t canonical_epoch);
    PreparedPidgeyTask prepare(std::span<std::uint32_t> word_swapped_rdram,
                              std::uint64_t canonical_epoch, std::size_t instance,
                              PhotoTargets targets);
    // The verified normal photo lighting path, not lighting-off score state.
    PreparedPidgeyTask prepare_preview(std::span<std::uint32_t> word_swapped_rdram,
                                      std::uint64_t canonical_epoch, std::size_t instance,
                                      PhotoTargets targets, const photo::LightingInputs& lighting);

    // Copies all bounded descriptors and validates slots/instances, complete
    // plane reservation, both cameras and native poses before any RAM write.
    FrozenFeedbackRequest freeze_feedback_request(const FeedbackRequest& request);
    // One complete task per FOV; inactive B visible bytes alone are CPU-zeroed.
    // Active B uses the exact two-half GAME GPU copy; hidden B is never cleared.
    PreparedPidgeyTask prepare_feedback_pass(std::span<std::uint32_t> word_swapped_rdram,
                                            std::uint64_t canonical_epoch,
                                            const FrozenFeedbackRequest& request,
                                            FeedbackPass pass);

    static constexpr std::uint32_t commands_address = 0x100000;
    static constexpr std::uint32_t commands_end = 0x101000;
    static constexpr std::uint32_t matrices_address = 0x101000;
    static constexpr std::uint32_t material_address = 0x101500;
    static constexpr std::uint32_t viewport_address = 0x102000;
    static constexpr std::uint32_t stack_address = 0x104000;
    static constexpr std::uint32_t yield_address = 0x105000;
    static constexpr std::uint32_t task_address = 0x106000;
    static constexpr std::uint32_t boot_address = 0x107000;
    static constexpr std::uint32_t model_lists_address = 0x108000;
    static constexpr std::uint32_t model_list_bytes = 0x400;
    static constexpr std::uint32_t node_matrices_address = 0x10b000;
    static constexpr std::uint32_t copy_list_address = 0x10e800;
    static constexpr std::uint32_t runtime_end = 0x110000;
    static constexpr std::uint32_t output_address = 0x200000;
    static constexpr std::uint32_t output_end = 0x210000;

private:
    struct Instance {
        IsleGameObject* object = nullptr;
        IsleGameModel* model = nullptr;
        IsleGameModelTransform baked_root{};
        std::array<IsleGameModelTransform, node_count> unbaked{};
        std::array<std::uint32_t, node_count> flags{};
        std::array<std::uint64_t, node_count> resident_revisions{};
        bool have_pose = false, resident_payloads = false;
        float animation_time = 0, yaw = 0;
        IsleGameVec3 position{};
    };
    Instance& instance_at(std::size_t instance);
    const Instance& instance_at(std::size_t instance) const;
    std::uint16_t upload_payloads(std::span<std::uint32_t> words,
                                  std::span<const std::size_t> instances,
                                  const photo::LightingInputs* lighting);
    PreparedPidgeyTask finish_task(std::span<std::uint32_t> words);
    PreparedPidgeyTask prepare_impl(std::span<std::uint32_t> words, std::uint64_t epoch,
                                   std::size_t instance, PhotoTargets targets,
                                   const photo::LightingInputs* lighting);
    IsleGame* game_;
    IsleGameCamera* camera_;
    const rom::Snapshot& snapshot_;
    const HdResources& resources_;
    std::size_t capacity_;
    const std::byte* snapshot_source_;
    std::array<Instance, max_instances> instances_{};
    std::array<std::uint64_t, 2> resident_camera_revisions_{};
    std::uint32_t* resident_memory_ = nullptr;
    std::uint64_t resident_epoch_ = 0;
    IsleGameViewport resident_viewport_{};
    bool resident_payloads_ = false;
    bool preview_inputs_ = false, preview_direction_ = false;
    std::array<float, 4> preview_camera_xz_{};
};
} // namespace isle::pidgey
#endif

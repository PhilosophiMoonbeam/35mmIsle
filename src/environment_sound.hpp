#ifndef ISLE_ENVIRONMENT_SOUND_HPP
#define ISLE_ENVIRONMENT_SOUND_HPP

#include "scene_services.hpp"
#include "source_angles.hpp"

namespace isle::environment_sound {
struct Descriptor {
    std::uint16_t sound;
    std::uint8_t pitch_modifier, hearing_range;
};
// Source course0..6 initialize exactly 53/43/49/62/57/58/9 own-GAME records.
// Only scalar descriptors are owned; the signed-angle lookup keeps its ROM borrow.
class Resources final {
public:
    explicit Resources(const rom::Snapshot&, std::int32_t course_id);
    Resources(rom::Snapshot&&, std::int32_t) = delete;
    Resources(const Resources&) = delete;
    Resources& operator=(const Resources&) = delete;
    Resources(Resources&&) = delete;
    Resources& operator=(Resources&&) = delete;
    std::int32_t course_id() const noexcept { return course_id_; }
    std::span<const Descriptor> descriptors() const;
    const source_math::Angles& angles() const noexcept { return angles_; }
private:
    source_math::Angles angles_;
    std::int32_t course_id_;
    std::size_t descriptor_count_ = 0;
    std::array<Descriptor,62> descriptors_{};
};
struct Source {
    IsleGameObject* identity;
    IsleGameModel* model;
    const float* player_distance;
};
// Serialized GAME owner over the real shared audio session. Source model and
// distance borrows end at matching fix_parameters/stop; fixed voices retain only
// comparison identity. Listener model/camera stay live until cleanup/destruction.
// Resources/Snapshot, Services and arena outlive their borrowers.
class Environment final {
public:
    Environment(scene::Services&, isle_arena&, const Resources&);
    ~Environment(); // Outside OM traversal; joins its update before storage ends.
    Environment(const Environment&) = delete;
    Environment& operator=(const Environment&) = delete;
    // Each source Init replaces the tracked list without stopping its voices
    // and publishes another update process. Null listeners are independently
    // valid; playback/update wait for both. Preparation publishes neither.
    bool initialize(IsleGameModel* player, IsleGameCamera* camera) noexcept;
    // Source descriptor policy plus actual catalog presence, without staging,
    // record allocation, listener/range tests or transient voice admission.
    bool admits_sound(std::int32_t sound) const noexcept;
    bool play(Source, std::uint8_t category, std::int32_t sound) noexcept;
    bool play(Source, std::uint8_t category, std::int32_t sound,
              std::uint8_t pitch_modifier, std::uint8_t hearing_range) noexcept;
    bool stop_category(IsleGameObject*, std::uint8_t category) noexcept;
    bool stop(IsleGameObject*) noexcept;
    void fix_parameters(IsleGameObject*) noexcept;
    // End comparison identity only after real joined retirement. Fixed voices
    // keep playing, but a recycled native address cannot stop the old voice.
    void release_identity(IsleGameObject*) noexcept;
    // Explicit listener changes after source initialization, never a substitute
    // for publishing its descriptors/list/update process.
    bool set_listener(IsleGameModel*, IsleGameCamera*) noexcept;
    int sound_handle(std::int32_t sound) const noexcept;
    void cleanup() noexcept; // Source clears listeners; does not stop voices.
    bool update() noexcept;
private:
    struct Record {
        Record* next = nullptr;
        std::uintptr_t identity = 0;
        Source source{};
        int handle = -1;
        std::uint16_t sound = 0;
        std::uint8_t hearing_range = 0, category = 0;
        bool fixed = false, occupied = false;
    };
    struct UpdatePublication {
        UpdatePublication* next = nullptr;
        IsleGameObject* object = nullptr;
    };
    scene::Services& services_;
    isle_arena& arena_;
    const Resources& resources_;
    IsleGameModel* player_ = nullptr;
    IsleGameCamera* camera_ = nullptr;
    UpdatePublication* updates_ = nullptr;
    Record* records_ = nullptr;
    Record* allocate() noexcept;
    bool volume_pan(Source, std::uint8_t, std::int32_t&, std::int32_t&) noexcept;
    static void tick(IsleGameObject*, void*) noexcept;
    static void joined(IsleGameObject*, void*) noexcept;
};
} // namespace isle::environment_sound
#endif

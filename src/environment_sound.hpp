#ifndef ISLE_ENVIRONMENT_SOUND_HPP
#define ISLE_ENVIRONMENT_SOUND_HPP

#include "scene_services.hpp"
#include "source_angles.hpp"

namespace isle::environment_sound {
struct Descriptor {
    std::uint16_t sound;
    std::uint8_t pitch_modifier, hearing_range;
};
// Beach initializes exactly53 own-GAME EnvSoundData records. The native table
// stores scalar descriptors only; the signed-angle lookup keeps its ROM borrow.
class BeachResources final {
public:
    explicit BeachResources(const rom::Snapshot&);
    BeachResources(rom::Snapshot&&) = delete;
    std::span<const Descriptor> descriptors() const;
    const source_math::Angles& angles() const noexcept { return angles_; }
private:
    source_math::Angles angles_;
    std::array<Descriptor,53> descriptors_{};
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
    Environment(scene::Services&, isle_arena&, const BeachResources&,
                IsleGameModel* player, IsleGameCamera* camera);
    ~Environment(); // Outside OM traversal; joins its update before storage ends.
    Environment(const Environment&) = delete;
    Environment& operator=(const Environment&) = delete;
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
    scene::Services& services_;
    isle_arena& arena_;
    const BeachResources& resources_;
    IsleGameModel* player_;
    IsleGameCamera* camera_;
    IsleGameObject* update_object_ = nullptr;
    Record* records_ = nullptr;
    Record* allocate() noexcept;
    bool volume_pan(Source, std::uint8_t, std::int32_t&, std::int32_t&) noexcept;
    static void tick(IsleGameObject*, void*) noexcept;
    static void joined(IsleGameObject*, void*) noexcept;
};
} // namespace isle::environment_sound
#endif

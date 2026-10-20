#ifndef ISLE_PRIVATE_NATIVE_COURSE_PROBE_HPP
#define ISLE_PRIVATE_NATIVE_COURSE_PROBE_HPP
#include "post_course_scene_probe.hpp"
#include <cstdint>
#include <filesystem>
#include <memory>

struct IslePlayerPhotoData;
namespace isle::game { class State; }
namespace isle::application { struct RuntimeView; struct SceneRetirementView; }
namespace isle::private_course {
enum class CaptureGoal { photographs,empty_roll,progression,sign,apple_evolution,lapras_photographs };
// Explicit authored unlocked profile, AFTER real Name confirmation and BEFORE
// Oak construction. Preserves Name; does not manufacture natural progression.
void seed_progress(game::State&,std::int32_t course,CaptureGoal);
// Borrows the actual typed Runtime owners only. Never creates/advances Source,
// owns a native Scene, changes a transform, or requests an exit/Manager entry.
// Volcano additionally requires the observed Source egg cart stop, a consumed
// controller item throw, natural egg/Moltres transition, captured species146 and
// resumed cart travel. Actor/placement observations never mutate native state.
// Apple evolution additionally requires a consumed new Apple incarnation, its
// exact Charmeleon hit, real evolution witnesses and qualified raw species5/6.
class Probe final {
public:
    Probe(std::int32_t course,private_post_course::Controller,const std::filesystem::path& proofRoot,CaptureGoal,game::State&,bool allowNaturalEmpty);
    ~Probe();
    Probe(const Probe&)=delete;
    Probe& operator=(const Probe&)=delete;
    // Serialized outside native/Source callbacks, once per actual OM update.
    void update(const application::RuntimeView&,std::uint64_t actualUpdates);
    // After genuine retirement joins, before the actual Manager acknowledgement.
    void retired(application::SceneRetirementView);
    // Only while the actual completed course Frame is borrowed, before queueing.
    void frame(const application::RuntimeView&,std::uint64_t ordinal);
    // Only after real Host/LUS presentation completed for that same copied Frame.
    void presented(std::int32_t course,std::uint64_t ordinal,std::uint64_t pixelDigest,bool varied);
    bool completed() const noexcept;
    // Genuine joined empty Source result13 followed by actual Manager Oak13 admission.
    bool empty_completed() const noexcept;
    // Canonical application roll survives course closure; no native object or
    // pose borrow is retained. Scorer-owned flag byte is excluded from identity.
    bool captured_photo(std::int32_t slot,const IslePlayerPhotoData*,std::uint64_t normalizedDigest) const noexcept;
    // Narrows the same canonical identity to the consumed late Lapras exposure.
    bool captured_lapras_late_photo(std::int32_t slot,const IslePlayerPhotoData*,std::uint64_t normalizedDigest) const noexcept;
    // Narrows canonical identity to the real Apple-assisted River child80 exposure.
    [[nodiscard]] bool captured_river_slowbro_photo(std::int32_t slot,const IslePlayerPhotoData*,std::uint64_t normalizedDigest) const noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> p_;
};
}
#endif

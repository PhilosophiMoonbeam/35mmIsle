#ifndef ISLE_PRIVATE_POST_COURSE_SCENE_PROBE_HPP
#define ISLE_PRIVATE_POST_COURSE_SCENE_PROBE_HPP
#include <cstdint>
#include <filesystem>
#include <memory>

namespace isle::post_course { class Scene; }
namespace isle::scene::graphics { struct Completed; }
namespace isle::private_post_course {
struct Controller {
    void* owner;
    void (*set)(void*,std::uint16_t buttons,std::int8_t x,std::int8_t y) noexcept;
};
// Borrowed actual Runtime/Manager consumer. This object never creates a scene,
// advances Source/GTL, services native work, or selects a Manager entry. Its
// receipt explicitly concerns authored-input native APIs, not gameplay/parity.
class Probe final {
public:
    Probe(Controller,const std::filesystem::path& proofRoot);
    ~Probe();
    Probe(const Probe&)=delete;
    Probe& operator=(const Probe&)=delete;
    // Serialized outside Source/native callbacks and GPU mutation. `updates`
    // is a monotonically increasing count of actual completed Source updates
    // (global or per-entry); repeated observations do not advance a pulse.
    void update(const post_course::Scene&,std::int32_t argument,std::uint64_t updates);
    // Capture bounded Source provenance under the genuine completed Frame.
    // Report additionally compares the reconstructed native detail child's
    // tiled RGBA5551 interior to this Frame's RGBA8 pixels, retaining only
    // scalar evidence; no Scene/Window/raster/pixel borrow survives the call.
    void frame(const post_course::Scene&,std::int32_t argument,std::uint64_t frameOrdinal);
    // ONLY after Host::try_present returned completed for the copied surface.
    // Scalars were computed under the corresponding real Frame borrow.
    // Menu-ready signals additionally require copied Source initialization,
    // unfaded/visible UI and settled real Window operations; first varied
    // fade-in pixels alone never authorize Album return or Gallery entry.
    // Report requires ordinary cover -> PKMN Report -> View -> first species A,
    // matched detail-image Host completions -> B/reopen the same canonical
    // photo -> B/list -> B/menu -> B/cover -> B and actual Oak13 retirement.
    // Existing Photos are read only; absent first-species input is a missing
    // prerequisite, never permission to manufacture a photo or callback result.
    void presented(std::int32_t argument,std::uint64_t frameOrdinal,
                   std::uint64_t pixelDigest,bool varied);
    // At actual Runtime retirement callback, after Scene::retire() joined.
    void retired(const post_course::Scene&,std::int32_t result);
    bool complete() const noexcept;
    // Bits follow 14,12,22,24,23,17,18,19,20. Unreached variants remain unset;
    // complete() truthfully requires all nine, including possibly unreachable20.
    std::uint32_t qualified_mask() const noexcept;
    // Harness route request only, NOT an entry override. Zero means all nine.
    std::int32_t next_argument() const noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> p_;
};
}
#endif

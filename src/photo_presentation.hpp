#ifndef ISLE_PHOTO_PRESENTATION_HPP
#define ISLE_PHOTO_PRESENTATION_HPP
#include "photo_presentation.h"
#include "photo_replay.hpp"
#include "scene_graphics.hpp"
#include "application_photos.hpp"
namespace isle::photo {
struct PresentationLayout {
    world::ProducerLayout world;
    menu::model::PacketLayout subjects, items;
    menu::RamRange commands, effects, effect_assets;
};
struct PresentationConfig {
    PresentationLayout memory;
    std::span<const menu::RamRange> external_reservations;
    // Both actual Frame framebuffer ranges must also be present in external.
    std::array<menu::RamRange,2> framebuffers;
    std::size_t record_capacity = 64;
    // Actual Source func_8009FB50 tuple, not camera DL priority/link:
    // Gallery={19,18,17}; End8={5,9,8}.
    std::uint32_t dl_link = 19, world_object_link = 18, photo_object_link = 17;
    scene::graphics::Frame* frame = nullptr;
    // Existing preview/score owner, not a second credits World.
    Replay* thumbnail_replay = nullptr;
    std::array<std::uint8_t,3>* background_rgb = nullptr;
};
// Snapshot, module, Photos, Frame, shared Replay and the actual Source camera
// outlive this unmoved owner. Cold admission caches up to record_capacity exact
// records; exhaustion fails closed. No eviction or warm draw allocation.
// Clear keeps its World (Source does too). Retire joins transient/inactive native
// owners and returns the current World to shared Replay, which MUST then retire
// at actual Source FBC4 before scene/module free. Destruction requires RETIRED.
class Presentation final {
public:
    Presentation(IsleGame*, const rom::Snapshot&, world::ModuleState&, application::Photos&, PresentationConfig);
    ~Presentation() noexcept;
    Presentation(const Presentation&) = delete;
    Presentation& operator=(const Presentation&) = delete;
    bool begin(IsleGameCamera*, const IslePhotoRecord&) noexcept;
    bool begin_working(IsleGameCamera*, IslePhotoWorking*) noexcept;
    IslePhotoPresentationStatus poll() noexcept;
    IslePhotoPresentationStatus clear() noexcept;
    // Nonterminal Photo/World handback at the actual Source hidden-camera boundary.
    // PENDING retains owners; IDLE has returned the unique World to shared Replay.
    IslePhotoPresentationStatus release_to_replay() noexcept;
    IslePhotoPresentationStatus retire() noexcept;
    bool pause_links(std::uint64_t, bool) noexcept;
    IslePhotoPresentationStatus prepare_course(const IslePhotoRecord&) noexcept;
    // Parent registers this actual compiled pass on the provided Source camera,
    // with its actual priority/flags/scissor/filter. Never substitute a sprite.
    static bool prepare_pass(void*, std::span<std::uint32_t>, std::uint64_t,
                             const scene::render::Pass&, scene::render::Prepared&);
    std::span<const menu::RamRange> reservations() const noexcept;
    // Merge immutable union into Frame Resources BEFORE prepare. effect_assets
    // is protected by this immutable union, not an overlapping mutable range.
    std::span<const sprite::AssetSpan> immutable() const noexcept;
    std::uint32_t pre_render_address() const noexcept;
    // Changes only on actual retained resource republication, not draws.
    std::uint64_t resource_revision() const noexcept;
    const char* error() const noexcept;
    IslePhotoPresentation* native_source() noexcept { return reinterpret_cast<IslePhotoPresentation*>(this); }
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
#endif

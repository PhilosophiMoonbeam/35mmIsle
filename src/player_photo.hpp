#ifndef ISLE_PLAYER_PHOTO_HPP
#define ISLE_PLAYER_PHOTO_HPP
#include "player_photo_source.h"
#include "actor_photo_eligibility.hpp"
#include "actor_producer.hpp"
#include "item_owner.hpp"
#include "scene_graphics.hpp"
#include "35mmisle/game_save.hpp"
#include <array>
#include <vector>

namespace isle::player {
/* Exact admitted own app_render data-table semantics. No asset bytes are copied
 * into the executable or published. Tables are decoded once per application. */
class PhotoResources final {
public:
    explicit PhotoResources(const rom::Snapshot&);
    PhotoResources(rom::Snapshot&&)=delete;
    PhotoResources(const PhotoResources&)=delete;
    PhotoResources& operator=(const PhotoResources&)=delete;
    // Application initialization, before root/UI source calls. Snapshot, these
    // resources and the actual save session outlive every bound source consumer.
    IsleGameResult bind(IslePlayerPhotoSource&,save::GameSession&) const noexcept;
    int animation_index(int species, std::uint32_t model_anims) const noexcept;
    float pose(int species, int animation) const noexcept;
private:
    struct Animation { std::uint32_t identity=0; float pose=0; };
    struct Species { int id=0; std::size_t first=0,count=0; };
    std::array<Species,83> species_{};
    std::vector<Animation> animations_;
    const rom::Snapshot* snapshot_;
    std::array<int8_t,151> report_indices_{};
    std::array<const char*,152> names_{};
};
/* Non-owning course bridge. Source/roll outlive it. Dependencies stay stable
 * until every ticket and consumer borrow is explicitly retired. Parent Frame
 * owns GPU/readback; no native join is accepted as completion evidence. */
class Photo final {
public:
    Photo(IslePlayerPhotoSource&, IslePlayerSource&, actor::Owner&, items::Owner&,
          actor::DetectorBinding&, const PhotoResources&, world::World&, int32_t level_id);
    ~Photo();
    Photo(const Photo&)=delete;
    Photo& operator=(const Photo&)=delete;
    // Called by the actual Actor prefix hook, before history stage/commit.
    // generation is the target's next committed history generation.
    // Before submission this is a synchronous single-attempt capability.
    // Retirement consumes it: never defer cleanup with an old ticket into a
    // retry, since rejected precommit attempts can share the next generation.
    IsleGameResult capture_prefix(std::uint16_t context,std::uint64_t generation) noexcept;
    // Only the matching successful producer commit makes a capture queueable.
    IsleGameResult seal_history(std::uint16_t context,std::uint64_t generation) noexcept;
    // Bind to the concrete executing Frame. It must outlive the ticket and
    // must not begin again or retire until this ticket is retired.
    // ticket.submission must be Frame::submission(). ticket.context identifies
    // Detector history, independently of Frame::submission_context().
    IsleGameResult enqueue(IslePlayerPhotoTicket,const scene::graphics::Frame&) noexcept;
    IsleGameResult bind_snapshot_owner(scene::Services&,IsleGameObject*) noexcept;
    IsleGameResult require_taken_context() noexcept;
    IsleGameResult published_focus(IslePlayerPhotoFocus*) noexcept;
    IsleGameResult clear_published_presence() noexcept;
    IsleGameResult borrow_taken(IslePlayerPhotoTicket*) noexcept;
    IsleGameResult release_taken(const IslePlayerPhotoTicket*) noexcept;
    IsleGameResult detach_snapshot_owner() noexcept;
    // Call only from Frame's actual completed-memory callback, before its
    // finish_postprocess. This performs detector analysis and red-center writes.
    IsleGameResult postprocess(IslePlayerPhotoTicket, std::span<std::uint32_t>,
        std::uint32_t regions,std::uint32_t framebuffer) noexcept;
    // Call only after matching Frame::poll()==completed, including RGBA readback.
    IsleGameResult complete(IslePlayerPhotoTicket) noexcept;
    IsleGameResult copy_info() noexcept;
    // Consumer borrow prevents context reuse even after complete/CopyInfo.
    IsleGameResult borrow(IslePlayerPhotoTicket,const IslePlayerPhotoContext*&) noexcept;
    IsleGameResult release(IslePlayerPhotoTicket) noexcept;
    IsleGameResult retire(IslePlayerPhotoTicket) noexcept;
    // Never publishes. Requires the same bound Frame's actual failed token
    // and quiescence; queued/postprocessed tickets only, with no active borrow.
    IsleGameResult retire_failed(IslePlayerPhotoTicket,const scene::graphics::Frame&) noexcept;
    bool quiescent() const noexcept;
    // C leaves used by original snapshot bodies.
    IsleGameResult camera(IslePlayerPhotoData&) noexcept;
    IsleGameResult pokemon(IslePlayerPhotoContext&) noexcept;
    IsleGameResult items(IslePlayerPhotoContext&) noexcept;
    IsleGameResult effects(IslePlayerPhotoContext&) noexcept;
private:
    friend class Controller;
    enum class Phase { idle,rejected,captured,prepared,queued,postprocessed,complete };
    struct Pending { IslePlayerPhotoTicket ticket{}; Phase phase=Phase::idle; unsigned borrows=0;
        std::array<IsleGameObject*,33> holds{}; std::size_t held=0; bool history_retained=false;
        const scene::graphics::Frame* frame=nullptr;
    };
    Pending* matching(IslePlayerPhotoTicket) noexcept;
    IsleGameResult source_access(bool cleanup=false) noexcept;
    IsleGameResult remember(IsleGameResult) noexcept;
    IsleGameResult hold(Pending&,IsleGameObject*) noexcept;
    IsleGameResult release_pending(Pending&) noexcept;
    bool capture_rows_available() const noexcept;
    IslePlayerPhotoSource& source_;
    IslePlayerSource& player_;
    actor::Owner& actors_;
    items::Owner& items_;
    actor::DetectorBinding& detector_;
    const PhotoResources& resources_;
    world::World& world_;
    actor::photo::Camera camera_{};
    std::array<Pending,2> pending_{};
    int32_t level_id_;
    scene::Services* services_ = nullptr;
    IsleGameObject* snapshot_owner_ = nullptr;
    std::array<IsleGameObject**,66> hold_fields_{};
    std::array<std::uint16_t,2> queue_{};
    std::size_t head_=0,count_=0;
    int completed_=-1,published_=-1;
};
/* Canonical big-endian GAME PhotoData, including source packed bitfields. */
void encode_photo(const IslePlayerPhotoData&,std::span<std::byte,0x3a0>) noexcept;
IsleGameResult decode_photo(std::span<const std::byte,0x3a0>,IslePlayerPhotoData&) noexcept;
}
#endif

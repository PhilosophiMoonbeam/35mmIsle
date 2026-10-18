#ifndef ISLE_SPRITE_PRODUCER_HPP
#define ISLE_SPRITE_PRODUCER_HPP

#include "35mmisle/sprite.hpp"
#include "sprite_raster.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace isle::sprite {
struct RamRange { std::uint32_t address = 0, size = 0; };
struct BankMapping { BankOwner owner; std::uint32_t guest_begin = 0; };
struct AssetSpan {
    std::uint32_t address = 0;
    std::span<const std::uint8_t> bytes;
};
// Pixel bounds are half-open. GAME's sprite camera intersects viewport bounds
// with its scaled global camera scissor before traversing source object order.
struct Bounds { std::int32_t left = 0, top = 0, right = 320, bottom = 240; };
struct Camera { Bounds viewport, scissor; };
// Msg's original lane0 texture rectangles use integer inclusive-source
// endpoints (already lowered by its C body) and a decal-alpha button mode.
enum class DrawMode : std::uint8_t { sprite, message_intensity, message_rgba };
struct Draw {
    std::size_t resource = 0;
    const IsleSprite* view = nullptr;
    // Required for dynamic slots, preventing stale native bindings after reuse.
    std::uint64_t generation = 0;
    DrawMode mode = DrawMode::sprite;
    std::int32_t right = 0, bottom = 0;
};
struct Sublist {
    std::uint32_t address = 0;
    std::size_t words = 0, rectangles = 0;
};
struct RasterBinding {
    static constexpr std::size_t no_index = static_cast<std::size_t>(-1);
    // An absent variant has no resource and cannot be selected for drawing.
    std::array<std::size_t, 2> resources{no_index, no_index};
    std::uint64_t generation = 0;
};

// Independent public F3DEX2/RDP producer, not an SDK sprite implementation.
// Construction compiles immutable bitmap extents once; only selected texture
// and TLUT bytes are installed at their numeric bank physical identities.
// Resources, bitmap arrays and shallow views must remain immutable except for
// the view's placement/scale/color/alpha/attr/z/frac fields. Draw order is exactly
// caller order. A copied view never transfers ownership of its bitmap array.
// Geometry preserves Source's pre-draw horizontal wrap, post-wrap full-row
// admission, actualHeight destination extent, bmheight row stride and unscaled
// explosion offsets. nbitmaps remains the bounded native descriptor inventory;
// istep zero repeats the selected descriptor, and width zero terminates it.
// Consecutive identical texture/band/tile setups reuse TMEM within one Sprite,
// never across Sprites or passes. Dry-run planning has the same bounded command
// capacity and resident-state transitions as the subsequent allocation-free write.
// Destination quantization and signed/overflowing derivative lowering retain
// the native RDP sample contract, not Source's integer-edge rounding arithmetic.
//
// The caller holds an exclusive graphics preparation lease for install/prepare,
// and retains this owner AND all source descriptors until actual GPU retirement.
// Epoch identifies caller-owned RAM reuse, not a frame count. No executor,
// task, GPU service, whole-bank overlay copy, or per-frame allocation is hidden.
// All validation precedes RAM writes and resident-cache publication. Rejected
// calls leave RAM and the previous successful installation unchanged.
class Producer final {
public:
    // Hold through actual GPU completion, not merely display-list generation.
    class Borrow final {
    public:
        ~Borrow();
        Borrow(const Borrow&) = delete;
        Borrow& operator=(const Borrow&) = delete;
        Borrow(Borrow&&) noexcept;
        Borrow& operator=(Borrow&&) noexcept;
    private:
        explicit Borrow(Producer&) noexcept;
        Producer* owner_;
        friend class Producer;
    };
    class PreparedResidency final {
    public:
        PreparedResidency(const PreparedResidency&) = delete;
        ~PreparedResidency();
        PreparedResidency& operator=(const PreparedResidency&) = delete;
        PreparedResidency(PreparedResidency&& other) noexcept;
        PreparedResidency& operator=(PreparedResidency&& other) noexcept;
        explicit operator bool() const noexcept { return owner_ != nullptr; }
    private:
        PreparedResidency(Producer& owner, std::uint32_t* memory, std::uint64_t epoch) noexcept
            : owner_(&owner), memory_(memory), epoch_(epoch), serial_(owner.serial_) {}
        Producer* owner_;
        std::uint32_t* memory_;
        std::uint64_t epoch_;
        std::uint64_t serial_;
        friend class Producer;
    };

    Producer(std::span<const IsleSprite* const> resources,
             std::span<const BankMapping> banks);
    Producer(const Producer&) = delete;
    Producer& operator=(const Producer&) = delete;
    Producer(Producer&&) = delete;
    Producer& operator=(Producer&&) = delete;

    std::span<const AssetSpan> assets() const noexcept { return assets_; }
    // At most 128 live/high-water registration slots and 4096 pieces per source.
    // Present variants are compiled once; absent variants have no_index.
    // Rebind selects the matching resource and descriptor view; it never edits
    // an immutable bank or reconstructs Producer.
    RasterBinding register_raster(std::shared_ptr<Raster>);
    void unregister_raster(RasterBinding);
    Borrow borrow();
    // Whole immutable + dynamic + reservation union preflight, changed-byte
    // uploads, then a single residency commit. Unchanged epochs skip ROM copies.
    void install_composed(std::span<std::uint32_t>, std::uint64_t epoch,
                          std::span<const AssetSpan> immutable,
                          std::span<const RamRange> reservations = {});
    // Reservations include every other live producer/task/framebuffer range.
    // Asset collisions require explicit caller relocation, never overwrite.
    void install(std::span<std::uint32_t> word_swapped_rdram, std::uint64_t epoch,
                 std::span<const RamRange> reservations);
    // Composition seam: preflight a caller's immutable selected-span union
    // BEFORE any component writes RAM. Coverage may be split/merged, but every
    // overlapping physical byte MUST identify the same immutable source byte.
    // Validation neither writes RAM nor publishes residency. Caller then copies
    // that exact proven plan under the SAME exclusive lease and commits once.
    // The token and Producer must remain live; discard/move has no side effect.
    PreparedResidency validate_residency(std::span<std::uint32_t> word_swapped_rdram,
                                        std::uint64_t epoch,
                                        std::span<const AssetSpan> installed,
                                        std::span<const RamRange> reservations = {});
    // Requires successful installation of the proven plan, with no RAM reuse
    // or intervening producer mutation. No scan/copy/write/allocation occurs.
    // Throws on stale mutation/foreign authority before changing residency.
    void commit_residency(PreparedResidency&& prepared);
    Sublist prepare(std::span<std::uint32_t> word_swapped_rdram, std::uint64_t epoch,
                    std::span<const Draw> draws, Camera camera, RamRange commands,
                    std::span<const RamRange> reservations = {}) const;
private:
    void validate_storage(std::span<std::uint32_t> word_swapped_rdram,
                          std::span<const RamRange> reservations) const;
    struct Piece {
        IsleBitmap bitmap{};
        std::uint32_t address = 0;
        std::uint16_t rows = 0, line_words = 0, load_rows = 0, row_step = 0, border_rows = 0;
        std::size_t row_bytes = 0;
    };
    struct Resource {
        IsleSprite original{};
        std::size_t first = 0, count = 0;
        std::uint32_t palette = 0;
        std::uint64_t generation = 0;
        bool active = true;
    };
    struct Dynamic {
        std::shared_ptr<Raster> raster;
        std::array<std::size_t, 2> resources{RasterBinding::no_index, RasterBinding::no_index};
        std::array<std::uint64_t, 2> resident_revisions{};
        std::size_t first = 0, capacity = 0;
        unsigned source_count = 0; // Raster topology is fixed for the registration lifetime.
        std::uint64_t generation = 0;
    };
    std::uint32_t resolve(const std::uint8_t*, std::size_t, const Raster::Source*);
    Resource compile(const IsleSprite&, const Raster::Source*);
    void release_borrow() noexcept;
    void release_prepared() noexcept;
    std::vector<Dynamic> dynamic_;
    std::uint64_t generation_ = 0;
    std::uint64_t serial_ = 0;
    unsigned borrowers_ = 0, prepared_ = 0;
    std::vector<BankMapping> banks_;
    std::vector<Resource> resources_;
    std::vector<Piece> pieces_;
    std::vector<AssetSpan> assets_;
    const std::uint32_t* resident_ = nullptr;
    std::uint64_t epoch_ = 0;
};
} // namespace isle::sprite
#endif

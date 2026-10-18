#ifndef ISLE_SPRITE_RASTER_HPP
#define ISLE_SPRITE_RASTER_HPP

#include "35mmisle/sprite.h"
#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace isle::sprite {
enum class RasterSource : unsigned { primary = 0, alternate = 1 };
struct RasterStorage {
    // Present planes must have distinct host and physical storage. Only an
    // all-empty alternate denotes absence; the primary is always required.
    // The lifetime owns the allocation, not an immutable bank or destination RDRAM.
    std::shared_ptr<void> lifetime;
    std::span<std::uint8_t> pixels;
    std::uint32_t address = 0;
};
struct RasterPiece { IsleBitmap bitmap{}; std::size_t offset = 0; };
// Fixed topology and allocations: no resize/reallocation after construction.
// The caller mutates pixels only between graphics retirement and preparation,
// then publishes every changed byte range before submitting another frame.
// Active graphics borrowing is a state error (std::logic_error), distinct
// from invalid topology/source/range parameters (std::invalid_argument).
class Raster final {
public:
    Raster(const IsleSprite& prototype, std::span<const RasterPiece> pieces,
           std::array<RasterStorage, 2> sources);
    Raster(const Raster&) = delete;
    Raster& operator=(const Raster&) = delete;
    bool has_source(RasterSource source) const noexcept;
    const IsleSprite& view(RasterSource source) const;
    std::span<std::uint8_t> pixels(RasterSource source);
    std::uint32_t address(RasterSource source) const;
    std::uint64_t revision(RasterSource source) const;
    void publish(RasterSource source, std::size_t offset, std::size_t size);
private:
    struct Source {
        RasterStorage storage;
        std::vector<IsleBitmap> bitmaps;
        IsleSprite sprite{};
        std::vector<std::uint64_t> changed;
        std::uint64_t revision = 0;
    };
    std::array<Source, 2> sources_;
    unsigned index(RasterSource source) const;
    unsigned borrowers_ = 0;
    friend class Producer;
};
} // namespace isle::sprite
#endif

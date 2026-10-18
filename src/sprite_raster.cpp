#include "sprite_raster.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace isle::sprite {
namespace {
void require(bool value, const char* message) {
    if (!value) throw std::invalid_argument(message);
}
}
Raster::Raster(const IsleSprite& prototype, std::span<const RasterPiece> pieces,
               std::array<RasterStorage, 2> sources) {
    require(!pieces.empty() && pieces.size() <= 4096 && prototype.nbitmaps >= 0 &&
            pieces.size() == static_cast<std::size_t>(prototype.nbitmaps),
            "dynamic raster requires bounded complete topology");
    require((prototype.bmfmt == 0 && (prototype.bmsiz == 2 || prototype.bmsiz == 3)) ||
            (prototype.bmfmt == 3 && prototype.bmsiz <= 2),
            "dynamic raster supports RGBA16/32 and IA4/8/16");
    require(!prototype.LUT && !prototype.LUT_bytes && !prototype.nTLUT,
            "dynamic raster cannot alias an immutable palette");
    for (unsigned i = 0; i < 2; ++i) {
        auto& storage = sources[i];
        if (i == 1 && !storage.lifetime && !storage.pixels.data() &&
            storage.pixels.empty() && !storage.address) continue;
        require(storage.lifetime && storage.pixels.data() && !storage.pixels.empty() &&
                storage.pixels.size() <= 2 * 1024 * 1024 && !(storage.address & 7u) &&
                storage.address <= 8 * 1024 * 1024 &&
                storage.pixels.size() <= 8 * 1024 * 1024 - storage.address,
                "dynamic raster requires live bounded physical storage");
        auto& out = sources_[i];
        out.storage = std::move(storage);
        out.bitmaps.reserve(pieces.size());
        for (const auto& piece : pieces) {
            auto bitmap = piece.bitmap;
            require(piece.offset <= out.storage.pixels.size() &&
                    bitmap.byte_count <= out.storage.pixels.size() - piece.offset &&
                    !(piece.offset & 7u), "dynamic bitmap exceeds or misaligns its fixed plane");
            bitmap.buf = bitmap.width && bitmap.byte_count ? out.storage.pixels.data() + piece.offset : nullptr;
            out.bitmaps.push_back(bitmap);
        }
        out.sprite = prototype;
        out.sprite.bitmap = out.bitmaps.data();
        out.changed.assign(out.storage.pixels.size(), 1);
        out.revision = 1;
    }
    if (!has_source(RasterSource::alternate)) return;
    const auto& a = sources_[0].storage;
    const auto& b = sources_[1].storage;
    require(std::uint64_t(a.address) + a.pixels.size() <= b.address ||
            std::uint64_t(b.address) + b.pixels.size() <= a.address,
            "dynamic source variants overlap physical storage");
    const auto ah = reinterpret_cast<std::uintptr_t>(a.pixels.data());
    const auto bh = reinterpret_cast<std::uintptr_t>(b.pixels.data());
    require(ah + a.pixels.size() <= bh || bh + b.pixels.size() <= ah,
            "dynamic source variants overlap CPU storage");
}
bool Raster::has_source(RasterSource source) const noexcept {
    const auto i = static_cast<unsigned>(source);
    return i < sources_.size() && bool(sources_[i].storage.lifetime);
}
unsigned Raster::index(RasterSource source) const {
    const auto i = static_cast<unsigned>(source);
    require(i < sources_.size(), "invalid raster source");
    require(has_source(source), "raster source is absent");
    return i;
}
const IsleSprite& Raster::view(RasterSource source) const { return sources_[index(source)].sprite; }
std::span<std::uint8_t> Raster::pixels(RasterSource source) {
    if (borrowers_) throw std::logic_error("dynamic raster is borrowed by graphics");
    return sources_[index(source)].storage.pixels;
}
std::uint32_t Raster::address(RasterSource source) const { return sources_[index(source)].storage.address; }
std::uint64_t Raster::revision(RasterSource source) const { return sources_[index(source)].revision; }
void Raster::publish(RasterSource source, std::size_t offset, std::size_t size) {
    if (borrowers_) throw std::logic_error("cannot publish a borrowed dynamic raster");
    auto& out = sources_[index(source)];
    require(offset <= out.changed.size() && size <= out.changed.size() - offset,
            "dynamic raster publication exceeds fixed storage");
    if (!size) return;
    require(out.revision != std::numeric_limits<std::uint64_t>::max(), "dynamic raster revision exhausted");
    ++out.revision;
    std::fill(out.changed.begin() + offset, out.changed.begin() + offset + size, out.revision);
}
} // namespace isle::sprite

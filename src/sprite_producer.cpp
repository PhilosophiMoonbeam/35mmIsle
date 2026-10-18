#include "sprite_producer.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <utility>

namespace isle::sprite {
namespace {
constexpr std::uint32_t ram_bytes = 8 * 1024 * 1024;
void require(bool value, const char* message) {
    if (!value) throw std::invalid_argument(message);
}
bool valid(RamRange r) noexcept {
    return r.address <= ram_bytes && r.size <= ram_bytes - r.address;
}
bool intersects(RamRange a, RamRange b) noexcept {
    return a.size && b.size && a.address < std::uint64_t(b.address) + b.size &&
           b.address < std::uint64_t(a.address) + a.size;
}
void memory_check(std::span<std::uint32_t> memory) {
    require(memory.size() == ram_bytes / 4, "sprite producer requires bounded 8 MiB word-swapped RAM");
}
unsigned bits(const IsleSprite& s) {
    require((s.bmfmt == 0 && (s.bmsiz == 2 || s.bmsiz == 3)) ||
            (s.bmfmt == 2 && s.bmsiz <= 1) || (s.bmfmt == 3 && s.bmsiz <= 2) ||
            (s.bmfmt == 4 && s.bmsiz <= 1), "unsupported sprite texture format");
    return 4u << s.bmsiz;
}
std::uint32_t xy(int x, int y) noexcept {
    return (std::uint32_t(x) << 12) | std::uint32_t(y);
}
std::uint32_t halves(int a, int b) noexcept {
    return (std::uint32_t(std::uint16_t(a)) << 16) | std::uint16_t(b);
}
// Windows x64 long double is binary64; GNU x64 may use extended precision.
// Define one lowering arithmetic contract rather than host-dependent floors.
static_assert(std::numeric_limits<double>::is_iec559 &&
              std::numeric_limits<double>::digits == 53);
struct Derivative {
    int encoded = 0;
    double reciprocal = 0;
    bool lower = false;
};
Derivative derivative(float scale) {
    const float value = 1024.0f / scale;
    if (std::isfinite(value) && value >= -32768 && value <= 32767) {
        const int encoded = static_cast<int>(value);
        require(encoded != 0, "sprite scale loses RDP texture precision");
        return {encoded, static_cast<double>(encoded), false};
    }
    // Only the conceptual derivative is widened. An overflowing S5.10 value
    // is never packed into a command; each consumed sample is rebased below.
    return {0, std::trunc(1024.0 / static_cast<double>(scale)), true};
}
int texture_origin(double value) {
    require(value >= -32768 && value <= 32767,
            "sprite texture coordinate exceeds RDP range");
    return static_cast<int>(value);
}
struct Stream {
    std::span<std::uint32_t> memory;
    std::size_t cursor = 0, rectangles = 0;
    std::uint32_t address = 0;
    std::size_t capacity = ram_bytes / 4;
    void command(std::uint32_t a, std::uint32_t b) {
        require(cursor <= capacity && capacity - cursor >= 2,
                "sprite display list exceeds caller capacity");
        if (!memory.empty()) {
            memory[address / 4 + cursor] = a;
            memory[address / 4 + cursor + 1] = b;
        }
        cursor += 2;
    }
};
Bounds clip(Camera c) {
    const Bounds b{std::max(c.viewport.left, c.scissor.left),
                   std::max(c.viewport.top, c.scissor.top),
                   std::min(c.viewport.right, c.scissor.right),
                   std::min(c.viewport.bottom, c.scissor.bottom)};
    require(c.viewport.left >= 0 && c.viewport.top >= 0 && c.viewport.right <= 1023 &&
            c.viewport.bottom <= 1023 && c.viewport.left <= c.viewport.right &&
            c.viewport.top <= c.viewport.bottom && c.scissor.left <= c.scissor.right &&
            c.scissor.top <= c.scissor.bottom, "invalid sprite camera bounds");
    return {std::clamp(b.left, 0, 1023), std::clamp(b.top, 0, 1023),
            std::clamp(b.right, 0, 1023), std::clamp(b.bottom, 0, 1023)};
}
void modes(Stream& out, const IsleSprite& s, bool copy, DrawMode mode) {
    const bool z = !copy && (s.attr & ISLE_SP_Z);
    const bool blend = !copy && (s.attr & ISLE_SP_TRANSPARENT);
    // Public othermode: non-perspective rectangles, RGBA16 TLUT for CI,
    // bilerp in normal cycle, point sampling in COPY; RGB/alpha dither off.
    const std::uint32_t high = (copy ? 0x200000u : 0x002000u) |
                              (s.bmfmt == 2 ? 0x008000u : 0u) | 0xcf0u;
    // Blender (pixel*pixelAlpha + memory*(1-pixelAlpha)); coverage clamp,
    // image read + force blend only for TRANSPARENT, primitive Z when enabled.
    std::uint32_t low = mode == DrawMode::sprite ?
        (blend ? 0x00504040u : 0x00000000u) : 0x00504240u;
    if (z) low |= 0x34u; // Z source=primitive, compare and update.
    if (s.attr & ISLE_SP_CUTOUT) low |= 1u;
    out.command(0xe7000000, 0);
    out.command(0xef000000 | high, low);
    // Both cycles: (TEXEL0 - 0) * PRIMITIVE + 0, for RGB and alpha.
    out.command(0xfc000000u | (1u << 20) | (3u << 15) | (1u << 12) |
                    ((mode == DrawMode::message_rgba ? 7u : 3u) << 9) | (1u << 5) | 3u,
                (8u << 28) | (8u << 24) | (1u << 21) | ((mode == DrawMode::message_rgba ? 7u : 3u) << 18) |
                    (7u << 15) | (7u << 12) | (7u << 9) | (7u << 6) |
                    ((mode == DrawMode::message_rgba ? 1u : 7u) << 3) | (mode == DrawMode::message_rgba ? 1u : 7u));
    out.command(0xfa000000, (std::uint32_t(s.red) << 24) | (std::uint32_t(s.green) << 16) |
                                   (std::uint32_t(s.blue) << 8) | s.alpha);
    out.command(0xf9000000, 1); // CUTOUT threshold; otherwise unused.
    if (z) out.command(0xee000000, std::uint32_t(std::uint16_t(s.zdepth)) << 16);
}
} // namespace

Producer::Producer(std::span<const IsleSprite* const> resources,
                   std::span<const BankMapping> banks) : banks_(banks.begin(), banks.end()) {
    for (const auto& bank : banks_) {
        require(bool(bank.owner), "sprite bank has no immutable owner");
        const auto base = bank.guest_begin & 0x1fffffffu;
        require((bank.guest_begin < ram_bytes || (bank.guest_begin & 0xff800000u) == 0x80000000u ||
                 (bank.guest_begin & 0xff800000u) == 0xa0000000u) &&
                base <= ram_bytes && bank.owner->size() <= ram_bytes - base,
                "sprite bank mapping exceeds RDRAM");
    }
    resources_.reserve(resources.size());
    for (const auto* source : resources) {
        require(source != nullptr, "null sprite root");
        resources_.push_back(compile(*source, nullptr));
    }
}

std::uint32_t Producer::resolve(const std::uint8_t* pointer, std::size_t size,
                                const Raster::Source* dynamic) {
    if (dynamic) {
        const auto at = reinterpret_cast<std::uintptr_t>(pointer);
        const auto base = reinterpret_cast<std::uintptr_t>(dynamic->storage.pixels.data());
        require(at >= base && at - base <= dynamic->storage.pixels.size() &&
                size <= dynamic->storage.pixels.size() - (at - base),
                "dynamic sprite texture exceeds registered storage");
        const auto address = dynamic->storage.address + static_cast<std::uint32_t>(at - base);
        require((address & 7u) == 0, "dynamic texture must be 64-bit aligned");
        return address;
    }
        require(pointer != nullptr && size > 0, "missing sprite texture or palette");
        // These are HOST bounds only. The resulting guest identity is computed
        // from a checked bank offset, never by casting a host pointer to guest.
        const auto at = reinterpret_cast<std::uintptr_t>(pointer);
        for (const auto& bank : banks_) {
            const auto base = reinterpret_cast<std::uintptr_t>(bank.owner->data());
            if (at < base || at - base > bank.owner->size()) continue;
            const auto offset = at - base;
            if (size > bank.owner->size() - offset) continue;
            const auto address = (bank.guest_begin & 0x1fffffffu) + std::uint32_t(offset);
            require((address & 7u) == 0, "sprite texture/TLUT must be 64-bit aligned");
            const RamRange extent{address, static_cast<std::uint32_t>(size)};
            bool covered = false;
            for (const auto& asset : assets_) {
                const RamRange previous{asset.address, static_cast<std::uint32_t>(asset.bytes.size())};
                if (!intersects(previous, extent)) continue;
                const auto lo = std::max(previous.address, extent.address);
                require(asset.bytes.data() + (lo - previous.address) == pointer + (lo - extent.address),
                        "different sprite banks overlap physical asset identities");
                covered = covered || (extent.address >= previous.address &&
                    std::uint64_t(extent.address) + extent.size <= std::uint64_t(previous.address) + previous.size);
            }
            if (!covered) assets_.push_back({address, {pointer, size}});
            return address;
        }
        throw std::invalid_argument("sprite resource pointer is outside supplied immutable banks");
}

Producer::Resource Producer::compile(const IsleSprite& s, const Raster::Source* dynamic) {
    const auto resolve = [&](const std::uint8_t* pointer, std::size_t size) {
        return this->resolve(pointer, size, dynamic);
    };
        const auto bpp = bits(s);
        require(s.nbitmaps >= 0 && (s.nbitmaps == 0 || s.bitmap) && s.width >= 0 && s.height >= 0 &&
                s.bmheight >= 0 && s.bmHreal >= s.bmheight, "invalid immutable sprite dimensions");
        Resource resource{s, pieces_.size(), static_cast<std::size_t>(s.nbitmaps), 0};
        if (s.bmfmt == 2) {
            require(s.startTLUT >= 0 && s.nTLUT > 0 && unsigned(s.startTLUT) + unsigned(s.nTLUT) <= 256 &&
                    s.LUT_bytes >= std::size_t(s.nTLUT) * 2, "invalid sprite palette extent");
            resource.palette = resolve(s.LUT, std::size_t(s.nTLUT) * 2);
        }
        bool terminated = false;
        for (int i = 0; i < s.nbitmaps; ++i) {
            const auto& b = s.bitmap[i];
            Piece p; p.bitmap = b;
            terminated = terminated || b.width == 0;
            if (!terminated && b.buf) {
                require(b.width > 0 && b.width_img > 0 && b.width_img <= 1024 && b.s >= 0 && b.t >= 0 &&
                        b.actualHeight >= 0 && b.actualHeight <= s.bmHreal, "invalid sprite bitmap");
                p.rows = static_cast<std::uint16_t>(b.actualHeight ? b.actualHeight : s.bmHreal);
                require(p.rows > 0 && p.rows <= 1024, "invalid sprite stored rows");
                p.row_bytes = (std::size_t(b.width_img) * bpp + 7) / 8;
                const auto size = p.row_bytes * p.rows;
                require(b.byte_count == size, "sprite bitmap stored extent changed");
                // RGBA32 uses equal halves of TMEM. CI reserves the upper half
                // for TLUT. Row padding is TMEM padding, never an RDRAM read.
                p.line_words = static_cast<std::uint16_t>((p.row_bytes + (bpp == 32 ? 15 : 7)) /
                                                           (bpp == 32 ? 16 : 8));
                const auto row_tmem_bytes = std::size_t(p.line_words) * 8 * (bpp == 32 ? 2 : 1);
                const auto capacity = s.bmfmt == 2 ? 2048u : 4096u;
                require(row_tmem_bytes && row_tmem_bytes <= capacity, "sprite row exceeds available TMEM");
                p.load_rows = static_cast<std::uint16_t>(std::min<std::size_t>(p.rows, capacity / row_tmem_bytes));
                p.row_step = p.load_rows;
                if (p.rows > p.load_rows) {
                    // Retain both upper and lower filtered borders while
                    // preserving load-address alignment and odd-row parity.
                    // Three GAME bitmaps are 32x68 RGBA16, larger than TMEM.
                    const auto period = s.attr & ISLE_SP_TEXSHUF ? 2u :
                        8u / std::gcd(static_cast<unsigned>(p.row_bytes), 8u);
                    require(p.load_rows > 2 * period, "sprite row cannot retain both filtered TMEM borders");
                    p.border_rows = static_cast<std::uint16_t>(period);
                    p.row_step = static_cast<std::uint16_t>((p.load_rows - 2 * period) / period * period);
                }
                if (s.attr & ISLE_SP_TEXSHUF) {
                    require(p.row_bytes % (bpp == 32 ? 16 : 8) == 0,
                            "pre-shuffled sprite row is not a complete hardware word");
                }
                if (s.bmfmt == 2 && s.bmsiz == 0)
                    require(b.LUToffset >= 0 && b.LUToffset <= 15, "invalid CI4 palette bank");
                p.address = resolve(b.buf, size);
                if (s.bmfmt == 2) {
                    const auto mask = (s.attr & ISLE_SP_TEXSHUF) ? 4u : 0u;
                    for (std::size_t y = 0; y < p.rows; ++y)
                        for (std::size_t x = 0; x < std::size_t(b.width_img); ++x) {
                            const auto offset = s.bmsiz == 0 ? x / 2 : x;
                            const auto packed = b.buf[y * p.row_bytes + (offset ^ ((y & 1) ? mask : 0u))];
                            const auto index = s.bmsiz == 0 ?
                                ((x & 1) ? packed & 15u : packed >> 4) + unsigned(b.LUToffset) * 16 : packed;
                            require(index >= unsigned(s.startTLUT) &&
                                    index - unsigned(s.startTLUT) < unsigned(s.nTLUT),
                                    "sprite CI texel references an uninstalled palette entry");
                        }
                }
            }
            pieces_.push_back(p);
        }
    return resource;
}

Producer::Borrow::Borrow(Producer& owner) noexcept : owner_(&owner) {
    ++owner.borrowers_;
    for (auto& item : owner.dynamic_) if (item.raster) ++item.raster->borrowers_;
}
Producer::Borrow::~Borrow() { if (owner_) owner_->release_borrow(); }
Producer::Borrow::Borrow(Borrow&& other) noexcept : owner_(std::exchange(other.owner_, nullptr)) {}
Producer::Borrow& Producer::Borrow::operator=(Borrow&& other) noexcept {
    if (this != &other) {
        if (owner_) owner_->release_borrow();
        owner_ = std::exchange(other.owner_, nullptr);
    }
    return *this;
}
Producer::Borrow Producer::borrow() { return Borrow(*this); }
void Producer::release_borrow() noexcept {
    --borrowers_;
    for (auto& item : dynamic_) if (item.raster) --item.raster->borrowers_;
}
void Producer::release_prepared() noexcept {
    --prepared_;
    for (auto& item : dynamic_) if (item.raster) --item.raster->borrowers_;
}
Producer::PreparedResidency::~PreparedResidency() { if (owner_) owner_->release_prepared(); }

RasterBinding Producer::register_raster(std::shared_ptr<Raster> raster) {
    require(!borrowers_ && !prepared_, "raster registration requires retired graphics borrowers");
    require(bool(raster) && !raster->borrowers_, "cannot register missing or borrowed raster");
    require(generation_ != UINT64_MAX, "dynamic raster generation exhausted");
    for (const auto& item : dynamic_)
        require(item.raster != raster, "dynamic raster already registered");
    const unsigned source_count = raster->has_source(RasterSource::alternate) ? 2u : 1u;
    for (unsigned i = 0; i < source_count; ++i) {
        const auto& source = raster->sources_[i];
        const auto host = reinterpret_cast<std::uintptr_t>(source.storage.pixels.data());
        for (const auto& bank : banks_) {
            const auto base = reinterpret_cast<std::uintptr_t>(bank.owner->data());
            require(host + source.storage.pixels.size() <= base || base + bank.owner->size() <= host,
                    "mutable raster aliases an immutable bank");
        }
        const RamRange extent{source.storage.address, static_cast<std::uint32_t>(source.storage.pixels.size())};
        for (const auto& asset : assets_)
            require(!intersects(extent, {asset.address, static_cast<std::uint32_t>(asset.bytes.size())}),
                    "dynamic raster overlaps immutable assets");
        for (const auto& item : dynamic_) if (item.raster)
            for (unsigned j = 0; j < item.source_count; ++j) {
                const auto& previous = item.raster->sources_[j];
                require(!intersects(extent, {previous.storage.address,
                        static_cast<std::uint32_t>(previous.storage.pixels.size())}),
                        "dynamic raster physical allocations overlap");
                const auto base = reinterpret_cast<std::uintptr_t>(previous.storage.pixels.data());
                require(host + source.storage.pixels.size() <= base ||
                        base + previous.storage.pixels.size() <= host,
                        "dynamic raster CPU allocations overlap");
            }
    }
    auto slot = std::find_if(dynamic_.begin(), dynamic_.end(), [](const auto& value) { return !value.raster; });
    const auto first = pieces_.size();
    const auto old_resources = resources_.size();
    const auto new_slot = slot == dynamic_.end();
    require(!new_slot || dynamic_.size() < 128, "dynamic raster registration capacity exhausted");
    std::array<std::size_t, 2> indices{RasterBinding::no_index, RasterBinding::no_index};
    unsigned assigned = 0;
    for (std::size_t i = 0; i < resources_.size() && assigned < source_count; ++i)
        if (!resources_[i].active) indices[assigned++] = i;
    auto resource_count = old_resources;
    while (assigned < source_count) indices[assigned++] = resource_count++;
    try {
        std::array<std::optional<Resource>, 2> compiled;
        for (unsigned i = 0; i < source_count; ++i)
            compiled[i].emplace(compile(raster->sources_[i].sprite, &raster->sources_[i]));
        resources_.resize(resource_count);
        if (new_slot) {
            dynamic_.push_back({});
            slot = dynamic_.end() - 1;
        }
        const auto generation = ++generation_;
        for (unsigned i = 0; i < source_count; ++i) {
            compiled[i]->generation = generation;
            resources_[indices[i]] = *compiled[i];
        }
        slot->resources = indices;
        slot->source_count = source_count;
        slot->first = first;
        slot->capacity = pieces_.size() - first;
        slot->generation = generation;
        slot->resident_revisions = {};
        slot->raster = std::move(raster);
        return {slot->resources, generation};
    } catch (...) {
        pieces_.resize(first);
        resources_.resize(old_resources);
        throw;
    }
}
void Producer::unregister_raster(RasterBinding binding) {
    require(!borrowers_ && !prepared_, "raster retirement requires actual retired borrowers");
    const auto slot = std::find_if(dynamic_.begin(), dynamic_.end(), [&](const auto& value) {
        return value.raster && value.generation == binding.generation && value.resources == binding.resources;
    });
    require(slot != dynamic_.end(), "stale or foreign dynamic raster binding");
    require(!slot->raster->borrowers_, "raster has other live graphics borrowers");
    for (unsigned i = 0; i < slot->source_count; ++i) resources_[slot->resources[i]].active = false;
    const auto first = slot->first, count = slot->capacity;
    const auto erase_first = pieces_.begin() + static_cast<std::ptrdiff_t>(first);
    pieces_.erase(erase_first, erase_first + static_cast<std::ptrdiff_t>(count));
    for (auto& resource : resources_) if (resource.active && resource.first >= first + count) resource.first -= count;
    for (auto& item : dynamic_) if (item.raster && item.first >= first + count) item.first -= count;
    slot->raster.reset(); // Releases actual CPU storage owner after the final borrower.
    slot->capacity = 0;
    slot->resources = {RasterBinding::no_index, RasterBinding::no_index};
    slot->source_count = 0;
}

void Producer::validate_storage(std::span<std::uint32_t> memory,
                                std::span<const RamRange> reservations) const {
    memory_check(memory);
    for (const auto r : reservations) require(valid(r), "invalid sprite installation reservation");
    const auto ram_begin = reinterpret_cast<std::uintptr_t>(memory.data());
    const auto ram_end = ram_begin + memory.size_bytes();
    for (const auto& asset : assets_) {
        const RamRange extent{asset.address, static_cast<std::uint32_t>(asset.bytes.size())};
        require(valid(extent), "sprite installation exceeds RDRAM");
        for (const auto r : reservations)
            require(!intersects(extent, r), "sprite asset collides with caller reservation; relocation required");
        const auto begin = reinterpret_cast<std::uintptr_t>(asset.bytes.data());
        require(begin >= ram_end || begin + asset.bytes.size() <= ram_begin, "sprite source aliases destination RAM");
    }
    for (const auto& item : dynamic_) if (item.raster)
        for (unsigned i = 0; i < item.source_count; ++i) {
            const auto& source = item.raster->sources_[i];
            const RamRange extent{source.storage.address, static_cast<std::uint32_t>(source.storage.pixels.size())};
            for (const auto r : reservations)
                require(!intersects(extent, r), "dynamic raster collides with caller reservation");
            const auto begin = reinterpret_cast<std::uintptr_t>(source.storage.pixels.data());
            require(begin >= ram_end || begin + source.storage.pixels.size() <= ram_begin,
                    "dynamic raster aliases destination RAM");
        }
}

Producer::PreparedResidency::PreparedResidency(PreparedResidency&& other) noexcept
    : owner_(std::exchange(other.owner_, nullptr)), memory_(other.memory_), epoch_(other.epoch_), serial_(other.serial_) {}

Producer::PreparedResidency& Producer::PreparedResidency::operator=(PreparedResidency&& other) noexcept {
    if (this != &other) {
        if (owner_) owner_->release_prepared();
        owner_ = std::exchange(other.owner_, nullptr);
        memory_ = other.memory_;
        epoch_ = other.epoch_;
        serial_ = other.serial_;
    }
    return *this;
}

Producer::PreparedResidency Producer::validate_residency(std::span<std::uint32_t> memory,
    std::uint64_t epoch, std::span<const AssetSpan> installed,
    std::span<const RamRange> reservations) {
    validate_storage(memory, reservations);
    require(serial_ != UINT64_MAX, "sprite residency serial exhausted");
    for (const auto& span : installed)
        require(span.address <= ram_bytes && span.bytes.size() <= ram_bytes - span.address &&
                (span.bytes.empty() || span.bytes.data()), "invalid composed sprite installation span");
    const auto ram_begin = reinterpret_cast<std::uintptr_t>(memory.data());
    const auto ram_end = ram_begin + memory.size_bytes();
    for (const auto& span : installed) {
        const RamRange extent{span.address, static_cast<std::uint32_t>(span.bytes.size())};
        const auto begin = reinterpret_cast<std::uintptr_t>(span.bytes.data());
        require(span.bytes.empty() || begin >= ram_end || begin + span.bytes.size() <= ram_begin,
                "composed immutable span aliases destination RAM");
        for (const auto reservation : reservations)
            require(!intersects(extent, reservation), "composed immutable span collides with reservation");
        for (const auto& other : installed) {
            const auto lo = std::max(span.address, other.address);
            const auto hi = std::min(std::uint64_t(span.address) + span.bytes.size(),
                                     std::uint64_t(other.address) + other.bytes.size());
            if (lo < hi)
                require(span.bytes.data() + (lo - span.address) == other.bytes.data() + (lo - other.address),
                        "composed immutable union has conflicting physical source identities");
        }
        for (const auto& item : dynamic_) if (item.raster)
            for (unsigned i = 0; i < item.source_count; ++i) {
                const auto& source = item.raster->sources_[i];
                require(!intersects(extent, {source.storage.address,
                        static_cast<std::uint32_t>(source.storage.pixels.size())}),
                        "immutable union overlaps dynamic raster storage");
                const auto host = reinterpret_cast<std::uintptr_t>(source.storage.pixels.data());
                require(span.bytes.empty() || begin + span.bytes.size() <= host ||
                        host + source.storage.pixels.size() <= begin,
                        "immutable union aliases mutable raster bytes");
            }
    }
    for (const auto& asset : assets_) {
        const auto end = std::uint64_t(asset.address) + asset.bytes.size();
        for (const auto& span : installed) {
            const auto span_end = std::uint64_t(span.address) + span.bytes.size();
            const auto lo = std::max(asset.address, span.address);
            if (lo >= std::min(end, span_end)) continue;
            require(asset.bytes.data() + (lo - asset.address) == span.bytes.data() + (lo - span.address),
                    "composed installation aliases different immutable source bytes");
        }
        std::uint64_t covered = asset.address;
        while (covered < end) {
            auto next = covered;
            for (const auto& span : installed) {
                const auto span_end = std::uint64_t(span.address) + span.bytes.size();
                if (span.address <= covered && covered < span_end)
                    next = std::max(next, std::min(end, span_end));
            }
            require(next > covered, "composed installation omits selected sprite bytes");
            covered = next;
        }
    }
    ++prepared_;
    for (auto& item : dynamic_) if (item.raster) ++item.raster->borrowers_;
    return PreparedResidency(*this, memory.data(), epoch);
}

void Producer::commit_residency(PreparedResidency&& prepared) {
    require(prepared.owner_ == this, "foreign or consumed sprite residency token");
    require(prepared.serial_ == serial_, "stale sprite residency token");
    ++serial_;
    release_prepared();
    prepared.owner_ = nullptr;
    resident_ = prepared.memory_;
    epoch_ = prepared.epoch_;
}

void Producer::install(std::span<std::uint32_t> memory, std::uint64_t epoch,
                       std::span<const RamRange> reservations) {
    install_composed(memory, epoch, assets_, reservations);
}

void Producer::install_composed(std::span<std::uint32_t> memory, std::uint64_t epoch,
                               std::span<const AssetSpan> immutable,
                               std::span<const RamRange> reservations) {
    require(!prepared_, "cannot upload while another residency plan is outstanding");
    auto prepared = validate_residency(memory, epoch, immutable, reservations);
    const bool full = resident_ != memory.data() || epoch_ != epoch;
    const auto copy = [&](std::uint32_t address, std::span<const std::uint8_t> bytes) noexcept {
        for (std::size_t i = 0; i < bytes.size(); ++i) {
            const auto at = address + static_cast<std::uint32_t>(i);
            const auto shift = (3u - (at & 3u)) * 8u;
            auto& word = memory[at / 4];
            word = (word & ~(255u << shift)) | (std::uint32_t(bytes[i]) << shift);
        }
    };
    if (full) for (const auto& span : immutable) copy(span.address, span.bytes);
    for (auto& item : dynamic_) if (item.raster)
        for (unsigned i = 0; i < item.source_count; ++i) {
            const auto& source = item.raster->sources_[i];
            const auto previous = full ? 0 : item.resident_revisions[i];
            if (previous == source.revision) continue;
            std::size_t cursor = 0;
            while (cursor < source.changed.size()) {
                while (cursor < source.changed.size() && source.changed[cursor] <= previous) ++cursor;
                const auto first = cursor;
                while (cursor < source.changed.size() && source.changed[cursor] > previous) ++cursor;
                if (cursor != first)
                    copy(source.storage.address + static_cast<std::uint32_t>(first),
                         source.storage.pixels.subspan(first, cursor - first));
            }
            item.resident_revisions[i] = source.revision;
        }
    commit_residency(std::move(prepared));
}

Sublist Producer::prepare(std::span<std::uint32_t> memory, std::uint64_t epoch,
                          std::span<const Draw> draws, Camera camera, RamRange commands,
                          std::span<const RamRange> reservations) const {
    memory_check(memory);
    require(resident_ == memory.data() && epoch_ == epoch, "sprite assets are not installed for this RAM epoch");
    require(valid(commands) && (commands.address & 7u) == 0 && (commands.size & 7u) == 0,
            "invalid sprite command arena");
    for (const auto r : reservations) {
        require(valid(r), "invalid sprite command reservation");
        require(!intersects(commands, r), "sprite command arena overlaps caller reservation");
    }
    for (const auto& asset : assets_)
        require(!intersects(commands, {asset.address, static_cast<std::uint32_t>(asset.bytes.size())}),
                "sprite commands overlap immutable texture or TLUT");
    for (const auto& item : dynamic_) if (item.raster)
        for (unsigned i = 0; i < item.source_count; ++i) {
            const auto& source = item.raster->sources_[i];
            require(item.resident_revisions[i] == source.revision,
                    "dynamic raster publication is not resident");
            require(!intersects(commands, {source.storage.address,
                    static_cast<std::uint32_t>(source.storage.pixels.size())}),
                    "sprite commands overlap dynamic raster storage");
        }
    const auto ram_begin = reinterpret_cast<std::uintptr_t>(memory.data());
    const auto outside_ram = [&](const void* pointer, std::size_t bytes) {
        const auto begin = reinterpret_cast<std::uintptr_t>(pointer);
        return begin >= ram_begin + memory.size_bytes() ||
               (begin <= ram_begin && bytes <= ram_begin - begin);
    };
    require(outside_ram(draws.data(), draws.size_bytes()), "sprite draw array aliases output RAM");
    for (const auto& draw : draws) {
        require(draw.resource < resources_.size() && draw.view, "invalid sprite draw binding");
        require(outside_ram(draw.view, sizeof(IsleSprite)), "sprite shallow view aliases output RAM");
        const auto& resource = resources_[draw.resource];
        require(resource.active && draw.generation == resource.generation, "stale sprite draw generation");
        require(!resource.generation || borrowers_, "dynamic draw requires graphics retirement borrow");
    }
    const auto bounds = clip(camera);
    const auto generate = [&](Stream& out) {
        out.command(0xe7000000, 0);
        out.command(0xed000000 | xy(bounds.left * 4, bounds.top * 4),
                    xy(std::max(bounds.left, bounds.right) * 4, std::max(bounds.top, bounds.bottom) * 4));
        if (bounds.left < bounds.right && bounds.top < bounds.bottom) for (const auto& draw : draws) {
            const auto& r = resources_[draw.resource];
            const auto& s = *draw.view;
            const auto& initial = r.original;
            require(s.bitmap == initial.bitmap && s.nbitmaps == initial.nbitmaps &&
                    s.bmfmt == initial.bmfmt && s.bmsiz == initial.bmsiz &&
                    s.bmheight == initial.bmheight && s.bmHreal == initial.bmHreal &&
                    s.LUT == initial.LUT && s.LUT_bytes == initial.LUT_bytes &&
                    s.nTLUT == initial.nTLUT && s.startTLUT == initial.startTLUT &&
                    ((s.attr ^ initial.attr) & ISLE_SP_TEXSHUF) == 0,
                    "sprite draw changed immutable resource layout");
            // SP_OVERLAP is a defined, inert source attribute: the original draw
            // path never tests it. Preserve it without inventing geometry or modes.
            require((s.attr & (ISLE_SP_EXTERN | 0xf800u)) == 0,
                    "unsupported external or undefined sprite state");
            require(s.width >= 0 && s.height >= 0 && s.istart >= 0 &&
                    (s.nbitmaps == 0 ? s.istart == 0 : s.istart < s.nbitmaps) && s.istep >= 0 &&
                    std::isfinite(s.scalex) && std::isfinite(s.scaley), "invalid mutable sprite scalar");
            if ((s.attr & ISLE_SP_HIDDEN) || !s.width || !s.height || !s.nbitmaps) continue;
            const float sx = s.attr & ISLE_SP_SCALE ? s.scalex : 1.0f;
            const float sy = s.attr & ISLE_SP_SCALE ? s.scaley : 1.0f;
            if (sx == 0 || sy == 0) continue;
            const bool copy = (s.attr & ISLE_SP_FASTCOPY) != 0;
            require(!copy || (sx == 1.0f && sy == 1.0f), "COPY sprites cannot scale or flip");
            require(!copy || s.bmsiz != 3, "COPY mode does not support RGBA32 textures");
            Derivative dx{}, dy{};
            bool derivatives_ready = false;
            const int t_shift = (s.attr & ISLE_SP_TEXSHIFT ? 16 : 0) +
                                (s.attr & ISLE_SP_FRACPOS ? int(s.frac_t) : 0);
            require(draw.mode == DrawMode::sprite ||
                    (s.nbitmaps == 1 && s.width == 14 && s.height == 14 &&
                     s.scalex > 0 && s.scaley > 0 && (s.attr & ISLE_SP_TEXSHUF) &&
                     s.bitmap[0].width_img == 16 && s.bitmap[0].actualHeight == 13 &&
                     ((s.bmfmt == 4 && s.bmsiz == 1) || (s.bmfmt == 0 && s.bmsiz == 2))),
                    "invalid source message draw");
            modes(out, s, copy, draw.mode);
            if (s.bmfmt == 2) {
                out.command(0xfd100000, r.palette);
                out.command(0xe8000000, 0);
                // TLUT upload replicates each 16-bit source color into four
                // TMEM banks. Its load-tile size is zero (4-bit), not the
                // RGBA16 source-image size; matching sizes leave stale banks.
                out.command(0xf5000000 | (256u + std::uint32_t(s.startTLUT)), 0x07000000);
                out.command(0xe6000000, 0);
                out.command(0xf0000000, 0x07000000 | (std::uint32_t(s.nTLUT - 1) << 14));
                out.command(0xe7000000, 0);
            }
            // Source's previous-buffer cache is local to each Sprite. Native
            // TMEM windows also depend on the loaded band and render-tile setup:
            // equal buffer pointers alone cannot prove residency after banding.
            // Format, size, shuffle mode and DrawMode are fixed for this loop.
            struct TextureState {
                std::uint32_t address;
                int rows, width, line_words, palette;
                bool operator==(const TextureState&) const = default;
            };
            std::optional<TextureState> resident;
            int column = 0, row = 0, px = 0, py = 0;
            // Native nbitmaps is the admitted descriptor inventory, not Source's
            // bitmap+istart+nbitmaps end pointer. Never walk beyond that inventory.
            // A zero step advances geometry, but deliberately retains the bitmap.
            for (int index = s.istart; index < s.nbitmaps; index += s.istep) {
                const auto& p = pieces_[r.first + std::size_t(index)];
                const auto& b = p.bitmap;
                if (!b.width) break;
                require(b.width > 0, "sprite bitmap requires positive column progress");
                require(s.istep > 0 || s.bmheight > 0,
                        "repeated sprite bitmap requires positive row progress");
                // Original wraps BEFORE drawing, admitting only complete rows
                // after a wrap. Neither the right edge nor final row invents a
                // cropped bitmap. actualHeight is geometry; bmheight is stride.
                const int used_h = b.actualHeight ? b.actualHeight : s.bmheight;
                if (px + b.width > s.width) {
                    px = 0; py += s.bmheight; ++row; column = 0;
                    if (py + used_h > s.height) break;
                }
                const int used_w = b.width;
                const bool load_only = b.s > b.width_img;
                if (b.buf && used_w > 0 && used_h > 0) {
                    const int w = std::min<int>(used_w, b.width_img - std::min<int>(b.s, b.width_img));
                    const int total_h = std::min<int>(used_h, int(p.rows) - b.t);
                    for (int band = 0; band < p.rows; band += p.row_step) {
                    int load_first = 0;
                    if (p.border_rows) {
                        // Follow whole-texel fractional-origin changes as well
                        // as retaining both interpolation neighbors. Only true
                        // bitmap edges, never artificial band edges, may clamp.
                        const int period = p.border_rows;
                        const int wanted = std::max(0, band + t_shift / 32 - period);
                        load_first = std::min(wanted / period * period,
                                              (int(p.rows) - 1) / period * period);
                    }
                    const int first_row = std::max<int>(b.t, band);
                    const int last_row = std::min<int>(b.t + total_h, band + p.row_step);
                    const int h = last_row - first_row;
                    if (!load_only && h <= 0) continue;
                    const int tile_t = first_row - load_first;
                    // Source explosion offsets accumulate in destination pixels,
                    // after scaling; they are not part of texture-space layout.
                    const float x0 = float(s.x) + float(px) * sx + float(column * s.expx);
                    const float y0 = float(s.y) + float(py + first_row - b.t) * sy + float(row * s.expy);
                    const float x1 = draw.mode == DrawMode::sprite ? x0 + float(w) * sx : float(draw.right);
                    const float y1 = draw.mode == DrawMode::sprite ? y0 + float(h) * sy : float(draw.bottom);
                    require(std::isfinite(x0) && std::isfinite(x1) && std::isfinite(y0) && std::isfinite(y1),
                            "sprite placement arithmetic overflow");
                    int left = 0, top = 0, right = 0, bottom = 0;
                    int ss = 0, tt = 0;
                    double wide_s = 0, wide_t = 0;
                    if (!load_only && w > 0 && h > 0) {
                        const float l = std::min(x0, x1), t = std::min(y0, y1);
                        const float rr = std::max(x0, x1), bb = std::max(y0, y1);
                        // clip() has checked every bound into [0, 1023], so
                        // these conversions are exact in binary32.
                        if (rr <= static_cast<float>(bounds.left) || bb <= static_cast<float>(bounds.top) ||
                            l >= static_cast<float>(bounds.right) || t >= static_cast<float>(bounds.bottom))
                            continue;
                        // Clip before narrowing; huge offscreen positions never
                        // overflow integer conversions. Texture coordinates track
                        // the same quantized derivative used by the hardware.
                        left = static_cast<int>(std::max(l, float(bounds.left)) * 4.0f);
                        top = static_cast<int>(std::max(t, float(bounds.top)) * 4.0f);
                        right = static_cast<int>(std::min(rr, float(bounds.right)) * 4.0f);
                        bottom = static_cast<int>(std::min(bb, float(bounds.bottom)) * 4.0f);
                        if (left >= right || top >= bottom) continue;
                        // modes() is non-AA: coverage bit 0 is the integer
                        // destination sample. Quarter coverage without such a
                        // sample needs no reciprocal or texture coordinate.
                        if (!copy && ((left + 3) / 4 >= (right + 3) / 4 ||
                                      (top + 3) / 4 >= (bottom + 3) / 4))
                            continue;
                        if (!derivatives_ready) {
                            dx = derivative(sx); dy = derivative(sy);
                            derivatives_ready = true;
                        }
                        const int base_s = (b.s + (sx < 0 ? w - 1 : 0)) * 32 +
                            (s.attr & ISLE_SP_TEXSHIFT ? 16 : 0) +
                            (s.attr & ISLE_SP_FRACPOS ? int(s.frac_s) : 0);
                        const int base_t = (tile_t + (sy < 0 ? h - 1 : 0)) * 32 + t_shift;
                        if (dx.lower)
                            wide_s = std::trunc(static_cast<double>(base_s) +
                                (static_cast<double>(left) / 4 - l) * dx.reciprocal / 32);
                        else {
                            const float start_s = float(b.s + (sx < 0 ? w - 1 : 0)) * 32.0f +
                                (s.attr & ISLE_SP_TEXSHIFT ? 16.0f : 0.0f) +
                                (s.attr & ISLE_SP_FRACPOS ? float(s.frac_s) : 0.0f) +
                                (float(left) * 0.25f - l) * float(dx.encoded) / 32.0f;
                            ss = texture_origin(start_s);
                        }
                        if (dy.lower)
                            wide_t = std::trunc(static_cast<double>(base_t) +
                                (static_cast<double>(top) / 4 - t) * dy.reciprocal / 32);
                        else {
                            const float start_t = float(tile_t + (sy < 0 ? h - 1 : 0)) * 32.0f +
                                float(t_shift) +
                                (float(top) * 0.25f - t) * float(dy.encoded) / 32.0f;
                            tt = texture_origin(start_t);
                        }
                        if (dy.lower && p.border_rows) {
                            // Fractional Y anchors can move the consumed sample
                            // outside its nominal source band. Select TMEM from
                            // actual global samples, not band geometry; retain
                            // both bilerp neighbors and source alignment/parity.
                            const int first_sample = (top + 3) / 4;
                            const int last_sample = (bottom + 3) / 4 - 1;
                            const double first_t = wide_t + load_first * 32 +
                                std::floor(dy.reciprocal * (first_sample - top / 4) / 32);
                            const double last_t = wide_t + load_first * 32 +
                                std::floor(dy.reciprocal * (last_sample - top / 4) / 32);
                            const int first_texel = static_cast<int>(std::clamp(
                                std::floor(std::min(first_t, last_t) / 32), 0.0,
                                static_cast<double>(p.rows - 1)));
                            const int last_texel = static_cast<int>(std::clamp(
                                std::floor(std::max(first_t, last_t) / 32) + 1, 0.0,
                                static_cast<double>(p.rows - 1)));
                            const int period = p.border_rows;
                            const int minimum = (std::max(0, last_texel + 1 - p.load_rows) +
                                                 period - 1) / period * period;
                            const int maximum = first_texel / period * period;
                            require(minimum <= maximum,
                                    "sprite consumed samples exceed aligned TMEM window");
                            const int previous = load_first;
                            load_first = std::clamp(load_first, minimum, maximum);
                            wide_t += (previous - load_first) * 32;
                        }
                    }
                    const int rows = std::min<int>(p.load_rows, p.rows - load_first);
                    const auto texture_address = p.address +
                        std::uint32_t(std::size_t(load_first) * p.row_bytes);
                    const TextureState texture{texture_address, rows, b.width_img, p.line_words,
                        s.bmfmt == 2 && s.bmsiz == 0 ? b.LUToffset : 0};
                    const bool reload = !resident || *resident != texture;
                    out.command(0xe7000000, 0);
                    if (reload) {
                    if (s.attr & ISLE_SP_TEXSHUF) {
                        // Already interleaved odd rows: dxt=0 deliberately avoids
                        // a second interleave. Load bytes as 16-bit, except RGBA32
                        // which must use its hardware split-bank load path.
                        const unsigned load_size = s.bmsiz == 3 ? 3 : 2;
                        const auto texels = draw.mode == DrawMode::sprite ?
                            p.row_bytes * std::size_t(rows) / (load_size == 3 ? 4 : 2) :
                            std::size_t(14 * 14) * (std::size_t(4u << s.bmsiz)) / 16;
                        out.command(0xfd000000 | (std::uint32_t(s.bmfmt) << 21) | (load_size << 19), texture_address);
                        out.command(0xf5000000 | (std::uint32_t(s.bmfmt) << 21) | (load_size << 19), 0x07000000);
                        out.command(0xe6000000, 0);
                        out.command(0xf3000000, 0x07000000 | (std::uint32_t(texels - 1) << 12));
                    } else {
                        // A 4-bit plane loads as bytes, preserving odd nibble-row
                        // pitch exactly; TMEM interprets it as 4-bit only at draw.
                        const unsigned load_size = s.bmsiz == 0 ? 1 : s.bmsiz;
                        const unsigned load_width = s.bmsiz == 0 ? unsigned(p.row_bytes) : unsigned(b.width_img);
                        out.command(0xfd000000 | (std::uint32_t(s.bmfmt) << 21) | (load_size << 19) |
                                    (load_width - 1), texture_address);
                        out.command(0xf5000000 | (std::uint32_t(s.bmfmt) << 21) | (load_size << 19) |
                                    (std::uint32_t(p.line_words) << 9), 0x07000000);
                        out.command(0xe6000000, 0);
                        out.command(0xf4000000, 0x07000000 | xy(int(load_width - 1) * 4, (rows - 1) * 4));
                    }
                    out.command(0xe7000000, 0);
                    out.command(0xf5000000 | (std::uint32_t(s.bmfmt) << 21) | (std::uint32_t(s.bmsiz) << 19) |
                                (std::uint32_t(p.line_words) << 9),
                                (s.bmfmt == 2 && s.bmsiz == 0 ? std::uint32_t(b.LUToffset) << 20 : 0u) |
                                (draw.mode == DrawMode::sprite ? 0x00080200u : 0u));
                    out.command(0xf2000000, draw.mode == DrawMode::sprite ?
                        xy((b.width_img - 1) * 4, (rows - 1) * 4) : xy(13 * 4, 13 * 4));
                    resident = texture;
                    }
                    if (!load_only && right > left && bottom > top && (dx.lower || dy.lower)) {
                        // Exact pinned parallel-rdp 1cecd042 consumer equations:
                        // rdp_device.cpp:735-771, span_setup.comp:111-148,
                        // interpolation.h:95-125, shading.h:176-199.
                        // For native non-AA pixel (p,q), in S10.5 units:
                        // S = ss + floor(Dx * (4*p-left) / 128)
                        // T = tt + floor(Dy * (q-floor(top/4)) / 32).
                        // X corrects its quarter fraction; Y anchors at top>>2.
                        // No LOD/perspective/texel1/AA state in modes() consumes
                        // derivative neighbors. Thus a sole sample can legally
                        // use derivative zero, including bilerp's texel phase.
                        // Retain original quarter edges inside each occupied
                        // cell; omit only cells whose coverage bit 0 is absent.
                        const int first_x = dx.lower ? (left + 3) / 4 * 4 : left;
                        const int first_y = dy.lower ? (top + 3) / 4 * 4 : top;
                        for (int y = first_y; y < bottom;) {
                            const int end_y = dy.lower ? std::min(y + 4, bottom) : bottom;
                            const int origin_t = dy.lower ? texture_origin(wide_t +
                                std::floor(dy.reciprocal * (y / 4 - top / 4) / 32)) : tt;
                            for (int x = first_x; x < right;) {
                                const int end_x = dx.lower ? std::min(x + 4, right) : right;
                                const int origin_s = dx.lower ? texture_origin(wide_s +
                                    std::floor(dx.reciprocal * (x - left) / 128)) : ss;
                                out.command(0xe4000000 | xy(end_x - (copy ? 4 : 0),
                                                          end_y - (copy ? 4 : 0)), xy(x, y));
                                out.command(0xe1000000, halves(origin_s, origin_t));
                                out.command(0xf1000000, halves(copy ? 4096 : dx.encoded, dy.encoded));
                                ++out.rectangles;
                                x = end_x;
                            }
                            y = end_y;
                        }
                    } else if (!load_only && right > left && bottom > top) {
                        out.command(0xe4000000 | xy(right - (copy ? 4 : 0), bottom - (copy ? 4 : 0)), xy(left, top));
                        out.command(0xe1000000, halves(ss, tt));
                        out.command(0xf1000000, halves(copy ? 4096 : dx.encoded, dy.encoded));
                        ++out.rectangles;
                    }
                    if (last_row >= b.t + total_h) break;
                    }
                }
                px += b.width; ++column;
                // The next bitmap, not this one's edge, decides when to wrap.
            }
        }
        out.command(0xe7000000, 0);
        // GAME sprite-camera epilogue restores perspective; other callers must
        // establish their own per-material state as usual after this sublist.
        out.command(0xef080cf0, 0);
        out.command(0xdf000000, 0);
    };
    Stream preflight{{}, 0, 0, 0, commands.size / 4}; generate(preflight);
    Stream output{memory, 0, 0, commands.address, commands.size / 4}; generate(output);
    return {commands.address, output.cursor, output.rectangles};
}
} // namespace isle::sprite

#include "beach_world_resources.hpp"
#include "course_resources.hpp"

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>

namespace isle::beach {
namespace {
constexpr std::uint32_t ram_base = 0x80000000;
constexpr std::uint32_t assets_guest = 0x8011b050;
constexpr std::size_t assets_rom_begin = 0xa74e0, assets_rom_end = 0x13c780;
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
[[noreturn]] void command_error(const char* message, std::uint32_t guest, std::uint32_t value) {
    throw std::runtime_error(std::string(message) + " at guest " + std::to_string(guest) +
                             ", operand " + std::to_string(value));
}
rom::GuestView view(const rom::Snapshot& snapshot, std::uint32_t guest, std::size_t size,
                    std::size_t alignment = 4) {
    require(guest >= assets_guest && guest % alignment == 0 && size != 0,
            "Beach resource is null, misaligned or outside admitted assets mapping");
    const auto delta = static_cast<std::size_t>(guest - assets_guest);
    require(delta <= assets_rom_end - assets_rom_begin &&
            size <= assets_rom_end - assets_rom_begin - delta,
            "Beach resource extent escapes admitted assets mapping");
    const auto offset = assets_rom_begin + delta;
    const auto bytes = snapshot.canonical();
    require(offset <= bytes.size() && size <= bytes.size() - offset,
            "Beach resource extent escapes live Snapshot");
    return {guest, offset, bytes.subspan(offset, size)};
}
std::vector<rom::GuestView> merged(const rom::Snapshot& snapshot, std::vector<rom::GuestView> spans) {
    std::sort(spans.begin(), spans.end(), [](const auto& a, const auto& b) {
        return a.guest_address < b.guest_address;
    });
    std::vector<rom::GuestView> result;
    result.reserve(spans.size());
    for (const auto& span : spans) {
        if (!result.empty()) {
            auto& previous = result.back();
            const auto end = static_cast<std::uint64_t>(previous.guest_address) + previous.bytes.size();
            if (span.guest_address <= end) {
                const auto next_end = static_cast<std::uint64_t>(span.guest_address) + span.bytes.size();
                if (next_end > end)
                    previous = view(snapshot, previous.guest_address,
                                    static_cast<std::size_t>(next_end - previous.guest_address), 1);
                continue;
            }
        }
        result.push_back(span);
    }
    return result;
}
void reservation_valid(RamReservation r) {
    if (r.offset > rdram_size || r.size > rdram_size - r.offset)
        throw std::invalid_argument("Beach install reservation escapes 8 MiB RDRAM");
}
void disjoint(const rom::GuestView& span, RamReservation r) {
    const auto begin = static_cast<std::size_t>(span.guest_address - ram_base);
    if (r.size != 0 && begin < r.offset + r.size && r.offset < begin + span.bytes.size())
        throw std::invalid_argument("Beach immutable resource overlaps caller runtime/frame reservation");
}

// Public F3DEX2/RDP wire ABI, not SDK producer code/layouts. Follow both the
// cull-continue path and calls/no-push branches. Every list must reach ENDDL or
// a terminating branch. Active target cycles, overdeep graphs and unknown
// operations (including unhandled address-bearing ops) are rejected explicitly.
struct ImageState {
    std::uint32_t guest = 0, command_guest = 0;
    std::uint8_t format = 0, size_code = 0;
    std::uint16_t width = 0;
};
class Closure final {
public:
    Closure(const rom::Snapshot& snapshot, std::vector<DisplayList>& lists,
            std::vector<VertexLoad>& vertices, std::vector<ImageLoad>& images)
        : snapshot_(snapshot), lists_(lists), vertices_(vertices), images_(images) {}
    void root(std::uint32_t guest) {
        ImageState image{};
        scan(guest, image);
    }
private:
    void scan(std::uint32_t guest, ImageState& image) {
        if (std::find(active_.begin(), active_.end(), guest) != active_.end())
            command_error("Cyclic Beach G_DL", guest, guest);
        require(active_.size() < 32, "Beach G_DL graph exceeds construction depth limit 32");
        active_.push_back(guest);
        std::size_t count = 0;
        bool terminated = false;
        while (!terminated) {
            // Finite framing even for malformed graphs with repeated DAG calls.
            require(++visited_ <= 1048576, "Beach GBI traversal exceeds finite construction limit");
            const auto address64 = static_cast<std::uint64_t>(guest) + count * 8;
            require(address64 <= 0xffffffffu, "Beach GBI command address overflow");
            const auto address = static_cast<std::uint32_t>(address64);
            const auto wire = view(snapshot_, address, 8, 8);
            const auto w0 = wire.u32(0), w1 = wire.u32(4);
            ++count;
            switch (w0 >> 24) {
            case 0x01: {
                const auto n = (w0 >> 12) & 0xffu, end = (w0 & 0xfffu) >> 1;
                require((w0 & 0xfff00fffu) == (0x01000000u | end * 2) &&
                        n != 0 && end >= n && end <= 32,
                        "Invalid Beach F3DEX2 vertex count/cache range");
                const auto source = view(snapshot_, w1, n * 16, 8);
                const auto old = std::find_if(vertices_.begin(), vertices_.end(), [address](const auto& v) {
                    return v.command_guest == address;
                });
                if (old == vertices_.end())
                    vertices_.push_back({source, address, static_cast<std::uint8_t>(end - n),
                                        static_cast<std::uint8_t>(n)});
                break;
            }
            case 0xde:
                if (w0 != 0xde000000 && w0 != 0xde010000)
                    command_error("Unknown Beach G_DL branch mode", address, w0);
                // Segmented/dynamic targets are never replaced by fake lists.
                view(snapshot_, w1, 8, 8);
                scan(w1, image);
                terminated = w0 == 0xde010000;
                break;
            case 0xdf:
                require(w0 == 0xdf000000 && w1 == 0, "Malformed Beach ENDDL");
                terminated = true;
                break;
            case 0xfd:
                view(snapshot_, w1, 1, 8); // Explicitly reject external image references, even unused ones.
                image = {w1, address, static_cast<std::uint8_t>((w0 >> 21) & 7u),
                         static_cast<std::uint8_t>((w0 >> 19) & 3u),
                         static_cast<std::uint16_t>((w0 & 0xfffu) + 1)};
                require((w0 & 0x0007f000u) == 0, "Malformed Beach SETTIMG reserved bits");
                break;
            case 0xf0: case 0xf3: {
                const bool palette = (w0 >> 24) == 0xf0;
                require(image.guest != 0 && image.size_code == 2 && image.width == 1 &&
                        (image.format == 0 || image.format == 2),
                        "Beach image load has missing/state-dependent/unsupported source format");
                require((w0 & 0xffffffu) == 0, "Beach image load uses unsupported nonzero source origin");
                const auto n = palette ? ((w1 >> 14) & 0x3ffu) + 1 : ((w1 >> 12) & 0xfffu) + 1;
                if (palette)
                    require(image.format == 0 && n <= 256 && (w1 & 0xf0003fffu) == 0,
                            "Invalid Beach RGBA16 TLUT load");
                const auto source = view(snapshot_, image.guest, n * 2, 8);
                const auto old = std::find_if(images_.begin(), images_.end(), [address](const auto& v) {
                    return v.load_command_guest == address;
                });
                if (old == images_.end())
                    images_.push_back({source, image.command_guest, address,
                                       palette ? ImageLoadKind::rgba16_palette : ImageLoadKind::texture_block,
                                       image.format, image.size_code, image.width, static_cast<std::uint16_t>(n)});
                else
                    require(old->wire.guest_address == source.guest_address &&
                            old->wire.bytes.size() == source.bytes.size(),
                            "Beach G_DL reuses an image load with state-dependent external data");
                break;
            }
            case 0x03: // CULLDL can return early; install the visible continuation too.
            case 0x05: case 0x06: // TRI1/TRI2 contain cache indices, not addresses.
            case 0xd7: case 0xd9: // Texture/geometry state.
            case 0xe2: case 0xe3: case 0xe6: case 0xe7: case 0xe8:
            case 0xf2: case 0xf5: case 0xfa: case 0xfc:
                break;
            default:
                // In particular: matrices, MOVEMEM, segment rebinding, BRANCH_Z,
                // DMA, LOADTILE and color/depth targets are NOT silently ignored.
                command_error("Unknown/unhandled Beach GBI resource opcode", address, w0);
            }
        }
        active_.pop_back();
        const auto found = std::find_if(lists_.begin(), lists_.end(), [guest](const auto& list) {
            return list.wire.guest_address == guest;
        });
        if (found == lists_.end()) lists_.push_back({view(snapshot_, guest, count * 8, 8)});
    }
    const rom::Snapshot& snapshot_;
    std::vector<DisplayList>& lists_;
    std::vector<VertexLoad>& vertices_;
    std::vector<ImageLoad>& images_;
    std::vector<std::uint32_t> active_;
    std::size_t visited_ = 0;
};
} // namespace

Command DisplayList::command(std::size_t index) const {
    if (index >= command_count()) throw std::out_of_range("Beach display-list command index");
    return {wire.u32(index * 8), wire.u32(index * 8 + 4)};
}
WorldDescriptors world_descriptors(const rom::Snapshot& snapshot) {
    auto decoded = course::world_descriptors(snapshot, course::beach_id);
    const auto& first = decoded.primary.front();
    require(decoded.primary.size() == 8 && decoded.secondary.size() == 4 &&
            first.gfx_wire.guest_address == 0x8011cd50 && first.display_list_guest == 0x8011cf00 &&
            first.render_callback_guest == 0x800a1530 && first.materials_guest == 0 &&
            first.texture_animation_guest == 0,
            "Beach start selection is not the admitted static Type-A-fogged model");
    const auto& sky = decoded.sky;
    require(sky.wire.guest_address == 0x8011b940 && sky.display_list_guest == 0x8011cc28 &&
            sky.render_callback_guest == 0x800e1ca4 && sky.materials_guest == 0 && sky.animation_guest == 0,
            "Beach sky selection requires an unadmitted callback or dynamic resource producer");
    return decoded;
}
StartResources::StartResources(const rom::Snapshot& snapshot)
    : snapshot_(&snapshot), source_data_(snapshot.canonical().data()) {
    auto decoded = world_descriptors(snapshot);
    world_ = decoded.world; sky_ = decoded.sky;
    primary_ = std::move(decoded.primary); secondary_ = std::move(decoded.secondary);
    const auto& first = start_block();

    Closure closure(snapshot, lists_, vertices_, images_);
    closure.root(first.display_list_guest);
    closure.root(sky_.display_list_guest);
    std::vector<rom::GuestView> spans, vertex_spans;
    for (const auto& list : lists_) {
        spans.push_back(list.wire);
        counts_.commands += list.command_count();
    }
    for (const auto& v : vertices_) {
        spans.push_back(v.wire); vertex_spans.push_back(v.wire);
        counts_.loaded_vertices += v.count;
    }
    for (const auto& image : images_) {
        spans.push_back(image.wire);
        if (image.kind == ImageLoadKind::rgba16_palette) {
            ++counts_.palette_loads; counts_.palette_bytes += image.wire.bytes.size();
        } else {
            ++counts_.texture_loads; counts_.texture_bytes += image.wire.bytes.size();
        }
    }
    for (const auto& span : merged(snapshot, std::move(vertex_spans)))
        counts_.unique_vertices += span.bytes.size() / 16;
    counts_.display_lists = lists_.size(); counts_.vertex_loads = vertices_.size();
    counts_.excluded_blocks = primary_.size() + secondary_.size() - 1;
    for (const auto& item : primary_)
        if (item.materials_guest != 0 || item.texture_animation_guest != 0) ++counts_.excluded_material_blocks;
    for (const auto& item : secondary_)
        if (item.materials_guest != 0 || item.texture_animation_guest != 0) ++counts_.excluded_material_blocks;
    install_ = merged(snapshot, std::move(spans));
    for (const auto& span : install_) {
        require(span.guest_address >= ram_base &&
                static_cast<std::size_t>(span.guest_address - ram_base) <= rdram_size &&
                span.bytes.size() <= rdram_size - static_cast<std::size_t>(span.guest_address - ram_base),
                "Beach installation span escapes RDRAM");
        for (const auto& reservation : producer_reservations) disjoint(span, reservation);
        counts_.installed_bytes += span.bytes.size();
    }
    counts_.installation_spans = install_.size();
}
void StartResources::install(std::span<std::byte> ram, std::span<const RamReservation> additional) const {
    if (ram.size() != rdram_size)
        throw std::invalid_argument("Beach install requires caller-owned word-swapped 8 MiB RDRAM");
    const auto canonical = snapshot_->canonical();
    require(canonical.size() == rom::us_size && canonical.data() == source_data_,
            "Beach install borrows a moved-from Snapshot");
    const std::less<const std::byte*> less;
    if (less(ram.data(), canonical.data() + canonical.size()) &&
        less(canonical.data(), ram.data() + ram.size()))
        throw std::invalid_argument("Beach install RAM aliases immutable ROM Snapshot");
    for (const auto& r : additional) reservation_valid(r);
    for (const auto& span : install_) {
        const auto offset = static_cast<std::size_t>(span.guest_address - ram_base);
        require(span.guest_address >= ram_base && offset <= rdram_size &&
                span.bytes.size() <= rdram_size - offset &&
                span.rom_offset <= canonical.size() && span.bytes.size() <= canonical.size() - span.rom_offset &&
                span.bytes.data() == canonical.data() + span.rom_offset,
                "Invalid Beach immutable installation target/provenance");
        for (const auto& r : producer_reservations) disjoint(span, r);
        for (const auto& r : additional) disjoint(span, r);
    }
    for (const auto& span : install_) {
        const auto offset = static_cast<std::size_t>(span.guest_address - ram_base);
        for (std::size_t i = 0; i < span.bytes.size(); ++i) ram[(offset + i) ^ 3u] = span.bytes[i];
    }
}
} // namespace isle::beach

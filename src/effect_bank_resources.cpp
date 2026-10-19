#include "effect_bank_resources.hpp"
#include "course_resources.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <compare>
#include <iterator>
#include <limits>
#include <map>
#include <utility>

namespace isle::menu {
namespace {
std::uint32_t identity(rom::GuestView view, std::size_t offset = 0) {
    return static_cast<std::uint32_t>((view.guest_address ? view.guest_address : view.rom_offset) + offset);
}
void require(bool value, rom::GuestView view, std::size_t offset = 0, Error error = Error::malformed) {
    if (!value) throw EffectResourceError({error, identity(view, offset)});
}
rom::GuestView slice(rom::GuestView source, std::size_t offset, std::size_t size) {
    require(offset <= source.bytes.size() && size <= source.bytes.size() - offset,
            source, offset, Error::extent);
    require(offset <= std::numeric_limits<std::size_t>::max() - source.rom_offset &&
            (!source.guest_address || offset <= std::numeric_limits<std::uint32_t>::max() - source.guest_address),
            source, offset, Error::extent);
    return {source.guest_address ? source.guest_address + static_cast<std::uint32_t>(offset) : 0,
            source.rom_offset + offset, source.bytes.subspan(offset, size)};
}
std::size_t product(std::size_t a, std::size_t b, rom::GuestView where) {
    require(!a || b <= std::numeric_limits<std::size_t>::max() / a, where, 0, Error::extent);
    return a * b;
}
float floating(rom::GuestView source, std::size_t offset) {
    const auto operand = slice(source, offset, 4);
    const auto value = operand.f32(0); // Packed operands are intentionally reframed, not cast.
    require(std::isfinite(value), source, offset);
    return value;
}
struct Range { std::size_t begin, end; };
bool overlaps(Range a, Range b) { return a.begin < b.end && b.begin < a.end; }
void add_record(std::vector<Range>& records, Range record, rom::GuestView section) {
    for (const auto other : records) require(!overlaps(record, other), section, record.begin);
    records.push_back(record);
}
std::vector<std::size_t> offsets(rom::GuestView section, rom::GuestView& table) {
    const auto count = slice(section, 0, 4).u32(0);
    require(count <= (section.bytes.size() - 4) / 4, section, 0, Error::extent);
    table = slice(section, 0, 4 + static_cast<std::size_t>(count) * 4);
    std::vector<std::size_t> result;
    result.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const auto offset = table.u32(4 + i * 4);
        require(offset >= table.bytes.size() && offset % 4 == 0, section, 4 + i * 4);
        result.push_back(offset);
    }
    return result;
}
std::size_t frame_bytes(const EffectImage& image) {
    const auto f = image.format, s = image.size_code;
    require((f == 4 && (s == 0 || s == 1)) || (f == 3 && s == 1) ||
            (f == 2 && s == 0) || (f == 0 && (s == 2 || s == 3)), image.header, 4);
    require(image.width && image.height && image.width <= 0x7fffffffu && image.height <= 0x7fffffffu,
            image.header, 12); // source dimensions are signed32, not unsigned extents
    const auto pixels = product(image.width, image.height, image.header);
    const auto bits = product(pixels, std::size_t{4} << s, image.header);
    require(bits <= std::numeric_limits<std::size_t>::max() - 7, image.header, 12, Error::extent);
    return (bits + 7) / 8;
}
struct Decoded {
    EffectInstruction instruction;
    std::uint16_t wait = 0;
    std::uint8_t count = 0;
    bool branch_seen = false, dynamic_branch = false;
};
struct Control {
    std::size_t pc = 0, loop = no_index, ret = 0;
    std::uint8_t count = 0;
    auto operator<=>(const Control&) const = default;
};
void decode_script(EffectScript& script, rom::GuestView bytecode,
                   std::size_t script_count, std::size_t frame_count) {
    std::map<std::size_t, Decoded> decoded;
    auto decode = [&](std::size_t start) {
        std::size_t p = start;
        auto byte = [&]() { const auto v = slice(bytecode, p, 1).u8(0); ++p; return v; };
        auto skip = [&](std::size_t n) { slice(bytecode, p, n); p += n; };
        auto word = [&]() { const auto hi = byte(); return (static_cast<unsigned>(hi) << 8) | byte(); };
        auto number = [&]() { const auto n = byte(); if (n & 0x80) skip(1); };
        auto scalar = [&]() { floating(bytecode, p); p += 4; };
        auto range = [&](std::size_t first, std::size_t count, std::size_t limit) {
            require(first < limit && count <= limit - first, bytecode, start);
        };
        Decoded result{};
        const auto op = byte();
        if (op < 0x80) {
            result.wait = op & 31;
            if (op & 32) result.wait = static_cast<std::uint16_t>((result.wait << 8) | byte());
            if ((op & 0xc0) == 0x40) require(byte() < frame_count, bytecode, start);
        } else if (op <= 0x9f) {
            for (unsigned b = 0; b < 3; ++b) if (op & (1u << b)) scalar();
        } else if ((op & 0xf0) == 0xc0 || (op & 0xf0) == 0xd0) {
            number(); skip(static_cast<std::size_t>(std::popcount(static_cast<unsigned>(op & 15))));
        } else switch (op) {
        case 0xa0: number(); scalar(); break;
        case 0xa1: skip(1); break;
        case 0xa2: case 0xa3: case 0xa9: case 0xab: scalar(); break;
        case 0xa4: case 0xa5: case 0xb9: require(word() < script_count, bytecode, start); break;
        case 0xa6: skip(4); break;
        case 0xa7: skip(1); break;
        case 0xa8: case 0xbe: scalar(); scalar(); scalar(); break;
        case 0xaa: { const auto first = word(), count = word(); range(first, count, script_count); break; }
        case 0xac: number(); scalar(); scalar(); break;
        case 0xad: case 0xae: case 0xaf: case 0xb0: case 0xb1: case 0xb2:
        case 0xb3: case 0xb4: case 0xb5: case 0xb6: break;
        case 0xb7: require(byte() == 0, bytecode, start); break; // source target table has one slot
        case 0xb8: require(byte() == 0, bytecode, start); scalar(); break;
        case 0xbf: { const auto slot = byte(); require(slot >= 1 && slot <= 8, bytecode, start); break; }
        case 0xba: case 0xbb: skip(4); break;
        case 0xbc: {
            const auto first = byte(), count = byte();
            // dataID is u8; a wrapping range can reach255 but not exceed it.
            require(first < frame_count &&
                    std::min(static_cast<unsigned>(first) + count, 256u) <= frame_count, bytecode, start);
            break;
        }
        case 0xbd: scalar(); scalar(); break;
        case 0xfa: result.count = byte(); break;
        case 0xfb: case 0xfc: case 0xfd: case 0xfe: case 0xff: break;
        default: require(false, bytecode, start);
        }
        result.instruction = {start, p - start, no_index, op};
        const auto after = decoded.lower_bound(start);
        if (after != decoded.end()) require(p <= after->first, bytecode, start);
        if (after != decoded.begin()) {
            const auto& previous = std::prev(after)->second.instruction;
            require(previous.offset + previous.size <= start, bytecode, start);
        }
        return result;
    };
    // Exact finite source control-state trajectory, not lexical back-edge
    // heuristics. No arbitrary instruction budget. Registers survive waits.
    // Repeating a state inside one tick is a hang; repeating it after a true
    // nonzero wait proves a safe periodic trajectory. FA overwrites one loop
    // register, FA00 wraps through all 256 counts; FC overwrites one return
    // register initialized to zero. FB before FA has undefined source state.
    std::map<Control, std::size_t> visited;
    std::size_t tick = 0;
    Control state;
    std::size_t end = 0;
    for (;;) {
        const auto [previous, inserted] = visited.emplace(state, tick);
        if (!inserted) {
            require(previous->second != tick, bytecode, state.pc);
            break;
        }
        auto found = decoded.find(state.pc);
        if (found == decoded.end()) found = decoded.emplace(state.pc, decode(state.pc)).first;
        auto& d = found->second;
        const auto& ins = d.instruction;
        end = std::max(end, ins.offset + ins.size);
        state.pc += ins.size;
        if (ins.opcode == 0xfe || ins.opcode == 0xff) break;
        if (ins.opcode == 0xfa) { state.loop = static_cast<std::uint16_t>(state.pc); state.count = d.count; }
        if (ins.opcode == 0xfb) {
            require(state.loop != no_index, bytecode, ins.offset);
            state.count = static_cast<std::uint8_t>(state.count - 1);
            if (state.count) state.pc = state.loop;
        }
        if (ins.opcode == 0xfc) state.ret = static_cast<std::uint16_t>(state.pc);
        if (ins.opcode == 0xfd) state.pc = state.ret;
        if (ins.opcode == 0xfb || ins.opcode == 0xfd) {
            const auto target = ins.opcode == 0xfb ? state.loop : state.ret;
            if (d.branch_seen && d.instruction.branch_offset != target) d.dynamic_branch = true;
            d.branch_seen = true;
            d.instruction.branch_offset = d.dynamic_branch ? no_index : target;
        }
        if (d.wait) {
            // The live source cursor is a byte pointer. Only saved cursors
            // narrow to the declared u16 fields (effect.h:62-64).
            state.pc = static_cast<std::uint16_t>(state.pc);
            ++tick;
        }
    }
    script.bytecode = slice(bytecode, 0, end);
    for (const auto& [offset, d] : decoded) script.instructions.push_back(d.instruction);
}
constexpr std::array<std::uint8_t, 20> admitted_sha1{
    0xed,0xc7,0xc4,0x9c,0xc5,0x68,0xc0,0x45,0xfe,0x48,
    0xbe,0x0d,0x18,0x01,0x1c,0x30,0xf3,0x93,0xcb,0xaf
};
struct Section { std::size_t scripts, images, end; std::uint32_t bank; };
constexpr std::array<Section, 9> sections{{
    {11229280,11229568,11240928,3}, {11240928,11241344,11265952,0},
    {11265952,11267024,11298960,0}, {11298960,11299456,11306256,0},
    {11306256,11307056,11335408,0}, {11335408,11336096,11341312,0},
    {11341312,11343424,11391760,0}, {11391760,11392464,11398240,0},
    {11398240,11398640,11404560,0}
}};
std::size_t index(EffectGroup group) {
    const auto value = static_cast<std::size_t>(group);
    if (value >= sections.size()) throw EffectResourceError({Error::malformed, 0});
    return value;
}
} // namespace

EffectResourceError::EffectResourceError(Status status)
    : std::runtime_error("Invalid relative effect resource or Snapshot lifetime"), status_(status) {}

EffectBank decode_effect_bank(rom::GuestView scripts, rom::GuestView images, std::uint32_t bank_id) {
    require(bank_id < 8, scripts);
    EffectBank bank{};
    bank.bank_id = bank_id;
    const auto script_offsets = offsets(scripts, bank.script_table);
    const auto image_offsets = offsets(images, bank.image_table);
    std::vector<Range> image_records{{0, bank.image_table.bytes.size()}};
    // Image descriptors are relocated in place by GAME. Repeated descriptors
    // would relocate twice and are not source aliases; payload aliases are.
    for (const auto offset : image_offsets) {
        EffectImage image{};
        image.header = slice(images, offset, 24);
        const auto& h = image.header;
        const auto frames = h.u32(0);
        require(frames != 0, h);
        image.format = h.u32(4); image.size_code = h.u32(8);
        image.width = h.u32(12); image.height = h.u32(16); image.flags = h.u32(20);
        const auto bytes = frame_bytes(image);
        const auto palettes = image.format == 2 ? ((image.flags & 1) ? 1u : frames) : 0u;
        const auto pointer_count = static_cast<std::size_t>(frames) + palettes;
        require(pointer_count >= frames, h, 0, Error::extent);
        const auto pointers = slice(images, offset + 24, product(pointer_count, 4, h));
        add_record(image_records, {offset, offset + 24 + pointers.bytes.size()}, images);
        for (std::size_t i = 0; i < pointer_count; ++i) {
            const auto payload = slice(images, pointers.u32(i * 4), i < frames ? bytes : 32);
            (i < frames ? image.frames : image.palettes).push_back(payload);
        }
        bank.images.push_back(std::move(image));
    }
    for (const auto& image : bank.images) for (const auto payloads : {std::span(image.frames), std::span(image.palettes)})
        for (const auto& payload : payloads) {
            const auto start = payload.rom_offset - images.rom_offset;
            for (const auto record : image_records)
                require(!overlaps({start, start + payload.bytes.size()}, record), images, start);
        }
    std::vector<std::size_t> distinct = script_offsets;
    std::sort(distinct.begin(), distinct.end());
    distinct.erase(std::unique(distinct.begin(), distinct.end()), distinct.end());
    for (const auto offset : script_offsets) {
        EffectScript script{};
        script.header = slice(scripts, offset, 48);
        const auto& h = script.header;
        script.kind = h.u16(0); script.texture = h.u16(2);
        script.effect_lifetime = h.u16(4); script.particle_lifetime = h.u16(6); script.flags = h.u32(8);
        require(script.kind == 0 || script.kind == 3, h); // selected source emitter kinds, not callback kinds
        require(script.texture < bank.images.size(), h, 2);
        for (std::size_t j = 0; j < script.parameters.size(); ++j) script.parameters[j] = floating(h, 12 + j * 4);
        const auto next = std::upper_bound(distinct.begin(), distinct.end(), offset);
        const auto end = next == distinct.end() ? scripts.bytes.size() : *next;
        require(end > offset && end - offset > 48, scripts, offset, Error::extent);
        decode_script(script, slice(scripts, offset + 48, end - offset - 48),
                      script_offsets.size(), bank.images[static_cast<std::uint8_t>(script.texture)].frames.size());
        bank.scripts.push_back(std::move(script));
    }
    return bank;
}

EffectResources::EffectResources(const rom::Snapshot& snapshot)
    : snapshot_(&snapshot), source_data_(snapshot.canonical().data()) {
    require(snapshot.canonical().size() == rom::us_size && snapshot.identity().size == rom::us_size &&
            snapshot.identity().sha1 == admitted_sha1, {}, 0, Error::invalid_snapshot);
    const rom::GuestView source{0, 0, snapshot.canonical()};
    for (std::size_t i = 0; i < sections.size(); ++i) {
        const auto s = sections[i];
        banks_[i] = decode_effect_bank(slice(source, s.scripts, s.images - s.scripts),
                                      slice(source, s.images, s.end - s.images), s.bank);
        std::vector<rom::GuestView> payloads;
        for (const auto& image : banks_[i].images) {
            payloads.insert(payloads.end(), image.frames.begin(), image.frames.end());
            payloads.insert(payloads.end(), image.palettes.begin(), image.palettes.end());
        }
        std::sort(payloads.begin(), payloads.end(), [](auto a, auto b) { return a.rom_offset < b.rom_offset; });
        auto& spans = installation_[i];
        for (const auto payload : payloads) {
            if (!spans.empty()) {
                auto& last = spans.back();
                const auto end = last.rom_offset + last.bytes.size();
                if (payload.rom_offset <= end) {
                    const auto merged_end = std::max(end, payload.rom_offset + payload.bytes.size());
                    last = slice(source, last.rom_offset, merged_end - last.rom_offset);
                    continue;
                }
            }
            spans.push_back(payload);
        }
    }
}
void EffectResources::require_live() const {
    require(snapshot_->canonical().data() == source_data_ && snapshot_->canonical().size() == rom::us_size,
            {}, 0, Error::invalid_snapshot);
}
const EffectBank& EffectResources::bank(EffectGroup group) const {
    require_live(); return banks_[index(group)];
}
EffectGroup effect_group(std::int32_t id) {
    switch(id) {
    case course::beach_id: return EffectGroup::beach;
    case course::tunnel_id: return EffectGroup::tunnel;
    case course::volcano_id: return EffectGroup::volcano;
    case course::river_id: return EffectGroup::river;
    case course::cave_id: return EffectGroup::cave;
    case course::valley_id: return EffectGroup::valley;
    case course::rainbow_id: return EffectGroup::rainbow;
    default: throw std::runtime_error("Effect resources course is not an original course");
    }
}
std::span<const rom::GuestView> EffectResources::installation_spans(EffectGroup group) const {
    require_live(); return installation_[index(group)];
}
} // namespace isle::menu

#include "effect_bank_resources.hpp"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace {
using namespace isle::menu;
using Bytes = std::vector<std::byte>;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void put32(Bytes& bytes, std::size_t offset, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) bytes.at(offset + i) = std::byte((value >> (24 - i * 8)) & 255);
}
void put16(Bytes& bytes, std::size_t offset, unsigned value) {
    bytes.at(offset) = std::byte(value >> 8); bytes.at(offset + 1) = std::byte(value & 255);
}
struct Image { unsigned format = 4, size = 0, width = 16, height = 16, frames = 1, flags = 1; };
struct Fixture {
    Bytes scripts, images;
    Fixture(const std::vector<std::vector<unsigned>>& programs = {{0xff}},
            const std::vector<Image>& specs = {{}}) {
        scripts.resize(4 + programs.size() * 4);
        put32(scripts, 0, static_cast<std::uint32_t>(programs.size()));
        for (std::size_t i = 0; i < programs.size(); ++i) {
            while (scripts.size() % 4) scripts.push_back(std::byte{0});
            const auto start = scripts.size();
            put32(scripts, 4 + i * 4, static_cast<std::uint32_t>(start));
            scripts.resize(start + 48);
            put16(scripts, start + 6, 7);
            put32(scripts, start + 16, std::bit_cast<std::uint32_t>(1.0f));
            for (const auto byte : programs[i]) scripts.push_back(std::byte(byte));
        }
        images.resize(4 + specs.size() * 4);
        put32(images, 0, static_cast<std::uint32_t>(specs.size()));
        std::vector<std::size_t> descriptors;
        for (std::size_t i = 0; i < specs.size(); ++i) {
            const auto spec = specs[i];
            const auto offset = images.size(); descriptors.push_back(offset);
            put32(images, 4 + i * 4, static_cast<std::uint32_t>(offset));
            const auto palettes = spec.format == 2 ? (spec.flags & 1 ? 1u : spec.frames) : 0u;
            images.resize(offset + 24 + (spec.frames + palettes) * 4);
            put32(images, offset, spec.frames); put32(images, offset + 4, spec.format);
            put32(images, offset + 8, spec.size); put32(images, offset + 12, spec.width);
            put32(images, offset + 16, spec.height); put32(images, offset + 20, spec.flags);
        }
        for (std::size_t i = 0; i < specs.size(); ++i) {
            const auto spec = specs[i];
            const auto palettes = spec.format == 2 ? (spec.flags & 1 ? 1u : spec.frames) : 0u;
            const auto frame_size = (std::size_t(spec.width) * spec.height * (4u << spec.size) + 7) / 8;
            for (unsigned j = 0; j < spec.frames + palettes; ++j) {
                while (images.size() % 8) images.push_back(std::byte{0});
                put32(images, descriptors[i] + 24 + j * 4, static_cast<std::uint32_t>(images.size()));
                images.resize(images.size() + (j < spec.frames ? frame_size : 32), std::byte((i * 17 + j) & 255));
            }
        }
    }
    EffectBank decode(unsigned bank = 0) const { return decode_effect_bank({0, 1000, scripts}, {0, 2000, images}, bank); }
};
template<class F> void rejects(F&& f, Error expected, const char* message) {
    bool caught = false;
    try { f(); } catch (const EffectResourceError& e) { caught = e.status().error == expected; }
    require(caught, message);
}
void relative_identity() {
    Fixture f({{0x01,0xff}, {0x02,0xff}, {0x03,0xff}});
    const auto first = isle::rom::GuestView{0,0,f.scripts}.u32(4);
    const auto second = isle::rom::GuestView{0,0,f.scripts}.u32(8);
    put32(f.scripts, 4, second); put32(f.scripts, 8, first); put32(f.scripts, 12, second);
    const auto bank = f.decode(3);
    require(bank.bank_id == 3 && bank.scripts.size() == 3 && bank.script_table.bytes.size() == 16,
            "source slot/table identity lost");
    require(bank.scripts[0].header.rom_offset == 1000 + second && bank.scripts[1].header.rom_offset == 1000 + first &&
            bank.scripts[2].header.bytes.data() == bank.scripts[0].header.bytes.data() &&
            bank.scripts[2].bytecode.bytes.data() == bank.scripts[0].bytecode.bytes.data(),
            "unordered IDs or repeated script descriptors lost borrowed identity");
    require(bank.scripts[0].instructions[0].opcode == 2 && bank.scripts[1].instructions[0].opcode == 1,
            "sorting extents reordered source script IDs");
    rejects([&] { f.decode(8); }, Error::malformed, "encoded list mistaken for resource slot");
}
void image_shapes_and_aliases() {
    const std::vector<Image> specs{{4,0,48,48,4,1}, {4,1,64,64,2,1}, {3,1,16,16,1,1},
        {0,2,32,32,10,0}, {0,3,32,32,1,1}, {2,0,32,32,2,0}, {2,0,16,16,3,1}};
    Fixture f({{0xff}}, specs);
    const auto bank = f.decode();
    const std::size_t sizes[]{1152,4096,256,2048,4096,512,128};
    for (std::size_t i = 0; i < specs.size(); ++i) {
        const auto& image = bank.images[i];
        require(image.frames.size() == specs[i].frames && image.frames[0].bytes.size() == sizes[i],
                "format/dimension-derived frame extent wrong");
        const auto raw = isle::rom::GuestView{0,0,f.images};
        const auto descriptor = raw.u32(4 + i * 4);
        const auto frame = raw.u32(descriptor + 24);
        require(image.frames[0].rom_offset == 2000 + frame && image.frames[0].bytes.data() == f.images.data() + frame,
                "frame pointer not relative to image-section base");
    }
    require(bank.images[0].palettes.empty() && bank.images[5].palettes.size() == 2 && bank.images[6].palettes.size() == 1 &&
            bank.images[5].palettes[1].bytes.size() == 32, "CI4 palette count/legalized extent wrong");
    const auto view = isle::rom::GuestView{0,0,f.images};
    const auto d = view.u32(4), payload = view.u32(d + 24);
    put32(f.images, d + 28, payload);
    const auto aliased = f.decode();
    require(aliased.images[0].frames[0].bytes.data() == aliased.images[0].frames[1].bytes.data(),
            "payload alias unexpectedly copied or rejected");
    put32(f.images, 8, d);
    rejects([&] { f.decode(); }, Error::malformed, "double-relocated duplicate image descriptor accepted");
}
void packed_operands() {
    // The scalar begins at bytecode+1, deliberately unaligned in host storage.
    Fixture f({{0x81,0xbf,0xa0,0,0,0xa0,0x80,0,0x40,0x20,0,0,0xff}});
    const auto b = f.decode(); const auto& s = b.scripts[0];
    require(s.instructions.size() == 3 && s.instructions[0].offset == 0 && s.instructions[0].size == 5 &&
            s.instructions[1].offset == 5 && s.instructions[1].size == 7 && s.bytecode.bytes.size() == 13,
            "packed float or extended duration framing wrong");
    const isle::rom::GuestView operand{0,s.bytecode.rom_offset + 1,s.bytecode.bytes.subspan(1,4)};
    require(operand.f32(0) == -1.25f, "packed binary32 value changed");
    const auto start = isle::rom::GuestView{0,0,f.scripts}.u32(4) + 48;
    put32(f.scripts, start + 1, 0x7f800000);
    rejects([&] { f.decode(); }, Error::malformed, "nonfinite packed scalar accepted");
    Fixture truncated({{0x81,0,0}});
    rejects([&] { truncated.decode(); }, Error::extent, "truncated float operand accepted");
}
void control_registers() {
    Fixture count0({{0xfa,0,0x00,0xfb,0xff}});
    const auto b = count0.decode();
    require(b.scripts[0].instructions.size() == 4 && b.scripts[0].instructions[2].branch_offset == 2,
            "FA00 did not retain eight-bit 256-iteration counted-loop semantics");
    std::vector<unsigned> long_body{0xfa,0}; long_body.insert(long_body.end(),300,0); long_body.push_back(0xfb); long_body.push_back(0xff);
    Fixture long_finite({long_body});
    require(long_finite.decode().scripts[0].instructions.size() == 303,
            "finite no-wait loop rejected by an arbitrary instruction budget");
    Fixture initial_return({{1,0xfd}});
    const auto initial = initial_return.decode();
    require(initial.scripts[0].bytecode.bytes.size() == 2 && initial.scripts[0].instructions[1].branch_offset == 0,
            "FD did not use initialized return register zero");
    Fixture replace_return({{0xfc,1,0xfc,2,0xfd}});
    require(replace_return.decode().scripts[0].instructions.back().branch_offset == 3,
            "FC behaved as a call stack rather than replacing one register");
    Fixture zero_extended({{0x20,0,0xfd}});
    rejects([&] { zero_extended.decode(); }, Error::malformed, "nominal extended wait with value zero certified a hang");
    Fixture reset_loop({{0xfa,2,0xfa,2,0xfb,0xfb,0xff}});
    rejects([&] { reset_loop.decode(); }, Error::malformed, "single overwritten loop register mistaken for nested finite loops");
    Fixture missing_loop({{0xfb,0xff}});
    rejects([&] { missing_loop.decode(); }, Error::malformed, "uninitialized source loop state admitted");
    Fixture terminal({{0xfe,0xe0}});
    require(terminal.decode().scripts[0].bytecode.bytes.size() == 1, "unreachable padding treated as executed bytecode");
    std::vector<unsigned> wrap_program(65535,0);
    wrap_program.push_back(1); wrap_program.push_back(0xe0);
    Fixture saved_cursor({wrap_program});
    const auto wrapped = saved_cursor.decode();
    require(wrapped.scripts[0].bytecode.bytes.size() == 65536 &&
            wrapped.scripts[0].instructions.size() == 65536,
            "nonzero wait did not narrow saved PC to source u16");
    std::vector<unsigned> loop_program(65534,0);
    loop_program.insert(loop_program.end(),{0xfa,2,0xfb,0xff});
    Fixture saved_loop({loop_program});
    rejects([&] { saved_loop.decode(); }, Error::malformed,
            "FA loop pointer did not narrow at source u16 assignment");
}
void bank_local_references_and_boundaries() {
    Fixture f({{0xaa,0,1,0,0,0xa4,0,0,0xff}, {0xff}});
    require(f.decode().scripts[0].instructions.size() == 3, "valid zero-width script range rejected");
    const auto start = isle::rom::GuestView{0,0,f.scripts}.u32(4) + 48;
    f.scripts[start + 2] = std::byte{2};
    rejects([&] { f.decode(); }, Error::malformed, "zero random range admitted out-of-bank base");
    Fixture frames({{0x40,1,0xbc,0,0,0xff}}, {{4,0,16,16,2,1}});
    require(frames.decode().scripts[0].instructions.size() == 3, "selected-image frame bounds not used");
    auto frame_start = isle::rom::GuestView{0,0,frames.scripts}.u32(4) + 48;
    frames.scripts[frame_start + 3] = std::byte{2};
    rejects([&] { frames.decode(); }, Error::malformed, "zero random frame range admitted invalid base");
    Fixture wrapping_frames({{0xbc,250,20,0xff}}, {{4,0,16,16,256,1}});
    require(wrapping_frames.decode().scripts[0].instructions.size() == 2,
            "source u8 frame wrapping was replaced by an unsigned range ceiling");
    std::vector<Image> many_images(257);
    many_images[256].frames = 2;
    Fixture narrowed_texture({{0x40,1,0xff}}, many_images);
    const auto header = isle::rom::GuestView{0,0,narrowed_texture.scripts}.u32(4);
    put16(narrowed_texture.scripts, header + 2, 256);
    rejects([&] { narrowed_texture.decode(); }, Error::malformed,
            "script frame validation ignored source particle u8 texture selector");
    Fixture crossing(std::vector<std::vector<unsigned>>{{0xa8}, {0xff}});
    rejects([&] { crossing.decode(); }, Error::extent, "operand crossed next distinct record header");
    Fixture payload;
    const auto d = isle::rom::GuestView{0,0,payload.images}.u32(4);
    put32(payload.images, d + 24, static_cast<std::uint32_t>(payload.images.size() - 127));
    rejects([&] { payload.decode(); }, Error::extent, "one-byte escaped frame extent accepted");
    put32(payload.images, d + 24, d);
    rejects([&] { payload.decode(); }, Error::malformed, "payload overlapping relocated descriptor accepted");
    Fixture overflow;
    put32(overflow.images, d + 4, 0); put32(overflow.images, d + 8, 3);
    put32(overflow.images, d + 12, 0x7fffffffu); put32(overflow.images, d + 16, 0x7fffffffu);
    rejects([&] { overflow.decode(); }, Error::extent, "dimension multiplication overflow accepted");
    Bytes tiny(3);
    rejects([&] { decode_effect_bank({0,0,tiny},{0,0,tiny},0); }, Error::extent, "short section accepted");
}
static_assert(!std::is_constructible_v<EffectResources, isle::rom::Snapshot&&>);
static_assert(!std::is_copy_constructible_v<EffectResources>);
} // namespace
int main() {
    try {
        relative_identity(); image_shapes_and_aliases(); packed_operands();
        control_registers(); bank_local_references_and_boundaries();
        std::cout << "Synthetic effect resource contracts passed\n"; return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

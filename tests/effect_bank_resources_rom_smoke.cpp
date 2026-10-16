#include "effect_bank_resources.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

// Opt-in private original-ROM resource consumption only. Never executes FX,
// installs guest code, creates a runtime or claims GPU/application readiness.
namespace {
using namespace isle::menu;
void require(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
void run(const char* filename) {
    auto snapshot = isle::rom::read_snapshot(filename);
    const EffectResources resources(snapshot);
    constexpr std::array groups{EffectGroup::common,EffectGroup::beach,EffectGroup::tunnel,EffectGroup::cave,
        EffectGroup::river,EffectGroup::volcano,EffectGroup::valley,EffectGroup::rainbow,EffectGroup::intro};
    constexpr std::array<std::size_t,9> script_counts{3,5,11,5,8,7,22,8,4}, image_counts{2,6,7,3,4,2,5,3,2};
    constexpr std::array<std::size_t,9> script_starts{0xab5860,0xab85e0,0xabe7a0,0xac6890,0xac8510,0xacf6f0,0xad0e00,0xadd310,0xadec60};
    constexpr std::array<std::size_t,9> image_starts{0xab5980,0xab8780,0xabebd0,0xac6a80,0xac8830,0xacf9a0,0xad1640,0xadd5d0,0xadedf0};
    const auto source = snapshot.canonical();
    auto borrowed = [&](isle::rom::GuestView v) {
        require(v.guest_address == 0 && v.rom_offset <= source.size() && v.bytes.size() <= source.size() - v.rom_offset &&
                v.bytes.data() == source.data() + v.rom_offset, "Resource lost original Snapshot identity");
    };
    std::size_t total_scripts = 0, total_images = 0;
    for (std::size_t i = 0; i < groups.size(); ++i) {
        const auto& bank = resources.bank(groups[i]);
        require(bank.bank_id == (i == 0 ? 3u : 0u) && bank.scripts.size() == script_counts[i] &&
                bank.images.size() == image_counts[i], "Wrong source resource group or active-slot identity");
        require(bank.script_table.rom_offset == script_starts[i] && bank.image_table.rom_offset == image_starts[i],
                "Wrong exact splat source ranges");
        borrowed(bank.script_table); borrowed(bank.image_table);
        total_scripts += bank.scripts.size(); total_images += bank.images.size();
        for (std::size_t j = 0; j < bank.scripts.size(); ++j) {
            const auto& script = bank.scripts[j]; borrowed(script.header); borrowed(script.bytecode);
            require(script.header.rom_offset == script_starts[i] + bank.script_table.u32(4 + j * 4) &&
                    script.bytecode.rom_offset == script.header.rom_offset + 48 && script.texture < bank.images.size(),
                    "Relative script order or selected-image reference lost");
            for (const auto ins : script.instructions)
                require(ins.size && ins.offset < script.bytecode.bytes.size() && ins.size <= script.bytecode.bytes.size() - ins.offset &&
                        script.bytecode.u8(ins.offset) == ins.opcode, "Callable instruction closure is not backed by borrowed bytes");
        }
        const auto spans = resources.installation_spans(groups[i]);
        std::vector<std::byte> packed;
        std::size_t previous_end = 0;
        for (const auto span : spans) {
            borrowed(span);
            require(span.rom_offset > previous_end && !span.bytes.empty(), "Installation extents are not unique merged spans");
            previous_end = span.rom_offset + span.bytes.size();
            packed.insert(packed.end(), span.bytes.begin(), span.bytes.end());
        }
        // A real private resource consumer packs each borrowed union once, then
        // resolves every image/palette reference into that caller-owned storage.
        for (const auto& image : bank.images) {
            borrowed(image.header);
            const auto expected = (std::size_t(image.width) * image.height * (4u << image.size_code) + 7) / 8;
            for (const auto payloads : {std::span(image.frames),std::span(image.palettes)}) for (const auto payload : payloads) {
                borrowed(payload);
                std::size_t packed_base = 0; bool resolved = false;
                for (const auto span : spans) {
                    if (payload.rom_offset >= span.rom_offset && payload.rom_offset - span.rom_offset <= span.bytes.size() &&
                        payload.bytes.size() <= span.bytes.size() - (payload.rom_offset - span.rom_offset)) {
                        const auto offset = packed_base + payload.rom_offset - span.rom_offset;
                        require(std::equal(payload.bytes.begin(),payload.bytes.end(),packed.begin() + offset),
                                "Packed consumer changed original frame/palette bytes");
                        resolved = true; break;
                    }
                    packed_base += span.bytes.size();
                }
                require(resolved, "Borrowed image payload has no installation extent");
            }
            for (const auto frame : image.frames) require(frame.bytes.size() == expected, "Incorrect format-sized borrowed frame");
            require(image.palettes.size() == (image.format == 2 ? ((image.flags & 1) ? 1 : image.frames.size()) : 0),
                    "Source shared/per-frame palette policy changed");
            for (const auto palette : image.palettes) require(palette.bytes.size() == 32, "CI4 TLUT escaped exact16-entry legalization");
        }
    }
    require(total_scripts == 73 && total_images == 34, "Incomplete nine-group resource closure");
    const auto& common = resources.bank(EffectGroup::common);
    const auto& beach = resources.bank(EffectGroup::beach);
    require(common.bank_id == 3 && beach.bank_id == 0 && common.images[0].frames[0].bytes.size() == 4096 &&
            beach.images[2].width == 48 && beach.images[2].frames[0].bytes.size() == 1152,
            "Common+Beach selection still has Title texture limits");
    require(resources.bank(EffectGroup::river).images[3].frames.size() == 10 &&
            resources.bank(EffectGroup::valley).images[0].size_code == 3,
            "Ten-frame/RGBA32 source inventory not consumer-ready");
    const auto title = hydrate(snapshot);
    require(bool(title), "Title Catalog did not migrate to shared decoding");
    const auto& a = title.catalog->effects(); const auto& b = resources.bank(EffectGroup::intro);
    require(a.scripts.size() == b.scripts.size() && a.images.size() == b.images.size(), "Title identity certification disagrees with intro selector");
    for (std::size_t i = 0; i < a.scripts.size(); ++i)
        require(a.scripts[i].bytecode.bytes.data() == b.scripts[i].bytecode.bytes.data() &&
                a.scripts[i].bytecode.bytes.size() == b.scripts[i].bytecode.bytes.size(), "Title duplicated or changed shared bytecode");
    auto moved = std::move(snapshot);
    bool bank_rejected = false, install_rejected = false;
    try { resources.bank(EffectGroup::common); } catch (const EffectResourceError& e) { bank_rejected = e.status().error == Error::invalid_snapshot; }
    try { resources.installation_spans(EffectGroup::intro); } catch (const EffectResourceError& e) { install_rejected = e.status().error == Error::invalid_snapshot; }
    require(bank_rejected && install_rejected && moved.canonical().size() == isle::rom::us_size,
            "Moved Snapshot did not invalidate every selector access");
}
} // namespace
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "usage: effect-bank-resources-rom-smoke <admitted-original.z64>\n"; return EXIT_FAILURE; }
    try { run(argv[1]); std::cout << "Private nine-group effect resources consumed\n"; return EXIT_SUCCESS; }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return EXIT_FAILURE; }
}

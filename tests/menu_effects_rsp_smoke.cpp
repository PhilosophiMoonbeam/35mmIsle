// Private-ROM actual canonical RSP/RDP proof source; no execution by worker.
// Explicit script3 stimulus is not a claim of title callback reachability:
// the first title model has a NULL callback. Script0 is the installed callback.
#include "menu_effects.hpp"
#include "rom_snapshot.hpp"
#include "rsp_fixture.hpp"
#include "rsp_render.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
namespace probe = isle::rsp_probe;
using namespace isle::menu;
constexpr std::uint32_t depth = 0x310000, asset_begin = 0x1b0000, asset_end = 0x1d0000;
constexpr std::uint32_t poison_palette = 0x1a8000;
constexpr std::uint32_t effect_commands = 0x1a0000;
void require(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
void ok(EffectsStatus s) { require(bool(s), "source effects transition rejected"); }
bool world(void*, EffectAttachment, EffectWorld& out) noexcept {
    for (unsigned r = 0; r < 4; ++r) for (unsigned c = 0; c < 4; ++c) out.matrix[r][c] = r == c ? 1.0f : 0.0f;
    return true;
}
struct Wire {
    std::span<std::uint32_t> ram;
    std::uint32_t next = effect_commands;
    void cmd(std::uint32_t a, std::uint32_t b) { ram[next/4] = a; ram[next/4+1] = b; next += 8; }
    void byte(std::uint32_t at, std::uint8_t b) {
        auto& w = ram[at/4]; const unsigned shift = (3-(at&3))*8;
        w = (w & ~(255u << shift)) | (std::uint32_t(b) << shift);
    }
    void fill(std::uint32_t target, std::uint32_t value) {
        cmd(0xe7000000, 0); cmd(0xff10003f, target); cmd(0xed000000, 0x001000c0);
        cmd(0xef300000, 0); cmd(0xf7000000, value); cmd(0xf60fc0bc, 0);
    }
};
struct Capture { std::vector<std::byte> color, z; std::size_t rectangles = 0; };
Capture draw(isle::rsp::CanonicalExecutor& executor, const isle::rom::Snapshot& snapshot,
             const Catalog& catalog, const MenuEffects& effects, bool cold, std::uint8_t poison, bool occluded) {
    auto preparation = executor.begin_preparation();
    if (cold) probe::initialize_fixture_memory(preparation.words(), snapshot.canonical());
    probe::Fixture fixture(preparation.words(), probe::Case::matrix);
    Wire wire{preparation.words()};
    fixture.task.commands_address = effect_commands;
    for (std::uint32_t at = asset_begin; at < asset_end; ++at) wire.byte(at, poison);
    std::array<EffectImageBinding, 2> images{};
    std::array<RamRange, 22> reservations{}; std::size_t reserved = 0;
    std::uint32_t address = asset_begin;
    const auto install = [&](const isle::rom::GuestView& view, std::uint32_t& output) {
        output = address; reservations[reserved++] = {address, view.bytes.size()};
        for (std::size_t k = 0; k < view.bytes.size(); ++k) wire.byte(address + std::uint32_t(k), std::to_integer<std::uint8_t>(view.bytes[k]));
        // Leave a genuinely unowned, poisoned 512-byte tail after every asset.
        address += std::uint32_t((view.bytes.size() + 7) & ~std::size_t(7)) + 512;
        require(address < asset_end, "private smoke asset extent");
    };
    const auto& bank = catalog.effects();
    for (std::size_t t = 0; t < bank.images.size(); ++t) {
        for (std::size_t f = 0; f < bank.images[t].frames.size(); ++f) install(bank.images[t].frames[f], images[t].frames[f]);
        for (std::size_t f = 0; f < bank.images[t].palettes.size(); ++f) install(bank.images[t].palettes[f], images[t].palettes[f]);
    }
    std::vector<std::uint32_t> assets_before(preparation.words().begin()+asset_begin/4, preparation.words().begin()+asset_end/4);
    for (unsigned i = 0; i < 512; ++i) wire.byte(poison_palette+i, poison);
    wire.fill(depth, occluded ? 0 : 0xfffcfffc); wire.fill(probe::color_address, 0x00010001);
    wire.cmd(0xe7000000, 0); wire.cmd(0xfe000000, depth); wire.cmd(0xef000cf0, 0);
    wire.cmd(0xdb060000, 0); // Physical segment0; no parallel/fake GPU path.
    // Poison the full prior TLUT. The producer must overwrite ONLY the reached
    // sixteen entries, and CI4 palette0 output must not depend on tail TMEM.
    wire.cmd(0xfd100000, poison_palette); wire.cmd(0xe8000000, 0);
    wire.cmd(0xf5000100, 0x07000000); wire.cmd(0xe6000000, 0);
    wire.cmd(0xf0000000, 0x073fc000); wire.cmd(0xe7000000, 0);
    EffectCamera camera{};
    camera.projection[0][0] = camera.projection[1][1] = 0.04f;
    camera.projection[2][2] = 0.001f; camera.projection[3][3] = 1;
    camera.viewport_scale = {128, 96, 511}; camera.viewport_translate = {128, 96, 511};
    const auto result = effects.render(camera, {
        preparation.words().subspan(wire.next/4, MenuEffects::max_render_words), images,
        std::span<const RamRange>(reservations).first(reserved)});
    ok(result.status); require(result.particles > 0, "actual effect smoke has no visible particles");
    wire.next += std::uint32_t(result.words*4);
    wire.cmd(0xe9000000, 0); wire.cmd(0xdf000000, 0);
    require(wire.next <= effect_commands+0x6000, "actual smoke command reservation exceeded");
    auto completed = executor.execute(std::move(preparation), fixture.task,
        snapshot.canonical().subspan(probe::boot_rom_begin, probe::boot_bytes));
    require(bool(completed.rsp_result()) && completed.rsp_result().full_syncs == 1, "actual RSP effect packet did not complete");
    Capture capture;
    for (const auto& packet : completed.packets().packets()) {
        const auto words = completed.packets().words().subspan(packet.offset, packet.count);
        if (((words[0]>>24)&63) == 0x24) { require(words.size() == 4, "canonical effect texrect packet shape"); ++capture.rectangles; }
    }
    require(capture.rectangles == result.particles, "actual RSP lost/changed particle texture rectangles");
    const auto ram = completed.rdram();
    capture.color.assign(ram.begin()+probe::color_address, ram.begin()+probe::color_address+64*48*2);
    capture.z.assign(ram.begin()+depth, ram.begin()+depth+64*48*2);
    for (std::uint32_t at = asset_begin; at < asset_end; ++at) {
        const auto expected = std::uint8_t(assets_before[(at-asset_begin)/4] >> ((3-(at&3))*8));
        require(ram[at^3u] == std::byte(expected), "effect rendering wrote assets/unowned adjacent RAM");
    }
    for (std::size_t i = 0; i < capture.z.size(); ++i)
        require(capture.z[i] == (occluded ? std::byte(0) : std::byte((i&1) ? 0xff : 0xfc)),
                "translucent source mode incorrectly updated depth");
    return capture;
}
void run(const isle::rom::Snapshot& snapshot, const Catalog& catalog) {
    isle::rsp::CanonicalExecutor executor(true, {1000000, probe::capture_words});
    bool cold = true;
    for (float argument : {1.0f, 4.0f}) {
        isle_rng rng{0x12345678}; MenuEffects effects(catalog, rng); ok(effects.status());
        for (unsigned i = 0; i < 10; ++i) require(effects.animation_event({animation::InstanceHandle{1,1,2}, 0}, 0, argument).spawned, "source smoke effect spawn");
        for (unsigned tick = 0; tick < 3; ++tick) { ok(effects.update_particles()); ok(effects.update_effects(world, nullptr)); }
        const auto first = draw(executor, snapshot, catalog, effects, cold, 0x5a, false); cold = false;
        const auto second = draw(executor, snapshot, catalog, effects, false, 0xa5, false);
        require(first.color == second.color && first.z == second.z, "CI4 output depended on unowned palette tail/prior TMEM");
        const auto blocked = draw(executor, snapshot, catalog, effects, false, 0x3c, true);
        require(first.color != blocked.color, "actual translucent particles ignored primitive Z test or painted no color");
        for (std::size_t i = 0; i < blocked.color.size(); ++i)
            require(blocked.color[i] == std::byte((i&1) ? 0 : 1), "occluded source particles wrote framebuffer");
        for (unsigned tick = 0; tick < 5; ++tick) ok(effects.update_particles());
        const auto animated = draw(executor, snapshot, catalog, effects, false, 0xc3, false);
        require(first.color != animated.color, "actual particle animation/physics did not change rendered colors");
        effects.detach_instance(animation::InstanceHandle{1,1,2});
        for (unsigned tick = 0; tick < 80; ++tick) ok(effects.update_particles());
        require(effects.particle_count() == 0 && effects.effect_count() == 0, "actual smoke teardown retained pools");
        require(executor.quiescent(), "actual GPU leases not retired before runtime/catalog teardown");
    }
    std::cout << "menu_effect_actual_rsp_rdp=passed scripts=0,3 pal16_tail_independent=yes primitive_z_test=yes "
                 "translucent_depth_unchanged=yes animation_color_change=yes teardown=yes Original_parity=unqualified\n";
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 2, "expected one explicit private ROM path");
        const auto snapshot = isle::rom::read_snapshot(std::filesystem::path(argv[1]));
        const auto hydrated = isle::menu::hydrate(snapshot); require(bool(hydrated), "menu resource admission failed");
        run(snapshot, *hydrated.catalog); return 0;
    } catch (const std::filesystem::filesystem_error&) { std::cerr << "private ROM operation failed\n"; }
      catch (const std::exception& e) { std::cerr << e.what() << '\n'; }
    return 1;
}

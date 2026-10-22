#ifndef ISLE_TEST_SOURCE_CAMERA_CONTRACT_HPP
#define ISLE_TEST_SOURCE_CAMERA_CONTRACT_HPP

#include "scene_graphics.hpp"
#include "scene_storage.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <span>
#include <string>
#include <stdexcept>
#include <thread>

namespace isle::test {
namespace source_camera_detail {
namespace g = ::isle::scene::graphics;
namespace r = ::isle::scene::render;
inline void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
inline void native(IsleGameResult result) {
    require(result == ISLE_GAME_OK, "source camera contract native operation rejected");
}
inline constexpr std::array<std::uint16_t, 8> original{648, 488, 511, 0, 640, 480, 511, 0};
inline constexpr sprite::RamRange arena{0x120000, 0x1000};
inline constexpr std::uint32_t primary_begin = arena.address + 0x100;
inline constexpr std::uint32_t secondary_begin = arena.address + 0x400;
inline constexpr std::uint32_t primary_end = secondary_begin;
inline constexpr std::uint32_t secondary_end = arena.address + arena.size;

struct Commands {
    std::array<std::uint32_t, 128> words{};
    std::size_t size = 0;
    std::uint32_t viewport_address = 0;
    std::array<std::uint16_t, 8> viewport{};
    unsigned scissors = 0, fills = 0;
    sprite::Bounds final_scissor{};
};
inline bool same(sprite::Bounds a, sprite::Bounds b) {
    return a.left == b.left && a.top == b.top && a.right == b.right && a.bottom == b.bottom;
}
inline sprite::Bounds rectangle(std::uint32_t a, std::uint32_t b, bool fill, bool inclusive = true) {
    const auto xy = [](std::uint32_t value, unsigned shift) { return int((value >> shift) & 0xfffu); };
    const int ax = xy(a, 12), ay = xy(a, 0), bx = xy(b, 12), by = xy(b, 0);
    require(((ax | ay | bx | by) & 3) == 0, "camera rectangle lost integer-pixel bounds");
    const sprite::Bounds result = fill ? sprite::Bounds{bx / 4, by / 4, ax / 4 + int(inclusive), ay / 4 + int(inclusive)} :
        sprite::Bounds{ax / 4, ay / 4, bx / 4, by / 4};
    require(result.left >= 0 && result.top >= 0 && result.right <= 320 && result.bottom <= 240 &&
            result.left < result.right && result.top < result.bottom,
            "camera emitted a scissor/fill outside its physical target");
    return result;
}
inline Commands inspect(std::span<const std::uint32_t> ram, std::uint32_t begin, std::uint32_t end) {
    Commands result;
    require(begin <= end && end <= ram.size_bytes() && !((begin | end) & 7u) &&
            (end - begin) / 4 <= result.words.size(), "camera inspection escaped bounded arena");
    result.size = (end - begin) / 4;
    unsigned viewports = 0;
    for (std::size_t i = 0; i < result.size; i += 2) {
        const auto a = ram[begin / 4 + i], b = ram[begin / 4 + i + 1];
        result.words[i] = a; result.words[i + 1] = b;
        if (a == 0xdc080008u) {
            ++viewports; result.viewport_address = b & 0x1fffffffu;
            require(!(result.viewport_address & 7u) && result.viewport_address <= ram.size_bytes() - 16,
                    "camera viewport reference escaped actual RAM");
            for (unsigned lane = 0; lane < result.viewport.size(); ++lane) {
                const auto packed = ram[result.viewport_address / 4 + lane / 2];
                result.viewport[lane] = static_cast<std::uint16_t>(packed >> ((lane & 1u) ? 0 : 16));
            }
            require(result.viewport == original, "camera upload changed an original 16-bit viewport lane");
        }
        if ((a >> 24) == 0xedu) { ++result.scissors; result.final_scissor = rectangle(a, b, false); }
        if ((a >> 24) == 0xf6u) {
            ++result.fills;
            require(same(rectangle(a, b, true), {0, 0, 320, 240}),
                    "overscan camera clear did not cover exactly its bounded target");
        }
    }
    require(viewports == 1, "camera list did not emit exactly one actual viewport reference");
    return result;
}
struct Probe {
    sprite::Bounds expected;
    Commands primary, secondary;
    unsigned dispatches = 0, completions = 0, rdp_scissors = 0, rdp_fills = 0;
    std::uint64_t token = 0;
    rsp::Result execution{};
    static bool control(void* context, g::SourceControl& source) {
        auto& self = *static_cast<Probe*>(context);
        require(++self.dispatches == 1, "camera contract dispatched more than once");
        // The global pre-render call is a real reserved, callable empty native list.
        // Primary/secondary bodies below are emitted by Frame, never manufactured.
        source.with_ram(nullptr,+[](void*,std::span<std::uint32_t> ram) {
            graphics::Memory memory(ram);
            memory.word(arena.address,0xdf000000u);memory.word(arena.address+4,0);
        });
        struct Inspect { Commands& commands;std::uint32_t begin,end; };
        const auto inspect_and_end=+[](void* raw,std::span<std::uint32_t> ram) {
            auto& value=*static_cast<Inspect*>(raw);
            value.commands=inspect(ram,value.begin,value.end);
            graphics::Memory memory(ram);
            memory.word(value.end,0xdf000000u);memory.word(value.end+4,0);
        };
        const auto end_primary=source.initialize_camera_into(primary_begin,primary_end-8);
        Inspect primary{self.primary,primary_begin,end_primary};
        source.with_ram(&primary,inspect_and_end);
        source.call({primary_begin,(end_primary+8-primary_begin)/4});
        const auto end_secondary=source.restore_camera(2,secondary_begin,secondary_end-8);
        Inspect secondary{self.secondary,secondary_begin,end_secondary};
        source.with_ram(&secondary,inspect_and_end);
        source.call({secondary_begin,(end_secondary+8-secondary_begin)/4});
        require(self.primary.fills == 2 && self.secondary.fills == 0,
                "camera primary clears or secondary no-clear contract changed");
        require(self.primary.viewport_address == self.secondary.viewport_address &&
                same(self.primary.final_scissor, self.expected) && same(self.secondary.final_scissor, self.expected),
                "secondary restoration lost the current viewport reference or final pass scissor");
        require(self.primary.scissors == 2 && self.secondary.scissors ==
                (same(self.expected, {0, 0, 320, 240}) ? 1u : 2u),
                "secondary emitted a missing or redundant pass scissor");
        // FILL-cycle scissoring has four-pixel granularity. A one-cycle solid
        // rectangle tests exact restoration without changing the expected edges.
        // Its lower/right coordinates are exclusive, unlike the primary clears.
        source.command(0xe7000000u, 0);
        source.command(0xe3000a01u, 0);
        source.command(0xe200001cu, 0);
        source.command(0xfcffffffu, 0xfffdf6fbu); // Both cycles select primitive RGBA.
        source.command(0xfa000000u, 0xffffffffu);
        source.command(0xf65003c0u, 0);
        source.command(0xe7000000u, 0);
        return true;
    }
    static void complete(void* context, rsp::CanonicalExecutor::MemoryLease& lease, std::uint32_t) {
        auto& self = *static_cast<Probe*>(context);
        require(++self.completions == 1, "camera task acquired multiple completed mappings");
        self.execution = lease.rsp_result();
        require(bool(self.execution) && self.execution.full_syncs == 1 && self.execution.dma_reads &&
                self.execution.packets, "camera task did not execute the real canonical RSP/RDP path");
        const auto& packets = lease.packets();
        sprite::Bounds final_scissor{};
        unsigned cycle_type=0;
        for (const auto packet : packets.packets()) {
            const auto words = packets.words().subspan(packet.offset, packet.count);
            require(!words.empty(), "camera task produced an empty RDP packet");
            const auto op = (words[0] >> 24) & 63u;
            if (op==0x2fu) cycle_type=(words[0]>>20)&3u;
            if (op == 0x2du || op == 0x36u) {
                require(words.size() >= 2, "camera task produced a truncated rectangle packet");
                require(op!=0x36u || cycle_type==0 || cycle_type==3, "camera rectangle used an unexpected cycle type");
                const auto bounds = rectangle(words[0], words[1], op == 0x36u, cycle_type==3);
                if (op == 0x2du) { ++self.rdp_scissors; final_scissor = bounds; }
                else ++self.rdp_fills;
            }
        }
        require(self.rdp_fills == 3 && self.rdp_scissors >= self.primary.scissors + self.secondary.scissors &&
                same(final_scissor, self.expected), "executed RDP stream lost bounded clears or restored scissor");
        require(!lease.postprocess_active(), "camera evidence retained writable GPU memory");
    }
};
struct Join {
    std::unique_ptr<g::Frame>& frame;
    ~Join() {
        if (!frame) return;
        while (frame->busy()) {
            const auto result = frame->poll();
            if (result == g::Poll::pending) std::this_thread::yield();
            else if (frame->busy()) std::terminate();
        }
        if (!frame->retire()) std::terminate();
    }
};
inline void save(const std::filesystem::path& path, std::span<const std::byte> bytes) {
    require(!std::filesystem::exists(path), "camera evidence file already exists");
    std::ofstream stream(path, std::ios::binary);
    stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    stream.close(); require(bool(stream), "camera completed RGBA evidence write failed");
}
inline void record(std::ostream& stream, const char* name, const Commands& commands) {
    stream << "# " << name << " viewport=0x" << std::hex << commands.viewport_address << std::dec << " lanes=";
    for (const auto lane : commands.viewport) stream << lane << ',';
    stream << " scissors=" << commands.scissors << " fills=" << commands.fills << '\n';
    for (std::size_t i = 0; i < commands.size; i += 2)
        stream << name << '\t' << i / 2 << '\t' << std::hex << std::setw(8) << std::setfill('0') <<
            commands.words[i] << '\t' << std::setw(8) << commands.words[i + 1] << std::dec << '\n';
}
} // namespace source_camera_detail

// Independent native Scene API/GPU regression, not a Root fixture or Source injection.
// GAME render.c:2073-2092,2721-2741 uploads the original viewport and separately
// divides translate/scale by four before clipping its scissor to the VI target.
inline void source_camera_contract(const rom::Snapshot& snapshot, const std::filesystem::path& new_output_directory) {
    namespace d = source_camera_detail;
    d::require(!new_output_directory.empty() && !std::filesystem::exists(new_output_directory) &&
        std::filesystem::is_directory(new_output_directory.parent_path()),
        "camera contract requires a new output directory with an existing parent");
    d::require(std::filesystem::create_directory(new_output_directory), "camera contract output already exists");
    scene::Storage storage;
    d::native(storage.create());
    auto* game = storage.game();
    IsleGameObject* object = nullptr;
    IsleGameCamera* camera = nullptr;
    d::native(isle_game_add_object(game, 7, 9, 9, nullptr, nullptr, &object));
    d::native(isle_game_attach_camera(game, object, &camera));
    const IsleGameViewport viewport{{648, 488, 511, 0}, {640, 480, 511, 0}};
    d::native(isle_game_camera_set_viewport(game, camera, &viewport));
    auto made = d::g::Frame::create(snapshot, {true});
    d::require(bool(made), made.message.data());
    d::Join join{made.frame};
    auto& frame = *made.frame;
    d::require(frame.reserve_render_capacity({0,1,1,1}),frame.message());
    const std::array reservations{d::arena};
    d::r::Resources resources;
    resources.identity = 1; resources.reservations = reservations; resources.pre_render = d::arena.address;
    std::array<d::Probe, 2> probes{};
    probes[0].expected = {12, 10, 308, 230};
    probes[1].expected = {0, 0, 320, 240};
    constexpr std::array names{"narrow", "target"};
    for (std::size_t index = 0; index < probes.size(); ++index) {
        auto& probe = probes[index];
        d::r::Pass pass;
        pass.kind = d::r::Kind::clear; pass.camera = camera; pass.flags = 3;
        pass.background_rgba = 0x102030ffu; pass.scissor = probe.expected;
        pass.control = d::Probe::control; pass.userdata = &probe;
        d::require(frame.begin({game, resources, {&pass, 1}, {}}, {static_cast<unsigned>(index)},
                              {&probe, d::Probe::complete}), frame.message());
        probe.token = frame.submission();
        d::require(probe.token && frame.submission_context() == index, "camera task has no actual execution token/context");
        d::g::Poll result;
        do { result = frame.poll(); if (result == d::g::Poll::pending) std::this_thread::yield(); }
        while (result == d::g::Poll::pending);
        d::require(result == d::g::Poll::completed, frame.message());
        d::require(frame.quiescent() && !frame.busy() && frame.submission() == 0 && probe.completions == 1,
                   "camera task did not join actual execution/readback");
        const auto completed = frame.completed();
        d::require(bool(completed) && completed.width == 320 && completed.height == 240 && completed.stride == 1280 &&
                   completed.rgba.size() == 320 * 240 * 4, "camera task has no full completed RGBA target");
        for (int y = 0; y < 240; ++y) for (int x = 0; x < 320; ++x) {
            const bool inside = x >= probe.expected.left && x < probe.expected.right &&
                                y >= probe.expected.top && y < probe.expected.bottom;
            const std::array<unsigned, 4> expected = inside ? std::array<unsigned, 4>{255, 255, 255, 255} :
                std::array<unsigned, 4>{16, 33, 49, 255};
            const auto offset = static_cast<std::size_t>((y * 320 + x) * 4);
            for (std::size_t channel = 0; channel < expected.size(); ++channel) {
                const auto actual=std::to_integer<unsigned>(completed.rgba[offset+channel]);
                if (actual!=expected[channel])
                    throw std::runtime_error("real post-restore raster mismatch: case="+std::string(names[index])+
                        " x="+std::to_string(x)+" y="+std::to_string(y)+" channel="+std::to_string(channel)+
                        " expected="+std::to_string(expected[channel])+" actual="+std::to_string(actual));
            }
        }
        d::save(new_output_directory / (std::string(names[index]) + ".rgba8"), completed.rgba);
    }
    // A physical address is not continuing mapping authority. Reject a task
    // boundary while a scoped writer is active, unwind it, and leave neither
    // a real submission nor a presentation/lease behind.
    {
        auto rejected=d::g::Frame::create(snapshot,{true});
        d::require(bool(rejected),rejected.message.data());
        d::Join rejected_join{rejected.frame};
        d::require(rejected.frame->reserve_render_capacity({0,1,1,1}),rejected.frame->message());
        bool continued=false;
        d::r::Pass pass;
        pass.kind=d::r::Kind::clear;pass.camera=camera;pass.flags=3;pass.userdata=&continued;
        pass.control=+[](void* raw,d::g::SourceControl& control) {
            control.command(0xe7000000u,0);
            struct Attempt { d::g::SourceControl& control;bool& continued; };
            Attempt attempt{control,*static_cast<bool*>(raw)};
            control.with_ram(&attempt,+[](void* raw,std::span<std::uint32_t>) {
                auto& value=*static_cast<Attempt*>(raw);
                value.control.process_commands(false);
                value.continued=true;
            });
            return true;
        };
        d::require(!rejected.frame->begin({game,resources,{&pass,1},{}},{0})&&!continued&&
            rejected.frame->error()==d::g::Error::preparation&&rejected.frame->task_receipts().empty()&&
            !rejected.frame->busy()&&rejected.frame->quiescent()&&!rejected.frame->completed()&&
            !rejected.frame->submission()&&!rejected.frame->presentation_submission(),
            "active mapped writer escaped its task boundary rejection or retained ownership");
    }
    d::require(frame.retire() && frame.quiescent(), "camera Frame retirement failed");
    made.frame.reset();
    d::native(isle_game_delete_object(game, object));
    IsleGameCounts counts{};
    d::native(isle_game_counts(game, &counts));
    d::require(!counts.objects && !counts.processes && !counts.sprites && !counts.cameras && !counts.matrices &&
               !counts.models && !counts.model_nodes, "camera native teardown retained live parties");
    d::native(isle_game_status(game));
    d::native(storage.destroy());
    d::require(storage.game() == nullptr, "camera Storage failed to join its native owner");
    // Publish the completed contract report only after both GPU contexts and native owners joined.
    const auto report_path = new_output_directory / "contract.tsv";
    d::require(!std::filesystem::exists(report_path), "camera contract report already exists");
    std::ofstream report(report_path);
    report << "# independent-native-camera real-Frame canonical-RSP-RDP source=GAME-render.c:2073-2092,2721-2741\n";
    for (std::size_t index = 0; index < probes.size(); ++index) {
        const auto& probe = probes[index];
        report << "# case=" << names[index] << " context=" << index << " token=" << probe.token <<
            " dma_reads=" << probe.execution.dma_reads << " packets=" << probe.execution.packets <<
            " full_syncs=" << probe.execution.full_syncs << " rdp_scissors=" << probe.rdp_scissors <<
            " rdp_fills=" << probe.rdp_fills << " all_pixels_checked=76800\n";
        d::record(report, "primary", probe.primary); d::record(report, "secondary", probe.secondary);
    }
    report << "# passed=all8-original-viewport-lanes bounded-camera-scissors-and-clears narrow-restoration no-secondary-clears "
              "no-redundant-target-scissor real-post-restore-raster actual-poll-readback-retire native-zero-party-joined-destroy\n"
              "# boundary=independent-native-camera-component-not-Root-scenario-qualification\n";
    report.close(); d::require(bool(report), "camera completed contract report write failed");
}
} // namespace isle::test
#endif

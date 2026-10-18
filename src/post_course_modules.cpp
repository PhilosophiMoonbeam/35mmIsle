#include "post_course_modules.hpp"
#include "application_manager.hpp"
#include <bit>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace isle::post_course {
namespace {
std::uint32_t word(std::span<const std::byte> bytes,std::size_t at) {
    if (at > bytes.size() || bytes.size()-at < 4)
        throw std::invalid_argument("post-course initialized module word exceeds ROM");
    return (std::uint32_t(bytes[at])<<24) | (std::uint32_t(bytes[at+1])<<16) |
           (std::uint32_t(bytes[at+2])<<8) | std::uint32_t(bytes[at+3]);
}
}
void Modules::observe_world(const application::Resources& resources) {
    const auto next = resources.overlay_generation(ISLE_APPLICATION_OVERLAY_WORLD);
    if (next < world_generation)
        throw std::invalid_argument("WORLD overlay generation moved backwards");
    if (!next) return;
    if (resources.overlay(ISLE_APPLICATION_OVERLAY_WORLD).bytes.empty())
        throw std::invalid_argument("WORLD module overlay authority is unavailable");
    if (next != world_generation) {
        world = ::isle::world::ModuleState{};
        world_generation = next;
    }
}
void Modules::observe_window(const rom::Snapshot& snapshot,const application::Resources& resources) {
    const auto next_window = resources.overlay_generation(ISLE_APPLICATION_OVERLAY_WINDOW);
    if (!next_window || next_window < window_generation)
        throw std::invalid_argument("Window module requires an actual monotonic overlay load");
    const auto bytes = snapshot.canonical();
    if (bytes.size() != rom::us_size || resources.overlay(ISLE_APPLICATION_OVERLAY_WINDOW).bytes.empty())
        throw std::invalid_argument("Window module overlay authority is unavailable");
    if (rom_binding && rom_binding != bytes.data())
        throw std::invalid_argument("post-course module Snapshot moved");
    if (!rom_binding) {
        // GAME47330.c gLevelNames: ROM57A74 seven BE pointers into the exact
        // rodata5AD90..5ADD0, guest800AF3E0..800AF420. Keep genuine Source bytes.
        std::array<std::string_view,7> names{};
        for (std::size_t i=0;i<names.size();++i) {
            const auto guest = word(bytes,0x57a74+i*4);
            if (guest < 0x800af3e0u || guest >= 0x800af420u)
                throw std::invalid_argument("original course-name pointer exceeds declared table");
            const auto offset = std::size_t(0x5ad90u+guest-0x800af3e0u);
            const auto* start = reinterpret_cast<const char*>(bytes.data()+offset);
            const auto* end = static_cast<const char*>(std::memchr(start,0,0x5add0-offset));
            if (!end) throw std::invalid_argument("original course name lacks declared terminator");
            names[i] = {start,static_cast<std::size_t>(end-start)};
        }
        course_names = names;
        rom_binding = bytes.data();
    }
    if (next_window != window_generation) {
        /* GAME window/847B60.c6-14,8486E0.c6-10 at df8815e:
         * guest803A6668/ROM879E18; BC/C0/C4/C8 map879E6C/70/74/78.
         * Numeric initialized data only; no SDK or extracted payload compiled. */
        const auto scale = std::bit_cast<float>(word(bytes,0x879e18));
        const IsleCameraCheckMusicState initial{
            std::bit_cast<std::int32_t>(word(bytes,0x879e6c)),
            std::bit_cast<std::int32_t>(word(bytes,0x879e70)),
            std::bit_cast<std::int32_t>(word(bytes,0x879e74)),nullptr};
        if (!std::isfinite(scale) || scale <= 0 || word(bytes,0x879e78) != 0)
            throw std::invalid_argument("invalid original Window module initialization");
        preview_scale = scale;
        music = initial;
        window_generation = next_window;
    }
}
}

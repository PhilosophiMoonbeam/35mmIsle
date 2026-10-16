#ifndef ISLE_OAKS_LAB_SCENE_PRIVATE_SMOKE_HPP
#define ISLE_OAKS_LAB_SCENE_PRIVATE_SMOKE_HPP
#include "oaks_lab_scene.hpp"
#include <string>
namespace isle::oaks_lab::private_smoke {
enum class Scenario { first_meet_course, reentry_course, save_decline, save_keep_going, save_title, save_failure, report_request, album_request, retire_during_dialogue };
struct Frame {
    std::span<const std::byte> rgba;
    // Zero/empty means source interval(1,2) performed no draw this update.
    // Nonzero means an actual new canonical RSP/RDP GPU/readback terminal.
    std::uint64_t actual_terminal_ordinal=0;
};
struct Hooks {
    void* owner=nullptr;
    // The actual scheduler publishes this software-owned physical-zero sample,
    // runs one source OM update, independently services real persistent audio,
    // and executes due render_input passes through canonical RSP/RDP. Complete
    // all actual GPU/readback/Producer borrowers before returning a frame.
    bool (*advance)(void*,const game_input::HostFrame&,Frame&) noexcept=nullptr;
    bool (*fence)(void*) noexcept=nullptr;
};
// Requires Main's real State/Storage/Services/audio/graphics owners and admitted
// private resources. No stand-in executor, source-state injection or fake scene
// result. Save-failure requires Main to arrange a real filesystem publication
// failure/evidence condition; the consumer never fabricates CommitResult.
// course is an actual unlocked source0..6 choice, never a backend replacement.
bool exercise(Scene&,game::State&,Hooks,Scenario,unsigned course,unsigned maximum_updates,std::string& error);
} // namespace isle::oaks_lab::private_smoke
#endif

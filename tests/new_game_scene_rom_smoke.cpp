#include "new_game_scene_private_smoke.hpp"
#include <algorithm>

namespace isle::new_game::private_smoke {
namespace {
struct Driver {
    Scene& scene;
    Hooks hooks;
    game_input::HostFrame sample{};
    std::string& error;
    unsigned remaining;
    std::uint64_t ordinal=0;
    bool saw_grid_pixels=false, saw_card_pixels=false;
    bool step() {
        if (!remaining--) { error="actual name UI scheduler update limit"; return false; }
        Frame frame;
        if (!hooks.advance(hooks.owner,sample,frame)) { error="actual name RSP/RDP/audio/controller step failed"; return false; }
        if (scene.failed()) { error=scene.error(); return false; }
        if (frame.actual_terminal_ordinal == 0) {
            if (!frame.rgba.empty()) { error="nondraw name update returned invented frame pixels"; return false; }
        } else {
            if (frame.actual_terminal_ordinal<=ordinal || frame.rgba.size()!=320*240*4) {
                error="name draw requires a new actual completed canonical RGBA frame"; return false;
            }
            ordinal=frame.actual_terminal_ordinal;
            const auto varied=[&](int left,int top,int right,int bottom) {
                const auto origin=static_cast<std::size_t>(top*320+left)*4;
                for (int y=top;y<bottom;++y) for (int x=left;x<right;++x) {
                    const auto pos=static_cast<std::size_t>(y*320+x)*4;
                    if (!std::equal(frame.rgba.begin()+origin,frame.rgba.begin()+origin+3,frame.rgba.begin()+pos)) return true;
                }
                return false;
            };
            if (scene.observation().controls_ready) {
                saw_grid_pixels|=varied(23,20,83,220);
                saw_card_pixels|=varied(96,21,280,120);
            }
        }
        return true;
    }
    bool pulse(std::uint16_t buttons,std::int8_t x=0,std::int8_t y=0) {
        sample[0].buttons=buttons; sample[0].stick_x=x; sample[0].stick_y=y;
        if (!step()) return false;
        sample[0].buttons=0; sample[0].stick_x=sample[0].stick_y=0;
        return step();
    }
};
bool flag(game::State& state,bool expected,std::string& error) {
    const auto snapshot=state.save()->snapshot();
    if (!snapshot) { error="name smoke persistent snapshot failed"; return false; }
    const auto actual=snapshot.value.player_flag(16);
    if (!actual || actual.value!=expected) { error="name PFID16 crossed wrong source completion boundary"; return false; }
    return true;
}
}
bool exercise(Scene& scene,game::State& state,Hooks hooks,Scenario scenario,unsigned maximum,std::string& error) {
    if (!hooks.advance || !maximum || !state.initialized() || !flag(state,false,error)) {
        if (error.empty()) error="name smoke requires actual initialized fresh profile and scheduler";
        return false;
    }
    const auto initial=state.save()->snapshot();
    if (!initial) { error="name smoke initial source snapshot failed"; return false; }
    const auto generation=initial.value.generation();
    const auto notices=state.scene_notices();
    Driver driver{scene,hooks,{},error,maximum};
    driver.sample[0].connection=game_input::Connection::present;
    driver.sample[0].identity=0x4e414d45;
    if (!scene.start()) { error=scene.error(); return false; }
    while (!scene.observation().controls_ready || !driver.saw_grid_pixels || !driver.saw_card_pixels)
        if (!driver.step()) return false;
    if (scene.complete_scene() || scene.completed() || !flag(state,false,error)) { error="selecting UI completed without actualEnd"; return false; }
    if (scenario==Scenario::abort_while_selecting) {
        scene.retire();
        if (scene.complete_scene() || !flag(state,false,error)) { error="aborted name scene fabricated completion"; return false; }
        const auto after=state.save()->snapshot();
        if (!after || after.value.generation()!=generation || !std::equal(after.value.player_name().begin(),after.value.player_name().end(),initial.value.player_name().begin())) {
            error="selection teardown changed full source save name or durable generation"; return false;
        }
        return true;
    }
    if (scenario==Scenario::seven_and_backspace) {
        for (unsigned i=0;i<7;++i) if (!driver.pulse(0x8000)) return false;
        auto view=scene.observation();
        if (view.name_bytes!=14 || view.row!=19 || view.column!=2) { error="actual controller seven-character autoEnd failed"; return false; }
        if (!driver.pulse(0x4000)) return false;
        view=scene.observation();
        if (view.name_bytes!=12 || view.exit_requested) { error="actual B did not backspace without cancellation"; return false; }
    } else {
        if (!driver.pulse(0x4000)) return false;
        if (scene.observation().row!=19 || scene.observation().column!=2) { error="actual empty B did not jumpEnd"; return false; }
    }
    if (!flag(state,false,error)) return false;
    driver.sample[0].buttons=0x8000;
    if (!driver.step()) return false;
    if (!scene.observation().confirming || scene.observation().confirmed || !flag(state,false,error)) {
        error="actual End skipped its source one-update confirmation wait"; return false;
    }
    const auto during=state.save()->snapshot();
    if (!during || !std::equal(during.value.player_name().begin(),during.value.player_name().end(),initial.value.player_name().begin())) {
        error="source name copied before actual confirmation wait"; return false;
    }
    driver.sample[0].buttons=0;
    if (!driver.step()) return false;
    if (!scene.observation().confirmed || !flag(state,false,error)) { error="actual confirmation wait did not commit RAM-only name"; return false; }
    const auto confirmed=scene.observation();
    while (!scene.exit_requested()) if (!driver.step()) return false;
    if (!flag(state,false,error)) return false;
    // Last advance already finished actual graphics/readback/producer borrowers;
    // complete_scene joins UI continuations, then performs the source PFID16.
    if (!scene.complete_scene() || !scene.completed() || !flag(state,true,error)) { error="actual post-scene completion failed"; return false; }
    std::int32_t result=-1;
    if (!scene.take_result(result) || result!=11 || scene.take_result(result) || scene.complete_scene()) {
        error="source Oak11 result was not emitted exactlyonce"; return false;
    }
    const auto after=state.save()->snapshot();
    if (!after || after.value.generation()!=generation || state.scene_notices()!=notices ||
        !std::equal(after.value.player_name().begin(),after.value.player_name().end(),reinterpret_cast<const std::byte*>(confirmed.name.data()))) {
        error="name result lost full16 bytes/notices or performed implicit durable commit"; return false;
    }
    return true;
}
bool reject_raster_extent(Scene& scene,game::State& state,Hooks hooks,unsigned maximum,std::string& error) {
    if (!hooks.advance || !hooks.fence || !maximum || !state.initialized()) {
        error="name error regression requires real services, scheduler and graphics fence"; return false;
    }
    const auto initial=state.save()->snapshot();
    if (!initial) { error="name error regression initial snapshot"; return false; }
    const auto before_flag=initial.value.player_flag(16);
    const auto notices=state.scene_notices();
    if (!before_flag) { error="name error regression initial PFID16"; return false; }
    game_input::HostFrame sample{};
    sample[0].connection=game_input::Connection::present; sample[0].identity=0x4e414d45;
    scene.start();
    for (unsigned update=0;update<maximum && !scene.failed();++update) {
        Frame frame;
        if (!hooks.advance(hooks.owner,sample,frame) && !scene.failed()) {
            error="name error regression failed outside actual scene admission"; return false;
        }
    }
    if (!scene.failed() || !scene.error()[0]) { error="invalid name raster extent did not retain actual scene error"; return false; }
    const std::string first_error=scene.error();
    if (!hooks.fence(hooks.owner)) { error="actual graphics error retirement fence failed"; return false; }
    scene.retire();
    std::int32_t result=-1;
    if (scene.complete_scene() || scene.take_result(result) || first_error!=scene.error()) {
        error="name admission error fabricated completion or replaced first error"; return false;
    }
    const auto after=state.save()->snapshot();
    if (!after) { error="name error regression final snapshot"; return false; }
    const auto after_flag=after.value.player_flag(16);
    if (!after_flag || after_flag.value!=before_flag.value || after.value.generation()!=initial.value.generation() ||
        state.scene_notices()!=notices || !std::equal(after.value.player_name().begin(),after.value.player_name().end(),initial.value.player_name().begin())) {
        error="name allocation error changed source name/PFID16/notices/durable generation"; return false;
    }
    return true;
}
} // namespace isle::new_game::private_smoke

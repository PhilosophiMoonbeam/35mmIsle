#include "oaks_lab_scene_private_smoke.hpp"
#include <algorithm>

namespace isle::oaks_lab::private_smoke {
namespace {
struct Driver {
    Scene& scene;
    Hooks hooks;
    std::string& error;
    unsigned remaining;
    game_input::HostFrame sample{};
    std::uint64_t terminal=0;
    bool face_pixels=false,dialog_pixels=false;
    bool step() {
        if(!remaining){error="actual Oak UI update bound exhausted";return false;}--remaining;
        Frame frame;
        if(!hooks.advance(hooks.owner,sample,frame)){error="actual Oak scheduler/audio/RSP/RDP step failed";return false;}
        if(scene.failed()){error=scene.error();return false;}
        if(!frame.actual_terminal_ordinal){if(!frame.rgba.empty()){error="Oak no-draw update exposed unacknowledged pixels";return false;}return true;}
        if(frame.actual_terminal_ordinal<=terminal||frame.rgba.size()!=320*240*4){error="Oak consumer requires a genuinely new completed canonical frame";return false;}
        terminal=frame.actual_terminal_ordinal;
        const auto varied=[&](int left,int top,int right,int bottom) {
            const auto first=static_cast<std::size_t>(top*320+left)*4;
            for(int y=top;y<bottom;++y)for(int x=left;x<right;++x){const auto at=static_cast<std::size_t>(y*320+x)*4;if(!std::equal(frame.rgba.begin()+first,frame.rgba.begin()+first+3,frame.rgba.begin()+at))return true;}
            return false;
        };
        const auto view=scene.view();
        if(view.phase!=Phase::opening&&view.phase!=Phase::closing&&view.phase!=Phase::complete){face_pixels|=varied(100,34,192,120);dialog_pixels|=varied(104,173,296,220);}
        return true;
    }
    bool pulse(std::uint16_t buttons,std::int8_t y=0) {
        sample[0].buttons=buttons;sample[0].stick_y=y;
        if(!step())return false;
        sample[0].buttons=0;sample[0].stick_y=0;return step();
    }
    bool until(Phase phase) {
        while(scene.view().phase!=phase){if(scene.view().exit_requested){error="Oak published an unexpected destination before requested control phase";return false;}if(!step())return false;}
        return true;
    }
    bool dialogs(bool repeat_once) {
        bool repeated=false;
        while(scene.view().phase!=Phase::lab) {
            const auto phase=scene.view().phase;
            if(phase==Phase::dialogue){if(!pulse(0x8000))return false;}
            else if(phase==Phase::page_wait){if(!pulse(0x8000))return false;}
            else if(phase==Phase::repeat_question){if(!pulse(repeat_once&&!repeated?0x8000:0x4000))return false;repeated=true;}
            else if(!step())return false;
        }
        if(repeat_once&&!repeated){error="first-meet source replay question was never reached";return false;}
        return true;
    }
    bool destination(int expected) {
        while(!scene.view().exit_requested)if(!step())return false;
        if(scene.view().phase!=Phase::complete||scene.view().source_result!=expected){error="Oak source destination was hidden, substituted or published before completion";return false;}
        return true;
    }
};
bool unchanged_name(game::State& state,const save::GameSnapshot& before,std::string& error,bool allow_frozen=false) {
    const auto after=state.save()->snapshot();
    // Frozen Store evidence intentionally disallows a new GameSession snapshot;
    // the original owning snapshot remains valid without close/reopen/retry.
    if(!after&&allow_frozen&&!static_cast<bool>(*state.save()))return true;
    if(!after||!std::equal(before.player_name().begin(),before.player_name().end(),after.value.player_name().begin())){error="Oak changed the persistent full16-byte source player name";return false;}
    return true;
}
}
bool exercise(Scene& scene,game::State& state,Hooks hooks,Scenario scenario,unsigned course,unsigned maximum,std::string& error) {
    if(!hooks.advance||!hooks.fence||!maximum||!state.initialized()||course>6){error="Oak consumer requires actual initialized owners, source course and scheduler/fence";return false;}
    const auto before=state.save()->snapshot();if(!before){error="Oak initial persistent source snapshot";return false;}
    const auto initial_generation=before.value.generation();
    auto* const initial_session=state.save();
    if((scenario==Scenario::first_meet_course||scenario==Scenario::reentry_course)&&course>static_cast<unsigned>(before.value.unlocked_course().value)){error="Oak course scenario needs the genuine unlocked source choice";return false;}
    const auto title_byte=state.title_start_flags;
    const bool first=scenario==Scenario::first_meet_course;
    if(first&&!(state.scene_notices()&1)){error="first-meet scenario requires actual transferred notice1";return false;}
    if(scenario==Scenario::reentry_course&&(state.scene_notices()&1)){error="reentry must reuse consumed notices rather than inject a second first-meet";return false;}
    Driver driver{scene,hooks,error,maximum};
    driver.sample[0].connection=game_input::Connection::present;driver.sample[0].identity=0x4f414b;
    if(!scene.start()){error=scene.error();return false;}
    if(scenario==Scenario::retire_during_dialogue) {
        while(scene.view().phase!=Phase::dialogue)if(!driver.step())return false;
        if(!hooks.fence(hooks.owner)){error="actual Oak cancellation borrower fence failed";return false;}
        scene.retire();
        if(scene.view().exit_requested||scene.view().commit_attempts){error="Oak dialogue retirement fabricated destination or implicit save";return false;}
        return unchanged_name(state,before.value,error);
    }
    if(!driver.dialogs(first))return false;
    if(!driver.face_pixels||!driver.dialog_pixels){error="completed canonical pixels lack actual Oak portrait/dialog raster variation";return false;}
    if((first&&!scene.view().first_meet_consumed)||(state.scene_notices()&1)||state.title_start_flags!=title_byte){error="Oak notice1 consumption or independent title-byte lifetime diverged";return false;}
    if(first&&scene.view().printed_characters==0){error="Oak typewriter/per-character native hook was never called";return false;}
    if(scene.view().commit_attempts||scene.view().exit_requested){error="Oak intro implicitly saved or completed the scene";return false;}
    if(scenario==Scenario::first_meet_course||scenario==Scenario::reentry_course) {
        if(!driver.pulse(0x8000)||!driver.until(Phase::courses))return false;
        if(!driver.pulse(0x4000)||!driver.until(Phase::lab))return false;
        if(scene.view().exit_requested){error="course B fabricated a title/name/Beach transition";return false;}
        if(!driver.pulse(0x8000)||!driver.until(Phase::courses))return false;
        while(scene.view().selected_slot!=static_cast<int>(course))if(!driver.pulse(0,-60))return false;
        if(!driver.pulse(0x8000)||!driver.until(Phase::confirmation))return false;
        if(scene.view().exit_requested){error="source course request appeared before confirmation";return false;}
        if(!driver.pulse(0x4000)||!driver.until(Phase::courses))return false;
        if(scene.view().exit_requested){error="confirmation B emitted the course request";return false;}
        if(!driver.pulse(0x8000)||!driver.until(Phase::confirmation)||!driver.pulse(0x8000)||!driver.destination(static_cast<int>(course)))return false;
        const auto after=state.save()->snapshot();
        if(!after||after.value.generation()!=initial_generation||scene.view().commit_attempts){error="course confirmation performed an implicit durable save";return false;}
    } else if(scenario==Scenario::report_request||scenario==Scenario::album_request) {
        const auto flag=before.value.player_flag(7);
        if(scenario==Scenario::report_request){if(!flag||!flag.value){error="Report scenario needs genuine source unlock7";return false;}if(!driver.pulse(0,-60))return false;}
        else {if(!driver.pulse(0,60)||!driver.pulse(0,60))return false;}
        if(!driver.pulse(0x8000)||!driver.destination(scenario==Scenario::report_request?14:12))return false;
    } else {
        if(!driver.pulse(0,60)||!driver.pulse(0x8000)||!driver.until(Phase::save_prompt))return false;
        if(scenario==Scenario::save_decline) {
            if(!driver.pulse(0x4000)||!driver.until(Phase::lab))return false;
            if(scene.view().commit_attempts||scene.view().exit_requested){error="initial save-prompt B must return lab without commit/exit";return false;}
        } else {
            if(!driver.pulse(0x8000))return false;
            if(scenario==Scenario::save_failure) {
                if(!driver.until(Phase::save_failed))return false;
                if(commit_acknowledged(scene.view().commit)||scene.view().commit_attempts!=1){error="real save failure was normalized to success or retried";return false;}
                for(unsigned i=0;i<32;++i)if(!driver.pulse(i&1?0x4000:0x8000))return false;
                if(scene.view().phase!=Phase::save_failed||scene.view().exit_requested||scene.view().commit_attempts!=1){error="source failure wait accepted controls, retried, or emitted success";return false;}
            } else {
                if(!driver.until(Phase::keep_going))return false;
                if(scene.view().commit_attempts!=1||!commit_acknowledged(scene.view().commit)){error="keep-going UI without acknowledged publication";return false;}
                if(scenario==Scenario::save_title){if(!driver.pulse(0x4000)||!driver.destination(8))return false;}
                else if(!driver.pulse(0x8000)||!driver.until(Phase::lab))return false;
            }
        }
    }
    if(!unchanged_name(state,before.value,error,scenario==Scenario::save_failure))return false;
    if(state.save()!=initial_session||state.title_start_flags!=title_byte){error="Oak replaced the persistent session or conflated the title byte";return false;}
    if(!hooks.fence(hooks.owner)){error="actual Oak final borrower fence failed";return false;}
    scene.retire();
    return true;
}
} // namespace isle::oaks_lab::private_smoke

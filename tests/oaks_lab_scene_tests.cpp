#include "oaks_lab_scene.hpp"
#include <array>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
}
int main(){
    try {
        using namespace isle;
        // Source layouts expose eight selection slots; terminal NONE and the
        // splitter itself are never selectable native buttons.
        std::array<window::Button,8> slots{};
        slots[0].id=6;slots[2].id=7;slots[7].id=5;
        game_input::MenuInput input{};
        input.pressed=game_input::stick_slow_up;
        auto step=oaks_lab::source_navigation(slots,0,input);
        check(step.slot==7&&step.move_sounds==1&&!step.back,"up wraps to actual bottom Back choice");
        input.pressed=game_input::stick_slow_down;
        step=oaks_lab::source_navigation(slots,0,input);
        check(step.slot==2&&step.move_sounds==1,"down skips source NONE slots without hiding destinations");
        input.current=game_input::stick_slow_left;
        step=oaks_lab::source_navigation(slots,0,input);
        check(step.slot==0&&step.move_sounds==0,"horizontal-held diagonal suppresses vertical navigation");
        input.current=0;input.pressed=game_input::stick_slow_up|game_input::stick_slow_down|0x4000;
        step=oaks_lab::source_navigation(slots,0,input);
        check(step.slot==0&&step.move_sounds==2&&step.back,"independent source up/down branches precede B return");
        input.pressed=0x8000;
        step=oaks_lab::source_navigation(slots,0,input);
        check(step.slot==0&&!step.back&&step.move_sounds==0,"A is not a fabricated navigation or scene completion");
        save::CommitResult result;
        check(!oaks_lab::commit_acknowledged(result),"default/not-published commit cannot be success");
        result.outcome=save::CommitOutcome::published;result.error={};
        check(!oaks_lab::commit_acknowledged(result),"publication without acknowledged generation/durability cannot succeed");
        result.generation=1;result.durability=save::Durability::linux_file_and_directory_synced;
        check(oaks_lab::commit_acknowledged(result),"actual acknowledged Linux publication succeeds");
        result.durability=save::Durability::win32_acknowledged;
        check(oaks_lab::commit_acknowledged(result),"actual acknowledged Windows publication succeeds");
        result.outcome=save::CommitOutcome::indeterminate;
        check(!oaks_lab::commit_acknowledged(result),"indeterminate/frozen evidence must enter source failure wait");
        result.outcome=save::CommitOutcome::not_published;
        check(!oaks_lab::commit_acknowledged(result),"not-published evidence must not show keep-going success");
        result.outcome=save::CommitOutcome::published;result.cleanup_error.code=save::StoreErrc::invalid_argument;
        check(!oaks_lab::commit_acknowledged(result),"cleanup freeze cannot be reported as successful save");
        result.cleanup_error={};result.error.code=save::StoreErrc::invalid_argument;
        check(!oaks_lab::commit_acknowledged(result),"reported Store failure is not silently normalized to success");
        std::cout<<"Oak source navigation and explicit publication regressions passed\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

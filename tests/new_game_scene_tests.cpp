#include "new_game_scene.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace ng=isle::new_game;
namespace in=isle::game_input;
namespace {
void require(bool value,const char* message) { if (!value) throw std::runtime_error(message); }
constexpr std::array<std::uint8_t,2> a{0xa3,0xc1};
constexpr std::array<std::uint16_t,4> todd{0xa3d4,0xa3ef,0xa3e4,0xa3e4};
void end(ng::Editor& editor) { editor.press(0x1000,a); }
void run() {
    ng::Editor e;
    require(e.byte_length()==0 && e.row()==0 && e.column()==0,"empty source BSS initial editor");
    require(!e.confirm(todd) && !e.confirmed(),"no default outside actual End semantic");
    auto effect=e.press(in::stick_slow_right|0xd000,a);
    require(e.column()==1 && e.byte_length()==0 && effect.sound_count==1 && effect.sounds[0]==69,"direction precedes B,A,Start");
    effect=e.press(0xd000,a);
    require(e.row()==19 && e.column()==2 && !e.confirming() && effect.sounds[0]==65,"B on empty sounds65 and jumpsEnd, never cancels");
    require(e.focus()==std::array<int,4>{63,212,15,5},"source End geometry");
    effect=e.press(0x8000,a);
    require(effect.confirm && e.confirming() && !e.confirmed() && e.name()[0]==0,"End first only requests confirmation wait");
    require(e.press(0xd000,a).sound_count==0 && e.confirming(),"no input edits during confirmation wait");
    require(e.confirm(todd) && e.confirmed(),"empty End defaults after confirmation boundary");
    require(e.name()[0]==0xa3 && e.name()[1]==0xd4 && e.name()[7]==0xe4 && e.name()[8]==0 && e.name()[15]==0,"full16 source default record");
    require(!e.confirm(todd) && e.press(0x4000,a).sound_count==0,"confirmed name terminal exactlyonce");

    ng::Editor seven;
    for (unsigned i=0;i<7;++i) effect=seven.press(0x8000,a);
    require(seven.byte_length()==14 && seven.row()==19 && seven.column()==2,"seven two-byte characters autoEnd");
    require(effect.sound_count==2 && effect.sounds[0]==65 && effect.sounds[1]==66,"ordinary capacity source audio order");
    require(seven.name()[14]==0 && seven.name()[15]==0,"two remaining source terminator bytes");
    seven.press(in::stick_slow_up,a);
    effect=seven.press(0x8000,a);
    require(seven.byte_length()==14 && seven.row()==19 && effect.sound_count==2 &&
            effect.sounds[0]==65 && effect.sounds[1]==66 && !effect.redraw_name,
            "ordinary character at capacity jumpsEnd and still plays select66");
    effect=seven.press(0xc000,a);
    require(seven.byte_length()==12 && effect.sounds[0]==74 && !seven.confirming(),"B precedes A, deletes pair without cancel");
    seven.press(in::stick_slow_up,a);
    require(seven.row()==18 && seven.column()==0,"jumpEnd preserves source remembered gridcolumn");
    seven.press(0x8000,a);
    seven.press(0x8000,a);
    require(seven.confirming() && !seven.confirm(todd) && seven.confirmed(),"nonblank name never substituted");
    for (unsigned i=0;i<14;i+=2) require(seven.name()[i]==a[0] && seven.name()[i+1]==a[1],"complete source name bytes retained");

    ng::Editor spaces;
    spaces.press(in::stick_slow_up,a); // row19,col0 (backspace).
    spaces.press(in::stick_slow_right,a); // source space,col1.
    for (unsigned i=0;i<7;++i) {
        effect=spaces.press(0x8000,a);
        require(effect.sound_count==1 && effect.sounds[0]==66 && !effect.redraw_name,"space append deliberately does not redraw or autoEnd");
    }
    require(spaces.byte_length()==14 && spaces.column()==1 && spaces.row()==19,"space at capacity remains space target");
    effect=spaces.press(0x8000,a);
    require(spaces.column()==2 && effect.sound_count==1 && effect.sounds[0]==65,"next space at capacity jumpsEnd without select sound");
    spaces.press(0x8000,a);
    require(spaces.confirm(todd),"every wholly fullwidth-space length accepts source default");
    require(spaces.name()[8]==0 && spaces.name()[9]==0xa1 && spaces.name()[13]==0xa1 && spaces.name()[15]==0,"default copies source string but preserves bytes beyond terminator in full16 record");
    for (unsigned count=1;count<7;++count) {
        ng::Editor blank;
        for (unsigned i=0;i<count;++i) blank.press(0x8000,{0xa1,0xa1});
        end(blank); blank.press(0x8000,a);
        require(blank.confirm(todd),"all-space shorter names also default");
    }

    ng::Editor nav;
    nav.press(in::stick_slow_left,a);
    require(nav.column()==4 && nav.row()==0,"grid left wraps");
    nav.press(in::stick_slow_up,a);
    require(nav.column()==1 && nav.row()==19,"enterEnd saves gridcol4 then integer-divides by3");
    nav.press(in::stick_slow_down|in::stick_slow_right,a);
    require(nav.row()==0 && nav.column()==4,"leaveEnd restores remembered column after axis edits");
    nav.press(in::stick_slow_up|in::stick_slow_down,a);
    require(nav.row()==20 && nav.column()==1,"simultaneous opposite source axes preserve source branch order");
    nav.press(in::stick_slow_up,a);
    require(nav.row()==19,"source out-of-grid simultaneous-axis state returns towardEnd");
    ng::Editor fast;
    effect=fast.press(in::stick_up|0x8000,a);
    require(fast.byte_length()==2 && effect.sounds[0]==66,"fast axis bits do not steal slow-only input branch");
    end(fast); effect=fast.press(0x9000,a);
    require(effect.confirm && fast.confirming(),"A precedes Start atEnd");
}
}
int main() {
    try { run(); std::cout<<"name source editor boundaries passed\n"; return EXIT_SUCCESS; }
    catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return EXIT_FAILURE; }
}

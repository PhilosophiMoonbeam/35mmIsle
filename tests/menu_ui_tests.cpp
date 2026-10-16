#include "menu_ui_scene.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace m=isle::menu::scene;
namespace in=isle::game_input;
namespace {
void require(bool ok,const char* what) {if(!ok) throw std::runtime_error(what);}
void run() {
    const m::MenuAvailability missing{false,false}, saved{true,false}, gallery{true,true};
    // Start expands, B collapses, and a later Start expands again without a
    // scene request. This is the same transition function the native UI uses.
    auto s=m::menu_transition(saved,2,1,0x1000,7,0,14);
    require(s.state==3 && s.selection==1,"Start must expand main menu");
    s=m::menu_transition(saved,s.state,s.selection,0x4000,8,0,14);
    require(s.state==2,"B must return to press Start without scene exit");
    s=m::menu_transition(saved,s.state,s.selection,0x8000,9,0,14);
    require(s.state==3,"A must reenter choices");
    s=m::menu_transition(saved,s.state,s.selection,0x8000,10,0,14);
    require(s.state==7 && m::menu_exit_scene(s.state)==m::SourceScene::oaks_lab && m::menu_entry_flags(s.state)==2,
            "existing save defaults to real Oaks Lab continuation");
    s=m::menu_transition(missing,3,0,in::stick_slow_up,0,0,14);
    require(s.selection==1,"up wraps two-item missing-save menu");
    s=m::menu_transition(missing,3,s.selection,0x1000,0,0,14);
    require(s.state==8,"missing-save second item opens real options");
    s=m::menu_transition(gallery,3,3,in::stick_slow_down,0,0,14);
    require(s.selection==0,"down wraps expanded four-item menu");
    s=m::menu_transition(gallery,3,2,0x8000,0,0,14);
    require(s.state==9 && m::menu_exit_scene(s.state)==m::SourceScene::gallery,"report threshold exposes gallery");
    s=m::menu_transition(saved,3,2,0x8000,0,0,14);
    require(s.state==8,"without report threshold the same position remains options");
    s=m::menu_transition(saved,3,1,in::stick_up,0,0,14);
    require(s.state==3 && s.selection==1,"fast axis is not slow menu repeat");
    s=m::menu_transition(saved,3,1,in::stick_slow_up|0x8000,0,0,14);
    require(s.state==3 && s.selection==0,"source axis branch precedes confirm");
    s=m::menu_transition(saved,3,1,0x4000|0x8000,0,0,14);
    require(s.state==2,"source cancel precedes confirm");
    require(m::menu_transition(saved,2,0,0,1799,0,14).state==2,"idle must not exit early");
    require(m::menu_transition(saved,2,0,0,1800,0,14).state==10,"idle requests actual attract scene");
    require(m::menu_transition(saved,3,1,0,1800,0,15).state==11,"alternate source entry returns to actual title");
    require(m::menu_transition(saved,2,0,0x1000,1800,0,14).state==3,"input resets idle before deadline decision");
    require(m::menu_transition(saved,5,0,0x9000,1800,-1,14).state==5,"absent physical controller never fakes input/attract");
    s=m::menu_transition(missing,3,0,0x1000,0,0,14);
    require(s.state==6 && m::menu_exit_scene(s.state)==m::SourceScene::beach && m::menu_entry_flags(s.state)==1,
            "new game requests Beach plus source loader flag, not a shortcut");
    require(m::menu_exit_scene(8)==m::SourceScene::invalid,"options is not an external scene completion");
}
}
int main() {
    try {run();std::cout<<"menu UI behavior boundaries passed\n";return EXIT_SUCCESS;}
    catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
}

#include "window_widget_math.hpp"
#include <iostream>
#include <stdexcept>
namespace {
void check(bool v,const char* message){if(!v)throw std::runtime_error(message);}
void geometry(){using isle::window::detail::frame_geometry;
 auto a=frame_geometry(60,60,204,56);
 check(a[2].x==60&&a[2].y==60&&a[4].x==256&&a[4].y==60,"source top corners");
 check(a[6].x==67&&a[6].y==67&&a[6].sx==23.75f&&a[6].sy==5.25f,"nine-slice center uses14px overlap law");
 check(a[1].x==60&&a[1].y==108&&a[8].x==67&&a[8].y==108,"source bottom piece ordering");
 a=frame_geometry(-3,4,0,999);
 check(a[2].sx==.25f&&a[2].sy==1&&a[6].x==-2&&a[6].y==11,"independent axis clamp and sub24 compression");
 check(a[3].x==-5&&a[3].y==236,"source signed placement retained");
 a=frame_geometry(0,0,24,24);check(a[6].sx==1.25f&&a[6].sy==1.25f,"24 threshold is stretched branch not compression");
 a=frame_geometry(184,320,204,56);
 check(a[2].x==184&&a[2].y==320&&a[3].x==380&&a[3].y==368,"Name and Oak offscreen frame setup is not screen-clamped");
}
void expansions(){using isle::window::detail::frame_expansion;
 for(int style=0;style<4;++style){auto end=frame_expansion(style,15,15);check(end.x==96&&end.y==165&&end.width==208&&end.height==60&&end.alpha==255,"all source expansion endpoints coincide");}
 auto center=frame_expansion(1,0,15),right=frame_expansion(2,0,15),left=frame_expansion(3,0,15);
 check(center.x==188&&center.y==201&&center.width==24&&center.height==24,"center fade initial geometry");
 check(right.x==280&&right.y==201&&left.x==96&&left.y==165,"right and left fades are distinct authored origins");
 auto middle=frame_expansion(0,7,15);check(middle.x==146&&middle.y==185&&middle.width==108&&middle.height==40&&middle.alpha==119,"integer division precedes source coordinate arithmetic");
}
void breathing(){isle::window::detail::FocusClock clock;
 for(int i=1;i<=16;++i){clock.step(false);const int expected=i<4?0:i<8?1:i<12?2:i<16?1:0;check(clock.offset==expected,"breathing changes every fourth update and reverses at two");}
 clock.step(true);check(clock.offset==0,"stopped breathing removes corner displacement");
 for(int i=0;i<3;++i){clock.step(false);}
 check(clock.offset==0,"resume restarts full four-update period");
 clock.step(false);check(clock.offset==1,"resume fourth update");
}
void focus_motion(){
 using isle::window::detail::focus_interpolate;
 using isle::window::detail::focus_geometry;
 // The first Name update moves from the default box toward its first glyph.
 const float x=focus_interpolate(22.0f,25.0f),y=focus_interpolate(29.0f,22.0f);
 const float width=focus_interpolate(62.0f,4.0f),height=focus_interpolate(13.0f,5.0f);
 auto corners=focus_geometry(x,y,width,height,0);
 check(corners[0].x==17&&corners[0].y==21,"focus interpolates before publishing the corners");
 check(corners[3].x==62&&corners[3].y==32,"right and bottom truncate the sum rather than individual terms");
 corners=focus_geometry(x,y,width,height,2);
 check(corners[0].x==15&&corners[0].y==19&&corners[3].x==64&&corners[3].y==34,"breathing offsets apply after corner truncation");
}
void transitions(){using isle::window::detail::animate_button;
 IsleSprite sprite{};sprite.attr=ISLE_SP_HIDDEN|ISLE_SP_SCALE;sprite.scalex=sprite.scaley=1;sprite.red=sprite.green=sprite.blue=128;
 float phase=-.5235987756f;
 check(animate_button(sprite,phase,25,1,3,false,false),"staggered slot contributes even while hidden");
 check(sprite.attr&ISLE_SP_HIDDEN,"negative phase does not prematurely expose slot");
 int count=0;while(animate_button(sprite,phase,25,1,3,false,false)&&++count<100){}
 check(count<100&&!(sprite.attr&ISLE_SP_HIDDEN),"expansion terminates through native update clocks");
 check(sprite.red==128&&sprite.green==128&&sprite.blue==128,"disabled tint survives all transition updates");
 const auto stable_y=sprite.y;const auto stable_attr=sprite.attr;
 check(!animate_button(sprite,phase,25,2,3,false,false)&&sprite.y==stable_y&&sprite.attr==stable_attr,"stable state neither delays callers nor moves visible buttons");
 check(!animate_button(sprite,phase,25,0,3,true,false),"instant collapse retires transition wait in one update");
 check((sprite.attr&ISLE_SP_HIDDEN)&&!(sprite.attr&ISLE_SP_SCALE)&&sprite.scaley==1,"collapse hides and clears scale with source reset");
 sprite.attr|=ISLE_SP_SCALE;phase=1.57079632679f;sprite.x=0;
 check(animate_button(sprite,phase,0,0,3,false,true)&&sprite.x>0&&sprite.scalex<1,"horizontal collapse applies86px origin compensation");
 sprite.attr=ISLE_SP_HIDDEN;phase=.7f;
 check(!animate_button(sprite,phase,25,0,3,false,false)&&(sprite.attr&ISLE_SP_HIDDEN),"inactive hidden unscaled slots do not keep waits alive");
 check(isle::window::detail::button_position(25.0f,.2f,false)==36,"double source compensation truncates below the next pixel");
 check(isle::window::detail::button_position(0.0f,.5f,true)==43,"horizontal button compensation uses the authored 86px width");
}
}
int main(){try{geometry();expansions();breathing();focus_motion();transitions();std::cout<<"window widgets tests passed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

#ifndef ISLE_WINDOW_WIDGET_MATH_HPP
#define ISLE_WINDOW_WIDGET_MATH_HPP
#include <algorithm>
#include <array>
#include "35mmisle/sprite.h"
#include <cmath>
#include <cstdint>
namespace isle::window::detail {
struct FramePiece { std::int16_t x,y;float sx,sy; };
inline std::array<FramePiece,9> frame_geometry(int x,int y,int width,int height) noexcept {
 width=std::clamp(width,6,320);height=std::clamp(height,6,240);
 const float cx=width<24?static_cast<float>(width)/24.0f:1.0f;
 const float cy=height<24?static_cast<float>(height)/24.0f:1.0f;
 // Source callers keep the complete frame in signed sprite-coordinate range.
 const auto mx=static_cast<std::int16_t>((cx*8.0f+static_cast<float>(x))-1.0f);
 const auto my=static_cast<std::int16_t>((cy*8.0f+static_cast<float>(y))-1.0f);
 const auto left=static_cast<std::int16_t>(x),top=static_cast<std::int16_t>(y);
 const auto right=static_cast<std::int16_t>((width+x)-8);
 const auto bottom=static_cast<std::int16_t>((height+y)-8);
 const std::int16_t xx[9]={right,left,left,right,right,mx,mx,left,mx};
 const std::int16_t yy[9]={my,bottom,top,bottom,top,top,my,my,bottom};
 const float sx=static_cast<float>(width-14)/8.0f;
 const float sy=static_cast<float>(height-14)/8.0f;
 std::array<FramePiece,9> out{};
 for(unsigned i=0;i<9;++i)out[i]={xx[i],yy[i],width<24?cx:((i==5||i==6||i==8)?sx:1.0f),height<24?cy:((i==0||i==6||i==7)?sy:1.0f)};
 return out;
}
struct FrameExpansion {int x,y,width,height,alpha;};
inline FrameExpansion frame_expansion(int style,int value,int frames) noexcept {
 int x=96,y=165,w=184*value/frames+24,h=36*value/frames+24;
 if(style<2){x=-92*value/frames+188;y=-36*value/frames+201;w=92*value/frames+212-x;h=225-y;}
 else if(style==2){x=-184*value/frames+280;y=-36*value/frames+201;w=304-x;h=225-y;}
 return {x,y,w,h,value*255/frames};
}
inline float focus_interpolate(float current,float target) noexcept {
 // The subtraction is f32; the unsuffixed source literal promotes the rest.
 return static_cast<float>(current+(target-current)*0.3);
}
struct FocusCorner {std::int16_t x,y;};
inline std::array<FocusCorner,4> focus_geometry(float x,float y,float width,float height,int offset) noexcept {
 // Truncate the f32 sum before adding the integer breathing offset.
 const int left=static_cast<int>(x)-offset-5,top=static_cast<int>(y)-offset-5;
 const int right=static_cast<int>(x+width)+offset-5,bottom=static_cast<int>(y+height)+offset-5;
 return {{{static_cast<std::int16_t>(left),static_cast<std::int16_t>(top)},
          {static_cast<std::int16_t>(right),static_cast<std::int16_t>(top)},
          {static_cast<std::int16_t>(left),static_cast<std::int16_t>(bottom)},
          {static_cast<std::int16_t>(right),static_cast<std::int16_t>(bottom)}}};
}
struct FocusClock {
 int offset=0,counter=4,increment=1;
 void step(bool stopped) noexcept {--counter;if(stopped){counter=4;offset=0;increment=1;}else if(counter==0){offset+=increment;counter=4;if(offset<=0)increment=1;else if(offset>=2)increment=-1;}}
};
inline std::int16_t button_position(float base,float scale,bool horizontal) noexcept {
 // Source slots are 0..7 (y=25..193); compensation is at most 86 or 15.
 // Keep both unsuffixed literals double until the sprite assignment truncates.
 return static_cast<std::int16_t>(base+(1.0-scale)*(horizontal?86.0:15.0));
}
// Preserve the preexisting binary32 approximation. The admitted own-GAME
// headers do not define PI/PI_2; original macro value/type equivalence is unproven.
inline constexpr float button_half_pi=1.5707963267948966f;
inline constexpr float button_pi=button_half_pi*2.0f;
// Returns whether this source slot still contributes to the transition wait.
inline bool animate_button(IsleSprite& v,float& phase,float base,int state,int frames,bool instant,bool horizontal) noexcept {
 if(state==2||!((v.attr^ISLE_SP_HIDDEN)&(ISLE_SP_HIDDEN|ISLE_SP_SCALE)))return false;
 const float step=static_cast<float>((frames+1)*90.0/30.0)*button_pi/180.0f;
 if(state==1){
  phase+=step;
  if(instant)phase=button_half_pi;
  if(phase>=button_half_pi){phase=button_half_pi;return false;}
  const float scale=std::max(.001f,std::sin(phase));
  if(phase>=0){
   v.attr&=~ISLE_SP_HIDDEN;
   if(horizontal){v.scalex=scale;v.x=button_position(base,scale,true);}
   else{v.scaley=scale;v.y=button_position(base,scale,false);}
  }
  return true;
 }
 phase-=step;
 if(instant)phase=-button_pi/1800.0f;
 if(phase<=0){
  v.attr|=ISLE_SP_HIDDEN;
  v.attr&=~ISLE_SP_SCALE;
  if(horizontal)v.scalex=1;else v.scaley=1;
  phase=button_pi/180.0f;
  return false;
 }
 const float scale=std::max(.001f,std::sin(phase));
 if(phase<button_half_pi){
  if(horizontal){v.scalex=scale;v.x=button_position(base,scale,true);}
  else{v.scaley=scale;v.y=button_position(base,scale,false);}
  v.attr|=ISLE_SP_SCALE;
 }
 return true;
} // animate_button
}
#endif

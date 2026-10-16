#include "window_text.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
namespace ui=isle::window;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
struct Font {
    // Entirely authored test alphabet/coverage, not a copy of GAME font data.
    std::array<std::uint8_t,740> characters{};
    std::array<std::uint8_t,188> ascii{};
    std::array<std::uint8_t,370> small_width{},large_width{};
    std::vector<std::uint8_t> small=std::vector<std::uint8_t>(370*32),large=std::vector<std::uint8_t>(370*72);
    Font(){
        small_width.fill(4);large_width.fill(8);
        for(unsigned i=0;i<370;++i){const auto ch=0x7000+i;characters[i*2]=static_cast<std::uint8_t>(ch>>8);characters[i*2+1]=static_cast<std::uint8_t>(ch);}
        for(unsigned i=0;i<94;++i){ascii[i*2]=0x70;ascii[i*2+1]=0;}
        small[0]=0x81;large[0]=0x81;
    }
    ui::FontData data(){return {characters,ascii,{small_width,large_width},{small,large}};}
};
std::uint16_t word(const std::vector<std::uint8_t>& bytes,std::size_t pixel){return static_cast<std::uint16_t>((bytes[pixel*2]<<8)|bytes[pixel*2+1]);}
void nibble_and_blend(){
    Font font;ui::Text text(font.data());text.set_shadow_offset(0);text.set_font(8);text.palettes({255,255,255,255},{0,0,0,255});
    std::vector<std::uint8_t> pixels(16*16*2,0);ui::TextSurface surface{pixels,16,16,2};
    text.draw_glyph(surface,0,0,0x7000);
    check(word(pixels,0)==0x1085&&word(pixels,1)==0x8c63,"glyph low nibble precedes high nibble, packed RGBA5551 coverage is exact");
    check(word(pixels,2)==0&&word(pixels,16)==0,"zero coverage does not overwrite prior raster");
    std::vector<std::uint8_t> ia8_backing(16*16,0);
    ui::TextSurface source_non32{ia8_backing,16,16,1};text.draw_glyph(source_non32,0,0,0x7000);
    check(ia8_backing[0]==0x10&&ia8_backing[1]==0x85&&ia8_backing[2]==0x8c&&ia8_backing[3]==0x63,
          "IA8-backed text preserves bounded source16-bit non32 dispatch, not invented IA8 glyph blending");
    std::vector<std::uint8_t> rgba(16*16*4);
    for(std::size_t i=0;i<rgba.size();i+=4){rgba[i]=30;rgba[i+1]=60;rgba[i+2]=90;rgba[i+3]=255;}
    ui::TextSurface surface32{rgba,16,16,4};text.palettes({240,150,0,255},{0,0,0,255});text.draw_glyph(surface32,0,0,0x7000);
    check(rgba[0]==44&&rgba[1]==66&&rgba[2]==84&&rgba[4]==142&&rgba[5]==108&&rgba[6]==42,"32-bit coverage blends against existing destination, not the 16-bit prebuilt background palette");
}
void shadow_paths_and_bounds(){
    Font font;font.small[0]=8;ui::Text text(font.data());text.set_font(8);text.set_shadow_offset(2);text.palettes({255,255,255,255},{200,0,0,255});
    std::vector<std::uint8_t> rgba(16*16*4,100);
    ui::TextSurface surface32{rgba,16,16,4};text.draw_glyph(surface32,0,0,0x7000);
    check(rgba[(16+1)*4]==0&&rgba[(32+2)*4]==47,"shadow offset2 places solid near shadow then coverage-blended far edge");
    std::vector<std::uint8_t> sixteen(16*16*2,0);sixteen[1]=1;
    ui::TextSurface surface16{sixteen,16,16,2};text.draw_glyph(surface16,0,0,0x7000);
    check(word(sixteen,0)==0x8c63,"16-bit overlap detects existing solid shadow and chooses foreground-over-shadow palette");
    std::vector<std::uint8_t> short_plane(8*8*4,0x55);const auto before=short_plane;
    ui::TextSurface short_surface{short_plane,8,8,4};bool rejected=false;
    try{text.draw_glyph(short_surface,0,0,0x7000);}catch(const std::out_of_range&){rejected=true;}
    check(rejected&&short_plane==before,"unsafe source 32-bit shadow overrun rejects before any raster byte changes");
}
struct Hooks {
    ui::Text* text;
    int waits=0,characters=0,buttons=0,changes=0;
    static bool wait(void* p,std::uint32_t n){auto& h=*static_cast<Hooks*>(p);h.waits+=static_cast<int>(n);if(h.waits==1)h.text->input(0x8000);return true;}
    static void character(void* p,std::uint16_t){++static_cast<Hooks*>(p)->characters;}
    static void button(void* p,int,int,char c,int scale){auto& h=*static_cast<Hooks*>(p);check(c=='a'&&scale==8,"source button code and size");++h.buttons;}
    static void changed(void* p,std::size_t,std::size_t){++static_cast<Hooks*>(p)->changes;}
};
void source_escapes_waits_and_skip(){
    Font font;ui::Text text(font.data());text.set_font(8);text.set_shadow_offset(0);text.set_spacing(-1,3);text.set_print_delay(3);
    std::vector<std::uint8_t> pixels(64*24*2,0);ui::TextSurface surface{pixels,64,24,2,11,13};
    Hooks hooks{&text};int x=0,y=0;
    check(text.print(surface,x,y,"\\tAA\\wA",{&hooks,Hooks::wait,Hooks::character,Hooks::button,Hooks::changed}),"source text completes");
    check(hooks.waits==4&&hooks.characters==2&&hooks.changes>=3&&x==15,"skip suppresses remaining waits/hooks until source reset escape; bounded glyph rows publish before waits");
    text.set_print_delay(0);x=y=0;hooks.characters=0;
    text.print(surface,x,y,"\\B\\a",{&hooks,nullptr,Hooks::character,Hooks::button,Hooks::changed});
    check(hooks.characters==0&&hooks.buttons==1&&x==5,"delay0 suppresses unknown escape hook; inline button advance follows source fallback glyph width, not measurement's12");
    check(text.string_width("\\a")==12&&text.string_width("A A")==12,"measuring button and narrow proportional space preserves distinct source rules");
    text.set_font(12);x=y=0;text.print(surface,x,y,"\\kA",{});
    check(text.glyph_size()==8,"source j/k escape passes invalid font ids and therefore selects font8, not invented font12");
    x=60;y=0;text.print(surface,x,y,"A",{});
    check(x==0&&y==11,"wrapping uses padded surface width and recomputed glyph/spacing/shadow intervals");
    text.set_proportional(false);check(text.string_width("A A")==18,"fixed-width charset0 uses six pixels including spaces");
}
}
int main(){try{nibble_and_blend();shadow_paths_and_bounds();source_escapes_waits_and_skip();std::cout<<"window text tests passed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

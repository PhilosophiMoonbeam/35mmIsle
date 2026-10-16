#include "window_element.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
namespace ui=isle::window;
namespace sp=isle::sprite;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::uint16_t pixel(const ui::Element& e,int x,int y){const auto p=static_cast<std::size_t>((y+e.border_width())*e.stride()+x+e.border_width())*2;return static_cast<std::uint16_t>((e.image()[p]<<8)|e.image()[p+1]);}
void formats_borders_and_clamps(){
    auto heap=std::make_shared<ui::Heap>(0x50000);ui::ElementList list;
    auto e=ui::Element::create(heap,list,20,30,5,4,ui::border,{0x400000,0x410000});
    check(e&&e->stride()==48&&e->border_width()==4&&e->descriptor().x==16&&e->descriptor().y==26,"native element border, padded stride and source sprite placement");
    e->fill(99,99,-8,-8,{255,128,0,254});
    check(pixel(*e,0,0)==0xfc00&&pixel(*e,4,3)==0xfc00&&e->image()[0]==0,"independent endpoint clamps, inclusive reversed rectangle and alpha-low-bit RGBA5551 packing");
    auto ia=ui::Element::create(heap,list,0,0,4,4,ui::bits8|ui::grayscale,{0x420000,0x430000});
    ia->fill(0,0,0,0,{0xab,0,0,0xcd});check(ia->image()[0]==0xac&&ia->image()[1]==0,"IA8 stores intensity high nibble and alpha high nibble without host-color reinterpretation");
    auto rgba=ui::Element::create(heap,list,0,0,4,4,ui::bits32,{0x440000,0x450000});
    rgba->fill(0,0,0,0,{1,2,3,4});check(rgba->image()[0]==1&&rgba->image()[1]==2&&rgba->image()[2]==3&&rgba->image()[3]==4,"RGBA32 canonical byte order");
    auto alpha=ui::Element::create(heap,list,0,0,4,4,ui::grayscale|ui::alpha_plane,{0x460000,0x470000});
    alpha->fill(0,0,0,0,{8,9,10,11});check(alpha->image()[0]==255&&alpha->image()[1]==11,"source alpha-plane IA16 overrides intensity with255");
}
void copy_scroll_and_cursor(){
    auto heap=std::make_shared<ui::Heap>(0x50000);ui::ElementList list;
    auto e=ui::Element::create(heap,list,0,0,4,4,0,{0x400000,0x410000});
    std::array<std::uint8_t,32> rows{};
    for(unsigned y=0;y<4;++y)for(unsigned x=0;x<4;++x){rows[(y*4+x)*2]=static_cast<std::uint8_t>((y+1)*0x11);rows[(y*4+x)*2+1]=static_cast<std::uint8_t>((y+1)*0x11|1);}
    e->draw_image(0,0,4,4,rows);e->scroll(1,1);
    check(pixel(*e,0,0)==1&&pixel(*e,0,1)==1&&pixel(*e,0,2)==0x2223&&pixel(*e,0,3)==0x3333,"source downward scroll clears inclusive row0..amount, including one copied row");
    e->draw_image(0,0,4,4,rows);e->scroll(1,0);
    check(pixel(*e,0,0)==0x2223&&pixel(*e,0,2)==0x4445&&pixel(*e,0,3)==1,"up scroll copies stride-correct rows then clears exposed bottom");
    e->set_text_pos(999,999);check(e->text_x()==3&&e->text_y()==3,"cursor clamps at content dimensions, not padded storage");
    e->clear();check(e->text_x()==0&&e->text_y()==0&&pixel(*e,2,2)==1,"clear restores background and source cursor state");
    const auto before=std::vector<std::uint8_t>(e->image().begin(),e->image().end());bool rejected=false;
    try{e->draw_image(0,0,20,20,rows);}catch(const std::out_of_range&){rejected=true;}
    check(rejected&&std::equal(before.begin(),before.end(),e->image().begin()),"image extent failure is byte-atomic");
}
void alternate_revisions_scalar_state_and_lifetime(){
    auto heap=std::make_shared<ui::Heap>(0x50000);ui::ElementList list;
    auto e=ui::Element::create(heap,list,10,20,4,4,ui::use_alt_image|ui::scalable,{0x400000,0x410000});
    auto raster=e->raster();const auto* primary=raster->view(sp::RasterSource::primary).bitmap;
    const auto* alternate=raster->view(sp::RasterSource::alternate).bitmap;
    check(primary!=alternate&&e->descriptor().bitmap==alternate,"source alternate-image create initially displays the distinct alternate descriptor");
    e->fill(0,0,3,3,{255,0,0,255});const auto revision=raster->revision(sp::RasterSource::primary);
    e->swap_images();check(e->descriptor().bitmap==primary&&e->writable_source()==sp::RasterSource::alternate,"swap selects prior writable source for display, never patches immutable descriptor storage");
    const auto copied=raster->pixels(sp::RasterSource::alternate);
    check(copied[0]==0xf8&&copied[1]==1&&raster->revision(sp::RasterSource::alternate)>1,"source swap copies the complete plane and publishes its revision");
    e->fill(0,0,0,0,{0,0,255,255});
    check(raster->pixels(sp::RasterSource::primary)[0]==0xf8&&raster->revision(sp::RasterSource::primary)==revision,"later writable changes never mutate displayed primary bytes/revision");
    e->set_visible(false);e->set_sprite_color({11,22,33,44});e->set_pos(30,40);
    check((e->descriptor().attr&ISLE_SP_HIDDEN)&&e->descriptor().red==11&&e->descriptor().x==30,"position-only changes preserve transient native visibility/color rather than replacing whole source descriptor");
    e->set_scale(2,3);check(e->descriptor().scalex==2&&e->descriptor().scaley==3,"scalable source updates retain native scale");
    IsleSprite external{};external.width=9;external.height=7;external.red=88;
    e->set_alt_pos(70,80);e->set_alt_sprite(42,external);e->show_alt_sprite(true);e->set_pos(90,100);
    check(e->descriptor().x==70&&e->descriptor().y==80&&e->descriptor().width==9&&e->descriptor().red==88,"external alternate sprite and geometry remain distinct from backing raster and primary position");
    e->show_alt_sprite(false);check(e->descriptor().x==90&&e->descriptor().y==100,"explicit primary rebind restores source primary scalar record");
    e.reset();check(!list.head()&&heap->allocated()!=0,"native descriptors retain pixel allocation after element retires while producer owns Raster");
    raster.reset();check(heap->allocated()==0,"last retired raster releases native UI heap allocation");
}
void list_order(){
    auto heap=std::make_shared<ui::Heap>(0x50000);ui::ElementList list;
    auto first=ui::Element::create(heap,list,0,0,4,4,0,{0x400000,0x410000});
    auto second=ui::Element::create(heap,list,0,0,4,4,0,{0x420000,0x430000});
    auto third=ui::Element::create(heap,list,0,0,4,4,0,{0x440000,0x450000});
    check(list.head()==third.get()&&list.tail()==first.get()&&first->id()==0&&third->id()==2,"elements head-link with source scene-local creation ids");
    list.move_head(*first);check(list.head()==first.get()&&first->next()==third.get(),"head reordering repairs both links");
    list.move_tail(*third);check(list.tail()==third.get()&&third->previous()==second.get(),"non-head tail reordering repairs both links");
    list.move_tail(*first);check(list.head()==first.get()&&list.tail()==third.get(),"pinned MoveToTail excludes current list head");
}
}
int main(){try{formats_borders_and_clamps();copy_scroll_and_cursor();alternate_revisions_scalar_state_and_lifetime();list_order();std::cout<<"window element tests passed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

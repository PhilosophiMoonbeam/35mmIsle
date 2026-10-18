#include "window_element.hpp"
#include "sprite_producer.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
namespace ui=isle::window;
namespace sp=isle::sprite;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class Exception,class F>void rejects(F&& action,const char* message){
    bool rejected=false;
    try{action();}catch(const Exception&){rejected=true;}
    check(rejected,message);
}
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
void raster_source_presence_and_rejection(){
    auto bytes=std::make_shared<std::vector<std::uint8_t>>(64);
    IsleSprite prototype{};
    prototype.width=prototype.height=prototype.bmheight=prototype.bmHreal=4;
    prototype.scalex=prototype.scaley=1;
    prototype.nbitmaps=prototype.istep=1;prototype.bmsiz=2;
    const std::array<sp::RasterPiece,1> pieces{{{{4,4,0,0,nullptr,32,4,0},0}}};
    const sp::RasterStorage primary{bytes,std::span(*bytes).first(32),0x400000};
    const sp::RasterStorage alternate{bytes,std::span(*bytes).subspan(32),0x410000};
    const auto make=[&](sp::RasterStorage first,sp::RasterStorage second){
        return std::make_shared<sp::Raster>(prototype,pieces,
            std::array<sp::RasterStorage,2>{{std::move(first),std::move(second)}});
    };
    auto single=make(primary,{});
    check(single->has_source(sp::RasterSource::primary)&&!single->has_source(sp::RasterSource::alternate),
          "only an explicitly empty alternate is absent");
    const auto invalid=static_cast<sp::RasterSource>(2);
    check(!single->has_source(invalid),"presence query is bounded for invalid source enums");
    for(const auto source:{sp::RasterSource::alternate,invalid}){
        rejects<std::invalid_argument>([&]{single->view(source);},"absent/invalid source cannot expose a descriptor");
        rejects<std::invalid_argument>([&]{single->pixels(source);},"absent/invalid source cannot expose writable pixels");
        rejects<std::invalid_argument>([&]{single->address(source);},"absent/invalid source cannot expose a physical address");
        rejects<std::invalid_argument>([&]{single->revision(source);},"absent/invalid source cannot expose a revision");
        rejects<std::invalid_argument>([&]{single->publish(source,0,0);},"even empty publication rejects absent/invalid source");
    }
    rejects<std::invalid_argument>([&]{make({},alternate);},"a primary plane is required even with a complete alternate");
    const std::array<sp::RasterStorage,7> partial{{
        {bytes,{},0}, {{},std::span(*bytes).subspan(32),0}, {{},{},0x410000},
        {bytes,{},0x410000}, {{},std::span(*bytes).subspan(32),0x410000},
        {bytes,{bytes->data()+32,0},0x410000}, {{},{bytes->data()+32,0},0}
    }};
    for(const auto& storage:partial)
        rejects<std::invalid_argument>([&]{make(primary,storage);},"partially formed alternate is not absence");
    rejects<std::invalid_argument>([&]{make(primary,{bytes,std::span(*bytes).subspan(32),0x400000});},
                                  "real source planes cannot overlap physical storage");
    rejects<std::invalid_argument>([&]{make(primary,{bytes,std::span(*bytes).first(32),0x410000});},
                                  "distinct physical addresses cannot conceal CPU aliasing");
    const auto revision=single->revision(sp::RasterSource::primary);
    rejects<std::invalid_argument>([&]{single->publish(sp::RasterSource::primary,31,2);},
                                  "single-image publication still enforces fixed bounds");
    check(single->revision(sp::RasterSource::primary)==revision,"rejected publication preserves the live revision");
}
void photo_single_images_bounded_heap_and_retirement(){
    auto heap=std::make_shared<ui::Heap>(0x32000);ui::ElementList list;
    sp::Producer producer({},{});
    constexpr std::array<std::array<int,2>,3> sizes{{{204,48},{204,120},{192,47}}};
    constexpr std::array<ui::Color,3> colors{{{255,0,0,255},{0,255,0,255},{0,0,255,255}}};
    constexpr std::array<std::uint16_t,3> rgba{{0xf801,0x07c1,0x003f}};
    constexpr std::array<std::uint8_t,8> patch{{0xf8,1,0x07,0xc1,0,0x3f,0xff,0xff}};
    std::array<std::shared_ptr<ui::Element>,3> elements;
    std::array<std::weak_ptr<sp::Raster>,3> retained;
    std::array<sp::RasterBinding,3> bindings;
    std::array<IsleSprite,3> views;
    std::array<sp::Draw,3> draws;
    for(std::size_t i=0;i<elements.size();++i){
        const auto address=0x400000u+static_cast<std::uint32_t>(i)*0x10000u;
        auto& e=elements[i];
        e=ui::Element::create(heap,list,0,0,sizes[i][0],sizes[i][1],0,{address,0});
        check(bool(e),"all three original Photo flags-zero elements must coexist in the unchanged heap");
        check(!e->raster()->has_source(sp::RasterSource::alternate),"Photo element must not allocate an incidental alternate");
        check(std::all_of(e->image().begin(),e->image().end(),[](auto byte){return byte==0;}),
              "every allocated primary byte, including multi-tile padding, starts cleared");
        e->set_color(false,colors[i]);e->set_text_pos(12,13);e->clear();
        check(pixel(*e,0,0)==rgba[i]&&pixel(*e,e->width()-1,e->height()-1)==rgba[i]&&
              e->text_x()==0&&e->text_y()==0,"each full-size primary supports real clear and cursor reset");
        e->draw_background();
        e->draw_image(e->width()-2,e->height()-2,2,2,patch);
        check(pixel(*e,e->width()-2,e->height()-2)==0xf801&&pixel(*e,e->width()-1,e->height()-1)==0xffff,
              "real drawing reaches the final rows and columns of each padded Photo plane");
        const auto revision=e->raster()->revision(sp::RasterSource::primary);
        const auto* bitmap=e->descriptor().bitmap;
        IsleSprite external{};external.width=3;external.height=2;
        e->set_alt_sprite(99,external);e->show_alt_sprite(true);e->swap_images();
        check(e->descriptor().bitmap==bitmap&&!(e->descriptor().attr&ISLE_SP_HIDDEN)&&
              e->raster()->revision(sp::RasterSource::primary)==revision,
              "Source no-alt swap restores primary after an external sprite without copying or publishing a fake plane");
        e->show_alt_sprite(false);
        bindings[i]=producer.register_raster(e->raster());retained[i]=e->raster();
        views[i]=e->descriptor();draws[i]={bindings[i].resources[0],&views[i],bindings[i].generation};
    }
    check(heap->capacity()==0x32000&&list.head()==elements[2].get()&&list.tail()==elements[0].get()&&
          elements[2]->next()==elements[1].get()&&elements[1]->next()==elements[0].get(),
          "the original Photo heap and all three simultaneously live list entries are preserved");
    std::vector<std::uint32_t> ram(8*1024*1024/4,0xa5a5a5a5);
    producer.install(ram,1,{});
    for(std::size_t i=0;i<elements.size();++i){
        const auto address=0x400000u+static_cast<std::uint32_t>(i)*0x10000u;
        const auto& e=*elements[i];
        const auto last_patch=static_cast<std::size_t>((e.height()-2)*e.stride()+e.width()-2)*2;
        check(ram[address/4]==std::uint32_t(rgba[i])*0x10001u&&
              ram[(address+last_patch)/4]==0xf80107c1&&
              ram[(address+last_patch+static_cast<std::size_t>(e.stride())*2)/4]==0x003fffff&&
              ram[(address+static_cast<std::size_t>(e.width())*2)/4]==0&&ram[(address+0xf000)/4]==0xa5a5a5a5,
              "real producer upload preserves authored pixels, padded stride and physical allocation guards");
    }
    const auto allocated=heap->allocated();
    {
        auto borrow=producer.borrow();
        rejects<std::logic_error>([&]{elements[0]->clear();},"single-plane mutation still excludes live graphics borrowers");
        rejects<std::logic_error>([&]{elements[1]->raster()->publish(sp::RasterSource::primary,0,1);},
                                  "single-plane publication still excludes live graphics borrowers");
        rejects<std::invalid_argument>([&]{producer.unregister_raster(bindings[0]);},
                                      "registration cannot retire before its real consumer borrow");
        for(auto& e:elements)e.reset();
        check(!list.head()&&!list.tail()&&heap->allocated()==allocated,
              "Element retirement unlinks all nodes but producer ownership retains actual fixed pixel allocations");
        const auto commands=producer.prepare(ram,1,draws,{}, {0x150000,0x10000});
        check(commands.rectangles==27,"retained descriptors draw all 27 real Photo bitmap tiles after Element retirement");
    }
    for(std::size_t i=0;i<bindings.size();++i){
        check(!retained[i].expired(),"live registration owns its Raster and native pixel storage");
        producer.unregister_raster(bindings[i]);
        check(retained[i].expired(),"retired registration releases its final Raster owner");
    }
    check(heap->allocated()==0,"all three Photo allocations return to the original heap after actual consumer retirement");
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
int main(){try{formats_borders_and_clamps();copy_scroll_and_cursor();raster_source_presence_and_rejection();photo_single_images_bounded_heap_and_retirement();alternate_revisions_scalar_state_and_lifetime();list_order();std::cout<<"window element tests passed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

// Opt-in private-input CPU hydration/publication proof. No executor, GPU, input
// device, audio endpoint, screenshot, or scene stand-in is created by this file.
#include "window_element.hpp"
#include "sprite_producer.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
namespace ui=isle::window;
namespace sp=isle::sprite;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::uint8_t ram_byte(const std::vector<std::uint32_t>& ram,std::uint32_t a){return static_cast<std::uint8_t>(ram[a/4]>>((3-(a&3))*8));}
}
int main(int argc,char** argv){
    if(argc!=2){std::cerr<<"one admitted private-ROM path is required\n";return 2;}
    try{
        const auto snapshot=isle::rom::read_snapshot(argv[1]);
        const auto resources=ui::Resources::load(snapshot);
        check(resources->name_character(255)==std::string_view("\xa3\xc1",2),"source offset255 must preview fullwidth A");
        check(resources->name_end_label(3)=="End","source End-row label");
        bool bound=false;try{(void)resources->name_character(351);}catch(const std::out_of_range&){bound=true;}
        check(bound,"name character table exact351-pointer boundary");
        check(resources->frame(3,6).sprite.view()->bitmap==resources->frame(2,6).sprite.view()->bitmap,"style3 uses style2's source background bitmap");
        for(const auto& resource:resources->sprites())check(resource.sprite&&resource.sprite.view()->bitmap,"every admitted common/name/Oak resource has stable native bitmap ownership");
        auto heap=std::make_shared<ui::Heap>(0x50000);ui::ElementList list;
        auto element=ui::Element::create(heap,list,16,16,48,16,0,{0x400000,0x410000});
        check(static_cast<bool>(element),"native private element allocation");
        element->draw_background();element->set_font(8);
        ui::Text text(resources->font());text.set_print_delay(0);text.set_shadow_offset(0);
        check(element->print(text,resources->name_character(255)),"private fullwidth glyph prints through source text engine");
        const auto glyph=resources->font().glyph(0,0xa3c1);
        check(glyph.size()==32,"private FONT8 glyph exact extent");
        for(unsigned y=0;y<8;++y)for(unsigned x=0;x<8;++x){
            const auto packed=glyph[y*4+x/2];const unsigned coverage=x&1?packed>>4:packed&15;
            const unsigned channel=coverage*17;
            const auto expected=static_cast<std::uint16_t>(((channel<<8)&0xf800)|((channel<<3)&0x7c0)|((channel>>2)&0x3e)|1);
            const auto offset=static_cast<std::size_t>(y*element->stride()+x)*2;
            const auto actual=static_cast<std::uint16_t>((element->image()[offset]<<8)|element->image()[offset+1]);
            check(actual==expected,"private low-first glyph texel matches independent coverage/5551 oracle");
        }
        std::vector<const IsleSprite*> roots;
        for(auto group:{ui::ResourceBank::Group::common,ui::ResourceBank::Group::name})
            for(const auto& r:resources->sprites(group))roots.push_back(r.sprite.view());
        const std::array<sp::BankMapping,2> banks{{{resources->banks()[0].owner,resources->banks()[0].guest},{resources->banks()[1].owner,resources->banks()[1].guest}}};
        sp::Producer producer(roots,banks);const auto binding=producer.register_raster(element->raster());
        std::vector<std::uint32_t> ram(8*1024*1024/4,0xa5a5a5a5);
        producer.install_composed(ram,1,producer.assets());
        const auto plane=element->raster()->pixels(element->displayed_source());
        for(std::size_t i=0;i<plane.size();++i)check(ram_byte(ram,0x400000+static_cast<std::uint32_t>(i))==plane[i],"dynamic source publication uploads canonical raster bytes");
        element->fill(0,0,0,0,{255,0,0,255});producer.install_composed(ram,1,producer.assets());
        check(ram_byte(ram,0x400000)==0xf8&&ram_byte(ram,0x400001)==1,"published changed revision reaches existing producer residency without bank mutation");
        producer.unregister_raster(binding);
        std::cout<<"private window hydration and CPU publication passed; no RSP/RDP/scene execution claimed\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

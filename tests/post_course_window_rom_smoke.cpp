// Opt-in PRIVATE_ROM NEW_OWNED_EMPTY_ROOT. Actual admitted Window assets,
// NativeGame/typed C continuations, Producer, GAME RSP and canonical RDP.
// Authored RGBA16 inputs below are mathematical mutation stimuli, NOT captured
// photos or a Replay provider. No gameplay-progress/scoring/Host clock claims,
// durable save commit, physical input or audio endpoint are made.
// Authority: pokemonsnap df8815e75f3d79618466e01e0827c6165b93d11c,
// camera_check/87A330.c111-146,501-533,677-726;87D1A0.c8-30,82-105,1091;
// photo_check/98C330.c629-662; window/847B60.c109-177.
#include "post_course_window.hpp"
#include "post_course_window_probe.h"
#include "actual_audio_driver.hpp"
#include "scene_storage.hpp"
#include "graphics_task.hpp"
#include "rsp_render.hpp"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
namespace fs=std::filesystem;
namespace pc=isle::post_course;
namespace ui=isle::window;
namespace sp=isle::sprite;
namespace sc=isle::scene;
void require(bool v,const char* reason){if(!v)throw std::runtime_error(reason);}
void ok(IsleGameResult v,const char* reason){require(v==ISLE_GAME_OK,reason);}
constexpr sp::RamRange commands{0x150000,0x50000};
constexpr std::uint32_t color=0x3b5000,depth=0x600000;
constexpr std::array<sp::RamRange,5> reservations{{
    {0,0x150000},{0x1a0000,0x40000},{color,320*240*2},{depth,320*240*2},{0x680000,0x40000}}};
struct Fixture {
    IsleWindowProbe probe{};
    IslePlayerSource player{};
    bool player_active=false;
    std::shared_ptr<const ui::Resources> common;
    std::shared_ptr<const pc::Resources> resources;
    sp::Producer producer;
    isle::game_input::Service input;
    sc::Storage storage;
    std::unique_ptr<sc::Services> services;
    std::shared_ptr<ui::Heap> heap=std::make_shared<ui::Heap>(2*1024*1024);
    ui::Text text;
    std::unique_ptr<ui::Frame> frame;
    std::unique_ptr<ui::Focus> focus;
    std::unique_ptr<ui::Layout> layout;
    std::unique_ptr<ui::Buttons> buttons;
    std::array<pc::WindowRasterSlot,1> element_slots{{{{0x500000,0x510000},0x10000}}};
    std::vector<pc::WindowRasterSlot> thumbnail_slots;
    std::unique_ptr<pc::Window> window;
    std::vector<IsleGameObject*> source_owners;
    unsigned ticks=0;
    Fixture(std::shared_ptr<const ui::Resources> c,std::shared_ptr<const pc::Resources> r,
            isle::game_audio::Session& audio,isle::game::State& state)
      :common(std::move(c)),resources(std::move(r)),producer(resources->roots(),resources->banks()),text(common->font()) {
        ok(storage.create(),"create real Window NativeGame");
        require(input.start({})==isle::game_input::Error::none,"start absent-port input service without invented button edges");
        auto view=resources->view();view.pools={128,128,256,8,1024};
        services=std::make_unique<sc::Services>(storage.game(),storage.arena(),view,input,audio,state);
        require(!services->failed(),services->error());
        frame=std::make_unique<ui::Frame>(*services,*common);
        focus=std::make_unique<ui::Focus>(*services,*common);
        layout=std::make_unique<ui::Layout>(*services,*common);
        buttons=std::make_unique<ui::Buttons>(common,heap,*services);
        std::uint32_t at=0x400000;
        const auto reserve=[&](std::uint32_t id){
            const auto bytes=pc::Window::thumbnail_plane_bytes(*resources,id);
            require(bytes && bytes<0x100000 && at+bytes*2<=0x500000,"explicit thumbnail allocation fits disjoint private GPU plan");
            thumbnail_slots.push_back({{at,static_cast<std::uint32_t>(at+bytes)},bytes});
            at+=static_cast<std::uint32_t>(bytes*2);
        };
        const bool camera=resources->kind()==pc::Resources::Kind::camera_check;
        reserve(camera?0x80201520u:0x801f3b10u);reserve(camera?0x80201520u:0x801f3b10u);
        if(camera)reserve(0x80208900u);
        pc::WindowCapacity capacity;capacity.rasters=element_slots;capacity.thumbnails=thumbnail_slots;
        window=std::make_unique<pc::Window>(*services,text,*frame,*focus,*layout,*buttons,*resources,heap,producer,capacity);
        probe.game=storage.game();probe.window=window->source();
        ok(isle_post_window_text_initialize(probe.window),"actual Window input polling process");
    }
    ~Fixture(){
        probe.aux_target=nullptr;
        for(auto* owner:source_owners)services->erase(owner);
        if(player_active && isle_player_source_deactivate(&player)!=ISLE_GAME_OK)std::terminate();
        window.reset();buttons.reset();layout.reset();focus.reset();frame.reset();text.detach();services.reset();
        IsleGameCounts counts{};
        if(isle_game_counts(storage.game(),&counts)!=ISLE_GAME_OK || counts.objects || counts.processes || counts.sprites ||
           storage.destroy()!=ISLE_GAME_OK)std::terminate();
    }
    void tick(){
        require(input.begin_update()==isle::game_input::Error::none,"actual OM input cache boundary");
        ok(isle_game_tick(storage.game()),"advance actual NativeGame OM processes");++ticks;
        require(!services->failed(),services->error());
    }
    void element(){
        ok(isle_post_window_element_create(probe.window,20,180,128,40,0,&probe.element),"real Producer-bound text Element");
        ok(isle_post_window_element_font(probe.window,probe.element,8),"source font8");
        ok(isle_post_window_text_shadow_offset(probe.window,0),"zero shadow keeps authored test glyph surface bounded");
        ok(isle_post_window_text_delay(probe.window,2),"positive real Source typewriter delay");
        IslePostCourseTextCallbacks callbacks{};callbacks.user=&probe;callbacks.character=isle_window_probe_character;
        ok(isle_post_window_text_callbacks(probe.window,&callbacks),"register actual pure-C character callback");
    }
    IsleGameObject* source(IsleGameSourceCallback callback,IsleGameProcessKind kind,std::uint32_t priority){
        auto* owner=services->object(0x710,0,priority);require(owner,"actual C probe owner creation");
        source_owners.push_back(owner);isle_window_probe_bind(owner,&probe);
        IsleGameProcess* process=nullptr;
        ok(isle_game_add_source_process(storage.game(),owner,callback,kind,0,&process),"install exact typed C probe identity");
        require(process,"native process is not a host dispatcher");return owner;
    }
    void launch(int mode,IsleGameProcessKind kind=ISLE_GAME_STACKFUL){
        probe.mode=mode;
        source(isle_window_probe_heartbeat,ISLE_GAME_SYNCHRONOUS,0xffffffffu);
        source(isle_window_probe_main,kind,1);
    }
    void finish(unsigned maximum=400){
        for(unsigned i=0;i<maximum && !probe.returned;++i)tick();
        require(probe.returned,"actual C driver did not complete within bounded native updates");
    }
    void text_claim_released(){
        std::array<std::uint8_t,64*32*2> pixels{};int x=0,y=0;
        ui::Text::PrintOperation operation;
        const auto result=text.begin_print(operation,{pixels,64,32,2,0,0},x,y,"Z");
        require(result.status==ui::StepStatus::more,"failed/cancelled C driver leaked the actual shared Text operation claim");
        text.cancel_print(operation);
    }
};
void text_cases(const std::shared_ptr<const ui::Resources>& common,const std::shared_ptr<const pc::Resources>& resources,
                isle::game_audio::Session& audio,isle::game::State& state,std::ostream& log){
    {
        Fixture f(common,resources,audio,state);f.element();f.launch(ISLE_WINDOW_PROBE_PRINT);
        f.tick();require(f.probe.entered && !f.probe.returned && f.probe.old_callbacks==1,
                         "first actual C callback/wait was bypassed or completed synchronously");
        f.finish();ok(f.probe.result,"real Source print completion");
        require(f.probe.callback_result==ISLE_GAME_OK && f.probe.callback_waits==1 &&
                f.probe.old_callbacks==1 && f.probe.new_callbacks==2 && f.probe.count==3 && f.probe.beats>1,
                "live registered callback replacement or actual Source callback wait order was lost");
        const auto& font=common->font();
        require(f.probe.characters[0]==font.ascii('A') && f.probe.characters[1]==font.ascii('B') &&
                f.probe.characters[2]==font.ascii('C') && f.probe.character_beats[1]>f.probe.character_beats[0],
                "source encoding/character ordering or wait resumption changed");
        f.text_claim_released();
        log<<"text_callback_replace\t"<<f.ticks<<"\t"<<f.probe.beats<<"\tpass\n";
    }
    {
        Fixture f(common,resources,audio,state);f.element();f.launch(ISLE_WINDOW_PROBE_CANCEL);
        f.tick();require(f.probe.count==1 && !f.probe.returned,"cancellation fixture never reached its real pending text wait");
        ok(isle_post_window_cancel_all(f.probe.window),"cancel actual waiting print claims");
        f.finish();require(f.probe.result==ISLE_GAME_INVALID_STATE && f.probe.count==1 &&
                          isle_post_window_status(f.probe.window)==ISLE_GAME_INVALID_STATE,
                          "cancel acknowledged success, emitted a suffix, or healed sticky failure");
        f.text_claim_released();log<<"text_cancel_pending\t"<<f.ticks<<"\t"<<f.probe.beats<<"\tpass\n";
    }
    for(const bool nested:{false,true}) {
        Fixture f(common,resources,audio,state);f.element();
        if(nested){
            ok(isle_player_source_prepare(f.storage.game(),&f.player,&f.services->rng,&f.probe,1),"prepare genuine nonwaiting C fence");
            ok(isle_player_source_activate(&f.player),"activate canonical per-game Source nonwaiting guard");
            f.player_active=true;f.probe.player=&f.player;
        }
        f.launch(nested?ISLE_WINDOW_PROBE_NESTED:ISLE_WINDOW_PROBE_SYNC,
                 nested?ISLE_GAME_STACKFUL:ISLE_GAME_SYNCHRONOUS);
        f.tick();require(f.probe.returned && f.probe.result==ISLE_GAME_INVALID_STATE && f.probe.count==1 &&
                         isle_game_status(f.storage.game())==ISLE_GAME_OK,
                         "forbidden Source wait suspended, consumed a later update, or faulted native cleanup");
        if(nested)require(f.probe.nested_result==ISLE_GAME_OK && f.probe.zero_wait_result==ISLE_GAME_OK &&
                          !f.player.nonwaiting_call_frame,"nested nonwaiting C fence did not restore its actual owner");
        f.text_claim_released();log<<(nested?"nested_no_wait":"sync_no_wait")<<"\t"<<f.ticks<<"\t"<<f.probe.beats<<"\tpass\n";
    }
}
void layout_case(const std::shared_ptr<const ui::Resources>& common,const std::shared_ptr<const pc::Resources>& resources,
                 isle::game_audio::Session& audio,isle::game::State& state,std::ostream& log){
    Fixture f(common,resources,audio,state);
    ok(isle_post_window_layout_initialize(f.probe.window),"real source Layout furniture/processes");
    ok(isle_post_window_table_resource(f.probe.window,0x8020896c,&f.probe.table),"actual five-entry CameraCheck table");
    f.launch(ISLE_WINDOW_PROBE_LAYOUT);f.finish();ok(f.probe.result,"C-driven real Layout create/transition");
    const std::array<int,9> expected{23,35,35,35,35,35,24,22,35};
    require(std::all_of(std::begin(f.probe.slots_before),std::end(f.probe.slots_before),[](int id){return id==35;}) &&
            std::equal(expected.begin(),expected.end(),f.probe.slots_after) &&
            f.probe.actions[0]==23 && f.probe.actions[1]==24 && f.probe.actions[2]==22 &&
            f.probe.help_sizes[0] && f.probe.help_sizes[1] && f.probe.help_sizes[2],
            "selector obtained before creation was a snapshot, lost splitter slots, or dispatched stale IDs");
    // Replacement uses the same retained context. This authored legal table is
    // a mutation stimulus for native Layout, not a second slot-state authority.
    const std::array<IslePostCourseButton,4> replacement{{{26,{"A",1}},{34,{}},{5,{"B",1}},{35,{}}}};
    ok(isle_post_window_table_borrow(f.probe.window,replacement.data(),replacement.size(),&f.probe.table),"borrow stable replacement table");
    f.probe.entered=f.probe.returned=0;
    f.source(isle_window_probe_main,ISLE_GAME_STACKFUL,1);f.finish();ok(f.probe.result,"actual replacement hide/build/wait");
    require(std::equal(expected.begin(),expected.end(),f.probe.slots_before) && f.probe.slots_after[0]==26 &&
            f.probe.slots_after[6]==35 && f.probe.slots_after[7]==5 &&
            f.probe.actions[0]==26 && f.probe.actions[1]==5 && f.probe.actions[2]==26 &&
            f.probe.help_sizes[0]==1 && f.probe.help_sizes[1]==1 && f.probe.help_sizes[2]==1,
            "retained canonical selector failed after real table replacement");
    log<<"live_layout_selector\t"<<f.ticks<<"\t"<<f.probe.beats<<"\tpass\n";
    // End all Source consumers while replacement's borrowed stack backing lives.
    for(auto* owner:f.source_owners)f.services->erase(owner);f.source_owners.clear();
    ok(f.window->retire(),"retire partial/table borrows before replacement backing expires");
}
struct PhotoPixels {
    std::size_t width,height,stride;
    unsigned seed;
    std::vector<std::uint8_t> bytes;
    PhotoPixels(std::size_t w,std::size_t h,unsigned s):width(w),height(h),stride(w*4+8),seed(s),bytes(stride*h*2,0x6d){
        for(std::size_t y=0;y<h*2;++y)for(std::size_t x=0;x<w*2;++x){
            const unsigned q=unsigned((y&1)*2+(x&1)),tx=unsigned(x/2),ty=unsigned(y/2);
            const unsigned r=(tx+3*ty+q+seed)&31,g=(2*tx+ty+2*q+seed*3)&31,b=(tx/2+7*ty+3*q+seed*5)&31;
            const auto value=(r<<11)|(g<<6)|(b<<1)|(q&1);
            const auto at=y*stride+x*2;bytes[at]=std::uint8_t(value>>8);bytes[at+1]=std::uint8_t(value);
        }
    }
    std::uint16_t expected(std::size_t x,std::size_t y) const {
        unsigned r=0,g=0,b=0;
        for(unsigned q=0;q<4;++q){
            r+=(unsigned(x)+3*unsigned(y)+q+seed)&31;
            g+=(2*unsigned(x)+unsigned(y)+2*q+seed*3)&31;
            b+=(unsigned(x)/2+7*unsigned(y)+3*q+seed*5)&31;
        }
        return std::uint16_t(((r/4)<<11)|((g/4)<<6)|((b/4)<<1)|1);
    }
};
struct Child {IsleGameSprite* sprite=nullptr;IslePostCourseThumbnail image{};std::uint32_t identity=0;int x=0,y=0;};
IsleSprite& native(Fixture& f,IsleGameSprite* sprite){IsleSprite* p=nullptr;ok(isle_post_window_sprite_view(f.probe.window,sprite,&p),"actual native mutable sprite view");require(p,"native view");return *p;}
Child child(Fixture& f,IsleGameSprite* parent,std::uint32_t identity,int x,int y){
    Child c;c.identity=identity;c.x=x;c.y=y;
    ok(isle_post_window_sprite_child(f.probe.window,parent,identity,&c.sprite),"create actual ROM thumbnail child hierarchy");
    auto& view=native(f,c.sprite);isle_sprite_move(&view,x,y);view.scalex=view.scaley=1;
    view.attr=ISLE_SP_TEXSHUF|ISLE_SP_TRANSPARENT;isle_sprite_color(&view,255,255,255,255);
    ok(isle_post_window_thumbnail_create(f.probe.window,c.sprite,identity,&c.image),"bind independent actual prototype bitmap pieces");
    return c;
}
void write(Fixture& f,const Child& c,const PhotoPixels& source){
    ok(isle_post_window_thumbnail_write_photo(f.probe.window,c.image,source.bytes.data(),source.bytes.size(),source.stride),"publish exact Source Photo reduction");
}
void pixels(Fixture& f,const Child& c,const PhotoPixels& source){
    const auto& view=native(f,c.sprite);const auto& prototype=*f.resources->roots()[f.resources->sprite_index(c.identity)];
    require(view.bitmap!=prototype.bitmap && view.nbitmaps==prototype.nbitmaps,"dynamic binding replaced topology or mutated immutable prototype descriptors");
    std::size_t row_in_piece=0,piece=0;
    const auto stride=std::size_t(prototype.bitmap[0].width_img),tile=std::size_t(prototype.bitmap[0].actualHeight);
    for(std::size_t y=0;y<source.height;++y){
        for(std::size_t x=0;x<source.width;++x){
            auto offset=(row_in_piece*stride+x)*2;if(row_in_piece&1)offset^=4;
            const auto* p=view.bitmap[piece].buf+offset;
            const auto actual=(unsigned(p[0])<<8)|p[1],expected=unsigned(source.expected(x,y));
            if(actual!=expected){
                std::cerr<<"thumbnail reduction child=0x"<<std::hex<<c.identity<<std::dec
                         <<" seed="<<source.seed<<" local="<<x<<','<<y
                         <<" piece="<<piece<<" piece_row="<<row_in_piece<<" byte_offset="<<offset
                         <<" expected_rgba5551="<<expected<<" actual_rgba5551="<<actual<<'\n';
                require(false,"actual thumbnail bytes lost averaging, source row stride, bitmap-local row reset or XOR4");
            }
        }
        if(++row_in_piece==tile){row_in_piece=0;++piece;}
    }
}
std::vector<sp::Draw> draws(Fixture& f,std::span<const Child> children){
    std::array<sc::SpriteDraw,256> all{};std::size_t count=0;
    require(f.services->sprite_draws(all,count),"prepare actual canonical native hierarchy draws");
    std::vector<sp::Draw> selected;
    for(const auto& child:children){
        const auto* bitmap=native(f,child.sprite).bitmap;
        const auto found=std::find_if(all.begin(),all.begin()+count,[&](const auto& d){return d.draw.view->bitmap==bitmap;});
        require(found!=all.begin()+count && found->draw.generation!=0,"actual child draw lost registered dynamic generation/hierarchy");
        selected.push_back(found->draw);
    }
    return selected;
}
std::uint16_t pixel(const std::vector<std::byte>& image,unsigned x,unsigned y){
    const auto at=(y*320+x)*2;return std::uint16_t((unsigned(image[at^3u])<<8)|unsigned(image[(at+1)^3u]));
}
void artifact(const fs::path& base,const std::vector<std::byte>& image){
    auto ppm=base,raw=base;ppm+=".ppm";raw+=".rgba5551";
    require(!fs::exists(ppm) && !fs::exists(raw),"private framebuffer artifacts must not overwrite existing paths");
    std::ofstream p(ppm,std::ios::binary),r(raw,std::ios::binary);p<<"P6\n320 240\n255\n";
    for(unsigned y=0;y<240;++y)for(unsigned x=0;x<320;++x){
        const auto v=pixel(image,x,y);const char rgba[]{char(v>>8),char(v)};r.write(rgba,2);
        const char rgb[]{char(((v>>11)&31)*255/31),char(((v>>6)&31)*255/31),char(((v>>1)&31)*255/31)};p.write(rgb,3);
    }
    p.flush();r.flush();
    require(bool(p)&&bool(r),"actual completed framebuffer evidence write failed");
}
struct Gpu {
    const isle::rom::Snapshot& rom;
    isle::rsp::CanonicalExecutor executor{true,{4000000,524288}};
    bool cold=true;
    unsigned frames=0;
    explicit Gpu(const isle::rom::Snapshot& r):rom(r){}
    std::vector<std::byte> render(Fixture& f,std::span<const Child> children,const Child* reject=nullptr,const PhotoPixels* input=nullptr){
        auto selected=draws(f,children);
        auto preparation=executor.begin_preparation();auto borrow=f.producer.borrow();auto ram=preparation.words();
        if(cold)isle::graphics::validate_cold_inputs(ram,rom,rom.canonical().data(),0x147000);
        f.producer.install(ram,1,reservations);
        if(cold)isle::graphics::install_cold_inputs(ram,rom,0x147000);
        const auto list=f.producer.prepare(ram,1,selected,{},commands,reservations);
        isle::graphics::Memory memory(ram);std::uint32_t at=0x100000;
        const auto cmd=[&](std::uint32_t a,std::uint32_t b){memory.word(at,a);memory.word(at+4,b);at+=8;};
        cmd(0xdb060000,0);cmd(0xe7000000,0);cmd(0xed000000,(1280u<<12)|960u);
        cmd(0xff10013f,depth);cmd(0xef300cf0,0);cmd(0xf7000000,0xfffcfffc);cmd(0xf6000000|(1276u<<12)|956u,0);
        cmd(0xe7000000,0);cmd(0xff10013f,color);cmd(0xf7000000,0x00010001);cmd(0xf6000000|(1276u<<12)|956u,0);
        cmd(0xe7000000,0);cmd(0xfe000000,depth);cmd(0xde000000,list.address);cmd(0xe9000000,0);cmd(0xdf000000,0);
        const auto task=isle::graphics::finish_task(ram,rom,0x147000,0x144000,0x145000,0x146000,0x680000,0x6c0000,0x100000);
        auto completed=executor.execute(std::move(preparation),task.task,task.boot);cold=false;++frames;
        require(bool(completed.rsp_result()) && completed.rsp_result().full_syncs==1,"actual GAME RSP/RDP FullSync execution");
        if(reject){
            require(input,"borrow rejection has real source stimulus");
            const auto& view=native(f,reject->sprite);
            std::vector<std::vector<std::uint8_t>> before;
            before.reserve(std::size_t(view.nbitmaps));
            for(int b=0;b<view.nbitmaps;++b){
                const auto& bitmap=view.bitmap[b];
                if(bitmap.buf && bitmap.byte_count)before.emplace_back(bitmap.buf,bitmap.buf+bitmap.byte_count);
                else before.emplace_back();
            }
            const auto written=isle_post_window_thumbnail_write_photo(f.probe.window,reject->image,input->bytes.data(),input->bytes.size(),input->stride);
            const auto released=isle_post_window_thumbnail_release(f.probe.window,reject->image);
            if(written!=ISLE_GAME_INVALID_STATE || released!=ISLE_GAME_INVALID_STATE){
                std::cerr<<"thumbnail held-result child=0x"<<std::hex<<reject->identity<<std::dec
                         <<" write_result="<<written<<" release_result="<<released<<'\n';
                require(false,"actual GPU/result borrower admitted pixel mutation or dynamic unregistration");
            }
            for(int b=0;b<view.nbitmaps;++b){
                const auto& saved=before[std::size_t(b)];
                const auto changed=std::mismatch(saved.begin(),saved.end(),view.bitmap[b].buf);
                if(changed.first!=saved.end()){
                    std::cerr<<"thumbnail held-result child=0x"<<std::hex<<reject->identity<<std::dec
                             <<" piece="<<b<<" byte_offset="<<changed.first-saved.begin()
                             <<" before="<<unsigned(*changed.first)<<" after="<<unsigned(*changed.second)<<'\n';
                    require(false,"borrow rejection changed actual native pixels in a bitmap piece");
                }
            }
        }
        const auto bytes=completed.rdram();std::vector<std::byte> image(bytes.begin()+color,bytes.begin()+color+320*240*2);
        completed.release();return image;
    }
};
std::uint16_t normal_source_pixel(std::uint16_t texel){
    // Independent closed-form oracle for this fixture's integer-aligned,
    // unit-scale RGBA16 draw: BILERP has zero fractional weights, then the
    // normal Source GUI path modulates RGBA by primitive255 and force-blends
    // over the black clear. It is NOT a texture-to-framebuffer COPY.
    // Public pipeline: Nintendo N64 Programming Manual chapter12, TF/CC/BL:
    // https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro12/12-05.html
    // (12-06.html and 12-07.html in the same directory).
    // Exact public arithmetic authority: https://github.com/Themaister/parallel-rdp
    // commit1cecd042b2619bc505c12bfdc713808386f2b54d, parallel-rdp/shaders/
    // texture.h75-80,903-910; combiner.h34-46,260-279;
    // blender.h83-125; coverage.h51-61; memory_interfacing.h212-221.
    // Alpha round(255*255/256)=254 is NOT the blender's alpha255 bypass.
    // Its 5-bit blend weight therefore matters even with white primitive RGBA.
    constexpr unsigned alpha=(255u*255u+128u)/256u;
    const auto channel=[](unsigned value){
        const unsigned expanded=(value<<3)|(value>>2);
        const unsigned combined=(expanded*255u+128u)/256u;
        const unsigned blended=(combined*(alpha/8u))/32u; // memory RGB is zero
        return blended/8u; // RGB dither disabled; framebuffer truncates to RGB5
    };
    // Full integer rectangles have coverage8; CLAMP stores coverage7, whose
    // high bit is framebuffer bit0. It is not the source texture's alpha bit.
    return std::uint16_t((channel((texel>>11)&31u)<<11)|
                         (channel((texel>>6)&31u)<<6)|(channel((texel>>1)&31u)<<1)|1u);
}
void image_matches(const std::vector<std::byte>& image,Fixture& f,std::span<const Child> children,
                   std::span<const PhotoPixels* const> sources,const char* phase){
    require(children.size()==sources.size() && image.size()==320*240*2,"complete framebuffer oracle geometry");
    // Check every pixel, not sparse interior samples: all rows, bitmap seams,
    // final short pieces, independent sibling rectangles and untouched guards.
    for(unsigned y=0;y<240;++y)for(unsigned x=0;x<320;++x){
        std::size_t selected=children.size(),local_x=0,local_y=0;
        for(std::size_t i=0;i<children.size();++i){
            const auto& c=children[i];const auto& source=*sources[i];
            if(int(x)<c.x || int(y)<c.y || std::size_t(int(x)-c.x)>=source.width ||
               std::size_t(int(y)-c.y)>=source.height)continue;
            require(selected==children.size(),"authored independent thumbnail placements overlap");
            selected=i;local_x=std::size_t(int(x)-c.x);local_y=std::size_t(int(y)-c.y);
        }
        const auto texel=selected==children.size()?std::uint16_t{1}:sources[selected]->expected(local_x,local_y);
        const auto expected=selected==children.size()?std::uint16_t{1}:normal_source_pixel(texel);
        const auto actual=pixel(image,x,y);
        if(actual==expected)continue;
        std::cerr<<"thumbnail framebuffer phase="<<phase<<" screen="<<x<<','<<y;
        if(selected==children.size())std::cerr<<" outside_all_children";
        else{
            const auto& c=children[selected];const auto& view=native(f,c.sprite);
            const auto tile=std::size_t(view.bitmap[0].actualHeight);
            std::cerr<<" child_index="<<selected<<" child=0x"<<std::hex<<c.identity<<std::dec
                     <<" seed="<<sources[selected]->seed<<" local="<<local_x<<','<<local_y
                     <<" piece="<<local_y/tile<<" piece_row="<<local_y%tile
                     <<" reduced_texel_rgba5551="<<texel;
        }
        std::cerr<<" expected_rgba5551="<<expected<<" actual_rgba5551="<<actual
                 <<" expected_rgb5="<<(expected>>11)<<','<<((expected>>6)&31u)<<','<<((expected>>1)&31u)
                 <<" actual_rgb5="<<(actual>>11)<<','<<((actual>>6)&31u)<<','<<((actual>>1)&31u)<<'\n';
        require(false,"published native children differ from independent normal Source GUI RDP arithmetic or framebuffer guards");
    }
}
void raster_case(const isle::rom::Snapshot& rom,const std::shared_ptr<const ui::Resources>& common,
                 const std::shared_ptr<const pc::Resources>& resources,isle::game_audio::Session& audio,
                 isle::game::State& state,const fs::path& output,std::ostream& log){
    Fixture f(common,resources,audio,state);Gpu gpu(rom);
    const bool camera=resources->kind()==pc::Resources::Kind::camera_check;
    IsleGameObject* object=nullptr;IsleGameSprite* parent=nullptr;
    ok(isle_post_window_sprite_object_create(f.probe.window,6,camera?0x801fe318u:0x801e5db0u,&object,&parent),"real source sprite parent");
    auto& root=native(f,parent);isle_sprite_move(&root,0,0);isle_sprite_color(&root,255,255,255,255);root.attr&=~ISLE_SP_HIDDEN;
    const auto small=camera?0x80201520u:0x801f3b10u;
    std::vector<Child> children{child(f,parent,small,20,20),child(f,parent,small,90,20)};
    if(camera)children.push_back(child(f,parent,0x80208900,160,20));
    std::vector<PhotoPixels> sources;
    std::vector<std::vector<std::vector<std::uint8_t>>> immutable;
    for(std::size_t i=0;i<children.size();++i){
        const auto& view=native(f,children[i].sprite);
        sources.emplace_back(std::size_t(view.width),std::size_t(view.height),unsigned(i+1));
        require(children[i].x+view.width<=320 && children[i].y+view.height<=240,"real prototype fits authored sample placement");
        const auto& prototype=*resources->roots()[resources->sprite_index(children[i].identity)];
        immutable.emplace_back();
        for(int b=0;b<prototype.nbitmaps;++b) {
            const auto& bitmap=prototype.bitmap[b];
            if(bitmap.buf && bitmap.byte_count)immutable.back().emplace_back(bitmap.buf,bitmap.buf+bitmap.byte_count);
            else immutable.back().emplace_back();
        }
        write(f,children[i],sources.back());pixels(f,children[i],sources.back());
        log<<(camera?"camera":"photo")<<"_topology\t"<<view.width<<'x'<<view.height<<"\tpieces="<<view.nbitmaps
           <<"\tfirst_actual_height="<<view.bitmap[0].actualHeight<<"\tplane="<<pc::Window::thumbnail_plane_bytes(*resources,children[i].identity)<<'\n';
    }
    require(native(f,children[0].sprite).bitmap[0].buf!=native(f,children[1].sprite).bitmap[0].buf,"two original thumbnail children alias writable pixels");
    if(camera)require(native(f,children.back().sprite).nbitmaps>1,"large preview fixture did not exercise its actual multi-bitmap consumer");
    f.probe.aux_target=children[0].sprite;
    f.source(isle_window_probe_heartbeat,ISLE_GAME_SYNCHRONOUS,0xffffffffu);
    for(int mode=0;mode<3;++mode){
        ok(isle_post_window_sprite_aux_set(f.probe.window,children[0].sprite,mode),"set actual SObj marker mode");f.tick();
        int32_t raw=-1;ok(isle_game_sprite_aux_get(f.storage.game(),children[0].sprite,&raw),"independent actual engine SObj read");
        require(f.probe.aux_result==ISLE_GAME_OK && f.probe.aux_observed==mode && raw==mode,
                "original C consumer and checked engine field disagree on native unk_54");
    }
    f.probe.aux_target=nullptr;
    const auto prefix=camera?"camera":"photo";
    std::array<const PhotoPixels*,3> source_views{};
    for(std::size_t i=0;i<sources.size();++i)source_views[i]=&sources[i];
    const auto all_sources=std::span<const PhotoPixels* const>(source_views.data(),sources.size());
    auto previous=gpu.render(f,children);image_matches(previous,f,children,all_sources,"cold-initial");
    artifact(output/(std::string(prefix)+"-initial"),previous);
    const auto warm=gpu.render(f,children);image_matches(warm,f,children,all_sources,"warm-unmodified");
    constexpr std::array<const char*,3> revision_phases{{
        "warm-revision-child-0","warm-revision-child-1","warm-revision-child-2"}};
    for(std::size_t updated_child=0;updated_child<children.size();++updated_child){
        sources[updated_child]=PhotoPixels(sources[updated_child].width,sources[updated_child].height,
                                          13u+unsigned(updated_child)*5u);
        write(f,children[updated_child],sources[updated_child]);
        for(std::size_t i=0;i<children.size();++i)pixels(f,children[i],sources[i]);
        auto updated=gpu.render(f,children);
        image_matches(updated,f,children,all_sources,revision_phases[updated_child]);
        require(previous!=updated,"independent child revision did not change actual completed framebuffer");
        artifact(output/(std::string(prefix)+"-revision-"+std::to_string(updated_child)),updated);
        previous=std::move(updated);
    }
    artifact(output/(std::string(prefix)+"-updated"),previous);
    ok(isle_post_window_thumbnail_release(f.probe.window,children[1].image),"release only after actual GPU/readback/Producer retirement");
    require(native(f,children[1].sprite).bitmap==resources->roots()[resources->sprite_index(small)]->bitmap,
            "release did not restore immutable resource layout on the same native child");
    std::vector<Child> retained{children[0]};if(camera)retained.push_back(children.back());
    const std::array<const PhotoPixels*,2> retained_sources{{&sources[0],&sources.back()}};
    const auto& reject=retained.back();const auto& rejected_source=camera?sources.back():sources[0];
    const PhotoPixels forbidden(rejected_source.width,rejected_source.height,29);
    const auto guarded=gpu.render(f,retained,&reject,&forbidden);
    image_matches(guarded,f,retained,std::span<const PhotoPixels* const>(retained_sources.data(),retained.size()),"warm-held-result");
    artifact(output/(std::string(prefix)+"-borrow-guard"),guarded);
    require(gpu.executor.quiescent() && isle_post_window_status(f.probe.window)==ISLE_GAME_INVALID_STATE,
            "borrow rejection healed sticky failure or published an unretired GPU result");
    for(const auto& c:retained)ok(isle_post_window_thumbnail_release(f.probe.window,c.image),"cleanup after real borrower release preserves sticky execution failure");
    int32_t marker=-1;
    ok(isle_game_sprite_aux_get(f.storage.game(),children[0].sprite,&marker),"actual marker field after dynamic binding restoration");
    require(marker==2,"dynamic raster release replaced the native SObj or discarded its actual marker state");
    for(std::size_t i=0;i<children.size();++i){
        const auto& prototype=*resources->roots()[resources->sprite_index(children[i].identity)];
        for(int b=0;b<prototype.nbitmaps;++b)
            require(std::equal(immutable[i][std::size_t(b)].begin(),immutable[i][std::size_t(b)].end(),prototype.bitmap[b].buf),
                    "source thumbnail update mutated an immutable ROM pixel piece");
    }
    log<<prefix<<"_actual_rsp_rdp\tframes="<<gpu.frames
       <<"\tfull_frame_exact_normal_source_math_all_child_revisions_release_all_piece_borrow_guard\tpass\n";
}
void mixed_size_reuse_case(const isle::rom::Snapshot& rom,const std::shared_ptr<const ui::Resources>& common,
                          const std::shared_ptr<const pc::Resources>& resources,isle::game_audio::Session& audio,
                          isle::game::State& state,const fs::path& output,std::ostream& log){
    Fixture f(common,resources,audio,state);Gpu gpu(rom);
    IsleGameObject* object=nullptr;IsleGameSprite* parent=nullptr;
    ok(isle_post_window_sprite_object_create(f.probe.window,6,0x801fe318u,&object,&parent),"mixed-size actual Camera parent");
    auto& root=native(f,parent);isle_sprite_move(&root,0,0);isle_sprite_color(&root,255,255,255,255);root.attr&=~ISLE_SP_HIDDEN;
    // Creation order matters: a shared first-fit pool first places the large
    // pair before both small pairs. Capacities remain the admitted two-small,
    // one-large plan used by the other Camera consumer case.
    std::array<Child,3> children{{
        child(f,parent,0x80208900u,160,20),
        child(f,parent,0x80201520u,20,20),
        child(f,parent,0x80201520u,90,20)}};
    std::array<PhotoPixels,3> sources{{
        PhotoPixels(140,105,37),PhotoPixels(52,39,41),PhotoPixels(52,39,43)}};
    std::array<const PhotoPixels*,3> source_views{{&sources[0],&sources[1],&sources[2]}};
    for(std::size_t i=0;i<children.size();++i){
        const auto& view=native(f,children[i].sprite);
        require(std::size_t(view.width)==sources[i].width && std::size_t(view.height)==sources[i].height,
                "mixed-size Camera consumers require actual admitted large and small geometry");
        write(f,children[i],sources[i]);pixels(f,children[i],sources[i]);
    }
    const auto initial=gpu.render(f,children);
    image_matches(initial,f,children,source_views,"mixed-size-initial");
    require(gpu.executor.quiescent(),"mixed-size release requires actual graphics retirement");
    ok(isle_post_window_thumbnail_release(f.probe.window,children[0].image),"retired large Camera binding release");
    ok(isle_post_window_thumbnail_release(f.probe.window,children[2].image),"retired second small Camera binding release");
    for(const auto index:{0u,2u})
        require(native(f,children[index].sprite).bitmap==
                    resources->roots()[resources->sprite_index(children[index].identity)]->bitmap,
                "mixed-size release must restore the same native child's immutable binding");
    // The surviving first small pair separates the free regions. Recreating
    // small1 must not consume large's retained capacity and strand its slot.
    ok(isle_post_window_thumbnail_create(f.probe.window,children[2].sprite,children[2].identity,&children[2].image),
       "recreate second small Camera thumbnail after real graphics retirement");
    sources[2]=PhotoPixels(52,39,47);write(f,children[2],sources[2]);
    ok(isle_post_window_thumbnail_create(f.probe.window,children[0].sprite,children[0].identity,&children[0].image),
       "recreate large Camera thumbnail with its admitted slot still available");
    sources[0]=PhotoPixels(140,105,53);write(f,children[0],sources[0]);
    for(std::size_t i=0;i<children.size();++i)pixels(f,children[i],sources[i]);
    const auto reused=gpu.render(f,children);
    image_matches(reused,f,children,source_views,"mixed-size-recreated");
    require(initial!=reused,"mixed-size recreation must publish changed real Camera pixels");
    artifact(output/"camera-mixed-size-recreated",reused);
    require(gpu.executor.quiescent(),"mixed-size final cleanup requires actual graphics retirement");
    for(const auto& c:children)
        ok(isle_post_window_thumbnail_release(f.probe.window,c.image),"retire every mixed-size Camera binding after real graphics completion");
    log<<"camera_mixed_size_reuse\tframes="<<gpu.frames
       <<"\tlarge_small_small_release_large_small_recreate_small_large_full_frame_exact\tpass\n";
}
void run(const fs::path& rom_path,const fs::path& root){
    require(root.is_absolute() && fs::is_directory(fs::symlink_status(root)) && !fs::is_symlink(fs::symlink_status(root)) &&
            fs::is_empty(root) && fs::canonical(root)==root.lexically_normal(),"NEW_OWNED_EMPTY_ROOT must exist, be empty, canonical, absolute and nonsymlink");
    const auto rom=isle::rom::read_snapshot(rom_path);const auto common=ui::Resources::load(rom);
    const auto camera=pc::Resources::load(rom,common,pc::Resources::Kind::camera_check);
    const auto photo=pc::Resources::load(rom,common,pc::Resources::Kind::photo_check);
    require(fs::create_directory(root/"profile") && fs::create_directory(root/"artifacts"),"create only new owned private output directories");
    isle::game::State state(root/"profile");require(state.initialize(1),state.error());
    auto hydrated=isle::game_audio::hydrate(rom);require(bool(hydrated),"actual ROM Audio resource admission");
    std::array<isle::audio::effects::Section,8> sections{};isle::audio::effects::Config effects;
    require(isle::game_audio::native_startup_effects(hydrated.resources,sections,effects)==isle::audio::Error::none,"actual default Audio effects");
    isle::private_audio::Capture pcm;isle::private_audio::Execution executor{rom,nullptr,{}};
    auto audio=isle::game_audio::Session::create(std::move(hydrated.resources),effects,pcm.output(),executor.executor());
    require(bool(audio)&&executor.machine,"real checked PCM/RSP Audio Session for Services");
    std::ofstream log(root/"artifacts"/"chronology.tsv");require(bool(log),"create fresh owned evidence chronology");
    log<<"case\tactual_updates_or_geometry\tactual_beats_or_topology\tresult\n";
    text_cases(common,camera,*audio.session,state,log);layout_case(common,camera,*audio.session,state,log);
    raster_case(rom,common,camera,*audio.session,state,root/"artifacts",log);
    mixed_size_reuse_case(rom,common,camera,*audio.session,state,root/"artifacts",log);
    raster_case(rom,common,photo,*audio.session,state,root/"artifacts",log);
    require(pcm.used==0 && pcm.finishes==0 && pcm.polls==0 && audio.session->accepted_frames()==0,"Window tests must not fabricate Audio task/output progress");
    audio.session.reset();require(!executor.machine && !executor.borrowed,"actual Audio RSP ownership retired");
    log.flush();
    require(bool(log),"flush private chronology evidence");
    std::cout<<"post-course Window ROM/native-C/actual-RSP-RDP regressions passed\n"
                "scope: actual Window assets and native consumers; authored pixel stimuli, not captured photos/replay/scoring or full scenes\n";
}
}
#ifdef _WIN32
int wmain(int argc,wchar_t** argv){
#else
int main(int argc,char** argv){
#endif
    try{require(argc==3,"expected PRIVATE_ROM NEW_OWNED_EMPTY_ROOT");run(fs::path(argv[1]),fs::path(argv[2]));return 0;}
    catch(const fs::filesystem_error&){std::cerr<<"Window smoke: private filesystem operation failed\n";}
    catch(const std::exception& e){std::cerr<<"Window smoke: "<<e.what()<<'\n';}
    return 1;
}

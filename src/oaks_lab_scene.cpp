#include "oaks_lab_scene.hpp"
#include "oaks_lab_resources.hpp"
#include "window_element.hpp"
#include "window_widgets.hpp"
#include "snap_station.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <exception>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace isle::oaks_lab {
Navigation source_navigation(std::span<const window::Button,8> slots,int slot,const game_input::MenuInput& input) noexcept {
    Navigation result{slot,0,(input.pressed&0x4000)!=0};
    if(input.current&(game_input::stick_slow_left|game_input::stick_slow_right))return result;
    for(auto direction:{game_input::stick_slow_up,game_input::stick_slow_down})if(input.pressed&direction) {
        for(unsigned tries=0;tries<8;++tries){result.slot=(result.slot+(direction==game_input::stick_slow_up?7:1))%8;if(slots[result.slot].id!=35)break;}
        ++result.move_sounds;
    }
    return result;
}
bool commit_acknowledged(const save::CommitResult& result) noexcept {
    return result.outcome==save::CommitOutcome::published && result.generation!=0 &&
           result.durability!=save::Durability::none && !result.error && !result.cleanup_error;
}
namespace {
constexpr std::uint32_t a_button=0x8000,b_button=0x4000,start_button=0x1000;
constexpr std::array<std::uint32_t,5> first_information{0x801c598c,0x801c59c4,0x801c5a08,0x801c5a4c,0x801c5aa0};
constexpr std::array<std::uint32_t,5> challenge{0x801c5b78,0x801c5bbc,0x801c5bf0,0x801c5c30,0x801c5c80};
constexpr std::array<std::uint32_t,4> report_gift{0x801c5de4,0x801c5e04,0x801c5e30,0x801c5e68};
constexpr std::array<std::uint32_t,3> dash_information{0x801c5f20,0x801c5f54,0x801c5fa4};
constexpr std::array<std::uint32_t,8> rainbow{0x801c6138,0x801c616c,0x801c61c8,0x801c6200,0x801c6248,0x801c6284,0x801c62bc,0x801c62ec};
constexpr std::array<std::uint32_t,7> apple{0x801c6428,0x801c6454,0x801c648c,0x801c64dc,0x801c6528,0x801c657c,0x801c65d8};
constexpr std::array<std::uint32_t,3> pester{0x801c65e8,0x801c664c,0x801c6688};
constexpr std::array<std::uint32_t,4> flute{0x801c66c0,0x801c66ec,0x801c6738,0x801c6770};
constexpr std::array<std::uint32_t,6> completed_report{0x801c67fc,0x801c6820,0x801c685c,0x801c6880,0x801c68d8,0x801c692c};
constexpr std::array<std::uint32_t,5> mew{0x801c6968,0x801c69ac,0x801c69e0,0x801c6a18,0x801c6a50};
}
struct Scene::Impl {
    scene::Services& s;
    std::shared_ptr<const Resources> r;
    sprite::Producer& producer;
    Config config;
    std::shared_ptr<window::Heap> heap=std::make_shared<window::Heap>(0x40000);
    window::ElementList elements;
    window::Text text;
    window::Buttons button_images;
    window::Frame frame;
    window::Focus focus;
    window::Layout layout;
    std::shared_ptr<window::Element> dialog,fade;
    sprite::RasterBinding dialog_binding{},fade_binding{};
    std::array<scene::SpriteHandle,18> art{};
    std::array<window::Color,8> source_colors{};
    std::array<std::uint32_t,18> bound_identity{},bound_table{};
    std::array<unsigned,18> bound_phase{};
    scene::SpriteHandle fade_sprite{};
    std::array<IsleGameObject*,4> cameras{};
    std::array<IsleGameCamera*,4> camera_handles{};
    IsleGameObject* owner=nullptr;
    IsleGameObject* input_object=nullptr;
    save::GameSnapshot saved;
    std::array<char,17> player_name{};
    std::array<scene::SpriteDraw,256> draws{};
    std::array<scene::render::Pass,4> passes{};
    std::array<sprite::RamRange,3> reservations{};
    std::vector<sprite::AssetSpan> immutable;
    View observed{};
    int brightness=0,expression=0,mouth_timer=0,eye_timer=180,mouth_frame=0,mouth_clock=12,mouth_index=0;
    int icon_mode=0,music_mode=3,music_delay=0,music_song=-1,initial_slot=0;
    int route_mode=0,route_course=0,gate_mode=0;
    int gate_clock=12;
    unsigned gate_frame=0;
    float plaque_angle=0;
    bool plaque_animating=false;
    bool retired=false,started=false,item_song=false,route_busy=false,route_complete=false,illegal_back=false;
    float icon_y=-72;

    Impl(scene::Services& services,std::shared_ptr<const Resources> resources,sprite::Producer& p,Config c)
        :s(services),r(std::move(resources)),producer(p),config(c),text(r->window()->font()),
         button_images(r->window(),heap,s),frame(s,*r->window()),focus(s,*r->window()),layout(s,*r->window()) {
        if(config.level_id<0 || config.level_id>=7) config.level_id=0;
        observed.selected_course=config.level_id;
        for(unsigned i=0;i<reservations.size();++i)
            reservations[i]={config.sprite_commands.address+i*config.sprite_commands.size,config.sprite_commands.size};
    }
    bool refresh() {
        auto value=s.save.snapshot();
        if(!value){s.fail("Oak persistent save snapshot unavailable");return false;}
        saved=std::move(value.value);return true;
    }
    bool flag(unsigned id) const {const auto v=saved.player_flag(id);return v&&v.value;}
    bool set_flag(unsigned id,bool value) {
        saved={};
        if(s.save.set_player_flag(id,value)){s.fail("Oak player flag mutation rejected");return false;}
        return refresh();
    }
    unsigned known_course() const {return saved.selected_course().value;}
    unsigned unlocked() const {return static_cast<unsigned>(std::clamp(saved.unlocked_course().value,0,6));}
    int album_count() const {
        int count=0;for(unsigned i=0;i<60;++i){const auto v=saved.photo_present(save::PhotoSet::album,i);if(v&&v.value)++count;}return count;
    }
    int report_count() const {
        int count=0;for(unsigned i=0;i<63;++i){const auto v=saved.photo_present(save::PhotoSet::report,i);if(v&&v.value)++count;}return count;
    }
    bool wait(unsigned count=1) {return s.wait(count);}
    static void invalidate_input(IsleGameObject*,void* ptr) noexcept {
        auto& self=*static_cast<Impl*>(ptr);
        do {
            if (self.s.input.begin_update()!=game_input::Error::none) {
                self.s.fail("Oak common input invalidation");
                return;
            }
        } while (self.wait());
    }
    std::uint32_t pressed() {
        const auto input=s.input.menu(0);
        if(input.error!=game_input::Error::none||!input.input){s.fail("Oak physical-zero menu input rejected");return 0;}
        return input.input->pressed;
    }
    static bool text_wait(void* ptr,std::uint32_t count) {return static_cast<Impl*>(ptr)->wait(count);}
    static void text_character(void* ptr,std::uint16_t ch) {static_cast<Impl*>(ptr)->character(ch);}
    window::TextHooks hooks() {return {this,text_wait,text_character,nullptr,nullptr};}
    bool draw_text(std::uint32_t address,bool clear=true) {
        observed.page_address=address;
        if(clear)dialog->clear();
        return dialog->print_format(text,hooks(),r->text(address).data(),player_name.data());
    }
    bool page(std::uint32_t address,int mode=0,bool sound=true) {
        observed.phase=Phase::dialogue;
        if(!draw_text(address))return false;
        if(mode==2)return true;
        return advance(mode==1,sound)!=0;
    }
    unsigned advance(bool repeat,bool sound) {
        unsigned result=0;
        text.set_skip_enabled(false);
        if (!repeat) {
            dialog->set_text_pos(180,32);
            if (!dialog->print(text,"\\l",hooks())) return 0;
        }
        observed.phase=repeat?Phase::repeat_question:Phase::page_wait;
        if (!wait()) return 0;
        while (!s.failed()) {
            if (pressed()&(a_button|b_button)) {
                const auto input=s.input.menu(0);
                const auto current=input.input->current;
                if (current&a_button) result=a_button;
                if (current&b_button) result=b_button;
                if (current&start_button) result=start_button;
                break;
            }
            if (!wait()) return 0;
        }
        if (sound) s.play_sound(0x4d);
        // Both ordinary pages and repeat questions clear only after the
        // source's actual one-update acknowledgement boundary.
        if (!wait()) return 0;
        dialog->clear();
        text.set_skip_enabled(true);
        return result;
    }
    bool pages(std::span<const std::uint32_t> list) {
        for (std::size_t i=0;i<list.size();++i) {
            if (!page(list[i])) return false;
        }
        return true;
    }
    bool repeat_pages(std::span<const std::uint32_t> information,std::uint32_t question) {
        while(!s.failed()) {
            if(!pages(information)||!draw_text(question))return false;
            if(advance(true,false)!=a_button){s.play_sound(0x4d);return true;}
            s.play_sound(0x53);
        }
        return false;
    }
    void show(unsigned index,bool visible) {
        if(!art[index].sprite)return;
        if(auto* v=s.view(art[index].sprite)){if(visible)v->attr&=~ISLE_SP_HIDDEN;else v->attr|=ISLE_SP_HIDDEN;}
    }
    void place(unsigned index,float x,float y) {if(!art[index].sprite)return;if(auto* v=s.view(art[index].sprite)){v->x=static_cast<std::int16_t>(x);v->y=static_cast<std::int16_t>(y);}}
    bool bitmap(unsigned index,std::uint32_t identity) {
        if(bound_identity[index]==identity)return true;
        const auto resource=r->sprite_index(identity);
        const auto* current=s.view(art[index].sprite);
        if(!current)return false;
        auto descriptor=*current;
        descriptor.bitmap=r->view().sprites[resource].sprite.view()->bitmap;
        if(!s.replace_sprite(art[index].sprite,resource,descriptor))return false;
        bound_identity[index]=identity;bound_table[index]=0;return true;
    }
    bool variant(unsigned index,std::uint32_t table,unsigned phase) {
        if(bound_table[index]==table&&bound_phase[index]==phase)return true;
        if(!bitmap(index,r->window()->table_variant(table,phase).identity))return false;
        bound_table[index]=table;bound_phase[index]=phase;return true;
    }
    void tint(int value) {
        brightness=std::clamp(value,0,255);
        for(unsigned i:{0u,1u,2u,5u})if(auto* v=s.view(art[i].sprite))v->red=v->green=v->blue=static_cast<std::uint8_t>(brightness);
        // Flatten only the own-GAME caller's known two sprite child chains.
        // Preserve its explicit 0x4000 color-inheritance exclusion and original
        // child channels; immutable templates are never modified.
        for(unsigned i:{3u,4u,6u,7u})if(auto* v=s.view(art[i].sprite)) {
            if(v->attr&0x4000)continue;
            const auto c=source_colors[i];const unsigned parent=i<5?2:5;
            v->red=static_cast<std::uint8_t>(c.r*brightness/255);
            v->green=static_cast<std::uint8_t>(c.g*brightness/255);
            v->blue=static_cast<std::uint8_t>(c.b*brightness/255);
            v->alpha=static_cast<std::uint8_t>(c.a*source_colors[parent].a/255);
        }
        if(art[17].sprite)if(auto* v=s.view(art[17].sprite))v->alpha=static_cast<std::uint8_t>(brightness);
        if(known_course()==6&&art[15].sprite)if(auto* v=s.view(art[15].sprite))v->red=v->green=v->blue=static_cast<std::uint8_t>(brightness);
    }
    void course_mode(bool enabled) {show(0,!enabled);show(2,!enabled);show(3,!enabled);show(4,!enabled);show(1,enabled);}
    void stage_icons(bool enabled) {icon_mode=enabled?1:0;show(6,enabled);}
    void icon(unsigned mode) {
        if(mode==0){icon_mode=0;show(6,false);show(7,false);return;}
        if(mode==1){icon_mode=1;return;}
        if(mode==2||mode>=7) {
            constexpr std::array<std::uint32_t,5> item{0x80141f38,0x80142e08,0x80143cd8,0x80141068,0x801396e8};
            const auto identity=mode==2?item[4]:item[mode-7];
            bitmap(7,identity);show(7,true);show(6,false);
        } else {
            constexpr std::array<std::uint32_t,4> stages{0x8013ba58,0x8013dde8,0x80136468,0x80140178};
            bitmap(6,stages[mode-3]);show(6,true);show(7,false);
        }
    }
    void character(std::uint16_t ch) {
        ++observed.printed_characters;mouth_timer=12;
        switch(ch) {
        case 0x5c4e:expression=0;item_song=false;break;
        case 0x5c4f:expression=1;break;
        case 0x5c4d:eye_timer=-36;break;
        default:
            if(ch>=0x5c30&&ch<=0x5c39){icon(ch-0x5c30);if(ch>=0x5c37&&!item_song){s.check(s.audio.play_song(1,0x20),"Oak item song");item_song=true;}}
            else if(ch==0x5c41){icon(10);if(!item_song)s.check(s.audio.play_song(1,0x20),"Oak dash song");item_song=true;}
            else {
                constexpr std::string_view voices="BCDEFGHIJKLPQSTUVWX";
                if((ch>>8)==0x5c) {
                    const auto at=voices.find(static_cast<char>(ch));
                    if(at!=std::string_view::npos){game_audio::Parameters p;p.pitch=.75f;s.play_sound(static_cast<std::uint32_t>(at<17?0xa2+at:at==17?0xa0:0xa1),p);}
                }
            }
            break;
        }
    }
    static void portrait(IsleGameObject*,void* ptr) noexcept {
        auto& p=*static_cast<Impl*>(ptr);
        try {while(!p.s.failed()) {
            if(p.mouth_timer>0) {
                if(--p.mouth_timer==0)p.mouth_frame=0;
                else if(p.mouth_clock>0)--p.mouth_clock;
                else {p.mouth_index=(p.mouth_index+1)%4;p.mouth_clock=7;p.mouth_frame=static_cast<int>(p.r->word(0x80195860+p.mouth_index*4));}
                p.variant(4,0x80195810,static_cast<unsigned>(p.mouth_frame*2+p.expression));
            }
            unsigned eye=0;
            if(p.eye_timer< -30){++p.eye_timer;eye=3;}
            else if(p.eye_timer< -6){++p.eye_timer;eye=4;}
            else if(p.eye_timer<0){++p.eye_timer;eye=3;}
            else if(p.eye_timer==0)p.eye_timer=180;
            else {if(p.eye_timer<=6)eye=1;else if(p.eye_timer<=12)eye=2;else if(p.eye_timer<=18)eye=1;--p.eye_timer;}
            p.variant(3,0x80195828,eye*2+static_cast<unsigned>(p.expression));
            if(!p.wait())return;
        }}catch(const std::exception& e){p.s.fail(e.what());}
    }
    static void calendars(IsleGameObject*,void* ptr) noexcept {
        auto& p=*static_cast<Impl*>(ptr);
        try {
            p.icon_y+=(float(p.icon_mode?13:-72)-p.icon_y)*.2f;
            p.place(5,208,p.icon_y);p.place(6,214,p.icon_y+6);p.place(7,233,p.icon_y+13);
            if(p.art[16].sprite) {
                if(!--p.gate_clock){p.gate_clock=12;++p.gate_frame;}
                p.show(16,p.gate_mode==1);p.show(17,p.gate_mode==2);
                if(p.gate_mode==1)p.variant(16,0x801c5484,p.gate_frame&3);
                else if(p.gate_mode==2)p.variant(17,0x801c5474,p.gate_frame&3);
            }
            if(p.plaque_animating) {
                p.show(15,true);
                if(auto* v=p.s.view(p.art[15].sprite))v->alpha=static_cast<std::uint8_t>((std::sin(p.plaque_angle)+.001f)*254);
                p.plaque_angle+=3.14159265358979323846f/96;
                if(p.plaque_angle>1.57079632679489661923f)p.plaque_animating=false;
            }
            if(p.music_delay&&!--p.music_delay&&p.music_song>=0){p.s.check(p.s.audio.play_song(0,static_cast<unsigned>(p.music_song)),"Oak scheduled song");p.s.check(p.s.audio.bgm_volume(0,0x7f00),"Oak scheduled volume");}
        }catch(const std::exception& e){p.s.fail(e.what());}
    }
    void music(int mode) {
        if(music_mode==mode)return;
        music_delay=music_mode==3?1:30;
        if(music_mode!=3)s.check(s.audio.bgm_volume(0,0,30),"Oak song fade");
        music_mode=mode;music_song=mode==0?8:mode==1?26:mode==2?9:-1;
    }
    bool make_art(unsigned slot,std::uint32_t identity,int x,int y,bool visible,bool window_wrapper=true) {
        auto* object=s.object(1u<<6,6,6);if(!object)return false;
        art[slot]={object,s.append_sprite(object,identity)};
        if(!art[slot].sprite||!s.set_render(object,{static_cast<std::uint8_t>(window_wrapper?30:2),6,window_wrapper?0x40000000u:4u}))return false;
        bound_identity[slot]=identity;
        if(slot<source_colors.size())if(const auto* v=s.view(art[slot].sprite))source_colors[slot]={v->red,v->green,v->blue,v->alpha};
        if (slot>1) place(slot,float(x),float(y));
        show(slot,visible);
        return true;
    }
    bool initialize() {
        if(!refresh())return false;
        const auto name=saved.player_name();
        if(name.size()!=16){s.fail("Oak source name record size");return false;}
        std::memcpy(player_name.data(),name.data(),name.size());
        if(s.input.reset_scene_menu()!=game_input::Error::none){s.fail("Oak scene-local input reset rejected");return false;}
        // Source common initialization: LINK_PLAYER9, object priority9,
        // kind0 stackful process priority0, once per actual OM update.
        input_object=s.object(1u<<9,9,9);
        if (!input_object || !s.process(input_object,invalidate_input,this,ISLE_GAME_STACKFUL,0)) return false;
        const auto sprite_assets=producer.assets();
        const auto common_assets=r->window()->common_render_assets();
        immutable.reserve(sprite_assets.size()+common_assets.size());
        immutable.insert(immutable.end(),sprite_assets.begin(),sprite_assets.end());
        immutable.insert(immutable.end(),common_assets.begin(),common_assets.end());
        owner=s.object(1u<<6,6,6);if(!owner)return false;
        constexpr std::array<std::uint8_t,4> camera_links{5,10,3,0};
        constexpr std::array<std::uint32_t,4> camera_priorities{5,10,3,0x80000000u};
        for(unsigned i=0;i<cameras.size();++i) {
            cameras[i]=s.object(i<3?0x200u:5u,camera_links[i],camera_priorities[i]);
            if(!cameras[i]||!s.check(isle_game_attach_camera(s.game,cameras[i],&camera_handles[i]),"Oak actual camera"))return false;
        }
        const IsleGameViewport viewport{{640,480,511,0},{640,480,511,0}};
        for(unsigned i=1;i<camera_handles.size();++i)
            if(!s.check(isle_game_camera_set_viewport(s.game,camera_handles[i],&viewport),"Oak source UI viewport"))return false;
        if(!make_art(0,0x8010c590,0,0,true)||!make_art(1,0x80190438,0,0,false)||
           !make_art(2,0x8011eea0,100,34,true)||!make_art(3,0x80149a38,143,66,true)||
           !make_art(4,0x80146918,140,84,true)||!make_art(5,0x80122548,208,13,true)||
           !make_art(6,0x80136498,214,19,false)||!make_art(7,0x80141f48,233,26,false))return false;
        text.set_font(12);text.set_shadow_offset(1);text.set_spacing(-1,3);
        if(!text.initialize(s)||!layout.initialize()||!focus.create()||!frame.create())return false;
        focus.show(false);focus.set_target_size(120,26);focus.set_target_pos(50,44);
        frame.set_style(1);frame.set_pos(184,320);frame.show(false);
        window::ElementDefaults defaults;defaults.background=frame.background_color();defaults.font=12;
        dialog=window::Element::create(heap,elements,104,173,192,47,0,config.raster_addresses,defaults);
        if(!dialog){s.fail("Oak native dialogue heap allocation");return false;}
        dialog_binding=producer.register_raster(dialog->raster());
        if(!dialog->attach(s,dialog_binding))return false;
        dialog->set_buttons(&button_images);dialog->draw_background();dialog->set_visible(false);
        fade=window::Element::create(heap,elements,0,0,8,8,window::scalable,
                                     {config.raster_addresses[0]+0x20000,config.raster_addresses[1]+0x20000});
        if(!fade){s.fail("Oak native fade raster allocation");return false;}
        fade->fill(0,0,7,7,{0,0,0,255});fade->set_scale(40,30);
        fade_binding=producer.register_raster(fade->raster());
        fade_sprite.object=s.object(5,0,0x80000000u);
        if(!fade_sprite.object)return false;
        fade_sprite.sprite=s.append_sprite(fade_sprite.object,fade_binding.resources[0],fade->descriptor(),fade_binding.generation);
        if(!fade_sprite.sprite||!s.set_render(fade_sprite.object,{1,0,2}))return false;
        if(auto* v=s.view(fade_sprite.sprite)){v->attr|=ISLE_SP_HIDDEN|ISLE_SP_TRANSPARENT;v->alpha=0;}
        tint(0);music(0);s.check(s.audio.bgm_volume(0,0x7f00),"Oak initial BGM volume");
        if(!s.process(art[2].object,portrait,this,ISLE_GAME_STACKFUL,0)||
           !s.process(art[5].object,calendars,this,ISLE_GAME_SYNCHRONOUS,0)||
           !s.process(owner,main,this,ISLE_GAME_STACKFUL,0))return false;
        return !s.failed();
    }
    bool fade_screen(bool reveal,float seconds) {
        const int ticks=std::max(1,static_cast<int>(seconds*60));
        auto* v=s.view(fade_sprite.sprite);if(!v)return false;v->attr&=~ISLE_SP_HIDDEN;
        for(int i=0;i<=ticks;++i){v->alpha=static_cast<std::uint8_t>((reveal?ticks-i:i)*255/ticks);if(!wait())return false;}
        if (reveal) v->attr|=ISLE_SP_HIDDEN;
        return true;
    }
    bool grow_button(int id) {
        s.play_sound(0x52);layout.set_instant_transition(true);
        if(!layout.create_buttons(r->main_buttons(flag(7))))return false;
        layout.show_button(true,id);layout.set_button_scale(.1f,id);
        for(int i=1;i<30;++i){layout.set_button_scale(float(i)/30,id);if(!wait())return false;}
        layout.set_button_scale(1,id);return true;
    }
    bool welcome_back() {if(!draw_text(0x801c5ec8)||!wait(54))return false;return advance(false,false)!=0;}
    bool gift(unsigned kind) {
        if(!page(0x801c6398)||!page(0x801c63f4))return false;
        if(!repeat_pages(kind==0?std::span<const std::uint32_t>(apple):kind==1?std::span<const std::uint32_t>(pester):std::span<const std::uint32_t>(flute),0x801c67b8))return false;
        icon(0);return true;
    }
    bool forced_save() {
        text.set_skip_enabled(false);text.set_print_delay(2);
        if(!draw_text(0x801c6a7c))return false;
        text.set_print_delay(0);dialog->set_text_pos(0,32);if(!draw_text(0x801c6a94,false)||!wait())return false;
        while(!(pressed()&(a_button|b_button)))if(!wait())return false;
        s.play_sound(0x53);if(!draw_text(0x801c6aac))return false;
        dialog->set_text_pos(0,32);if(!draw_text(0x801c6ac8,false))return false;
        s.check(s.audio.bgm_volume(0,0,30),"Oak final save music fade");if(!wait(33)||!commit())return false;
        if (!wait(6)) return false;
        dialog->clear();
        return fade_screen(false,1);
    }
    bool notices() {
        text.set_print_delay(2);initial_slot=0;
        if(s.state.consume_scene_notices(0x100000)){initial_slot=6;return true;}
        if(s.state.consume_scene_notices(0x200000)){initial_slot=1;return true;}
        if(s.state.consume_scene_notices(1)) {
            observed.first_meet_consumed=true;
            if(!page(0x801c5950)||!repeat_pages(first_information,0x801c5ae0)||!page(0x801c5b24))return false;
            icon(0);stage_icons(false);
        }
        if(s.state.consume_scene_notices(2)) {
            if(flag(14)){if(!page(0x801c5d10))return false;}
            else if(!flag(17)&&flag(11)){if(!pages(challenge)||!set_flag(17,true))return false;if(flag(20)&&(!set_flag(20,false)||!set_flag(14,true)))return false;}
            else if(!page(0x801c5cc8))return false;
        }
        if(s.state.consume_scene_notices(8)) {
            if(!set_flag(6,true)||!page(0x801c5d48)||!page(0x801c5d88,2)||!grow_button(3)||!advance(false,true)||!page(0x801c5da8))return false;
            initial_slot=6;
        }
        if(s.state.consume_scene_notices(0x10)&&!welcome_back())return false;
        if(s.state.consume_scene_notices(0x20)) {
            if(!flag(7)&&report_count()!=0){if(!set_flag(7,true)||!pages(report_gift)||!grow_button(2))return false;initial_slot=1;}
            else if(!welcome_back())return false;
        }
        if(s.state.consume_scene_notices(0x800)) {
            if(!set_flag(5,true)||!page(0x801c5edc)||!repeat_pages(dash_information,0x801c5fe4)||!page(0x801c6028))return false;
            icon(0);initial_slot=1;
        }
        bool all_signs=false;
        if(initial_slot!=1&&s.state.consume_scene_notices(0x400)) {
            if(unlocked()==6){if(!pages(rainbow))return false;all_signs=true;}
            else if(unlocked()==2||unlocked()==4){if(!page(0x801c6090)||!page(0x801c60dc))return false;}
            else if(!page(0x801c6054))return false;
        }
        if(!all_signs)for(std::uint32_t mask=0x1000;mask<=0x20000;mask<<=1)
            if(s.state.consume_scene_notices(mask)&&(!page(0x801c6334)||!page(0x801c6360)))return false;
        for(unsigned i=0;i<3;++i)if(s.state.consume_scene_notices(0x80u<<i)&&(!set_flag(i,true)||!gift(i)))return false;
        const bool report=s.state.consume_scene_notices(0x40),mew_notice=s.state.consume_scene_notices(0x40000),mew_again=s.state.consume_scene_notices(0x80000),finished=flag(14);
        int result=-1;
        if(mew_notice&&report){if(!set_flag(11,true)||!set_flag(20,true)||!pages(completed_report))return false;result=19;}
        else if(mew_notice){if(!set_flag(11,true)||!pages(mew))return false;result=18;}
        else if(report){if(!set_flag(14,true)||!pages(completed_report))return false;result=19;}
        else if(mew_again&&finished){if(!pages(mew))return false;result=17;}
        if(result>=0){if(!forced_save())return false;s.state.clear_scene_notices();publish_exit(result);return false;}
        text.set_print_delay(0);return true;
    }
    bool printer_save_notification(std::uint8_t command) {
        auto* station=s.input.printer();
        if(!station)return true;
        if(station->error()!=snap_station::Error::none) {
            s.fail(station->message());return false;
        }
        if(!station->available())return true;
        auto idle=[&]() {
            for(;;) {
                std::uint8_t status=0;
                if(!station->status(status)) {s.fail(station->message());return false;}
                if(status!=8)return true;
                if(!wait())return false;
            }
        };
        if(!idle())return false;
        if(!station->command(command)) {s.fail(station->message());return false;}
        return idle();
    }
    bool commit() {
        observed.phase=Phase::saving;++observed.commit_attempts;
        if(!printer_save_notification(0xcc))return false;
        observed.commit=s.save.commit(s.raw_game_time);
        if(!commit_acknowledged(observed.commit)) {
            save_failure();return false;
        }
        if(!printer_save_notification(0x33))return false;
        return refresh();
    }
    void save_failure() {
        observed.phase=Phase::save_failed;
        if(!art[8].object)make_art(8,0x80128f38,40,92,true);
        focus.set_target_pos(40,92);focus.set_target_size(240,56);text.set_print_delay(1);text.set_skip_enabled(false);
        s.check(s.audio.bgm_volume(0,0x3f80,60),"Oak save failure music");
        draw_text(0x801c5550);
        observed.phase=Phase::save_failed;
        while(!s.failed())if(!wait())return;
    }
    bool navigate() {
        const auto input=s.input.menu(0);if(input.error!=game_input::Error::none||!input.input){s.fail("Oak navigation input");return false;}
        focus.set_target_pos(22,observed.selected_slot*24+29);
        const auto slots=layout.buttons();
        if(slots.size()<8){s.fail("Oak source layout lost its eight selection slots");return false;}
        const auto step=source_navigation(std::span<const window::Button,8>(slots.data(),8),observed.selected_slot,*input.input);
        observed.selected_slot=step.slot;
        for(unsigned i=0;i<step.move_sounds;++i)s.play_sound(65);
        return !step.back;
    }
    void reset_focus() {focus.set_pos(22,observed.selected_slot*24+29);focus.set_size(62,13);focus.show(true);}
    bool description(std::string_view value) {dialog->clear();text.set_shadow_offset(1);text.set_spacing(-1,3);return dialog->print(text,value,hooks());}
    bool lab_description(int id) {
        if(id==3) {
            const int count=album_count();
            if(!count)return draw_text(0x801c5770);
            if(!draw_text(count==1?0x801c56f8:0x801c5708))return false;
            dialog->set_text_pos(124,0);if(!dialog->print_format(text,hooks(),r->text(count==1?0x801c5718:0x801c5728).data(),count))return false;
            dialog->set_text_pos(0,16);if(!draw_text(0x801c573c,false))return false;
            dialog->set_text_pos(124,16);return dialog->print_format(text,hooks(),r->text(60-count==1?0x801c574c:0x801c575c).data(),60-count);
        }
        if(id==2) {
            int count=0,points=0;for(unsigned i=0;i<63;++i){const auto p=saved.photo_present(save::PhotoSet::report,i);if(p&&p.value){++count;points+=saved.report_score(i).value;}}
            if (!draw_text(0x801c57c8)) return false;
            dialog->set_text_pos(0,16);
            if (!draw_text(0x801c57dc,false)) return false;
            dialog->set_text_pos(86,0);if(!dialog->print_format(text,hooks(),r->text(0x801c57ec).data(),count,count==1?"":r->text(0x801c57fc).data()))return false;
            dialog->set_text_pos(86,16);return dialog->print_format(text,hooks(),r->text(0x801c5804).data(),points);
        }
        return description(window::Layout::button_text(r->main_buttons(flag(7)),id));
    }
    int lab() {
        if(!layout.create_buttons(r->main_buttons(flag(7))))return -1;
        layout.disable_button(album_count()==0,3);focus.set_target_size(62,13);
        observed.selected_slot=initial_slot;initial_slot=0;
        if (!wait()) return -1;
        reset_focus();
        int previous=-1;
        while(!s.failed()) {
            observed.phase=Phase::lab;navigate();const auto buttons=pressed();
            const int id=layout.buttons()[observed.selected_slot].id;
            if(buttons&a_button) {
                s.play_sound(66);
                if (id==1) return 1;
                if (id==2) return 14;
                if (id==3&&album_count()) return 12;
                if (id==4) return 3;
            }
            if(previous!=observed.selected_slot){previous=observed.selected_slot;if(!lab_description(id))return -1;}
            if(!wait())return -1;
        }
        return -1;
    }
    bool new_course() {
        const auto course=unlocked();
        if(known_course()>=course)return true;
        saved={};
        if(s.save.set_selected_course(course)){s.fail("Oak acknowledged course mutation rejected");return false;}
        if(!refresh()||!wait())return false;
        if(course!=2&&course!=4&&course!=6){text.set_skip_enabled(false);text.set_print_delay(2);if(!page(r->word(0x80195888)))return false;text.set_print_delay(0);}
        else if(!wait(42))return false;
        s.play_sound(0x52);layout.show_button(true,static_cast<int>(course+6));layout.set_button_scale(.1f,static_cast<int>(course+6));
        if(course==6){gate_mode=1;plaque_angle=0;plaque_animating=true;}
        for(int i=1;i<30;++i){layout.set_button_scale(float(i)/30,static_cast<int>(course+6));if(!wait())return false;}
        layout.set_button_scale(1,static_cast<int>(course+6));route_mode=4;route_busy=true;dialog->clear();return wait(15);
    }
    bool confirmation() {
        dialog->clear();text.set_shadow_offset(1);text.set_spacing(-1,3);
        if(!flag(11)){if(!draw_text(0x801c58b0))return false;}
        else {
            const auto score=saved.course_score(static_cast<unsigned>(observed.selected_course)).value;
            const auto& common=r->window()->banks()[0];
            const auto at=0x803a66a0-common.guest+static_cast<unsigned>(observed.selected_course)*4;
            const auto* b=common.owner->data()+at;
            const auto target=static_cast<std::int32_t>((std::uint32_t(b[0])<<24)|(std::uint32_t(b[1])<<16)|(std::uint32_t(b[2])<<8)|b[3]);
            dialog->set_color(true,target<score?window::Color{255,255,0,255}:window::Color{255,255,255,255});
            if (!draw_text(0x801c58ec)) return false;
            dialog->set_text_pos(96,0);
            if (!dialog->print_format(text,hooks(),r->text(0x801c58f8).data(),score)) return false;
            dialog->set_color(true,target>=score?window::Color{255,255,0,255}:window::Color{255,255,255,255});
            dialog->set_text_pos(0,16);if(!draw_text(0x801c5904,false))return false;
            dialog->set_text_pos(96,32);if(!dialog->print_format(text,hooks(),r->text(0x801c5914).data(),target))return false;
            dialog->set_color(true,{255,255,255,255});
        }
        stage_icons(false);if(auto* v=s.view(art[1].sprite))v->red=v->green=v->blue=255;
        if(!wait())return false;
        while(!s.failed()) {
            observed.phase=Phase::confirmation;
            const auto buttons=pressed();
            if(buttons&a_button){game_audio::Parameters p;p.pitch=.75f;s.play_sound(0xa0,p);route_course=observed.selected_course;route_mode=1;route_busy=true;route_complete=false;while(!route_complete&&!s.failed())if(!wait())return false;return !s.failed();}
            if(buttons&b_button){s.play_sound(67);layout.disable_all_except(false,0);if(!wait())return false;stage_icons(true);return false;}
            if(!wait())return false;
        }
        return false;
    }
    int courses() {
        music(1);course_mode(true);if(auto* v=s.view(art[1].sprite))v->red=v->green=v->blue=0xb2;
        stage_icons(true);map_mode(true);
        if(unlocked()>known_course()) {
            focus.show(false);layout.show_button(false,static_cast<int>(unlocked()+6));
            if(!layout.create_buttons(r->course_buttons(unlocked()))||!layout.wait_transition()||!new_course())return -1;
            observed.selected_slot=static_cast<int>(known_course());
        } else {observed.selected_slot=observed.selected_course;if(!layout.create_buttons(r->course_buttons(known_course())))return -1;}
        focus.set_target_size(62,13);reset_focus();int previous=-1;
        while(!s.failed()) {
            observed.phase=Phase::courses;
            // Source AA740 suppresses fresh navigation during its vehicle banner.
            const bool back=route_busy?false:!navigate();const auto buttons=route_busy?0u:pressed();
            if(back){s.play_sound(67);stage_icons(false);return 0;}
            const auto id=layout.buttons()[observed.selected_slot].id;
            if(observed.selected_slot<7){variant(6,0x801957f4,static_cast<unsigned>(observed.selected_slot));stage_icons(true);}else stage_icons(false);
            if(buttons&a_button) {
                s.play_sound(66);layout.disable_all_except(true,id);
                if(id>=6&&id<=12){observed.selected_course=id-6;if(confirmation())return observed.selected_course;if(auto* v=s.view(art[1].sprite))v->red=v->green=v->blue=0xb2;previous=-1;}
                else if(id==5){illegal_back=true;stage_icons(false);layout.disable_all_except(false,id);return 0;}
            }
            if(previous!=observed.selected_slot){previous=observed.selected_slot;if(!description(window::Layout::button_text(r->course_buttons(known_course()),id)))return -1;}
            if(!wait())return -1;
        }
        return -1;
    }
    int save_prompt() {
        dialog->clear();if(!wait()||!draw_text(0x801c5810))return -1;focus.show(false);
        while(!s.failed()) {
            observed.phase=Phase::save_prompt;const auto buttons=pressed();
            if(buttons&a_button) {
                game_audio::Parameters p;p.pitch=.75f;s.play_sound(0xa0,p);s.play_sound(0x53);
                dialog->clear();dialog->set_text_pos(0,32);if(!draw_text(0x801c5840,false)||!wait(48))return -1;
                s.check(s.audio.bgm_volume(0,0,30),"Oak explicit save fade");if(!wait(33))return -1;
                if(flag(21))illegal_copy();
                if(!commit())return -1;
                s.check(s.audio.bgm_volume(0,0x7f00,18),"Oak acknowledged save volume");if(!wait(21)||!draw_text(0x801c5850))return -1;
                while(!s.failed()) {
                    observed.phase=Phase::keep_going;const auto next=pressed();
                    if(next&b_button){s.play_sound(0x53);if(!draw_text(0x801c5880)||!wait(120))return -1;return 8;}
                    if(next&a_button){s.play_sound(0x4d);return 0;}
                    if(!wait())return -1;
                }
            }
            if(buttons&b_button){s.play_sound(0x4d);return 0;}
            if(!wait())return -1;
        }
        return -1;
    }
    void illegal_copy() {
        // GAME's anti-copy branch uses its existing random/time source and only
        // removes a report entry when the source predicate selects an admitted
        // species. Never turn this required callback into an empty sink.
        const bool act=illegal_back?(s.raw_game_time&0x28)==0:(isle_rng_next16(&s.rng)&0x12)==0;
        const int index=r->species_index(isle_rng_time8(s.raw_game_time));
        if(act&&index>=0){if(s.save.remove_photo(save::PhotoSet::report,static_cast<unsigned>(index)))s.fail("Oak anti-copy report removal rejected");else refresh();}
    }
    void map_mode(bool visible) {
        route_mode=0;route_busy=false;gate_mode=visible?gate_mode:0;
        for(unsigned i=9;i<=15;++i)show(i,false);
        show(9,visible);show(10,visible);if(known_course()==6)show(15,visible);
        place(9,105,117);place(10,170,121);place(11,137,121);place(12,105,117);place(13,170,121);place(14,170,121);
        gate_mode=visible?1:0;
    }
    bool create_route() {
        constexpr std::array<std::uint32_t,9> ids{0x80197eb0,0x80198fb8,0x8019c2d0,0x801a2900,0x801ad198,0x801b93a8,0x801bed90,0x801c31b0,0x801bfe98};
        constexpr std::array<int,9> xs{105,170,137,170,105,170,109,263,263};
        constexpr std::array<int,9> ys{117,121,121,121,117,121,24,121,121};
        for(unsigned i=0;i<ids.size();++i)if(!make_art(9+i,ids[i],xs[i],ys[i],false,false))return false;
        if(auto* v=s.view(art[14].sprite))v->attr|=ISLE_SP_SCALE;
        for(unsigned i:{15u,17u})if(auto* v=s.view(art[i].sprite))v->attr|=ISLE_SP_TRANSPARENT;
        return s.process(art[9].object,route,this,ISLE_GAME_STACKFUL,0)!=nullptr;
    }
    static void route(IsleGameObject*,void* ptr) noexcept {
        auto& p=*static_cast<Impl*>(ptr);
        try {unsigned idle=0;int count=12;
        while(!p.s.failed()) {
            if(p.route_mode==0){if(!--count){count=12;idle=(idle+1)%4;p.variant(10,0x801c5404,idle);}}
            else if(p.route_mode==1) {
                p.show(10,false);p.show(11,true);p.route_mode=2;
                for(unsigned i=0;i<15;++i){if(i%4==3)p.variant(11,0x801c5414,(i/4+1)%6);p.interpolate(11,0x801c54e8,0x801c54f4,float(i)/15);if(!p.wait())return;}
                continue;
            } else if(p.route_mode==2) {
                p.place(13,105,117);p.show(9,false);p.show(11,false);p.show(13,true);p.route_mode=3;p.gate_mode=2;p.s.play_sound(0x16);
                for(unsigned i=0;i<21;++i){if(i%4==3)p.variant(13,0x801c5450,i/4+1);if(!p.wait())return;}
                continue;
            } else if(p.route_mode==3) {
                p.show(13,false);p.show(14,true);
                for(unsigned i=0;i<60;++i){if(i%4==3)p.variant(14,0x801c5468,(i/4+1)%3);p.interpolate(14,0x801c5500,0x801c5494+static_cast<unsigned>(p.route_course)*12,float(i)/60);if(auto* v=p.s.view(p.art[14].sprite)){v->scalex=v->scaley=1-float(i)/60;}if(!p.wait())return;}
                p.route_busy=false;p.route_complete=true;p.route_mode=5;
            } else if(p.route_mode==4) {
                p.route_mode=0;p.show(10,false);p.show(12,true);p.variant(12,0x801c542c,0);game_audio::Parameters params;params.pitch=.75f;p.s.play_sound(0xa1,params);
                for(unsigned i=0;i<63;++i){if(i%7==6){if(i/7+1>=9)break;p.variant(12,0x801c542c,i/7+1);}if(!p.wait())return;}
                if (!p.wait(30)) return;
                p.show(10,true);
                p.show(12,false);
                p.route_busy=false;
            }
            if(!p.wait())return;
        }}catch(const std::exception& e){p.s.fail(e.what());}
    }
    void interpolate(unsigned art_index,std::uint32_t from,std::uint32_t to,float t) {
        place(art_index,r->real(from)+(r->real(to)-r->real(from))*t,r->real(from+4)+(r->real(to+4)-r->real(from+4))*t);
    }
    void publish_exit(int result) {
        observed.source_result=result;observed.exit_requested=true;observed.phase=Phase::complete;s.request_exit(result);
    }
    bool close_scene(int result) {
        observed.phase=Phase::closing;music(3);focus.show(false);dialog->set_visible(false);frame.show_background(true);
        layout.set_header_flags(0);layout.set_instant_transition(false);
        if (!layout.hide_buttons()||!layout.wait_transition()) return false;
        layout.show_panel(false);
        for(int i=10;i>=0;--i){tint(i*255/10);frame.set_opacity(i*255/10);if(!wait())return false;}
        tint(0);button_images.clear();if(!layout.wait_transition())return false;s.state.clear_scene_notices();if(!wait(6))return false;
        publish_exit(result);return wait();
    }
    void run() {
        observed.phase=Phase::opening;
        if(report_count()>=4&&!set_flag(8,true))return;
        for(int i=0;i<60;++i){tint(i*255/60);if(!wait())return;}
        tint(255);layout.show_panel(true);if(!wait(30))return;
        // Source peeks these notices by consume/re-OR so their later dialogue
        // consumer remains the sole final consumer.
        if(s.state.consume_scene_notices(8)) {
            s.state.add_scene_notices(8);const bool prior=flag(6);if(!prior&&!set_flag(6,true))return;
            if (!layout.create_buttons(r->main_buttons(flag(7)))) return;
            if (!prior) {
                if (!set_flag(6,false)) return;
                layout.show_button(false,3);
            }
        } else if(s.state.consume_scene_notices(0x20)) {
            s.state.add_scene_notices(0x20);const bool prior=flag(7);if(!prior&&!set_flag(7,true))return;
            if (!layout.create_buttons(r->main_buttons(flag(7)))) return;
            if (!prior) {
                if (!set_flag(7,false)) return;
                layout.show_button(false,2);
            }
        } else if(!layout.create_buttons(r->main_buttons(flag(7))))return;
        if(known_course()!=unlocked()){s.state.add_scene_notices(0x400);if(!set_flag(19,false))return;}
        layout.disable_button(album_count()==0,3);
        if(!layout.wait_transition()||!frame.fade_in(1))return;
        dialog->set_visible(true);text.set_print_delay(0);dialog->set_font(12);text.set_shadow_offset(1);text.set_spacing(-1,3);
        if(!notices()||!create_route())return;
        if(s.input.set_menu_repeat(24,12,0xf0000)!=game_input::Error::none){s.fail("Oak repeat configuration");return;}
        focus.move_front();layout.set_instant_transition(true);text.set_print_delay(0);
        while(!s.failed()) {
            const int selection=lab();if(selection<0)return;
            if(selection==14||selection==12){close_scene(selection);return;}
            if(selection==3){const int result=save_prompt();if(result==8){close_scene(8);return;}if(result<0)return;continue;}
            if(selection==1) {
                if (!fade_screen(false,.2f)) return;
                focus.show(false);
                frame.set_style(0);
                dialog->set_color(false,frame.background_color());
                dialog->clear();
                if(!fade_screen(true,.2f))return;
                const int result=courses();if(result<0)return;
                // Source result zero is a confirmed Beach choice, while selector
                // cancellation is a distinct local result, never a scene return.
                if(route_complete){close_scene(result);return;}
                if (!fade_screen(false,.2f)) return;
                focus.show(false);
                frame.set_style(1);
                dialog->set_color(false,frame.background_color());
                dialog->clear();
                course_mode(false);
                map_mode(false);
                stage_icons(false);
                music(0);
                if (!fade_screen(true,.2f)) return;
            }
        }
    }
    static void main(IsleGameObject*,void* ptr) noexcept {auto& p=*static_cast<Impl*>(ptr);try{p.run();}catch(const std::exception& e){p.s.fail(e.what());}catch(...){p.s.fail("Oak native process exception");}}
    void retire() noexcept {
        if (retired) return;
        retired=true;
        // PrintOperation values live on the suspended Source worker's stack.
        // Release their Element/Text borrows before joining that worker unmaps
        // its stack; detaching afterwards dereferences an abandoned operation.
        if(dialog)dialog->detach();
        text.detach();
        if(owner)s.end_processes(owner);
        if (input_object) s.end_processes(input_object);
        for(auto& handle:art)if(handle.object)s.end_processes(handle.object);
        button_images.clear();layout.retire();focus.retire();frame.retire();
        for(auto& handle:art)if(handle.object){s.erase(handle.object);handle={};}
        if(fade_sprite.object){s.erase(fade_sprite.object);fade_sprite={};}
        for(auto*& camera:cameras)if(camera){s.erase(camera);camera=nullptr;}
        if(owner){s.erase(owner);owner=nullptr;}
        if (input_object) {
            s.erase(input_object);
            input_object=nullptr;
        }
        try{if(dialog_binding.generation)producer.unregister_raster(dialog_binding);if(fade_binding.generation)producer.unregister_raster(fade_binding);}catch(const std::exception& e){s.fail(e.what());}
        dialog.reset();fade.reset();observed.phase=Phase::retired;
    }
};
Scene::Scene(scene::Services& s,std::shared_ptr<const Resources> r,sprite::Producer& producer,Config c)
    :impl_(std::make_unique<Impl>(s,std::move(r),producer,c)){}
Scene::~Scene(){retire();}
bool Scene::start() noexcept {
    if (impl_->started||impl_->retired) return false;
    impl_->started=true;
    try{return impl_->initialize();}catch(const std::exception& e){impl_->s.fail(e.what());return false;}catch(...){impl_->s.fail("Oak initialization exception");return false;}
}
View Scene::view() const noexcept{return impl_->observed;}
bool Scene::failed() const noexcept{return impl_->s.failed();}
const char* Scene::error() const noexcept{return impl_->s.error();}
void Scene::retire() noexcept{if(impl_)impl_->retire();}
scene::render::Input Scene::render_input() {
    auto& p=*impl_;if(p.retired)throw std::logic_error("Oak render after retirement");
    save::ScreenOffsets offsets;
    if (!p.s.state.screen_offsets().translation(offsets)) {
        p.s.fail("Oak live VI endpoints cannot be represented by native translation");
        throw std::runtime_error(p.s.error());
    }
    std::size_t count=0;if(!p.s.sprite_draws(p.draws,count))throw std::runtime_error(p.s.error());
    auto& shell=p.passes[0];
    shell={};
    shell.kind=scene::render::Kind::clear;
    shell.camera=p.camera_handles[0];
    shell.priority=20;
    shell.flags=4;
    shell.background_rgba=0x7101;
    constexpr std::array<unsigned,3> links{30,2,1};
    constexpr std::array<unsigned,3> priorities{30,2,1};
    for (unsigned i=0;i<3;++i) {
        auto& pass=p.passes[i+1];
        pass={};
        pass.kind=scene::render::Kind::sprites;
        pass.camera=p.camera_handles[i+1];
        pass.priority=priorities[i];
        pass.flags=i<2?4:8;
        pass.link_mask=std::uint64_t(1)<<links[i];
        pass.camera_tag=1u<<links[i];
        pass.sprites=std::span<const scene::SpriteDraw>(p.draws).first(count);
        pass.sprite_commands=p.reservations[i];
    }
    return {p.s.game,
            {p.config.resource_identity,&p.producer,p.immutable,p.reservations,
             {0x3b5000,0x3da800},p.r->window()->common_pre_render()},
            p.passes,offsets};
}
} // namespace isle::oaks_lab

#include "menu_ui_scene.hpp"

#include <algorithm>
#include <array>
#include <exception>

namespace isle::menu::scene {
unsigned menu_item_count(MenuAvailability a) noexcept { return a.has_save ? (a.gallery ? 4u : 3u) : 2u; }
std::uint8_t menu_item_state(MenuAvailability a, unsigned n) noexcept {
    if (n >= menu_item_count(a)) return 0;
    if (n == 0) return 6;
    if (!a.has_save) return 8;
    if (n == 1) return 7;
    return a.gallery && n == 2 ? 9 : 8;
}
SourceScene menu_exit_scene(std::uint8_t n) noexcept {
    switch (n) {
    case 6: return SourceScene::beach;
    case 7: return SourceScene::oaks_lab;
    case 9: return SourceScene::gallery;
    case 10: return SourceScene::attract;
    default: return SourceScene::invalid;
    }
}
std::uint32_t menu_entry_flags(std::uint8_t n) noexcept { return n == 6 ? 1u : n == 7 ? 2u : 0u; }
MenuTransition menu_transition(MenuAvailability a, std::uint8_t state, unsigned selection,
                               std::uint32_t p, std::uint32_t idle,
                               int controller, unsigned argument) noexcept {
    if(controller!=0) return {state,selection};
    if(p) idle=0;
    if(idle>=1800) return {static_cast<std::uint8_t>(argument==15?11:10),selection};
    if(state==2) return {static_cast<std::uint8_t>((p&0x9000)?3:2),selection};
    if(state!=3 && state!=4) return {state,selection};
    const auto count=menu_item_count(a);
    if(p&game_input::stick_slow_up) return {state,(selection+count-1)%count};
    if(p&game_input::stick_slow_down) return {state,(selection+1)%count};
    if(p&0x4000) return {2,selection};
    if(p&0x9000) return {menu_item_state(a,selection),selection};
    return {state,selection};
}

struct Ui::Impl {
    static constexpr std::uint16_t attr = ISLE_SP_TEXSHUF | ISLE_SP_TRANSPARENT;
    static constexpr std::uint32_t A = 0x8000, B = 0x4000, Start = 0x1000;
    static constexpr std::uint32_t Up = game_input::stick_slow_up, Down = game_input::stick_slow_down,
        Left = game_input::stick_slow_left, Right = game_input::stick_slow_right;
    struct Group {
        IsleGameObject* object = nullptr;
        std::array<IsleGameSprite*, 9> sprites{};
        unsigned count = 0;
    };
    struct Motion {
        Impl* owner = nullptr;
        Group* group = nullptr;
        IsleGameProcess* process = nullptr;
        unsigned kind = 0, index = 0;
    };
    ::isle::scene::Services& s;
    Title& title;
    const Catalog& catalog;
    const MenuInitializedFacts initialized;
    std::array<Group, 28> groups{};
    std::array<Motion, 16> motions{};
    IsleGameObject* control = nullptr;
    IsleGameObject* timer = nullptr;
    std::uint8_t state = 0;
    std::uint32_t idle = 0;
    bool idle_running = false, started = false, retired = false;
    MenuAvailability available{};
    save::ScreenOffsets offsets{};
    bool live_offsets(save::ScreenOffsets& out) noexcept {
        if (s.state.screen_offsets().translation(out)) return true;
        s.fail("menu live VI endpoints cannot be represented by native translation");
        return false;
    }
    std::array<bool, 3> settings{};
    unsigned option = 0, setting_choice = 0;
    // 0 background, 1 stacked logo, 2 logo/copyright, 3/4 scrolling sprites,
    // 5 press start, 6 new, 7 continue, 8 gallery, 9 options, 10 reset prompt,
    // 11 no-controller; options reuse 12..23; calibration uses 24/25.
    explicit Impl(::isle::scene::Services& services, Title& t, const Catalog& resources)
        : s(services), title(t), catalog(resources), initialized(catalog.menu_initialized()) {
        for (auto& m : motions) m.owner = this;
        load_save();
    }
    void load_save() noexcept {
        try {
            const auto got = s.save.snapshot();
            if (!got) { s.fail("menu save snapshot unavailable"); return; }
            const auto screen = got.value.screen_offsets();
            const auto saved = got.value.player_flag(16);
            if (!screen || !saved) { s.fail("menu save fields unavailable"); return; }
            offsets = screen.value;
            available.has_save = saved.value;
            unsigned reports = 0;
            for (unsigned id = 0; id < 151; ++id) {
                const auto index = catalog.report_photo_index(static_cast<std::int32_t>(id));
                if (index < 0 || index >= static_cast<int>(save::game_report_count)) continue;
                const auto present = got.value.photo_present(save::PhotoSet::report, static_cast<std::size_t>(index));
                if (!present) { s.fail("menu report Pokemon lookup failed"); return; }
                reports += present.value;
            }
            available.gallery = reports > 3;
            constexpr unsigned ids[]{9, 12, 13};
            for (unsigned i = 0; i < 3; ++i) {
                const auto f = got.value.player_flag(ids[i]);
                if (!f) { s.fail("menu option flag unavailable"); return; }
                settings[i] = f.value;
            }
        } catch (const std::exception& e) { s.fail(e.what()); }
        catch (...) { s.fail("menu save read exception"); }
    }
    bool save_options() noexcept {
        try {
            constexpr unsigned ids[]{9, 12, 13};
            save::ScreenOffsets live;
            if (!live_offsets(live)) return false;
            if (s.save.set_screen_offsets(live.horizontal, live.vertical)) {
                s.fail("menu screen offset mutation failed"); return false;
            }
            for (unsigned i = 0; i < 3; ++i) if (s.save.set_player_flag(ids[i], settings[i])) {
                s.fail("menu option flag mutation failed"); return false;
            }
            return true;
        } catch (const std::exception& e) { s.fail(e.what()); }
        catch (...) { s.fail("menu options mutation exception"); }
        return false;
    }
    bool reset_save() noexcept {
        try {
            const auto editable_offsets = offsets;
            if (s.save.reset(s.raw_game_time)) { s.fail("menu new-game reset failed"); return false; }
            load_save();
            // Saved RAM defaults and menu editing BSS are not the live VI owner.
            offsets = editable_offsets;
            return !s.failed();
        } catch (const std::exception& e) { s.fail(e.what()); }
        catch (...) { s.fail("menu reset exception"); }
        return false;
    }
    IsleSprite* view(Group& g, unsigned n = 0) noexcept {
        if (n >= g.count || !g.sprites[n]) { s.fail("menu sprite index unavailable"); return nullptr; }
        return s.view(g.sprites[n]);
    }
    void tint(Group& g, unsigned n, unsigned r, unsigned green, unsigned b) noexcept {
        if (auto* v = view(g,n)) {
            v->red = static_cast<std::uint8_t>(r);
            v->green = static_cast<std::uint8_t>(green);
            v->blue = static_cast<std::uint8_t>(b);
        }
    }
    void gray(Group& g, unsigned n, unsigned c) noexcept { tint(g,n,c,c,c); }
    void visible(Group& g, unsigned n, bool show) noexcept {
        if (auto* v = view(g,n)) {
            if (show) v->attr &= ~ISLE_SP_HIDDEN; else v->attr |= ISLE_SP_HIDDEN;
        }
    }
    void show_all(Group& g, bool show) noexcept { for (unsigned n=0;n<g.count;++n) visible(g,n,show); }
    void position(Group& g, unsigned n, int x, int y) noexcept {
        if (auto* v = view(g,n)) {
            v->x = static_cast<std::int16_t>(x);
            v->y = static_cast<std::int16_t>(y);
        }
    }
    void erase(Group& g) noexcept {
        for (auto& m : motions) if (m.group == &g) {
            if (m.process) s.end_process(m.process);
            m.process = nullptr; m.group = nullptr;
        }
        if (g.object) s.erase(g.object);
        g = {};
    }
    void clear(unsigned first, unsigned end) noexcept { for (unsigned i=first;i<end;++i) erase(groups[i]); }
    void add(Group& g, std::uint32_t resource, int x, int y, std::uint16_t flags = attr) noexcept {
        if (s.failed()) return;
        if (g.count == g.sprites.size()) { s.fail("menu sprite chain overflow"); return; }
        IsleGameSprite* sprite;
        if (!g.object) {
            const auto h = s.sprite(resource,1,0x80000000u,14);
            g.object = h.object; sprite = h.sprite;
        } else sprite = s.append_sprite(g.object, resource);
        if (!sprite) return;
        const auto n = g.count++;
        g.sprites[n] = sprite;
        if (auto* v = view(g,n)) {
            v->x = static_cast<std::int16_t>(x);
            v->y = static_cast<std::int16_t>(y);
            v->attr = flags;
        }
    }
    std::uint32_t pressed() noexcept {
        const auto r = s.input.menu(0);
        if (r.error != game_input::Error::none || !r.input) { s.fail("menu input unavailable"); return 0; }
        return r.input->pressed;
    }
    std::uint32_t raw_pressed() const noexcept { return s.input.snapshot().primary().pressed; }
    bool sound(unsigned id) noexcept { return s.play_sound(id) >= 0 && !s.failed(); }
    bool volume(unsigned player, unsigned value, unsigned ticks=0) noexcept {
        return s.check(s.audio.bgm_volume(player,value,ticks),"menu BGM volume");
    }
    bool song(unsigned player, unsigned id) noexcept { return s.check(s.audio.play_song(player,id),"menu song"); }
    bool stop_music() noexcept { return s.check(s.audio.stop_bgm(),"menu stop BGM"); }
    bool fade(bool reveal, std::array<std::uint8_t,3> rgb, float seconds) noexcept {
        if (!s.begin_fade(reveal,seconds,rgb)) return false;
        for (unsigned i=0;i<static_cast<std::uint16_t>(seconds*60.0f);++i) if (!s.wait()) return false;
        return !s.failed();
    }
    void reset_idle() noexcept { idle_running=false; idle=0; }
    void input_activity(std::uint32_t p) noexcept { if (p) { reset_idle(); idle_running=true; } }
    static void timer_entry(IsleGameObject*, void* p) noexcept {
        auto& u=*static_cast<Impl*>(p);
        while (!u.s.failed()) {
            if (u.idle_running && u.idle<1800) ++u.idle;
            if (!u.s.wait()) return;
        }
    }
    void stop_motion(Group& g) noexcept {
        for (auto& m: motions) if (m.group==&g) {
            if (m.process) s.end_process(m.process);
            m.process=nullptr; m.group=nullptr;
        }
    }
    void motion(Group& g, unsigned kind, unsigned index=0) noexcept {
        for (auto& m: motions) if (!m.group) {
            m.group=&g; m.kind=kind; m.index=index;
            m.process=s.process(g.object,motion_entry,&m);
            return;
        }
        s.fail("menu concurrent cosmetic process capacity");
    }
    static void motion_entry(IsleGameObject*, void* p) noexcept {
        auto& m=*static_cast<Motion*>(p);
        auto& u=*m.owner;
        auto& g=*m.group;
        unsigned phase=0, count=0, index=m.index, color=255, tick=0;
        float x=m.kind==1 ? 287.0f : 671.0f;
        if (m.kind==1 || m.kind==2) {
            u.position(g,0,static_cast<int>(x),m.kind==1?172:150);
        }
        if (m.kind==0) { color=0; u.gray(g,index,0); }
        if (m.kind==5) color=128;
        while (!u.s.failed()) {
            if (m.kind==1 || m.kind==2) {
                if (++tick%4==0) {
                    u.visible(g,index,false);
                    const auto old=index;
                    index=(index+1)%g.count;
                    if (auto* v=u.view(g,old)) if (auto* next=u.view(g,index)) next->x=v->x;
                    u.visible(g,index,true);
                }
                if (auto* v=u.view(g,index)) {
                    if (v->x<=-32) {
                        u.show_all(g,false);
                        while(!u.s.failed()) if(!u.s.wait()) return;
                        return;
                    }
                    if (m.kind==1) { x-=0.5f; v->x=static_cast<std::int16_t>(x); }
                    else --v->x;
                }
            } else if (m.kind==0) {
                if (phase==0) { color=static_cast<std::uint8_t>(color+5); u.gray(g,index,color); if (color>250) { color=255; count=0; phase=1; } }
                else if (phase==1) { if (++count==120) phase=2; }
                else { color=static_cast<std::uint8_t>(color-5); if (color<128) {color=128;phase=0;} u.gray(g,index,color); }
            } else if(m.kind==4) {
                if(phase==0) { if(color>=132) color-=4; else {color=128;phase=1;} }
                else if(phase==1) { if(color<226) color+=30; else {color=255;count=0;phase=2;} }
                else if(count++>30) phase=0;
                u.gray(g,index,color);
            } else if(m.kind==5 || m.kind==8) {
                if(phase==0) {
                    if(color>=132) {u.gray(g,index,color);color-=4;}
                    else {u.gray(g,index,128);color=128;phase=1;}
                } else if(phase==1) {
                    if(color<226) {u.gray(g,index,color);color+=30;}
                    else {u.gray(g,index,255);color=0;phase=2;}
                } else if(color++>30) {color=255;phase=0;}
            } else {
                if (phase==0) { if (color<226) color+=30; else {color=255;count=0;phase=1;} }
                else if (phase==1) { if (count++>30) phase=2; }
                else { if (color>=132) color-=4; else {color=128;phase=0;} }
                u.gray(g,index,color);
            }
            if (!u.s.wait()) return;
        }
    }
    void background(unsigned value) noexcept {
        add(groups[0],0x802dabc0,0,0,attr|ISLE_SP_SCALE);
        gray(groups[0],0,value);
    }
    bool stacked_logo() noexcept {
        background(255);
        if (!fade(true,{0,0,0},1.0f)) return false;
        constexpr std::uint32_t ids[]{0x802db328,0x802db588,0x802dbce8,0x802dc1a8,0x802dc718,0x802dc978,0x802dce88,0x802dfab0};
        constexpr int xs[]{42,67,78,98,117,134,146,166}, ys[]{51,60,48,39,46,55,48,53};
        for (unsigned n=0;n<8;++n) {
            add(groups[1],ids[n],xs[n],ys[n],attr|ISLE_SP_SCALE|ISLE_SP_HIDDEN);
            if(auto* v=view(groups[1],n)) v->scalex=v->scaley=1.2f;
        }
        bool skip=false;
        for(unsigned n=0;n<8 && !skip;++n) {
            auto* v=view(groups[1],n);
            if(!v) return false;
            const float cx=v->x+v->width/2,cy=v->y+v->height/2;
            visible(groups[1],n,true);
            for(unsigned j=0;j<6;++j) {
                if(pressed()&(A|Start)) {skip=true;break;}
                const float scale=static_cast<float>(1.2-((j*0.2)/5.0));
                v->scalex=v->scaley=scale;
                v->x=static_cast<std::int16_t>(cx-v->width*scale*0.5f);
                v->y=static_cast<std::int16_t>(cy-v->height*scale*0.5f);
                if(!s.wait()) return false;
            }
            if(!skip && !(n&1)) {
                game_audio::Parameters parameters;
                parameters.pan=static_cast<std::uint8_t>(initialized.pan[n/2]);
                parameters.reverb=n==6 ? 10 : 0;
                if(s.play_sound(0,parameters)<0) return false;
            }
        }
        // Keep the native object and its equal-priority DL insertion position.
        if(!s.remove_sprites(groups[1].object)) return false;
        groups[1].sprites={}; groups[1].count=0;
        add(groups[1],0x802e8dd0,35,35);
        if(!fade(false,{255,255,255},17.0f/120.0f)) return false;
        clear(0,2);
        return !s.failed();
    }
    void menu_sprites(unsigned entry) noexcept {
        background(entry==13?255:128);
        add(groups[3],0x80309770,0,172,attr|ISLE_SP_HIDDEN);
        add(groups[3],0x8030a4e0,0,172,attr|ISLE_SP_HIDDEN);
        add(groups[3],0x8030b250,0,172,attr|ISLE_SP_HIDDEN);
        add(groups[3],0x8030bfc0,0,172,attr|ISLE_SP_HIDDEN);
        add(groups[4],0x8030e020,0,150,attr|ISLE_SP_HIDDEN);
        add(groups[4],0x80310080,0,150,attr|ISLE_SP_HIDDEN);
        add(groups[4],0x803120e0,0,150,attr|ISLE_SP_HIDDEN);
        add(groups[2],0x802f20f0,35,35);
        add(groups[2],0x802f82c8,74,198);
        if(s.controller==-1) { add(groups[11],0x8034fad8,59,147,attr|ISLE_SP_HIDDEN); return; }
        load_save();
        add(groups[5],0x802f4828,59,147,attr|ISLE_SP_HIDDEN);
        add(groups[6],0x8034b920,0,0,attr|ISLE_SP_HIDDEN);
        if(available.gallery) add(groups[8],0x8034d3a0,0,0,attr|ISLE_SP_HIDDEN);
        add(groups[9],0x802f6490,0,0,attr|ISLE_SP_HIDDEN);
        if(available.has_save) {
            add(groups[7],0x8034c4b0,0,0,attr|ISLE_SP_HIDDEN);
            add(groups[10],0x8034b020,40,131,attr|ISLE_SP_HIDDEN);
            add(groups[10],0x8034bbb8,170,172,attr|ISLE_SP_HIDDEN);
            add(groups[10],0x8034c710,124,172,attr|ISLE_SP_HIDDEN);
            gray(groups[10],1,128); gray(groups[10],2,128);
        }
        for(unsigned n=5;n<10;++n) if(groups[n].count) gray(groups[n],0,128);
    }
    bool menu_music(unsigned entry) noexcept {
        if(!volume(0,0x7f00)||!volume(1,0x7f00)||!song(0,0x17)) return false;
        if(entry==13) return fade(true,{255,255,255},1.0f);
        const auto* background_view=view(groups[0]);
        if(!background_view) return false;
        for(unsigned c=background_view->blue+5;c<=255;c+=5) {
            gray(groups[0],0,c);
            if(c>250) break;
            if(!s.wait()) return false;
        }
        gray(groups[0],0,255);
        return !s.failed();
    }
    bool leave_menu(bool quick) noexcept {
        if(!volume(0,0,quick?30:60)) return false;
        if(quick) {if(!s.wait(30)) return false;}
        else if(!fade(false,{0,0,0},1.0f)) return false;
        clear(0,12);
        return !s.failed();
    }
    std::uint8_t prompt() noexcept {
        stop_motion(groups[6]); gray(groups[6],0,255);
        show_all(groups[10],true); gray(groups[10],1,128);gray(groups[10],2,128);
        bool yes=false;
        motion(groups[10],3,1);
        if(!volume(1,0x7f00)||!song(1,0x23)||!volume(0,0x4000,30)||!s.wait(30)) return 0;
        while(!s.failed()) {
            const auto p=pressed();
            if(p&B) {yes=false;break;}
            if((p&Left)&&!yes) {sound(0x41);yes=true;stop_motion(groups[10]);gray(groups[10],1,128);motion(groups[10],3,2);}
            else if((p&Right)&&yes) {sound(0x41);yes=false;stop_motion(groups[10]);gray(groups[10],2,128);motion(groups[10],3,1);}
            else if(p&(A|Start)) break;
            if(!s.wait()) return 0;
        }
        if(yes) {if(!sound(0x42)||!reset_save()) return 0;}
        else {
            stop_motion(groups[10]);show_all(groups[10],false);
            if(!sound(0x43)||!volume(0,0x7fff,30)||!s.wait(30)) return 0;
        }
        if(!s.wait()) return 0;
        return yes?6:4;
    }
    std::uint8_t press_start(unsigned arg) noexcept {
        auto& g=groups[s.controller==0?5:11];
        visible(g,0,true);motion(g,0);
        while(!s.failed()) {
            const auto p=s.controller==0?pressed():0;
            if(s.controller==0) input_activity(p);
            const auto next=menu_transition(available,state,0,p,idle,s.controller,arg);
            if(next.state==10 || next.state==11) {
                if(!leave_menu(false)) return 0;
                return next.state;
            }
            if(next.state==3) {
                visible(g,0,false);stop_motion(g);
                return sound(0x42)?3:0;
            }
            if(!s.wait()) return 0;
        }
        return 0;
    }
    std::uint8_t select_menu(unsigned arg) noexcept {
        const unsigned variant=!available.has_save?0:available.gallery?2:1;
        const unsigned count=menu_item_count(available);
        std::array<unsigned,4> ordered{6,9,0,0};
        if(available.has_save) ordered={6,7,available.gallery?8u:9u,9};
        unsigned selection=state==4?0:available.has_save?1:0;
        for(unsigned n=0;n<count;++n) {
            auto& g=groups[ordered[n]];
            position(g,0,ordered[n]==6?127:ordered[n]==8?116:128,initialized.y_positions[variant][n]);
            visible(g,0,true);tint(g,0,192,192,0);
        }
        motion(groups[ordered[selection]],3);
        if(!s.wait()) return 0;
        while(!s.failed()) {
            const auto p=pressed();input_activity(p);
            const auto next=menu_transition(available,state,selection,p,idle,s.controller,arg);
            if(next.state==10 || next.state==11) {if(!leave_menu(false))return 0;return next.state;}
            if(p&(Up|Down)) {
                sound(0x41);auto& old=groups[ordered[selection]];stop_motion(old);tint(old,0,192,192,0);
                selection=next.selection;
                motion(groups[ordered[selection]],3);
            } else if(p&B) {
                sound(0x43);stop_motion(groups[ordered[selection]]);
                for(unsigned n=0;n<count;++n)visible(groups[ordered[n]],0,false);
                return 2;
            } else if(p&(A|Start)) {
                auto result=next.state;
                if(result==6 && available.has_save) result=prompt();
                else sound(result==7?0x40:0x42);
                if(result==4) return result;
                if(!result||!leave_menu(result==6||result==8)) return 0;
                return result;
            }
            if(!s.wait()) return 0;
        }
        return 0;
    }
    void option_sprites() noexcept {
        add(groups[12],0x802dabc0,0,0);gray(groups[12],0,102);
        for(int y : {36,54,166,197}) add(groups[13],0x803128e0,40,y);
        add(groups[13],0x80312238,39,168);add(groups[13],0x80312238,278,168);
        for(int y : {91,107,123}) add(groups[13],0x8033ed70,158,y);
        add(groups[14],0x80341790,43,40);add(groups[14],0x80341160,161,41);
        constexpr std::uint32_t labels[]{0x8033f498,0x8033fbb8,0x8033e938,0x8033e210,0x8033ecd0};
        constexpr int ys[]{73,89,105,121,137};
        for(unsigned n=0;n<5;++n) add(groups[15],labels[n],43,ys[n]);
        constexpr std::uint32_t values[3][2]{{0x80342ff0,0x80342150},{0x80341c70,0x803434d0},{0x80342b10,0x80342630}};
        for(unsigned n=0;n<3;++n) {
            add(groups[21+n],values[n][0],163,89+16*static_cast<int>(n));
            add(groups[21+n],values[n][1],222,89+16*static_cast<int>(n));
        }
        constexpr std::uint32_t descriptions[]{0x8032f360,0x80339f50,0x803280c0,0x80316230,0x8032ba10};
        for(auto id: descriptions) add(groups[16],id,41,168,attr|ISLE_SP_HIDDEN);
        add(groups[17],0x80332cb0,41,168,attr|ISLE_SP_HIDDEN);
        add(groups[18],0x8033d8a0,41,168,attr|ISLE_SP_HIDDEN);
        add(groups[18],0x80336600,41,168,attr|ISLE_SP_HIDDEN);
        add(groups[19],0x80320e20,41,168,attr|ISLE_SP_HIDDEN);
        add(groups[19],0x80324770,41,168,attr|ISLE_SP_HIDDEN);
        add(groups[20],0x8031d4d0,41,168,attr|ISLE_SP_HIDDEN);
        add(groups[20],0x80319b80,41,168,attr|ISLE_SP_HIDDEN);
        option=0;setting_choice=0;
        for(unsigned n=0;n<5;++n) gray(groups[15],n,255);
        for(unsigned n=0;n<3;++n) option_tints(n);
    }
    void option_tints(unsigned n) noexcept {
        for(unsigned i=0;i<2;++i) {
            if(i==static_cast<unsigned>(settings[n])) tint(groups[21+n],i,255,130,65);
            else gray(groups[21+n],i,255);
        }
    }
    unsigned choose_option() noexcept {
        motion(groups[15],4,option);
        visible(groups[16],option,true);
        if(!s.wait()) return 5;
        bool cancel=false;
        while(!s.failed()) {
            const auto p=pressed(), raw=raw_pressed();
            if(raw&A) {sound(0x42);break;}
            if(raw&B) {sound(0x43);cancel=true;break;}
            if(p&(Up|Down)) {
                sound(0x41);stop_motion(groups[15]);gray(groups[15],option,255);
                visible(groups[16],option,false);
                option=(option+(p&Up?4:1))%5;
                motion(groups[15],4,option);visible(groups[16],option,true);
            }
            if(!s.wait()) return 5;
        }
        show_all(groups[16],false);stop_motion(groups[15]);
        if(!s.wait()) return 5;
        tint(groups[15],option,255,130,65);
        if(!s.wait()) return 5;
        return cancel?4:option;
    }
    bool choose_setting() noexcept {
        const auto n=option-1;
        const bool original=settings[n];
        setting_choice=static_cast<unsigned>(original);
        motion(groups[21+n],5,setting_choice);visible(groups[18+n],setting_choice,true);
        if(!s.wait()) return false;
        while(!s.failed()) {
            const auto p=pressed(), raw=raw_pressed();
            if(raw&A) {sound(0x42);break;}
            if(raw&B) {
                sound(0x43);setting_choice=static_cast<unsigned>(original);
                if(n==0) s.audio.high_quality(!setting_choice);
                break;
            }
            if(p&(Left|Right)) {
                sound(0x41);
                const unsigned next=(p&Left)?0:1;
                if(next!=setting_choice) {
                    stop_motion(groups[21+n]);gray(groups[21+n],setting_choice,255);
                    visible(groups[18+n],setting_choice,false);
                    setting_choice=next;
                    motion(groups[21+n],8,setting_choice);visible(groups[18+n],setting_choice,true);
                }
                if(n==0) s.audio.high_quality(!setting_choice);
            }
            if(!s.wait()) return false;
        }
        settings[n]=setting_choice!=0;
        show_all(groups[18+n],false);stop_motion(groups[21+n]);
        if(!s.wait()) return false;
        option_tints(n);
        return s.wait();
    }
    void shift(Group& g,int delta) noexcept {
        for(unsigned n=0;n<g.count;++n) if(auto* v=view(g,n)) {
            v->x=static_cast<std::int16_t>(v->x+delta);
            v->y=static_cast<std::int16_t>(v->y+delta);
        }
    }
    bool calibration() noexcept {
        if (!live_offsets(offsets)) return false;
        const auto original=offsets;
        visible(groups[17],0,true);
        add(groups[24],0x80344388,0,-3);add(groups[24],0x80343b28,0,0);
        add(groups[24],0x80344618,135,-24);add(groups[24],0x80343c08,-24,95);
        add(groups[25],0x80344388,48,238);add(groups[25],0x80343b28,317,48);
        add(groups[25],0x803444d8,183,234);add(groups[25],0x80343c08,313,143);
        for(unsigned n=0;n<12;++n) {
            shift(groups[24],2);shift(groups[25],-2);if(!s.wait()) return false;
        }
        while(!s.failed()) {
            const auto p=pressed(), raw=raw_pressed();
            if(raw&Start) offsets={};
            else if(raw&A) {sound(0x42);break;}
            else if(raw&B) {sound(0x43);offsets=original;break;}
            if(p&Up) --offsets.vertical;
            else if(p&Down) ++offsets.vertical;
            if(p&Left) --offsets.horizontal;
            else if(p&Right) ++offsets.horizontal;
            offsets.horizontal=std::clamp(offsets.horizontal,-12,12);
            offsets.vertical=std::clamp(offsets.vertical,-12,12);
            s.state.set_screen_offsets({offsets.horizontal,offsets.horizontal,offsets.vertical,offsets.vertical});
            if(!s.wait()) return false;
        }
        // A accepts and B restores at the Source post-loop VI service, before
        // the first exit-animation wait; Start and directional updates publish
        // inside the loop before their own wait.
        s.state.set_screen_offsets({offsets.horizontal,offsets.horizontal,offsets.vertical,offsets.vertical});
        for(unsigned n=0;n<24;++n) {
            shift(groups[24],-2);shift(groups[25],2);if(!s.wait()) return false;
        }
        visible(groups[17],0,false);clear(24,26);
        return s.wait();
    }
    bool options() noexcept {
        load_save();option_sprites();
        for(unsigned c=255;c>102;c-=5) {gray(groups[12],0,c);if(!s.wait()) return false;}
        gray(groups[12],0,102);
        if(!s.wait()||!volume(0,0x7f00)||!song(0,0x1b)) return false;
        while(!s.failed()) {
            const auto chosen=choose_option();
            if(chosen==0) {if(!calibration()) return false;}
            else if(chosen<4) {if(!choose_setting()) return false;}
            else if(chosen==4) {if(!save_options()) return false;break;}
            else return false;
        }
        if(!volume(0,0,30)||!s.wait(30)||!stop_music()) return false;
        clear(12,24);
        state=12;
        return !s.failed();
    }
    bool menu(unsigned arg) noexcept {
        if(s.controller==0) {
            if(!stacked_logo()) return false;
            state=13;
        } else state=12;
        while(!s.failed()) {
            const auto entry=state;
            reset_idle();menu_sprites(entry);
            if(s.failed()||!menu_music(entry)) return false;
            motion(groups[3],1);motion(groups[4],2);
            idle_running=true;
            state=s.controller==-1?5:2;
            do {
                switch(state) {
                case 2: case 5: state=press_start(arg);break;
                case 3: case 4: state=select_menu(arg);break;
                case 8:
                    if(!stop_music()||!options()) return false;
                    break;
                case 11: return stop_music();
                case 6: case 7: case 9: case 10:
                    if(!stop_music()) return false;
                    s.request_exit(static_cast<std::int32_t>(menu_exit_scene(state)));
                    return true;
                default: s.fail("menu reached invalid source state");return false;
                }
            } while(!s.failed() && state!=12);
        }
        return false;
    }
    void run() noexcept {
        if(!s.check(s.audio.stop_sounds(),"menu entry stop sounds")) return;
        if(s.controller==-1) s.start_flags|=1;
        if((s.start_flags%2)==0) {
            s.start_flags|=1;
            if(!title.play()||!menu(14)) return;
        } else {
            if(!menu(15)||s.exit_requested()) return;
            if(!title.play()||!menu(16)) return;
        }
        while(!s.failed()&&!s.exit_requested()) if(!s.wait()) return;
    }
    static void control_entry(IsleGameObject*,void* p) noexcept { static_cast<Impl*>(p)->run(); }
    bool start() noexcept {
        if(started||retired||s.failed()) return false;
        if(s.controller!=0 && s.controller!=-1) {s.fail("menu source controller must be 0 or -1");return false;}
        if(s.input.set_menu_repeat(24,12,0xf0000)!=game_input::Error::none) {s.fail("menu repeat setup failed");return false;}
        timer=s.object(14);
        if(!timer||!s.process(timer,timer_entry,this)) return false;
        control=s.object(14);
        if(!control||!s.process(control,control_entry,this)) return false;
        started=true;
        return true;
    }
    void retire() noexcept {
        if(retired) return;
        if(control) s.erase(control);
        control=nullptr;
        if(timer) s.erase(timer);
        timer=nullptr;
        clear(0,static_cast<unsigned>(groups.size()));
        retired=true;
    }
};
Ui::Ui(::isle::scene::Services& s,Title& t,const Catalog& catalog)
    :impl_(std::make_unique<Impl>(s,t,catalog)) {}
Ui::~Ui() { retire(); }
bool Ui::start() noexcept { return impl_->start(); }
std::uint8_t Ui::source_state() const noexcept { return impl_->state; }
save::ScreenOffsets Ui::screen_offsets() const noexcept {
    save::ScreenOffsets out;
    impl_->live_offsets(out);
    return out;
}
std::uint32_t Ui::entry_flags() const noexcept { return menu_entry_flags(impl_->state); }
void Ui::retire() noexcept { if(impl_) impl_->retire(); }
} // namespace isle::menu::scene

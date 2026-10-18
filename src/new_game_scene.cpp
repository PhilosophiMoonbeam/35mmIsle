#include "new_game_scene.hpp"
#include "window_element.hpp"
#include "window_widgets.hpp"
#include <algorithm>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <vector>

namespace isle::new_game {
namespace {
constexpr std::uint32_t directions = 0xf0000;
void sound(EditEffect& e, unsigned id) noexcept { e.sounds[e.sound_count++] = id; }
std::string_view source_string(std::span<const std::uint8_t> bytes) noexcept {
    std::size_t size = 0;
    while (size < bytes.size() && bytes[size]) ++size;
    return {reinterpret_cast<const char*>(bytes.data()), size};
}
}
void Editor::jump_end(EditEffect& e) noexcept { x_=2; y_=19; sound(e,65); }
void Editor::backspace(EditEffect& e) noexcept {
    if (!length_) { name_[0]=name_[1]=0; jump_end(e); }
    else { name_[--length_]=0; name_[--length_]=0; sound(e,74); }
    e.redraw_name=true;
}
EditEffect Editor::press(std::uint32_t buttons, std::array<std::uint8_t,2> selection) noexcept {
    EditEffect e;
    if (confirming_ || confirmed_) return e;
    if (buttons & directions) {
        if (y_<19) {
            if (buttons & game_input::stick_slow_up) { if (--y_<0) y_=19; }
            if (buttons & game_input::stick_slow_down) ++y_;
            if (buttons & game_input::stick_slow_left) { if (--x_<0) x_=4; }
            if (buttons & game_input::stick_slow_right) x_=(x_+1)%5;
            if (y_>=19) { remembered_x_=x_; x_/=3; sound(e,65); }
            else sound(e,69);
        } else {
            if (buttons & game_input::stick_slow_up) --y_;
            if (buttons & game_input::stick_slow_down) y_=(y_+1)%20;
            if (buttons & game_input::stick_slow_left) x_=std::max(0,x_-1);
            if (buttons & game_input::stick_slow_right) x_=std::min(2,x_+1);
            if (y_<19) { x_=remembered_x_; sound(e,69); }
            else sound(e,65);
        }
        e.refresh_preview=true;
    } else if (buttons & 0x4000) {
        backspace(e); e.refresh_preview=true;
    } else if (buttons & 0x8000) {
        if (y_<19) {
            if (length_>=14) jump_end(e);
            else {
                name_[length_]=selection[0]; name_[length_+1]=selection[1];
                length_+=2; name_[length_]=0;
                e.redraw_name=true;
                if (length_==14) jump_end(e);
            }
            sound(e,66);
        } else if (x_==0) backspace(e);
        else if (x_==1) {
            if (length_<14) {
                sound(e,66); name_[length_]=0xa1; name_[length_+1]=0xa1;
                length_+=2; name_[length_]=0;
                // Source space helper does not redraw or auto-End at capacity.
            } else jump_end(e);
        } else { confirming_=true; e.confirm=true; }
        e.refresh_preview=!e.confirm;
    } else if (buttons & 0x1000) { jump_end(e); e.refresh_preview=true; }
    return e;
}
bool Editor::confirm(std::array<std::uint16_t,4> defaults) noexcept {
    if (!confirming_ || confirmed_) return false;
    bool blank=true;
    for (unsigned i=0;i<length_;++i) if (name_[i]!=0xa1) { blank=false; break; }
    if (blank) {
        // Preserve bytes after the source terminator, just as setPlayerName's
        // full 16-byte copy does after the source default string replacement.
        for (unsigned i=0;i<defaults.size();++i) {
            name_[2*i]=static_cast<std::uint8_t>(defaults[i]>>8);
            name_[2*i+1]=static_cast<std::uint8_t>(defaults[i]);
        }
        name_[8]=0;
    }
    confirming_=false; confirmed_=true;
    return blank;
}
std::array<int,4> Editor::focus() const noexcept {
    return y_>=19 && x_>=2 ? std::array<int,4>{63,y_*10+22,15,5} :
        std::array<int,4>{x_*13+25,y_*10+22,4,5};
}

struct Scene::Impl {
    std::shared_ptr<const window::Resources> resources;
    game_input::Service& input;
    game_audio::Session& audio;
    game::State& state;
    scene::Services* services;
    sprite::Producer* producer;
    Config config;
    std::shared_ptr<window::Heap> heap;
    window::ElementList list;
    std::unique_ptr<window::Buttons> buttons;
    std::unique_ptr<window::Text> text;
    std::unique_ptr<window::Frame> frame;
    std::unique_ptr<window::Focus> focus;
    std::array<std::shared_ptr<window::Element>,23> elements{};
    std::array<sprite::RasterBinding,23> bindings{};
    std::shared_ptr<sprite::Raster> fade_raster;
    sprite::RasterBinding fade_binding{};
    IsleSprite fade_view{};
    scene::SpriteDraw fade_draw{};
    std::array<scene::SpriteDraw,256> draws{};
    std::array<scene::render::Pass,4> passes{};
    std::array<sprite::RamRange,3> reservations;
    std::vector<sprite::AssetSpan> immutable;
    IsleGameCamera* sprite_camera=nullptr;
    IsleGameCamera* window_camera=nullptr;
    IsleGameObject* main_object=nullptr;
    IsleGameObject* preview_object=nullptr;
    IsleGameObject* input_object=nullptr;
    std::array<IsleGameObject*,2> camera_objects{};
    scene::SpriteHandle background{}, card{}, caret{};
    Editor editor;
    std::array<std::uint8_t,2> preview{};
    std::uint32_t next_pixels=0x400000;
    bool started=false, retired=false, ready=false, completed=false, result_taken=false;
    std::array<char,512> message{};

    Impl(scene::Services& s,std::shared_ptr<const window::Resources> r,sprite::Producer& p,Config c)
        : resources(std::move(r)),input(s.input),audio(s.audio),state(s.state),
          services(&s),producer(&p),config(c),reservations(c.commands),next_pixels(c.raster_begin) {}
    void fail(const char* error) noexcept {
        if (!message[0]) {
            std::size_t n=0;
            while (n+1<message.size() && error && error[n]) { message[n]=error[n]; ++n; }
            message[n]=0;
        }
        if (services) services->fail(message.data());
    }
    bool bad() const noexcept { return message[0] || (services && services->failed()); }
    static bool wait_text(void* owner,std::uint32_t count) { return static_cast<Impl*>(owner)->services->wait(count); }
    window::Element* element(unsigned slot,int x,int y,int w,int h,std::uint32_t flags,int font,window::Color bg,window::Color fg) {
        const int bpp=flags & window::bits32 ? 4 : 2;
        const int stride=(32+w+(bpp>>2)*16+15)&~15;
        const int tile_height=4096/bpp/stride;
        const auto bytes=static_cast<std::uint32_t>(stride*tile_height*((h+tile_height-1)/tile_height)*bpp);
        if (next_pixels>config.raster_end || bytes*2>config.raster_end-next_pixels)
            throw std::runtime_error("name UI physical raster extent");
        auto e=window::Element::create(heap,list,x,y,w,h,flags,{next_pixels,next_pixels+bytes});
        if (!e) throw std::runtime_error("name UI heap exhausted");
        next_pixels+=bytes*2;
        elements[slot]=std::move(e);
        bindings[slot]=producer->register_raster(elements[slot]->raster());
        auto* out=elements[slot].get();
        if (!out->attach(*services,bindings[slot])) throw std::runtime_error(services->error());
        out->set_buttons(buttons.get()); out->set_font(font); out->set_color(false,bg); out->set_color(true,fg);
        out->draw_background();
        return out;
    }
    bool setup() {
        if (!resources || !state.initialized()) throw std::invalid_argument("name scene requires initialized persistent State and actual window Resources");
        if (input.reset_scene_menu()!=game_input::Error::none)
            throw std::runtime_error("name source common input initialization");
        const auto sprite_assets=producer->assets();
        const auto common_assets=resources->common_render_assets();
        immutable.reserve(sprite_assets.size()+common_assets.size());
        immutable.insert(immutable.end(),sprite_assets.begin(),sprite_assets.end());
        immutable.insert(immutable.end(),common_assets.begin(),common_assets.end());
        heap=std::make_shared<window::Heap>(0x50000);
        buttons=std::make_unique<window::Buttons>(resources,heap,*services);
        text=std::make_unique<window::Text>(resources->font());
        if (!text->initialize(*services)) return false;
        frame=std::make_unique<window::Frame>(*services,*resources);
        focus=std::make_unique<window::Focus>(*services,*resources);
        if (input.set_menu_repeat(24,6,directions)!=game_input::Error::none) throw std::runtime_error("name source input repeat");
        camera_objects[0]=services->object(3);
        if (!camera_objects[0] || !services->check(isle_game_attach_camera(services->game,camera_objects[0],&sprite_camera),"name sprite camera")) return false;
        camera_objects[1]=services->object(0x200,3,3);
        if (!camera_objects[1] || !services->check(isle_game_attach_camera(services->game,camera_objects[1],&window_camera),"name window camera")) return false;
        // Source LINK_PLAYER9/object priority9, kind0 stackful process
        // priority0: invalidate after the lower-link Text consumers.
        input_object=services->object(1u<<9,9,9);
        if (!input_object || !services->process(input_object,invalidate_input,this,ISLE_GAME_STACKFUL,0)) return false;
        IsleSprite prototype{}; prototype.width=prototype.height=prototype.bmheight=prototype.bmHreal=1;
        prototype.nbitmaps=1; prototype.bmsiz=2; prototype.istep=1;
        prototype.attr=ISLE_SP_TRANSPARENT|ISLE_SP_SCALE; prototype.scalex=320; prototype.scaley=240;
        prototype.alpha=255;
        IsleBitmap bitmap{}; bitmap.width=bitmap.width_img=bitmap.actualHeight=1; bitmap.byte_count=2;
        const std::array<sprite::RasterPiece,1> pieces{{{bitmap,0}}};
        auto pixels=std::make_shared<std::array<std::uint8_t,16>>(); (*pixels)[1]=(*pixels)[9]=1;
        fade_raster=std::make_shared<sprite::Raster>(prototype,pieces,std::array<sprite::RasterStorage,2>{{
            {pixels,{pixels->data(),8},config.fade_addresses[0]},{pixels,{pixels->data()+8,8},config.fade_addresses[1]}}});
        fade_binding=producer->register_raster(fade_raster); fade_view=fade_raster->view(sprite::RasterSource::primary);
        fade_draw={{fade_binding.resources[0],&fade_view,fade_binding.generation},nullptr,{3,0,UINT32_MAX}};
        main_object=services->object(14);
        return main_object && services->process(main_object,run,this,ISLE_GAME_STACKFUL,1);
    }
    static void invalidate_input(IsleGameObject*,void* owner) noexcept {
        auto& self=*static_cast<Impl*>(owner);
        do {
            if (self.input.begin_update()!=game_input::Error::none) { self.fail("name common input invalidation"); return; }
        } while (self.services->wait());
    }
    void apply_focus() {
        const auto rect=editor.focus(); focus->set_target_pos(rect[0],rect[1]); focus->set_target_size(rect[2],rect[3]);
    }
    void redraw_name() {
        text->set_shadow_offset(0); text->set_spacing(0,4); text->set_print_delay(0);
        elements[21]->clear(); elements[21]->set_text_pos(0,0);
        elements[21]->print(*text,source_string(editor.name()));
    }
    bool caret_visible=false;
    void refresh_preview() {
        text->set_font(12); text->set_spacing(0,4); text->set_shadow_offset(0); text->set_print_delay(0);
        const int x=text->string_width(source_string(editor.name()))+203;
        if (auto* view=services->view(caret.sprite)) { view->x=static_cast<std::int16_t>(x); view->y=64; }
        if (editor.byte_length()>=14 || editor.row()>=19) {
            caret_visible=editor.byte_length()<14; elements[22]->set_visible(false); return;
        }
        const auto character=resources->name_character(255+editor.row()*5+editor.column());
        if (character.size()!=2) throw std::runtime_error("name grid requires two-byte source character");
        preview={static_cast<std::uint8_t>(character[0]),static_cast<std::uint8_t>(character[1])};
        caret_visible=preview[0]==0xa1 && preview[1]==0xa1;
        elements[22]->set_visible(!caret_visible); elements[22]->clear(); elements[22]->set_text_pos(0,0);
        elements[22]->set_pos(x,64); elements[22]->print(*text,character);
    }
    void initialize_ui() {
        background=services->sprite(0x80109880,1,0x80000000,14);
        card=services->sprite(0x80117f98,1,0x80000000,14);
        if (!background.sprite || !card.sprite) return;
        services->view(background.sprite)->attr=ISLE_SP_TEXSHUF|ISLE_SP_TRANSPARENT;
        auto* v=services->view(card.sprite); v->x=96; v->y=21; v->attr=ISLE_SP_TEXSHUF|ISLE_SP_TRANSPARENT;
        if (!frame->create()) return;
        frame->set_style(1); const auto color=frame->background_color(); frame->set_pos(184,320);
        frame->show(true); frame->show_background(false);
        if (!frame->fade_in(1)) return;
        auto* description=element(20,104,173,192,47,0,12,{color.r,color.g,color.b,255},{255,255,255,255});
        text->set_shadow_offset(1); text->set_spacing(-1,3);
        if (!description->print(*text,"Enter your name on the card.\nChoose with the Control Stick\nand select with the \\a Button!",{this,wait_text})) return;
        for (int i=0;i<20;++i) {
            text->set_print_delay(0); text->set_shadow_offset(1); text->set_spacing(-1,3);
            element(static_cast<unsigned>(i),23,i*10+20,60,8,0,8,{0,0,0,0},{255,255,255,255});
        }
        text->set_print_delay(0); text->set_shadow_offset(0); text->set_spacing(0,4);
        element(21,203,64,62,12,window::bits32,12,{0,0,0,0},{0,0,0,255});
        element(22,206,64,12,12,window::bits32,12,{0,0,0,0},{255,0,0,255});
        text->set_proportional(false);
        for (int y=0;y<19;++y) for (int x=0;x<5;++x) {
            elements[y]->set_text_pos(x*13,0); elements[y]->print(*text,resources->name_character(255+y*5+x));
        }
        text->set_proportional(true);
        for (int x=0;x<4;++x) { elements[19]->set_text_pos(x*13,0); elements[19]->print(*text,resources->name_end_label(x)); }
        if (!focus->create()) return;
        focus->show(false); apply_focus(); focus->move_front(); focus->show(true);
        for (int brightness=255;brightness>128;brightness-=5) {
            auto* bg=services->view(background.sprite); bg->red=bg->green=bg->blue=static_cast<std::uint8_t>(brightness);
            if (!services->wait()) return;
        }
        auto* bg=services->view(background.sprite); bg->red=bg->green=bg->blue=128;
        if (!services->wait()) return;
        services->check(audio.bgm_volume(0,0x7f00),"name BGM volume"); services->check(audio.play_song(0,12),"name BGM song12");
        preview_object=services->object(14);
        if (!preview_object || !services->process(preview_object,blink_preview,this,ISLE_GAME_STACKFUL,1)) return;
        caret=services->sprite(0x80118068,1,0x80000000,14);
        if (!caret.sprite) return;
        auto* cursor=services->view(caret.sprite); cursor->attr=ISLE_SP_TEXSHUF|ISLE_SP_TRANSPARENT; cursor->x=203; cursor->y=64;
        if (!services->process(caret.object,blink_caret,this,ISLE_GAME_STACKFUL,1)) return;
        refresh_preview(); ready=true;
    }
    void control() {
        while (!bad()) {
            const auto polled=input.menu(0);
            if (polled.error!=game_input::Error::none || !polled.input) { fail("name physical-port-zero input"); return; }
            const auto effect=editor.press(polled.input->pressed,preview);
            for (unsigned i=0;i<effect.sound_count;++i) services->play_sound(effect.sounds[i]);
            if (effect.redraw_name) redraw_name();
            if (effect.refresh_preview) { apply_focus(); refresh_preview(); }
            if (effect.confirm) {
                caret_visible=false;
                if (!services->wait()) return;
                const auto& font=resources->font();
                if (editor.confirm({font.ascii('T'),font.ascii('o'),font.ascii('d'),font.ascii('d')})) {
                    redraw_name(); services->play_sound(96,{0x7fff,84,10,1.0f});
                }
                if (services->save.set_player_name(std::as_bytes(editor.name()))) { fail("name full16-byte RAM record update"); return; }
                services->play_sound(66); ready=false; return;
            }
            if (!services->wait()) return;
        }
    }
    static void run(IsleGameObject*,void* owner) noexcept {
        auto& self=*static_cast<Impl*>(owner);
        try {
            self.initialize_ui();
            if (!self.ready || self.bad()) return;
            self.control();
            if (!self.editor.confirmed() || self.bad()) return;
            self.services->check(self.audio.bgm_volume(0,0,60),"name BGM source fade60");
            if (!self.services->begin_fade(false,1.0f,{0,0,0})) return;
            for (unsigned i=0;i<60;++i) if (!self.services->wait()) return;
            // menu_new_game/A5CC50.c294 passes uninitialized values instead of
            // output pointers. Preserve the actual live VI rather than inventing
            // an output write from reset profile RAM or undefined stack bytes.
            self.services->request_exit(11);
            while (self.services->wait()) {}
        } catch (const std::exception& e) { self.fail(e.what()); }
        catch (...) { self.fail("name native continuation failure"); }
    }
    static void blink_preview(IsleGameObject*,void* owner) noexcept {
        auto& self=*static_cast<Impl*>(owner); int value=10,delta=5;
        try { while (!self.bad()) {
            value+=delta; if (value>255) { value=255; delta=-5; } else if (value<120) { value=120; delta=5; }
            self.elements[22]->set_sprite_color({255,255,255,static_cast<std::uint8_t>(value>160?255:0)});
            if (!self.services->wait()) return;
        }} catch (const std::exception& e) { self.fail(e.what()); }
    }
    static void blink_caret(IsleGameObject*,void* owner) noexcept {
        auto& self=*static_cast<Impl*>(owner); int value=10,delta=5;
        while (!self.bad()) {
            auto* view=self.services->view(self.caret.sprite); if (!view) return;
            if (!self.caret_visible) { value=10; delta=5; view->attr|=ISLE_SP_HIDDEN; } else view->attr&=~ISLE_SP_HIDDEN;
            value+=delta; if (value>255) { value=255; delta=-5; } else if (value<120) { value=120; delta=5; }
            view->alpha=static_cast<std::uint8_t>(value);
            if (!self.services->wait()) return;
        }
    }
    void retire() noexcept {
        if (retired) return;
        // Join native borrowers first, while every callback owner/descriptor is
        // still alive. Then detach/unregister before arena and raster release.
        if (services) {
            services->erase(main_object); main_object=nullptr;
            services->erase(preview_object); preview_object=nullptr;
            services->erase(input_object); input_object=nullptr;
            services->erase(caret.object); caret={};
            services->erase(background.object); background={};
            services->erase(card.object); card={};
            for (auto*& object:camera_objects) { services->erase(object); object=nullptr; }
            if (text) text->detach();
            if (focus) focus->retire();
            if (frame) frame->retire();
            if (buttons) buttons->clear();
            services->stop_sound_ramps();
        }
        for (std::size_t i=0;i<elements.size();++i) if (elements[i]) {
            elements[i]->detach();
            try { if (bindings[i].generation) producer->unregister_raster(bindings[i]); } catch (const std::exception& e) { fail(e.what()); return; }
            elements[i].reset();
        }
        if (fade_raster) {
            try { if (fade_binding.generation) producer->unregister_raster(fade_binding); } catch (const std::exception& e) { fail(e.what()); return; }
            fade_raster.reset();
        }
        focus.reset(); frame.reset(); text.reset(); buttons.reset();
        heap.reset(); retired=true; ready=false;
    }
};
Scene::Scene(scene::Services& s,std::shared_ptr<const window::Resources> r,sprite::Producer& p,Config c)
    : impl_(std::make_unique<Impl>(s,std::move(r),p,c)) {}
Scene::~Scene() { impl_->retire(); }
bool Scene::start() noexcept {
    if (impl_->started || impl_->retired) return false;
    impl_->started=true;
    try { return impl_->setup() && !impl_->bad(); }
    catch (const std::exception& e) { impl_->fail(e.what()); return false; }
}
scene::render::Input Scene::render_input() noexcept {
    auto& s=*impl_;
    if (!s.services || !s.producer || s.retired || s.bad()) return {};
    const auto offsets=screen_offsets();
    if (s.bad()) return {};
    std::size_t count=0;
    if (!s.services->sprite_draws(s.draws,count)) return {};
    auto& clear=s.passes[0]; clear={}; clear.kind=scene::render::Kind::clear; clear.priority=100; clear.flags=6; clear.background_rgba=0xff;
    for (unsigned i=0;i<2;++i) {
        auto& pass=s.passes[i+1]; pass={}; pass.kind=scene::render::Kind::sprites;
        pass.camera=i?s.window_camera:s.sprite_camera; pass.priority=i?2:3;
        pass.flags=i?4:8; pass.link_mask=std::uint64_t(1)<<(i?2:1);
        pass.camera_tag=i?4u:UINT32_MAX;
        pass.tag_match=i?scene::render::TagMatch::intersection:scene::render::TagMatch::equality;
        pass.sprites=std::span<const scene::SpriteDraw>(s.draws).first(count);
        pass.sprite_commands=s.reservations[i];
    }
    auto& fade=s.passes[3]; fade={}; fade.kind=scene::render::Kind::sprites; fade.camera=s.sprite_camera;
    fade.priority=1; fade.link_mask=8; fade.camera_tag=UINT32_MAX; fade.sprite_commands=s.reservations[2];
    const auto fade_state=s.services->fade();
    s.fade_view.alpha=fade_state.visible?fade_state.alpha:0;
    s.fade_view.attr=ISLE_SP_TRANSPARENT|ISLE_SP_SCALE|(fade_state.visible?0:ISLE_SP_HIDDEN);
    fade.sprites={&s.fade_draw,1};
    return {s.services->game,
            {s.config.resource_identity,s.producer,s.immutable,s.reservations,
             {0x3b5000,0x3da800},s.resources->common_pre_render()},
            s.passes,offsets};
}
Observation Scene::observation() const noexcept {
    const auto& s=*impl_; Observation out;
    out.column=s.editor.column(); out.row=s.editor.row(); out.name_bytes=s.editor.byte_length();
    std::copy(s.editor.name().begin(),s.editor.name().end(),out.name.begin());
    out.controls_ready=s.ready; out.confirming=s.editor.confirming(); out.confirmed=s.editor.confirmed();
    out.exit_requested=exit_requested(); out.completed=s.completed; return out;
}
bool Scene::failed() const noexcept { return impl_->bad(); }
const char* Scene::error() const noexcept { return impl_->message[0]?impl_->message.data():impl_->services?impl_->services->error():""; }
bool Scene::exit_requested() const noexcept { return impl_->services && impl_->services->exit_requested(); }
bool Scene::completed() const noexcept { return impl_->completed; }
bool Scene::complete_scene() noexcept {
    if (impl_->completed || impl_->bad() || !exit_requested() || !impl_->editor.confirmed()) return false;
    try {
        impl_->retire();
        if (!impl_->retired || impl_->bad()) return false;
        if (impl_->state.save()->set_player_flag(16,true)) { impl_->fail("name post-scene PFID16"); return false; }
        impl_->completed=true; return true;
    } catch (const std::exception& e) { impl_->fail(e.what()); return false; }
}
bool Scene::take_result(std::int32_t& out) noexcept {
    if (!impl_->completed || impl_->result_taken) return false;
    out=11; impl_->result_taken=true; return true;
}
save::ScreenOffsets Scene::screen_offsets() const noexcept {
    save::ScreenOffsets out;
    if (!impl_->state.screen_offsets().translation(out))
        impl_->fail("name live VI endpoints cannot be represented by native translation");
    return out;
}
void Scene::retire() noexcept { impl_->retire(); }
} // namespace isle::new_game

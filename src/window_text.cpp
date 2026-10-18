#include "window_text.hpp"
#include "scene_services.hpp"
#include <algorithm>
#include <stdexcept>

namespace isle::window {
namespace {
Color blend(Color a, Color b, int amount) noexcept {
    return {static_cast<std::uint8_t>(int(a.r) + (int(b.r) - a.r) * amount / 15),
            static_cast<std::uint8_t>(int(a.g) + (int(b.g) - a.g) * amount / 15),
            static_cast<std::uint8_t>(int(a.b) + (int(b.b) - a.b) * amount / 15),
            static_cast<std::uint8_t>(int(a.a) + (int(b.a) - a.a) * amount / 15)};
}
std::uint16_t pack(Color c) noexcept {
    return static_cast<std::uint16_t>(((c.r << 8) & 0xf800) | ((c.g << 3) & 0x7c0) | ((c.b >> 2) & 0x3e) | (c.a != 0));
}
std::uint16_t read16(const std::uint8_t* p) noexcept { return static_cast<std::uint16_t>((p[0] << 8) | p[1]); }
void write16(std::uint8_t* p, std::uint16_t v) noexcept { p[0] = static_cast<std::uint8_t>(v >> 8); p[1] = static_cast<std::uint8_t>(v); }
bool button_code(std::uint16_t c) noexcept {
    if ((c >> 8) != '\\') return false;
    switch (c & 255) { case 'a':case 'b':case 'c':case 'd':case 'f':case 'l':case 'm':case 'r':case 's':case 'u':case 'z':return true; default:return false; }
}
struct Character { std::uint16_t value; std::size_t length; unsigned first; };
Character next_character(const FontData& data, std::string_view s, std::size_t i) {
    const auto c = static_cast<unsigned char>(s[i]);
    if (c == '\n') return {0x0a,1,c};
    if (c == ' ') return {0xa1a1,1,c};
    if (c > 32 && c < 127 && c != '\\') return {data.ascii(c),1,c};
    if (i + 1 >= s.size()) throw std::invalid_argument("truncated source text character");
    return {static_cast<std::uint16_t>((c << 8) | static_cast<unsigned char>(s[i+1])),2,c};
}
}
Text::Text(FontData data) : data_(data) { data_.validate(); palettes(foreground_, background_); }
Text::~Text() { detach(); }
bool Text::initialize(scene::Services& services) {
    if (services_) return false;
    auto* object=services.object(1u<<6,6,6);
    if (!object) return false;
    if (!services.process(object,poll_input,this,ISLE_GAME_SYNCHRONOUS,0)) { services.erase(object);return false; }
    services_=&services;input_object_=object;set_print_delay(0);return true;
}
void Text::detach() noexcept {
    if (active_print_) cancel_print(*active_print_);
    if (services_) services_->erase(input_object_);
    services_=nullptr;input_object_=nullptr;
}
void Text::poll_input(IsleGameObject*,void* userdata) noexcept {
    auto* text=static_cast<Text*>(userdata);
    const auto sample=text->services_->input.menu(0);
    if (sample.error!=game_input::Error::none || !sample.input) {
        text->services_->fail("window text input publication failed");return;
    }
    text->input(sample.input->pressed);
}
void Text::intervals() noexcept {
    horizontal_interval_ = glyph_size() + horizontal_spacing_ + shadow_offset_;
    vertical_interval_ = glyph_size() + vertical_spacing_ + shadow_offset_;
}
void Text::set_font(int f) noexcept { charset_ = f == 12 ? 1 : 0; intervals(); }
void Text::set_spacing(int h, int v) noexcept { horizontal_spacing_ = h; vertical_spacing_ = v; intervals(); }
void Text::set_shadow_offset(int p) {
    if (p < 0 || p > 320) throw std::invalid_argument("window shadow offset");
    shadow_offset_ = p; intervals();
}
void Text::input(std::uint32_t pressed) noexcept { if (skip_enabled_ && (pressed & 0xc000)) delay_disabled_ = true; }
void Text::palettes(Color fg, Color bg) noexcept {
    foreground_ = fg; background_ = bg;
    for (int i = 0; i < 16; ++i) {
        text_palette_[i] = pack(blend(bg, fg, i));
        shadow_palette_[i] = pack(blend(bg, shadow_, i));
        shadowed_palette_[i] = pack(blend(shadow_, fg, i));
    }
}
int Text::string_width(std::string_view s) const {
    int width = 0;
    bool proportional = proportional_;
    for (std::size_t i = 0; i < s.size() && s[i];) {
        const auto c = next_character(data_, s, i); i += c.length;
        auto ch = c.value;
        if (button_code(ch)) { width += 12; continue; }
        if (ch == 0x5c69) { proportional = false; continue; }
        if (ch == 0x5c67) { proportional = true; continue; }
        if (ch == 0xa1dd) ch = 0xa1bc;
        if (ch == 0x0a || ch == 0xa2a1 || c.first == '\\') continue;
        if (ch == 0xa1a1 && proportional) width += charset_ ? 4 : 2;
        else width += proportional ? data_.widths[charset_][data_.index(ch)] + 1 : (charset_ ? 9 : 6);
    }
    return width;
}
int Text::draw_glyph(TextSurface& s, int x, int y, std::uint16_t ch) {
    wrote_glyph_ = false;
    const auto glyph = data_.glyph(static_cast<unsigned>(charset_), ch);
    const int n = glyph_size();
    if (glyph.empty()) return n + shadow_offset_;
    if (s.width <= 0 || s.height <= 0 || x < 0 || y < 0 || (s.bpp != 1 && s.bpp != 2 && s.bpp != 4))
        throw std::invalid_argument("window text surface");
    // GAME dispatches every non-32-bit surface through PrintChar16, including
    // 8-bit elements. Keep that distinction; bound its real 16-bit byte writes
    // against the complete padded allocation rather than inventing an IA8 font.
    const std::size_t pixel_bytes = s.bpp == 4 ? 4 : 2;
    const auto visible_rows = std::max(0, std::min(n, s.height - y));
    const auto visible_columns = std::max(0, std::min(n, s.width - x));
    if (!visible_rows || !visible_columns) return n + shadow_offset_;
    const int last_row = s.bpp == 4 ? y + visible_rows - 1 + shadow_offset_ : std::min(s.height - 1, y + visible_rows - 1 + shadow_offset_);
    const int last_column = s.bpp == 4 ? x + visible_columns - 1 + shadow_offset_ : std::min(s.width - 1, x + visible_columns - 1 + shadow_offset_);
    const auto last = (static_cast<std::size_t>(last_row) * s.width + static_cast<std::size_t>(last_column)) * pixel_bytes;
    if (last > s.image.size() || pixel_bytes > s.image.size() - last)
        throw std::out_of_range("source glyph/shadow allocation boundary");
    std::size_t glyph_byte = 0;
    for (int row = 0; row < n; ++row) {
        if (y + row >= s.height) continue;
        for (int col = 0; col < n; ++col) {
            if (x + col >= s.width) continue;
            const auto ci = (col & 1) ? glyph[glyph_byte++] >> 4 : glyph[glyph_byte] & 15;
            if (!ci) continue;
            wrote_glyph_ = true;
            const auto at = static_cast<std::size_t>((y + row) * s.width + x + col) * pixel_bytes;
            auto* pixel = s.image.data() + at;
            if (s.bpp == 4) {
                const auto color = blend({pixel[0],pixel[1],pixel[2],pixel[3]}, foreground_, ci);
                pixel[0]=color.r; pixel[1]=color.g; pixel[2]=color.b; pixel[3]=color.a;
            } else {
                const auto diagonal = at + (static_cast<std::size_t>(s.width) + 1) * 2;
                const bool on_shadow = read16(pixel) == shadow_palette_[15] ||
                    (diagonal + 2 <= s.image.size() && read16(s.image.data() + diagonal) == shadow_palette_[15]);
                write16(pixel, on_shadow ? shadowed_palette_[ci] : text_palette_[ci]);
            }
            int shadow_step = 0;
            for (int k = shadow_offset_; k > 0; --k) {
                if (s.bpp != 4 && (y + row + k >= s.height || x + col + k >= s.width)) continue;
                // Both source loops walk from the nearest diagonal outward,
                // despite counting k downward: first pixel gets solid shadow
                // for offsets >1; the final pixel gets the antialiased edge.
                const int step = ++shadow_step;
                auto* shadow = pixel + static_cast<std::size_t>(step) * (s.width + 1) * pixel_bytes;
                if (s.bpp == 4) {
                    const auto color = blend({shadow[0],shadow[1],shadow[2],shadow[3]}, shadow_, k == 1 ? ci : 15);
                    shadow[0]=color.r; shadow[1]=color.g; shadow[2]=color.b; shadow[3]=color.a;
                } else write16(shadow, k == 1 ? shadow_palette_[ci] : shadow_palette_[15]);
            }
        }
    }
    return n + shadow_offset_;
}
StepResult Text::begin_print(PrintOperation& op, TextSurface surface, int& x, int& y,
                            std::string_view bytes, TextEvents events, TextEncoding encoding) noexcept {
    if (active_print_ || (op.owner_ && op.result_.status!=StepStatus::done && op.result_.status!=StepStatus::error))
        return {StepStatus::error,StepError::busy};
    op.owner_=this;op.surface_=surface;op.x_=&x;op.y_=&y;op.bytes_=bytes;
    op.events_=events;op.encoding_=encoding;op.index_=0;op.phase_=PrintOperation::Phase::next;
    op.result_={StepStatus::more};op.event_={};active_print_=&op;
    return op.result_;
}
StepResult Text::finish_print(PrintOperation& op,StepError error) noexcept {
    op.result_={error==StepError::none?StepStatus::done:StepStatus::error,error};
    op.event_={};
    op.surface_={};op.bytes_={};op.x_=op.y_=nullptr;
    if (active_print_==&op) active_print_=nullptr;
    return op.result_;
}
void Text::cancel_print(PrintOperation& op) noexcept {
    if (op.owner_==this && active_print_==&op) finish_print(op,StepError::cancelled);
}
StepResult Text::step_print(PrintOperation& op,bool resume) noexcept {
    if (op.owner_!=this) return {StepStatus::error,StepError::invalid_operation};
    if (op.result_.status==StepStatus::done || op.result_.status==StepStatus::error) return op.result_;
    if (active_print_!=&op) return {StepStatus::error,StepError::invalid_operation};
    if (op.result_.status==StepStatus::event || op.result_.status==StepStatus::needs_source_wait) {
        if (!resume) return op.result_;
        op.result_={StepStatus::more};op.event_={};
    } else if (resume) return {StepStatus::error,StepError::invalid_operation};
    try { return step_print_impl(op); }
    catch (const std::invalid_argument&) { return finish_print(op,StepError::invalid_surface); }
    catch (const std::out_of_range&) { return finish_print(op,StepError::surface_boundary); }
    catch (...) { return finish_print(op,StepError::native_failure); }
}
StepResult Text::step_print_impl(PrintOperation& op) {
    using Phase=PrintOperation::Phase;
    auto& surface=op.surface_;
    auto& x=*op.x_;auto& y=*op.y_;
    const auto event=[&op](TextEvent value) {
        op.event_=value;op.result_={StepStatus::event};return op.result_;
    };
    switch (op.phase_) {
    case Phase::next: {
        if (op.index_>=op.bytes_.size() || !op.bytes_[op.index_]) {
            if (op.encoding_==TextEncoding::source) delay_disabled_=false;
            return finish_print(op);
        }
        op.advance_=true;op.newline_=false;op.escape_=false;
        op.phase_=Phase::delay;op.waits_=0;
        if (op.encoding_==TextEncoding::ascii) {
            const auto c=static_cast<unsigned char>(op.bytes_[op.index_++]);
            if (c>32 && c<127) {
                op.character_=data_.ascii(c);op.position_=x;op.draw_y_=y;
                draw_glyph(surface,x,y,op.character_);
                op.row_=op.shadow_step_=0;
                op.columns_=std::min(glyph_size(),surface.width-x);
                op.phase_=op.events_.changed && wrote_glyph_?Phase::changed:Phase::advance;
            } else op.phase_=Phase::advance;
            return op.result_;
        }
        const auto first=static_cast<unsigned char>(op.bytes_[op.index_]);
        if (first!='\n' && first!=' ' && !(first>32 && first<127 && first!='\\') &&
            op.index_+1>=op.bytes_.size()) return finish_print(op,StepError::invalid_text);
        const auto c=next_character(data_,op.bytes_,op.index_);op.index_+=c.length;
        auto ch=c.value;op.character_=ch;
        if (button_code(ch)) {
            if (!op.events_.button) return finish_print(op,StepError::missing_button);
            return event({TextEvent::Kind::button,0,x+surface.screen_x,y+surface.screen_y,glyph_size(),static_cast<char>(ch)});
        }
        switch (ch) {
        case 0x5c65: palettes({255,0,0,255},background_);op.advance_=false;break;
        case 0x5c68: palettes({255,255,0,255},background_);op.advance_=false;break;
        case 0x5c70: palettes({255,255,255,255},background_);op.advance_=false;break;
        case 0x5c77: delay_disabled_=false;skip_enabled_=false;op.advance_=false;break;
        case 0x5c74: skip_enabled_=true;op.advance_=false;break;
        case 0x5c67: proportional_=true;op.advance_=false;break;
        case 0x5c69: proportional_=false;op.advance_=false;break;
        case 0x5c6a: set_font(0);op.advance_=false;break;
        case 0x5c6b: set_font(1);op.advance_=false;break;
        case 0x0a: case 0x5c6e: case 0xa2a1: op.newline_=true;op.advance_=false;break;
        case 0xa1a1: break;
        default:
            if (c.first=='\\') {
                op.escape_=true;op.advance_=false;op.phase_=Phase::character;break;
            }
            if (ch==0xa1dd) ch=0xa1bc;
            op.character_=ch;
            op.position_=proportional_?x:x+static_cast<int>(((charset_?9:6)-data_.widths[charset_][data_.index(ch)])*.5+.5);
            draw_glyph(surface,op.position_,y,ch);
            op.draw_y_=y;
            op.row_=op.shadow_step_=0;
            op.columns_=std::min(glyph_size(),surface.width-op.position_);
            op.phase_=op.events_.changed && wrote_glyph_?Phase::changed:Phase::character;
            break;
        }
        return op.result_;
    }
    case Phase::changed:
        // Publish the exact original row/diagonal ranges, one deferred event at
        // a time. No event queue/allocation or callbacks under the native leaf.
        while (op.row_<glyph_size() && op.draw_y_+op.row_<surface.height) {
            if (op.shadow_step_>shadow_offset_) {++op.row_;op.shadow_step_=0;continue;}
            const int step=op.shadow_step_++;
            if (surface.bpp!=4 && (op.draw_y_+op.row_+step>=surface.height || op.position_+step>=surface.width)) continue;
            const int count=surface.bpp==4?op.columns_:std::min(op.columns_,surface.width-op.position_-step);
            if (count<=0) continue;
            const auto bytes=static_cast<std::size_t>(surface.bpp==4?4:2);
            TextEvent value;value.kind=TextEvent::Kind::changed;
            value.offset=(static_cast<std::size_t>(op.draw_y_+op.row_+step)*surface.width+static_cast<std::size_t>(op.position_+step))*bytes;
            value.size=static_cast<std::size_t>(count)*bytes;
            return event(value);
        }
        op.phase_=op.encoding_==TextEncoding::ascii?Phase::advance:Phase::character;
        return op.result_;
    case Phase::character:
        op.phase_=Phase::delay;
        if (op.events_.character && !delay_disabled_ && (!op.escape_ || delay_!=0))
            return event({TextEvent::Kind::character,op.character_});
        return op.result_;
    case Phase::delay:
        // Snapshot delay only after the callback, just like the source loop.
        // The negative sentinel distinguishes initialization from later waits.
        if (op.waits_==0) op.waits_=delay_>0?delay_:-1;
        if (op.advance_ && op.waits_>0 && !delay_disabled_) {
            --op.waits_;
            if (!op.waits_) op.waits_=-1;
            op.result_={StepStatus::needs_source_wait,StepError::none,1};
            return op.result_;
        }
        op.phase_=Phase::advance;
        return op.result_;
    case Phase::advance:
        if (op.encoding_==TextEncoding::ascii) {
            x+=horizontal_interval_;
            op.newline_=x>surface.width-horizontal_interval_;
        } else if (op.advance_) {
            const auto ch=op.character_;
            x+=proportional_?(ch==0xa1a1?(charset_?4:2):data_.widths[charset_][data_.index(ch)]+1):(charset_?9:6);
            op.newline_=x>surface.width-horizontal_interval_;
        }
        if (op.newline_) {
            x=0;y+=vertical_interval_;
            if (y>surface.height-glyph_size()-shadow_offset_) y=0;
        }
        op.phase_=Phase::next;
        return op.result_;
    }
    return finish_print(op,StepError::invalid_operation);
}
bool Text::run_print(TextSurface& surface,int& x,int& y,std::string_view bytes,TextHooks hooks,TextEncoding encoding) {
    PrintOperation op;
    auto result=begin_print(op,surface,x,y,bytes,
        {hooks.character!=nullptr,hooks.button!=nullptr,hooks.changed!=nullptr},encoding);
    try {
        for (;;) {
            switch (result.status) {
            case StepStatus::more: result=step_print(op);break;
            case StepStatus::event: {
                const auto value=op.event();
                switch (value.kind) {
                case TextEvent::Kind::character: hooks.character(hooks.userdata,value.character);break;
                case TextEvent::Kind::button: hooks.button(hooks.userdata,value.x,value.y,value.button,value.scale);break;
                case TextEvent::Kind::changed: hooks.changed(hooks.userdata,value.offset,value.size);break;
                case TextEvent::Kind::none: break;
                }
                result=step_print(op,true);break;
            }
            case StepStatus::needs_source_wait:
                if (!hooks.wait) throw std::logic_error("typewriter requires native process wait");
                if (!hooks.wait(hooks.userdata,result.updates)) {cancel_print(op);return false;}
                result=step_print(op,true);break;
            case StepStatus::done: return true;
            case StepStatus::error:
                switch (result.error) {
                case StepError::cancelled: return false;
                case StepError::invalid_text: throw std::invalid_argument("truncated source text character");
                case StepError::invalid_surface: throw std::invalid_argument("window text surface");
                case StepError::surface_boundary: throw std::out_of_range("source glyph/shadow allocation boundary");
                case StepError::missing_button: throw std::logic_error("button escape requires native button owner");
                default: throw std::logic_error("window text operation failed");
                }
            }
        }
    } catch (...) {cancel_print(op);throw;}
}
bool Text::print(TextSurface& surface,int& x,int& y,std::string_view bytes,TextHooks hooks) {
    return run_print(surface,x,y,bytes,hooks,TextEncoding::source);
}
void Text::print_ascii(TextSurface& surface,int& x,int& y,std::string_view bytes,TextHooks hooks) {
    run_print(surface,x,y,bytes,{hooks.userdata,nullptr,nullptr,nullptr,hooks.changed},TextEncoding::ascii);
}
} // namespace isle::window

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
void Text::publish_glyph(const TextSurface& s,int x,int y,TextHooks hooks) const {
    if (!hooks.changed || !wrote_glyph_) return;
    const auto bytes=static_cast<std::size_t>(s.bpp==4?4:2);
    const int columns=std::min(glyph_size(),s.width-x);
    for (int row=0;row<glyph_size() && y+row<s.height;++row) {
        for (int step=0;step<=shadow_offset_;++step) {
            if (s.bpp!=4 && (y+row+step>=s.height || x+step>=s.width)) continue;
            const int count=s.bpp==4?columns:std::min(columns,s.width-x-step);
            if (count<=0) continue;
            const auto offset=(static_cast<std::size_t>(y+row+step)*s.width+static_cast<std::size_t>(x+step))*bytes;
            hooks.changed(hooks.userdata,offset,static_cast<std::size_t>(count)*bytes);
        }
    }
}
bool Text::print(TextSurface& surface, int& x, int& y, std::string_view s, TextHooks h) {
    // Only trivial automatic state crosses h.wait; all fonts/palettes/pixels and
    // callback userdata are retained by their scene-lifetime owners.
    for (std::size_t i = 0; i < s.size() && s[i];) {
        const auto c = next_character(data_, s, i); i += c.length;
        auto ch = c.value;
        bool advance = true, newline = false;
        if (button_code(ch)) {
            if (!h.button) throw std::logic_error("button escape requires native button owner");
            h.button(h.userdata, x + surface.screen_x, y + surface.screen_y, static_cast<char>(ch), glyph_size());
        } else switch (ch) {
        case 0x5c65: palettes({255,0,0,255},background_); advance=false; break;
        case 0x5c68: palettes({255,255,0,255},background_); advance=false; break;
        case 0x5c70: palettes({255,255,255,255},background_); advance=false; break;
        case 0x5c77: delay_disabled_=false; skip_enabled_=false; advance=false; break;
        case 0x5c74: skip_enabled_=true; advance=false; break;
        case 0x5c67: proportional_=true; advance=false; break;
        case 0x5c69: proportional_=false; advance=false; break;
        case 0x5c6a: set_font(0); advance=false; break;
        case 0x5c6b: set_font(1); advance=false; break;
        case 0x0a: case 0x5c6e: case 0xa2a1: newline=true; advance=false; break;
        case 0xa1a1: break;
        default:
            if (c.first == '\\') {
                if (delay_ != 0 && h.character && !delay_disabled_) h.character(h.userdata,ch);
                advance=false; break;
            }
            if (ch == 0xa1dd) ch=0xa1bc;
            {
                const int position=proportional_?x:x+static_cast<int>(((charset_?9:6)-data_.widths[charset_][data_.index(ch)])*.5+.5);
                draw_glyph(surface,position,y,ch);
                publish_glyph(surface,position,y,h);
            }
            if (h.character && !delay_disabled_) h.character(h.userdata,ch);
            break;
        }
        if (advance) {
            for (int wait = delay_; wait > 0 && !delay_disabled_; --wait) {
                if (!h.wait) throw std::logic_error("typewriter requires native process wait");
                if (!h.wait(h.userdata,1)) return false;
            }
            x += proportional_ ? (ch == 0xa1a1 ? (charset_ ? 4 : 2) : data_.widths[charset_][data_.index(ch)] + 1) : (charset_ ? 9 : 6);
            newline = x > surface.width - horizontal_interval_;
        }
        if (newline) {
            x = 0; y += vertical_interval_;
            if (y > surface.height - glyph_size() - shadow_offset_) y = 0;
        }
    }
    delay_disabled_ = false;
    return true;
}
void Text::print_ascii(TextSurface& s, int& x, int& y, std::string_view text, TextHooks hooks) {
    for (unsigned char c : text) {
        if (!c) break;
        if (c > 32 && c < 127) {
            draw_glyph(s,x,y,data_.ascii(c));
            publish_glyph(s,x,y,hooks);
        }
        x += horizontal_interval_;
        if (x > s.width - horizontal_interval_) {
            x=0; y+=vertical_interval_;
            if (y > s.height - glyph_size() - shadow_offset_) y=0;
        }
    }
}
} // namespace isle::window

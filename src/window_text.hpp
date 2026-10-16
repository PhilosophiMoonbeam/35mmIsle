#ifndef ISLE_WINDOW_TEXT_HPP
#define ISLE_WINDOW_TEXT_HPP
#include "window_resources.hpp"
#include "35mmisle/engine.h"
#include <array>
#include <cstdint>
#include <span>
#include <string_view>

namespace isle::scene { class Services; }
namespace isle::window {
struct Color { std::uint8_t r = 0, g = 0, b = 0, a = 255; };
struct TextSurface {
    std::span<std::uint8_t> image;
    int width = 0, height = 0, bpp = 2, screen_x = 0, screen_y = 0;
};
struct TextHooks {
    void* userdata = nullptr;
    // Wait must suspend the actual native process. No owning automatic objects
    // may be live across this call; Text::print itself holds only trivial locals.
    bool (*wait)(void*, std::uint32_t) = nullptr;
    void (*character)(void*, std::uint16_t) = nullptr;
    void (*button)(void*, int, int, char, int) = nullptr;
    void (*changed)(void*, std::size_t, std::size_t) = nullptr;
};
class Text final {
public:
    explicit Text(FontData data);
    ~Text();
    Text(const Text&) = delete;
    Text& operator=(const Text&) = delete;
    // Installs the actual synchronous source skip-input process on link6,
    // priority0. Services/input and this owner survive until detach joins it.
    bool initialize(scene::Services&);
    void detach() noexcept;
    void set_font(int font) noexcept;
    void set_spacing(int horizontal, int vertical) noexcept;
    void set_shadow_offset(int pixels);
    void set_shadow_color(Color color) noexcept { shadow_ = color; }
    void set_print_delay(int frames) noexcept { delay_ = frames; delay_disabled_ = false; }
    void set_proportional(bool enabled) noexcept { proportional_ = enabled; }
    void set_skip_enabled(bool enabled) noexcept { skip_enabled_ = enabled; }
    void input(std::uint32_t pressed) noexcept;
    void palettes(Color foreground, Color background) noexcept;
    int string_width(std::string_view text) const;
    int glyph_size() const noexcept { return charset_ ? 12 : 8; }
    int horizontal_interval() const noexcept { return horizontal_interval_; }
    int vertical_interval() const noexcept { return vertical_interval_; }
    int draw_glyph(TextSurface&, int x, int y, std::uint16_t character);
    bool print(TextSurface&, int& x, int& y, std::string_view text, TextHooks = {});
    void print_ascii(TextSurface&, int& x, int& y, std::string_view text, TextHooks = {});
private:
    void intervals() noexcept;
    void publish_glyph(const TextSurface&, int x, int y, TextHooks) const;
    bool wrote_glyph_ = false;
    static void poll_input(IsleGameObject*, void*) noexcept;
    scene::Services* services_ = nullptr;
    IsleGameObject* input_object_ = nullptr;
    FontData data_;
    int charset_ = 0, horizontal_spacing_ = -2, vertical_spacing_ = 4, shadow_offset_ = 2;
    int horizontal_interval_ = 12, vertical_interval_ = 12, delay_ = 3;
    bool proportional_ = true, delay_disabled_ = false, skip_enabled_ = false;
    Color foreground_{255,255,255,255}, background_{0,0,0,255}, shadow_{0,0,0,255};
    std::array<std::uint16_t, 16> text_palette_{}, shadow_palette_{}, shadowed_palette_{};
};
} // namespace isle::window
#endif

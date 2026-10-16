#ifndef ISLE_WINDOW_ELEMENT_HPP
#define ISLE_WINDOW_ELEMENT_HPP
#include "window_memory.hpp"
#include "window_text.hpp"
#include "sprite_raster.hpp"
#include "scene_services.hpp"
#include <array>
#include <memory>

namespace isle::window {
enum Flags : std::uint32_t {
    border = 0x001, thick_border = 0x002, alpha_plane = 0x010,
    show_alt = 0x040, use_alt_image = 0x080, scalable = 0x100,
    bits8 = 0x200, bits32 = 0x400, grayscale = 0x800
};
struct ElementDefaults { Color foreground{255,255,255,255}, background{0,0,0,255}; int font = 0, mode = 0; };
class Element;
class Buttons final {
public:
    Buttons(std::shared_ptr<const Resources>, std::shared_ptr<Heap>, scene::Services&);
    ~Buttons();
    void create(int x, int y, char name, int scale);
    void clear() noexcept;
    void erase_rect(int x1, int y1, int x2, int y2) noexcept;
    void show_rect(bool visible, int x1, int y1, int x2, int y2) noexcept;
    void set_animation_interval(char name, float seconds);
private:
    struct Entry;
    static void update(IsleGameObject*, void*);
    void erase(Entry*) noexcept;
    std::shared_ptr<const Resources> resources_;
    std::shared_ptr<Heap> heap_;
    scene::Services& services_;
    Entry* head_ = nullptr;
    std::array<int,11> intervals_{};
};
class ElementList final {
public:
    Element* head() const noexcept { return head_; }
    Element* tail() const noexcept { return tail_; }
    void move_head(Element&);
    void move_tail(Element&);
private:
    void unlink(Element&) noexcept;
    Element* head_ = nullptr;
    Element* tail_ = nullptr;
    std::uint32_t next_id_ = 0;
    friend class Element;
};
class Element final {
public:
    static std::shared_ptr<Element> create(std::shared_ptr<Heap>, ElementList&, int x, int y,
        int width, int height, std::uint32_t flags, std::array<std::uint32_t,2> physical,
        const ElementDefaults& = {});
    ~Element();
    Element(const Element&) = delete;
    Element& operator=(const Element&) = delete;
    std::shared_ptr<sprite::Raster> raster() const noexcept { return raster_; }
    std::span<std::uint8_t> image();
    std::span<const std::uint8_t> image() const;
    const IsleSprite& descriptor() const noexcept { return displayed_; }
    sprite::RasterSource displayed_source() const noexcept { return display_source_; }
    sprite::RasterSource writable_source() const noexcept { return write_source_; }
    int width() const noexcept { return width_; }
    int height() const noexcept { return height_; }
    int stride() const noexcept { return stride_; }
    int bpp() const noexcept { return bpp_; }
    int border_width() const noexcept { return border_; }
    std::uint32_t flags() const noexcept { return flags_; }
    std::uint32_t id() const noexcept { return id_; }
    Element* next() const noexcept { return next_; }
    void set_buttons(Buttons* buttons) noexcept { buttons_ = buttons; }
    Element* previous() const noexcept { return previous_; }
    Color foreground() const noexcept { return foreground_; }
    Color background() const noexcept { return background_; }
    void set_color(bool foreground, Color);
    void set_font(int font) noexcept { font_ = font; }
    void set_text_pos(int x, int y) noexcept;
    int text_x() const noexcept { return text_x_; }
    int text_y() const noexcept { return text_y_; }
    void fill(int x1, int y1, int x2, int y2, Color);
    void fill_with_border(int x1, int y1, int x2, int y2, Color);
    void clear();
    void draw_background();
    void scroll(int amount, unsigned direction);
    void vertical_scroll(int amount, unsigned direction, int x1, int y1, int x2, int y2);
    void draw_image(int x, int y, int width, int height, std::span<const std::uint8_t>);
    bool print(Text&, std::string_view, TextHooks = {});
    void print_ascii(Text&, std::string_view, TextHooks = {});
    bool print_format(Text&, TextHooks, const char* format, ...);
    void swap_images();
    void set_pos(int x, int y);
    void set_alt_pos(int x, int y);
    void set_alt_sprite(std::size_t producer_resource, const IsleSprite&, std::uint64_t generation = 0);
    void show_alt_sprite(bool show);
    void set_visible(bool visible);
    void set_scale(float x, float y);
    void set_sprite_color(Color);
    // Caller registers raster first, then supplies its two producer indices.
    // Attachment is a real native object/sprite, not a rendering callback shim.
    bool attach(scene::Services&, sprite::RasterBinding);
    void detach() noexcept;
private:
    Element() = default;
    void synchronize();
    TextSurface surface();
    void fill_storage(int x1, int y1, int x2, int y2, Color);
    void copy_pixel(std::size_t destination, std::size_t source);
    static bool wait_hook(void*, std::uint32_t);
    static void character_hook(void*, std::uint16_t);
    static void button_hook(void*, int, int, char, int);
    static void changed_hook(void*, std::size_t, std::size_t);
    TextHooks active_hooks_{};
    Buttons* buttons_ = nullptr;
    std::shared_ptr<sprite::Raster> raster_;
    ElementList* list_ = nullptr;
    Element* next_ = nullptr;
    Element* previous_ = nullptr;
    scene::Services* services_ = nullptr;
    scene::SpriteHandle attachment_{};
    sprite::RasterBinding binding_{};
    std::size_t alt_resource_ = 0;
    std::uint64_t alt_generation_ = 0;
    IsleSprite primary_{}, alternate_{}, displayed_{};
    sprite::RasterSource write_source_ = sprite::RasterSource::primary;
    sprite::RasterSource display_source_ = sprite::RasterSource::primary;
    int x_ = 0, y_ = 0, width_ = 0, height_ = 0, border_ = 0, stride_ = 0, bpp_ = 2;
    int alt_x_ = 0, alt_y_ = 0, alt_width_ = 0, alt_height_ = 0;
    int font_ = 0, text_state_ = 0, text_mode_ = 0, text_x_ = 0, text_y_ = 0;
    std::array<std::uint32_t,4> source_state_{};
    std::array<char,400> format_buffer_{};
    std::uint32_t flags_ = 0;
    std::uint32_t id_ = 0;
    Color foreground_{255,255,255,255}, background_{0,0,0,255};
    friend class ElementList;
};
} // namespace isle::window
#endif

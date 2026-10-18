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
// Returning leaves never wait, invoke callbacks, or transfer source control.
// `more` needs another leaf call, not a source update. An event/wait is sticky:
// dispatch it in C after the leaf returns, then acknowledge with resume=true.
// A failed callback/wait must cancel, never acknowledge successful completion.
enum class StepStatus : std::uint8_t { more, event, needs_source_wait, done, error };
enum class StepError : std::uint8_t {
    none, invalid_operation, busy, cancelled, invalid_text, invalid_surface,
    surface_boundary, missing_button, native_failure
};
struct StepResult {
    StepStatus status = StepStatus::done;
    StepError error = StepError::none;
    std::uint32_t updates = 0;
};
enum class TextEncoding : std::uint8_t { source, ascii };
struct TextEvents { bool character = false, button = false, changed = false; };
struct TextEvent {
    enum class Kind : std::uint8_t { none, character, button, changed };
    Kind kind = Kind::none;
    std::uint16_t character = 0;
    int x = 0, y = 0, scale = 0;
    char button = 0;
    std::size_t offset = 0, size = 0;
};
struct TextHooks {
    void* userdata = nullptr;
    // Convenience-driver hooks only. Source veneers use returning operations
    // instead: a source wait/abort must never unwind through a native leaf.
    bool (*wait)(void*, std::uint32_t) = nullptr;
    void (*character)(void*, std::uint16_t) = nullptr;
    void (*button)(void*, int, int, char, int) = nullptr;
    void (*changed)(void*, std::size_t, std::size_t) = nullptr;
};
class Text final {
public:
    // Caller-owned, address-stable record. It and all borrowed pixels, cursor
    // references, encoded bytes and event userdata outlive completion/cancel.
    // One operation per Text; nested/concurrent begins fail without mutation.
    // Cancel preserves already drawn pixels/cursor/settings (no rollback).
    // Destroying/reusing a live record, or stepping after owner death, is invalid.
    class PrintOperation final {
    public:
        PrintOperation() noexcept = default;
        PrintOperation(const PrintOperation&) = delete;
        PrintOperation& operator=(const PrintOperation&) = delete;
        const TextEvent& event() const noexcept { return event_; }
        StepResult result() const noexcept { return result_; }
    private:
        friend class Text;
        enum class Phase : std::uint8_t { next, changed, character, delay, advance };
        Text* owner_ = nullptr;
        TextSurface surface_{};
        int* x_ = nullptr;
        int* y_ = nullptr;
        std::string_view bytes_{};
        TextEvents events_{};
        TextEncoding encoding_ = TextEncoding::source;
        StepResult result_{};
        TextEvent event_{};
        Phase phase_ = Phase::next;
        std::size_t index_ = 0;
        std::uint16_t character_ = 0;
        int position_ = 0, draw_y_ = 0, row_ = 0, shadow_step_ = 0, columns_ = 0, waits_ = 0;
        bool advance_ = false, newline_ = false, escape_ = false;
    };
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
    bool printing() const noexcept { return active_print_ != nullptr; }
    bool skip_enabled() const noexcept { return skip_enabled_; }
    void palettes(Color foreground, Color background) noexcept;
    int string_width(std::string_view text) const;
    int glyph_size() const noexcept { return charset_ ? 12 : 8; }
    int horizontal_interval() const noexcept { return horizontal_interval_; }
    int vertical_interval() const noexcept { return vertical_interval_; }
    int draw_glyph(TextSurface&, int x, int y, std::uint16_t character);
    StepResult begin_print(PrintOperation&, TextSurface, int& x, int& y,
        std::string_view, TextEvents = {}, TextEncoding = TextEncoding::source) noexcept;
    StepResult step_print(PrintOperation&, bool resume = false) noexcept;
    void cancel_print(PrintOperation&) noexcept;
    bool print(TextSurface&, int& x, int& y, std::string_view text, TextHooks = {});
    void print_ascii(TextSurface&, int& x, int& y, std::string_view text, TextHooks = {});
private:
    void intervals() noexcept;
    StepResult step_print_impl(PrintOperation&);
    StepResult finish_print(PrintOperation&, StepError = StepError::none) noexcept;
    bool run_print(TextSurface&, int&, int&, std::string_view, TextHooks, TextEncoding);
    PrintOperation* active_print_ = nullptr;
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

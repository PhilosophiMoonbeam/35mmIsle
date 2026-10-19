#ifndef ISLE_POST_COURSE_RESOURCES_HPP
#define ISLE_POST_COURSE_RESOURCES_HPP
#include "window_resources.hpp"
#include "window_widgets.hpp"
#include <optional>
#include <utility>

namespace isle::post_course {
class Resources final {
public:
    enum class Kind { camera_check, photo_check, album, report, gallery, end_level_8, credits };
    struct Origin {
        std::size_t rom_begin, rom_end;
        std::uint32_t guest_begin;
    };
    static Origin origin(Kind);
    static std::shared_ptr<const Resources> load(const rom::Snapshot&,
        std::shared_ptr<const window::Resources>, Kind);
    Kind kind() const noexcept { return kind_; }
    const std::shared_ptr<const window::Resources>& window() const noexcept { return window_; }
    scene::ResourceView view() const noexcept;
    // Source identities, NOT relocated compositor addresses. Exactly common +
    // the selected initialized overlay; never combine mutually exclusive banks.
    std::span<const sprite::BankMapping> banks() const noexcept { return banks_; }
    std::span<const IsleSprite* const> roots() const noexcept { return roots_; }
    // Raw initialized ROM only. Mutable source state belongs to the consumer.
    std::span<const std::uint8_t> bytes(std::uint32_t guest, std::size_t count) const;
    std::string_view text(std::uint32_t guest) const;
    std::uint32_t word(std::uint32_t guest) const;
    float real(std::uint32_t guest) const;
    std::size_t sprite_index(std::uint32_t identity) const;
    // Exact declared source tables, bounded independently of bank residency.
    std::size_t table_size(std::uint32_t table) const;
    std::uint32_t table_word(std::uint32_t table, std::size_t index) const;
    // Pointer-table NULL remains absent, not an invented empty string.
    std::optional<std::string_view> table_text(std::uint32_t table, std::size_t index) const;
    // Stable native decoding; includes the source BUTTON_NONE terminator.
    std::span<const window::Button> buttons(std::uint32_t table) const;
private:
    struct Buttons { std::uint32_t guest; std::vector<window::Button> native; };
    struct Table { std::uint32_t guest; std::size_t count; bool strings; };
    Kind kind_{};
    std::shared_ptr<const window::Resources> window_;
    std::array<sprite::BankMapping,2> banks_{};
    std::vector<scene::SpriteResource> sprites_;
    std::vector<const IsleSprite*> roots_;
    std::vector<std::pair<std::uint32_t,std::size_t>> indices_;
    std::vector<Buttons> buttons_;
    std::vector<Table> tables_;
    const Table& table(std::uint32_t) const;
    void decode_buttons(std::uint32_t, std::size_t);
};
} // namespace isle::post_course
#endif

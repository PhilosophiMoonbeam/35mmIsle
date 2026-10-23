#include "diagnostic_resources.hpp"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace isle::post_course {
std::shared_ptr<const DiagnosticResources> DiagnosticResources::load(std::shared_ptr<const Resources> owner) {
    if (!owner || owner->kind() != Resources::Kind::printer_diagnostic)
        throw std::invalid_argument("diagnostic resource kind");
    auto out = std::shared_ptr<DiagnosticResources>(new DiagnosticResources);
    out->owner_ = std::move(owner);
    const auto& resources = *out->owner_;
    // Bounded initialized numeric admission; links are bytes at word-spaced
    // addresses, not little-endian or numeric u32 link identifiers.
    constexpr std::array<std::uint8_t,3> expected_links{5,9,3};
    for (std::size_t i = 0; i < out->links_.size(); ++i) {
        const auto bytes = resources.bytes(0x801e54bc + static_cast<std::uint32_t>(i * 4),4);
        if (bytes[0] != expected_links[i] || bytes[1] || bytes[2] || bytes[3])
            throw std::invalid_argument("diagnostic initialized link bytes");
        out->links_[i] = bytes[0];
    }
    out->pending_ = std::bit_cast<std::int32_t>(resources.word(0x801e54c8));
    if (out->pending_ != -1) throw std::invalid_argument("diagnostic initialized pending latch");
    constexpr std::array<std::uint32_t,16> expected_selections{
        0,0,1,1,0,0,1,1,2,2,3,3,2,2,3,3};
    for (std::size_t i = 0; i < out->selections_.size(); ++i) {
        const auto word = resources.word(0x801e54cc + static_cast<std::uint32_t>(i * 4));
        if (word > 3 || word != expected_selections[i])
            throw std::invalid_argument("diagnostic initialized selection order");
        out->selections_[i] = word;
    }
    out->postprocess_initial_ = resources.word(0x801e550c);
    if (out->postprocess_initial_ != 0)
        throw std::invalid_argument("diagnostic initialized postprocess word");
    constexpr std::array<std::uint32_t,7> expected_screen{
        0x806d4000,0x8076a000,0,0,640,480,0x695};
    std::array<std::uint32_t,7> screen{};
    for (std::size_t i = 0; i < screen.size(); ++i) {
        screen[i] = resources.word(0x801e5510 + static_cast<std::uint32_t>(i * 4));
        if (screen[i] != expected_screen[i])
            throw std::invalid_argument("diagnostic initialized screen words");
    }
    out->screen_ = {{screen[0],screen[1],screen[2]},screen[3],screen[4],screen[5],screen[6]};
    // Keep these as numeric words. In particular word 4 remains initialized 0;
    // Source's later 0x2d4000 write belongs to the mutable native lifecycle.
    constexpr std::array<std::uint32_t,35> expected_setup{
        0,0x8000affc,0x8000ada0,0x80400000,0,1,2,0xc800,0xc800,0x400,
        0,0x19000,0x20000,0x4000,0x800a1a50,0x80004860,0,0x500,
        0,0,0,0,0x5c,0,0,0,0,0,0,0x88,0,0x58,0,0x90,0x801dccdc};
    for (std::size_t i = 0; i < out->setup_words_.size(); ++i) {
        const auto word = resources.word(0x801e552c + static_cast<std::uint32_t>(i * 4));
        if (word != expected_setup[i]) throw std::invalid_argument("diagnostic initialized setup words");
        out->setup_words_[i] = word;
    }
    const auto aspect_bits = resources.word(0x801e55c0);
    out->aspect_ = std::bit_cast<float>(aspect_bits);
    if (aspect_bits != 0x3fb201d7 || !std::isfinite(out->aspect_) || out->aspect_ <= 0)
        throw std::invalid_argument("diagnostic initialized aspect");
    // Resource loading has already run the generic bounded Sprite/Bitmap and
    // palette codec. Require real, nonempty decoded payload without assuming
    // any unadmitted sprite dimensions, format, bitmap count, or placement.
    const auto index = resources.sprite_index(0x801e5478);
    out->sprite_ = resources.roots()[index];
    if (!out->sprite_ || out->sprite_->nbitmaps <= 0 || !out->sprite_->bitmap)
        throw std::invalid_argument("diagnostic hydrated sprite");
    bool nonempty = false;
    for (int i = 0; i < out->sprite_->nbitmaps; ++i) {
        const auto& bitmap = out->sprite_->bitmap[i];
        if (bitmap.buf && bitmap.byte_count) nonempty = true;
    }
    if (!nonempty) throw std::invalid_argument("diagnostic empty sprite payload");
    return out;
}
} // namespace isle::post_course

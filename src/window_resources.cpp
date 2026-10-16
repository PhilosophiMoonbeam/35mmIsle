#include "window_resources.hpp"
#include "photo_lighting.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace isle::window {
namespace {
std::uint16_t u16(const std::uint8_t* p) noexcept { return static_cast<std::uint16_t>((p[0] << 8) | p[1]); }
std::uint32_t u32(const std::uint8_t* p) noexcept { return (std::uint32_t(p[0]) << 24) | (std::uint32_t(p[1]) << 16) | (std::uint32_t(p[2]) << 8) | p[3]; }
constexpr std::string_view buttons = "abcdfrzlsmu";
}
void FontData::validate() const {
    if (characters_be.size() != 740 || ascii_be.size() != 188 || widths[0].size() != 370 ||
        widths[1].size() != 370 || glyphs[0].size() != 370 * 32 || glyphs[1].size() != 370 * 72)
        throw std::invalid_argument("window font extent");
    for (unsigned c = 0; c < 2; ++c)
        for (auto width : widths[c]) if (!width || width > (c ? 12 : 8))
            throw std::invalid_argument("window glyph width");
}
std::size_t FontData::index(std::uint16_t ch) const noexcept {
    for (std::size_t i = 0; i < characters_be.size() / 2; ++i)
        if (u16(characters_be.data() + i * 2) == ch) return i;
    return 0;
}
std::uint16_t FontData::ascii(unsigned ch) const noexcept {
    return ch > 32 && ch < 127 ? u16(ascii_be.data() + (ch - 33) * 2) : 0xa1a1;
}
std::span<const std::uint8_t> FontData::glyph(unsigned c, std::uint16_t ch) const noexcept {
    if (c > 1) return {};
    const auto i = index(ch);
    if (u16(characters_be.data() + i * 2) != ch) return {};
    const std::size_t size = c ? 72 : 32;
    return glyphs[c].subspan(i * size, size);
}
std::span<const std::uint8_t> Resources::bytes(unsigned b, std::uint32_t a, std::size_t n) const {
    const auto& bank = banks_.at(b);
    if (a < bank.guest || a - bank.guest > bank.owner->size() || n > bank.owner->size() - (a - bank.guest))
        throw std::out_of_range("window resource address");
    return std::span(*bank.owner).subspan(a - bank.guest, n);
}
std::uint32_t Resources::pointer(unsigned b, std::uint32_t t, std::size_t i) const {
    if (i > 4096) throw std::out_of_range("window pointer table");
    return u32(bytes(b, t + static_cast<std::uint32_t>(i * 4), 4).data());
}
std::string_view Resources::string(unsigned b, std::uint32_t a) const {
    const auto data = bytes(b, a, 1);
    const auto remaining = banks_[b].owner->size() - (a - banks_[b].guest);
    const auto* end = static_cast<const std::uint8_t*>(std::memchr(data.data(), 0, remaining));
    if (!end) throw std::invalid_argument("unterminated window string");
    return {reinterpret_cast<const char*>(data.data()), static_cast<std::size_t>(end - data.data())};
}
const scene::SpriteResource& Resources::sprite(std::uint32_t identity) const {
    const auto it = std::find_if(sprites_.begin(), sprites_.end(), [identity](const auto& s) { return s.identity == identity; });
    if (it == sprites_.end()) throw std::out_of_range("window sprite identity");
    return *it;
}
void Resources::root(unsigned b, std::uint32_t address) {
    const auto result = sprite::hydrate(banks_[b].owner, banks_[b].guest, address);
    if (!result) throw std::invalid_argument("window sprite root admission");
    sprites_.push_back({address, result.asset});
}
void Resources::variant(unsigned b, std::uint32_t prototype, std::uint32_t address) {
    if (std::any_of(sprites_.begin(), sprites_.end(), [address](const auto& s) { return s.identity == address; })) return;
    const auto& source = *sprite(prototype).sprite.view();
    const auto result = sprite::hydrate_bitmaps(banks_[b].owner, banks_[b].guest, source, address,
                                               static_cast<std::size_t>(source.nbitmaps));
    if (!result) throw std::invalid_argument("window bitmap variant admission");
    sprites_.push_back({address, result.asset});
}
std::shared_ptr<const Resources> Resources::load(const rom::Snapshot& snapshot) {
    constexpr std::array<std::uint8_t, 20> admitted{0xed,0xc7,0xc4,0x9c,0xc5,0x68,0xc0,0x45,0xfe,0x48,0xbe,0x0d,0x18,0x01,0x1c,0x30,0xf3,0x93,0xcb,0xaf};
    if (snapshot.identity().sha1 != admitted || snapshot.identity().size != rom::us_size ||
        snapshot.canonical().size() != rom::us_size) throw std::invalid_argument("window ROM identity");
    auto r = std::shared_ptr<Resources>(new Resources);
    // Both pinned SceneSetup callbacks select func_800A1A50 -> D_800AEC18.
    // Reuse its existing admitted public-wire decoder, not a second GBI parser.
    // Only its pre-render list and exact light-DMA union belong to this scene;
    // the independent photo-preview setup is never copied or published here.
    const photo::LightingInputs lighting(snapshot);
    const std::array<photo::LightingSpan, 2> common_render{
        lighting.installation_spans()[0], lighting.pre_render()};
    auto common_owner = std::make_shared<sprite::Bank>(
        common_render[0].bytes.size() + common_render[1].bytes.size());
    std::size_t common_offset = 0;
    for (unsigned i = 0; i < common_render.size(); ++i) {
        const auto& span = common_render[i];
        std::memcpy(common_owner->data() + common_offset, span.bytes.data(), span.bytes.size());
        r->common_render_assets_[i] = {
            span.guest_address - 0x80000000u,
            std::span<const std::uint8_t>(*common_owner).subspan(common_offset, span.bytes.size())};
        common_offset += span.bytes.size();
    }
    r->common_render_owner_ = std::move(common_owner);
    // Exact initialized data spans, never overlay code, BSS or whole-ROM aliases.
    constexpr std::array<std::array<std::size_t, 3>, 3> facts{{
        {0x848bc0,0x87a0b0,0x80375410}, {0xa5e3a0,0xa93460,0x800e2ff0}, {0x8ac010,0x98c330,0x800e67f0}}};
    const auto input = snapshot.canonical();
    for (unsigned i = 0; i < facts.size(); ++i) {
        const auto& f = facts[i];
        if (f[1] > input.size()) throw std::out_of_range("window ROM data extent");
        const auto data = input.subspan(f[0], f[1] - f[0]);
        auto owner = std::make_shared<sprite::Bank>(data.size());
        std::memcpy(owner->data(), data.data(), data.size());
        r->banks_[i] = {std::move(owner), static_cast<std::uint32_t>(f[2]), f[0], f[1]};
    }
    r->font_.characters_be = r->bytes(0, 0x8037eae4, 740);
    r->font_.ascii_be = r->bytes(0, 0x8037edcc, 188);
    for (unsigned c = 0; c < 2; ++c) {
        r->font_.widths[c] = r->bytes(0, 0x8037ee8c + c * 370, 370);
        r->font_.glyphs[c] = r->bytes(0, r->pointer(0, 0x8037ea68, c), c ? 370 * 72 : 370 * 32);
    }
    r->font_.validate();
    constexpr std::array<std::uint32_t, 9> common{0x8037f2a8,0x80380c88,0x80381a38,0x80382ce0,0x803a1360,0x803a1b10,0x803a22c0,0x803a51a0,0x803a6220};
    for (auto address : common) r->root(0, address);
    for (unsigned i = 0; i < 22; ++i) r->variant(0, 0x8037f2a8, r->pointer(0, 0x80380b50, i));
    for (unsigned i = 0; i < 27; ++i) r->variant(0, 0x80380c88, r->pointer(0, 0x803818b0, i));
    for (unsigned i = 0; i < 4; ++i) r->variant(0, 0x80381a38, r->pointer(0, 0x80381d98, i));
    for (unsigned i = 0; i < 33; ++i) r->variant(0, 0x80382ce0, r->pointer(0, 0x803a6564, i));
    for (auto address : {0x803a6210u,0x803a6390u,0x803a6510u}) r->variant(0, 0x803a6220, address);
    r->common_count_ = r->sprites_.size();
    for (auto address : {0x80109880u,0x80117f98u,0x80118068u}) r->root(1, address);
    r->name_count_ = r->sprites_.size() - r->common_count_;
    constexpr std::array<std::uint32_t, 18> oak{0x8010c590,0x8011eea0,0x80122548,0x80128f38,0x80136498,0x80141f48,0x80146918,0x80149a38,0x80190438,0x80197eb0,0x80198fb8,0x8019c2d0,0x801a2900,0x801ad198,0x801b93a8,0x801bed90,0x801bfe98,0x801c31b0};
    for (auto address : oak) r->root(2, address);
    for (unsigned i = 0; i < 7; ++i) r->variant(2, 0x80136498, r->pointer(2, 0x801957f4, i));
    for (auto address : {0x8013ba58u,0x8013dde8u,0x80140178u}) r->variant(2, 0x80136498, address);
    for (auto address : {0x801396e8u,0x80141f38u,0x80142e08u,0x80143cd8u,0x80141068u}) r->variant(2, 0x80141f48, address);
    for (unsigned i = 0; i < 6; ++i) r->variant(2, 0x80146918, r->pointer(2, 0x80195810, i));
    for (unsigned i = 0; i < 10; ++i) r->variant(2, 0x80149a38, r->pointer(2, 0x80195828, i));
    constexpr std::array<std::array<std::uint32_t, 3>, 7> oak_tables{{
        {0x80198fb8,0x801c5404,4}, {0x8019c2d0,0x801c5414,6},
        {0x801a2900,0x801c542c,9}, {0x801ad198,0x801c5450,6},
        {0x801b93a8,0x801c5468,3}, {0x801bfe98,0x801c5474,4},
        {0x801c31b0,0x801c5484,4}}};
    for (const auto& table : oak_tables)
        for (unsigned i = 0; i < table[2]; ++i) r->variant(2, table[0], r->pointer(2, table[1], i));
    for (unsigned i = 0; i < 351; ++i) (void)r->name_character(i);
    for (unsigned i = 0; i < 4; ++i) (void)r->name_end_label(i);
    return r;
}
std::uint32_t Resources::common_pre_render() const noexcept {
    return common_render_assets_[1].address;
}
std::span<const sprite::AssetSpan> Resources::common_render_assets() const noexcept {
    return common_render_assets_;
}
const scene::SpriteResource& Resources::frame(unsigned s, unsigned p) const {
    if (p >= 9) throw std::out_of_range("frame piece");
    return sprite(pointer(0, 0x803818b0, std::min(s, 2u) * 9 + p));
}
std::array<std::uint8_t,4> Resources::frame_background(unsigned style) const {
    const auto& bitmap=frame(style,6).sprite.view()->bitmap[0];
    if (!bitmap.buf || bitmap.byte_count<2) throw std::invalid_argument("frame background pixel extent");
    const auto pixel=u16(bitmap.buf);
    return {static_cast<std::uint8_t>((pixel>>8)&0xf8),static_cast<std::uint8_t>((pixel>>3)&0xf8),
            static_cast<std::uint8_t>((pixel<<2)&0xf8),static_cast<std::uint8_t>((pixel&1)*255)};
}
int Resources::frame_animation_frames() const {
    const auto value=pointer(0,0x80381920,0);
    if (!value || value>240) throw std::invalid_argument("frame animation duration");
    return static_cast<int>(value);
}
const scene::SpriteResource& Resources::focus(unsigned c) const {
    if (c >= 4) throw std::out_of_range("focus corner");
    return sprite(pointer(0, 0x80381d98, c));
}
const scene::SpriteResource& Resources::header_arrow(unsigned index) const {
    constexpr std::array<std::uint32_t,3> identities{0x803a6210,0x803a6390,0x803a6510};
    return sprite(identities.at(index));
}
const scene::SpriteResource& Resources::button(char name, unsigned phase) const {
    const auto i = buttons.find(name);
    if (i == std::string_view::npos || phase > 1) throw std::out_of_range("button image");
    return sprite(pointer(0, 0x80380b50, i * 2 + phase));
}
const scene::SpriteResource& Resources::panel_button(unsigned i) const {
    if (i >= 33) throw std::out_of_range("panel button");
    return sprite(pointer(0, 0x803a6564, i));
}
std::int32_t Resources::button_interval(char name) const {
    const auto i = buttons.find(name);
    if (i == std::string_view::npos) throw std::out_of_range("button interval");
    return static_cast<std::int32_t>(pointer(0, 0x80380ba8, i));
}
std::string_view Resources::name_character(std::size_t i) const {
    if (i >= 351) throw std::out_of_range("name character");
    return string(1, pointer(1, 0x800e2ff0, i));
}
std::string_view Resources::name_end_label(unsigned i) const {
    if (i >= 4) throw std::out_of_range("name end label");
    return string(1, pointer(1, 0x800e356c, i));
}
std::span<const scene::SpriteResource> Resources::sprites(ResourceBank::Group group) const noexcept {
    const auto all = std::span(sprites_);
    switch (group) {
    case ResourceBank::Group::common: return all.first(common_count_);
    case ResourceBank::Group::name: return all.subspan(common_count_, name_count_);
    case ResourceBank::Group::oak: return all.subspan(common_count_ + name_count_);
    }
    return {};
}
const scene::SpriteResource& Resources::table_variant(std::uint32_t table, unsigned i) const {
    constexpr std::array<std::array<std::uint32_t,3>,14> tables{{
        {0x80380b50,22,0},{0x803818b0,27,0},{0x80381d98,4,0},{0x803a6564,33,0},
        {0x801957f4,7,2},{0x80195810,6,2},{0x80195828,10,2},
        {0x801c5404,4,2},{0x801c5414,6,2},{0x801c542c,9,2},{0x801c5450,6,2},
        {0x801c5468,3,2},{0x801c5474,4,2},{0x801c5484,4,2}}};
    for (const auto& fact : tables) if (fact[0] == table && i < fact[1])
        return sprite(pointer(fact[2],table,i));
    throw std::out_of_range("unadmitted window bitmap table/index");
}
std::string_view Resources::text(ResourceBank::Group group, std::uint32_t address) const {
    return string(static_cast<unsigned>(group), address);
}
} // namespace isle::window

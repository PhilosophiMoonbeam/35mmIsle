#include "post_course_resources.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <stdexcept>
#include <string>

namespace isle::post_course {
std::span<const std::uint8_t> Resources::bytes(std::uint32_t guest, std::size_t count) const {
    for (const auto& bank : banks_) {
        if (guest < bank.guest_begin) continue;
        const auto offset = std::size_t(guest - bank.guest_begin);
        if (offset <= bank.owner->size() && count <= bank.owner->size() - offset)
            return std::span<const std::uint8_t>(*bank.owner).subspan(offset, count);
    }
    throw std::out_of_range("post-course initialized-data extent");
}
std::uint32_t Resources::word(std::uint32_t guest) const {
    const auto b = bytes(guest, 4);
    return (std::uint32_t(b[0]) << 24) | (std::uint32_t(b[1]) << 16) |
           (std::uint32_t(b[2]) << 8) | b[3];
}
float Resources::real(std::uint32_t guest) const { return std::bit_cast<float>(word(guest)); }
std::string_view Resources::text(std::uint32_t guest) const {
    if (!guest) throw std::invalid_argument("post-course NULL text pointer");
    for (const auto& bank : banks_) {
        if (guest < bank.guest_begin || std::size_t(guest - bank.guest_begin) >= bank.owner->size()) continue;
        const auto b = std::span<const std::uint8_t>(*bank.owner).subspan(guest - bank.guest_begin);
        const auto end = std::find(b.begin(), b.end(), 0);
        if (end == b.end()) throw std::invalid_argument("post-course unterminated text");
        return {reinterpret_cast<const char*>(b.data()), static_cast<std::size_t>(end - b.begin())};
    }
    throw std::out_of_range("post-course text pointer");
}
const Resources::Table& Resources::table(std::uint32_t guest) const {
    const auto it = std::find_if(tables_.begin(), tables_.end(), [guest](const auto& t) { return t.guest == guest; });
    if (it == tables_.end()) throw std::out_of_range("post-course source table identity");
    return *it;
}
std::size_t Resources::table_size(std::uint32_t guest) const { return table(guest).count; }
std::uint32_t Resources::table_word(std::uint32_t guest, std::size_t index) const {
    if (index >= table(guest).count) throw std::out_of_range("post-course source table index");
    return word(guest + static_cast<std::uint32_t>(index * 4));
}
std::optional<std::string_view> Resources::table_text(std::uint32_t guest, std::size_t index) const {
    if (!table(guest).strings) throw std::invalid_argument("post-course non-string table");
    const auto pointer = table_word(guest, index);
    if (!pointer) return std::nullopt;
    return text(pointer);
}
void Resources::decode_buttons(std::uint32_t guest, std::size_t count) {
    Buttons decoded{guest, {}};
    decoded.native.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const auto id = word(guest + static_cast<std::uint32_t>(i * 8));
        const auto pointer = word(guest + static_cast<std::uint32_t>(i * 8 + 4));
        if (id > 35 || ((id == 34 || id == 35) && pointer) || (id == 35) != (i + 1 == count))
            throw std::invalid_argument("post-course source button table");
        decoded.native.push_back({static_cast<int>(id), pointer ? text(pointer) : std::string_view{}});
    }
    buttons_.push_back(std::move(decoded));
}
std::span<const window::Button> Resources::buttons(std::uint32_t guest) const {
    const auto it = std::find_if(buttons_.begin(), buttons_.end(), [guest](const auto& b) { return b.guest == guest; });
    if (it == buttons_.end()) throw std::out_of_range("post-course button table identity");
    return it->native;
}
std::shared_ptr<const Resources> Resources::load(const rom::Snapshot& snapshot,
        std::shared_ptr<const window::Resources> window, Kind kind) {
    constexpr std::array<std::uint8_t,20> admitted{0xed,0xc7,0xc4,0x9c,0xc5,0x68,0xc0,0x45,0xfe,0x48,0xbe,0x0d,0x18,0x01,0x1c,0x30,0xf3,0x93,0xcb,0xaf};
    if (!window || snapshot.identity().sha1 != admitted || snapshot.identity().size != rom::us_size ||
        snapshot.canonical().size() != rom::us_size)
        throw std::invalid_argument("post-course ROM/window admission");
    if (kind != Kind::camera_check && kind != Kind::photo_check)
        throw std::invalid_argument("post-course overlay kind");
    auto out = std::shared_ptr<Resources>(new Resources);
    out->kind_ = kind;
    out->window_ = std::move(window);
    const auto common = out->window_->banks();
    if (common.empty() || common[0].guest != 0x80375410 || common[0].rom_begin != 0x848bc0 ||
        common[0].rom_end != 0x87a0b0 || !common[0].owner || common[0].owner->size() != 0x314f0)
        throw std::invalid_argument("post-course common initialized bank");
    // Authority: ethteck/pokemonsnap df8815e75f3d79618466e01e0827c6165b93d11c,
    // splat.yaml 10150-10179 and 10212-10235. First initialized .data through
    // next overlay ROM boundary; ends equal the first BSS VRAM (802098F0 /
    // 801F70A0). Neither code nor BSS is copied, interpreted, or installed.
    const bool camera = kind == Kind::camera_check;
    const std::size_t begin = camera ? 0x8817e0 : 0x994e10;
    // Common bank authority: pinned splat.yaml 10105-10154, initialized
    // ROM [848BC0,87A0B0), source VRAM 80375410, shared with Window.
    const std::size_t end = camera ? 0x8a70e0 : 0x9a6b10;
    const std::uint32_t guest = camera ? 0x801e3ff0 : 0x801e53a0;
    const auto input = snapshot.canonical().subspan(begin, end - begin);
    auto owner = std::make_shared<sprite::Bank>(input.size());
    std::memcpy(owner->data(), input.data(), input.size());
    out->banks_ = {sprite::BankMapping{common[0].owner, common[0].guest}, sprite::BankMapping{std::move(owner), guest}};
    for (const auto& resource : out->window_->sprites(window::ResourceBank::Group::common))
        out->sprites_.push_back(resource);
    const auto root = [&out](std::uint32_t address) {
        const auto& bank = out->banks_[1];
        const auto result = sprite::hydrate(bank.owner, bank.guest_begin, address);
        if (!result) throw std::invalid_argument("post-course sprite root " + std::to_string(address) +
            " admission error " + std::to_string(static_cast<unsigned>(result.error)) +
            " at guest " + std::to_string(result.address));
        out->sprites_.push_back({address, result.asset});
    };
    if (camera) {
        // camera_check.h 38-57; 87A330.c 64-174, 506-560, 1003,1087.
        // Every referenced source prototype, including the independently copied
        // thumbnail bitmap. Source changes placement/attrs, never ROM identity.
        constexpr std::array<std::uint32_t,20> roots{
            0x801f0560,0x801fcc58,0x801fce78,0x801fd088,0x801fd408,
            0x801fd4e8,0x801fd5c8,0x801fd6a8,0x801fd9e0,0x801fddb8,
            0x801fe160,0x801fe318,0x801fe448,0x801ff128,0x801ff988,
            0x801ffb08,0x801ffc88,0x802004e8,0x80201520,0x80208900};
        for (auto address : roots) root(address);
        // 87D1A0.c 8-30: each has five entries, including BUTTON_NONE.
        for (auto address : {0x8020896cu,0x80208994u,0x802089bcu}) {
            out->decode_buttons(address, 5);
            out->tables_.push_back({address,10,false});
        }
    } else {
        // 98C330.c 593,638-650,2464-2486: every reached prototype.
        // The unused 801E5840 extern is not a Sprite in the admitted ROM.
        for (auto address : {0x801e5db0u,0x801e5ee0u,0x801f2378u,
                             0x801f26b0u,0x801f2910u,0x801f29f0u,0x801f2ad0u,0x801f3b10u}) root(address);
        // 98C330.c 121-328 and 98D0F0.c 6-230. Bounds follow exact
        // declarations, not a scan for the first NULL (which is real data).
        out->tables_ = {{0x801f3b50,1,false},{0x801f3b54,13,true},{0x801f3b88,168,true},
                        {0x801f3e3c,9,false},{0x801f3e68,183,true},{0x801f4144,1,false},
                        {0x801f4148,2,false},{0x801f4150,12,false},{0x801f4180,5,false}};
        // 993C50.c 23-24,79 and 98D0F0.c 205-218: two adjacent
        // three-entry lists; never reinterpret guest storage as native UIButton.
        out->decode_buttons(0x801f4150,3);
        out->decode_buttons(0x801f4168,3);
        for (const auto& t : out->tables_)
            if (t.strings) for (std::size_t i = 0; i < t.count; ++i) (void)out->table_text(t.guest,i);
    }
    out->roots_.reserve(out->sprites_.size());
    out->indices_.reserve(out->sprites_.size());
    for (std::size_t i = 0; i < out->sprites_.size(); ++i) {
        out->roots_.push_back(out->sprites_[i].sprite.view());
        out->indices_.emplace_back(out->sprites_[i].identity,i);
    }
    std::sort(out->indices_.begin(),out->indices_.end());
    return out;
}
scene::ResourceView Resources::view() const noexcept { return {{128,64,0,4,1024},sprites_}; }
std::size_t Resources::sprite_index(std::uint32_t identity) const {
    const auto it = std::lower_bound(indices_.begin(),indices_.end(),identity,
        [](const auto& entry,std::uint32_t key) { return entry.first < key; });
    if (it == indices_.end() || it->first != identity) throw std::out_of_range("post-course sprite identity");
    return it->second;
}
} // namespace isle::post_course

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
Resources::Origin Resources::origin(Kind kind) {
    // Exact initialized data only; neither overlay code nor BSS is resident.
    constexpr std::array<Origin,8> origins{{
        {0x8817e0,0x8a70e0,0x801e3ff0},
        {0x994e10,0x9a6b10,0x801e53a0},
        {0x9ae430,0x9d3230,0x801e41e0},
        {0x9da580,0x9fa580,0x801e3c10},
        {0x9ff2e0,0xa084b0,0x801e1620},
        {0xa08d10,0xa08e30,0x801dd120},
        {0xa95760,0xaa0650,0x801deba0},
        {0xaa20a0,0xaaa610,0x801dd060}}};
    const auto index=static_cast<std::size_t>(kind);
    if (index>=origins.size()) throw std::invalid_argument("post-course overlay kind");
    return origins[index];
}
std::shared_ptr<const Resources> Resources::load(const rom::Snapshot& snapshot,
        std::shared_ptr<const window::Resources> window, Kind kind) {
    constexpr std::array<std::uint8_t,20> admitted{0xed,0xc7,0xc4,0x9c,0xc5,0x68,0xc0,0x45,0xfe,0x48,0xbe,0x0d,0x18,0x01,0x1c,0x30,0xf3,0x93,0xcb,0xaf};
    if (!window || snapshot.identity().sha1 != admitted || snapshot.identity().size != rom::us_size ||
        snapshot.canonical().size() != rom::us_size)
        throw std::invalid_argument("post-course ROM/window admission");
    const auto selected=origin(kind);
    auto out = std::shared_ptr<Resources>(new Resources);
    out->kind_ = kind;
    out->window_ = std::move(window);
    const auto common = out->window_->banks();
    if (common.empty() || common[0].guest != 0x80375410 || common[0].rom_begin != 0x848bc0 ||
        common[0].rom_end != 0x87a0b0 || !common[0].owner || common[0].owner->size() != 0x314f0)
        throw std::invalid_argument("post-course common initialized bank");
    // Pinned splat initialized boundaries, independently checked by the actual
    // canonical Snapshot/Sprite hydration consumer for all five new menus.
    const bool camera = kind == Kind::camera_check;
    const auto begin=selected.rom_begin;
    const auto end=selected.rom_end;
    const auto guest=selected.guest_begin;
    const auto input = snapshot.canonical().subspan(begin, end - begin);
    auto owner = std::make_shared<sprite::Bank>(input.size());
    std::memcpy(owner->data(), input.data(), input.size());
    out->banks_ = {sprite::BankMapping{common[0].owner, common[0].guest}, sprite::BankMapping{std::move(owner), guest}};
    if(kind!=Kind::printer_diagnostic)
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
    } else if (kind==Kind::photo_check) {
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
    if (kind==Kind::album) {
        for (auto address : {0x801e4518u,0x801ed550u,0x801f2d28u,0x801f2f88u,
            0x801f3108u,0x801f3288u,0x801f3408u,0x801f3588u,0x801f6b50u,
            0x801fb1d0u,0x801ffa10u,0x801ffc80u,0x80200058u,0x80200400u,
            0x802005b8u,0x802006e8u,0x80201728u,0x80208b10u}) root(address);
    } else if (kind==Kind::report) {
        for (auto address : {0x801e3f48u,0x801edd60u,0x801edef0u,0x801e8f70u,
            0x801ee150u,0x801ef7d0u,0x801f0e50u,0x801f2278u,0x801f36a0u,
            0x801f3860u,0x801f3a10u,0x801f3b90u,0x801f4d48u,0x801f5120u,
            0x801f54c8u,0x801f5680u,0x801f57b0u,0x80202ce8u,0x801f67e8u,
            0x801f7820u,0x801f8858u,0x801f9890u,0x801fa8c8u,0x801fb900u}) root(address);
        // Two separate declared six-pointer caption tables, not text pointers.
        out->tables_={{0x80202e8c,6,false},{0x80202ea4,6,false}};
        constexpr std::array<std::pair<std::uint32_t,std::size_t>,8> buttons{{
            {0x80202ec8,6},{0x80202ef8,5},{0x80202f20,5},{0x80202f48,4},
            {0x80202f68,4},{0x80202f88,4},{0x80202fa8,3},{0x80202fc0,7}}};
        for (const auto [address,count] : buttons) out->decode_buttons(address,count);
    } else if (kind==Kind::gallery) {
        for (auto address : {0x801e80e8u,0x801e82a0u,0x801e83d0u,0x801e8890u,
            0x801e8d50u,0x801e8fb0u,0x801ea1b0u}) root(address);
    } else if (kind==Kind::credits) {
        for (auto address : {0x801e6058u,0x801e6430u,0x801e67d8u,
            0x801e6aa8u,0x801e8b50u}) root(address);
    } else if (kind==Kind::printer_diagnostic) {
        // Pinned AA18E0.c declaration and source creation identity.
        root(0x801e5478);
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

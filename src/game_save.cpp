#include "35mmisle/game_save.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <utility>

namespace isle::save {
namespace {
std::uint32_t read32(const std::byte* p) noexcept {
    return (std::to_integer<std::uint32_t>(p[0]) << 24) |
           (std::to_integer<std::uint32_t>(p[1]) << 16) |
           (std::to_integer<std::uint32_t>(p[2]) << 8) |
           std::to_integer<std::uint32_t>(p[3]);
}
void write32(std::byte* p, std::uint32_t value) noexcept {
    for (unsigned i = 0; i < 4; ++i) p[i] = std::byte(value >> (24 - i * 8));
}
constexpr GameError unavailable{GameErrc::unavailable};
constexpr GameError bounds{GameErrc::field_bounds};
// Source flag IDs are not consecutive wire bits (15 is transient RAM state).
constexpr std::array<unsigned, 22> flag_bits{
    15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 3, 2, 1, 0, 0, 31, 30, 29, 28, 27, 4,
};
struct PhotoRegion { std::size_t offset; std::size_t count; std::size_t stride; };
PhotoRegion region(PhotoSet set) noexcept {
    switch (set) {
    case PhotoSet::report: return {0x180, game_report_count, game_photo_bytes};
    case PhotoSet::temporary: return {0xfba0, 4, game_photo_bytes};
    case PhotoSet::album: return {0x10a20, game_album_count, 0x3e0};
    }
    return {};
}
GameError source_state(std::span<const std::byte> bytes) noexcept {
    const auto flags = read32(bytes.data() + 0x64);
    const auto unlocked = std::bit_cast<std::int32_t>(read32(bytes.data() + 0x50));
    if ((flags >> 29) > 6 || ((flags >> 22) & 127) > 100 ||
        ((flags >> 16) & 63) > 50 || unlocked < 0 || unlocked > 6)
        return {GameErrc::source_state};
    return {};
}
} // namespace

struct GameSnapshot::Data {
    alignas(16) std::array<std::byte, image_bytes> bytes;
    std::uint64_t generation = 0;
    bool transient_flag = false;
    Data() noexcept {} // Missing initializes all bytes; load fills them once.
};
ImageView GameSnapshot::image() const noexcept {
    // The session seals before publishing; immutable ownership preserves validity.
    return data_ ? image_ : ImageView{};
}
std::uint64_t GameSnapshot::generation() const noexcept { return data_ ? data_->generation : 0; }
GameValue<bool> GameSnapshot::player_flag(unsigned id) const noexcept {
    if (!data_) return {{}, unavailable};
    if (id >= flag_bits.size()) return {{}, bounds};
    if (id == 15) return {data_->transient_flag};
    return {bool(read32(data_->bytes.data() + (id >= 16 && id <= 20 ? 0x68 : 0x64)) &
                 (std::uint32_t{1} << flag_bits[id]))};
}
GameValue<ScreenOffsets> GameSnapshot::screen_offsets() const noexcept {
    if (!data_) return {{}, unavailable};
    const auto flags = read32(data_->bytes.data() + 0x64);
    return {{static_cast<int>((flags >> 22) & 127) - 76,
             static_cast<int>((flags >> 16) & 63) - 36}};
}
GameValue<unsigned> GameSnapshot::selected_course() const noexcept {
    if (!data_) return {{}, unavailable};
    return {read32(data_->bytes.data() + 0x64) >> 29};
}
GameValue<int> GameSnapshot::unlocked_course() const noexcept {
    if (!data_) return {{}, unavailable};
    return {std::bit_cast<std::int32_t>(read32(data_->bytes.data() + 0x50))};
}
GameValue<std::int32_t> GameSnapshot::course_score(std::size_t index) const noexcept {
    if (!data_) return {{}, unavailable};
    if (index >= 7) return {{}, bounds};
    return {std::bit_cast<std::int32_t>(read32(data_->bytes.data() + 0x34 + index * 4))};
}
GameValue<std::int32_t> GameSnapshot::report_score(std::size_t index) const noexcept {
    if (!data_) return {{}, unavailable};
    if (index >= game_report_count) return {{}, bounds};
    return {std::bit_cast<std::int32_t>(read32(data_->bytes.data() + 0x6c + index * 4))};
}
std::span<const std::byte> GameSnapshot::player_name() const noexcept {
    return data_ ? std::span<const std::byte>(data_->bytes).subspan(0x54, 16) : std::span<const std::byte>{};
}
GameValue<std::span<const std::byte>> GameSnapshot::photo(PhotoSet set, std::size_t index) const noexcept {
    if (!data_) return {{}, unavailable};
    const auto r = region(set);
    if (index >= r.count) return {{}, bounds};
    return {std::span<const std::byte>(data_->bytes).subspan(r.offset + index * r.stride, game_photo_bytes)};
}
GameValue<bool> GameSnapshot::photo_present(PhotoSet set, std::size_t index) const noexcept {
    const auto p = photo(set, index);
    if (!p) return {{}, p.error};
    return {read32(p.value.data() + 4) != 0xffffffffu};
}
GameValue<std::span<const std::byte>> GameSnapshot::album_comment(std::size_t index) const noexcept {
    if (!data_) return {{}, unavailable};
    if (index >= game_album_count) return {{}, bounds};
    return {std::span<const std::byte>(data_->bytes).subspan(0x10a20 + index * 0x3e0 + game_photo_bytes, 64)};
}

struct GameSession::Impl {
    Store store;
    std::shared_ptr<GameSnapshot::Data> data;
    ImageView image{};
    bool initialization_notice = false;
    bool initialization_notice_secondary = false;
    bool course_notice = false;
    bool dirty = false;
    bool writable = false;
    GameSnapshot::Data& mutate() {
        if (data.use_count() != 1) data = std::make_shared<GameSnapshot::Data>(*data);
        dirty = true;
        return *data;
    }
    GameError initialize(std::uint32_t time) noexcept {
        auto& state = *data;
        state.bytes.fill(std::byte{0});
        state.transient_flag = false;
        course_notice = false;
        // Wire-format constants/field facts, never host pointers or raw structs.
        constexpr std::array<unsigned char, 16> version{
            0x48,0x41,0x4c,0x5f,0x53,0x4e,0x41,0x50,0x5f,0x56,0x31,0x2e,0x30,0x2d,0x31,0x00,
        };
        for (std::size_t i = 0; i < version.size(); ++i) state.bytes[0x10 + i] = std::byte(version[i]);
        write32(state.bytes.data() + 0x20, 0x6c);
        write32(state.bytes.data() + 0x24, 0x180);
        write32(state.bytes.data() + 0x28, 0xfba0);
        write32(state.bytes.data() + 0x2c, 0x10a20);
        for (const auto set : {PhotoSet::report, PhotoSet::temporary, PhotoSet::album}) {
            const auto r = region(set);
            for (std::size_t i = 0; i < r.count; ++i)
                write32(state.bytes.data() + r.offset + i * r.stride + 4, 0xffffffffu);
        }
        write32(state.bytes.data() + 0x64, (76u << 22) | (36u << 16) | (1u << 5));
        return seal(time);
    }
    GameError seal(std::uint32_t time) noexcept {
        const auto sealed = seal_image(data->bytes, time);
        if (!sealed) return {GameErrc::canonical_image, sealed.error};
        image = sealed.image;
        dirty = false;
        return {};
    }
};
GameSession::GameSession() noexcept = default;
GameSession::~GameSession() = default;
GameSession::GameSession(GameSession&&) noexcept = default;
GameSession& GameSession::operator=(GameSession&&) noexcept = default;
GameSession::operator bool() const noexcept { return impl_ && impl_->data && impl_->writable; }
GameOpenResult GameSession::open(const std::filesystem::path& directory, std::uint32_t time) {
    GameOpenResult result;
    auto opened = Store::open(directory);
    result.load = opened.load;
    result.store_error = opened.error;
    if (!opened) return result;
    auto owner = std::make_unique<Impl>();
    owner->store = std::move(opened.store);
    if (result.load.state == LoadState::conflict) {
        result.state = GameLoadState::conflict;
    } else if (result.load.state == LoadState::recovery_required) {
        result.state = GameLoadState::recovery_required;
    } else {
        owner->data = std::make_shared<GameSnapshot::Data>();
        auto& data = *owner->data;
        if (result.load.state == LoadState::missing) {
            result.error = owner->initialize(time);
            owner->initialization_notice = true;
            owner->initialization_notice_secondary = true;
            result.state = GameLoadState::initialized_missing;
        } else {
            const auto bytes = result.load.snapshot.image().bytes();
            std::copy(bytes.begin(), bytes.end(), data.bytes.begin());
            data.generation = result.load.snapshot.generation();
            owner->image = inspect_image(data.bytes).image;
            result.error = source_state(data.bytes);
            result.state = GameLoadState::ready;
        }
        if (result.error) {
            owner->data.reset();
            result.state = GameLoadState::invalid_state;
        } else owner->writable = true;
    }
    result.session.impl_ = std::move(owner);
    return result;
}
GameError GameSession::reset(std::uint32_t time) {
    if (!*this) return unavailable;
    if (impl_->data.use_count() != 1) {
        // A reset replaces every byte; preserve snapshots without first copying
        // their payload only to overwrite it with defaults.
        auto replacement = std::make_shared<GameSnapshot::Data>();
        replacement->generation = impl_->data->generation;
        impl_->data = std::move(replacement);
    }
    return impl_->initialize(time);
}
GameValue<GameSnapshot> GameSession::snapshot() {
    if (!*this) return {{}, unavailable};
    if (impl_->dirty) {
        if (const auto error = impl_->seal(read32(impl_->data->bytes.data() + 0x30))) return {{}, error};
    }
    GameSnapshot result;
    result.image_ = impl_->image;
    result.data_ = impl_->data;
    return {std::move(result)};
}
GameError GameSession::set_player_flag(unsigned id, bool value) {
    if (!*this) return unavailable;
    if (id >= flag_bits.size()) return bounds;
    auto& data = impl_->mutate();
    if (id == 15) { data.transient_flag = value; return {}; }
    auto* word = data.bytes.data() + (id >= 16 && id <= 20 ? 0x68 : 0x64);
    const auto mask = std::uint32_t{1} << flag_bits[id];
    write32(word, value ? read32(word) | mask : read32(word) & ~mask);
    return {};
}
GameError GameSession::set_screen_offsets(int horizontal, int vertical) {
    if (!*this) return unavailable;
    auto* word = impl_->mutate().bytes.data() + 0x64;
    const auto x = static_cast<std::uint32_t>(std::clamp(horizontal, -76, 24) + 76);
    const auto y = static_cast<std::uint32_t>(std::clamp(vertical, -36, 14) + 36);
    write32(word, (read32(word) & ~0x1fff0000u) | (x << 22) | (y << 16));
    return {};
}
GameError GameSession::set_selected_course(unsigned course) {
    if (!*this) return unavailable;
    if (course >= 7) return bounds;
    auto* word = impl_->mutate().bytes.data() + 0x64;
    write32(word, (read32(word) & 0x1fffffffu) | (course << 29));
    return {};
}
GameError GameSession::set_unlocked_course(int course) {
    if (!*this) return unavailable;
    const auto clamped = static_cast<std::uint32_t>(std::clamp(course, 0, 6));
    auto& data = impl_->mutate();
    write32(data.bytes.data() + 0x50, clamped);
    if ((read32(data.bytes.data() + 0x64) >> 29) < clamped) impl_->course_notice = true;
    return {};
}
GameError GameSession::set_course_score(std::size_t index, std::int32_t score) {
    if (!*this) return unavailable;
    if (index >= 7) return bounds;
    write32(impl_->mutate().bytes.data() + 0x34 + index * 4, std::bit_cast<std::uint32_t>(score));
    return {};
}
GameError GameSession::set_report_score(std::size_t index, std::int32_t score) {
    if (!*this) return unavailable;
    if (index >= game_report_count) return bounds;
    write32(impl_->mutate().bytes.data() + 0x6c + index * 4, std::bit_cast<std::uint32_t>(score));
    return {};
}
GameError GameSession::set_player_name(std::span<const std::byte> name) {
    if (!*this) return unavailable;
    if (name.size() != 16) return bounds;
    auto& data = impl_->mutate();
    std::copy(name.begin(), name.end(), data.bytes.begin() + 0x54);
    return {};
}
GameError GameSession::remove_photo(PhotoSet set, std::size_t index) {
    if (!*this) return unavailable;
    const auto r = region(set);
    if (index >= r.count) return bounds;
    auto* photo = impl_->mutate().bytes.data() + r.offset + index * r.stride;
    write32(photo + 4, 0xffffffffu);
    if (set == PhotoSet::album) { photo[game_photo_bytes] = std::byte{0}; photo[game_photo_bytes + 1] = std::byte{0}; }
    return {};
}
GameError GameSession::set_album_comment(std::size_t index, std::span<const std::byte> comment) {
    if (!*this) return unavailable;
    if (index >= game_album_count || comment.size() != 64) return bounds;
    auto& data = impl_->mutate();
    std::copy(comment.begin(), comment.end(), data.bytes.begin() + 0x10a20 + index * 0x3e0 + game_photo_bytes);
    return {};
}
CommitResult GameSession::commit(std::uint32_t time) {
    if (!*this) {
        CommitResult result;
        result.error = {StoreErrc::recovery_required, StorePhase::preflight};
        return result;
    }
    auto& data = impl_->mutate();
    if (const auto error = impl_->seal(time)) {
        CommitResult result;
        result.error = {StoreErrc::codec, StorePhase::preflight, 0, error.codec_error};
        return result;
    }
    auto result = impl_->store.commit(impl_->image, data.generation);
    if (result.outcome == CommitOutcome::published) data.generation = result.generation;
    if (result.outcome == CommitOutcome::indeterminate || result.cleanup_error ||
        result.error.code == StoreErrc::session_frozen) impl_->writable = false;
    return result;
}
bool GameSession::consume_initialization_notice() noexcept {
    return impl_ && std::exchange(impl_->initialization_notice, false);
}
bool GameSession::consume_initialization_notice_secondary() noexcept {
    return impl_ && std::exchange(impl_->initialization_notice_secondary, false);
}
bool GameSession::consume_course_notice() noexcept {
    return impl_ && std::exchange(impl_->course_notice, false);
}

} // namespace isle::save

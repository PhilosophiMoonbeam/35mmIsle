#include "application_photos.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <exception>
#include <stdexcept>
#include <limits>
namespace isle::application {
namespace {
enum class IdleTail { complete, tunnel_unwritten };
IslePlayerIdleScript decode_idle(std::span<const std::byte> bytes, std::vector<IslePlayerInput>& inputs,
    IdleTail tail = IdleTail::complete) {
    if (bytes.size()<16) throw std::invalid_argument("idle script header exceeds admitted section");
    const auto u16=[&](std::size_t p) {
        return std::uint32_t(std::to_integer<unsigned>(bytes[p])<<8 |
            std::to_integer<unsigned>(bytes[p+1]));
    };
    const auto u32=[&](std::size_t p) { return u16(p)<<16 | u16(p+2); };
    // Preserve Original updateIdle's dataSize/sizeof(ControllerInput), including
    // Tunnel's final partial record. Its exact-size Original ROM allocation has
    // no releasedButtons/stickX/stickY payload for that record. Like the finite
    // Source-unwritten spawn repair, native playback supplies neutral values for
    // only those three fields; this is not matching N64 residual-heap evidence.
    const auto count=u32(0)/10;
    const auto complete=std::min<std::size_t>(count,(bytes.size()-16)/10);
    const bool unwritten=count>complete;
    if (unwritten && (tail!=IdleTail::tunnel_unwritten || count!=1738 ||
        bytes.size()!=17392 || complete!=1737))
        throw std::invalid_argument("idle inputs exceed admitted original section");
    inputs.resize(count);
    for (std::size_t n=0;n<complete;++n) {
        const auto p=16+n*10;
        inputs[n]={u16(p),u16(p+2),u16(p+4),u16(p+6),
            float(std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(bytes[p+8]))),
            float(std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(bytes[p+9])))};
    }
    if (unwritten) {
        const auto p=16+complete*10;
        inputs.back()={u16(p),u16(p+2),u16(p+4),0,0.0f,0.0f};
    }
    return {inputs.data(),inputs.size(),u32(8),u32(12)};
}
}
Photos::Photos(const rom::Snapshot& snapshot, save::GameSession& save) : save_(save), resources_(snapshot) {
    if (resources_.bind(source_,save)!=ISLE_GAME_OK)
        throw std::invalid_argument("application photo roll requires admitted tables and live save");
    // Distinct admitted assets share application lifetime: State retains the
    // selected Source pointer after its disposable native entry/scene retires.
    beach_idle_=decode_idle(snapshot.canonical().subspan(0xaaa660,0xab1470-0xaaa660),beach_inputs_);
    tunnel_idle_=decode_idle(snapshot.canonical().subspan(0xab1470,0xab5860-0xab1470),tunnel_inputs_,
        IdleTail::tunnel_unwritten);
    roll_revisions_.fill(1);
    if (save_.bind_photo_projection({this, photo_changed, comment_changed, session_reset}))
        throw std::invalid_argument("application photo store requires an exclusive stable save binding");
}
Photos::~Photos() {
    if (source_.owner || replay_borrower_ || working_owners_) std::terminate();
    save_.unbind_photo_projection(this);
}
IsleGameResult Photos::reset() noexcept {
    if (replay_borrower_ || source_.owner || !source_.save || !source_.D_800AE4E4 || !source_.D_800AE284)
        return ISLE_GAME_INVALID_STATE;
    // Complete original 47380.c853-868 is nonwaiting and has no native calls.
    isle_player_photo_func_8009CE00(&source_);
    if (source_.status == ISLE_GAME_OK) {
        selected_count_ = 0;
        for (auto& revision : roll_revisions_) ++revision;
    }
    return source_.status;
}

save::GameError Photos::select(IslePlayerPhotoData* photo, bool for_oak,
    bool source_second_flag, bool for_album) noexcept {
    if (!photo) { selected_count_ = 0; return {}; }
    if (selected_count_ == static_cast<std::int32_t>(selected_.size())) return {};
    // Equality is only a roll-membership check, NOT replay authorization.
    const auto end = std::clamp(count(), std::int32_t{0}, std::int32_t{60});
    for (std::int32_t i = 0; i < end; ++i) {
        if (photo == &source_.D_800B0598[i]) {
            selected_[static_cast<std::size_t>(selected_count_++)] =
                {photo, for_oak, source_second_flag, for_album};
            return {};
        }
    }
    return {save::GameErrc::field_bounds};
}
const IsleSelectedPhoto* Photos::selected(std::int32_t index) const noexcept {
    return index >= 0 && index < selected_count_ ? &selected_[static_cast<std::size_t>(index)] : nullptr;
}
Photos::SavedPhoto* Photos::saved_photo(save::PhotoSet set, std::size_t index) noexcept {
    switch (set) {
    case save::PhotoSet::report: return index < report_.size() ? &report_[index] : nullptr;
    case save::PhotoSet::temporary: return index < temporary_.size() ? &temporary_[index] : nullptr;
    case save::PhotoSet::album: return index < album_.size() ? &album_[index] : nullptr;
    }
    return nullptr;
}
save::GameValue<IslePhotoRecord> Photos::record(IslePhotoStorage storage, std::int32_t index) noexcept {
    constexpr save::GameError bounds{save::GameErrc::field_bounds};
    if (index < 0) return {{}, bounds};
    const auto slot = static_cast<std::size_t>(index);
    if (storage == ISLE_PHOTO_ROLL) {
        if (index >= count() || slot >= roll_revisions_.size()) return {{}, bounds};
        return {{native_source(), &source_.D_800B0598[slot], storage, index, roll_revisions_[slot]}};
    }
    save::PhotoSet set;
    switch (storage) {
    case ISLE_PHOTO_REPORT: set = save::PhotoSet::report; break;
    case ISLE_PHOTO_TEMPORARY: set = save::PhotoSet::temporary; break;
    case ISLE_PHOTO_ALBUM: set = save::PhotoSet::album; break;
    default: return {{}, bounds};
    }
    auto* cached = saved_photo(set, slot);
    if (!cached) return {{}, bounds};
    if (!save_) return {{}, {save::GameErrc::unavailable}};
    if (!cached->loaded) {
        const auto bytes = save_.photo(set, slot);
        if (!bytes) return {{}, bytes.error};
        player::decode_photo(std::span<const std::byte, save::game_photo_bytes>{bytes.value.data(),
            save::game_photo_bytes}, cached->photo);
        cached->loaded = true;
    }
    return {{native_source(), std::bit_cast<std::int32_t>(cached->photo.unk_04) == -1 ? nullptr : &cached->photo,
        storage, index, cached->revision}};
}
std::int32_t Photos::report_index(std::int32_t source_id) noexcept {
    // The admitted signed table and exact original aliases/sign order already
    // have one owner in player_photo_source; do not rebuild a species-1 map.
    const auto index = isle_player_photo_func_8009BB4C(&source_, source_id);
    return index >= 0 && index < static_cast<std::int32_t>(save::game_report_count) ? index : -1;
}
save::GameValue<bool> Photos::report_present(std::int32_t source_id) noexcept {
    const auto index = report_index(source_id);
    if (index < 0) return {false};
    const auto wire = save_.photo(save::PhotoSet::report, static_cast<std::size_t>(index));
    if (!wire) return {{}, wire.error};
    return {wire.value[4] != std::byte{0xff} || wire.value[5] != std::byte{0xff} ||
        wire.value[6] != std::byte{0xff} || wire.value[7] != std::byte{0xff}};
}
save::GameValue<IslePhotoRecord> Photos::identify(const IslePlayerPhotoData* photo) noexcept {
    if (!photo) return {{native_source(), nullptr, ISLE_PHOTO_ROLL, -1, 0}};
    const auto end = std::clamp(count(), std::int32_t{0}, std::int32_t{60});
    for (std::int32_t i = 0; i < end; ++i)
        if (photo == &source_.D_800B0598[i]) return record(ISLE_PHOTO_ROLL, i);
    for (const auto storage : {ISLE_PHOTO_REPORT, ISLE_PHOTO_TEMPORARY, ISLE_PHOTO_ALBUM}) {
        const auto set = storage == ISLE_PHOTO_REPORT ? save::PhotoSet::report :
            storage == ISLE_PHOTO_TEMPORARY ? save::PhotoSet::temporary : save::PhotoSet::album;
        const auto size = storage == ISLE_PHOTO_REPORT ? report_.size() :
            storage == ISLE_PHOTO_TEMPORARY ? temporary_.size() : album_.size();
        for (std::size_t i = 0; i < size; ++i) {
            const auto& cached = *saved_photo(set, i);
            if (cached.loaded && photo == &cached.photo) return record(storage, static_cast<std::int32_t>(i));
        }
    }
    return {{}, {save::GameErrc::field_bounds}};
}
save::GameError Photos::set_photo(save::PhotoSet set, std::size_t index, const IslePlayerPhotoData& photo) {
    if (!saved_photo(set, index)) return {save::GameErrc::field_bounds};
    const auto origin = identify(&photo);
    if (origin && origin.value.storage != ISLE_PHOTO_ROLL) {
        const auto source_set = origin.value.storage == ISLE_PHOTO_REPORT ? save::PhotoSet::report :
            origin.value.storage == ISLE_PHOTO_TEMPORARY ? save::PhotoSet::temporary : save::PhotoSet::album;
        const auto wire = save_.photo(source_set, static_cast<std::size_t>(origin.value.index));
        if (!wire) return wire.error;
        // Saved aliases copy their canonical bytes, including source reserved
        // bytes absent from the semantic codec; never round-trip them through
        // a host-layout struct or overwrite them during scoring.
        return save_.set_photo(set, index, wire.value);
    }
    if (!origin && origin.error.code != save::GameErrc::field_bounds) return origin.error;
    std::array<std::byte, save::game_photo_bytes> wire;
    player::encode_photo(photo, wire);
    return save_.set_photo(set, index, wire);
}
save::GameError Photos::apply_photo_flags(const IslePhotoRecord& record, std::uint8_t flags) {
    if (record.owner != native_source()) return {save::GameErrc::source_state};
    if (replay_borrower_ && !replay_working_ && replay_record_.storage == record.storage &&
        replay_record_.index == record.index) return {save::GameErrc::source_state};
    const auto current = this->record(record.storage, record.index);
    if (!current) return current.error;
    if (!current.value.photo || current.value.photo != record.photo || current.value.revision != record.revision)
        return {save::GameErrc::source_state};
    if (current.value.photo->unk_00_16 == flags) return {};
    if (record.storage == ISLE_PHOTO_ROLL) {
        source_.D_800B0598[record.index].unk_00_16 = flags;
        ++roll_revisions_[static_cast<std::size_t>(record.index)];
        return {};
    }
    const auto set = record.storage == ISLE_PHOTO_REPORT ? save::PhotoSet::report :
        record.storage == ISLE_PHOTO_TEMPORARY ? save::PhotoSet::temporary : save::PhotoSet::album;
    return save_.set_photo_flags(set, static_cast<std::size_t>(record.index), flags);
}
save::GameError Photos::acquire_replay(const IslePhotoRecord& record, const void* borrower,
    const std::uint64_t** revision) noexcept {
    if (revision) *revision = nullptr;
    if (!revision || !borrower || replay_borrower_ || source_.owner ||
        record.owner != native_source() || !record.photo) return {save::GameErrc::source_state};
    const auto current = this->record(record.storage, record.index);
    if (!current) return current.error;
    if (current.value.photo != record.photo || current.value.revision != record.revision)
        return {save::GameErrc::source_state};
    const bool saved = record.storage != ISLE_PHOTO_ROLL;
    const auto set = record.storage == ISLE_PHOTO_REPORT ? save::PhotoSet::report :
        record.storage == ISLE_PHOTO_TEMPORARY ? save::PhotoSet::temporary : save::PhotoSet::album;
    if (const auto error = save_.borrow_replay(borrower, saved, set,
        static_cast<std::size_t>(record.index))) return error;
    replay_record_ = record;
    replay_borrower_ = borrower;
    *revision = saved ? &saved_photo(set, static_cast<std::size_t>(record.index))->revision :
        &roll_revisions_[static_cast<std::size_t>(record.index)];
    return {};
}
save::GameError Photos::release_replay(const void* borrower) noexcept {
    if (!borrower || replay_borrower_ != borrower) return {save::GameErrc::source_state};
    if (const auto error = save_.release_replay(borrower)) return error;
    if (replay_working_) replay_working_->borrower_ = nullptr;
    replay_working_ = nullptr;
    replay_borrower_ = nullptr;
    replay_record_ = {};
    return {};
}
save::GameValue<const std::uint16_t*> Photos::album_comment(std::size_t index) noexcept {
    if (index >= comments_.size()) return {{}, {save::GameErrc::field_bounds}};
    if (!save_) return {{}, {save::GameErrc::unavailable}};
    auto& cached = comments_[index];
    if (!cached.loaded) {
        const auto bytes = save_.album_comment(index);
        if (!bytes) return {{}, bytes.error};
        cached.loaded = true;
        comment_changed(this, index, bytes.value);
    }
    return {cached.text.data()};
}
save::GameError Photos::set_album_comment(std::size_t index, const std::uint16_t* comment) {
    if (index >= comments_.size()) return {save::GameErrc::field_bounds};
    if (!comment) return {};
    std::array<std::byte, 64> wire;
    for (std::size_t i = 0; i < 32; ++i) {
        wire[i * 2] = std::byte(comment[i] >> 8);
        wire[i * 2 + 1] = std::byte(comment[i]);
    }
    return save_.set_album_comment(index, wire);
}
save::GameError Photos::set_album_comment_char(std::size_t index,
    std::size_t character, std::uint16_t value) {
    if (index >= comments_.size() || character >= 32) return {save::GameErrc::field_bounds};
    const auto current = save_.album_comment(index);
    if (!current) return current.error;
    std::array<std::byte, 64> wire;
    std::copy(current.value.begin(), current.value.end(), wire.begin());
    wire[character * 2] = std::byte(value >> 8);
    wire[character * 2 + 1] = std::byte(value);
    return save_.set_album_comment(index, wire);
}
save::GameError Photos::set_working(save::PhotoSet set, std::size_t index,
    const PhotoWorking& working) {
    if (!saved_photo(set, index)) return {save::GameErrc::field_bounds};
    if (&working.photos_ != this || !working.valid_ || source_.owner)
        return {save::GameErrc::source_state};
    return save_.set_photo(set, index, working.wire_);
}
save::GameError Photos::set_album(std::size_t index, const IslePlayerPhotoData* photo,
    const std::uint16_t* comment) {
    if (index >= album_.size()) return {save::GameErrc::field_bounds};
    if (photo) if (const auto error = set_photo(save::PhotoSet::album, index, *photo)) return error;
    return set_album_comment(index, comment);
}
void Photos::photo_changed(void* owner, save::PhotoSet set, std::size_t index,
    std::span<const std::byte> bytes, bool flags_only) noexcept {
    auto& self = *static_cast<Photos*>(owner);
    auto& cached = *self.saved_photo(set, index);
    ++cached.revision;
    if (!cached.loaded) return;
    if (flags_only) cached.photo.unk_00_16 = std::to_integer<std::uint8_t>(bytes[1]);
    else player::decode_photo(std::span<const std::byte, save::game_photo_bytes>{bytes.data(),
        save::game_photo_bytes}, cached.photo);
}
void Photos::comment_changed(void* owner, std::size_t index, std::span<const std::byte> bytes) noexcept {
    auto& cached = static_cast<Photos*>(owner)->comments_[index];
    if (!cached.loaded) return;
    for (std::size_t i = 0; i < cached.text.size(); ++i)
        cached.text[i] = static_cast<std::uint16_t>(
            std::to_integer<std::uint16_t>(bytes[i * 2]) << 8 |
            std::to_integer<std::uint16_t>(bytes[i * 2 + 1]));
}
void Photos::reset_projection() noexcept {
    selected_count_ = 0; // Original 5BF20.c646; do not reset the captured roll.
    for (const auto set : {save::PhotoSet::report, save::PhotoSet::temporary, save::PhotoSet::album}) {
        const auto size = set == save::PhotoSet::report ? report_.size() :
            set == save::PhotoSet::temporary ? temporary_.size() : album_.size();
        for (std::size_t i = 0; i < size; ++i) {
            auto& cached = *saved_photo(set, i);
            ++cached.revision;
            if (cached.loaded) {
                const auto bytes = save_.photo(set, i);
                player::decode_photo(std::span<const std::byte, save::game_photo_bytes>{
                    bytes.value.data(), save::game_photo_bytes}, cached.photo);
            }
        }
    }
    for (auto& comment : comments_) if (comment.loaded) comment.text.fill(0);
}
void Photos::session_reset(void* owner) noexcept {
    static_cast<Photos*>(owner)->reset_projection();
}

PhotoWorking::PhotoWorking(Photos& photos) noexcept
    : handle_(reinterpret_cast<IslePhotoWorking*>(this)), photos_(photos) {
    photo_.unk_04.f32 = -1.0f;
    ++photos_.working_owners_;
}
PhotoWorking::~PhotoWorking() noexcept {
    if (borrower_ || photos_.replay_working_ == this) std::terminate();
    --photos_.working_owners_;
}
bool PhotoWorking::mutable_now() const noexcept {
    return !borrower_ && !photos_.source_.owner &&
        revision_ != std::numeric_limits<std::uint64_t>::max();
}
save::GameError PhotoWorking::copy_record(const IslePhotoRecord& record) noexcept {
    if (!mutable_now() || record.owner != photos_.native_source() || !record.photo)
        return {save::GameErrc::source_state};
    const auto current = photos_.record(record.storage, record.index);
    if (!current) return current.error;
    if (current.value.photo != record.photo || current.value.revision != record.revision)
        return {save::GameErrc::source_state};
    if (record.storage == ISLE_PHOTO_ROLL) {
        // The native captured roll has semantic storage only; this is its
        // canonical Source wire encoding, not a round-trip of a saved record.
        player::encode_photo(*record.photo, wire_);
    } else {
        const auto set = record.storage == ISLE_PHOTO_REPORT ? save::PhotoSet::report :
            record.storage == ISLE_PHOTO_TEMPORARY ? save::PhotoSet::temporary : save::PhotoSet::album;
        const auto bytes = photos_.save_.photo(set, static_cast<std::size_t>(record.index));
        if (!bytes) return bytes.error;
        std::copy(bytes.value.begin(), bytes.value.end(), wire_.begin());
    }
    player::decode_photo(wire_, photo_);
    ++revision_;
    valid_ = true;
    return {};
}
save::GameError PhotoWorking::copy(const PhotoWorking& other) noexcept {
    if (!mutable_now() || &photos_ != &other.photos_ || !other.valid_)
        return {save::GameErrc::source_state};
    if (this != &other) {
        wire_ = other.wire_;
        photo_ = other.photo_;
    }
    ++revision_;
    valid_ = true;
    return {};
}
save::GameError PhotoWorking::clear() noexcept {
    if (!mutable_now()) return {save::GameErrc::source_state};
    // Original writes only its semantic presence sentinel. Old bytes remain
    // private and inaccessible to saved writes/reads until a genuine copy.
    photo_.unk_04.f32 = -1.0f;
    valid_ = false;
    ++revision_;
    return {};
}
IslePhotoWorkingRead PhotoWorking::describe() const noexcept {
    return {source(), photos_.native_source(), valid_ ? &photo_ : nullptr,
        &revision_, revision_, nullptr};
}
save::GameError Photos::acquire_working_replay(PhotoWorking& working,const void* borrower,
    IslePhotoWorkingRead& output) noexcept {
    if (!borrower || !working.valid_ || working.borrower_ || source_.owner || replay_borrower_)
        return {save::GameErrc::source_state};
    if (const auto error = save_.borrow_replay(borrower, false, save::PhotoSet::album, 0))
        return error;
    replay_borrower_ = borrower;
    replay_record_ = {};
    replay_working_ = &working;
    working.borrower_ = borrower;
    output = working.describe();
    output.borrower = borrower;
    return {};
}
save::GameError PhotoWorking::acquire(const void* borrower, IslePhotoWorkingRead& output) noexcept {
    return photos_.acquire_working_replay(*this,borrower,output);
}
bool PhotoWorking::validates(const IslePhotoWorkingRead& read) const noexcept {
    return valid_ && borrower_ && read.owner == source() &&
        read.photos == photos_.native_source() && read.photo == &photo_ &&
        read.revision_address == &revision_ && read.revision == revision_ &&
        read.borrower == borrower_ && photos_.replay_borrower_ == borrower_ &&
        photos_.replay_working_ == this && !photos_.source_.owner;
}
save::GameError PhotoWorking::release(IslePhotoWorkingRead& read) noexcept {
    if (!validates(read)) return {save::GameErrc::source_state};
    if (const auto error = photos_.release_replay(borrower_)) return error;
    read = {};
    return {};
}
}

namespace {
using NativePhotos = isle::application::Photos;
using PhotoSet = isle::save::PhotoSet;
IsleGameResult photo_result(isle::save::GameError error) noexcept {
    if (!error) return ISLE_GAME_OK;
    return error.code == isle::save::GameErrc::field_bounds ? ISLE_GAME_INVALID_ARGUMENT : ISLE_GAME_INVALID_STATE;
}
template<class Operation>
IsleGameResult photo_leaf(IsleApplicationPhotos* owner, Operation&& operation) noexcept {
    if (!owner) return ISLE_GAME_INVALID_ARGUMENT;
    try {
        return operation(*reinterpret_cast<NativePhotos*>(owner));
    } catch (...) {
        return ISLE_GAME_INVALID_STATE;
    }
}
bool photo_slot(std::int32_t index, std::size_t capacity) noexcept {
    return index >= 0 && static_cast<std::size_t>(index) < capacity;
}
// Original source getters return NULL for invalid slots; generic record() is
// checked instead. Keep absence distinct from a native service failure.
IsleGameResult source_photo(NativePhotos& self, IslePhotoStorage storage, std::int32_t index,
    std::size_t capacity, IslePhotoRecord& output) noexcept {
    if (!photo_slot(index, capacity)) {
        output = {self.native_source(), nullptr, storage, index, 0};
        return ISLE_GAME_OK;
    }
    const auto value = self.record(storage, index);
    if (!value) return photo_result(value.error);
    output = value.value;
    return ISLE_GAME_OK;
}
}

extern "C" IsleGameResult isle_application_photos_select(IsleApplicationPhotos* owner,
    IslePlayerPhotoData* photo, int oak, int second, int album) {
    return photo_leaf(owner, [&](NativePhotos& self) {
        return photo_result(self.select(photo, oak != 0, second != 0, album != 0));
    });
}
extern "C" IsleGameResult isle_application_photos_selected_count(IsleApplicationPhotos* owner,
    std::int32_t* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = 0;
    return photo_leaf(owner, [&](NativePhotos& self) {
        *output = self.selected_count();
        return ISLE_GAME_OK;
    });
}
extern "C" IsleGameResult isle_application_photos_selected(IsleApplicationPhotos* owner,
    std::int32_t index, const IsleSelectedPhoto** output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = nullptr;
    return photo_leaf(owner, [&](NativePhotos& self) {
        *output = self.selected(index);
        return ISLE_GAME_OK;
    });
}
extern "C" IsleGameResult isle_application_photos_record(IsleApplicationPhotos* owner,
    IslePhotoStorage storage, std::int32_t index, IslePhotoRecord* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = {};
    return photo_leaf(owner, [&](NativePhotos& self) {
        const auto value = self.record(storage, index);
        if (!value) return photo_result(value.error);
        *output = value.value;
        return ISLE_GAME_OK;
    });
}
extern "C" IsleGameResult isle_application_photos_apply_photo_flags(IsleApplicationPhotos* owner,
    const IslePhotoRecord* record, std::uint8_t flags) {
    if (!record) return ISLE_GAME_INVALID_ARGUMENT;
    return photo_leaf(owner, [&](NativePhotos& self) {
        return photo_result(self.apply_photo_flags(*record, flags));
    });
}
extern "C" IsleGameResult isle_application_photos_report_index(IsleApplicationPhotos* owner,
    std::int32_t id, std::int32_t* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = -1;
    return photo_leaf(owner, [&](NativePhotos& self) {
        *output = self.report_index(id);
        return ISLE_GAME_OK;
    });
}
extern "C" IsleGameResult isle_application_photos_report(IsleApplicationPhotos* owner,
    std::int32_t id, IslePhotoRecord* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = {};
    return photo_leaf(owner, [&](NativePhotos& self) {
        return source_photo(self, ISLE_PHOTO_REPORT, self.report_index(id),
            isle::save::game_report_count, *output);
    });
}
extern "C" IsleGameResult isle_application_photos_set_report(IsleApplicationPhotos* owner,
    std::int32_t id, const IslePlayerPhotoData* photo) {
    return photo_leaf(owner, [&](NativePhotos& self) {
        const auto index = self.report_index(id);
        if (index < 0) return ISLE_GAME_OK;
        if (!photo) return ISLE_GAME_INVALID_ARGUMENT;
        return photo_result(self.set_photo(PhotoSet::report, static_cast<std::size_t>(index), *photo));
    });
}
extern "C" IsleGameResult isle_application_photos_remove_report(IsleApplicationPhotos* owner,
    std::int32_t id) {
    return photo_leaf(owner, [&](NativePhotos& self) {
        const auto index = self.report_index(id);
        return index < 0 ? ISLE_GAME_OK :
            photo_result(self.save_session().remove_photo(PhotoSet::report, static_cast<std::size_t>(index)));
    });
}
extern "C" IsleGameResult isle_application_photos_report_score(IsleApplicationPhotos* owner,
    std::int32_t id, std::int32_t* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = 0;
    return photo_leaf(owner, [&](NativePhotos& self) {
        const auto index = self.report_index(id);
        if (index < 0) return ISLE_GAME_OK;
        const auto value = self.save_session().report_score(static_cast<std::size_t>(index));
        if (!value) return photo_result(value.error);
        *output = value.value;
        return ISLE_GAME_OK;
    });
}
extern "C" IsleGameResult isle_application_photos_set_report_score(IsleApplicationPhotos* owner,
    std::int32_t id, std::int32_t score) {
    return photo_leaf(owner, [&](NativePhotos& self) {
        const auto index = self.report_index(id);
        return index < 0 ? ISLE_GAME_OK :
            photo_result(self.save_session().set_report_score(static_cast<std::size_t>(index), score));
    });
}
extern "C" IsleGameResult isle_application_photos_temporary(IsleApplicationPhotos* owner,
    std::int32_t index, IslePhotoRecord* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = {};
    return photo_leaf(owner, [&](NativePhotos& self) {
        return source_photo(self, ISLE_PHOTO_TEMPORARY, index, isle::save::game_temporary_count, *output);
    });
}
extern "C" IsleGameResult isle_application_photos_set_temporary(IsleApplicationPhotos* owner,
    std::int32_t index, const IslePlayerPhotoData* photo) {
    return photo_leaf(owner, [&](NativePhotos& self) {
        if (!photo_slot(index, isle::save::game_temporary_count) || !photo) return ISLE_GAME_OK;
        return photo_result(self.set_photo(PhotoSet::temporary, static_cast<std::size_t>(index), *photo));
    });
}
extern "C" IsleGameResult isle_application_photos_remove_temporary(IsleApplicationPhotos* owner,
    std::int32_t index) {
    return photo_leaf(owner, [&](NativePhotos& self) {
        return !photo_slot(index, isle::save::game_temporary_count) ? ISLE_GAME_OK :
            photo_result(self.save_session().remove_photo(PhotoSet::temporary, static_cast<std::size_t>(index)));
    });
}
extern "C" IsleGameResult isle_application_photos_album(IsleApplicationPhotos* owner,
    std::int32_t index, IslePhotoRecord* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = {};
    return photo_leaf(owner, [&](NativePhotos& self) {
        return source_photo(self, ISLE_PHOTO_ALBUM, index, isle::save::game_album_count, *output);
    });
}
extern "C" IsleGameResult isle_application_photos_set_album(IsleApplicationPhotos* owner,
    std::int32_t index, const IslePlayerPhotoData* photo, const std::uint16_t* comment) {
    return photo_leaf(owner, [&](NativePhotos& self) {
        if (!photo_slot(index, isle::save::game_album_count)) return ISLE_GAME_OK;
        return photo_result(self.set_album(static_cast<std::size_t>(index), photo, comment));
    });
}
extern "C" IsleGameResult isle_application_photos_remove_album(IsleApplicationPhotos* owner,
    std::int32_t index) {
    return photo_leaf(owner, [&](NativePhotos& self) {
        return !photo_slot(index, isle::save::game_album_count) ? ISLE_GAME_OK :
            photo_result(self.save_session().remove_photo(PhotoSet::album, static_cast<std::size_t>(index)));
    });
}
extern "C" IsleGameResult isle_application_photos_album_comment(IsleApplicationPhotos* owner,
    std::int32_t index, const std::uint16_t** output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = nullptr;
    return photo_leaf(owner, [&](NativePhotos& self) {
        if (!photo_slot(index, isle::save::game_album_count)) return ISLE_GAME_OK;
        const auto value = self.album_comment(static_cast<std::size_t>(index));
        if (!value) return photo_result(value.error);
        *output = value.value;
        return ISLE_GAME_OK;
    });
}
extern "C" IsleGameResult isle_application_photos_set_album_comment(IsleApplicationPhotos* owner,
    std::int32_t index, const std::uint16_t* comment) {
    return photo_leaf(owner, [&](NativePhotos& self) {
        if (!photo_slot(index, isle::save::game_album_count)) return ISLE_GAME_OK;
        if (!comment) return ISLE_GAME_INVALID_ARGUMENT;
        return photo_result(self.set_album_comment(static_cast<std::size_t>(index), comment));
    });
}
extern "C" IsleGameResult isle_application_photos_pending_item_grant(IsleApplicationPhotos* owner,
    int index, int* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = 0;
    return photo_leaf(owner, [&](NativePhotos& self) {
        const auto value = self.save_session().pending_item_grant(index);
        if (!value) return photo_result(value.error);
        *output = value.value ? 1 : 0;
        return ISLE_GAME_OK;
    });
}
extern "C" IsleGameResult isle_application_photos_set_pending_item_grant(IsleApplicationPhotos* owner,
    int index, int value) {
    return photo_leaf(owner, [&](NativePhotos& self) {
        return photo_result(self.save_session().set_pending_item_grant(index, value != 0));
    });
}
extern "C" IsleGameResult isle_application_photos_report_present(IsleApplicationPhotos* owner,
    std::int32_t id, int* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = 0;
    return photo_leaf(owner, [&](NativePhotos& self) {
        const auto present = self.report_present(id);
        if (!present) return photo_result(present.error);
        *output = present.value ? 1 : 0;
        return ISLE_GAME_OK;
    });
}
extern "C" IsleGameResult isle_application_photos_report_count(IsleApplicationPhotos* owner,
    std::int32_t* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = 0;
    return photo_leaf(owner, [&](NativePhotos& self) {
        std::int32_t count = 0;
        for (std::int32_t id = 1; id <= 151; ++id) {
            const auto present = self.report_present(id);
            if (!present) return photo_result(present.error);
            if (present.value) ++count;
        }
        *output = count;
        return ISLE_GAME_OK;
    });
}
extern "C" IsleGameResult isle_application_photos_report_ordinal(IsleApplicationPhotos* owner,
    std::int32_t ordinal, IslePhotoRecord* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = {};
    return photo_leaf(owner, [&](NativePhotos& self) {
        IslePhotoRecord record{self.native_source(), nullptr, ISLE_PHOTO_REPORT, -1, 0};
        if (ordinal >= 0) {
            for (std::int32_t id = 1; id <= 151; ++id) {
                const auto present = self.report_present(id);
                if (!present) return photo_result(present.error);
                if (!present.value || ordinal--) continue;
                const auto value = self.record(ISLE_PHOTO_REPORT, self.report_index(id));
                if (!value) return photo_result(value.error);
                record = value.value;
                break;
            }
        }
        *output = record;
        return ISLE_GAME_OK;
    });
}
extern "C" IsleGameResult isle_application_photos_identify(IsleApplicationPhotos* owner,
    const IslePlayerPhotoData* photo, IslePhotoRecord* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = {};
    return photo_leaf(owner, [&](NativePhotos& self) {
        const auto value = self.identify(photo);
        if (!value) return photo_result(value.error);
        *output = value.value;
        return ISLE_GAME_OK;
    });
}

extern "C" IsleGameResult isle_photo_working_copy_record(IslePhotoWorking* owner,
    const IslePhotoRecord* record) {
    if (!owner || !record) return ISLE_GAME_INVALID_ARGUMENT;
    return photo_result(reinterpret_cast<isle::application::PhotoWorking*>(owner)->copy_record(*record));
}
extern "C" IsleGameResult isle_photo_working_copy(IslePhotoWorking* owner,
    const IslePhotoWorking* other) {
    if (!owner || !other) return ISLE_GAME_INVALID_ARGUMENT;
    return photo_result(reinterpret_cast<isle::application::PhotoWorking*>(owner)->copy(
        *reinterpret_cast<const isle::application::PhotoWorking*>(other)));
}
extern "C" IsleGameResult isle_photo_working_clear(IslePhotoWorking* owner) {
    if (!owner) return ISLE_GAME_INVALID_ARGUMENT;
    return photo_result(reinterpret_cast<isle::application::PhotoWorking*>(owner)->clear());
}
extern "C" IsleGameResult isle_photo_working_photo(const IslePhotoWorking* owner,
    const IslePlayerPhotoData** output) {
    if (output) *output = nullptr;
    if (!owner || !output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = reinterpret_cast<const isle::application::PhotoWorking*>(owner)->photo();
    return ISLE_GAME_OK;
}
extern "C" IsleGameResult isle_photo_working_describe(const IslePhotoWorking* owner,
    IslePhotoWorkingRead* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    if (output->borrower) return ISLE_GAME_INVALID_STATE;
    *output = {};
    if (!owner) return ISLE_GAME_INVALID_ARGUMENT;
    *output = reinterpret_cast<const isle::application::PhotoWorking*>(owner)->describe();
    return ISLE_GAME_OK;
}
extern "C" IsleGameResult isle_photo_working_acquire_read(IslePhotoWorking* owner,
    const void* borrower, IslePhotoWorkingRead* output) {
    if (!owner || !output || !borrower) return ISLE_GAME_INVALID_ARGUMENT;
    // An output already holding a lease must not be cleared/reused. Other
    // failed admissions leave a zero descriptor, never a fabricated borrow.
    if (output->borrower) return ISLE_GAME_INVALID_STATE;
    *output = {};
    return photo_result(reinterpret_cast<isle::application::PhotoWorking*>(owner)->acquire(borrower, *output));
}
extern "C" IsleGameResult isle_photo_working_validate_read(const IslePhotoWorkingRead* read) {
    if (!read || !read->owner) return ISLE_GAME_INVALID_ARGUMENT;
    return reinterpret_cast<const isle::application::PhotoWorking*>(read->owner)->validates(*read) ?
        ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
}
extern "C" IsleGameResult isle_photo_working_release_read(IslePhotoWorkingRead* read) {
    if (!read || !read->owner) return ISLE_GAME_INVALID_ARGUMENT;
    return photo_result(reinterpret_cast<isle::application::PhotoWorking*>(read->owner)->release(*read));
}
extern "C" IsleGameResult isle_application_photos_set_temporary_working(IsleApplicationPhotos* owner,
    int32_t index, const IslePhotoWorking* working) {
    return photo_leaf(owner, [&](NativePhotos& self) {
        if (!photo_slot(index, isle::save::game_temporary_count) || !working) return ISLE_GAME_OK;
        return photo_result(self.set_working(PhotoSet::temporary, static_cast<std::size_t>(index),
            *reinterpret_cast<const isle::application::PhotoWorking*>(working)));
    });
}
extern "C" IsleGameResult isle_application_photos_set_album_working(IsleApplicationPhotos* owner,
    int32_t index, const IslePhotoWorking* working, const uint16_t* comment) {
    return photo_leaf(owner, [&](NativePhotos& self) {
        if (!photo_slot(index, isle::save::game_album_count)) return ISLE_GAME_OK;
        if (working) {
            if (const auto error = self.set_working(PhotoSet::album, static_cast<std::size_t>(index),
                *reinterpret_cast<const isle::application::PhotoWorking*>(working))) return photo_result(error);
        }
        return photo_result(self.set_album_comment(static_cast<std::size_t>(index), comment));
    });
}
extern "C" IsleGameResult isle_application_photos_set_album_comment_char(IsleApplicationPhotos* owner,
    int32_t index, int32_t character, uint16_t value) {
    return photo_leaf(owner, [&](NativePhotos& self) {
        if (!photo_slot(index, isle::save::game_album_count) || !photo_slot(character, 32))
            return ISLE_GAME_INVALID_ARGUMENT;
        return photo_result(self.set_album_comment_char(static_cast<std::size_t>(index),
            static_cast<std::size_t>(character), value));
    });
}

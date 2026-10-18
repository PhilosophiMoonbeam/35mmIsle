// Opt-in PRIVATE_ROM NEW_OWNED_EMPTY_ROOT consumer. The root must already be an
// empty absolute nonsymlink directory; only its newly created profiles are used.
// Authored canonical records below are mutation inputs, NOT original captured
// photos. No Host, renderer, GPU, audio endpoint, physical input, gameplay-phase
// writes, Source-progress simulation, replay, scoring or durable commit occurs.
// Authority: ethteck/pokemonsnap@df8815e75f3d79618466e01e0827c6165b93d11c,
// app_render/47380.c329-364 (603/sign mapping), more_funcs/5BF20.c560-654
// (pending grants/reset). Window assertions consume actual ROM Resources and
// native returning APIs, and advance actual OM processes via isle_game_tick.
#include "application_photos.hpp"
#include "actual_audio_driver.hpp"
#include "scene_storage.hpp"
#include "window_element.hpp"
#include "window_widgets.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace {
namespace app = isle::application;
namespace save = isle::save;
namespace ui = isle::window;
namespace sc = isle::scene;
namespace fs = std::filesystem;
using Wire = std::array<std::byte, save::game_photo_bytes>;
using Comment = std::array<std::byte, 64>;

void require(bool value, const char* reason) {
    if (!value) throw std::runtime_error(reason);
}
void ok(IsleGameResult value, const char* reason) { require(value == ISLE_GAME_OK, reason); }
void ok(save::GameError value, const char* reason) { require(!value, reason); }
template<class T> T value(save::GameValue<T> result, const char* reason) {
    require(bool(result), reason);
    return std::move(result.value);
}
bool equal(std::span<const std::byte> a, std::span<const std::byte> b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}
void put32(Wire& bytes, std::size_t offset, std::uint32_t word) {
    for (unsigned i = 0; i < 4; ++i) bytes[offset + i] = std::byte(word >> (24 - i * 8));
}
Wire authored_photo(std::uint32_t marker, unsigned species, std::uint8_t flags) {
    Wire bytes{};
    bytes[0] = std::byte{5}; // Course2, low source block bit1.
    bytes[1] = std::byte{flags};
    bytes[2] = std::byte{0xa5}; bytes[3] = std::byte{0x5a}; // Reserved, not semantic fields.
    put32(bytes, 4, marker);
    put32(bytes, 8, 0x3f800000); // Camera position x=1.
    put32(bytes, 20, 0x40000000); // Camera target x=2.
    put32(bytes, 32, (species << 19) | (3u << 13) | (7u << 8) | 11u);
    put32(bytes, 36, 0x40400000); // Model time=3.
    put32(bytes, 40, 0x40800000); // Pokemon position x=4.
    bytes[0x140] = std::byte{2}; bytes[0x141] = std::byte{9};
    bytes[0x142] = std::byte{0xc3}; bytes[0x143] = std::byte{0x3c};
    put32(bytes, 0x144, 0x40a00000); // Item position x=5.
    bytes[0x1a0] = std::byte{4}; bytes[0x1a1] = std::byte{8};
    bytes[0x1a2] = std::byte{1}; bytes[0x1a3] = std::byte{6};
    bytes[0x1a4] = std::byte{0xff}; bytes[0x1a5] = std::byte{0xfe}; // Effect x=-2.
    bytes[0x1aa] = std::byte{0x12}; bytes[0x1ab] = std::byte{0x34};
    bytes[0x1ac] = std::byte{19}; bytes[0x1ad] = std::byte{37};
    bytes[0x1ae] = std::byte{73}; bytes[0x1af] = std::byte{255};
    bytes.back() = std::byte{0x69};
    return bytes;
}
Comment authored_comment(std::uint16_t first) {
    Comment bytes{};
    for (unsigned i = 0; i < 32; ++i) {
        const auto character = static_cast<std::uint16_t>(first + i);
        bytes[i * 2] = std::byte(character >> 8);
        bytes[i * 2 + 1] = std::byte(character);
    }
    return bytes;
}
IslePhotoRecord record(app::Photos& photos, IslePhotoStorage storage, int index) {
    IslePhotoRecord result{};
    ok(isle_application_photos_record(photos.native_source(), storage, index, &result),
       "checked native saved-record lookup");
    return result;
}
void check_wire(save::GameSession& session, save::PhotoSet set, std::size_t index,
                const Wire& expected, const char* reason) {
    require(equal(value(session.photo(set, index), "canonical saved-record view"), expected), reason);
}
void check_comment(const std::uint16_t* actual, const Comment& expected) {
    require(actual != nullptr, "stable album comment is available without a photo");
    for (std::size_t i = 0; i < 32; ++i)
        require(actual[i] == (std::to_integer<unsigned>(expected[i * 2]) << 8 |
                              std::to_integer<unsigned>(expected[i * 2 + 1])),
                "stable album comment follows exact numeric BE16 characters");
}
void check_projection(const IslePlayerPhotoData* photo, std::uint32_t marker,
                      unsigned species, std::uint8_t flags) {
    require(photo && photo->levelID == 2 && photo->unk_00_24 == 1 && photo->unk_00_16 == flags &&
            std::bit_cast<std::uint32_t>(photo->unk_04) == marker,
            "native saved projection decodes canonical header fields");
    require(photo->unk_08.x == 1 && photo->unk_14.x == 2 &&
            photo->pokemons[0].pokemonID == static_cast<int>(species) &&
            photo->pokemons[0].unk_00_13 == 3 && photo->pokemons[0].specialID == 7 &&
            photo->pokemons[0].poseID == 11 && photo->pokemons[0].animationTime == 3 &&
            photo->pokemons[0].position.x == 4 && photo->items[0].itemType == 2 &&
            photo->items[0].animationTime == 9 && photo->items[0].pos.x == 5 &&
            photo->effects[0].textureID == 4 && photo->effects[0].posX == -2 &&
            photo->effects[0].size == 0x1234 && photo->effects[0].primColor[2] == 73,
            "native saved projection decodes camera/model/item/effect fields independently of reserved bytes");
}

void photos_contract(const isle::rom::Snapshot& rom, isle::game::State& state,
                     const fs::path& foreign_profile) {
    auto& session = *state.save();
    app::Photos photos(rom, session);
    auto* source = photos.native_source();
    require(photos.count() == 0, "this direct consumer has no captured course photos");
    std::int32_t mapped = -1;
    ok(isle_application_photos_report_index(source, 25, &mapped), "Pikachu report mapping");
    require(mapped == 9, "admitted original Pikachu report slot is9");
    constexpr int report_slot = 9;
    std::int32_t shellder = -1, alias = -1;
    ok(isle_application_photos_report_index(source, 90, &shellder), "Shellder report mapping");
    ok(isle_application_photos_report_index(source, 603, &alias), "603 report mapping");
    require(shellder == 36 && alias == 36, "603 aliases Shellder90 in its actual canonical slot36");
    constexpr std::array sign_ids{1004, 1010, 1028, 1022, 1018, 1035};
    for (unsigned i = 0; i < sign_ids.size(); ++i) {
        ok(isle_application_photos_report_index(source, sign_ids[i], &mapped), "original sign report mapping");
        require(mapped == 63 + static_cast<int>(i), "six signs retain original nonnumeric ordering at slots63..68");
    }
    for (int id : {0, 2, 152, 500, 1000, -1}) {
        ok(isle_application_photos_report_index(source, id, &mapped), "unsupported source ID lookup");
        require(mapped == -1, "unsupported IDs have no invented report slot");
    }

    const auto a = authored_photo(0x123, 25, 0x24);
    const auto b = authored_photo(0x456, 90, 0x72);
    const auto ca = authored_comment(0xa3c1);
    const auto cb = authored_comment(0xa4a1);
    ok(isle_application_photos_set_report_score(source, 25, 4321), "independent source report score write");
    require(!record(photos, ISLE_PHOTO_REPORT, report_slot).photo,
            "writing a score alone does not invent a report photo");
    ok(session.set_photo(save::PhotoSet::report, report_slot, a), "direct canonical report photo write");
    const auto first = record(photos, ISLE_PHOTO_REPORT, report_slot);
    const auto* stable_report = first.photo;
    check_projection(stable_report, 0x123, 25, 0x24);
    check_wire(session, save::PhotoSet::report, report_slot, a, "set_photo copies all canonical/reserved bytes exactly");
    std::int32_t score = 0;
    ok(isle_application_photos_report_score(source, 25, &score), "report score read");
    require(score == 4321, "photo write does not recompute or replace independent score");
    {
        int borrower = 0, foreign_borrower = 0;
        const std::uint64_t* revision = nullptr;
        auto stale = first;
        ++stale.revision;
        require(bool(photos.acquire_replay(stale, &borrower, &revision)) && !revision,
                "stale canonical revision cannot issue a replay read capability");
        ok(photos.acquire_replay(first, &borrower, &revision), "acquire actual canonical replay read lease");
        require(revision && *revision == first.revision,
                "replay revision is stable owner storage, not an IslePhotoRecord temporary");
        const std::uint64_t* rejected_revision = nullptr;
        require(bool(photos.acquire_replay(first, &foreign_borrower, &rejected_revision)) && !rejected_revision,
                "second replay borrower cannot overlap the actual owner lease");
        require(session.set_photo(save::PhotoSet::report, report_slot, b).code == save::GameErrc::source_state &&
                session.remove_photo(save::PhotoSet::report, report_slot).code == save::GameErrc::source_state &&
                session.set_photo_flags(save::PhotoSet::report, report_slot, 0xff).code == save::GameErrc::source_state &&
                photos.apply_photo_flags(first, 0xff).code == save::GameErrc::source_state &&
                session.reset(9).code == save::GameErrc::source_state &&
                photos.reset() == ISLE_GAME_INVALID_STATE,
                "lease rejects same-photo replacement/removal/flags and whole-owner reset");
        ok(session.set_report_score(report_slot, 4321), "independent report score remains writable while replay reads photo");
        ok(session.set_photo(save::PhotoSet::report, 36, b), "unrelated canonical photo remains writable during replay");
        ok(session.set_album_comment(0, ca), "independent comment remains writable during replay");
        std::array<std::byte, 64> empty_comment{};
        ok(session.set_album_comment(0, empty_comment), "restore independently written comment");
        const auto dash = value(value(session.snapshot(), "lease snapshot").player_flag(5), "lease dash flag");
        ok(session.set_player_flag(5, dash), "independent player flag remains writable during replay");
        require(*revision == first.revision && record(photos, ISLE_PHOTO_REPORT, report_slot).photo == stable_report,
                "unrelated COW mutations preserve borrowed projection address and revision");
        check_wire(session, save::PhotoSet::report, report_slot, a, "rejected replay mutations preserve every canonical byte");
        require(bool(photos.release_replay(&foreign_borrower)), "foreign borrower cannot release the canonical read lease");
        ok(photos.release_replay(&borrower), "actual replay owner releases after consumption or cancellation");
        require(bool(photos.release_replay(&borrower)), "duplicate canonical lease release rejects");
    }
    const auto before = value(session.snapshot(), "owning snapshot before replacement");
    ok(session.set_photo(save::PhotoSet::report, report_slot, b), "direct report replacement");
    const auto replaced = record(photos, ISLE_PHOTO_REPORT, report_slot);
    require(replaced.photo == stable_report && replaced.revision != first.revision,
            "direct replacement keeps pointer stable and changes record revision");
    check_projection(stable_report, 0x456, 90, 0x72);
    require(equal(value(before.photo(save::PhotoSet::report, report_slot), "retained snapshot record"), a),
            "COW replacement preserves outstanding snapshot including reserved bytes");
    require(isle_application_photos_apply_photo_flags(source, &first, 0x81) == ISLE_GAME_INVALID_STATE,
            "stale saved revision cannot mutate replacement flags");
    auto flagged = b; flagged[1] = std::byte{0x83};
    ok(isle_application_photos_apply_photo_flags(source, &replaced, 0x83), "native flag-only scoring-side mutation");
    check_wire(session, save::PhotoSet::report, report_slot, flagged,
               "native apply flags changes exactly canonical byte1 and preserves all reserved data");
    require(stable_report->unk_00_16 == 0x83, "retained projection follows native flag-only mutation");
    require(isle_application_photos_apply_photo_flags(source, &replaced, 0x91) == ISLE_GAME_INVALID_STATE,
            "flag write itself invalidates an older revision");
    auto direct_flagged = flagged; direct_flagged[1] = std::byte{0x45};
    ok(session.set_photo_flags(save::PhotoSet::report, report_slot, 0x45), "direct canonical flag-only write");
    require(stable_report->unk_00_16 == 0x45, "direct session flag write updates retained native projection");
    check_wire(session, save::PhotoSet::report, report_slot, direct_flagged,
               "direct flag write also preserves every nonflag byte");
    ok(isle_application_photos_set_report_score(source, 25, -17), "lower independent report score write");
    check_wire(session, save::PhotoSet::report, report_slot, direct_flagged,
               "report score write never changes photo bytes or chooses a maximum");
    require(value(session.report_score(report_slot), "independent negative score") == -17,
            "report scores preserve the caller's exact signed value");

    {
        auto foreign = save::GameSession::open(foreign_profile, 2);
        require(bool(foreign), "independent foreign profile admission");
        app::Photos other(rom, foreign.session);
        ok(foreign.session.set_photo(save::PhotoSet::report, report_slot, a), "foreign canonical report write");
        const auto foreign_record = record(other, ISLE_PHOTO_REPORT, report_slot);
        require(isle_application_photos_apply_photo_flags(source, &foreign_record, 0xff) == ISLE_GAME_INVALID_STATE,
                "real foreign owner's revision/record is rejected");
    }
    check_wire(session, save::PhotoSet::report, report_slot, direct_flagged,
               "rejected stale/foreign writes leave the canonical owner unchanged");
    ok(isle_application_photos_set_report(source, 603, stable_report), "saved alias canonical report copy");
    check_wire(session, save::PhotoSet::report, 36, direct_flagged,
               "saved pointer copy retains reserved bytes rather than semantic re-encoding");
    IslePhotoRecord alias_record{};
    ok(isle_application_photos_report(source, 90, &alias_record), "read copied Shellder record");
    require(alias_record.index == 36 && alias_record.photo && alias_record.photo != stable_report,
            "603 and90 reach the independent Shellder storage slot");
    ok(isle_application_photos_report(source, 603, &alias_record), "read603 alias record");
    require(alias_record.index == 36, "603 getter uses same canonical slot as90");
    for (unsigned i = 0; i < sign_ids.size(); ++i) {
        ok(isle_application_photos_set_report(source, sign_ids[i], stable_report), "write actual sign report slot");
        check_wire(session, save::PhotoSet::report, 63 + i, direct_flagged, "sign write reaches its exact canonical slot");
    }

    std::array<const IslePlayerPhotoData*, 4> temporary{};
    for (int i = 0; i < 4; ++i) {
        ok(isle_application_photos_set_temporary(source, i, stable_report), "write one of FOUR original temporary slots");
        IslePhotoRecord slot{};
        ok(isle_application_photos_temporary(source, i, &slot), "temporary source getter");
        temporary[i] = slot.photo;
        check_wire(session, save::PhotoSet::temporary, i, direct_flagged, "all four temporary copies retain exact canonical bytes");
        require(slot.photo && slot.index == i, "temporary slot is present at its own index");
        const auto next = authored_photo(0x700 + i, 25, 0x24);
        ok(session.set_photo(save::PhotoSet::temporary, i, next), "direct temporary replacement");
        require(record(photos, ISLE_PHOTO_TEMPORARY, i).photo == temporary[i], "temporary pointer survives direct replacement");
        check_projection(temporary[i], 0x700 + i, 25, 0x24);
    }
    for (int index : {-1, 4}) {
        IslePhotoRecord missing{};
        ok(isle_application_photos_set_temporary(source, index, stable_report), "original invalid temporary write is ignored");
        ok(isle_application_photos_temporary(source, index, &missing), "original invalid temporary read");
        require(!missing.photo, "temporary boundary is exactly0..3");
    }
    require(session.set_photo(save::PhotoSet::temporary, 4, a).code == save::GameErrc::field_bounds &&
            session.set_photo(save::PhotoSet::report, 69, a).code == save::GameErrc::field_bounds &&
            session.set_photo(save::PhotoSet::album, 60, a).code == save::GameErrc::field_bounds &&
            session.set_photo(save::PhotoSet::report, report_slot, std::span(a).first(a.size() - 1)).code == save::GameErrc::field_bounds,
            "checked canonical writes reject exact set boundaries and short wire input");

    const std::uint16_t* stable_comment = nullptr;
    ok(isle_application_photos_album_comment(source, 0, &stable_comment), "borrow stable comment for absent album photo");
    ok(session.set_album_comment(0, ca), "direct comment-only write");
    check_comment(stable_comment, ca);
    require(!record(photos, ISLE_PHOTO_ALBUM, 0).photo, "comment write does not create an album photo");
    ok(isle_application_photos_set_album(source, 0, stable_report, nullptr), "album photo with independently omitted comment");
    const auto* stable_album = record(photos, ISLE_PHOTO_ALBUM, 0).photo;
    check_wire(session, save::PhotoSet::album, 0, direct_flagged, "album alias copy preserves exact canonical bytes");
    check_comment(stable_comment, ca);
    ok(session.set_album_comment(0, cb), "direct replacement of existing album comment");
    check_comment(stable_comment, cb);
    require(record(photos, ISLE_PHOTO_ALBUM, 0).photo == stable_album, "comment replacement preserves album photo pointer");
    check_wire(session, save::PhotoSet::album, 0, direct_flagged, "comment replacement leaves album photo bytes unchanged");
    std::array<std::uint16_t, 32> numeric{};
    for (unsigned i = 0; i < numeric.size(); ++i) numeric[i] = static_cast<std::uint16_t>(0xa3c1 + i);
    ok(isle_application_photos_set_album(source, 0, nullptr, numeric.data()), "album comment with independently omitted photo");
    check_comment(stable_comment, ca);
    check_wire(session, save::PhotoSet::album, 0, direct_flagged, "comment-only source mutation preserves existing photo");
    const auto album_snapshot = value(session.snapshot(), "owning album snapshot before remove");
    ok(session.remove_photo(save::PhotoSet::album, 0), "direct album removal");
    auto removed_album = direct_flagged;
    std::fill(removed_album.begin() + 4, removed_album.begin() + 8, std::byte{0xff});
    check_wire(session, save::PhotoSet::album, 0, removed_album, "album removal changes only its source absence marker");
    require(!record(photos, ISLE_PHOTO_ALBUM, 0).photo &&
            std::bit_cast<std::uint32_t>(stable_album->unk_04) == 0xffffffff,
            "retained album pointer observes direct removal without becoming dangling");
    auto removed_comment = ca; removed_comment[0] = removed_comment[1] = std::byte{0};
    check_comment(stable_comment, removed_comment);
    require(equal(value(album_snapshot.photo(save::PhotoSet::album, 0), "snapshot removed photo"), direct_flagged) &&
            equal(value(album_snapshot.album_comment(0), "snapshot removed comment"), ca),
            "removal preserves prior immutable album photo AND comment");
    ok(session.set_photo(save::PhotoSet::album, 0, b), "direct album resurrection");
    require(record(photos, ISLE_PHOTO_ALBUM, 0).photo == stable_album, "album replacement after removal reuses stable projection");
    ok(session.remove_photo(save::PhotoSet::report, report_slot), "direct report removal");
    require(!record(photos, ISLE_PHOTO_REPORT, report_slot).photo &&
            std::bit_cast<std::uint32_t>(stable_report->unk_04) == 0xffffffff &&
            value(session.report_score(report_slot), "score after photo removal") == -17,
            "report removal updates retained projection but does not clear independent score");
    ok(session.remove_photo(save::PhotoSet::temporary, 3), "direct fourth temporary removal");
    require(!record(photos, ISLE_PHOTO_TEMPORARY, 3).photo &&
            std::bit_cast<std::uint32_t>(temporary[3]->unk_04) == 0xffffffff,
            "fourth temporary slot has coherent stable removal");

    IslePlayerPhotoData foreign_photo{};
    require(isle_application_photos_select(source, &foreign_photo, 1, 0, 1) == ISLE_GAME_INVALID_ARGUMENT &&
            isle_application_photos_select(source, &photos.source().D_800B0598[0], 1, 1, 1) == ISLE_GAME_INVALID_ARGUMENT,
            "selection rejects foreign and inactive-roll pointers without fabricating a capture");
    ok(isle_application_photos_select(source, nullptr, 1, 1, 1), "genuine NULL selection reset");
    std::int32_t selected = -1;
    ok(isle_application_photos_selected_count(source, &selected), "selection count after NULL reset");
    const IsleSelectedPhoto* selection = nullptr;
    ok(isle_application_photos_selected(source, 0, &selection), "empty selection lookup");
    require(selected == 0 && !selection && photos.count() == 0, "NULL resets selection without manufacturing roll progress");

    require(!session.consume_course_notice(), "new profile has no course notice before grant mutation");
    for (int index = 0; index < 3; ++index) {
        const auto before_grant = value(session.snapshot(), "owning pre-grant snapshot");
        ok(session.set_pending_item_grant(index, true), "set pending grant without possession");
        require(value(session.pending_item_grant(index), "read pending grant") && session.consume_course_notice() &&
                !session.consume_course_notice(), "unpossessed grant uses consume-once shared course notice");
        const auto after_grant = value(session.snapshot(), "owning post-grant snapshot");
        require(!value(after_grant.player_flag(index), "grant must not grant possession") &&
                equal(value(before_grant.photo(save::PhotoSet::album, 0), "pre-grant album"),
                      value(after_grant.photo(save::PhotoSet::album, 0), "post-grant album")),
                "pending grant is independent RAM state, not possession or a photo mutation");
        ok(session.set_player_flag(index, true), "set actual possession flag independently");
        ok(session.set_pending_item_grant(index, false), "clear pending grant");
        ok(session.set_pending_item_grant(index, true), "set already possessed pending grant");
        require(!session.consume_course_notice(), "already possessed item does not raise a new notice");
        ok(session.set_player_flag(index, false), "restore absent possession independently");
    }
    ok(isle_application_photos_set_pending_item_grant(source, std::numeric_limits<int>::min(), 0), "negative grant index clamps to apple");
    ok(isle_application_photos_set_pending_item_grant(source, std::numeric_limits<int>::max(), 0), "large grant index clamps to flute");
    int pending = -1;
    ok(isle_application_photos_pending_item_grant(source, -17, &pending), "clamped apple read");
    require(pending == 0 && value(session.pending_item_grant(1), "middle grant unaffected") &&
            !value(session.pending_item_grant(2), "upper grant clamped clear"), "grant clamping affects only endpoints0/2");
    ok(session.set_unlocked_course(100), "original upper course clamp");
    require(value(value(session.snapshot(), "clamped course snapshot").unlocked_course(), "clamped course") == 6 &&
            session.consume_course_notice(), "unlock progress uses the SAME course-notice latch");
    ok(session.set_screen_offsets(-1000, 1000), "original signed screen clamps");
    const auto offsets = value(value(session.snapshot(), "clamped screen snapshot").screen_offsets(), "screen offsets");
    require(offsets.horizontal == -76 && offsets.vertical == 14, "screen offsets retain original clamp endpoints");
    const auto pre_reset = record(photos, ISLE_PHOTO_ALBUM, 0);
    ok(session.reset(9), "direct RAM-only source reset");
    require(!record(photos, ISLE_PHOTO_ALBUM, 0).photo &&
            !record(photos, ISLE_PHOTO_REPORT, report_slot).photo &&
            std::bit_cast<std::uint32_t>(stable_album->unk_04) == 0xffffffff &&
            std::bit_cast<std::uint32_t>(stable_report->unk_04) == 0xffffffff,
            "direct reset updates all retained saved-photo projections in place");
    check_comment(stable_comment, Comment{});
    for (int index = 0; index < 3; ++index)
        require(!value(session.pending_item_grant(index), "reset pending grant"), "source reset clears every pending grant");
    for (int i = 0; i < 4; ++i) {
        require(!record(photos, ISLE_PHOTO_TEMPORARY, i).photo &&
                std::bit_cast<std::uint32_t>(temporary[i]->unk_04) == 0xffffffff,
                "reset clears all four stable temporary projections");
        ok(session.set_photo(save::PhotoSet::temporary, i, a), "temporary write after reset");
        require(record(photos, ISLE_PHOTO_TEMPORARY, i).photo == temporary[i], "temporary address survives reset and refill");
    }
    require(!session.consume_course_notice() && value(session.report_score(report_slot), "reset report score") == 0 &&
            isle_application_photos_apply_photo_flags(source, &pre_reset, 1) == ISLE_GAME_INVALID_STATE,
            "reset clears notice/score and rejects pre-reset revision");
    require(equal(value(before.photo(save::PhotoSet::report, report_slot), "snapshot retained across reset"), a),
            "immutable pre-replacement snapshot survives subsequent flags/removal/reset");
    ok(session.set_photo(save::PhotoSet::report, report_slot, a), "refill stable report after reset");
    ok(session.set_photo(save::PhotoSet::album, 0, a), "refill stable album after reset");
    ok(session.set_album_comment(0, cb), "refill stable comment after reset");
    require(record(photos, ISLE_PHOTO_REPORT, report_slot).photo == stable_report &&
            record(photos, ISLE_PHOTO_ALBUM, 0).photo == stable_album,
            "report and album addresses survive reset/refill");
    const std::uint16_t* comment_again = nullptr;
    ok(isle_application_photos_album_comment(source, 0, &comment_again), "comment address after reset/refill");
    require(comment_again == stable_comment, "album comment address survives direct mutation/remove/reset");
    check_comment(stable_comment, cb);
}

void tick(sc::Storage& storage, sc::Services& services, unsigned count = 1) {
    for (unsigned i = 0; i < count; ++i) {
        ok(isle_game_tick(storage.game()), "actual native OM update");
        require(!services.failed(), services.error());
    }
}
struct NativeDraws {
    std::array<sc::SpriteDraw, 64> entries{};
    std::size_t count = 0;
    auto begin() const noexcept { return entries.begin(); }
    auto end() const noexcept { return entries.begin() + count; }
    std::size_t size() const noexcept { return count; }
    const sc::SpriteDraw& operator[](std::size_t index) const noexcept { return entries[index]; }
};
NativeDraws draws(sc::Services& services) {
    // These Window fixtures inspect each owner's actual first attachment,
    // including hidden furniture. GPU draw preparation intentionally prunes
    // hidden hierarchies and is not a native-state inspection interface.
    NativeDraws result;
    std::array<sc::RenderOrderEntry,64> owners{};
    const auto resources=services.sprite_resources();
    for (unsigned link=0;link!=32;++link) {
        std::size_t count=0;
        require(services.render_order(static_cast<std::uint8_t>(link),owners,count),
                "native Window owner observation capacity");
        for (std::size_t i=0;i<count;++i) {
            IsleGameSprite* attachment=nullptr;
            const auto found=isle_game_object_sprite(services.game,owners[i].object,&attachment);
            if (found==ISLE_GAME_INVALID_ARGUMENT) continue;
            ok(found,"borrow actual Window root attachment");
            IsleSprite* view=nullptr;
            ok(isle_game_sprite_view(services.game,attachment,&view),"borrow actual Window local state");
            const auto resource=std::find_if(resources.begin(),resources.end(),
                [=](const auto& value){return value.sprite.view()->bitmap==view->bitmap;});
            const auto index=resource==resources.end()?resources.size():static_cast<std::size_t>(resource-resources.begin());
            require(result.count<result.entries.size(),"native Window root observation capacity");
            result.entries[result.count++]={{index,view,0},owners[i].object,owners[i].render};
        }
    }
    return result;
}
IsleSprite sprite(sc::Services& services, std::uint32_t identity) {
    const auto resources = services.sprite_resources();
    const auto resource = std::find_if(resources.begin(), resources.end(), [=](const auto& r) { return r.identity == identity; });
    require(resource != resources.end(), "inspected sprite has actual admitted resource identity");
    const auto index = static_cast<std::size_t>(resource - resources.begin());
    const auto current = draws(services);
    const auto found = std::find_if(current.begin(), current.end(), [=](const auto& draw) { return draw.draw.resource == index; });
    require(found != current.end() && found->draw.view && found->draw.view->bitmap, "actual native sprite is attached with ROM bitmap");
    return *found->draw.view;
}
bool same_visual(const IsleSprite& a, const IsleSprite& b) {
    return a.x == b.x && a.y == b.y && a.scalex == b.scalex && a.scaley == b.scaley &&
           a.attr == b.attr && a.alpha == b.alpha && a.bitmap == b.bitmap;
}
struct TextPrint {
    ui::Text& text; ui::Text::PrintOperation operation;
    ~TextPrint() { text.cancel_print(operation); }
};
struct FrameFade {
    ui::Frame& frame; ui::Frame::FadeOperation operation;
    ~FrameFade() { frame.cancel_fade(operation); }
};
struct LayoutStep {
    ui::Layout& layout; ui::Layout::Operation operation;
    ~LayoutStep() { layout.cancel(operation); }
};

void text_contract(sc::Storage& storage, sc::Services& services,
                   const std::shared_ptr<const ui::Resources>& resources) {
    ui::Text text(resources->font());
    text.set_font(8); text.set_shadow_offset(0); text.set_proportional(true); text.set_print_delay(2);
    std::array<std::uint8_t, 32 * 16 * 2> pixels{};
    ui::TextSurface surface{pixels, 32, 16, 2, 40, 50};
    int x = 0, y = 0;
    TextPrint print{text};
    const auto glyph_text = resources->name_character(255);
    auto result = text.begin_print(print.operation, surface, x, y, glyph_text, {true, false, true});
    unsigned changed = 0, characters = 0, waits = 0;
    for (unsigned leaves = 0; result.status != ui::StepStatus::done; ++leaves) {
        require(leaves < 128 && result.status != ui::StepStatus::error, "bounded returning glyph print completes");
        if (result.status == ui::StepStatus::event) {
            const auto event = print.operation.event();
            const auto retained = pixels;
            require(text.step_print(print.operation).status == ui::StepStatus::event && pixels == retained && x == 0,
                    "unacknowledged text event neither redraws nor advances cursor");
            if (event.kind == ui::TextEvent::Kind::changed) {
                require(characters == 0 && waits == 0 && event.offset == changed * 64 && event.size == 16,
                        "drawn glyph rows are published before character/delay events");
                ++changed;
            } else {
                require(event.kind == ui::TextEvent::Kind::character && event.character == 0xa3c1 && changed == 8,
                        "character event follows all eight real FONT8 rows");
                ++characters;
            }
            result = text.step_print(print.operation, true);
        } else if (result.status == ui::StepStatus::needs_source_wait) {
            require(characters == 1 && result.updates == 1 && x == 0, "glyph delay precedes cursor advancement");
            require(text.step_print(print.operation).status == ui::StepStatus::needs_source_wait,
                    "text wait requires explicit acknowledgement");
            tick(storage, services, result.updates); ++waits;
            result = text.step_print(print.operation, true);
        } else result = text.step_print(print.operation);
    }
    const auto glyph_width = resources->font().widths[0][resources->font().index(0xa3c1)];
    require(changed == 8 && characters == 1 && waits == 2 && x == glyph_width + 1 && y == 0,
            "one glyph consumes exact events/two real update waits and its admitted proportional advance");
    const auto glyph = resources->font().glyph(0, 0xa3c1);
    require(glyph.size() == 32, "admitted FONT8 glyph has exact32-byte extent");
    for (unsigned row = 0; row < 16; ++row) for (unsigned col = 0; col < 32; ++col) {
        unsigned expected = 0;
        if (row < 8 && col < 8) {
            const auto packed = glyph[row * 4 + col / 2];
            const unsigned coverage = col & 1 ? packed >> 4 : packed & 15;
            const auto channel = coverage * 17;
            if (coverage) expected = ((channel << 8) & 0xf800) | ((channel << 3) & 0x7c0) | ((channel >> 2) & 0x3e) | 1;
        }
        const auto at = (row * 32 + col) * 2;
        require(((unsigned(pixels[at]) << 8) | pixels[at + 1]) == expected,
                "returning Text draws actual ROM low-nibble-first glyph, with untouched surrounding pixels");
    }
    text.set_print_delay(0); x = 3; y = 4;
    auto heap = std::make_shared<ui::Heap>(0x50000);
    ui::Buttons buttons(resources, heap, services);
    TextPrint button{text};
    result = text.begin_print(button.operation, surface, x, y, "\\a", {false, true, false});
    require(result.status == ui::StepStatus::more, "begin returning button print");
    result = text.step_print(button.operation);
    require(result.status == ui::StepStatus::event && button.operation.event().kind == ui::TextEvent::Kind::button,
            "button event returns before native consumer dispatch");
    const auto event = button.operation.event();
    require(event.button == 'a' && event.x == 43 && event.y == 54 && event.scale == 8,
            "button event retains source screen position and glyph scale");
    require(services.sprite_count() == 0, "returning Text did not invoke a hidden button callback");
    buttons.create(event.x, event.y, event.button, event.scale); // Actual native consumer AFTER leaf return.
    const auto initial_button = draws(services);
    require(initial_button.size() == 1 && initial_button[0].draw.view->x == 43 &&
            initial_button[0].draw.view->y == 54 && initial_button[0].draw.view->bitmap,
            "deferred event consumer creates actual admitted native button sprite");
    const auto count = services.sprite_count();
    require(text.step_print(button.operation).status == ui::StepStatus::event && services.sprite_count() == count,
            "unacknowledged button event does not duplicate native sprite");
    text.cancel_print(button.operation);
    result = text.step_print(button.operation, true);
    require(result.status == ui::StepStatus::error && result.error == ui::StepError::cancelled && x == 3 && y == 4,
            "cancelled button event cannot be acknowledged as print success or advance its cursor");
    tick(storage, services);
    const auto animated_button = draws(services);
    require(animated_button.size() == 1 && animated_button[0].draw.view->bitmap == resources->button('a', 1).sprite.view()->bitmap,
            "real OM button process switches to actual second ROM bitmap after cancellation of print");
    tick(storage, services, 3);
    require(services.sprite_count() == 1, "cancel leaves already-consumed native button alive until its owner clears it");
    buttons.clear();
    require(services.sprite_count() == 0, "native button owner retires its sprite");

    x = y = 0;
    pixels.fill(0);
    text.set_print_delay(2);
    TextPrint cancelled{text};
    result = text.begin_print(cancelled.operation, surface, x, y, glyph_text, {true, false, true});
    result = text.step_print(cancelled.operation);
    while (result.status == ui::StepStatus::more) result = text.step_print(cancelled.operation);
    require(result.status == ui::StepStatus::event, "cancel text after genuine partial pixel draw");
    const auto partial = pixels;
    require(std::any_of(partial.begin(), partial.end(), [](auto byte) { return byte != 0; }),
            "cancel point retains newly drawn actual glyph pixels, not an empty fixture");
    text.cancel_print(cancelled.operation);
    tick(storage, services, 3);
    result = text.step_print(cancelled.operation);
    require(result.status == ui::StepStatus::error && result.error == ui::StepError::cancelled && pixels == partial && x == 0,
            "text cancellation retains partial pixels but never finishes cursor/delay work");
}

unsigned drive_frame(ui::Frame& frame, ui::Frame::FadeOperation& operation, ui::StepResult result,
                     sc::Storage& storage, sc::Services& services) {
    unsigned waits = 0;
    for (unsigned leaves = 0; result.status != ui::StepStatus::done; ++leaves) {
        require(leaves < 512 && result.status != ui::StepStatus::error, "bounded returning Frame completion");
        if (result.status == ui::StepStatus::needs_source_wait) {
            require(result.updates == 1, "Frame asks for one actual update per inclusive expansion sample");
            const auto before = draws(services);
            std::array<IsleSprite, 9> retained{};
            require(before.size() == retained.size(), "Frame owns exactly nine native pieces during its wait");
            for (unsigned i = 0; i < before.size(); ++i) retained[i] = *before[i].draw.view;
            require(frame.step_fade(operation).status == ui::StepStatus::needs_source_wait,
                    "Frame wait polling does not acknowledge its update");
            const auto polled = draws(services);
            require(polled.size() == retained.size(), "unacknowledged Frame wait does not add or remove native pieces");
            for (unsigned i = 0; i < polled.size(); ++i)
                require(same_visual(*polled[i].draw.view, retained[i]), "pending Frame wait preserves actual native geometry");
            tick(storage, services, result.updates); ++waits;
            result = frame.step_fade(operation, true);
        } else result = frame.step_fade(operation);
    }
    return waits;
}
void frame_contract(sc::Storage& storage, sc::Services& services, const ui::Resources& resources) {
    ui::Frame frame(services, resources);
    require(frame.create(), "create actual nine-sprite native Frame");
    FrameFade fade{frame};
    auto result = frame.begin_fade_in(fade.operation, 3);
    result = frame.step_fade(fade.operation);
    result = frame.step_fade(fade.operation);
    require(result.status == ui::StepStatus::needs_source_wait, "Frame expansion returns its initial wait");
    auto corner = sprite(services, resources.frame(2, 2).identity);
    require(corner.x == 96 && corner.y == 165 && corner.alpha == 0,
            "style3 starts at source anchor with zero opacity");
    auto opposite = sprite(services, resources.frame(2, 3).identity);
    require(opposite.x == 112 && opposite.y == 181, "initial style3 expansion is exactly24x24");
    const auto waits = drive_frame(frame, fade.operation, result, storage, services);
    require(waits == static_cast<unsigned>(resources.frame_animation_frames() + 1),
            "Frame includes both endpoint samples, not merely frame-count waits");
    corner = sprite(services, resources.frame(2, 2).identity);
    opposite = sprite(services, resources.frame(2, 3).identity);
    const auto background = sprite(services, resources.frame(2, 6).identity);
    require(corner.x == 96 && corner.y == 165 && opposite.x == 296 && opposite.y == 217 &&
            corner.alpha == 255 && !(corner.attr & ISLE_SP_HIDDEN) && (background.attr & ISLE_SP_HIDDEN) &&
            background.x == 103 && background.y == 172 && background.scalex == 24.25f && background.scaley == 5.75f,
            "completed Frame has exact208x60 native geometry, visible border and hidden background");
    FrameFade cancelled{frame};
    result = frame.begin_fade_out(cancelled.operation);
    result = frame.step_fade(cancelled.operation);
    result = frame.step_fade(cancelled.operation);
    require(result.status == ui::StepStatus::needs_source_wait, "begin genuine returning Frame collapse");
    tick(storage, services);
    result = frame.step_fade(cancelled.operation, true);
    require(result.status == ui::StepStatus::needs_source_wait, "partial collapse has another real wait");
    const auto partial = sprite(services, resources.frame(2, 3).identity);
    frame.cancel_fade(cancelled.operation);
    tick(storage, services, 4);
    require(same_visual(partial, sprite(services, resources.frame(2, 3).identity)),
            "cancelled Frame retains partial native geometry instead of finishing collapse");
    result = frame.step_fade(cancelled.operation, true);
    require(result.status == ui::StepStatus::error && result.error == ui::StepError::cancelled,
            "cancelled Frame wait cannot acknowledge successful collapse");
    FrameFade finish{frame};
    require(drive_frame(frame, finish.operation, frame.begin_fade_out(finish.operation), storage, services) == waits,
            "Frame owner can start a fresh complete operation after cancellation");
    for (const auto& draw : draws(services)) require(draw.draw.view->attr & ISLE_SP_HIDDEN, "completed collapse hides every native Frame piece");
    frame.retire();
    require(services.sprite_count() == 0, "Frame retires all native sprites before Services");
}

unsigned drive_layout(ui::Layout& layout, ui::Layout::Operation& operation, ui::StepResult result,
                      sc::Storage& storage, sc::Services& services) {
    unsigned waits = 0;
    for (unsigned leaves = 0; result.status != ui::StepStatus::done; ++leaves) {
        require(leaves < 2048 && result.status != ui::StepStatus::error, "bounded returning Layout completion");
        if (result.status == ui::StepStatus::needs_source_wait) {
            require(result.updates == 1 && layout.step(operation).status == ui::StepStatus::needs_source_wait,
                    "Layout transition waits remain pending until explicitly acknowledged");
            tick(storage, services, result.updates); ++waits;
            result = layout.step(operation, true);
        } else result = layout.step(operation);
    }
    return waits;
}
void wait_layout(ui::Layout& layout, sc::Storage& storage, sc::Services& services) {
    LayoutStep wait{layout};
    (void)drive_layout(layout, wait.operation, layout.begin_wait_transition(wait.operation), storage, services);
}
void partially_build(ui::Layout& layout, ui::Layout::Operation& operation, std::span<const ui::Button> table,
                     int first_id, sc::Storage& storage, sc::Services& services, const ui::Resources& resources) {
    auto result = layout.begin_create_buttons(operation, table);
    for (unsigned leaves = 0; leaves < 2048; ++leaves) {
        require(result.status != ui::StepStatus::done && result.status != ui::StepStatus::error,
                "incremental Layout must not complete before its first built sprite is observed");
        if (sprite(services, resources.panel_button(first_id).identity).attr & ISLE_SP_SCALE) return;
        if (result.status == ui::StepStatus::needs_source_wait) {
            tick(storage, services, result.updates);
            result = layout.step(operation, true);
        } else result = layout.step(operation);
    }
    throw std::runtime_error("incremental Layout failed to build a genuine native sprite");
}
void layout_contract(sc::Storage& storage, sc::Services& services, const ui::Resources& resources) {
    // Backing precedes Layout: borrowed spans survive cancellation AND owner retirement.
    const std::array<ui::Button, 3> old_table{{{2, "Old"}, {4, "Old second"}, {35, {}}}};
    const std::array<ui::Button, 2> replacement{{{7, "Replacement"}, {35, {}}}};
    const std::array<ui::Button, 1> retry_table{{{11, "Same-pointer retry"}}};
    ui::Layout layout(services, resources);
    require(layout.initialize(), "initialize native Layout sprites and actual stackful OM animation process");
    LayoutStep cancelled{layout};
    partially_build(layout, cancelled.operation, old_table, 2, storage, services, resources);
    const auto partial = sprite(services, resources.panel_button(2).identity);
    require((partial.attr & ISLE_SP_SCALE) && (partial.attr & ISLE_SP_HIDDEN) &&
            !(sprite(services, resources.panel_button(4).identity).attr & ISLE_SP_SCALE),
            "incremental cancellation point is exactly one built native button, not a completed table");
    layout.cancel(cancelled.operation);
    auto result = layout.step(cancelled.operation, true);
    require(result.status == ui::StepStatus::error && result.error == ui::StepError::cancelled,
            "cancelled incremental create never reports completion");
    LayoutStep replace{layout};
    require(drive_layout(layout, replace.operation, layout.begin_create_buttons(replace.operation, replacement), storage, services) > 0,
            "replacement actually waits for canceled prefix's native hide transition");
    LayoutStep cancelled_wait{layout};
    result = layout.begin_wait_transition(cancelled_wait.operation);
    for (unsigned leaves = 0; result.status == ui::StepStatus::more && leaves < 16; ++leaves)
        result = layout.step(cancelled_wait.operation);
    require(result.status == ui::StepStatus::needs_source_wait, "replacement has an actual unfinished native show transition");
    const auto pending = sprite(services, resources.panel_button(7).identity);
    require(layout.step(cancelled_wait.operation).status == ui::StepStatus::needs_source_wait &&
            same_visual(pending, sprite(services, resources.panel_button(7).identity)),
            "unacknowledged Layout wait cannot itself animate the native replacement");
    layout.cancel(cancelled_wait.operation);
    result = layout.step(cancelled_wait.operation, true);
    require(result.status == ui::StepStatus::error && result.error == ui::StepError::cancelled,
            "Layout wait cancellation does not fabricate transition completion");
    wait_layout(layout, storage, services);
    tick(storage, services, 90);
    const auto old = sprite(services, resources.panel_button(2).identity);
    const auto current = sprite(services, resources.panel_button(7).identity);
    require((old.attr & ISLE_SP_HIDDEN) && !(old.attr & ISLE_SP_SCALE) && old.scaley == 1 &&
            !(current.attr & ISLE_SP_HIDDEN) && current.alpha == 255 && current.scaley > .9f && current.scalex == 1,
            "after real OM updates canceled old sprite stays hidden/unscaled while replacement is actually visible");
    require(layout.buttons()[0].id == 7, "completed replacement occupies its actual source layout slot");

    LayoutStep partial_retry{layout};
    partially_build(layout, partial_retry.operation, retry_table, 11, storage, services, resources);
    layout.cancel(partial_retry.operation);
    LayoutStep retry{layout};
    result = layout.begin_create_buttons(retry.operation, retry_table);
    result = layout.step(retry.operation);
    require(result.status == ui::StepStatus::more,
            "same-pointer equal-size partially built table must not shortcut as already completed");
    require(drive_layout(layout, retry.operation, result, storage, services) > 0,
            "same-pointer retry really hides its retained partial prefix before rebuilding");
    wait_layout(layout, storage, services);
    tick(storage, services, 90);
    const auto retried = sprite(services, resources.panel_button(11).identity);
    require(!(retried.attr & ISLE_SP_HIDDEN) && retried.alpha == 255 && retried.scaley > .9f &&
            (sprite(services, resources.panel_button(7).identity).attr & ISLE_SP_HIDDEN) &&
            (sprite(services, resources.panel_button(2).identity).attr & ISLE_SP_HIDDEN) && layout.buttons()[0].id == 11,
            "same-pointer retry finishes actual native show; prior tables remain hidden");
    LayoutStep hide{layout};
    (void)drive_layout(layout, hide.operation, layout.begin_hide_buttons(hide.operation), storage, services);
    wait_layout(layout, storage, services);
    const auto hidden = sprite(services, resources.panel_button(11).identity);
    require((hidden.attr & ISLE_SP_HIDDEN) && !(hidden.attr & ISLE_SP_SCALE) && hidden.scaley == 1,
            "returning hide and acknowledged native transition retire the visible button's scale state");
    layout.retire();
    require(services.sprite_count() == 0, "Layout joins its actual processes and erases native sprites");
}

void windows_contract(const isle::rom::Snapshot& rom, isle::game::State& state) {
    const auto resources = ui::Resources::load(rom);
    auto hydrated = isle::game_audio::hydrate(rom);
    require(bool(hydrated), "actual ROM Audio resource admission for concrete Services");
    std::array<isle::audio::effects::Section, 8> sections{};
    isle::audio::effects::Config effects;
    require(isle::game_audio::native_startup_effects(hydrated.resources, sections, effects) == isle::audio::Error::none,
            "actual GAME default Audio effects derivation");
    // The existing checked CPU PCM owner and real RSP borrower admit a genuine
    // Session, not a fabricated reference. No task/retrace/output call is made.
    isle::private_audio::Capture pcm;
    isle::private_audio::Execution executor{rom, nullptr, {}};
    auto audio = isle::game_audio::Session::create(std::move(hydrated.resources), effects, pcm.output(), executor.executor());
    require(bool(audio) && executor.machine, "concrete Services borrows an actual decoded GAME Audio Session");
    isle::game_input::Service input;
    sc::Storage storage;
    ok(storage.create(), "create actual native engine domain");
    {
        // Common only: Name and Oak overlay banks must never share residency.
        sc::Services services(storage.game(), storage.arena(), {{64, 64, 128, 8, 1024},
            resources->sprites(ui::ResourceBank::Group::common)}, input, *audio.session, state);
        require(!services.failed(), services.error());
        text_contract(storage, services, resources);
        frame_contract(storage, services, *resources);
        layout_contract(storage, services, *resources);
    }
    IsleGameCounts counts{};
    ok(isle_game_counts(storage.game(), &counts), "native counts after joined Services retirement");
    require(counts.objects == 0 && counts.processes == 0 && counts.sprites == 0,
            "all Window native objects/processes/sprites are retired before engine storage");
    ok(storage.destroy(), "joined native engine destruction");
    require(pcm.used == 0 && pcm.finishes == 0 && pcm.polls == 0 && audio.session->accepted_frames() == 0,
            "direct Window consumer never opens/pumps/claims audio output");
    audio.session.reset();
    require(!executor.machine && !executor.borrowed, "real Audio RSP borrower retires before its backing owners");
}
void run(const fs::path& rom_path, const fs::path& root) {
    require(root.is_absolute() && fs::is_directory(fs::symlink_status(root)) && !fs::is_symlink(fs::symlink_status(root)) &&
            fs::is_empty(root) && fs::canonical(root) == root.lexically_normal(),
            "NEW_OWNED_EMPTY_ROOT must be an existing empty absolute nonsymlink canonical directory");
    const auto rom = isle::rom::read_snapshot(rom_path);
    const auto profile = root / "profile";
    const auto foreign_profile = root / "foreign-profile";
    require(fs::create_directory(profile) && fs::create_directory(foreign_profile), "create only new owned private profiles");
    {
        isle::game::State state(profile);
        require(state.initialize(1), state.error());
        require(state.save_initialization()->state == save::GameLoadState::initialized_missing,
                "fresh profile initializes RAM only from actual Missing evidence");
        photos_contract(rom, state, foreign_profile);
        windows_contract(rom, state);
    }
    auto reopened = save::GameSession::open(profile, 10);
    require(bool(reopened) && reopened.state == save::GameLoadState::initialized_missing,
            "all direct mutations/reset/Window teardown leave durable store genuinely Missing");
    const auto snapshot = value(reopened.session.snapshot(), "reopened uncommitted RAM defaults");
    require(!value(snapshot.photo_present(save::PhotoSet::report, 9), "reopened report absence") &&
            !value(snapshot.photo_present(save::PhotoSet::album, 0), "reopened album absence") &&
            value(snapshot.report_score(9), "reopened default score") == 0,
            "no implicit commit persisted authored records or independent scores");
    std::cout << "post-course direct native interfaces passed: canonical Photos/GameSession, returning Text/Frame/Layout, real OM updates\n"
                 "scope: authored mutation records + admitted ROM resources; no captured photos, gameplay progression, replay, GPU, audio output or durable commit\n";
}
} // namespace

#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
#else
int main(int argc, char** argv) {
#endif
    try {
        require(argc == 3, "usage: post-course-interfaces-rom-smoke PRIVATE_ROM NEW_OWNED_EMPTY_ROOT");
        run(fs::path(argv[1]), fs::path(argv[2]));
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "post-course direct native interfaces: " << error.what() << '\n';
        return 1;
    }
}

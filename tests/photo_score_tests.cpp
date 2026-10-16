#include "35mmisle/photo_score.hpp"

#include <bit>
#include <cfenv>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

namespace {
namespace ps = isle::photo_score;
int failures = 0;
void check(bool condition, const char* description) {
    if (!condition) { ++failures; std::cerr << "FAIL: " << description << '\n'; }
}

// Authored rule data, deliberately NOT the original game's bonus tables.
struct Fixture {
    std::vector<std::byte> image = std::vector<std::byte>(ps::RulesView::maximum_bytes);
    ps::RulesView rules;
    ps::Photo photo;
    isle::photo_feedback::FinalCounts counts{};
    std::array<ps::SubjectMap, 12> mapping{};
    std::size_t count = 1;

    void byte(std::uint32_t guest, std::uint8_t value) {
        image[guest - ps::RulesView::guest_begin] = static_cast<std::byte>(value);
    }
    void half(std::uint32_t guest, std::uint16_t value) {
        byte(guest, static_cast<std::uint8_t>(value >> 8)); byte(guest + 1, static_cast<std::uint8_t>(value));
    }
    void word(std::uint32_t guest, std::uint32_t value) {
        half(guest, static_cast<std::uint16_t>(value >> 16)); half(guest + 2, static_cast<std::uint16_t>(value));
    }
    void real(std::uint32_t guest, float value) { word(guest, std::bit_cast<std::uint32_t>(value)); }
    void animation(std::uint32_t row, std::uint32_t pokemon, std::uint32_t pointer, std::uint32_t length) {
        word(0x800ad4b8 + row * 12, pokemon);
        word(0x800ad4bc + row * 12, pointer);
        word(0x800ad4c0 + row * 12, length);
    }
    Fixture() {
        for (std::uint32_t i = 0; i < 151; ++i) byte(0x800ae4e4 + i, 0xff);
        byte(0x800ae4e4 + 15, 0); byte(0x800ae4e4 + 16, 0); byte(0x800ae4e4 + 89, 1);
        half(0x800ae744, 1); half(0x800ae746, 3);
        for (std::uint32_t row = 0; row < 4; ++row)
            for (std::uint32_t col = 0; col < 12; ++col)
                half(0x800ae6e4 + (row * 12 + col) * 2, static_cast<std::uint16_t>(row * 300 + col * 10));
        half(0x800ae59e, 300); half(0x800ae5a0, 199); half(0x800ae5a2, 200);
        half(0x800ae582, 100);
        animation(0, 16, 0x800ac000, 2); real(0x800ac00c, -1.0f);
        animation(1, 17, 0x800ac020, 1);
        animation(2, 90, 0x800ac040, 1);
        animation(3, 603, 0x800ac060, 1);
        photo.subjects[0] = {16, 0, 1, 0, 0};
        photo.eye_z = 1;
        counts[0] = {768, 1, 768, 0, 768};
        check(ps::RulesView::borrow(image, rules), "borrow authored GAME-shaped rule image");
    }
    bool evaluate(ps::Result& out) const { return ps::evaluate(rules, photo, counts, {mapping.data(), count}, out); }
    ps::Result result() const { ps::Result out; check(evaluate(out), "evaluate authored score input"); return out; }
    void second(std::int16_t id = 16) {
        count = 2; photo.subjects[1] = {id, 0, 1, 0, 0};
        mapping[1] = {1, 1}; counts[1] = {768, 0, 768, 0, 768};
    }
};

void bonuses_and_framing() {
    Fixture f;
    auto out = f.result();
    check(out.total == 2600 && out.proximity == 1000 && out.completeness == 10000 &&
          out.pose_points == 300 && out.well_framed && out.selected_photo_index == 0,
          "isolated center doubles size plus pose despite no visible center pixels");
    f.photo.subjects[0].special_id = 1; f.second(); out = f.result();
    check(out.total == 3800 && out.special_bonus == 100 && out.special_id == 1 &&
          out.same_species_bonus == 1000 && out.same_species_number == 1,
          "special bonus is doubled, same-species bonus is added afterward");
    f.counts[0].center = 0; out = f.result();
    check(out.total == 1400 && !out.well_framed && out.same_species_number == 0,
          "off-center subject receives neither doubling nor same-species bonus");
    f.counts[0].center = 1; f.photo.subjects[0].pose_id = 2; out = f.result();
    check(out.total == 1299 && out.pose_points == 199 && !out.well_framed,
          "199-point pose stops before framing and companions");
    f.photo.subjects[0].pose_id = 3; out = f.result();
    check(out.total == 3600 && out.well_framed, "200-point pose admits framing and companions");
    f.photo.subjects[0].pokemon_id = 603; f.photo.subjects[0].special_id = 0;
    f.photo.subjects[1].pokemon_id = 90; out = f.result();
    check(out.pokemon_in_focus == 90 && out.same_species_bonus == 1000,
          "alternate Shellder ID normalizes focus and shares the bonus family");
}

void thresholds_and_rounding() {
    Fixture f;
    f.counts[0].visible = 46; f.counts[0].approximate_total = 46;
    auto out = f.result();
    check(out.total == 240 && out.pose_points == 0 && !out.well_framed,
          "46 visible pixels remain below the pose size gate");
    f.counts[0].visible = 47; f.counts[0].approximate_total = 47; out = f.result();
    check(out.total == 1100 && out.proximity == 250 && out.well_framed,
          "47 visible pixels cross the unrounded proximity threshold");
    f.counts[0].visible = 599; f.counts[0].approximate_total = 1000; out = f.result();
    check(out.completeness == 5990 && out.total == 490 && out.pose_points == 0,
          "completeness below sixty percent stops after rounded size");
    f.counts[0].visible = 600; out = f.result();
    check(out.completeness == 6000 && out.total == 1580 && out.well_framed,
          "sixty percent completeness admits pose and framing");
    f.counts[0] = {3072, 1, 3072, 0, 1}; out = f.result();
    check(out.proximity == 1000 && out.completeness == 10000 && out.total == 2600,
          "size caps at768 but full visible ratio clamps to one");
    f.counts[0] = {768, 1, 768, 0, 768}; f.second();
    f.counts[1] = {4, 0, 3, 0, 4}; out = f.result();
    check(out.same_species_number == 0, "three visible companion pixels score nothing");
    f.counts[1].visible = 4; out = f.result();
    check(out.same_species_bonus == 200 && out.same_species_number == 1,
          "four visible companion pixels earn the minimum200 bonus");
    f.counts[1] = {12, 0, 12, 0, 168}; out = f.result();
    check(out.same_species_bonus == 20,
          "companion fraction remains unquantized: quantizing to714/10000 would incorrectly give10");
    f.counts[1].visible = 0; out = f.result();
    check(out.total == 2600 && out.same_species_number == 0 && out.selected_photo_index == 0,
          "active zero-visible mapping earns nothing without changing selected identity");
    f.count = 12;
    for (std::uint8_t i = 0; i < 12; ++i) {
        f.photo.subjects[i] = {16, 0, 1, 0, 0};
        const auto slot = static_cast<std::uint8_t>(11 - i);
        f.mapping[i] = {i, slot};
        f.counts[slot] = {64, 1, 64, 0, 64};
    }
    out = f.result();
    check(out.total == 3980 && out.same_species_bonus == 2860 && out.same_species_number == 11 &&
          out.selected_photo_index == 0,
          "all twelve reversed feedback slots retain first-rank tie and eleven companion bonuses");
}

void subject_selection_and_flags() {
    Fixture f; f.second(17);
    f.counts[0] = {650, 1, 650, 0, 1000};
    f.counts[1] = {1000, 1, 1000, 0, 2000};
    check(f.result().selected_photo_index == 0, "strong centered category outranks larger weak centered subject");
    f.counts[0].center = 0;
    check(f.result().selected_photo_index == 1, "weak centered category outranks strong off-center subject");
    f.counts[1].center = 0;
    check(f.result().selected_photo_index == 0, "strong off-center category outranks weak off-center subject");
    f.counts[0].approximate_total = 2000;
    check(f.result().selected_photo_index == 1, "largest visible subject wins within one category");
    f.counts[0].visible = 1000;
    check(f.result().selected_photo_index == 0, "equal visibility retains the first subject");
    f.photo.flags = 0x20; f.count = 1; f.counts[0] = {12, 0, 12, 0, 240};
    auto out = f.result();
    check(out.selected_photo_index == 0 && out.completeness == 500 && out.photo_flags == 0x20,
          "tagged subject accepts twelve pixels at five percent and retains focus flag");
    f.counts[0].approximate_total = 241; out = f.result();
    check(out.pokemon_in_focus == 0 && out.photo_flags == 0,
          "below five percent falls through, fails twenty percent, clears focus flag");
    f.photo.flags = 0; f.counts[0] = {12, 0, 12, 0, 60};
    check(f.result().pokemon_in_focus == 16, "ordinary selection accepts exact twenty percent");
    f.counts[0].approximate_total = 61;
    check(f.result().pokemon_in_focus == 0, "ordinary selection rejects less than twenty percent");
    f.counts[0] = {12, 0, 11, 0, 12};
    check(f.result().pokemon_in_focus == 0, "eleven pixels cannot select an ordinary subject");
    f.count = 2; f.photo.subjects[0].pokemon_id = 1001;
    f.photo.subjects[1] = {16, 0, 1, 0, 0}; f.photo.subjects[2] = {17, 0, 1, 0, 0};
    f.mapping[0] = {1, 11}; f.mapping[1] = {2, 3};
    f.counts[11] = {768, 1, 768, 0, 768};
    f.counts[3] = {650, 1, 650, 0, 650}; out = f.result();
    check(out.selected_photo_index == 2 && out.pokemon_in_focus == 17,
          "sparse feedback preserves raw classifier and mapped selected photo identity");
}

void signs_eggs_and_eligibility() {
    Fixture f; f.count = 0;
    for (const auto id : {1004, 1010, 1018, 1022, 1028, 1035}) {
        f.photo.flags = 0x20; f.photo.subjects[0].pokemon_id = static_cast<std::int16_t>(id);
        f.photo.has_dash_engine = false;
        check(f.result().pokemon_in_focus == 500, "sign hidden before dash-engine unlock");
        f.photo.has_dash_engine = true;
        check(f.result().pokemon_in_focus == id && f.result().total == 0,
              "sign identity unlocked without invented numeric bonus");
    }
    for (const auto flags : {0x60, 0x80, 0xa0, 0xe0}) {
        f.photo.flags = static_cast<std::uint8_t>(flags);
        const auto out = f.result();
        check(out.pokemon_in_focus > 1000 && out.photo_flags == flags, "scene-sign flags resolve before feedback");
    }
    for (const auto id : {600, 601, 602}) {
        f.photo.flags = 0x20; f.photo.subjects[0].pokemon_id = static_cast<std::int16_t>(id);
        const auto out = f.result();
        check(out.pokemon_in_focus == id && out.total == 0 && out.photo_flags == 0x20,
              "egg focus has no numeric score and preserves tag");
    }
    f.photo.subjects[0] = {16, 1, 1, 0, 0}; f.count = 1;
    check(f.result().pokemon_in_focus == 500, "negative animation eligibility rejects even a tagged photograph");
    f.photo.flags = 0;
    check(f.result().pokemon_in_focus == 500, "negative eligibility also rejects ordinary selected subject");
    f.photo.subjects[0].pokemon_id = 25;
    check(f.result().pokemon_in_focus == 500, "missing animation family is unscorable, not a fabricated pose");
    f.photo.level_id = -1; f.photo.flags = 0x60; f.count = 0;
    check(f.result().pokemon_in_focus == 0 && f.result().photo_flags == 0x40,
          "negative level disables scene-sign shortcut and ordinary flow clears20h");
}

void orientation_endpoints_and_failures() {
    Fixture f; f.photo.subjects[0].pose_id = 0;
    check(f.result().pose_points == 300, "front orientation chooses first authored angle column");
    f.photo.subjects[0].yaw = 0x1.921fb6p+1f;
    check(f.result().pose_points == 600, "exact binary32 pi reads next row rather than clamping column12");
    f.photo.subjects[0].yaw = -0x1.921fb6p+1f;
    check(f.result().pose_points == 600, "negative pi has the same folded endpoint");
    f.photo.subjects[0].yaw = 0x1.921fb6p+2f;
    check(f.result().pose_points == 300, "full binary32 turn folds to front orientation");
    f.photo.subjects[0].yaw = 0; f.photo.eye_x = 1; f.photo.eye_z = 0;
    check(f.result().pose_points == 360, "camera azimuth participates in orientation bins");
    f.photo.eye_x = 0; f.photo.eye_z = 1; f.photo.subjects[0].pokemon_id = 90;
    f.photo.subjects[0].yaw = 0x1.921fb6p+1f;
    check(f.result().pose_points == 1 && !f.result().well_framed,
          "last angle-row endpoint reads following GAME class table without native array UB");
    ps::Result out; out.total = 12345; const auto saved = out;
    auto rejects = [&](const char* message) { check(!f.evaluate(out) && out == saved, message); };
    f.photo.subjects[0].yaw = std::numeric_limits<float>::max(); rejects("nonprogressing finite angle rejected without output mutation");
    f.photo.subjects[0].yaw = std::numeric_limits<float>::quiet_NaN(); rejects("used NaN angle rejected");
    f.photo.subjects[0].pose_id = 1;
    check(f.evaluate(out) && out.pose_points == 300, "explicit pose does not evaluate unused camera/yaw path");
    out = saved; f.photo.subjects[0].pose_id = 164; rejects("pose table overrun rejected transactionally");
    f.photo.subjects[0].pose_id = 1; f.photo.subjects[0].special_id = 14; rejects("special table overrun rejected");
    f.photo.subjects[0].special_id = 0; f.photo.subjects[0].animation_index = 1; rejects("animation table count enforced");
    f.photo.subjects[0].animation_index = 0;
    f.real(0x800ac040, std::numeric_limits<float>::infinity()); rejects("nonfinite animation eligibility rejected");
    f.real(0x800ac040, 0); f.word(0x800ad4bc + 2 * 12, 0xfffffffcu); rejects("guest table address escape rejected");
    f.word(0x800ad4bc + 2 * 12, 0x800ac040);
    f.counts[0].visible = 3073; rejects("impossible visible counter rejected");
    f.counts[0].visible = 768; f.counts[0].center = 17; rejects("impossible isolated center rejected");
    f.counts[0].center = 1; f.mapping[0].photo_index = 12; rejects("photo mapping overrun rejected before shift");
    f.mapping[0].photo_index = 1; rejects("mapping past photo sentinel rejected");
    f.mapping[0].photo_index = 0; f.mapping[0].feedback_slot = 12; rejects("feedback slot overrun rejected");
    f.mapping[0].feedback_slot = 0; f.count = 2;
    f.mapping[1] = f.mapping[0]; rejects("duplicate photo mapping rejected");
    f.photo.subjects[1] = f.photo.subjects[0]; f.mapping[1].photo_index = 1;
    rejects("duplicate feedback slot rejected independently of photo mapping");
    f.count = 1;
    const int previous = std::fegetround();
    if (std::fesetround(FE_DOWNWARD) == 0) rejects("foreign rounding mode rejected without score mutation");
    check(std::fesetround(previous) == 0, "restore test rounding state");
    ps::RulesView empty;
    check(!ps::evaluate(empty, f.photo, f.counts, {f.mapping.data(), 1}, out) && out == saved,
          "invalid rule view never reads tables or changes output");
    check(!ps::RulesView::borrow(std::span<const std::byte>(f.image).first(ps::RulesView::minimum_bytes - 1), empty),
          "short rule image rejected");
}
}

int main() {
    bonuses_and_framing(); thresholds_and_rounding(); subject_selection_and_flags();
    signs_eggs_and_eligibility(); orientation_endpoints_and_failures();
    if (failures) { std::cerr << failures << " photo score checks failed\n"; return 1; }
    std::cout << "Native Oak selection, rounding, bonus, orientation and invalid-input checks passed\n";
    return 0;
}

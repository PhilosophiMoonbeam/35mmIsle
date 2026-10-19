#include "native_campaign_policy.hpp"
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace isle::private_campaign {
namespace {

// Immutable Source df8815e75f3d79618466e01e0827c6165b93d11c:
// app_render/47380.c:70-76,329-356 maps Mew151 to Report62, then signs
// 1004/1010/1028/1022/1018/1035 to Reports63..68 (course order, NOT the
// separate sign_evidence() array's order). Presence is the canonical photo
// sentinel, exposed by GameSnapshot::photo_present, never a score test.
constexpr std::size_t mew_report = 62;
constexpr std::size_t first_sign_report = 63;
constexpr int ordinary_courses = 6;
constexpr int course_count = 7;
static_assert(save::game_report_count == first_sign_report + ordinary_courses);

void require(bool valid, const char* message) {
    if (!valid) throw std::runtime_error(message);
}

template<class T>
T observation(save::GameValue<T> value, const char* message) {
    require(bool(value), message);
    return value.value;
}

struct Progress {
    int known = 0;
    int unlocked = 0;
    bool apple = false;
    bool pester = false;
    bool flute = false;
    bool dash = false;
};

Progress progress(const save::GameSnapshot& snapshot) {
    require(bool(snapshot), "campaign requires an actual canonical snapshot");
    const auto known = observation(snapshot.selected_course(), "campaign known-course observation unavailable");
    const int unlocked = observation(snapshot.unlocked_course(), "campaign unlocked-course observation unavailable");
    require(known < course_count && unlocked >= 0 && unlocked < course_count &&
            static_cast<int>(known) <= unlocked, "campaign canonical K/U outside natural course bounds");
    // more_funcs/5BF20.c:391-408,543-558 and native_sign_puzzles.cpp use
    // actual ownership flags 0/1/2/5. K is acknowledged discovery, not the
    // last ride and not the access ceiling; U may legitimately lead K.
    return {static_cast<int>(known), unlocked,
        observation(snapshot.player_flag(0), "campaign Apple observation unavailable"),
        observation(snapshot.player_flag(1), "campaign Pester observation unavailable"),
        observation(snapshot.player_flag(2), "campaign Flute observation unavailable"),
        observation(snapshot.player_flag(5), "campaign Dash observation unavailable")};
}

bool reported(const save::GameSnapshot& snapshot, std::size_t index) {
    return observation(snapshot.photo_present(save::PhotoSet::report, index),
        "campaign canonical Report presence unavailable");
}

bool finished(const save::GameSnapshot& snapshot, const Progress& observed) {
    if (observed.unlocked != 6 || !observed.apple || !observed.pester ||
        !observed.flute || !observed.dash) return false;
    for (int course = 0; course < ordinary_courses; ++course)
        if (!reported(snapshot, first_sign_report + static_cast<std::size_t>(course))) return false;
    return reported(snapshot, mew_report);
}

private_course::CaptureGoal goal(const save::GameSnapshot& snapshot, const Progress& observed, int course) {
    using private_course::CaptureGoal;
    // Natural Report6/22/40 rewards publish U1/3/5 themselves. Never infer
    // access from those counts, nor replay an already superseded U2/U4 event.
    // Tunnel electrode -> endpoint1014 needs Apple; River Porygon ->1019
    // needs Pester. Valley's actual endpoint dialogue grants Dash, not U6.
    if ((course == 1 && observed.unlocked < 2 && observed.apple) ||
        (course == 3 && observed.unlocked < 4 && observed.pester) ||
        (course == 5 && !observed.dash && observed.pester)) return CaptureGoal::progression;

    if (course >= ordinary_courses || !observed.dash ||
        reported(snapshot, first_sign_report + static_cast<std::size_t>(course)))
        return CaptureGoal::photographs;

    // Dash authorizes every sign's marking. Tunnel additionally needs the
    // Apple lure and Flute/Zapdos power chain; Volcano needs Pester smoke;
    // River needs Flute/Vileplume. Beach, Cave and Valley need no producer item.
    if ((course == 1 && (!observed.apple || !observed.flute)) ||
        (course == 2 && !observed.pester) || (course == 3 && !observed.flute))
        return CaptureGoal::photographs;
    return CaptureGoal::sign;
}

bool reassessment(const save::GameSnapshot& snapshot, const Progress& observed) {
    if (observed.apple && observed.pester && observed.flute) return false;
    std::int64_t score = 0;
    // photo_check/98C330.c:2033-2070 counts only present ordinary Reports
    // and adds scores for present signs too. Absent slots' stale scores do
    // not contribute, and zero-score signs remain genuine Report entries.
    for (std::size_t index = 0; index < save::game_report_count; ++index) {
        if (!reported(snapshot, index)) continue;
        const auto value = observation(snapshot.report_score(index), "campaign Report score unavailable");
        require(value >= 0, "campaign present Report has a negative native score");
        score += value;
        require(score <= std::numeric_limits<std::int32_t>::max(), "campaign Report total exceeds native score range");
    }

    // 98C330.c:2197-2253 first suppresses equipment for U>K or the Valley
    // notice800, then runs this EXCLUSIVE ladder. An owned higher-branch
    // reward does not fall through to a lower one. Permission is for a real
    // later Photo15, not a grant or evidence of new Report publication.
    // Oak8A7360.c:470-472 naturally acknowledges K=U before that next ride;
    // use actual U for that future K, without altering either observation.
    const int known_after_oak = observed.known < observed.unlocked ? observed.unlocked : observed.known;
    if (score >= 130000 && observed.dash) return !observed.flute;
    if (score >= 72500 && known_after_oak >= 3) return !observed.pester;
    if (score >= 24000 && known_after_oak > 0) return !observed.apple;
    return false;
}

} // namespace

CampaignTrip CampaignPolicy::next(const save::GameSnapshot& snapshot) const {
    const auto observed = progress(snapshot);
    if (finished(snapshot, observed)) return {};

    // Remember the first legal round-robin successor, but let any legally
    // unvisited course take precedence. Neither a new unlock nor repeated
    // next() inspection resets the cursor; only a real completed ride does.
    int chosen = 0;
    bool have_legal = false;
    for (int offset = 1; offset <= course_count; ++offset) {
        const int course = (last_ride_ + offset) % course_count;
        if (course > observed.unlocked) continue;
        if (!have_legal) { chosen = course; have_legal = true; }
        if (!visited_[static_cast<std::size_t>(course)]) { chosen = course; break; }
    }
    return {chosen, goal(snapshot, observed, chosen), reassessment(snapshot, observed)};
}

void CampaignPolicy::completed(int course) {
    require(course >= 0 && course < course_count, "campaign completed ride outside actual course bounds");
    visited_[static_cast<std::size_t>(course)] = true;
    last_ride_ = course;
}

bool CampaignPolicy::complete(const save::GameSnapshot& snapshot) const {
    return finished(snapshot, progress(snapshot));
}

} // namespace isle::private_campaign

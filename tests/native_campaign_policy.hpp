#ifndef ISLE_PRIVATE_NATIVE_CAMPAIGN_POLICY_HPP
#define ISLE_PRIVATE_NATIVE_CAMPAIGN_POLICY_HPP

#include "35mmisle/game_save.hpp"
#include "native_course_probe.hpp"
#include <array>

namespace isle::private_campaign {

struct CampaignTrip {
    int course = -1;
    private_course::CaptureGoal goal = private_course::CaptureGoal::photographs;
    bool allow_reassessment = false;
};

// Private driver policy only: immutable observations never grant access, items,
// Report entries or scores. Invalid/unavailable observations throw rather than
// masquerading as campaign completion. Successful calls allocate nothing.
class CampaignPolicy {
public:
    [[nodiscard]] CampaignTrip next(const save::GameSnapshot&) const;
    // Record only a genuinely completed ride, independently of Source's K/U.
    void completed(int course);
    [[nodiscard]] bool complete(const save::GameSnapshot&) const;
private:
    std::array<bool, 7> visited_{};
    int last_ride_ = -1;
};

} // namespace isle::private_campaign

#endif

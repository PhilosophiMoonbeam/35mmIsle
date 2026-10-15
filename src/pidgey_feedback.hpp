#ifndef ISLE_PIDGEY_FEEDBACK_HPP
#define ISLE_PIDGEY_FEEDBACK_HPP

#include "35mmisle/photo_feedback.hpp"
#include "pidgey_producer.hpp"

namespace isle::rsp { class CanonicalExecutor; }

namespace isle::pidgey {
enum class InputInstallation { already_resident, cold_epoch };
struct FeedbackCounts {
    photo_feedback::WideCounts wide;
    photo_feedback::FinalCounts narrow;
};

// Synchronous, two genuine tasks on the caller's persistent executor. Copies
// bounded input descriptors before acquiring preparation, validates before any
// RAM write, freezes both poses/cameras and restores the borrowed native camera.
// Wide reduction holds the actual completed lease; only value counts survive
// release before narrow preparation. No RAM view, lease or fabricated completion
// escapes. Either failure throws and publishes no counts; no executor reset or
// per-subject serialization. Caller retains all producer input owners through
// return and selects cold installation explicitly for a new canonical epoch.
FeedbackCounts render_feedback(rsp::CanonicalExecutor& executor, PidgeyProducer& producer,
                               std::uint64_t canonical_epoch, const FeedbackRequest& request,
                               InputInstallation installation);
} // namespace isle::pidgey
#endif

#include "pidgey_feedback.hpp"
#include "rsp_render.hpp"

#include <stdexcept>
#include <utility>

namespace isle::pidgey {
namespace {
std::span<const std::uint8_t> bytes(const rsp::CanonicalExecutor::MemoryLease& lease) {
    const auto memory = lease.rdram();
    return {reinterpret_cast<const std::uint8_t*>(memory.data()), memory.size()};
}
photo_feedback::WideCounts reduce_wide(const rsp::CanonicalExecutor::MemoryLease& lease,
                                       std::uint32_t base) {
    photo_feedback::PlanesView view;
    photo_feedback::WideCounts counts{};
    if (!lease || !photo_feedback::PlanesView::borrow(bytes(lease), base, view) ||
        !photo_feedback::count_wide(view, counts))
        throw std::runtime_error("Completed native wide feedback reduction failed");
    return counts; // View dies before caller releases the actual lease.
}
photo_feedback::FinalCounts reduce_narrow(const rsp::CanonicalExecutor::MemoryLease& lease,
                                         std::uint32_t base,
                                         const photo_feedback::WideCounts& wide) {
    photo_feedback::PlanesView view;
    photo_feedback::FinalCounts counts{};
    if (!lease || !photo_feedback::PlanesView::borrow(bytes(lease), base, view) ||
        !photo_feedback::count_narrow(view, wide, counts))
        throw std::runtime_error("Completed native narrow feedback reduction failed");
    return counts;
}
} // namespace
FeedbackCounts render_feedback(rsp::CanonicalExecutor& executor, PidgeyProducer& producer,
                               std::uint64_t epoch, const FeedbackRequest& input,
                               InputInstallation installation) {
    if (installation != InputInstallation::cold_epoch &&
        installation != InputInstallation::already_resident)
        throw std::runtime_error("Invalid native feedback installation policy");
    // No reference to caller actors/camera is retained after this copy. Both
    // effective cameras and every native pose are checked before cold RAM writes.
    const auto request = producer.freeze_feedback_request(input);
    FeedbackCounts counts{};
    {
        auto preparation = executor.begin_preparation();
        PreparedPidgeyTask task{};
        {
            const auto words = preparation.words();
            if (installation == InputInstallation::cold_epoch) producer.install_inputs(words, epoch);
            task = producer.prepare_feedback_pass(words, epoch, request, FeedbackPass::wide);
        } // CPU writer borrow has ceased before execute consumes preparation.
        auto completed = executor.execute(std::move(preparation), task.task, task.boot);
        counts.wide = reduce_wide(completed, request.planes_base());
        completed.release();
    }
    {
        auto preparation = executor.begin_preparation();
        PreparedPidgeyTask task{};
        {
            const auto words = preparation.words();
            task = producer.prepare_feedback_pass(words, epoch, request, FeedbackPass::narrow);
        }
        auto completed = executor.execute(std::move(preparation), task.task, task.boot);
        counts.narrow = reduce_narrow(completed, request.planes_base(), counts.wide);
        completed.release();
    }
    return counts;
}
} // namespace isle::pidgey

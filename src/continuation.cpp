#include "35mmisle/continuation.h"
#include "35mmisle/process.hpp"
#include "continuation_trampoline.h"

#include <atomic>
#include <cstddef>
#include <exception>
#include <thread>
#include <type_traits>
#include <utility>

struct IsleContinuationDomain {
    const std::thread::id owner = std::this_thread::get_id();
    IsleContinuation* first = nullptr;
    IsleContinuation* active = nullptr; // Accessed only by the owner.
    char copied_error[512]{};
    std::atomic<const char*> error{"no continuation error"};
};

struct IsleContinuation {
    IsleContinuationDomain* const domain;
    IsleContinuation* previous = nullptr;
    IsleContinuation* next = nullptr;
    const IsleContinuationEntry entry;
    void* const user;
    IsleContinuationFrame frame;
    isle::Yield* yield = nullptr;
    std::exception_ptr pending_exception;
    bool reported_failure = false;
    char failure_message[512]{};
    isle::Process process;

    IsleContinuation(IsleContinuationDomain* owner, IsleContinuationEntry callback, void* data);
    void run(isle::Yield& current_yield);
};

namespace {

thread_local IsleContinuation* active_context = nullptr;

struct ReportedFailure {};

void copy_diagnostic(char (&destination)[512], const char* message) noexcept {
    std::size_t i = 0;
    while (i + 1 < sizeof(destination) && message[i] != '\0') {
        destination[i] = message[i];
        ++i;
    }
    destination[i] = '\0';
}

void report_failure(IsleContinuation* node) noexcept {
    copy_diagnostic(node->domain->copied_error, node->failure_message);
    node->domain->error.store(node->domain->copied_error, std::memory_order_relaxed);
}

IsleContinuationResult error(IsleContinuationDomain* domain,
                             IsleContinuationResult result, const char* message) noexcept {
    if (domain != nullptr) {
        domain->error.store(message, std::memory_order_relaxed);
    }
    return result;
}

bool is_owner(const IsleContinuationDomain* domain) noexcept {
    return std::this_thread::get_id() == domain->owner;
}

bool is_callback(const IsleContinuationDomain* domain) noexcept {
    return active_context != nullptr && active_context->domain == domain;
}

IsleContinuationState public_state(isle::Process::State state) noexcept {
    switch (state) {
    case isle::Process::State::ready: return ISLE_CONT_READY;
    case isle::Process::State::running: return ISLE_CONT_RUNNING;
    case isle::Process::State::waiting: return ISLE_CONT_WAITING;
    case isle::Process::State::completed: return ISLE_CONT_COMPLETED;
    case isle::Process::State::cancelled: return ISLE_CONT_CANCELLED;
    case isle::Process::State::failed: return ISLE_CONT_FAILED;
    }
    std::terminate();
}

// This adapter catches a C++ entry's exception before returning through the C
// trampoline. C entries must likewise never throw across their own C frames.
// Only trivial automatic state is above the trampoline's saved C frame.
void guarded_entry(void* context) noexcept {
    auto* const node = static_cast<IsleContinuation*>(context);
    try {
        node->entry(node->user);
    } catch (...) {
        node->pending_exception = std::current_exception();
    }
}

IsleContinuationResult owner_create(IsleContinuationDomain* domain,
                                    IsleContinuationEntry entry, void* user,
                                    IsleContinuation** output) noexcept {
    try {
        auto* const node = new IsleContinuation(domain, entry, user);
        node->next = domain->first;
        if (node->next != nullptr) {
            node->next->previous = node;
        }
        domain->first = node;
        *output = node;
        return ISLE_CONT_OK;
    } catch (...) {
        return error(domain, ISLE_CONT_RESOURCE_FAILURE,
                     "cannot allocate or start a continuation worker");
    }
}

IsleContinuationResult owner_cancel(IsleContinuation* node) noexcept {
    if (node->domain->active == node) {
        return error(node->domain, ISLE_CONT_INVALID_STATE,
                     "the current callback must finish instead of cancelling itself");
    }
    try {
        node->process.cancel();
        return ISLE_CONT_OK;
    } catch (...) {
        return error(node->domain, ISLE_CONT_EXECUTION_FAILURE,
                     "cannot cancel and join continuation worker");
    }
}

IsleContinuationResult owner_destroy(IsleContinuation* node) noexcept {
    const auto result = owner_cancel(node);
    if (result != ISLE_CONT_OK) {
        return result;
    }
    if (node->previous != nullptr) {
        node->previous->next = node->next;
    } else {
        node->domain->first = node->next;
    }
    if (node->next != nullptr) {
        node->next->previous = node->previous;
    }
    delete node;
    return ISLE_CONT_OK;
}

IsleContinuationResult owner_state(IsleContinuation* node,
                                   IsleContinuationState* output) noexcept {
    try {
        *output = public_state(node->process.state());
        return ISLE_CONT_OK;
    } catch (...) {
        return error(node->domain, ISLE_CONT_EXECUTION_FAILURE,
                     "cannot inspect continuation state");
    }
}

enum class Operation { create, cancel, destroy, state, invoke };
struct Request {
    Operation operation;
    IsleContinuationDomain* domain;
    IsleContinuation* node;
    IsleContinuationEntry entry;
    void* user;
    IsleContinuation** created;
    IsleContinuationState* state;
    IsleContinuationResult result;
};
static_assert(std::is_trivially_destructible_v<Request>);

void service(void* payload) noexcept {
    auto* const request = static_cast<Request*>(payload);
    switch (request->operation) {
    case Operation::create:
        request->result = owner_create(request->domain, request->entry,
                                       request->user, request->created);
        break;
    case Operation::cancel:
        request->result = owner_cancel(request->node);
        break;
    case Operation::destroy:
        request->result = owner_destroy(request->node);
        break;
    case Operation::state:
        request->result = owner_state(request->node, request->state);
        break;
    case Operation::invoke:
        try {
            request->entry(request->user);
            request->result = ISLE_CONT_OK;
        } catch (...) {
            request->result = error(request->domain,ISLE_CONT_EXECUTION_FAILURE,
                                    "owner service callback threw an exception");
        }
        break;
    }
}

IsleContinuationResult dispatch(Request* request) noexcept {
    if (is_owner(request->domain)) {
        service(request);
        return request->result;
    }
    if (!is_callback(request->domain)) {
        return error(request->domain, ISLE_CONT_WRONG_OWNER,
                     "continuation operations require the owner or its active callback");
    }
    try {
        active_context->yield->on_owner(service, request);
        return request->result;
    } catch (...) {
        active_context->pending_exception = std::current_exception();
    }
    // The catch has ended. No exception runtime state or nontrivial automatic
    // object is skipped by the C abort. The outer C++ wrapper rethrows later.
    isle_continuation_abort(&active_context->frame);
}

IsleContinuationResult require_callback(IsleContinuationDomain* domain) noexcept {
    if (domain == nullptr) {
        return ISLE_CONT_INVALID_ARGUMENT;
    }
    if (!is_callback(domain)) {
        return error(domain, is_owner(domain) ? ISLE_CONT_INVALID_STATE : ISLE_CONT_WRONG_OWNER,
                     "wait and finish require this domain's active compiled-C callback");
    }
    return ISLE_CONT_OK;
}

} // namespace

IsleContinuation::IsleContinuation(IsleContinuationDomain* owner,
                                   IsleContinuationEntry callback, void* data)
    : domain(owner), entry(callback), user(data),
      process([this](isle::Yield& current_yield) { run(current_yield); }) {}

void IsleContinuation::run(isle::Yield& current_yield) {
    yield = &current_yield;
    active_context = this;
    isle_continuation_trampoline(&frame, guarded_entry, this);
    active_context = nullptr;
    yield = nullptr;
    if (pending_exception) {
        auto failure = std::move(pending_exception);
        std::rethrow_exception(failure);
    }
    if (reported_failure) {
        throw ReportedFailure{};
    }
}

extern "C" IsleContinuationResult
isle_continuation_domain_create(IsleContinuationDomain** output) {
    if (output == nullptr) {
        return ISLE_CONT_INVALID_ARGUMENT;
    }
    // A callback cannot create a separately owned domain on its worker.
    if (active_context != nullptr) {
        return error(active_context->domain, ISLE_CONT_INVALID_STATE,
                     "a callback cannot construct a continuation domain");
    }
    try {
        *output = new IsleContinuationDomain;
        return ISLE_CONT_OK;
    } catch (...) {
        return ISLE_CONT_RESOURCE_FAILURE;
    }
}

extern "C" IsleContinuationResult
isle_continuation_domain_check_access(const IsleContinuationDomain* domain) {
    if (domain == nullptr) {
        return ISLE_CONT_INVALID_ARGUMENT;
    }
    return is_owner(domain) || is_callback(domain) ? ISLE_CONT_OK : ISLE_CONT_WRONG_OWNER;
}

extern "C" IsleContinuationResult
isle_continuation_domain_destroy(IsleContinuationDomain* domain) {
    if (domain == nullptr) {
        return ISLE_CONT_INVALID_ARGUMENT;
    }
    if (is_callback(domain) || (is_owner(domain) && domain->active != nullptr)) {
        return error(domain, ISLE_CONT_INVALID_STATE,
                     "an active continuation domain cannot destroy itself");
    }
    if (!is_owner(domain)) {
        return error(domain, ISLE_CONT_WRONG_OWNER,
                     "domain destruction requires the constructing owner");
    }
    while (domain->first != nullptr) {
        const auto result = owner_destroy(domain->first);
        if (result != ISLE_CONT_OK) {
            return result;
        }
    }
    delete domain;
    return ISLE_CONT_OK;
}

extern "C" IsleContinuationResult
isle_continuation_on_owner(IsleContinuationEntry entry, void* user) {
    if (entry == nullptr) return ISLE_CONT_INVALID_ARGUMENT;
    if (active_context == nullptr) return ISLE_CONT_INVALID_STATE;
    Request request{Operation::invoke,active_context->domain,active_context,entry,user,
                    nullptr,nullptr,ISLE_CONT_OK};
    return dispatch(&request);
}

extern "C" IsleContinuationResult
isle_continuation_create(IsleContinuationDomain* domain, IsleContinuationEntry entry,
                         void* user, IsleContinuation** output) {
    if (domain == nullptr || entry == nullptr || output == nullptr) {
        return error(domain, ISLE_CONT_INVALID_ARGUMENT,
                     "creation requires a domain, callback, and output pointer");
    }
    Request request{Operation::create, domain, nullptr, entry, user, output,
                    nullptr, ISLE_CONT_OK};
    return dispatch(&request);
}

extern "C" IsleContinuationResult
isle_continuation_resume(IsleContinuation* node) {
    if (node == nullptr) {
        return ISLE_CONT_INVALID_ARGUMENT;
    }
    auto* const domain = node->domain;
    if (!is_owner(domain)) {
        return error(domain, ISLE_CONT_WRONG_OWNER,
                     "only the constructing owner may dispatch a continuation");
    }
    if (domain->active != nullptr) {
        return error(domain, ISLE_CONT_INVALID_STATE,
                     "continuation callbacks cannot be dispatched recursively");
    }
    domain->active = node;
    try {
        node->process.resume();
        domain->active = nullptr;
        return ISLE_CONT_OK;
    } catch (...) {
        domain->active = nullptr;
        if (node->reported_failure) {
            report_failure(node);
            return ISLE_CONT_EXECUTION_FAILURE;
        }
        return error(domain, ISLE_CONT_EXECUTION_FAILURE,
                     "continuation execution failed; failure is retained");
    }
}

extern "C" IsleContinuationResult
isle_continuation_cancel(IsleContinuation* node) {
    if (node == nullptr) {
        return ISLE_CONT_INVALID_ARGUMENT;
    }
    Request request{Operation::cancel, node->domain, node, nullptr, nullptr,
                    nullptr, nullptr, ISLE_CONT_OK};
    return dispatch(&request);
}

extern "C" IsleContinuationResult
isle_continuation_destroy(IsleContinuation* node) {
    if (node == nullptr) {
        return ISLE_CONT_INVALID_ARGUMENT;
    }
    Request request{Operation::destroy, node->domain, node, nullptr, nullptr,
                    nullptr, nullptr, ISLE_CONT_OK};
    return dispatch(&request);
}

extern "C" IsleContinuationResult
isle_continuation_state(IsleContinuation* node, IsleContinuationState* output) {
    if (node == nullptr || output == nullptr) {
        return error(node == nullptr ? nullptr : node->domain, ISLE_CONT_INVALID_ARGUMENT,
                     "state requires a continuation and output pointer");
    }
    Request request{Operation::state, node->domain, node, nullptr, nullptr,
                    nullptr, output, ISLE_CONT_OK};
    return dispatch(&request);
}

extern "C" IsleContinuationResult
isle_continuation_wait(IsleContinuationDomain* domain, uint64_t resumes) {
    const auto result = require_callback(domain);
    if (result != ISLE_CONT_OK) {
        return result;
    }
    try {
        active_context->yield->wait(resumes);
        return ISLE_CONT_OK;
    } catch (...) {
        active_context->pending_exception = std::current_exception();
    }
    isle_continuation_abort(&active_context->frame);
}

extern "C" IsleContinuationResult
isle_continuation_finish(IsleContinuationDomain* domain) {
    const auto result = require_callback(domain);
    if (result != ISLE_CONT_OK) {
        return result;
    }
    try {
        active_context->yield->finish();
    } catch (...) {
        active_context->pending_exception = std::current_exception();
    }
    isle_continuation_abort(&active_context->frame);
}

extern "C" IsleContinuationResult
isle_continuation_fail(IsleContinuationDomain* domain, const char* message) {
    const auto result = require_callback(domain);
    if (result != ISLE_CONT_OK) {
        return result;
    }
    if (message == nullptr) {
        return error(domain, ISLE_CONT_INVALID_ARGUMENT, "callback failure requires a diagnostic");
    }
    copy_diagnostic(active_context->failure_message, message);
    active_context->reported_failure = true;
    report_failure(active_context);
    isle_continuation_abort(&active_context->frame);
}

extern "C" const char*
isle_continuation_error(const IsleContinuationDomain* domain) {
    if (domain == nullptr) {
        return "no continuation domain";
    }
    if (!is_owner(domain) && !is_callback(domain)) {
        return "continuation diagnostics require the owner or its active callback";
    }
    return domain->error.load(std::memory_order_relaxed);
}

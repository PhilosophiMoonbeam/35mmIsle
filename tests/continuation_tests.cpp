#include "35mmisle/continuation.h"
#include "continuation_fixture.h"

#include <array>
#include <atomic>
#include <cstring>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {

int failures = 0;

void check(bool condition, const char* description) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << description << '\n';
    }
}

struct Domain {
    IsleContinuationDomain* value = nullptr;
    Domain() {
        if (isle_continuation_domain_create(&value) != ISLE_CONT_OK) {
            throw std::runtime_error("cannot construct test continuation domain");
        }
    }
    ~Domain() {
        if (value != nullptr) {
            check(isle_continuation_domain_destroy(value) == ISLE_CONT_OK,
                  "domain scope joins and releases its accepted continuations");
        }
    }
    Domain(const Domain&) = delete;
    Domain& operator=(const Domain&) = delete;
    void shutdown() {
        check(isle_continuation_domain_destroy(value) == ISLE_CONT_OK,
              "explicit domain shutdown joins every accepted continuation");
        value = nullptr;
    }
};

struct WorkerExit {
    std::atomic<int>* count = nullptr;
    ~WorkerExit() {
        if (count != nullptr) {
            count->fetch_add(1, std::memory_order_relaxed);
        }
    }
};

// The C fixture calls this and returns before suspending. Its lifetime witness
// is thread-local, never a C++ automatic object above the C saved jump frame.
void witness_worker(void* user) {
    thread_local WorkerExit witness;
    witness.count = static_cast<std::atomic<int>*>(user);
}

void attach_witness(IsleCFixture& fixture, std::atomic<int>& exits) {
    fixture.enter_hook = witness_worker;
    fixture.hook_user = &exits;
}

void create(Domain& domain, IsleCFixture& fixture, IsleContinuationEntry entry) {
    fixture.domain = domain.value;
    if (isle_continuation_create(domain.value, entry, &fixture, &fixture.self) != ISLE_CONT_OK) {
        throw std::runtime_error("cannot construct compiled-C test continuation");
    }
}

IsleContinuationState state(IsleContinuation* continuation) {
    IsleContinuationState result = ISLE_CONT_FAILED;
    check(isle_continuation_state(continuation, &result) == ISLE_CONT_OK,
          "owner can inspect continuation state");
    return result;
}

void trace(const IsleCFixture& fixture, std::initializer_list<int> expected,
           const char* description) {
    bool equal = fixture.trace_count == static_cast<int>(expected.size());
    int index = 0;
    for (const int value : expected) {
        if (index >= fixture.trace_count || fixture.trace[index] != value) {
            equal = false;
        }
        ++index;
    }
    check(equal, description);
}

void arguments_and_owner_context() {
    Domain domain;
    IsleCFixture fixture{};
    create(domain, fixture, isle_c_fixture_immediate_finish);
    IsleContinuation* untouched = fixture.self;
    check(isle_continuation_domain_create(nullptr) == ISLE_CONT_INVALID_ARGUMENT,
          "domain creation rejects a missing output");
    check(isle_continuation_domain_destroy(nullptr) == ISLE_CONT_INVALID_ARGUMENT &&
          isle_continuation_domain_check_access(nullptr) == ISLE_CONT_INVALID_ARGUMENT,
          "domain operations reject a missing domain");
    check(isle_continuation_domain_check_access(domain.value) == ISLE_CONT_OK,
          "constructing owner passes the side-effect-free access check");
    check(isle_continuation_create(domain.value, nullptr, nullptr, &untouched) == ISLE_CONT_INVALID_ARGUMENT &&
          untouched == fixture.self,
          "failed creation never publishes a replacement output");
    check(isle_continuation_create(domain.value, isle_c_fixture_nested, &fixture, nullptr) == ISLE_CONT_INVALID_ARGUMENT &&
          isle_continuation_create(nullptr, isle_c_fixture_nested, &fixture, &untouched) == ISLE_CONT_INVALID_ARGUMENT,
          "creation requires both its domain and output");
    check(isle_continuation_resume(nullptr) == ISLE_CONT_INVALID_ARGUMENT &&
          isle_continuation_cancel(nullptr) == ISLE_CONT_INVALID_ARGUMENT &&
          isle_continuation_destroy(nullptr) == ISLE_CONT_INVALID_ARGUMENT &&
          isle_continuation_state(fixture.self, nullptr) == ISLE_CONT_INVALID_ARGUMENT,
          "handle operations reject missing arguments");
    check(isle_continuation_wait(domain.value, 0) == ISLE_CONT_INVALID_STATE &&
          isle_continuation_finish(domain.value) == ISLE_CONT_INVALID_STATE &&
          isle_continuation_fail(domain.value, "not a callback") == ISLE_CONT_INVALID_STATE,
          "owner cannot terminate or wait without an active callback");
    check(isle_continuation_wait(nullptr, 1) == ISLE_CONT_INVALID_ARGUMENT &&
          isle_continuation_finish(nullptr) == ISLE_CONT_INVALID_ARGUMENT &&
          isle_continuation_fail(nullptr, "missing domain") == ISLE_CONT_INVALID_ARGUMENT,
          "termination operations reject a missing domain");
    const char* const diagnostic = isle_continuation_error(domain.value);
    check(isle_continuation_domain_check_access(domain.value) == ISLE_CONT_OK &&
          diagnostic == isle_continuation_error(domain.value),
          "successful access checks do not replace diagnostics");
    check(std::strlen(isle_continuation_error(nullptr)) != 0 &&
          state(fixture.self) == ISLE_CONT_READY && fixture.entries == 0,
          "invalid-context calls leave a real ready callback untouched");
}

void compiled_c_nested_waits() {
    Domain domain;
    IsleCFixture fixture{};
    create(domain, fixture, isle_c_fixture_nested);
    check(state(fixture.self) == ISLE_CONT_READY && fixture.entries == 0,
          "compiled-C callback is parked before its first dispatch");
    check(isle_continuation_resume(fixture.self) == ISLE_CONT_OK &&
          state(fixture.self) == ISLE_CONT_WAITING,
          "first dispatch enters actual nested C frames");
    trace(fixture, {1, 2, 3}, "C wait zero stays inline before the nested wait three");
    for (int tick = 0; tick < 2; ++tick) {
        check(isle_continuation_resume(fixture.self) == ISLE_CONT_OK &&
              state(fixture.self) == ISLE_CONT_WAITING,
              "nested C wait three remains parked for its first two subsequent resumes");
        trace(fixture, {1, 2, 3}, "no C user statement runs during a wait countdown");
    }
    check(isle_continuation_resume(fixture.self) == ISLE_CONT_OK &&
          state(fixture.self) == ISLE_CONT_WAITING,
          "third subsequent resume reaches the next C wait");
    trace(fixture, {1, 2, 3, 4}, "C nested stack continues at the exact three-tick boundary");
    check(isle_continuation_resume(fixture.self) == ISLE_CONT_OK &&
          state(fixture.self) == ISLE_CONT_COMPLETED,
          "C wait one returns exactly on the next owner dispatch");
    trace(fixture, {1, 2, 3, 4, 5, 6}, "both original C frames return in order");
    check(fixture.failures == 0 && fixture.resource == 34 && fixture.entries == 1,
          "C nested values, modifications, and stack addresses survive all suspensions");
    check(isle_continuation_resume(fixture.self) == ISLE_CONT_OK &&
          isle_continuation_cancel(fixture.self) == ISLE_CONT_OK &&
          state(fixture.self) == ISLE_CONT_COMPLETED && fixture.entries == 1,
          "terminal C dispatch and cancellation do not restart or relabel completion");
}

void compiled_c_finish_fence() {
    std::atomic<int> exits{0};
    Domain domain;
    IsleCFixture nested{};
    attach_witness(nested, exits);
    create(domain, nested, isle_c_fixture_finish);
    check(isle_continuation_resume(nested.self) == ISLE_CONT_OK &&
          state(nested.self) == ISLE_CONT_WAITING && exits.load() == 0,
          "nested C finish first parks its original stack");
    check(isle_continuation_resume(nested.self) == ISLE_CONT_OK &&
          state(nested.self) == ISLE_CONT_COMPLETED && exits.load() == 1,
          "C finish crosses its C-only fence and joins as completed");
    check(nested.after_retirement == 0 && nested.failures == 0,
          "successful finish returns to neither the nested nor outer C caller");
    IsleCFixture immediate{};
    attach_witness(immediate, exits);
    create(domain, immediate, isle_c_fixture_immediate_finish);
    check(isle_continuation_resume(immediate.self) == ISLE_CONT_OK &&
          state(immediate.self) == ISLE_CONT_COMPLETED && exits.load() == 2 &&
          immediate.after_retirement == 0 && immediate.entries == 1,
          "C finish also terminates safely before any wait");
}

void callback_creates_centrally_owned_child() {
    std::atomic<int> exits{0};
    Domain domain;
    Domain other;
    IsleCFixture parent{};
    IsleCFixture child{};
    child.domain = domain.value;
    parent.other_domain = other.value;
    parent.child_data = &child;
    attach_witness(parent, exits);
    attach_witness(child, exits);
    create(domain, parent, isle_c_fixture_creator);
    check(isle_continuation_resume(parent.self) == ISLE_CONT_OK &&
          state(parent.self) == ISLE_CONT_WAITING && parent.failures == 0,
          "compiled C creation/state/lifetime requests synchronously rendezvous with the owner");
    trace(parent, {30, 31}, "owner rendezvous returns only at the callback's actual wait");
    check(parent.child != nullptr && state(parent.child) == ISLE_CONT_READY && child.entries == 0,
          "callback creation publishes a real ready child without dispatching it");
    check(isle_continuation_resume(parent.self) == ISLE_CONT_OK &&
          state(parent.self) == ISLE_CONT_WAITING,
          "creation and state rendezvous consume no ticks of C wait two");
    trace(parent, {30, 31}, "one subsequent parent tick still runs no C callback code");
    check(isle_continuation_resume(parent.child) == ISLE_CONT_OK &&
          state(parent.child) == ISLE_CONT_WAITING && child.entries == 1,
          "central owner can later dispatch its callback-created child");
    trace(child, {40}, "child starts only at its independent owner dispatch");
    check(isle_continuation_resume(parent.self) == ISLE_CONT_OK &&
          state(parent.self) == ISLE_CONT_COMPLETED && parent.after_retirement == 0,
          "second subsequent parent tick finishes its original C callback");
    trace(parent, {30, 31, 32}, "parent preserves wait count despite all owner requests");
    check(isle_continuation_resume(parent.child) == ISLE_CONT_OK &&
          state(parent.child) == ISLE_CONT_COMPLETED && child.resource == 53,
          "child's separate C local survives its own subsequent dispatch");
    trace(child, {40, 41}, "callback-created child completes through its original C frame");
    check(parent.failures == 0 && child.failures == 0 && exits.load() == 2,
          "rejected self/foreign/nested operations preserve both workers and their eventual joins");
}

void compiled_c_owner_cancellation() {
    std::atomic<int> exits{0};
    Domain domain;
    IsleCFixture parked{};
    parked.resource = 37;
    attach_witness(parked, exits);
    create(domain, parked, isle_c_fixture_park);
    check(isle_continuation_resume(parked.self) == ISLE_CONT_OK &&
          state(parked.self) == ISLE_CONT_WAITING,
          "nested C callback parks with a maximum-length countdown");
    check(isle_continuation_cancel(parked.self) == ISLE_CONT_OK &&
          state(parked.self) == ISLE_CONT_CANCELLED && exits.load() == 1,
          "owner cancellation bypasses C frames and joins before returning");
    check(parked.after_retirement == 0 && parked.resource == 37,
          "cancellation never returns into retired nested C code");
    parked.resource = 93;
    check(isle_continuation_resume(parked.self) == ISLE_CONT_OK &&
          state(parked.self) == ISLE_CONT_CANCELLED && parked.resource == 93,
          "cancelled continuation cannot touch subsequently reused data");
    check(isle_continuation_destroy(parked.self) == ISLE_CONT_OK,
          "cancelled C handle releases its already joined worker");
    IsleCFixture ready{};
    create(domain, ready, isle_c_fixture_park);
    check(isle_continuation_cancel(ready.self) == ISLE_CONT_OK &&
          state(ready.self) == ISLE_CONT_CANCELLED && ready.entries == 0,
          "cancellation before first entry joins without executing C code");
}

void callback_retires_other_before_reuse() {
    for (int operation = 0; operation < 2; ++operation) {
        std::atomic<int> exits{0};
        Domain domain;
        IsleCFixture victim{};
        IsleCFixture caller{};
        victim.resource = 47;
        caller.operation = operation;
        caller.victim_data = &victim;
        attach_witness(victim, exits);
        attach_witness(caller, exits);
        create(domain, victim, isle_c_fixture_park);
        caller.victim = victim.self;
        create(domain, caller, isle_c_fixture_retire_other);
        check(isle_continuation_resume(victim.self) == ISLE_CONT_OK &&
              state(victim.self) == ISLE_CONT_WAITING,
              "other real C continuation is suspended before callback retirement");
        check(isle_continuation_resume(caller.self) == ISLE_CONT_OK &&
              state(caller.self) == ISLE_CONT_WAITING && caller.victim == nullptr,
              "compiled C callback synchronously cancels/destroys its other parked process");
        trace(caller, {20, 21}, "callback retirement reaches its next real wait without another dispatch");
        check(exits.load() == 1 && victim.after_retirement == 0 && victim.resource == 71,
              "retired worker is joined before its callback-owned data is reused");
        check(isle_continuation_resume(caller.self) == ISLE_CONT_OK &&
              state(caller.self) == ISLE_CONT_COMPLETED && exits.load() == 2,
              "retiring callback resumes later with its one-tick wait preserved");
        trace(caller, {20, 21, 22}, "retiring callback continues only after its later owner dispatch");
        check(caller.failures == 0 && victim.failures == 0 && victim.resource == 71,
              "both direct destruction and cancellation-then-destruction prevent retired C writes");
    }
}

void compiled_c_wrong_owner_boundaries() {
    Domain domain;
    IsleCFixture fixture{};
    fixture.resource = 29;
    create(domain, fixture, isle_c_fixture_park);
    std::thread before([&] { isle_c_fixture_wrong_owner(&fixture); });
    before.join();
    check(fixture.failures == 0 && state(fixture.self) == ISLE_CONT_READY && fixture.entries == 0,
          "compiled-C foreign-thread calls reject all owner operations before entry");
    check(isle_continuation_resume(fixture.self) == ISLE_CONT_OK &&
          state(fixture.self) == ISLE_CONT_WAITING,
          "rejected foreign calls leave later valid dispatch intact");
    std::thread after([&] { isle_c_fixture_wrong_owner(&fixture); });
    after.join();
    check(fixture.failures == 0 && state(fixture.self) == ISLE_CONT_WAITING &&
          fixture.after_retirement == 0,
          "foreign wait/finish/fail cannot jump into a parked worker's saved C frame");
    check(isle_continuation_cancel(fixture.self) == ISLE_CONT_OK &&
          state(fixture.self) == ISLE_CONT_CANCELLED,
          "owner retains cancellation authority after wrong-owner errors");
}

void compiled_c_failure_fence() {
    std::atomic<int> exits{0};
    Domain domain;
    IsleCFixture fixture{};
    attach_witness(fixture, exits);
    create(domain, fixture, isle_c_fixture_fail);
    check(isle_continuation_resume(fixture.self) == ISLE_CONT_OK &&
          state(fixture.self) == ISLE_CONT_WAITING,
          "compiled-C explicit failure first preserves its nested local diagnostic");
    trace(fixture, {50}, "failing C callback reaches its real suspension");
    check(isle_continuation_resume(fixture.self) == ISLE_CONT_EXECUTION_FAILURE &&
          state(fixture.self) == ISLE_CONT_FAILED && exits.load() == 1,
          "C fail terminates through the C fence and retains a joined failed worker");
    check(fixture.after_retirement == 0 && fixture.failures == 0,
          "explicit failure never returns into either retired C frame");
    check(std::strcmp(isle_continuation_error(domain.value), "compiled C resource failure 77") == 0,
          "explicit C failure copies its stack-local diagnostic before discarding C state");
    check(isle_continuation_resume(fixture.self) == ISLE_CONT_EXECUTION_FAILURE &&
          isle_continuation_cancel(fixture.self) == ISLE_CONT_OK &&
          state(fixture.self) == ISLE_CONT_FAILED && fixture.entries == 1,
          "repeated dispatch and cancellation preserve explicit failure rather than completing it");
    check(isle_continuation_destroy(fixture.self) == ISLE_CONT_OK &&
          std::strcmp(isle_continuation_error(domain.value), "compiled C resource failure 77") == 0,
          "copied diagnostic remains valid after successful failed-handle destruction");
    IsleCFixture later{};
    create(domain, later, isle_c_fixture_immediate_finish);
    check(isle_continuation_resume(later.self) == ISLE_CONT_OK &&
          state(later.self) == ISLE_CONT_COMPLETED && later.after_retirement == 0 &&
          std::strcmp(isle_continuation_error(domain.value), "compiled C resource failure 77") == 0,
          "a later successful C callback neither inherits failure nor manufactures a diagnostic event");
}

struct EntryFailure { int marker; };
struct EntryFailureData { int entries = 0; int destroyed = 0; };

void throwing_entry(void* user) {
    auto& data = *static_cast<EntryFailureData*>(user);
    struct Automatic {
        int& destroyed;
        ~Automatic() { ++destroyed; }
    } lifetime{data.destroyed};
    ++data.entries;
    throw EntryFailure{71};
}

void cpp_entry_failure_boundary() {
    Domain domain;
    EntryFailureData data;
    IsleContinuation* continuation = nullptr;
    check(isle_continuation_create(domain.value, throwing_entry, &data, &continuation) == ISLE_CONT_OK,
          "C interface also accepts a direct C++ consumer entry");
    check(isle_continuation_resume(continuation) == ISLE_CONT_EXECUTION_FAILURE &&
          state(continuation) == ISLE_CONT_FAILED && data.entries == 1 && data.destroyed == 1,
          "entry adapter catches C++ failure before returning through the C trampoline");
    check(isle_continuation_resume(continuation) == ISLE_CONT_EXECUTION_FAILURE &&
          isle_continuation_cancel(continuation) == ISLE_CONT_OK &&
          state(continuation) == ISLE_CONT_FAILED && data.entries == 1,
          "C++ entry failure is retained and never escapes any public C API");
}

void shutdown_joins_all_accepted_processes() {
    std::atomic<int> exits{0};
    Domain domain;
    std::array<IsleCFixture, 4> fixtures{};
    IsleCFixture child{};
    child.domain = domain.value;
    fixtures[3].child_data = &child;
    attach_witness(child, exits);
    for (std::size_t i = 0; i < fixtures.size(); ++i) {
        fixtures[i].resource = static_cast<int>(i) + 81;
        attach_witness(fixtures[i], exits);
        create(domain, fixtures[i], i == 3 ? isle_c_fixture_creator : isle_c_fixture_park);
    }
    for (const std::size_t i : {std::size_t{0}, std::size_t{1}, std::size_t{3}}) {
        check(isle_continuation_resume(fixtures[i].self) == ISLE_CONT_OK &&
              state(fixtures[i].self) == ISLE_CONT_WAITING,
              "shutdown fixture parks each selected real C worker");
    }
    check(state(fixtures[2].self) == ISLE_CONT_READY &&
          fixtures[3].child != nullptr && state(fixtures[3].child) == ISLE_CONT_READY &&
          exits.load() == 0,
          "domain accepts both initial and callback-created ready workers before shutdown");
    domain.shutdown();
    check(exits.load() == 3 && fixtures[2].entries == 0 && child.entries == 0,
          "shutdown joins all entered workers and cancels both unentered accepted workers");
    for (auto& fixture : fixtures) {
        check(fixture.failures == 0 && fixture.after_retirement == 0,
              "domain destruction never returns to retired compiled C callbacks");
        fixture.resource = 101;
    }
    check(child.failures == 0 && child.after_retirement == 0 && exits.load() == 3,
          "all callback storage can be reused immediately after complete domain shutdown");
}

struct OwnerServiceData {
    IsleContinuationDomain* domain;
    std::thread::id owner;
    bool service_on_owner = true, saw_first = false, saw_failure = false, saw_second = false;
    bool after_wait = false;
    int value = 0;
};
void owner_increment(void* payload) {
    auto& data = *static_cast<OwnerServiceData*>(payload);
    data.service_on_owner &= std::this_thread::get_id() == data.owner;
    data.value += 11;
}
void owner_throws(void*) { throw 37; }
void owner_service_entry(void* payload) {
    auto& data = *static_cast<OwnerServiceData*>(payload);
    data.saw_first = isle_continuation_on_owner(owner_increment,&data) == ISLE_CONT_OK && data.value == 11;
    data.saw_failure = isle_continuation_on_owner(owner_throws,&data) == ISLE_CONT_EXECUTION_FAILURE && data.value == 11;
    data.saw_second = isle_continuation_on_owner(owner_increment,&data) == ISLE_CONT_OK && data.value == 22;
    if (isle_continuation_wait(data.domain,1) != ISLE_CONT_OK) return;
    data.after_wait = true;
}
void callback_owner_service_fence() {
    Domain domain;
    OwnerServiceData data{domain.value,std::this_thread::get_id()};
    check(isle_continuation_on_owner(owner_increment,&data) == ISLE_CONT_INVALID_STATE && data.value == 0,
          "owner service rejects a call outside an active continuation without executing it");
    std::atomic<IsleContinuationResult> foreign{ISLE_CONT_OK};
    std::thread unrelated([&] { foreign = isle_continuation_on_owner(owner_increment,&data); });
    unrelated.join();
    check(foreign == ISLE_CONT_INVALID_STATE && data.value == 0,
          "an unrelated thread cannot dispatch an owner service");
    IsleContinuation* callback = nullptr;
    check(isle_continuation_create(domain.value,owner_service_entry,&data,&callback) == ISLE_CONT_OK,
          "create real callback for owner-affine endpoint work");
    check(isle_continuation_resume(callback) == ISLE_CONT_OK && state(callback) == ISLE_CONT_WAITING &&
          data.service_on_owner && data.saw_first && data.saw_failure && data.saw_second && !data.after_wait,
          "real owner services and handled failure finish within one resume before the actual GAME wait");
    check(isle_continuation_resume(callback) == ISLE_CONT_OK && state(callback) == ISLE_CONT_COMPLETED && data.after_wait,
          "owner handoffs consume no extra gameplay resume or wait");
}

struct ScheduledWorkerData {
    IsleContinuationDomain* sender_domain;
    IsleContinuationDomain* worker_domain;
    IsleContinuation* sender = nullptr;
    IsleContinuation* worker = nullptr;
    int sender_phase = 0, worker_phase = 0;
    IsleContinuationResult dispatched = ISLE_CONT_OK, self_resume = ISLE_CONT_OK;
    bool ordinary_rejected = false, worker_saw_parked_sender = false;
};
void scheduled_worker_entry(void* raw) {
    auto& data=*static_cast<ScheduledWorkerData*>(raw);
    data.worker_saw_parked_sender=data.sender_phase==1;
    data.worker_phase=1;
    if(isle_continuation_wait(data.worker_domain,1)!=ISLE_CONT_OK)return;
    data.worker_phase=2;
}
void dispatch_independent_worker(void* raw) {
    auto& data=*static_cast<ScheduledWorkerData*>(raw);
    data.self_resume=isle_continuation_resume(data.sender);
    data.dispatched=isle_continuation_resume(data.worker);
}
void scheduling_sender_entry(void* raw) {
    auto& data=*static_cast<ScheduledWorkerData*>(raw);
    data.sender_phase=1;
    if(isle_continuation_on_owner(dispatch_independent_worker,&data)!=ISLE_CONT_OK)return;
    data.ordinary_rejected=data.dispatched==ISLE_CONT_EXECUTION_FAILURE && data.worker_phase==0;
    if(isle_continuation_on_owner_dispatch(dispatch_independent_worker,&data)!=ISLE_CONT_OK)return;
    data.sender_phase=2;
    if(isle_continuation_wait(data.sender_domain,1)!=ISLE_CONT_OK)return;
    data.sender_phase=3;
}
void independent_worker_scheduler_boundary() {
    Domain sender_domain,worker_domain;
    ScheduledWorkerData data{sender_domain.value,worker_domain.value};
    check(isle_continuation_on_owner_dispatch(dispatch_independent_worker,&data)==ISLE_CONT_INVALID_STATE &&
          data.worker_phase==0,"scheduler rejects calls outside an active Source callback");
    check(isle_continuation_create(worker_domain.value,scheduled_worker_entry,&data,&data.worker)==ISLE_CONT_OK &&
          isle_continuation_create(sender_domain.value,scheduling_sender_entry,&data,&data.sender)==ISLE_CONT_OK,
          "create independent real worker and sending Source continuations");
    check(isle_continuation_resume(data.sender)==ISLE_CONT_OK &&
          state(data.sender)==ISLE_CONT_WAITING && state(data.worker)==ISLE_CONT_WAITING &&
          data.ordinary_rejected && data.dispatched==ISLE_CONT_OK &&
          data.self_resume==ISLE_CONT_INVALID_STATE && data.worker_saw_parked_sender &&
          data.sender_phase==2 && data.worker_phase==1,
          "only explicit scheduling dispatches the worker before sender continuation, without recursive self-resume");
    check(isle_continuation_resume(data.sender)==ISLE_CONT_OK &&
          state(data.sender)==ISLE_CONT_COMPLETED && data.sender_phase==3 && data.worker_phase==1,
          "worker dispatch consumes no extra sender wait and invents no worker resume");
    check(isle_continuation_resume(data.worker)==ISLE_CONT_OK &&
          state(data.worker)==ISLE_CONT_COMPLETED && data.worker_phase==2,
          "independent worker retains its real suspended stack until its own next resume");
}

} // namespace

int main() {
    try {
        arguments_and_owner_context();
        compiled_c_nested_waits();
        compiled_c_finish_fence();
        callback_creates_centrally_owned_child();
        compiled_c_owner_cancellation();
        callback_retires_other_before_reuse();
        compiled_c_wrong_owner_boundaries();
        compiled_c_failure_fence();
        cpp_entry_failure_boundary();
        shutdown_joins_all_accepted_processes();
        callback_owner_service_fence();
        independent_worker_scheduler_boundary();
    } catch (const std::exception& exception) {
        check(false, exception.what());
    } catch (...) {
        check(false, "unexpected exception escaped a public C continuation boundary");
    }
    if (failures != 0) {
        std::cerr << failures << " continuation checks failed\n";
        return 1;
    }
    std::cout << "continuation checks passed\n";
    return 0;
}

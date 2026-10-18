#include "rsp_render.hpp"

#include "context.hpp"
#include "device.hpp"
#include "rdp_device.hpp"

#include <atomic>
#include <bit>
#include <cstdint>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace isle::rsp {
namespace {
constexpr std::size_t rdram_bytes = 8u << 20;
constexpr std::size_t hidden_bytes = rdram_bytes / 2;

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class ErrorSink final : public RDP::ValidationInterface {
public:
    void record(const char* message) {
        std::lock_guard lock(mutex_);
        if (errors_.fetch_add(1) == 0)
            first_ = message ? message : "no backend diagnostic";
    }
    void report_rdp_crash(RDP::ValidationError, const char* message) override {
        record(message);
    }
    void require_clean() {
        std::lock_guard lock(mutex_);
        if (errors_.load() != 0)
            throw std::runtime_error("canonical RSP/RDP backend error: " + first_);
    }
private:
    std::mutex mutex_;
    std::atomic<unsigned> errors_{0};
    std::string first_;
};

void initialize_context(Vulkan::Context& context, bool cpu_only) {
    require(Vulkan::Context::init_loader(nullptr), "canonical Vulkan loader initialization failed");
    if (!cpu_only) {
        require(context.init_instance_and_device(nullptr, 0, nullptr, 0),
                "canonical headless Vulkan instance/device initialization failed");
        return;
    }
    require(context.init_instance(nullptr, 0), "canonical Vulkan instance initialization failed");
    std::uint32_t count = 0;
    require(vkEnumeratePhysicalDevices(context.get_instance(), &count, nullptr) == VK_SUCCESS && count != 0,
            "canonical executor found no actual Vulkan physical device");
    std::vector<VkPhysicalDevice> devices(count);
    require(vkEnumeratePhysicalDevices(context.get_instance(), &count, devices.data()) == VK_SUCCESS,
            "canonical Vulkan physical-device enumeration failed");
    for (std::uint32_t index = 0; index < count; ++index) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(devices[index], &properties);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU) {
            require(context.init_device(devices[index], VK_NULL_HANDLE, nullptr, 0),
                    "canonical executor could not initialize its actual CPU Vulkan device");
            return;
        }
    }
    throw std::runtime_error("canonical executor requires VK_PHYSICAL_DEVICE_TYPE_CPU but none exists");
}

const char* error_name(Error error) noexcept {
    switch (error) {
    case Error::none: return "none";
    case Error::invalid_task: return "invalid_task";
    case Error::address_out_of_range: return "address_out_of_range";
    case Error::execution_budget: return "execution_budget";
    case Error::malformed_packet: return "malformed_packet";
    case Error::packet_capacity: return "packet_capacity";
    case Error::sink_rejected: return "sink_rejected";
    case Error::unexpected_stop: return "unexpected_stop";
    }
    return "unknown";
}

[[noreturn]] void rsp_failure(const Result& result) {
    throw std::runtime_error(std::string("canonical actual RSP task failed: error=") + error_name(result.error) +
        " sp_status=" + std::to_string(result.sp_status) +
        " dma_reads=" + std::to_string(result.dma_reads) +
        " dma_writes=" + std::to_string(result.dma_writes) +
        " imem_writes=" + std::to_string(result.imem_writes) +
        " packets=" + std::to_string(result.packets) +
        " packet_words=" + std::to_string(result.packet_words) +
        " full_syncs=" + std::to_string(result.full_syncs));
}
} // namespace

struct CanonicalExecutor::Impl {
    enum class GpuPhase { pending, failure_needs_owner, draining, ready };
    enum class GpuWork { none, execute, drain };

    Impl(bool require_cpu, Limits requested) : limits(requested), packets(requested.packet_words) {
        context.set_notification_callback([this](const char* message) { errors.record(message); });
        initialize_context(context, require_cpu);
        require(context.get_gpu() != VK_NULL_HANDLE && context.get_device() != VK_NULL_HANDLE,
                "canonical initialization returned no actual Vulkan device");
        errors.require_clean();
        device.set_context(context);
        processor.emplace(device, nullptr, 0, rdram_bytes, hidden_bytes,
            RDP::COMMAND_PROCESSOR_FLAG_HOST_VISIBLE_HIDDEN_RDRAM_BIT);
        processor->set_validation_interface(&errors);
        require(processor->device_is_supported(),
                "canonical Vulkan device does not support pinned paraLLEl-RDP");
        processor->idle();
        require(vkDeviceWaitIdle(context.get_device()) == VK_SUCCESS,
                "canonical cold device completion failed");
        auto* memory = static_cast<std::uint32_t*>(processor->begin_read_rdram());
        require(memory != nullptr, "canonical processor-owned RDRAM mapping failed");
        require(reinterpret_cast<std::uintptr_t>(memory) % alignof(std::uint32_t) == 0,
                "canonical RDRAM mapping is not word aligned");
        words = {memory, rdram_bytes / sizeof(std::uint32_t)};
        // The pinned processor has already initialized hidden to 0x03 once,
        // during cold construction. Never redundantly fill it from the CPU or
        // reset it on a task. Caller admission initializes task-used RDRAM.
        errors.require_clean();
        machine.emplace(words, PacketBuffer::sink, &packets);
        gpu_worker = std::thread([this] { run_gpu(); });
    }
    ~Impl() {
        {
            std::lock_guard lock(gpu_mutex);
            gpu_stopping = true;
        }
        gpu_condition.notify_one();
        if (gpu_worker.joinable()) gpu_worker.join();
    }

    void run_gpu() noexcept {
        for (;;) {
            GpuWork work;
            {
                std::unique_lock lock(gpu_mutex);
                gpu_condition.wait(lock, [this] { return gpu_work != GpuWork::none || gpu_stopping; });
                if (gpu_stopping) return;
                work = std::exchange(gpu_work, GpuWork::none);
            }
            if (work == GpuWork::drain) {
                // The creating thread has already destroyed the RAM-borrowing
                // JIT. Keep all backend dependencies alive through this drain.
                processor.reset();
                completed_rdram = completed_hidden = nullptr;
                {
                    std::lock_guard lock(gpu_mutex);
                    gpu_phase.store(GpuPhase::ready, std::memory_order_release);
                }
                gpu_condition.notify_one();
                continue;
            }
            auto phase = GpuPhase::ready;
            try {
                errors.require_clean();
                // CPU preparation and the real RSP have ceased all RAM access.
                // Packets remain immutable until completed() retires this token.
                for (const auto& packet : packets.packets())
                    processor->enqueue_command(static_cast<unsigned>(packet.count),
                                               packets.words().data() + packet.offset);
                processor->idle();
                require(vkDeviceWaitIdle(context.get_device()) == VK_SUCCESS,
                        "canonical Vulkan device completion failed");
                errors.require_clean();
                completed_rdram = static_cast<const std::byte*>(processor->begin_read_rdram());
                completed_hidden = static_cast<const std::byte*>(processor->begin_read_hidden_rdram());
                require(completed_rdram == reinterpret_cast<const std::byte*>(words.data()) &&
                        completed_hidden, "canonical completed memory mapping failed or changed address");
                errors.require_clean();
            } catch (...) {
                gpu_failure = std::current_exception();
                // Stop using the processor before requesting owner-side JIT
                // retirement. Failure is not ready until the later drain job.
                phase = GpuPhase::failure_needs_owner;
            }
            {
                std::lock_guard lock(gpu_mutex);
                gpu_phase.store(phase, std::memory_order_release);
            }
            gpu_condition.notify_one();
        }
    }

    void require_creator() const {
        require(creator == std::this_thread::get_id(),
                "canonical executor must be used on its creating thread");
    }
    void require_creator_noexcept() const noexcept {
        if (creator != std::this_thread::get_id()) std::terminate();
    }
    void begin_failure_drain() noexcept {
        require_creator_noexcept();
        poisoned = true;
        // Only the owner may touch the shared upstream JIT globals. The worker
        // published failure_needs_owner after leaving all processor accesses,
        // and cannot destroy its RAM until this owner-to-worker handoff.
        machine.reset();
        {
            std::lock_guard lock(gpu_mutex);
            gpu_phase.store(GpuPhase::draining, std::memory_order_release);
            gpu_work = GpuWork::drain;
        }
        gpu_condition.notify_one();
    }

    void wait_for_gpu() {
        require_creator();
        for (;;) {
            const auto phase = gpu_phase.load(std::memory_order_acquire);
            if (phase == GpuPhase::ready) return;
            if (phase == GpuPhase::failure_needs_owner) {
                begin_failure_drain();
                continue;
            }
            std::unique_lock lock(gpu_mutex);
            gpu_condition.wait(lock, [this] {
                const auto phase = gpu_phase.load(std::memory_order_acquire);
                return phase == GpuPhase::ready || phase == GpuPhase::failure_needs_owner;
            });
        }
    }

    void poison() noexcept {
        poisoned = true;
        // The JIT retains the mapped RAM pointer. Destroy it BEFORE the actual
        // processor allocation, also on failure/cancellation, never afterwards.
        machine.reset();
        processor.reset();
        words = {};
    }

    // Reverse destruction preserves diagnostics/Vulkan dependencies through the
    // processor's drain and destroys the RAM-borrowing machine before its owner.
    const std::thread::id creator = std::this_thread::get_id();
    Limits limits;
    PacketBuffer packets;
    Result result{};
    ErrorSink errors;
    Vulkan::Context context;
    Vulkan::Device device;
    std::optional<RDP::CommandProcessor> processor;
    std::optional<Machine> machine;
    std::span<std::uint32_t> words;
    bool preparing = false;
    bool executing = false;
    bool leased = false;
    bool poisoned = false;
    std::mutex gpu_mutex;
    std::condition_variable gpu_condition;
    std::thread gpu_worker;
    std::atomic<GpuPhase> gpu_phase{GpuPhase::pending};
    GpuWork gpu_work = GpuWork::none;
    bool gpu_stopping = false;
    std::exception_ptr gpu_failure;
    const std::byte *completed_rdram = nullptr, *completed_hidden = nullptr;
    std::uint64_t next_token = 0, active_token = 0;
};

CanonicalExecutor::CanonicalExecutor(bool require_cpu, Limits limits) {
    if constexpr (std::endian::native != std::endian::little)
        throw std::runtime_error("canonical RSP/RDP requires a little-endian word-swapped host");
    impl_ = std::make_unique<Impl>(require_cpu, limits);
}

CanonicalExecutor::~CanonicalExecutor() {
    impl_->require_creator_noexcept();
    if (impl_->preparing || impl_->leased) std::terminate();
    if (impl_->executing) {
        try { retire(impl_->active_token); }
        catch (...) { /* Retire already drained/poisoned; destruction cannot report a frame. */ }
    }
}

CanonicalExecutor::PreparationLease::PreparationLease(Impl& owner) noexcept : owner_(&owner) {}
CanonicalExecutor::PreparationLease::~PreparationLease() { cancel(); }

CanonicalExecutor::PreparationLease::PreparationLease(PreparationLease&& other) noexcept {
    if (other.owner_) other.owner_->require_creator_noexcept();
    owner_ = std::exchange(other.owner_, nullptr);
}

CanonicalExecutor::PreparationLease&
CanonicalExecutor::PreparationLease::operator=(PreparationLease&& other) noexcept {
    if (owner_) owner_->require_creator_noexcept();
    if (other.owner_) other.owner_->require_creator_noexcept();
    if (this != &other) {
        cancel();
        owner_ = std::exchange(other.owner_, nullptr);
    }
    return *this;
}

CanonicalExecutor::PreparationLease::operator bool() const noexcept {
    if (owner_) owner_->require_creator_noexcept();
    return owner_ != nullptr;
}

std::span<std::uint32_t> CanonicalExecutor::PreparationLease::words() const noexcept {
    if (!owner_) return {};
    owner_->require_creator_noexcept();
    if (!owner_->preparing || owner_->executing || owner_->leased) std::terminate();
    return owner_->words;
}

void CanonicalExecutor::PreparationLease::cancel() noexcept {
    if (!owner_) return;
    owner_->require_creator_noexcept();
    if (!owner_->preparing || owner_->executing || owner_->leased) std::terminate();
    owner_->poison();
    owner_->preparing = false;
    owner_ = nullptr;
}

CanonicalExecutor::MemoryLease::MemoryLease(
    Impl& owner, std::span<const std::byte> rdram, std::span<const std::byte> hidden) noexcept
    : owner_(&owner), rdram_(rdram), hidden_(hidden) {}

CanonicalExecutor::MemoryLease::~MemoryLease() { release(); }

CanonicalExecutor::MemoryLease::MemoryLease(MemoryLease&& other) noexcept {
    if (other.owner_) other.owner_->require_creator_noexcept();
    owner_ = std::exchange(other.owner_, nullptr);
    rdram_ = std::exchange(other.rdram_, {});
    hidden_ = std::exchange(other.hidden_, {});
    postprocessing_ = std::exchange(other.postprocessing_, false);
}

CanonicalExecutor::MemoryLease&
CanonicalExecutor::MemoryLease::operator=(MemoryLease&& other) noexcept {
    if (owner_) owner_->require_creator_noexcept();
    if (other.owner_) other.owner_->require_creator_noexcept();
    if (this != &other) {
        release();
        owner_ = std::exchange(other.owner_, nullptr);
        rdram_ = std::exchange(other.rdram_, {});
        hidden_ = std::exchange(other.hidden_, {});
        postprocessing_ = std::exchange(other.postprocessing_, false);
    }
    return *this;
}

CanonicalExecutor::MemoryLease::operator bool() const noexcept {
    if (owner_) owner_->require_creator_noexcept();
    return owner_ != nullptr;
}

std::span<const std::byte> CanonicalExecutor::MemoryLease::rdram() const noexcept {
    if (owner_) owner_->require_creator_noexcept();
    return rdram_;
}

std::span<const std::byte> CanonicalExecutor::MemoryLease::hidden() const noexcept {
    if (owner_) owner_->require_creator_noexcept();
    return hidden_;
}
std::span<std::uint32_t> CanonicalExecutor::MemoryLease::postprocess_words() {
    require(owner_ != nullptr,"canonical postprocess requires an active completed lease");
    owner_->require_creator();
    require(owner_->leased && !owner_->preparing && !owner_->executing && !owner_->poisoned && !postprocessing_,
            "canonical completed CPU write phase is already active or invalid");
    postprocessing_ = true;
    return owner_->words;
}

void CanonicalExecutor::MemoryLease::finish_postprocess() {
    require(owner_ != nullptr,"canonical postprocess completion requires an active lease");
    owner_->require_creator();
    require(owner_->leased && !owner_->preparing && !owner_->executing && postprocessing_,
            "canonical completed CPU write phase is not active");
    try {
        // The actual GPU/readback already completed. Do not remap/invalidate
        // these CPU writes; write them back before presentation or RAM reuse.
        owner_->processor->end_write_rdram();
        owner_->errors.require_clean();
        postprocessing_ = false;
    } catch (...) {
        postprocessing_ = false;
        owner_->poison();
        rdram_ = {}; hidden_ = {};
        throw;
    }
}


const Result& CanonicalExecutor::MemoryLease::rsp_result() const {
    require(owner_ != nullptr, "canonical result lease is not active");
    owner_->require_creator();
    return owner_->result;
}

const PacketBuffer& CanonicalExecutor::MemoryLease::packets() const {
    require(owner_ != nullptr, "canonical result lease is not active");
    owner_->require_creator();
    return owner_->packets;
}

void CanonicalExecutor::MemoryLease::release() noexcept {
    if (!owner_) return;
    owner_->require_creator_noexcept();
    if (!owner_->leased || owner_->preparing || owner_->executing) std::terminate();
    if (postprocessing_) { owner_->poison(); postprocessing_ = false; }
    owner_->leased = false;
    owner_ = nullptr;
    rdram_ = {};
    hidden_ = {};
}

bool CanonicalExecutor::quiescent() const noexcept {
    impl_->require_creator_noexcept();
    return !impl_->preparing && !impl_->executing && !impl_->leased;
}

CanonicalExecutor::PreparationLease CanonicalExecutor::begin_preparation() {
    auto& state = *impl_;
    state.require_creator();
    require(!state.preparing && !state.executing && !state.leased,
            "canonical memory is already preparing, executing or leased");
    require(!state.poisoned, "canonical executor is poisoned; a new cold session is required");
    state.preparing = true;
    try {
        state.errors.require_clean();
        auto& processor = *state.processor;
        processor.idle();
        // Prior real execution is complete and its result lease retired before
        // any command-pool reuse or CPU mapping. This is the ONLY invalidation
        // before the owner and RSP write; execute never remaps those writes.
        processor.begin_frame_context();
        auto* mapped = static_cast<std::uint32_t*>(processor.begin_read_rdram());
        require(mapped == state.words.data(), "canonical RDRAM mapping changed its persistent address");
        state.errors.require_clean();
        return PreparationLease(state);
    } catch (...) {
        state.poison();
        state.preparing = false;
        throw;
    }
}

CanonicalExecutor::Token CanonicalExecutor::begin(
    PreparationLease&& preparation, const Task& task, std::span<const std::byte> boot) {
    auto& state = *impl_;
    state.require_creator();
    require(preparation.owner_ == &state && state.preparing && !state.executing && !state.leased,
            "canonical execution requires this executor's exclusive preparation lease");
    require(!state.poisoned, "canonical executor is poisoned; a new cold session is required");
    preparation.owner_ = nullptr;
    state.preparing = false;
    state.executing = true;
    try {
        state.errors.require_clean();
        state.packets.clear();
        // The native borrower has ceased access. The actual persistent RSP
        // executes on the exact processor-owned words; only packets are copied.
        state.result = state.machine->execute(task, boot, state.limits);
        if (!state.result) rsp_failure(state.result);
        require(!state.packets.rejected() && state.result.packet_words == state.packets.words().size() &&
                state.result.packets == state.packets.packets().size(),
                "canonical RSP packet evidence does not match the bounded sink");
        auto& processor = *state.processor;
        // Flush BOTH native and RSP writes before any GPU command consumes RAM.
        // No intervening read-map invalidation may discard noncoherent writes.
        processor.end_write_rdram();
        state.errors.require_clean();
        require(state.next_token != UINT64_MAX, "canonical execution token exhausted");
        state.active_token = ++state.next_token;
        state.completed_rdram = nullptr;
        state.completed_hidden = nullptr;
        state.gpu_failure = nullptr;
        state.gpu_phase.store(Impl::GpuPhase::pending, std::memory_order_relaxed);
        {
            std::lock_guard lock(state.gpu_mutex);
            state.gpu_work = Impl::GpuWork::execute;
        }
        state.gpu_condition.notify_one();
        return state.active_token;
    } catch (...) {
        // Invalid partial packet output never reaches the RDP, and no failed
        // execution publishes ready storage. Teardown drains any enqueued work.
        state.poison();
        state.executing = false;
        throw;
    }
}

bool CanonicalExecutor::poll(Token token) {
    auto& state = *impl_;
    state.require_creator();
    require(token != 0 && token == state.active_token && state.executing,
            "canonical completion requires the outstanding execution token");
    const auto phase = state.gpu_phase.load(std::memory_order_acquire);
    if (phase == Impl::GpuPhase::failure_needs_owner) {
        state.begin_failure_drain();
        return false;
    }
    if (phase != Impl::GpuPhase::ready) return false;
    if (state.gpu_failure) {
        const auto failure = state.gpu_failure;
        // The persistent worker has now destroyed/drained the processor, after
        // owner-only JIT retirement. No failed mapping or live storage escapes.
        state.words = {};
        state.active_token = 0;
        state.executing = false;
        std::rethrow_exception(failure);
    }
    return true;
}

CanonicalExecutor::MemoryLease CanonicalExecutor::completed(Token token) {
    require(poll(token), "canonical GPU execution has not completed");
    auto& state = *impl_;
    state.leased = true;
    state.executing = false;
    state.active_token = 0;
    return MemoryLease(state, {state.completed_rdram, rdram_bytes},
                       {state.completed_hidden, hidden_bytes});
}

void CanonicalExecutor::retire(Token token) {
    auto& state = *impl_;
    state.require_creator();
    require(token != 0 && token == state.active_token && state.executing,
            "canonical retirement requires the outstanding execution token");
    state.wait_for_gpu();
    auto lease = completed(token);
    lease.release();
}

CanonicalExecutor::MemoryLease CanonicalExecutor::execute(
    PreparationLease&& preparation, const Task& task, std::span<const std::byte> boot) {
    const auto token = begin(std::move(preparation), task, boot);
    impl_->wait_for_gpu();
    return completed(token);
}
} // namespace isle::rsp

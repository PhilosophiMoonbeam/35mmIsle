#include "rsp_render.hpp"

#include "context.hpp"
#include "device.hpp"
#include "rdp_device.hpp"

#include <atomic>
#include <bit>
#include <cstdint>
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
    }

    void require_creator() const {
        require(creator == std::this_thread::get_id(),
                "canonical executor must be used on its creating thread");
    }
    void require_creator_noexcept() const noexcept {
        if (creator != std::this_thread::get_id()) std::terminate();
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
};

CanonicalExecutor::CanonicalExecutor(bool require_cpu, Limits limits) {
    if constexpr (std::endian::native != std::endian::little)
        throw std::runtime_error("canonical RSP/RDP requires a little-endian word-swapped host");
    impl_ = std::make_unique<Impl>(require_cpu, limits);
}

CanonicalExecutor::~CanonicalExecutor() {
    impl_->require_creator_noexcept();
    if (impl_->preparing || impl_->executing || impl_->leased) std::terminate();
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

CanonicalExecutor::MemoryLease CanonicalExecutor::execute(
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
        // No fixture geometry/state gate: actual texture, depth, copy, fill and
        // all other machine-framed commands retain their original packet order.
        for (const auto& packet : state.packets.packets())
            processor.enqueue_command(static_cast<unsigned>(packet.count),
                                      state.packets.words().data() + packet.offset);
        processor.idle();
        require(vkDeviceWaitIdle(state.context.get_device()) == VK_SUCCESS,
                "canonical Vulkan device completion failed");
        state.errors.require_clean();
        const auto* memory = static_cast<const std::byte*>(processor.begin_read_rdram());
        const auto* hidden = static_cast<const std::byte*>(processor.begin_read_hidden_rdram());
        require(memory == reinterpret_cast<const std::byte*>(state.words.data()) && hidden,
                "canonical completed memory mapping failed or changed address");
        state.errors.require_clean();
        state.leased = true;
        state.executing = false;
        return MemoryLease(state, {memory, rdram_bytes}, {hidden, hidden_bytes});
    } catch (...) {
        // Invalid partial packet output never reaches the RDP, and no failed
        // execution publishes ready storage. Teardown drains any enqueued work.
        state.poison();
        state.executing = false;
        throw;
    }
}
} // namespace isle::rsp

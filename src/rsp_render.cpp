#include "rsp_render.hpp"
#include "rsp_fixture.hpp"

#include "context.hpp"
#include "device.hpp"
#include "rdp_device.hpp"

#include <array>
#include <atomic>
#include <bit>
#include <cstdint>
#include <cstring>
#include <exception>
#include <fstream>
#include <mutex>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace isle::rsp::probe {
namespace {
namespace fixture = ::isle::rsp_probe;
constexpr std::string_view renderer_pin = "1cecd042b2619bc505c12bfdc713808386f2b54d";
constexpr std::size_t hidden_bytes = fixture::rdram_bytes / 2;
constexpr std::size_t color_bytes = fixture::width * fixture::height * 2;
constexpr std::size_t guard_bytes = 64;
constexpr std::size_t snapshot_begin = fixture::color_address - guard_bytes;
constexpr std::size_t snapshot_end = fixture::color_address + color_bytes + guard_bytes;
static_assert(snapshot_end <= fixture::rdram_bytes && (snapshot_begin & 3) == 0);

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
            throw std::runtime_error("RSP-to-RDP replay backend error: " + first_);
    }
private:
    std::mutex mutex_;
    std::atomic<unsigned> errors_{0};
    std::string first_;
};

// This entry point admits only the current memory-independent stimulus. In
// particular it cannot silently replay texture/Z/CPU-input commands against A5.
void validate_capture(const fixture::Capture& capture) {
    const auto words = capture.words();
    require(!capture.rejected() && !words.empty() && words.size() <= fixture::capture_words &&
            !capture.packets().empty(), "empty or rejected RSP capture");
    std::size_t next = 0;
    unsigned triangles = 0, full_syncs = 0;
    bool color = false, scissor = false, modes = false, combine = false;
    for (const auto& packet : capture.packets()) {
        require(packet.offset == next && packet.offset <= words.size() &&
                packet.count <= words.size() - packet.offset && packet.count >= 2,
                "RSP capture packet storage is not contiguous and bounded");
        const auto* command = words.data() + packet.offset;
        const unsigned opcode = (command[0] >> 24) & 63u;
        require(full_syncs == 0, "RSP capture has commands after FullSync");
        require(packet.count == (opcode == 0x0c ? 24u : 2u),
                "RSP capture has an unsupported raw-RDP packet length");
        switch (opcode) {
        case 0x00: case 0x26: case 0x27: case 0x28: break;
        case 0x29: ++full_syncs; break;
        case 0x0c:
            require(color && scissor && modes && combine,
                    "RSP triangle precedes explicit independent RDP state");
            ++triangles;
            break;
        case 0x2d: {
            const auto left = (command[0] >> 12) & 0xfffu;
            const auto top = command[0] & 0xfffu;
            const auto right = (command[1] >> 12) & 0xfffu;
            const auto bottom = command[1] & 0xfffu;
            require(left <= right && right <= fixture::width * 4 && top == 0 &&
                    bottom == fixture::height * 4 && (command[1] & 0xff000000u) == 0,
                    "RSP replay scissor escapes its bounded noninterlaced color target");
            scissor = true;
            break;
        }
        case 0x2f:
            require((command[0] & 0xffffffu) == 0xf0 && command[1] == 0,
                    "RSP replay requires explicit one-cycle no-Z/no-texture/no-blend modes");
            modes = true;
            break;
        case 0x3a: break; // Primitive marker, not the selected shade color.
        case 0x3c:
            require((command[0] & 0xffffffu) == 0x887f10 && command[1] == 0x88fe793c,
                    "RSP replay requires the explicit shade-only combiner");
            combine = true;
            break;
        case 0x3f:
            require((command[0] & 0xffffffu) == (0x100000u | (fixture::width - 1)) &&
                    command[1] == fixture::color_address,
                    "RSP replay requires the bounded synthetic RGBA16 color image");
            color = true;
            break;
        default:
            throw std::runtime_error("RSP replay rejects commands needing other memory/state inputs");
        }
        next += packet.count;
    }
    require(next == words.size() && triangles != 0 && full_syncs == 1,
            "RSP replay needs complete geometry followed by exactly one actual FullSync");
}

void initialize_context(Vulkan::Context& context, bool cpu_only) {
    require(Vulkan::Context::init_loader(nullptr), "RSP replay Vulkan loader initialization failed");
    if (!cpu_only) {
        require(context.init_instance_and_device(nullptr, 0, nullptr, 0),
                "RSP replay headless Vulkan instance/device initialization failed");
        return;
    }
    require(context.init_instance(nullptr, 0), "RSP replay Vulkan instance initialization failed");
    std::uint32_t count = 0;
    require(vkEnumeratePhysicalDevices(context.get_instance(), &count, nullptr) == VK_SUCCESS && count != 0,
            "RSP replay found no actual Vulkan physical device");
    std::vector<VkPhysicalDevice> devices(count);
    require(vkEnumeratePhysicalDevices(context.get_instance(), &count, devices.data()) == VK_SUCCESS,
            "RSP replay Vulkan physical-device enumeration failed");
    for (std::uint32_t index = 0; index < count; ++index) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(devices[index], &properties);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU) {
            require(context.init_device(devices[index], VK_NULL_HANDLE, nullptr, 0),
                    "RSP replay could not initialize its actual CPU Vulkan device");
            return;
        }
    }
    throw std::runtime_error("RSP replay requires VK_PHYSICAL_DEVICE_TYPE_CPU but none exists");
}

void write_binary(const std::filesystem::path& path, std::span<const std::byte> bytes) {
    std::ofstream output(path, std::ios::binary);
    output.exceptions(std::ios::badbit | std::ios::failbit);
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    output.close();
}

void json_string(std::ostream& output, std::string_view text) {
    constexpr char digits[] = "0123456789abcdef";
    output.put('"');
    for (const unsigned char character : text) {
        if (character == '"' || character == '\\') {
            output.put('\\');
            output.put(static_cast<char>(character));
        } else if (character < 0x20) {
            output << "\\u00" << digits[character >> 4] << digits[character & 15];
        } else {
            output.put(static_cast<char>(character));
        }
    }
    output.put('"');
}

void validate_guards(std::span<const std::byte> memory, std::span<const std::byte> hidden) {
    for (std::size_t offset = 0; offset < guard_bytes; ++offset) {
        require(memory[snapshot_begin + offset] == std::byte{0xa5} &&
                memory[fixture::color_address + color_bytes + offset] == std::byte{0xa5},
                "RSP replay changed a color-target guard byte");
    }
    for (std::size_t offset = 0; offset < guard_bytes / 2; ++offset) {
        require(hidden[snapshot_begin / 2 + offset] == std::byte{0} &&
                hidden[(fixture::color_address + color_bytes) / 2 + offset] == std::byte{0},
                "RSP replay changed a hidden color-target guard byte");
    }
}

void write_ppm(const std::filesystem::path& path, std::span<const std::byte> memory) {
    std::ofstream output(path, std::ios::binary);
    output.exceptions(std::ios::badbit | std::ios::failbit);
    output << "P6\n" << fixture::width << ' ' << fixture::height << "\n255\n";
    std::array<char, fixture::width * 3> row{};
    for (unsigned y = 0; y < fixture::height; ++y) {
        for (unsigned x = 0; x < fixture::width; ++x) {
            const auto address = fixture::color_address + (y * fixture::width + x) * 2;
            const unsigned rgba = (std::to_integer<unsigned>(memory[address ^ 3u]) << 8) |
                                   std::to_integer<unsigned>(memory[(address + 1) ^ 3u]);
            for (unsigned channel = 0; channel < 3; ++channel) {
                const unsigned five = (rgba >> (11 - channel * 5)) & 31u;
                row[x * 3 + channel] = static_cast<char>((five << 3) | (five >> 2));
            }
        }
        output.write(row.data(), static_cast<std::streamsize>(row.size()));
    }
    output.close();
}

void write_metadata(const std::filesystem::path& directory, const fixture::Capture& capture,
                    std::string_view label, const Vulkan::Context& context) {
    const auto& properties = context.get_gpu_props();
    const auto& features = context.get_enabled_device_features();
    std::ofstream output(directory / "metadata.json");
    output.exceptions(std::ios::badbit | std::ios::failbit);
    output << "{\n  \"schema_version\":1,\n  \"label\":";
    json_string(output, label);
    output << ",\n  \"scope\":\"after-SP original synthetic raw-RDP replay in separate explicitly initialized memory; NOT shared production RDRAM/coherency, ROM frame, VI, hardware or full-W06 proof\",\n"
           << "  \"renderer_commit\":\"" << renderer_pin << "\",\n  \"device_name\":";
    json_string(output, properties.deviceName);
    output << ",\n  \"device_type\":" << properties.deviceType
           << ",\n  \"vendor_id\":" << properties.vendorID
           << ",\n  \"device_id\":" << properties.deviceID
           << ",\n  \"driver_version\":" << properties.driverVersion
           << ",\n  \"driver_id\":" << features.driver_id
           << ",\n  \"api_version\":" << properties.apiVersion
           << ",\n  \"completion\":\"CommandProcessor::idle flush and real GPU fence/coherency timeline wait, then successful vkDeviceWaitIdle before CPU mapping; captured FullSync is not host completion\",\n"
           << "  \"timeline_complete\":true,\n  \"backend_errors\":0,\n"
           << "  \"memory_layout\":\"little-endian host word-swapped N64: canonical byte a at host byte[a^3], halfword a at host u16[(a/2)^1], word a at host u32[a/4]; hidden byte at a/2\",\n"
           << "  \"rdram_size\":" << fixture::rdram_bytes << ",\n  \"hidden_size\":" << hidden_bytes
           << ",\n  \"initial_rdram_byte\":165,\n  \"initial_hidden_byte\":0,\n"
           << "  \"width\":" << fixture::width << ",\n  \"height\":" << fixture::height
           << ",\n  \"color_format\":\"RGBA5551\",\n  \"color_base\":" << fixture::color_address
           << ",\n  \"plane_bytes\":" << color_bytes << ",\n  \"guard_bytes_each_side\":" << guard_bytes
           << ",\n  \"rdram_file\":\"rdram.bin\",\n  \"rdram_file_offset\":" << snapshot_begin
           << ",\n  \"rdram_file_bytes\":" << snapshot_end - snapshot_begin
           << ",\n  \"hidden_file\":\"hidden.bin\",\n  \"hidden_file_offset\":" << snapshot_begin / 2
           << ",\n  \"hidden_file_bytes\":" << (snapshot_end - snapshot_begin) / 2
           << ",\n  \"ppm\":\"color.ppm is direct RGBA5551 RGB bit-replication; alpha/coverage ignored, no VI filtering\",\n"
           << "  \"words_encoding\":\"canonical host-value RDP u32 integers, not raw RDRAM byte order\",\n  \"words\":[";
    for (std::size_t index = 0; index < capture.words().size(); ++index)
        output << (index ? "," : "") << capture.words()[index];
    output << "],\n  \"packets\":[";
    for (std::size_t index = 0; index < capture.packets().size(); ++index) {
        const auto& packet = capture.packets()[index];
        output << (index ? "," : "") << "{\"word_offset\":" << packet.offset
               << ",\"word_count\":" << packet.count << '}';
    }
    output << "]\n}\n";
    output.close();
}
} // namespace

struct CanonicalCaptureExecutor::Impl {
    explicit Impl(bool require_cpu_device) {
        context.set_notification_callback([this](const char* message) { errors.record(message); });
        initialize_context(context, require_cpu_device);
        require(context.get_gpu() != VK_NULL_HANDLE && context.get_device() != VK_NULL_HANDLE,
                "RSP replay initialization returned no actual Vulkan device");
        require_clean();
        device.set_context(context);
        processor.emplace(device, nullptr, 0, fixture::rdram_bytes, hidden_bytes,
            RDP::COMMAND_PROCESSOR_FLAG_HOST_VISIBLE_HIDDEN_RDRAM_BIT);
        processor->set_validation_interface(&errors);
        require(processor->device_is_supported(),
                "RSP replay Vulkan device does not support pinned paraLLEl-RDP");
        require_clean();
    }

    void require_creator() const {
        require(creator == std::this_thread::get_id(),
                "RSP replay executor must be used on its creating thread");
    }

    void require_creator_noexcept() const noexcept {
        if (creator != std::this_thread::get_id()) std::terminate();
    }

    void require_clean() {
        try {
            errors.require_clean();
        } catch (...) {
            poisoned = true;
            throw;
        }
    }

    // Reverse destruction preserves the diagnostic sink and Vulkan dependencies
    // through the processor's final drain, including partial construction.
    const std::thread::id creator = std::this_thread::get_id();
    ErrorSink errors;
    Vulkan::Context context;
    Vulkan::Device device;
    std::optional<RDP::CommandProcessor> processor;
    bool executing = false;
    bool leased = false;
    bool poisoned = false;
};

CanonicalCaptureExecutor::CanonicalCaptureExecutor(bool require_cpu_device) {
    if constexpr (std::endian::native != std::endian::little)
        throw std::runtime_error("RSP replay raw snapshots require a little-endian word-swapped host");
    impl_ = std::make_unique<Impl>(require_cpu_device);
}

CanonicalCaptureExecutor::~CanonicalCaptureExecutor() {
    impl_->require_creator_noexcept();
    if (impl_->executing || impl_->leased) std::terminate();
}

CanonicalCaptureExecutor::MemoryLease::MemoryLease(
    Impl& owner, std::span<const std::byte> rdram, std::span<const std::byte> hidden) noexcept
    : owner_(&owner), rdram_(rdram), hidden_(hidden) {}

CanonicalCaptureExecutor::MemoryLease::~MemoryLease() {
    release();
}

CanonicalCaptureExecutor::MemoryLease::MemoryLease(MemoryLease&& other) noexcept
    : owner_(std::exchange(other.owner_, nullptr)),
      rdram_(std::exchange(other.rdram_, {})), hidden_(std::exchange(other.hidden_, {})) {
    if (owner_) owner_->require_creator_noexcept();
}

CanonicalCaptureExecutor::MemoryLease&
CanonicalCaptureExecutor::MemoryLease::operator=(MemoryLease&& other) noexcept {
    if (this != &other) {
        release();
        if (other.owner_) other.owner_->require_creator_noexcept();
        owner_ = std::exchange(other.owner_, nullptr);
        rdram_ = std::exchange(other.rdram_, {});
        hidden_ = std::exchange(other.hidden_, {});
    }
    return *this;
}

std::span<const std::byte> CanonicalCaptureExecutor::MemoryLease::rdram() const noexcept {
    if (owner_) owner_->require_creator_noexcept();
    return rdram_;
}

std::span<const std::byte> CanonicalCaptureExecutor::MemoryLease::hidden() const noexcept {
    if (owner_) owner_->require_creator_noexcept();
    return hidden_;
}

void CanonicalCaptureExecutor::MemoryLease::release() noexcept {
    if (!owner_) return;
    owner_->require_creator_noexcept();
    if (!owner_->leased || owner_->executing) std::terminate();
    owner_->leased = false;
    owner_ = nullptr;
    rdram_ = {};
    hidden_ = {};
}

bool CanonicalCaptureExecutor::quiescent() const noexcept {
    impl_->require_creator_noexcept();
    return !impl_->executing && !impl_->leased;
}

CanonicalCaptureExecutor::MemoryLease
CanonicalCaptureExecutor::execute(const ::isle::rsp_probe::Capture& capture) {
    auto& state = *impl_;
    state.require_creator();
    require(!state.executing && !state.leased,
            "RSP replay cannot execute or reinitialize memory while executing or leased");
    require(!state.poisoned, "RSP replay executor is poisoned by an earlier backend failure");
    validate_capture(capture);
    state.executing = true;
    try {
        state.require_clean();
        auto& processor = *state.processor;
        processor.idle();
        // Idle waits; advancing the frame context recycles Granite command pools.
        // The old CPU lease is already released before this reuse boundary.
        processor.begin_frame_context();
        state.require_clean();
        auto* memory = static_cast<std::byte*>(processor.begin_read_rdram());
        auto* hidden = static_cast<std::byte*>(processor.begin_read_hidden_rdram());
        require(memory && hidden, "RSP replay could not map independently owned RDRAM/hidden buffers");
        std::memset(memory, 0xa5, fixture::rdram_bytes);
        processor.end_write_rdram();
        std::memset(hidden, 0, hidden_bytes);
        processor.end_write_hidden_rdram();
        state.require_clean();

        // Borrow stable packet spans directly. There is no capture/RAM clone,
        // per-packet host allocation or per-execution backend construction.
        for (const auto& packet : capture.packets())
            processor.enqueue_command(static_cast<unsigned>(packet.count),
                                      capture.words().data() + packet.offset);
        processor.idle();
        require(vkDeviceWaitIdle(state.context.get_device()) == VK_SUCCESS,
                "RSP replay Vulkan device completion failed");
        state.require_clean();
        const auto* completed_memory = static_cast<const std::byte*>(processor.begin_read_rdram());
        const auto* completed_hidden = static_cast<const std::byte*>(processor.begin_read_hidden_rdram());
        require(completed_memory && completed_hidden, "RSP replay completed memory mapping failed");
        const std::span<const std::byte> memory_view(completed_memory, fixture::rdram_bytes);
        const std::span<const std::byte> hidden_view(completed_hidden, hidden_bytes);
        state.require_clean();
        validate_guards(memory_view, hidden_view);
        state.leased = true;
        state.executing = false;
        return MemoryLease(state, memory_view, hidden_view);
    } catch (...) {
        state.poisoned = true;
        // Its destructor drains while the caller's immutable capture is still
        // alive. Never leave failed work awaiting a later task/destructor, and
        // never rebuild this failed backend or publish a completion lease.
        state.processor.reset();
        state.executing = false;
        throw;
    }
}

void render_capture(CanonicalCaptureExecutor& executor,
                    const ::isle::rsp_probe::Capture& capture,
                    const std::filesystem::path& output_directory,
                    std::string_view label) {
    require(!output_directory.empty() && std::filesystem::is_directory(output_directory) &&
            std::filesystem::is_empty(output_directory),
            "RSP replay output must be an existing newly-created empty directory");
    auto lease = executor.execute(capture);
    const auto memory = lease.rdram();
    const auto hidden = lease.hidden();
    write_binary(output_directory / "rdram.bin", memory.subspan(snapshot_begin, snapshot_end - snapshot_begin));
    write_binary(output_directory / "hidden.bin", hidden.subspan(snapshot_begin / 2, (snapshot_end - snapshot_begin) / 2));
    write_ppm(output_directory / "color.ppm", memory);
    executor.impl_->require_clean();
    // Written last, with the actual mappings still leased: no successful
    // completion metadata for failed snapshots.
    write_metadata(output_directory, capture, label, executor.impl_->context);
}
} // namespace isle::rsp::probe

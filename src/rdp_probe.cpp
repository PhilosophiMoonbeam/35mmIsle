#include "rdp_fixture.hpp"

#include "context.hpp"
#include "device.hpp"
#include "rdp_device.hpp"

#include <atomic>
#include <bit>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
namespace probe = isle::rdp_probe;
constexpr std::string_view renderer_pin = "1cecd042b2619bc505c12bfdc713808386f2b54d";
#ifdef _WIN32
using Argument = wchar_t;
constexpr std::wstring_view cpu_argument = L"--cpu-device";
constexpr std::wstring_view raw_argument = L"--raw-out";
constexpr std::wstring_view help_argument = L"--help";
#else
using Argument = char;
constexpr std::string_view cpu_argument = "--cpu-device";
constexpr std::string_view raw_argument = "--raw-out";
constexpr std::string_view help_argument = "--help";
#endif

const char *stage_name(probe::StageKind kind)
{
    switch (kind) {
    case probe::StageKind::clear: return "clear";
    case probe::StageKind::geometry: return "geometry";
    case probe::StageKind::depth_copy: return "depth_copy";
    }
    throw std::runtime_error("invalid fixture stage kind");
}

class ErrorSink final : public RDP::ValidationInterface {
public:
    void record(const char *source, const char *message)
    {
        std::lock_guard lock(mutex_);
        ++errors_;
        std::cerr << "backend_error source=" << source << " message="
                  << std::quoted(message ? message : "(no diagnostic)") << '\n';
    }

    void report_rdp_crash(RDP::ValidationError, const char *message) override
    {
        record("rdp", message);
    }

    unsigned errors() const { return errors_.load(); }

private:
    std::atomic<unsigned> errors_{0};
    std::mutex mutex_;
};

void write_binary(const std::filesystem::path &path, std::span<const std::byte> bytes)
{
    std::ofstream output(path, std::ios::binary);
    output.exceptions(std::ios::badbit | std::ios::failbit);
    output.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    output.close();
}

void write_raw_metadata(const std::filesystem::path &directory, const probe::Fixture &fixture)
{
    std::ofstream output(directory / "metadata.json");
    output.exceptions(std::ios::badbit | std::ios::failbit);
    output << "{\n  \"schema_version\":1,\n  \"scope\":\"original synthetic fixture only; no native frame, ROM, VI or game assets\",\n"
           << "  \"renderer_commit\":\"" << renderer_pin << "\",\n"
           << "  \"memory_layout\":\"little-endian host word-swapped N64: byte a at a^3, halfword a at u16[(a/2)^1], word a at u32[a/4]; hidden byte at a/2\",\n"
           << "  \"rdram_size\":" << probe::rdram_size << ",\n  \"hidden_size\":" << probe::hidden_size
           << ",\n  \"width\":" << probe::width << ",\n  \"height\":" << probe::height
           << ",\n  \"plane_bytes\":" << probe::plane_bytes
           << ",\n  \"color_base\":" << probe::color_base << ",\n  \"depth_base\":" << probe::depth_base
           << ",\n  \"before_base\":" << probe::before_base << ",\n  \"rdram_file_offset\":" << probe::color_base
           << ",\n  \"rdram_file_bytes\":" << probe::region_end - probe::color_base
           << ",\n  \"hidden_file_offset\":" << probe::color_base / 2
           << ",\n  \"hidden_file_bytes\":" << (probe::region_end - probe::color_base) / 2
           << ",\n  \"initial_rdram_byte\":165,\n  \"initial_hidden_byte\":0,\n"
           << "  \"words_encoding\":\"host-value RDP u32 integers in JSON, not RDRAM byte order\",\n"
           << "  \"words\":[";
    for (std::size_t index = 0; index < fixture.words.size(); ++index)
        output << (index ? "," : "") << fixture.words[index];
    output << "],\n  \"packets\":[";
    for (std::size_t index = 0; index < fixture.packets.size(); ++index) {
        const auto &packet = fixture.packets[index];
        output << (index ? "," : "") << "{\"word_offset\":" << packet.offset
               << ",\"word_count\":" << packet.word_count << '}';
    }
    output << "],\n  \"stages\":[";
    for (std::size_t index = 0; index < fixture.stages.size(); ++index) {
        const auto &stage = fixture.stages[index];
        output << (index ? "," : "") << "{\"name\":\"" << stage_name(stage.kind)
               << "\",\"first_packet\":" << stage.first_packet << ",\"packet_count\":" << stage.packet_count
               << ",\"rdram_file\":\"" << stage_name(stage.kind) << "-rdram.bin\",\"hidden_file\":\""
               << stage_name(stage.kind) << "-hidden.bin\"}";
    }
    output << "]\n}\n";
    output.close();
}

void initialize_context(Vulkan::Context &context, bool cpu_only)
{
    if (!Vulkan::Context::init_loader(nullptr))
        throw std::runtime_error("Vulkan loader initialization failed; no real Vulkan backend is available");
    if (!cpu_only) {
        if (!context.init_instance_and_device(nullptr, 0, nullptr, 0))
            throw std::runtime_error("headless Vulkan instance/device initialization failed");
        return;
    }
    // The pinned API admits an explicit VkPhysicalDevice. CPU selection is based
    // on its real Vulkan device type, never a name, ICD filename or environment guess.
    if (!context.init_instance(nullptr, 0))
        throw std::runtime_error("headless Vulkan instance initialization failed");
    std::uint32_t count = 0;
    if (vkEnumeratePhysicalDevices(context.get_instance(), &count, nullptr) != VK_SUCCESS || count == 0)
        throw std::runtime_error("no Vulkan physical devices were enumerated");
    std::vector<VkPhysicalDevice> devices(count);
    if (vkEnumeratePhysicalDevices(context.get_instance(), &count, devices.data()) != VK_SUCCESS)
        throw std::runtime_error("Vulkan physical-device enumeration failed");
    for (std::uint32_t index = 0; index < count; ++index) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(devices[index], &properties);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU) {
            if (!context.init_device(devices[index], VK_NULL_HANDLE, nullptr, 0))
                throw std::runtime_error("selected real CPU Vulkan device could not be initialized");
            return;
        }
    }
    throw std::runtime_error("--cpu-device requested but no VK_PHYSICAL_DEVICE_TYPE_CPU device exists");
}

void report_device(const Vulkan::Context &context)
{
    if (context.get_gpu() == VK_NULL_HANDLE || context.get_device() == VK_NULL_HANDLE)
        throw std::runtime_error("Vulkan initialization returned no actual physical/logical device");
    const auto &properties = context.get_gpu_props();
    const auto &features = context.get_enabled_device_features();
    std::cout << "vulkan_device name=" << std::quoted(properties.deviceName)
              << " vendor_id=" << properties.vendorID << " device_id=" << properties.deviceID
              << " type=" << properties.deviceType << " driver_version=" << properties.driverVersion
              << " driver_id=" << features.driver_id << " api_version=" << properties.apiVersion << '\n';
    std::cout << "vulkan_enabled_features storage_buffer_16bit=" << features.vk11_features.storageBuffer16BitAccess
              << " storage_buffer_8bit=" << features.vk12_features.storageBuffer8BitAccess
              << " shader_int16=" << features.enabled_features.shaderInt16
              << " shader_int8=" << features.vk12_features.shaderInt8
              << " timeline_semaphore=" << features.vk12_features.timelineSemaphore
              << " subgroup_size=" << features.vk11_props.subgroupSize
              << " subgroup_operations=" << features.vk11_props.subgroupSupportedOperations << '\n';
}

int run(int argc, Argument **argv)
{
    bool cpu_only = false;
    std::filesystem::path raw_directory;
    for (int index = 1; index < argc; ++index) {
        const std::basic_string_view<Argument> argument(argv[index]);
        if (argument == cpu_argument) {
            cpu_only = true;
        } else if (argument == raw_argument && index + 1 < argc && raw_directory.empty()) {
            raw_directory = argv[++index];
            if (raw_directory.empty())
                throw std::runtime_error("--raw-out requires a nonempty new directory");
        } else if (argument == help_argument) {
            std::cout << "35mmisle-rdp-probe [--cpu-device] [--raw-out NEW_DIRECTORY]\n"
                         "Runs only a bounded original three-stage 64x48 RDP fixture; no ROM/frame input.\n"
                         "Raw output: three stage snapshots of the 0x27000-byte region plus raw hidden bytes and packet metadata.\n";
            return 0;
        } else {
            throw std::runtime_error("unknown or incomplete argument");
        }
    }
    if constexpr (std::endian::native != std::endian::little)
        throw std::runtime_error("the probe requires a little-endian host for its raw word-swapped memory contract");
    const auto fixture = probe::make_fixture();
    if (!raw_directory.empty()) {
        if (!std::filesystem::create_directory(raw_directory))
            throw std::runtime_error("--raw-out directory already exists; refusing to overwrite");
        write_raw_metadata(raw_directory, fixture);
    }

    // Destruction is reverse construction: callbacks and Device/Context owners
    // survive CommandProcessor's destructor, which also idles its command ring.
    ErrorSink errors;
    Vulkan::Context context;
    context.set_notification_callback([&errors](const char *message) { errors.record("vulkan", message); });
    initialize_context(context, cpu_only);
    report_device(context);
    Vulkan::Device device;
    device.set_context(context);
    RDP::CommandProcessor processor(device, nullptr, 0, probe::rdram_size, probe::hidden_size,
        RDP::COMMAND_PROCESSOR_FLAG_HOST_VISIBLE_HIDDEN_RDRAM_BIT);
    processor.set_validation_interface(&errors);
    if (!processor.device_is_supported())
        throw std::runtime_error("actual Vulkan device does not satisfy pinned paraLLEl-RDP requirements");
    processor.idle();
    auto *rdram = static_cast<std::byte *>(processor.begin_read_rdram());
    auto *hidden = static_cast<std::byte *>(processor.begin_read_hidden_rdram());
    if (!rdram || !hidden)
        throw std::runtime_error("backend failed to map owned RDRAM/hidden memory");
    std::memset(rdram, 0xa5, probe::rdram_size);
    processor.end_write_rdram();
    std::memset(hidden, 0, probe::hidden_size);
    processor.end_write_hidden_rdram();
    std::cout << "fixture renderer_commit=" << renderer_pin << " width=" << probe::width
              << " height=" << probe::height << " stages=" << fixture.stages.size()
              << " packets=" << fixture.packets.size() << " words=" << fixture.words.size()
              << " scope=synthetic-only\n";

    std::size_t completed = 0;
    for (const auto &stage : fixture.stages) {
        const char *name = stage_name(stage.kind);
        try {
            if (stage.first_packet > fixture.packets.size() ||
                stage.packet_count > fixture.packets.size() - stage.first_packet)
                throw std::runtime_error("stage packet metadata exceeds fixture storage");
            unsigned triangles = 0, fills = 0, texture_rectangles = 0;
            std::size_t words = 0;
            for (std::size_t index = stage.first_packet; index < stage.first_packet + stage.packet_count; ++index) {
                const auto &packet = fixture.packets[index];
                if (packet.offset > fixture.words.size() || packet.word_count < 2 ||
                    packet.word_count > fixture.words.size() - packet.offset)
                    throw std::runtime_error("packet metadata exceeds fixture word storage");
                const auto opcode = (fixture.words[packet.offset] >> 24) & 0x3f;
                triangles += opcode >= 0x08 && opcode <= 0x0f;
                fills += opcode == 0x36;
                texture_rectangles += opcode == 0x24 || opcode == 0x25;
                words += packet.word_count;
                processor.enqueue_command(packet.word_count, fixture.words.data() + packet.offset);
            }
            // idle() flushes and waits on the real command/timeline work. There
            // is no VI end_frame or host-renderer substitute for completion.
            processor.idle();
            const auto memory = std::span(static_cast<const std::byte *>(processor.begin_read_rdram()), probe::rdram_size);
            const auto hidden_memory = std::span(static_cast<const std::byte *>(processor.begin_read_hidden_rdram()), probe::hidden_size);
            if (!memory.data() || !hidden_memory.data())
                throw std::runtime_error("completed backend memory mapping failed");
            if (!raw_directory.empty()) {
                write_binary(raw_directory / (std::string(name) + "-rdram.bin"),
                    memory.subspan(probe::color_base, probe::region_end - probe::color_base));
                write_binary(raw_directory / (std::string(name) + "-hidden.bin"),
                    hidden_memory.subspan(probe::color_base / 2, (probe::region_end - probe::color_base) / 2));
            }
            if (errors.errors() != 0)
                throw std::runtime_error("real backend reported " + std::to_string(errors.errors()) + " validation errors");
            probe::validate_stage(stage.kind, memory, hidden_memory);
            ++completed;
            std::cout << "stage name=" << name << " packets=" << stage.packet_count << " words=" << words
                      << " triangles=" << triangles << " fills=" << fills << " texture_rectangles=" << texture_rectangles
                      << " backend_errors=" << errors.errors() << " semantic_errors=0 timeline_complete=true\n";
        } catch (const std::exception &error) {
            std::cerr << "stage name=" << name << " complete=false error=" << std::quoted(error.what()) << '\n';
            throw;
        }
    }
    processor.idle();
    std::cout << "rdp_probe complete=true stages=" << completed
              << " errors=" << errors.errors() << " independent_reference_parity=not-established\n";
    return 0;
}
} // namespace

#ifdef _WIN32
int wmain(int argc, wchar_t **argv)
#else
int main(int argc, char **argv)
#endif
{
    try {
        return run(argc, argv);
    } catch (const std::exception &error) {
        std::cerr << "rdp_probe complete=false error=" << std::quoted(error.what()) << '\n';
        return 1;
    }
}

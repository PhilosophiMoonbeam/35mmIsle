#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>

namespace isle::snap_station {
enum class Error { none, invalid, unsupported, io, failed };
enum class CacheOperation { writeback, invalidate };
struct Memory {
    std::uint32_t* words = nullptr;
    std::size_t word_count = 0xffffc;
    std::size_t byte_count = 0x400000;
};
struct FrameIdentity {
    std::uint64_t application = 0, entry = 0, ordinal = 0, submission = 0;
    const void* owner = nullptr;
    std::uint32_t pass = 0;
};
// Serialized owner: callers must retire all borrowers before reset/disconnect.
// No frame pointers or application owners are retained beyond their identity.
class Device final {
public:
    Device() noexcept;
    ~Device();
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&&) = delete;
    Device& operator=(Device&&) = delete;
    bool connect(const std::filesystem::path& directory) noexcept;
    void disconnect() noexcept;
    void cancel() noexcept;
    bool available() const noexcept;
    Error error() const noexcept;
    const char* message() const noexcept;
    bool memory(Memory&) noexcept;
    bool cache(Memory, CacheOperation) noexcept;
    bool begin_pass(std::uint64_t application, std::uint64_t entry,
                    const void* owner, std::uint32_t pass) noexcept;
    bool stage_frame(FrameIdentity, std::span<const std::byte> rgba,
                     std::uint32_t width, std::uint32_t height,
                     std::size_t stride) noexcept;
    bool command(std::uint8_t) noexcept;
    bool status(std::uint8_t&) noexcept;
    bool reset_pending() const noexcept;
    bool take_reset_request(std::uint64_t& ticket) noexcept;
    bool complete_reset(std::uint64_t new_application) noexcept;
    // Published files survive cancellation. Path is empty when disconnected.
    const std::filesystem::path& output_directory() const noexcept;
private:
    struct State;
    std::unique_ptr<State> state_;
    Error error_ = Error::none;
    const char* message_ = "";
    bool fail(Error, const char*) noexcept;
};
} // namespace isle::snap_station

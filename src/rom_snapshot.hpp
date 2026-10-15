#ifndef ISLE_ROM_SNAPSHOT_HPP
#define ISLE_ROM_SNAPSHOT_HPP

#include "35mmisle/rom.hpp"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>
#include <utility>

namespace isle::rom {
// Probe-only admitted input. One read and normalization; consumers share const
// canonical bytes, then retain their own typed resources instead of the ROM.
// Moving transfers storage; a moved-from snapshot must not be consumed.
class Snapshot final {
public:
    Snapshot(Snapshot&&) noexcept = default;
    Snapshot& operator=(Snapshot&&) = delete;
    Snapshot(const Snapshot&) = delete;
    Snapshot& operator=(const Snapshot&) = delete;

    // Identity records the original input order; bytes are always canonical z64.
    const Identity& identity() const noexcept { return identity_; }
    std::span<const std::byte> canonical() const noexcept {
        if (!bytes_) return {};
        return std::as_bytes(std::span<const char>(bytes_.get(), static_cast<std::size_t>(us_size)));
    }
private:
    friend Snapshot read_snapshot(const std::filesystem::path& path);
    Snapshot(std::unique_ptr<char[]>&& bytes, Identity identity) noexcept
        : bytes_(std::move(bytes)), identity_(identity) {}
    std::unique_ptr<const char[]> bytes_;
    Identity identity_;
};
// Throws on unreadable/short/oversized/incorrect input; never logs its path.
// Admission and every extraction use the same exclusively owned bounded bytes.
Snapshot read_snapshot(const std::filesystem::path& path);
} // namespace isle::rom
#endif

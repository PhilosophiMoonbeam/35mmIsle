#include "rom_snapshot.hpp"

#include <algorithm>
#include <fstream>
#include <istream>
#include <stdexcept>
#include <streambuf>

namespace isle::rom {
namespace {
class SnapshotBuffer final : public std::streambuf {
public:
    explicit SnapshotBuffer(std::span<char> bytes) {
        setg(bytes.data(), bytes.data(), bytes.data() + bytes.size());
    }
};
} // namespace

Snapshot read_snapshot(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) throw std::runtime_error("cannot open private ROM for reading");
    const auto size = static_cast<std::size_t>(us_size);
    auto bytes = std::make_unique_for_overwrite<char[]>(size);
    input.read(bytes.get(), static_cast<std::streamsize>(size));
    if (input.gcount() != static_cast<std::streamsize>(size) || input.bad())
        throw std::runtime_error("private ROM snapshot is incomplete or unreadable");
    char extra = 0;
    if (input.get(extra)) throw std::runtime_error("private ROM exceeds the exact admitted image size");
    if (!input.eof() || input.bad()) throw std::runtime_error("private ROM snapshot read failed");
    input.close();

    SnapshotBuffer buffer(std::span<char>(bytes.get(), size));
    std::istream memory_input(&buffer);
    const auto admitted = validate_us(memory_input);
    if (!admitted) throw std::runtime_error("private ROM snapshot failed exact-US admission");
    if (admitted.identity.order == ByteOrder::v64) {
        for (std::size_t i = 0; i < size; i += 2)
            std::swap(bytes[i], bytes[i + 1]);
    } else if (admitted.identity.order == ByteOrder::n64) {
        for (std::size_t i = 0; i < size; i += 4) {
            std::swap(bytes[i], bytes[i + 3]);
            std::swap(bytes[i + 1], bytes[i + 2]);
        }
    }
    return Snapshot(std::move(bytes), admitted.identity);
}
} // namespace isle::rom

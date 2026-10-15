#ifndef ISLE_SAVE_IMAGE_HPP
#define ISLE_SAVE_IMAGE_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace isle::save {

inline constexpr std::size_t image_bytes = 0x1F2A4;
inline constexpr std::size_t envelope_header_bytes = 128;
inline constexpr std::size_t envelope_file_bytes = envelope_header_bytes + image_bytes;

enum class Error {
    none,
    invalid_argument,
    image_size,
    image_version,
    image_offsets,
    image_digest,
    envelope_size,
    envelope_magic,
    envelope_version,
    envelope_header,
    envelope_codec,
    envelope_baseline,
    envelope_generation,
    envelope_digest,
};

struct ImageResult;

// Borrowed structural validity only: not photo/scene replay safety. The owner must
// keep the bytes alive AND immutable for the lifetime of every view and commit.
class ImageView {
public:
    constexpr ImageView() noexcept = default;
    [[nodiscard]] constexpr std::span<const std::byte> bytes() const noexcept { return bytes_; }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return bytes_.size() == image_bytes; }

private:
    explicit constexpr ImageView(std::span<const std::byte> bytes) noexcept : bytes_(bytes) {}
    std::span<const std::byte> bytes_{};

    friend ImageResult inspect_image(std::span<const std::byte>) noexcept;
    friend ImageResult seal_image(std::span<std::byte>, std::uint32_t) noexcept;
};

struct ImageResult {
    Error error = Error::invalid_argument;
    ImageView image{};
    [[nodiscard]] explicit operator bool() const noexcept { return error == Error::none && bool(image); }
};

// All operations are allocation-free. Failure never publishes a borrowed view.
[[nodiscard]] ImageResult inspect_image(std::span<const std::byte> bytes) noexcept;

// Preflights exact size, all version bytes and canonical relative offsets before
// mutation. On success changes only [0,16) and the raw BE game-time word at 0x30.
// This seals caller-authored/original bytes; it is not a new-game initializer.
[[nodiscard]] ImageResult seal_image(std::span<std::byte> bytes, std::uint32_t raw_game_time) noexcept;

using EnvelopeHeader = std::array<std::byte, envelope_header_bytes>;

// Fixed baseline, codec 1, exact EOF, no compression/migration. Generation zero
// is invalid. Failure leaves output unchanged. Output must not alias image bytes.
// SHA-256 provides integrity, not authentication or filesystem durability.
[[nodiscard]] Error encode_envelope_header(ImageView image, std::uint64_t generation,
                                           EnvelopeHeader& output) noexcept;

struct EnvelopeResult {
    Error error = Error::invalid_argument;
    ImageView image{};
    std::uint64_t generation = 0;
    [[nodiscard]] explicit operator bool() const noexcept {
        return error == Error::none && bool(image) && generation != 0;
    }
};

// Verifies complete header metadata/integrity and original image structure/hash
// before publishing a view borrowed directly from the supplied complete file.
[[nodiscard]] EnvelopeResult inspect_envelope(std::span<const std::byte> bytes) noexcept;

} // namespace isle::save

#endif

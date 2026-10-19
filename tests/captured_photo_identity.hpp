#ifndef ISLE_PRIVATE_CAPTURED_PHOTO_IDENTITY_HPP
#define ISLE_PRIVATE_CAPTURED_PHOTO_IDENTITY_HPP

#include "player_photo.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace isle::private_course {
// Original scorer-owned flags may revise the same canonical captured record.
// Everything else in the complete wire image remains part of its identity.
inline std::uint64_t photo_identity(const IslePlayerPhotoData& photo,
        std::array<std::byte,save::game_photo_bytes>& bytes) noexcept {
    player::encode_photo(photo,bytes);
    bytes[1]=std::byte{0};
    std::uint64_t hash=14695981039346656037ull;
    for(const auto byte:bytes) {hash^=std::to_integer<std::uint8_t>(byte);hash*=1099511628211ull;}
    return hash;
}
} // namespace isle::private_course
#endif

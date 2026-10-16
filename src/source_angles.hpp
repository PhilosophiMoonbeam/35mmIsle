#ifndef ISLE_SOURCE_ANGLES_HPP
#define ISLE_SOURCE_ANGLES_HPP

#include "rom_snapshot.hpp"
#include "rom_view.hpp"

namespace isle::source_math {
// Own GAME trig.c:5-42, not host atan2. The immutable 4097-entry table is
// borrowed from the admitted ROM; no embedded or reconstructed table payload.
class Angles final {
public:
    explicit Angles(const rom::Snapshot&);
    Angles(rom::Snapshot&&) = delete;
    Angles(const Angles&) = delete;
    Angles& operator=(const Angles&) = delete;
    Angles(Angles&&) = delete;
    Angles& operator=(Angles&&) = delete;
    // Source returns zero when both absolute components truncate to zero.
    // Only the source's defined finite float-to-s32 domain is admitted.
    // Rejection leaves output unchanged, including after Snapshot movement.
    bool atan2s(float y, float x, std::uint16_t& output) const noexcept;
    bool live() const noexcept;
private:
    const rom::Snapshot* snapshot_;
    const std::byte* source_data_;
    rom::GuestView table_;
};
} // namespace isle::source_math
#endif

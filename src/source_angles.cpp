#include "source_angles.hpp"

#include <cmath>
#include <stdexcept>

namespace isle::source_math {
namespace {
constexpr std::size_t table_offset = 0x534000, table_bytes = 4097 * 2;
constexpr std::uint32_t table_guest = 0x80393bf0;
constexpr std::array<std::uint8_t,20> admitted_sha1{
    0xed,0xc7,0xc4,0x9c,0xc5,0x68,0xc0,0x45,0xfe,0x48,
    0xbe,0x0d,0x18,0x01,0x1c,0x30,0xf3,0x93,0xcb,0xaf
};
}
Angles::Angles(const rom::Snapshot& snapshot)
    : snapshot_(&snapshot), source_data_(snapshot.canonical().data()) {
    const auto bytes = snapshot.canonical();
    if (bytes.size() != rom::us_size || snapshot.identity().size != rom::us_size ||
        snapshot.identity().sha1 != admitted_sha1)
        throw std::invalid_argument("Source angles require the admitted live US Snapshot");
    table_ = {table_guest, table_offset, bytes.subspan(table_offset, table_bytes)};
}
bool Angles::live() const noexcept {
    const auto bytes = snapshot_->canonical();
    return bytes.size() == rom::us_size && bytes.data() == source_data_;
}
bool Angles::atan2s(float y, float x, std::uint16_t& output) const noexcept {
    if (!live() || !std::isfinite(x) || !std::isfinite(y)) return false;
    const float ax = std::fabs(x), ay = std::fabs(y);
    // GAME always converts abs(x), but short-circuits the abs(y) conversion
    // when abs(x) truncates nonzero. Preserve that defined finite domain.
    if (ax >= 2147483648.0f || (ax < 1.0f && ay >= 2147483648.0f)) return false;
    if (ax < 1.0f && ay < 1.0f) { output = 0; return true; }
    const bool x_major = ax >= ay;
    const float scaled = (x_major ? ay : ax) * 4096.0f;
    const float ratio = scaled / (x_major ? ax : ay);
    if (!(ratio >= 0.0f && ratio <= 4096.0f)) return false;
    const auto index = static_cast<std::size_t>(ratio);
    std::int32_t angle = table_.u16(index * 2);
    if (x_major) {
        if (x >= 0.0f && y < 0.0f) angle = -angle;
        else if (x < 0.0f) angle = y >= 0.0f ? 0x4000 - angle : 0x4000 + angle;
    } else if (y >= 0.0f) {
        angle = x >= 0.0f ? 0x2000 - angle : 0x2000 + angle;
    } else {
        angle = x >= 0.0f ? 0x6000 + angle : 0x6000 - angle;
    }
    output = static_cast<std::uint16_t>(angle & 0x7fff);
    return true;
}
} // namespace isle::source_math

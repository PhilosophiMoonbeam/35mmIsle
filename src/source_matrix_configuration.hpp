#ifndef ISLE_SOURCE_MATRIX_CONFIGURATION_HPP
#define ISLE_SOURCE_MATRIX_CONFIGURATION_HPP

#include <array>
#include <cstdint>

namespace isle::menu::model {
// PokemonInitData selects configured kinds only with flag0x10. High tree flags
// subsequently decompose/override the effective sequence in the model kernel.
inline constexpr std::array<std::uint8_t, 3> select_source_matrix_kinds(
    std::uint32_t flags, std::array<std::uint8_t, 3> stored) noexcept {
    return flags & 0x10u ? stored : std::array<std::uint8_t, 3>{28, 0, 0};
}
} // namespace isle::menu::model
#endif

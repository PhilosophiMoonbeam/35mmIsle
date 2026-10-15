#ifndef ISLE_AUDIO_DSP_HPP
#define ISLE_AUDIO_DSP_HPP

#include "audio_synth_internal.hpp"

namespace isle::audio::detail {
// Shared low-level producer convention for voices and the static wet graph.
// Building writes command RAM only; mutable DSP storage changes on execution.
class DspWriter {
public:
    explicit DspWriter(Context& context) noexcept : context_(context) {}
    void buffers(std::uint16_t input, std::uint16_t output, std::uint16_t bytes) noexcept {
        context_.emit(0x08000000u | input, (static_cast<std::uint32_t>(output) << 16) | bytes);
    }
    void clear(std::uint16_t output, std::uint16_t bytes) noexcept {
        if (bytes != 0) context_.emit(0x02000000u | output, bytes);
    }
    void move(std::uint16_t input, std::uint16_t output, std::uint16_t bytes) noexcept {
        if (bytes != 0) context_.emit(0x0a000000u | input, (static_cast<std::uint32_t>(output) << 16) | bytes);
    }
    bool address(std::uint32_t value, std::uint32_t bytes) noexcept {
        if (context_.range(value, bytes)) return true;
        context_.write_error = Error::address_range;
        return false;
    }
    void load(std::uint32_t source, std::uint16_t dest, std::uint16_t bytes) noexcept {
        if (!address(source, bytes)) return;
        buffers(dest, 0, bytes);
        context_.emit(0x04000000u, source);
    }
    void save(std::uint16_t source, std::uint32_t dest, std::uint16_t bytes) noexcept {
        if (!address(dest, bytes)) return;
        buffers(0, source, bytes);
        context_.emit(0x06000000u, dest);
    }
protected:
    Context& context_;
};
} // namespace isle::audio::detail
#endif

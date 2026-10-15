#ifndef ISLE_RSP_RENDER_HPP
#define ISLE_RSP_RENDER_HPP

#include <filesystem>
#include <string_view>

namespace isle::rsp_probe { class Capture; }

namespace isle::rsp::probe {
// After-SP synthetic replay in separately initialized RDP-owned memory, not a
// shared production RDRAM/coherency test. The caller supplies a new empty directory.
// Throws on unsupported packets, device/backend errors or incomplete output.
void render_capture(const rsp_probe::Capture& capture,
                    const std::filesystem::path& output_directory,
                    std::string_view label, bool require_cpu_device);
} // namespace isle::rsp::probe
#endif

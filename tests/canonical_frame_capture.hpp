#ifndef ISLE_PRIVATE_CANONICAL_FRAME_CAPTURE_HPP
#define ISLE_PRIVATE_CANONICAL_FRAME_CAPTURE_HPP

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <span>
#include <stdexcept>

namespace isle::private_capture {
// Call while the actual completed-frame lease is valid. Canonical RDP pixels
// are not a window screenshot; record Host presentation and inspect it separately.
inline void write_frame(const std::filesystem::path& path,std::span<const std::byte> rgba) {
    if(rgba.size()!=320u*240u*4u)throw std::runtime_error("canonical frame capture has an invalid RGBA extent");
    std::ofstream out(path,std::ios::binary);
    if(!out)throw std::runtime_error("canonical frame capture could not be opened");
    out << "P6\n320 240\n255\n";
    std::array<char,320*3> row;
    for(std::size_t y=0;y<240;++y) {
        const auto* source=reinterpret_cast<const char*>(rgba.data()+y*1280);
        for(std::size_t x=0;x<320;++x)std::copy_n(source+x*4,3,row.data()+x*3);
        out.write(row.data(),static_cast<std::streamsize>(row.size()));
    }
    out.close();
    if(!out)throw std::runtime_error("canonical frame capture could not be retained");
}
} // namespace isle::private_capture
#endif

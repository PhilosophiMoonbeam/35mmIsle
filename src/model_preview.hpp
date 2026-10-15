#ifndef ISLE_MODEL_PREVIEW_HPP
#define ISLE_MODEL_PREVIEW_HPP

#include "rom_snapshot.hpp"
#include <libultraship/libultra/gbi.h>

#include <array>
#include <cstdint>
#include <memory>

// Bounded real native model presentation, not scene boot or a recorded image.
// Snapshot and this owner outlive all canonical work AND LUS/GL texture users.
// Creator-thread only. The host must finish GL and invalidate these three exact
// texture bases before preparing another frame; a declined draw retains bytes.
class ModelPreview final {
public:
    ModelPreview(const isle::rom::Snapshot& snapshot, bool require_cpu_device);
    ~ModelPreview();
    ModelPreview(const ModelPreview&) = delete;
    ModelPreview& operator=(const ModelPreview&) = delete;
    ModelPreview(ModelPreview&&) = delete;
    ModelPreview& operator=(ModelPreview&&) = delete;

    void PrepareFrame(std::uint32_t frame);
    Gfx* Commands() noexcept;
    std::array<const std::uint8_t*, 3> TextureBases() const noexcept;
    void Report() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

#endif

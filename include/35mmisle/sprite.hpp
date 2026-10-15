#ifndef ISLE_SPRITE_HPP
#define ISLE_SPRITE_HPP

#include "35mmisle/sprite.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace isle::sprite {

// The caller transfers an immutable bank reference, not a host relocation base.
// It must not retain a mutable alias to the vector or its bytes.
using Bank = std::vector<std::uint8_t>;
using BankOwner = std::shared_ptr<const Bank>;

enum class Error {
    none,
    invalid_owner,
    address_space_overflow,
    out_of_bank,
    invalid_scalar,
    unsupported_format,
    invalid_bitmap,
    invalid_palette,
    unsupported_external_state,
    bitmap_index_out_of_range,
    output_too_small,
    resource_failure
};

struct HydrateResult;

class Sprite {
public:
    Sprite() noexcept = default;

    // Copies and moves share stable native descriptor storage and retain the
    // immutable bank. Bare IsleSprite copies still require a live Sprite owner.
    const IsleSprite* view() const noexcept;
    explicit operator bool() const noexcept { return view() != nullptr; }

private:
    struct Storage;
    explicit Sprite(std::shared_ptr<const Storage> storage) noexcept;
    std::shared_ptr<const Storage> storage_;
    friend HydrateResult hydrate(BankOwner, std::uint32_t, std::uint32_t) noexcept;
};

struct HydrateResult {
    Error error = Error::none;
    Sprite asset;
    std::uint32_t address = 0; // Canonical address associated with a failure.

    explicit operator bool() const noexcept { return error == Error::none; }
};

// Decode one 68-byte BE Sprite and its bounded 16-byte BE Bitmap array. Every
// reachable texture and palette belongs to this bank; canonical addresses use
// unsigned arithmetic, never host pointer identity. Failure publishes no asset.
// Cache/display-list pointers at wire offsets 0x38/0x3c are deliberately omitted
// and never resolved. SP_EXTERN requires unspecified external drawing state and
// is explicitly unsupported. This is not a Sprite/RDP drawing implementation.
//
// Supported format/size pairs: RGBA16/32, CI4/8 (RGBA16 TLUT), IA4/8/16, I4/8.
// YUV requires conversion state not present here; other combinations reject.
// Palette entries cover [startTLUT, startTLUT+nTLUT). CI4 LUToffset selects a
// 16-entry palette bank (0..15); CI8 uses the whole table and ignores LUToffset.
//
// width_img is the stored row width, including any padding texels. Rows contain
// ceil(width_img * bits_per_texel / 8) canonical bytes, without implicit padding.
// Storage has positive actualHeight rows when specified; zero uses bmHreal.
// bmHreal is nominal, not permission to read invented rows after a short piece.
// A zero-width descriptor terminates bitmap use: it and all later descriptors
// retain scalars but have null native buffers. A NULL buf is a skipped bitmap,
// not malformed texture data. Aliased non-NULL canonical buffers remain aliases.
HydrateResult hydrate(BankOwner bank, std::uint32_t canonical_base,
                      std::uint32_t root_address) noexcept;

struct NormalizeResult {
    Error error = Error::none;
    std::size_t width = 0;
    std::size_t height = 0;
    std::size_t bytes_written = 0;

    explicit operator bool() const noexcept { return error == Error::none; }
};

// Produce unmodulated RGBA8 for the complete stored plane (width_img x positive
// actualHeight, or bmHreal when actualHeight is zero). The renderer applies
// width, s/t, placement, scale, tint, and alpha;
// none of those drawing operations are synthesized here. A skipped/terminated
// bitmap succeeds with zero dimensions and writes nothing. Output is tightly
// packed R,G,B,A bytes; any unused output capacity is unchanged. No allocation.
//
// SP_TEXSHUF reverses pre-shuffling while sampling: on odd stored rows byte
// offsets XOR 4 for <=16-bit formats, XOR 8 for RGBA32. Such multirow planes must
// have row byte counts divisible by 8 (16 for RGBA32); no guessed row padding is
// read. Canonical bank bytes are never altered. RGBA16 expands 5-bit channels by
// bit replication; I alpha equals intensity. CI palettes are BE RGBA5551.
//
// view/bitmap/palette spans must be live and output must not overlap any of them.
// Structural/capacity failures write nothing. A malformed CI index can leave a
// complete-texel prefix, counted by bytes_written; hydrated immutable data has
// already had all palette indices checked. Mutable C view copies may change
// placement/tint/attr, but changing pointers/extents requires rehydration.
NormalizeResult normalize_rgba8(const IsleSprite& view, std::size_t bitmap_index,
                                std::span<std::uint8_t> output) noexcept;

} // namespace isle::sprite

#endif

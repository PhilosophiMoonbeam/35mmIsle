#include "35mmisle/sprite.hpp"

#include <bit>
#include <cmath>
#include <limits>
#include <utility>

namespace isle::sprite {

struct Sprite::Storage {
    explicit Storage(BankOwner owner) : bank(std::move(owner)) {}

    BankOwner bank;
    std::vector<IsleBitmap> bitmaps;
    IsleSprite view{};
};

Sprite::Sprite(std::shared_ptr<const Storage> storage) noexcept
    : storage_(std::move(storage)) {}

const IsleSprite* Sprite::view() const noexcept {
    return storage_ ? &storage_->view : nullptr;
}

namespace {

static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559);

std::uint16_t be16(const std::uint8_t* bytes) noexcept {
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(bytes[0]) << 8) |
                                     bytes[1]);
}

std::uint32_t be32(const std::uint8_t* bytes) noexcept {
    return (static_cast<std::uint32_t>(bytes[0]) << 24) |
           (static_cast<std::uint32_t>(bytes[1]) << 16) |
           (static_cast<std::uint32_t>(bytes[2]) << 8) | bytes[3];
}

std::int16_t signed16(const std::uint8_t* bytes) noexcept {
    return std::bit_cast<std::int16_t>(be16(bytes));
}

float float32(const std::uint8_t* bytes) noexcept {
    return std::bit_cast<float>(be32(bytes));
}

class BankReader {
public:
    BankReader(const Bank& bank, std::uint32_t base) noexcept : bank_(bank), base_(base) {}

    const std::uint8_t* resolve(std::uint32_t address, std::size_t extent) const noexcept {
        if (address < base_) {
            return nullptr;
        }
        const auto offset = static_cast<std::uint64_t>(address) - base_;
        if (offset > bank_.size() || extent > bank_.size() - static_cast<std::size_t>(offset)) {
            return nullptr;
        }
        return bank_.data() + static_cast<std::size_t>(offset);
    }

private:
    const Bank& bank_;
    std::uint32_t base_;
};

Error format_bits(std::uint8_t format, std::uint8_t size, unsigned& bits) noexcept {
    // Numeric values are wire/interface facts, not imported SDK declarations.
    const bool supported =
        (format == 0 && (size == 2 || size == 3)) || // RGBA5551 / RGBA8888
        (format == 2 && size <= 1) ||               // CI4 / CI8, RGBA5551 TLUT
        (format == 3 && size <= 2) ||               // IA31 / IA44 / IA88
        (format == 4 && size <= 1);                 // I4 / I8
    if (!supported) {
        return Error::unsupported_format;
    }
    bits = 4U << size;
    return Error::none;
}

struct Layout {
    std::size_t width = 0;
    std::size_t height = 0;
    std::size_t row_bytes = 0;
    std::size_t byte_count = 0;
    unsigned bits = 0;
    std::size_t shuffle_mask = 0;
};

Error layout(const IsleSprite& sprite, const IsleBitmap& bitmap, Layout& result) noexcept {
    if (bitmap.width == 0 || bitmap.buf == nullptr) {
        return Error::none;
    }
    if (bitmap.width < 0 || bitmap.width_img <= 0 || sprite.bmHreal <= 0 ||
        sprite.bmheight < 0 || sprite.bmheight > sprite.bmHreal ||
        bitmap.actualHeight < 0 || bitmap.actualHeight > sprite.bmHreal ||
        (bitmap.s <= bitmap.width && bitmap.width > bitmap.width_img)) {
        return Error::invalid_bitmap;
    }
    auto error = format_bits(sprite.bmfmt, sprite.bmsiz, result.bits);
    if (error != Error::none) {
        return error;
    }
    result.width = static_cast<std::size_t>(bitmap.width_img);
    result.height = static_cast<std::size_t>(bitmap.actualHeight != 0 ? bitmap.actualHeight : sprite.bmHreal);
    const auto row_bytes = (static_cast<std::uint64_t>(result.width) * result.bits + 7) / 8;
    const auto byte_count = row_bytes * result.height;
    if (byte_count > std::numeric_limits<std::size_t>::max()) {
        return Error::invalid_bitmap;
    }
    result.row_bytes = static_cast<std::size_t>(row_bytes);
    result.byte_count = static_cast<std::size_t>(byte_count);
    if ((sprite.attr & ISLE_SP_TEXSHUF) != 0) {
        result.shuffle_mask = result.bits == 32 ? 8 : 4;
        if (result.height > 1 && result.row_bytes % (result.shuffle_mask * 2) != 0) {
            return Error::invalid_bitmap;
        }
    }
    return Error::none;
}

std::size_t source_byte(const Layout& storage, std::size_t x_byte, std::size_t row) noexcept {
    return row * storage.row_bytes + (x_byte ^ ((row & 1U) != 0 ? storage.shuffle_mask : 0));
}

unsigned palette_index(const IsleBitmap& bitmap, const Layout& storage,
                       std::size_t x, std::size_t y) noexcept {
    if (storage.bits == 4) {
        const auto packed = bitmap.buf[source_byte(storage, x / 2, y)];
        return ((x & 1U) == 0 ? packed >> 4 : packed & 15U) +
               static_cast<unsigned>(bitmap.LUToffset) * 16U;
    }
    return bitmap.buf[source_byte(storage, x, y)];
}

Error palette_layout(const IsleSprite& sprite, const IsleBitmap& bitmap) noexcept {
    if (sprite.startTLUT < 0 || sprite.nTLUT <= 0 ||
        static_cast<unsigned>(sprite.startTLUT) + static_cast<unsigned>(sprite.nTLUT) > 256 ||
        sprite.LUT == nullptr || sprite.LUT_bytes < static_cast<std::size_t>(sprite.nTLUT) * 2) {
        return Error::invalid_palette;
    }
    if (sprite.bmsiz == 0 && (bitmap.LUToffset < 0 || bitmap.LUToffset > 15)) {
        return Error::invalid_palette;
    }
    return Error::none;
}

bool palette_contains(const IsleSprite& sprite, unsigned index) noexcept {
    const auto start = static_cast<unsigned>(sprite.startTLUT);
    return index >= start && index - start < static_cast<unsigned>(sprite.nTLUT);
}

Error check_palette_indices(const IsleSprite& sprite, const IsleBitmap& bitmap,
                            const Layout& storage) noexcept {
    const auto error = palette_layout(sprite, bitmap);
    if (error != Error::none) {
        return error;
    }
    for (std::size_t y = 0; y < storage.height; ++y) {
        for (std::size_t x = 0; x < storage.width; ++x) {
            if (!palette_contains(sprite, palette_index(bitmap, storage, x, y))) {
                return Error::invalid_palette;
            }
        }
    }
    return Error::none;
}

std::uint8_t expand5(unsigned value) noexcept {
    return static_cast<std::uint8_t>((value << 3) | (value >> 2));
}

void rgba16(std::uint16_t value, std::uint8_t* output) noexcept {
    output[0] = expand5((value >> 11) & 31U);
    output[1] = expand5((value >> 6) & 31U);
    output[2] = expand5((value >> 1) & 31U);
    output[3] = (value & 1U) != 0 ? 255 : 0;
}

HydrateResult failure(Error error, std::uint32_t address) noexcept {
    return {error, {}, address};
}

} // namespace

HydrateResult hydrate(BankOwner bank, std::uint32_t canonical_base,
                      std::uint32_t root_address) noexcept {
    if (!bank) {
        return failure(Error::invalid_owner, root_address);
    }
    constexpr std::uint64_t address_limit = std::uint64_t{1} << 32;
    if (bank->size() > address_limit - canonical_base) {
        return failure(Error::address_space_overflow, canonical_base);
    }
    const BankReader reader(*bank, canonical_base);
    const auto* root = reader.resolve(root_address, 68);
    if (root == nullptr) {
        return failure(Error::out_of_bank, root_address);
    }
    try {
        IsleSprite view{};
        view.x = signed16(root);
        view.y = signed16(root + 2);
        view.width = signed16(root + 4);
        view.height = signed16(root + 6);
        view.scalex = float32(root + 8);
        view.scaley = float32(root + 12);
        view.expx = signed16(root + 16);
        view.expy = signed16(root + 18);
        view.attr = be16(root + 20);
        view.zdepth = signed16(root + 22);
        view.red = root[24];
        view.green = root[25];
        view.blue = root[26];
        view.alpha = root[27];
        view.startTLUT = signed16(root + 28);
        view.nTLUT = signed16(root + 30);
        view.istart = signed16(root + 36);
        view.istep = signed16(root + 38);
        view.nbitmaps = signed16(root + 40);
        view.ndisplist = signed16(root + 42);
        view.bmheight = signed16(root + 44);
        view.bmHreal = signed16(root + 46);
        view.bmfmt = root[48];
        view.bmsiz = root[49];
        view.frac_s = signed16(root + 64);
        view.frac_t = signed16(root + 66);
        if (view.startTLUT < 0 || view.nTLUT < 0 ||
            static_cast<unsigned>(view.startTLUT) + static_cast<unsigned>(view.nTLUT) > 256) {
            return failure(Error::invalid_palette, root_address);
        }
        const auto palette_address = be32(root + 32);
        if (view.nTLUT != 0) {
            view.LUT_bytes = static_cast<std::size_t>(view.nTLUT) * 2;
            view.LUT = palette_address == 0 ? nullptr : reader.resolve(palette_address, view.LUT_bytes);
            if (view.LUT == nullptr) {
                return failure(palette_address == 0 ? Error::invalid_palette : Error::out_of_bank,
                               palette_address);
            }
        }
        return hydrate_bitmaps(std::move(bank), canonical_base, view, be32(root + 52),
                               static_cast<std::size_t>(view.nbitmaps));
    } catch (...) {
        return failure(Error::resource_failure, root_address);
    }
}

HydrateResult hydrate_bitmaps(BankOwner bank, std::uint32_t canonical_base,
                              const IsleSprite& prototype, std::uint32_t bitmap_address,
                              std::size_t count) noexcept {
    if (!bank) return failure(Error::invalid_owner, bitmap_address);
    if (bank->size() > (std::uint64_t{1} << 32) - canonical_base)
        return failure(Error::address_space_overflow, canonical_base);
    const BankReader reader(*bank, canonical_base);
    try {
        auto storage = std::make_shared<Sprite::Storage>(std::move(bank));
        auto& view = storage->view;
        view = prototype;
        view.bitmap = nullptr;
        if ((view.attr & ISLE_SP_EXTERN) != 0)
            return failure(Error::unsupported_external_state, bitmap_address);
        if (view.width < 0 || view.height < 0 || !std::isfinite(view.scalex) ||
            !std::isfinite(view.scaley) || count > static_cast<std::size_t>(std::numeric_limits<std::int16_t>::max()) ||
            view.ndisplist < 0 || view.bmheight < 0 || view.bmHreal < 0 || view.istart < 0 ||
            (count == 0 ? view.istart != 0 : static_cast<std::size_t>(view.istart) >= count))
            return failure(Error::invalid_scalar, bitmap_address);
        view.nbitmaps = static_cast<std::int16_t>(count);
        unsigned bits = 0;
        const auto format_error = format_bits(view.bmfmt, view.bmsiz, bits);
        if (format_error != Error::none) return failure(format_error, bitmap_address);
        if (view.startTLUT < 0 || view.nTLUT < 0 ||
            static_cast<unsigned>(view.startTLUT) + static_cast<unsigned>(view.nTLUT) > 256)
            return failure(Error::invalid_palette, bitmap_address);
        if (view.nTLUT != 0) {
            const auto begin = reinterpret_cast<std::uintptr_t>(storage->bank->data());
            const auto palette = reinterpret_cast<std::uintptr_t>(view.LUT);
            const auto extent = static_cast<std::size_t>(view.nTLUT) * 2;
            if (!view.LUT || palette < begin || palette - begin > storage->bank->size() ||
                extent > storage->bank->size() - (palette - begin) || view.LUT_bytes < extent)
                return failure(Error::invalid_palette, bitmap_address);
        } else { view.LUT = nullptr; view.LUT_bytes = 0; }
        if (count == 0) {
            return {Error::none, Sprite(std::move(storage)), 0};
        }
        const auto* descriptors = bitmap_address == 0 ? nullptr : reader.resolve(bitmap_address, count * 16);
        if (descriptors == nullptr) {
            return failure(bitmap_address == 0 ? Error::invalid_bitmap : Error::out_of_bank,
                           bitmap_address);
        }
        storage->bitmaps.resize(count);
        bool terminated = false;
        for (std::size_t i = 0; i < count; ++i) {
            const auto* wire = descriptors + i * 16;
            auto& bitmap = storage->bitmaps[i];
            bitmap.width = signed16(wire);
            bitmap.width_img = signed16(wire + 2);
            bitmap.s = signed16(wire + 4);
            bitmap.t = signed16(wire + 6);
            bitmap.actualHeight = signed16(wire + 12);
            bitmap.LUToffset = signed16(wire + 14);
            const auto buffer_address = be32(wire + 8);
            terminated = terminated || bitmap.width == 0;
            if (terminated || buffer_address == 0) {
                continue;
            }
            // Resolve a byte before computing the full extent; no host address
            // is ever constructed from an unchecked canonical pointer.
            bitmap.buf = reader.resolve(buffer_address, 1);
            if (bitmap.buf == nullptr) {
                return failure(Error::out_of_bank, buffer_address);
            }
            Layout extent;
            const auto bitmap_error = layout(view, bitmap, extent);
            if (bitmap_error != Error::none) {
                return failure(bitmap_error, static_cast<std::uint32_t>(
                    static_cast<std::uint64_t>(bitmap_address) + i * 16));
            }
            bitmap.buf = reader.resolve(buffer_address, extent.byte_count);
            if (bitmap.buf == nullptr) {
                return failure(Error::out_of_bank, buffer_address);
            }
            bitmap.byte_count = extent.byte_count;
            if (view.bmfmt == 2) {
                const auto palette_error = check_palette_indices(view, bitmap, extent);
                if (palette_error != Error::none) {
                    return failure(palette_error, buffer_address);
                }
            }
        }
        view.bitmap = storage->bitmaps.data();
        return {Error::none, Sprite(std::move(storage)), 0};
    } catch (...) {
        return failure(Error::resource_failure, bitmap_address);
    }
}

NormalizeResult normalize_rgba8(const IsleSprite& view, std::size_t bitmap_index,
                                std::span<std::uint8_t> output) noexcept {
    if ((view.attr & ISLE_SP_EXTERN) != 0) {
        return {Error::unsupported_external_state};
    }
    if (view.nbitmaps < 0 || bitmap_index >= static_cast<std::size_t>(view.nbitmaps)) {
        return {Error::bitmap_index_out_of_range};
    }
    if (view.bitmap == nullptr) {
        return {Error::invalid_bitmap};
    }
    const auto& bitmap = view.bitmap[bitmap_index];
    Layout storage;
    const auto error = layout(view, bitmap, storage);
    if (error != Error::none) {
        return {error};
    }
    NormalizeResult result{Error::none, storage.width, storage.height, 0};
    if (storage.byte_count == 0) {
        return result;
    }
    if (bitmap.byte_count < storage.byte_count) {
        result.error = Error::invalid_bitmap;
        return result;
    }
    const auto output_bytes = static_cast<std::uint64_t>(storage.width) * storage.height * 4;
    if (output_bytes > output.size()) {
        result.error = Error::output_too_small;
        return result;
    }
    if (view.bmfmt == 2) {
        result.error = palette_layout(view, bitmap);
        if (result.error != Error::none) {
            return result;
        }
    }
    for (std::size_t y = 0; y < storage.height; ++y) {
        for (std::size_t x = 0; x < storage.width; ++x) {
            auto* destination = output.data() + result.bytes_written;
            if (view.bmfmt == 2) {
                const auto index = palette_index(bitmap, storage, x, y);
                if (!palette_contains(view, index)) {
                    result.error = Error::invalid_palette;
                    return result;
                }
                rgba16(be16(view.LUT + (index - static_cast<unsigned>(view.startTLUT)) * 2), destination);
            } else if (view.bmfmt == 0) {
                const auto offset = source_byte(storage, x * (storage.bits / 8), y);
                if (storage.bits == 16) {
                    rgba16(be16(bitmap.buf + offset), destination);
                } else {
                    destination[0] = bitmap.buf[offset];
                    destination[1] = bitmap.buf[offset + 1];
                    destination[2] = bitmap.buf[offset + 2];
                    destination[3] = bitmap.buf[offset + 3];
                }
            } else {
                std::uint8_t intensity;
                std::uint8_t alpha;
                if (storage.bits == 4) {
                    const auto packed = bitmap.buf[source_byte(storage, x / 2, y)];
                    const auto value = (x & 1U) == 0 ? packed >> 4 : packed & 15U;
                    if (view.bmfmt == 3) {
                        const auto three_bits = value >> 1;
                        intensity = static_cast<std::uint8_t>((three_bits << 5) | (three_bits << 2) |
                                                              (three_bits >> 1));
                        alpha = (value & 1U) != 0 ? 255 : 0;
                    } else {
                        intensity = static_cast<std::uint8_t>(value * 17U);
                        alpha = intensity;
                    }
                } else {
                    const auto offset = source_byte(storage, x * (storage.bits / 8), y);
                    const auto value = bitmap.buf[offset];
                    if (view.bmfmt == 3 && storage.bits == 16) {
                        intensity = value;
                        alpha = bitmap.buf[offset + 1];
                    } else if (view.bmfmt == 3) {
                        intensity = static_cast<std::uint8_t>((value >> 4) * 17U);
                        alpha = static_cast<std::uint8_t>((value & 15U) * 17U);
                    } else {
                        intensity = value;
                        alpha = value;
                    }
                }
                destination[0] = intensity;
                destination[1] = intensity;
                destination[2] = intensity;
                destination[3] = alpha;
            }
            result.bytes_written += 4;
        }
    }
    return result;
}

} // namespace isle::sprite

#include "photo_effects_renderer.hpp"

#include <bit>
#include <cmath>
#include <exception>
#include <limits>
#include <utility>

// Binary32 grouping follows the pinned GAME expressions. Compile without
// fast-math or floating-point contraction, as for menu_effects.cpp.
namespace isle::photo_effects {
namespace {
bool course_group(std::int32_t id, menu::EffectGroup& group, std::uint32_t& environment) noexcept {
    switch (id) {
    case 0: group = menu::EffectGroup::beach; environment = 0xffffff00; return true;
    case 1: group = menu::EffectGroup::tunnel; environment = 0x80800000; return true;
    case 2: group = menu::EffectGroup::volcano; environment = 0xff000000; return true;
    case 3: group = menu::EffectGroup::river; environment = 0x80808000; return true;
    case 4: group = menu::EffectGroup::cave; environment = 0xffffff00; return true;
    case 5: group = menu::EffectGroup::valley; environment = 0x80808000; return true;
    case 6: group = menu::EffectGroup::rainbow; environment = 0xffffff00; return true;
    default: return false;
    }
}
bool intersects(std::size_t a, std::size_t an, std::size_t b, std::size_t bn) noexcept {
    return an && bn && a < b + bn && b < a + an;
}
bool integer(float value, std::int32_t& output) noexcept {
    if (!std::isfinite(value) || double(value) < double(std::numeric_limits<std::int32_t>::min()) ||
        double(value) > double(std::numeric_limits<std::int32_t>::max())) return false;
    output = static_cast<std::int32_t>(value);
    return true;
}
unsigned mirror_mask(std::uint32_t dimension) noexcept {
    return dimension >= 2 && dimension <= 256 && !(dimension & (dimension - 1)) ?
        std::countr_zero(dimension) : 0;
}
std::uint32_t xy(std::int32_t x, std::int32_t y) noexcept {
    return ((static_cast<std::uint32_t>(x) & 0xfffu) << 12) |
        (static_cast<std::uint32_t>(y) & 0xfffu);
}
// Independent numeric encoding; pinned ultralib/include/PR/gbi.h:4498-4523,
// SHA256 940e538dde97a77d4850f7a220c304770904b95a2e743033e56105170834dba5.
// No SDK macro body is imported.
std::int16_t signed_half(std::int32_t value) noexcept {
    return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(value));
}
std::int32_t clipped_endpoint(std::int32_t value) noexcept {
    const auto narrowed = signed_half(value);
    return narrowed < 0 ? 0 : narrowed;
}
std::uint16_t clipped_start(std::int32_t position, std::int32_t derivative, bool clip) noexcept {
    if (!clip) return 0;
    const auto step = signed_half(derivative);
    const auto delta = (std::int32_t(signed_half(position)) * step) >> 7;
    const auto adjustment = step < 0 ? (delta > 0 ? delta : 0) : (delta < 0 ? delta : 0);
    return static_cast<std::uint16_t>(-adjustment);
}
struct Draw {
    const menu::EffectImage* image = nullptr;
    std::uint32_t texture = 0, palette = 0, primitive = 0, tile = 0;
    std::int32_t left = 0, top = 0, right = 0, bottom = 0, dsdx = 0, dtdy = 0;
    std::uint16_t flags = 0, s = 0, t = 0, depth = 0;
    bool load_palette = false, load_texture = false, set_lut = false;
    bool set_blend = false, set_alpha = false;
};
} // namespace

Packet::Packet(Renderer& owner, Storage storage, List setup, List body) noexcept
    : owner_(&owner), ram_(storage.ram), epoch_(storage.epoch), setup_(setup), body_(body) {}
Packet::~Packet() { release(); }
Packet::Packet(Packet&& other) noexcept
    : owner_(std::exchange(other.owner_, nullptr)), ram_(std::exchange(other.ram_, {})),
      epoch_(std::exchange(other.epoch_, 0)), setup_(std::exchange(other.setup_, {})),
      body_(std::exchange(other.body_, {})) {}
Packet& Packet::operator=(Packet&& other) noexcept {
    if (this != &other) {
        release();
        owner_ = std::exchange(other.owner_, nullptr);
        ram_ = std::exchange(other.ram_, {});
        epoch_ = std::exchange(other.epoch_, 0);
        setup_ = std::exchange(other.setup_, {});
        body_ = std::exchange(other.body_, {});
    }
    return *this;
}
void Packet::release() noexcept {
    if (owner_) {
        if (!owner_->borrowed_) std::terminate();
        owner_->borrowed_ = false;
        owner_ = nullptr;
    }
    ram_ = {}; epoch_ = 0; setup_ = {}; body_ = {};
}

Renderer::Renderer(const menu::EffectResources& resources,
                   const menu::EffectGraphicsResources& graphics, std::int32_t course) noexcept
    : course_(course) {
    menu::EffectGroup group;
    if (!course_group(course, group, environment_)) { status_ = {Error::course}; return; }
    try {
        banks_[0] = &resources.bank(group);
        banks_[3] = &resources.bank(menu::EffectGroup::common);
        installation_ = graphics.installation_spans();
        reservations_ = graphics.reservations();
        for (const auto& binding : graphics.bindings()) {
            if (binding.bank_id >= banks_.size() || !banks_[binding.bank_id] || bindings_[binding.bank_id]) {
                status_ = {Error::resource}; return;
            }
            bindings_[binding.bank_id] = &binding;
        }
        status_ = validate_bindings();
    } catch (const menu::EffectResourceError&) {
        status_ = {Error::resource};
    }
}
Renderer::~Renderer() { if (borrowed_) std::terminate(); }

Status Renderer::validate_bindings() const noexcept {
    const auto owns = [&](std::uint32_t address, std::size_t bytes) {
        if ((address & 7u) || address > menu::rdram_size || bytes > menu::rdram_size - address) return false;
        for (const auto& range : reservations_)
            if (address >= range.offset && address - range.offset <= range.size &&
                bytes <= range.size - (address - range.offset)) return true;
        return false;
    };
    const auto provenance = [&](std::uint32_t address, rom::GuestView source) {
        if (!owns(address, source.bytes.size())) return false;
        for (const auto& installed : installation_) {
            if (address < installed.address) continue;
            const auto offset = std::size_t(address - installed.address);
            if (offset <= installed.bytes.size() && source.bytes.size() <= installed.bytes.size() - offset &&
                static_cast<const void*>(installed.bytes.data() + offset) ==
                    static_cast<const void*>(source.bytes.data())) return true;
        }
        return false;
    };
    for (const auto& range : reservations_)
        if ((range.offset & 7u) || range.offset > menu::rdram_size || range.size > menu::rdram_size - range.offset)
            return {Error::resource};
    for (const auto& installed : installation_)
        if (installed.bytes.empty() || !owns(installed.address, installed.bytes.size()))
            return {Error::resource, menu::no_index, installed.address};
    for (std::size_t slot = 0; slot < banks_.size(); ++slot) {
        const auto* bank = banks_[slot];
        if (!bank) continue;
        const auto* binding = bindings_[slot];
        if (bank->bank_id != slot || !binding || binding->images.size() != bank->images.size())
            return {Error::resource};
        for (std::size_t texture = 0; texture < bank->images.size(); ++texture) {
            const auto& image = bank->images[texture];
            const auto& addresses = binding->images[texture];
            const bool ci = image.format == 2 && image.size_code == 0;
            // Complete formats admitted by EffectResources, never a conversion
            // to a substitute image or a generic sprite/material recipe.
            if (!ci && !((image.format == 4 && image.size_code <= 1) ||
                         (image.format == 3 && image.size_code == 1) ||
                         (image.format == 0 && (image.size_code == 2 || image.size_code == 3))))
                return {Error::resource};
            if (!image.width || !image.height || image.width > 1024 || image.height > 1024 || image.frames.empty())
                return {Error::extent};
            const auto pixels = std::size_t(image.width) * image.height;
            const auto bytes = (pixels * (4u << image.size_code) + 7u) / 8u;
            const auto load_bytes = image.size_code == 0 ? ((pixels + 3u) / 4u) * 2u :
                image.size_code == 1 ? ((pixels + 1u) / 2u) * 2u : bytes;
            if (bytes > (ci ? 2048u : 4096u) || load_bytes > bytes ||
                addresses.frames.size() != image.frames.size() || addresses.palettes.size() != image.palettes.size() ||
                image.palettes.size() != (ci ? ((image.flags & 1u) ? 1u : image.frames.size()) : 0u))
                return {Error::extent};
            for (std::size_t frame = 0; frame < image.frames.size(); ++frame)
                if (image.frames[frame].bytes.size() != bytes || !provenance(addresses.frames[frame], image.frames[frame]))
                    return {Error::resource, menu::no_index, addresses.frames[frame]};
            for (std::size_t palette = 0; palette < image.palettes.size(); ++palette)
                if (image.palettes[palette].bytes.size() != 32 || !provenance(addresses.palettes[palette], image.palettes[palette]))
                    return {Error::resource, menu::no_index, addresses.palettes[palette]};
        }
    }
    return {};
}

Result Renderer::prepare(const IslePlayerPhotoData& photo, const menu::EffectCamera& camera, Storage storage) noexcept {
    if (photo.levelID != course_) return {{Error::course}};
    return prepare(std::span<const IslePlayerEffectPhoto, effect_count>(photo.effects), camera, storage);
}
Result Renderer::prepare(std::span<const IslePlayerEffectPhoto, effect_count> effects,
                         const menu::EffectCamera& camera, Storage storage) noexcept {
    if (borrowed_) return {{Error::borrowed}};
    if (!status_) return {status_};
    if (storage.ram.size() != menu::rdram_size / sizeof(std::uint32_t) || !storage.epoch ||
        (storage.commands.address & 7u) || (storage.commands.size & 7u) ||
        storage.commands.address > menu::rdram_size || storage.commands.size > menu::rdram_size - storage.commands.address)
        return {{Error::storage, menu::no_index, storage.commands.address}};
    for (const auto& range : reservations_)
        if (intersects(storage.commands.address, storage.commands.size, range.offset, range.size))
            return {{Error::storage, menu::no_index, storage.commands.address}};
    for (const auto& range : storage.protected_ranges) {
        if (range.address > menu::rdram_size || range.size > menu::rdram_size - range.address ||
            intersects(storage.commands.address, storage.commands.size, range.address, range.size))
            return {{Error::storage, menu::no_index, range.address}};
    }
    const auto& matrix = camera.projection;
    for (const auto& row : matrix) for (const auto value : row)
        if (!std::isfinite(value)) return {{Error::arithmetic}};
    for (unsigned axis = 0; axis < 3; ++axis)
        if (!std::isfinite(camera.viewport_scale[axis]) || !std::isfinite(camera.viewport_translate[axis]))
            return {{Error::arithmetic}};
    const float scale_x = std::sqrt(matrix[0][0] * matrix[0][0] + matrix[1][0] * matrix[1][0] + matrix[2][0] * matrix[2][0]);
    const float scale_y = std::sqrt(matrix[0][1] * matrix[0][1] + matrix[1][1] * matrix[1][1] + matrix[2][1] * matrix[2][1]);
    if (!std::isfinite(scale_x) || !std::isfinite(scale_y)) return {{Error::arithmetic}};
    std::array<Draw, effect_count> draws{};
    std::size_t count = 0;
    for (std::size_t row = 0; row < effects.size(); ++row) {
        const auto& effect = effects[row];
        if (effect.textureID < 0) continue; // Source skips each inactive row, not the remaining table.
        const float px = effect.posX * 0.125f, py = effect.posY * 0.125f, pz = effect.posZ * 0.125f;
        float x = matrix[0][0] * px + matrix[1][0] * py + matrix[2][0] * pz + matrix[3][0];
        float y = matrix[0][1] * px + matrix[1][1] * py + matrix[2][1] * pz + matrix[3][1];
        float z = matrix[0][2] * px + matrix[1][2] * py + matrix[2][2] * pz + matrix[3][2];
        float inverse_w = matrix[0][3] * px + matrix[1][3] * py + matrix[2][3] * pz + matrix[3][3];
        if (inverse_w == 0.0f) continue;
        inverse_w = 1.0f / inverse_w;
        x *= inverse_w; y *= inverse_w; z *= inverse_w;
        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) return {{Error::arithmetic, row}};
        if (x < -1.0f || x > 1.0f || y < -1.0f || y > 1.0f || z < -1.0f || z > 1.0f) continue;
        inverse_w *= effect.size / 128.0f;
        float right = (inverse_w * scale_x + x) * camera.viewport_scale[0] + camera.viewport_translate[0];
        float bottom = (inverse_w * scale_y + y) * -camera.viewport_scale[1] + camera.viewport_translate[1];
        x = x * camera.viewport_scale[0] + camera.viewport_translate[0];
        y = y * -camera.viewport_scale[1] + camera.viewport_translate[1];
        float left, top;
        if (x < right) left = x - (right - x); else { left = right; right = x - (right - x); }
        if (y < bottom) top = y - (bottom - y); else { top = bottom; bottom = y - (bottom - y); }
        const auto bank_id = effect.bankID & 7u;
        const auto* bank = banks_[bank_id];
        const auto texture_id = static_cast<std::uint8_t>(effect.textureID);
        if (!bank) return {{Error::resource, row}};
        if (texture_id >= bank->images.size()) return {{Error::extent, row}};
        const auto& image = bank->images[texture_id];
        const auto& binding = bindings_[bank_id]->images[texture_id];
        if (effect.dataID >= binding.frames.size()) return {{Error::extent, row}};
        Draw draw;
        draw.image = &image; draw.texture = binding.frames[effect.dataID];
        draw.flags = static_cast<std::uint16_t>(effect.unk_03) << 4;
        if (image.format == 2) {
            const std::size_t palette = (draw.flags & 0x10u) ? 0u : effect.dataID;
            if (palette >= binding.palettes.size()) return {{Error::extent, row}};
            draw.palette = binding.palettes[palette];
        }
        std::int32_t depth = 0;
        if (!integer(left, draw.left) || !integer(top, draw.top) || !integer(right, draw.right) || !integer(bottom, draw.bottom) ||
            !integer((image.width * 4096.0f) / (right - left), draw.dsdx) ||
            !integer((image.height * 4096.0f) / (bottom - top), draw.dtdy) ||
            !integer((z * camera.viewport_scale[2] + camera.viewport_translate[2]) * 32.0f, depth))
            return {{Error::arithmetic, row}};
        const bool mirror_s = draw.flags & 0x20u, mirror_t = draw.flags & 0x40u;
        if ((mirror_s && (draw.dsdx > std::numeric_limits<std::int32_t>::max() / 2 ||
                          draw.dsdx < std::numeric_limits<std::int32_t>::min() / 2)) ||
            (mirror_t && (draw.dtdy > std::numeric_limits<std::int32_t>::max() / 2 ||
                          draw.dtdy < std::numeric_limits<std::int32_t>::min() / 2)))
            return {{Error::arithmetic, row}};
        if (mirror_s) draw.dsdx *= 2;
        if (mirror_t) draw.dtdy *= 2;
        draw.tile = ((mirror_t ? 1u : 2u) << 18) | ((mirror_t ? mirror_mask(image.height) : 0u) << 14) |
            ((mirror_s ? 1u : 2u) << 8) | ((mirror_s ? mirror_mask(image.width) : 0u) << 4);
        // Public signed16 packet fields and arithmetic-floor clipping, not
        // truncation of a positive distance. The Y clipping predicate uses
        // the original signed32 endpoint; X tests its signed16 field.
        draw.s = clipped_start(draw.left, draw.dsdx, signed_half(draw.left) < 0);
        draw.t = clipped_start(draw.top, draw.dtdy, draw.top < 0);
        draw.left = clipped_endpoint(draw.left); draw.top = clipped_endpoint(draw.top);
        draw.right = clipped_endpoint(draw.right); draw.bottom = clipped_endpoint(draw.bottom);
        draw.depth = static_cast<std::uint16_t>(depth);
        draw.primitive = (std::uint32_t(effect.primColor[0]) << 24) | (std::uint32_t(effect.primColor[1]) << 16) |
            (std::uint32_t(effect.primColor[2]) << 8) | effect.primColor[3];
        draws[count++] = draw;
    }
    // Plan the exact Source pointer caches once, after all projection/resource
    // checks. Publication does not repeat projection, allocation or admission.
    std::size_t required = 44; // setup9+END, body setup8, restore3+END.
    std::uint32_t last_texture = 0, last_palette = 0;
    bool texture_valid = false, palette_valid = false, blend_valid = false;
    int last_lut = -1, last_alpha = -1;
    for (std::size_t index = 0; index < count; ++index) {
        auto& draw = draws[index];
        const bool ci = draw.image->format == 2;
        if (ci && (!palette_valid || last_palette != draw.palette)) {
            draw.load_palette = true; required += 12; palette_valid = true; last_palette = draw.palette;
        }
        const int lut = ci ? 2 : 0;
        if (last_lut != lut) { draw.set_lut = true; required += 2; last_lut = lut; }
        if (!texture_valid || last_texture != draw.texture) {
            draw.load_texture = true; required += 14; texture_valid = true; last_texture = draw.texture;
        }
        const int alpha = (draw.flags & 0x400u) ? 3 : 1;
        if (alpha == 1 && !blend_valid) { draw.set_blend = true; required += 2; blend_valid = true; }
        if (last_alpha != alpha) { draw.set_alpha = true; required += 2; last_alpha = alpha; }
        required += 12; // primitive, combiner, primitive depth and three-word-pair rectangle.
    }
    if (last_lut != 0) required += 2;
    if (required > storage.commands.size / sizeof(std::uint32_t)) return {{Error::capacity}, required, count};
    std::size_t cursor = storage.commands.address / sizeof(std::uint32_t);
    const auto command = [&](std::uint32_t first, std::uint32_t second) {
        storage.ram[cursor++] = first; storage.ram[cursor++] = second;
    };
    const auto mode = [&](bool high, unsigned shift, unsigned length, std::uint32_t value) {
        command((high ? 0xe3000000u : 0xe2000000u) | ((32u - shift - length) << 8) | (length - 1u), value);
    };
    // 1395-1403, before the caller's real combine-all-lanes boundary.
    command(0xe7000000, 0); mode(true, 20, 2, 0); mode(false, 3, 29, 0x004049d8);
    mode(true, 6, 2, 0); mode(true, 4, 2, 0); command(0xfb000000, environment_);
    mode(true, 19, 1, 0x80000); mode(false, 2, 1, 0); mode(true, 14, 2, 0);
    command(0xdf000000, 0);
    constexpr std::size_t setup_words = 20;
    // 1424-1431. The recorded renderer always uses primitive Z, magic-square
    // color dither, pattern alpha dither and this course's original environment.
    command(0xe7000000, 0); mode(true, 20, 2, 0); mode(false, 3, 29, 0x004049d8);
    mode(true, 19, 1, 0); mode(false, 2, 1, 4); mode(true, 6, 2, 0); mode(true, 4, 2, 0);
    command(0xfb000000, environment_);
    for (std::size_t index = 0; index < count; ++index) {
        const auto& draw = draws[index]; const auto& image = *draw.image;
        if (draw.load_palette) {
            // Same legal CI4 palette0 legalization as EffectRuntime::render:
            // Source pal256 overreads a 32-byte palette; only these16 RGBA16
            // entries are addressable. No fabricated/copied palette tail.
            command(0xfd100000, draw.palette); command(0xe8000000, 0);
            command(0xf5000100, 0x07000000); command(0xe6000000, 0);
            command(0xf0000000, 0x0703c000); command(0xe7000000, 0);
        }
        if (draw.set_lut) mode(true, 14, 2, image.format == 2 ? 0x8000 : 0);
        if (draw.load_texture) {
            const auto size = image.size_code, format = image.format << 21;
            const auto load_size = size == 3 ? 3u : 2u;
            const auto pixels = image.width * image.height;
            const auto load_texels = size == 0 ? (pixels + 3u) >> 2 : size == 1 ? (pixels + 1u) >> 1 : pixels;
            const auto row_words = size == 0 ? image.width >> 4 : (image.width << (size - 1u)) >> 3;
            const auto words = row_words ? row_words : 1u;
            const auto dxt = (2048u + words - 1u) / words;
            const auto line = size == 0 ? ((image.width >> 1) + 7u) >> 3 :
                ((image.width << (size == 3 ? 1u : size - 1u)) + 7u) >> 3;
            command(0xfd000000 | format | (load_size << 19), draw.texture);
            command(0xf5000000 | format | (load_size << 19), 0x07000000 | draw.tile);
            command(0xe6000000, 0);
            command(0xf3000000, 0x07000000 | ((load_texels - 1u) << 12) | dxt);
            command(0xe7000000, 0);
            command(0xf5000000 | format | (size << 19) | (line << 9), draw.tile);
            command(0xf2000000, ((image.width - 1u) << 14) | ((image.height - 1u) << 2));
            // Deliberately no live-effect.c >=2048 palette-cache invalidation:
            // the recorded-photo renderer1573-1603 has no such operation.
        }
        command(0xfa000000, draw.primitive);
        if (draw.flags & 0x80u) {
            command(0xfc000000 | (3u << 20) | (1u << 15) | (3u << 12) | (1u << 9) | (3u << 5) | 1u,
                (5u << 28) | (5u << 24) | (3u << 21) | (1u << 18) | (5u << 15) |
                (5u << 12) | (5u << 9) | (5u << 6) | (5u << 3) | 5u);
        } else if (draw.flags & 0x100u) {
            command(0xfc000000 | (7u << 20) | (1u << 15) | (1u << 12) | (3u << 9) | (7u << 5) | 1u,
                (8u << 28) | (8u << 24) | (1u << 21) | (3u << 18) | (7u << 15) |
                (7u << 12) | (7u << 9) | (7u << 6) | (7u << 3) | 7u);
        } else {
            command(0xfc000000 | (1u << 20) | (3u << 15) | (1u << 12) | (3u << 9) | (1u << 5) | 3u,
                (8u << 28) | (8u << 24) | (1u << 21) | (3u << 18) | (7u << 15) |
                (7u << 12) | (7u << 9) | (7u << 6) | (7u << 3) | 7u);
        }
        if (draw.set_blend) command(0xf9000000, 8);
        if (draw.set_alpha) mode(false, 0, 2, (draw.flags & 0x400u) ? 3u : 1u);
        command(0xee000000, std::uint32_t(draw.depth) << 16);
        command(0xe4000000 | xy(draw.right, draw.bottom), xy(draw.left, draw.top));
        command(0xe1000000, (std::uint32_t(draw.s) << 16) | draw.t);
        command(0xf1000000, ((static_cast<std::uint32_t>(draw.dsdx) & 0xffffu) << 16) |
            (static_cast<std::uint32_t>(draw.dtdy) & 0xffffu));
    }
    // Exact recorded renderer restoration1638-1643, not a general RDP reset.
    command(0xe7000000, 0); mode(true, 19, 1, 0x80000); mode(false, 2, 1, 0);
    if (last_lut != 0) mode(true, 14, 2, 0);
    command(0xdf000000, 0);
    borrowed_ = true;
    return {{}, required, count, Packet(*this, storage,
        {storage.commands.address, setup_words},
        {storage.commands.address + static_cast<std::uint32_t>(setup_words * sizeof(std::uint32_t)), required - setup_words})};
}
} // namespace isle::photo_effects

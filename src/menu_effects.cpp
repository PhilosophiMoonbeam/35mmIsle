#include "menu_effects.hpp"

#include <bit>
#include <cmath>
#include <initializer_list>
#include <limits>

// Own GAME df8815e75f3d79618466e01e0827c6165b93d11c effect.c/effect.h.
// Binary32 source expression grouping; compile without fast-math/contraction.
// Native C float libm parity with Original remains unqualified.
namespace isle::menu {
namespace {
constexpr float tau = 6.2831855f;
std::uint8_t byte(const rom::GuestView& v, std::size_t p) noexcept {
    return std::to_integer<std::uint8_t>(v.bytes[p]);
}
float scalar(const rom::GuestView& v, std::size_t& p) noexcept {
    std::uint32_t w = 0;
    for (unsigned i = 0; i < 4; ++i) w = (w << 8) | byte(v, p++);
    return std::bit_cast<float>(w);
}
std::uint16_t duration(const rom::GuestView& v, std::size_t& p) noexcept {
    unsigned n = byte(v, p++);
    if (n & 128) n = ((n & 127) << 8) + byte(v, p++);
    return static_cast<std::uint16_t>(n + 1);
}
std::uint8_t operation(std::uint8_t op) noexcept {
    if (op >= 0x80 && op <= 0x9f) return op & 0xf8;
    if ((op & 0xf0) == 0xc0 || (op & 0xf0) == 0xd0) return op & 0xf0;
    return op;
}
bool admitted(std::uint8_t raw) noexcept {
    if (raw < 128) return true;
    const auto op = operation(raw);
    return op == 0xa0 || op == 0xa1 || op == 0xa9 || op == 0xab ||
        op == 0xac || op == 0xad || op == 0xc0 || op == 0xd0 ||
        op == 0xfc || op == 0xfd || op == 0xff;
}
float magnitude(Vec3 v) noexcept { return std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z); }
std::uint32_t rgba(const std::array<std::uint8_t, 4>& c) noexcept {
    return (std::uint32_t(c[0]) << 24) | (std::uint32_t(c[1]) << 16) |
        (std::uint32_t(c[2]) << 8) | c[3];
}
void interpolate(std::array<std::uint8_t, 4>& c, const std::array<std::uint8_t, 4>& t,
                 std::uint16_t& n) noexcept {
    if (!n) return;
    for (unsigned k = 0; k < 4; ++k) {
        const int value = (int(c[k]) << 16) + (int(t[k]) - int(c[k])) * (65536 / int(n));
        c[k] = static_cast<std::uint8_t>(value >> 16);
    }
    --n;
}
bool finite(Vec3 v) noexcept { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }
}

MenuEffects::MenuEffects(const Catalog& catalog, isle_rng& rng) noexcept
    : rng_(&rng) {
    try { bank_ = &catalog.effects(); } catch (...) { status_ = {EffectsError::resource}; return; }
    if (bank_->particle_pool != 100 || bank_->effect_pool != 10 || bank_->bank_id != 0 ||
        bank_->scripts.size() != 4 || bank_->images.size() != 2) {
        status_ = {EffectsError::resource}; return;
    }
    for (const std::size_t script : {std::size_t(0), std::size_t(3)}) {
        const auto& s = bank_->scripts[script];
        if ((s.kind != 0 && s.kind != 3) || s.texture >= bank_->images.size() || (s.flags & ~0x93u)) {
            status_ = {EffectsError::unsupported, script}; return;
        }
        for (const auto& ins : s.instructions) {
            if (!admitted(ins.opcode)) { status_ = {EffectsError::unsupported, script, ins.offset}; return; }
            if (ins.offset >= s.bytecode.bytes.size() || ins.size > s.bytecode.bytes.size() - ins.offset) {
                status_ = {EffectsError::extent, script, ins.offset}; return;
            }
        }
        for (float f : s.parameters) if (!std::isfinite(f)) { status_ = {EffectsError::resource, script}; return; }
    }
    for (const auto& image : bank_->images) {
        if ((image.format != 4 && image.format != 2) || image.size_code != 0 ||
            image.width != 32 || image.height != 32 || image.frames.empty() || image.frames.size() > 6) {
            status_ = {EffectsError::unsupported}; return;
        }
        if (image.format == 2 && image.palettes.size() != ((image.flags & 1) ? 1 : image.frames.size())) {
            status_ = {EffectsError::resource}; return;
        }
    }
    reset();
}
void MenuEffects::reset() noexcept {
    particle_head_ = effect_head_ = absent;
    particle_free_ = effect_free_ = 0;
    current_id_ = 0;
    for (std::size_t i = 0; i < particles_.size(); ++i) {
        particles_[i] = {}; particles_[i].next = i + 1 < particles_.size() ? int(i + 1) : absent;
    }
    for (std::size_t i = 0; i < effects_.size(); ++i) {
        effects_[i] = {}; effects_[i].next = i + 1 < effects_.size() ? int(i + 1) : absent;
    }
}
int MenuEffects::spawn_effect(std::size_t script, EffectAttachment attachment) noexcept {
    if (effect_free_ == absent) return absent; // GAME exhaustion is a null spawn.
    const int index = effect_free_; auto& e = effects_[index];
    effect_free_ = e.next; e = {}; e.next = effect_head_; effect_head_ = index;
    const auto& s = bank_->scripts[script];
    e.id = ++current_id_; e.script = script; e.attachment = attachment;
    e.lifetime = s.effect_lifetime; e.flags = static_cast<std::uint16_t>(s.flags);
    if (bank_->images[s.texture].flags) e.flags |= 0x10;
    e.velocity = {s.parameters[2], s.parameters[3], s.parameters[4]};
    return index;
}
EffectEventResult MenuEffects::animation_event(EffectAttachment attachment, std::int32_t event,
                                              float argument) noexcept {
    if (!status_) return {status_};
    if (event == -1 || event == -2) return {{}, true};
    if (!std::isfinite(argument) || argument - 1.0f < float(std::numeric_limits<int>::min()) ||
        double(argument - 1.0f) > double(std::numeric_limits<int>::max()))
        return {{EffectsError::arithmetic}};
    const int script = int(argument - 1.0f);
    if (script < 0) return {};
    if (event != 0 || (script != 0 && script != 3)) return {{EffectsError::unsupported, std::size_t(script)}};
    const int index = spawn_effect(std::size_t(script), attachment);
    return {{}, false, index != absent, index == absent ? std::uint16_t(0) : effects_[index].id};
}
int MenuEffects::spawn_particle(int effect, Vec3 position, Vec3 velocity) noexcept {
    if (particle_free_ == absent) return absent;
    const int index = particle_free_; auto& p = particles_[index];
    particle_free_ = p.next; p = {}; p.next = particle_head_; particle_head_ = index;
    const auto& e = effects_[effect]; const auto& s = bank_->scripts[e.script];
    p.script = e.script;
    p.value.position = position; p.value.velocity = velocity;
    p.value.effect_id = e.id; p.value.flags = e.flags;
    p.value.texture = static_cast<std::uint8_t>(s.texture);
    p.value.size = s.parameters[8]; p.gravity = s.parameters[0]; p.friction = s.parameters[1];
    p.value.lifetime = static_cast<std::uint16_t>(s.particle_lifetime + 1);
    p.value.primitive.fill(255); p.value.environment.fill(0);
    tick_particle(index, absent); // GAME fx_makeParam's immediate first tick.
    return index;
}
int MenuEffects::tick_particle(int index, int previous) noexcept {
    auto& p = particles_[index]; auto& v = p.value;
    if (v.flags & 0x800) return p.next;
    const auto& code = bank_->scripts[p.script].bytecode;
    if (p.timer && --p.timer == 0) {
        std::size_t cursor = p.cursor;
        bool stop = false;
        while (!stop) {
            if (cursor >= code.bytes.size()) { status_ = {EffectsError::extent, p.script, cursor}; return p.next; }
            const auto raw = byte(code, cursor++); const auto op = operation(raw);
            if (raw < 128) {
                unsigned wait = raw & 31;
                if (raw & 32) wait = byte(code, cursor++) + (wait << 8);
                if ((raw & 0xc0) == 0x40) v.frame = byte(code, cursor++);
                if (wait) { p.timer = static_cast<std::uint16_t>(wait); stop = true; }
            } else if (op == 0xc0 || op == 0xd0) {
                auto& target = op == 0xc0 ? p.primitive_target : p.environment_target;
                auto& current = op == 0xc0 ? v.primitive : v.environment;
                auto& time = op == 0xc0 ? p.primitive_time : p.environment_time;
                time = duration(code, cursor); target = current;
                for (unsigned k = 0; k < 4; ++k) if (raw & (1u << k)) target[k] = byte(code, cursor++);
                if (time == 1) { current = target; time = 0; }
            } else switch (op) {
            case 0xa0: case 0xac:
                p.size_time = duration(code, cursor); p.size_target = scalar(code, cursor);
                if (op == 0xac) { const float range = scalar(code, cursor); p.size_target += range * isle_rng_next_float(rng_); }
                if (p.size_time == 1) { v.size = p.size_target; p.size_time = 0; } break;
            case 0xa1: v.flags = byte(code, cursor++); break;
            case 0xa9: {
                const float angle = scalar(code, cursor); const Vec3 old = v.velocity;
                const float pitch = std::atan2(old.y, old.z), sp = std::sin(pitch), cp = std::cos(pitch);
                const float yaw = std::atan2(old.x, old.y * sp + old.z * cp), sy = std::sin(yaw), cy = std::cos(yaw);
                const float mag = magnitude(old), t = isle_rng_next_float(rng_) * tau;
                const float sa = std::sin(angle) * mag, x = std::cos(t) * sa, y = std::sin(t) * sa;
                const float ca = std::cos(angle) * mag;
                v.velocity = {x*cy + ca*sy, -x*sp*sy + y*cp + ca*sp*cy, -x*cp*sy - y*sp + ca*cp*cy}; break;
            }
            case 0xab: { const float f = scalar(code, cursor); v.velocity.x *= f; v.velocity.y *= f; v.velocity.z *= f; break; }
            case 0xad: v.flags |= 0x80; break;
            case 0xfc: p.return_cursor = cursor; break;
            case 0xfd: cursor = p.return_cursor; break;
            case 0xff: v.lifetime = 1; stop = true; break;
            default: status_ = {EffectsError::unsupported, p.script, cursor - 1}; return p.next;
            }
        }
        p.cursor = cursor;
    }
    if (p.size_time) { v.size += (p.size_target - v.size) / p.size_time; --p.size_time; }
    interpolate(v.primitive, p.primitive_target, p.primitive_time);
    interpolate(v.environment, p.environment_target, p.environment_time);
    if (--v.lifetime == 0) {
        const int next = p.next;
        if (previous == absent) particle_head_ = next; else particles_[previous].next = next;
        p.next = particle_free_; particle_free_ = index; return next;
    }
    if (v.flags & 1) v.velocity.y -= p.gravity;
    if (v.flags & 2) { v.velocity.x *= p.friction; v.velocity.y *= p.friction; v.velocity.z *= p.friction; }
    v.position.x += v.velocity.x; v.position.y += v.velocity.y; v.position.z += v.velocity.z;
    if (!finite(v.position) || !finite(v.velocity) || !std::isfinite(v.size)) status_ = {EffectsError::arithmetic, p.script, p.cursor};
    return p.next;
}
EffectsStatus MenuEffects::update_particles() noexcept {
    if (!status_) return status_;
    int previous = absent, index = particle_head_;
    while (index != absent) {
        const int next = tick_particle(index, previous);
        if (!status_) return status_;
        if (particles_[index].next == next) previous = index;
        index = next;
    }
    return {};
}
void MenuEffects::remove_effect(int index, int previous) noexcept {
    auto& e = effects_[index];
    if (previous == absent) effect_head_ = e.next; else effects_[previous].next = e.next;
    e.next = effect_free_; effect_free_ = index;
}
EffectsStatus MenuEffects::update_effects(EffectWorldResolver resolve, void* context) noexcept {
    if (!status_) return status_;
    // Resolve every live attachment before any pool/RNG mutation, so an invalid
    // node cannot produce a partially published tick.
    std::array<EffectWorld, 10> worlds{};
    for (int i = effect_head_; i != absent; i = effects_[i].next) {
        if (!effects_[i].attachment.instance || (effects_[i].flags & 0x800)) continue;
        if (!resolve || !resolve(context, effects_[i].attachment, worlds[i])) return {EffectsError::attachment, effects_[i].script};
        for (const auto& row : worlds[i].matrix) for (float f : row)
            if (!std::isfinite(f)) return {EffectsError::arithmetic, effects_[i].script};
        auto& m = worlds[i].matrix;
        for (unsigned col = 0; col < 3; ++col) {
            if (m[0][col] == 0 && m[1][col] == 0 && m[2][col] == 0) continue;
            const float n = std::sqrt(m[0][col]*m[0][col] + m[1][col]*m[1][col] + m[2][col]*m[2][col]);
            if (n == 0 || !std::isfinite(n) || !std::isfinite(1.0f/n))
                return {EffectsError::arithmetic, effects_[i].script};
            const float inv = 1.0f/n;
            m[0][col] *= inv; m[1][col] *= inv; m[2][col] *= inv;
        }
    }
    int previous = absent;
    for (int i = effect_head_; i != absent;) {
        auto& e = effects_[i]; const auto& s = bank_->scripts[e.script];
        const int next = e.next;
        if (e.flags & 0x800) { previous = i; i = next; continue; }
        const float rate = s.parameters[7];
        if (rate < 0.0f) e.accumulator -= rate;
        else e.accumulator += isle_rng_next_float(rng_) * rate;
        Vec3 velocity = e.velocity;
        float azimuth = 0, step = 0;
        if (e.accumulator >= 1.0f) {
            if (e.attachment.instance) {
                const auto& matrix = worlds[i].matrix;
                e.position = {matrix[3][0], matrix[3][1], matrix[3][2]};
                const Vec3 v = velocity;
                velocity = {matrix[0][0]*v.x + matrix[1][0]*v.y + matrix[2][0]*v.z,
                            matrix[0][1]*v.x + matrix[1][1]*v.y + matrix[2][1]*v.z,
                            matrix[0][2]*v.x + matrix[1][2]*v.y + matrix[2][2]*v.z};
            }
            if (!std::isfinite(e.accumulator) || double(e.accumulator) > double(std::numeric_limits<int>::max()))
                return status_ = {EffectsError::arithmetic, e.script};
            azimuth = isle_rng_next_float(rng_) * tau;
            step = tau / int(e.accumulator);
        }
        while (e.accumulator >= 1.0f) {
            const float pitch = std::atan2(velocity.y, velocity.z), sp = std::sin(pitch), cp = std::cos(pitch);
            const float yaw = std::atan2(velocity.x, velocity.y*sp + velocity.z*cp), sy = std::sin(yaw), cy = std::cos(yaw);
            const float mag = magnitude(velocity);
            float fraction = 1.0f, radius;
            if (s.parameters[5] < 0.0f) radius = -s.parameters[5];
            else { fraction = isle_rng_next_float(rng_); if (s.kind != 0) fraction = std::sqrt(fraction); radius = s.parameters[5] * fraction; }
            float angle = fraction;
            if (s.parameters[6] < 0.0f) { azimuth += step; angle = -s.parameters[6]; }
            else { azimuth = isle_rng_next_float(rng_) * tau; angle *= s.parameters[6]; }
            const float px = std::cos(azimuth) * radius, py = std::sin(azimuth) * radius;
            const float radial_velocity = std::sin(angle) * mag;
            const float x = std::cos(azimuth) * radial_velocity, y = std::sin(azimuth) * radial_velocity;
            const float axial_velocity = std::cos(angle) * mag;
            const Vec3 position = {px*cy + 0.0f + e.position.x, -px*sp*sy + py*cp + 0.0f + e.position.y,
                                   -px*cp*sy - py*sp + 0.0f + e.position.z};
            Vec3 emitted = {x*cy + axial_velocity*sy, -x*sp*sy + y*cp + axial_velocity*sp*cy,
                            -x*cp*sy - y*sp + axial_velocity*cp*cy};
            if (s.kind == 3) { emitted.x *= fraction; emitted.y *= fraction; emitted.z *= fraction; }
            spawn_particle(i, position, emitted);
            if (!status_) return status_;
            e.accumulator -= 1.0f;
        }
        if (e.lifetime && --e.lifetime == 0) remove_effect(i, previous); else previous = i;
        i = next;
    }
    return {};
}
void MenuEffects::detach_instance(animation::InstanceHandle instance) noexcept {
    int previous = absent;
    for (int i = effect_head_; i != absent;) {
        const int next = effects_[i].next;
        if (effects_[i].attachment.instance == instance) remove_effect(i, previous); else previous = i;
        i = next;
    }
}
std::size_t MenuEffects::particle_count() const noexcept {
    std::size_t n = 0; for (int i = particle_head_; i != absent; i = particles_[i].next) ++n; return n;
}
std::size_t MenuEffects::effect_count() const noexcept {
    std::size_t n = 0; for (int i = effect_head_; i != absent; i = effects_[i].next) ++n; return n;
}
bool MenuEffects::particle(std::size_t order, EffectParticleView& output) const noexcept {
    for (int i = particle_head_; i != absent; i = particles_[i].next) {
        if (!order--) { output = particles_[i].value; return true; }
    }
    return false;
}

EffectRenderResult MenuEffects::render(const EffectCamera& camera, EffectRenderStorage storage) const noexcept {
    if (!status_) return {status_};
    if (storage.images.size() != bank_->images.size()) return {{EffectsError::resource}};
    for (const auto& row : camera.projection) for (float f : row)
        if (!std::isfinite(f)) return {{EffectsError::arithmetic}};
    for (unsigned k = 0; k < 3; ++k)
        if (!std::isfinite(camera.viewport_scale[k]) || !std::isfinite(camera.viewport_translate[k]))
            return {{EffectsError::arithmetic}};
    auto owns = [&](std::uint32_t address, std::size_t bytes) {
        if (address % 8 || address >= rdram_size || bytes > rdram_size - address) return false;
        for (const auto& r : storage.asset_reservations)
            if (address >= r.offset && address - r.offset <= r.size && bytes <= r.size - (address - r.offset)) return true;
        return false;
    };
    // CI4 semantic legalization: source pal256 overreads a 32-byte palette.
    // Only palette0 indices0..15 are reachable; no CI8/other-bank consumer.
    for (std::size_t t = 0; t < bank_->images.size(); ++t) {
        const auto& image = bank_->images[t]; const auto& binding = storage.images[t];
        for (std::size_t f = 0; f < image.frames.size(); ++f)
            if (!owns(binding.frames[f], image.frames[f].bytes.size())) return {{EffectsError::extent}};
        for (std::size_t f = 0; f < image.palettes.size(); ++f)
            if (image.palettes[f].bytes.size() != 32 || !owns(binding.palettes[f], 32)) return {{EffectsError::extent}};
    }
    struct Draw {
        const EffectParticleView* particle;
        int xl, yl, xh, yh, dsdx, dtdy;
        std::uint16_t s, t, z;
    };
    std::array<Draw, 100> draws{};
    std::size_t count = 0;
    const auto& m = camera.projection;
    const float sx = std::sqrt(m[0][0]*m[0][0] + m[1][0]*m[1][0] + m[2][0]*m[2][0]);
    const float sy = std::sqrt(m[0][1]*m[0][1] + m[1][1]*m[1][1] + m[2][1]*m[2][1]);
    auto integer = [](float f, int& value) {
        if (!std::isfinite(f) || double(f) < double(std::numeric_limits<int>::min()) ||
            double(f) > double(std::numeric_limits<int>::max())) return false;
        value = int(f); return true;
    };
    for (int i = particle_head_; i != absent; i = particles_[i].next) {
        const auto& p = particles_[i].value;
        if (p.size == 0.0f) continue;
        if (p.texture >= bank_->images.size() || p.frame >= bank_->images[p.texture].frames.size())
            return {{EffectsError::extent, particles_[i].script}};
        if (p.flags & ~std::uint16_t(0x93)) return {{EffectsError::unsupported, particles_[i].script}};
        const auto& v = p.position;
        float x = m[0][0]*v.x + m[1][0]*v.y + m[2][0]*v.z + m[3][0];
        float y = m[0][1]*v.x + m[1][1]*v.y + m[2][1]*v.z + m[3][1];
        float z = m[0][2]*v.x + m[1][2]*v.y + m[2][2]*v.z + m[3][2];
        float w = m[0][3]*v.x + m[1][3]*v.y + m[2][3]*v.z + m[3][3];
        if (w == 0.0f) continue;
        w = 1.0f / w; x *= w; y *= w; z *= w;
        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) return {{EffectsError::arithmetic}};
        if (x < -1 || x > 1 || y < -1 || y > 1 || z < -1 || z > 1) continue;
        w *= p.size;
        float xh = (w*sx + x)*camera.viewport_scale[0] + camera.viewport_translate[0];
        float yh = (w*sy + y)*-camera.viewport_scale[1] + camera.viewport_translate[1];
        x = x*camera.viewport_scale[0] + camera.viewport_translate[0];
        y = y*-camera.viewport_scale[1] + camera.viewport_translate[1];
        float xl, yl;
        if (x < xh) xl = x - (xh - x); else { xl = xh; xh = x - (xh - x); }
        if (y < yh) yl = y - (yh - y); else { yl = yh; yh = y - (yh - y); }
        Draw draw{&p, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        int depth = 0;
        if (!integer(xl, draw.xl) || !integer(yl, draw.yl) || !integer(xh, draw.xh) ||
            !integer(yh, draw.yh) || !integer(32*4096.0f/(xh-xl), draw.dsdx) ||
            !integer(32*4096.0f/(yh-yl), draw.dtdy) ||
            !integer((z*camera.viewport_scale[2] + camera.viewport_translate[2])*32.0f, depth))
            return {{EffectsError::arithmetic}};
        if (draw.xh <= 0 || draw.yh <= 0) continue;
        if (draw.xh > 4095 || draw.yh > 4095) return {{EffectsError::extent}};
        // Public scissored texrect: quarter-pixel top/left clipping advances
        // signed S/T (5 fractional bits) without modifying source derivatives.
        if (draw.xl < 0) {
            draw.s = static_cast<std::uint16_t>((-std::int64_t(draw.xl)*draw.dsdx) >> 7);
            draw.xl = 0;
        }
        if (draw.yl < 0) {
            draw.t = static_cast<std::uint16_t>((-std::int64_t(draw.yl)*draw.dtdy) >> 7);
            draw.yl = 0;
        }
        draw.z = static_cast<std::uint16_t>(depth); draws[count++] = draw;
    }
    // Count exact cached projections before touching destination. Emission has
    // no repeated projection/script/asset computation and never mutates RNG.
    auto emit = [&](bool publish) {
        std::size_t n = 0;
        auto cmd = [&](std::uint32_t a, std::uint32_t b) {
            if (publish) { storage.words[n] = a; storage.words[n + 1] = b; }
            n += 2;
        };
        auto mode = [&](bool high, unsigned shift, unsigned length, std::uint32_t value) {
            cmd((high ? 0xe3000000u : 0xe2000000u) | ((32-shift-length) << 8) | (length-1), value);
        };
        cmd(0xe7000000, 0);
        mode(true, 20, 2, 0);
        // AA_ZB_XLU_SURF/NOOP2; or XLU_SURF/NOOP2, primitive Z.
        mode(false, 3, 29, camera.depth_test ? 0x004049d8u : 0x00404240u);
        mode(true, 19, 1, 0); mode(false, 2, 1, 4);
        mode(true, 6, 2, bank_->color_dither); mode(true, 4, 2, bank_->alpha_dither);
        std::uint32_t previous_image = 0, previous_palette = 0;
        bool previous_blend = false, previous_alpha = false;
        int lut = -1;
        for (std::size_t i = 0; i < count; ++i) {
            const auto& d = draws[i]; const auto& p = *d.particle;
            const auto& image = bank_->images[p.texture]; const auto& binding = storage.images[p.texture];
            const auto address = binding.frames[p.frame];
            if (image.format == 2) {
                const auto palette = binding.palettes[(p.flags & 0x10) ? 0 : p.frame];
                if (previous_palette != palette || i == 0) {
                    cmd(0xfd100000, palette); cmd(0xe8000000, 0);
                    cmd(0xf5000100, 0x07000000); cmd(0xe6000000, 0);
                    cmd(0xf0000000, 0x0703c000); // 16 RGBA16 entries, palette0.
                    cmd(0xe7000000, 0); previous_palette = palette;
                }
                if (lut != 1) { mode(true, 14, 2, 0x8000); lut = 1; }
            } else if (lut != 0) { mode(true, 14, 2, 0); lut = 0; }
            if (previous_image != address || i == 0) {
                cmd(0xfd100000 | (image.format << 21), address);
                cmd(0xf5100000 | (image.format << 21), 0x07080200);
                cmd(0xe6000000, 0); cmd(0xf3000000, 0x070ff400);
                cmd(0xe7000000, 0);
                cmd(0xf5000400 | (image.format << 21), 0x00080200);
                cmd(0xf2000000, 0x0007c07c);
                previous_image = address;
            }
            cmd(0xfa000000, rgba(p.primitive));
            if (p.flags & 0x80) {
                cmd(0xfb000000, rgba(p.environment));
                // (PRIMITIVE-ENVIRONMENT)*TEXEL0+ENVIRONMENT, RGB+A.
                cmd(0xfc000000 | (3u<<20) | (1u<<15) | (3u<<12) | (1u<<9) | (3u<<5) | 1u,
                    (5u<<28) | (5u<<24) | (3u<<21) | (1u<<18) | (5u<<15) |
                    (5u<<12) | (5u<<9) | (5u<<6) | (5u<<3) | 5u);
            } else {
                cmd(0xfc000000 | (1u<<20) | (3u<<15) | (1u<<12) | (3u<<9) | (1u<<5) | 3u,
                    (8u<<28) | (8u<<24) | (1u<<21) | (3u<<18) | (7u<<15) |
                    (7u<<12) | (7u<<9) | (7u<<6) | (7u<<3) | 7u);
            }
            if (!previous_blend) { cmd(0xf9000000, 8); previous_blend = true; }
            if (!previous_alpha) { mode(false, 0, 2, 1); previous_alpha = true; }
            cmd(0xee000000, std::uint32_t(d.z) << 16);
            cmd(0xe4000000 | (std::uint32_t(d.xh)<<12) | std::uint32_t(d.yh),
                (std::uint32_t(d.xl)<<12) | std::uint32_t(d.yl));
            cmd(0xe1000000, (std::uint32_t(d.s)<<16) | d.t);
            cmd(0xf1000000, ((std::uint32_t(d.dsdx)&65535)<<16) | (std::uint32_t(d.dtdy)&65535));
        }
        if (lut != 0) mode(true, 14, 2, 0);
        return n;
    };
    const auto required = emit(false);
    if (required > storage.words.size()) return {{EffectsError::extent}, required, count};
    emit(true);
    return {{}, required, count};
}

} // namespace isle::menu

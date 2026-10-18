#include "menu_effects.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <exception>
#include <utility>

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
const EffectInstruction* instruction(const EffectScript& script, std::size_t cursor) noexcept {
    const auto it = std::lower_bound(script.instructions.begin(), script.instructions.end(), cursor,
        [](const EffectInstruction& ins, std::size_t p) { return ins.offset < p; });
    return it != script.instructions.end() && it->offset == cursor ? &*it : nullptr;
}
struct Control {
    std::size_t cursor = 0, loop = 0, returned = 0;
    std::uint8_t count = 0;
    bool operator==(const Control&) const = default;
};
// Deterministic source control registers permit finite loops without waits.
// Floyd detection needs no allocation or arbitrary instruction/tick cutoff.
bool advance_control(const EffectScript& script, Control& state,
                     bool& terminal, bool& yields) noexcept {
    const auto* ins = instruction(script, state.cursor);
    if (!ins) return false;
    const auto raw = ins->opcode;
    std::size_t cursor = ins->offset + 1;
    terminal = yields = false;
    state.cursor = ins->offset + ins->size;
    if (raw < 128) {
        unsigned delay = raw & 31;
        if (raw & 32) delay = (delay << 8) + byte(script.bytecode, cursor);
        yields = delay != 0;
        if (yields) state.cursor = static_cast<std::uint16_t>(state.cursor);
    } else if (raw == 0xfa) {
        state.count = byte(script.bytecode, cursor);
        state.loop = static_cast<std::uint16_t>(state.cursor);
    } else if (raw == 0xfb) {
        if (--state.count) state.cursor = state.loop;
    } else if (raw == 0xfc) state.returned = static_cast<std::uint16_t>(state.cursor);
    else if (raw == 0xfd) state.cursor = state.returned;
    else if (raw == 0xfe || raw == 0xff) terminal = true;
    return true;
}
bool yielding_control(const EffectScript& script) noexcept {
    Control slow{}, fast{};
    bool terminal = false, yields = false;
    do {
        if (!advance_control(script, slow, terminal, yields)) return false;
        if (terminal) return true;
        for (unsigned n = 0; n < 2; ++n) {
            if (!advance_control(script, fast, terminal, yields)) return false;
            if (terminal) return true;
        }
    } while (!(slow == fast));
    const auto cycle = slow;
    bool has_wait = false;
    do {
        if (!advance_control(script, slow, terminal, yields)) return false;
        has_wait |= yields;
    } while (!(slow == cycle));
    return has_wait;
}
bool instruction_shape(const EffectScript& script, const EffectInstruction& ins,
                       const EffectBank& bank, EffectTargetServices targets,
                       EffectsError& error) noexcept {
    error = EffectsError::resource;
    const auto& code = script.bytecode;
    if (!ins.size || ins.offset >= code.bytes.size() ||
        ins.size > code.bytes.size() - ins.offset || byte(code, ins.offset) != ins.opcode) return false;
    const auto raw = ins.opcode, op = operation(raw);
    std::size_t p = ins.offset + 1, end = ins.offset + ins.size;
    auto take = [&](std::size_t n) {
        if (n > end - p) return false;
        p += n; return true;
    };
    auto timed = [&] {
        if (p == end) return false;
        const auto n = byte(code, p++);
        return !(n & 128) || take(1);
    };
    auto real = [&] {
        if (end - p < 4) return false;
        const auto f = scalar(code, p);
        return std::isfinite(f);
    };
    auto word = [&](std::uint16_t& n) {
        if (end - p < 2) return false;
        n = static_cast<std::uint16_t>((byte(code, p) << 8) | byte(code, p + 1));
        p += 2; return true;
    };
    const auto frames = bank.images[static_cast<std::uint8_t>(script.texture)].frames.size();
    bool ok = true;
    if (raw < 128) {
        if (raw & 32) ok = take(1);
        if (ok && (raw & 0xc0) == 0x40) {
            ok = p < end && byte(code, p) < frames;
            if (ok) ++p;
        }
    } else if (op >= 0x80 && op <= 0x98) {
        for (unsigned k = 0; k < 3 && ok; ++k) if (raw & (1u << k)) ok = real();
    } else if (op == 0xc0 || op == 0xd0) {
        ok = timed() && take(std::popcount(unsigned(raw & 15)));
    } else switch (op) {
    case 0xa0: ok = timed() && real(); break;
    case 0xa1:
        ok = p < end;
        if (ok && (byte(code, p) & 4)) { error = EffectsError::unsupported; return false; }
        if (ok) ++p;
        break;
    case 0xa2: case 0xa3: case 0xa9: case 0xab: ok = real(); break;
    case 0xa4: case 0xa5: case 0xb9: {
        std::uint16_t id = 0; ok = word(id) && id < bank.scripts.size(); break;
    }
    case 0xa6: ok = take(4); break;
    case 0xa7: ok = take(1); break;
    case 0xa8: case 0xbe: ok = real() && real() && real(); break;
    case 0xaa: {
        std::uint16_t first = 0, range = 0;
        ok = word(first) && word(range) && first < bank.scripts.size() &&
            range <= bank.scripts.size() - first;
        break;
    }
    case 0xac: ok = timed() && real() && real(); break;
    case 0xad: case 0xae: case 0xaf: case 0xb0: case 0xb1:
    case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: break;
    case 0xb7: case 0xb8:
        if (!targets.read) { error = EffectsError::attachment; return false; }
        ok = p < end && byte(code, p++) == 0;
        if (ok && op == 0xb8) ok = real();
        break;
    case 0xba: case 0xbb: ok = take(4); break;
    case 0xbc:
        ok = end - p >= 2 && byte(code, p) < frames &&
            byte(code, p + 1) <= frames - byte(code, p);
        if (ok) p += 2;
        break;
    case 0xbd: ok = real() && real(); break;
    case 0xbf:
        if (!targets.write) { error = EffectsError::attachment; return false; }
        ok = p < end && byte(code, p) >= 1 && byte(code, p) <= 8;
        if (ok) ++p;
        break;
    case 0xfa: ok = take(1); break;
    case 0xfb: case 0xfc: case 0xfd: case 0xfe: case 0xff: break;
    default: error = EffectsError::unsupported; return false;
    }
    return ok && p == end;
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

RetainedEffectAttachment::RetainedEffectAttachment(void* context, IsleGameModelRef binding,
    std::size_t node, Resolve resolve, Release release) noexcept
    : context_(context), binding_(binding), node_(node), resolve_(resolve), release_(release) {
    if (!context_ || !binding_.object || !binding_.model || !binding_.incarnation ||
        node_ == no_index || !resolve_ || !release_) std::terminate();
}
RetainedEffectAttachment::~RetainedEffectAttachment() { reset(); }
RetainedEffectAttachment::RetainedEffectAttachment(RetainedEffectAttachment&& other) noexcept
    : context_(std::exchange(other.context_, nullptr)), binding_(other.binding_), node_(other.node_),
      resolve_(other.resolve_), release_(other.release_) {}
RetainedEffectAttachment& RetainedEffectAttachment::operator=(RetainedEffectAttachment&& other) noexcept {
    if (this != &other) {
        reset();
        context_ = std::exchange(other.context_, nullptr);
        binding_ = other.binding_; node_ = other.node_;
        resolve_ = other.resolve_; release_ = other.release_;
    }
    return *this;
}
void RetainedEffectAttachment::reset() noexcept {
    if (auto* context = std::exchange(context_, nullptr)) release_(context, binding_, node_);
}
bool RetainedEffectAttachment::resolve(EffectWorld& output) const noexcept {
    return context_ && resolve_(context_, binding_, node_, output);
}

EffectRuntime::EffectRuntime(std::span<const EffectBank* const> active, isle_rng& rng,
                             EffectTargetServices targets) noexcept
    : rng_(&rng), targets_(targets) {
    reset();
    for (const auto* selected : active) {
        if (!selected) { status_ = {EffectsError::resource}; return; }
        status_ = install_bank(*selected);
        if (!status_) return;
    }
}
EffectsStatus EffectRuntime::validate_bank(const EffectBank& selected) const noexcept {
    if (selected.bank_id >= banks_.size() || selected.scripts.empty() || selected.images.empty())
        return {EffectsError::resource};
    for (const auto& image : selected.images) {
        const bool format = (image.format == 4 && image.size_code <= 1) ||
            (image.format == 3 && image.size_code == 1) ||
            (image.format == 2 && image.size_code == 0) ||
            (image.format == 0 && (image.size_code == 2 || image.size_code == 3));
        if (!format || !image.width || !image.height || image.width > 64 || image.height > 64 ||
            image.frames.empty() || image.frames.size() > 256)
            return {EffectsError::unsupported};
        const std::size_t bytes = (std::size_t(image.width) * image.height * (4u << image.size_code) + 7) / 8;
        if (bytes > 4096) return {EffectsError::unsupported};
        for (const auto& frame : image.frames) if (frame.bytes.size() != bytes)
            return {EffectsError::extent};
        const auto palette_count = image.format == 2 ? ((image.flags & 1) ? 1 : image.frames.size()) : 0;
        if (image.palettes.size() != palette_count) return {EffectsError::resource};
        for (const auto& palette : image.palettes) if (palette.bytes.size() != 32)
            return {EffectsError::extent};
    }
    for (std::size_t id = 0; id < selected.scripts.size(); ++id) {
        const auto& script = selected.scripts[id];
        // Kind2/flag4 uses parent-coupled table-sine physics, not ordinary
        // particles. None of the admitted source descriptors has that kind.
        if ((script.kind != 0 && script.kind != 3) || (script.flags & 4))
            return {EffectsError::unsupported, id};
        if ((script.flags & 0x8000) && !targets_.write) return {EffectsError::attachment, id};
        if (script.texture >= selected.images.size() || script.instructions.empty())
            return {EffectsError::resource, id};
        for (float f : script.parameters) if (!std::isfinite(f))
            return {EffectsError::resource, id};
        std::size_t end = 0;
        for (const auto& ins : script.instructions) {
            EffectsError error{};
            if (ins.offset != end || !instruction_shape(script, ins, selected, targets_, error))
                return {error == EffectsError::none ? EffectsError::resource : error, id, ins.offset};
            end = ins.offset + ins.size;
        }
        if (!yielding_control(script)) return {EffectsError::resource, id};
    }
    return {};
}
EffectsStatus EffectRuntime::install_bank(const EffectBank& selected) noexcept {
    if (!status_) return status_;
    const auto admitted = validate_bank(selected);
    if (!admitted) return admitted;
    if (banks_[selected.bank_id]) return {EffectsError::resource};
    banks_[selected.bank_id] = &selected;
    return {};
}
EffectsStatus EffectRuntime::reset_bank(std::size_t slot) noexcept {
    if (slot >= banks_.size()) return {EffectsError::extent};
    for (const auto& effect : effects_)
        if (effect.live && (effect.bank_id & 7) == slot) return {EffectsError::resource};
    for (const auto& particle : particles_)
        if (particle.live && (particle.value.bank_id & 7) == slot) return {EffectsError::resource};
    banks_[slot] = nullptr;
    return {};
}
const EffectBank* EffectRuntime::bank(std::uint16_t encoded) const noexcept {
    return encoded < 128 ? banks_[encoded & 7] : nullptr;
}
void EffectRuntime::reset() noexcept {
    particle_heads_.fill(absent);
    effect_head_ = absent;
    particle_free_ = effect_free_ = 0;
    paused_lists_ = 0;
    aim_targets_.fill({});
    output_targets_.fill({});
    for (std::size_t i = 0; i < particles_.size(); ++i) {
        particles_[i].live = false;
        particles_[i].next = i + 1 < particles_.size() ? int(i + 1) : absent;
    }
    for (std::size_t i = 0; i < effects_.size(); ++i) {
        effects_[i].retained_attachment = {};
        effects_[i].attachment = {};
        effects_[i].live = false;
        effects_[i].next = i + 1 < effects_.size() ? int(i + 1) : absent;
    }
}
void EffectRuntime::set_dither(std::uint8_t color, std::uint8_t alpha) noexcept {
    color_dither_ = color & 0xc0;
    alpha_dither_ = alpha & 0x30;
}
int EffectRuntime::spawn_effect(std::uint16_t encoded, std::size_t script,
                                EffectAttachment attachment) noexcept {
    if (effect_free_ == absent) return absent;
    const int index = effect_free_;
    auto& e = effects_[index];
    if (e.generation == std::numeric_limits<std::uint32_t>::max()) {
        status_ = {EffectsError::arithmetic, script}; return absent;
    }
    const auto generation = e.generation + 1;
    effect_free_ = e.next;
    e = {}; e.generation = generation; e.live = true;
    e.next = effect_head_; effect_head_ = index;
    const auto& selected = *bank(encoded);
    const auto& s = selected.scripts[script];
    e.id = ++current_id_; e.bank_id = encoded; e.script = script; e.attachment = attachment;
    e.lifetime = s.effect_lifetime; e.flags = static_cast<std::uint16_t>(s.flags);
    if (selected.images[s.texture].flags != 0) e.flags |= 0x10;
    e.velocity = {s.parameters[2], s.parameters[3], s.parameters[4]};
    return index;
}
bool EffectRuntime::has_effect(std::uint16_t encoded, std::size_t script) const noexcept {
    if (!status_ || encoded >= 128) return false;
    const auto* selected = bank(encoded);
    return selected && script < selected->scripts.size();
}
EffectSpawnResult EffectRuntime::create_effect(std::uint16_t encoded, std::size_t script,
                                               EffectAttachment attachment) noexcept {
    if (!status_) return {status_};
    if (encoded >= 128) return {{EffectsError::extent, script}};
    const auto* selected = bank(encoded);
    if (!selected || script >= selected->scripts.size()) return {};
    const int index = spawn_effect(encoded, script, attachment);
    if (index == absent) return {status_};
    const auto& effect = effects_[index];
    return {{}, true, effect.id, {effect.generation, static_cast<std::uint8_t>(index)}};
}
EffectsStatus EffectRuntime::configure_effect(EffectHandle handle, Vec3 position, Vec3 velocity) noexcept {
    if (!status_) return status_;
    if (!handle || handle.slot >= effects_.size() || !effects_[handle.slot].live ||
        effects_[handle.slot].generation != handle.generation) return {EffectsError::attachment};
    if (!finite(position) || !finite(velocity)) return {EffectsError::arithmetic};
    effects_[handle.slot].position = position;
    effects_[handle.slot].velocity = velocity;
    return {};
}
EffectsStatus EffectRuntime::set_effect_position(EffectHandle handle, Vec3 position) noexcept {
    if (!status_) return status_;
    if (!handle || handle.slot >= effects_.size() || !effects_[handle.slot].live ||
        effects_[handle.slot].generation != handle.generation) return {EffectsError::attachment};
    if (!finite(position)) return {EffectsError::arithmetic};
    effects_[handle.slot].position = position;
    return {};
}
EffectsStatus EffectRuntime::set_effect_attachment(EffectHandle handle, RetainedEffectAttachment attachment) noexcept {
    if (!status_) return status_;
    if (!handle || handle.slot >= effects_.size() || !effects_[handle.slot].live ||
        effects_[handle.slot].generation != handle.generation) return {EffectsError::attachment};
    if (attachment) {
        EffectWorld current{};
        if (!attachment.resolve(current)) return {EffectsError::attachment};
        for (const auto& row : current.matrix) for (float value : row)
            if (!std::isfinite(value)) return {EffectsError::arithmetic};
    }
    auto& effect = effects_[handle.slot];
    effect.retained_attachment = std::move(attachment);
    effect.attachment = {};
    return {};
}
int EffectRuntime::spawn_particle(std::uint16_t encoded, std::size_t script, Vec3 position,
                                  Vec3 velocity, int parent, int effect) noexcept {
    if (particle_free_ == absent) return absent;
    const int index = particle_free_;
    auto& p = particles_[index];
    particle_free_ = p.next;
    // GAME does not initialize color-target registers at allocation. Preserve
    // their previous slot values; interpolation lengths are always reset.
    const auto primitive = p.primitive_target, environment = p.environment_target;
    p = {}; p.primitive_target = primitive; p.environment_target = environment;
    p.live = true;
    if (parent == absent) {
        p.next = particle_heads_[encoded >> 3]; particle_heads_[encoded >> 3] = index;
    } else { p.next = particles_[parent].next; particles_[parent].next = index; }
    const auto& selected = *bank(encoded);
    const auto& s = selected.scripts[script];
    p.script = script; p.value.bank_id = encoded;
    p.value.position = position; p.value.velocity = velocity;
    p.value.effect_id = effect == absent ? ++current_id_ : effects_[effect].id;
    p.value.flags = effect == absent ? static_cast<std::uint16_t>(s.flags) : effects_[effect].flags;
    if (effect == absent && selected.images[s.texture].flags != 0) p.value.flags |= 0x10;
    p.value.texture = static_cast<std::uint8_t>(s.texture); p.value.size = s.parameters[8];
    p.gravity = s.parameters[0]; p.friction = s.parameters[1];
    p.value.lifetime = static_cast<std::uint16_t>(s.particle_lifetime + 1);
    p.value.primitive.fill(255); p.value.environment.fill(0);
    return index;
}
EffectSpawnResult EffectRuntime::create_particle(std::uint16_t encoded, std::size_t script) noexcept {
    if (!status_) return {status_};
    if (encoded >= 128) return {{EffectsError::extent, script}};
    const auto* selected = bank(encoded);
    if (!selected || script >= selected->scripts.size()) return {};
    const auto& s = selected->scripts[script];
    return create_particle(encoded, script, {}, {s.parameters[2], s.parameters[3], s.parameters[4]});
}
EffectSpawnResult EffectRuntime::create_particle(std::uint16_t encoded, std::size_t script,
                                                 Vec3 position, Vec3 velocity) noexcept {
    if (!status_) return {status_};
    if (encoded >= 128) return {{EffectsError::extent, script}};
    const auto* selected = bank(encoded);
    if (!selected || script >= selected->scripts.size()) return {};
    if (!finite(position) || !finite(velocity)) return {{EffectsError::arithmetic, script}};
    const int index = spawn_particle(encoded, script, position, velocity, absent, absent);
    if (index == absent) return {};
    const auto id = particles_[index].value.effect_id;
    tick_particle(index, absent);
    return {status_, true, id};
}
void EffectRuntime::remove_particle(int index, int previous) noexcept {
    auto& p = particles_[index];
    if (previous == absent) particle_heads_[p.value.bank_id >> 3] = p.next;
    else particles_[previous].next = p.next;
    p.live = false; p.next = particle_free_; particle_free_ = index;
}
int EffectRuntime::tick_particle(int index, int previous) noexcept {
    auto& p = particles_[index]; auto& v = p.value;
    if (v.flags & 0x800) return p.next;
    const auto& selected = *bank(v.bank_id);
    const auto& script = selected.scripts[p.script];
    const auto& code = script.bytecode;
    if (p.timer && --p.timer == 0) {
        std::size_t cursor = p.cursor;
        bool stop = false;
        while (!stop) {
            // Immutable admission certified every reachable source control state.
            const auto raw = byte(code, cursor++), op = operation(raw);
            auto word = [&] {
                const auto n = static_cast<std::uint16_t>((byte(code, cursor) << 8) | byte(code, cursor + 1));
                cursor += 2; return n;
            };
            if (raw < 128) {
                unsigned wait = raw & 31;
                if (raw & 32) wait = byte(code, cursor++) + (wait << 8);
                if ((raw & 0xc0) == 0x40) v.frame = byte(code, cursor++);
                if (wait) { p.timer = static_cast<std::uint16_t>(wait); stop = true; }
            } else if (op >= 0x80 && op <= 0x98) {
                auto& target = op < 0x90 ? v.position : v.velocity;
                float* components[3]{&target.x, &target.y, &target.z};
                for (unsigned k = 0; k < 3; ++k) if (raw & (1u << k)) {
                    const float value = scalar(code, cursor);
                    if (op == 0x88 || op == 0x98) *components[k] += value; else *components[k] = value;
                }
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
                if (p.size_time == 1) { v.size = p.size_target; p.size_time = 0; }
                break;
            case 0xa1: v.flags = byte(code, cursor++); break;
            case 0xa2:
                p.gravity = scalar(code, cursor);
                v.flags = p.gravity == 0 ? v.flags & ~1u : v.flags | 1u;
                break;
            case 0xa3:
                p.friction = scalar(code, cursor);
                v.flags = p.friction == 1 ? v.flags & ~2u : v.flags | 2u;
                break;
            case 0xa4: case 0xaa: case 0xb9: {
                std::size_t child_script = word();
                if (op == 0xaa) { const auto range = word(); child_script += int(range * isle_rng_next_float(rng_)); }
                const auto& child = selected.scripts[child_script];
                const Vec3 velocity = op == 0xb9 ? v.velocity :
                    Vec3{child.parameters[2], child.parameters[3], child.parameters[4]};
                const int made = spawn_particle(v.bank_id, child_script, v.position, velocity, index, absent);
                if (made != absent) {
                    particles_[made].value.effect_id = v.effect_id;
                    tick_particle(made, index);
                    if (!status_) return p.next;
                }
                break;
            }
            case 0xa5: {
                const auto child_script = word();
                const int made = spawn_effect(v.bank_id, child_script, {});
                if (!status_) return p.next;
                if (made != absent) { effects_[made].position = v.position; effects_[made].id = v.effect_id; }
                break;
            }
            case 0xa6: {
                const auto first = word(), range = word();
                v.lifetime = static_cast<std::uint16_t>(first + int(isle_rng_next_float(rng_) * range));
                break;
            }
            case 0xa7:
                if (byte(code, cursor++) >= int(isle_rng_next_float(rng_) * 100.0f)) {
                    v.lifetime = 1; p.timer = 0; stop = true;
                }
                break;
            case 0xa8: {
                const float x = scalar(code, cursor); v.position.x += x * isle_rng_next_float(rng_);
                const float y = scalar(code, cursor); v.position.y += y * isle_rng_next_float(rng_);
                const float z = scalar(code, cursor); v.position.z += z * isle_rng_next_float(rng_);
                break;
            }
            case 0xa9: {
                const float angle = scalar(code, cursor); const Vec3 old = v.velocity;
                const float pitch = std::atan2(old.y, old.z), sp = std::sin(pitch), cp = std::cos(pitch);
                const float yaw = std::atan2(old.x, old.y * sp + old.z * cp), sy = std::sin(yaw), cy = std::cos(yaw);
                const float mag = magnitude(old), t = isle_rng_next_float(rng_) * tau;
                const float sa = std::sin(angle) * mag, x = std::cos(t) * sa, y = std::sin(t) * sa;
                const float ca = std::cos(angle) * mag;
                v.velocity = {x*cy + ca*sy, -x*sp*sy + y*cp + ca*sp*cy, -x*cp*sy - y*sp + ca*cp*cy};
                break;
            }
            case 0xab: {
                const float f = scalar(code, cursor);
                v.velocity.x *= f; v.velocity.y *= f; v.velocity.z *= f; break;
            }
            case 0xad: v.flags |= 0x80; break;
            case 0xae: v.flags &= ~0x60u; break;
            case 0xaf: v.flags = (v.flags & ~0x40u) | 0x20u; break;
            case 0xb0: v.flags = (v.flags & ~0x20u) | 0x40u; break;
            case 0xb1: v.flags |= 0x60; break;
            case 0xb2: v.flags |= 0x200; break;
            case 0xb3: v.flags &= ~0x400u; break;
            case 0xb4: v.flags |= 0x400; break;
            case 0xb5: v.flags |= 0x100; break;
            case 0xb6: v.flags &= ~0x100u; break;
            case 0xb7: case 0xb8: {
                const auto slot = byte(code, cursor++);
                const float attraction = op == 0xb8 ? scalar(code, cursor) : 0.0f;
                if (!aim_targets_[slot].instance) break;
                Vec3 target{};
                if (!targets_.read(targets_.context, aim_targets_[slot], target)) {
                    status_ = {EffectsError::attachment, p.script, cursor}; return p.next;
                }
                if (!finite(target)) { status_ = {EffectsError::arithmetic, p.script, cursor}; return p.next; }
                const Vec3 delta{target.x - v.position.x, target.y - v.position.y, target.z - v.position.z};
                const float square = delta.x*delta.x + delta.y*delta.y + delta.z*delta.z;
                if (square != 0) {
                    const float f = op == 0xb7 ? magnitude(v.velocity) / std::sqrt(square) : attraction / square;
                    if (op == 0xb7) v.velocity = {delta.x*f, delta.y*f, delta.z*f};
                    else { v.velocity.x += f*delta.x; v.velocity.y += f*delta.y; v.velocity.z += f*delta.z; }
                }
                break;
            }
            case 0xba: case 0xbb: {
                auto& target = op == 0xba ? p.primitive_target : p.environment_target;
                for (auto& component : target) {
                    const float range = byte(code, cursor++);
                    component = static_cast<std::uint8_t>(int(float(component) + range * isle_rng_next_float(rng_)));
                }
                if (op == 0xba && !p.primitive_time) v.primitive = target;
                if (op == 0xbb && !p.environment_time) v.environment = target;
                break;
            }
            case 0xbc: {
                const auto first = byte(code, cursor++), range = byte(code, cursor++);
                v.frame = static_cast<std::uint8_t>(int(float(first) + range * isle_rng_next_float(rng_)));
                break;
            }
            case 0xbd: {
                float speed = scalar(code, cursor);
                const float range = scalar(code, cursor);
                speed += range * isle_rng_next_float(rng_);
                const float f = speed / magnitude(v.velocity);
                v.velocity.x *= f; v.velocity.y *= f; v.velocity.z *= f;
                break;
            }
            case 0xbe:
                v.velocity.x *= scalar(code, cursor);
                v.velocity.y *= scalar(code, cursor);
                v.velocity.z *= scalar(code, cursor);
                break;
            case 0xbf: v.flags |= static_cast<std::uint16_t>(0x8000 | ((byte(code, cursor++) - 1) << 12)); break;
            case 0xfa: p.loop_count = byte(code, cursor++); p.loop_cursor = static_cast<std::uint16_t>(cursor); break;
            case 0xfb: if (--p.loop_count) cursor = p.loop_cursor; break;
            case 0xfc: p.return_cursor = static_cast<std::uint16_t>(cursor); break;
            case 0xfd: cursor = p.return_cursor; break;
            case 0xfe: case 0xff: v.lifetime = 1; p.timer = 0; stop = true; break;
            default: status_ = {EffectsError::unsupported, p.script, cursor - 1}; return p.next;
            }
        }
        p.cursor = static_cast<std::uint16_t>(cursor);
    }
    if (p.size_time) { v.size += (p.size_target - v.size) / p.size_time; --p.size_time; }
    interpolate(v.primitive, p.primitive_target, p.primitive_time);
    interpolate(v.environment, p.environment_target, p.environment_time);
    if (--v.lifetime == 0) { const auto next = p.next; remove_particle(index, previous); return next; }
    if (v.flags & 1) v.velocity.y -= p.gravity;
    if (v.flags & 2) { v.velocity.x *= p.friction; v.velocity.y *= p.friction; v.velocity.z *= p.friction; }
    v.position.x += v.velocity.x; v.position.y += v.velocity.y; v.position.z += v.velocity.z;
    if (!finite(v.position) || !finite(v.velocity) || !std::isfinite(v.size)) {
        status_ = {EffectsError::arithmetic, p.script, p.cursor}; return p.next;
    }
    if (v.flags & 0x8000) {
        const auto target = output_targets_[(v.flags >> 12) & 7];
        if (target.instance && !targets_.write(targets_.context, target, v.position))
            status_ = {EffectsError::attachment, p.script, p.cursor};
    }
    return p.next;
}
EffectsStatus EffectRuntime::update_particles() noexcept {
    if (!status_) return status_;
    for (unsigned list = 0; list < particle_heads_.size(); ++list) {
        if (paused_lists_ & (1u << list)) continue;
        int previous = absent, index = particle_heads_[list];
        while (index != absent) {
            const int next = tick_particle(index, previous);
            if (!status_) return status_;
            if (particles_[index].live) previous = index;
            index = next;
        }
    }
    return {};
}
void EffectRuntime::remove_effect(int index, int previous) noexcept {
    // A5 may have inserted an emitter at the head during a particle's immediate
    // tick. Find the actual predecessor rather than unlinking that new head.
    if (previous == absent && effect_head_ != index) {
        for (previous = effect_head_; previous != absent && effects_[previous].next != index;
             previous = effects_[previous].next) {}
    }
    auto& e = effects_[index];
    if (previous == absent) effect_head_ = e.next; else effects_[previous].next = e.next;
    e.live = false; e.next = effect_free_; effect_free_ = index;
    e.attachment = {};
    e.retained_attachment = {};
}
EffectsStatus EffectRuntime::update_effects(EffectWorldResolver resolve, void* context) noexcept {
    if (!status_) return status_;
    int previous = absent;
    for (int i = effect_head_; i != absent;) {
        auto& e = effects_[i];
        const auto& s = bank(e.bank_id)->scripts[e.script];
        const int next = e.next;
        if ((e.flags & 0x800) || (paused_lists_ & (1u << (e.bank_id >> 3)))) {
            previous = i; i = next; continue;
        }
        const float rate = s.parameters[7];
        if (rate < 0.0f) e.accumulator -= rate;
        else e.accumulator += isle_rng_next_float(rng_) * rate;
        Vec3 velocity = e.velocity;
        float azimuth = 0, step = 0;
        if (!std::isfinite(e.accumulator) || double(e.accumulator) > double(std::numeric_limits<int>::max()))
            return status_ = {EffectsError::arithmetic, e.script};
        if (e.accumulator >= 1.0f) {
            // Source evaluates a live DObj only on an actual emission tick.
            if (e.retained_attachment || e.attachment.instance) {
                EffectWorld world{};
                if (e.retained_attachment ? !e.retained_attachment.resolve(world) :
                    (!resolve || !resolve(context, e.attachment, world)))
                    return status_ = {EffectsError::attachment, e.script};
                auto& m = world.matrix;
                for (const auto& row : m) for (float f : row) if (!std::isfinite(f))
                    return status_ = {EffectsError::arithmetic, e.script};
                e.position = {m[3][0], m[3][1], m[3][2]};
                for (unsigned col = 0; col < 3; ++col) {
                    if (m[0][col] == 0 && m[1][col] == 0 && m[2][col] == 0) continue;
                    const float n = std::sqrt(m[0][col]*m[0][col] + m[1][col]*m[1][col] + m[2][col]*m[2][col]);
                    if (n == 0 || !std::isfinite(n) || !std::isfinite(1.0f/n))
                        return status_ = {EffectsError::arithmetic, e.script};
                    const float inv = 1.0f/n;
                    m[0][col] *= inv; m[1][col] *= inv; m[2][col] *= inv;
                }
                const Vec3 v = velocity;
                velocity = {m[0][0]*v.x + m[1][0]*v.y + m[2][0]*v.z,
                            m[0][1]*v.x + m[1][1]*v.y + m[2][1]*v.z,
                            m[0][2]*v.x + m[1][2]*v.y + m[2][2]*v.z};
            }
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
            const Vec3 position{px*cy + 0.0f + e.position.x, -px*sp*sy + py*cp + 0.0f + e.position.y,
                                -px*cp*sy - py*sp + 0.0f + e.position.z};
            Vec3 emitted{x*cy + axial_velocity*sy, -x*sp*sy + y*cp + axial_velocity*sp*cy,
                         -x*cp*sy - y*sp + axial_velocity*cp*cy};
            if (s.kind == 3) { emitted.x *= fraction; emitted.y *= fraction; emitted.z *= fraction; }
            const int made = spawn_particle(e.bank_id, e.script, position, emitted, absent, i);
            if (made != absent) tick_particle(made, absent);
            if (!status_) return status_;
            e.accumulator -= 1.0f;
        }
        if (e.lifetime && --e.lifetime == 0) remove_effect(i, previous); else previous = i;
        i = next;
    }
    return {};
}
EffectsStatus EffectRuntime::register_aim_target(std::size_t slot, EffectAttachment target) noexcept {
    if (slot >= aim_targets_.size()) return {EffectsError::extent};
    if (target.instance && !targets_.read) return {EffectsError::attachment};
    aim_targets_[slot] = target; return {};
}
EffectsStatus EffectRuntime::register_output_target(std::size_t slot, EffectAttachment target) noexcept {
    if (slot >= output_targets_.size()) return {EffectsError::extent};
    if (target.instance && !targets_.write) return {EffectsError::attachment};
    output_targets_[slot] = target; return {};
}
void EffectRuntime::clear_targets(animation::InstanceHandle instance) noexcept {
    for (auto& target : aim_targets_) if (target.instance == instance) target = {};
    for (auto& target : output_targets_) if (target.instance == instance) target = {};
}
void EffectRuntime::detach_instance(animation::InstanceHandle instance) noexcept {
    clear_targets(instance);
    int previous = absent;
    for (int i = effect_head_; i != absent;) {
        const int next = effects_[i].next;
        if (effects_[i].attachment.instance == instance) remove_effect(i, previous); else previous = i;
        i = next;
    }
}
void EffectRuntime::eject_effect(EffectHandle handle) noexcept {
    if (!handle || handle.slot >= effects_.size() || !effects_[handle.slot].live ||
        effects_[handle.slot].generation != handle.generation) return;
    remove_effect(handle.slot, absent);
}
void EffectRuntime::eject_id(std::uint16_t id, std::uint8_t list) noexcept {
    if (list >= particle_heads_.size()) return;
    int previous = absent;
    for (int i = particle_heads_[list]; i != absent;) {
        const int next = particles_[i].next;
        if (particles_[i].value.effect_id == id) remove_particle(i, previous); else previous = i;
        i = next;
    }
    previous = absent;
    for (int i = effect_head_; i != absent;) {
        const int next = effects_[i].next;
        if (effects_[i].id == id) remove_effect(i, previous); else previous = i;
        i = next;
    }
}
void EffectRuntime::pause_id(std::uint16_t id, std::uint8_t list, bool paused) noexcept {
    if (list >= particle_heads_.size()) return;
    for (int i = particle_heads_[list]; i != absent; i = particles_[i].next)
        if (particles_[i].value.effect_id == id)
            particles_[i].value.flags = paused ? particles_[i].value.flags | 0x800u : particles_[i].value.flags & ~0x800u;
    for (int i = effect_head_; i != absent; i = effects_[i].next)
        if (effects_[i].id == id)
            effects_[i].flags = paused ? effects_[i].flags | 0x800u : effects_[i].flags & ~0x800u;
}
void EffectRuntime::translate(Vec3 delta) noexcept {
    if (!finite(delta)) { status_ = {EffectsError::arithmetic}; return; }
    for (const int head : particle_heads_) for (int i = head; i != absent; i = particles_[i].next) {
        auto& p = particles_[i].value.position; p.x += delta.x; p.y += delta.y; p.z += delta.z;
    }
    for (int i = effect_head_; i != absent; i = effects_[i].next) {
        auto& p = effects_[i].position; p.x += delta.x; p.y += delta.y; p.z += delta.z;
    }
}
std::size_t EffectRuntime::particle_count() const noexcept {
    std::size_t n = 0;
    for (const int head : particle_heads_) for (int i = head; i != absent; i = particles_[i].next) ++n;
    return n;
}
std::size_t EffectRuntime::effect_count() const noexcept {
    std::size_t n = 0; for (int i = effect_head_; i != absent; i = effects_[i].next) ++n; return n;
}
bool EffectRuntime::particle(std::size_t order, EffectParticleView& output) const noexcept {
    for (const int head : particle_heads_) for (int i = head; i != absent; i = particles_[i].next)
        if (!order--) { output = particles_[i].value; return true; }
    return false;
}

EffectRenderResult EffectRuntime::render(const EffectCamera& camera, EffectRenderStorage storage) const noexcept {
    if (!status_) return {status_};
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
    std::array<const EffectBankBinding*, 8> bindings{};
    for (const auto& binding : storage.banks) {
        if (binding.bank_id >= banks_.size() || !banks_[binding.bank_id] || bindings[binding.bank_id])
            return {{EffectsError::resource}};
        bindings[binding.bank_id] = &binding;
    }
    // Every immutable active bank belongs to the same installed scene lease,
    // including banks whose particle lists this camera does not select.
    for (std::size_t b = 0; b < banks_.size(); ++b) {
        if (!banks_[b]) continue;
        if (!bindings[b] || bindings[b]->images.size() != banks_[b]->images.size())
            return {{EffectsError::resource}};
        for (std::size_t t = 0; t < banks_[b]->images.size(); ++t) {
            const auto& image = banks_[b]->images[t];
            const auto& binding = bindings[b]->images[t];
            const bool ci = image.format == 2 && image.size_code == 0;
            if (!ci && !((image.format == 4 && image.size_code <= 1) ||
                         (image.format == 3 && image.size_code == 1) ||
                         (image.format == 0 && (image.size_code == 2 || image.size_code == 3))))
                return {{EffectsError::unsupported}};
            if (!image.width || !image.height || image.width > 1024 || image.height > 1024 ||
                image.frames.empty())
                return {{EffectsError::extent}};
            const std::size_t pixels = std::size_t(image.width) * image.height;
            const std::size_t bytes = (pixels * (4u << image.size_code) + 7) / 8;
            // Source loadBlock uses 16-bit transfer texels for 4/8/16-bit
            // images, and the split-bank 32-bit path for RGBA32.
            const std::size_t load_bytes = image.size_code == 0 ? ((pixels + 3) / 4) * 2 :
                image.size_code == 1 ? ((pixels + 1) / 2) * 2 : bytes;
            if (bytes > (ci ? 2048u : 4096u) || load_bytes > bytes ||
                binding.frames.size() != image.frames.size() ||
                binding.palettes.size() != image.palettes.size() ||
                image.palettes.size() != (ci ? ((image.flags & 1) ? 1 : image.frames.size()) : 0))
                return {{EffectsError::extent}};
            for (std::size_t f = 0; f < image.frames.size(); ++f)
                if (image.frames[f].bytes.size() != bytes || !owns(binding.frames[f], load_bytes))
                    return {{EffectsError::extent}};
            // CI4 legalization retains exactly its 16 reachable RGBA16 entries.
            // No palette tail or source pal256 overread is installed or read.
            for (std::size_t f = 0; f < image.palettes.size(); ++f)
                if (image.palettes[f].bytes.size() != 32 || !owns(binding.palettes[f], 32))
                    return {{EffectsError::extent}};
        }
    }
    struct Draw {
        const EffectParticleView* particle = nullptr;
        const EffectImage* image = nullptr;
        std::uint32_t address = 0, palette = 0, tile_flags = 0;
        int xl = 0, yl = 0, xh = 0, yh = 0, dsdx = 0, dtdy = 0;
        std::uint16_t s = 0, t = 0, z = 0;
        bool invalidates_palette = false;
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
    auto mirror_mask = [](std::uint32_t dimension) -> unsigned {
        return dimension >= 2 && dimension <= 256 && !(dimension & (dimension - 1)) ?
            std::countr_zero(dimension) : 0;
    };
    // Own effect.c:1034-1087: list order, then linked particle order; resource
    // selection uses only the low three bits of the encoded particle bank.
    for (std::size_t list = 0; list < particle_heads_.size(); ++list) {
        if (!(camera.particle_lists & (std::uint16_t(1) << list))) continue;
        for (int i = particle_heads_[list]; i != absent; i = particles_[i].next) {
            const auto& p = particles_[i].value;
            if (p.size == 0.0f) continue;
            const auto bank_id = p.bank_id & 7;
            const auto* resource = banks_[bank_id];
            if (!resource) return {{EffectsError::resource, particles_[i].script}};
            if (p.texture >= resource->images.size() || p.frame >= resource->images[p.texture].frames.size())
                return {{EffectsError::extent, particles_[i].script}};
            const auto& image = resource->images[p.texture];
            const auto& binding = bindings[bank_id]->images[p.texture];
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
            Draw draw;
            draw.particle = &p; draw.image = &image; draw.address = binding.frames[p.frame];
            draw.invalidates_palette = image.frames[p.frame].bytes.size() >= 2048;
            if (image.format == 2) {
                const std::size_t palette = (p.flags & 0x10) ? 0 : p.frame;
                if (palette >= binding.palettes.size()) return {{EffectsError::extent, particles_[i].script}};
                draw.palette = binding.palettes[palette];
            }
            int depth = 0;
            if (!integer(xl, draw.xl) || !integer(yl, draw.yl) || !integer(xh, draw.xh) ||
                !integer(yh, draw.yh) || !integer((image.width*4096.0f)/(xh-xl), draw.dsdx) ||
                !integer((image.height*4096.0f)/(yh-yl), draw.dtdy) ||
                !integer((z*camera.viewport_scale[2] + camera.viewport_translate[2])*32.0f, depth))
                return {{EffectsError::arithmetic}};
            // Source doubles the already-truncated derivatives for mirrored axes.
            const bool mirror_s = p.flags & 0x20, mirror_t = p.flags & 0x40;
            if ((mirror_s && (draw.dsdx > std::numeric_limits<int>::max()/2 ||
                              draw.dsdx < std::numeric_limits<int>::min()/2)) ||
                (mirror_t && (draw.dtdy > std::numeric_limits<int>::max()/2 ||
                              draw.dtdy < std::numeric_limits<int>::min()/2)))
                return {{EffectsError::arithmetic}};
            if (mirror_s) draw.dsdx *= 2;
            if (mirror_t) draw.dtdy *= 2;
            draw.tile_flags = ((mirror_t ? 1u : 2u) << 18) |
                ((mirror_t ? mirror_mask(image.height) : 0u) << 14) |
                ((mirror_s ? 1u : 2u) << 8) |
                ((mirror_s ? mirror_mask(image.width) : 0u) << 4);
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
    }
    // Count cached draws before touching destination; no projection, resource
    // validation, allocation or RNG operation is repeated during publication.
    // Bound in two-word commands: setup 7 + 100*(TLUT 6 + LUT mode 1 +
    // texture 7 + primitive 1 + environment/combine 2 + blend 1 + alpha mode 1 +
    // depth 1 + rectangle 3) + final LUT reset 1 = 2308, or 4616 words.
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
        // AA_ZB_XLU_SURF/NOOP2; or XLU_SURF/NOOP2, primitive Z, no Z update.
        mode(false, 3, 29, camera.depth_test ? 0x004049d8u : 0x00404240u);
        mode(true, 19, 1, 0); mode(false, 2, 1, 4);
        mode(true, 6, 2, color_dither_); mode(true, 4, 2, alpha_dither_);
        std::uint32_t previous_image = 0, previous_palette = 0;
        bool image_valid = false, palette_valid = false;
        int previous_blend = -1, previous_alpha = -1, lut = -1;
        for (std::size_t i = 0; i < count; ++i) {
            const auto& d = draws[i]; const auto& p = *d.particle; const auto& image = *d.image;
            if (image.format == 2) {
                if (!palette_valid || previous_palette != d.palette) {
                    cmd(0xfd100000, d.palette); cmd(0xe8000000, 0);
                    cmd(0xf5000100, 0x07000000); cmd(0xe6000000, 0);
                    cmd(0xf0000000, 0x0703c000); // 16 RGBA16 entries, palette0.
                    cmd(0xe7000000, 0); previous_palette = d.palette; palette_valid = true;
                }
                if (lut != 1) { mode(true, 14, 2, 0x8000); lut = 1; }
            } else if (lut != 0) { mode(true, 14, 2, 0); lut = 0; }
            if (!image_valid || previous_image != d.address) {
                // Own effect.c:1177-1205 loadBlock variants. Four-bit and
                // eight-bit planes transfer as 16-bit; RGBA32 retains its
                // split-bank load size and half-width render-tile line stride.
                const auto size = image.size_code, format = image.format << 21;
                const auto load_size = size == 3 ? 3u : 2u;
                const auto pixels = image.width * image.height;
                const auto load_texels = size == 0 ? (pixels + 3) >> 2 :
                    size == 1 ? (pixels + 1) >> 1 : pixels;
                const auto row_words = size == 0 ? image.width >> 4 : (image.width << (size - 1)) >> 3;
                const auto words = row_words ? row_words : 1;
                const auto dxt = (2048 + words - 1) / words;
                const auto line = size == 0 ? ((image.width >> 1) + 7) >> 3 :
                    ((image.width << (size == 3 ? 1 : size - 1)) + 7) >> 3;
                cmd(0xfd000000 | format | (load_size << 19), d.address);
                cmd(0xf5000000 | format | (load_size << 19), 0x07000000 | d.tile_flags);
                cmd(0xe6000000, 0);
                cmd(0xf3000000, 0x07000000 | ((load_texels - 1) << 12) | dxt);
                cmd(0xe7000000, 0);
                cmd(0xf5000000 | format | (size << 19) | (line << 9), d.tile_flags);
                cmd(0xf2000000, ((image.width - 1) << 14) | ((image.height - 1) << 2));
                // Source invalidates at >=2048 bytes for every size, even
                // when the next CI4 draw uses the same palette address.
                if (d.invalidates_palette) palette_valid = false;
                previous_image = d.address; image_valid = true;
            }
            cmd(0xfa000000, rgba(p.primitive));
            if (p.flags & 0x80) {
                cmd(0xfb000000, rgba(p.environment));
                // Environment combine takes precedence over noise combine.
                cmd(0xfc000000 | (3u<<20) | (1u<<15) | (3u<<12) | (1u<<9) | (3u<<5) | 1u,
                    (5u<<28) | (5u<<24) | (3u<<21) | (1u<<18) | (5u<<15) |
                    (5u<<12) | (5u<<9) | (5u<<6) | (5u<<3) | 5u);
            } else if (p.flags & 0x100) {
                // RGB NOISE*TEXEL0; alpha TEXEL0*PRIMITIVE, both cycles.
                cmd(0xfc000000 | (7u<<20) | (1u<<15) | (1u<<12) | (3u<<9) | (7u<<5) | 1u,
                    (8u<<28) | (8u<<24) | (1u<<21) | (3u<<18) | (7u<<15) |
                    (7u<<12) | (7u<<9) | (7u<<6) | (7u<<3) | 7u);
            } else {
                cmd(0xfc000000 | (1u<<20) | (3u<<15) | (1u<<12) | (3u<<9) | (1u<<5) | 3u,
                    (8u<<28) | (8u<<24) | (1u<<21) | (3u<<18) | (7u<<15) |
                    (7u<<12) | (7u<<9) | (7u<<6) | (7u<<3) | 7u);
            }
            const int alpha = (p.flags & 0x400) ? 3 : 1;
            if (alpha == 1) {
                const int blend = (p.flags & 0x200) ? p.environment[3] : 8;
                if (previous_blend != blend) { cmd(0xf9000000, blend); previous_blend = blend; }
            }
            if (previous_alpha != alpha) { mode(false, 0, 2, alpha); previous_alpha = alpha; }
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

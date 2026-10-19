#ifndef ISLE_MENU_EFFECTS_HPP
#define ISLE_MENU_EFFECTS_HPP

#include "menu_animation.hpp"
#include "35mmisle/engine.h"
#include "35mmisle/rng.h"

#include <array>
#include <span>

namespace isle::menu {
enum class EffectsError { none, resource, unsupported, attachment, extent, arithmetic };
struct EffectsStatus {
    EffectsError error = EffectsError::none;
    std::size_t script = no_index, offset = 0;
    explicit operator bool() const noexcept { return error == EffectsError::none; }
};
struct EffectAttachment { animation::InstanceHandle instance; std::size_t node = no_index; };
// Raw row-vector GAME world matrix. The runtime normalizes velocity columns.
struct EffectWorld { std::array<std::array<float, 4>, 4> matrix{}; };
using EffectWorldResolver = bool (*)(void*, EffectAttachment, EffectWorld&) noexcept;
// A live model-incarnation/node credit, not an event pointer or pose snapshot.
// The issuing owner outlives this move-only value and defers model retirement
// until release. Resolution reads its current native pose; animation may advance.
class RetainedEffectAttachment final {
public:
    using Resolve = bool (*)(void*, const IsleGameModelRef&, std::size_t, EffectWorld&) noexcept;
    using Release = void (*)(void*, const IsleGameModelRef&, std::size_t) noexcept;
    RetainedEffectAttachment() noexcept = default;
    // Adopts one credit already acquired by the issuing owner.
    RetainedEffectAttachment(void*, IsleGameModelRef, std::size_t, Resolve, Release) noexcept;
    ~RetainedEffectAttachment();
    RetainedEffectAttachment(RetainedEffectAttachment&&) noexcept;
    RetainedEffectAttachment& operator=(RetainedEffectAttachment&&) noexcept;
    RetainedEffectAttachment(const RetainedEffectAttachment&) = delete;
    RetainedEffectAttachment& operator=(const RetainedEffectAttachment&) = delete;
    explicit operator bool() const noexcept { return context_ != nullptr; }
    bool resolve(EffectWorld&) const noexcept;
private:
    void reset() noexcept;
    void* context_ = nullptr;
    IsleGameModelRef binding_{};
    std::size_t node_ = no_index;
    Resolve resolve_ = nullptr;
    Release release_ = nullptr;
};
// Source B7/B8 read local DObj position; BF writes local position. These are
// separate registrations, not world-matrix lookups or renderer-cache accesses.
struct EffectTargetServices {
    void* context = nullptr;
    bool (*read)(void*, EffectAttachment, Vec3&) noexcept = nullptr;
    bool (*write)(void*, EffectAttachment, Vec3) noexcept = nullptr;
};
struct EffectCamera {
    std::array<std::array<float, 4>, 4> projection{};
    std::array<float, 3> viewport_scale{}, viewport_translate{};
    bool depth_test = true;
    std::uint16_t particle_lists = 0xffff;
};
struct EffectImageBinding {
    std::span<const std::uint32_t> frames, palettes;
};
struct EffectBankBinding {
    std::uint32_t bank_id = 0;
    std::span<const EffectImageBinding> images;
};
struct EffectRenderStorage {
    std::span<std::uint32_t> words;
    std::span<const EffectBankBinding> banks;
    // Exclusive installed reservations, retained through actual GPU retirement.
    std::span<const RamRange> asset_reservations;
};
struct EffectRenderResult { EffectsStatus status; std::size_t words = 0, particles = 0; };
struct EffectParticleView {
    Vec3 position{}, velocity{};
    float size = 0;
    std::uint16_t lifetime = 0, flags = 0, effect_id = 0;
    std::uint16_t bank_id = 0;
    std::uint8_t texture = 0, frame = 0;
    std::array<std::uint8_t, 4> primitive{}, environment{};
};
struct EffectHandle {
    std::uint32_t generation = 0;
    std::uint8_t slot = 0;
    explicit operator bool() const noexcept { return generation != 0; }
};
struct EffectSpawnResult {
    EffectsStatus status;
    bool spawned = false;
    std::uint16_t effect_id = 0;
    EffectHandle handle{};
};
// One scene pool/RNG/source-ID domain across eight independently installed bank
// slots. Banks, their Snapshot and RNG outlive this owner. A used bank cannot be
// reset/replaced; exhaustion is a successful null spawn, with immediate ticking.
class EffectRuntime final {
public:
    // Setup7 + 100*(TLUT6+LUT1+load7+color3+blend1+alpha1+Z1+rect3) + endLUT1.
    static constexpr std::size_t max_render_words = 4616;
    static constexpr std::size_t bank_count = 8;
    EffectRuntime(std::span<const EffectBank* const> banks, isle_rng& rng,
                  EffectTargetServices targets = {}) noexcept;
    EffectRuntime(const EffectRuntime&) = delete;
    EffectRuntime& operator=(const EffectRuntime&) = delete;
    EffectsStatus status() const noexcept { return status_; }
    // Empty construction stages the original reset-all/install-common3/Beach0
    // sequence. Existing callers may still supply their complete admitted banks.
    EffectsStatus reset_bank(std::size_t slot) noexcept;
    EffectsStatus install_bank(const EffectBank&) noexcept;
    // Cold interpreter/resource capability check only. No installation, pool
    // mutation or RNG advancement; live bank presence remains has_effect().
    EffectsStatus validate_bank(const EffectBank&) const noexcept;
    // Immutable admitted bank/script presence, independent of the ten-emitter
    // pool and RNG. A full pool remains an ordinary successful null spawn.
    bool has_effect(std::uint16_t encoded_bank, std::size_t script) const noexcept;
    EffectSpawnResult create_effect(std::uint16_t encoded_bank, std::size_t script,
                                    EffectAttachment attachment = {}) noexcept;
    EffectsStatus configure_effect(EffectHandle, Vec3 position, Vec3 velocity) noexcept;
    // Source position stores preserve the script-initialized emission velocity.
    EffectsStatus set_effect_position(EffectHandle, Vec3 position) noexcept;
    // Replaces the actual generation-checked emitter attachment. The incoming
    // credit is consumed even on rejection; an empty value explicitly detaches.
    EffectsStatus set_effect_attachment(EffectHandle, RetainedEffectAttachment) noexcept;
    EffectSpawnResult create_particle(std::uint16_t encoded_bank, std::size_t script) noexcept;
    EffectSpawnResult create_particle(std::uint16_t encoded_bank, std::size_t script,
                                      Vec3 position, Vec3 velocity) noexcept;
    EffectsStatus update_particles() noexcept;
    EffectsStatus update_effects(EffectWorldResolver resolve = nullptr, void* context = nullptr) noexcept;
    EffectsStatus register_aim_target(std::size_t slot, EffectAttachment) noexcept;
    EffectsStatus register_output_target(std::size_t slot, EffectAttachment) noexcept;
    void clear_targets(animation::InstanceHandle) noexcept;
    void detach_instance(animation::InstanceHandle) noexcept;
    void eject_effect(EffectHandle) noexcept;
    void eject_id(std::uint16_t id, std::uint8_t particle_list) noexcept;
    void pause_id(std::uint16_t id, std::uint8_t particle_list, bool paused) noexcept;
    void pause_lists(std::uint16_t lists) noexcept { paused_lists_ = lists; }
    void translate(Vec3 displacement) noexcept;
    // Requires no submitted GPU lease; preserves RNG and wrapping source counter.
    void reset() noexcept;
    void set_dither(std::uint8_t color, std::uint8_t alpha) noexcept;
    std::size_t particle_count() const noexcept;
    std::size_t effect_count() const noexcept;
    bool particle(std::size_t source_order, EffectParticleView& output) const noexcept;
    EffectRenderResult render(const EffectCamera&, EffectRenderStorage) const noexcept;
private:
    static constexpr int absent = -1;
    struct Particle {
        EffectParticleView value;
        int next = absent;
        bool live = false;
        std::size_t script = 0, cursor = 0, return_cursor = 0, loop_cursor = 0;
        std::uint8_t loop_count = 0;
        std::uint16_t timer = 1, size_time = 0, primitive_time = 0, environment_time = 0;
        float gravity = 0, friction = 1, size_target = 0;
        std::array<std::uint8_t, 4> primitive_target{}, environment_target{};
    };
    struct Effect {
        int next = absent;
        bool live = false;
        std::uint32_t generation = 0;
        std::size_t script = 0;
        EffectAttachment attachment{};
        RetainedEffectAttachment retained_attachment;
        std::uint16_t id = 0, bank_id = 0, lifetime = 0, flags = 0;
        Vec3 position{}, velocity{};
        float accumulator = 0;
    };
    std::array<const EffectBank*, bank_count> banks_{};
    isle_rng* rng_;
    EffectTargetServices targets_;
    EffectsStatus status_{};
    std::array<EffectAttachment, 1> aim_targets_{};
    std::array<EffectAttachment, 8> output_targets_{};
    std::array<Particle, 100> particles_{};
    std::array<Effect, 10> effects_{};
    std::array<int, 16> particle_heads_{};
    int particle_free_ = absent;
    int effect_free_ = absent, effect_head_ = absent;
    std::uint16_t current_id_ = 0, paused_lists_ = 0;
    std::uint8_t color_dither_ = 0xc0, alpha_dither_ = 0x30;
    const EffectBank* bank(std::uint16_t encoded) const noexcept;
    int spawn_effect(std::uint16_t encoded, std::size_t script, EffectAttachment) noexcept;
    int spawn_particle(std::uint16_t encoded, std::size_t script, Vec3 position,
                       Vec3 velocity, int parent, int effect) noexcept;
    int tick_particle(int index, int previous) noexcept;
    void remove_particle(int index, int previous) noexcept;
    void remove_effect(int index, int previous) noexcept;
};
} // namespace isle::menu
#endif

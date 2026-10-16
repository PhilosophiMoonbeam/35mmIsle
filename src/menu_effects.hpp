#ifndef ISLE_MENU_EFFECTS_HPP
#define ISLE_MENU_EFFECTS_HPP

#include "menu_animation.hpp"
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
// Row-vector GAME world matrix, including every parent/local auxiliary transform.
// Velocity is transformed by independently normalized columns, NOT frame motion.
struct EffectWorld { std::array<std::array<float, 4>, 4> matrix{}; };
using EffectWorldResolver = bool (*)(void*, EffectAttachment, EffectWorld&) noexcept;
struct EffectCamera {
    // GAME look-at-roll * perspective-fast result, row-vector convention.
    std::array<std::array<float, 4>, 4> projection{};
    std::array<float, 3> viewport_scale{}, viewport_translate{};
    bool depth_test = true;
};
struct EffectImageBinding {
    // Physical RDRAM offsets. All frames/palettes are installed once by caller.
    std::array<std::uint32_t, 6> frames{}, palettes{};
};
struct EffectRenderStorage {
    std::span<std::uint32_t> words; // Canonical F3DEX2 words, not host bytes.
    std::span<const EffectImageBinding> images;
    // Caller-owned, exclusive, installed reservations; retained through GPU idle.
    std::span<const RamRange> asset_reservations;
};
struct EffectRenderResult { EffectsStatus status; std::size_t words = 0, particles = 0; };
struct EffectParticleView {
    Vec3 position{}, velocity{};
    float size = 0;
    std::uint16_t lifetime = 0, flags = 0, effect_id = 0;
    std::uint8_t texture = 0, frame = 0;
    std::array<std::uint8_t, 4> primitive{}, environment{};
};
struct EffectEventResult {
    EffectsStatus status;
    bool title_finished = false, spawned = false;
    std::uint16_t effect_id = 0;
};
// Serialized gameplay owner. Catalog/Snapshot and RNG must outlive this runtime;
// immutable GPU asset leases outlive submitted packets until caller GPU quiescence.
// No SDK layouts, guest pointers, global renderer, allocation, or throwing callbacks.
// Call update_particles and update_effects separately in OM source object order;
// newly emitted particles execute their first interpreter/physics tick immediately.
class MenuEffects final {
public:
    static constexpr std::size_t max_render_words = 6020;
    MenuEffects(const Catalog& catalog, isle_rng& rng) noexcept;
    MenuEffects(const MenuEffects&) = delete;
    MenuEffects& operator=(const MenuEffects&) = delete;
    EffectsStatus status() const noexcept { return status_; }
    EffectEventResult animation_event(EffectAttachment attachment, std::int32_t event_id,
                                     float argument) noexcept;
    EffectsStatus update_particles() noexcept;
    EffectsStatus update_effects(EffectWorldResolver resolve, void* context) noexcept;
    // Detach source fx_ejectEffectDObj: remove attached emitters; live ordinary
    // particles survive independently. reset requires no outstanding GPU leases.
    void detach_instance(animation::InstanceHandle) noexcept;
    void reset() noexcept;
    std::size_t particle_count() const noexcept;
    std::size_t effect_count() const noexcept;
    bool particle(std::size_t source_order, EffectParticleView& output) const noexcept;
    // Failure leaves words untouched; caller owns color/depth/scissor/task state.
    // No END/full-sync appended: splice this producer into canonical scene packet.
    EffectRenderResult render(const EffectCamera&, EffectRenderStorage) const noexcept;
private:
    static constexpr int absent = -1;
    struct Particle {
        EffectParticleView value;
        int next = absent;
        std::size_t script = 0, cursor = 0, return_cursor = 0;
        std::uint16_t timer = 1, size_time = 0, primitive_time = 0, environment_time = 0;
        float gravity = 0, friction = 1, size_target = 0;
        std::array<std::uint8_t, 4> primitive_target{}, environment_target{};
    };
    struct Effect {
        int next = absent;
        std::size_t script = 0;
        EffectAttachment attachment{};
        std::uint16_t id = 0, lifetime = 0, flags = 0;
        Vec3 position{}, velocity{};
        float accumulator = 0;
    };
    const EffectBank* bank_ = nullptr;
    isle_rng* rng_;
    EffectsStatus status_{};
    std::array<Particle, 100> particles_{};
    std::array<Effect, 10> effects_{};
    int particle_free_ = absent, particle_head_ = absent;
    int effect_free_ = absent, effect_head_ = absent;
    std::uint16_t current_id_ = 0;
    int spawn_effect(std::size_t script, EffectAttachment) noexcept;
    int spawn_particle(int effect, Vec3 position, Vec3 velocity) noexcept;
    int tick_particle(int index, int previous) noexcept;
    void remove_effect(int index, int previous) noexcept;
};
} // namespace isle::menu
#endif

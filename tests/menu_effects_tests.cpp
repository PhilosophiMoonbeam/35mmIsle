// Private admitted ROM transition tests. No assets or bytecode are embedded.
#include "menu_effects.hpp"
#include "rom_snapshot.hpp"

#include <array>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
using namespace isle::menu;
void require(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
void ok(EffectsStatus s) { require(bool(s), "effect runtime rejected source transition"); }
bool world(void* context, EffectAttachment, EffectWorld& out) noexcept {
    out = *static_cast<EffectWorld*>(context); return true;
}
bool missing(void*, EffectAttachment, EffectWorld&) noexcept { return false; }
EffectWorld identity() {
    EffectWorld w{}; for (unsigned k = 0; k < 4; ++k) w.matrix[k][k] = 1; return w;
}
bool same(const EffectParticleView& a, const EffectParticleView& b) {
    return a.position.x == b.position.x && a.position.y == b.position.y && a.position.z == b.position.z &&
        a.velocity.x == b.velocity.x && a.velocity.y == b.velocity.y && a.velocity.z == b.velocity.z &&
        a.size == b.size && a.lifetime == b.lifetime && a.flags == b.flags && a.effect_id == b.effect_id &&
        a.texture == b.texture && a.frame == b.frame && a.primitive == b.primitive && a.environment == b.environment;
}
void transitions(const Catalog& catalog) {
    isle_rng rng{0x12345678}; MenuEffects runtime(catalog, rng); ok(runtime.status());
    require(runtime.animation_event({animation::InstanceHandle{1,1,2}, 0}, -1, 0).title_finished, "END callback not delivered");
    const auto before = rng.seed;
    require(runtime.animation_event({animation::InstanceHandle{1,1,2}, 0}, 1, 1).status.error == EffectsError::unsupported,
            "unadmitted bank accepted");
    require(!runtime.animation_event({animation::InstanceHandle{1,1,2}, 0}, 0, 0).spawned && rng.seed == before,
            "negative source script selection consumed RNG");
    for (unsigned i = 0; i < 10; ++i) {
        const auto event = runtime.animation_event({animation::InstanceHandle{1,1,2}, 0}, 0, 1);
        require(bool(event.status) && event.spawned && event.effect_id == i + 1, "source effect free-list/id order");
    }
    const auto exhausted = runtime.animation_event({animation::InstanceHandle{1,1,2}, 0}, 0, 1);
    require(bool(exhausted.status) && !exhausted.spawned && runtime.effect_count() == 10 && rng.seed == before,
            "effect exhaustion is not GAME null-spawn");
    require(runtime.update_effects(missing, nullptr).error == EffectsError::attachment && rng.seed == before &&
            runtime.particle_count() == 0, "attachment failure partially published tick");
    auto w = identity();
    for (unsigned tick = 0; tick < 5; ++tick) { ok(runtime.update_particles()); ok(runtime.update_effects(world, &w)); }
    require(runtime.effect_count() == 0 && runtime.particle_count() == 100,
            "emitter lifetime/pool transition");
    const auto retired_seed = rng.seed;
    ok(runtime.update_effects(world, &w));
    require(rng.seed == retired_seed, "retired emitter consumed RNG");
    // Once the pool is exhausted GAME still consumes emitter geometry RNG; no
    // fabricated particles or retry-on-next-frame accounting is introduced.
    for (unsigned tick = 0; tick < 80; ++tick) ok(runtime.update_particles());
    require(runtime.particle_count() == 0, "particle lifetime did not retire source loops");
    runtime.reset(); require(runtime.effect_count() == 0 && runtime.particle_count() == 0, "reset retained pool state");
    const auto event = runtime.animation_event({animation::InstanceHandle{1,1,2}, 1}, 0, 1); require(event.spawned, "spawn after reset");
    for (unsigned tick = 0; tick < 5 && runtime.particle_count() == 0; ++tick) ok(runtime.update_effects(world, &w));
    const auto live = runtime.particle_count(); require(live > 0, "deterministic source emitter produced no particles");
    runtime.detach_instance(animation::InstanceHandle{1,1,2});
    require(runtime.effect_count() == 0 && runtime.particle_count() == live, "DObj detach removed ordinary particles");
    for (unsigned tick = 0; tick < 80; ++tick) ok(runtime.update_particles());
    require(runtime.particle_count() == 0, "detached particle teardown");
    const animation::InstanceHandle old_instance{7,1,2}, reused_instance{7,2,2}, foreign_pool{8,2,2};
    require(runtime.animation_event({old_instance,0},0,1).spawned &&
            runtime.animation_event({reused_instance,0},0,1).spawned &&
            runtime.animation_event({foreign_pool,0},0,1).spawned,
            "explicit instance attachment emitters");
    runtime.detach_instance(old_instance);
    require(runtime.effect_count() == 2,"retiring incarnation detached another same-slot occupant or pool");
    runtime.detach_instance(old_instance);
    require(runtime.effect_count() == 2,"stale detach removed a newer incarnation");
    runtime.detach_instance(foreign_pool);
    require(runtime.effect_count() == 1,"foreign pool detach conflated otherwise equal slot/incarnation");
    runtime.detach_instance(reused_instance);
    require(runtime.effect_count() == 0,"live incarnation attachment did not detach");
}
void replay_and_columns(const Catalog& catalog) {
    isle_rng a_rng{0x12345678}, b_rng{0x12345678};
    MenuEffects a(catalog, a_rng), b(catalog, b_rng); ok(a.status()); ok(b.status());
    auto normal = identity(), scaled = identity();
    scaled.matrix[0][0] = 2; scaled.matrix[1][1] = 3; scaled.matrix[2][2] = 4;
    for (float argument : {1.0f, 4.0f}) {
        a.reset(); b.reset(); a_rng.seed = b_rng.seed = 0x12345678;
        require(a.animation_event({animation::InstanceHandle{1,1,2}, 0}, 0, argument).spawned && b.animation_event({animation::InstanceHandle{1,1,2}, 0}, 0, argument).spawned,
                "explicit selected script spawn");
        for (unsigned tick = 0; tick < 100; ++tick) {
            ok(a.update_particles()); ok(b.update_particles());
            ok(a.update_effects(world, &normal)); ok(b.update_effects(world, &scaled));
            require(a_rng.seed == b_rng.seed && a.particle_count() == b.particle_count() && a.effect_count() == b.effect_count(),
                    "scaled columns changed source RNG/pool order");
            for (std::size_t p = 0; p < a.particle_count(); ++p) {
                EffectParticleView av{}, bv{}; require(a.particle(p, av) && b.particle(p, bv) && same(av, bv),
                    "velocity used scale/frame delta rather than normalized world columns");
            }
        }
        require(a.particle_count() == 0 && a.effect_count() == 0, "script teardown retained live state");
    }
}
void script_transitions(const Catalog& catalog) {
    auto w = identity();
    for (const std::size_t script : {std::size_t(0), std::size_t(3)}) {
        isle_rng rng{0x12345678}; MenuEffects runtime(catalog, rng); ok(runtime.status());
        require(runtime.animation_event({animation::InstanceHandle{1,1,2}, 0}, 0, float(script+1)).spawned, "transition script spawn");
        for (unsigned t = 0; t < 5 && runtime.particle_count() == 0; ++t) ok(runtime.update_effects(world, &w));
        require(runtime.particle_count() > 0, "transition script emitted no particles");
        runtime.detach_instance(animation::InstanceHandle{1,1,2});
        const auto& source = catalog.effects().scripts[script];
        EffectParticleView p{}; require(runtime.particle(0, p), "first source particle");
        require(p.lifetime == source.particle_lifetime, "immediate particle first tick/lifetime off by one");
        float expected_size = source.parameters[8], target_size = 0;
        unsigned size_time = 0;
        for (const auto& ins : source.instructions) if (ins.opcode == 0xa0) {
            size_time = source.bytecode.u8(ins.offset+1)+1;
            const auto offset = ins.offset+2;
            const isle::rom::GuestView operand{0, source.bytecode.rom_offset+offset,
                                               source.bytecode.bytes.subspan(offset, 4)};
            target_size = operand.f32(0);
        }
        if (script == 0) {
            expected_size += (target_size-expected_size)/float(size_time--);
            require(p.frame == 0 && p.size == expected_size, "script0 initial frame/size ramp");
            for (unsigned t = 1; t <= 22; ++t) {
                ok(runtime.update_particles()); require(runtime.particle(0, p), "script0 premature retirement");
                expected_size += (target_size-expected_size)/float(size_time--);
                const unsigned frame = t < 6 ? 0 : t < 10 ? 1 : t < 12 ? 2 : t < 16 ? 3 : t < 18 ? 4 : t < 22 ? 5 : 0;
                require(p.frame == frame && p.size == expected_size, "source duration/FD return/size ramp transition");
            }
            for (unsigned t = 22; t < source.particle_lifetime; ++t) ok(runtime.update_particles());
        } else {
            require(p.frame == 1 && p.size >= 4 && p.size < 8.2f, "zero-duration frame must continue into AC/frame1");
            for (unsigned t = 0; t < 20; ++t) ok(runtime.update_particles());
            require(runtime.particle(0, p) && p.frame == 4 && p.primitive[3] == 250,
                    "source color ramp integer16 interpolation at first target tick");
            for (unsigned t = 20; t < 44; ++t) ok(runtime.update_particles());
            require(runtime.particle_count() > 0, "script3 expired before bytecode END");
            ok(runtime.update_particles());
        }
        require(runtime.particle_count() == 0, "source lifetime/bytecode END precedence");
    }
}
void packet_boundaries(const Catalog& catalog) {
    isle_rng rng{0x12345678}; MenuEffects runtime(catalog, rng); ok(runtime.status());
    auto w = identity();
    for (unsigned i = 0; i < 10; ++i) require(runtime.animation_event({animation::InstanceHandle{1,1,2}, 0}, 0, 4).spawned, "packet script spawn");
    for (unsigned t = 0; t < 3; ++t) { ok(runtime.update_particles()); ok(runtime.update_effects(world, &w)); }
    std::array<EffectImageBinding, 2> bindings{};
    std::uint32_t address = 0x1b0000;
    for (std::size_t t = 0; t < catalog.effects().images.size(); ++t) {
        const auto& image = catalog.effects().images[t];
        for (std::size_t f = 0; f < image.frames.size(); ++f) { bindings[t].frames[f] = address; address += 512; }
        for (std::size_t f = 0; f < image.palettes.size(); ++f) { bindings[t].palettes[f] = address; address += 32; }
    }
    const std::array<RamRange, 1> reservations{{{0x1b0000, address-0x1b0000}}};
    EffectCamera camera{};
    camera.projection[0][0] = camera.projection[1][1] = 0.04f;
    camera.projection[2][2] = 0.001f; camera.projection[3][3] = 1;
    camera.viewport_scale = {128, 96, 511}; camera.viewport_translate = {128, 96, 511};
    std::array<std::uint32_t, MenuEffects::max_render_words> words{};
    words.fill(0xdeadbeef);
    auto result = runtime.render(camera, {std::span(words).first(4), bindings, reservations});
    require(result.status.error == EffectsError::extent && result.words > 4, "small packet storage accepted");
    for (auto word : words) require(word == 0xdeadbeef, "packet capacity failure partially published");
    result = runtime.render(camera, {words, bindings, {}});
    require(result.status.error == EffectsError::extent, "unreserved GPU asset accepted");
    for (auto word : words) require(word == 0xdeadbeef, "asset extent failure partially published");
    result = runtime.render(camera, {words, bindings, reservations}); ok(result.status);
    require(result.particles > 0 && result.words <= words.size(), "canonical packet failed to emit particles");
    unsigned palette_loads = 0, rectangles = 0;
    for (std::size_t i = 0; i < result.words; i += 2) {
        if (words[i] >> 24 == 0xf0) {
            require(((words[i+1] >> 14)&1023) == 15, "CI4 palette legalization read unowned entries");
            ++palette_loads;
        }
        if (words[i] >> 24 == 0xe4) ++rectangles;
    }
    require(palette_loads > 0 && rectangles == result.particles, "canonical CI4 packet command closure");
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 2, "expected one explicit private ROM path");
        const auto snapshot = isle::rom::read_snapshot(std::filesystem::path(argv[1]));
        const auto hydrated = isle::menu::hydrate(snapshot); require(bool(hydrated), "resource admission failed");
        transitions(*hydrated.catalog); replay_and_columns(*hydrated.catalog);
        script_transitions(*hydrated.catalog); packet_boundaries(*hydrated.catalog);
        std::cout << "menu_effect_transitions=passed Original_parity=unqualified\n"; return 0;
    } catch (const std::filesystem::filesystem_error&) { std::cerr << "private ROM operation failed\n"; }
      catch (const std::exception& e) { std::cerr << e.what() << '\n'; }
    return 1;
}

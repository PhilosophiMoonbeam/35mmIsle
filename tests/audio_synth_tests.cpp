#include "35mmisle/audio_synth.hpp"
#include "../src/audio_synth_internal.hpp"
#include "audio_fixture.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <initializer_list>
#include <limits>
#include <string_view>
#include <utility>
#include <vector>

namespace a = isle::audio;
namespace d = isle::audio::detail;
namespace {
int failures = 0;
void check(bool condition, std::string_view meaning) {
    if (!condition) { ++failures; std::cerr << "FAIL: " << meaning << '\n'; }
}
struct Core {
    isle::audio_test::Fixture fixture;
    a::BankFile bank;
    std::unique_ptr<d::Context> storage = std::make_unique<d::Context>();
    explicit Core(std::uint16_t physical = 22, std::uint16_t logical = 42, std::uint64_t owner = 17,
                  bool pack_catalog = true) {
        auto decoded = fixture.decode();
        check(static_cast<bool>(decoded), "authored synth fixture hydrates");
        bank = std::move(decoded.asset);
        auto& context = *storage;
        context.owner = owner;
        context.config = a::SynthConfig{32006, physical, logical, {448, 512}};
        context.coefficients = isle::audio_test::coefficients();
        context.memory = std::make_unique<std::uint32_t[]>(a::AudioService::rdram_bytes / 4);
        if (pack_catalog) {
            const std::array owners{bank};
            check(d::build_catalog(context, owners) == a::Error::none, "authored immutable catalog packs");
        }
    }
    d::Context& context() { return *storage; }
    d::CpuState& state() { return storage->state(); }
    a::WaveId wave() const { return a::WaveId{storage->owner, 0}; }
    a::VoiceHandle voice() {
        a::VoiceHandle handle{};
        check(d::create_voice(context(), state(), handle) == a::Error::none, "logical descriptor creates");
        return handle;
    }
    a::Allocation allocate(a::VoiceHandle handle, std::int16_t priority = 64, std::uint64_t due = 0) {
        a::Allocation result{};
        check(d::allocate_voice(context(), state(), handle, priority, a::ParamTime{due}, result) == a::Error::none,
              "physical allocation succeeds");
        return result;
    }
    void start(a::VoiceHandle handle, std::uint64_t due = 0, std::uint32_t attack = 0) {
        check(d::start_voice(context(), state(), handle, wave(), {1.0f, {24000}, 64}, {attack}, {due}) == a::Error::none,
              "direct linear-gain source starts");
    }
    void pull(std::uint32_t frames, std::uint32_t offset = 0, std::uint32_t total = 0) {
        check(d::render(context(), state(), frames, offset, total == 0 ? frames : total) == a::Error::none,
              "planned shared DSP span renders");
        if (offset + frames == (total == 0 ? frames : total)) d::collect_voices(state());
    }
};
bool same(a::VoiceHandle left, a::VoiceHandle right) {
    return left.owner == right.owner && left.generation == right.generation && left.slot == right.slot;
}
void descriptor_and_global_allocator() {
    Core core;
    std::array<a::VoiceHandle, 42> handles{};
    for (auto& handle : handles) handle = core.voice();
    a::VoiceHandle untouched{123, 456, 7};
    check(d::create_voice(core.context(), core.state(), untouched) == a::Error::descriptor_capacity &&
          untouched.owner == 123 && untouched.generation == 456, "42 descriptors are bounded independently of22 physical voices");
    for (std::uint16_t slot = 0; slot < 22; ++slot) {
        const auto allocation = core.allocate(handles[slot], slot == 7 || slot == 15 ? 5 : 100);
        check(allocation.binding.physical_slot == slot && allocation.displaced.owner == 0,
              "first physical allocation order is P01 through P22");
    }
    a::Allocation rejected{};
    check(d::allocate_voice(core.context(), core.state(), handles[22], 4, {0}, rejected) == a::Error::voice_unavailable,
          "priority below global minimum is rejected");
    const auto original_age = core.state().physical[7].allocation_age;
    const auto equal = core.allocate(handles[22], 5);
    check(equal.binding.physical_slot == 7 && same(equal.displaced, handles[7]),
          "equality accepts oldest allocation among global minimum, not pool edge");
    a::BindingInfo binding{};
    check(d::voice_binding(core.context(), core.state(), handles[7], binding) == a::Error::none && !binding.bound,
          "steal immediately unbinds old owner without invalidating logical handle");
    check(d::pan_voice(core.context(), core.state(), handles[7], 32, {0}) == a::Error::unbound,
          "stolen owner's control cannot affect replacement");
    const auto next = core.allocate(handles[23], 5);
    check(next.binding.physical_slot == 15 && same(next.displaced, handles[15]),
          "an inactive displaced binding is excluded immediately from the next minimum-priority admission");
    check(core.state().physical[7].allocation_age == original_age,
          "displacement keeps current allocation age rather than reinserting its physical node");
    check(d::destroy_voice(core.context(), core.state(), handles[7]) == a::Error::none,
          "unbound displaced logical descriptor can be destroyed");
    auto reused = core.voice();
    check(reused.slot == handles[7].slot && reused.generation != handles[7].generation,
          "descriptor reuse has a new reservation generation");
    check(d::priority_voice(core.context(), core.state(), handles[7], 0) == a::Error::stale_handle,
          "old logical generation is rejected");
}
void allocation_age_and_stop_binding() {
    Core core(2, 6);
    auto first = core.voice(), second = core.voice(), third = core.voice();
    core.allocate(first); core.allocate(second);
    core.start(first);
    check(d::stop_voice(core.context(), core.state(), first, {0}) == a::Error::none, "stop deactivates source");
    a::BindingInfo binding{};
    check(d::voice_binding(core.context(), core.state(), first, binding) == a::Error::none && binding.bound,
          "stop retains physical binding");
    core.start(first);
    check(d::stop_voice(core.context(), core.state(), first, {0}) == a::Error::none, "restart can stop again");
    const auto steal = core.allocate(third);
    check(same(steal.displaced, first), "stop and restart do not refresh allocation age");
    Core reuse(2, 4);
    auto reused_first = reuse.voice(), reused_second = reuse.voice();
    auto fresh = reuse.voice(), equal = reuse.voice();
    reuse.allocate(reused_first); reuse.allocate(reused_second);
    check(d::free_voice(reuse.context(), reuse.state(), reused_first, {0}) == a::Error::none,
          "normal stopped never-stolen binding frees immediately");
    check(reuse.allocate(fresh).binding.physical_slot == 0, "fresh allocation reuses normal free slot");
    check(same(reuse.allocate(equal).displaced, reused_second),
          "free and fresh allocation refresh age rather than preserving old node age");
}
void inactive_displacement_exclusion() {
    Core core;
    std::array<a::VoiceHandle, 42> handles{};
    for (auto& handle : handles) handle = core.voice();
    for (std::uint16_t slot = 0; slot < 22; ++slot) core.allocate(handles[slot]);
    for (std::uint16_t index = 0; index < 20; ++index) {
        const auto allocation = core.allocate(handles[22 + index]);
        check(allocation.binding.physical_slot == index && same(allocation.displaced, handles[index]),
              "inactive equal-priority batch displaces each eligible physical allocation once in oldest order");
    }
    check(core.allocate(handles[0]).binding.physical_slot == 20 &&
          core.allocate(handles[1]).binding.physical_slot == 21,
          "all22 physical bindings admit exactly one inactive displacement before exclusion");
    a::Allocation untouched{{true, 9, 123}, {}};
    check(d::allocate_voice(core.context(), core.state(), handles[2], 32767, {0}, untouched) == a::Error::voice_unavailable &&
          untouched.binding.physical_slot == 9,
          "after22 initial plus22 displaced allocations, higher priority cannot bypass universal stolen exclusion");
    check(d::free_voice(core.context(), core.state(), handles[22], {0}) == a::Error::none &&
          core.state().physical[0].allocated && core.state().physical[0].owner == a::no_slot,
          "inactive stolen binding also remains reserved until explicit retirement");
}
void free_retiring_and_collection_order() {
    Core immediate(4, 7);
    auto first = immediate.voice(), second = immediate.voice(), third = immediate.voice();
    auto next = immediate.voice(), last = immediate.voice();
    immediate.allocate(first); immediate.allocate(second); immediate.allocate(third);
    check(d::free_voice(immediate.context(), immediate.state(), second, {0}) == a::Error::none &&
          d::free_voice(immediate.context(), immediate.state(), first, {0}) == a::Error::none,
          "two ordinary frees enter the retiring list without a CPU AudioFrame collection");
    check(immediate.allocate(next).binding.physical_slot == 0 &&
          immediate.allocate(last).binding.physical_slot == 1,
          "allocation consumes retiring heads before an existing free-chain entry");

    Core collected(3, 7);
    first = collected.voice(); second = collected.voice(); third = collected.voice();
    next = collected.voice(); last = collected.voice();
    collected.allocate(first); collected.allocate(second); collected.allocate(third);
    check(d::free_voice(collected.context(), collected.state(), second, {0}) == a::Error::none &&
          d::free_voice(collected.context(), collected.state(), first, {0}) == a::Error::none,
          "same frees can instead reach an AudioFrame collection");
    collected.pull(16);
    check(collected.allocate(next, 64, 16).binding.physical_slot == 1 &&
          collected.allocate(last, 64, 16).binding.physical_slot == 0,
          "AudioFrame reverses the retiring chain onto free head before later allocation");

    Core deferred(3, 7);
    first = deferred.voice(); second = deferred.voice(); third = deferred.voice();
    next = deferred.voice(); last = deferred.voice();
    auto reuse_first = deferred.voice(), reuse_second = deferred.voice();
    deferred.allocate(first, 0); deferred.allocate(second, 0); deferred.allocate(third, 100);
    deferred.start(first); deferred.start(second); deferred.pull(16);
    deferred.allocate(next, 0, 16); deferred.allocate(last, 0, 16);
    check(d::free_voice(deferred.context(), deferred.state(), next, {16}) == a::Error::none &&
          d::free_voice(deferred.context(), deferred.state(), last, {16}) == a::Error::none,
          "stolen frees remain on allocated list until their effective parameter events");
    deferred.pull(512);
    check(d::advance_parameters(deferred.context(), deferred.state(), {528}) == a::Error::none,
          "both deferred frees become ready at their shared effective boundary");
    d::collect_voices(deferred.state());
    check(deferred.allocate(reuse_first, 0, 528).binding.physical_slot == 1 &&
          deferred.allocate(reuse_second, 0, 528).binding.physical_slot == 0,
          "same-time deferred frees follow physical visit order before retiring-to-free reversal, not caller slot order");
}
void ordinary_active_free_is_immediately_reclaimable() {
    Core core(1, 4);
    auto old = core.voice(), incoming = core.voice();
    core.allocate(old, 100); core.start(old); core.pull(160);
    const auto age = core.state().physical[0].allocation_age;
    check(d::free_voice(core.context(), core.state(), old, {160}) == a::Error::none,
          "ordinary active owner can explicitly free without a prior stop");
    a::BindingInfo binding{};
    check(d::voice_binding(core.context(), core.state(), old, binding) == a::Error::none && !binding.bound,
          "ordinary active free immediately unbinds old logical owner");
    const auto allocation = core.allocate(incoming, 0, 160);
    check(allocation.binding.physical_slot == 0 && allocation.displaced.owner == 0 &&
          core.state().physical[0].stealable && core.state().physical[0].allocation_age > age,
          "ordinary active free is immediately reclaimable by lower priority as a fresh allocation");
    core.start(incoming, 160);
    check(!core.state().physical[0].pending.valid && core.state().physical[0].current.binding_epoch == allocation.binding.epoch,
          "ordinary active free does not impose a stolen512-frame restart delay");
    check(core.state().physical[0].current.cursor == 0 && core.state().physical[0].current.decoded_to == 0 &&
          core.state().physical[0].current.adpcm_flags == 0 && !core.state().physical[0].current.resample_init &&
          core.state().physical[0].current.envelope_init,
          "unstopped fresh start restarts CPU source at zero but retains initialized DSP histories");
}
void ordinary_free_tail_and_stopped_reuse() {
    Core core(1, 4, 17, false);
    isle::audio_test::Fixture replacement_fixture;
    isle::audio_test::put16(*replacement_fixture.bank, 178, 0x1000);
    auto replacement_bank = replacement_fixture.decode();
    check(static_cast<bool>(replacement_bank), "distinct authored replacement bank hydrates");
    const std::array owners{core.bank, replacement_bank.asset};
    check(d::build_catalog(core.context(), owners) == a::Error::none,
          "distinct authored old and replacement waves pack before rendering");
    a::SoundId replacement_sound{};
    const d::CatalogSound* replacement_resource = nullptr;
    check(d::find_sound(core.context(), isle::audio_test::selected(replacement_bank.asset), replacement_sound) == a::Error::none &&
          d::catalog_sound(core.context(), replacement_sound, replacement_resource) == a::Error::none,
          "replacement source has its own catalog identity");
    if (replacement_resource == nullptr) return;
    const a::WaveId replacement_wave{core.context().owner, replacement_resource->wave_index};
    auto old = core.voice(), incoming = core.voice();
    core.allocate(old); core.start(old);
    check(d::pitch_voice(core.context(), core.state(), old, 1.0f / 3.0f, {0}) == a::Error::none,
          "old chain can have a nonzero planned resampler fraction");
    core.pull(160);
    const auto source_epoch = core.state().physical[0].current.epoch;
    check(d::free_voice(core.context(), core.state(), old, {160}) == a::Error::none &&
          !core.state().physical[0].allocated && core.state().physical[0].current.active &&
          core.state().parameter_count == 0,
          "ordinary free changes ownership without kill, stop or512 retirement events");
    core.pull(656);
    auto& current = core.state().physical[0].current;
    check(current.active && current.epoch == source_epoch && !current.resample_init,
          "unreused ordinary free keeps its source live beyond free plus512");
    const auto fraction = current.fraction;
    check(fraction != 0, "authored old pitch naturally produces nonzero CPU fraction");
    core.allocate(incoming, 0, 816);
    check(d::start_voice(core.context(), core.state(), incoming, replacement_wave, {1.5f, {6000}, 127}, {0}, {816}) == a::Error::none &&
          current.wave.index == replacement_wave.index && current.cursor == 0 && current.decoded_to == 0 &&
          current.fraction == fraction && current.adpcm_flags == 0 && !current.resample_init && current.envelope_init,
          "active reuse selects new wave at CPU offset zero, retains live DSP state and keeps fraction consistent");

    Core stopped(1, 4);
    auto stopped_old = stopped.voice(), stopped_new = stopped.voice();
    stopped.allocate(stopped_old); stopped.start(stopped_old); stopped.pull(160);
    check(d::stop_voice(stopped.context(), stopped.state(), stopped_old, {192}) == a::Error::none &&
          d::stop_voice(stopped.context(), stopped.state(), stopped_old, {224}) == a::Error::none &&
          d::free_voice(stopped.context(), stopped.state(), stopped_old, {192}) == a::Error::none,
          "future stop and free can precede immediate logical reassignment");
    stopped.allocate(stopped_new, 0, 192); stopped.start(stopped_new, 192);
    stopped.pull(32);
    check(d::advance_parameters(stopped.context(), stopped.state(), {192}) == a::Error::none &&
          stopped.state().physical[0].current.adpcm_flags == 1 && stopped.state().physical[0].current.resample_init &&
          stopped.state().physical[0].current.fraction == 0,
          "queued source-qualified stop survives reassignment and stopped reuse initializes both DSP histories");
    stopped.pull(32);
    check(d::advance_parameters(stopped.context(), stopped.state(), {224}) == a::Error::none &&
          stopped.state().physical[0].current.active,
          "later stop from the old source epoch never stops its replacement");
}
struct PlannedSpan {
    std::uint32_t begin = 0, frames = 0;
    std::array<bool, a::AudioService::max_physical_voices> adpcm{}, resample{};
    std::array<std::uint8_t, a::AudioService::max_physical_voices> envelope_flags{}, resample_flags{};
};
// Decode emitted ABI1 intent, not samples: each SAVE identifies the shared
// output span containing the preceding per-physical DSP visits.
std::vector<PlannedSpan> planned_spans(const d::Context& context) {
    std::vector<PlannedSpan> result;
    PlannedSpan pending{};
    std::uint32_t bytes = 0;
    for (auto address = a::AudioService::command_address; address < context.write_address; address += 8) {
        const auto first = context.memory[address / 4];
        const auto second = context.memory[address / 4 + 1];
        const auto opcode = first >> 24;
        if (opcode == 8 && ((first >> 16) & 0xffu) == 0) bytes = second & 0xffffu;
        if ((opcode == 1 || opcode == 3 || opcode == 5) && second >= a::AudioService::state_address &&
            second < a::AudioService::state_address + a::AudioService::max_physical_voices * a::AudioService::state_stride) {
            const auto slot = (second - a::AudioService::state_address) / a::AudioService::state_stride;
            if (opcode == 1) pending.adpcm[slot] = true;
            if (opcode == 3) pending.envelope_flags[slot] = static_cast<std::uint8_t>(first >> 16);
            if (opcode == 5) {
                pending.resample[slot] = true;
                pending.resample_flags[slot] = static_cast<std::uint8_t>(first >> 16);
            }
        }
        if (opcode == 6 && second >= a::AudioService::pcm_address &&
            second < a::AudioService::pcm_address + a::AudioService::max_frames * 4u) {
            pending.begin = (second - a::AudioService::pcm_address) / 4u;
            pending.frames = bytes / 4u;
            result.push_back(pending);
            pending = PlannedSpan{};
        }
    }
    return result;
}
enum class RestartCase { ordinary, stopped, stolen, allocation_only };
void start_bearing_pull_plan(std::uint32_t due_offset, RestartCase kind) {
    Core core(2, 5);
    auto old = core.voice(), neighbor = core.voice(), incoming = core.voice();
    core.allocate(old, 5); core.allocate(neighbor, 100);
    core.start(old); core.start(neighbor); core.pull(160);
    const auto task_start = core.state().sample_clock;
    const auto allocation_due = kind == RestartCase::ordinary ? task_start + 320u : task_start + due_offset;
    const auto due = task_start + due_offset;
    if (kind == RestartCase::stolen) {
        core.allocate(incoming, 5, task_start);
    } else {
        if (kind == RestartCase::stopped)
            check(d::stop_voice(core.context(), core.state(), old, {due}) == a::Error::none,
                  "stop before free and scheduled start is staged at its actual parameter time");
        check(d::free_voice(core.context(), core.state(), old, {allocation_due}) == a::Error::none,
              "ordinary active source explicitly frees before reassignment");
        core.allocate(incoming, 0, allocation_due);
    }
    if (kind != RestartCase::allocation_only)
        core.start(incoming, kind == RestartCase::stolen ? task_start : due);
    core.context().write_address = a::AudioService::command_address;
    // A callback-sized subdivision must not redefine the160-frame outer grid.
    core.pull(64, 0, 528);
    core.pull(464, 64, 528);
    const auto spans = planned_spans(core.context());
    std::uint32_t old_bus_frames = 0, neighbor_frames = 0, saved_frames = 0;
    bool found_restart = false;
    const auto bearing_begin = ((due_offset - 1u) / 160u) * 160u;
    for (const auto& span : spans) {
        saved_frames += span.frames;
        const bool gap = kind == RestartCase::ordinary && span.begin >= bearing_begin && span.begin < due_offset;
        if (gap) {
            check(!span.adpcm[0] && !span.resample[0] && span.envelope_flags[0] == 0,
                  "start-bearing prefix emits no old-source ADPCM, resampling or envelope contribution");
        } else {
            check(span.resample[0] && span.envelope_flags[0] != 0,
                  "physical source contributes throughout every nonsuppressed output span");
            old_bus_frames += span.frames;
        }
        check(span.resample[1] && span.envelope_flags[1] == 8,
              "other physical voice continues through the omitted prefix on the shared bus");
        neighbor_frames += span.envelope_flags[1] != 0 ? span.frames : 0;
        if (kind != RestartCase::allocation_only && span.begin == due_offset) {
            found_restart = true;
            check(span.envelope_flags[0] == 9 &&
                  span.resample_flags[0] == (kind == RestartCase::ordinary ? 0 : 1),
                  "restart initializes ENV while DSP history initialization follows live versus stopped chain");
        }
    }
    const auto omitted = kind == RestartCase::ordinary ? due_offset - bearing_begin : 0u;
    check(saved_frames == 528 && neighbor_frames == 528 && old_bus_frames == 528 - omitted,
          "only the measured physical prefix is omitted, never host-muted or removed from the shared output");
    check(kind == RestartCase::allocation_only || found_restart,
          "first new contribution occurs exactly at its scheduled start, including an outer endpoint");
}
void start_bearing_outer_pull_regressions() {
    // Original-qualified Free->reallocate->Start offsets. q+80 and a start
    // first discovered by a later callback within the pull remain unmeasured.
    for (const auto due : {240u, 320u, 400u}) start_bearing_pull_plan(due, RestartCase::ordinary);
    start_bearing_pull_plan(240, RestartCase::stopped);
    start_bearing_pull_plan(320, RestartCase::stopped);
    start_bearing_pull_plan(512, RestartCase::stolen);
    start_bearing_pull_plan(240, RestartCase::allocation_only);
}
void stolen_tail_pending_controls_and_sticky_reuse() {
    Core core(1, 5);
    auto old = core.voice(), replacement = core.voice(), contender = core.voice();
    core.allocate(old); core.start(old);
    core.pull(160);
    auto& physical = core.state().physical[0];
    const auto old_source = physical.current.epoch;
    const auto transfer = core.allocate(replacement, 64, 160);
    check(same(transfer.displaced, old) && physical.owner == replacement.slot &&
          physical.current.epoch == old_source && physical.current.active && !physical.stealable,
          "active steal transfers owner immediately while keeping separate ineligible old tail");
    core.start(replacement, 160, 96);
    check(physical.pending.valid && physical.pending.due == 672, "replacement source waits512 frames after steal");
    check(d::pitch_voice(core.context(), core.state(), replacement, 0.5f, {160}) == a::Error::none &&
          d::pan_voice(core.context(), core.state(), replacement, 17, {160}) == a::Error::none &&
          d::gain_voice(core.context(), core.state(), replacement, {12000}, {32}, {160}) == a::Error::none,
          "pending controls share the binding's shifted start boundary");
    check(d::pitch_voice(core.context(), core.state(), replacement, 1.5f, {192}) == a::Error::none,
          "later raw control retains its own512-shifted post-start boundary");
    core.pull(352, 0, 512);
    check(physical.current.epoch == old_source && physical.current.pitch == 0x8000 &&
          physical.pending.parameters.pitch == 1.0f && physical.pending.parameters.pan == 64 &&
          physical.pending.parameters.gain.value == 24000 && physical.pending.attack.value == 96,
          "shifted pre-start controls do not run early or mutate the old kill tail");
    core.pull(160, 352, 512);
    check(d::advance_parameters(core.context(), core.state(), {672}) == a::Error::none,
          "restart boundary applies before another callback allocates");
    check(physical.current.active && physical.current.epoch != old_source && !physical.pending.valid &&
          physical.current.pitch == 0x4000 && physical.current.parameters.pan == 17 &&
          physical.current.parameters.gain.value == 12000,
          "replacement starts with the pending pitch, pan and gain parameters");
    core.pull(32);
    check(physical.current.pitch == 0x4000, "post-start control is not applied before exact parameter boundary");
    check(d::advance_parameters(core.context(), core.state(), {704}) == a::Error::none && physical.current.pitch == 0xc000,
          "post-start pitch applies at its own boundary");
    check(d::stop_voice(core.context(), core.state(), replacement, {704}) == a::Error::none,
          "stolen replacement can stop without freeing");
    core.start(replacement, 704);
    a::Allocation unavailable{};
    check(d::allocate_voice(core.context(), core.state(), contender, 32767, {704}, unavailable) == a::Error::voice_unavailable,
          "stolen-active binding stays ineligible beyond restart even for higher priority");
    check(d::stop_voice(core.context(), core.state(), replacement, {704}) == a::Error::none &&
          d::free_voice(core.context(), core.state(), replacement, {704}) == a::Error::none,
          "sticky stop and free unbind logical owner");
    check(physical.allocated && physical.owner == a::no_slot, "sticky free remains physically reserved");
    check(physical.current.active && physical.current.parameters.gain.value == 12000,
          "sticky stop/free keeps unchanged current source until effective raw time plus512, without a free-time kill");
    core.pull(496);
    check(d::allocate_voice(core.context(), core.state(), contender, 32767, {1200}, unavailable) == a::Error::voice_unavailable,
          "sticky retirement is not reusable one parameter tick early");
    core.pull(16);
    check(d::advance_parameters(core.context(), core.state(), {1216}) == a::Error::none && !physical.allocated,
          "sticky retirement becomes visible exactly at free plus512");
    core.allocate(contender, 64, 1216);
    check(physical.stealable, "fresh allocation after explicit retirement restores eligibility");
}
void sticky_controls_keep_binding_offset() {
    Core core(2, 5);
    auto old = core.voice(), ordinary = core.voice(), replacement = core.voice();
    core.allocate(old, 0); core.allocate(ordinary, 100);
    core.start(old); core.start(ordinary); core.pull(160);
    core.allocate(replacement, 0, 160);
    check(d::start_voice(core.context(), core.state(), replacement, core.wave(), {1.5f, {1098}, 127}, {0}, {160}) == a::Error::none,
          "stolen source has an explicit initial pitch and linear gain");
    core.pull(512);
    check(d::advance_parameters(core.context(), core.state(), {672}) == a::Error::none,
          "stolen source starts at the first shifted deadline");
    core.pull(160);
    check(d::pitch_voice(core.context(), core.state(), replacement, 0.5f, {832}) == a::Error::none &&
          d::pan_voice(core.context(), core.state(), replacement, 0, {832}) == a::Error::none &&
          d::gain_voice(core.context(), core.state(), replacement, {4394}, {32}, {832}) == a::Error::none &&
          d::pitch_voice(core.context(), core.state(), ordinary, 0.5f, {832}) == a::Error::none,
          "post-restart sticky and ordinary controls share a raw timestamp, not a global delay");
    core.pull(496);
    auto& sticky = core.state().physical[0].current;
    check(sticky.pitch == 0xc000 && sticky.parameters.gain.value == 1098 &&
          core.state().physical[1].current.pitch == 0x4000,
          "ordinary control applies immediately while stolen binding retains old control values for512 frames");
    core.pull(16);
    check(d::advance_parameters(core.context(), core.state(), {1344}) == a::Error::none &&
          sticky.pitch == 0x4000 && sticky.parameters.gain.value == 4394,
          "post-restart pitch, pan and gain retarget at raw timestamp plus512");
    check(d::stop_voice(core.context(), core.state(), replacement, {1344}) == a::Error::none &&
          d::free_voice(core.context(), core.state(), replacement, {1344}) == a::Error::none &&
          d::stop_voice(core.context(), core.state(), ordinary, {1344}) == a::Error::none,
          "sticky stop/free retains the same binding-local offset as later pitch and gain");
    core.context().write_address = a::AudioService::command_address;
    core.pull(512);
    std::array<bool, 2> repanned_from{};
    const std::array<std::uint16_t, 2> expected_from{1097, 1};
    for (auto address = a::AudioService::command_address; address < core.context().write_address; address += 8)
        for (std::size_t channel = 0; channel != repanned_from.size(); ++channel)
            if (core.context().memory[address / 4] ==
                    (0x09040000u | (channel == 0 ? 0x00020000u : 0u) | expected_from[channel]) &&
                core.context().memory[address / 4 + 1] == 0) repanned_from[channel] = true;
    check(repanned_from[0] && repanned_from[1],
          "live constant pan followed by gain emits the new-pan origin, not cached old-pan channels");
    std::uint32_t sticky_frames = 0, ordinary_frames = 0;
    for (const auto& span : planned_spans(core.context())) {
        sticky_frames += span.envelope_flags[0] != 0 ? span.frames : 0;
        ordinary_frames += span.envelope_flags[1] != 0 ? span.frames : 0;
    }
    check(sticky_frames == 512 && ordinary_frames == 0 && sticky.active,
          "only the stolen physical source contributes through the delayed stop tail");
    check(d::advance_parameters(core.context(), core.state(), {1856}) == a::Error::none && !sticky.active &&
          !core.state().physical[0].allocated,
          "sticky source stop and physical retirement meet exactly at raw stop/free plus512");
}
void event_epoch_and_order_guards() {
    Core core(1, 6);
    auto first = core.voice(), second = core.voice(), third = core.voice();
    core.allocate(first);
    core.start(first, 32);
    check(d::pan_voice(core.context(), core.state(), first, 11, {16}) == a::Error::none,
          "pending first start has pre-start pan");
    check(d::free_voice(core.context(), core.state(), first, {0}) == a::Error::none,
          "unstarted binding can free without retaining a phantom source");
    core.allocate(second); core.start(second);
    const auto source = core.state().physical[0].current.epoch;
    core.pull(48);
    check(core.state().physical[0].current.epoch == source && core.state().physical[0].current.parameters.pan == 64,
          "old pending start and controls cannot mutate a reused physical epoch");
    check(d::stop_voice(core.context(), core.state(), second, {48}) == a::Error::none, "source stop prepares immediate free");
    const auto old_binding = core.state().physical[0].epoch;
    check(d::free_voice(core.context(), core.state(), second, {48}) == a::Error::none, "stopped source frees");
    core.allocate(third, 64, 48); core.start(third, 48);
    // Model a delayed retirement already emitted by an older binding. This is
    // planned CPU event semantics, not execution of fabricated RSP output.
    auto& state = core.state();
    d::ParameterEvent stale{};
    stale.kind = d::ParameterKind::retire;
    stale.physical = 0; stale.epoch = old_binding; stale.due = 64;
    stale.sequence = ++state.next_parameter_sequence;
    check(state.parameter_count == 0, "old canceled events were drained");
    state.parameters[0] = stale; state.parameter_count = 1;
    core.pull(32);
    check(core.state().physical[0].owner == third.slot && core.state().physical[0].current.active,
          "stale retirement epoch never retires a newer owner");
    check(d::pan_voice(core.context(), core.state(), third, 20, {96}) == a::Error::none &&
          d::pan_voice(core.context(), core.state(), third, 30, {96}) == a::Error::none &&
          d::pan_voice(core.context(), core.state(), third, 10, {80}) == a::Error::none,
          "heap accepts reverse deadline insertion and equal-due sequence order");
    core.pull(32);
    check(core.state().physical[0].current.parameters.pan == 30,
          "heap processes earliest due first and preserves insertion order for equal due");
    core.start(third, 128);
    const auto canceled_source = core.state().physical[0].pending.epoch;
    core.start(third, 144);
    check(core.state().physical[0].pending.epoch != canceled_source,
          "a replacement start reserves a distinct source epoch on the same binding");
    core.pull(32);
    check(core.state().physical[0].current.epoch != canceled_source &&
          core.state().physical[0].pending.valid,
          "superseded pending start cannot initialize at its old deadline");
    check(d::advance_parameters(core.context(), core.state(), {144}) == a::Error::none &&
          !core.state().physical[0].pending.valid,
          "latest reserved source starts at its own boundary");
}
void capacity_failure_is_atomic_and_shadow_local() {
    Core core(1, 5);
    auto old = core.voice(), incoming = core.voice();
    core.allocate(old); core.start(old);
    for (std::size_t i = 0; i < a::AudioService::parameter_capacity; ++i)
        check(d::pitch_voice(core.context(), core.state(), old, 1.0f, {1024}) == a::Error::none,
              "bounded heap accepts capacity events");
    const auto sequence = core.state().next_parameter_sequence;
    const auto epoch = core.state().next_binding_epoch;
    const auto age = core.state().next_allocation_age;
    const auto write_address = core.context().write_address;
    a::Allocation output{{true, 9, 123}, {99, 88, 7}};
    check(d::allocate_voice(core.context(), core.state(), incoming, 64, {0}, output) == a::Error::event_capacity,
          "active steal atomically rejects insufficient kill and stop event capacity");
    check(core.state().logical[old.slot].physical == 0 && core.state().logical[incoming.slot].physical == a::no_slot &&
          core.state().physical[0].owner == old.slot && core.state().next_binding_epoch == epoch &&
          core.state().next_allocation_age == age && core.state().next_parameter_sequence == sequence &&
          output.binding.physical_slot == 9 && output.displaced.owner == 99,
          "capacity rollback preserves owners, output, counters, eligibility and allocation age");
    check(d::start_voice(core.context(), core.state(), old, core.wave(), {1.0f, {100}, 0}, {0}, {0}) == a::Error::event_capacity &&
          !core.state().physical[0].pending.valid,
          "failed start reserves no replacement source");
    check(core.context().write_address == write_address &&
          std::all_of(core.context().memory.get() + a::AudioService::command_address / 4,
                      core.context().memory.get() + a::AudioService::resource_address / 4,
                      [](std::uint32_t value) { return value == 0; }),
          "CPU controls and failed reservations never write published command, PCM or DSP RAM");
    auto& shadow = core.context().cpu[d::prepare_shadow(core.context())];
    shadow.physical[0].stealable = false;
    check(d::free_voice(core.context(), shadow, old, {0}) == a::Error::event_capacity &&
          shadow.logical[old.slot].physical == 0 && shadow.physical[0].owner == old.slot,
          "failed sticky deferred free does not unbind owner");
    d::prepare_shadow(core.context()); // Return to the ordinary binding before the isolation check.
    shadow.parameter_count = 0;
    check(d::stop_voice(core.context(), shadow, old, {0}) == a::Error::none &&
          !shadow.physical[0].current.active && core.state().physical[0].current.active,
          "internal controls operate on explicit transactional shadow, never implicit live state");
}
void identity_exhaustion_is_atomic() {
    Core core(1, 4);
    auto old = core.voice(), incoming = core.voice();
    core.allocate(old); core.start(old);
    core.state().next_parameter_sequence = std::numeric_limits<std::uint64_t>::max() - 1;
    a::Allocation output{};
    check(d::allocate_voice(core.context(), core.state(), incoming, 64, {0}, output) == a::Error::identity_exhausted &&
          core.state().physical[0].owner == old.slot && core.state().physical[0].stealable &&
          core.state().logical[incoming.slot].physical == a::no_slot,
          "multi-event steal preflights sequence overflow before any ownership mutation");
    core.state().next_parameter_sequence = std::numeric_limits<std::uint64_t>::max();
    check(d::start_voice(core.context(), core.state(), old, core.wave(), {1.0f, {100}, 0}, {0}, {0}) == a::Error::identity_exhausted &&
          !core.state().physical[0].pending.valid,
          "source epoch exhaustion never reserves a pending start");
    core.state().next_binding_epoch = std::numeric_limits<std::uint64_t>::max();
    check(d::allocate_voice(core.context(), core.state(), incoming, 64, {0}, output) == a::Error::identity_exhausted &&
          core.state().physical[0].owner == old.slot,
          "binding identity exhaustion never transfers ownership");
    core.context().next_voice = std::numeric_limits<std::uint64_t>::max();
    a::VoiceHandle untouched{123, 456, 7};
    check(d::create_voice(core.context(), core.state(), untouched) == a::Error::identity_exhausted &&
          untouched.owner == 123 && !core.state().logical[2].live,
          "logical reservation exhaustion leaves descriptor and result untouched");
}
void same_time_gain_then_pan_keeps_ramp_origin() {
    Core core(1, 4);
    const auto voice = core.voice();
    core.allocate(voice);
    check(d::start_voice(core.context(), core.state(), voice, core.wave(), {1.0f, {6758}, 10}, {0}, {0}) == a::Error::none &&
          d::gain_voice(core.context(), core.state(), voice, {2433}, {32}, {0}) == a::Error::none &&
          d::gain_voice(core.context(), core.state(), voice, {13247}, {32}, {0}) == a::Error::none &&
          d::pan_voice(core.context(), core.state(), voice, 0, {0}) == a::Error::none &&
          d::pan_voice(core.context(), core.state(), voice, 127, {0}) == a::Error::none,
          "sequential same-time gains precede repeated pans before the first source pull");
    core.pull(16);
    const auto& pan = core.context().coefficients.pan;
    const std::array<std::uint16_t, 2> expected_from{
        static_cast<std::uint16_t>((6758u * pan[10]) >> 15),
        static_cast<std::uint16_t>((6758u * pan[117]) >> 15)};
    std::array<bool, 2> found{};
    for (auto address = a::AudioService::command_address; address < core.context().write_address; address += 8)
        for (std::size_t channel = 0; channel != found.size(); ++channel)
            if (core.context().memory[address / 4] ==
                    (0x09040000u | (channel == 0 ? 0x00020000u : 0u) | expected_from[channel]) &&
                core.context().memory[address / 4 + 1] == 0) found[channel] = true;
    check(found[0] && found[1],
          "nonzero pending ramps retain the original stereo FROM operands across later pans");
}
void midramp_controls_preserve_signed_cpu_progress() {
    enum class Control { pan0, pan64, pan127, gain32, gain0, gain2471_32, gain2471_0, mute, fade3200, render512 };
    const auto exercise = [](std::initializer_list<Control> controls,
                             std::array<std::uint16_t, 2> expected_from,
                             std::array<std::uint16_t, 2> expected_to,
                             std::array<std::uint32_t, 2> expected_rate,
                             std::string_view meaning) {
        Core core(1, 4);
        // Authored asymmetric stereo coefficients isolate estimator feedback.
        core.context().coefficients.pan[64] = 23025;
        core.context().coefficients.pan[63] = 23313;
        const auto voice = core.voice();
        core.allocate(voice);
        check(d::start_voice(core.context(), core.state(), voice, core.wave(),
                            {1.0f, {122}, 64}, {0}, {0}) == a::Error::none,
              "low-amplitude source establishes the historical stereo origin");
        core.pull(16);
        check(d::gain_voice(core.context(), core.state(), voice, {4394}, {3200}, {16}) == a::Error::none,
              "long ramp establishes a nonterminal CPU estimator clock");
        for (unsigned i = 0; i != 16; ++i) core.pull(160);
        for (const auto control : controls) {
            const a::ParamTime due{core.state().sample_clock};
            auto error = a::Error::none;
            switch (control) {
            case Control::pan0: error = d::pan_voice(core.context(), core.state(), voice, 0, due); break;
            case Control::pan64: error = d::pan_voice(core.context(), core.state(), voice, 64, due); break;
            case Control::pan127: error = d::pan_voice(core.context(), core.state(), voice, 127, due); break;
            case Control::gain32: error = d::gain_voice(core.context(), core.state(), voice, {1098}, {32}, due); break;
            case Control::gain0: error = d::gain_voice(core.context(), core.state(), voice, {1098}, {0}, due); break;
            case Control::gain2471_32: error = d::gain_voice(core.context(), core.state(), voice, {2471}, {32}, due); break;
            case Control::gain2471_0: error = d::gain_voice(core.context(), core.state(), voice, {2471}, {0}, due); break;
            case Control::mute: error = d::gain_voice(core.context(), core.state(), voice, {0}, {0}, due); break;
            case Control::fade3200: error = d::gain_voice(core.context(), core.state(), voice, {0}, {3200}, due); break;
            case Control::render512: core.pull(512); continue;
            }
            check(error == a::Error::none, "scheduled control is admitted before the following ENV");
        }
        core.context().write_address = a::AudioService::command_address;
        core.pull(16);
        std::array<std::uint16_t, 2> actual_from{}, actual_to{};
        std::array<std::uint32_t, 2> actual_rate{};
        std::array<unsigned, 2> from_count{}, target_count{};
        for (auto address = a::AudioService::command_address; address < core.context().write_address; address += 8) {
            const auto word = core.context().memory[address / 4];
            const auto channel = (word & 0x00020000u) != 0 ? 0u : 1u;
            if ((word & 0xfffc0000u) == 0x09040000u) {
                actual_from[channel] = static_cast<std::uint16_t>(word);
                ++from_count[channel];
            } else if ((word & 0xfffc0000u) == 0x09000000u) {
                actual_to[channel] = static_cast<std::uint16_t>(word);
                actual_rate[channel] = core.context().memory[address / 4 + 1];
                ++target_count[channel];
            }
        }
        check(actual_from == expected_from && actual_to == expected_to && actual_rate == expected_rate &&
              from_count == std::array<unsigned, 2>{1, 1} && target_count == std::array<unsigned, 2>{1, 1}, meaning);
    };
    exercise({Control::pan127, Control::gain32}, {28449, 28779}, {0, 1097}, {0x13dc, 0x705b},
             "pan then gain advances twice using the retained historical rate and elapsed time");
    exercise({Control::gain32, Control::pan127}, {1555, 1573}, {0, 1097}, {0x28bb, 0xebb5},
             "gain resets the CPU clock before a subsequent same-time pan");
    exercise({Control::pan127, Control::pan0, Control::gain32}, {0xf12f, 0x08c5}, {1097, 0}, {0x5be44, 0x2521},
             "successive estimates wrap to signed16 while ENV preserves the raw negative FROM");
    exercise({Control::pan127, Control::pan0, Control::pan127, Control::gain32},
             {0xf0ed, 0xa071}, {0, 1097}, {0x103e8, 0x5be44},
             "a wrapped negative origin is sign-extended during the next CPU estimate");
    exercise({Control::pan64, Control::gain32}, {28449, 28779}, {771, 781}, {0x6728, 0x6728},
             "an unchanged pan still consumes an estimator transition");
    exercise({Control::gain2471_0, Control::gain32, Control::pan127},
             {1736, 1758}, {0, 1097}, {0x27b9, 0xe0b7},
             "a pending zero-duration gain supersedes the old ramp before later controls");
    exercise({Control::gain32, Control::gain2471_32}, {1555, 1573}, {1736, 1758}, {0x103e8, 0x103e8},
             "successive gains do not consume the historical elapsed time twice");
    exercise({Control::pan127, Control::pan0, Control::gain0}, {0xf12f, 0x08c5}, {1097, 0}, {0x7fffffff, 0},
             "zero-duration rate direction compares signed origins before logarithm normalization");
    exercise({Control::pan127, Control::pan0, Control::pan127, Control::gain0},
             {0xf0ed, 0xa071}, {0, 1097}, {0x7fffffff, 0x7fffffff},
             "a negative origin targeting zero retains the positive zero-duration rate");
    exercise({Control::pan127}, {1555, 1573}, {0, 4393}, {0xfb55, 0x100ad},
             "pan alone calculates rates over the full gain-authored duration, not its remainder");
    exercise({Control::pan127, Control::render512, Control::gain32},
             {1, 4328}, {0, 1097}, {0x103e8, 0xb7c7},
             "pan ENV installs new rates without resetting the CPU progress clock");
    exercise({Control::pan127, Control::render512, Control::render512, Control::gain32},
             {1, 4393}, {0, 1097}, {0x103e8, 0xb395},
             "the old nominal endpoint is reached while the new pan DSP ramp is still running");
    exercise({Control::pan127, Control::render512, Control::pan0, Control::render512, Control::gain32},
             {4393, 1}, {1097, 0}, {0xb395, 0x103e8},
             "successive rendered pans preserve the original gain clock and current endpoint");
    exercise({Control::render512, Control::render512, Control::pan127, Control::gain32},
             {1, 4393}, {0, 1097}, {0x103e8, 0xb395},
             "a completed nonzero-duration clock consumes a newly panned endpoint before ENV");
    exercise({Control::mute, Control::pan127}, {1, 1}, {0, 0}, {0, 0},
             "pan normalizes the preceding zero-duration silent CPU endpoint");
    exercise({Control::mute, Control::gain32}, {1, 1}, {771, 781}, {0x5517b, 0x5517b},
             "gain normalizes the preceding zero-duration silent CPU endpoint");
    exercise({Control::fade3200, Control::render512, Control::render512, Control::render512,
              Control::render512, Control::render512, Control::pan127, Control::pan0, Control::pan127},
             {1, 1}, {0, 0}, {0x10009, 0x10009},
             "midramp integer underflow normalizes zero after narrowing without signed saturation");
}
void static_zero_and_cpu_zero_have_distinct_origins() {
    Core core(1, 3);
    const auto voice = core.voice();
    core.allocate(voice);
    check(d::start_voice(core.context(), core.state(), voice, core.wave(),
                        {1.0f, {24000}, 0}, {0}, {0}) == a::Error::none,
          "static source starts with a silent right channel");
    const auto expect_right = [&](std::uint16_t from, std::uint32_t rate, std::string_view meaning) {
        bool found_from = false, found_target = false;
        for (auto address = a::AudioService::command_address; address < core.context().write_address; address += 8) {
            const auto word = core.context().memory[address / 4];
            const auto argument = core.context().memory[address / 4 + 1];
            found_from = found_from || (word == (0x09040000u | from) && argument == 0);
            found_target = found_target || (word == 0x09000000u && argument == rate);
        }
        check(found_from && found_target, meaning);
    };
    core.pull(16);
    expect_right(0, 0x7fffffff, "untouched static zero is emitted raw, without CPU-origin normalization");
    check(d::gain_voice(core.context(), core.state(), voice, {24000}, {32}, {16}) == a::Error::none,
          "an unchanged gain still authors a new ramp");
    core.context().write_address = a::AudioService::command_address;
    core.pull(16);
    expect_right(1, 0x103e8, "a control-produced zero endpoint is normalized before its positive-duration rate");

    Core attack(1, 3);
    const auto attacking = attack.voice();
    attack.allocate(attacking);
    check(d::start_voice(attack.context(), attack.state(), attacking, attack.wave(),
                        {1.0f, {122}, 64}, {160}, {0}) == a::Error::none &&
          d::pan_voice(attack.context(), attack.state(), attacking, 127, {0}) == a::Error::none &&
          d::gain_voice(attack.context(), attack.state(), attacking, {1098}, {32}, {0}) == a::Error::none,
          "initial attack receives pan and gain before its first ENV");
    attack.pull(16);
    std::array<bool, 2> initial{};
    for (auto address = a::AudioService::command_address; address < attack.context().write_address; address += 8) {
        const auto word = attack.context().memory[address / 4];
        initial[0] = initial[0] || word == 0x09060001u;
        initial[1] = initial[1] || word == 0x09040001u;
    }
    check(initial[0] && initial[1], "start establishes its nonzero CPU duration before any following control");
}
void pending_zero_duration_has_distinct_cpu_and_dsp_origins() {
    const auto expect_from = [](Core& core, std::uint16_t gain, std::uint8_t pan,
                                std::string_view meaning) {
        const auto& coefficients = core.context().coefficients.pan;
        const std::array<std::uint16_t, 2> expected{
            static_cast<std::uint16_t>((gain * coefficients[pan]) >> 15),
            static_cast<std::uint16_t>((gain * coefficients[127u - pan]) >> 15)};
        std::array<bool, 2> found{};
        for (auto address = a::AudioService::command_address; address < core.context().write_address; address += 8)
            for (std::size_t channel = 0; channel != found.size(); ++channel)
                if (core.context().memory[address / 4] ==
                        (0x09040000u | (channel == 0 ? 0x00020000u : 0u) | expected[channel]) &&
                    core.context().memory[address / 4 + 1] == 0) found[channel] = true;
        check(found[0] && found[1], meaning);
    };
    Core pan_only(1, 4);
    const auto voice = pan_only.voice();
    pan_only.allocate(voice);
    check(d::start_voice(pan_only.context(), pan_only.state(), voice, pan_only.wave(),
                        {1.0f, {122}, 64}, {0}, {0}) == a::Error::none,
          "constant source starts before the isolated pan");
    pan_only.pull(16);
    pan_only.context().write_address = a::AudioService::command_address;
    check(d::pan_voice(pan_only.context(), pan_only.state(), voice, 127, {16}) == a::Error::none,
          "pan-only transition has no following gain control");
    pan_only.pull(16);
    expect_from(pan_only, 122, 64, "pan alone preserves old DSP FROM, not the next-event CPU endpoint");

    Core gain_pair(1, 4);
    const auto paired = gain_pair.voice();
    gain_pair.allocate(paired);
    check(d::start_voice(gain_pair.context(), gain_pair.state(), paired, gain_pair.wave(),
                        {1.0f, {122}, 64}, {0}, {0}) == a::Error::none &&
          d::gain_voice(gain_pair.context(), gain_pair.state(), paired, {2471}, {0}, {0}) == a::Error::none &&
          d::gain_voice(gain_pair.context(), gain_pair.state(), paired, {1098}, {32}, {0}) == a::Error::none,
          "zero-duration gain precedes a nonzero gain ramp before the first ENV");
    gain_pair.pull(16);
    expect_from(gain_pair, 2471, 64, "following gain consumes the preceding zero-duration CPU endpoint");
}
void direct_gain_ramps_source_fences_and_identity() {
    Core core(1, 4);
    auto voice = core.voice();
    core.allocate(voice); core.start(voice);
    auto& current = core.state().physical[0].current;
    const auto expected = static_cast<std::uint16_t>((24000u * core.context().coefficients.pan[64]) >> 15);
    check(current.retarget_current[0] == expected,
          "zero attack uses unsquared direct amplitude at static target");
    core.pull(32);
    check(d::gain_voice(core.context(), core.state(), voice, {16000}, {160}, {32}) == a::Error::none,
          "explicit gain ramp targets linear amplitude");
    core.pull(48);
    core.start(voice, 80, 96);
    check(current.retarget_current == std::array<std::uint16_t, 2>{1, 1}, "nonzero attack initializes both channels from1");
    a::BindingInfo untouched{true, 12, 90};
    auto foreign = voice; ++foreign.owner;
    check(d::voice_binding(core.context(), core.state(), foreign, untouched) == a::Error::stale_handle &&
          untouched.physical_slot == 12, "foreign handle never exposes or alters local binding");
    check(d::pitch_voice(core.context(), core.state(), voice, 0.0f, {80}) == a::Error::invalid_parameter &&
          d::pan_voice(core.context(), core.state(), voice, 128, {80}) == a::Error::invalid_parameter &&
          d::gain_voice(core.context(), core.state(), voice, {32768}, {0}, {80}) == a::Error::invalid_parameter &&
          d::stop_voice(core.context(), core.state(), voice, {79}) == a::Error::invalid_parameter,
          "direct controls validate pitch, pan, gain and aligned non-backdated time");
    check(d::duration_frames(1000, 32006) == 32 && d::duration_frames(16000, 32006) == 512,
          "qualified CPU duration math retains frame quantization");
    std::uint16_t pitch = 0;
    check(d::pitch_word(1.0f, pitch) && pitch == 0x8000 && d::pitch_word(0.5f, pitch) && pitch == 0x4000,
          "qualified pitch math retains unsigned Q1.15 values");
}
void pending_gain_starts_from_original_linear_gain() {
    Core core(1, 4);
    auto old = core.voice(), replacement = core.voice();
    core.allocate(old); core.start(old);
    core.allocate(replacement);
    check(d::start_voice(core.context(), core.state(), replacement, core.wave(), {1.0f, {122}, 64}, {0}, {0}) == a::Error::none &&
          d::pan_voice(core.context(), core.state(), replacement, 127, {0}) == a::Error::none &&
          d::pitch_voice(core.context(), core.state(), replacement, 1.5f, {0}) == a::Error::none &&
          d::gain_voice(core.context(), core.state(), replacement, {1098}, {32}, {0}) == a::Error::none,
          "deferred replacement accepts zero-attack start followed by pan, pitch and explicit gain ramp");
    core.pull(512);
    check(d::advance_parameters(core.context(), core.state(), {512}) == a::Error::none,
          "pending mixed controls initialize at deferred restart");
    core.context().write_address = a::AudioService::command_address;
    core.pull(16);
    std::array<bool, 2> from{};
    for (auto address = a::AudioService::command_address; address < core.context().write_address; address += 8) {
        const auto word = core.context().memory[address / 4];
        from[0] = from[0] || (word == 0x09060001u && core.context().memory[address / 4 + 1] == 0);
        from[1] = from[1] || (word == 0x09040079u && core.context().memory[address / 4 + 1] == 0);
    }
    check(from[0] && from[1],
          "folded gain emits the normalized original source origin only after its512-frame replacement wait");
}
void finite_outer_grid_and_eof_cache() {
    Core core(1, 3);
    auto voice = core.voice();
    core.allocate(voice); core.start(voice);
    auto& current = core.state().physical[0].current;
    current.loops_remaining = 1;
    core.pull(64, 0, 176);
    check(current.loops_remaining == 1 && current.deferred_loops == 1,
          "finite repeat consumption defers across callback subdivision before160 fence");
    core.pull(96, 64, 176);
    check(current.loops_remaining == 0 && current.deferred_loops == 0 && current.source_finished,
          "finite wraps saturate consumption once at outer160 fence");
    core.pull(16, 160, 176);
    check(current.cursor == 32 && current.source_finished && core.state().sample_clock == 176,
          "EOF discards decoded cache at next outer pull and final short pull advances once");
    Core short_task(1, 3);
    auto short_voice = short_task.voice(); short_task.allocate(short_voice); short_task.start(short_voice);
    auto& short_current = short_task.state().physical[0].current;
    short_current.loops_remaining = 1;
    short_task.pull(48);
    check(short_current.loops_remaining == 0 && short_current.deferred_loops == 0,
          "last short outer task pull consumes finite wraps exactly once");
}
void cent_ratios_preserve_binary32_steps() {
    check(std::bit_cast<std::uint32_t>(d::cents_ratio(1200)) == 0x400000fau &&
          std::bit_cast<std::uint32_t>(d::cents_ratio(-1200)) == 0x3f00007du,
          "octave pitch ratios preserve distinct rounded up/down unit-cent powers");
    check(d::cents_ratio(std::numeric_limits<std::int32_t>::min()) == 0.0f,
          "negative cent magnitude includes INT32_MIN without signed overflow");
    check(std::isinf(d::cents_ratio(std::numeric_limits<std::int32_t>::max())),
          "positive cent power overflows as floating point rather than integer arithmetic");
}
void public_guards_and_live_lease_controls() {
    isle::audio_test::Fixture fixture;
    auto decoded = fixture.decode();
    const std::array owners{decoded.asset};
    auto created = a::AudioService::create(owners, {32006, 1, 3, {448, 512}}, isle::audio_test::coefficients(),
                                         {a::effects::Routing::direct});
    check(static_cast<bool>(created), "public shared service creates");
    if (!created) return;
    auto& service = *created.service;
    auto synth = service.synth();
    a::VoiceHandle voice{};
    a::Allocation allocation{};
    a::SoundId sound{}; a::WaveId wave{};
    check(service.sound_id(isle::audio_test::selected(decoded.asset), sound) == a::Error::none &&
          service.wave_id(sound, wave) == a::Error::none && synth.create_voice(voice) == a::Error::none &&
          synth.allocate(voice, 64, {0}, allocation) == a::Error::none &&
          synth.start(voice, wave, {1.0f, {20000}, 64}, {0}, {0}) == a::Error::none,
          "public Synth creates, binds and starts a catalog source");
    a::TaskView task{};
    check(service.build({1, 0, 32}, task) == a::Error::none, "public task predicts32-frame frontier");
    check(synth.control_frontier().value == 32 && synth.set_pan(voice, 20, {16}) == a::Error::invalid_parameter,
          "live lease cannot backdate controls into published commands");
    const auto words = service.rdram_words();
    const auto before = words[a::AudioService::state_address / 4];
    check(synth.set_pan(voice, 20, {32}) == a::Error::none &&
          words[a::AudioService::state_address / 4] == before,
          "live task staging modifies predicted CPU only, not published DSP state");
    a::PcmView pcm{};
    check(service.complete(task.token, false, pcm) == a::Error::execution_failed,
          "reported execution failure poisons shared DSP without pretending to execute PCM");
    check(synth.set_priority(voice, 1) == a::Error::needs_reset && synth.create_voice(voice) == a::Error::needs_reset,
          "public wrappers reject poisoned context");
}
} // namespace
int main() {
    cent_ratios_preserve_binary32_steps();
    descriptor_and_global_allocator();
    allocation_age_and_stop_binding();
    inactive_displacement_exclusion();
    free_retiring_and_collection_order();
    ordinary_active_free_is_immediately_reclaimable();
    ordinary_free_tail_and_stopped_reuse();
    start_bearing_outer_pull_regressions();
    stolen_tail_pending_controls_and_sticky_reuse();
    sticky_controls_keep_binding_offset();
    event_epoch_and_order_guards();
    capacity_failure_is_atomic_and_shadow_local();
    identity_exhaustion_is_atomic();
    same_time_gain_then_pan_keeps_ramp_origin();
    midramp_controls_preserve_signed_cpu_progress();
    static_zero_and_cpu_zero_have_distinct_origins();
    pending_zero_duration_has_distinct_cpu_and_dsp_origins();
    direct_gain_ramps_source_fences_and_identity();
    pending_gain_starts_from_original_linear_gain();
    finite_outer_grid_and_eof_cache();
    public_guards_and_live_lease_controls();
    if (failures) return EXIT_FAILURE;
    std::cout << "Shared synth allocator, parameter, source and CPU-plan checks passed\n";
    return EXIT_SUCCESS;
}

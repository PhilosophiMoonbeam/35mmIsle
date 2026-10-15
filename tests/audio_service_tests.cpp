#include "audio_fixture.hpp"
#include "35mmisle/audio_sfx.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace a = isle::audio;
namespace s = isle::audio::sfx;
namespace f = isle::audio_test;
namespace {
int failures = 0;
void check(bool value, std::string_view message) {
    if (!value) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
a::ServiceResult make(const a::BankFile& bank, std::uint16_t physical = 22) {
    const std::array owners{bank};
    a::SynthConfig config{32006};
    config.physical_voices = physical;
    return a::AudioService::create(owners, config, f::coefficients());
}
a::SoundId sound(a::AudioService& service, const a::BankFile& bank) {
    a::SoundId id;
    check(service.sound_id(f::selected(bank), id) == a::Error::none, "authored sound is retained");
    return id;
}
a::PlayerId player(a::AudioService& service, std::uint64_t due, std::uint16_t capacity = 1) {
    s::Config config;
    config.max_sounds = capacity;
    config.first_due = a::RawSampleTime{due};
    config.first_client_us = 0;
    a::PlayerId id;
    check(service.add_sound_player(config, id) == a::Error::none, "bounded client registration");
    return id;
}
// These acknowledgements test the CPU ownership protocol only. No RSP is run,
// and these tests never claim synthesized PCM; private actual-RSP gates do that.
void acknowledge(a::AudioService& service, const a::TaskView& task) {
    a::PcmView pcm;
    check(service.complete(task.token, true, pcm) == a::Error::none, "matching terminal acknowledgement");
    check(service.release(pcm.token) == a::Error::none, "explicit PCM ownership retirement");
}
void independent_tokens_and_staging() {
    f::Fixture fixture;
    auto bank = fixture.decode();
    if (!bank) { check(false, "wire fixture hydrates"); return; }
    auto first = make(bank.asset), second = make(bank.asset);
    if (!first || !second) { check(false, "independent services create"); return; }
    auto& service = *first.service;
    auto client = service.player(player(service, 0));
    s::SoundHandle playback;
    check(client.stage_start(sound(service, bank.asset), {}, playback) == a::Error::none &&
          client.stage() == a::Error::none, "initial logical playback publishes");
    a::TaskView task, foreign;
    check(service.build({1, 0, 16}, task) == a::Error::none &&
          second.service->build({1, 0, 16}, foreign) == a::Error::none, "both services publish first tasks");
    check(task.token.serial == foreign.token.serial && task.token.owner != foreign.token.owner,
          "same local serial never identifies a foreign service task");
    a::PcmView pcm;
    check(service.complete(foreign.token, true, pcm) == a::Error::stale_task && service.has_lease(),
          "foreign acknowledgement cannot retire the executing task");
    check(client.stage_stop(playback) == a::Error::none && client.stage() == a::Error::none,
          "task-live client controls are accepted for future callbacks");
    check(service.complete(task.token, true, pcm) == a::Error::none,
          "terminal completion grants the same graph-wide PCM token");
    check(service.reset() == a::Error::task_busy, "completed PCM lease still fences reset");
    std::array<std::int16_t, 33> guarded;
    guarded.fill(113);
    const auto untouched = guarded;
    auto forged = pcm;
    forged.token.owner = foreign.token.owner;
    check(service.copy_pcm(forged, guarded) == a::Error::stale_task && guarded == untouched,
          "foreign PCM cannot overwrite the consumer destination");
    check(service.copy_pcm(pcm, std::span(guarded).first(31)) == a::Error::invalid_parameter &&
          guarded == untouched && service.has_lease(), "short extraction is atomic and retains the lease");
    check(service.release(pcm.token) == a::Error::none, "consumer explicitly releases PCM");
    a::TaskView after;
    check(service.build({2, 0, 1024}, after) == a::Error::none && !client.active(playback),
          "completion did not overwrite task-live staged stop and release deadline");
    acknowledge(service, after);
    const auto* storage = service.rdram_words().data();
    check(service.reset() == a::Error::none && service.rdram_words().data() == storage,
          "reset preserves the borrowed allocation");
    check(client.stage_stop(playback) == a::Error::stale_handle,
          "reset invalidates old playback and registration capabilities");
    a::TaskView replay;
    check(service.build({1, 0, 16}, replay) == a::Error::none &&
          replay.token.owner == task.token.owner && replay.token.serial > after.token.serial,
          "reset preserves owner identity and monotonic graph serials");
    check(service.release(task.token) == a::Error::stale_task && service.has_lease(),
          "old release cannot retire a new executing task");
    acknowledge(service, replay);
    acknowledge(*second.service, foreign);
}
void callback_boundary(std::uint64_t due, bool in_first_task) {
    f::Fixture fixture;
    auto bank = fixture.decode();
    if (!bank) { check(false, "deadline fixture hydrates"); return; }
    auto made = make(bank.asset);
    if (!made) { check(false, "deadline service creates"); return; }
    auto& service = *made.service;
    auto client = service.player(player(service, due));
    s::SoundHandle handle;
    check(client.stage_start(sound(service, bank.asset), {}, handle) == a::Error::none &&
          client.stage() == a::Error::none, "deadline playback queues");
    a::TaskView first;
    check(service.build({1, 0, 64}, first) == a::Error::none, "deadline first task publishes");
    s::Status status;
    check(client.status(handle, status) == a::Error::none && status.bound == in_first_task,
          "raw callback deadline, not DSP floor, decides task admission");
    acknowledge(service, first);
    if (!in_first_task) {
        a::TaskView next;
        check(service.build({2, 0, 16}, next) == a::Error::none &&
              client.status(handle, status) == a::Error::none && status.bound,
              "exact-end and end-plus-one callbacks dispatch in the following task");
        acknowledge(service, next);
    }
}
void shared_raw_order(std::uint64_t first_due, std::uint64_t second_due, bool first_wins) {
    f::Fixture fixture;
    auto bank = fixture.decode();
    if (!bank) { check(false, "ordering fixture hydrates"); return; }
    auto made = make(bank.asset, 1);
    if (!made) { check(false, "ordering service creates"); return; }
    auto& service = *made.service;
    const auto selected = sound(service, bank.asset);
    auto first = service.player(player(service, first_due));
    auto second = service.player(player(service, second_due));
    s::SoundHandle one, two;
    check(first.stage_start(selected, {}, one) == a::Error::none && first.stage() == a::Error::none &&
          second.stage_start(selected, {}, two) == a::Error::none && second.stage() == a::Error::none,
          "two actual client contexts publish independently");
    a::TaskView task;
    check(service.build({1, 0, 64}, task) == a::Error::none, "shared client task publishes");
    s::Status a_status, b_status;
    check(first.status(one, a_status) == a::Error::none && second.status(two, b_status) == a::Error::none &&
          a_status.bound == first_wins && b_status.bound != first_wins,
          "raw order precedes player-head ties, independent of a shared DSP floor");
    acknowledge(service, task);
}
void rephase_client_deadline() {
    f::Fixture fixture;
    auto bank = fixture.decode();
    if (!bank) { check(false, "rephasing fixture hydrates"); return; }
    auto made = make(bank.asset);
    if (!made) { check(false, "rephasing service creates"); return; }
    auto& service = *made.service;
    const auto id = player(service, 512);
    auto client = service.player(id);
    s::SoundHandle handle;
    check(client.stage_start(sound(service, bank.asset), {}, handle) == a::Error::none &&
          client.stage() == a::Error::none, "playback waits on its original future callback");
    a::TaskView first;
    check(service.build({1, 0, 64}, first) == a::Error::none,
          "clock advances before client rephasing");
    check(service.set_player_deadline(id, {64}) == a::Error::task_busy,
          "executing ownership fences raw deadline changes");
    acknowledge(service, first);
    auto foreign = id;
    ++foreign.owner;
    auto stale = id;
    ++stale.generation;
    check(service.set_player_deadline(foreign, {64}) == a::Error::stale_handle &&
          service.set_player_deadline(stale, {64}) == a::Error::stale_handle,
          "foreign and superseded registration capabilities cannot rephase a client");
    check(service.set_player_deadline(id, {128}) == a::Error::none &&
          service.set_player_deadline(id, {63}) == a::Error::invalid_parameter,
          "rephasing rejects a raw deadline behind the committed clock");
    a::TaskView second;
    s::Status status;
    check(service.build({2, 0, 64}, second) == a::Error::none &&
          client.status(handle, status) == a::Error::none && !status.bound,
          "rejected backdating preserves exact-end callback deferral");
    acknowledge(service, second);
    check(service.set_player_deadline(id, {129}) == a::Error::none,
          "raw end-plus-one remains a representable deadline");
    a::TaskView third;
    check(service.build({3, 0, 16}, third) == a::Error::none &&
          client.status(handle, status) == a::Error::none && status.bound,
          "rephased callback floors only after raw task admission and consumes its preserved start");
    acknowledge(service, third);
    check(service.reset() == a::Error::none &&
          service.set_player_deadline(id, {144}) == a::Error::stale_handle,
          "reset registration cannot be revived by deadline rephasing");
}
void command_failure_rolls_back_clients() {
    f::Fixture fixture;
    f::put32(*fixture.bank, 128, 1); // Authored one-sample infinite loop stresses command capacity.
    auto bank = fixture.decode();
    if (!bank) { check(false, "short-loop fixture hydrates"); return; }
    auto made = make(bank.asset, 2);
    if (!made) { check(false, "rollback service creates"); return; }
    auto& service = *made.service;
    auto client = service.player(player(service, 0, 2));
    const auto selected = sound(service, bank.asset);
    s::Parameters parameters;
    parameters.pitch = 65535.0f / 32768.0f;
    s::SoundHandle one, two;
    check(client.stage_start(selected, parameters, one) == a::Error::none &&
          client.stage_start(selected, parameters, two) == a::Error::none && client.stage() == a::Error::none,
          "two short-loop client starts publish atomically");
    a::TaskView failed;
    check(service.build({1, 0, 1024}, failed) == a::Error::command_capacity &&
          failed.token.owner == 0 && !service.has_lease(), "exhausted command arena publishes no task or lease");
    s::Status first_status, second_status;
    check(service.synth().control_frontier().value == 0 &&
          client.status(one, first_status) == a::Error::none && !first_status.bound &&
          client.status(two, second_status) == a::Error::none && !second_status.bound,
          "failed speculative callbacks advance no clock or physical binding");
    check(client.stage_pitch(one, 1.0f) == a::Error::none && client.stage_pitch(two, 1.0f) == a::Error::none &&
          client.stage() == a::Error::none, "accepted playback identities and controls survive failed build");
    a::TaskView recovered;
    check(service.build({1, 0, 16}, recovered) == a::Error::none && recovered.token.serial == 1 &&
          client.status(one, first_status) == a::Error::none && first_status.bound &&
          client.status(two, second_status) == a::Error::none && second_status.bound,
          "smaller retry consumes preserved starts, original service id and first task serial");
    acknowledge(service, recovered);
}
}
int main() {
    independent_tokens_and_staging();
    callback_boundary(63, true);
    callback_boundary(64, false);
    callback_boundary(65, false);
    shared_raw_order(32, 32, true); // Newest head B runs first; equal-priority A then steals it.
    shared_raw_order(32, 33, false); // A earlier raw due wins selection, B later steals despite same floor.
    rephase_client_deadline();
    command_failure_rolls_back_clients();
    if (failures) return EXIT_FAILURE;
    std::cout << "Shared audio ownership, raw calendar and transactional rollback checks passed\n";
    return EXIT_SUCCESS;
}

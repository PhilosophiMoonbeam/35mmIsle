#include "application_manager.hpp"

#include <cstdio>
#include <exception>
#include <limits>
#include <new>
#include <thread>
#include <utility>

namespace isle::application {
namespace {
IsleGameResult result(IsleContinuationResult value) noexcept {
    switch (value) {
    case ISLE_CONT_OK: return ISLE_GAME_OK;
    case ISLE_CONT_INVALID_ARGUMENT: return ISLE_GAME_INVALID_ARGUMENT;
    case ISLE_CONT_WRONG_OWNER: return ISLE_GAME_WRONG_OWNER;
    case ISLE_CONT_INVALID_STATE: return ISLE_GAME_INVALID_STATE;
    case ISLE_CONT_RESOURCE_FAILURE: return ISLE_GAME_EXHAUSTED;
    case ISLE_CONT_EXECUTION_FAILURE: return ISLE_GAME_EXECUTION_FAILURE;
    }
    return ISLE_GAME_EXECUTION_FAILURE;
}
IsleGameResult result(audio::Error value) noexcept {
    switch (value) {
    case audio::Error::none: return ISLE_GAME_OK;
    case audio::Error::invalid_parameter: return ISLE_GAME_INVALID_ARGUMENT;
    case audio::Error::invalid_state: return ISLE_GAME_INVALID_STATE;
    case audio::Error::allocation_failure: return ISLE_GAME_EXHAUSTED;
    default: return ISLE_GAME_EXECUTION_FAILURE;
    }
}
void text(std::array<char, 512>& target, const char* message) noexcept {
    std::snprintf(target.data(), target.size(), "%s", message ? message : "application service failed");
}
struct Mapping { std::size_t first, end; std::uint32_t guest; };
// Numeric ROM/VRAM facts from the verified df8815e75f3d79618466e01e0827c6165b93d11c
// splat.yaml blob d2690bd3d2cb0aeac5c648db20666c5a42e5ba45. These are bounded
// immutable data mappings, not SDK Overlay records or executable guest code.
constexpr std::array<Mapping, 30> overlay_mappings{{
    {0xaa18e0,0xaaa610,0x801dc8a0}, {0x83d730,0x87a0b0,0x80369f80},
    {0x87a0b0,0x8a70e0,0x801dc8c0}, {0x8a70e0,0x98c330,0x800e18c0},
    {0x9a6b10,0x9d3230,0x801dc8c0}, {0x9d3230,0x9fa580,0x801dc8c0},
    {0x98c330,0x9a6b10,0x801dc8c0}, {0x9fa580,0xa084b0,0x801dc8c0},
    {0xa084b0,0xa08e30,0x801dc8c0}, {0x05bf20,0x05f050,0x800bf080},
    {0xa08e30,0xa0f830,0x800e18a0}, {0xa5cc50,0xa93460,0x800e18a0},
    {0xa93460,0xaa0650,0x801dc8a0}, {0xaa0650,0xaa0b80,0x800e18a0},
    {0x4f0610,0x54b5d0,0x80350200}, {0x05f050,0x0731b0,0x800e18a0},
    {0x0a74e0,0x13c780,0x8011b050}, {0x55c110,0x5df5d0,0x802c40a0},
    {0x162cb0,0x1d1d90,0x8011cb50}, {0x5df5d0,0x6401b0,0x802e2500},
    {0x1f5e70,0x27ab80,0x8012a0c0}, {0x6401b0,0x6c05e0,0x802bdd00},
    {0x29a190,0x30af90,0x8012a0c0}, {0x6c05e0,0x7272e0,0x802d8800},
    {0x326c10,0x3d0560,0x800fff90}, {0x7272e0,0x79f1b0,0x802d60e0},
    {0x3f63d0,0x47cf30,0x800fff90}, {0x79f1b0,0x825e30,0x802c5c20},
    {0x4a8160,0x4ec000,0x800f5d90}, {0x825e30,0x82f8e0,0x803466c0}
}};
constexpr std::array<Mapping, 19> rom_mappings{{
    {0x13c780,0x162cb0,0x801b0310}, {0x0731b0,0x07d3b0,0x800f5d90},
    {0x82f8e0,0x832960,0x803b1f80}, {0x07d3b0,0x098470,0x800fff90},
    {0x832960,0x837360,0x803ad580}, {0x1d1d90,0x1f5e70,0x8018bc50},
    {0x098470,0x099f70,0x8011b050}, {0x837360,0x83a1e0,0x803aa700},
    {0x27ab80,0x29a190,0x801aedf0}, {0x099f70,0x0a74e0,0x8011cb50},
    {0x83a1e0,0x83d730,0x803a71b0}, {0x30af90,0x326c10,0x8019aee0},
    {0x3d0560,0x3f63d0,0x801a9900}, {0x47cf30,0x4a8160,0x80186b10},
    {0x4ec000,0x4f0610,0x80139c50}, {0x54b5d0,0x54d6a0,0x8034e130},
    {0x54d6a0,0x554130,0x803476a0}, {0x554130,0x557050,0x80344780},
    {0x557050,0x55c110,0x8033f6c0}
}};
rom::GuestView mapped(const rom::Snapshot& snapshot, Mapping map) noexcept {
    const auto bytes = snapshot.canonical();
    if (map.first >= map.end || map.end > bytes.size()) return {};
    return {map.guest, map.first, bytes.subspan(map.first, map.end - map.first)};
}
bool byte_argument(std::int64_t value) noexcept { return value >= 0 && value <= 255; }
bool signed_byte_argument(std::int64_t value) noexcept { return value >= -128 && value <= 127; }
}

Resources::Resources(const rom::Snapshot& snapshot, const menu::Catalog& catalog) noexcept
    : snapshot_(snapshot), catalog_(catalog), binding_(snapshot.canonical().data()) {}
bool Resources::live() const noexcept {
    if (!binding_ || snapshot_.canonical().size() != rom::us_size ||
        snapshot_.canonical().data() != binding_) return false;
    try {
        // A Catalog from a different (even identically admitted) allocation is
        // not this owner's residency. Its own live check also rejects a move.
        const auto& logo = catalog_.setup(menu::Scene::logo).wire;
        const auto& title = catalog_.setup(menu::Scene::title_menu).wire;
        return logo.rom_offset == 0xaa0af0 && title.rom_offset == 0xa0f718 &&
            logo.bytes.data() == binding_ + logo.rom_offset &&
            title.bytes.data() == binding_ + title.rom_offset;
    } catch (...) { return false; }
}
rom::GuestView Resources::overlay(IsleApplicationOverlay id) const noexcept {
    const auto index = static_cast<unsigned>(id);
    return live() && index < overlays_.size() ? overlays_[index] : rom::GuestView{};
}
std::uint64_t Resources::overlay_generation(IsleApplicationOverlay id) const noexcept {
    const auto index = static_cast<unsigned>(id);
    return live() && index < overlay_generations_.size() ? overlay_generations_[index] : 0;
}
rom::GuestView Resources::rom(IsleApplicationRom id) const noexcept {
    const auto index = static_cast<unsigned>(id);
    return live() && index < roms_.size() ? roms_[index] : rom::GuestView{};
}
rom::GuestView Resources::vpk(IsleApplicationVpk id) const noexcept {
    const auto index = static_cast<unsigned>(id);
    return live() && index < vpks_.size() ? vpks_[index] : rom::GuestView{};
}
IsleGameResult Resources::retain(IsleApplicationOverlay id) noexcept {
    const auto index = static_cast<unsigned>(id);
    if (index >= overlays_.size()) return ISLE_GAME_INVALID_ARGUMENT;
    if (!live()) return ISLE_GAME_INVALID_STATE;
    if (overlay_generations_[index] == std::numeric_limits<std::uint64_t>::max())
        return ISLE_GAME_EXHAUSTED;
    const auto view = mapped(snapshot_, overlay_mappings[index]);
    if (view.bytes.empty()) return ISLE_GAME_EXECUTION_FAILURE;
    overlays_[index] = view;
    ++overlay_generations_[index];
    return ISLE_GAME_OK;
}
IsleGameResult Resources::retain(IsleApplicationRom id) noexcept {
    const auto index = static_cast<unsigned>(id);
    if (index >= roms_.size()) return ISLE_GAME_INVALID_ARGUMENT;
    if (!live()) return ISLE_GAME_INVALID_STATE;
    const auto view = mapped(snapshot_, rom_mappings[index]);
    if (view.bytes.empty()) return ISLE_GAME_EXECUTION_FAILURE;
    roms_[index] = view;
    return ISLE_GAME_OK;
}
IsleGameResult Resources::retain(IsleApplicationVpk id) noexcept {
    const auto index = static_cast<unsigned>(id);
    if (index >= vpks_.size()) return ISLE_GAME_INVALID_ARGUMENT;
    if (!live()) return ISLE_GAME_INVALID_STATE;
    try {
        const auto& bank = catalog_.bank(index == 0 ? menu::Scene::logo : menu::Scene::title_menu);
        const std::size_t first = index == 0 ? 0xaa0b80 : 0xa0f830;
        const std::size_t end = index == 0 ? 0xaa18d3 : 0xa5cc46;
        const std::size_t decoded_size = index == 0 ? 0x7110 : 0xf3770;
        if (!bank.owner || bank.rom_begin != first || bank.rom_end != end ||
            bank.compressed_bytes_consumed != end - first || bank.owner->size() != decoded_size ||
            bank.guest_begin != menu::bank_guest) return ISLE_GAME_EXECUTION_FAILURE;
        // Catalog's checked vpk0::decode already owns the complete output. A
        // shared immutable owner retains it without decoding/copying it again.
        banks_[index] = bank.owner;
        vpks_[index] = {bank.guest_begin, menu::decompressed_rom_offset,
                       std::as_bytes(std::span(*banks_[index]))};
        return ISLE_GAME_OK;
    } catch (const std::bad_alloc&) { return ISLE_GAME_EXHAUSTED; }
    catch (...) { return ISLE_GAME_EXECUTION_FAILURE; }
}

struct Manager::Impl {
    game::State& state;
    game_input::Service& input;
    game_audio::Session& audio;
    host::GtlClock& clock;
    Resources resources;
    const std::thread::id owner = std::this_thread::get_id();
    IsleContinuationDomain* domain = nullptr;
    IsleContinuation* continuation = nullptr;
    IsleApplicationSource source{};
    EntryIntent entry{};
    ApplicationEndState end{};
    std::array<char, 512> diagnostic{};
    std::uint64_t serial = 0, released_serial = 0;
    std::int32_t entry_result = 0;
    std::int32_t level = -1; // pinned app_render/47330.c:gLevelID initializer
    IsleGameResult failure = ISLE_GAME_OK;
    bool outstanding = false, completed = false, borrowed = false;
    bool resuming = false, cancelled = false, retired = false;

    Impl(const rom::Snapshot& snapshot, game::State& s, game_input::Service& i,
         game_audio::Session& a, host::GtlClock& c, const menu::Catalog& catalog) noexcept
        : state(s), input(i), audio(a), clock(c), resources(snapshot, catalog) {}
    ~Impl() {
        // Construction failure also follows the same join-before-link-release
        // order. A live native scene borrow is a fatal ownership contract error.
        if (owner != std::this_thread::get_id() || borrowed) std::terminate();
        if (continuation) {
            if (isle_continuation_cancel(continuation) != ISLE_CONT_OK ||
                isle_continuation_destroy(continuation) != ISLE_CONT_OK) std::terminate();
        }
        if (domain && isle_continuation_domain_destroy(domain) != ISLE_CONT_OK) std::terminate();
    }
    bool on_owner() const noexcept { return owner == std::this_thread::get_id(); }
    IsleGameResult fail(IsleGameResult error, const char* reason) noexcept {
        if (failure == ISLE_GAME_OK) { failure = error; text(diagnostic, reason); }
        return error;
    }
};

Manager::Manager(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}
Manager::~Manager() = default;
CreateResult Manager::create(const rom::Snapshot& snapshot, game::State& state,
    game_input::Service& input, game_audio::Session& audio, host::GtlClock& clock,
    const menu::Catalog& catalog, std::int32_t argument) noexcept {
    CreateResult made;
    try {
        auto impl = std::make_unique<Impl>(snapshot, state, input, audio, clock, catalog);
        if (!impl->resources.live()) {
            made.error = ISLE_GAME_INVALID_STATE;
            text(made.message, "application resources require the same live admitted Snapshot and Catalog");
            return made;
        }
        // This is service admission, not gameplay routing. If a caller supplied
        // a different program, retain the real restart and postpone C admission
        // until its actual audio executor/client/PCM retirement completes.
        made.error = result(audio.custom_reverb());
        if (made.error != ISLE_GAME_OK) {
            text(made.message, "application custom reverb admission failed"); return made;
        }
        made.error = result(isle_continuation_domain_create(&impl->domain));
        if (made.error == ISLE_GAME_OK)
            made.error = isle_application_source_prepare(&impl->source, &state.manager_state(),
                impl->domain, impl.get(), argument);
        if (made.error == ISLE_GAME_OK)
            made.error = result(isle_continuation_create(impl->domain,
                isle_application_source_run, &impl->source, &impl->continuation));
        if (made.error != ISLE_GAME_OK) {
            text(made.message, impl->domain ? isle_continuation_error(impl->domain) :
                 "application continuation domain admission failed");
            return made;
        }
        made.manager = std::unique_ptr<Manager>(new Manager(std::move(impl)));
    } catch (const std::bad_alloc&) {
        made.error = ISLE_GAME_EXHAUSTED; text(made.message, "application manager allocation failed");
    } catch (...) {
        made.error = ISLE_GAME_EXECUTION_FAILURE; text(made.message, "application manager admission failed");
    }
    return made;
}
IsleGameResult Manager::resume() noexcept {
    auto& p = *impl_;
    if (!p.on_owner()) return ISLE_GAME_WRONG_OWNER;
    if (p.retired || p.cancelled || p.resuming) return ISLE_GAME_INVALID_STATE;
    if (p.failure != ISLE_GAME_OK) return p.failure;
    if (!p.resources.live()) return p.fail(ISLE_GAME_INVALID_STATE, "application Snapshot residency expired");
    if (!p.source.started && !p.audio.initialization_ready()) {
        if (p.audio.state() == game_audio::Session::State::ready) return ISLE_GAME_OK;
        return p.fail(ISLE_GAME_EXECUTION_FAILURE, "application audio admission failed before original C start");
    }
    p.resuming = true;
    const auto resumed = isle_continuation_resume(p.continuation);
    p.resuming = false;
    if (p.source.status != ISLE_GAME_OK)
        return p.fail(p.source.status, "original application manager native leaf failed");
    if (resumed != ISLE_CONT_OK)
        return p.fail(result(resumed), isle_continuation_error(p.domain));
    return ISLE_GAME_OK;
}
const EntryIntent* Manager::pending_entry() const noexcept {
    const auto& p = *impl_;
    return p.on_owner() && !p.resuming && !p.cancelled && !p.retired && p.outstanding &&
        !p.completed ? &p.entry : nullptr;
}
EntryBorrow Manager::borrow_entry(std::uint64_t serial) noexcept {
    auto& p = *impl_;
    const auto* entry = pending_entry();
    if (!entry || entry->serial != serial || p.borrowed || p.released_serial == serial) return {};
    p.borrowed = true;
    return EntryBorrow(*this, *entry);
}
bool Manager::owns_entry(const EntryBorrow& borrow, IsleApplicationEntryOperation operation,
    std::uint32_t argument) const noexcept {
    const auto& p = *impl_;
    const auto* entry = pending_entry();
    // Intent alone is not authority: ownership and the live borrow must agree.
    return borrow.manager_ == this && entry && p.borrowed &&
        p.released_serial != entry->serial && borrow.intent_.serial == entry->serial &&
        borrow.intent_.operation == entry->operation && borrow.intent_.argument == entry->argument &&
        entry->operation == operation && entry->argument == argument;
}
IsleGameResult Manager::complete_entry(std::uint64_t serial, std::int32_t value) noexcept {
    auto& p = *impl_;
    if (!p.on_owner()) return ISLE_GAME_WRONG_OWNER;
    if (p.resuming || p.retired || p.cancelled || !p.outstanding || p.completed ||
        p.borrowed || p.entry.serial != serial || p.released_serial != serial) return ISLE_GAME_INVALID_STATE;
    p.entry_result = value;
    p.completed = true;
    return ISLE_GAME_OK;
}
void Manager::release_entry(std::uint64_t serial) noexcept {
    auto& p = *impl_;
    if (!p.on_owner() || !p.borrowed || !p.outstanding || p.entry.serial != serial)
        std::terminate();
    p.borrowed = false;
    p.released_serial = serial;
}
IsleGameResult Manager::cancel() noexcept {
    auto& p = *impl_;
    if (!p.on_owner()) return ISLE_GAME_WRONG_OWNER;
    if (p.resuming) return ISLE_GAME_INVALID_STATE;
    if (p.retired || p.cancelled) return ISLE_GAME_OK;
    const auto cancelled = isle_continuation_cancel(p.continuation);
    if (cancelled != ISLE_CONT_OK) return p.fail(result(cancelled), isle_continuation_error(p.domain));
    p.cancelled = true;
    return ISLE_GAME_OK;
}
IsleGameResult Manager::retire() noexcept {
    auto& p = *impl_;
    if (!p.on_owner()) return ISLE_GAME_WRONG_OWNER;
    if (p.resuming || p.borrowed) return ISLE_GAME_INVALID_STATE;
    if (p.retired) return ISLE_GAME_OK;
    const auto cancelled = cancel();
    if (cancelled != ISLE_GAME_OK) return cancelled;
    auto retired = isle_continuation_destroy(p.continuation);
    if (retired != ISLE_CONT_OK) return p.fail(result(retired), isle_continuation_error(p.domain));
    p.continuation = nullptr;
    retired = isle_continuation_domain_destroy(p.domain);
    if (retired != ISLE_CONT_OK) return p.fail(result(retired), "application domain retirement failed");
    p.domain = nullptr;
    p.source.domain = nullptr;
    p.source.owner = nullptr;
    p.outstanding = p.completed = false;
    p.retired = true;
    return ISLE_GAME_OK;
}
Status Manager::status() const noexcept {
    const auto& p = *impl_;
    if (!p.on_owner() || p.failure != ISLE_GAME_OK || p.source.status != ISLE_GAME_OK) return Status::failed;
    if (p.retired) return Status::retired;
    if (p.cancelled) return Status::cancelled;
    if (p.outstanding && !p.completed) return Status::entry_pending;
    IsleContinuationState state;
    if (isle_continuation_state(p.continuation, &state) != ISLE_CONT_OK) return Status::failed;
    switch (state) {
    case ISLE_CONT_READY: return Status::ready;
    case ISLE_CONT_RUNNING: case ISLE_CONT_WAITING: return Status::waiting;
    case ISLE_CONT_COMPLETED: return Status::completed;
    case ISLE_CONT_CANCELLED: return Status::cancelled;
    case ISLE_CONT_FAILED: return Status::failed;
    }
    return Status::failed;
}
IsleGameResult Manager::error() const noexcept { return impl_->failure; }
const char* Manager::message() const noexcept { return impl_->diagnostic.data(); }
std::int32_t Manager::source_level() const noexcept { return impl_->level; }
const game::VideoOffsets& Manager::screen_offsets() const noexcept { return impl_->state.screen_offsets(); }
IsleGameResult Manager::transition_end(std::int32_t reason) noexcept {
    auto& p = *impl_;
    if (!p.on_owner()) return ISLE_GAME_WRONG_OWNER;
    if (p.retired) return ISLE_GAME_INVALID_STATE;
    // Pinned own GAME app_render/4CDB0.c228–235: preserve the exact signed
    // reason and set all three transition channels, independently of fading
    // or the native scene owner's exit/completion state.
    p.end.reason = reason;
    p.end.transition_color.fill(reason == 2 ? 255 : 0);
    return ISLE_GAME_OK;
}
const ApplicationEndState& Manager::end_state() const noexcept { return impl_->end; }
const Resources& Manager::resources() const noexcept { return impl_->resources; }

EntryBorrow::EntryBorrow(Manager& manager, EntryIntent entry) noexcept : manager_(&manager), intent_(entry) {}
EntryBorrow::~EntryBorrow() { if (manager_) manager_->release_entry(intent_.serial); }
EntryBorrow::EntryBorrow(EntryBorrow&& other) noexcept
    : manager_(std::exchange(other.manager_, nullptr)), intent_(other.intent_) {}
EntryBorrow& EntryBorrow::operator=(EntryBorrow&& other) noexcept {
    if (this != &other) {
        if (manager_) manager_->release_entry(intent_.serial);
        manager_ = std::exchange(other.manager_, nullptr);
        intent_ = other.intent_;
    }
    return *this;
}

// This internal tagged call record is synchronous caller-owned handoff storage,
// not a configurable services table. It never owns or drives a native scene.
struct NativeAccess {
    enum class Operation {
        reverb_custom, global_reverb, extra_reverb, stop_bgm, bgm_volume,
        global_volume, channel_volume, overlay, rom, vpk, printer_available,
        printer_memory, cache, flag, saved_offsets, quality, offsets, intervals,
        level, request_entry, entry_result
    };
    struct Call {
        IsleApplicationSource* source;
        Operation operation;
        std::int64_t a = 0, b = 0, c = 0, d = 0;
        void* output = nullptr;
        void* output2 = nullptr;
        IsleGameResult answer = ISLE_GAME_INVALID_STATE;
    };
    static IsleGameResult execute(Manager::Impl& p, Call& call) {
        auto integer = [](void* output, std::int32_t value) {
            *static_cast<std::int32_t*>(output) = value;
        };
        switch (call.operation) {
        case Operation::reverb_custom: return result(p.audio.custom_reverb());
        case Operation::global_reverb:
            return byte_argument(call.a) ? result(p.audio.global_sound_reverb(static_cast<std::uint8_t>(call.a))) : ISLE_GAME_INVALID_ARGUMENT;
        case Operation::extra_reverb:
            return call.a >= 0 && call.a < 2 && signed_byte_argument(call.b) ?
                result(p.audio.bgm_extra_reverb(static_cast<unsigned>(call.a), static_cast<std::int8_t>(call.b))) : ISLE_GAME_INVALID_ARGUMENT;
        case Operation::stop_bgm: return result(p.audio.stop_bgm());
        case Operation::bgm_volume:
            return call.a >= 0 && call.a < 2 && call.b >= 0 ?
                result(p.audio.bgm_volume(static_cast<unsigned>(call.a), static_cast<std::uint32_t>(call.b))) : ISLE_GAME_INVALID_ARGUMENT;
        case Operation::global_volume:
            return byte_argument(call.a) ? result(p.audio.global_sound_volume(static_cast<std::uint8_t>(call.a))) : ISLE_GAME_INVALID_ARGUMENT;
        case Operation::channel_volume:
            return call.a >= 0 && call.a < 2 && call.b >= 0 && call.b < 16 && signed_byte_argument(call.c) ?
                result(p.audio.bgm_channel_volume(static_cast<unsigned>(call.a), static_cast<std::uint8_t>(call.b), static_cast<std::int8_t>(call.c))) : ISLE_GAME_INVALID_ARGUMENT;
        case Operation::overlay: return p.resources.retain(static_cast<IsleApplicationOverlay>(call.a));
        case Operation::rom: return p.resources.retain(static_cast<IsleApplicationRom>(call.a));
        case Operation::vpk: return p.resources.retain(static_cast<IsleApplicationVpk>(call.a));
        case Operation::printer_available: {
            if (!call.output) return ISLE_GAME_INVALID_ARGUMENT;
            const auto capability = p.input.require(game_input::Operation::printer);
            // No N64 printer endpoint exists on these native platforms. The
            // concrete input service states unsupported, not generic success.
            if (capability == game_input::Error::unsupported) { integer(call.output, 0); return ISLE_GAME_OK; }
            // A future endpoint requires its real memory/cache owner too. Do
            // not advertise support merely because controller samples work.
            return p.fail(ISLE_GAME_EXECUTION_FAILURE, "printer capability lacks a native memory/cache endpoint");
        }
        case Operation::printer_memory:
        case Operation::cache:
            return p.fail(ISLE_GAME_EXECUTION_FAILURE, "N64 printer memory/cache operation unavailable on native platform");
        case Operation::flag:
        case Operation::saved_offsets: {
            if (!call.output || (call.operation == Operation::saved_offsets && !call.output2)) return ISLE_GAME_INVALID_ARGUMENT;
            auto* save = p.state.save();
            if (!save) return p.fail(ISLE_GAME_INVALID_STATE, "Source settings requested before real boot profile initialization");
            const auto snapshot = save->snapshot();
            if (!snapshot) return p.fail(ISLE_GAME_EXECUTION_FAILURE, "canonical save snapshot unavailable");
            if (call.operation == Operation::flag) {
                if (call.a < 0) return ISLE_GAME_INVALID_ARGUMENT;
                const auto value = snapshot.value.player_flag(static_cast<unsigned>(call.a));
                if (!value) return ISLE_GAME_INVALID_ARGUMENT;
                integer(call.output, value.value ? 1 : 0);
            } else {
                const auto value = snapshot.value.screen_offsets();
                if (!value) return ISLE_GAME_EXECUTION_FAILURE;
                integer(call.output, value.value.horizontal);
                integer(call.output2, value.value.vertical);
            }
            return ISLE_GAME_OK;
        }
        case Operation::quality:
            if (call.a != 0 && call.a != 1) return ISLE_GAME_INVALID_ARGUMENT;
            p.audio.high_quality(call.a != 0);
            return ISLE_GAME_OK;
        case Operation::offsets:
            // Retain all four live VI endpoints. This does NOT mutate the
            // persistent save. Hosts with a single translation pair must
            // reject unequal endpoints rather than silently collapse them.
            p.state.set_screen_offsets({static_cast<std::int32_t>(call.a), static_cast<std::int32_t>(call.b),
                                        static_cast<std::int32_t>(call.c), static_cast<std::int32_t>(call.d)});
            return ISLE_GAME_OK;
        case Operation::intervals: {
            if (call.a <= 0 || call.a > std::numeric_limits<std::uint16_t>::max() ||
                call.b <= 0 || call.b > std::numeric_limits<std::uint16_t>::max()) return ISLE_GAME_INVALID_ARGUMENT;
            const auto changed = p.clock.set_intervals(static_cast<std::uint16_t>(call.a), static_cast<std::uint16_t>(call.b));
            return changed == host::ClockError::none ? ISLE_GAME_OK :
                changed == host::ClockError::invalid ? ISLE_GAME_INVALID_ARGUMENT : ISLE_GAME_INVALID_STATE;
        }
        case Operation::level:
            p.level = static_cast<std::int32_t>(call.a);
            return ISLE_GAME_OK;
        case Operation::request_entry:
            if (call.a < ISLE_APPLICATION_ENTRY_BOOT || call.a > ISLE_APPLICATION_ENTRY_PRINTER_DIAGNOSTIC)
                return ISLE_GAME_INVALID_ARGUMENT;
            if (p.outstanding || p.borrowed || p.serial == std::numeric_limits<std::uint64_t>::max()) return ISLE_GAME_INVALID_STATE;
            p.entry = {++p.serial, static_cast<IsleApplicationEntryOperation>(call.a), static_cast<std::uint32_t>(call.b)};
            p.outstanding = true;
            p.completed = false;
            return ISLE_GAME_OK;
        case Operation::entry_result:
            if (!call.output || !call.output2) return ISLE_GAME_INVALID_ARGUMENT;
            if (!p.outstanding) return ISLE_GAME_INVALID_STATE;
            integer(call.output, p.completed ? 1 : 0);
            if (p.completed) {
                if (p.borrowed) return ISLE_GAME_INVALID_STATE;
                integer(call.output2, p.entry_result);
                p.outstanding = false;
                p.completed = false;
            }
            return ISLE_GAME_OK;
        }
        return ISLE_GAME_EXECUTION_FAILURE;
    }
    static void on_owner(void* opaque) noexcept {
        auto& call = *static_cast<Call*>(opaque);
        auto& p = *static_cast<Manager::Impl*>(call.source->owner);
        if (!p.on_owner() || !p.resuming || p.retired || p.cancelled) {
            call.answer = ISLE_GAME_INVALID_STATE; return;
        }
        try { call.answer = execute(p, call); }
        catch (const std::bad_alloc&) { call.answer = ISLE_GAME_EXHAUSTED; }
        catch (...) { call.answer = ISLE_GAME_EXECUTION_FAILURE; }
        if (call.answer != ISLE_GAME_OK)
            p.fail(call.answer, "returning application native owner service failed");
    }
    static IsleGameResult invoke(Call call) noexcept {
        if (!call.source || !call.source->owner || !call.source->domain) return ISLE_GAME_INVALID_ARGUMENT;
        auto& p = *static_cast<Manager::Impl*>(call.source->owner);
        if (&p.source != call.source || call.source->state != &p.state.manager_state() ||
            call.source->domain != p.domain) return ISLE_GAME_INVALID_STATE;
        const auto access = isle_continuation_domain_check_access(call.source->domain);
        if (access != ISLE_CONT_OK) return result(access);
        const auto invoked = isle_continuation_on_owner(on_owner, &call);
        return invoked == ISLE_CONT_OK ? call.answer : result(invoked);
    }
};
} // namespace isle::application

extern "C" {
IsleGameResult isle_application_native_reverb_custom(IsleApplicationSource* s) {
    using A = isle::application::NativeAccess; return A::invoke({s,A::Operation::reverb_custom});
}
IsleGameResult isle_application_native_sound_global_reverb(IsleApplicationSource* s, int32_t value) {
    using A = isle::application::NativeAccess; return A::invoke({s,A::Operation::global_reverb,value});
}
IsleGameResult isle_application_native_bgm_extra_reverb(IsleApplicationSource* s, int32_t player, int32_t value) {
    using A = isle::application::NativeAccess; return A::invoke({s,A::Operation::extra_reverb,player,value});
}
IsleGameResult isle_application_native_stop_bgm(IsleApplicationSource* s) {
    using A = isle::application::NativeAccess; return A::invoke({s,A::Operation::stop_bgm});
}
IsleGameResult isle_application_native_bgm_volume(IsleApplicationSource* s, int32_t player, int32_t value) {
    using A = isle::application::NativeAccess; return A::invoke({s,A::Operation::bgm_volume,player,value});
}
IsleGameResult isle_application_native_sound_global_volume(IsleApplicationSource* s, int32_t value) {
    using A = isle::application::NativeAccess; return A::invoke({s,A::Operation::global_volume,value});
}
IsleGameResult isle_application_native_bgm_channel_volume(IsleApplicationSource* s, int32_t player, int32_t channel, int32_t value) {
    using A = isle::application::NativeAccess; return A::invoke({s,A::Operation::channel_volume,player,channel,value});
}
IsleGameResult isle_application_native_overlay(IsleApplicationSource* s, IsleApplicationOverlay value) {
    using A = isle::application::NativeAccess; return A::invoke({s,A::Operation::overlay,value});
}
IsleGameResult isle_application_native_rom(IsleApplicationSource* s, IsleApplicationRom value) {
    using A = isle::application::NativeAccess; return A::invoke({s,A::Operation::rom,value});
}
IsleGameResult isle_application_native_vpk(IsleApplicationSource* s, IsleApplicationVpk value) {
    using A = isle::application::NativeAccess; return A::invoke({s,A::Operation::vpk,value});
}
IsleGameResult isle_application_native_printer_available(IsleApplicationSource* s, int32_t* output) {
    using A = isle::application::NativeAccess; return A::invoke({s,A::Operation::printer_available,0,0,0,0,output});
}
IsleGameResult isle_application_native_printer_memory(IsleApplicationSource* s, IsleApplicationPrinterMemory* output) {
    using A = isle::application::NativeAccess; return A::invoke({s,A::Operation::printer_memory,0,0,0,0,output});
}
IsleGameResult isle_application_native_cache(IsleApplicationSource* s, IsleApplicationCacheOperation op,
    const IsleApplicationPrinterMemory* memory, size_t bytes) {
    using A = isle::application::NativeAccess;
    if (!memory || bytes > static_cast<size_t>(std::numeric_limits<int64_t>::max())) return ISLE_GAME_INVALID_ARGUMENT;
    return A::invoke({s,A::Operation::cache,op,static_cast<int64_t>(bytes)});
}
IsleGameResult isle_application_native_check_flag(IsleApplicationSource* s, int32_t flag, int32_t* output) {
    using A = isle::application::NativeAccess; return A::invoke({s,A::Operation::flag,flag,0,0,0,output});
}
IsleGameResult isle_application_native_saved_offsets(IsleApplicationSource* s, int32_t* horizontal, int32_t* vertical) {
    using A = isle::application::NativeAccess; return A::invoke({s,A::Operation::saved_offsets,0,0,0,0,horizontal,vertical});
}
IsleGameResult isle_application_native_sound_quality(IsleApplicationSource* s, int32_t value) {
    using A = isle::application::NativeAccess; return A::invoke({s,A::Operation::quality,value});
}
IsleGameResult isle_application_native_screen_offsets(IsleApplicationSource* s, int32_t h0, int32_t h1, int32_t v0, int32_t v1) {
    using A = isle::application::NativeAccess; return A::invoke({s,A::Operation::offsets,h0,h1,v0,v1});
}
IsleGameResult isle_application_native_intervals(IsleApplicationSource* s, uint32_t update, uint32_t draw) {
    using A = isle::application::NativeAccess; return A::invoke({s,A::Operation::intervals,update,draw});
}
IsleGameResult isle_application_native_set_level(IsleApplicationSource* s, int32_t level) {
    using A = isle::application::NativeAccess; return A::invoke({s,A::Operation::level,level});
}
IsleGameResult isle_application_native_request_entry(IsleApplicationSource* s, IsleApplicationEntryOperation op, uint32_t argument) {
    using A = isle::application::NativeAccess; return A::invoke({s,A::Operation::request_entry,op,argument});
}
IsleGameResult isle_application_native_entry_result(IsleApplicationSource* s, int32_t* ready, int32_t* source_result) {
    using A = isle::application::NativeAccess; return A::invoke({s,A::Operation::entry_result,0,0,0,0,ready,source_result});
}
} // extern "C"

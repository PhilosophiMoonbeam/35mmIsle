#include "game_audio_driver.hpp"
#include "graphics_task.hpp"
#include "35mmisle/rsp.hpp"

#include <algorithm>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <utility>

namespace isle::game_audio {
namespace {
constexpr std::uint32_t audio_boot = 0x107000;
constexpr std::uint32_t audio_text = 0x3d980, audio_data = 0x44ba0;

template<std::size_t N> void text(std::array<char, N>& to, const char* from) noexcept {
    const auto n = from ? std::min(N - 1, std::strlen(from)) : 0;
    if (n) std::memcpy(to.data(), from, n);
    to[n] = '\0';
}
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
} // namespace

struct Driver::Impl {
    Output output;
    DriverError error = DriverError::none;
    std::array<char, 512> message{};
    bool retired = false;
    // Only initialize_executor borrows Snapshot. All subsequent RSP execution
    // uses owned boot bytes and the admitted image in Session's producer RAM.
    const rom::Snapshot* initializing_snapshot = nullptr;
    std::array<std::byte, graphics::boot_bytes> boot{};
    std::unique_ptr<rsp::Machine> machine;
    std::span<std::uint32_t> borrowed_audio_ram;
    // Session's destructor calls retire_executor before destroying service RAM.
    // Keep these executor callback fields alive even during partial creation.
    std::unique_ptr<Session> audio_session;

    explicit Impl(Output endpoint) noexcept : output(endpoint) {}
    ~Impl() { retire(); }
    bool fail(DriverError kind, const char* reason) noexcept {
        if (error == DriverError::none) { error = kind; text(message, reason); }
        return false;
    }
    bool active() noexcept {
        if (retired || !audio_session) return fail(DriverError::retired, "GAME audio Driver has retired");
        return error == DriverError::none;
    }
    static bool reject_dp(void*, std::span<const std::uint32_t>) noexcept { return false; }
    static bool initialize_executor(void* owner, std::span<std::uint32_t> words) noexcept {
        auto& self = *static_cast<Impl*>(owner);
        try {
            require(self.initializing_snapshot && !self.machine, "Audio executor initialization has no admitted ROM");
            const auto& snapshot = *self.initializing_snapshot;
            graphics::validate_cold_inputs(words, snapshot, snapshot.canonical().data(), audio_boot);
            graphics::install_cold_inputs(words, snapshot, audio_boot);
            std::copy_n(snapshot.canonical().begin() + graphics::boot_rom, self.boot.size(), self.boot.begin());
            self.machine = std::make_unique<rsp::Machine>(words, reject_dp, nullptr);
            self.borrowed_audio_ram = words;
            return true;
        } catch (const std::bad_alloc&) {
            self.fail(DriverError::allocation, "Audio RSP allocation failed");
        } catch (const std::exception& e) {
            self.fail(DriverError::execution, e.what());
        } catch (...) { self.fail(DriverError::execution, "Audio RSP initialization failed"); }
        self.machine.reset(); self.borrowed_audio_ram = {};
        return false;
    }
    static bool executor_ready(void* owner, std::span<std::uint32_t> words) noexcept {
        const auto& self = *static_cast<Impl*>(owner);
        return self.machine && self.borrowed_audio_ram.data() == words.data() && self.borrowed_audio_ram.size() == words.size();
    }
    static void retire_executor(void* owner) noexcept {
        auto& self = *static_cast<Impl*>(owner);
        self.machine.reset(); self.borrowed_audio_ram = {};
    }
    bool init(const rom::Snapshot& snapshot) noexcept {
        if (!output.owner || !output.inspect || !output.submit || !output.finish || !output.poll)
            return fail(DriverError::invalid, "GAME audio Driver requires a complete actual output endpoint");
        auto hydrated = hydrate(snapshot);
        if (!hydrated)
            return fail(hydrated.error == audio::Error::allocation_failure ? DriverError::allocation : DriverError::resources,
                        "GAME audio resource hydration failed");
        std::array<audio::effects::Section, 8> sections{};
        audio::effects::Config effects;
        const auto derived = native_startup_effects(hydrated.resources, sections, effects);
        if (derived != audio::Error::none)
            return fail(derived == audio::Error::allocation_failure ? DriverError::allocation : DriverError::resources,
                        "GAME default eight-section effects derivation failed");
        initializing_snapshot = &snapshot;
        auto made = Session::create(std::move(hydrated.resources), effects, output,
                                    {this, initialize_executor, executor_ready, retire_executor});
        initializing_snapshot = nullptr;
        if (!made) {
            const auto kind = made.error == audio::Error::allocation_failure ? DriverError::allocation :
                made.error == audio::Error::execution_failed ? DriverError::execution :
                made.error == audio::Error::invalid_state ? DriverError::output : DriverError::resources;
            return fail(kind, "GAME audio/player/output/RSP initialization failed");
        }
        audio_session = std::move(made.session);
        if (!audio_session->initialization_ready() || !executor_ready(this, audio_session->service().rdram_words()))
            return fail(DriverError::execution, "GAME audio Session is not actually ready");
        return true;
    }
    bool execute_audio(const audio::TaskView& built) noexcept {
        bool success = false;
        try {
            require(executor_ready(this, audio_session->service().rdram_words()),
                    "Audio task lost its actual RSP borrower");
            const rsp::Task task{2, 0, audio_boot, graphics::boot_bytes, audio_text, 0x1000, audio_data, 0x800,
                0, 0, 0, 0, built.command_address, built.command_bytes, 0, 0};
            const auto actual = machine->execute(task, boot, {10000000, 0});
            success = bool(actual) && actual.sp_status == 0x203 && actual.packets == 0 &&
                actual.packet_words == 0 && actual.full_syncs == 0;
            if (!success) fail(DriverError::execution, "Audio RSP failed SP0x203/no-DPC/budget terminal validation");
        } catch (const std::exception& e) { fail(DriverError::execution, e.what()); }
        catch (...) { fail(DriverError::execution, "Audio RSP execution failed"); }
        // Every published task receives its actual terminal, including exception,
        // unexpected DPC, budget and malformed status paths. No PCM on failure.
        const auto completed = audio_session->complete(built.token, success);
        if (completed != audio::Error::none)
            return fail(DriverError::execution, "Audio actual-terminal completion failed");
        return success;
    }
    bool pump() noexcept {
        if (!active()) return false;
        if (output.inspect(output.owner).state == Output::State::failed)
            return fail(DriverError::output, "Audio endpoint failed");
        if (audio_session->submit() != audio::Error::none)
            return fail(DriverError::output, "Audio PCM acceptance failed");
        audio::TaskView built;
        const auto result = audio_session->build(built);
        if (result != audio::Error::none) {
            if (built.token.serial) (void)audio_session->complete(built.token, false);
            return fail(DriverError::execution, "Audio source task build failed");
        }
        if (built.token.serial && !execute_audio(built)) return false;
        if (audio_session->submit() != audio::Error::none)
            return fail(DriverError::output, "Audio source-gated PCM acceptance failed");
        return true;
    }
    bool retire() noexcept {
        if (retired) return error == DriverError::none;
        // Execution is synchronous: pump never leaves an executing task behind.
        // A retained completed lease is explicitly discarded, not called drained.
        // Session retires its real RSP borrower before releasing AudioService RAM.
        if (audio_session) {
            if (audio_session->service().has_lease() && audio_session->abandon_pcm() != audio::Error::none)
                fail(DriverError::execution, "Audio abort could not retire its completed PCM lease");
            audio_session.reset();
        }
        retire_executor(this);
        initializing_snapshot = nullptr;
        retired = true;
        return error == DriverError::none;
    }
};

Driver::Driver(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}
Driver::~Driver() = default;
DriverCreateResult Driver::create(const rom::Snapshot& snapshot, Output output) noexcept {
    DriverCreateResult result;
    std::unique_ptr<Impl> impl;
    try {
        impl = std::make_unique<Impl>(output);
        if (impl->init(snapshot)) {
            result.driver.reset(new Driver(std::move(impl)));
            return result;
        }
    } catch (const std::bad_alloc&) {
        if (impl) impl->fail(DriverError::allocation, "GAME audio Driver allocation failed");
        result.error = DriverError::allocation;
        text(result.message, "GAME audio Driver allocation failed");
    } catch (const std::exception& e) {
        if (impl) impl->fail(DriverError::execution, e.what());
        result.error = DriverError::execution;
        text(result.message, e.what());
    } catch (...) {
        if (impl) impl->fail(DriverError::execution, "GAME audio Driver initialization failed");
        result.error = DriverError::execution;
        text(result.message, "GAME audio Driver initialization failed");
    }
    if (impl) {
        result.error = impl->error;
        text(result.message, impl->message.data());
    }
    return result;
}
Session& Driver::session() noexcept {
    if (!impl_->audio_session) std::terminate();
    return *impl_->audio_session;
}
DriverError Driver::error() const noexcept { return impl_->error; }
const char* Driver::message() const noexcept { return impl_->message.data(); }
bool Driver::notify_retrace(std::uint64_t ordinal, std::uint64_t ns) noexcept {
    if (!impl_->active()) return false;
    return impl_->audio_session->retrace(ordinal, ns) == audio::Error::none ||
        impl_->fail(DriverError::execution, "Source audio retrace rejected");
}
bool Driver::pump() noexcept { return impl_->pump(); }
bool Driver::stop() noexcept {
    if (!impl_->active()) return false;
    return impl_->audio_session->stop() == audio::Error::none ||
        impl_->fail(DriverError::execution, "Source audio stop failed");
}
Drain Driver::poll_drain() noexcept {
    if (!impl_->active()) return Drain::failed;
    const auto state = impl_->audio_session->state();
    if (state == Session::State::drained) return Drain::drained;
    if (state == Session::State::failed) {
        impl_->fail(DriverError::execution, "Source audio drain failed");
        return Drain::failed;
    }
    if (state != Session::State::draining) return Drain::pending;
    if (impl_->audio_session->poll_drain() != audio::Error::none) {
        impl_->fail(DriverError::output, "Source audio stream drain failed");
        return Drain::failed;
    }
    return impl_->audio_session->state() == Session::State::drained ? Drain::drained : Drain::pending;
}
bool Driver::retire() noexcept { return impl_->retire(); }
} // namespace isle::game_audio

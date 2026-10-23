#ifndef ISLE_GAME_INPUT_HPP
#define ISLE_GAME_INPUT_HPP

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace isle::snap_station { class Device; }
namespace isle::game_input {

inline constexpr std::size_t port_count = 4;
enum class Error { none, stopped, already_started, invalid_sample, invalid_parameter,
                   read_in_flight, pending_update, stale_token, exhausted, unsupported,
                   transfer_full, transfer_empty, endpoint_failure };
enum class Connection { absent, present };
// Identity is supplied by acquisition, not synthesized by the service. Zero means
// absent. Error is the host's actual per-port read error, never accessory status.
struct PortSample {
    Connection connection = Connection::absent;
    std::uint64_t identity = 0;
    std::uint8_t error = 0;
    std::uint16_t buttons = 0;
    std::int8_t stick_x = 0, stick_y = 0;
};
using HostFrame = std::array<PortSample, port_count>;
struct Pad {
    std::uint16_t buttons = 0, pressed = 0, held = 0, released = 0;
    std::int8_t stick_x = 0, stick_y = 0;
};
struct Snapshot {
    std::array<Pad, port_count> pads{};
    HostFrame ports{}; // Last published acquisition metadata; pads freeze on error.
    std::array<int, port_count> indices{-1, -1, -1, -1};
    std::size_t controller_count = 0;
    // GAME's scalar gContInput* aliases always select physical port zero,
    // NOT the first healthy controller. indices provides explicit selection.
    [[nodiscard]] const Pad& primary() const noexcept { return pads[0]; }
};
class Service;
struct RequestToken {
    const Service* owner = nullptr;
    std::uint64_t generation = 0, serial = 0;
    [[nodiscard]] explicit operator bool() const noexcept { return owner != nullptr; }
    friend bool operator==(RequestToken, RequestToken) = default;
};
struct ReadToken {
    const Service* owner = nullptr;
    std::uint64_t generation = 0, serial = 0;
    [[nodiscard]] explicit operator bool() const noexcept { return owner != nullptr; }
    friend bool operator==(ReadToken, ReadToken) = default;
};
struct EventResult {
    Error error = Error::none;
    RequestToken accepted{}; // update/read_and_update ticket, if accepted.
    ReadToken read{};        // Acquisition must complete THIS read, even if unchanged.
    RequestToken acknowledged{}; // Only real publication acknowledges a ticket.
};
struct StopResult {
    Error error = Error::none;
    std::array<RequestToken, 2> cancelled{}; // Exactly once; never acknowledgements.
};
enum class Operation { controller_samples, rumble, controller_pak, transfer_pak, printer };
inline constexpr std::uint32_t stick_slow_up = 0x10000, stick_slow_down = 0x20000,
    stick_slow_right = 0x40000, stick_slow_left = 0x80000,
    stick_up = 0x100000, stick_down = 0x200000,
    stick_right = 0x400000, stick_left = 0x800000;
struct MenuInput {
    std::uint64_t update = 0;
    float stick_x = 0, stick_y = 0, unused_axis = 0;
    std::uint32_t current = 0, pressed = 0, released = 0, not_pressed = 0;
};
struct MenuResult { Error error = Error::none; const MenuInput* input = nullptr; };

// One serialized gameplay/service owner, independent of optional presentation.
// No method is freely concurrent; the host MUST NOT mutate this object. Use
// HostTransfer below, or hand samples over at a caller-provided synchronization
// boundary. References remain at fixed addresses, but only the owner may read
// them during mutation. No allocations, host API calls or exceptions are used.
// Tokens and queued transfers must not outlive this Service object. Reentry is
// stop/start on this stable object, never reconstruction beneath live queues.
class Service {
public:
    Service() noexcept = default;
    Service(const Service&) = delete;
    Service& operator=(const Service&) = delete;
    // Initialization uses only actual connection/error metadata, not buttons.
    // Reentry restores initialized/BSS input state; generations never wrap.
    [[nodiscard]] Error start(const HostFrame& initial) noexcept;
    [[nodiscard]] StopResult stop() noexcept;
    [[nodiscard]] EventResult retrace() noexcept;
    [[nodiscard]] EventResult update() noexcept;
    [[nodiscard]] EventResult read_and_update() noexcept;
    [[nodiscard]] EventResult complete_read(ReadToken token, const HostFrame& frame) noexcept;
    [[nodiscard]] Error set_update_every_tick(bool enabled) noexcept;
    [[nodiscard]] Error set_hold(std::int32_t delay, std::int32_t interval) noexcept;
    [[nodiscard]] Error require(Operation operation) const noexcept;
    // Borrowed native endpoint; owner-thread only and must outlive this binding.
    void bind_printer(snap_station::Device* station) noexcept { station_ = station; }
    snap_station::Device* printer() const noexcept { return station_; }
    [[nodiscard]] bool running() const noexcept { return running_; }
    [[nodiscard]] bool needs_publication() const noexcept { return dirty_; }
    [[nodiscard]] bool update_every_tick() const noexcept { return every_tick_; }
    [[nodiscard]] RequestToken pending_update() const noexcept { return pending_; }
    [[nodiscard]] ReadToken pending_read() const noexcept { return read_; }
    [[nodiscard]] const Snapshot& snapshot() const noexcept { return snapshot_; }
    // Source func_800AA81C invalidates once before each OM update, NOT per pad
    // publication. menu() computes ALL ports once and caches across publications.
    // neutral() shares that cache gate, but never populates it.
    [[nodiscard]] Error begin_update() noexcept;
    [[nodiscard]] MenuResult menu(int physical_port) noexcept;
    // Source func_800AA740 returns separate stable storage, never pad/menu data.
    // Unless menu() already populated this update's cache, a valid query zeros
    // axes/current/pressed/released, sets not_pressed to ~0, and stamps the current
    // menu serial for the contiguous prefix [0, snapshot().controller_count).
    // Remaining entries retain their old/BSS values, regardless of physical
    // connection indices. Invalid ports return neutral port zero without writes.
    // No acquisition, serial increment, cache fill, edge or repeat-state changes.
    [[nodiscard]] MenuResult neutral(int physical_port) noexcept;
    // Source AAE28 scene entry resets delay clocks only. Preserve the cached
    // samples, repeat intervals and every acquisition/publication ticket.
    [[nodiscard]] Error reset_scene_menu() noexcept;
    [[nodiscard]] Error set_menu_repeat(std::int32_t delay, std::int32_t interval,
                                        std::uint32_t mask) noexcept;
    // Source AA85C and AA870 are independent writes, not parameter resets.
    [[nodiscard]] Error set_menu_repeat(std::int32_t delay, std::int32_t interval) noexcept;
    [[nodiscard]] Error set_menu_mask(std::uint32_t mask) noexcept;
    [[nodiscard]] Error set_menu_scale(std::uint32_t value) noexcept;
    [[nodiscard]] std::uint32_t menu_scale() const noexcept { return menu_scale_ * 8u; }
private:
    snap_station::Device* station_ = nullptr;
    struct State {
        Pad raw{};
        std::uint16_t pressed = 0, released = 0, held = 0;
        std::int32_t counter = 30;
    };
    [[nodiscard]] Error available() const noexcept;
    [[nodiscard]] Error reserve(std::uint64_t count) const noexcept;
    [[nodiscard]] RequestToken ticket() noexcept;
    [[nodiscard]] ReadToken read_ticket() noexcept;
    void publish() noexcept;
    std::array<State, port_count> state_{};
    HostFrame ports_{};
    Snapshot snapshot_{};
    std::array<MenuInput, port_count> menu_{};
    std::array<MenuInput, port_count> neutral_{};
    std::array<std::array<std::int32_t, 24>, port_count> menu_delay_{}, menu_interval_{};
    std::uint64_t generation_ = 0, serial_ = 0, menu_update_ = 0;
    std::int32_t hold_delay_ = 30, hold_interval_ = 5;
    std::int32_t menu_hold_delay_ = 36, menu_hold_interval_ = 12;
    std::uint32_t menu_mask_ = 0, menu_scale_ = 0;
    bool running_ = false, dirty_ = false, every_tick_ = true, menu_cached_ = false;
    RequestToken pending_{}, fresh_{};
    ReadToken read_{};
};

struct ReadCompletion { ReadToken token{}; HostFrame frame{}; };
// Two fixed SPSC rings: gameplay sends read requests, main acquires actual LUS
// samples and sends completions, gameplay consumes them. Full is backpressure,
// never drop/coalesce/acknowledge. Only one producer and one consumer per ring.
// Retain a failed push and retry it before acquiring/accepting another event.
// Construct before either thread starts; destroy only after both join. Do not
// reset on scene reentry: old completions are rejected by Service generations.
class HostTransfer {
public:
    static constexpr std::size_t capacity = 16;
    [[nodiscard]] Error request_read(ReadToken token) noexcept;
    [[nodiscard]] Error acquire_request(ReadToken& token) noexcept;
    [[nodiscard]] Error submit_sample(const ReadCompletion& completion) noexcept;
    [[nodiscard]] Error acquire_sample(ReadCompletion& completion) noexcept;
private:
    template<class T> struct Ring {
        std::array<T, capacity + 1> values{};
        std::atomic<std::size_t> write{0}, read{0};
        Error push(const T& value) noexcept {
            const auto at = write.load(std::memory_order_relaxed);
            const auto next = (at + 1) % values.size();
            if (next == read.load(std::memory_order_acquire)) return Error::transfer_full;
            values[at] = value;
            write.store(next, std::memory_order_release);
            return Error::none;
        }
        Error pop(T& value) noexcept {
            const auto at = read.load(std::memory_order_relaxed);
            if (at == write.load(std::memory_order_acquire)) return Error::transfer_empty;
            value = values[at];
            read.store((at + 1) % values.size(), std::memory_order_release);
            return Error::none;
        }
    };
    Ring<ReadToken> requests_{};
    Ring<ReadCompletion> completions_{};
};

} // namespace isle::game_input
#endif

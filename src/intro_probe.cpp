#include "intro_probe.h"

#include "35mmisle/engine.h"
#include "rom_snapshot.hpp"
#include "35mmisle/sprite.hpp"
#include "35mmisle/vpk0.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <mutex>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <thread>
#include <vector>

namespace {
// Address/extent facts for the admitted US image, not extracted payloads.
constexpr std::size_t introRomStart = 0xAA0B80;
constexpr std::size_t introRomEnd = 0xAA18D3;
constexpr std::size_t introBankSize = 28944;
constexpr std::uint32_t introBankBase = 0x802B5000;
constexpr std::uint32_t introSpriteAddress = 0x802BC0C0;
constexpr std::uint32_t holdTicks = 60;
constexpr std::uint16_t fadeTicks = 20;
constexpr std::size_t bitmapCount = 8;
constexpr std::size_t tmemBytes = 4096;

isle::sprite::BankOwner LoadBank(const isle::rom::Snapshot& snapshot) {
    if (snapshot.canonical().size() != isle::rom::us_size)
        throw std::runtime_error("Private intro requires a live admitted ROM snapshot");
    std::cout << "INTRO_ROM_IDENTITY: exact US image; size=" << snapshot.identity().size
              << "; normalized SHA1=" << isle::rom::digest_hex(snapshot.identity()) << '\n';

    const auto compressed = snapshot.canonical().subspan(introRomStart, introRomEnd - introRomStart);
    auto writableBank = std::make_shared<isle::sprite::Bank>(introBankSize);
    const auto decoded = isle::vpk0::decode(compressed, std::as_writable_bytes(std::span(*writableBank)));
    if (!decoded || decoded.bytes_written != introBankSize || decoded.bytes_consumed > compressed.size()) {
        throw std::runtime_error("Private intro vpk0 stream failed bounded decoding");
    }
    // Conversion drops the only mutable bank alias before any C view is made.
    return isle::sprite::BankOwner(std::move(writableBank));
}

class NativeGame final {
  public:
    NativeGame() : bytes(65536) {
        if (isle_arena_init(&arena, bytes.data(), bytes.size()) != ISLE_OK) {
            throw std::runtime_error("Intro native arena initialization failed");
        }
        Require(isle_game_create(&arena, &game));
    }
    ~NativeGame() {
        if (game != nullptr && isle_game_destroy(game) != ISLE_GAME_OK) {
            // Releasing an arena while a C continuation may still refer to it is
            // never a recoverable cleanup strategy.
            std::abort();
        }
    }
    NativeGame(const NativeGame&) = delete;
    NativeGame& operator=(const NativeGame&) = delete;

    void Require(IsleGameResult result) const {
        if (result != ISLE_GAME_OK) {
            throw std::runtime_error("Authoritative native intro game operation failed");
        }
    }
    void Stop() {
        if (game != nullptr) {
            Require(isle_game_destroy(game)); // Ends and joins every continuation.
            game = nullptr;
        }
    }
    IsleGame* Get() const noexcept { return game; }

  private:
    std::vector<unsigned char> bytes;
    isle_arena arena{};
    IsleGame* game = nullptr;
};

void CheckLayout(const IsleSprite& view) {
    if (view.width != 128 || view.height != 112 || view.scalex != 1.0f || view.scaley != 1.0f ||
        view.expx != 0 || view.expy != 0 || view.frac_s != 0 || view.frac_t != 0 ||
        view.istart != 0 || view.istep != 1 || view.nbitmaps != static_cast<int>(bitmapCount) ||
        view.bmfmt != 0 || view.bmsiz != 2 || view.bmheight != 64 || view.bmHreal != 64 ||
        view.bitmap == nullptr) {
        throw std::runtime_error("Unsupported intro Sprite layout: only the unscaled 128x112 US mosaic is implemented");
    }
    for (std::size_t i = 0; i < bitmapCount; ++i) {
        const auto& bitmap = view.bitmap[i];
        if (bitmap.width != 32 || bitmap.width_img != 32 || bitmap.s != 0 || bitmap.t != 0 ||
            bitmap.actualHeight != (i < 4 ? 64 : 48) || bitmap.buf == nullptr) {
            throw std::runtime_error("Unsupported intro Bitmap layout: requires 32-wide planes, zero s/t, and visible heights 64/48");
        }
    }
}
} // namespace

namespace {
struct IntroScene {
    isle::sprite::BankOwner bank;
    isle::sprite::Sprite asset;
    std::array<std::vector<std::uint8_t>, bitmapCount> planes;
    std::array<Gfx, 256> commands{};
    Gfx* textures = nullptr;
    // Last owning member is destroyed first, before borrowed asset/texel owners.
    NativeGame world;
    IsleGameObject* object = nullptr;
    IsleGameSprite* sprite = nullptr;
    IsleSprite* view = nullptr;
    std::uint32_t ticks = 0;

    explicit IntroScene(const isle::rom::Snapshot& snapshot) : bank(LoadBank(snapshot)) {
        auto hydrated = isle::sprite::hydrate(bank, introBankBase, introSpriteAddress);
        if (!hydrated) {
            throw std::runtime_error("Private intro Sprite hydration failed; no procedural fallback is permitted");
        }
        asset = std::move(hydrated.asset);
        CheckLayout(*asset.view());
        IsleSprite initial = *asset.view();
        initial.attr = ISLE_SP_TEXSHUF | ISLE_SP_TRANSPARENT;
        initial.x = static_cast<std::int16_t>(160 - initial.width / 2);
        initial.y = static_cast<std::int16_t>(120 - initial.height / 2);
        world.Require(isle_game_add_object(world.Get(), 0xE, 0, 0x80000000, nullptr, nullptr, &object));
        world.Require(isle_game_add_sprite(world.Get(), object, &initial, &sprite));
        world.Require(isle_game_sprite_view(world.Get(), sprite, &view));
        for (std::size_t i = 0; i < bitmapCount; ++i) {
            // These pieces have explicit stored heights, including the short
            // final mosaic row. Never read nominal padding past the bank.
            const auto capacity = static_cast<std::size_t>(view->bitmap[i].width_img) *
                                  view->bitmap[i].actualHeight * 4;
            planes[i].resize(capacity);
            const auto result = isle::sprite::normalize_rgba8(*view, i, planes[i]);
            if (!result || result.width != static_cast<std::size_t>(view->bitmap[i].width_img) ||
                result.height != static_cast<std::size_t>(view->bitmap[i].actualHeight) ||
                result.bytes_written != result.width * result.height * 4) {
                throw std::runtime_error("Private intro Bitmap normalization failed");
            }
        }
        BuildCommands();
        std::cout << "BOUNDED_INTRO_ASSET_SCOPE: original 128x112 US sprite centered at (96,64); "
                     "8 bitmap views normalized once; native C GObj/SObj; TEXSHUF|TRANSPARENT.\n"
                     "GTL experiment: dedicated ordinary game-owner thread; rational VI clock, bounded retraces; "
                     "60 game updates before the selected original 20-update RGB fade (alpha is not faded).\n"
                     "Not full intro entry, scene boot, persistence/save, original intro audio/cue timing, photo fidelity, full Sprite/RDP parity, "
                     "or broad platform evidence.\n";
    }

    void BuildCommands() {
        Gfx* out = commands.data();
        gDPPipeSync(out++);
        gDPSetScissor(out++, G_SC_NON_INTERLACE, 0, 0, 320, 240);
        gDPSetCycleType(out++, G_CYC_FILL);
        gDPSetColorImage(out++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320, 0x00100000);
        gDPSetDepthImage(out++, 0x00200000);
        gDPSetFillColor(out++, 0x00010001);
        gDPFillRectangle(out++, 0, 0, 319, 239);
        gDPPipeSync(out++);
        gDPSetCycleType(out++, G_CYC_1CYCLE);
        gDPSetTexturePersp(out++, G_TP_NONE);
        gDPSetTextureLOD(out++, G_TL_TILE);
        gDPSetTextureLUT(out++, G_TT_NONE);
        gDPSetTextureFilter(out++, G_TF_POINT);
        gDPSetAlphaCompare(out++, G_AC_NONE);
        gDPSetRenderMode(out++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        gDPSetCombineMode(out++, G_CC_MODULATERGBA_PRIM, G_CC_MODULATERGBA_PRIM);
        gSPEndDisplayList(out++);
        textures = out;

        for (std::size_t i = 0; i < bitmapCount; ++i) {
            const auto& bitmap = view->bitmap[i];
            const int left = view->x + static_cast<int>(i % 4) * bitmap.width;
            const int top = view->y + static_cast<int>(i / 4) * view->bmheight;
            const int rowsPerTile = static_cast<int>(tmemBytes / (static_cast<std::size_t>(bitmap.width_img) * 4));
            for (int row = 0; row < bitmap.actualHeight; row += rowsPerTile) {
                const int rows = std::min(rowsPerTile, static_cast<int>(bitmap.actualHeight) - row);
                const auto* pixels = planes[i].data() + static_cast<std::size_t>(row) * bitmap.width_img * 4;
                // RGBA32 uses two TMEM banks; 32x32 is 4096 bytes total,
                // 2048 per bank. No full 32x64/8192-byte plane is loaded.
                // Point sampling and clamp avoid cross-tile filtering seams.
                gDPLoadTextureTile(out++, pixels, G_IM_FMT_RGBA, G_IM_SIZ_32b,
                                   bitmap.width_img, rows, 0, 0, bitmap.width - 1, rows - 1, 0,
                                   G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
                gSPTextureRectangle(out++, left * 4, (top + row) * 4,
                                    (left + bitmap.width) * 4, (top + row + rows) * 4,
                                    G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
                gDPPipeSync(out++);
            }
        }
        gDPFullSync(out++);
        gSPEndDisplayList(out++);
        // Two immutable lists fit here; each context owns four wrapper commands.
    }

    void BuildDraw(std::span<Gfx, 4> draw) const {
        Gfx* out = draw.data();
        gDma1p(out++, G_DL, commands.data(), 0, G_DL_PUSH);
        gDPSetPrimColor(out++, 0, 0, view->red, view->green, view->blue, view->alpha);
        gDma1p(out++, G_DL, textures, 0, G_DL_PUSH);
        gSPEndDisplayList(out++);
    }
    void Advance() {
        if (world.Get() == nullptr) throw std::runtime_error("Cannot advance a stopped intro world");
        if (ticks + 1 == holdTicks) {
            IsleGameProcess* fade = nullptr;
            world.Require(isle_game_add_intro_fade(world.Get(), object, sprite, fadeTicks, 5, &fade));
        }
        world.Require(isle_game_tick(world.Get()));
        ++ticks;
        if (ticks == holdTicks || ticks == holdTicks + fadeTicks - 1) {
            std::cout << "INTRO_C_FADE_STATE: tick=" << ticks
                      << "; RGB=" << static_cast<unsigned>(view->red) << ','
                      << static_cast<unsigned>(view->green) << ','
                      << static_cast<unsigned>(view->blue)
                      << "; alpha=" << static_cast<unsigned>(view->alpha) << '\n';
        }
    }
    void StopGame() {
        if (world.Get() != nullptr) {
            world.Stop();
            object = nullptr;
            sprite = nullptr;
            view = nullptr;
            std::cout << "INTRO_C_USERS_QUIESCENT: owner destroyed game and joined continuations; "
                         "arena, bank, pixels and immutable display lists retained for renderer shutdown.\n";
        }
    }
};
} // namespace

struct IntroProbe::Impl {
    using Clock = std::chrono::steady_clock;
    using Error = isle::host::ClockError;
    std::mutex mutex;
    std::condition_variable changed;
    isle::host::GtlClock policy{{}};
    std::array<std::array<Gfx, 4>, 2> wrappers{};
    std::array<Frame, 2> queue{};
    std::array<bool, 2> borrowed{};
    std::size_t head = 0, count = 0;
    std::uint32_t ticks = 0;
    bool ready = false, started = false, game_stopped = false, release_assets = false;
    std::exception_ptr failure;
    std::thread owner;

    explicit Impl(const isle::rom::Snapshot& snapshot)
        : owner([this, &snapshot] { Run(snapshot); }) {
        std::unique_lock lock(mutex);
        changed.wait(lock, [this] { return ready; });
        if (failure) {
            lock.unlock();
            owner.join();
            std::rethrow_exception(failure);
        }
    }
    void StopScene(std::unique_lock<std::mutex>& lock, IntroScene& scene) {
        if (game_stopped) return;
        lock.unlock();
        try { scene.StopGame(); }
        catch (...) { std::abort(); } // Never free an unquiescent C arena.
        lock.lock();
        game_stopped = true;
        changed.notify_all();
    }
    void Run(const isle::rom::Snapshot& snapshot) {
        std::optional<IntroScene> scene;
        try { scene.emplace(snapshot); }
        catch (...) {
            std::lock_guard lock(mutex);
            failure = std::current_exception();
            ready = game_stopped = true;
            policy.request_stop();
            (void)policy.poll();
            changed.notify_all();
            return;
        }
        std::unique_lock lock(mutex);
        ready = true; // No snapshot reference is used after this admission barrier.
        changed.notify_all();
        changed.wait(lock, [this] { return started || policy.stopping(); });
        const auto origin = Clock::now();
        // Documented NTSC progressive rational (not the emulator's modal VI
        // period, physical hardware qualification, or a presentation callback).
        isle::host::RetraceClock retraces(2250000, 37609);
        for (;;) {
            if (policy.stopping()) StopScene(lock, *scene);
            if (!policy.stopping()) {
                const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - origin);
                const auto due = retraces.advance(static_cast<std::uint64_t>(elapsed.count()));
                if (due != 0) (void)policy.offer_retraces(due); // Drop newest when full.
            }
            const auto action = policy.poll();
            switch (action.kind) {
            case isle::host::ActionKind::update:
                lock.unlock();
                try { scene->Advance(); }
                catch (...) {
                    lock.lock();
                    failure = std::current_exception();
                    (void)policy.abandon_update();
                    changed.notify_all();
                    break;
                }
                lock.lock();
                ticks = scene->ticks;
                (void)policy.finish_update();
                break;
            case isle::host::ActionKind::draw:
                lock.unlock();
                try { scene->BuildDraw(wrappers[action.token.context]); }
                catch (...) {
                    lock.lock();
                    failure = std::current_exception();
                    (void)policy.abandon_draw(action.token);
                    changed.notify_all();
                    break;
                }
                lock.lock();
                if (policy.stopping()) {
                    (void)policy.abandon_draw(action.token);
                    break;
                }
                if (count == queue.size()) std::abort(); // At most two live context leases.
                queue[(head + count) % queue.size()] = {action.token, wrappers[action.token.context].data()};
                ++count;
                if (policy.accept_draw(action.token) != Error::none) std::abort();
                changed.notify_all();
                break;
            case isle::host::ActionKind::paused:
                changed.notify_all();
                break;
            case isle::host::ActionKind::stopped:
                StopScene(lock, *scene);
                changed.notify_all();
                changed.wait(lock, [this] { return release_assets; });
                lock.unlock();
                scene.reset(); // C users and borrowed GPU/cache accesses already ceased.
                return;
            case isle::host::ActionKind::wait:
                if (policy.stopping()) changed.wait(lock);
                else changed.wait_for(lock, std::chrono::nanoseconds(retraces.nanoseconds_until_next()));
                break;
            }
        }
    }
};

IntroProbe::IntroProbe(const isle::rom::Snapshot& snapshot) : impl(std::make_unique<Impl>(snapshot)) {}
IntroProbe::~IntroProbe() {
    try {
        StopGame();
        std::unique_lock lock(impl->mutex);
        for (const auto borrowed : impl->borrowed) if (borrowed) std::abort();
        impl->changed.wait(lock, [this] { return impl->policy.stopped(); });
        impl->release_assets = true;
        impl->changed.notify_all();
        lock.unlock();
        impl->owner.join();
    } catch (...) { std::abort(); }
}
void IntroProbe::Start() {
    std::lock_guard lock(impl->mutex);
    if (impl->started || impl->policy.stopping()) throw std::logic_error("Intro owner cannot restart");
    impl->started = true;
    impl->changed.notify_all();
}
bool IntroProbe::TakeFrame(Frame& frame) {
    std::lock_guard lock(impl->mutex);
    if (impl->count == 0) return false;
    frame = impl->queue[impl->head];
    impl->head = (impl->head + 1) % impl->queue.size();
    --impl->count;
    impl->borrowed[frame.token.context] = true;
    return true;
}
isle::host::ClockError IntroProbe::CompleteFrame(isle::host::FrameToken token, isle::host::Terminal terminal) {
    std::lock_guard lock(impl->mutex);
    if (token.context >= impl->borrowed.size() || !impl->borrowed[token.context])
        return Impl::Error::not_ready;
    const auto result = impl->policy.offer_terminal(token, terminal);
    if (result == Impl::Error::none) {
        impl->borrowed[token.context] = false;
        impl->changed.notify_all();
    }
    return result;
}
void IntroProbe::RequestStop() {
    std::lock_guard lock(impl->mutex);
    impl->policy.request_stop();
    impl->changed.notify_all();
}
void IntroProbe::StopGame() {
    std::unique_lock lock(impl->mutex);
    impl->policy.request_stop();
    while (impl->count != 0) {
        const auto token = impl->queue[impl->head].token;
        impl->head = (impl->head + 1) % impl->queue.size();
        --impl->count;
        if (impl->policy.offer_terminal(token, isle::host::Terminal::cancelled) != Impl::Error::none)
            std::abort();
    }
    impl->changed.notify_all();
    impl->changed.wait(lock, [this] { return impl->game_stopped; });
}
isle::host::ClockError IntroProbe::RequestPause() {
    std::lock_guard lock(impl->mutex);
    const auto result = impl->policy.request_pause();
    impl->changed.notify_all();
    return result;
}
isle::host::ClockError IntroProbe::Resume() {
    std::lock_guard lock(impl->mutex);
    const auto result = impl->policy.resume();
    impl->changed.notify_all();
    return result;
}
bool IntroProbe::Paused() const {
    std::lock_guard lock(impl->mutex);
    return impl->policy.paused();
}
isle::host::ClockError IntroProbe::SetIntervals(std::uint16_t update, std::uint16_t draw) {
    std::lock_guard lock(impl->mutex);
    const auto result = impl->policy.set_intervals(update, draw);
    impl->changed.notify_all();
    return result;
}
std::uint32_t IntroProbe::Ticks() const {
    std::lock_guard lock(impl->mutex);
    return impl->ticks;
}
isle::host::ClockStats IntroProbe::Stats() const {
    std::lock_guard lock(impl->mutex);
    return impl->policy.stats();
}
void IntroProbe::Check() const {
    std::lock_guard lock(impl->mutex);
    if (impl->failure) std::rethrow_exception(impl->failure);
    if (impl->policy.failed()) throw std::runtime_error("Intro GTL task/clock failed");
}

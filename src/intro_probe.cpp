#include "intro_probe.h"

#include "35mmisle/engine.h"
#include "35mmisle/rom.hpp"
#include "35mmisle/sprite.hpp"
#include "35mmisle/vpk0.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <span>
#include <stdexcept>
#include <streambuf>
#include <utility>
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

class SnapshotBuffer final : public std::streambuf {
  public:
    explicit SnapshotBuffer(std::span<const std::uint8_t> bytes) {
        // basic_streambuf's input API predates const buffers. Only an istream
        // is exposed; no put area or write operation exists for this snapshot.
        char* first = const_cast<char*>(reinterpret_cast<const char*>(bytes.data()));
        setg(first, first, first + bytes.size());
    }
};

std::vector<std::uint8_t> ReadSnapshot(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::in | std::ios::binary);
    if (!input.is_open()) {
        throw std::runtime_error("Cannot open private ROM for a read-only snapshot");
    }
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(isle::rom::us_size));
    input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (input.gcount() != static_cast<std::streamsize>(bytes.size()) || input.bad() || input.fail()) {
        throw std::runtime_error("Private ROM snapshot is unreadable or shorter than exactly 16 MiB");
    }
    char extra = 0;
    input.read(&extra, 1);
    if (input.gcount() != 0 || input.bad() || !input.eof()) {
        throw std::runtime_error("Private ROM snapshot is unreadable or larger than exactly 16 MiB");
    }
    return bytes;
}

isle::sprite::BankOwner LoadBank(const std::filesystem::path& path) {
    // The sole file read ends here. Identity and decoding see the same immutable
    // bounded bytes; replacing the path later cannot change the admitted data.
    const auto snapshot = ReadSnapshot(path);
    SnapshotBuffer buffer(snapshot);
    std::istream input(&buffer);
    const auto identity = isle::rom::validate_us(input);
    if (!identity) {
        throw std::runtime_error("Private ROM rejected: exact normalized US identity is required before decoding");
    }
    std::cout << "INTRO_ROM_IDENTITY: exact US image; size=" << identity.identity.size
              << "; normalized SHA1=" << isle::rom::digest_hex(identity.identity) << '\n';

    // ByteOrder is established by isle-rom's content magic and whole-image hash.
    // Select canonical bytes from this snapshot, not from another file opening.
    std::size_t byteXor = 0;
    switch (identity.identity.order) {
    case isle::rom::ByteOrder::z64: break;
    case isle::rom::ByteOrder::v64: byteXor = 1; break;
    case isle::rom::ByteOrder::n64: byteXor = 3; break;
    }
    std::array<std::byte, introRomEnd - introRomStart> canonicalStorage;
    std::span<const std::byte> compressed =
        std::as_bytes(std::span(snapshot)).subspan(introRomStart, introRomEnd - introRomStart);
    if (byteXor != 0) {
        for (std::size_t i = 0; i < canonicalStorage.size(); ++i) {
            canonicalStorage[i] = static_cast<std::byte>(snapshot[(introRomStart + i) ^ byteXor]);
        }
        compressed = canonicalStorage;
    }
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

struct IntroProbe::Impl {
    isle::sprite::BankOwner bank;
    isle::sprite::Sprite asset;
    std::array<std::vector<std::uint8_t>, bitmapCount> planes;
    std::array<Gfx, 256> commands{};
    Gfx* modulation = nullptr;
    // Last owning member is destroyed first, before borrowed asset/texel owners.
    NativeGame world;
    IsleGameObject* object = nullptr;
    IsleGameSprite* sprite = nullptr;
    IsleSprite* view = nullptr;
    std::uint32_t ticks = 0;

    explicit Impl(const std::filesystem::path& path) : bank(LoadBank(path)) {
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
                     "Fixed-step experiment: initial state presented first, one C update between accepted submissions; "
                     "60 presentation frames before the selected original 20-update RGB fade (alpha is not faded).\n"
                     "Not full intro entry, scene boot, persistence/save, audio, photo fidelity, full Sprite/RDP parity, "
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
        modulation = out;
        gDPSetPrimColor(out++, 0, 0, view->red, view->green, view->blue, view->alpha);

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
        // Fixed layout above emits 195 commands, within the resident 256 slots.
    }
};

IntroProbe::IntroProbe(const std::filesystem::path& romPath) : impl(std::make_unique<Impl>(romPath)) {}
IntroProbe::~IntroProbe() = default;
Gfx* IntroProbe::Commands() noexcept { return impl->commands.data(); }
std::uint32_t IntroProbe::Ticks() const noexcept { return impl->ticks; }

void IntroProbe::Advance() {
    if (impl->world.Get() == nullptr) {
        throw std::runtime_error("Cannot advance a stopped intro world");
    }
    if (impl->ticks + 1 == holdTicks) {
        IsleGameProcess* fade = nullptr;
        impl->world.Require(isle_game_add_intro_fade(impl->world.Get(), impl->object, impl->sprite,
                                                    fadeTicks, 5, &fade));
    }
    impl->world.Require(isle_game_tick(impl->world.Get()));
    ++impl->ticks;
    gDPSetPrimColor(impl->modulation, 0, 0, impl->view->red, impl->view->green,
                   impl->view->blue, impl->view->alpha);
    if (impl->ticks == holdTicks || impl->ticks == holdTicks + fadeTicks - 1) {
        std::cout << "INTRO_C_FADE_STATE: tick=" << impl->ticks
                  << "; RGB=" << static_cast<unsigned>(impl->view->red) << ','
                  << static_cast<unsigned>(impl->view->green) << ','
                  << static_cast<unsigned>(impl->view->blue)
                  << "; alpha=" << static_cast<unsigned>(impl->view->alpha) << '\n';
    }
}

void IntroProbe::StopGame() {
    if (impl->world.Get() != nullptr) {
        impl->world.Stop();
        impl->object = nullptr;
        impl->sprite = nullptr;
        impl->view = nullptr;
        std::cout << "INTRO_C_USERS_QUIESCENT: game destroyed and continuations joined; "
                     "arena, canonical bank, normalized pixels and display lists still retained for renderer shutdown.\n";
    }
}

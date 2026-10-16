#include "scene_graphics.hpp"
#include "graphics_task.hpp"
#include "menu_scene_bridge.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>


namespace isle::scene::graphics {
using startup::View;
namespace {
namespace sp = ::isle::sprite;
namespace wire = ::isle::graphics;
constexpr std::uint32_t width = 320, height = 240, target_bytes = width * height * 2;
constexpr std::uint32_t top = 0x100000, top_end = 0x10fc00;
constexpr std::uint32_t identity = 0x10fc00, reset_viewport = 0x10fc40;
constexpr std::uint32_t fade_viewport = 0x1f00c0;
constexpr std::uint32_t stack = 0x280000, yield = 0x284000, task_address = 0x286000, boot = 0x287000;
constexpr sp::RamRange sprite_commands{0x150000, 0x50000};
constexpr std::uint32_t effect_commands = 0x1a0000, effect_assets = 0x1b0000, effect_end = 0x1f0000;
static_assert((menu::EffectRuntime::max_render_words + 2) * sizeof(std::uint32_t) <=
              effect_assets - effect_commands, "FX packet plus caller END exceeds command arena");
constexpr std::uint32_t fade_projection = 0x1f0000, fade_view = 0x1f0040, fade_scale = 0x1f0080;
constexpr std::uint32_t rsp_output = 0x200000, output_end = 0x240000, depth = 0x600000;
constexpr std::uint32_t fade_list = 0xaecb0, fade_vertices = 0xaec70;
constexpr std::array<menu::RamRange, 11> runtime_ranges{{
    {0, wire::bss_end}, {top, 0x10000}, {0x110000, 0x10000},
    {0x120000, 0x10000}, {0x130000, 0x10000}, {0x280000, 0x10000},
    // Selected effect pixels are immutable union members, not writable
    // reservations. Only the command arena ends at effect_assets.
    {sprite_commands.address, sprite_commands.size}, {effect_commands, effect_assets - effect_commands},
    {fade_projection, 0x10000}, {rsp_output, output_end - rsp_output}, {depth, target_bytes}}};
struct Failure { Error error; const char* message; };
bool intersects(sp::RamRange a, sp::RamRange b) noexcept {
    return a.size && b.size && a.address < std::uint64_t(b.address) + b.size &&
           b.address < std::uint64_t(a.address) + a.size;
}
void require(bool ok, const char* message, Error error = Error::invalid) {
    if (!ok) throw Failure{error, message};
}
void native(IsleGameResult value) {
    require(value == ISLE_GAME_OK, "startup graphics native view rejected", Error::native);
}
std::uint32_t physical(std::uint32_t guest) {
    require(guest < menu::rdram_size || (guest & 0xff800000u) == 0x80000000u,
            "startup graphics nonphysical source identity", Error::resource);
    return guest & 0x1fffffffu;
}
std::span<const std::uint8_t> bytes(std::span<const std::byte> value) {
    return {reinterpret_cast<const std::uint8_t*>(value.data()), value.size()};
}
void diagnostic(std::span<char> output, const char* message) noexcept {
    if (message == nullptr) message = "startup graphics failure";
    std::size_t n = 0;
    while (n + 1 < output.size() && message[n]) { output[n] = message[n]; ++n; }
    output[n] = '\0';
}
std::uint32_t pair(unsigned x, unsigned y) { return (x << 12) | y; }
std::uint32_t packed_color(std::uint32_t rgba) {
    const auto value = ((rgba >> 16) & 0xf800u) | ((rgba >> 13) & 0x7c0u) |
                       ((rgba >> 10) & 0x3eu) | ((rgba >> 7) & 1u);
    return (value << 16) | value;
}
IsleGameViewport full_viewport() {
    IsleGameViewport vp{};
    vp.scale[0] = vp.translate[0] = static_cast<std::int16_t>(width * 2);
    vp.scale[1] = vp.translate[1] = static_cast<std::int16_t>(height * 2);
    vp.scale[2] = vp.translate[2] = 511;
    return vp;
}
sp::Bounds bounds(const IsleGameViewport& vp) {
    const sp::Bounds result{vp.translate[0] / 4 - vp.scale[0] / 4,
        vp.translate[1] / 4 - vp.scale[1] / 4,
        vp.translate[0] / 4 + vp.scale[0] / 4,
        vp.translate[1] / 4 + vp.scale[1] / 4};
    require(result.left >= 0 && result.top >= 0 && result.right <= int(width) &&
            result.bottom <= int(height) && result.left < result.right && result.top < result.bottom,
            "startup source viewport exceeds source GPU target", Error::preparation);
    return result;
}
void viewport(wire::Memory& memory, std::uint32_t at, const IsleGameViewport& vp) {
    static_cast<void>(bounds(vp));
    for (unsigned i = 0; i < 4; ++i) {
        memory.half(at + i * 2, static_cast<std::uint16_t>(vp.scale[i]));
        memory.half(at + 8 + i * 2, static_cast<std::uint16_t>(vp.translate[i]));
    }
}
struct Commands {
    wire::Memory& memory;
    std::uint32_t cursor = top;
    std::uint32_t end = top_end;
    void command(std::uint32_t a, std::uint32_t b) {
        require(cursor <= end - 8, "scene command list exhausted", Error::preparation);
        memory.word(cursor, a); memory.word(cursor + 4, b); cursor += 8;
    }
    void call(std::uint32_t address) { command(0xde000000, address); }
    void mode(bool high, unsigned shift, unsigned length, std::uint32_t value) {
        command((high ? 0xe3000000u : 0xe2000000u) |
                ((32 - shift - length) << 8) | (length - 1), value);
    }
    void scissor(sp::Bounds b) {
        command(0xed000000u | pair(unsigned(b.left) * 4, unsigned(b.top) * 4),
                pair(unsigned(b.right) * 4, unsigned(b.bottom) * 4));
    }
    void fill(sp::Bounds b) {
        command(0xf6000000u | pair(unsigned(b.right - 1) * 4, unsigned(b.bottom - 1) * 4),
                pair(unsigned(b.left) * 4, unsigned(b.top) * 4));
    }
    void camera(std::uint32_t vp_address, const IsleGameViewport& vp,
                unsigned flags, std::uint32_t rgba, unsigned lane, std::uint32_t color) {
        const auto clipped = bounds(vp); // Global renCameraScis* are all source-initialized zero.
        command(0xdc080008, vp_address);
        scissor(clipped);
        if (flags & 1u) {
            command(0xe7000000, 0); mode(true, 20, 2, 0x300000);
            mode(false, 3, 29, 0); command(0xff10013f, depth);
            command(0xf7000000, 0xfffcfffc); fill(clipped);
        }
        command(0xe7000000, 0); command(0xff10013f, color);
        if (flags & 2u) {
            mode(true, 20, 2, 0x300000); mode(false, 3, 29, 0);
            command(0xf7000000, packed_color(rgba)); fill(clipped);
        }
        command(0xe7000000, 0); mode(true, 20, 2, 0);
        mode(false, 3, 29, (lane & 1u) ? 0x005049d8u : 0x00552078u);
    }
    void reset(std::uint32_t color, std::uint32_t pre_render) {
        command(0xdb060000, 0); command(0xdb06003c, color); command(0xfe000000, depth);
        command(0xe7000000, 0); command(0xdc080008, reset_viewport);
        constexpr std::uint32_t cleared = 1u | 4u | 0x600u | 0x10000u | 0x20000u |
                                          0x40000u | 0x80000u | 0x100000u | 0x200000u;
        command(0xd9000000u | ((~cleared) & 0xffffffu), 0);
        command(0xdb040004, 1); command(0xdb04000c, 1);
        command(0xdb040014, 0xffff); command(0xdb04001c, 0xffff);
        command(0xd7000000, 0); command(0xd9ffffff, 0x200405);
        command(0xda380007, identity); command(0xda380003, identity);
        mode(true, 20, 2, 0); mode(true, 23, 1, 0);
        command(0xfcffffff, 0xfffe793c); // Public SHADE combiner in both cycles.
        mode(true, 16, 1, 0); mode(true, 14, 2, 0); mode(true, 17, 2, 0);
        mode(true, 19, 1, 0x80000); mode(true, 12, 2, 0x2000);
        mode(true, 9, 3, 0xc00); mode(true, 8, 1, 0);
        mode(false, 0, 2, 0); mode(false, 3, 29, 0x0f0a4000);
        mode(true, 6, 2, 0); command(0xe7000000, 0);
        scissor({0, 0, int(width), int(height)});
        call(pre_render); // GAME global callback follows ordinary reset.
    }
};
struct SceneAssets {
    std::unique_ptr<sp::Producer> sprites;
    std::vector<sp::AssetSpan> installation;
    std::vector<sp::RamRange> reservations;
};
// A source alias is accepted only when it names the identical immutable byte,
// not merely a permissive reservation or coincidentally equal content. Split
// overlapping extents into a disjoint union, so every physical byte copies once.
void disjoint(std::vector<sp::AssetSpan>& spans) {
    std::sort(spans.begin(), spans.end(), [](const auto& a, const auto& b) {
        return a.address < b.address || (a.address == b.address && a.bytes.size() > b.bytes.size());
    });
    std::size_t count = 0;
    for (auto item : spans) {
        require(item.address <= menu::rdram_size && item.bytes.size() <= menu::rdram_size - item.address,
                "startup immutable span exceeds RAM", Error::resource);
        if (item.bytes.empty()) continue;
        if (count) {
            const auto& previous = spans[count - 1];
            const auto end = previous.address + previous.bytes.size();
            if (item.address < end) {
                const auto overlap = std::min(end - item.address, item.bytes.size());
                const auto offset = item.address - previous.address;
                require(previous.bytes.data() + offset == item.bytes.data() &&
                        std::equal(item.bytes.begin(), item.bytes.begin() + overlap,
                                   previous.bytes.begin() + offset),
                        "startup immutable physical aliases have different backing identities", Error::resource);
                item.address += static_cast<std::uint32_t>(overlap);
                item.bytes = item.bytes.subspan(overlap);
            }
        }
        if (!item.bytes.empty()) spans[count++] = item;
    }
    spans.resize(count);
}
const IsleGameCameraParameters& camera_parameters(IsleGame* game, IsleGameCamera* camera) {
    const IsleGameCameraParameters* parameters = nullptr;
    require(camera != nullptr, "startup native source camera is missing", Error::native);
    native(isle_game_camera_parameters(game, camera, &parameters));
    require(parameters != nullptr, "startup camera parameter borrow is empty", Error::native);
    return *parameters;
}
menu::EffectCamera effect_camera(IsleGame* game, IsleGameCamera* camera) {
    const auto& parameters = camera_parameters(game, camera);
    native(isle_game_camera_compute_all(game, camera));
    const IsleGameCameraMatrix *projection = nullptr, *view = nullptr;
    native(isle_game_camera_matrix(game, camera, 0, &projection));
    native(isle_game_camera_matrix(game, camera, 1, &view));
    require(projection && view && projection->kind == ISLE_GAME_CAMERA_PERSPECTIVE_FAST &&
            view->kind == ISLE_GAME_CAMERA_LOOK_AT_ROLL, "startup FX needs actual source kind3/kind8 camera", Error::native);
    menu::EffectCamera output;
    for (unsigned r = 0; r < 4; ++r) for (unsigned c = 0; c < 4; ++c) {
        output.projection[r][c] = ((view->floating[r][0] * projection->floating[0][c] +
            view->floating[r][1] * projection->floating[1][c]) +
            view->floating[r][2] * projection->floating[2][c]) +
            view->floating[r][3] * projection->floating[3][c];
    }
    for (unsigned k = 0; k < 3; ++k) {
        output.viewport_scale[k] = parameters.viewport.scale[k];
        output.viewport_translate[k] = parameters.viewport.translate[k];
    }
    output.depth_test = true;
    return output;
}
} // namespace

struct Frame::Impl {
    const menu::Catalog* catalog;
    const rom::Snapshot& snapshot;
    const std::byte* source;
    std::array<SceneAssets, 2> scenes;
    struct EffectBankAssets {
        struct ImageAddresses { std::vector<std::uint32_t> frames, palettes; };
        std::vector<ImageAddresses> addresses;
        std::vector<menu::EffectImageBinding> images;
    };
    std::vector<EffectBankAssets> effect_bank_assets;
    std::vector<menu::EffectBankBinding> effect_banks;
    std::vector<menu::RamRange> image_ranges;
    std::vector<sp::Draw> draws;
    std::vector<scene::SpriteDraw> startup_sprites;
    std::vector<std::byte> rgba = std::vector<std::byte>(width * height * 4);
    sp::Producer empty_sprites{std::span<const IsleSprite* const>{}, std::span<const sp::BankMapping>{}};
    std::vector<sp::RamRange> composition_reservations, command_reservations;
    std::uint64_t installed_identity = 0;
    sp::Producer* installed_producer = nullptr;
    std::optional<sp::Producer::Borrow> graphics_borrow;
    // The executor dies before immutable source/resource storage is released.
    rsp::CanonicalExecutor executor;
    IsleGameVec3 cached_fade_scale{};
    IsleGameFixedMatrix cached_fade_matrix{};
    bool have_fade_scale = false;
    rsp::CanonicalExecutor::Token token = 0;
    std::uint64_t epoch = 0, ordinal = 0;
    Error error = Error::none;
    std::array<char, 512> message{};
    save::ScreenOffsets offsets{};
    std::uint32_t color = 0;
    bool cold = true, resident = false, ready = false, retired = false;

    void install_effects(std::span<const menu::EffectBank* const> selected,
                         std::vector<sp::AssetSpan>& spans) {
        struct Payload {
            std::size_t begin, end;
            std::uint32_t* address;
        };
        std::vector<Payload> payloads;
        std::array<bool, 8> occupied{};
        effect_bank_assets.resize(selected.size());
        effect_banks.resize(selected.size());
        const auto source_address = reinterpret_cast<std::uintptr_t>(source);
        const auto source_size = snapshot.canonical().size();
        const auto collect = [&](const rom::GuestView& value, std::uint32_t& address) {
            const auto at = reinterpret_cast<std::uintptr_t>(value.bytes.data());
            require(!value.bytes.empty() && at >= source_address &&
                    at - source_address <= source_size &&
                    value.bytes.size() <= source_size - (at - source_address),
                    "startup FX payload does not borrow its retained Snapshot", Error::resource);
            const auto begin = static_cast<std::size_t>(at - source_address);
            payloads.push_back({begin, begin + value.bytes.size(), &address});
        };
        for (std::size_t bank = 0; bank < selected.size(); ++bank) {
            const auto* resource = selected[bank];
            require(resource && resource->bank_id < occupied.size() && !occupied[resource->bank_id],
                    "startup FX active bank identities overlap or exceed source slots", Error::resource);
            occupied[resource->bank_id] = true;
            auto& storage = effect_bank_assets[bank];
            storage.addresses.resize(resource->images.size());
            storage.images.resize(resource->images.size());
            for (std::size_t image = 0; image < resource->images.size(); ++image) {
                const auto& input = resource->images[image];
                auto& output = storage.addresses[image];
                output.frames.resize(input.frames.size());
                output.palettes.resize(input.palettes.size());
                for (std::size_t frame = 0; frame < input.frames.size(); ++frame)
                    collect(input.frames[frame], output.frames[frame]);
                for (std::size_t palette = 0; palette < input.palettes.size(); ++palette)
                    collect(input.palettes[palette], output.palettes[palette]);
                storage.images[image] = {output.frames, output.palettes};
            }
            effect_banks[bank] = {resource->bank_id, storage.images};
        }
        std::sort(payloads.begin(), payloads.end(), [](const Payload& a, const Payload& b) {
            return a.begin < b.begin || (a.begin == b.begin && a.end < b.end);
        });
        auto next_asset = effect_assets;
        for (std::size_t first = 0; first < payloads.size();) {
            const auto begin = payloads[first].begin;
            auto end = payloads[first].end;
            auto last = first + 1;
            // Preserve source identity for exact, nested and partial aliases.
            // One borrowed union upload backs every overlapping payload view.
            while (last < payloads.size() && payloads[last].begin < end) {
                end = std::max(end, payloads[last].end);
                ++last;
            }
            const auto extent = end - begin;
            require(next_asset <= effect_end && extent <= effect_end - next_asset,
                    "startup selected FX assets exceed reservation", Error::resource);
            const auto padded = (extent + 7u) & ~std::size_t(7u);
            require(padded <= effect_end - next_asset,
                    "startup selected FX asset alignment exceeds reservation", Error::resource);
            for (auto item = first; item < last; ++item) {
                const auto offset = payloads[item].begin - begin;
                require(!(offset & 7u), "startup FX payload alias is not texture aligned", Error::resource);
                *payloads[item].address = next_asset + static_cast<std::uint32_t>(offset);
            }
            spans.push_back({next_asset, bytes(snapshot.canonical().subspan(begin, extent))});
            image_ranges.push_back({next_asset, extent});
            next_asset += static_cast<std::uint32_t>(padded);
            first = last;
        }
    }
    Impl(const menu::Catalog* c, const rom::Snapshot& s, Config config)
        : catalog(c), snapshot(s), source(s.canonical().data()), executor(config.require_cpu, config.limits) {
        require(source != nullptr && snapshot.canonical().size() == rom::us_size,
                "startup graphics requires an admitted stable Snapshot", Error::resource);
        if (!catalog) return;
        const rom::GuestView fade_wire{0x800aecb0, 0x5a660, snapshot.canonical().subspan(0x5a660, 0x30)};
        require(fade_wire.u32(24) == 0x01004008 && fade_wire.u32(28) == 0x800aec70 &&
                fade_wire.u32(40) == 0xdf000000 && fade_wire.u32(44) == 0,
                "startup private source fade closure is malformed", Error::resource);
        std::vector<sp::AssetSpan> effect_spans;
        // Startup installs only intro. The installation/binding domain itself
        // is bank-qualified and can admit any selected immutable bank set.
        const std::array<const menu::EffectBank*, 1> selected{&catalog->effects()};
        install_effects(selected, effect_spans);
        for (std::size_t index = 0; index < scenes.size(); ++index) {
            const auto scene = index == 0 ? menu::Scene::logo : menu::Scene::title_menu;
            auto& result = scenes[index];
            const auto& screen = catalog->screen(scene);
            require(screen.width == width && screen.height == height &&
                    screen.allocated_depth_bytes == target_bytes &&
                    physical(screen.framebuffer_guests[0]) == 0x3b5000 &&
                    physical(screen.framebuffer_guests[1]) == 0x3da800 && screen.framebuffer_guests[2] == 0,
                    "startup source framebuffer layout differs", Error::resource);
            const auto roots = catalog->sprites(scene);
            std::vector<const IsleSprite*> resources;
            resources.reserve(roots.size());
            for (const auto& root : roots) resources.push_back(root.sprite.view());
            const auto& bank = catalog->bank(scene);
            const std::array<sp::BankMapping, 1> banks{{{bank.owner, bank.guest_begin}}};
            result.sprites = std::make_unique<sp::Producer>(resources, banks);
            const auto plan = catalog->plan_install(scene, runtime_ranges);
            require(bool(plan.status), "startup selected Catalog closure collides with runtime RAM", Error::resource);
            for (const auto& span : plan.spans)
                result.installation.push_back({physical(span.guest_address), bytes(span.bytes)});
            for (const auto& span : result.sprites->assets()) result.installation.push_back(span);
            if (scene == menu::Scene::title_menu) {
                result.installation.insert(result.installation.end(), effect_spans.begin(), effect_spans.end());
                result.installation.push_back({fade_list, bytes(fade_wire.bytes)});
                result.installation.push_back({fade_vertices, bytes(snapshot.canonical().subspan(0x5a620, 0x40))});
            }
            disjoint(result.installation);
            for (const auto range : runtime_ranges) {
                const sp::RamRange converted{static_cast<std::uint32_t>(range.offset), static_cast<std::uint32_t>(range.size)};
                result.reservations.push_back(converted);
            }
            for (unsigned i = 0; i < 2; ++i) {
                const sp::RamRange framebuffer{physical(screen.framebuffer_guests[i]), target_bytes};
                result.reservations.push_back(framebuffer);
            }
        }
        draws.reserve(std::max(catalog->setup(menu::Scene::logo).sprites,
                               catalog->setup(menu::Scene::title_menu).sprites));
        startup_sprites.reserve(draws.capacity());
    }
    void fail(Error value, const char* reason) noexcept {
        for (auto& draw : draws) draw = {};
        draws.clear();
        for (auto& sprite : startup_sprites) sprite = {};
        startup_sprites.clear();
        ready = false;
        if (error == Error::none) { error = value; diagnostic(message, reason); }
        if (executor.quiescent()) token = 0;
        if (executor.quiescent()) graphics_borrow.reset();
    }
    void fade(View view, wire::Memory& memory, Commands& commands) {
        if (view.fade_camera == nullptr || view.services == nullptr) return;
        const auto state = view.services->fade();
        if (!state.visible) return;
        const auto* transform = view.services->fade_transform();
        require(transform != nullptr, "startup visible fade has no live source pose", Error::native);
        const auto& parameters = camera_parameters(view.game, view.fade_camera);
        native(isle_game_camera_compute_all(view.game, view.fade_camera));
        const IsleGameCameraMatrix *projection = nullptr, *look_at = nullptr;
        native(isle_game_camera_matrix(view.game, view.fade_camera, 0, &projection));
        native(isle_game_camera_matrix(view.game, view.fade_camera, 1, &look_at));
        require(projection && look_at && projection->kind == ISLE_GAME_CAMERA_PERSPECTIVE_FAST &&
                look_at->kind == ISLE_GAME_CAMERA_LOOK_AT,
                "startup fade needs actual source kind3/kind6 camera", Error::native);
        std::uint16_t normalization = 0;
        native(isle_game_camera_perspective_normalization(view.game, view.fade_camera, &normalization));
        require(transform->position.x == 0 && transform->position.y == 0 && transform->position.z == 0 &&
                transform->rotation.x == 0 && transform->rotation.y == 0 && transform->rotation.z == 0 &&
                std::isfinite(transform->scale.x) && std::isfinite(transform->scale.y) &&
                std::isfinite(transform->scale.z) && transform->scale.x > 0 &&
                transform->scale.y > 0 && transform->scale.z > 0,
                "startup fade separate source T/R/S pose differs", Error::native);
        // Source uses separate identity translation/rotation and a scale matrix,
        // NOT kind28's quantized scale*256 combined RTS payload.
        if (!have_fade_scale || cached_fade_scale.x != transform->scale.x ||
            cached_fade_scale.y != transform->scale.y || cached_fade_scale.z != transform->scale.z) {
            native(isle_menu_scale_matrix(&transform->scale, &cached_fade_matrix));
            cached_fade_scale = transform->scale;
            have_fade_scale = true;
        }
        memory.matrix(fade_projection, projection->fixed); memory.matrix(fade_view, look_at->fixed);
        memory.matrix(fade_scale, cached_fade_matrix); viewport(memory, fade_viewport, parameters.viewport);
        commands.camera(fade_viewport, parameters.viewport, 0, 0, 0, color);
        commands.command(0xda380007, fade_projection); commands.command(0xdb0e0000, normalization);
        commands.command(0xda380005, fade_view); commands.command(0xda380003, identity);
        commands.command(0xe7000000, 0);
        commands.command(0xda380000, fade_scale); // Source root pushes; caller pops after immutable quad.
        commands.command(0xfa000000, (std::uint32_t(state.rgb[0]) << 24) |
            (std::uint32_t(state.rgb[1]) << 16) | (std::uint32_t(state.rgb[2]) << 8) | state.alpha);
        commands.mode(false, 3, 29, 0x00504240); // XLU_SURF + XLU_SURF2, no Z compare/write.
        commands.call(fade_list); commands.command(0xd8380002, 64);
    }
    struct StartupPreparation { Impl* owner; View view; };
    static bool title_pass(void* userdata, std::span<std::uint32_t> ram, std::uint64_t epoch,
                           const render::Pass&, render::Prepared& result) {
        const auto& context = *static_cast<StartupPreparation*>(userdata);
        auto prepared = context.view.title->models()->prepare(ram, epoch);
        require(bool(prepared), "startup native model/material preparation rejected", Error::preparation);
        result.camera = {prepared.frame.camera.address, prepared.frame.camera.commands * 2u};
        for (unsigned i = 0; i < 4; ++i)
            result.lanes[i] = {prepared.frame.lanes[i].address, prepared.frame.lanes[i].commands * 2u};
        return true;
    }
    static bool effect_pass(void* userdata, std::span<std::uint32_t> ram, std::uint64_t,
                            const render::Pass&, render::Prepared& result) {
        const auto& context = *static_cast<StartupPreparation*>(userdata);
        auto& owner = *context.owner;
        const auto camera = effect_camera(context.view.game, context.view.title->camera());
        require(effect_commands / 4 <= ram.size() &&
                menu::EffectRuntime::max_render_words + 2 <= ram.size() - effect_commands / 4,
                "startup FX packet and caller END exceed RAM", Error::preparation);
        const auto rendered = context.view.title->effects()->render(camera,
            {ram.subspan(effect_commands / 4, menu::EffectRuntime::max_render_words),
             owner.effect_banks, owner.image_ranges});
        require(bool(rendered.status) && rendered.words <= menu::EffectRuntime::max_render_words &&
                !(rendered.words & 1u), "startup actual native FX render rejected", Error::preparation);
        wire::Memory memory(ram);
        memory.word(effect_commands + static_cast<std::uint32_t>(rendered.words * 4), 0xdf000000);
        memory.word(effect_commands + static_cast<std::uint32_t>(rendered.words * 4 + 4), 0);
        result.lanes[0] = {effect_commands, rendered.words + 2};
        return true;
    }
    static bool fade_pass(void* userdata, std::span<std::uint32_t> ram, std::uint64_t,
                          const render::Pass&, render::Prepared& result) {
        const auto& context = *static_cast<StartupPreparation*>(userdata);
        wire::Memory memory(ram);
        constexpr std::uint32_t begin = 0x1f0100;
        Commands commands{memory, begin, 0x1f1000};
        context.owner->fade(context.view, memory, commands);
        commands.command(0xdf000000, 0);
        result.lanes[0] = {begin, (commands.cursor - begin) / 4};
        return true;
    }
    void prepare(View view, FrameContext context, rsp::CanonicalExecutor::PreparationLease& preparation) {
        require(catalog && view.game && view.sprite_camera && context.buffer_index < 2,
                "startup adapter requires actual source resources/camera/context");
        const auto index = view.resources == menu::Scene::logo ? 0u : 1u;
        auto& assets = scenes[index];
        const auto& screen = catalog->screen(view.resources);
        std::size_t count = 0;
        if (view.resources == menu::Scene::logo) {
            startup_sprites.resize(view.logo_sprite ? 1 : 0);
            if (view.logo_sprite) {
                IsleSprite* scalar = nullptr;
                native(isle_game_sprite_view(view.game, view.logo_sprite, &scalar));
                const auto roots = catalog->sprites(view.resources);
                const auto root = std::find_if(roots.begin(), roots.end(), [](const auto& value) {
                    return value.identity == 0x802bc0c0;
                });
                require(root != roots.end(), "startup logo resource missing", Error::resource);
                startup_sprites[0] = {{static_cast<std::size_t>(root - roots.begin()), scalar},
                                     nullptr, {1,0x80000000u,UINT32_MAX}};
                count = 1;
            }
        } else {
            require(view.services != nullptr, "startup menu services missing");
            startup_sprites.resize(view.services->sprite_count());
            require(view.services->sprite_draws(startup_sprites, count),
                    "startup ordered native sprite draw borrow failed", Error::native);
        }
        StartupPreparation startup{this, view};
        std::array<render::Pass, 5> passes{};
        std::size_t used = 0;
        auto& clear = passes[used++];
        clear.kind = render::Kind::clear; clear.priority = 100;
        clear.flags = 3; clear.background_rgba = view.background_rgba;
        if (view.title && view.title->models()) {
            auto& pass = passes[used++];
            pass.camera = view.title->camera(); pass.priority = 5; pass.flags = 7;
            pass.background_rgba = 0xff; pass.prepare = title_pass; pass.userdata = &startup;
        }
        if (view.title && view.title->effects() && view.title->effects()->particle_count()) {
            auto& pass = passes[used++];
            pass.camera = view.title->camera(); pass.priority = 4;
            pass.prepare = effect_pass; pass.userdata = &startup;
        }
        auto& sprites = passes[used++];
        sprites.kind = render::Kind::sprites; sprites.camera = view.sprite_camera;
        sprites.priority = 3; sprites.flags = 8; sprites.link_mask = 2;
        sprites.camera_tag = UINT32_MAX; sprites.tag_match = render::TagMatch::equality;
        sprites.sprites = std::span<const scene::SpriteDraw>(startup_sprites).first(count);
        sprites.sprite_commands = sprite_commands;
        if (view.fade_camera && view.services && view.services->fade().visible) {
            auto& pass = passes[used++];
            pass.camera = view.fade_camera; pass.priority = 1;
            pass.prepare = fade_pass; pass.userdata = &startup;
        }
        render::Input input{view.game,
            {index + 1, assets.sprites.get(), assets.installation, assets.reservations,
             {physical(screen.framebuffer_guests[0]), physical(screen.framebuffer_guests[1])},
             physical(catalog->pre_render(view.resources).display_list.guest_address)},
            std::span<const render::Pass>(passes).first(used), view.screen_offsets};
        prepare(input, context, preparation);
        for (auto& sprite : startup_sprites) sprite = {};
        startup_sprites.clear();
    }
    void prepare(render::Input input, FrameContext context,
                 rsp::CanonicalExecutor::PreparationLease& preparation) {
        require(input.game && context.buffer_index < 2 && input.passes.size() <= 32,
                "scene graphics requires a native game and bounded source passes");
        require(snapshot.canonical().data() == source, "scene graphics Snapshot moved", Error::resource);
        auto* producer = input.resources.sprites ? input.resources.sprites : &empty_sprites;
        auto ram = preparation.words();
        wire::validate_cold_inputs(ram, snapshot, source, boot);
        composition_reservations.clear();
        constexpr std::array<sp::RamRange, 5> core_ranges{{
            {0, wire::bss_end}, {top, 0x10000}, {stack, 0x10000},
            {rsp_output, output_end - rsp_output}, {depth, target_bytes}}};
        composition_reservations.insert(composition_reservations.end(), core_ranges.begin(), core_ranges.end());
        for (const auto framebuffer : input.resources.framebuffers) {
            require(!(framebuffer & 7u) && framebuffer <= menu::rdram_size &&
                    target_bytes <= menu::rdram_size - framebuffer,
                    "scene framebuffer exceeds physical RAM", Error::resource);
            for (const auto core : core_ranges)
                require(!intersects({framebuffer, target_bytes}, core),
                        "scene framebuffer overlaps core task/depth/command storage");
            composition_reservations.push_back({framebuffer, target_bytes});
        }
        require(input.resources.framebuffers[0] + target_bytes <= input.resources.framebuffers[1] ||
                input.resources.framebuffers[1] + target_bytes <= input.resources.framebuffers[0],
                "scene source framebuffer contexts overlap", Error::resource);
        const auto protected_count = composition_reservations.size();
        require(input.resources.pre_render && input.resources.pre_render < menu::rdram_size &&
                !(input.resources.pre_render & 7u), "scene pre-render list has invalid physical identity");
        composition_reservations.insert(composition_reservations.end(), input.resources.reservations.begin(),
                                        input.resources.reservations.end());
        std::array<std::size_t, 32> order{};
        for (std::size_t i = 0; i < input.passes.size(); ++i) {
            const auto& pass = input.passes[i];
            require((pass.camera || pass.kind == render::Kind::clear) &&
                    (pass.kind != render::Kind::compiled || pass.prepare),
                    "scene pass is missing its actual camera/compiled preparation");
            if (pass.camera) static_cast<void>(bounds(camera_parameters(input.game, pass.camera).viewport));
            if (pass.kind == render::Kind::sprites) {
                for (std::size_t r = 0; r < protected_count; ++r)
                    require(!intersects(pass.sprite_commands, composition_reservations[r]),
                            "sprite commands overlap core task/framebuffer storage");
                composition_reservations.push_back(pass.sprite_commands);
            }
            require(pass.tag_match == render::TagMatch::intersection ||
                    pass.tag_match == render::TagMatch::equality, "invalid scene camera tag rule");
            order[i] = i;
        }
        // Stable source priority order, without a frame-time sorting allocation.
        for (std::size_t i = 1; i < input.passes.size(); ++i) {
            const auto item = order[i]; auto j = i;
            while (j && input.passes[order[j - 1]].priority < input.passes[item].priority) {
                order[j] = order[j - 1]; --j;
            }
            order[j] = item;
        }
        const bool changed = !resident || installed_identity != input.resources.identity ||
                             installed_producer != producer;
        require(!changed || epoch != std::numeric_limits<std::uint64_t>::max(), "scene RAM epoch exhausted");
        const auto next_epoch = epoch + (changed ? 1 : 0);
        graphics_borrow.emplace(producer->borrow());
        // Validate the complete ROM/mutable/producer/task/framebuffer union
        // before any RAM or residency write; only changed publications upload.
        producer->install_composed(ram, next_epoch, input.resources.immutable, composition_reservations);
        if (cold) wire::install_cold_inputs(ram, snapshot, boot);
        cold = false; resident = true; epoch = next_epoch;
        installed_identity = input.resources.identity; installed_producer = producer;
        color = input.resources.framebuffers[context.buffer_index];
        wire::Memory memory(ram);
        const IsleGameFixedMatrix one{{{0x10000, 0, 1, 0}, {0, 0x10000, 0, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}}};
        memory.matrix(identity, one);
        viewport(memory, reset_viewport, full_viewport());
        Commands commands{memory};
        commands.reset(color, input.resources.pre_render);
        constexpr std::array<unsigned, 4> lanes{0, 2, 1, 3};
        for (std::size_t n = 0; n < input.passes.size(); ++n) {
            const auto& pass = input.passes[order[n]];
            const auto vp = pass.camera ? camera_parameters(input.game, pass.camera).viewport : full_viewport();
            const auto vp_address = reset_viewport + 16u * static_cast<std::uint32_t>(n + 1);
            viewport(memory, vp_address, vp);
            commands.camera(vp_address, vp, pass.flags, pass.background_rgba, 0, color);
            const auto vp_bounds = bounds(vp);
            const sp::Bounds clipped{std::max(vp_bounds.left, pass.scissor.left),
                std::max(vp_bounds.top, pass.scissor.top), std::min(vp_bounds.right, pass.scissor.right),
                std::min(vp_bounds.bottom, pass.scissor.bottom)};
            require(clipped.left <= clipped.right && clipped.top <= clipped.bottom,
                    "scene pass scissor is disjoint or reversed");
            commands.scissor(clipped);
            if (pass.kind == render::Kind::clear) continue;
            if (pass.kind == render::Kind::sprites) {
                draws.clear();
                for (const auto& sprite : pass.sprites) if (pass.includes(sprite.render)) draws.push_back(sprite.draw);
                command_reservations.clear();
                const auto command_end = std::uint64_t(pass.sprite_commands.address) + pass.sprite_commands.size;
                bool covered = false;
                for (const auto range : composition_reservations) {
                    const auto end = std::uint64_t(range.address) + range.size;
                    if (range.address <= pass.sprite_commands.address && command_end <= end) {
                        covered = true;
                        if (range.address < pass.sprite_commands.address)
                            command_reservations.push_back({range.address, pass.sprite_commands.address - range.address});
                        if (command_end < end)
                            command_reservations.push_back({static_cast<std::uint32_t>(command_end),
                                                            static_cast<std::uint32_t>(end - command_end)});
                    } else command_reservations.push_back(range);
                }
                require(covered, "scene sprite command arena is not reserved");
                const auto list = producer->prepare(ram, epoch, draws, {vp_bounds, pass.scissor},
                                                    pass.sprite_commands, command_reservations);
                commands.call(list.address);
                commands.mode(true, 19, 1, 0x80000);
                draws.clear();
                continue;
            }
            render::Prepared prepared;
            require(pass.prepare(pass.userdata, ram, epoch, pass, prepared),
                    "scene compiled producer rejected preparation", Error::preparation);
            const auto call = [&](render::List list) {
                if (!list.words) return;
                require(!(list.address & 7u) && list.words >= 2 && !(list.words & 1u) &&
                        list.address <= menu::rdram_size &&
                        list.words <= (menu::rdram_size - list.address) / 4,
                        "scene compiled sublist exceeds physical RAM", Error::preparation);
                const sp::RamRange extent{list.address, static_cast<std::uint32_t>(list.words * 4)};
                for (std::size_t r = 0; r < protected_count; ++r)
                    require(!intersects(extent, composition_reservations[r]),
                            "compiled sublist overlaps core task/framebuffer storage");
                bool covered = false;
                for (const auto range : input.resources.reservations)
                    covered = covered || (range.address <= extent.address &&
                        std::uint64_t(extent.address) + extent.size <= std::uint64_t(range.address) + range.size);
                require(covered, "compiled sublist has no source producer reservation");
                commands.call(list.address);
            };
            for (const auto lane : lanes) {
                if (lane && prepared.lanes[lane].words <= 2) continue;
                if (lane) {
                    commands.camera(vp_address, vp, 0, 0, lane, color);
                    commands.scissor(clipped);
                }
                commands.command(0xda380003, identity);
                call(prepared.camera); call(prepared.lanes[lane]);
            }
        }
        commands.command(0xe9000000, 0); commands.command(0xdf000000, 0);
        offsets = input.screen_offsets;
    }
};

Frame::Frame(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}
Frame::~Frame() = default;
CreateResult Frame::create(const menu::Catalog& catalog, const rom::Snapshot& snapshot, Config config) noexcept {
    CreateResult result;
    try { result.frame.reset(new Frame(std::make_unique<Impl>(&catalog, snapshot, config))); }
    catch (const Failure& e) { result.error = e.error; diagnostic(result.message, e.message); }
    catch (const std::exception& e) { result.error = Error::resource; diagnostic(result.message, e.what()); }
    catch (...) { result.error = Error::resource; diagnostic(result.message, "startup graphics construction exception"); }
    return result;
}
CreateResult Frame::create(const rom::Snapshot& snapshot, Config config) noexcept {
    CreateResult result;
    try { result.frame.reset(new Frame(std::make_unique<Impl>(nullptr, snapshot, config))); }
    catch (const Failure& e) { result.error = e.error; diagnostic(result.message, e.message); }
    catch (const std::exception& e) { result.error = Error::resource; diagnostic(result.message, e.what()); }
    catch (...) { result.error = Error::resource; diagnostic(result.message, "scene graphics construction exception"); }
    return result;
}
namespace {
template<class State, class Input>
bool begin_frame(State& state, Input view, FrameContext context) noexcept {
    if (state.error != Error::none || state.retired || state.token != 0) return false;
    state.ready = false;
    try {
        auto preparation = state.executor.begin_preparation();
        state.prepare(view, context, preparation);
        const auto task = wire::finish_task(preparation.words(), state.snapshot, boot, stack, yield,
            task_address, rsp_output, output_end, top);
        // Native views have ended; only RAM/task packets reach the GPU worker.
        // Raster ownership remains pinned until real GPU completion/readback.
        state.token = state.executor.begin(std::move(preparation), task.task, task.boot);
        return true;
    } catch (const Failure& e) { state.fail(e.error, e.message); }
      catch (const std::exception& e) { state.fail(Error::execution, e.what()); }
      catch (...) { state.fail(Error::execution, "startup graphics submission exception"); }
    return false;
}
} // namespace
bool Frame::begin(startup::View view, FrameContext context) noexcept {
    return begin_frame(*impl_, view, context);
}
bool Frame::begin(render::Input input, FrameContext context) noexcept {
    return begin_frame(*impl_, input, context);
}
Poll Frame::poll() noexcept {
    auto& state = *impl_;
    if (state.error != Error::none) return Poll::failed;
    if (state.token == 0) return state.ready ? Poll::completed : Poll::idle;
    try {
        if (!state.executor.poll(state.token)) return Poll::pending;
        auto completed = state.executor.completed(state.token);
        require(bool(completed.rsp_result()) && completed.rsp_result().full_syncs == 1,
                "startup actual RSP task did not finish its source FullSync", Error::execution);
        const auto ram = completed.rdram();
        require(ram.size() == menu::rdram_size && state.color <= ram.size() &&
                target_bytes <= ram.size() - state.color,
                "startup completed source framebuffer exceeds mapped GPU RAM", Error::execution);
        for (std::size_t y = 0; y < height; ++y) {
            const auto row = state.color + y * width * 2;
            require(row <= ram.size() && width * 2 <= ram.size() - row,
                    "startup completed source row exceeds mapped GPU RAM", Error::execution);
            for (std::size_t x = 0; x < width; ++x) {
                const auto at = row + x * 2;
                const auto pixel = (std::to_integer<unsigned>(ram[at ^ 3u]) << 8) |
                                   std::to_integer<unsigned>(ram[(at + 1) ^ 3u]);
                const unsigned r = (pixel >> 11) & 31, g = (pixel >> 6) & 31, b = (pixel >> 1) & 31;
                const auto destination = (y * width + x) * 4;
                state.rgba[destination] = std::byte((r << 3) | (r >> 2));
                state.rgba[destination + 1] = std::byte((g << 3) | (g >> 2));
                state.rgba[destination + 2] = std::byte((b << 3) | (b >> 2));
                state.rgba[destination + 3] = std::byte(255); // VI scanout is opaque; bit0 is coverage.
            }
        }
        completed.release();
        require(state.executor.quiescent(), "startup GPU result retained after readback", Error::execution);
        state.graphics_borrow.reset();
        state.token = 0; ++state.ordinal; state.ready = true;
        return Poll::completed;
    } catch (const Failure& e) { state.fail(e.error, e.message); }
      catch (const std::exception& e) { state.fail(Error::execution, e.what()); }
      catch (...) { state.fail(Error::execution, "startup graphics completion exception"); }
    return Poll::failed;
}
bool Frame::busy() const noexcept { return impl_->token != 0 || !impl_->executor.quiescent(); }
bool Frame::quiescent() const noexcept { return !busy() && impl_->executor.quiescent(); }
Error Frame::error() const noexcept { return impl_->error; }
const char* Frame::message() const noexcept { return impl_->message.data(); }
Completed Frame::completed() const noexcept {
    if (!impl_->ready || busy() || impl_->retired || impl_->error != Error::none) return {};
    return {impl_->rgba, width, height, width * 4, impl_->offsets, impl_->ordinal};
}
bool Frame::retire() noexcept {
    if (busy()) return false;
    impl_->graphics_borrow.reset();
    impl_->ready = false; impl_->retired = true;
    return true;
}
} // namespace isle::scene::graphics

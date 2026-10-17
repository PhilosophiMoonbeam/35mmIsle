#ifndef ISLE_PLAYER_RENDERER_HPP
#define ISLE_PLAYER_RENDERER_HPP
#include "player_render_source.h"
#include "player_photo_frame.hpp"
#include "photo_lighting.hpp"
#include "scene_graphics.hpp"
#include "effect_graphics_resources.hpp"
#include "graphics_task.hpp"
#include <memory>

namespace isle::actor { class Owner; }
namespace isle::player::cart { class Owner; }
namespace isle::player {
// Retained camera scheduling/render metadata, not another Player, camera pose,
// clock, Detector transaction registry or original Root entrypoint. Services,
// Source, providers, ROM and resources outlive detach().
// Canonical Player main5/UI0/FX0 are admitted. Task-flushing camera flags10/40
// require a real multi-task Frame transaction and reject, never succeed as NOPs.
// Postflag4 combines pending source lanes; flag4-off lanes remain deferred until
// the actual final-GTL phase. No extra camera, fake task or completion is used.
class Renderer final {
public:
    static constexpr std::size_t camera_capacity = 32;
    struct Config {
        scene::render::Resources resources;
        scene::render::Prepare prepare = nullptr;
        void* prepare_user = nullptr;
        const graphics::SourcePrograms* programs = nullptr;
        graphics::SourceProgramSelection program_selection{graphics::SourceProgram::f3dex2_non_fifo};
        Controller* photos = nullptr;
        // Four bounded per-camera wire lanes; includes source pre-render packets,
        // camera restores and one terminating EndDL. No geometry is copied here.
        sprite::RamRange lanes{0x690000,0x40000};
        // Partitioned into 32 disjoint scheduling-camera arenas, including
        // multiple genuine sprite cameras in the same submitted Frame.
        sprite::RamRange sprite_commands{0x780000,0x50000};
        sprite::RamRange effect_commands{0x680c00,0x5400};
        const menu::EffectGraphicsResources* effect_resources = nullptr;
        std::size_t sprite_capacity = 128;
    };
    struct Camera {
        IsleGameObject* scheduling_object = nullptr;
        IsleGameCamera* projection = nullptr;
        IslePlayerRenderCallback render = nullptr;
        IslePlayerPreRenderCallback prerender = nullptr;
        std::int32_t priority = 0;
        std::uint32_t flags = 0, background = 0, link_mask = 0, tag = 0;
        scene::render::TagMatch tag_match = scene::render::TagMatch::intersection;
        // Optional actual per-camera provider (e.g. another genuine compiled
        // camera). NULL uses the configured global source producer.
        scene::render::Prepare prepare=nullptr;
        void* prepare_user=nullptr;
    };
    // Bounded command publication metadata, not retained RAM/native borrows.
    // It describes the last real source dispatch, including exact C callback
    // CPU order. GPU order remains encoded by the actual unpushed branches.
    struct Dispatch {
        std::array<scene::render::List,4> lanes{};
        scene::render::List prefix,reset;
        std::array<std::uint8_t,4> prerender_order{};
        std::size_t prerender_count=0,sprite_views=0;
        bool combined=false;
        std::uint64_t epoch=0;
        std::uint32_t framebuffer=0,depth=0;
    };
    Renderer(scene::Services&,IslePlayerSource&,const photo::LightingInputs&,Config);
    ~Renderer();
    Renderer(const Renderer&)=delete;
    Renderer& operator=(const Renderer&)=delete;
    // Complete original C constructor; output is its actual returned UI object.
    IsleGameResult create_main(std::int32_t background,IsleGameObject** ui) noexcept;
    // Register another genuine scheduling camera without replacing its update
    // or primary semantic owner's lifecycle. Equal priorities are head-inserted,
    // as the original camera list. Projection and scheduling identities differ.
    IsleGameResult attach(const Camera&) noexcept;
    IsleGameResult adopt_effects(actor::Owner&) noexcept;
    // Cold attachment after original camera creation and PhotoScene binding.
    // The sole production Controller owns all capture identities.
    IsleGameResult bind_photos(Controller&) noexcept;
    IsleGameResult bind_cart(cart::Owner&,std::span<scene::RenderOrderEntry> cold_order_scratch) noexcept;
    IsleGameObject* ui_camera_object() const noexcept;
    IsleGameResult metadata(IsleGameObject*,Camera&) const noexcept;
    IsleGameResult dispatch_metadata(IsleGameObject*,Dispatch&) const noexcept;
    // reserve() is cold-only, before ANY begin on this actual Frame.
    bool reserve(scene::graphics::Frame&) noexcept;
    bool begin(scene::graphics::Frame&,scene::graphics::FrameContext = {},
               scene::graphics::CompletedWork = {}) noexcept;
    scene::graphics::Poll poll() noexcept;
    // Completed Photo tickets deliberately survive poll, OM CopyInfo and
    // shutter use. This releases their real holds before Frame/camera reuse.
    IsleGameResult release_completed() noexcept;
    IsleGameResult remove_main() noexcept;
    IsleGameResult collect_joined() noexcept;
    // Delete/join scheduling helpers through their real composed owners.
    // Rejects outstanding GPU/readback/Photo borrows and active C dispatch.
    IsleGameResult detach() noexcept;
    IsleGameResult status() const noexcept;
    const char* message() const noexcept;
    photo::LightDirection light_direction() const noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    friend struct RendererAccess;
};
}
#endif

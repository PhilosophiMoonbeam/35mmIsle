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
namespace isle::screen_transition { class Resources; class Owner; }
namespace isle::player {
class Ui;
// Retained camera scheduling/render metadata, not another Player, camera pose,
// clock, Detector transaction registry or original Root entrypoint. Services,
// Source, providers, ROM and resources outlive detach().
// One logical Frame owns sequential native process/reset task boundaries.
// Postflag4 combines pending lanes; flag10 submits and drains before resetting
// Source cursors, while flag40 submits without allocator reset. Only whole
// logical completion publishes the image and releases resource owners.
class Renderer final {
public:
    // Includes adopted Source message and transition scheduling cameras.
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
    // or primary semantic owner's lifecycle. Original camera insertion appends
    // after equal priorities. Projection and scheduling identities differ.
    IsleGameResult attach(const Camera&) noexcept;
    // Individually adopt each original FX factory result BEFORE it is returned
    // to the C canonical-field assignment. Each exact field is registered once
    // by the actual main-camera receiver and credited here before publication.
    IsleGameResult adopt_particle(actor::Owner&,IsleGameObject*) noexcept;
    IsleGameResult adopt_effect(actor::Owner&,IsleGameObject*) noexcept;
    IsleGameResult adopt_effects(actor::Owner&) noexcept;
    // Changes the real GTL task selector, never the camera near-plane scalar.
    // Rejects outstanding preparation/GPU/Photo ownership before changing it.
    IsleGameResult set_near_clipping_disabled(bool) noexcept;
    // Cold attachment after original camera creation and PhotoScene binding.
    // The sole production Controller owns all capture identities.
    IsleGameResult bind_photos(Controller&) noexcept;
    IsleGameResult bind_cart(cart::Owner&,std::span<scene::RenderOrderEntry> cold_order_scratch) noexcept;
    // Bind immediately after original Msg_InitCamera, before any later genuine
    // priority3 camera and before reserve/first begin. Ordinary Ui sprites stay
    // Services-owned; Ui survives primary-owner/secondary-observer collection.
    // Caller supplies the arena from its complete, asset-disjoint RAM plan.
    IsleGameResult bind_ui(Ui&,sprite::RamRange message_commands) noexcept;
    // Bind cold, before reserve/first begin; the Source camera may not exist yet.
    // One of the same32 slots is reserved, not another camera registry. Resources
    // and Owner outlive this same Frame's preparation, GPU and readback.
    IsleGameResult bind_transition(screen_transition::Resources&,screen_transition::Owner&) noexcept;
    // Source SC callback registration, independent of per-actor capture enable.
    IsleGameResult set_postprocess(bool enabled) noexcept;
    // Standard whole Source draw dispatches, not OM/GPU/audio epochs.
    std::int32_t drawn_frame_counter() const noexcept;
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
    // Logical request is safe inside OM and while consumers retain the camera.
    // Immediately pauses its actual updater/processes and hides its scheduling
    // object, preventing later camera work in the same OM/preparation traversal.
    // Does not clear Source fields, delete payloads, join or complete GPU/Photo.
    // No new begin is admitted after the request. Physical Source deletion is
    // performed only by service_main_removal at a proven idle owner boundary.
    IsleGameResult remove_main() noexcept;
    IsleGameResult service_main_removal() noexcept;
    // Cancellation-only main-camera teardown, without entering a Source fence
    // or clearing its failure. Requires real idle/GPU/Photo closure; immediately
    // stops and actually erases/joins only the main object, retaining records
    // for later collection. No published main object is a successful no-op.
    IsleGameResult abort_main() noexcept;
    bool main_removal_pending() const noexcept;
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

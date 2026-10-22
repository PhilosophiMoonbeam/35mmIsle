#include "post_course_scene_internal.hpp"
#include "screen_transition_resources.hpp"
#include <algorithm>
#include <limits>

namespace isle::post_course {
namespace {
constexpr sprite::RamRange sprite_commands{0x590000,0x10000};

// Resource publication, never a draw operation. Keep the existing producer's
// canonical backing identity on shared bytes, and append only the other owner's
// uncovered bytes. The raw copies must agree before accepting a physical alias.
void append_immutable(std::vector<sprite::AssetSpan>& output,sprite::AssetSpan asset) {
    if(asset.address>0x800000u || asset.bytes.size()>0x800000u-asset.address)
        throw std::runtime_error("post-course immutable publication escapes RDRAM");
    const auto end=std::uint64_t(asset.address)+asset.bytes.size();
    const auto existing=output.size();
    for(std::size_t i=0;i<existing;++i) {
        const auto& prior=output[i];
        const auto lo=std::max(asset.address,prior.address);
        const auto hi=std::min(end,std::uint64_t(prior.address)+prior.bytes.size());
        if(lo<hi && !std::equal(asset.bytes.begin()+(lo-asset.address),
            asset.bytes.begin()+(hi-asset.address),prior.bytes.begin()+(lo-prior.address)))
            throw std::runtime_error("post-course shared immutable publication differs from admitted bytes");
    }
    auto cursor=std::uint64_t(asset.address);
    while(cursor<end) {
        auto next=end;
        auto covered=cursor;
        for(std::size_t i=0;i<existing;++i) {
            const auto& prior=output[i];
            const auto prior_end=std::uint64_t(prior.address)+prior.bytes.size();
            if(prior.address<=cursor && cursor<prior_end)covered=std::max(covered,prior_end);
            else if(prior.address>cursor)next=std::min(next,std::uint64_t(prior.address));
        }
        if(covered>cursor) {cursor=std::min(covered,end);continue;}
        output.push_back({static_cast<std::uint32_t>(cursor),
            asset.bytes.subspan(static_cast<std::size_t>(cursor-asset.address),static_cast<std::size_t>(next-cursor))});
        cursor=next;
    }
}
void append_range(std::vector<sprite::RamRange>& output,sprite::RamRange range) {
    for(const auto prior:output) {
        if(prior.address==range.address && prior.size==range.size)return;
        if(std::uint64_t(prior.address)<std::uint64_t(range.address)+range.size &&
           std::uint64_t(range.address)<std::uint64_t(prior.address)+prior.size)
            throw std::runtime_error("post-course source producer reservations overlap");
    }
    output.push_back(range);
}
}

IsleGameResult Scene::Impl::initialize_presentation() noexcept {
    try {
        auto& scene=live();
        if((kind!=Resources::Kind::gallery && kind!=Resources::Kind::end_level_8) ||
           !scene.pipeline || !scene.graphics || scene.presentation)
            return fail(ISLE_GAME_INVALID_STATE,"post-course full-photo initialization lacks its unique shared Replay");
        photo::PresentationConfig config;
        config.frame=scene.graphics.get();
        config.thumbnail_replay=scene.pipeline.get();
        config.background_rgb=&state.background_rgb;
        if(kind==Resources::Kind::end_level_8) {
            config.dl_link=5;config.world_object_link=9;config.photo_object_link=8;
        }
        auto& memory=config.memory;
        memory.world={{0x400000,0x20000},{0x420000,0x10000},{0x430000,0x10000}};
        memory.subjects={{0x440000,0x60000},{0x4a0000,0x20000},{0x4c0000,0x20000},{}};
        memory.items={{0x4e0000,0x10000},{0x4f0000,0x10000},{0x500000,0x10000},{}};
        memory.commands={0x510000,0x20000};
        memory.effects={0x580000,0x10000};memory.effect_assets={0x630000,0x80000};
        const std::array shared{memory.world.payloads,memory.world.lists,memory.world.materials,
            memory.subjects.payloads,memory.subjects.lists,memory.subjects.materials,
            memory.items.payloads,memory.items.lists,memory.items.materials,
            memory.commands,memory.effects,memory.effect_assets};
        std::vector<menu::RamRange> external;
        external.reserve(scene.graphics->reservations().size()+scene.reservations.size()+5);
        const auto add_external=[&](menu::RamRange range) {
            if(!range.size)return;
            for(const auto prior:external)
                if(prior.offset==range.offset && prior.size==range.size)return;
            external.push_back(range);
        };
        for(const auto range:scene.graphics->reservations()) {
            if(range.address==0 && range.size==::isle::graphics::bss_end)continue;
            add_external({range.address,range.size});
        }
        for(const auto range:scene.reservations)add_external({range.address,range.size});
        // Only the planned producer ranges move with the one actual World.
        // Replay's output/workspace and overlapping preview/feedback union stay
        // external and protected; none is lent to normal display composition.
        for(const auto range:scene.pipeline->reservations()) {
            bool handed_off=false;
            for(const auto own:shared)
                if(own.offset==range.offset && own.size==range.size)handed_off=true;
            if(!handed_off)add_external(range);
        }
        const auto framebuffer_bytes=std::uint64_t(screen_words[4])*screen_words[5]*2u;
        if(!framebuffer_bytes || framebuffer_bytes>std::numeric_limits<std::uint32_t>::max())
            return fail(ISLE_GAME_INVALID_STATE,"post-course Source framebuffer extent");
        for(std::size_t i=0;i<config.framebuffers.size();++i) {
            config.framebuffers[i]={screen_words[i]&0x1fffffffu,framebuffer_bytes};
            add_external(config.framebuffers[i]);
        }
        config.external_reservations=external;
        scene.presentation=std::make_unique<photo::Presentation>(scene.storage.game(),snapshot,
            modules.world,photos,config);
        for(const auto range:scene.presentation->reservations())
            append_range(scene.frame_reservations,{static_cast<std::uint32_t>(range.offset),
                static_cast<std::uint32_t>(range.size)});
        return ISLE_GAME_OK;
    } catch(const std::exception& error) {return fail(ISLE_GAME_INVALID_STATE,error.what());}
    catch(...) {return fail(ISLE_GAME_EXECUTION_FAILURE,"post-course full-photo construction threw");}
}

scene::render::Input Scene::Impl::Native::render_input() {
    if(retired || !activated || !services || services->failed())
        throw std::logic_error("post-course render requires live Source Services");
    std::size_t count=0;
    if(!services->sprite_draws(draws,count))throw std::runtime_error(services->error());
    if(!pass_count)throw std::runtime_error("post-course Source created no native cameras");

    auto& transition=services->screen_transition();
    const auto transition_camera=services->screen_transition_source()->D_800BE2C8;
    if(presentation && presentation_revision!=presentation->resource_revision()) {
        // The actual Source publication occurs while the Frame is quiescent,
        // before begin takes its exclusive preparation lease. Stable draws do
        // not allocate, rebuild a union, copy assets or advance this identity.
        immutable.resize(base_immutable_count);
        for(const auto asset:presentation->immutable())append_immutable(immutable,asset);
        presentation_revision=presentation->resource_revision();
    }

    IsleGameCamera* photo_camera=nullptr;
    if(const auto* gallery=std::get_if<GallerySceneNative>(&menu))photo_camera=gallery->photo_camera;
    else if(const auto* end=std::get_if<EndLevel8SceneNative>(&menu))photo_camera=end->camera;
    const auto draw_control=+[](void*,scene::graphics::SourceControl& control) {
        control.draw();
        // These ordinary camera callbacks own the actual outer command group.
        // Keep Source's independent post ordering: process/reset10 precedes40;
        // an empty group after reset is not another graphics task. Custom
        // Source controls below retain their own callback-specific post policy.
        const auto flags=control.camera_flags();
        if(flags&0x10u)control.process_commands(true);
        if(flags&0x40u)control.process_commands(false);
        return true;
    };
    const auto skip_control=+[](void*,scene::graphics::SourceControl&) {return true;};
    for(std::size_t i=0;i<pass_count;++i) {
        auto& pass=passes[i];
        // Copy the actual compiled transition recipe, including both its owner
        // lifetime and conservative depth declaration. Only kind0 is depthless.
        if(pass.camera==transition_camera)pass=graphics->transition_pass(transition);
        if(photo_camera && pass.camera==photo_camera) {
            if(!presentation)throw std::logic_error("post-course Source full-photo camera has no Presentation owner");
            pass.kind=scene::render::Kind::compiled;
            pass.prepare=photo::Presentation::prepare_pass;
            pass.userdata=presentation.get();
            pass.requires_depth=true;
        }
        IsleGameCameraRenderState state{};
        if(isle_game_camera_render_state(storage.game(),pass.camera,&state)!=ISLE_GAME_OK)
            throw std::runtime_error("post-course native camera render state");
        pass.flags=state.flags;pass.background_rgba=state.background_rgba;
        if(owner.kind!=Resources::Kind::camera_check && owner.kind!=Resources::Kind::photo_check)
            pass.tag_match=(state.flags&8u)?scene::render::TagMatch::equality:
                scene::render::TagMatch::intersection;
        if(pass.kind==scene::render::Kind::sprites) {
            pass.sprites=std::span<const scene::SpriteDraw>(draws).first(count);
            pass.sprite_commands=sprite_commands;
        }
        if(owner.kind!=Resources::Kind::camera_check && owner.kind!=Resources::Kind::photo_check &&
           (!pass.control || pass.control==draw_control || pass.control==skip_control)) {
            IsleGameObject* object=nullptr;
            IsleGameObjectMetadata metadata{};
            if(isle_game_camera_object(storage.game(),pass.camera,&object)!=ISLE_GAME_OK ||
               isle_game_object_metadata(storage.game(),object,&metadata)!=ISLE_GAME_OK)
                throw std::runtime_error("post-course Source camera membership");
            // Original hidden camera objects dispatch nothing. Skip under the
            // Source control before depth preflight/preparation, not by making
            // an empty photo or adding a normal offscreen Replay camera pass.
            pass.control=(metadata.flags&1u)?skip_control:draw_control;
        }
    }
    save::ScreenOffsets offsets{};
    if(!owner.state.screen_offsets().translation(offsets))
        throw std::runtime_error("post-course live VI endpoints cannot be represented by native translation");
    ::isle::graphics::SourceProgramSelection programs;
    programs.disable_near_clipping=owner.near_clipping_disabled;
    if(presentation_revision>(std::numeric_limits<std::uint64_t>::max()>>8))
        throw std::runtime_error("post-course Presentation resource identity exhausted");
    const auto identity=(presentation_revision<<8)|static_cast<std::uint32_t>(owner.source().scene);
    return {storage.game(),{identity,producer.get(),immutable,frame_reservations,
        {owner.screen_words[0]&0x1fffffffu,owner.screen_words[1]&0x1fffffffu},common->common_pre_render()},
        std::span<const scene::render::Pass>(passes).first(pass_count),offsets,
        ::isle::graphics::source_task_program(programs,false)};
}
} // namespace isle::post_course

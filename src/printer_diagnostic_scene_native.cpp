#include "printer_diagnostic_scene_native.hpp"
#include "post_course_native_helpers.hpp"
#include "photo_presentation.hpp"
#include "snap_station.hpp"
#include "menu_scene_bridge.h"

IsleGameResult isle_post_course_native_progress(IslePostCourseSceneSource*);
namespace isle::post_course {
namespace {
using Source = IslePrinterDiagnosticSource;
template<class F> IsleGameResult call(Source* source,F&& function) noexcept {
    return SceneAccess::invoke(source ? &source->common : nullptr,[&](auto& owner) {
        if (owner.kind!=Resources::Kind::printer_diagnostic ||
            std::get_if<Source>(&owner.context)!=source) return ISLE_GAME_INVALID_STATE;
        auto& bound=std::get<PrinterDiagnosticSceneNative>(owner.live().menu);
        if (bound.source!=source || !bound.resources ||
            bound.resources->owner()!=owner.live().resources) return ISLE_GAME_INVALID_STATE;
        return function(owner,bound);
    });
}
IsleGameResult hidden(Source* source,IsleGameObject* object,int value) noexcept {
    return call(source,[&](auto&,auto&) {
        if (!object || (value!=0 && value!=1)) return ISLE_GAME_INVALID_ARGUMENT;
        IsleGameObjectMetadata metadata{};
        auto result=isle_game_object_metadata(source->common.game,object,&metadata);
        if (result!=ISLE_GAME_OK) return result;
        return isle_game_object_set_flags(source->common.game,object,value ? metadata.flags|1u : metadata.flags&~1u);
    });
}
IsleGameResult camera_setup(Source* source) noexcept {
    return call(source,[&](auto& owner,auto& bound) {
        auto& live=owner.live();
        if (bound.depth_admitted || !live.graphics) return ISLE_GAME_INVALID_STATE;
        const auto* depth=live.graphics->reserve_source_depth(0x96000,0x40);
        const auto& screen=bound.resources->screen();
        if (!depth || !live.graphics->configure_source_screen({screen.width,screen.height,screen.flags,screen.framebuffers},depth))
            return ISLE_GAME_INVALID_STATE;
        bound.depth_admitted=true;
        IsleGameObject* clear_object=nullptr; IsleGameCamera* clear_camera=nullptr;
        auto result=camera_owner(owner,0,0,0x80000000,clear_object,clear_camera);
        if (result!=ISLE_GAME_OK) return result;
        result=viewport(owner,clear_camera,640,480);
        if (result!=ISLE_GAME_OK) return result;
        const IsleGameCameraRenderState clear_render{6,0xff,0};
        result=isle_game_camera_set_render_state(source->common.game,clear_camera,&clear_render);
        if (result!=ISLE_GAME_OK) return result;
        live.add_camera(clear_object,clear_camera,0x64,0,0);
        live.passes[live.pass_count-1].kind=scene::render::Kind::clear;
        bound.camera_object=live.services->object(1,0,0x80000000);
        if (!bound.camera_object) return ISLE_GAME_EXHAUSTED;
        live.source_objects.push_back(bound.camera_object);
        result=isle_game_attach_camera(source->common.game,bound.camera_object,&bound.camera);
        if (result!=ISLE_GAME_OK) return result;
        source->camera=bound.camera;
        IsleGameProcess* process=nullptr;
        result=isle_game_add_source_process(source->common.game,bound.camera_object,
            isle_printer_diagnostic_func_801DC9D0_AA1A10,ISLE_GAME_STACKFUL,1,&process);
        if (result!=ISLE_GAME_OK) return result;
        const IsleGameCameraRenderState render{5,0,UINT32_MAX};
        result=isle_game_camera_set_render_state(source->common.game,bound.camera,&render);
        if (result!=ISLE_GAME_OK) return result;
        result=viewport(owner,bound.camera,640,480);
        if (result!=ISLE_GAME_OK) return result;
        const IsleGameCameraParameters* parameters=nullptr;
        result=isle_game_camera_parameters(source->common.game,bound.camera,&parameters);
        if (result!=ISLE_GAME_OK) return result;
        auto perspective=parameters->perspective;
        auto view=parameters->viewport;
        view.scale[0]=static_cast<int16_t>(view.scale[0]*97/100);
        view.scale[1]=static_cast<int16_t>(view.scale[1]*93/100);
        view.translate[1]=static_cast<int16_t>(view.translate[1]*93/100);
        result=isle_game_camera_set_viewport(source->common.game,bound.camera,&view);
        if (result!=ISLE_GAME_OK) return result;
        result=isle_game_camera_add_matrix(source->common.game,bound.camera,ISLE_GAME_CAMERA_PERSPECTIVE_FAST);
        if (result!=ISLE_GAME_OK) return result;
        result=isle_game_camera_add_matrix(source->common.game,bound.camera,ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL);
        if (result!=ISLE_GAME_OK) return result;
        perspective.near_plane=10; perspective.far_plane=25600; perspective.fovy=60;
        perspective.aspect=bound.resources->aspect();
        result=isle_game_camera_set_perspective(source->common.game,bound.camera,&perspective);
        if (result!=ISLE_GAME_OK) return result;
        const IsleGameLookAt pose{{0,0,100},{0,0,0},{0,1,0}};
        result=isle_game_camera_set_look_at(source->common.game,bound.camera,&pose);
        if (result!=ISLE_GAME_OK) return result;
        live.add_camera(bound.camera_object,bound.camera,5,1u<<source->photo_link,UINT32_MAX);
        auto& pass=live.passes[live.pass_count-1];
        pass.kind=scene::render::Kind::compiled;
        pass.tag_match=scene::render::TagMatch::intersection;
        pass.prepare=photo::Presentation::prepare_pass;
        pass.userdata=live.presentation.get();
        IsleGameCamera* sprite_camera=nullptr;
        result=camera_owner(owner,3,0,0x80000000,source->sprite_camera,sprite_camera);
        if (result!=ISLE_GAME_OK) return result;
        result=viewport(owner,sprite_camera,640,480);
        if (result!=ISLE_GAME_OK) return result;
        const IsleGameCameraRenderState sprite_render{8,0xff,UINT32_MAX};
        result=isle_game_camera_set_render_state(source->common.game,sprite_camera,&sprite_render);
        if (result!=ISLE_GAME_OK) return result;
        result=isle_game_object_set_flags(source->common.game,source->sprite_camera,1);
        if (result!=ISLE_GAME_OK) return result;
        live.add_camera(source->sprite_camera,sprite_camera,3,2,UINT32_MAX);
        const auto sprite=live.services->sprite(0x801e5478,1,0x80000000,31);
        if (!sprite.object || !sprite.sprite) return ISLE_GAME_EXHAUSTED;
        live.source_objects.push_back(sprite.object);
        return ISLE_GAME_OK;
    });
}
IsleGameResult saved_photo(Source* source,int index,const IslePhotoRecord** output) noexcept {
    if (output) *output=nullptr;
    if (!output || index<0 || index>=4) return ISLE_GAME_INVALID_ARGUMENT;
    return call(source,[&](auto& owner,auto& bound) {
        auto record=owner.photos.record(ISLE_PHOTO_TEMPORARY,index);
        if (!record) return ISLE_GAME_INVALID_STATE;
        bound.saved[static_cast<std::size_t>(index)]=record.value;
        if (record.value.photo) *output=&bound.saved[static_cast<std::size_t>(index)];
        return ISLE_GAME_OK;
    });
}
IsleGameResult replay_initialize(Source* source,int photo,int actor,int item) noexcept {
    return call(source,[&](auto& owner,auto& bound) {
        if (photo!=source->photo_link || actor!=source->actor_link || item!=source->item_link) return ISLE_GAME_INVALID_ARGUMENT;
        auto result=owner.initialize_replay();
        if (result!=ISLE_GAME_OK) return result;
        auto& live=owner.live();
        if (!live.presentation) return ISLE_GAME_INVALID_STATE;
        for (std::size_t i=0;i<live.pass_count;++i) if (live.passes[i].camera==bound.camera)
            live.passes[i].userdata=live.presentation.get();
        return ISLE_GAME_OK;
    });
}
IsleGameResult presentation_begin(Source* source,IsleGameCamera* camera,const IslePhotoRecord* record) noexcept {
    if (!record || !record->photo) return ISLE_GAME_INVALID_ARGUMENT;
    auto result=call(source,[&](auto& owner,auto& bound) {
        auto& live=owner.live();
        if (camera!=bound.camera || !live.presentation || !live.graphics->quiescent()) return ISLE_GAME_INVALID_STATE;
        bound.photo_ready=false; bound.frame_staged=false;
        bound.first_ordinal=live.graphics->completed().ordinal;
        return live.presentation->begin(camera,*record) ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
    });
    if (result!=ISLE_GAME_OK) return result;
    bool ready=false;
    while (!ready) {
        result=call(source,[&](auto& owner,auto& bound) {
            const auto status=owner.live().presentation->poll();
            if (status==ISLE_PHOTO_PRESENTATION_PENDING) return ISLE_GAME_OK;
            if (status!=ISLE_PHOTO_PRESENTATION_READY) return ISLE_GAME_INVALID_STATE;
            for (const auto link:{source->actor_link,source->item_link})
                for (auto* object=isle_menu_first_object(source->common.game,link);object;
                     object=isle_menu_next_object(source->common.game,object)) {
                    const auto paused=isle_game_pause_object_processes(source->common.game,object,1);
                    if (paused!=ISLE_GAME_OK) return paused;
                }
            bound.photo_ready=true; ready=true; return ISLE_GAME_OK;
        });
        if (result!=ISLE_GAME_OK) return result;
        if (!ready) { result=isle_post_course_native_progress(&source->common); if (result!=ISLE_GAME_OK) return result; }
    }
    return ISLE_GAME_OK;
}
IsleGameResult postprocess(Source* source,int enabled) noexcept {
    return call(source,[&](auto&,auto& bound) {
        if (enabled!=0 && enabled!=1) return ISLE_GAME_INVALID_ARGUMENT;
        bound.postprocess=enabled!=0; return ISLE_GAME_OK;
    });
}
IsleGameResult printer_status(Source* source,int32_t* output) noexcept {
    if (output) *output=0;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return call(source,[&](auto& owner,auto&) {
        std::uint8_t value=0;
        if (!owner.station || !owner.station->status(value)) return ISLE_GAME_INVALID_STATE;
        *output=value; return ISLE_GAME_OK;
    });
}
IsleGameResult printer_command(Source* source,int command) noexcept {
    return call(source,[&](auto& owner,auto& bound) {
        if (!owner.station || command<0 || command>255) return ISLE_GAME_INVALID_ARGUMENT;
        if (command==2 && !bound.frame_staged) return ISLE_GAME_INVALID_STATE;
        return owner.station->command(static_cast<std::uint8_t>(command)) ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;
    });
}
IsleGameResult pressed_buttons(Source* source,uint32_t* output) noexcept {
    if (output) *output=0;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return call(source,[&](auto& owner,auto&) {
        if (!owner.input.running()) return ISLE_GAME_INVALID_STATE;
        *output=owner.input.snapshot().primary().pressed; return ISLE_GAME_OK;
    });
}
IsleGameResult replay_cleanup(Source* source,int* done) noexcept {
    if (done) *done=0;
    if (!done) return ISLE_GAME_INVALID_ARGUMENT;
    return SceneAccess::invoke_retiring(source ? &source->common : nullptr,[&](auto& owner) {
        auto& live=owner.live();
        if (!live.graphics || !live.graphics->quiescent()) return ISLE_GAME_OK;
        const auto completed=live.graphics->completed();
        if (completed && completed.ordinal!=live.released_ordinal) return ISLE_GAME_OK;
        if (live.presentation) {
            const auto status=live.presentation->retire();
            if (status==ISLE_PHOTO_PRESENTATION_PENDING) return ISLE_GAME_OK;
            if (status!=ISLE_PHOTO_PRESENTATION_RETIRED) return ISLE_GAME_INVALID_STATE;
        }
        if (live.replay) { bool retired=false; const auto status=live.replay->retire(retired); if (!retired || status!=ISLE_GAME_OK) return status; }
        if (live.pipeline && !live.pipeline->retire()) return ISLE_GAME_OK;
        *done=1; return ISLE_GAME_OK;
    });
}
IsleGameResult terminal(Source* source) noexcept {
    return call(source,[&](auto& owner,auto&) { return owner.request_exit(source->result_words[0]); });
}
IsleGameResult panic(Source* source) noexcept {
    return call(source,[](auto& owner,auto&) { return owner.fail(ISLE_GAME_EXECUTION_FAILURE,"Original printer diagnostic PANIC"); });
}
IsleGameResult entry_begin(Source* source,int32_t argument) noexcept {
    return call(source,[&](auto& owner,auto& bound) {
        if (!owner.station || argument<0 || argument>16 ||
            !owner.station->begin_pass(owner.application_epoch,owner.entry_serial,&bound,static_cast<uint32_t>(argument)))
            return ISLE_GAME_INVALID_STATE;
        owner.setup_words[4]=0x2d4000;
        return owner.setup_begin(argument,0x801e5510,0x801e552c,bound.resources->screen().flags,1,1,1);
    });
}
IsleGameResult entry_poll(void* raw,int* done,int32_t* output) noexcept {
    if (!done || !output) return ISLE_GAME_INVALID_ARGUMENT;
    *done=0; *output=0;
    return SceneAccess::invoke_owner(raw,[&](auto& owner) {
        if (owner.kind!=Resources::Kind::printer_diagnostic) return ISLE_GAME_INVALID_STATE;
        return owner.setup_poll(done,output);
    });
}
const IslePrinterDiagnosticNative table{camera_setup,hidden,saved_photo,replay_initialize,
    presentation_begin,postprocess,printer_status,printer_command,pressed_buttons,
    replay_cleanup,terminal,panic,entry_begin,entry_poll};
}
const IslePrinterDiagnosticNative& printer_diagnostic_scene_native_table() noexcept { return table; }
IsleGameResult printer_diagnostic_scene_native_prepare(Source* source,PrinterDiagnosticSceneNative& bound) noexcept {
    return SceneAccess::invoke(source ? &source->common : nullptr,[&](auto& owner) {
        if (owner.kind!=Resources::Kind::printer_diagnostic || std::get_if<Source>(&owner.context)!=source ||
            &std::get<PrinterDiagnosticSceneNative>(owner.live().menu)!=&bound || bound.source) return ISLE_GAME_INVALID_STATE;
        bound.resources=owner.diagnostic_resources;
        if (!bound.resources || bound.resources->pending()!=-1) return ISLE_GAME_INVALID_STATE;
        bound.source=source;
        source->photo_link=bound.resources->links()[0]; source->actor_link=bound.resources->links()[1];
        source->item_link=bound.resources->links()[2];
        source->result_words[0]=bound.resources->pending();
        for (std::size_t i=0;i<16;++i) source->result_words[i+1]=static_cast<int32_t>(bound.resources->selections()[i]);
        source->render_complete=static_cast<int32_t>(bound.resources->postprocess_initial());
        source->requiredNativeTable=&table;
        return ISLE_GAME_OK;
    });
}
IsleGameResult printer_diagnostic_scene_native_completed(Source* source,const scene::graphics::Completed& completed) noexcept {
    return call(source,[&](auto& owner,auto& bound) {
        if (!bound.postprocess || !bound.photo_ready || bound.frame_staged || !completed ||
            completed.ordinal<=bound.first_ordinal) return ISLE_GAME_OK;
        auto& live=owner.live();
        if (completed.width!=640 || completed.height!=480 || !live.graphics ||
            !live.graphics->presentation_submission() || !owner.station) return ISLE_GAME_INVALID_STATE;
        const snap_station::FrameIdentity identity{owner.application_epoch,owner.entry_serial,
            completed.ordinal,live.graphics->presentation_submission(),&bound,static_cast<uint32_t>(source->argument)};
        if (!owner.station->stage_frame(identity,completed.rgba,completed.width,completed.height,completed.stride))
            return ISLE_GAME_EXECUTION_FAILURE;
        bound.frame_staged=true;
        isle_printer_diagnostic_func_801DC9BC_AA19FC(source);
        return ISLE_GAME_OK;
    });
}
}

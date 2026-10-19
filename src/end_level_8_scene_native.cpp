#include "end_level_8_scene_native.hpp"
#include "post_course_native_helpers.hpp"
#include "photo_presentation.hpp"
#include <array>
#include <cstring>
#include <limits>
#include <utility>

namespace isle::post_course {
namespace {
// Qualified initialized Snapshot descriptor, ROM a08d10..a08e30. These
// are big-endian words, not copied SceneSetup/ScreenSettings SDK layouts.
constexpr std::array<std::uint32_t,72> initial_words{
    0x05000000,0x09000000,0x03000000,0,0x803b5000,0x803da800,0,0,
    320,240,0x16a99,0,0x8000affc,0x8000ada0,0x8020f260,0,
    1,1,0xe000,0x2000,0,0,0x5000,0x00020000,0x3000,0x800a1a50,
    0x80004860,0x10,0x4000,0x10,0,0x20,0,0x5c,0x400,0,
    0x801dd070,0,0,0,0x88,0,0x60,0,0x90,0x801dcfe0,0,0,
    0xa4dba4ec,0xa4dca4ec,0xa4b9a4eb,0xa1a1a4b7,0xa4e3a4b7,
    0xa4f3a4c0,0xa4cd210a,0x5c61a4c4,0xa4aea4f2,0xa4dfa4eb,
    0xa1a15c62,0xa4e2a4a6,0xa4a4a4a4,0xa4e40a5c,0x7aa5e1a5,
    0xc3a5bba1,0xbca5b8a4,0xf2a1a1a4,0xb1a4b9a1,0xa6a4d2a4,
    0xe7a4a6a4,0xb8000000,0,0
};
constexpr char initial_sha256[] =
    "35bd648e5e9ad6b015d1e000046aa9002f908674c730b71279ad663e2083f730";

template<class F>
IsleGameResult invoke(IsleEndLevel8Source* source,F&& function) noexcept {
    return SceneAccess::invoke(source ? &source->common : nullptr,[&](auto& owner) {
        if (owner.kind != Resources::Kind::end_level_8 ||
            std::get_if<IsleEndLevel8Source>(&owner.context) != source)
            return owner.fail(ISLE_GAME_INVALID_STATE,"End8 exact active Source mismatch");
        if (owner.failure != ISLE_GAME_OK) return owner.failure;
        const auto status = function(owner);
        return status == ISLE_GAME_OK ? status : owner.fail(status,"End8 native operation failed");
    });
}
template<class F>
IsleGameResult drain(IsleEndLevel8Source* source,F&& function) noexcept {
    return SceneAccess::invoke_retiring(source ? &source->common : nullptr,[&](auto& owner) {
        if (owner.kind!=Resources::Kind::end_level_8 ||
            std::get_if<IsleEndLevel8Source>(&owner.context)!=source)
            return owner.fail(ISLE_GAME_INVALID_STATE,"End8 retirement Source mismatch");
        const auto status=function(owner);
        if (status!=ISLE_GAME_OK) (void)owner.fail(status,"End8 native retirement failed");
        return owner.failure!=ISLE_GAME_OK ? owner.failure :
            owner.cancelling ? ISLE_GAME_STOPPED : status;
    });
}
template<class O>
bool graphics_pending(O& owner) {
    auto& live=owner.live();
    if (!live.graphics) throw std::logic_error("End8 lost actual graphics owner");
    const auto completed=live.graphics->completed();
    return !live.graphics->quiescent() ||
        (completed && completed.ordinal!=live.released_ordinal);
}
template<class O>
EndLevel8SceneNative& state(O& owner) {
    auto& bound = std::get<EndLevel8SceneNative>(owner.live().menu);
    if (bound.source != &std::get<IsleEndLevel8Source>(owner.context) ||
        !bound.initial_data_admitted || !bound.resources ||
        owner.live().resources != bound.resources)
        throw std::logic_error("End8 native state/resources admission mismatch");
    return bound;
}
template<class O>
IsleGameResult joined_graphics(O& owner) {
    auto& live = owner.live();
    if (!live.graphics || !live.graphics->quiescent()) return ISLE_GAME_INVALID_STATE;
    const auto completed = live.graphics->completed();
    // Scene::update admits no busy or unconsumed frame. This checks the actual
    // submitted executor/readback and host consumer, never Presentation READY.
    if (completed && completed.ordinal != live.released_ordinal) return ISLE_GAME_INVALID_STATE;
    return live.graphics->error() == scene::graphics::Error::none ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;
}
IsleGameResult admit_initial_data(IsleEndLevel8Source* source,const IsleEndLevel8InitialData* data) noexcept {
    if (!data || data->rom != 0xa08d10 || data->guest != 0x801dd120 ||
        data->byte_count != initial_words.size()*4 || !data->words || !data->sha256 ||
        std::strcmp(data->sha256,initial_sha256) != 0) return ISLE_GAME_INVALID_ARGUMENT;
    return invoke(source,[&](auto& owner) {
        auto& bound=state(owner);
        for (std::size_t i=0;i<initial_words.size();++i)
            if (data->words[i]!=initial_words[i] ||
                bound.resources->word(data->guest+static_cast<std::uint32_t>(i*4))!=initial_words[i])
                return ISLE_GAME_INVALID_STATE;
        // Byte equality to the entire qualified bank establishes its fixed SHA.
        return ISLE_GAME_OK;
    });
}
IsleGameResult camera(IsleEndLevel8Source* source,IsleGameObject** object,IsleEndLevel8Camera** camera_out) noexcept {
    if (object) *object=nullptr;
    if (camera_out) *camera_out=nullptr;
    if (!object || !camera_out) return ISLE_GAME_INVALID_ARGUMENT;
    IsleGameObject* created=nullptr;
    IsleGameCamera* cam=nullptr;
    const auto result = invoke(source,[&](auto& owner) {
        auto& bound=state(owner);
        auto& live=owner.live();
        if (bound.camera || bound.camera_object || !bound.depth_admitted) return ISLE_GAME_INVALID_STATE;
        // ohCreateCamera(1,ohUpdateDefault,0,5,ren_func_800192DC,
        //                dlPriority10,dlMask32,tag-1,0,1,NULL,0,0), kinds3/14.
        created=live.services->object(1,0,5);
        if (!created) return ISLE_GAME_EXHAUSTED;
        live.source_objects.push_back(created);
        auto status=isle_game_attach_camera(source->common.game,created,&cam);
        if (status!=ISLE_GAME_OK) return status;
        status=isle_game_camera_add_matrix(source->common.game,cam,ISLE_GAME_CAMERA_PERSPECTIVE_FAST);
        if (status!=ISLE_GAME_OK) return status;
        status=isle_game_camera_add_matrix(source->common.game,cam,ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL);
        if (status!=ISLE_GAME_OK) return status;
        const IsleGameCameraParameters* existing=nullptr;
        status=isle_game_camera_parameters(source->common.game,cam,&existing);
        if (status!=ISLE_GAME_OK) return status;
        auto perspective=existing->perspective;
        perspective.near_plane=10; perspective.far_plane=25600; perspective.fovy=55;
        status=isle_game_camera_set_perspective(source->common.game,cam,&perspective);
        if (status!=ISLE_GAME_OK) return status;
        const IsleGameLookAt pose{{0,0,100},{0,0,0},{0,1,0}};
        status=isle_game_camera_set_look_at(source->common.game,cam,&pose);
        if (status!=ISLE_GAME_OK) return status;
        const IsleGameCameraRenderState render{5,0,UINT32_MAX};
        status=isle_game_camera_set_render_state(source->common.game,cam,&render);
        if (status!=ISLE_GAME_OK) return status;
        status=isle_game_object_set_flags(source->common.game,created,1);
        if (status!=ISLE_GAME_OK) return status;
        live.add_camera(created,cam,10,32,UINT32_MAX);
        auto& pass=live.passes[live.pass_count-1];
        pass.kind=scene::render::Kind::compiled;
        pass.tag_match=scene::render::TagMatch::intersection;
        pass.prepare=photo::Presentation::prepare_pass;
        // Parent binds userdata to its ONE Presentation at replay_initialize;
        // camera exists before original FB50 constructs that owner.
        pass.userdata=live.presentation.get();
        bound.camera_object=created; bound.camera=cam;
        return ISLE_GAME_OK;
    });
    if (result==ISLE_GAME_OK) { *object=created; *camera_out=cam; }
    return result;
}
IsleGameResult hidden(IsleEndLevel8Source* source,IsleGameObject* object,int value) noexcept {
    return invoke(source,[&](auto& owner) {
        auto& bound=state(owner);
        if (object!=bound.camera_object || (value!=0 && value!=1)) return ISLE_GAME_INVALID_ARGUMENT;
        IsleGameObjectMetadata metadata{};
        auto result=isle_game_object_metadata(source->common.game,object,&metadata);
        if (result!=ISLE_GAME_OK) return result;
        return isle_game_object_set_flags(source->common.game,object,value ? metadata.flags|1u : metadata.flags&~1u);
    });
}
IsleGameResult join_graphics(IsleEndLevel8Source* source,int* done) noexcept {
    if (done) *done=0;
    if (!done) return ISLE_GAME_INVALID_ARGUMENT;
    return drain(source,[&](auto& owner) {
        (void)state(owner);
        if (graphics_pending(owner)) return ISLE_GAME_OK;
        *done=1;
        return owner.live().graphics->error()==scene::graphics::Error::none ?
            ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;
    });
}
IsleGameResult presentation_begin(IsleEndLevel8Source* source,IsleEndLevel8Camera* camera,
                                  const IslePhotoRecord* record) noexcept {
    if (!record) return ISLE_GAME_INVALID_ARGUMENT;
    return invoke(source,[&](auto& owner) {
        auto& bound=state(owner); auto& live=owner.live();
        if (camera!=bound.camera || !live.presentation || joined_graphics(owner)!=ISLE_GAME_OK)
            return ISLE_GAME_INVALID_STATE;
        return live.presentation->begin(camera,*record) ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult presentation_poll(IsleEndLevel8Source* source,int* ready) noexcept {
    if (ready) *ready=0;
    if (!ready) return ISLE_GAME_INVALID_ARGUMENT;
    int value=0;
    const auto result=invoke(source,[&](auto& owner) {
        (void)state(owner); auto& p=owner.live().presentation;
        if (!p) return ISLE_GAME_INVALID_STATE;
        switch (p->poll()) {
        case ISLE_PHOTO_PRESENTATION_PENDING: return ISLE_GAME_OK;
        case ISLE_PHOTO_PRESENTATION_READY: value=1; return ISLE_GAME_OK;
        default: return ISLE_GAME_INVALID_STATE;
        }
    });
    if (result==ISLE_GAME_OK) *ready=value;
    return result;
}
IsleGameResult presentation_clear(IsleEndLevel8Source* source,int* done) noexcept {
    if (done) *done=0;
    if (!done) return ISLE_GAME_INVALID_ARGUMENT;
    return drain(source,[&](auto& owner) {
        (void)state(owner); auto& p=owner.live().presentation;
        if (!p) return ISLE_GAME_INVALID_STATE;
        if (graphics_pending(owner)) return ISLE_GAME_OK;
        const auto result=p->clear();
        if (result==ISLE_PHOTO_PRESENTATION_PENDING) return ISLE_GAME_OK;
        if (result==ISLE_PHOTO_PRESENTATION_IDLE || result==ISLE_PHOTO_PRESENTATION_FAILED) {
            *done=1;
            return result==ISLE_PHOTO_PRESENTATION_IDLE ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;
        }
        return ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult replay_initialize(IsleEndLevel8Source* source,int dl,int world,int photo_link) noexcept {
    return invoke(source,[&](auto& owner) {
        auto& bound=state(owner);
        if (dl!=5 || world!=9 || photo_link!=8 || !bound.camera) return ISLE_GAME_INVALID_ARGUMENT;
        auto result=owner.initialize_replay();
        if (result!=ISLE_GAME_OK) return result;
        auto& live=owner.live();
        if (!live.presentation) return ISLE_GAME_INVALID_STATE;
        for (std::size_t i=0;i<live.pass_count;++i) if (live.passes[i].camera==bound.camera) {
            auto& pass=live.passes[i];
            pass.kind=scene::render::Kind::compiled; pass.prepare=photo::Presentation::prepare_pass;
            pass.userdata=live.presentation.get();
            return ISLE_GAME_OK;
        }
        return ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult replay_cleanup(IsleEndLevel8Source* source,int* done) noexcept {
    if (done) *done=0;
    if (!done) return ISLE_GAME_INVALID_ARGUMENT;
    return drain(source,[&](auto& owner) {
        (void)state(owner); auto& live=owner.live();
        if (!live.presentation || !live.replay || !live.pipeline) return ISLE_GAME_INVALID_STATE;
        if (graphics_pending(owner)) return ISLE_GAME_OK;
        const auto status=live.presentation->retire();
        if (status==ISLE_PHOTO_PRESENTATION_PENDING) return ISLE_GAME_OK;
        if (status!=ISLE_PHOTO_PRESENTATION_RETIRED) return ISLE_GAME_INVALID_STATE;
        bool retired=false;
        const auto result=live.replay->retire(retired);
        if (!retired) return result;
        if (!live.pipeline->retire()) return result;
        *done=1;
        if (live.presentation->error()[0]) return ISLE_GAME_EXECUTION_FAILURE;
        return result;
    });
}
IsleGameResult fade(IsleEndLevel8Source* source,int reveal,float seconds) noexcept {
    return invoke(source,[&](auto& owner) {
        (void)state(owner);
        if ((reveal!=0 && reveal!=1) || seconds!=1.0f) return ISLE_GAME_INVALID_ARGUMENT;
        auto* transition=owner.live().services->screen_transition_source();
        if (!transition) return ISLE_GAME_INVALID_STATE;
        // Source func800A7860 is the existing nonwaiting returning C service.
        return isle_screen_transition_func_800A7860(transition,reveal,seconds);
    });
}
IsleGameResult fade_pending(IsleEndLevel8Source* source,int32_t* pending) noexcept {
    if (pending) *pending=0;
    if (!pending) return ISLE_GAME_INVALID_ARGUMENT;
    std::int32_t value=0;
    const auto result=invoke(source,[&](auto& owner) {
        (void)state(owner); auto* transition=owner.live().services->screen_transition_source();
        if (!transition || transition->status!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE;
        value=isle_screen_transition_func_800A7460(transition);
        return transition->status;
    });
    if (result==ISLE_GAME_OK) *pending=value;
    return result;
}
IsleGameResult input(IsleEndLevel8Source* source,int controller,IsleEndLevel8Input* output) noexcept {
    if (output) *output={};
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    IsleEndLevel8Input value{};
    const auto result=invoke(source,[&](auto& owner) {
        (void)state(owner);
        if (!owner.live().input_initialized || controller!=0) return ISLE_GAME_INVALID_ARGUMENT;
        const auto read=owner.input.menu(controller);
        if (read.error!=game_input::Error::none || !read.input) return ISLE_GAME_INVALID_STATE;
        value={read.input->current,read.input->pressed}; return ISLE_GAME_OK;
    });
    if (result==ISLE_GAME_OK) *output=value;
    return result;
}
IsleGameResult input_initialize(IsleEndLevel8Source* source) noexcept {
    return invoke(source,[](auto& owner) { (void)state(owner); return ::isle::post_course::initialize_input(owner); });
}
IsleGameResult sound(IsleEndLevel8Source* source,int id) noexcept {
    return invoke(source,[&](auto& owner) {
        (void)state(owner);
        if (id!=66 && id!=67) return ISLE_GAME_INVALID_ARGUMENT;
        return ::isle::post_course::sound(owner,id,0x7fff,64,1.0f,0);
    });
}
IsleGameResult volume(IsleEndLevel8Source* source,int player,int value) noexcept {
    return invoke(source,[&](auto& owner) {
        (void)state(owner);
        if (player!=0 || value!=0x7f00) return ISLE_GAME_INVALID_ARGUMENT;
        return ::isle::post_course::volume(owner,player,value,0);
    });
}
IsleGameResult aux_camera(IsleEndLevel8Source* source,int link,int mode) noexcept {
    return invoke(source,[&](auto& owner) {
        (void)state(owner);
        if (link!=1 || mode!=0x101) return ISLE_GAME_INVALID_ARGUMENT;
        auto& live=owner.live(); auto* transition=live.services->screen_transition_source();
        if (!transition || transition->camera_object) return ISLE_GAME_INVALID_STATE;
        auto result=isle_screen_transition_camera(transition,link,mode);
        if (result!=ISLE_GAME_OK) return result;
        auto* cam=transition->D_800BE2C8;
        result=::isle::post_course::viewport(owner,cam,320,240);
        if (result!=ISLE_GAME_OK) return result;
        const IsleGameCameraParameters* existing=nullptr;
        result=isle_game_camera_parameters(source->common.game,cam,&existing);
        if (result!=ISLE_GAME_OK) return result;
        auto perspective=existing->perspective;
        perspective.fovy=45; perspective.near_plane=8; perspective.far_plane=1024; perspective.scale=1;
        result=isle_game_camera_set_perspective(source->common.game,cam,&perspective);
        if (result!=ISLE_GAME_OK) return result;
        const IsleGameLookAt pose{{290,0,0},{0,0,0},{0,0,1}};
        result=isle_game_camera_set_look_at(source->common.game,cam,&pose);
        if (result!=ISLE_GAME_OK) return result;
        const IsleGameCameraRenderState render{4,0x10001,0};
        result=isle_game_camera_set_render_state(source->common.game,cam,&render);
        if (result!=ISLE_GAME_OK) return result;
        transition->D_800BE2C0=link; transition->camera_flags=4;
        transition->camera_mask=2; transition->camera_buffer_mask=2;
        transition->D_800AF054=transition->D_800AF058=transition->D_800AF05C=transition->D_800AF050=0;
        live.source_objects.push_back(transition->camera_object);
        live.add_camera(transition->camera_object,cam,1,2,2);
        return ISLE_GAME_OK;
    });
}
IsleGameResult ui_heap(IsleEndLevel8Source* source,std::size_t bytes) noexcept {
    return invoke(source,[&](auto& owner) { (void)state(owner); return ::isle::post_course::admit_heap(owner,bytes,0x32000); });
}
IsleGameResult depth_screen(IsleEndLevel8Source* source,std::size_t bytes,std::size_t alignment) noexcept {
    return invoke(source,[&](auto& owner) {
        auto& bound=state(owner); auto& live=owner.live();
        if (bytes!=0x25800 || alignment!=0x40 || bound.depth_admitted || !live.graphics)
            return ISLE_GAME_INVALID_ARGUMENT;
        const auto* depth=live.graphics->reserve_source_depth(0x25800,0x40);
        if (!depth) return ISLE_GAME_INVALID_STATE;
        const scene::graphics::SourceScreen screen{320,240,0x16a99,{0x803b5000,0x803da800,0}};
        if (!live.graphics->configure_source_screen(screen,depth)) return ISLE_GAME_INVALID_STATE;
        bound.depth_admitted=true; return ISLE_GAME_OK;
    });
}
IsleGameResult deallocate(IsleEndLevel8Source* source,void* allocation) noexcept {
    // Shared allocation metadata validates the real originating Window/native
    // allocation and generation, and survives setup retirement through entry.
    return drain(source,[&](auto& owner) { return SceneAccess::deallocate_native(owner,allocation); });
}
IsleGameResult process_object(IsleEndLevel8Source* source,int link,int dl,void* list,IsleGameObject** output) noexcept {
    if (output) *output=nullptr;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    IsleGameObject* object=nullptr;
    const auto result=invoke(source,[&](auto& owner) {
        (void)state(owner);
        if (link!=6 || dl!=0 || list) return ISLE_GAME_INVALID_ARGUMENT;
        return ::isle::post_course::process_object(owner,link,dl,list,&object);
    });
    if (result==ISLE_GAME_OK) *output=object;
    return result;
}
IsleGameResult scene_result(IsleEndLevel8Source* source,int32_t result) noexcept {
    return invoke(source,[&](auto& owner) {
        if (result!=23) return ISLE_GAME_INVALID_ARGUMENT;
        if (!owner.requested && !owner.native_retired) {
            // Original entry pre-latches D800AF3C0 before setup, not an exit.
            owner.output_scene=result; return ISLE_GAME_OK;
        }
        (void)state(owner); return owner.request_exit(result);
    });
}
IsleGameResult entry_begin(IsleEndLevel8Source* source,int32_t argument) noexcept {
    return invoke(source,[&](auto& owner) {
        if (owner.output_scene!=23) return ISLE_GAME_INVALID_STATE;
        return owner.setup_begin(argument,0x801dd130,0x801dd14c,0x16a99,1,1,1);
    });
}
IsleGameResult entry_poll(void* raw,int* complete,int32_t* output) noexcept {
    if (complete) *complete=0;
    if (output) *output=0;
    if (!complete || !output) return ISLE_GAME_INVALID_ARGUMENT;
    int done=0; std::int32_t result=0;
    const auto status=SceneAccess::invoke_owner(raw,[&](auto& owner) {
        if (owner.kind!=Resources::Kind::end_level_8 ||
            !std::get_if<IsleEndLevel8Source>(&owner.context)) return ISLE_GAME_INVALID_STATE;
        return owner.setup_poll(&done,&result);
    });
    if (status==ISLE_GAME_OK) { *complete=done; *output=result; }
    return status;
}
const IsleEndLevel8Native table{
    admit_initial_data,camera,hidden,join_graphics,presentation_begin,presentation_poll,
    presentation_clear,replay_initialize,replay_cleanup,fade,fade_pending,input,
    input_initialize,sound,volume,aux_camera,ui_heap,depth_screen,deallocate,
    process_object,scene_result,entry_begin,entry_poll
};
}
const IsleEndLevel8Native& end_level_8_scene_native_table() noexcept { return table; }
IsleGameResult end_level_8_scene_native_prepare(IsleEndLevel8Source* source,EndLevel8SceneNative& bound) noexcept {
    return invoke(source,[&](auto& owner) {
        if (bound.source || bound.resources) return ISLE_GAME_INVALID_STATE;
        auto resources=owner.live().resources;
        if (&std::get<EndLevel8SceneNative>(owner.live().menu)!=&bound)
            return ISLE_GAME_INVALID_STATE;
        if (!resources || resources->kind()!=Resources::Kind::end_level_8) return ISLE_GAME_INVALID_STATE;
        for (std::size_t i=0;i<initial_words.size();++i)
            if (resources->word(0x801dd120+static_cast<std::uint32_t>(i*4))!=initial_words[i])
                return ISLE_GAME_INVALID_STATE;
        bound.resources=std::move(resources); bound.source=source; bound.initial_data_admitted=true;
        source->requiredNativeTable=&table;
        return ISLE_GAME_OK;
    });
}
}

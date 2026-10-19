#include "gallery_scene_native.hpp"
#include "post_course_native_helpers.hpp"
#include "photo_presentation.hpp"
#include "menu_scene_bridge.h"
#include <bit>
#include <limits>
#include <utility>

namespace isle::post_course {
namespace {
using S = IsleGallerySource;
// qualified-640a initialized Snapshot, gallery_entry_initialized. Comparing
// every word also admits the actual screen/setup callback identities, not a
// host reinterpretation of SceneSetup or defaults supplied by this binding.
constexpr std::array<std::uint32_t,72> entry_words{
    0xe7000000,0,0xe3000800,0,0xd9fdffff,0,0xe3000f00,0,
    0xe3000d01,0,0xe3000c00,0x80000,0xe3001001,0,0xe3001201,0x2000,
    0xe3001402,0xc00,0xdb040004,6,0xdb04000c,6,0xdb040014,0xfffa,
    0xdb04001c,0xfffa,0xdf000000,0,
    0x803b5000,0x803da800,0,0,320,240,0x16a99,
    0,0x8000affc,0x8000ada0,0x80230af0,0,1,1,0xc000,0x2000,0,0,
    0x4400,0x20000,0x2000,0x800a1a50,0x80004860,0x10,0x2000,0x10,
    0,0x20,0,0x5c,0x400,0,0x801dce90,0,0x10,0,0x88,0,0x60,0,
    0x90,0x801dcddc,0,0
};
constexpr std::array<std::uint32_t,48> table_words{
    1,0,0,0xffffffff,0,0,0,
    2,0,3,0,26,0,20,0,34,0,4,0,35,0,
    2,0,3,0,26,0,20,0,34,0,32,0,4,0,35,0,
    0,0,0,0,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff
};
#define GALLERY_SCALARS(X) \
    X(D_801E16BC_9FF37C,0x801e16bc) \
    X(D_801EA1FC_A07EBC,0x801ea1fc) \
    X(D_801EA1F4_A07EB4,0x801ea1f4) \
    X(D_801EA1F8_A07EB8,0x801ea1f8) \
    X(D_801EA200_A07EC0,0x801ea200) \
    X(D_801EA204_A07EC4,0x801ea204) \
    X(D_801EA288_A07F48,0x801ea288) \
    X(D_801EA28C_A07F4C,0x801ea28c) \
    X(D_801EA290_A07F50,0x801ea290) \
    X(D_801EA294_A07F54,0x801ea294) \
    X(D_801EA298_A07F58,0x801ea298) \
    X(D_801EA29C_A07F5C,0x801ea29c) \
    X(D_801EA2A0_A07F60,0x801ea2a0) \
    X(D_801EA2A4_A07F64,0x801ea2a4) \
    X(D_801EA2A8_A07F68,0x801ea2a8) \
    X(D_801EA2AC_A07F6C,0x801ea2ac) \
    X(D_801EA2B0_A07F70,0x801ea2b0)

template<class O> GallerySceneNative* state_for(O& owner, S* source) {
    if (!owner.native || std::get_if<S>(&owner.context) != source ||
        (source->common.scene != 22 && source->common.scene != 23)) return nullptr;
    auto* state = std::get_if<GallerySceneNative>(&owner.native->menu);
    return state && state->source == source && state->native_allocation == owner.native.get() ? state : nullptr;
}
template<class F> IsleGameResult call(S* source,F&& fn) noexcept {
    return SceneAccess::invoke(source ? &source->common : nullptr,[&](auto& owner) {
        auto* state = state_for(owner,source);
        if (!state) return ISLE_GAME_INVALID_STATE;
        if (state->failure != ISLE_GAME_OK) return state->failure;
        if (state->cancel_requested || owner.cancelling) return ISLE_GAME_STOPPED;
        const auto result = fn(owner,*state);
        if (result != ISLE_GAME_OK && result != ISLE_GAME_STOPPED && state->failure == ISLE_GAME_OK)
            state->failure = result;
        return result;
    });
}
IsleGameResult validate(S* source,const IsleGalleryInitialData* initial) noexcept {
    return call(source,[&](auto& owner,auto& state) {
        if (!state.prepared || initial != &state.initial ||
            initial->canonical_receipt != state.resources.get() ||
            state.resources != owner.live().resources ||
            owner.photos.native_source() != source->common.photos ||
            initial->D_801EA210_A07ED0 != state.absent_buttons.data() ||
            initial->D_801EA248_A07F08 != state.present_buttons.data() ||
            initial->absent_button_count != 7 || initial->present_button_count != 8)
            return ISLE_GAME_INVALID_ARGUMENT;
#define COMPARE(field,address) if (std::bit_cast<std::uint32_t>(initial->field) != state.resources->word(address)) return ISLE_GAME_INVALID_STATE;
        GALLERY_SCALARS(COMPARE)
#undef COMPARE
        return ISLE_GAME_OK;
    });
}
IsleGameResult input(S* source,int32_t player,IsleGalleryInput* out) noexcept {
    if (out) *out = {};
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    return call(source,[&](auto& owner,auto&) {
        if (!owner.live().input_initialized) return ISLE_GAME_INVALID_STATE;
        const auto value = owner.input.menu(player);
        if (value.error != game_input::Error::none || !value.input) return ISLE_GAME_INVALID_STATE;
        *out = {value.input->current,value.input->pressed};
        return ISLE_GAME_OK;
    });
}
IsleGameResult input_initialize(S* s) noexcept {
    return call(s,[](auto& owner,auto&) { return initialize_input(owner); });
}
IsleGameResult repeat(S* s,int32_t a,int32_t b) noexcept {
    return call(s,[&](auto& owner,auto&) { return owner.input.set_menu_repeat(a,b) == game_input::Error::none ? ISLE_GAME_OK : ISLE_GAME_INVALID_ARGUMENT; });
}
IsleGameResult mask(S* s,uint32_t bits) noexcept {
    return call(s,[&](auto& owner,auto&) { return owner.input.set_menu_mask(bits) == game_input::Error::none ? ISLE_GAME_OK : ISLE_GAME_INVALID_ARGUMENT; });
}
IsleGameResult camera(S* s,int32_t mode,IsleGameObject** out) noexcept {
    if (out) *out = nullptr;
    if (!out || (mode != 0 && mode != 1)) return ISLE_GAME_INVALID_ARGUMENT;
    return call(s,[&](auto& owner,auto& state) {
        auto& native = owner.live();
        auto*& object = mode ? state.photo_object : state.main_object;
        auto*& view = mode ? state.photo_camera : state.main_camera;
        if (object || view) return ISLE_GAME_INVALID_STATE;
        IsleGameResult result;
        if (!mode) {
            result = camera_owner(owner,0x200,5,5,object,view);
            if (result != ISLE_GAME_OK) return result;
            result = main_camera_pose(owner,view,true);
            if (result != ISLE_GAME_OK) return result;
            const IsleGameCameraRenderState render{4,0x10001,0x100000};
            result = isle_game_camera_set_render_state(s->common.game,view,&render);
            if (result != ISLE_GAME_OK) return result;
            native.add_camera(object,view,20,0x100000,0x100000);
        } else {
            // 9FA580:78-95: default viewport/aspect, matrix3+matrix14,
            // not main-camera helper's matrix6 or a guessed 321/241 aspect.
            object = native.services->object(0x200,5,5);
            if (!object) return ISLE_GAME_INVALID_STATE;
            native.source_objects.push_back(object);
            result = isle_game_attach_camera(s->common.game,object,&view);
            if (result != ISLE_GAME_OK) return result;
            result = isle_game_camera_add_matrix(s->common.game,view,ISLE_GAME_CAMERA_PERSPECTIVE_FAST);
            if (result != ISLE_GAME_OK) return result;
            result = isle_game_camera_add_matrix(s->common.game,view,ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL);
            if (result != ISLE_GAME_OK) return result;
            const IsleGameCameraParameters* current = nullptr;
            result = isle_game_camera_parameters(s->common.game,view,&current);
            if (result != ISLE_GAME_OK) return result;
            auto perspective = current->perspective;
            perspective.near_plane = 10; perspective.far_plane = 25600; perspective.fovy = 55;
            result = isle_game_camera_set_perspective(s->common.game,view,&perspective);
            if (result != ISLE_GAME_OK) return result;
            const IsleGameLookAt look{{0,0,100},{0,0,0},{0,1,0}};
            result = isle_game_camera_set_look_at(s->common.game,view,&look);
            if (result != ISLE_GAME_OK) return result;
            const IsleGameCameraRenderState render{5,0,0xffffffff};
            result = isle_game_camera_set_render_state(s->common.game,view,&render);
            if (result != ISLE_GAME_OK) return result;
            IsleGameObjectMetadata metadata{};
            result = isle_game_object_metadata(s->common.game,object,&metadata);
            if (result != ISLE_GAME_OK) return result;
            result = isle_game_object_set_flags(s->common.game,object,metadata.flags | 1u);
            if (result != ISLE_GAME_OK) return result;
            native.add_camera(object,view,19,0x80000,0xffffffff);
        }
        *out = object;
        return ISLE_GAME_OK;
    });
}
IsleGameResult aux_camera(S* s,int32_t link,int32_t mode) noexcept {
    return call(s,[&](auto& owner,auto&) {
        if (link != 1 || mode != 0x101) return ISLE_GAME_INVALID_ARGUMENT;
        auto& native = owner.live();
        auto* transition = native.services->screen_transition_source();
        if (!transition || transition->camera_object) return ISLE_GAME_INVALID_STATE;
        auto result = isle_screen_transition_camera(transition,link,mode);
        if (result != ISLE_GAME_OK) return result;
        auto* view = transition->D_800BE2C8;
        result = viewport(owner,view,320,240);
        if (result != ISLE_GAME_OK) return result;
        const IsleGamePerspective perspective{45,321.0f/241.0f,8,1024,1};
        result = isle_game_camera_set_perspective(s->common.game,view,&perspective);
        if (result != ISLE_GAME_OK) return result;
        const IsleGameLookAt look{{290,0,0},{0,0,0},{0,0,1}};
        result = isle_game_camera_set_look_at(s->common.game,view,&look);
        if (result != ISLE_GAME_OK) return result;
        const IsleGameCameraRenderState render{4,0x10001,0};
        result = isle_game_camera_set_render_state(s->common.game,view,&render);
        if (result != ISLE_GAME_OK) return result;
        transition->D_800BE2C0 = link; transition->camera_flags = 4;
        transition->camera_mask = transition->camera_buffer_mask = 2;
        transition->D_800AF054 = transition->D_800AF058 = transition->D_800AF05C = transition->D_800AF050 = 0;
        native.source_objects.push_back(transition->camera_object);
        native.add_camera(transition->camera_object,view,1,2,2);
        return ISLE_GAME_OK;
    });
}
IsleGameResult process(S* s,int32_t link,int32_t draw,void* list,IsleGameObject** out) noexcept {
    if (out) *out = nullptr;
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    return call(s,[&](auto& owner,auto&) { return process_object(owner,link,draw,list,out); });
}
void emit_reset(GallerySceneNative& state) {
    // The admitted list contains only these state commands and EndDL. Inline
    // its body into the actual Frame cursor; never execute EndDL in its caller.
    for (std::size_t i = 0; i < 26; i += 2) {
        state.control->command(entry_words[i],entry_words[i+1]);
        ++state.display.commands;
    }
}
IsleGameResult display_list(S* s,void** cursor) noexcept {
    if (!cursor) return ISLE_GAME_INVALID_ARGUMENT;
    const auto token = *cursor;
    *cursor = nullptr;
    return call(s,[&](auto&,auto& state) {
        if (!state.control || token != &state.display || state.display.state != &state)
            return ISLE_GAME_INVALID_STATE;
        emit_reset(state);
        *cursor = &state.display;
        return ISLE_GAME_OK;
    });
}
IsleGameResult render_state(S* s,int32_t,int32_t) noexcept {
    return call(s,[&](auto&,auto& state) {
        if (!state.control) return ISLE_GAME_INVALID_STATE;
        emit_reset(state);
        state.control->command(0xe3001801,0); // G_CD_MAGICSQ
        state.control->command(0xe7000000,0);
        state.control->command(0xe200001c,0x0c4c2078);
        state.control->command(0xfc1219ff,0xfffffe38); // MODULATEIA / PASS2
        state.display.commands += 4;
        return ISLE_GAME_OK;
    });
}
IsleGameResult screen(S* s,size_t bytes,size_t alignment) noexcept {
    return call(s,[&](auto& owner,auto& state) {
        if (bytes != 0x25800 || alignment != 0x40 || state.screen_applied) return ISLE_GAME_INVALID_ARGUMENT;
        auto& native = owner.live();
        const auto* depth = native.graphics->reserve_source_depth(static_cast<std::uint32_t>(bytes),static_cast<std::uint32_t>(alignment));
        if (!depth) return ISLE_GAME_INVALID_STATE;
        const scene::graphics::SourceScreen settings{320,240,0x16a99,{0x803b5000,0x803da800,0}};
        if (!native.graphics->configure_source_screen(settings,depth)) return ISLE_GAME_INVALID_STATE;
        state.screen_applied = true;
        return ISLE_GAME_OK;
    });
}
IsleGameResult heap(S* s,size_t bytes) noexcept {
    return call(s,[&](auto& owner,auto&) { return admit_heap(owner,bytes,0x46000); });
}
IsleGameResult deallocate(S* s,void* allocation) noexcept {
    return SceneAccess::invoke_retiring(s ? &s->common : nullptr,[&](auto& owner) {
        auto* state = state_for(owner,s);
        if (!state) return ISLE_GAME_INVALID_STATE;
        return SceneAccess::deallocate_native(owner,allocation);
    });
}
IsleGameResult hidden(S* s,IsleGameObject* object,int value) noexcept {
    return call(s,[&](auto&,auto&) {
        if (!object || (value != 0 && value != 1)) return ISLE_GAME_INVALID_ARGUMENT;
        IsleGameObjectMetadata metadata{};
        auto result = isle_game_object_metadata(s->common.game,object,&metadata);
        if (result != ISLE_GAME_OK) return result;
        return isle_game_object_set_flags(s->common.game,object,value ? metadata.flags | 1u : metadata.flags & ~1u);
    });
}
IsleGameResult first(S* s,int32_t link,IsleGameObject** out) noexcept {
    if (out) *out = nullptr;
    if (!out || link < 0 || link >= 32) return ISLE_GAME_INVALID_ARGUMENT;
    return call(s,[&](auto&,auto&) { *out = isle_menu_first_object(s->common.game,static_cast<std::uint8_t>(link)); return ISLE_GAME_OK; });
}
IsleGameResult next(S* s,IsleGameObject* object,IsleGameObject** out) noexcept {
    if (out) *out = nullptr;
    if (!out || !object) return ISLE_GAME_INVALID_ARGUMENT;
    return call(s,[&](auto&,auto&) {
        IsleGameObjectMetadata metadata{};
        const auto result = isle_game_object_metadata(s->common.game,object,&metadata);
        if (result != ISLE_GAME_OK) return result;
        *out = isle_menu_next_object(s->common.game,object);
        return ISLE_GAME_OK;
    });
}
IsleGameResult object_id(S* s,IsleGameObject* object,int32_t* out) noexcept {
    if (out) *out = 0;
    if (!out || !object) return ISLE_GAME_INVALID_ARGUMENT;
    return call(s,[&](auto&,auto&) {
        std::uint32_t id = 0;
        const auto result = isle_game_object_id(s->common.game,object,&id);
        if (result == ISLE_GAME_OK) *out = std::bit_cast<std::int32_t>(id);
        return result;
    });
}
IsleGameResult pause(S* s,IsleGameObject* object) noexcept {
    return call(s,[&](auto&,auto&) {
        if (!object) return ISLE_GAME_INVALID_ARGUMENT;
        // Source's actual link18 then link17 walks choose each object. No
        // automatic pause/resume at begin, poll, clear or unrelated links.
        return isle_game_pause_object_processes(s->common.game,object,1);
    });
}
IsleGameResult graphics_join(S* s,int32_t* pending) noexcept {
    if (pending) *pending = 0;
    if (!pending) return ISLE_GAME_INVALID_ARGUMENT;
    return call(s,[&](auto& owner,auto&) {
        auto& native = owner.live();
        const auto polled = native.graphics->poll();
        if (polled == scene::graphics::Poll::failed) return ISLE_GAME_EXECUTION_FAILURE;
        const auto completed = native.graphics->completed();
        *pending = !native.graphics->quiescent() || (completed && completed.ordinal != native.released_ordinal);
        return ISLE_GAME_OK;
    });
}
IsleGameResult present(S* s,IsleGameObject* object,const IslePlayerPhotoData* photo) noexcept {
    if (!photo || !object) return ISLE_GAME_INVALID_ARGUMENT;
    return call(s,[&](auto& owner,auto& state) {
        auto& native = owner.live();
        if (!native.presentation || object != state.photo_object || !native.graphics->quiescent()) return ISLE_GAME_INVALID_STATE;
        IsleGameObjectMetadata metadata{};
        auto result = isle_game_object_metadata(s->common.game,object,&metadata);
        if (result != ISLE_GAME_OK) return result;
        if (!(metadata.flags & 1u)) return ISLE_GAME_INVALID_STATE;
        const auto identity = owner.photos.identify(photo);
        if (!identity || !identity.value.photo) return ISLE_GAME_INVALID_ARGUMENT;
        return native.presentation->begin(state.photo_camera,identity.value) ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult present_poll(S* s,int32_t* pending) noexcept {
    if (pending) *pending = 0;
    if (!pending) return ISLE_GAME_INVALID_ARGUMENT;
    return call(s,[&](auto& owner,auto&) {
        auto* presentation = owner.live().presentation.get();
        if (!presentation) return ISLE_GAME_INVALID_STATE;
        const auto status = presentation->poll();
        if (presentation->error()[0]) return ISLE_GAME_EXECUTION_FAILURE;
        if (status == ISLE_PHOTO_PRESENTATION_READY) return ISLE_GAME_OK;
        if (status != ISLE_PHOTO_PRESENTATION_PENDING) return ISLE_GAME_INVALID_STATE;
        *pending = 1;
        return ISLE_GAME_OK;
    });
}
IsleGameResult present_release(S* s,int32_t* pending) noexcept {
    if (pending) *pending = 0;
    if (!pending) return ISLE_GAME_INVALID_ARGUMENT;
    return call(s,[&](auto& owner,auto&) {
        auto* presentation = owner.live().presentation.get();
        if (!presentation || !owner.live().graphics->quiescent()) return ISLE_GAME_INVALID_STATE;
        const auto status = presentation->release_to_replay();
        if (presentation->error()[0]) return ISLE_GAME_EXECUTION_FAILURE;
        if (status == ISLE_PHOTO_PRESENTATION_IDLE) return ISLE_GAME_OK;
        if (status != ISLE_PHOTO_PRESENTATION_PENDING) return ISLE_GAME_INVALID_STATE;
        *pending = 1;
        return ISLE_GAME_OK;
    });
}
IsleGameResult fade(S* s,int32_t direction,float duration) noexcept {
    return call(s,[&](auto& owner,auto&) { return isle_screen_transition_func_800A7860(owner.live().services->screen_transition_source(),direction,duration); });
}
IsleGameResult fade_poll(S* s,int32_t* out) noexcept {
    if (out) *out = 0;
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    return call(s,[&](auto& owner,auto&) {
        auto* transition = owner.live().services->screen_transition_source();
        if (!transition) return ISLE_GAME_INVALID_STATE;
        if (transition->status != ISLE_GAME_OK) return transition->status;
        const auto value = isle_screen_transition_func_800A7460(transition);
        if (transition->status != ISLE_GAME_OK) return transition->status;
        *out = value; return ISLE_GAME_OK;
    });
}
IsleGameResult clear_matrices(S* s,IsleGameSprite* sprite) noexcept {
    return call(s,[&](auto&,auto&) {
        IsleSprite* view = nullptr;
        const auto result = isle_post_window_sprite_view(s->common.window,sprite,&view);
        if (result != ISLE_GAME_OK) return result;
        return isle_game_sprite_clear_display_cache(s->common.game,sprite);
    });
}
IsleGameResult scale(S* s,float* out) noexcept {
    if (out) *out = 0;
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    return call(s,[&](auto& owner,auto&) { *out = owner.modules.preview_scale; return ISLE_GAME_OK; });
}
IsleGameResult replay_initialize(S* s) noexcept {
    return call(s,[&](auto& owner,auto& state) {
        const auto result = owner.initialize_replay();
        if (result != ISLE_GAME_OK) return result;
        auto& native = owner.live();
        if (!native.presentation || !state.photo_camera) return ISLE_GAME_INVALID_STATE;
        for (std::size_t i = 0; i < native.pass_count; ++i) if (native.passes[i].camera == state.photo_camera) {
            auto& pass = native.passes[i];
            pass.kind = scene::render::Kind::compiled;
            pass.prepare = photo::Presentation::prepare_pass;
            pass.userdata = native.presentation.get();
            return ISLE_GAME_OK;
        }
        return ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult replay_cleanup(S* s,int32_t* pending) noexcept {
    if (pending) *pending = 0;
    if (!pending) return ISLE_GAME_INVALID_ARGUMENT;
    return call(s,[&](auto& owner,auto&) {
        auto& native = owner.live();
        if (!native.replay_initialized || !native.replay || !native.pipeline || !native.presentation)
            return ISLE_GAME_INVALID_STATE;
        const auto completed = native.graphics->completed();
        if (!native.graphics->quiescent() || (completed && completed.ordinal != native.released_ordinal)) {
            *pending = 1; return ISLE_GAME_OK;
        }
        const auto presented = native.presentation->retire();
        if (native.presentation->error()[0]) return ISLE_GAME_EXECUTION_FAILURE;
        if (presented == ISLE_PHOTO_PRESENTATION_PENDING) { *pending = 1; return ISLE_GAME_OK; }
        if (presented != ISLE_PHOTO_PRESENTATION_RETIRED) return ISLE_GAME_INVALID_STATE;
        bool joined = false;
        const auto result = native.replay->retire(joined);
        if (result != ISLE_GAME_OK) return result;
        if (!joined) { *pending = 1; return ISLE_GAME_OK; }
        if (!native.pipeline->retire()) {
            if (native.pipeline->error()[0]) return ISLE_GAME_EXECUTION_FAILURE;
            *pending = 1;
        }
        // Joined facades/pipeline remain alive through actual Source detach.
        return ISLE_GAME_OK;
    });
}
IsleGameResult printer_available(S* s,int32_t* out) noexcept {
    if (out) *out = 0;
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    return call(s,[&](auto& owner,auto& state) {
        state.printer_requirement = owner.input.require(game_input::Operation::printer);
        if (state.printer_requirement != game_input::Error::unsupported) return ISLE_GAME_INVALID_STATE;
        // This is the real controller service's absent accessory projection.
        // No fake printer instance, cache/RAM test, or successful command.
        *out = 0; return ISLE_GAME_OK;
    });
}
IsleGameResult printer_status(S* s,int32_t* out) noexcept {
    if (out) *out = 0;
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    return call(s,[](auto& owner,auto& state) {
        state.printer_requirement = owner.input.require(game_input::Operation::printer);
        return ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult printer_command(S* s,int32_t) noexcept {
    return call(s,[](auto& owner,auto& state) {
        state.printer_requirement = owner.input.require(game_input::Operation::printer);
        return ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult sound_default(S* s,int32_t id) noexcept {
    return call(s,[&](auto& owner,auto&) { return sound(owner,id,0x7fff,64,1,0); });
}
IsleGameResult sound_parameters(S* s,int32_t id,int32_t volume,int32_t pan,float pitch,int32_t reverb) noexcept {
    return call(s,[&](auto& owner,auto&) { return sound(owner,id,volume,pan,pitch,reverb); });
}
IsleGameResult music(S* s,int32_t channel,int32_t id) noexcept {
    return call(s,[&](auto& owner,auto&) { return song(owner,channel,id); });
}
IsleGameResult music_volume(S* s,int32_t channel,int32_t value) noexcept {
    return call(s,[&](auto& owner,auto&) { return volume(owner,channel,value,0); });
}
IsleGameResult music_smooth(S* s,int32_t channel,int32_t value,int32_t frames) noexcept {
    return call(s,[&](auto& owner,auto&) { return volume(owner,channel,value,frames); });
}
IsleGameResult scene_result(S* s,int32_t value) noexcept {
    return call(s,[&](auto& owner,auto&) {
        if (value != 24 && value != 8) return ISLE_GAME_INVALID_ARGUMENT;
        return owner.request_exit(value);
    });
}
IsleGameResult name(S* s,int32_t id,const char** out) noexcept {
    if (out) *out = nullptr;
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    return call(s,[&](auto& owner,auto&) { return pokemon_name(owner,id,*out); });
}
IsleGameResult save(S* s,int32_t* out) noexcept {
    if (out) *out = 0;
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    return call(s,[&](auto& owner,auto& state) {
        if (owner.photos.source().status != ISLE_GAME_OK) return owner.photos.source().status;
        state.last_commit = owner.photos.save_session().commit(owner.live().services->raw_game_time);
        const auto& result = state.last_commit;
        *out = result.outcome == isle::save::CommitOutcome::published && result.generation != 0 &&
            result.durability != isle::save::Durability::none && !result.error && !result.cleanup_error ? 0 : 1;
        // Real persistence failure is Original scalar1 (error image + wait).
        // Photo observer/native faults above and exceptions remain native faults.
        return ISLE_GAME_OK;
    });
}
IsleGameResult entry_begin(S* s,int32_t argument) noexcept {
    return call(s,[&](auto& owner,auto&) {
        if ((argument != 22 && argument != 23) || argument != owner.argument) return ISLE_GAME_INVALID_ARGUMENT;
        return owner.setup_begin(argument,0x801e1690,0x801e16ac,0x16a99,1,1,1);
    });
}
IsleGameResult entry_poll(void* raw,int* completed,int32_t* result) noexcept {
    if (completed) *completed = 0;
    if (result) *result = 0;
    if (!completed || !result) return ISLE_GAME_INVALID_ARGUMENT;
    int done = 0; int32_t value = 0;
    const auto status = SceneAccess::invoke_owner(raw,[&](auto& owner) {
        auto* s = std::get_if<S>(&owner.context);
        if (!s || !state_for(owner,s)) return ISLE_GAME_INVALID_STATE;
        return owner.setup_poll(&done,&value);
    });
    if (status == ISLE_GAME_OK) { *completed = done; *result = value; }
    return status;
}
const IsleGalleryNative table{
    .initial_validate=validate,.input=input,.input_initialize=input_initialize,
    .input_repeat=repeat,.input_mask=mask,.camera=camera,.aux_camera=aux_camera,
    .process_object=process,.display_list=display_list,.render_state=render_state,
    .screen=screen,.ui_heap=heap,.deallocate=deallocate,.object_hidden=hidden,
    .object_first=first,.object_next=next,.object_id=object_id,.object_pause=pause,
    .graphics_join=graphics_join,.presentation_begin=present,.presentation_poll=present_poll,
    .presentation_release=present_release,.fade=fade,.fade_poll=fade_poll,
    .sprite_clear_matrices=clear_matrices,.preview_scale=scale,
    .replay_initialize=replay_initialize,.replay_cleanup=replay_cleanup,
    .printer_available=printer_available,.printer_status=printer_status,
    .printer_command=printer_command,.sound=sound_default,.sound_params=sound_parameters,
    .song=music,.volume=music_volume,.volume_smooth=music_smooth,.scene_result=scene_result,
    .pokemon_name=name,.save=save,.entry_begin=entry_begin,.entry_poll=entry_poll
};
}
GallerySceneNative::GallerySceneNative(application::Photos& owner) noexcept
    : working{application::PhotoWorking(owner),application::PhotoWorking(owner),
              application::PhotoWorking(owner),application::PhotoWorking(owner)},photos(&owner) {
    for (std::size_t i = 0; i < working.size(); ++i) working_handles[i] = working[i].source();
    display.state = this;
}
void GallerySceneNative::request_cancel() noexcept { cancel_requested = true; }
bool GallerySceneNative::cancel_drain(scene::graphics::Frame& frame,photo::Presentation* presentation) noexcept {
    cancel_requested = true;
    if (frame.poll() == scene::graphics::Poll::failed && failure == ISLE_GAME_OK) failure = ISLE_GAME_EXECUTION_FAILURE;
    if (!frame.quiescent()) return false;
    if (presentation) {
        const auto status = presentation->retire();
        if (presentation->error()[0] && failure == ISLE_GAME_OK) failure = ISLE_GAME_EXECUTION_FAILURE;
        if (status != ISLE_PHOTO_PRESENTATION_RETIRED) return false;
    }
    return true; // Parent still joins actual Replay/FBC4 before releasing state.
}
const IsleGalleryNative& gallery_scene_native_table() noexcept { return table; }
IsleGameResult gallery_scene_native_prepare(S* source,GallerySceneNative& state) noexcept {
    return SceneAccess::invoke(source ? &source->common : nullptr,[&](auto& owner) {
        auto& native = owner.live();
        if (std::get_if<S>(&owner.context) != source || std::get_if<GallerySceneNative>(&native.menu) != &state ||
            owner.kind != Resources::Kind::gallery || (owner.argument != 22 && owner.argument != 23) ||
            state.prepared || state.photos != &owner.photos || !native.resources ||
            native.resources->kind() != Resources::Kind::gallery) return ISLE_GAME_INVALID_STATE;
        state.resources = native.resources;
        const auto& resources = *state.resources;
        for (std::size_t i = 0; i < entry_words.size(); ++i)
            if (resources.word(0x801e1620 + static_cast<std::uint32_t>(i*4)) != entry_words[i]) return ISLE_GAME_INVALID_STATE;
        for (std::size_t i = 0; i < table_words.size(); ++i)
            if (resources.word(0x801ea1f4 + static_cast<std::uint32_t>(i*4)) != table_words[i]) return ISLE_GAME_INVALID_STATE;
#define READ_FIELD(field,address) state.initial.field = std::bit_cast<std::int32_t>(resources.word(address));
        GALLERY_SCALARS(READ_FIELD)
#undef READ_FIELD
        const auto decode = [&](auto& buttons,std::uint32_t guest) {
            for (std::size_t i = 0; i < buttons.size(); ++i) {
                const auto address = guest + static_cast<std::uint32_t>(i*8);
                if (resources.word(address+4)) return false;
                buttons[i] = {std::bit_cast<std::int32_t>(resources.word(address)),{nullptr,0}};
            }
            return true;
        };
        if (!decode(state.absent_buttons,0x801ea210) || !decode(state.present_buttons,0x801ea248)) return ISLE_GAME_INVALID_STATE;
        for (auto guest : {0x801e80e8u,0x801e82a0u,0x801e83d0u,0x801e8890u,0x801e8d50u,0x801e8fb0u,0x801ea1b0u}) (void)resources.sprite_index(guest);
        state.initial.canonical_receipt = state.resources.get();
        state.initial.D_801EA210_A07ED0 = state.absent_buttons.data();
        state.initial.D_801EA248_A07F08 = state.present_buttons.data();
        state.initial.absent_button_count = state.absent_buttons.size();
        state.initial.present_button_count = state.present_buttons.size();
        state.source = source; state.native_allocation = &native; state.prepared = true;
        return ISLE_GAME_OK;
    });
}
#undef GALLERY_SCALARS
namespace {
bool control_call(void* raw,scene::graphics::SourceControl& control,bool render) {
    if (!raw) return false;
    auto& state = *static_cast<GallerySceneNative*>(raw);
    if (!state.source || state.control || state.cancel_requested ||
        state.display.dispatch == std::numeric_limits<std::uint64_t>::max()) return false;
    const auto admission = call(state.source,[&](auto&,auto& actual) {
        return &actual == &state ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
    });
    if (admission != ISLE_GAME_OK) return false;
    state.control = &control; ++state.display.dispatch; state.display.commands = 0;
    const auto status = isle_post_course_source_call(&state.source->common,
        render ? +[](IslePostCourseSceneSource* common,void*) {
            isle_gallery_func_801DC948_9FA608(reinterpret_cast<S*>(common),0,0);
        } : +[](IslePostCourseSceneSource* common,void* raw_cursor) {
            void* cursor = raw_cursor;
            isle_gallery_func_801DC8C0_9FA580(reinterpret_cast<S*>(common),&cursor);
        },&state.display);
    state.control = nullptr;
    if (status != ISLE_GAME_OK || state.failure != ISLE_GAME_OK) return false;
    control.draw();
    return true;
}
}
bool gallery_scene_native_display_control(void* raw,scene::graphics::SourceControl& control) {
    return control_call(raw,control,false);
}
bool gallery_scene_native_render_control(void* raw,scene::graphics::SourceControl& control) {
    return control_call(raw,control,true);
}
}

#include "post_course_native_helpers.hpp"
#include "application_photos.hpp"
#include <bit>
#include <cmath>
#include <limits>
#include <utility>


using isle::post_course::invoke;

extern "C" IsleGameResult isle_camera_check_native_setup_begin(IsleCameraCheckSource* source,
    int32_t argument, uint32_t screen, uint32_t setup, uint32_t flags,
    int near_disabled, int update_interval, int draw_interval) {
    return invoke(source, [&](auto& owner) { return owner.setup_begin(argument,screen,setup,
        flags,near_disabled,update_interval,draw_interval); });
}
extern "C" IsleGameResult isle_camera_check_native_setup_poll(IsleCameraCheckSource* source,
    int* complete, int32_t* result) {
    if (complete) *complete = 0;
    if (result) *result = 0;
    if (!complete || !result) return ISLE_GAME_INVALID_ARGUMENT;
    int completed = 0;
    int32_t value = 0;
    const auto status = invoke(source, [&](auto& owner) { return owner.setup_poll(&completed,&value); });
    if (status == ISLE_GAME_OK) { *complete = completed; *result = value; }
    return status;
}
extern "C" IsleGameResult isle_camera_check_native_ui_heap(IsleCameraCheckSource* source, size_t bytes) {
    return invoke(source, [&](auto& owner) { return isle::post_course::admit_heap(owner,bytes,0x40000); });
}
extern "C" IsleGameResult isle_photo_check_native_ui_heap(IslePhotoCheckSource* source, size_t bytes) {
    return invoke(source, [&](auto& owner) { return isle::post_course::admit_heap(owner,bytes,0x32000); });
}
extern "C" IsleGameResult isle_camera_check_native_input(IsleCameraCheckSource* source,
    int controller, int neutral, const IsleCameraCheckInput** output) {
    if (output) *output = nullptr;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return invoke(source, [&](auto& owner) {
        auto& native = owner.live();
        if (!native.input_initialized) return ISLE_GAME_INVALID_STATE;
        const auto value = neutral ? owner.input.neutral(controller) : owner.input.menu(controller);
        if (value.error != isle::game_input::Error::none || !value.input) return ISLE_GAME_INVALID_STATE;
        const auto index = controller >= 0 && controller < 4 ? static_cast<std::size_t>(controller) : 0;
        auto& projection = native.camera_inputs[index];
        projection = {value.input->current,value.input->pressed};
        native.camera_input_observed[index] = true;
        *output = &projection;
        return ISLE_GAME_OK;
    });
}
extern "C" IsleGameResult isle_photo_check_native_input(IslePhotoCheckSource* source,
    int32_t controller, IslePhotoCheckInput* output) {
    if (output) *output = {};
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return invoke(source, [&](auto& owner) {
        if (!owner.live().input_initialized) return ISLE_GAME_INVALID_STATE;
        const auto value = owner.input.menu(controller);
        if (value.error != isle::game_input::Error::none || !value.input) return ISLE_GAME_INVALID_STATE;
        *output = {value.input->current,value.input->pressed};
        return ISLE_GAME_OK;
    });
}
extern "C" IsleGameResult isle_camera_check_native_replay_initialize(IsleCameraCheckSource* source) {
    return invoke(source, [](auto& owner) { return owner.initialize_replay(); });
}
extern "C" IsleGameResult isle_photo_check_native_replay_initialize(IslePhotoCheckSource* source) {
    return invoke(source, [](auto& owner) { return owner.initialize_replay(); });
}
extern "C" IsleGameResult isle_camera_check_native_replay_cleanup(IsleCameraCheckSource* source) {
    return invoke(source, [](auto& owner) { return owner.cleanup_replay(); });
}
extern "C" IsleGameResult isle_photo_check_native_replay_cleanup(IslePhotoCheckSource* source) {
    return invoke(source, [](auto& owner) { return owner.cleanup_replay(); });
}
extern "C" IsleGameResult isle_camera_check_native_preview_scale(IsleCameraCheckSource* source, float* output) {
    if (output) *output = 0;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return invoke(source, [&](auto& owner) { *output = owner.modules.preview_scale; return ISLE_GAME_OK; });
}
extern "C" IsleGameResult isle_photo_check_native_preview_scale(IslePhotoCheckSource* source, float* output) {
    if (output) *output = 0;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return invoke(source, [&](auto& owner) { *output = owner.modules.preview_scale; return ISLE_GAME_OK; });
}
extern "C" IsleGameResult isle_camera_check_native_sound(IsleCameraCheckSource* source,
    int id, int value, int pan, float pitch, int reverb, int32_t* output) {
    if (output) *output = 0;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return invoke(source, [&](auto& owner) { return isle::post_course::sound(owner,id,value,pan,pitch,reverb,output); });
}
extern "C" IsleGameResult isle_photo_check_native_sound(IslePhotoCheckSource* source, int32_t id) {
    return invoke(source, [&](auto& owner) { return isle::post_course::sound(owner,id,0x7fff,64,1.0f,0); });
}
extern "C" IsleGameResult isle_photo_check_native_sound_params(IslePhotoCheckSource* source,
    int32_t id, int32_t value, int32_t pan, float pitch, int32_t reverb) {
    return invoke(source, [&](auto& owner) { return isle::post_course::sound(owner,id,value,pan,pitch,reverb); });
}
extern "C" IsleGameResult isle_camera_check_native_song(IsleCameraCheckSource* source, int player, int id) {
    return invoke(source, [&](auto& owner) { return isle::post_course::song(owner,player,id); });
}
extern "C" IsleGameResult isle_photo_check_native_song(IslePhotoCheckSource* source, int32_t player, int32_t id) {
    return invoke(source, [&](auto& owner) { return isle::post_course::song(owner,player,id); });
}
extern "C" IsleGameResult isle_camera_check_native_volume(IsleCameraCheckSource* source, int player, int value, int frames) {
    return invoke(source, [&](auto& owner) { return isle::post_course::volume(owner,player,value,frames); });
}
extern "C" IsleGameResult isle_photo_check_native_volume(IslePhotoCheckSource* source, int32_t player, int32_t value) {
    return invoke(source, [&](auto& owner) { return isle::post_course::volume(owner,player,value,0); });
}
extern "C" IsleGameResult isle_photo_check_native_volume_smooth(IslePhotoCheckSource* source,
    int32_t player, int32_t value, int32_t frames) {
    return invoke(source, [&](auto& owner) { return isle::post_course::volume(owner,player,value,frames); });
}
extern "C" IsleGameResult isle_camera_check_native_flag(IsleCameraCheckSource* source, int id, int* output) {
    if (output) *output = 0;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return invoke(source, [&](auto& owner) { return isle::post_course::player_flag(owner,id,*output); });
}
extern "C" IsleGameResult isle_photo_check_native_player_flag(IslePhotoCheckSource* source, int32_t id, int32_t* output) {
    if (output) *output = 0;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return invoke(source, [&](auto& owner) { return isle::post_course::player_flag(owner,id,*output); });
}
extern "C" IsleGameResult isle_camera_check_native_set_flag(IsleCameraCheckSource* source, int id, int value) {
    return invoke(source, [&](auto& owner) {
        if (id < 0) return ISLE_GAME_INVALID_ARGUMENT;
        return owner.photos.save_session().set_player_flag(static_cast<unsigned>(id),value != 0) ?
            ISLE_GAME_INVALID_STATE : ISLE_GAME_OK;
    });
}
extern "C" IsleGameResult isle_camera_check_native_level(IsleCameraCheckSource* source, int32_t* output) {
    if (output) *output = 0;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return invoke(source, [&](auto& owner) { *output = owner.manager.source_level(); return ISLE_GAME_OK; });
}
extern "C" IsleGameResult isle_photo_check_native_level_id(IslePhotoCheckSource* source, int32_t* output) {
    if (output) *output = 0;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return invoke(source, [&](auto& owner) { *output = owner.manager.source_level(); return ISLE_GAME_OK; });
}
extern "C" IsleGameResult isle_camera_check_native_roll_count(IsleCameraCheckSource* source, int32_t* output) {
    if (output) *output = 0;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return invoke(source, [&](auto& owner) { *output = owner.photos.count(); return ISLE_GAME_OK; });
}
extern "C" IsleGameResult isle_camera_check_native_music_state(IsleCameraCheckSource* source, IsleCameraCheckMusicState** output) {
    if (output) *output = nullptr;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return invoke(source, [&](auto& owner) { *output = &owner.modules.music; return ISLE_GAME_OK; });
}
extern "C" IsleGameResult isle_camera_check_native_name(IsleCameraCheckSource* source, int kind, int32_t id, const char** output) {
    if (output) *output = nullptr;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return invoke(source, [&](auto& owner) {
        switch (kind) {
        case 0: return isle::post_course::course_name(owner,id,*output);
        case 1: return isle::post_course::pokemon_name(owner,id,*output);
        case 2: return isle::post_course::player_name(owner,*output);
        default: return ISLE_GAME_INVALID_ARGUMENT;
        }
    });
}
extern "C" IsleGameResult isle_photo_check_native_player_name(IslePhotoCheckSource* source, const char** output) {
    if (output) *output = nullptr;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return invoke(source, [&](auto& owner) { return isle::post_course::player_name(owner,*output); });
}
extern "C" IsleGameResult isle_photo_check_native_pokemon_name(IslePhotoCheckSource* source, int32_t id, const char** output) {
    if (output) *output = nullptr;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return invoke(source, [&](auto& owner) { return isle::post_course::pokemon_name(owner,id,*output); });
}
extern "C" IsleGameResult isle_photo_check_native_level_name(IslePhotoCheckSource* source, int32_t id, const char** output) {
    if (output) *output = nullptr;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return invoke(source, [&](auto& owner) { return isle::post_course::course_name(owner,id,*output); });
}
extern "C" IsleGameResult isle_camera_check_native_course_score(IsleCameraCheckSource* source, int level, int32_t* output) {
    if (output) *output = 0;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return invoke(source, [&](auto& owner) {
        auto saved = owner.photos.save_session().snapshot();
        if (!saved) return ISLE_GAME_INVALID_STATE;
        auto value = saved.value.course_score(static_cast<std::size_t>(level));
        if (!value) return ISLE_GAME_INVALID_ARGUMENT;
        *output = value.value;
        return ISLE_GAME_OK;
    });
}
extern "C" IsleGameResult isle_camera_check_native_set_course_score(IsleCameraCheckSource* source, int level, int32_t value) {
    return invoke(source, [&](auto& owner) {
        return owner.photos.save_session().set_course_score(static_cast<std::size_t>(level),value) ?
            ISLE_GAME_INVALID_ARGUMENT : ISLE_GAME_OK;
    });
}
extern "C" IsleGameResult isle_camera_check_native_challenge_score(IsleCameraCheckSource* source, int level, int32_t* output) {
    if (output) *output = 0;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return invoke(source, [&](auto& owner) {
        if (level >= 0 && level < 7)
            *output = std::bit_cast<std::int32_t>(owner.live().resources->word(0x803a66a0u + 4u * static_cast<unsigned>(level)));
        return ISLE_GAME_OK;
    });
}
extern "C" IsleGameResult isle_photo_check_native_selected_course(IslePhotoCheckSource* source, int32_t* output) {
    if (output) *output = 0;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return invoke(source, [&](auto& owner) {
        auto saved = owner.photos.save_session().snapshot();
        if (!saved) return ISLE_GAME_INVALID_STATE;
        auto value = saved.value.selected_course();
        if (!value) return ISLE_GAME_INVALID_STATE;
        *output = static_cast<std::int32_t>(value.value);
        return ISLE_GAME_OK;
    });
}
extern "C" IsleGameResult isle_photo_check_native_unlock_progress(IslePhotoCheckSource* source, int32_t* output) {
    if (output) *output = 0;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return invoke(source, [&](auto& owner) {
        auto saved = owner.photos.save_session().snapshot();
        if (!saved) return ISLE_GAME_INVALID_STATE;
        auto value = saved.value.unlocked_course();
        if (!value) return ISLE_GAME_INVALID_STATE;
        *output = value.value;
        return ISLE_GAME_OK;
    });
}
extern "C" IsleGameResult isle_photo_check_native_set_unlock_progress(IslePhotoCheckSource* source, int32_t value) {
    return invoke(source, [&](auto& owner) {
        return owner.photos.save_session().set_unlocked_course(value) ? ISLE_GAME_INVALID_STATE : ISLE_GAME_OK;
    });
}
extern "C" IsleGameResult isle_camera_check_native_notice(IsleCameraCheckSource* source, int notice) {
    return invoke(source, [&](auto& owner) { owner.state.add_scene_notices(static_cast<std::uint32_t>(notice)); return ISLE_GAME_OK; });
}
extern "C" IsleGameResult isle_photo_check_native_notices_or(IslePhotoCheckSource* source, uint32_t notices) {
    return invoke(source, [&](auto& owner) { owner.state.add_scene_notices(notices); return ISLE_GAME_OK; });
}
extern "C" IsleGameResult isle_photo_check_native_notices_test(IslePhotoCheckSource* source, uint32_t notices, int32_t* output) {
    if (output) *output = 0;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return invoke(source, [&](auto& owner) { *output = owner.state.consume_scene_notices(notices) ? 1 : 0; return ISLE_GAME_OK; });
}
extern "C" IsleGameResult isle_camera_check_native_transition(IsleCameraCheckSource* source, int result) {
    return invoke(source, [&](auto& owner) { return owner.request_exit(result); });
}
extern "C" IsleGameResult isle_photo_check_native_scene_result(IslePhotoCheckSource* source, int32_t result) {
    return invoke(source, [&](auto& owner) { return owner.request_exit(result); });
}
extern "C" IsleGameResult isle_camera_check_native_zero_focus_caption(IsleCameraCheckSource* source,
    const IslePlayerPhotoData* photo, int* populated) {
    if (populated) *populated = 0;
    if (!populated || !photo) return ISLE_GAME_INVALID_ARGUMENT;
    return invoke(source, [&](auto& owner) {
        const auto identity = owner.photos.identify(photo);
        if (!identity || identity.value.storage != ISLE_PHOTO_REPORT || !identity.value.photo)
            return ISLE_GAME_INVALID_ARGUMENT;
        const auto record = owner.photos.record(ISLE_PHOTO_REPORT,identity.value.index);
        if (!record || record.value.photo != photo || record.value.revision != identity.value.revision)
            return ISLE_GAME_INVALID_STATE;
        // Authorized native policy (parent Bead20): an actual canonical Report
        // record is populated. This intentionally corrects the original unwritten
        // zero-focus scratch result; ROM proof establishes only the nonzero-focus
        // branch, including zero-score signs. Never infer presence from score or
        // reproduce undefined stack residue as an original numeric result.
        *populated = 1;
        return ISLE_GAME_OK;
    });
}
extern "C" IsleGameResult isle_photo_check_native_entry_begin(IslePhotoCheckSource* source, int32_t argument) {
    return invoke(source, [&](auto& owner) {
        return owner.setup_begin(argument,0x801e5410u,0x801e542cu,0x16a99u,1,1,1);
    });
}

extern "C" IsleGameResult isle_camera_check_native_process_owner(IsleCameraCheckSource* source,
    int link, int draw_link, IsleGameObject** output) {
    if (output) *output = nullptr;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return invoke(source, [&](auto& owner) {
        return isle::post_course::process_object(owner,link,draw_link,nullptr,output);
    });
}
extern "C" IsleGameResult isle_photo_check_native_process_object(IslePhotoCheckSource* source,
    int32_t link, int32_t draw_link, void* display_list, IsleGameObject** output) {
    if (output) *output = nullptr;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return invoke(source, [&](auto& owner) {
        return isle::post_course::process_object(owner,link,draw_link,display_list,output);
    });
}

extern "C" IsleGameResult isle_camera_check_native_camera(IsleCameraCheckSource* source, IsleGameObject** output) {
    if (output) *output = nullptr;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return invoke(source, [&](auto& owner) { return isle::post_course::create_main_camera(owner,false,output); });
}
extern "C" IsleGameResult isle_photo_check_native_camera(IslePhotoCheckSource* source, IsleGameObject** output) {
    if (output) *output = nullptr;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    return invoke(source, [&](auto& owner) { return isle::post_course::create_main_camera(owner,true,output); });
}
extern "C" IsleGameResult isle_camera_check_native_input_initialize(IsleCameraCheckSource* source) {
    return invoke(source, [](auto& owner) { return isle::post_course::initialize_input(owner); });
}
extern "C" IsleGameResult isle_photo_check_native_input_initialize(IslePhotoCheckSource* source) {
    return invoke(source, [](auto& owner) { return isle::post_course::initialize_input(owner); });
}
extern "C" IsleGameResult isle_camera_check_native_repeat(IsleCameraCheckSource* source, int delay, int interval) {
    return invoke(source, [&](auto& owner) {
        return owner.input.set_menu_repeat(delay,interval) == isle::game_input::Error::none ?
            ISLE_GAME_OK : ISLE_GAME_INVALID_ARGUMENT;
    });
}
extern "C" IsleGameResult isle_photo_check_native_input_repeat(IslePhotoCheckSource* source, int32_t delay, int32_t interval) {
    return invoke(source, [&](auto& owner) {
        return owner.input.set_menu_repeat(delay,interval) == isle::game_input::Error::none ?
            ISLE_GAME_OK : ISLE_GAME_INVALID_ARGUMENT;
    });
}
extern "C" IsleGameResult isle_camera_check_native_input_mask(IsleCameraCheckSource* source, uint32_t mask) {
    return invoke(source, [&](auto& owner) {
        return owner.input.set_menu_mask(mask) == isle::game_input::Error::none ?
            ISLE_GAME_OK : ISLE_GAME_INVALID_ARGUMENT;
    });
}
extern "C" IsleGameResult isle_photo_check_native_input_mask(IslePhotoCheckSource* source, uint32_t mask) {
    return invoke(source, [&](auto& owner) {
        return owner.input.set_menu_mask(mask) == isle::game_input::Error::none ?
            ISLE_GAME_OK : ISLE_GAME_INVALID_ARGUMENT;
    });
}
extern "C" IsleGameResult isle_photo_check_native_userdata_set(IslePhotoCheckSource* source,
    IsleGameObject* object, const void* value) {
    return invoke(source, [&](auto& owner) {
        return isle_game_object_set_userdata(owner.live().services->game,object,value);
    });
}
extern "C" IsleGameResult isle_photo_check_native_entry_poll(void* owner, int* retired, int32_t* result) {
    if (retired) *retired = 0;
    if (result) *result = 0;
    if (!retired || !result) return ISLE_GAME_INVALID_ARGUMENT;
    int complete = 0;
    int32_t value = 0;
    const auto status = isle::post_course::SceneAccess::invoke_owner(owner,
        [&](auto& scene) { return scene.setup_poll(&complete,&value); });
    if (status == ISLE_GAME_OK) { *retired = complete; *result = value; }
    return status;
}
extern "C" IsleGameResult isle_photo_check_native_aux_camera(IslePhotoCheckSource* source,
    int32_t link, int32_t mode) {
    return invoke(source, [&](auto& owner) {
        if (link < 0 || link >= 32) return ISLE_GAME_INVALID_ARGUMENT;
        auto& native = owner.live();
        auto* transition = native.services->screen_transition_source();
        if (!transition || transition->camera_object) return ISLE_GAME_INVALID_STATE;
        // Reuse the canonical transition owner's returning allocation leaf;
        // never call func800A7F68's original C body under a C++ stack.
        auto status = isle_screen_transition_camera(transition,link,mode);
        if (status != ISLE_GAME_OK) return status;
        auto* camera = transition->D_800BE2C8;
        const auto size = static_cast<std::uint32_t>(mode) & 3u;
        if (size) {
            status = isle::post_course::viewport(owner,camera,320u*size,240u*size);
            if (status != ISLE_GAME_OK) return status;
        }
        const IsleGameCameraParameters* existing = nullptr;
        status = isle_game_camera_parameters(owner.source().game,camera,&existing);
        if (status != ISLE_GAME_OK) return status;
        auto perspective = existing->perspective;
        perspective.fovy = 45;
        perspective.near_plane = 8;
        perspective.far_plane = 1024;
        perspective.scale = 1;
        status = isle_game_camera_set_perspective(owner.source().game,camera,&perspective);
        if (status != ISLE_GAME_OK) return status;
        const IsleGameLookAt pose{{290,0,0},{0,0,0},{0,0,1}};
        status = isle_game_camera_set_look_at(owner.source().game,camera,&pose);
        if (status != ISLE_GAME_OK) return status;
        const auto flags = (static_cast<std::uint32_t>(mode) & 0x100u) ? 4u : 5u;
        const IsleGameCameraRenderState render{flags,0x10001,0};
        status = isle_game_camera_set_render_state(owner.source().game,camera,&render);
        if (status != ISLE_GAME_OK) return status;
        const auto mask = std::uint32_t(1) << link;
        transition->D_800BE2C0 = link;
        transition->camera_flags = flags;
        transition->camera_mask = mask;
        transition->camera_buffer_mask = mask;
        transition->D_800AF054 = 0;
        transition->D_800AF058 = 0;
        transition->D_800AF05C = 0;
        transition->D_800AF050 = 0;
        native.source_objects.push_back(transition->camera_object);
        native.add_camera(transition->camera_object,camera,1,mask,mask);
        return ISLE_GAME_OK;
    });
}

extern "C" IsleGameResult isle_post_course_native_input_invalidate(IslePostCourseSceneSource* source) {
    return isle::post_course::SceneAccess::invoke(source, [](auto& owner) {
        return owner.input.begin_update() == isle::game_input::Error::none ?
            ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
    });
}

extern "C" IsleGameResult isle_post_course_native_progress(IslePostCourseSceneSource* source) {
    return isle::post_course::SceneAccess::invoke(source, [](auto& owner) {
        return owner.native_progress(&owner);
    });
}

namespace isle::post_course {
IsleGameResult Scene::Impl::background_color(std::uint32_t* output) const noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output=0;
    if (creator!=std::this_thread::get_id()) return ISLE_GAME_WRONG_OWNER;
    const auto& rgb=state.background_rgb;
    *output=(std::uint32_t(rgb[0])<<24)|(std::uint32_t(rgb[1])<<16)|
        (std::uint32_t(rgb[2])<<8)|1u;
    return ISLE_GAME_OK;
}

IsleGameResult SceneAccess::credits_display_context(IsleCreditsDisplayListCursor** cursor,
    IsleCreditsSource** output) noexcept {
    if (output) *output=nullptr;
    if (!output || !cursor || !*cursor || !(*cursor)->source)
        return ISLE_GAME_INVALID_ARGUMENT;
    auto* source=(*cursor)->source;
    return invoke(&source->common,[&](auto& owner) {
        if (!owner.native || owner.native->retired || owner.kind!=Resources::Kind::credits ||
            std::get_if<IsleCreditsSource>(&owner.context)!=source ||
            source->native!=&credits_scene_native_table() || source->argument!=owner.argument ||
            owner.argument<17 || owner.argument>20 || source->common.scene!=owner.argument ||
            source->common.game!=owner.native->storage.game())
            return ISLE_GAME_INVALID_STATE;
        auto* state=std::get_if<CreditsSceneNative>(&owner.native->menu);
        if (!state || !state->prepared || state->source!=source || !state->display_active ||
            cursor!=&state->display_current ||
            (*cursor!=&state->display[0] && *cursor!=&state->display[1]))
            return ISLE_GAME_INVALID_STATE;
        if (source->common.status!=ISLE_GAME_OK) return source->common.status;
        if (state->failure!=ISLE_GAME_OK) return state->failure;
        if (state->cancel_requested) return ISLE_GAME_STOPPED;
        const auto& current=**cursor;
        if (!current.control || !current.generation ||
            current.generation!=current.control->epoch())
            return ISLE_GAME_INVALID_STATE;
        for (const auto& entry:state->display) {
            if (entry.allocation!=owner.native.get() || entry.source!=source ||
                entry.state!=state || entry.generation!=current.generation ||
                entry.control!=current.control)
                return ISLE_GAME_INVALID_STATE;
        }
        *output=source;
        return ISLE_GAME_OK;
    });
}

IsleGameResult SceneAccess::credits_cleanup_context(void* allocation,
    IsleCreditsSource** output) noexcept {
    if (output) *output=nullptr;
    if (!output || !allocation) return ISLE_GAME_INVALID_ARGUMENT;
    // The SDK owns this native envelope; Original's argument is neither a
    // Source struct nor guest cache data. There is currently no SDK producer.
    auto* envelope=static_cast<Scene::Impl::Native::UiAllocation*>(allocation);
    auto* source=envelope->source;
    if (!source || !envelope->owner) return ISLE_GAME_INVALID_ARGUMENT;
    return invoke_retiring(&source->common,[&](auto& owner) {
        if (envelope->owner!=&owner || !owner.native || owner.native->retired ||
            owner.kind!=Resources::Kind::credits ||
            std::get_if<IsleCreditsSource>(&owner.context)!=source ||
            source->native!=&credits_scene_native_table() || source->argument!=owner.argument ||
            owner.argument<17 || owner.argument>20 || source->common.scene!=owner.argument ||
            source->common.game!=owner.native->storage.game())
            return ISLE_GAME_INVALID_STATE;
        auto& native=*owner.native;
        auto* state=std::get_if<CreditsSceneNative>(&native.menu);
        if (!state || !state->prepared || state->source!=source ||
            !native.activated || !native.heap_admitted || !native.heap)
            return ISLE_GAME_INVALID_STATE;
        bool owned=false;
        for (const auto& entry:native.ui_allocations) {
            if (&entry==envelope) {
                owned=true;
                break;
            }
        }
        if (!owned || envelope->allocation!=&native || !envelope->payload ||
            !envelope->bytes || !envelope->generation ||
            envelope->generation!=owner.modules.window_generation ||
            !native.heap->owns_allocation(envelope->payload,envelope->bytes))
            return ISLE_GAME_INVALID_ARGUMENT;
        *output=source;
        return ISLE_GAME_OK;
    });
}
}

#include "post_course_scene_internal.hpp"
#include "application_photos.hpp"
#include <bit>
#include <cmath>
#include <limits>
#include <utility>

namespace isle::post_course {
namespace {
template<class C, class F>
IsleGameResult invoke(C* source, F&& function) noexcept {
    return SceneAccess::invoke(source ? &source->common : nullptr,
                               std::forward<F>(function));
}

template<class O>
IsleGameResult admit_heap(O& owner, std::size_t bytes, std::size_t expected) {
    auto& native = owner.live();
    if (bytes != expected) return ISLE_GAME_INVALID_ARGUMENT;
    if (native.heap_admitted || !native.heap || native.heap_bytes != bytes ||
        native.heap->capacity() != bytes) return ISLE_GAME_INVALID_STATE;
    native.heap_admitted = true;
    return ISLE_GAME_OK;
}

template<class O>
IsleGameResult player_flag(O& owner, int id, std::int32_t& output) {
    auto& source = owner.photos.source();
    if (source.status != ISLE_GAME_OK) return source.status;
    const auto value = isle_player_photo_checkPlayerFlag(&source, id);
    if (source.status != ISLE_GAME_OK) return source.status;
    output = value;
    return ISLE_GAME_OK;
}

template<class O>
IsleGameResult pokemon_name(O& owner, int id, const char*& output) {
    auto& source = owner.photos.source();
    if (source.status != ISLE_GAME_OK) return source.status;
    const auto* name = isle_player_photo_getPokemonName(&source, id);
    if (source.status != ISLE_GAME_OK) return source.status;
    output = name;
    return ISLE_GAME_OK;
}

template<class O>
IsleGameResult player_name(O& owner, const char*& output) {
    auto saved = owner.photos.save_session().snapshot();
    if (!saved) return ISLE_GAME_INVALID_STATE;
    const auto bytes = saved.value.player_name();
    auto& name = owner.live().player_name;
    if (bytes.size() != name.size() - 1) return ISLE_GAME_INVALID_STATE;
    std::memcpy(name.data(), bytes.data(), bytes.size());
    name.back() = '\0';
    output = name.data();
    return ISLE_GAME_OK;
}

template<class O>
IsleGameResult course_name(O& owner, int id, const char*& output) {
    if (id >= 0 && id < static_cast<int>(owner.modules.course_names.size())) {
        const auto name = owner.modules.course_names[static_cast<std::size_t>(id)];
        if (name.data() == nullptr) return ISLE_GAME_INVALID_STATE;
        output = name.data();
    }
    return ISLE_GAME_OK;
}

template<class O>
IsleGameResult sound(O& owner, int id, int volume, int pan, float pitch,
                     int reverb, std::int32_t* output = nullptr) {
    if (id < 0 || volume < 0 || volume > 0x7fff || pan < 0 || pan > 127 ||
        reverb < 0 || reverb > 127 || !std::isfinite(pitch) || pitch <= 0)
        return ISLE_GAME_INVALID_ARGUMENT;
    game_audio::Parameters parameters;
    parameters.volume = static_cast<std::uint16_t>(volume);
    parameters.pan = static_cast<std::uint8_t>(pan);
    parameters.pitch = pitch;
    parameters.reverb = static_cast<std::uint8_t>(reverb);
    int handle = -1;
    const auto result = owner.audio.play_sound(static_cast<std::uint32_t>(id), parameters, handle);
    if (result != audio::Error::none) return ISLE_GAME_INVALID_STATE;
    if (output) *output = handle;
    return ISLE_GAME_OK;
}

template<class O>
IsleGameResult song(O& owner, int player, int id) {
    if (player < 0 || id < 0) return ISLE_GAME_INVALID_ARGUMENT;
    return owner.audio.play_song(static_cast<unsigned>(player), static_cast<std::uint32_t>(id)) ==
        audio::Error::none ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
}

template<class O>
IsleGameResult volume(O& owner, int player, int value, int frames) {
    if (player < 0 || value < 0 || frames < 0) return ISLE_GAME_INVALID_ARGUMENT;
    return owner.audio.bgm_volume(static_cast<unsigned>(player), static_cast<std::uint32_t>(value),
        static_cast<std::uint32_t>(frames)) == audio::Error::none ?
        ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
}

template<class O>
IsleGameResult process_object(O& owner, int link, int draw_link, const void* display_list,
                             IsleGameObject** output) {
    // Every admitted 800A85E8/800A86A4 caller supplies a NULL DObj list.
    // Its real typed root still exists; it contributes no invented geometry.
    if (link < 0 || link >= 32 || draw_link >= 32 || display_list)
        return ISLE_GAME_INVALID_ARGUMENT;
    auto& native = owner.live();
    const auto tag = draw_link < 0 ? 0u : std::uint32_t(1) << draw_link;
    const auto render_link = draw_link < 0 ? 0 : draw_link;
    auto* object = native.services->object(std::uint32_t(1) << link,
        static_cast<std::uint8_t>(link),static_cast<std::uint32_t>(link));
    if (!object) return ISLE_GAME_INVALID_STATE;
    // Original TRANSLATE + ROTATE_RPY + SCALE has zero translation/rotation
    // and unit scale for these immutable, NULL-list owners. The canonical
    // native RTS root is matrix-equivalent here; this is not admission of a
    // new general display-list or contextual rotation recipe.
    const IsleGameModelNodeInit root{std::numeric_limits<std::uint32_t>::max(),
        {{0,0,0},{0,0,0},{1,1,1}},ISLE_GAME_MODEL_ROTATE_RPY_TRANSLATE_SCALE};
    IsleGameModel* model = nullptr;
    auto result = isle_game_attach_model(owner.source().game,object,&root,1,&model);
    if (result != ISLE_GAME_OK) return result;
    if (!native.services->set_render(object,{static_cast<std::uint8_t>(render_link),
        static_cast<std::uint32_t>(link),tag})) return ISLE_GAME_INVALID_STATE;
    native.source_objects.push_back(object);
    *output = object;
    return ISLE_GAME_OK;
}

template<class O>
IsleGameResult camera_owner(O& owner, std::uint32_t id, std::uint8_t link,
                           std::uint32_t priority, IsleGameObject*& object,
                           IsleGameCamera*& camera) {
    auto& native = owner.live();
    object = native.services->object(id,link,priority);
    if (!object) return ISLE_GAME_INVALID_STATE;
    auto result = isle_game_attach_camera(owner.source().game,object,&camera);
    if (result != ISLE_GAME_OK) return result;
    result = isle_game_camera_add_matrix(owner.source().game,camera,ISLE_GAME_CAMERA_PERSPECTIVE_FAST);
    if (result != ISLE_GAME_OK) return result;
    result = isle_game_camera_add_matrix(owner.source().game,camera,ISLE_GAME_CAMERA_LOOK_AT);
    if (result != ISLE_GAME_OK) return result;
    native.source_objects.push_back(object);
    return ISLE_GAME_OK;
}

template<class O>
IsleGameResult viewport(O& owner, IsleGameCamera* camera, std::uint32_t width, std::uint32_t height) {
    if (!width || !height || width > 16382 || height > 16382)
        return ISLE_GAME_INVALID_ARGUMENT;
    const IsleGameCameraParameters* existing = nullptr;
    auto result = isle_game_camera_parameters(owner.source().game,camera,&existing);
    if (result != ISLE_GAME_OK) return result;
    auto perspective = existing->perspective;
    auto view = existing->viewport;
    // Source 53AD0.c72-111 uses inclusive extent+1, then INTEGER halves.
    // In particular the aspect is 641/481 (Camera) or 321/241 (Photo),
    // not a replacement 4:3 projection.
    perspective.aspect = static_cast<float>(width+1) / static_cast<float>(height+1);
    view.scale[0] = view.translate[0] = static_cast<std::int16_t>(((width+1)/2)*4);
    view.scale[1] = view.translate[1] = static_cast<std::int16_t>(((height+1)/2)*4);
    result = isle_game_camera_set_perspective(owner.source().game,camera,&perspective);
    if (result != ISLE_GAME_OK) return result;
    return isle_game_camera_set_viewport(owner.source().game,camera,&view);
}

template<class O>
IsleGameResult main_camera_pose(O& owner, IsleGameCamera* camera, bool photo) {
    auto result = viewport(owner,camera,photo ? 320u : 640u,photo ? 240u : 480u);
    if (result != ISLE_GAME_OK) return result;
    const IsleGameCameraParameters* existing = nullptr;
    result = isle_game_camera_parameters(owner.source().game,camera,&existing);
    if (result != ISLE_GAME_OK) return result;
    auto perspective = existing->perspective;
    perspective.fovy = 33;
    perspective.far_plane = 8000;
    perspective.near_plane = 62.5f;
    perspective.scale = 1;
    result = isle_game_camera_set_perspective(owner.source().game,camera,&perspective);
    if (result != ISLE_GAME_OK) return result;
    // Actual initialized GAME data: D800AF094 @ ROM5AA44 and D800AF0C4
    // @ ROM5AA74, pinned df8815e7. These are not a conventional Z-up guess.
    const IsleGameLookAt pose{{-1,0,0},{0,0,0},{0,0,1}};
    return isle_game_camera_set_look_at(owner.source().game,camera,&pose);
}

template<class O>
IsleGameResult create_main_camera(O& owner, bool photo, IsleGameObject** output) {
    IsleGameObject* object = nullptr;
    IsleGameCamera* camera = nullptr;
    auto result = camera_owner(owner,0x200,5,5,object,camera);
    if (result != ISLE_GAME_OK) return result;
    result = main_camera_pose(owner,camera,photo);
    if (result != ISLE_GAME_OK) return result;
    const IsleGameCameraRenderState render{4,photo ? 1u : 0x7101u,0x100000};
    result = isle_game_camera_set_render_state(owner.source().game,camera,&render);
    if (result != ISLE_GAME_OK) return result;
    owner.live().add_camera(object,camera,20,0x100000,0x100000);
    *output = object;
    return ISLE_GAME_OK;
}

template<class O>
IsleGameResult initialize_input(O& owner) {
    auto& native = owner.live();
    if (native.input_initialized) return ISLE_GAME_INVALID_STATE;
    auto result = owner.input.reset_scene_menu();
    if (result != game_input::Error::none) return ISLE_GAME_INVALID_STATE;
    // AAE28 creates the real link9 cache-process owner. Original C registers
    // its kind0/priority0 callback only after this returning leaf has unwound.
    IsleGameObject* input_object = nullptr;
    auto status = process_object(owner,9,0,nullptr,&input_object);
    if (status != ISLE_GAME_OK) return status;
    owner.source().input_object = input_object;
    // AA1DC creates both native sprite cameras. Their matrices and camera
    // storage are real; the sprite renderer consumes their exact viewport.
    for (unsigned i = 0; i < 2; ++i) {
        const auto draw_link = i == 0 ? 2u : 30u;
        const auto update_link = static_cast<std::uint8_t>(i == 0 ? 3 : 10);
        IsleGameObject* object = nullptr;
        IsleGameCamera* camera = nullptr;
        status = camera_owner(owner,0x200,update_link,update_link,object,camera);
        if (status != ISLE_GAME_OK) return status;
        status = viewport(owner,camera,owner.screen_words[4],owner.screen_words[5]);
        if (status != ISLE_GAME_OK) return status;
        const IsleGameCameraRenderState render{4,0xff,0};
        status = isle_game_camera_set_render_state(owner.source().game,camera,&render);
        if (status != ISLE_GAME_OK) return status;
        const auto mask = std::uint32_t(1) << draw_link;
        native.add_camera(object,camera,draw_link,mask,mask);
    }
    native.input_initialized = true;
    return ISLE_GAME_OK;
}
}
}

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

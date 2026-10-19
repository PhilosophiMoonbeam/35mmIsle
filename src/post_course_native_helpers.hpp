#ifndef ISLE_POST_COURSE_NATIVE_HELPERS_HPP
#define ISLE_POST_COURSE_NATIVE_HELPERS_HPP
#include "post_course_scene_internal.hpp"
#include <cmath>
#include <limits>
#include <utility>
namespace isle::post_course {
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
#endif

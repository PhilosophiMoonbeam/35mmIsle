#include "course_actor_native_services.hpp"
#include "actor_native_context.hpp"
#include "actor_owner.hpp"
#include "actor_owner_source.h"
#include "actor_interaction_services.hpp"
#include "actor_spatial.h"
#include "course_native_host.hpp"
#include "menu_scene_bridge.h"
#include "player_root.hpp"
#include "player_root_source.h"
#include "player_cart.hpp"
#include "player_cart_resources.hpp"
#include "player_camera_animation.hpp"
#include "player_ui_source.h"
#include "item_owner.hpp"
#include "scene_services.hpp"
#include <cmath>
#include <limits>

namespace isle::course {
namespace {
using Source = IsleCaveActorSource;
using Host = NativeHost;
using Handle = menu::animation::InstanceHandle;

template<class F> IsleGameResult invoke(Source* source, F&& function) noexcept {
    Host* host = nullptr;
    const auto result = actor::native_host(source, &host);
    if (result != ISLE_GAME_OK) return result;
    try { return function(*host); }
    catch (...) { return ISLE_GAME_INVALID_STATE; }
}
IsleGameResult status(actor::OwnerStatus result) noexcept {
    return result ? ISLE_GAME_OK : result.error == actor::OwnerError::admission ?
        ISLE_GAME_INVALID_ARGUMENT : ISLE_GAME_EXECUTION_FAILURE;
}
IsleGameResult audio_status(audio::Error result) noexcept {
    return result == audio::Error::none ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;
}
world::Block* block(IsleCaveWorldBlock* value) noexcept {
    return reinterpret_cast<world::Block*>(value);
}
IsleCaveWorldBlock* block(world::Block* value) noexcept {
    return reinterpret_cast<IsleCaveWorldBlock*>(value);
}
IsleCaveCamera* camera(IsleGameCamera* value) noexcept {
    return reinterpret_cast<IsleCaveCamera*>(value);
}
bool camera_matches(Host& host, IsleCaveCamera* value) noexcept {
    return value && value == camera(host.root()->source().camera) && host.camera();
}
Handle actor_handle(Host& host, IsleGameObject* object) noexcept {
    for (const auto& binding : host.actors()->render_bindings())
        if (binding.model.object == object) return binding.model.state.instance;
    return {};
}
IsleGameResult actor_record(Host& host, IsleGameObject* object, IsleActorSource** output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = nullptr;
    const auto handle = actor_handle(host, object);
    auto* source = host.actors()->source(handle);
    if (!source || source->game != host.game() || source->object != object)
        return ISLE_GAME_INVALID_ARGUMENT;
    IsleActorSource* canonical = nullptr;
    const auto result = isle_actor_source_get(host.game(), object, &canonical);
    if (result != ISLE_GAME_OK) return result;
    if (canonical != source) return ISLE_GAME_INVALID_STATE;
    *output = source;
    return ISLE_GAME_OK;
}
const menu::AnimationPath* path(Host& host, const void* value) noexcept {
    if (!value || host.world_owner()->retired() || !host.world_owner()->error().empty()) return nullptr;
    for (const auto& candidate : host.world_owner()->resources().paths)
        if (&candidate == value) return &candidate;
    return nullptr;
}
IsleGameResult model_native(Host& host, IsleGameObject* object, IsleGameModelRef* output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = {};
    if (!object) return ISLE_GAME_INVALID_ARGUMENT;
    IsleGameModel* model = nullptr;
    if (object == host.root()->source().object) model = host.root()->source().model;
    if (!model) for (const auto& binding : host.actors()->render_bindings())
        if (binding.model.object == object) { model = binding.model.model; break; }
    if (!model && host.items_owner()) model = host.items_owner()->native_model(host.items_owner()->instance(object));
    if (!model && host.cart()) model = host.cart()->native_model(host.cart()->instance(object));
    if (!model) return ISLE_GAME_INVALID_ARGUMENT;
    return isle_game_model_reference(host.game(), object, model, output);
}
IsleGameResult vector_native(Host& host, IsleCaveVectorRef reference, const IsleGameVec3** output) noexcept {
    IsleGameModel* model = nullptr;
    auto result = isle_game_model_resolve(host.game(), &reference.model, &model);
    if (result != ISLE_GAME_OK) return result;
    if (reference.component == 0) {
        const IsleGameModelPlacement* placement = nullptr;
        result = isle_game_model_placement(host.game(), model, &placement);
        if (result != ISLE_GAME_OK) return result;
        if (!placement || placement->mode == ISLE_GAME_MODEL_PLACEMENT_DISABLED) return ISLE_GAME_INVALID_STATE;
        *output = &placement->transform.position;
        return ISLE_GAME_OK;
    }
    if (reference.component != 9) return ISLE_GAME_INVALID_ARGUMENT;
    const IsleGameModelNode* nodes = nullptr;
    std::size_t count = 0;
    result = isle_game_model_peek_nodes(host.game(), model, &nodes, &count);
    if (result != ISLE_GAME_OK) return result;
    if (!count || nodes[0].parent != UINT32_MAX) return ISLE_GAME_INVALID_STATE;
    *output = &nodes[0].transform.position;
    return ISLE_GAME_OK;
}
template<class Transform> auto& component(Transform& value, std::uint32_t index) noexcept {
    if (index == 0) return value.position.x;
    if (index == 1) return value.position.y;
    if (index == 2) return value.position.z;
    if (index == 3) return value.rotation.x;
    if (index == 4) return value.rotation.y;
    if (index == 5) return value.rotation.z;
    if (index == 6) return value.scale.x;
    if (index == 7) return value.scale.y;
    return value.scale.z;
}
IsleGameResult model_read_native(Host& host, IsleGameModelRef reference, std::uint32_t index, float* output) noexcept {
    if (!output || index > 11) return ISLE_GAME_INVALID_ARGUMENT;
    IsleGameModel* model = nullptr;
    auto result = isle_game_model_resolve(host.game(), &reference, &model);
    if (result != ISLE_GAME_OK) return result;
    if (index < 9) {
        const IsleGameModelPlacement* placement = nullptr;
        result = isle_game_model_placement(host.game(), model, &placement);
        if (result != ISLE_GAME_OK) return result;
        if (!placement || placement->mode == ISLE_GAME_MODEL_PLACEMENT_DISABLED ||
            (index >= 6 && placement->mode != ISLE_GAME_MODEL_PLACEMENT_RTS)) return ISLE_GAME_INVALID_STATE;
        const auto& transform = placement->transform;
        *output = component(transform, index);
    } else {
        const IsleGameModelNode* nodes = nullptr;
        std::size_t count = 0;
        result = isle_game_model_peek_nodes(host.game(), model, &nodes, &count);
        if (result != ISLE_GAME_OK) return result;
        if (!count || nodes[0].parent != UINT32_MAX) return ISLE_GAME_INVALID_STATE;
        const auto& position = nodes[0].transform.position;
        *output = index == 9 ? position.x : index == 10 ? position.y : position.z;
    }
    return ISLE_GAME_OK;
}
IsleGameResult model_store_native(Host& host, IsleGameModelRef reference, std::uint32_t index, float value) noexcept {
    if (index > 11) return ISLE_GAME_INVALID_ARGUMENT;
    IsleActorSource* source = nullptr;
    auto result = actor_record(host, reference.object, &source);
    if (result != ISLE_GAME_OK) return result;
    if (index < 9) {
        IsleGameModelTransform transform{};
        component(transform, index) = value;
        return isle_actor_owner_aux_store(source, &reference, 1u << index, &transform);
    }
    IsleGameModel* model = nullptr;
    result = isle_game_model_resolve(host.game(), &reference, &model);
    if (result != ISLE_GAME_OK) return result;
    const IsleGameModelNode* nodes = nullptr;
    std::size_t count = 0;
    result = isle_game_model_peek_nodes(host.game(), model, &nodes, &count);
    if (result != ISLE_GAME_OK) return result;
    if (!count) return ISLE_GAME_INVALID_STATE;
    auto position = nodes[0].transform.position;
    if (index == 9) position.x = value;
    else if (index == 10) position.y = value;
    else position.z = value;
    return isle_actor_owner_root_position_store(source, &reference, &position);
}
IsleGameResult sound_native(Host& host, std::int32_t id, std::int32_t volume,
    std::int32_t pan, float pitch, std::int32_t reverb, std::int32_t* output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    const game_audio::Parameters parameters{std::uint16_t(volume), std::uint8_t(pan), std::uint8_t(reverb), pitch};
    *output = host.services()->play_sound(std::uint32_t(id), parameters);
    return host.services()->failed() ? ISLE_GAME_EXECUTION_FAILURE : ISLE_GAME_OK;
}
void camera_tick(IsleGameObject* object, void* user) noexcept {
    auto* source = static_cast<Source*>(user);
    Host* active = nullptr;
    const auto result = invoke(source, [&](Host& host) {
        active = &host;
        IsleGameObject* actual = nullptr;
        auto error = isle_game_camera_object(host.game(), host.root()->source().camera, &actual);
        if (error != ISLE_GAME_OK) return error;
        if (object != actual || !host.camera()) return ISLE_GAME_INVALID_STATE;
        IsleGameObjectMetadata metadata{};
        error = isle_game_object_metadata(host.game(), object, &metadata);
        if (error != ISLE_GAME_OK) return error;
        const auto stepped = host.camera()->update(metadata.flags);
        return stepped ? ISLE_GAME_OK : stepped.native != ISLE_GAME_OK ? stepped.native : ISLE_GAME_EXECUTION_FAILURE;
    });
    if (result != ISLE_GAME_OK) {
        if (active) active->fail(result, "Cave camera animation update failed");
        if (source && source->base.status == ISLE_GAME_OK) source->base.status = result;
    }
}
void cart_event(void* user, const IslePlayerCartNode* node, std::int32_t event, float argument) noexcept {
    auto* source = static_cast<Source*>(user);
    Host* active = nullptr;
    const auto result = invoke(source, [&](Host& host) {
        active = &host;
        if (!node || !host.cart() || !host.cart()->instance(node->binding.object)) return ISLE_GAME_INVALID_STATE;
        IsleGameModel* model = nullptr;
        auto error = isle_game_model_resolve(host.game(), &node->binding, &model);
        if (error != ISLE_GAME_OK) return error;
        if (model != host.cart()->native_model(host.cart()->instance(node->binding.object))) return ISLE_GAME_INVALID_STATE;
        // func802c5778 ignores DObj. Keep its exact ModelRef ABI, distinct from
        // lifecycle func802c5bb0's retained event-node/effect attachment ABI.
        return isle_cave_actor_invoke_animation(source, &node->binding, event, argument);
    });
    if (result != ISLE_GAME_OK) {
        if (active) active->fail(result, "Cave Cart animation callback failed");
        if (source && source->base.status == ISLE_GAME_OK) source->base.status = result;
    }
}
}

IsleCaveActorServices make_cave_actor_services() noexcept {
    return {
        .model = +[](Source* s, IsleGameObject* object, IsleGameModelRef* out) noexcept {
            return invoke(s, [&](Host& h) { return model_native(h, object, out); });
        },
        .model_read = +[](Source* s, IsleGameModelRef model, std::uint32_t index, float* out) noexcept {
            return invoke(s, [&](Host& h) { return model_read_native(h, model, index, out); });
        },
        .model_store = +[](Source* s, IsleGameModelRef model, std::uint32_t index, float value) noexcept {
            return invoke(s, [&](Host& h) { return model_store_native(h, model, index, value); });
        },
        .model_child_flags = +[](Source* s, IsleGameObject* object, std::uint32_t mask, std::uint32_t value) noexcept {
            return invoke(s, [&](Host& h) {
                IsleActorSource* actor = nullptr;
                auto result = actor_record(h, object, &actor);
                if (result != ISLE_GAME_OK) return result;
                const auto handle = actor_handle(h, object);
                const auto state = h.actors()->animation().model(handle);
                if (!state || state.nodes.empty()) return ISLE_GAME_INVALID_STATE;
                std::size_t child = 1;
                while (child < state.nodes.size() && state.nodes[child].parent != 0) ++child;
                if (child == state.nodes.size()) return ISLE_GAME_INVALID_STATE;
                const auto flags = (state.nodes[child].flags & ~mask) | (value & mask);
                if (!h.actors()->animation().set_node_flags(handle, child, flags)) return ISLE_GAME_EXECUTION_FAILURE;
                return isle_game_model_set_flags(h.game(), h.actors()->native_model(handle), child, flags);
            });
        },
        .model_speed = +[](Source* s, IsleGameModelRef model, float* out) noexcept {
            return invoke(s, [&](Host& h) {
                if (!out) return ISLE_GAME_INVALID_ARGUMENT;
                IsleGameModel* resolved = nullptr;
                auto result = isle_game_model_resolve(h.game(), &model, &resolved);
                if (result != ISLE_GAME_OK) return result;
                const auto handle = actor_handle(h, model.object);
                if (h.actors()->native_model(handle) != resolved) return ISLE_GAME_INVALID_ARGUMENT;
                const auto state = h.actors()->animation().model(handle);
                if (!state || state.nodes.empty()) return ISLE_GAME_INVALID_STATE;
                *out = state.nodes[0].clock.speed;
                return ISLE_GAME_OK;
            });
        },
        .player_model_ref = +[](Source* s, IsleGameModelRef* out) noexcept {
            return invoke(s, [&](Host& h) { return model_native(h, h.root()->source().object, out); });
        },
        .player_object = +[](Source* s, IsleGameObject** out) noexcept {
            return invoke(s, [&](Host& h) {
                if (!out) return ISLE_GAME_INVALID_ARGUMENT;
                *out = h.root()->source().object;
                return *out ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
            });
        },
        .reference = +[](Source* s, IsleGameObject** field, IsleGameObject* object) noexcept {
            return invoke(s, [&](Host& h) { return h.reference(field, object); });
        },
        .current_block = +[](Source* s, IsleCaveWorldBlock** out) noexcept {
            return invoke(s, [&](Host& h) {
                if (!out) return ISLE_GAME_INVALID_ARGUMENT;
                *out = block(h.world_owner()->current());
                return ISLE_GAME_OK;
            });
        },
        .block_position = +[](Source* s, IsleCaveWorldBlock* value, IsleGameVec3* out) noexcept {
            return invoke(s, [&](Host& h) {
                auto* native = block(value);
                if (!out || !h.valid_block(native) || !native->resource) return ISLE_GAME_INVALID_ARGUMENT;
                const auto p = native->resource->metadata.world_position;
                *out = {p.x,p.y,p.z};
                return ISLE_GAME_OK;
            });
        },
        .add_one = +[](Source* s, IsleCaveWorldBlock* a, IsleCaveWorldBlock* b, const IsleCaveAutomaticSpawn* spawn, const IsleCaveDefinition* definition, IsleGameObject** out) noexcept {
            if (out) *out = nullptr;
            return invoke(s, [&](Host& h) {
                if (!spawn || !definition || !out || !h.valid_block(block(a)) || !h.valid_block(block(b))) return ISLE_GAME_INVALID_ARGUMENT;
                const auto result = h.actors()->add_one_cave(*definition, *spawn, block(a), block(b));
                *out = result.object;
                return status(result.status);
            });
        },
        .add_geo = +[](Source* s, IsleGameObject* object, std::uint16_t id, const IsleCaveDefinition* definition, IsleGameObject** out) noexcept {
            if (out) *out = nullptr;
            return invoke(s, [&](Host& h) {
                if (!definition || !out) return ISLE_GAME_INVALID_ARGUMENT;
                const auto result = h.actors()->add_geo_cave(object, id, *definition);
                *out = result.object;
                return status(result.status);
            });
        },
        .sound = +[](Source* s, std::int32_t id, std::int32_t* out) noexcept {
            return invoke(s, [&](Host& h) { return sound_native(h,id,0x7fff,64,1.0f,0,out); });
        },
        .sound_params = +[](Source* s, std::int32_t id, std::int32_t volume, std::int32_t pan, float pitch, std::int32_t reverb, std::int32_t* out) noexcept {
            return invoke(s, [&](Host& h) { return sound_native(h,id,volume,pan,pitch,reverb,out); });
        },
        .sound_active = +[](Source* s, std::int32_t handle, std::int32_t* out) noexcept {
            return invoke(s, [&](Host& h) {
                if (!out) return ISLE_GAME_INVALID_ARGUMENT;
                *out = h.audio().playing_sound(std::uint32_t(handle));
                return ISLE_GAME_OK;
            });
        },
        .sound_stop = +[](Source* s, std::int32_t handle) noexcept {
            return invoke(s, [&](Host& h) { return audio_status(h.audio().stop_sound(std::uint32_t(handle))); });
        },
        .sound_volume = +[](Source* s, std::int32_t handle, std::int32_t volume) noexcept {
            return invoke(s, [&](Host& h) { return audio_status(h.audio().sound_volume(std::uint32_t(handle),std::uint32_t(volume))); });
        },
        .sound_stop_all = +[](Source* s) noexcept {
            return invoke(s, [&](Host& h) { return audio_status(h.audio().stop_sounds()); });
        },
        .bgm_play = +[](Source* s, std::int32_t player, std::int32_t id) noexcept {
            return invoke(s, [&](Host& h) { return audio_status(h.audio().play_song(std::uint32_t(player),std::uint32_t(id))); });
        },
        .bgm_stop = +[](Source* s, std::int32_t player) noexcept {
            return invoke(s, [&](Host& h) { return audio_status(h.audio().stop_song(std::uint32_t(player))); });
        },
        .bgm_volume = +[](Source* s, std::int32_t player, std::uint32_t volume) noexcept {
            return invoke(s, [&](Host& h) { return audio_status(h.audio().bgm_volume(std::uint32_t(player),volume)); });
        },
        .bgm_channel_volume = +[](Source* s, std::int32_t player, std::uint8_t channel, std::int32_t volume) noexcept {
            return invoke(s, [&](Host& h) { return audio_status(h.audio().bgm_channel_volume(std::uint32_t(player),channel,std::int8_t(volume))); });
        },
        .bgm_reverb = +[](Source* s, std::int32_t player, std::int32_t reverb) noexcept {
            return invoke(s, [&](Host& h) { return audio_status(h.audio().bgm_reverb(std::uint32_t(player),std::uint32_t(reverb))); });
        },
        .global_reverb = +[](Source* s, std::int32_t reverb) noexcept {
            return invoke(s, [&](Host& h) { return audio_status(h.audio().global_sound_reverb(std::uint8_t(reverb))); });
        },
        .background_music = +[](Source* s, std::int32_t id) noexcept {
            return invoke(s, [&](Host& h) { return isle_player_root_invoke_set_background_music(&h.root()->source(),id); });
        },
        .texture_speed = +[](Source* s, IsleGameObject* object, float speed) noexcept {
            return invoke(s, [&](Host& h) {
                IsleActorSource* actor = nullptr;
                auto result = actor_record(h,object,&actor);
                if (result != ISLE_GAME_OK) return result;
                return h.actors()->animation().set_texture_speed(actor_handle(h,object),speed) ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;
            });
        },
        .animation_speed = +[](Source* s, IsleGameObject* object, float speed) noexcept {
            return invoke(s, [&](Host& h) {
                IsleActorSource* actor = nullptr;
                auto result = actor_record(h,object,&actor);
                if (result != ISLE_GAME_OK) return result;
                return status(h.actors()->set_animation_speed(actor_handle(h,object),speed));
            });
        },
        .header_speed = +[](Source* s, std::uint32_t identity, float* out) noexcept {
            return invoke(s, [&](Host& h) {
                if (!out) return ISLE_GAME_INVALID_ARGUMENT;
                const auto& resources = h.actor_resources();
                const auto& initialization = resources.initialization(0x802c7cccu);
                if (identity != 0x802c7be4u ||
                    h.actors()->admitted_factory(initialization.actor_id,0x802c4e64u) != &initialization)
                    return ISLE_GAME_INVALID_ARGUMENT;
                const auto initializations = resources.initializations();
                for (const auto& header : resources.headers())
                    if (header.wire.guest_address == identity && header.initialization < initializations.size() &&
                        &initializations[header.initialization] == &initialization) {
                        *out = header.speed;
                        return ISLE_GAME_OK;
                    }
                return ISLE_GAME_INVALID_STATE;
            });
        },
        .level_progress = +[](Source* s, std::int32_t* out, float* fraction) noexcept {
            return invoke(s, [&](Host& h) {
                if (!out || !fraction) return ISLE_GAME_INVALID_ARGUMENT;
                const auto& movement = h.root()->source().movement;
                for (const auto& current : h.world_owner()->blocks()) if (&current == movement.block) {
                    if (current.index > std::size_t(INT32_MAX)) return ISLE_GAME_INVALID_STATE;
                    *out = std::int32_t(current.index); *fraction = movement.move_time;
                    return ISLE_GAME_OK;
                }
                return ISLE_GAME_INVALID_STATE;
            });
        },
        .ground = +[](Source* s, float x, float z, IsleActorGround* out) noexcept {
            return invoke(s, [&](Host& h) {
                if (!out) return ISLE_GAME_INVALID_ARGUMENT;
                world::GroundResult ground;
                h.world_owner()->ground_at(x,z,ground);
                *out = {ground.height,{ground.normal.x,ground.normal.y,ground.normal.z},std::uint32_t(ground.surface)};
                return ISLE_GAME_OK;
            });
        },
        .remove_flying_items = +[](Source* s) noexcept {
            return invoke(s, [&](Host& h) { return h.items_owner()->remove_flying() ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE; });
        },
        .camera_reset = +[](Source* s) noexcept {
            return invoke(s, [&](Host& h) { return isle_player_root_invoke_reset_main_camera_settings(&h.root()->source()); });
        },
        .messages_reset = +[](Source* s) noexcept {
            return invoke(s, [&](Host& h) { return isle_player_ui_Msg_Reset(h.root()->source().ui); });
        },
        .cutscene_start = +[](Source* s, IsleGameObject* object, std::uint32_t animation, float duration) noexcept {
            return invoke(s, [&](Host& h) {
                if (animation != 0) return ISLE_GAME_INVALID_ARGUMENT;
                return isle_player_root_invoke_start_cutscene(&h.root()->source(),object,nullptr,duration);
            });
        },
        .process_pause = +[](Source* s, IsleGameObject* object) noexcept {
            return invoke(s, [&](Host& h) { return isle_game_pause_object_processes(h.game(),object,1); });
        },
        .camera_speed = +[](Source* s, IsleCaveCamera* value, float speed) noexcept {
            return invoke(s, [&](Host& h) {
                if (!camera_matches(h,value)) return ISLE_GAME_INVALID_ARGUMENT;
                return h.camera()->set_speed(speed) ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;
            });
        },
        .camera_animation = +[](Source* s, IsleCaveCamera* value, std::uint32_t identity, float start) noexcept {
            return invoke(s, [&](Host& h) {
                if (!camera_matches(h,value)) return ISLE_GAME_INVALID_ARGUMENT;
                if (!identity) return h.camera()->set(nullptr,start) ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;
                const auto index = h.cart_resources().camera_animation(identity);
                if (index == menu::no_index) return ISLE_GAME_INVALID_ARGUMENT;
                IslePlayerCameraAnimationClip clip{};
                if (!h.camera()->clip(index,clip)) return ISLE_GAME_INVALID_STATE;
                return h.camera()->set(&clip,start) ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;
            });
        },
        .player_animation = +[](Source* s, std::uint32_t models, std::uint32_t materials, float start, float speed) noexcept {
            return invoke(s, [&](Host& h) {
                const auto* model = models ? h.cart()->model_animation(models) : nullptr;
                const auto* material = materials ? h.cart()->material_animation(materials) : nullptr;
                if ((models && !model) || (materials && !material)) return ISLE_GAME_INVALID_ARGUMENT;
                return h.cart()->set_animation(model,material,start,speed) ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;
            });
        },
        .animation_callback = +[](Source* s, IsleGameObject* object, IsleCaveAnimationCallback callback) noexcept {
            return invoke(s, [&](Host& h) {
                if (h.cart()->instance(object)) {
                    if (callback != func_802C5778_647C28) return ISLE_GAME_INVALID_ARGUMENT;
                    return h.cart()->set_callback(object,cart_event,s) ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;
                }
                return status(h.actors()->bind_animation_callback(*s,object,callback));
            });
        },
        .remove_pokemon = +[](Source* s, const IsleCaveRemovalList* list) noexcept {
            return invoke(s, [&](Host& h) {
                if (!list) return ISLE_GAME_INVALID_ARGUMENT;
                auto* current = isle_menu_current_object(h.game());
                return isle_actor_source_remove_pokemons_call(h.game(),current,list->values,2);
            });
        },
        .environment_sound = +[](Source* s, IsleGameObject* object, std::int32_t category, std::int32_t sound) noexcept {
            return invoke(s, [&](Host& h) {
                if (category < 0 || category > UINT8_MAX) return ISLE_GAME_INVALID_ARGUMENT;
                IsleActorSource* actor = nullptr;
                auto result = actor_record(h,object,&actor);
                return result == ISLE_GAME_OK ? isle_actor_owner_environment_sound(actor,std::uint8_t(category),sound) : result;
            });
        },
        .path_save = +[](Source* s, IsleGameObject* object, const void* value) noexcept {
            return invoke(s, [&](Host& h) {
                IsleActorSource* actor = nullptr;
                const auto result = actor_record(h,object,&actor);
                if (result != ISLE_GAME_OK) return result;
                if (value != actor->path || (value && !path(h,value))) return ISLE_GAME_INVALID_ARGUMENT;
                return isle_actor_owner_saved_path_save(actor,2);
            });
        },
        .main_camera = +[](Source* s, IsleCaveCamera** out) noexcept {
            return invoke(s, [&](Host& h) {
                if (!out) return ISLE_GAME_INVALID_ARGUMENT;
                *out = camera(h.root()->source().camera);
                return *out && h.camera() ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
            });
        },
        .player_model_init = +[](Source* s, IsleGameObject** out) noexcept {
            if (out) *out = nullptr;
            return invoke(s, [&](Host& h) {
                if (!out) return ISLE_GAME_INVALID_ARGUMENT;
                const auto result = h.cart()->init();
                *out = result.object;
                if (result || result.status.error == player::cart::OwnerError::capacity) return ISLE_GAME_OK;
                return ISLE_GAME_EXECUTION_FAILURE;
            });
        },
        .flute = +[](Source* s, std::int32_t* out) noexcept {
            return invoke(s, [&](Host& h) {
                if (!out) return ISLE_GAME_INVALID_ARGUMENT;
                *out = h.items_owner()->flute().command();
                return ISLE_GAME_OK;
            });
        },
        .path_restore = +[](Source* s, IsleGameObject* object, const void** out) noexcept {
            if (out) *out = nullptr;
            return invoke(s, [&](Host& h) {
                if (!out) return ISLE_GAME_INVALID_ARGUMENT;
                IsleActorSource* actor = nullptr;
                auto result = actor_record(h,object,&actor);
                if (result != ISLE_GAME_OK) return result;
                result = isle_actor_owner_saved_path_restore(actor,2);
                if (result == ISLE_GAME_OK) *out = actor->path;
                return result;
            });
        },
        .path_point = +[](Source* s, const void* value, std::size_t index, float* out) noexcept {
            return invoke(s, [&](Host& h) {
                const auto* actual = path(h,value);
                if (!out || !actual || index >= actual->timestamps.size()) return ISLE_GAME_INVALID_ARGUMENT;
                *out = actual->timestamps[index];
                return ISLE_GAME_OK;
            });
        },
        .interpolated_position = +[](Source* s, IsleGameVec3* out, const void* value, float t) noexcept {
            return invoke(s, [&](Host& h) {
                const auto* actual = path(h,value);
                if (!out || !actual) return ISLE_GAME_INVALID_ARGUMENT;
                const auto p = menu::animation::path_position(*actual,t);
                *out = {p.x,p.y,p.z};
                return ISLE_GAME_OK;
            });
        },
        .camera_process = +[](Source* s, IsleGameObject* object, std::int32_t kind, std::int32_t priority, IsleGameProcess** out) noexcept {
            if (out) *out = nullptr;
            return invoke(s, [&](Host& h) {
                if (!out || kind != 1 || priority < 0 || !h.camera()) return ISLE_GAME_INVALID_ARGUMENT;
                IsleGameObject* actual = nullptr;
                const auto result = isle_game_camera_object(h.game(),h.root()->source().camera,&actual);
                if (result != ISLE_GAME_OK) return result;
                if (object != actual) return ISLE_GAME_INVALID_ARGUMENT;
                return isle_game_add_process(h.game(),object,camera_tick,s,ISLE_GAME_SYNCHRONOUS,std::uint32_t(priority),out);
            });
        },
        .sky_speed = +[](Source* s, float speed) noexcept {
            return invoke(s, [&](Host& h) { h.world_owner()->set_sky_animation_speed(speed); return ISLE_GAME_OK; });
        },
        .item_collision_velocity = +[](Source* s, IsleGameObject* object, IsleGameVec3* out) noexcept {
            return invoke(s, [&](Host& h) {
                if (!out || !object) return ISLE_GAME_INVALID_ARGUMENT;
                IsleItemSource* item = nullptr;
                const auto result = isle_item_source_get(h.game(),object,&item);
                if (result != ISLE_GAME_OK) return result;
                if (!item || h.items_owner()->source(object) != item) return ISLE_GAME_INVALID_STATE;
                *out = item->collisionVelocity;
                return ISLE_GAME_OK;
            });
        },
        .camera_object = +[](Source* s, IsleCaveCamera* value, IsleGameObject** out) noexcept {
            if (out) *out = nullptr;
            return invoke(s, [&](Host& h) {
                if (!out || !camera_matches(h,value)) return ISLE_GAME_INVALID_ARGUMENT;
                return isle_game_camera_object(h.game(),h.root()->source().camera,out);
            });
        },
        .rand = +[](Source* s, std::int32_t range, std::int32_t* out) noexcept {
            return invoke(s, [&](Host& h) {
                if (!out) return ISLE_GAME_INVALID_ARGUMENT;
                *out = isle_rng_range(&h.services()->rng,range);
                return ISLE_GAME_OK;
            });
        },
        .distance = +[](Source* s, IsleGameObject* a, IsleGameObject* b, float* out) noexcept {
            return invoke(s, [&](Host& h) { return isle_actor_spatial_distance(h.game(),a,b,out); });
        },
        .distance_vectors = +[](Source* s, IsleCaveVectorRef a, IsleCaveVectorRef b, float* out) noexcept {
            return invoke(s, [&](Host& h) {
                if (!out || (a.component != 0 && a.component != 9) || (b.component != 0 && b.component != 9)) return ISLE_GAME_INVALID_ARGUMENT;
                const IsleGameVec3* first = nullptr;
                const IsleGameVec3* second = nullptr;
                auto result = vector_native(h,a,&first);
                if (result == ISLE_GAME_OK) result = vector_native(h,b,&second);
                if (result != ISLE_GAME_OK) return result;
                const float x=first->x-second->x,y=first->y-second->y,z=first->z-second->z;
                *out=std::sqrt(x*x+y*y+z*z);
                return ISLE_GAME_OK;
            });
        },
        .weighted = +[](Source* s, IsleGameObject* object, std::uint32_t identity, const IsleActorRandomState** out) noexcept {
            if (out) *out = nullptr;
            return invoke(s, [&](Host& h) {
                if (!out) return ISLE_GAME_INVALID_ARGUMENT;
                IsleActorSource* actor = nullptr;
                const auto result = actor_record(h,object,&actor);
                if (result != ISLE_GAME_OK) return result;
                const auto* interactions = h.actors()->bound_interactions();
                if (!interactions || !interactions->admits_source()) return ISLE_GAME_INVALID_STATE;
                *out = interactions->native_weighted(identity).rows.data();
                return ISLE_GAME_OK;
            });
        },
        .path_assign = +[](Source* s, IsleGameObject* object, const void* value) noexcept {
            return invoke(s, [&](Host& h) {
                IsleActorSource* actor = nullptr;
                const auto result = actor_record(h,object,&actor);
                if (result != ISLE_GAME_OK) return result;
                if (value && !path(h,value)) return ISLE_GAME_INVALID_ARGUMENT;
                actor->path = value;
                return ISLE_GAME_OK;
            });
        },
    };
}
} // namespace isle::course

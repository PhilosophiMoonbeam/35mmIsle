#include "course_actor_native_services.hpp"
#include "volcano_actor_native.hpp"
#include "actor_native_context.hpp"
#include "actor_owner.hpp"
#include "actor_spatial.h"
#include "course_native_host.hpp"
#include "course_scene_source.h"
#include "item_owner.hpp"
#include "player_root.hpp"
#include "player_cart.hpp"
#include "player_cart_resources.hpp"
#include "player_camera_animation.hpp"
#include "player_ui_source.h"
#include "beach_meowth_source.h"
#include "game_audio.hpp"
#include <cmath>
#include <cstdint>

namespace isle::course {
namespace {
using Source = IsleVolcanoActorSource;
using Handle = menu::animation::InstanceHandle;
IsleGameResult boolean(bool value) noexcept { return value ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE; }
IsleGameResult audio_result(audio::Error value) noexcept { return value == audio::Error::none ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE; }
template<class Function> IsleGameResult with(Source* source, Function&& function) noexcept {
    NativeHost* host = nullptr;
    const auto result = actor::native_host(source,&host);
    if (result != ISLE_GAME_OK) return result;
    if (!host || !host->root() || !host->actors() || !host->services()) return ISLE_GAME_INVALID_STATE;
    try { return function(*host); }
    catch (...) { return ISLE_GAME_EXECUTION_FAILURE; }
}
Handle actor_handle(NativeHost& host,IsleGameObject* object) noexcept {
    for (const auto& binding : host.actors()->render_bindings())
        if (binding.model.object == object && object &&
            host.actors()->object(binding.model.state.instance) == object)
            return binding.model.state.instance;
    return {};
}
IsleGameResult actor_source(NativeHost& host,IsleGameObject* object,IsleActorSource** output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = nullptr;
    const auto handle = actor_handle(host,object);
    if (!handle) return ISLE_GAME_INVALID_ARGUMENT;
    auto result = isle_actor_source_get(host.game(),object,output);
    if (result == ISLE_GAME_OK && (!*output || host.actors()->source(handle) != *output)) {
        *output = nullptr; return ISLE_GAME_INVALID_STATE;
    }
    return result;
}
IsleGameResult receiver(NativeHost& host,IsleGameObject* object) noexcept {
    if (!object) return ISLE_GAME_INVALID_ARGUMENT;
    IsleGameObjectMetadata metadata{};
    return isle_game_object_metadata(host.game(),object,&metadata);
}
IsleGameResult model(NativeHost& host,IsleGameObject* object,IsleGameModelRef* output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = {};
    IsleGameModel* native = nullptr;
    auto result = isle_actor_spatial_object_model(host.game(),object,&native);
    if (result != ISLE_GAME_OK) return result;
    if (!native) return ISLE_GAME_INVALID_ARGUMENT;
    return isle_game_model_reference(host.game(),object,native,output);
}
IsleGameResult auxiliary(NativeHost& host,const IsleGameModelRef* reference,const IsleGameModelTransform** output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = nullptr;
    IsleGameModel* native = nullptr;
    auto result = isle_game_model_resolve(host.game(),reference,&native);
    if (result != ISLE_GAME_OK) return result;
    const IsleGameModelPlacement* placement = nullptr;
    result = isle_game_model_placement(host.game(),native,&placement);
    if (result != ISLE_GAME_OK) return result;
    if (!placement || (placement->mode != ISLE_GAME_MODEL_PLACEMENT_RT && placement->mode != ISLE_GAME_MODEL_PLACEMENT_RTS)) return ISLE_GAME_INVALID_STATE;
    *output = &placement->transform;
    return ISLE_GAME_OK;
}
IsleGameResult component(NativeHost& host,const IsleGameModelRef* reference,std::uint32_t mask,float value) noexcept {
    if (!mask || (mask & ~std::uint32_t(0x1ff))) return ISLE_GAME_INVALID_ARGUMENT;
    IsleGameModel* native = nullptr;
    auto result = isle_game_model_resolve(host.game(),reference,&native);
    if (result != ISLE_GAME_OK) return result;
    const IsleGameModelPlacement* placement = nullptr;
    result = isle_game_model_placement(host.game(),native,&placement);
    if (result != ISLE_GAME_OK) return result;
    if (!placement || (placement->mode != ISLE_GAME_MODEL_PLACEMENT_RT && placement->mode != ISLE_GAME_MODEL_PLACEMENT_RTS) ||
        ((mask & 0x1c0u) && placement->mode != ISLE_GAME_MODEL_PLACEMENT_RTS)) return ISLE_GAME_INVALID_STATE;
    auto transform = placement->transform;
    if (mask & 1u) transform.position.x = value;
    if (mask & 2u) transform.position.y = value;
    if (mask & 4u) transform.position.z = value;
    if (mask & 8u) transform.rotation.x = value;
    if (mask & 16u) transform.rotation.y = value;
    if (mask & 32u) transform.rotation.z = value;
    if (mask & 64u) transform.scale.x = value;
    if (mask & 128u) transform.scale.y = value;
    if (mask & 256u) transform.scale.z = value;
    return isle_game_model_set_placement(host.game(),native,placement->mode,&transform);
}
IsleGameResult camera_clip(NativeHost& host,std::uint32_t identity,IslePlayerCameraAnimationClip& output) {
    if (!host.camera()) return ISLE_GAME_INVALID_STATE;
    const auto selected = host.cart_resources().camera_animation(identity);
    if (!identity || selected == menu::no_index) return ISLE_GAME_INVALID_ARGUMENT;
    return boolean(bool(host.camera()->clip(selected,output)));
}
IsleGameResult sound(NativeHost& host,std::int32_t id,std::int32_t volume,std::int32_t pan,float pitch,std::int32_t reverb,std::int32_t* output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = -1;
    int handle = -1;
    const auto result = host.audio().play_sound(std::uint32_t(id),{std::uint16_t(volume),std::uint8_t(pan),std::uint8_t(reverb),pitch},handle);
    *output = handle;
    return audio_result(result);
}
IsleGameResult packet(NativeHost& host,std::uint32_t lane,std::uint32_t first,std::uint32_t second) noexcept {
    auto* producer = host.actor_packet_producer();
    return producer ? producer->source_packet(lane,first,second) : ISLE_GAME_INVALID_STATE;
}
}
IsleGameResult volcano_actor_source_render(void* user,IsleGameObject* object,std::uint32_t identity) noexcept {
    auto* host = static_cast<NativeHost*>(user);
    if (!host || !host->actors() || host->actors()->course_context().course_id() != 2) return ISLE_GAME_INVALID_STATE;
    auto& state = host->actors()->course_context().volcano();
    auto& source = state.actors;
    // Outside OM there is intentionally no current-object/fence yet. The actual
    // typed C invoke performs complete admission and establishes that fence;
    // native_host() inside each returned leaf then validates the same association.
    if (state.native_host != host || source.base.nativeGame != host->game() ||
        !actor_handle(*host,object)) return ISLE_GAME_INVALID_STATE;
    return isle_volcano_actor_invoke_render(&source,object,identity);
}
IsleVolcanoActorServices make_volcano_actor_services() noexcept {
    return {
        .misc_object_get = +[](Source* s,IsleGameObject* object,std::uint32_t index,IsleGameObject** output) noexcept {
            if (!output) return ISLE_GAME_INVALID_ARGUMENT; *output = nullptr;
            return with(s,[&](NativeHost& h) { IsleActorSource* a = nullptr; auto r = actor_source(h,object,&a); return r == ISLE_GAME_OK ? isle_actor_owner_misc_object_get(a,index,output) : r; });
        },
        .misc_object_set = +[](Source* s,IsleGameObject* object,std::uint32_t index,IsleGameObject* target) noexcept {
            return with(s,[&](NativeHost& h) { IsleActorSource* a = nullptr; auto r = actor_source(h,object,&a); return r == ISLE_GAME_OK ? isle_actor_owner_misc_object_set(a,index,target) : r; });
        },
        .reference_assign = +[](Source* s,IsleGameObject** field,IsleGameObject* value) noexcept { return with(s,[&](NativeHost& h) { return h.reference(field,value); }); },
        .send_to_link = +[](Source* s,IsleGameObject* requester,std::uint8_t link,std::int32_t command,IsleGameObject* sender) noexcept {
            return with(s,[&](NativeHost& h) { auto r = receiver(h,requester); return r == ISLE_GAME_OK ? boolean(h.services()->send_command_to_link(link,command,sender)) : r; });
        },
        .send_addressed = +[](Source* s,IsleGameObject* requester,IsleGameObject* destination,std::int32_t command,IsleGameObject* sender) noexcept {
            return with(s,[&](NativeHost& h) { auto r = receiver(h,requester); return r == ISLE_GAME_OK ? boolean(h.services()->send_command(destination,command,sender)) : r; });
        },
        .model = +[](Source* s,IsleGameObject* object,IsleGameModelRef* output) noexcept { if (output) *output = {}; return with(s,[&](NativeHost& h) { return model(h,object,output); }); },
        .aux_read = +[](Source* s,const IsleGameModelRef* reference,const IsleGameModelTransform** output) noexcept { if (output) *output = nullptr; return with(s,[&](NativeHost& h) { return auxiliary(h,reference,output); }); },
        .root_read = +[](Source* s,const IsleGameModelRef* reference,const IsleGameVec3** output) noexcept {
            if (!output) return ISLE_GAME_INVALID_ARGUMENT; *output = nullptr;
            return with(s,[&](NativeHost& h) { IsleGameModel* native = nullptr; auto r = isle_game_model_resolve(h.game(),reference,&native); if (r != ISLE_GAME_OK) return r; const IsleGameModelNode* nodes = nullptr; std::size_t count = 0; r = isle_game_model_peek_nodes(h.game(),native,&nodes,&count); if (r != ISLE_GAME_OK) return r; if (!nodes || !count || nodes[0].parent != UINT32_MAX) return ISLE_GAME_INVALID_STATE; *output = &nodes[0].transform.position; return ISLE_GAME_OK; });
        },
        .component_store = +[](Source* s,const IsleGameModelRef* reference,std::uint32_t mask,float value) noexcept { return with(s,[&](NativeHost& h) { return component(h,reference,mask,value); }); },
        .item = +[](Source* s,IsleGameObject* object,const IsleItemSource** output) noexcept {
            if (!output) return ISLE_GAME_INVALID_ARGUMENT; *output = nullptr;
            return with(s,[&](NativeHost& h) { IsleItemSource* actual = nullptr; auto r = isle_item_source_get(h.game(),object,&actual); if (r != ISLE_GAME_OK) return r; if (!h.items_owner() || !actual || h.items_owner()->source(object) != actual) return ISLE_GAME_INVALID_STATE; *output = actual; return ISLE_GAME_OK; });
        },
        .current_block = +[](Source* s,IsleVolcanoBlock** output) noexcept { if (!output) return ISLE_GAME_INVALID_ARGUMENT; *output = nullptr; return with(s,[&](NativeHost& h) { if (!h.world_owner()) return ISLE_GAME_INVALID_STATE; *output = reinterpret_cast<IsleVolcanoBlock*>(h.world_owner()->current()); return ISLE_GAME_OK; }); },
        .block_position = +[](Source* s,IsleVolcanoBlock* block,IsleGameVec3* output) noexcept {
            return with(s,[&](NativeHost& h) { auto* native = reinterpret_cast<world::Block*>(block); if (!output || !h.valid_block(native) || !native->resource) return ISLE_GAME_INVALID_ARGUMENT; const auto p = native->resource->metadata.world_position; *output = {p.x,p.y,p.z}; return ISLE_GAME_OK; });
        },
        .add_one = +[](Source* s,IsleVolcanoBlock* first,IsleVolcanoBlock* second,const IsleVolcanoAutomaticSpawn* spawn,const IsleVolcanoDefinition* definition,IsleGameObject** output) noexcept {
            if (!output) return ISLE_GAME_INVALID_ARGUMENT; *output = nullptr;
            return with(s,[&](NativeHost& h) { if (!spawn || !definition) return ISLE_GAME_INVALID_ARGUMENT; auto* a = reinterpret_cast<world::Block*>(first); auto* b = reinterpret_cast<world::Block*>(second); if (!h.valid_block(a) || !h.valid_block(b)) return ISLE_GAME_INVALID_ARGUMENT; auto made = h.actors()->add_one_volcano(*definition,*spawn,a,b); if (!made) return ISLE_GAME_EXECUTION_FAILURE; *output = made.object; return ISLE_GAME_OK; });
        },
        .add_geo = +[](Source* s,IsleGameObject* object,std::uint16_t requested,const IsleVolcanoDefinition* definition,IsleGameObject** output) noexcept {
            if (!output) return ISLE_GAME_INVALID_ARGUMENT; *output = nullptr;
            return with(s,[&](NativeHost& h) { if (!definition) return ISLE_GAME_INVALID_ARGUMENT; auto r = receiver(h,object); if (r != ISLE_GAME_OK) return r; auto made = h.actors()->add_geo_volcano(object,requested,*definition); if (!made) return ISLE_GAME_EXECUTION_FAILURE; *output = made.object; return ISLE_GAME_OK; });
        },
        .random_range = +[](Source* s,std::int32_t range,std::int32_t* output) noexcept { return with(s,[&](NativeHost& h) { if (!output || !h.root()->source().rng) return ISLE_GAME_INVALID_ARGUMENT; *output = isle_rng_range(h.root()->source().rng,range); return ISLE_GAME_OK; }); },
        .random_float = +[](Source* s,float* output) noexcept { return with(s,[&](NativeHost& h) { if (!output || !h.root()->source().rng) return ISLE_GAME_INVALID_ARGUMENT; *output = isle_rng_next_float(h.root()->source().rng); return ISLE_GAME_OK; }); },
        .ground = +[](Source* s,float x,float z,IsleActorGround* output,std::int32_t* found) noexcept {
            return with(s,[&](NativeHost& h) { if (!output || !found || !h.world_owner()) return ISLE_GAME_INVALID_ARGUMENT; world::GroundResult ground; *found = h.world_owner()->ground_at(x,z,ground) ? 1 : 0; *output = {ground.height,{ground.normal.x,ground.normal.y,ground.normal.z},std::uint32_t(ground.surface)}; return ISLE_GAME_OK; });
        },
        .player = +[](Source* s,IsleGameObject** output) noexcept { if (!output) return ISLE_GAME_INVALID_ARGUMENT; *output = nullptr; return with(s,[&](NativeHost& h) { auto* object = h.root()->source().object; auto r = receiver(h,object); if (r == ISLE_GAME_OK) *output = object; return r; }); },
        .player_model = +[](Source* s,IsleGameModelRef* output) noexcept { if (output) *output = {}; return with(s,[&](NativeHost& h) { return model(h,h.root()->source().object,output); }); },
        .sound_play = +[](Source* s,std::int32_t id,std::int32_t* output) noexcept { if (output) *output = -1; return with(s,[&](NativeHost& h) { return sound(h,id,0x7fff,64,1.0f,0,output); }); },
        .sound_play_params = +[](Source* s,std::int32_t id,std::int32_t volume,std::int32_t pan,float pitch,std::int32_t reverb,std::int32_t* output) noexcept { if (output) *output = -1; return with(s,[&](NativeHost& h) { return sound(h,id,volume,pan,pitch,reverb,output); }); },
        .sound_identity = +[](Source* s,std::uint32_t handle,std::int32_t* output) noexcept { return with(s,[&](NativeHost& h) { if (!output) return ISLE_GAME_INVALID_ARGUMENT; *output = h.audio().playing_sound(handle); return ISLE_GAME_OK; }); },
        .sound_stop = +[](Source* s,std::uint32_t handle) noexcept { return with(s,[&](NativeHost& h) { return audio_result(h.audio().stop_sound(handle)); }); },
        .sound_volume = +[](Source* s,std::uint32_t handle,std::int32_t volume) noexcept { return with(s,[&](NativeHost& h) { return audio_result(h.audio().sound_volume(handle,std::uint32_t(volume))); }); },
        .sound_stop_all = +[](Source* s) noexcept { return with(s,[](NativeHost& h) { return audio_result(h.audio().stop_sounds()); }); },
        .bgm_volume = +[](Source* s,std::int32_t player,std::int32_t volume) noexcept { return with(s,[&](NativeHost& h) { return audio_result(h.audio().bgm_volume(std::uint32_t(player),std::uint32_t(volume))); }); },
        .bgm_stop = +[](Source* s) noexcept { return with(s,[](NativeHost& h) { return audio_result(h.audio().stop_bgm()); }); },
        .bgm_play = +[](Source* s,std::int32_t song) noexcept { return with(s,[&](NativeHost& h) { return audio_result(h.audio().play_song(0,std::uint32_t(song))); }); },
        .environment_sound = +[](Source* s,IsleGameObject* object,std::uint8_t category,std::int32_t id) noexcept { return with(s,[&](NativeHost& h) { IsleActorSource* a = nullptr; auto r = actor_source(h,object,&a); return r == ISLE_GAME_OK ? isle_actor_owner_environment_sound(a,category,id) : r; }); },
        .remove_items = +[](Source* s) noexcept { return with(s,[](NativeHost& h) { if (!h.items_owner()) return ISLE_GAME_INVALID_STATE; return h.items_owner()->remove_flying() ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE; }); },
        .reset_camera = +[](Source* s) noexcept { return with(s,[](NativeHost& h) { return isle_player_root_invoke_reset_main_camera_settings(&h.root()->source()); }); },
        .reset_messages = +[](Source* s) noexcept { return with(s,[](NativeHost& h) { auto* ui = h.root()->source().ui; return ui ? isle_player_ui_Msg_Reset(ui) : ISLE_GAME_INVALID_STATE; }); },
        .cutscene_start = +[](Source* s,IsleGameObject* object,std::uint32_t identity,float frame) noexcept {
            return with(s,[&](NativeHost& h) { auto r = receiver(h,object); if (r != ISLE_GAME_OK) return r; IslePlayerCameraAnimationClip clip{}; if (identity) { r = camera_clip(h,identity,clip); if (r != ISLE_GAME_OK) return r; } return isle_player_root_invoke_start_cutscene(&h.root()->source(),object,identity ? &clip : nullptr,frame); });
        },
        .main_camera = +[](Source* s,IsleGameCamera** output) noexcept { if (!output) return ISLE_GAME_INVALID_ARGUMENT; *output = nullptr; return with(s,[&](NativeHost& h) { auto* camera = h.root()->source().camera; IsleGameObject* object = nullptr; auto r = isle_game_camera_object(h.game(),camera,&object); if (r == ISLE_GAME_OK) *output = camera; return r; }); },
        .camera_object = +[](Source* s,IsleGameCamera* camera,IsleGameObject** output) noexcept { if (output) *output = nullptr; return with(s,[&](NativeHost& h) { return isle_game_camera_object(h.game(),camera,output); }); },
        .camera_speed = +[](Source* s,IsleGameCamera* camera,float speed) noexcept { return with(s,[&](NativeHost& h) { if (camera != h.root()->source().camera || !h.camera()) return ISLE_GAME_INVALID_ARGUMENT; return boolean(bool(h.camera()->set_speed(speed))); }); },
        .camera_animation = +[](Source* s,IsleGameCamera* camera,std::uint32_t identity,float frame) noexcept {
            return with(s,[&](NativeHost& h) {
                if (camera != h.root()->source().camera || !h.camera()) return ISLE_GAME_INVALID_ARGUMENT;
                if (!identity) return boolean(bool(h.camera()->set(nullptr,frame)));
                IslePlayerCameraAnimationClip clip{};
                const auto result = camera_clip(h,identity,clip);
                return result == ISLE_GAME_OK ? boolean(bool(h.camera()->set(&clip,frame))) : result;
            });
        },
        .camera_process = +[](Source* s,IsleGameObject* object,std::int32_t kind,std::int32_t priority,IsleGameProcess** output) noexcept {
            if (!output) return ISLE_GAME_INVALID_ARGUMENT; *output = nullptr;
            return with(s,[&](NativeHost& h) { IsleGameObject* actual = nullptr; auto r = isle_game_camera_object(h.game(),h.root()->source().camera,&actual); if (r != ISLE_GAME_OK) return r; if (object != actual || (kind != 0 && kind != 1) || priority < 0) return ISLE_GAME_INVALID_ARGUMENT; return isle_game_add_source_process(h.game(),object,isle_course_scene_camera_update,kind == 0 ? ISLE_GAME_STACKFUL : ISLE_GAME_SYNCHRONOUS,std::uint32_t(priority),output); });
        },
        .pause_processes = +[](Source* s,IsleGameObject* object) noexcept { return with(s,[&](NativeHost& h) { return isle_game_pause_object_processes(h.game(),object,1); }); },
        .sky_speed = +[](Source* s,float speed) noexcept { return with(s,[&](NativeHost& h) { if (!h.world_owner()) return ISLE_GAME_INVALID_STATE; h.world_owner()->set_sky_animation_speed(speed); return ISLE_GAME_OK; }); },
        .player_model_init = +[](Source* s,IsleGameObject** output) noexcept {
            if (!output) return ISLE_GAME_INVALID_ARGUMENT; *output = nullptr;
            return with(s,[&](NativeHost& h) { if (!h.cart()) return ISLE_GAME_INVALID_STATE; const auto made = h.cart()->init(); if (made) { *output = made.object; return ISLE_GAME_OK; } return made.status.error == player::cart::OwnerError::capacity ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE; });
        },
        .player_animation = +[](Source* s,std::uint32_t model_identity,std::uint32_t material_identity,float frame,float speed) noexcept {
            return with(s,[&](NativeHost& h) {
                if (!h.cart()) return ISLE_GAME_INVALID_STATE;
                const auto* model_clip = model_identity ? h.cart()->model_animation(model_identity) : nullptr;
                const auto* material_clip = material_identity ? h.cart()->material_animation(material_identity) : nullptr;
                if ((model_identity && !model_clip) || (material_identity && !material_clip)) return ISLE_GAME_INVALID_ARGUMENT;
                return boolean(bool(h.cart()->set_animation(model_clip,material_clip,frame,speed)));
            });
        },
        .animation_callback = +[](Source* s,IsleGameObject* object,IsleVolcanoAnimationCallback callback) noexcept {
            return with(s,[&](NativeHost& h) { if (h.cart() && h.cart()->instance(object)) { if (callback && callback != volcano_gate_PlayerAnimCallback) return ISLE_GAME_INVALID_ARGUMENT; return boolean(bool(h.cart()->set_callback(object,callback ? isle_volcano_actor_cart_event : nullptr,callback ? s : nullptr))); } return boolean(bool(h.actors()->bind_animation_callback(*s,object,callback))); });
        },
        .animation_speed = +[](Source* s,IsleGameObject* object,float speed) noexcept { return with(s,[&](NativeHost& h) { if (h.cart() && h.cart()->instance(object)) return boolean(bool(h.cart()->set_animation_speed(speed))); const auto handle = actor_handle(h,object); return handle ? boolean(bool(h.actors()->set_animation_speed(handle,speed))) : ISLE_GAME_INVALID_ARGUMENT; }); },
        .change_block_ground = +[](Source* s,IsleGameObject* object,float ax,float ay,float az,float bx,float by,float bz) noexcept { return with(s,[&](NativeHost& h) { const auto handle = actor_handle(h,object); return handle ? isle_meowth_source_call_change_block(object,ax,ay,az,bx,by,bz,1) : ISLE_GAME_INVALID_ARGUMENT; }); },
        .gb_pipe_sync = +[](Source* s,std::uint32_t stream) noexcept { return with(s,[&](NativeHost& h) { return packet(h,stream,0xe7000000u,0); }); },
        .gb_cycle = +[](Source* s,std::uint32_t stream,IsleVolcanoCycle cycle) noexcept { return with(s,[&](NativeHost& h) { if (cycle != ISLE_VOLCANO_CYCLE_ONE && cycle != ISLE_VOLCANO_CYCLE_TWO) return ISLE_GAME_INVALID_ARGUMENT; return packet(h,stream,0xe3000a01u,cycle == ISLE_VOLCANO_CYCLE_TWO ? 0x00100000u : 0u); }); },
        .gb_render_mode = +[](Source* s,std::uint32_t stream,IsleVolcanoRenderMode first,IsleVolcanoRenderMode second) noexcept {
            return with(s,[&](NativeHost& h) {
                if ((first != ISLE_VOLCANO_RENDER_PASS && first != ISLE_VOLCANO_RENDER_AA_ZB_XLU) ||
                    second != ISLE_VOLCANO_RENDER_AA_ZB_XLU_SECOND) return ISLE_GAME_INVALID_ARGUMENT;
                // Pinned ultralib/include/PR/gbi.h654-687,699-702,909-910,1006:
                // PASS=GBL_c1(IN,0,IN,1), XLU1/2=RM_AA_ZB_XLU_SURF(1/2).
                constexpr std::uint32_t pass = 0x0c080000u, xlu_first = 0x004049d8u, xlu_second = 0x001049d8u;
                return packet(h,stream,0xe200001cu,(first == ISLE_VOLCANO_RENDER_PASS ? pass : xlu_first) | xlu_second);
            });
        },
        .gb_texture_lut = +[](Source* s,std::uint32_t stream,IsleVolcanoTextureLut lut) noexcept { return with(s,[&](NativeHost& h) { if (lut != ISLE_VOLCANO_TEXTURE_LUT_NONE) return ISLE_GAME_INVALID_ARGUMENT; return packet(h,stream,0xe3001001u,0); }); },
        .render_type_i = +[](Source* s,IsleGameObject* object) noexcept { return with(s,[&](NativeHost& h) { auto* producer = h.actor_packet_producer(); return producer ? producer->source_hierarchy(object,menu::RenderType::i_no_fog) : ISLE_GAME_INVALID_STATE; }); },
        .render_type_d_fogged = +[](Source* s,IsleGameObject* object) noexcept { return with(s,[&](NativeHost& h) { auto* producer = h.actor_packet_producer(); return producer ? producer->source_hierarchy(object,menu::RenderType::d_fogged) : ISLE_GAME_INVALID_STATE; }); },
        .vector_distance = +[](Source* s,const IsleGameVec3* first,const IsleGameVec3* second,float* output) noexcept { return with(s,[&](NativeHost&) { if (!first || !second || !output) return ISLE_GAME_INVALID_ARGUMENT; const float x = first->x-second->x, y = first->y-second->y, z = first->z-second->z; *output = std::sqrt(x*x+y*y+z*z); return ISLE_GAME_OK; }); }
    };
}
}

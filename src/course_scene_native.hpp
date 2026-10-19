#ifndef ISLE_COURSE_SCENE_NATIVE_HPP
#define ISLE_COURSE_SCENE_NATIVE_HPP
#include "course_native_host.hpp"
#include "game_audio.hpp"
#include "player_cart.hpp"
#include "player_cart_resources.hpp"
#include "player_camera_animation.hpp"
#include "player_renderer.hpp"
#include "scene_services.hpp"
#include "item_owner.hpp"
#include "beach_meowth_source.h"
#include "course_scene_source.h"
#include <algorithm>
#include <array>
#include <span>
#include <stdexcept>
#include <vector>

namespace isle::course {
// Hosted-native policy is explicit. Legacy N64 CIC/SP startup probes are not
// applicable, not observed/passed; only an admitted canonical Snapshot plus
// this platform selection authorizes the translated illegal-copy predicate.
struct SceneAsset { std::uint32_t first = 0, end = 0; std::span<const std::byte> bytes; bool loaded = false; };
struct SceneEffect { menu::EffectHandle handle; };
struct EnvelopeEvidence {
    // Physical reservations are explicit operational envelopes. A frozen pool
    // alone is not a proof of the full Original reachable continuation peak.
    bool actual_envelope_proven = false;
    std::size_t initialized_spawns = 0, world_objects = 0, reference_fields = 0;
    const char* unknown_actual_envelope = "Unbounded-play queued-command/retained-incarnation and stackful-continuation peak require actual Source closure proof";
};
struct SceneMechanics {
    NativeHost* host = nullptr;
    application::Manager* manager = nullptr;
    application::Photos* photos = nullptr;
    game::State* state = nullptr;
    game_input::Service* input = nullptr;
    game_audio::Session* audio = nullptr;
    EntryNativeProfile native_profile = EntryNativeProfile::unspecified;
    const rom::Snapshot* snapshot = nullptr;
    const SceneJoinReceipt* receipt = nullptr;
    IsleGameObject* helper = nullptr;
    bool requested = false, ui_bound = false, end_bound = false, pause_bound = false;
    bool height_bound = false, ceiling_bound = false;
    std::array<SceneAsset,5> assets{};
    std::array<SceneEffect,10> effects{};
    EnvelopeEvidence envelope;
    std::vector<IsleGameModelCapacity> model_blocks;
    NativeCapacity capacity;
    IslePlayerSource* player() const noexcept { return host && host->root() ? &host->root()->source() : nullptr; }
    bool setup() const noexcept { return host && host->closure_prepared() && !host->initialized() && isle_game_require_idle(host->game()) == ISLE_GAME_OK; }
    bool receiver(IsleGameObject*) const noexcept;
    IsleGameResult joined() const noexcept;
    IsleGameResult request(const void* actual, const void* expected) noexcept;
    IsleGameResult initialize_ui(IsleGameObject** output = nullptr) noexcept;
    IsleGameResult background(std::uint8_t*,std::uint8_t*,std::uint8_t*) noexcept;
    IsleGameResult map(std::uint32_t,bool ceiling) noexcept;
    IsleGameResult world(std::uint32_t,std::int32_t,std::int32_t,std::int32_t,std::int32_t,std::int32_t) noexcept;
    IsleGameResult block_next(const world::Block*,world::Block**) noexcept;
    IsleGameResult block_position(const world::Block*,IsleGameVec3*) noexcept;
    IsleGameResult registry(std::int32_t,std::int32_t,std::int32_t,std::int32_t) noexcept;
    IsleGameResult pokemon_add(world::Block*,world::Block*,std::uint32_t,std::size_t) noexcept;
    IsleGameResult pokemon_change(world::Block*,world::Block*,std::uint32_t,std::size_t,IsleCourseActorBase*,IsleGameResult (*)(IsleCourseActorBase*,std::uint32_t,IsleGameObject*,const IsleGameVec3*,const IsleGameVec3*)) noexcept;
    IsleGameResult pokemon_remove(world::Block*,std::uint32_t,std::size_t,IsleCourseActorBase*,IsleGameResult (*)(IsleCourseActorBase*,std::uint32_t,IsleGameObject*)) noexcept;
    template<class Rows> bool definitions(const Rows* rows,std::size_t count,std::uint32_t table) const noexcept {
        if (!host || !rows) return false;
        const auto values=host->actor_resources().definitions();
        for (std::size_t i=0;i<count;++i) {
            const auto found=std::find_if(values.begin(),values.end(),[&](const auto& value){ return value.wire.guest_address==table+16*i; });
            if (found==values.end() || found->actor_id!=rows[i].id || found->factory_identity!=rows[i].initialize_identity || found->change_block_identity!=rows[i].update_identity || found->remove_identity!=rows[i].remove_identity) return false;
        }
        return true;
    }
    IsleGameResult asset(std::size_t,std::uint32_t,std::uint32_t,SceneAsset**) noexcept;
    IsleGameResult bank(std::int32_t,const SceneAsset*,const SceneAsset*,menu::EffectGroup) noexcept;
    IsleGameResult fx_camera(std::int32_t,std::int32_t,IsleGameCamera*,IsleGameObject**) noexcept;
    IsleGameResult fx_pool(std::int32_t,IsleGameObject**) noexcept;
    IsleGameResult fx_publish(std::uint32_t,IsleGameObject*) noexcept;
    IsleGameResult dither(std::uint32_t,std::uint32_t) noexcept;
    IsleGameResult effect(std::int32_t,std::int32_t,SceneEffect**) noexcept;
    IsleGameResult attach(SceneEffect*,const IslePlayerCartNode*) noexcept;
    IsleGameResult reference(IsleGameObject**,IsleGameObject*) noexcept;
    IsleGameResult helper_create(std::int32_t,IsleGameSourceCallback,std::int32_t,std::uint32_t,IsleGameObject**) noexcept;
    IsleGameResult process(IsleGameObject*,IsleGameSourceCallback,std::int32_t,std::int32_t,IsleGameProcess**) noexcept;
    IsleGameResult camera_component(IsleGameCamera*,std::uint32_t,float*,bool,float = 0) noexcept;
    IsleGameResult cart_retire(IsleGameObject*) noexcept;
    IsleGameResult release_references() noexcept;
    IsleGameResult environment(const void*,std::size_t,std::size_t,std::uint16_t (*)(const void*,std::size_t),std::uint8_t (*)(const void*,std::size_t),std::uint8_t (*)(const void*,std::size_t)) noexcept;
    IsleGameResult sound(std::int32_t,std::int32_t,std::int32_t,float,std::int32_t,std::int32_t*) noexcept;
    static IsleGameResult audio_result(isle::audio::Error value) noexcept { return value==isle::audio::Error::none ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE; }
    void prepare_capacity(std::int32_t,std::size_t);
};
}
#endif

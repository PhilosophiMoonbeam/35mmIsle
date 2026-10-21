#include "actor_owner.hpp"
#include "actor_interaction_services.hpp"
#include "actor_photo_eligibility.hpp"
#include "actor_collision.hpp"
#include "beach_initial_species_source.h"
#include "beach_meowth_source.h"
#include "beach_snorlax_kangaskhan_source.h"
#include "beach_eevee_chansey_source.h"
#include "beach_scyther_special_source.h"
#include "source_effect_world.hpp"
#include "player_root_source.h"
#include "menu_scene_bridge.h"
#include "player_root.hpp"
#include "actor_resource_catalog.hpp"
#include "rainbow_actor.hpp"
#include "actor_native_context.hpp"
#include "course_native_host.hpp"
#include "tunnel_actor.hpp"
#include "volcano_actor.hpp"
#include "river_actor.hpp"
#include "cave_actor.hpp"
#include "valley_actor.hpp"

#include <bit>
#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>
#include <stdexcept>
#include <utility>
#include <type_traits>

namespace isle::actor {
namespace {
OwnerStatus rejected(OwnerError error, const char* detail) noexcept { return {error,detail}; }
IsleGameModelTransform native(const menu::animation::Transform& t) noexcept {
    return {{t.position.x,t.position.y,t.position.z},{t.rotation.x,t.rotation.y,t.rotation.z},{t.scale.x,t.scale.y,t.scale.z}};
}
constexpr float maximum_distance = std::numeric_limits<float>::max();
constexpr std::size_t retained_misc_count =
    sizeof(IsleActorSource{}.miscVars)/sizeof(IsleActorSource{}.miscVars[0]);
static_assert(retained_misc_count<32);
enum class AutomaticPathRepair { semantic_null, source_written };
struct AutomaticSpawnRepair {
    std::int32_t course;
    std::uint32_t helper, line, written_fields, family, factory;
    std::int32_t behavior;
    AutomaticPathRepair path;
};
// Explicit native portability repair, NOT authored Source defaults or recovered
// N64 residual-stack values. Only these exact automatic origins resolve absent
// behavior to0 and absent path to semantic NULL. Original Source masks stay
// unchanged. Shellder's repaired0 deliberately selects its visible path branch.
constexpr AutomaticSpawnRepair automatic_spawn_repairs[] = {
    {3,0x802d8a48u,91,0x3ffu,603,0x802da9a4u,0,AutomaticPathRepair::semantic_null},
    {3,0x802d8b34u,122,0x3ffu,80,0x802da4b8u,0,AutomaticPathRepair::semantic_null},
    {3,0x802d8c24u,152,0x3ffu,129,0x802dd0a8u,0,AutomaticPathRepair::semantic_null},
    {3,0x802d8cf0u,181,0x3ffu,60,0x802dceb0u,0,AutomaticPathRepair::semantic_null},
    {3,0x802d8dc4u,211,0x3ffu,54,0x802dbb54u,0,AutomaticPathRepair::semantic_null},
    {3,0x802da9e0u,27,0x7ffu,90,0x802da8ccu,0,AutomaticPathRepair::source_written},
    {3,0x802daaccu,59,0x7ffu,91,0x802db064u,0,AutomaticPathRepair::source_written},
    {3,0x802dff94u,29,0x3ffu,1002,0x802dff5cu,0,AutomaticPathRepair::semantic_null},
    {5,0x802c5e68u,84,0x3ffu,118,0x802d2540u,0,AutomaticPathRepair::semantic_null},
    {5,0x802c5f70u,120,0x3ffu,147,0x802ca688u,0,AutomaticPathRepair::semantic_null},
    {5,0x802c606cu,153,0x3ffu,129,0x802d21c8u,0,AutomaticPathRepair::semantic_null},
};
const AutomaticSpawnRepair* automatic_spawn_repair(std::int32_t course,
    std::uint32_t helper,std::uint32_t line,std::uint32_t fields,
    std::uint32_t family,std::uint32_t factory) noexcept {
    for (const auto& policy:automatic_spawn_repairs)
        if (policy.course==course && policy.helper==helper && policy.line==line &&
            policy.written_fields==fields && policy.family==family && policy.factory==factory)
            return &policy;
    return nullptr;
}
enum class FactoryWrapper { ground, ordinary, ground_dl4, ordinary_dl4, meowth, valley_magikarp };
struct FactorySpec {
    std::uint32_t actor_id = 0, identity = 0;
    FactoryWrapper wrapper = FactoryWrapper::ground;
    IsleActorReturningFactory returning = nullptr;
    IsleActorVoidFactory nonreturning = nullptr;
    IsleGameSourceCallback initial_state = nullptr;
    std::uint32_t initialization = 0;
    IsleActorSixFactory six = nullptr;
};
// Callable authority: real typed C definitions, never guest-function casts.
// Numeric model/init/setup/header authority remains the same closed compiled
// CourseCatalog that decoded Resources; there is no second tuple registry.
const FactorySpec beach_factories[] = {
    {1003,0x8035e0d4,FactoryWrapper::ground_dl4,func_8035E0D4_4FE4E4,nullptr,func_8035DF20_4FE330},
    {129,0x802cac48,FactoryWrapper::ground,func_beach_802CAC48,nullptr,func_beach_802CAB50},
    {12,0x802c7e38,FactoryWrapper::ordinary,nullptr,func_beach_802C7E38,func_beach_802C7B10},
    {84,0x802c8570,FactoryWrapper::ground,nullptr,func_beach_802C8570,func_beach_802C82B0},
    {131,0x802c8274,FactoryWrapper::ordinary,nullptr,func_beach_802C8274,func_beach_802C7E70},
    {16,0x802c8d3c,FactoryWrapper::ordinary,nullptr,func_beach_802C8D3C,func_beach_802C85B0},
    {16,0x802c8d74,FactoryWrapper::ordinary,nullptr,func_beach_802C8D74,func_beach_802C85B0},
    {25,0x802cbca8,FactoryWrapper::ground,nullptr,func_beach_802CBCA8,func_beach_802CACD4},
    {1004,0x802ca418,FactoryWrapper::ordinary,nullptr,func_beach_802CA418,func_beach_802CA300},
    {1001,0x802cab14,FactoryWrapper::ordinary_dl4,nullptr,func_beach_802CAB14,func_beach_802CA450},
    {143,0x802c5c28,FactoryWrapper::ground,nullptr,func_beach_802C5C28,func_beach_802C56C8},
    {52,0x802c7270,FactoryWrapper::meowth,nullptr,func_beach_802C7270,func_beach_802C5DAC},
    {133,0x802c7ad0,FactoryWrapper::ground,nullptr,func_beach_802C7AD0,func_beach_802C72E0},
    {123,0x802c91cc,FactoryWrapper::ordinary,nullptr,func_beach_802C91CC,func_beach_802C8DB0},
    {123,0x802c9204,FactoryWrapper::ordinary,nullptr,func_beach_802C9204,func_beach_802C8DB0},
    {113,0x802c9a7c,FactoryWrapper::ground,nullptr,func_beach_802C9A7C,func_beach_802C9240},
    {115,0x802ca05c,FactoryWrapper::ground,nullptr,func_beach_802CA05C,func_beach_802C9AC0},
    {1005,0x802ca2bc,FactoryWrapper::ordinary,nullptr,func_beach_802CA2BC,func_beach_802CA148},
    {1006,0x802cbda0,FactoryWrapper::ordinary_dl4,nullptr,func_beach_802CBDA0,func_beach_802CBCE0},
};
FactorySpec compiled_factory(std::int32_t course,std::uint32_t id,std::uint32_t factory) noexcept {
    if (course==0 || (course>=1 && course<=5 && id==1003 && factory==0x8035e0d4u)) {
        for (const auto& spec:beach_factories)
            if (spec.actor_id==id && spec.identity==factory) return spec;
        return {};
    }
    auto callback=[&](std::uint32_t identity) {
        for (const auto& binding:course_callbacks(course))
            if (binding.identity==identity) return binding.function;
        return static_cast<IsleGameSourceCallback>(nullptr);
    };
    switch (course) {
    case 1:
        for (const auto& binding:tunnel::actor_factories()) {
            if (binding.actor_id!=id || binding.factory!=factory) continue;
            FactoryWrapper wrapper=FactoryWrapper::ground;
            switch (binding.wrapper) {
            case tunnel::ActorFactoryWrapper::ground: wrapper=FactoryWrapper::ground; break;
            case tunnel::ActorFactoryWrapper::ordinary: wrapper=FactoryWrapper::ordinary; break;
            case tunnel::ActorFactoryWrapper::ground_dl4: wrapper=FactoryWrapper::ground_dl4; break;
            case tunnel::ActorFactoryWrapper::ordinary_dl4: wrapper=FactoryWrapper::ordinary_dl4; break;
            }
            return {id,factory,wrapper,binding.function,nullptr,callback(binding.initial_state),binding.initialization,nullptr};
        }
        break;
    case 2:
        for (const auto& binding:volcano::actor_factories()) {
            if (binding.actor_id!=id || binding.factory!=factory) continue;
            FactoryWrapper wrapper=FactoryWrapper::ground;
            switch (binding.wrapper) {
            case volcano::ActorFactoryWrapper::ground: wrapper=FactoryWrapper::ground; break;
            case volcano::ActorFactoryWrapper::ordinary: wrapper=FactoryWrapper::ordinary; break;
            case volcano::ActorFactoryWrapper::ground_dl4: wrapper=FactoryWrapper::ground_dl4; break;
            case volcano::ActorFactoryWrapper::ordinary_dl4: wrapper=FactoryWrapper::ordinary_dl4; break;
            }
            return {id,factory,wrapper,binding.function,nullptr,callback(binding.initial_state),binding.initialization,nullptr};
        }
        break;
    case 3:
        for (const auto& binding:river::actor_factories()) {
            if (binding.actor_id!=id || binding.factory!=factory) continue;
            FactoryWrapper wrapper=FactoryWrapper::ground;
            switch (binding.wrapper) {
            case river::ActorFactoryWrapper::ground: wrapper=FactoryWrapper::ground; break;
            case river::ActorFactoryWrapper::ordinary: wrapper=FactoryWrapper::ordinary; break;
            case river::ActorFactoryWrapper::ground_dl4: wrapper=FactoryWrapper::ground_dl4; break;
            case river::ActorFactoryWrapper::ordinary_dl4: wrapper=FactoryWrapper::ordinary_dl4; break;
            }
            return {id,factory,wrapper,binding.native_adapter,nullptr,callback(binding.initial_state),binding.initialization,nullptr};
        }
        break;
    case 4:
        for (const auto& binding:cave::actor_factories()) {
            if (binding.actor_id!=id || binding.factory!=factory) continue;
            FactoryWrapper wrapper=FactoryWrapper::ground;
            switch (binding.wrapper) {
            case cave::ActorFactoryWrapper::ground: wrapper=FactoryWrapper::ground; break;
            case cave::ActorFactoryWrapper::ordinary: wrapper=FactoryWrapper::ordinary; break;
            case cave::ActorFactoryWrapper::ordinary_dl4: wrapper=FactoryWrapper::ordinary_dl4; break;
            }
            return {id,factory,wrapper,binding.returning,binding.nonreturning,callback(binding.initial_state),binding.initialization,nullptr};
        }
        break;
    case 5:
        for (const auto& binding:valley::actor_factories()) {
            if (binding.actor_id!=id || binding.factory!=factory) continue;
            FactoryWrapper wrapper=FactoryWrapper::ground;
            switch (binding.wrapper) {
            case valley::ActorFactoryWrapper::ground: wrapper=FactoryWrapper::ground; break;
            case valley::ActorFactoryWrapper::ordinary: wrapper=FactoryWrapper::ordinary; break;
            case valley::ActorFactoryWrapper::ordinary_dl4: wrapper=FactoryWrapper::ordinary_dl4; break;
            case valley::ActorFactoryWrapper::behavior_not3_ground_else_ordinary: wrapper=FactoryWrapper::valley_magikarp; break;
            }
            return {id,factory,wrapper,binding.returning5,binding.nonreturning5,callback(binding.initial_state),binding.initialization,binding.returning6};
        }
        break;
    case 6:
        for (const auto& binding:rainbow::actor_factories()) {
            if (binding.actor_id!=id || binding.factory!=factory) continue;
            return {id,factory,binding.wrapper==rainbow::ActorFactoryWrapper::ground ?
                FactoryWrapper::ground:FactoryWrapper::ordinary_dl4,
                binding.function,nullptr,callback(binding.initial_state),binding.initialization};
        }
        break;
    }
    return {};
}
bool initialized_definition(const actor::Resources& resources,const ActorDefinition& definition) {
    if (definition.wire.bytes.size()!=16 || definition.wire.u32(0)!=definition.actor_id ||
        definition.wire.u32(4)!=definition.factory_identity ||
        definition.wire.u32(8)!=definition.change_block_identity ||
        definition.wire.u32(12)!=definition.remove_identity) return false;
    for (const auto& fact:detail::course_catalog(resources.course_id()).definitions)
        if (fact.identity==definition.wire.guest_address && fact.actor_id==definition.actor_id &&
            fact.factory_identity==definition.factory_identity && fact.initialization==definition.initialization &&
            fact.change_block_identity==definition.change_block_identity &&
            fact.remove_identity==definition.remove_identity && fact.singleton==definition.singleton)
            return fact.actor_id!=0;
    return false;
}
}
OwnerStatus dispatch_event(IsleActorSource& actor, const menu::animation::CallbackEvent& event, EventServices context) noexcept {
    IsleActorSource* bound = nullptr;
    const auto state = context.animation.model(event.instance);
    const IsleGameModelNode* nodes = nullptr;
    std::size_t count = 0;
    if (isle_actor_source_get(context.services.game,actor.object,&bound) != ISLE_GAME_OK || bound != &actor ||
        isle_game_model_validate_binding(context.services.game,actor.object,context.model)!=ISLE_GAME_OK ||
        !state || isle_game_model_peek_nodes(context.services.game,context.model,&nodes,&count) != ISLE_GAME_OK ||
        event.node >= count || count != state.nodes.size())
        return rejected(OwnerError::admission,"actor callback context is not a live canonical native pose");
    if (context.native_callback && context.native_callback->index()!=0) {
        IsleActorNodeRef node{};
        if (isle_game_model_reference(actor.game,actor.object,context.model,&node.model)!=ISLE_GAME_OK)
            return rejected(OwnerError::native,"typed animation callback lost canonical model incarnation");
        node.node=event.node;
        IsleGameResult result=ISLE_GAME_INVALID_STATE;
        switch (context.native_callback->index()) {
        case 1: { const auto& binding=std::get<TunnelAnimationBinding>(*context.native_callback);
            result=isle_actor_native_tunnel_animation_event(binding.source,binding.function,&node,event.event_id,event.argument); break; }
        case 2: { const auto& binding=std::get<VolcanoAnimationBinding>(*context.native_callback);
            result=isle_actor_native_volcano_animation_event(binding.source,binding.function,&node,event.event_id,event.argument); break; }
        case 3: { const auto& binding=std::get<RiverAnimationBinding>(*context.native_callback);
            result=isle_actor_native_river_animation_event(binding.source,binding.function,&node,event.event_id,event.argument); break; }
        case 4: { const auto& binding=std::get<CaveAnimationBinding>(*context.native_callback);
            result=isle_actor_native_cave_animation_event(binding.source,binding.function,&node,event.event_id,event.argument); break; }
        case 5: { const auto& binding=std::get<ValleyAnimationBinding>(*context.native_callback);
            result=isle_actor_native_valley_animation_event(binding.source,binding.function,&node,event.event_id,event.argument); break; }
        }
        return result==ISLE_GAME_OK ? OwnerStatus{} : rejected(OwnerError::native,"original typed animation callback failed");
    }
    if (event.event_id == -1 || event.event_id == -2) {
        if (actor.loopCount == INT32_MAX) return rejected(OwnerError::admission,"actor loop count exceeds source signed domain");
        ++actor.loopCount;
        return {};
    }
    if (event.event_id < 0 || event.event_id > 3) return {};
    // Source conversion precedes the zero/positive policy, unlike Title.
    if (!std::isfinite(event.argument) || event.argument < -2147483648.0f || event.argument >= 2147483648.0f)
        return rejected(OwnerError::admission,"actor callback value exceeds source signed conversion domain");
    const auto value = static_cast<std::int32_t>(event.argument);
    const auto category = static_cast<std::uint8_t>(event.event_id);
    if (nodes[event.node].parent == UINT32_MAX) {
        if (value == 0) return context.sound.stop_category(actor.object,category) ? OwnerStatus{} : rejected(OwnerError::services,"actor sound category stop");
        const auto* header = static_cast<const actor::ActorAnimationHeader*>(actor.animHeader);
        if (!header) return rejected(OwnerError::admission,"root sound callback without current header");
        if (header->sound_ids.empty()) return {};
        if (value < 1 || static_cast<std::size_t>(value) > header->sound_ids.size())
            return rejected(OwnerError::admission,"root sound callback outside admitted current-header IDs");
        IsleGameObjectMetadata metadata{};
        if (isle_game_object_metadata(context.services.game,actor.object,&metadata) != ISLE_GAME_OK)
            return rejected(OwnerError::native,"actor sound source flags");
        if (!context.sound_enabled || (metadata.flags & 1u)) return {};
        return context.sound.play({actor.object,context.model,&actor.playerDist},category,header->sound_ids[static_cast<std::size_t>(value)-1])
            ? OwnerStatus{} : rejected(OwnerError::services,"actor sound play");
    }
    if (!actor.tangible || value <= 0 || !context.particle_owner || !context.effect_owner) return {};
    IsleGameObjectMetadata metadata{};
    if (isle_game_object_metadata(context.services.game,context.particle_owner,&metadata) != ISLE_GAME_OK ||
        isle_game_object_metadata(context.services.game,context.effect_owner,&metadata) != ISLE_GAME_OK)
        return rejected(OwnerError::lifetime,"actor source FX owner retired");
    const auto effect = context.effects.create_effect(category,static_cast<std::size_t>(value)-1,{event.instance,event.node});
    return effect.status ? OwnerStatus{} : rejected(OwnerError::services,"actor attached effect creation");
}

OwnerStatus step_source_animation(IsleActorSource& source,menu::animation::InstanceHandle instance,EventServices context) noexcept {
    IsleActorSource* bound=nullptr;
    IsleGameObjectMetadata metadata{};
    if (isle_actor_source_get(context.services.game,source.object,&bound)!=ISLE_GAME_OK || bound!=&source ||
        isle_game_model_validate_binding(context.services.game,source.object,context.model)!=ISLE_GAME_OK ||
        isle_game_object_metadata(context.services.game,source.object,&metadata)!=ISLE_GAME_OK)
        return rejected(OwnerError::native,"actor animation canonical source flags");
    if (!context.animation.set_object_flags(instance,metadata.flags) || !context.animation.step_model(instance))
        return rejected(OwnerError::animation,"actor animation evaluation");
    const auto state=context.animation.model(instance);
    if (!state) return rejected(OwnerError::animation,"actor active animation pose");
    for (std::size_t i=0;i<state.nodes.size();++i) {
        const auto transform=native(state.nodes[i].transform);
        if (isle_game_model_validate_transform(context.services.game,context.model,i,&transform)!=ISLE_GAME_OK)
            return rejected(OwnerError::native,"actor animation native pose preflight");
    }
    for (std::size_t i=0;i<state.nodes.size();++i) {
        const auto transform=native(state.nodes[i].transform);
        if (isle_game_model_set_transform(context.services.game,context.model,i,&transform)!=ISLE_GAME_OK ||
            isle_game_model_set_flags(context.services.game,context.model,i,state.nodes[i].flags)!=ISLE_GAME_OK)
            return rejected(OwnerError::native,"actor animation native pose publication");
    }
    for (const auto& event:context.animation.events()) {
        const auto result=dispatch_event(source,event,context);
        if (!result) return result;
    }
    return {};
}
OwnerStatus apply_source_animation(IsleActorSource& source,menu::animation::InstanceHandle instance,
    const actor::ActorAnimationHeader* header,float start,bool force,const actor::ActorInitialization& initialization,
    const actor::Resources& resources,EventServices context) noexcept {
    bool initialization_member=false,header_member=!header;
    std::size_t initialization_index=0;
    for (std::size_t i=0;i<resources.initializations().size();++i) if (&resources.initializations()[i]==&initialization) {
        initialization_member=true; initialization_index=i; break;
    }
    if (header) for (const auto& candidate:resources.headers()) if (&candidate==header) { header_member=true; break; }
    const auto state=context.animation.model(instance);
    IsleActorSource* bound=nullptr;
    if (!std::isfinite(start) || !initialization_member || !header_member || !state ||
        state.resource_model!=initialization.model || (header && header->initialization!=initialization_index) ||
        isle_actor_source_get(context.services.game,source.object,&bound)!=ISLE_GAME_OK || bound!=&source ||
        isle_game_model_validate_binding(context.services.game,source.object,context.model)!=ISLE_GAME_OK)
        return rejected(OwnerError::admission,"actor header/context not admitted for source initialization");
    if (header) {
        if (!source.modelAnims && isle_game_pause_source_processes(context.services.game,source.object,isle_actor_source_animation_process,0)!=ISLE_GAME_OK)
            return rejected(OwnerError::native,"actor animation function resume");
        const auto model_identity=header->model_animation_table.bytes.empty()?0:header->model_animation_table.guest_address;
        const auto material_identity=header->material_animation_table.bytes.empty()?0:header->material_animation_table.guest_address;
        if (source.modelAnims!=model_identity || force) {
            if (!context.sound.stop_category(source.object,0)) return rejected(OwnerError::services,"actor model rebind category zero stop");
            if (!context.animation.bind_model(instance,header->model_bindings,start,menu::animation::ModelBindPose::restore))
                return rejected(OwnerError::animation,"actor model-array binding");
            source.modelAnims=model_identity;
        }
        if (material_identity) {
            if (source.matAnims!=material_identity || force) {
                if (!context.animation.bind_materials(instance,header->material_bindings,0))
                    return rejected(OwnerError::animation,"actor material-array binding");
                source.matAnims=material_identity;
            }
            if (!context.animation.set_model_speed(instance,header->speed) || !context.animation.set_texture_speed(instance,header->speed))
                return rejected(OwnerError::animation,"actor model/material speed");
        } else {
            if (initialization.textures_present && !context.animation.reset_materials(instance))
                return rejected(OwnerError::animation,"actor original textures reset");
            if (!context.animation.set_model_speed(instance,header->speed)) return rejected(OwnerError::animation,"actor model speed");
            source.matAnims=0;
        }
        source.animHeader=header;
        const auto result=step_source_animation(source,instance,context);
        if (!result) return result;
    } else {
        if (isle_game_pause_source_processes(context.services.game,source.object,isle_actor_source_animation_process,1)!=ISLE_GAME_OK)
            return rejected(OwnerError::native,"actor animation function pause");
        source.animHeader=nullptr; source.matAnims=0; source.modelAnims=0;
    }
    // Setter resets follow immediate events, never precede them.
    source.loopCount=0;
    source.processFlags&=~static_cast<std::uint32_t>(POKEMON_PROCESS_FLAG_ANIMATION_ENDED);
    source.pokemonLoopTarget=1; source.lastAnimationFrame=0.0f;
    return {};
}

struct Owner::Impl {
    struct SavedPath {
        const menu::AnimationPath* value = nullptr;
        bool saved = false; // A saved Source NULL is distinct from not saved.
    };
    struct RetainedFields {
        std::vector<IsleGameObject*> objects;
        std::vector<SavedPath> paths;
        std::vector<IsleGameObject**> source_fields;
        std::uint32_t object_mask = 0, path_mask = 0;
    };
    struct Slot {
        Impl* owner = nullptr;
        std::size_t index = 0;
        IsleActorSource source{};
        menu::animation::InstanceHandle instance;
        IsleGameObject* object = nullptr;
        IsleGameModel* model = nullptr;
        const actor::ActorInitialization* initialization = nullptr;
        const actor::ActorDefinition* definition = nullptr;
        const actor::AutomaticActorDefinition* automatic_definition = nullptr;
        std::uint32_t spawn_helper = 0, spawn_written_fields = 0;
        const AutomaticSpawnRepair* spawn_repair = nullptr;
        scene::RenderMetadata render{};
        std::uint64_t insertion = 0;
        NativeAnimationBinding animation_callback;
        std::array<IsleGameObject**,3> source_fields{};
        std::unique_ptr<RetainedFields> retained;
        bool active = false, retiring = false, joined = false, cleaned = false, source_owned = false;
        bool logical = false, registered = false;
    };
    struct BlockEntry { Slot* slot = nullptr; int next = -1; };
    scene::Services& services;
    Owner* facade = nullptr;
    const actor::Resources& resources;
    world::World& world;
    environment_sound::Environment& sound;
    focus::Detector& detector;
    std::vector<menu::animation::SlotCapacity> capacities;
    std::vector<Slot> slots;
    std::vector<RetainedFieldFamily> retained_families;
    std::vector<ActorRenderBinding> bindings;
    std::vector<IsleGameModelNodeInit> scratch;
    std::unique_ptr<menu::animation::Runtime> runtime;
    menu::EffectRuntime fx;
    IsleGameObject* particle_object = nullptr;
    IsleGameObject* effect_object = nullptr;
    bool particle_publication_started = false, effect_publication_started = false;
    std::array<Slot*,40> logical{};
    std::size_t logical_count = 0;
    std::array<BlockEntry,100> block_entries{};
    std::vector<int> block_heads;
    std::size_t transform_cache = 0, pokemon_cache = 0;
    std::uint64_t insertion = 0;
    IslePlayerSource& player;
    CourseContext course_context;
    bool sound_enabled = true;
    InteractionServices* interactions = nullptr;
    IsleBeachEndingServices ending_services{};
    IsleBeachEndingSource ending{};
    // Preserve existing standalone course callers; the original lifecycle
    // publishes its actual registry configuration at503D38 before actors exist.
    std::int32_t minimum_pokemon_id = 0x80, maximum_pokemon_id = 0xe4;
    std::int32_t pokemon_link = 3, registry_arg3 = 5;
    // getFreePokemonId's source static survives resets and reconfiguration.
    std::uint32_t pokemon_counter = 0;
    // Synchronous automatic factory transport capability; never retained by a
    // process. Source optional fields are absent unless the immutable per-origin
    // portability policy resolves a complete, separate effective factory input.
    const world::Spawn* transforms_only_spawn=nullptr;
    std::size_t transforms_only_definition=actor_no_index, transforms_only_automatic=actor_no_index;
    std::uint32_t transforms_only_helper=0, transforms_only_written_fields=0;
    std::int32_t transforms_only_requested_id=-1;
    const AutomaticSpawnRepair* transforms_only_repair=nullptr;
    struct FactoryTransport {
        const IsleActorFactoryBlock* block;
        const IsleActorFactoryBlock* reference;
        const IsleActorFactorySpawn* spawn;
        const SpawnRequest* request;
        SpawnResult* result;
        const FactoryTransport* previous;
    };
    const FactoryTransport* factory_transport=nullptr;
    struct GeoTransport {
        const IsleActorGeoDefinition* definition;
        const IsleActorSource* parent;
        std::uint32_t requested, origin;
        std::size_t resource_definition, automatic_definition;
        SpawnResult* result;
        const GeoTransport* previous;
    };
    const GeoTransport* geo_transport=nullptr;
    const world::Spawn* initialized_automatic_spawn=nullptr;
    std::size_t initialized_automatic_index=actor_no_index;
    std::int32_t initialized_requested_id=-1;
    // Cold proof over immutable copied capacities and admitted resource shapes;
    // never records occupancy, Source state or future child reservations.
    bool course_capacity_shape=false;

    Impl(scene::Services& s,const actor::Resources& r,CourseContext state,world::World& w,environment_sound::Environment& e,
         focus::Detector& d,IslePlayerSource& p,std::span<const menu::animation::SlotCapacity> c,std::span<const menu::EffectBank* const> banks,
         menu::animation::ExecutionCapacity execution,RetainedFieldOptions retained)
        : services(s),resources(r),world(w),sound(e),detector(d),capacities(c.begin(),c.end()),slots(c.size()),
          retained_families(retained.families.begin(),retained.families.end()),bindings(c.size()),
          fx(banks,s.rng,{this,read_target,write_target}),block_heads(w.blocks().size(),-1),player(p),course_context(state) {
        resources.require_live();
        course_context.require_live();
        const auto setup_identity=detail::course_catalog(course_context.course_id()).world_setup_identity;
        if (resources.course_id()!=course_context.course_id() || !w.resources().setup ||
            w.resources().setup->wire.guest_address!=setup_identity)
            throw std::invalid_argument("actor owner requires matching typed course resources and actual World setup");
        if (course_context.course_id()==6 && course_context.rainbow().actors.nativeGame &&
            course_context.rainbow().actors.nativeGame!=s.game)
            throw std::invalid_argument("Rainbow actor state is bound to another native Game");
        IslePlayerSource* active=nullptr;
        if (p.game!=s.game || isle_player_source_context(s.game,&active)!=ISLE_GAME_OK || active!=&p)
            throw std::invalid_argument("actor owner requires active same-game canonical Player Source");
        if (c.empty() || !fx.status()) throw std::invalid_argument("actor owner needs native capacities and admitted shared FX banks");
        if (!retained_families.empty() && (state.course_id()==0 || state.course_id()==6))
            throw std::invalid_argument("retained misc alternatives are not part of course0/6 policy");
        const RetainedFieldFamily expected[] = {
            {81,0x802ee660u,1u,0u}, {1026,0x802e2fd8u,2u,0u},
            {39,0x802c7590u,0u,4u}, {147,0x802d3100u,1u,0u}
        };
        const RetainedFieldFamily* actual=nullptr;
        switch (state.course_id()) {
        case 1: actual=&expected[0]; break;
        case 2: actual=&expected[1]; break;
        case 4: actual=&expected[2]; break;
        case 5: actual=&expected[3]; break;
        }
        if (state.course_id()>=1 && state.course_id()<=5) {
            if (retained_families.empty() && actual) retained_families.push_back(*actual);
            if (retained_families.size()!=(actual ? 1u : 0u) ||
                (actual && (retained_families[0].actor_id!=actual->actor_id ||
                 retained_families[0].initialization_identity!=actual->initialization_identity ||
                 retained_families[0].misc_object_mask!=actual->misc_object_mask ||
                 retained_families[0].saved_path_mask!=actual->saved_path_mask)))
                throw std::invalid_argument("retained Actor alternatives differ from exact family+Init Source policy");
        }
        for (std::size_t i=0;i<retained_families.size();++i) {
            const auto& family=retained_families[i];
            if (!(family.misc_object_mask|family.saved_path_mask) ||
                ((family.misc_object_mask|family.saved_path_mask)&~((1u<<retained_misc_count)-1u)) ||
                (family.misc_object_mask&family.saved_path_mask))
                throw std::invalid_argument("retained Actor alternatives need distinct actual misc indexes");
            bool found=false;
            for (const auto& init:r.initializations())
                if (init.actor_id==family.actor_id && init.wire.guest_address==family.initialization_identity)
                    found=true;
            if (!found) throw std::invalid_argument("retained Actor family is not an actual resource initialization");
            for (std::size_t j=0;j<i;++j)
                if (retained_families[j].actor_id==family.actor_id &&
                    retained_families[j].initialization_identity==family.initialization_identity)
                    throw std::invalid_argument("retained Actor alternatives repeat a family");
        }
        std::size_t maximum_nodes=0;
        for (std::size_t i=0;i<c.size();++i) {
            slots[i].owner=this; slots[i].index=i;
            auto& slot=slots[i];
            slot.source_fields={&slot.source.apple,&slot.source.interactionTarget,&ending.D_beach_80347658};
            std::size_t objects=0,paths=0;
            for (const auto& family:retained_families) {
                const auto found=std::find_if(r.initializations().begin(),r.initializations().end(),
                    [&](const auto& init) { return init.actor_id==family.actor_id &&
                        init.wire.guest_address==family.initialization_identity; });
                if (found->model>=r.models().size())
                    throw std::invalid_argument("retained Actor initialization has no model");
                const auto required=menu::animation::slot_capacity(r.models()[found->model]);
                if (c[i].nodes<required.nodes || c[i].material_attachments<required.material_attachments) continue;
                objects=std::max(objects,static_cast<std::size_t>(std::popcount(family.misc_object_mask)));
                paths=std::max(paths,static_cast<std::size_t>(std::popcount(family.saved_path_mask)));
            }
            if (objects || paths) {
                slot.retained=std::make_unique<RetainedFields>();
                auto& fields=*slot.retained;
                fields.objects.resize(objects);
                fields.paths.resize(paths);
                if (objects) {
                    fields.source_fields.resize(2+objects);
                    fields.source_fields[0]=&slot.source.apple;
                    fields.source_fields[1]=&slot.source.interactionTarget;
                    for (std::size_t j=0;j<objects;++j) fields.source_fields[2+j]=&fields.objects[j];
                }
            }
            maximum_nodes=std::max(maximum_nodes,c[i].nodes);
        }
        scratch.resize(maximum_nodes);
        auto result=menu::animation::Runtime::create({r.models(),r.materials(),r.animations(),menu::no_index,r.paths(),r.material_playbacks()},c,{},execution);
        if (!result) throw std::runtime_error("actor animation Runtime admission failed");
        runtime=std::move(result.runtime);
    }
    Slot* find(menu::animation::InstanceHandle handle,bool retiring=false) noexcept {
        if (!handle || handle.slot>=slots.size()) return nullptr;
        auto& slot=slots[handle.slot];
        return slot.active && slot.instance==handle && (retiring || !slot.retiring) ? &slot : nullptr;
    }
    const Slot* find(menu::animation::InstanceHandle handle,bool retiring=false) const noexcept {
        return const_cast<Impl*>(this)->find(handle,retiring);
    }
    OwnerStatus publish_fx(bool& started,std::uint32_t id,IsleGameCallback update,IsleGameObject*& output) noexcept {
        if (services.failed())
            return rejected(OwnerError::services,"actor FX publication requires live Services");
        if (started)
            return rejected(OwnerError::lifetime,"actor FX helper already published or publication attempted");
        started=true;
        auto* object=services.object(id);
        if (!object) return rejected(OwnerError::services,"actor FX native allocation failed");
        if (!services.set_lifecycle(object,{nullptr,fx_joined,this})) {
            services.erase(object);
            return rejected(OwnerError::services,"actor FX join observer installation failed");
        }
        output=object;
        if (services.set_update(object,update,this)) return {};
        services.erase(object);
        return rejected(OwnerError::services,"actor FX native update publication failed");
    }
    static void fx_joined(IsleGameObject* object,void* user) noexcept {
        auto& self=*static_cast<Impl*>(user);
        if (self.particle_object==object) self.particle_object=nullptr;
        if (self.effect_object==object) self.effect_object=nullptr;
    }
    static void particles_tick(IsleGameObject* object,void* user) noexcept {
        auto& self=*static_cast<Impl*>(user);
        IsleGameObjectMetadata metadata{};
        if (isle_game_object_metadata(self.services.game,object,&metadata)!=ISLE_GAME_OK) {
            self.services.fail("actor particle owner flags");
            return;
        }
        self.fx.pause_lists(static_cast<std::uint16_t>(metadata.flags>>16));
        if (!self.fx.update_particles()) self.services.fail("actor shared particle update");
    }
    static void effects_tick(IsleGameObject* object,void* user) noexcept {
        auto& self=*static_cast<Impl*>(user);
        IsleGameObjectMetadata metadata{};
        if (isle_game_object_metadata(self.services.game,object,&metadata)!=ISLE_GAME_OK) {
            self.services.fail("actor effect owner flags");
            return;
        }
        self.fx.pause_lists(static_cast<std::uint16_t>(metadata.flags>>16));
        if (!self.fx.update_effects(resolve,&self)) self.services.fail("actor shared effect update");
    }
    static bool resolve(void* user,menu::EffectAttachment attachment,menu::EffectWorld& output) noexcept {
        auto& self=*static_cast<Impl*>(user);
        auto* slot=self.find(attachment.instance);
        return slot && bool(menu::model::source_effect_world(self.services.game,slot->model,attachment.node,output));
    }
    static bool read_target(void* user,menu::EffectAttachment attachment,menu::Vec3& output) noexcept {
        auto& self=*static_cast<Impl*>(user);
        auto* slot=self.find(attachment.instance);
        const IsleGameModelNode* nodes=nullptr; std::size_t count=0;
        if (!slot || isle_game_model_peek_nodes(self.services.game,slot->model,&nodes,&count)!=ISLE_GAME_OK || attachment.node>=count) return false;
        const auto& p=nodes[attachment.node].transform.position;
        output={p.x,p.y,p.z}; return true;
    }
    static bool write_target(void* user,menu::EffectAttachment attachment,menu::Vec3 position) noexcept {
        auto& self=*static_cast<Impl*>(user);
        auto* slot=self.find(attachment.instance);
        if (!slot) return false;
        auto state=self.runtime->model(slot->instance);
        if (!state || attachment.node>=state.nodes.size()) return false;
        auto pose=state.nodes[attachment.node].transform; pose.position=position;
        return bool(self.local(*slot,attachment.node,pose));
    }
    OwnerStatus local(Slot& slot,std::size_t node,const menu::animation::Transform& pose) noexcept {
        const auto transform=native(pose);
        if (isle_game_model_validate_transform(services.game,slot.model,node,&transform)!=ISLE_GAME_OK)
            return rejected(OwnerError::native,"actor local transform preflight");
        if (!runtime->set_node_transform(slot.instance,node,pose)) return rejected(OwnerError::animation,"actor authoritative local transform");
        if (isle_game_model_set_transform(services.game,slot.model,node,&transform)!=ISLE_GAME_OK)
            return rejected(OwnerError::native,"actor accepted native local transform");
        return {};
    }
    EventServices event_services(Slot& slot) noexcept {
        return {services,sound,fx,*runtime,slot.model,sound_enabled,particle_object,effect_object,&slot.animation_callback};
    }
    OwnerStatus step(Slot& slot) noexcept {
        return step_source_animation(slot.source,slot.instance,event_services(slot));
    }
    OwnerStatus animation(Slot& slot,const actor::ActorAnimationHeader* header,float start,bool force) noexcept {
        return apply_source_animation(slot.source,slot.instance,header,start,force,*slot.initialization,resources,event_services(slot));
    }
    void unlink_logical(Slot& slot) noexcept {
        if (!slot.logical) return;
        for (std::size_t i=0;i<logical_count;++i) if (logical[i]==&slot) {
            for (std::size_t j=i+1;j<logical_count;++j) logical[j-1]=logical[j];
            logical[--logical_count]=nullptr; slot.logical=false; return;
        }
        services.fail("actor source logical membership corrupted");
    }
    int registered_entry(Slot& slot) noexcept {
        auto* block=static_cast<world::Block*>(slot.source.baseBlock);
        if (!block || block->index>=block_heads.size()) return -1;
        for (int i=block_heads[block->index];i!=-1;i=block_entries[static_cast<std::size_t>(i)].next)
            if (block_entries[static_cast<std::size_t>(i)].slot==&slot) return i;
        return -1;
    }
    void unlink_block(Slot& slot) noexcept {
        auto* block=static_cast<world::Block*>(slot.source.baseBlock);
        if (!slot.registered || !block || block->index>=block_heads.size()) return;
        int* link=&block_heads[block->index];
        while (*link!=-1) {
            auto& entry=block_entries[static_cast<std::size_t>(*link)];
            if (entry.slot==&slot) { *link=entry.next; entry={}; slot.registered=false; return; }
            link=&entry.next;
        }
    }
    void focus_cleanup(Slot& slot) noexcept {
        auto* const previous_focus=player.gPreviousPokemonInFocus;
        isle_player_func_80357428_4F7838(&player,slot.object);
        if (services.source_owner_registered(player.object) &&
            !services.source_reference_reconcile(player.object,&player.gPreviousPokemonInFocus,previous_focus))
            services.fail("actor Player focus cleanup reference reconciliation");
    }
    void invalidate(Slot& slot) noexcept {
        detector.cleanup(reinterpret_cast<focus::Object>(slot.object));
        focus_cleanup(slot);
        fx.detach_instance(slot.instance); fx.clear_targets(slot.instance);
        sound.fix_parameters(slot.object);
    }
    static void retiring_callback(IsleGameObject*,void* user) noexcept {
        auto& slot=*static_cast<Slot*>(user); auto& self=*slot.owner;
        if (slot.retiring) return;
        slot.animation_callback=std::monostate{};
        self.invalidate(slot);
        self.unlink_logical(slot);
        slot.retiring=true;
        if (slot.retained)
            for (auto& path:slot.retained->paths) path={};
        if (!self.runtime->begin_retire(slot.instance)) self.services.fail("actor Runtime retirement");
        // Hooks only invalidate borrowers/record facts. No OM calls or source
        // cleanup recursion, waits, throwing or physical release here.
    }
    static void joined_callback(IsleGameObject*,void* user) noexcept {
        auto& slot=*static_cast<Slot*>(user);
        slot.joined=true; slot.model=nullptr;
        slot.owner->unlink_block(slot);
        // Sound no longer borrows the joined model. The native identity itself
        // remains held until outside-OM collection also closes command credits.
    }
    OwnerStatus notify_target_death(IsleGameObject* sender) noexcept {
        // Source logical Pokemon list, deliberately not all native link3.
        // Sender may be an Item; identity comparison never reads its payload.
        for (std::size_t i=0;i<logical_count;++i) {
            auto& target=*logical[i];
            if (target.source.interactionTarget==sender &&
                !services.send_command(target.object,22,sender))
                return rejected(OwnerError::services,"actor die-command enqueue");
        }
        return {};
    }
    OwnerStatus cleanup(Slot& slot) noexcept {
        if (slot.cleaned || slot.retiring) return rejected(OwnerError::lifetime,"actor source cleanup already requested");
        // Actual deletePokemon must find this object in its source base-block list.
        if (!slot.logical || !slot.registered || registered_entry(slot)<0)
            return rejected(OwnerError::admission,"actor cleanup requires actual logical and registered base-block membership");
        IsleGameObjectMetadata metadata{};
        if (isle_game_object_metadata(services.game,slot.object,&metadata)!=ISLE_GAME_OK ||
            isle_game_object_set_flags(services.game,slot.object,metadata.flags|1u)!=ISLE_GAME_OK)
            return rejected(OwnerError::native,"actor cleanup hide");
        detector.cleanup(reinterpret_cast<focus::Object>(slot.object));
        focus_cleanup(slot);
        const auto notified=notify_target_death(slot.object);
        if (!notified) return notified;
        // Source cache metadata retains at most40 freed values, independently
        // of the40 logical entries,100 block entries and caller physical slots.
        transform_cache=std::min<std::size_t>(40,transform_cache+1);
        pokemon_cache=std::min<std::size_t>(40,pokemon_cache+1);
        unlink_logical(slot);
        fx.detach_instance(slot.instance); fx.clear_targets(slot.instance);
        sound.fix_parameters(slot.object);
        slot.cleaned=true;
        services.erase(slot.object); // Real native delete/join; may defer self-delete.
        unlink_block(slot); // Source deletion unlinks registry AFTER nativeDelete.
        return services.failed()?rejected(OwnerError::services,"actor native registered deletion"):OwnerStatus{};
    }
    OwnerStatus distance(Slot& slot) noexcept {
        IslePlayerSource* active=nullptr;
        if (isle_player_source_context(services.game,&active)!=ISLE_GAME_OK || active!=&player ||
            !player.object || !player.model ||
            isle_game_model_validate_binding(services.game,player.object,player.model)!=ISLE_GAME_OK)
            return rejected(OwnerError::native,"actor distance requires active published canonical Player");
        const IsleGameModelPlacement *actor_position=nullptr,*player_position=nullptr;
        if (isle_game_model_placement(services.game,slot.model,&actor_position)!=ISLE_GAME_OK ||
            isle_game_model_placement(services.game,player.model,&player_position)!=ISLE_GAME_OK ||
            player_position->mode==ISLE_GAME_MODEL_PLACEMENT_DISABLED)
            return rejected(OwnerError::native,"actor/player source placement distance");
        const float dx=actor_position->transform.position.x-player_position->transform.position.x;
        const float dy=actor_position->transform.position.y-player_position->transform.position.y;
        const float dz=actor_position->transform.position.z-player_position->transform.position.z;
        slot.source.playerDist=std::sqrt(dx*dx+dy*dy+dz*dz);
        return {};
    }
};

struct SourceAccess {
    static Owner* canonical_owner(IsleGame* game, bool allow_fault=false) noexcept {
        IslePlayerSource* player=nullptr;
        if (isle_actor_owner_associated_player(game,&player)!=ISLE_GAME_OK || !player ||
            player->game!=game || !player->owner || !player->rng || !player->native_root)
            return nullptr;
        if (!allow_fault && (isle_actor_source_execution_status(game)!=ISLE_GAME_OK ||
            player->status!=ISLE_GAME_OK)) return nullptr;
        auto& root=*static_cast<isle::player::Root*>(player->native_root);
        auto* owner=root.bound_actors();
        return &root.source()==player && owner && owner->impl_ &&
            &owner->player_source()==player && owner->impl_->services.game==game &&
            owner->impl_->facade==owner ? owner : nullptr;
    }
    static tunnel::CourseState& state_for(IsleTunnelActorSource*, const CourseContext& context) { return context.tunnel(); }
    static volcano::CourseState& state_for(IsleVolcanoActorSource*, const CourseContext& context) { return context.volcano(); }
    static river::CourseState& state_for(IsleRiverActorSource*, const CourseContext& context) { return context.river(); }
    static cave::CourseState& state_for(IsleCaveActorSource*, const CourseContext& context) { return context.cave(); }
    static valley::CourseState& state_for(IsleValleyActorSource*, const CourseContext& context) { return context.valley(); }
    template<class Source>
    static IsleGameResult typed_context(IsleGame* game, IsleGameObject* object, Source** output,
        Owner** admitted=nullptr, bool explicit_nonobject=false) noexcept {
        if (output) *output=nullptr;
        if (admitted) *admitted=nullptr;
        if (!game || !output) return ISLE_GAME_INVALID_ARGUMENT;
        auto* owner=canonical_owner(game);
        if (!owner) return ISLE_GAME_INVALID_STATE;
        try {
            auto& self=*owner->impl_;
            auto& state=state_for(static_cast<Source*>(nullptr),self.course_context);
            auto& source=state.actors;
            state.require_live();
            if (self.services.failed() || !self.world.error().empty() ||
                self.resources.course_id()!=self.course_context.course_id() ||
                source.base.course_id!=self.course_context.course_id() ||
                source.base.phase!=ISLE_COURSE_ACTOR_ACTIVE || source.base.nativeGame!=game ||
                source.services!=&state.actor_services || !native_services_complete(source.services) ||
                !state.native_host || state.native_host->game()!=game ||
                state.native_host->actors()!=owner || state.native_host->root()!=
                    static_cast<isle::player::Root*>(self.player.native_root) ||
                !self.world.resources().setup || self.world.resources().setup->wire.guest_address!=
                    detail::course_catalog(self.resources.course_id()).world_setup_identity)
                return ISLE_GAME_INVALID_STATE;
            if (object) {
                IsleGameObjectMetadata metadata{};
                if (isle_game_object_metadata(game,object,&metadata)!=ISLE_GAME_OK ||
                    !self.services.source_owner_registered(object) ||
                    self.services.source_owner_joined(object)) return ISLE_GAME_INVALID_STATE;
            } else if (!explicit_nonobject || (!source.base.nonwaiting_call_frame &&
                !state.source.nonwaiting_call_frame && !self.factory_transport)) {
                return ISLE_GAME_INVALID_STATE;
            }
            self.resources.require_live();
            if constexpr (std::is_same_v<Source,IsleTunnelActorSource>) {
                // Only Tunnel's actual Lifecycle Source declares this borrow.
                // The other four exact records use canonical Owner/Root above.
                if (state.source.player && state.source.player!=&self.player)
                    return ISLE_GAME_INVALID_STATE;
            }
            *output=&source;
            if (admitted) *admitted=owner;
            return ISLE_GAME_OK;
        } catch (...) { return ISLE_GAME_INVALID_STATE; }
    }
    template<class Source, class Services>
    static IsleGameResult typed_admission(const Source* source, IsleGame* game, const Services* services) noexcept {
        if (!source || !game || !services) return ISLE_GAME_INVALID_ARGUMENT;
        auto* owner=canonical_owner(game);
        if (!owner) return ISLE_GAME_INVALID_STATE;
        try {
            auto& self=*owner->impl_;
            auto& state=state_for(static_cast<Source*>(nullptr),self.course_context);
            state.require_live();
            if (&state.actors!=source || services!=&state.actor_services ||
                !native_services_complete(services) || !state.native_host ||
                state.native_host->game()!=game || state.native_host->actors()!=owner ||
                state.native_host->root()!=static_cast<isle::player::Root*>(self.player.native_root) ||
                !state.native_host->closure_prepared() || self.services.failed() ||
                !self.world.error().empty() || !self.course_capacity_shape ||
                !self.interactions || !self.interactions->matches(*owner,self.services,
                    self.resources,self.world,self.detector)) return ISLE_GAME_INVALID_STATE;
            if (source->base.phase==ISLE_COURSE_ACTOR_PREPARED) {
                if (self.world.retired() || source->base.nativeGame || source->services || source->base.nonwaiting_call_frame ||
                    isle_game_require_idle(game)!=ISLE_GAME_OK ||
                    !self.interactions->admits_proposed_source()) return ISLE_GAME_INVALID_STATE;
            } else if (source->base.phase!=ISLE_COURSE_ACTOR_ACTIVE ||
                source->base.nativeGame!=game || source->services!=services ||
                !self.interactions->admits_source()) return ISLE_GAME_INVALID_STATE;
            return ISLE_GAME_OK;
        } catch (...) { return ISLE_GAME_INVALID_STATE; }
    }
    template<class Source>
    static IsleGameResult typed_retirement(Source* source) noexcept {
        if (!source || !source->base.nativeGame) return ISLE_GAME_INVALID_ARGUMENT;
        // Faulted Game/Player/Source still retain a legitimate teardown borrow.
        auto* owner=canonical_owner(source->base.nativeGame,true);
        if (!owner) return ISLE_GAME_INVALID_STATE;
        try {
            auto& self=*owner->impl_;
            auto& state=state_for(static_cast<Source*>(nullptr),self.course_context);
            if (&state.actors!=source || source->base.phase!=ISLE_COURSE_ACTOR_ACTIVE ||
                source->base.course_id!=self.course_context.course_id() ||
                !source->base.owner || state.source.owner!=source->base.owner ||
                state.source.actors!=source || !state.source.domain ||
                source->services!=&state.actor_services || !state.native_host ||
                state.native_host->game()!=self.services.game || state.native_host->actors()!=owner ||
                state.native_host->root()!=static_cast<isle::player::Root*>(self.player.native_root) ||
                !state.native_host->detach_ready() || source->base.nonwaiting_call_frame ||
                self.factory_transport) return ISLE_GAME_INVALID_STATE;
            for (const auto& slot:self.slots) if (slot.active) return ISLE_GAME_INVALID_STATE;
            return ISLE_GAME_OK;
        } catch (...) { return ISLE_GAME_INVALID_STATE; }
    }
    template<class Source>
    static IsleGameResult typed_factory_context(const IsleActorFactoryBlock* block,
        const IsleActorFactoryBlock* reference,const IsleActorFactorySpawn* spawn,Source** output) noexcept {
        if (output) *output=nullptr;
        if (!output) return ISLE_GAME_INVALID_ARGUMENT;
        auto* owner=factory_owner(block,reference,spawn);
        if (!owner) return ISLE_GAME_INVALID_STATE;
        Source* canonical=nullptr;
        Owner* actual=nullptr;
        const auto result=typed_context(owner->impl_->services.game,nullptr,&canonical,&actual,true);
        if (result!=ISLE_GAME_OK) return result;
        if (actual!=owner || typed_admission(canonical,owner->impl_->services.game,
            canonical->services)!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE;
        *output=canonical;
        return ISLE_GAME_OK;
    }
    static IsleGameResult course_admission(Owner& owner) noexcept {
        auto& self=*owner.impl_;
        switch (self.course_context.course_id()) {
        case 0: return ISLE_GAME_OK;
        case 1: { auto& s=self.course_context.tunnel().actors;
            return typed_admission(&s,self.services.game,s.services); }
        case 2: { auto& s=self.course_context.volcano().actors;
            return typed_admission(&s,self.services.game,s.services); }
        case 3: { auto& s=self.course_context.river().actors;
            return typed_admission(&s,self.services.game,s.services); }
        case 4: { auto& s=self.course_context.cave().actors;
            return typed_admission(&s,self.services.game,s.services); }
        case 5: { auto& s=self.course_context.valley().actors;
            return typed_admission(&s,self.services.game,s.services); }
        case 6: return rainbow_admission(&self.course_context.rainbow().actors);
        }
        return ISLE_GAME_INVALID_STATE;
    }

    static IsleGameResult rainbow_context(IsleGame* game,IsleRainbowActorSource** output,
        Owner** admitted_owner=nullptr) noexcept {
        if (output) *output=nullptr;
        if (admitted_owner) *admitted_owner=nullptr;
        if (!game || !output) return ISLE_GAME_INVALID_ARGUMENT;
        const auto status=isle_actor_source_execution_status(game);
        if (status!=ISLE_GAME_OK) return status;
        IslePlayerSource* player=nullptr;
        const auto read=isle_player_source_context(game,&player);
        if (read!=ISLE_GAME_OK) return read;
        if (!player || player->game!=game || player->status!=ISLE_GAME_OK ||
            !player->owner || !player->native_root) return ISLE_GAME_INVALID_STATE;
        auto& root=*static_cast<isle::player::Root*>(player->native_root);
        if (&root.source()!=player) return ISLE_GAME_INVALID_STATE;
        auto* owner=root.bound_actors();
        if (!owner || &owner->player_source()!=player) return ISLE_GAME_INVALID_STATE;
        try {
            auto& self=*owner->impl_;
            // Original destroyWorld marks logical retirement before invoking
            // removal/cleanup callbacks. Their retained canonical association
            // remains valid until detach; new-work entry points close separately.
            if (self.services.game!=game || self.services.failed() ||
                !self.world.error().empty() ||
                self.course_context.course_id()!=6 || self.resources.course_id()!=6 ||
                !self.world.resources().setup ||
                self.world.resources().setup->wire.guest_address!=0x800f5da0u)
                return ISLE_GAME_INVALID_STATE;
            self.resources.require_live();
            auto& state=self.course_context.rainbow();
            auto& source=state.actors;
            const auto& lifecycle=state.source;
            if (!source.owner || lifecycle.owner!=source.owner || lifecycle.actors!=&source ||
                !lifecycle.domain || lifecycle.status!=ISLE_GAME_OK ||
                (lifecycle.player && lifecycle.player!=player) || source.status!=ISLE_GAME_OK ||
                source.nativeGame!=game || !source.services ||
                (source.phase!=ISLE_RAINBOW_ACTOR_ACTIVE && source.phase!=ISLE_RAINBOW_ACTOR_PREPARED))
                return ISLE_GAME_INVALID_STATE;
            if (source.phase==ISLE_RAINBOW_ACTOR_PREPARED) {
                // Only activate's provisional nonexecuting lookup is allowed.
                // require_live intentionally rejects these provisional fields.
                if (source.nonwaiting_call_frame || isle_game_require_idle(game)!=ISLE_GAME_OK)
                    return ISLE_GAME_INVALID_STATE;
            } else state.require_live();
            if (self.interactions &&
                !self.interactions->matches(*owner,self.services,self.resources,self.world,self.detector))
                return ISLE_GAME_INVALID_STATE;
            *output=&source;
            if (admitted_owner) *admitted_owner=owner;
            return ISLE_GAME_OK;
        } catch (...) { return ISLE_GAME_INVALID_STATE; }
    }
    static IsleGameResult rainbow_admission(IsleRainbowActorSource* source) noexcept {
        if (!source) return ISLE_GAME_INVALID_ARGUMENT;
        if (source->phase!=ISLE_RAINBOW_ACTOR_ACTIVE || !source->nativeGame)
            return ISLE_GAME_INVALID_STATE;
        IsleRainbowActorSource* canonical=nullptr;
        Owner* owner=nullptr;
        const auto context=rainbow_context(source->nativeGame,&canonical,&owner);
        if (context!=ISLE_GAME_OK) return context;
        if (canonical!=source || source->phase!=ISLE_RAINBOW_ACTOR_ACTIVE)
            return ISLE_GAME_INVALID_STATE;
        if (!owner) return ISLE_GAME_INVALID_STATE;
        auto& self=*owner->impl_;
        if (!self.course_capacity_shape || !self.interactions || !self.interactions->admits_source())
            return ISLE_GAME_INVALID_STATE;
        return ISLE_GAME_OK;
    }
    static Owner::Impl::Slot* ending_slot(IsleActorSource* source) noexcept {
        auto* result=slot(source);
        if (!result || source->id!=1001 || !result->owner->ending_services.context ||
            source->initData!=result->initialization ||
            result->initialization->wire.guest_address!=0x802cda10u) return nullptr;
        auto& self=*result->owner;
        return self.ending.player==&self.player &&
            self.ending_services.validate(self.ending_services.context,&self.ending)==ISLE_GAME_OK ? result : nullptr;
    }
    static Owner::Impl::Slot* slot(IsleActorSource* source) noexcept {
        if (!source || !source->owner || !source->game || !source->object) return nullptr;
        IsleActorSource* bound=nullptr;
        if (isle_actor_source_get(source->game,source->object,&bound)!=ISLE_GAME_OK || bound!=source) return nullptr;
        auto* slot=static_cast<Owner::Impl::Slot*>(source->owner);
        if (&slot->source!=source || !slot->active || slot->retiring || slot->object!=source->object ||
            slot->owner->services.game!=source->game ||
            isle_game_model_validate_binding(source->game,slot->object,slot->model)!=ISLE_GAME_OK) return nullptr;
        if (slot->owner->course_context.course_id()!=0 &&
            course_admission(*slot->owner->facade)!=ISLE_GAME_OK) return nullptr;
        return slot;
    }
    static Owner::Impl::Slot* retained_scope_slot(IsleActorSource* source) noexcept {
        // Only a begun command scope may use this stable semantic record after
        // native retirement. Source-owner collection cannot run inside it.
        if (!source || !source->owner || !source->game || !source->object) return nullptr;
        auto* slot=static_cast<Owner::Impl::Slot*>(source->owner);
        return &slot->source==source && slot->active && slot->object==source->object &&
            slot->owner->services.game==source->game?slot:nullptr;
    }
    static SpawnResult spawn(Owner& owner,const SpawnRequest& request,uint16_t id,uint32_t initialization) noexcept {
        const auto& resources=owner.impl_->resources;
        if (request.definition>=resources.definitions().size())
            return {{OwnerError::admission,"typed source factory definition outside resources"},{},nullptr};
        const auto& definition=resources.definitions()[request.definition];
        if (definition.actor_id!=id || definition.initialization>=resources.initializations().size() ||
            resources.initializations()[definition.initialization].wire.guest_address!=initialization)
            return {{OwnerError::admission,"typed source factory ID/initialization differs from definition"},{},nullptr};
        return owner.spawn_native(request);
    }
    static SpawnResult spawn_ordinary(Owner& owner,const SpawnRequest& request,uint16_t id,uint32_t initialization) noexcept {
        auto result=spawn(owner,request,id,initialization);
        if (!result) return result;
        auto& self=*owner.impl_;
        auto* slot=self.find(result.instance);
        const IsleGameModelPlacement* current=nullptr;
        if (!slot || isle_game_model_placement(self.services.game,slot->model,&current)!=ISLE_GAME_OK ||
            !current || current->mode!=ISLE_GAME_MODEL_PLACEMENT_RTS) {
            self.services.erase(result.object);
            return {{OwnerError::native,"ordinary source factory late AUX placement read"},{},nullptr};
        }
        // pokemon.c2113-2117: complete Ground (including immediate animation
        // events, processes and canonical source caches), then ONLY AUX Y.
        auto placement=current->transform;
        placement.position.y=request.spawn->position.y*100.0f+
            (request.block->resource->metadata.world_position.y-
             request.reference->resource->metadata.world_position.y)*100.0f;
        if (isle_game_model_set_placement(self.services.game,slot->model,current->mode,&placement)!=ISLE_GAME_OK) {
            self.services.erase(result.object);
            return {{OwnerError::native,"ordinary source factory late AUX Y store"},{},nullptr};
        }
        return result;
    }
    static const AutomaticSpawnRepair* portability_repair(const Owner::Impl& self,
        const SpawnRequest& request) noexcept {
        if (!request.spawn || request.spawn!=self.transforms_only_spawn ||
            !self.transforms_only_repair || request.definition!=self.transforms_only_definition ||
            request.definition>=self.resources.definitions().size() ||
            request.automatic_definition!=self.transforms_only_automatic ||
            request.automatic_definition>=self.resources.automatic_definitions().size()) return nullptr;
        const auto& definition=self.resources.definitions()[request.definition];
        const auto& automatic=self.resources.automatic_definitions()[request.automatic_definition];
        const auto* policy=automatic_spawn_repair(self.course_context.course_id(),
            automatic.origin.function_identity,automatic.origin.line,self.transforms_only_written_fields,
            definition.actor_id,definition.factory_identity);
        if (!policy || policy!=self.transforms_only_repair ||
            self.transforms_only_helper!=policy->helper || automatic.actor_id!=definition.actor_id ||
            automatic.factory_identity!=definition.factory_identity ||
            automatic.initialization!=definition.initialization ||
            automatic.change_block_identity!=definition.change_block_identity ||
            automatic.remove_identity!=definition.remove_identity ||
            request.spawn->behavior!=policy->behavior ||
            (policy->path==AutomaticPathRepair::semantic_null && request.spawn->path!=world::no_index))
            return nullptr;
        return policy;
    }
    static Owner* factory_owner(const IsleActorFactoryBlock* block,
        const IsleActorFactoryBlock* reference,const IsleActorFactorySpawn* spawn) noexcept {
        if (!block || !reference || !spawn || !block->owner) return nullptr;
        auto* owner=static_cast<Owner*>(block->owner);
        if (!owner->impl_) return nullptr;
        auto& self=*owner->impl_;
        const auto* ticket=self.factory_transport;
        if (!ticket || ticket->block!=block || ticket->reference!=reference || ticket->spawn!=spawn ||
            !ticket->request || !ticket->result || reference->owner!=owner ||
            block->result!=ticket->result || reference->result!=ticket->result) return nullptr;
        const auto& request=*ticket->request;
        if (block->block!=request.block || reference->block!=request.reference ||
            spawn->spawn!=request.spawn || spawn->definition!=request.definition ||
            spawn->automatic_definition!=request.automatic_definition || !request.spawn ||
            request.definition>=self.resources.definitions().size() || self.world.retired() ||
            !self.world.error().empty() || self.facade!=owner || !self.services.game ||
            self.player.game!=self.services.game) return nullptr;
        bool block_found=false,reference_found=false;
        for (const auto& candidate:self.world.blocks()) {
            block_found|=&candidate==request.block && candidate.resource;
            reference_found|=&candidate==request.reference && candidate.resource;
        }
        if (!block_found || !reference_found) return nullptr;
        const auto& definition=self.resources.definitions()[request.definition];
        if (!initialized_definition(self.resources,definition)) return nullptr;
        const auto spec=compiled_factory(self.resources.course_id(),definition.actor_id,definition.factory_identity);
        if (spec.wrapper==FactoryWrapper::meowth || spec.wrapper==FactoryWrapper::valley_magikarp) {
            if (spawn->behavior!=&request.spawn->behavior ||
                (request.spawn==self.transforms_only_spawn &&
                 (spec.wrapper!=FactoryWrapper::valley_magikarp || !portability_repair(self,request)))) return nullptr;
        } else if (spawn->behavior) return nullptr;
        return owner;
    }
    static IsleGameResult factory_game(const IsleActorFactoryBlock* block,
        const IsleActorFactoryBlock* reference,const IsleActorFactorySpawn* spawn,IsleGame** output) noexcept {
        if (output) *output=nullptr;
        if (!output) return ISLE_GAME_INVALID_ARGUMENT;
        auto* owner=factory_owner(block,reference,spawn);
        if (!owner) return ISLE_GAME_INVALID_STATE;
        auto& self=*owner->impl_;
        if (!self.player.owner || !self.player.rng || !self.player.native_root ||
            !self.player.nonwaiting_call_frame) return ISLE_GAME_INVALID_STATE;
        auto& root=*static_cast<isle::player::Root*>(self.player.native_root);
        if (&root.source()!=&self.player || root.bound_actors()!=owner) return ISLE_GAME_INVALID_STATE;
        *output=self.services.game;
        return ISLE_GAME_OK;
    }
    static IsleGameObject* factory(std::int32_t object_id,std::uint16_t id,
        IsleActorFactoryBlock* block,IsleActorFactoryBlock* reference,IsleActorFactorySpawn* spawn,
        const IsleActorFactoryInitialization* initialization,FactoryWrapper wrapper) noexcept {
        auto* admitted=factory_owner(block,reference,spawn);
        if (!admitted || !initialization) return nullptr;
        auto& owner=*admitted;
        auto& self=*owner.impl_;
        auto& result=*self.factory_transport->result;
        const auto fail=[&]() {
            result={{OwnerError::admission,"source wrapper differs from issued compiled factory proof"},{},nullptr};
            return static_cast<IsleGameObject*>(nullptr);
        };
        if (spawn->definition>=self.resources.definitions().size()) return fail();
        const auto& definition=self.resources.definitions()[spawn->definition];
        const auto* init=owner.admitted_factory(id,definition.factory_identity);
        if (!init || definition.actor_id!=id || init->wire.guest_address!=initialization->identity ||
            definition.initialization>=self.resources.initializations().size() ||
            &self.resources.initializations()[definition.initialization]!=init) return fail();
        const auto spec=compiled_factory(self.resources.course_id(),id,definition.factory_identity);
        auto expected=spec.wrapper;
        const auto* descriptor=static_cast<const world::Spawn*>(spawn->spawn);
        if (expected==FactoryWrapper::meowth || expected==FactoryWrapper::valley_magikarp) {
            if (!descriptor || spawn->behavior!=&descriptor->behavior ||
                (descriptor==self.transforms_only_spawn &&
                 (expected!=FactoryWrapper::valley_magikarp ||
                  !portability_repair(self,*self.factory_transport->request)))) return fail();
            expected=expected==FactoryWrapper::meowth ?
                (*spawn->behavior==1?FactoryWrapper::ordinary:FactoryWrapper::ground) :
                (*spawn->behavior==3?FactoryWrapper::ordinary:FactoryWrapper::ground);
        } else if (spawn->behavior) return fail();
        if (expected!=wrapper) return fail();
        const SpawnRequest request{static_cast<std::uint32_t>(object_id),spawn->definition,
            static_cast<world::Block*>(block->block),static_cast<world::Block*>(reference->block),
            descriptor,spawn->automatic_definition};
        result=wrapper==FactoryWrapper::ordinary || wrapper==FactoryWrapper::ordinary_dl4 ?
            spawn_ordinary(owner,request,id,initialization->identity):SourceAccess::spawn(owner,request,id,initialization->identity);
        if (result && (wrapper==FactoryWrapper::ground_dl4 || wrapper==FactoryWrapper::ordinary_dl4)) {
            auto* slot=self.find(result.instance);
            if (!slot) return fail();
            slot->render={4,0x80000000u,UINT32_MAX};
            if (!self.services.set_render(slot->object,slot->render)) {
                self.services.erase(slot->object);
                result={{OwnerError::services,"actor source late DL4 move"},{},nullptr};
            }
        }
        return result.object;
    }
    static const menu::AnimationPath* path(Owner::Impl::Slot& slot,const void* value) noexcept {
        if (!value || slot.owner->world.retired() || !slot.owner->world.error().empty()) return nullptr;
        for (const auto& candidate:slot.owner->world.resources().paths)
            if (&candidate==value) return &candidate;
        return nullptr;
    }
    static Owner::Impl::Slot* retained_slot(IsleActorSource* source,std::uint32_t index,bool object) noexcept {
        if (index>=retained_misc_count) return nullptr;
        auto* actual=slot(source);
        if (!actual || !actual->retained || !actual->source_owned || !actual->initialization ||
            !actual->definition || source->initData!=actual->initialization ||
            !actual->owner->services.source_owner_registered(actual->object)) return nullptr;
        const auto& fields=*actual->retained;
        for (const auto& family:actual->owner->retained_families)
            if (family.actor_id==actual->definition->actor_id &&
                family.initialization_identity==actual->initialization->wire.guest_address &&
                family.misc_object_mask==fields.object_mask && family.saved_path_mask==fields.path_mask)
                return ((object?fields.object_mask:fields.path_mask)&(1u<<index))?actual:nullptr;
        return nullptr;
    }
    static std::size_t retained_index(std::uint32_t mask,std::uint32_t index) noexcept {
        return static_cast<std::size_t>(std::popcount(mask&((1u<<index)-1u)));
    }
    static IsleGameResult auxiliary(IsleActorSource* source,const IsleGameModelRef* reference,
        const IsleGameModelPlacement*& output) noexcept {
        if (!reference) return ISLE_GAME_INVALID_ARGUMENT;
        if (!slot(source)) return ISLE_GAME_INVALID_STATE;
        IsleGameModel* model=nullptr;
        auto result=isle_game_model_resolve(source->game,reference,&model);
        if (result!=ISLE_GAME_OK) return result;
        result=isle_game_model_placement(source->game,model,&output);
        if (result!=ISLE_GAME_OK) return result;
        if (!output || (output->mode!=ISLE_GAME_MODEL_PLACEMENT_RT &&
            output->mode!=ISLE_GAME_MODEL_PLACEMENT_RTS)) return ISLE_GAME_INVALID_STATE;
        return ISLE_GAME_OK;
    }
    static IsleGameResult collision_status(OwnerStatus status) noexcept {
        if (status) return ISLE_GAME_OK;
        return status.error==OwnerError::admission?ISLE_GAME_INVALID_ARGUMENT:ISLE_GAME_INVALID_STATE;
    }
};
bool native_services_complete(const IsleTunnelActorServices* table) noexcept {
    return table && table->reference_assign &&
        table->source &&
        table->model &&
        table->model_transform &&
        table->model_store &&
        table->node &&
        table->node_transform &&
        table->node_world &&
        table->misc_object_get &&
        table->misc_object_set &&
        table->current_block &&
        table->add_one &&
        table->add_at_geo &&
        table->weighted &&
        table->header &&
        table->sound_play &&
        table->sound_play_params &&
        table->sound_identity &&
        table->sound_stop &&
        table->sound_stop_all &&
        table->sound_volume &&
        table->sound_global_volume &&
        table->sound_global_reverb &&
        table->bgm_play &&
        table->bgm_stop &&
        table->bgm_volume &&
        table->bgm_volume_smooth &&
        table->bgm_channel_volume &&
        table->bgm_reverb &&
        table->remove_flying_items &&
        table->reset_camera &&
        table->reset_messages &&
        table->remove_pokemon &&
        table->cutscene_start &&
        table->resume_processes &&
        table->pause_processes &&
        table->main_camera &&
        table->camera_speed &&
        table->camera_animation &&
        table->camera_animation_process &&
        table->sky_speed &&
        table->player_model_init &&
        table->player_model_animation &&
        table->player_model_change_animation &&
        table->player_model_speed &&
        table->model_speed &&
        table->animation_callback &&
        table->collision_initialize &&
        table->collision_query &&
        table->collision_callback &&
        table->level_progress &&
        table->environment_sound &&
        table->ground &&
        table->background_music &&
        table->notice &&
        table->player_model &&
        table->player_object &&
        table->random_range &&
        table->path_count &&
        table->path_point &&
        table->path_param &&
        table->block_position &&
        table->camera_look_read &&
        table->camera_look_store &&
        table->command &&
        table->command_link &&
        table->flute_command &&
        table->vector_distance &&
        table->matrix_rts &&
        table->matrix_cat &&
        table->sound_pan_ramp &&
        table->sound_volume_ramp &&
        table->sound_ramp_reset &&
        table->fade_status &&
        table->fade_color &&
        table->fade_duration &&
        table->fade_start &&
        table->progress_get &&
        table->progress_set &&
        table->helper_create &&
        table->helper_delete &&
        table->camera_object &&
        table->spawn_effect &&
        table->sky_default_speed &&
        table->root_position;
}
bool native_services_complete(const IsleVolcanoActorServices* table) noexcept {
    return table && table->misc_object_get &&
        table->misc_object_set &&
        table->reference_assign &&
        table->send_to_link &&
        table->send_addressed &&
        table->model &&
        table->aux_read &&
        table->root_read &&
        table->component_store &&
        table->item &&
        table->current_block &&
        table->block_position &&
        table->add_one &&
        table->add_geo &&
        table->random_range &&
        table->random_float &&
        table->ground &&
        table->player &&
        table->player_model &&
        table->sound_play &&
        table->sound_play_params &&
        table->sound_identity &&
        table->sound_stop &&
        table->sound_volume &&
        table->sound_stop_all &&
        table->bgm_volume &&
        table->bgm_stop &&
        table->bgm_play &&
        table->environment_sound &&
        table->remove_items &&
        table->reset_camera &&
        table->reset_messages &&
        table->cutscene_start &&
        table->main_camera &&
        table->camera_object &&
        table->camera_speed &&
        table->camera_animation &&
        table->camera_process &&
        table->pause_processes &&
        table->sky_speed &&
        table->player_model_init &&
        table->player_animation &&
        table->animation_callback &&
        table->animation_speed &&
        table->change_block_ground &&
        table->gb_pipe_sync &&
        table->gb_cycle &&
        table->gb_render_mode &&
        table->gb_texture_lut &&
        table->render_type_i &&
        table->render_type_d_fogged &&
        table->vector_distance;
}
bool native_services_complete(const IsleRiverActorServices* table) noexcept {
    return table && table->reference_assign &&
        table->model &&
        table->model_read &&
        table->model_store &&
        table->root_position &&
        table->root_store &&
        table->node_descend &&
        table->node_world &&
        table->current_block &&
        table->block_position &&
        table->path_count &&
        table->path_duration &&
        table->path_parameter &&
        table->path_point &&
        table->item &&
        table->player_object &&
        table->player_model &&
        table->rng_range &&
        table->weighted &&
        table->counted_options &&
        table->graph &&
        table->add_one &&
        table->ground &&
        table->sound &&
        table->sound_params &&
        table->sound_active &&
        table->sound_stop &&
        table->bgm_play &&
        table->bgm_volume &&
        table->bgm_volume_smooth &&
        table->sound_volume_smooth &&
        table->sound_global_volume &&
        table->animation_speed &&
        table->model_speed &&
        table->camera &&
        table->camera_read &&
        table->camera_store &&
        table->camera_speed &&
        table->camera_animation &&
        table->camera_process &&
        table->player_model_init &&
        table->player_animation &&
        table->player_change_animation &&
        table->player_animation_speed &&
        table->animation_callback &&
        table->pause_processes &&
        table->resume_processes &&
        table->sky_speed &&
        table->notice &&
        table->progress_read &&
        table->progress_write &&
        table->transition_prepare &&
        table->transition_color &&
        table->transition_fade &&
        table->transition_active &&
        table->create_helper &&
        table->delete_object &&
        table->send_command &&
        table->send_command_link &&
        table->distance &&
        table->sound_stop_all &&
        table->bgm_stop_all &&
        table->remove_flying_items &&
        table->reset_messages &&
        table->audio_cutscene &&
        table->source_process &&
        table->environment_sound &&
        table->definition &&
        table->root_read;
}
bool native_services_complete(const IsleCaveActorServices* table) noexcept {
    return table && table->model &&
        table->model_read &&
        table->model_store &&
        table->model_child_flags &&
        table->model_speed &&
        table->player_model_ref &&
        table->player_object &&
        table->reference &&
        table->current_block &&
        table->block_position &&
        table->add_one &&
        table->add_geo &&
        table->sound &&
        table->sound_params &&
        table->sound_active &&
        table->sound_stop &&
        table->sound_volume &&
        table->sound_stop_all &&
        table->bgm_play &&
        table->bgm_stop &&
        table->bgm_volume &&
        table->bgm_channel_volume &&
        table->bgm_reverb &&
        table->global_reverb &&
        table->background_music &&
        table->texture_speed &&
        table->animation_speed &&
        table->level_progress &&
        table->ground &&
        table->remove_flying_items &&
        table->camera_reset &&
        table->messages_reset &&
        table->cutscene_start &&
        table->process_pause &&
        table->camera_speed &&
        table->camera_animation &&
        table->player_animation &&
        table->animation_callback &&
        table->remove_pokemon &&
        table->environment_sound &&
        table->path_save &&
        table->main_camera &&
        table->player_model_init &&
        table->flute &&
        table->path_restore &&
        table->path_point &&
        table->interpolated_position &&
        table->camera_process &&
        table->sky_speed &&
        table->item_collision_velocity &&
        table->camera_object &&
        table->rand &&
        table->distance &&
        table->distance_vectors &&
        table->weighted &&
        table->path_assign;
}
bool native_services_complete(const IsleValleyActorServices* table) noexcept {
    return table && table->reference_assign &&
        table->model &&
        table->player_model &&
        table->node_path &&
        table->vector &&
        table->vector_read &&
        table->vector_store &&
        table->item &&
        table->item_direction &&
        table->current_block &&
        table->graph &&
        table->graph_copy &&
        table->weighted &&
        table->weighted_copy &&
        table->forbidden_ground &&
        table->add_one &&
        table->add_geo &&
        table->ground &&
        table->node_world &&
        table->level_progress &&
        table->path_position &&
        table->path_start &&
        table->animation_callback &&
        table->model_animation_speed &&
        table->camera &&
        table->camera_speed &&
        table->camera_read &&
        table->camera_component &&
        table->camera_object &&
        table->camera_animation &&
        table->camera_animation_process &&
        table->pause_processes &&
        table->resume_processes &&
        table->player_model_init &&
        table->player_model_animation &&
        table->player_model_change &&
        table->player_model_speed &&
        table->cutscene_start &&
        table->sound &&
        table->sound_params &&
        table->sound_active &&
        table->sound_stop &&
        table->sound_stop_all &&
        table->sound_volume &&
        table->sound_global_volume &&
        table->sound_volume_smooth &&
        table->bgm_play &&
        table->bgm_stop &&
        table->bgm_volume &&
        table->bgm_volume_smooth &&
        table->background_music &&
        table->check_player_flag &&
        table->set_player_flag &&
        table->remove_flying_items &&
        table->reset_camera &&
        table->sky_speed &&
        table->message_reset &&
        table->message_interactive &&
        table->message_noninteractive &&
        table->message_printed &&
        table->message_show &&
        table->message_handler &&
        table->dialogue &&
        table->sprite &&
        table->sprite_list &&
        table->sprite_add &&
        table->sprite_move &&
        table->sprite_x2_move &&
        table->sprite_scale &&
        table->sprite_color &&
        table->sprite_link &&
        table->object_add &&
        table->object_default_add &&
        table->input_pressed &&
        table->player_object &&
        table->player_name &&
        table->effect_create &&
        table->effect_attach &&
        table->application_notice &&
        table->transition_begin &&
        table->transition_color &&
        table->transition_busy &&
        table->audio_cutscene_reset &&
        table->environment_sound &&
        table->kick_item &&
        table->jump_bounce &&
        table->common_child1 &&
        table->common_child2 &&
        table->block_change &&
        table->block_change_ground &&
        table->common_remove &&
        table->misc_object_get &&
        table->misc_object_set &&
        table->model_time_left &&
        table->rng_range &&
        table->rng_float &&
        table->player_cutscene_process &&
        table->flute &&
        table->command &&
        table->command_link &&
        table->vector_distance &&
        table->dialogue_format;
}
IsleGameResult native_owner(IsleTunnelActorSource* source, Owner** output) noexcept {
    if (output) *output=nullptr;
    if (!source || !output) return ISLE_GAME_INVALID_ARGUMENT;
    if (!source->base.nativeGame) return ISLE_GAME_INVALID_STATE;
    IsleTunnelActorSource* canonical=nullptr;
    Owner* owner=nullptr;
    const auto result=SourceAccess::typed_context(source->base.nativeGame,
        isle_menu_current_object(source->base.nativeGame),&canonical,&owner,true);
    if (result!=ISLE_GAME_OK) return result;
    if (canonical!=source || SourceAccess::typed_admission(source,source->base.nativeGame,
        source->services)!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE;
    *output=owner;
    return ISLE_GAME_OK;
}
IsleGameResult native_host(IsleTunnelActorSource* source, course::NativeHost** output) noexcept {
    if (output) *output=nullptr;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    Owner* owner=nullptr;
    const auto result=native_owner(source,&owner);
    if (result!=ISLE_GAME_OK) return result;
    *output=owner->course_context().tunnel().native_host;
    return *output ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
}
IsleGameResult native_owner(IsleVolcanoActorSource* source, Owner** output) noexcept {
    if (output) *output=nullptr;
    if (!source || !output) return ISLE_GAME_INVALID_ARGUMENT;
    if (!source->base.nativeGame) return ISLE_GAME_INVALID_STATE;
    IsleVolcanoActorSource* canonical=nullptr;
    Owner* owner=nullptr;
    const auto result=SourceAccess::typed_context(source->base.nativeGame,
        isle_menu_current_object(source->base.nativeGame),&canonical,&owner,true);
    if (result!=ISLE_GAME_OK) return result;
    if (canonical!=source || SourceAccess::typed_admission(source,source->base.nativeGame,
        source->services)!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE;
    *output=owner;
    return ISLE_GAME_OK;
}
IsleGameResult native_host(IsleVolcanoActorSource* source, course::NativeHost** output) noexcept {
    if (output) *output=nullptr;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    Owner* owner=nullptr;
    const auto result=native_owner(source,&owner);
    if (result!=ISLE_GAME_OK) return result;
    *output=owner->course_context().volcano().native_host;
    return *output ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
}
IsleGameResult native_owner(IsleRiverActorSource* source, Owner** output) noexcept {
    if (output) *output=nullptr;
    if (!source || !output) return ISLE_GAME_INVALID_ARGUMENT;
    if (!source->base.nativeGame) return ISLE_GAME_INVALID_STATE;
    IsleRiverActorSource* canonical=nullptr;
    Owner* owner=nullptr;
    const auto result=SourceAccess::typed_context(source->base.nativeGame,
        isle_menu_current_object(source->base.nativeGame),&canonical,&owner,true);
    if (result!=ISLE_GAME_OK) return result;
    if (canonical!=source || SourceAccess::typed_admission(source,source->base.nativeGame,
        source->services)!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE;
    *output=owner;
    return ISLE_GAME_OK;
}
IsleGameResult native_host(IsleRiverActorSource* source, course::NativeHost** output) noexcept {
    if (output) *output=nullptr;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    Owner* owner=nullptr;
    const auto result=native_owner(source,&owner);
    if (result!=ISLE_GAME_OK) return result;
    *output=owner->course_context().river().native_host;
    return *output ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
}
IsleGameResult native_owner(IsleCaveActorSource* source, Owner** output) noexcept {
    if (output) *output=nullptr;
    if (!source || !output) return ISLE_GAME_INVALID_ARGUMENT;
    if (!source->base.nativeGame) return ISLE_GAME_INVALID_STATE;
    IsleCaveActorSource* canonical=nullptr;
    Owner* owner=nullptr;
    const auto result=SourceAccess::typed_context(source->base.nativeGame,
        isle_menu_current_object(source->base.nativeGame),&canonical,&owner,true);
    if (result!=ISLE_GAME_OK) return result;
    if (canonical!=source || SourceAccess::typed_admission(source,source->base.nativeGame,
        source->services)!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE;
    *output=owner;
    return ISLE_GAME_OK;
}
IsleGameResult native_host(IsleCaveActorSource* source, course::NativeHost** output) noexcept {
    if (output) *output=nullptr;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    Owner* owner=nullptr;
    const auto result=native_owner(source,&owner);
    if (result!=ISLE_GAME_OK) return result;
    *output=owner->course_context().cave().native_host;
    return *output ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
}
IsleGameResult native_owner(IsleValleyActorSource* source, Owner** output) noexcept {
    if (output) *output=nullptr;
    if (!source || !output) return ISLE_GAME_INVALID_ARGUMENT;
    if (!source->base.nativeGame) return ISLE_GAME_INVALID_STATE;
    IsleValleyActorSource* canonical=nullptr;
    Owner* owner=nullptr;
    const auto result=SourceAccess::typed_context(source->base.nativeGame,
        isle_menu_current_object(source->base.nativeGame),&canonical,&owner,true);
    if (result!=ISLE_GAME_OK) return result;
    if (canonical!=source || SourceAccess::typed_admission(source,source->base.nativeGame,
        source->services)!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE;
    *output=owner;
    return ISLE_GAME_OK;
}
IsleGameResult native_host(IsleValleyActorSource* source, course::NativeHost** output) noexcept {
    if (output) *output=nullptr;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    Owner* owner=nullptr;
    const auto result=native_owner(source,&owner);
    if (result!=ISLE_GAME_OK) return result;
    *output=owner->course_context().valley().native_host;
    return *output ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
}
OwnerStatus Owner::bind_ending(IsleBeachEndingServices services, IsleBeachMusicSource& music,
    const rom::Snapshot& snapshot) noexcept {
    auto& self=*impl_;
    if (self.ending_services.context || self.services.failed() || music.player!=&self.player ||
        !services.context || !services.validate || !services.release || !services.remove_items ||
        !services.reset_message || !services.reset_camera || !services.start_cutscene ||
        !services.camera || !services.cart_init || !services.cart_animation ||
        !services.cart_retain || !services.cart_callback ||
        std::any_of(self.slots.begin(),self.slots.end(),[](const auto& slot) { return slot.active; }))
        return rejected(OwnerError::admission,"ending requires a complete stable service binding before spawn");
    const auto bytes=snapshot.canonical();
    // Snapshot construction already admits the exact US identity; an empty
    // canonical view identifies a moved-from owner without rehashing/copying.
    if (bytes.size()!=rom::us_size)
        return rejected(OwnerError::admission,"ending data requires admitted US Snapshot");
    const auto u16=[&](std::size_t at) { return static_cast<std::uint16_t>(
        std::to_integer<unsigned>(bytes[at])*256u+std::to_integer<unsigned>(bytes[at+1])); };
    const auto u32=[&](std::size_t at) { return static_cast<std::uint32_t>(u16(at))*65536u+u16(at+2); };
    // Exact Beach .data offset = guest-0x802cc004+ROM0x564074.
    const auto signed32=[](std::uint32_t value) { return value<=INT32_MAX?
        static_cast<std::int32_t>(value):-1-static_cast<std::int32_t>(UINT32_MAX-value); };
    self.ending.D_beach_802CD9D0=signed32(u32(0x565a40));
    self.ending.D_beach_802CD9D4=signed32(u32(0x565a44));
    self.ending.exceptions[0]=u16(0x565a68);
    self.ending.exceptions[1]=u16(0x565a6a);
    self.ending.sky_speed=std::bit_cast<float>(u32(0xa7db4));
    if (self.ending.exceptions[0]!=1001 || self.ending.exceptions[1]!=0 ||
        !std::isfinite(self.ending.sky_speed))
        return rejected(OwnerError::admission,"ending exception/sky source data is invalid");
    self.ending.player=&self.player;
    self.ending.music=&music;
    if (services.validate(services.context,&self.ending)!=ISLE_GAME_OK)
        return rejected(OwnerError::admission,"ending provider does not match canonical course domains");
    self.ending_services=services;
    return {};
}
OwnerStatus Owner::release_ending() noexcept {
    auto& self=*impl_;
    if (!self.ending_services.context) return {};
    for (const auto& slot:self.slots) if (slot.active)
        return rejected(OwnerError::lifetime,"ending release requires actor reset/native joins");
    if (self.ending_services.release(self.ending_services.context)!=ISLE_GAME_OK)
        return rejected(OwnerError::lifetime,"ending provider native/cart/camera joins remain");
    self.ending.D_beach_80347658=nullptr;
    self.ending_services={};
    return {};
}
Owner::Owner(scene::Services& s,const actor::Resources& r,CourseContext state,world::World& w,environment_sound::Environment& e,
             focus::Detector& d,IslePlayerSource& p,std::span<const menu::animation::SlotCapacity> c,std::span<const menu::EffectBank* const> banks,
             menu::animation::ExecutionCapacity execution,RetainedFieldOptions retained)
    : impl_(std::make_unique<Impl>(s,r,state,w,e,d,p,c,banks,execution,retained)) {
    impl_->facade=this;
    if (state.course_id()!=0) {
        impl_->course_capacity_shape=[this]() {
            for (const auto& init:impl_->resources.initializations()) {
                if (init.model>=impl_->resources.models().size()) return false;
                const auto required=menu::animation::slot_capacity(impl_->resources.models()[init.model]);
                bool fits=false;
                for (const auto& capacity:impl_->capacities)
                    fits |= capacity.nodes>=required.nodes &&
                        capacity.material_attachments>=required.material_attachments;
                if (!fits) return false;
            }
            return true;
        }();
    }
}
OwnerStatus Owner::publish_particle_camera() noexcept {
    auto& self=*impl_;
    return self.publish_fx(self.particle_publication_started,static_cast<std::uint32_t>(-6),
        Impl::particles_tick,self.particle_object);
}
OwnerStatus Owner::publish_effect_emitters() noexcept {
    auto& self=*impl_;
    return self.publish_fx(self.effect_publication_started,static_cast<std::uint32_t>(-7),
        Impl::effects_tick,self.effect_object);
}
OwnerStatus Owner::publish_effects() noexcept {
    auto& self=*impl_;
    // Reject mixed/repeated bulk use without deleting a separately owned helper.
    if (self.particle_publication_started || self.effect_publication_started)
        return rejected(OwnerError::lifetime,"actor bulk FX publication already started");
    auto result=publish_particle_camera();
    if (result) result=publish_effect_emitters();
    if (result) return {};
    if (self.particle_object) self.services.erase(self.particle_object);
    if (self.effect_object) self.services.erase(self.effect_object);
    return result;
}
Owner::~Owner() {
    if (!reset()) {
        // Failure shutdown is not a successful source reset. Still perform real
        // joins and queue cancellation before callback storage can disappear.
        for (auto& slot:impl_->slots) if (slot.active && !slot.retiring) impl_->services.erase(slot.object);
        if (!collect_joined()) std::terminate();
    }
    if (!release_ending()) std::terminate();
    if (impl_->particle_object) impl_->services.erase(impl_->particle_object);
    if (impl_->effect_object) impl_->services.erase(impl_->effect_object);
    if (impl_->particle_object || impl_->effect_object) std::terminate();
}
const ActorInitialization* Owner::admitted_factory(std::uint32_t id,std::uint32_t factory) const noexcept {
    try {
        const auto& self=*impl_;
        self.resources.require_live();
        if (self.resources.course_id()!=self.course_context.course_id() ||
            !self.world.resources().setup || self.world.resources().setup->wire.guest_address!=
                detail::course_catalog(self.resources.course_id()).world_setup_identity) return nullptr;
        const auto spec=compiled_factory(self.resources.course_id(),id,factory);
        if (!spec.identity || !spec.initial_state ||
            (unsigned(bool(spec.returning))+unsigned(bool(spec.nonreturning))+unsigned(bool(spec.six)))!=1) return nullptr;
        const auto& catalog=detail::course_catalog(self.resources.course_id());
        for (std::size_t i=0;i<catalog.initializations.size();++i) {
            const auto& fact=catalog.initializations[i];
            if (spec.initialization && spec.initialization!=fact.identity) continue;
            if (fact.actor_id!=id) continue;
            if (i>=self.resources.initializations().size()) return nullptr;
            const auto& init=self.resources.initializations()[i];
            if (init.actor_id!=id || init.wire.guest_address!=fact.identity || init.wire.bytes.size()!=52 ||
                init.wire.u32(0)!=fact.model_identity || init.wire.u32(4)!=fact.material_table_identity ||
                init.wire.u32(8)!=fact.render_identity || init.wire.u32(12)!=fact.setup_identity ||
                init.render_callback_identity!=fact.render_identity || init.model>=self.resources.models().size() ||
                init.textures_present!=(fact.material_table_identity!=0) ||
                init.setup.wire.guest_address!=fact.setup_identity || init.setup.wire.bytes.size()!=20 ||
                init.setup.initial_state_identity!=fact.initial_state_identity ||
                init.setup.process_kind!=fact.process_kind || init.setup.unknown_0c_identity!=fact.unknown_0c_identity ||
                init.setup.transition_graph_identity!=fact.transition_graph_identity ||
                init.setup.wire.u32(0)!=fact.initial_header_identity ||
                init.setup.wire.u32(4)!=fact.initial_state_identity ||
                std::bit_cast<std::int8_t>(init.setup.wire.u8(8))!=fact.process_kind ||
                init.setup.wire.u32(12)!=fact.unknown_0c_identity ||
                init.setup.wire.u32(16)!=fact.transition_graph_identity) return nullptr;
            const auto& model=self.resources.models()[init.model];
            if (model.tree.guest_address!=fact.model_identity || model.nodes.size()!=fact.nodes ||
                model.material_table.guest_address!=fact.material_table_identity) return nullptr;
            if (!fact.initial_header_identity) {
                if (init.animations_enabled || init.setup.initial_header!=actor_no_index) return nullptr;
            } else {
                if (!init.animations_enabled || init.setup.initial_header>=self.resources.headers().size()) return nullptr;
                const auto& header=self.resources.headers()[init.setup.initial_header];
                if (header.wire.guest_address!=fact.initial_header_identity || header.initialization!=i) return nullptr;
            }
            if (self.resources.course_id()==6) {
                bool matched=false;
                for (const auto& binding:rainbow::actor_factories())
                    if (binding.actor_id==id && binding.factory==factory &&
                        binding.initialization==fact.identity && binding.initial_state==fact.initial_state_identity &&
                        binding.function==spec.returning) matched=true;
                if (!matched) return nullptr;
            }
            return &init;
        }
    } catch (...) {}
    return nullptr;
}
InteractionServices* Owner::bound_interactions() const noexcept { return impl_->interactions; }
SpawnResult Owner::spawn(const SpawnRequest& request) noexcept {
    auto& self=*impl_;
    if (self.world.retired())
        return {{OwnerError::admission,"actor factory cannot enter a retiring World"},{},nullptr};
    if (request.definition>=self.resources.definitions().size())
        return {{OwnerError::admission,"actor definition outside admitted resources"},{},nullptr};
    const auto& definition=self.resources.definitions()[request.definition];
    if (!initialized_definition(self.resources,definition) ||
        !admitted_factory(definition.actor_id,definition.factory_identity))
        return {{OwnerError::admission,"configured source factory tuple/origin is not admitted"},{},nullptr};
    if (self.course_context.course_id()!=0 && SourceAccess::course_admission(*this)!=ISLE_GAME_OK)
        return {{OwnerError::admission,"factory requires ACTIVE canonical complete typed Source closure"},{},nullptr};
    const auto spec=compiled_factory(self.resources.course_id(),definition.actor_id,definition.factory_identity);
    const bool unwritten=request.spawn && request.spawn==self.transforms_only_spawn;
    if ((spec.wrapper==FactoryWrapper::meowth || spec.wrapper==FactoryWrapper::valley_magikarp) &&
        (!request.spawn || (unwritten && (spec.wrapper!=FactoryWrapper::valley_magikarp ||
            !SourceAccess::portability_repair(self,request)))))
        return {{OwnerError::admission,"conditional factory requires an authored or exactly repaired behavior borrow"},{},nullptr};
    SpawnResult result{{OwnerError::admission,"source factory did not publish a result"},{},nullptr};
    IsleActorFactoryBlock block{this,request.block,&result},reference{this,request.reference,&result};
    IsleActorFactorySpawn spawn{request.spawn,request.definition,
        (spec.wrapper==FactoryWrapper::meowth || spec.wrapper==FactoryWrapper::valley_magikarp)?&request.spawn->behavior:nullptr,request.automatic_definition};
    IsleGameObject* returned=nullptr;
    const Impl::FactoryTransport transport{&block,&reference,&spawn,&request,&result,self.factory_transport};
    self.factory_transport=&transport;
    IsleActorFactoryInitialization initialization{self.resources.initializations()[definition.initialization].wire.guest_address};
    const auto called=spec.six ? isle_actor_source_factory_six_call(self.services.game,spec.six,
        static_cast<int32_t>(request.object_id),static_cast<uint16_t>(definition.actor_id),
        &block,&reference,&spawn,&initialization,&returned) :
        isle_actor_source_factory_call(self.services.game,spec.returning,spec.nonreturning,
        static_cast<int32_t>(request.object_id),static_cast<uint16_t>(definition.actor_id),
        &block,&reference,&spawn,&returned);
    self.factory_transport=transport.previous;
    if (called!=ISLE_GAME_OK)
        return {{OwnerError::native,"original C factory fence failed"},{},nullptr};
    if ((spec.returning || spec.six) && returned!=result.object)
        return {{OwnerError::native,"original C factory returned a different publication"},{},nullptr};
    return result;
}
OwnerStatus Owner::configure_registry(std::int32_t minimum,std::int32_t maximum,
    std::int32_t link,std::int32_t arg3) noexcept {
    auto& self=*impl_;
    if (self.services.failed()) return rejected(OwnerError::services,"registry configuration requires live Services");
    if (minimum<0 || maximum<minimum || link<0 || link>=32)
        return rejected(OwnerError::admission,"source registry ID range/native link is invalid");
    for (const auto& slot:self.slots) if (slot.active)
        return rejected(OwnerError::lifetime,"source registry configuration precedes all actor publication");
    self.minimum_pokemon_id=minimum;
    self.maximum_pokemon_id=maximum;
    self.pokemon_link=link;
    self.registry_arg3=arg3;
    self.block_entries.fill({});
    return {};
}
SpawnResult Owner::add_one(std::size_t definition,world::Block* block,world::Block* reference,
                           const world::Spawn& descriptor) noexcept {
    auto& self=*impl_;
    // Exact source cursor/two half-open searches and inclusive max fallback.
    const auto minimum=static_cast<std::uint32_t>(self.minimum_pokemon_id);
    const auto maximum=static_cast<std::uint32_t>(self.maximum_pokemon_id);
    if (++self.pokemon_counter>maximum-minimum) self.pokemon_counter=0;
    std::uint32_t id=maximum;
    for (unsigned pass=0;pass<2 && id==maximum;++pass) {
        const auto first=pass?minimum:minimum+self.pokemon_counter;
        const auto last=pass?minimum+self.pokemon_counter:maximum;
        for (auto candidate=first;candidate<last;++candidate) {
            int used=0;
            if (isle_actor_source_id_in_use_link(self.services.game,static_cast<std::uint8_t>(self.pokemon_link),candidate,&used)!=ISLE_GAME_OK)
                return {{OwnerError::native,"source native ID search failed"},{},nullptr};
            if (!used) { id=candidate; break; }
        }
    }
    return spawn({id,definition,block,reference,&descriptor,
        &descriptor==self.transforms_only_spawn?self.transforms_only_automatic:
        &descriptor==self.initialized_automatic_spawn?self.initialized_automatic_index:actor_no_index});
}
SpawnResult Owner::add_one_magikarp_unwritten(std::size_t definition,world::Block* block,
    world::Block* reference,IsleGameVec3 translation,IsleGameVec3 euler,IsleGameVec3 scale) noexcept {
    auto& self=*impl_;
    if (self.course_context.course_id()!=0 || definition>=self.resources.definitions().size() || self.transforms_only_spawn)
        return {{OwnerError::admission,"unwritten Magikarp factory transport invalid"},{},nullptr};
    const auto& def=self.resources.definitions()[definition];
    if (def.wire.guest_address!=0x802cbff4u || def.actor_id!=129 || def.factory_identity!=0x802cac48u ||
        !initialized_definition(self.resources,def) || !admitted_factory(def.actor_id,def.factory_identity))
        return {{OwnerError::admission,"unwritten fields belong only to standalone original CBFF4"},{},nullptr};
    world::Spawn spawn{};
    spawn.id=129;
    spawn.position={translation.x,translation.y,translation.z};
    spawn.rotation={euler.x,euler.y,euler.z};
    spawn.scale={scale.x,scale.y,scale.z};
    self.transforms_only_spawn=&spawn;
    self.transforms_only_definition=definition;
    self.transforms_only_helper=0x802cbff4u;
    self.transforms_only_written_fields=ISLE_RAINBOW_SPAWN_ID_TRANSFORMS;
    const auto result=add_one(definition,block,reference,spawn);
    self.transforms_only_spawn=nullptr;
    self.transforms_only_definition=actor_no_index;
    self.transforms_only_helper=0;
    self.transforms_only_written_fields=0;
    return result;
}
SpawnResult Owner::add_one_rainbow(const IsleRainbowDefinition& definition,
    const IsleRainbowAutomaticSpawn& written,world::Block* block,world::Block* reference) noexcept {
    auto& self=*impl_;
    const auto reject=[](const char* detail) {
        return SpawnResult{{OwnerError::admission,detail},{},nullptr};
    };
    if (self.course_context.course_id()!=6 || self.transforms_only_spawn ||
        !block || block!=self.world.current() || reference!=block ||
        written.written_fields!=ISLE_RAINBOW_SPAWN_ID_TRANSFORMS ||
        written.id<0 || static_cast<std::uint32_t>(written.id)!=definition.actor_id)
        return reject("Rainbow child requires its current canonical block and exact written-field mask");
    if (SourceAccess::rainbow_admission(&self.course_context.rainbow().actors)!=ISLE_GAME_OK)
        return reject("Rainbow child requires ACTIVE complete canonical Source admission");
    bool origin=false;
    for (const auto& child:rainbow::actor_child_factories()) {
        const auto& fact=child.definition;
        if (child.helper_identity==written.helper_identity && child.written_fields==written.written_fields &&
            fact.identity==definition.identity && fact.origin==definition.origin &&
            fact.actor_id==definition.actor_id && fact.factory==definition.factory &&
            fact.change_block==definition.change_block && fact.remove==definition.remove &&
            fact.initialization==definition.initialization) origin=true;
    }
    const auto* initialization=admitted_factory(definition.actor_id,definition.factory);
    if (!origin || !initialization || initialization->wire.guest_address!=definition.initialization)
        return reject("Rainbow child helper/definition/factory/init origin differs from compiled Source");
    std::size_t automatic=actor_no_index;
    if (!definition.identity) {
        const auto facts=detail::rainbow_catalog().automatic_definitions;
        const auto actual=self.resources.automatic_definitions();
        for (std::size_t i=0;i<facts.size() && i<actual.size();++i) {
            const auto& fact=facts[i]; const auto& candidate=actual[i];
            if (fact.origin.function_identity==definition.origin &&
                candidate.origin.function_identity==fact.origin.function_identity &&
                candidate.origin.line==fact.origin.line && candidate.actor_id==definition.actor_id &&
                candidate.factory_identity==definition.factory && candidate.change_block_identity==definition.change_block &&
                candidate.remove_identity==definition.remove && candidate.initialization==fact.initialization &&
                candidate.initialization<self.resources.initializations().size() &&
                &self.resources.initializations()[candidate.initialization]==initialization)
                automatic=i;
        }
        if (automatic==actor_no_index) return reject("Rainbow automatic Source function/line origin is not admitted");
    }
    std::size_t index=actor_no_index;
    for (std::size_t i=0;i<self.resources.definitions().size();++i) {
        const auto& candidate=self.resources.definitions()[i];
        if ((definition.identity ? candidate.wire.guest_address==definition.identity && candidate.singleton : !candidate.singleton) &&
            candidate.actor_id==definition.actor_id && candidate.factory_identity==definition.factory &&
            candidate.change_block_identity==definition.change_block && candidate.remove_identity==definition.remove &&
            candidate.initialization<self.resources.initializations().size() &&
            &self.resources.initializations()[candidate.initialization]==initialization &&
            initialized_definition(self.resources,candidate)) { index=i; break; }
    }
    if (index==actor_no_index) return reject("Rainbow child has no exact initialized resource family");
    world::Spawn descriptor{};
    descriptor.id=written.id;
    descriptor.position={written.translation.x,written.translation.y,written.translation.z};
    descriptor.rotation=written.euler;
    descriptor.scale={written.scale.x,written.scale.y,written.scale.z};
    self.transforms_only_spawn=&descriptor;
    self.transforms_only_definition=index;
    self.transforms_only_automatic=automatic;
    self.transforms_only_helper=written.helper_identity;
    self.transforms_only_written_fields=written.written_fields;
    const auto result=add_one(index,block,reference,descriptor);
    self.transforms_only_spawn=nullptr;
    self.transforms_only_definition=actor_no_index;
    self.transforms_only_automatic=actor_no_index;
    self.transforms_only_helper=0;
    self.transforms_only_written_fields=0;
    return result;
}
SpawnResult Owner::add_geo_typed(IsleGameObject* object,std::uint32_t requested,
    std::uint32_t singleton,std::uint32_t origin,std::uint32_t line,std::uint32_t family,
    std::uint32_t factory,std::uint32_t initialization,std::uint32_t change,std::uint32_t remove) noexcept {
    auto& self=*impl_;
    const auto reject=[](const char* detail) { return SpawnResult{{OwnerError::admission,detail},{},nullptr}; };
    IsleActorSource* parent=nullptr;
    if (requested>UINT16_MAX || isle_actor_source_get(self.services.game,object,&parent)!=ISLE_GAME_OK ||
        !parent || !SourceAccess::slot(parent) || SourceAccess::slot(parent)->owner!=&self ||
        SourceAccess::course_admission(*this)!=ISLE_GAME_OK)
        return reject("AddAtGeo requires this canonical live Actor and complete typed Source");
    const auto* init=admitted_factory(family,factory);
    if (!init || (initialization && init->wire.guest_address!=initialization))
        return reject("AddAtGeo family/Init lacks exact compiled factory admission");
    std::size_t automatic=actor_no_index;
    if (origin && line) {
        for (std::size_t i=0;i<self.resources.automatic_definitions().size();++i) {
            const auto& def=self.resources.automatic_definitions()[i];
            if (def.origin.function_identity==origin && def.origin.line==line && def.actor_id==family &&
                def.factory_identity==factory && def.change_block_identity==change && def.remove_identity==remove &&
                def.initialization<self.resources.initializations().size() &&
                &self.resources.initializations()[def.initialization]==init) automatic=i;
        }
        if (automatic==actor_no_index) return reject("AddAtGeo automatic function/line provenance is not admitted");
    }
    std::size_t index=actor_no_index;
    // Exact automatic origins already validate their callback tuple above;
    // their initialized resource family can use different registry callbacks.
    const bool automatic_family=!singleton && automatic!=actor_no_index;
    for (std::size_t i=0;i<self.resources.definitions().size();++i) {
        const auto& def=self.resources.definitions()[i];
        if ((!singleton || (def.singleton && def.wire.guest_address==singleton)) && def.actor_id==family &&
            def.factory_identity==factory && (automatic_family ||
                (def.change_block_identity==change && def.remove_identity==remove)) &&
            def.initialization<self.resources.initializations().size() &&
            &self.resources.initializations()[def.initialization]==init && initialized_definition(self.resources,def)) {
            index=i; break;
        }
    }
    if (index==actor_no_index || (!singleton && automatic==actor_no_index))
        return reject("AddAtGeo has no exact definition/singleton authority");
    if (requested!=family && automatic==actor_no_index)
        return reject("AddAtGeo requested/family mismatch lacks exact automatic Source origin");
    IsleActorGeoDefinition definition{singleton};
    SpawnResult result{{OwnerError::admission,"Original AddAtGeo did not publish an Actor"},{},nullptr};
    const Impl::GeoTransport ticket{&definition,parent,requested,origin,index,automatic,&result,self.geo_transport};
    self.geo_transport=&ticket;
    IsleGameObject* output=nullptr;
    const auto called=isle_actor_source_add_geo_call(self.services.game,object,static_cast<std::uint16_t>(requested),&definition,&output);
    self.geo_transport=ticket.previous;
    if (called!=ISLE_GAME_OK || output!=result.object)
        return reject("Original AddAtGeo C fence failed or returned a different canonical publication");
    return result;
}
SpawnResult Owner::add_geo_tunnel(IsleGameObject* object,std::uint16_t requested,
    const IsleTunnelDefinition& definition) noexcept {
    const auto reject=[](const char* detail) { return SpawnResult{{OwnerError::admission,detail},{},nullptr}; };
    if (impl_->course_context.course_id()!=1) return reject("typed AddAtGeo belongs to another course");
    for (const auto& child:tunnel::actor_child_factories()) {
        const auto& fact=child.definition;
        if (!child.from_geo || requested!=fact.actor_id ||
            definition.identity!=fact.identity || definition.origin!=fact.origin ||
            definition.actor_id!=fact.actor_id || definition.factory!=fact.factory ||
            definition.initialization!=fact.initialization || definition.change_block!=fact.change_block ||
            definition.remove!=fact.remove) continue;
        // Source's automatic PokemonDef is not an invented singleton. Preserve
        // its function/line authority while the original AddAtGeo initializes
        // every spawn field and copies the canonical parent's final position.
        return add_geo_typed(object,requested,fact.identity,child.origin.containing_function,
            fact.identity ? 0u : child.origin.declaration_line,fact.actor_id,fact.factory,
            fact.initialization,fact.change_block,fact.remove);
    }
    if (requested==1003 && definition.identity==0x8038837cu && definition.origin==0 &&
        definition.actor_id==1003 && definition.factory==0x8035e0d4u &&
        definition.initialization==0x80388348u && definition.change_block==0x80363dbcu && definition.remove==0x80364280u)
        return add_geo_typed(object,requested,definition.identity,0,0,definition.actor_id,definition.factory,
            definition.initialization,definition.change_block,definition.remove);
    return reject("Tunnel AddAtGeo differs from exact compiled Source child provenance");
}
SpawnResult Owner::add_geo_volcano(IsleGameObject* object,std::uint16_t requested,
    const IsleVolcanoDefinition& definition) noexcept {
    const auto reject=[](const char* detail) { return SpawnResult{{OwnerError::admission,detail},{},nullptr}; };
    if (impl_->course_context.course_id()!=2) return reject("typed AddAtGeo belongs to another course");
    for (const auto& child:volcano::actor_child_factories()) {
        const auto& fact=child.definition;
        if (!child.from_geo || requested!=child.requested_id || definition.identity!=fact.identity ||
            definition.containing_function!=fact.containing_function || definition.declaration_line!=fact.declaration_line ||
            definition.copied_from!=fact.copied_from || definition.actor_id!=fact.actor_id || definition.factory!=fact.factory ||
            definition.change_block!=fact.change_block || definition.remove!=fact.remove) continue;
        return add_geo_typed(object,requested,fact.identity,child.helper_identity,
            fact.identity ? 0u : child.declaration_line,fact.actor_id,fact.factory,0,fact.change_block,fact.remove);
    }
    if (requested==1003 && definition.identity==0x8038837cu && definition.actor_id==1003 &&
        definition.factory==0x8035e0d4u && definition.change_block==0x80363dbcu && definition.remove==0x80364280u)
        return add_geo_typed(object,requested,definition.identity,0,0,1003,definition.factory,0x80388348u,
            definition.change_block,definition.remove);
    return reject("Volcano AddAtGeo differs from exact compiled Source child provenance");
}
SpawnResult Owner::add_geo_cave(IsleGameObject* object,std::uint16_t requested,
    const IsleCaveDefinition& definition) noexcept {
    const auto reject=[](const char* detail) { return SpawnResult{{OwnerError::admission,detail},{},nullptr}; };
    if (impl_->course_context.course_id()!=4) return reject("typed AddAtGeo belongs to another course");
    for (const auto& child:cave::actor_child_factories()) {
        const auto& fact=child.definition;
        if (requested!=fact.actor_id || definition.identity!=fact.identity || definition.origin!=fact.origin ||
            definition.actor_id!=fact.actor_id || definition.factory!=fact.factory || definition.change_block!=fact.change_block ||
            definition.remove!=fact.remove || definition.initialization!=fact.initialization ||
            definition.singleton_identity!=fact.singleton_identity || definition.declaration_line!=fact.declaration_line) continue;
        return add_geo_typed(object,requested,fact.identity,child.helper_identity,
            fact.identity ? 0u : child.declaration_line,fact.actor_id,fact.factory,
            fact.initialization,fact.change_block,fact.remove);
    }
    return reject("Cave AddAtGeo differs from exact compiled Source child provenance");
}
SpawnResult Owner::add_geo_valley(IsleGameObject* object,std::uint16_t requested,
    const IsleValleyDefinition& definition) noexcept {
    const auto reject=[](const char* detail) { return SpawnResult{{OwnerError::admission,detail},{},nullptr}; };
    if (impl_->course_context.course_id()!=5) return reject("typed AddAtGeo belongs to another course");
    for (const auto& child:valley::actor_child_factories()) {
        if (child.written_fields || requested!=child.requested_id || definition.identity!=child.singleton_origin ||
            definition.family!=child.family || definition.factory!=child.factory ||
            definition.change_block!=child.change_block || definition.remove!=child.remove ||
            definition.origin || definition.declaration_line) continue;
        return add_geo_typed(object,requested,child.singleton_origin,child.helper_identity,0,child.family,
            child.factory,child.initialization,child.change_block,child.remove);
    }
    return reject("Valley AddAtGeo differs from exact compiled Source child provenance");
}
SpawnResult Owner::add_one_typed(std::uint32_t singleton,std::uint32_t origin,
    std::uint32_t line,std::uint32_t family,std::uint32_t factory,std::uint32_t initialization,
    std::uint32_t change,std::uint32_t remove,std::uint32_t requested,std::uint32_t fields,
    IsleGameVec3 translation,IsleGameVec3 euler,IsleGameVec3 scale,const void* path,
    world::Block* block,world::Block* reference) noexcept {
    auto& self=*impl_;
    const auto reject=[](const char* detail) { return SpawnResult{{OwnerError::admission,detail},{},nullptr}; };
    if (self.transforms_only_spawn || !block || block!=self.world.current() || reference!=block ||
        (fields!=0x3ffu && fields!=0x7ffu) || SourceAccess::course_admission(*this)!=ISLE_GAME_OK)
        return reject("typed automatic spawn requires canonical current blocks, written fields and ACTIVE Source");
    const auto* init=admitted_factory(family,factory);
    if (!init || (initialization && init->wire.guest_address!=initialization))
        return reject("typed automatic factory does not match its actual resource Init");
    const auto* repair=automatic_spawn_repair(self.course_context.course_id(),origin,line,fields,family,factory);
    if ((self.course_context.course_id()==3 || self.course_context.course_id()==5) && !repair)
        return reject("automatic Source origin lacks an exact native portability policy");
    std::size_t automatic=actor_no_index;
    if (origin && line) {
        const auto facts=detail::course_catalog(self.resources.course_id()).automatic_definitions;
        const auto actual=self.resources.automatic_definitions();
        for (std::size_t i=0;i<facts.size() && i<actual.size();++i) {
            const auto& fact=facts[i]; const auto& candidate=actual[i];
            if (fact.origin.function_identity==origin && fact.origin.line==line &&
                candidate.origin.function_identity==origin && candidate.origin.line==line &&
                candidate.actor_id==family && candidate.factory_identity==factory &&
                candidate.change_block_identity==change && candidate.remove_identity==remove &&
                candidate.initialization==fact.initialization && candidate.initialization<self.resources.initializations().size() &&
                &self.resources.initializations()[candidate.initialization]==init) automatic=i;
        }
        if (automatic==actor_no_index) return reject("typed automatic function/line origin is outside admitted Source authority");
    }
    if (!singleton && automatic==actor_no_index)
        return reject("typed child has neither an actual singleton nor an admitted automatic origin");
    std::size_t index=actor_no_index;
    for (std::size_t i=0;i<self.resources.definitions().size();++i) {
        const auto& def=self.resources.definitions()[i];
        if ((!singleton || (def.singleton && def.wire.guest_address==singleton)) &&
            def.actor_id==family && def.factory_identity==factory &&
            def.change_block_identity==change && def.remove_identity==remove &&
            def.initialization<self.resources.initializations().size() &&
            &self.resources.initializations()[def.initialization]==init && initialized_definition(self.resources,def)) {
            index=i; break;
        }
    }
    if (index==actor_no_index) return reject("typed child has no exact admitted definition family");
    world::Spawn descriptor{};
    descriptor.id=static_cast<std::int32_t>(requested);
    descriptor.position={translation.x,translation.y,translation.z};
    descriptor.rotation=euler; descriptor.scale={scale.x,scale.y,scale.z};
    if (repair) {
        // This complete native input is built field-by-field; neither River's
        // unwritten behavior nor any unwritten native path member is evaluated.
        descriptor.behavior=repair->behavior;
        descriptor.path=world::no_index;
    }
    if (fields&0x400u) {
        if (path) {
            bool found=false;
            for (std::size_t i=0;i<self.world.resources().paths.size();++i)
                if (&self.world.resources().paths[i]==path) { descriptor.path=i; found=true; break; }
            if (!found) return reject("Source-written automatic path is not a canonical World path");
        }
    } else if (path) return reject("unwritten automatic path was supplied by native caller");
    self.transforms_only_spawn=&descriptor; self.transforms_only_definition=index;
    self.transforms_only_automatic=automatic; self.transforms_only_helper=origin;
    self.transforms_only_written_fields=fields;
    self.transforms_only_requested_id=descriptor.id;
    self.transforms_only_repair=repair;
    const auto result=add_one(index,block,reference,descriptor);
    self.transforms_only_spawn=nullptr; self.transforms_only_definition=actor_no_index;
    self.transforms_only_automatic=actor_no_index; self.transforms_only_helper=0;
    self.transforms_only_written_fields=0; self.transforms_only_requested_id=-1;
    self.transforms_only_repair=nullptr;
    return result;
}
SpawnResult Owner::add_one_tunnel(const IsleTunnelDefinition& definition,
    const IsleTunnelAutomaticSpawn& written,world::Block* block,world::Block* reference) noexcept {
    const auto reject=[](const char* detail) { return SpawnResult{{OwnerError::admission,detail},{},nullptr}; };
    if (impl_->course_context.course_id()!=1 || written.id<0)
        return reject("Tunnel automatic spawn belongs to another typed course");
    for (const auto& child:tunnel::actor_child_factories()) {
        const auto& fact=child.definition;
        if (child.from_geo || written.origin!=child.origin.containing_function || written.written_fields!=0x3ffu ||
            definition.identity!=fact.identity || definition.origin!=fact.origin ||
            definition.actor_id!=fact.actor_id || definition.factory!=fact.factory ||
            definition.change_block!=fact.change_block || definition.remove!=fact.remove ||
            definition.initialization!=fact.initialization || static_cast<std::uint32_t>(written.id)!=fact.actor_id) continue;
        return add_one_typed(fact.identity,child.origin.containing_function,
            fact.identity ? 0u : child.origin.declaration_line,fact.actor_id,fact.factory,
            fact.initialization,fact.change_block,fact.remove,written.id,written.written_fields,
            written.translation,written.euler,written.scale,nullptr,block,reference);
    }
    return reject("Tunnel child differs from exact compiled Source helper/definition provenance");
}
SpawnResult Owner::add_one_volcano(const IsleVolcanoDefinition& definition,
    const IsleVolcanoAutomaticSpawn& written,world::Block* block,world::Block* reference) noexcept {
    const auto reject=[](const char* detail) { return SpawnResult{{OwnerError::admission,detail},{},nullptr}; };
    if (impl_->course_context.course_id()!=2 || written.id<0)
        return reject("Volcano automatic spawn belongs to another typed course");
    for (const auto& child:volcano::actor_child_factories()) {
        const auto& fact=child.definition;
        if (child.from_geo || written.containing_function!=child.helper_identity ||
            written.written_fields!=child.written_fields ||
            static_cast<std::uint32_t>(written.id)!=child.requested_id ||
            definition.identity!=fact.identity || definition.containing_function!=fact.containing_function ||
            definition.declaration_line!=fact.declaration_line || definition.copied_from!=fact.copied_from ||
            definition.actor_id!=fact.actor_id || definition.factory!=fact.factory ||
            definition.change_block!=fact.change_block || definition.remove!=fact.remove) continue;
        return add_one_typed(fact.identity,child.helper_identity,child.declaration_line,
            fact.actor_id,fact.factory,0,fact.change_block,fact.remove,written.id,written.written_fields,
            written.translation,written.euler,written.scale,
            child.copy_path ? written.path : nullptr,block,reference);
    }
    return reject("Volcano child differs from exact compiled Source helper/definition provenance");
}
SpawnResult Owner::add_one_river(const IsleRiverDefinition& definition,
    const IsleRiverAutomaticSpawn& written,world::Block* block,world::Block* reference) noexcept {
    const auto reject=[](const char* detail) { return SpawnResult{{OwnerError::admission,detail},{},nullptr}; };
    if (impl_->course_context.course_id()!=3 || written.id<0)
        return reject("River automatic spawn belongs to another typed course");
    for (const auto& child:river::actor_child_factories()) {
        const auto& fact=child.definition;
        if (written.origin.function!=child.origin.function || written.origin.line!=child.origin.line ||
            written.written_fields!=child.written_fields || definition.identity!=fact.identity ||
            definition.origin.function!=fact.origin.function || definition.origin.line!=fact.origin.line ||
            definition.actor_id!=fact.actor_id || definition.factory!=fact.factory ||
            definition.change_block!=fact.change_block || definition.remove!=fact.remove ||
            definition.initialization!=fact.initialization || static_cast<std::uint32_t>(written.id)!=fact.actor_id) continue;
        const auto* repair=automatic_spawn_repair(3,child.origin.function,child.origin.line,
            child.written_fields,fact.actor_id,fact.factory);
        if (!repair) return reject("River automatic Source origin lacks its exact native portability policy");
        // Only these two Source0x7ff helpers wrote path. The conditional never
        // reads the other six records' path or any record's unwritten behavior.
        const void* path=repair->path==AutomaticPathRepair::source_written ? written.path : nullptr;
        return add_one_typed(fact.identity,child.origin.function,child.origin.line,fact.actor_id,
            fact.factory,fact.initialization,fact.change_block,fact.remove,written.id,written.written_fields,
            written.translation,written.euler,written.scale,path,block,reference);
    }
    return reject("River child differs from exact compiled Source helper/definition provenance");
}
SpawnResult Owner::add_one_cave(const IsleCaveDefinition& definition,
    const IsleCaveAutomaticSpawn& written,world::Block* block,world::Block* reference) noexcept {
    const auto reject=[](const char* detail) { return SpawnResult{{OwnerError::admission,detail},{},nullptr}; };
    if (impl_->course_context.course_id()!=4 || written.id<0)
        return reject("Cave automatic spawn belongs to another typed course");
    for (const auto& child:cave::actor_child_factories()) {
        const auto& fact=child.definition;
        if (written.helper_identity!=child.helper_identity || written.written_fields!=child.written_fields ||
            definition.identity!=fact.identity || definition.origin!=fact.origin ||
            definition.actor_id!=fact.actor_id || definition.factory!=fact.factory ||
            definition.change_block!=fact.change_block || definition.remove!=fact.remove ||
            definition.initialization!=fact.initialization || definition.singleton_identity!=fact.singleton_identity ||
            definition.declaration_line!=fact.declaration_line || static_cast<std::uint32_t>(written.id)!=fact.actor_id) continue;
        return add_one_typed(fact.identity,child.helper_identity,
            fact.identity ? 0u : child.declaration_line,fact.actor_id,fact.factory,
            fact.initialization,fact.change_block,fact.remove,written.id,written.written_fields,
            written.translation,written.euler,written.scale,nullptr,block,reference);
    }
    return reject("Cave child differs from exact compiled Source helper/definition provenance");
}
SpawnResult Owner::add_one_valley(const IsleValleyDefinition& definition,
    const IsleValleyAutomaticSpawn& written,world::Block* block,world::Block* reference) noexcept {
    const auto reject=[](const char* detail) { return SpawnResult{{OwnerError::admission,detail},{},nullptr}; };
    if (impl_->course_context.course_id()!=5 || written.id<0)
        return reject("Valley automatic spawn belongs to another typed course");
    for (const auto& child:valley::actor_child_factories()) {
        // Matching-MIPS639d establishes the actual numeric request147, not
        // Pokedex inference from Source's ARTICUNO label. Keep the exact
        // descriptor's requested identity separate from definition family.
        const auto requested=child.requested_id;
        if (!child.written_fields || written.origin!=child.helper_identity ||
            written.declaration_line!=child.spawn_declaration_line || written.written_fields!=child.written_fields ||
            static_cast<std::uint32_t>(written.id)!=requested || definition.identity!=0 ||
            definition.singleton_origin!=child.singleton_origin || definition.origin!=child.helper_identity ||
            definition.declaration_line!=child.definition_declaration_line || definition.family!=child.family ||
            definition.factory!=child.factory || definition.change_block!=child.change_block ||
            definition.remove!=child.remove) continue;
        return add_one_typed(0,child.helper_identity,child.definition_declaration_line,child.family,child.factory,
            child.initialization,child.change_block,child.remove,written.id,written.written_fields,
            written.translation,written.euler,written.scale,nullptr,block,reference);
    }
    return reject("Valley child differs from exact compiled Source helper/definition provenance");
}
OwnerStatus Owner::block_members(const world::Block& block,
    std::span<menu::animation::InstanceHandle> output,std::size_t& written) const noexcept {
    written=0;
    const auto& self=*impl_;
    if (block.index>=self.block_heads.size() || &self.world.blocks()[block.index]!=&block)
        return rejected(OwnerError::admission,"block membership requires this World's canonical block");
    std::size_t count=0;
    for (int i=self.block_heads[block.index];i!=-1;) {
        if (i<0 || static_cast<std::size_t>(i)>=self.block_entries.size() || ++count>self.block_entries.size())
            return rejected(OwnerError::lifetime,"source block membership list is corrupt");
        const auto& entry=self.block_entries[static_cast<std::size_t>(i)];
        if (!entry.slot || !entry.slot->registered || !entry.slot->logical || entry.slot->retiring)
            return rejected(OwnerError::lifetime,"source block list names a nonlogical actor");
        i=entry.next;
    }
    if (count>output.size()) return rejected(OwnerError::capacity,"block membership output exhausted");
    for (int i=self.block_heads[block.index];i!=-1;) {
        const auto& entry=self.block_entries[static_cast<std::size_t>(i)];
        output[written++]=entry.slot->instance; i=entry.next;
    }
    return {};
}
OwnerStatus Owner::step_walk(menu::animation::InstanceHandle handle, float velocity_x, float velocity_z,
                            std::uint32_t flags, bool& blocked) noexcept {
    auto& self=*impl_;
    if (self.services.failed()) return rejected(OwnerError::services,"actor walking requires healthy native Services");
    auto* slot=self.find(handle);
    if (!slot || !slot->logical || !slot->registered)
        return rejected(OwnerError::lifetime,"actor walking requires a current logical native instance");
    return collision::step_walk(self.world,slot->source,slot->model,velocity_x,velocity_z,flags,blocked);
}
OwnerStatus Owner::update_collisions() noexcept {
    auto& self=*impl_;
    if (self.services.failed()) return rejected(OwnerError::services,"actor collision update requires healthy native Services");
    const auto refresh=[](Impl::Slot& slot) noexcept -> OwnerStatus {
        IsleGameVec3 position;
        if (photo::collision_position(slot.source,slot.model,position)!=ISLE_GAME_OK)
            return rejected(OwnerError::native,"canonical source collision position refresh failed");
        slot.source.collPosition=position;
        return {};
    };
    for (std::size_t i=0;i<self.logical_count;++i) {
        auto& slot=*self.logical[i];
        const auto result=refresh(slot); if (!result) return result;
        slot.source.processFlags&=~std::uint32_t{0x80};
    }
    // Own pokemon.c2250-2321. These snapshots are deliberately NOT refreshed
    // when a walk recalculates canonical fields. Source pair order is logical
    // insertion order, independent of physical slots and native render/link3.
    const auto displacement=[](const IsleGameVec3& to,const IsleGameVec3& from,float length,
                               float& velocity_x,float& velocity_z) noexcept {
        float dx=to.x-from.x,dz=to.z-from.z;
        if (std::fabs(dz)<0.00001) { dx=1.0f; dz=0.0f; }
        const float inverse=length/std::sqrt(dx*dx+dz*dz);
        velocity_x=dx*inverse; velocity_z=dz*inverse;
    };
    const auto notify=[&](Impl::Slot& recipient,IsleGameObject* sender) noexcept -> OwnerStatus {
        if (recipient.source.processFlags&0x80u) return {};
        recipient.source.processFlags|=0x80u;
        if (!self.services.send_command(recipient.object,26,sender))
            return rejected(OwnerError::services,"source collision command26 enqueue failed");
        return {};
    };
    const auto move=[&](Impl::Slot& slot,const IsleGameVec3& to,const IsleGameVec3& from,float length) noexcept -> OwnerStatus {
        float velocity_x,velocity_z; displacement(to,from,length,velocity_x,velocity_z);
        bool blocked;
        const auto result=collision::step_walk(self.world,slot.source,slot.model,velocity_x,velocity_z,1,blocked);
        if (!result) return result;
        return refresh(slot); // Source refreshes even after ordinary blocking.
    };
    for (std::size_t i=0;i<self.logical_count;++i) {
        auto& first=*self.logical[i]; auto& a=first.source;
        if (!(a.flags&0x200u)) continue;
        const auto first_position=a.collPosition;
        for (std::size_t j=i+1;j<self.logical_count;++j) {
            auto& second=*self.logical[j]; auto& b=second.source;
            if (!(b.flags&0x200u)) continue;
            const auto second_position=b.collPosition;
            const float dx=first_position.x-second_position.x,dy=first_position.y-second_position.y,
                        dz=first_position.z-second_position.z;
            const float separation=std::sqrt((dx*dx+dy*dy)+dz*dz)-(a.collisionRadius+b.collisionRadius);
            if (!(separation<0.0f)) continue;
            const float penetration=-separation;
            auto notified=notify(first,second.object); if (!notified) return notified;
            notified=notify(second,first.object); if (!notified) return notified;
            // Source tests INNER800 twice: inner alone moves BOTH endpoints.
            // Its unreachable inner-only branch is omitted, not "corrected".
            if (b.flags&0x800u) {
                auto result=move(first,first_position,second_position,penetration/2); if (!result) return result;
                result=move(second,second_position,first_position,penetration/2); if (!result) return result;
            } else if (a.flags&0x800u) {
                const auto result=move(first,first_position,second_position,penetration); if (!result) return result;
            }
        }
    }
    return {};
}
SpawnResult Owner::spawn_native(const SpawnRequest& request) noexcept {
    auto& self=*impl_;
    const auto reject=[](OwnerError error,const char* detail) { return SpawnResult{{error,detail},{},nullptr}; };
    if (self.world.retired())
        return reject(OwnerError::admission,"native actor factory cannot enter a retiring World");
    if (!request.block || !request.block->resource || !request.reference || !request.reference->resource || !request.spawn ||
        request.definition>=self.resources.definitions().size() || request.block->index>=self.block_heads.size())
        return reject(OwnerError::admission,"actor factory requires admitted definition, block, reference and spawn");
    bool block_found=false,reference_found=false;
    for (auto& block:self.world.blocks()) { block_found|=&block==request.block; reference_found|=&block==request.reference; }
    if (!block_found || !reference_found) return reject(OwnerError::admission,"actor factory block belongs to another World");
    const auto& definition=self.resources.definitions()[request.definition];
    if (definition.initialization>=self.resources.initializations().size()) return reject(OwnerError::admission,"actor definition has no initialization");
    const auto& initialization=self.resources.initializations()[definition.initialization];
    const auto* admitted=admitted_factory(definition.actor_id,definition.factory_identity);
    if (admitted!=&initialization || !initialized_definition(self.resources,definition))
        return reject(OwnerError::admission,"configured actor factory full tuple/origin is not natively admitted");
    const auto spec=compiled_factory(self.resources.course_id(),definition.actor_id,definition.factory_identity);
    const bool beach=self.course_context.course_id()==0;
    const bool common=beach && definition.actor_id==1003;
    const bool magikarp=beach && definition.actor_id==129;
    const bool ending=beach && definition.actor_id==1001;
    const actor::ActorAnimationHeader* initial_header=initialization.animations_enabled ?
        &self.resources.headers()[initialization.setup.initial_header]:nullptr;
    if (!beach && SourceAccess::course_admission(*this)!=ISLE_GAME_OK)
        return reject(OwnerError::admission,"native factory requires ACTIVE canonical typed Source");
    const bool unwritten=request.spawn==self.transforms_only_spawn;
    const auto* repair=SourceAccess::portability_repair(self,request);
    if (unwritten && self.transforms_only_repair && !repair)
        return reject(OwnerError::admission,"automatic effective fields lost their exact immutable repair authority");
    const bool written_path=!unwritten || (self.transforms_only_written_fields&0x400u) || repair;
    const bool initialized_automatic=request.spawn==self.initialized_automatic_spawn && self.geo_transport &&
        request.automatic_definition==self.initialized_automatic_index &&
        request.definition==self.geo_transport->resource_definition;
    const bool exact_requested=request.automatic_definition!=actor_no_index &&
        self.course_context.course_id()>=1 && self.course_context.course_id()<=5 &&
        ((unwritten && self.transforms_only_requested_id>=0 && self.transforms_only_requested_id==request.spawn->id) ||
         (initialized_automatic && self.initialized_requested_id>=0 && self.initialized_requested_id==request.spawn->id));
    if (request.spawn->id!=static_cast<std::int32_t>(definition.actor_id) && !exact_requested)
        return reject(OwnerError::admission,"requested Source spawn ID differs without its exact admitted automatic origin");
    if (unwritten && (self.transforms_only_definition!=request.definition ||
        self.transforms_only_automatic!=request.automatic_definition ||
        (self.transforms_only_written_fields!=0x3ffu &&
         !((self.course_context.course_id()==2 || (self.course_context.course_id()==3 && repair)) &&
            self.transforms_only_written_fields==0x7ffu)) ||
        (beach && (!magikarp || definition.wire.guest_address!=0x802cbff4u ||
                   self.transforms_only_helper!=0x802cbff4u))))
        return reject(OwnerError::admission,"unwritten optional fields lack their exact issued Source origin");
    if (request.automatic_definition!=actor_no_index &&
        ((!unwritten && !initialized_automatic) || beach || request.automatic_definition>=self.resources.automatic_definitions().size()))
        return reject(OwnerError::admission,"automatic Source definition requires the exact transforms-only helper capability");
    if (written_path && request.spawn->path!=world::no_index && request.spawn->path>=self.world.resources().paths.size())
        return reject(OwnerError::admission,"actor source spawn path outside World");
    if (ending && (!self.ending_services.context || !self.interactions ||
        self.ending_services.validate(self.ending_services.context,&self.ending)!=ISLE_GAME_OK ||
        !self.interactions->admits_ending(initialization)))
        return reject(OwnerError::admission,"ending requires complete native course/callback/resource closure");
    if (!common && !magikarp && !ending) {
        const void* path=!written_path || request.spawn->path==world::no_index ?
            nullptr:&self.world.resources().paths[request.spawn->path];
        if (!self.interactions || !self.interactions->admits_initial_species(initialization,path))
            return reject(OwnerError::admission,"actor native species requires complete native callback/graph/resource/service closure");
    }
    if (self.logical_count==self.logical.size()) return reject(OwnerError::capacity,"source logical actor-list40 exhausted (not a physical slot cap)");
    int registry=-1;
    for (std::size_t i=0;i<self.block_entries.size();++i) if (!self.block_entries[i].slot) { registry=static_cast<int>(i); break; }
    if (registry<0) return reject(OwnerError::capacity,"source block registry100 exhausted");
    const auto& model=self.resources.models()[initialization.model];
    const auto required=menu::animation::slot_capacity(model);
    Impl::Slot* slot=nullptr;
    for (auto& candidate:self.slots) if (!candidate.active && self.capacities[candidate.index].nodes>=required.nodes &&
        self.capacities[candidate.index].material_attachments>=required.material_attachments) { slot=&candidate; break; }
    if (!slot) return reject(OwnerError::capacity,"caller native/Runtime actor capacity exhausted");
    const auto acquired=self.runtime->acquire(slot->index,initialization.model,true);
    if (!acquired) return reject(OwnerError::animation,"actor Runtime instance acquire");
    slot->instance=acquired.handle; slot->active=true; slot->initialization=&initialization; slot->definition=&definition;
    slot->automatic_definition=request.automatic_definition==actor_no_index ?
        nullptr:&self.resources.automatic_definitions()[request.automatic_definition];
    slot->spawn_helper=unwritten?self.transforms_only_helper:initialized_automatic?self.geo_transport->origin:0;
    slot->spawn_written_fields=unwritten?self.transforms_only_written_fields:initialized_automatic?0xfffu:0;
    slot->spawn_repair=repair;
    if (slot->retained) {
        auto& fields=*slot->retained;
        fields.object_mask=0; fields.path_mask=0;
        for (const auto& family:self.retained_families)
            if (family.actor_id==definition.actor_id && family.initialization_identity==initialization.wire.guest_address) {
                fields.object_mask=family.misc_object_mask; fields.path_mask=family.saved_path_mask;
                break;
            }
    }
    bool lifecycle_installed=false;
    const auto rollback=[&]() {
        if (slot->object) {
            if (lifecycle_installed) self.services.erase(slot->object);
            else {
                // This unpublished object has no source record or workers and
                // is not the current object. Require its real synchronous join;
                // Services' own native retirement hooks still unlink its record.
                const auto deleted=isle_game_delete_object(self.services.game,slot->object);
                if (deleted!=ISLE_GAME_OK) self.services.fail("actor unpublished native object rollback");
                else {
                    self.runtime->begin_retire(slot->instance); slot->retiring=true; slot->joined=true; slot->object=nullptr;
                }
            }
        } else {
            self.runtime->begin_retire(slot->instance); slot->retiring=true; slot->joined=true;
        }
    };
    slot->object=self.services.object(request.object_id,3,0x80000000u);
    if (!slot->object) { rollback(); return reject(OwnerError::native,"actor native Services object allocation"); }
    if (!self.services.set_lifecycle(slot->object,{Impl::retiring_callback,Impl::joined_callback,slot})) {
        rollback(); return reject(OwnerError::services,"actor native lifecycle callbacks");
    }
    lifecycle_installed=true;
    slot->render={5,0x80000000u,UINT32_MAX};
    if (!self.services.set_render(slot->object,slot->render)) { rollback(); return reject(OwnerError::services,"actor source initial DL5 link"); }
    if (!menu::model::attach_source_model(self.services.game,slot->object,model,self.runtime->model(slot->instance),
        menu::model::RootRecipe::source,self.scratch,&slot->model)) {
        rollback(); return reject(OwnerError::native,"actor native source tree attachment");
    }
    const auto& block=request.block->resource->metadata.world_position;
    const auto& reference=request.reference->resource->metadata.world_position;
    const auto& spawn=*request.spawn;
    IsleGameModelTransform placement{};
    placement.position.x=spawn.position.x*100.0f+(block.x-reference.x)*100.0f;
    placement.position.z=spawn.position.z*100.0f+(block.z-reference.z)*100.0f;
    world::GroundResult ground{};
    placement.position.y=self.world.ground_at(placement.position.x,placement.position.z,ground)?ground.height:0.0f;
    placement.rotation=spawn.rotation;
    placement.scale={initialization.scale.x*0.1f,initialization.scale.y*0.1f,initialization.scale.z*0.1f};
    if (isle_game_model_set_placement(self.services.game,slot->model,ISLE_GAME_MODEL_PLACEMENT_RTS,&placement)!=ISLE_GAME_OK ||
        isle_actor_source_bind(self.services.game,slot->object,&slot->source,&self.services.rng,slot)!=ISLE_GAME_OK) {
        rollback(); return reject(OwnerError::native,"actor source placement/record binding");
    }
    const auto fields=slot->retained && slot->retained->object_mask ?
        std::span<IsleGameObject** const>(slot->retained->source_fields).first(
            2+static_cast<std::size_t>(std::popcount(slot->retained->object_mask))) :
        std::span<IsleGameObject** const>(slot->source_fields).first(ending?3u:2u);
    if (!self.services.source_owner_register(slot->object,fields)) {
        rollback(); return reject(OwnerError::services,"actor semantic source owner registration");
    }
    slot->source_owned=true;
    if (!self.services.source_commands_native(slot->object) ||
        isle_actor_source_install_update(self.services.game,slot->object)!=ISLE_GAME_OK) {
        rollback(); return reject(OwnerError::native,"actor direct C update/command ownership");
    }
    auto& source=slot->source;
    source.initData=&initialization; source.animSetup=&initialization.setup;
    source.pos1=placement.position; source.targetPos=placement.position; source.euler=placement.rotation;
    source.collisionOffset={initialization.collision_center.x/initialization.scale.x,
        initialization.collision_center.y/initialization.scale.y,initialization.collision_center.z/initialization.scale.z};
    source.collisionRadius=initialization.radius*initialization.scale.y;
    source.flags=initialization.flags; source.id=static_cast<std::int32_t>(definition.actor_id); source.tangible=1;
    source.baseBlock=request.block; source.forbiddenGround=nullptr; source.loopCount=0;
    source.playerDist=maximum_distance; source.interactionDist=maximum_distance;
    IsleGameProcess* process=nullptr;
    if (isle_game_add_source_process(self.services.game,slot->object,isle_actor_source_animation_process,ISLE_GAME_SYNCHRONOUS,3,&process)!=ISLE_GAME_OK) {
        rollback(); return reject(OwnerError::native,"actor animation priority3 process");
    }
    // pokemon.c2194–2195 creates this process even for NULL: the original
    // ForceAnimation(NULL) path pauses it and clears the source animation fields.
    const auto animated=self.animation(*slot,initial_header,0,true);
    if (!animated) { rollback(); return {animated,{},nullptr}; }
    const IsleGameSourceCallback initial_state=spec.initial_state;
    if (isle_game_add_source_process(self.services.game,slot->object,isle_actor_source_distance_process,ISLE_GAME_SYNCHRONOUS,8,&process)!=ISLE_GAME_OK ||
        isle_game_add_source_process(self.services.game,slot->object,initial_state,ISLE_GAME_STACKFUL,7,&source.stateProc)!=ISLE_GAME_OK) {
        rollback(); return reject(OwnerError::native,"actor distance/initial state process creation");
    }
    // Late factory resets occur only AFTER forced header and immediate events.
    source.unk_18=-1; source.processFlags=0; source.counter=0; source.pathProc=nullptr;
    source.transitionGraph=nullptr; source.pokemonLoopTarget=0; source.lastAnimationFrame=0;
    source.jumpVel=0; source.hSpeed=0; source.facingYaw=0;
    source.transitionGraph=nullptr; source.unk_E4=255;
    if (written_path && spawn.path!=world::no_index) source.path=&self.world.resources().paths[spawn.path];
    else if (repair) source.path=nullptr;
    if (!unwritten || repair) source.behavior=spawn.behavior;
    source.pathParam=0; source.eggGeo=nullptr; source.poseID=0; source.specialPoseID=0;
    source.playerDist=maximum_distance;
    self.logical[self.logical_count++]=slot; slot->logical=true;
    auto& entry=self.block_entries[static_cast<std::size_t>(registry)];
    entry.slot=slot; entry.next=self.block_heads[request.block->index]; self.block_heads[request.block->index]=registry; slot->registered=true;
    if (self.transform_cache) --self.transform_cache;
    if (self.pokemon_cache) --self.pokemon_cache;
    slot->insertion=++self.insertion;
    return {{},slot->instance,slot->object};
}
OwnerStatus Owner::set_animation(menu::animation::InstanceHandle h,const actor::ActorAnimationHeader* header,float start,bool force) noexcept {
    auto* slot=impl_->find(h); return slot?impl_->animation(*slot,header,start,force):rejected(OwnerError::lifetime,"stale actor animation handle");
}
OwnerStatus Owner::step_animation(menu::animation::InstanceHandle h) noexcept {
    auto* slot=impl_->find(h); return slot?impl_->step(*slot):rejected(OwnerError::lifetime,"stale actor step handle");
}
OwnerStatus Owner::set_animation_speed(menu::animation::InstanceHandle h,float speed) noexcept {
    auto& self=*impl_;
    if (self.services.failed()) return rejected(OwnerError::services,"actor speed requires healthy native Services");
    auto* slot=self.find(h);
    if (!slot || !slot->source_owned || SourceAccess::slot(&slot->source)!=slot)
        return rejected(OwnerError::lifetime,"actor speed requires the current canonical model incarnation");
    // This is animSetModelTreeSpeed, not a header replacement, a step, or a pause.
    // Runtime updates only existing node-clock speeds; time/phase/header/process
    // suspension and cold execution/material capacities remain untouched.
    const auto result=self.runtime->set_model_speed(h,speed);
    return result ? OwnerStatus{} : rejected(OwnerError::animation,"actor model speed rejected");
}
OwnerStatus Owner::set_local(menu::animation::InstanceHandle h,std::size_t node,const menu::animation::Transform& pose) noexcept {
    auto* slot=impl_->find(h); return slot?impl_->local(*slot,node,pose):rejected(OwnerError::lifetime,"stale actor local handle");
}
OwnerStatus Owner::set_placement(menu::animation::InstanceHandle h,const IsleGameModelTransform& placement) noexcept {
    auto* slot=impl_->find(h);
    if (!slot) return rejected(OwnerError::lifetime,"stale actor placement handle");
    return isle_game_model_set_placement(impl_->services.game,slot->model,ISLE_GAME_MODEL_PLACEMENT_RTS,&placement)==ISLE_GAME_OK?
        OwnerStatus{}:rejected(OwnerError::native,"actor placement publication");
}
OwnerStatus Owner::trigger_common1003(menu::animation::InstanceHandle h,menu::Vec3 position) noexcept {
    auto* slot=impl_->find(h);
    if (!slot || slot->source.id!=1003) return rejected(OwnerError::admission,"common1003 trigger requires actual live registered actor");
    IsleGameVec3 native_position{position.x,position.y,position.z};
    const auto result=func_8035E264_4FE674(slot->object,&native_position);
    return result==ISLE_GAME_OK?OwnerStatus{}:rejected(OwnerError::native,"actual common1003 C trigger placement/signal");
}
OwnerStatus Owner::cleanup(menu::animation::InstanceHandle h) noexcept {
    auto* slot=impl_->find(h); return slot?impl_->cleanup(*slot):rejected(OwnerError::lifetime,"stale actor cleanup handle");
}
OwnerStatus Owner::notify_target_death(IsleGameObject* sender) noexcept {
    return impl_->notify_target_death(sender);
}
OwnerStatus Owner::request_cleanup(menu::animation::InstanceHandle h) noexcept {
    auto* slot=impl_->find(h);
    if (!slot) return rejected(OwnerError::lifetime,"stale actor cleanup request");
    IsleGameProcess* process=nullptr;
    return isle_game_add_source_process(impl_->services.game,slot->object,Pokemon_Cleanup,ISLE_GAME_SYNCHRONOUS,4,&process)==ISLE_GAME_OK?
        OwnerStatus{}:rejected(OwnerError::native,"actor source cleanup priority4 process");
}
OwnerStatus Owner::bind_animation_callback(IsleTunnelActorSource& source,IsleGameObject* object,
    IsleTunnelAnimationCallback function) noexcept {
    auto& self=*impl_;
    if (self.course_context.course_id()!=1 || &self.course_context.tunnel().actors!=&source ||
        SourceAccess::typed_admission(&source,self.services.game,source.services)!=ISLE_GAME_OK)
        return rejected(OwnerError::admission,"animation callback requires the exact active typed course Source");
    bool admitted=!function;
    for (const auto& binding:tunnel::actor_animation_callbacks()) admitted |= binding.function==function;
    if (!admitted) return rejected(OwnerError::admission,"animation callback is not an actual compiled typed Original");
    for (auto& slot:self.slots) if (slot.active && !slot.retiring && slot.object==object &&
        slot.source_owned && self.services.source_owner_registered(object) &&
        isle_game_model_validate_binding(self.services.game,object,slot.model)==ISLE_GAME_OK) {
        if (function) slot.animation_callback=TunnelAnimationBinding{&source,function};
        else slot.animation_callback=std::monostate{};
        return {};
    }
    return rejected(OwnerError::lifetime,"animation callback receiver is not this Owner's canonical live Actor incarnation");
}
OwnerStatus Owner::bind_animation_callback(IsleVolcanoActorSource& source,IsleGameObject* object,
    IsleVolcanoAnimationCallback function) noexcept {
    auto& self=*impl_;
    if (self.course_context.course_id()!=2 || &self.course_context.volcano().actors!=&source ||
        SourceAccess::typed_admission(&source,self.services.game,source.services)!=ISLE_GAME_OK)
        return rejected(OwnerError::admission,"animation callback requires the exact active typed course Source");
    bool admitted=!function;
    for (const auto& binding:volcano::actor_animation_callbacks()) admitted |= binding.function==function;
    if (!admitted) return rejected(OwnerError::admission,"animation callback is not an actual compiled typed Original");
    for (auto& slot:self.slots) if (slot.active && !slot.retiring && slot.object==object &&
        slot.source_owned && self.services.source_owner_registered(object) &&
        isle_game_model_validate_binding(self.services.game,object,slot.model)==ISLE_GAME_OK) {
        if (function) slot.animation_callback=VolcanoAnimationBinding{&source,function};
        else slot.animation_callback=std::monostate{};
        return {};
    }
    return rejected(OwnerError::lifetime,"animation callback receiver is not this Owner's canonical live Actor incarnation");
}
OwnerStatus Owner::bind_animation_callback(IsleRiverActorSource& source,IsleGameObject* object,
    IsleRiverAnimationCallback function) noexcept {
    auto& self=*impl_;
    if (self.course_context.course_id()!=3 || &self.course_context.river().actors!=&source ||
        SourceAccess::typed_admission(&source,self.services.game,source.services)!=ISLE_GAME_OK)
        return rejected(OwnerError::admission,"animation callback requires the exact active typed course Source");
    bool admitted=!function;
    for (const auto& binding:river::actor_animation_callbacks()) admitted |= binding.function==function;
    if (!admitted) return rejected(OwnerError::admission,"animation callback is not an actual compiled typed Original");
    for (auto& slot:self.slots) if (slot.active && !slot.retiring && slot.object==object &&
        slot.source_owned && self.services.source_owner_registered(object) &&
        isle_game_model_validate_binding(self.services.game,object,slot.model)==ISLE_GAME_OK) {
        if (function) slot.animation_callback=RiverAnimationBinding{&source,function};
        else slot.animation_callback=std::monostate{};
        return {};
    }
    return rejected(OwnerError::lifetime,"animation callback receiver is not this Owner's canonical live Actor incarnation");
}
OwnerStatus Owner::bind_animation_callback(IsleCaveActorSource& source,IsleGameObject* object,
    IsleCaveAnimationCallback function) noexcept {
    auto& self=*impl_;
    if (self.course_context.course_id()!=4 || &self.course_context.cave().actors!=&source ||
        SourceAccess::typed_admission(&source,self.services.game,source.services)!=ISLE_GAME_OK)
        return rejected(OwnerError::admission,"animation callback requires the exact active typed course Source");
    bool admitted=!function;
    for (const auto& binding:cave::actor_animation_callbacks()) admitted |= binding.function==function;
    if (!admitted) return rejected(OwnerError::admission,"animation callback is not an actual compiled typed Original");
    for (auto& slot:self.slots) if (slot.active && !slot.retiring && slot.object==object &&
        slot.source_owned && self.services.source_owner_registered(object) &&
        isle_game_model_validate_binding(self.services.game,object,slot.model)==ISLE_GAME_OK) {
        if (function) slot.animation_callback=CaveAnimationBinding{&source,function};
        else slot.animation_callback=std::monostate{};
        return {};
    }
    return rejected(OwnerError::lifetime,"animation callback receiver is not this Owner's canonical live Actor incarnation");
}
OwnerStatus Owner::bind_animation_callback(IsleValleyActorSource& source,IsleGameObject* object,
    IsleValleyAnimationCallback function) noexcept {
    auto& self=*impl_;
    if (self.course_context.course_id()!=5 || &self.course_context.valley().actors!=&source ||
        SourceAccess::typed_admission(&source,self.services.game,source.services)!=ISLE_GAME_OK)
        return rejected(OwnerError::admission,"animation callback requires the exact active typed course Source");
    bool admitted=!function;
    for (const auto& binding:valley::actor_animation_callbacks()) admitted |= binding.function==function;
    if (!admitted) return rejected(OwnerError::admission,"animation callback is not an actual compiled typed Original");
    for (auto& slot:self.slots) if (slot.active && !slot.retiring && slot.object==object &&
        slot.source_owned && self.services.source_owner_registered(object) &&
        isle_game_model_validate_binding(self.services.game,object,slot.model)==ISLE_GAME_OK) {
        if (function) slot.animation_callback=ValleyAnimationBinding{&source,function};
        else slot.animation_callback=std::monostate{};
        return {};
    }
    return rejected(OwnerError::lifetime,"animation callback receiver is not this Owner's canonical live Actor incarnation");
}
OwnerStatus Owner::change_block_source(menu::animation::InstanceHandle handle,std::uint32_t identity,
    menu::Vec3 previous,menu::Vec3 current) noexcept {
    auto* slot=impl_->find(handle);
    if (!slot || !slot->registered || !slot->definition ||
        slot->definition->change_block_identity!=identity)
        return rejected(OwnerError::admission,"block callback differs from canonical definition");
    IsleActorBlockChange function=nullptr;
    switch (impl_->course_context.course_id()) {
    case 2:
        for (const auto& binding:volcano::actor_block_change_callbacks())
            if (binding.identity==identity) function=binding.function;
        break;
    case 5:
        for (const auto& binding:valley::actor_block_change_callbacks())
            if (binding.identity==identity) function=binding.function;
        break;
    }
    if (!function) {
        if (identity==0x80363dbcu) return change_block(handle,previous,current);
        if (identity==0x80363eb4u) return change_block_ground(handle,previous,current);
        return rejected(OwnerError::admission,"block callback lacks an exact compiled Source binding");
    }
    if (SourceAccess::course_admission(*this)!=ISLE_GAME_OK)
        return rejected(OwnerError::admission,"custom block callback requires complete active typed Source");
    IsleCourseActorBase* source=nullptr;
    switch (impl_->course_context.course_id()) {
    case 2: source=&impl_->course_context.volcano().actors.base; break;
    case 5: source=&impl_->course_context.valley().actors.base; break;
    }
    const auto result=isle_actor_source_block_callback_call(impl_->services.game,source,function,slot->object,
        previous.x,previous.y,previous.z,current.x,current.y,current.z);
    return result==ISLE_GAME_OK ? OwnerStatus{} : rejected(OwnerError::native,"original typed block callback failed");
}
OwnerStatus Owner::change_block(menu::animation::InstanceHandle h,menu::Vec3 previous,menu::Vec3 current) noexcept {
    auto* slot=impl_->find(h);
    if (!slot || !slot->registered) return rejected(OwnerError::lifetime,"unregistered actor block change");
    return isle_meowth_source_call_change_block(slot->object,previous.x,previous.y,previous.z,
        current.x,current.y,current.z,0)==ISLE_GAME_OK?OwnerStatus{}:
        rejected(OwnerError::native,"original nonground block change failed");
}
OwnerStatus Owner::change_block_ground(menu::animation::InstanceHandle h,menu::Vec3 previous,menu::Vec3 current) noexcept {
    auto* slot=impl_->find(h);
    if (!slot || !slot->registered) return rejected(OwnerError::lifetime,"unregistered actor ground block change");
    return isle_meowth_source_call_change_block(slot->object,previous.x,previous.y,previous.z,
        current.x,current.y,current.z,1)==ISLE_GAME_OK?OwnerStatus{}:
        rejected(OwnerError::native,"original ground block change failed");
}
OwnerStatus Owner::remove_block(world::Block& block) noexcept {
    if (block.index>=impl_->block_heads.size()) return rejected(OwnerError::admission,"actor removal block index");
    if (&impl_->world.blocks()[block.index]!=&block) return rejected(OwnerError::admission,"actor removal block belongs to another World");
    for (int i=impl_->block_heads[block.index];i!=-1;) {
        auto& entry=impl_->block_entries[static_cast<std::size_t>(i)]; const int next=entry.next;
        if (entry.slot) { auto result=request_cleanup(entry.slot->instance); if (!result) return result; }
        i=next;
    }
    return {};
}
OwnerStatus Owner::collect_joined() noexcept {
    for (auto& slot:impl_->slots) if (slot.active && slot.retiring) {
        if (!slot.joined || (slot.source_owned && !impl_->services.source_owner_collectable(slot.object)))
            return rejected(OwnerError::lifetime,"actor real native join/source identity borrows still outstanding");
        if (slot.object) impl_->sound.release_identity(slot.object);
        if (slot.source_owned) {
            if (!impl_->services.source_owner_release(slot.object))
                return rejected(OwnerError::native,"actor joined source identity release requires outside-OM collection");
            slot.source_owned=false;
        }
        if (!impl_->runtime->release_retired(slot.instance)) return rejected(OwnerError::animation,"actor physical Runtime release");
        const auto index=slot.index;
        auto retained=std::move(slot.retained);
        const auto fields=slot.source_fields;
        if (retained) {
            for (auto& path:retained->paths) path={};
            retained->object_mask=0; retained->path_mask=0;
        }
        slot=Impl::Slot{}; slot.owner=impl_.get(); slot.index=index;
        slot.source_fields=fields; slot.retained=std::move(retained);
    }
    return {};
}
OwnerStatus Owner::reset() noexcept {
    auto& self=*impl_;
    if (self.services.failed() || isle_game_status(self.services.game)!=ISLE_GAME_OK) {
        for (auto& slot:self.slots) if (slot.active && !slot.retiring) self.services.erase(slot.object);
        const auto collected=collect_joined();
        if (!collected) return collected;
        self.fx.reset();
        self.interactions=nullptr;
        return rejected(OwnerError::services,"failed native scene requires application reconstruction, not successful actor reset");
    }
    for (auto& slot:self.slots) if (slot.active && !slot.retiring) {
        auto result=self.cleanup(slot); if (!result) return result;
    }
    auto result=collect_joined(); if (!result) return result;
    self.fx.reset();
    self.interactions=nullptr;
    return {};
}
IsleActorSource* Owner::source(menu::animation::InstanceHandle h) noexcept { auto* s=impl_->find(h); return s?&s->source:nullptr; }
const IsleActorSource* Owner::source(menu::animation::InstanceHandle h) const noexcept { const auto* s=impl_->find(h); return s?&s->source:nullptr; }
IsleGameObject* Owner::object(menu::animation::InstanceHandle h) const noexcept { auto* s=impl_->find(h); return s?s->object:nullptr; }
IsleGameModel* Owner::native_model(menu::animation::InstanceHandle h) const noexcept { auto* s=impl_->find(h); return s?s->model:nullptr; }
menu::animation::InstanceHandle Owner::instance(IsleGameObject* object) const noexcept {
    if (!object) return {};
    for (const auto& slot:impl_->slots)
        if (slot.active && !slot.retiring && slot.registered && slot.object==object)
            return slot.instance;
    return {};
}
bool Owner::registered(menu::animation::InstanceHandle h) const noexcept { auto* s=impl_->find(h); return s && s->registered; }
Membership Owner::membership(menu::animation::InstanceHandle h) const noexcept {
    const auto* s=impl_->find(h,true);
    return {s && s->logical,s && s->registered,s && s->retiring,s && s->joined,impl_->transform_cache,impl_->pokemon_cache,
        s?s->source.processFlags:0u,s?s->source.loopCount:0};
}
std::span<const ActorRenderBinding> Owner::render_bindings() noexcept {
    auto& self=*impl_; std::size_t count=0;
    for (auto& slot:self.slots) if (slot.active && !slot.retiring) {
        ActorRenderBinding binding{{slot.index,self.runtime->model(slot.instance),slot.object,slot.model,menu::model::PoseSource::animation,
            menu::model::RootRecipe::source,true,self.runtime.get()},slot.render};
        std::size_t at=count;
        while (at>0) {
            const auto& before=self.bindings[at-1]; const auto& old=self.slots[before.model.slot];
            const bool earlier=slot.render.link<old.render.link || (slot.render.link==old.render.link &&
                (slot.render.priority>old.render.priority || (slot.render.priority==old.render.priority && slot.insertion<old.insertion)));
            if (!earlier) break;
            self.bindings[at]=before; --at;
        }
        self.bindings[at]=binding; ++count;
    }
    for (std::size_t i=count;i<self.bindings.size();++i) self.bindings[i]={};
    return self.bindings;
}
menu::animation::Runtime& Owner::animation() noexcept { return *impl_->runtime; }
const menu::animation::Runtime& Owner::animation() const noexcept { return *impl_->runtime; }
menu::EffectRuntime& Owner::effects() noexcept { return impl_->fx; }
const environment_sound::Environment& Owner::environment() const noexcept { return impl_->sound; }
IsleGameObject* Owner::particle_object() const noexcept { return impl_->particle_object; }
IsleGameObject* Owner::effect_object() const noexcept { return impl_->effect_object; }
const IsleBeachEndingSource& Owner::ending_source() const noexcept { return impl_->ending; }
OwnerStatus Owner::update_particles() noexcept { return impl_->fx.update_particles()?OwnerStatus{}:rejected(OwnerError::services,"actor particles update"); }
OwnerStatus Owner::update_effects() noexcept { return impl_->fx.update_effects(Impl::resolve,impl_.get())?OwnerStatus{}:rejected(OwnerError::services,"actor effects update"); }
OwnerStatus Owner::bind_interactions(InteractionServices& context) noexcept {
    auto& self=*impl_;
    if (self.interactions || !context.matches(*this,self.services,self.resources,self.world,self.detector))
        return rejected(OwnerError::admission,"interaction services require this exact unbound source owner");
    for (const auto& slot:self.slots) if (slot.active)
        return rejected(OwnerError::lifetime,"interaction services must bind before native actor spawning");
    self.interactions=&context;
    return {};
}
IslePlayerSource& Owner::player_source() noexcept { return impl_->player; }
const IslePlayerSource& Owner::player_source() const noexcept { return impl_->player; }
const CourseContext& Owner::course_context() const noexcept { return impl_->course_context; }
} // namespace isle::actor

extern "C" {
IsleGameResult isle_tunnel_actor_native_context(IsleGame* game,IsleGameObject* object,IsleTunnelActorSource** output) {
    return isle::actor::SourceAccess::typed_context(game,object,output,nullptr,true);
}
IsleGameResult isle_tunnel_actor_native_admission(const IsleTunnelActorSource* source,IsleGame* game,const IsleTunnelActorServices* services) {
    return isle::actor::SourceAccess::typed_admission(source,game,services);
}
IsleGameResult isle_tunnel_actor_native_retirement(IsleTunnelActorSource* source) {
    return isle::actor::SourceAccess::typed_retirement(source);
}
IsleGameResult isle_tunnel_actor_native_factory_context(const IsleActorFactoryBlock* block,
    const IsleActorFactoryBlock* reference,const IsleActorFactorySpawn* spawn,IsleTunnelActorSource** output) {
    return isle::actor::SourceAccess::typed_factory_context(block,reference,spawn,output);
}
IsleGameResult isle_volcano_actor_native_context(IsleGame* game,IsleGameObject* object,IsleVolcanoActorSource** output) {
    return isle::actor::SourceAccess::typed_context(game,object,output,nullptr,true);
}
IsleGameResult isle_volcano_actor_native_admission(const IsleVolcanoActorSource* source,IsleGame* game,const IsleVolcanoActorServices* services) {
    return isle::actor::SourceAccess::typed_admission(source,game,services);
}
IsleGameResult isle_volcano_actor_native_retirement(IsleVolcanoActorSource* source) {
    return isle::actor::SourceAccess::typed_retirement(source);
}
IsleGameResult isle_volcano_actor_native_factory_context(const IsleActorFactoryBlock* block,
    const IsleActorFactoryBlock* reference,const IsleActorFactorySpawn* spawn,IsleVolcanoActorSource** output) {
    return isle::actor::SourceAccess::typed_factory_context(block,reference,spawn,output);
}
IsleGameResult isle_river_actor_native_context(IsleGame* game,IsleGameObject* object,IsleRiverActorSource** output) {
    return isle::actor::SourceAccess::typed_context(game,object,output,nullptr,true);
}
IsleGameResult isle_river_actor_native_admission(const IsleRiverActorSource* source,IsleGame* game,const IsleRiverActorServices* services) {
    return isle::actor::SourceAccess::typed_admission(source,game,services);
}
IsleGameResult isle_river_actor_native_retirement(IsleRiverActorSource* source) {
    return isle::actor::SourceAccess::typed_retirement(source);
}
IsleGameResult isle_river_actor_native_factory_context(const IsleActorFactoryBlock* block,
    const IsleActorFactoryBlock* reference,const IsleActorFactorySpawn* spawn,IsleRiverActorSource** output) {
    return isle::actor::SourceAccess::typed_factory_context(block,reference,spawn,output);
}
IsleGameResult isle_cave_actor_native_context(IsleGame* game,IsleGameObject* object,IsleCaveActorSource** output) {
    return isle::actor::SourceAccess::typed_context(game,object,output,nullptr,true);
}
IsleGameResult isle_cave_actor_native_admission(const IsleCaveActorSource* source,IsleGame* game,const IsleCaveActorServices* services) {
    return isle::actor::SourceAccess::typed_admission(source,game,services);
}
IsleGameResult isle_cave_actor_native_retirement(IsleCaveActorSource* source) {
    return isle::actor::SourceAccess::typed_retirement(source);
}
IsleGameResult isle_cave_actor_native_factory_context(const IsleActorFactoryBlock* block,
    const IsleActorFactoryBlock* reference,const IsleActorFactorySpawn* spawn,IsleCaveActorSource** output) {
    return isle::actor::SourceAccess::typed_factory_context(block,reference,spawn,output);
}
IsleGameResult isle_valley_actor_native_context(IsleGame* game,IsleGameObject* object,IsleValleyActorSource** output) {
    return isle::actor::SourceAccess::typed_context(game,object,output,nullptr,true);
}
IsleGameResult isle_valley_actor_native_admission(const IsleValleyActorSource* source,IsleGame* game,const IsleValleyActorServices* services) {
    return isle::actor::SourceAccess::typed_admission(source,game,services);
}
IsleGameResult isle_valley_actor_native_retirement(IsleValleyActorSource* source) {
    return isle::actor::SourceAccess::typed_retirement(source);
}
IsleGameResult isle_valley_actor_native_factory_context(const IsleActorFactoryBlock* block,
    const IsleActorFactoryBlock* reference,const IsleActorFactorySpawn* spawn,IsleValleyActorSource** output) {
    return isle::actor::SourceAccess::typed_factory_context(block,reference,spawn,output);
}
IsleGameResult isle_actor_owner_factory_game(const IsleActorFactoryBlock* block,
    const IsleActorFactoryBlock* reference,const IsleActorFactorySpawn* spawn,IsleGame** output) {
    return isle::actor::SourceAccess::factory_game(block,reference,spawn,output);
}
IsleGameResult isle_rainbow_actor_native_context(IsleGame* game,IsleRainbowActorSource** output) {
    return isle::actor::SourceAccess::rainbow_context(game,output);
}
IsleGameResult isle_rainbow_actor_native_admission(IsleRainbowActorSource* source) {
    return isle::actor::SourceAccess::rainbow_admission(source);
}
IsleGameObject* isle_actor_owner_spawn_ground_dl4(int32_t object_id,uint16_t id,
    IsleActorFactoryBlock* block,IsleActorFactoryBlock* reference,IsleActorFactorySpawn* spawn,
    const IsleActorFactoryInitialization* initialization) {
    return isle::actor::SourceAccess::factory(object_id,id,block,reference,spawn,initialization,
        isle::actor::FactoryWrapper::ground_dl4);
}
IsleGameObject* isle_actor_owner_spawn_ground(int32_t object_id,uint16_t id,
    IsleActorFactoryBlock* block,IsleActorFactoryBlock* reference,IsleActorFactorySpawn* spawn,
    const IsleActorFactoryInitialization* initialization) {
    return isle::actor::SourceAccess::factory(object_id,id,block,reference,spawn,initialization,
        isle::actor::FactoryWrapper::ground);
}
IsleGameObject* isle_actor_owner_spawn(int32_t object_id,uint16_t id,
    IsleActorFactoryBlock* block,IsleActorFactoryBlock* reference,IsleActorFactorySpawn* spawn,
    const IsleActorFactoryInitialization* initialization) {
    return isle::actor::SourceAccess::factory(object_id,id,block,reference,spawn,initialization,
        isle::actor::FactoryWrapper::ordinary);
}
IsleGameResult isle_actor_owner_animation(IsleActorSource* source,uint32_t identity,float start,int force) {
    auto* slot=isle::actor::SourceAccess::slot(source);
    if (!slot) return ISLE_GAME_INVALID_STATE;
    const isle::actor::ActorAnimationHeader* header=nullptr;
    if (identity) {
        for (const auto& candidate:slot->owner->resources.headers())
            if (candidate.wire.guest_address==identity && candidate.initialization==slot->definition->initialization) {
                header=&candidate; break;
            }
        if (!header) return ISLE_GAME_INVALID_ARGUMENT;
    }
    return slot->owner->animation(*slot,header,start,force!=0)?ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE;
}
IsleGameResult isle_actor_owner_environment_sound(IsleActorSource* source,uint8_t category,int32_t sound) {
    auto* slot=isle::actor::SourceAccess::slot(source);
    if (!slot || slot->owner->services.failed()) return ISLE_GAME_INVALID_STATE;
    return slot->owner->sound.play({slot->object,slot->model,&source->playerDist},category,sound)?
        ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE;
}
IsleGameResult isle_actor_owner_root_position_store(IsleActorSource* source,const IsleGameModelRef* reference,
    const IsleGameVec3* position) {
    if (!reference || !position) return ISLE_GAME_INVALID_ARGUMENT;
    auto* slot=isle::actor::SourceAccess::slot(source);
    if (!slot || slot->owner->services.failed()) return ISLE_GAME_INVALID_STATE;
    IsleGameModel* model=nullptr;
    const auto resolved=isle_game_model_resolve(source->game,reference,&model);
    if (resolved!=ISLE_GAME_OK) return resolved;
    if (reference->object!=slot->object || model!=slot->model) return ISLE_GAME_INVALID_ARGUMENT;
    const IsleGameModelNode* nodes=nullptr;
    std::size_t count=0;
    const auto read=isle_game_model_peek_nodes(source->game,model,&nodes,&count);
    if (read!=ISLE_GAME_OK) return read;
    if (!count || nodes[0].parent!=UINT32_MAX) return ISLE_GAME_INVALID_STATE;
    const auto& current=nodes[0].transform;
    isle::menu::animation::Transform pose{{position->x,position->y,position->z},
        {current.rotation.x,current.rotation.y,current.rotation.z},
        {current.scale.x,current.scale.y,current.scale.z}};
    return isle::actor::SourceAccess::collision_status(slot->owner->local(*slot,0,pose));
}
IsleGameResult isle_actor_owner_self_model(IsleActorSource* source,IsleGameModelRef* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output={};
    auto* slot=isle::actor::SourceAccess::slot(source);
    if (!slot) return ISLE_GAME_INVALID_STATE;
    return isle_game_model_reference(source->game,slot->object,slot->model,output);
}
IsleGameResult isle_actor_owner_aux_read(IsleActorSource* source,const IsleGameModelRef* reference,
    const IsleGameModelTransform** output) {
    if (output) *output=nullptr;
    if (!output || !reference) return ISLE_GAME_INVALID_ARGUMENT;
    const IsleGameModelPlacement* placement=nullptr;
    const auto result=isle::actor::SourceAccess::auxiliary(source,reference,placement);
    if (result!=ISLE_GAME_OK) return result;
    *output=&placement->transform;
    return ISLE_GAME_OK;
}
IsleGameResult isle_actor_owner_aux_store(IsleActorSource* source,const IsleGameModelRef* reference,
    uint32_t mask,const IsleGameModelTransform* values) {
    if (!values || (mask&~0x1ffu)) return ISLE_GAME_INVALID_ARGUMENT;
    const IsleGameModelPlacement* placement=nullptr;
    const auto result=isle::actor::SourceAccess::auxiliary(source,reference,placement);
    if (result!=ISLE_GAME_OK) return result;
    const auto* latest=&placement->transform;
    if ((mask&(ISLE_ACTOR_SX|ISLE_ACTOR_SY|ISLE_ACTOR_SZ)) &&
        placement->mode!=ISLE_GAME_MODEL_PLACEMENT_RTS) return ISLE_GAME_INVALID_STATE;
    if (!mask) return ISLE_GAME_OK;
    // RT does not consume scale. Never load unselected caller components.
    IsleGameModelTransform next{};
    next.position=latest->position;
    next.rotation=latest->rotation;
    if (placement->mode==ISLE_GAME_MODEL_PLACEMENT_RTS) next.scale=latest->scale;
    if (mask&ISLE_ACTOR_PX) next.position.x=values->position.x;
    if (mask&ISLE_ACTOR_PY) next.position.y=values->position.y;
    if (mask&ISLE_ACTOR_PZ) next.position.z=values->position.z;
    if (mask&ISLE_ACTOR_RX) next.rotation.x=values->rotation.x;
    if (mask&ISLE_ACTOR_RY) next.rotation.y=values->rotation.y;
    if (mask&ISLE_ACTOR_RZ) next.rotation.z=values->rotation.z;
    if (mask&ISLE_ACTOR_SX) next.scale.x=values->scale.x;
    if (mask&ISLE_ACTOR_SY) next.scale.y=values->scale.y;
    if (mask&ISLE_ACTOR_SZ) next.scale.z=values->scale.z;
    return isle_game_model_set_placement(source->game,reference->model,placement->mode,&next);
}
IsleGameResult isle_actor_owner_step_walk(IsleActorSource* source,float vx,float vz,uint32_t flags,int32_t* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    auto* slot=isle::actor::SourceAccess::slot(source);
    if (!slot || slot->owner->services.failed()) return ISLE_GAME_INVALID_STATE;
    bool blocked=false;
    const auto result=isle::actor::collision::step_walk(slot->owner->world,*source,slot->model,vx,vz,flags,blocked);
    if (!result) return isle::actor::SourceAccess::collision_status(result);
    *output=blocked?1:0;
    return ISLE_GAME_OK;
}
IsleGameResult isle_actor_owner_scale_initialization(IsleActorSource* source,IsleActorScaleInitialization* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    auto* slot=isle::actor::SourceAccess::slot(source);
    if (!slot || source->initData!=slot->initialization) return ISLE_GAME_INVALID_STATE;
    for (const auto& candidate:slot->owner->resources.initializations()) if (&candidate==slot->initialization) {
        *output={{candidate.scale.x,candidate.scale.y,candidate.scale.z},
            {candidate.collision_center.x,candidate.collision_center.y,candidate.collision_center.z},candidate.radius};
        return ISLE_GAME_OK;
    }
    return ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_actor_owner_misc_object_get(IsleActorSource* source,uint32_t index,IsleGameObject** output) {
    if (output) *output=nullptr;
    if (!output || index>=isle::actor::retained_misc_count) return ISLE_GAME_INVALID_ARGUMENT;
    auto* slot=isle::actor::SourceAccess::retained_slot(source,index,true);
    if (!slot) return ISLE_GAME_INVALID_STATE;
    const auto& fields=*slot->retained;
    auto* value=fields.objects[isle::actor::SourceAccess::retained_index(fields.object_mask,index)];
    if (value && !slot->owner->services.source_owner_registered(value)) return ISLE_GAME_INVALID_STATE;
    *output=value;
    return ISLE_GAME_OK;
}
IsleGameResult isle_actor_owner_misc_object_set(IsleActorSource* source,uint32_t index,IsleGameObject* value) {
    if (index>=isle::actor::retained_misc_count) return ISLE_GAME_INVALID_ARGUMENT;
    auto* slot=isle::actor::SourceAccess::retained_slot(source,index,true);
    if (!slot) return ISLE_GAME_INVALID_STATE;
    if (value && !slot->owner->services.source_owner_registered(value)) return ISLE_GAME_INVALID_STATE;
    auto& fields=*slot->retained;
    auto** field=&fields.objects[isle::actor::SourceAccess::retained_index(fields.object_mask,index)];
    return slot->owner->services.source_reference_assign(slot->object,field,value)?
        ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_actor_owner_saved_path_save(IsleActorSource* source,uint32_t index) {
    if (index>=isle::actor::retained_misc_count) return ISLE_GAME_INVALID_ARGUMENT;
    auto* slot=isle::actor::SourceAccess::retained_slot(source,index,false);
    if (!slot || slot->owner->world.retired() || !slot->owner->world.error().empty()) return ISLE_GAME_INVALID_STATE;
    const auto* path=source->path?isle::actor::SourceAccess::path(*slot,source->path):nullptr;
    if (source->path && !path) return ISLE_GAME_INVALID_ARGUMENT;
    auto& fields=*slot->retained;
    fields.paths[isle::actor::SourceAccess::retained_index(fields.path_mask,index)]={path,true};
    return ISLE_GAME_OK;
}
IsleGameResult isle_actor_owner_saved_path_restore(IsleActorSource* source,uint32_t index) {
    if (index>=isle::actor::retained_misc_count) return ISLE_GAME_INVALID_ARGUMENT;
    auto* slot=isle::actor::SourceAccess::retained_slot(source,index,false);
    if (!slot || slot->owner->world.retired() || !slot->owner->world.error().empty()) return ISLE_GAME_INVALID_STATE;
    const auto& fields=*slot->retained;
    const auto& saved=fields.paths[isle::actor::SourceAccess::retained_index(fields.path_mask,index)];
    if (!saved.saved) return ISLE_GAME_INVALID_STATE;
    if (saved.value && !isle::actor::SourceAccess::path(*slot,saved.value)) return ISLE_GAME_INVALID_STATE;
    source->path=saved.value;
    return ISLE_GAME_OK;
}
IsleGameResult isle_actor_owner_path_info(IsleActorSource* source,const void* value,IsleActorPathInfo* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    auto* slot=isle::actor::SourceAccess::slot(source);
    if (!slot) return ISLE_GAME_INVALID_STATE;
    const auto* path=isle::actor::SourceAccess::path(*slot,value);
    if (!path) return ISLE_GAME_INVALID_ARGUMENT;
    *output={path->kind,path->duration};
    return ISLE_GAME_OK;
}
IsleGameResult isle_actor_owner_path_position(IsleActorSource* source,const void* value,float parameter,IsleGameVec3* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    auto* slot=isle::actor::SourceAccess::slot(source);
    if (!slot) return ISLE_GAME_INVALID_STATE;
    const auto* path=isle::actor::SourceAccess::path(*slot,value);
    if (!path) return ISLE_GAME_INVALID_ARGUMENT;
    try {
        const auto position=isle::menu::animation::path_position(*path,parameter);
        *output={position.x,position.y,position.z};
        return ISLE_GAME_OK;
    } catch (...) { return ISLE_GAME_INVALID_ARGUMENT; }
}
IsleGameResult isle_actor_owner_path_velocity(IsleActorSource* source,const void* value,float parameter,IsleGameVec3* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    auto* slot=isle::actor::SourceAccess::slot(source);
    if (!slot) return ISLE_GAME_INVALID_STATE;
    const auto* path=isle::actor::SourceAccess::path(*slot,value);
    if (!path) return ISLE_GAME_INVALID_ARGUMENT;
    try {
        const auto velocity=isle::menu::animation::path_velocity(*path,parameter);
        *output={velocity.x,velocity.y,velocity.z};
        return ISLE_GAME_OK;
    } catch (...) { return ISLE_GAME_INVALID_ARGUMENT; }
}
IsleGameResult isle_actor_owner_current_world_position(IsleActorSource* source,IsleGameVec3* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    auto* slot=isle::actor::SourceAccess::slot(source);
    if (!slot || slot->owner->world.retired() || !slot->owner->world.error().empty()) return ISLE_GAME_INVALID_STATE;
    const auto* block=slot->owner->world.current();
    for (const auto& candidate:slot->owner->world.blocks()) if (&candidate==block) {
        for (const auto& resource:slot->owner->world.resources().blocks) if (&resource==candidate.resource) {
            const auto& position=resource.metadata.world_position;
            *output={position.x,position.y,position.z};
            return ISLE_GAME_OK;
        }
        return ISLE_GAME_INVALID_STATE;
    }
    return ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_actor_owner_step(IsleActorSource* source) {
    auto* slot=isle::actor::SourceAccess::slot(source);
    return slot && slot->owner->step(*slot)?ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE;
}
IsleGameResult isle_actor_owner_interaction_context(IsleActorSource* source,void** output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output=nullptr;
    auto* slot=isle::actor::SourceAccess::slot(source);
    if (!slot) return ISLE_GAME_INVALID_STATE;
    *output=slot->owner->interactions;
    return ISLE_GAME_OK;
}
IsleGameResult isle_actor_owner_cleanup(IsleActorSource* source) {
    auto* slot=isle::actor::SourceAccess::slot(source);
    return slot && slot->owner->cleanup(*slot)?ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE;
}
IsleGameResult isle_actor_owner_commands_begin(IsleActorSource* source,IsleSourceCommandScope* scope) {
    if (!scope) return ISLE_GAME_INVALID_ARGUMENT;
    *scope={};
    auto* slot=isle::actor::SourceAccess::slot(source);
    return slot && slot->owner->services.source_commands_begin(slot->object,*scope)?
        ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_actor_owner_commands_end(IsleActorSource* source,IsleSourceCommandScope* scope,
    IsleGameResult result,int delete_requested) {
    if (!scope) return ISLE_GAME_INVALID_ARGUMENT;
    auto* slot=isle::actor::SourceAccess::retained_scope_slot(source);
    return slot && slot->owner->services.source_commands_end(*scope,result,delete_requested)?
        ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_actor_owner_command_status(IsleActorSource* source) {
    auto* slot=isle::actor::SourceAccess::retained_scope_slot(source);
    return slot && !slot->owner->services.failed()?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_actor_owner_execution_enabled(IsleActorSource* source,int32_t* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    auto* slot=isle::actor::SourceAccess::slot(source);
    if (!slot) return ISLE_GAME_INVALID_STATE;
    *output=slot->owner->player.IsPaused==0 && slot->owner->player.D_80382D44_523154==0;
    return ISLE_GAME_OK;
}
IsleGameResult isle_actor_owner_reference_assign(IsleActorSource* source,IsleGameObject** field,IsleGameObject* sender) {
    auto* slot=isle::actor::SourceAccess::slot(source);
    return slot && slot->owner->services.source_reference_assign(slot->object,field,sender)?
        ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_actor_owner_command_before_transfer(IsleActorSource* source) {
    auto* slot=isle::actor::SourceAccess::slot(source);
    return slot && slot->owner->services.source_command_before_transfer(slot->object)?
        ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_actor_owner_distance(IsleActorSource* source) {
    auto* slot=isle::actor::SourceAccess::slot(source);
    return slot && slot->owner->distance(*slot)?ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE;
}
IsleGameResult isle_actor_owner_local_subtract_y(IsleActorSource* source,float amount) {
    auto* slot=isle::actor::SourceAccess::slot(source);
    if (!slot) return ISLE_GAME_INVALID_STATE;
    const auto state=slot->owner->runtime->model(slot->instance);
    if (!state || state.nodes.empty()) return ISLE_GAME_INVALID_STATE;
    auto pose=state.nodes[0].transform; pose.position.y-=amount;
    return slot->owner->local(*slot,0,pose)?ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE;
}
IsleGameResult isle_actor_owner_visibility(IsleActorSource* source,int hidden) {
    auto* slot=isle::actor::SourceAccess::slot(source);
    if (!slot) return ISLE_GAME_INVALID_STATE;
    IsleGameObjectMetadata metadata{};
    if (isle_game_object_metadata(source->game,source->object,&metadata)!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE;
    const auto result=isle_game_object_set_flags(source->game,source->object,hidden?(metadata.flags|3u):0u);
    if (result==ISLE_GAME_OK) source->tangible=hidden?0:1;
    return result;
}
IsleGameResult isle_actor_owner_trigger_position(IsleActorSource* source,IsleGameVec3 position) {
    auto* slot=isle::actor::SourceAccess::slot(source);
    if (!slot) return ISLE_GAME_INVALID_STATE;
    const IsleGameModelPlacement* current=nullptr;
    if (isle_game_model_placement(source->game,slot->model,&current)!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE;
    auto placement=current->transform; placement.position=position;
    return isle_game_model_set_placement(source->game,slot->model,ISLE_GAME_MODEL_PLACEMENT_RTS,&placement);
}
float isle_actor_owner_animation_time(IsleActorSource* source) {
    auto* slot=isle::actor::SourceAccess::slot(source);
    return slot?slot->owner->runtime->model(slot->instance).animation_time:0.0f;
}
int isle_actor_owner_paused(IsleActorSource* source) {
    auto* slot=isle::actor::SourceAccess::slot(source);
    return slot && slot->owner->player.IsPaused!=0;
}
IsleGameResult isle_actor_owner_magikarp_active(IsleActorSource* source,int active) {
    auto* slot=isle::actor::SourceAccess::slot(source);
    if (!slot || source->id!=129 || slot->owner->course_context.course_id()!=0) return ISLE_GAME_INVALID_STATE;
    slot->owner->course_context.beach().magikarp_active=active?1:0;
    return ISLE_GAME_OK;
}
IsleGameResult isle_actor_owner_ground_node(IsleActorSource* source,size_t* output) {
    auto* slot=isle::actor::SourceAccess::slot(source);
    // The shared helper follows each course's canonical Magikarp model.
    if (!slot || !output || source->id!=129)
        return ISLE_GAME_INVALID_STATE;
    const auto state=slot->owner->runtime->model(slot->instance);
    const IsleGameModelNode* nodes=nullptr; size_t count=0;
    if (!state || state.resource_model!=slot->initialization->model ||
        isle_game_model_peek_nodes(source->game,slot->model,&nodes,&count)!=ISLE_GAME_OK ||
        !count || count!=state.nodes.size()) return ISLE_GAME_INVALID_STATE;
    const auto first=nodes[0].first_child;
    if (first>=count || nodes[first].parent!=0) return ISLE_GAME_INVALID_STATE;
    const auto child=nodes[first].first_child;
    if (child>=count || nodes[child].parent!=first) return ISLE_GAME_INVALID_STATE;
    const auto next=nodes[child].next_sibling;
    if (next>=count || nodes[next].parent!=first) return ISLE_GAME_INVALID_STATE;
    *output=next;
    return ISLE_GAME_OK;
}
IsleGameResult isle_actor_owner_node_world(IsleActorSource* source,size_t node,IsleGameVec3* position,IsleGameVec3* velocity) {
    IsleActorNodeRef reference{};
    const auto result=isle_actor_owner_self_model(source,&reference.model);
    if (result!=ISLE_GAME_OK) return result;
    reference.node=node;
    return isle_actor_owner_node_world_ref(source,&reference,position,velocity);
}
IsleGameResult isle_actor_owner_node_world_ref(IsleActorSource* source,const IsleActorNodeRef* reference,
    IsleGameVec3* position,IsleGameVec3* velocity) {
    if (!reference || !position || !velocity) return ISLE_GAME_INVALID_ARGUMENT;
    if (!isle::actor::SourceAccess::slot(source)) return ISLE_GAME_INVALID_STATE;
    IsleGameModel* model=nullptr;
    const auto resolution=isle_game_model_resolve(source->game,&reference->model,&model);
    if (resolution!=ISLE_GAME_OK) return resolution;
    return isle::menu::model::source_effect_position_velocity(source->game,model,reference->node,position,velocity);
}
IsleGameResult isle_actor_owner_ground(IsleActorSource* source,float x,float z,IsleActorGround* output,int32_t* found) {
    auto* slot=isle::actor::SourceAccess::slot(source);
    if (!slot || !output || !found) return ISLE_GAME_INVALID_ARGUMENT;
    bool queried=false;
    const auto result=isle::actor::collision::ground_query(slot->owner->world,x,z,*output,queried);
    if (!result) return isle::actor::SourceAccess::collision_status(result);
    *found=queried?1:0;
    return ISLE_GAME_OK;
}
IsleGameResult isle_actor_owner_current_block(IsleActorSource* source,void** output) {
    if (output) *output=nullptr;
    auto* slot=isle::actor::SourceAccess::slot(source);
    if (!slot || !output) return ISLE_GAME_INVALID_ARGUMENT;
    *output=slot->owner->world.current();
    return ISLE_GAME_OK;
}
IsleGameResult isle_actor_owner_add_one(IsleActorSource* source,void* block,void* reference,
    const IsleActorGeoSpawn* spawn,const IsleActorGeoDefinition* definition,IsleGameObject** output) {
    if (output) *output=nullptr;
    auto* slot=isle::actor::SourceAccess::slot(source);
    if (!slot || !spawn || !definition || !output ||
        block!=slot->owner->world.current() || reference!=block || !block)
        return ISLE_GAME_INVALID_ARGUMENT;
    auto& self=*slot->owner;
    if (self.geo_transport) {
        const auto& ticket=*self.geo_transport;
        if (ticket.definition!=definition || ticket.parent!=source || ticket.requested!=static_cast<uint32_t>(spawn->id) ||
            !ticket.result || ticket.resource_definition>=self.resources.definitions().size() ||
            self.initialized_automatic_spawn) return ISLE_GAME_INVALID_ARGUMENT;
        isle::world::Spawn descriptor{};
        descriptor.id=spawn->id; descriptor.behavior=spawn->behavior;
        descriptor.position={spawn->translation.x,spawn->translation.y,spawn->translation.z};
        descriptor.rotation=spawn->euler; descriptor.scale={spawn->scale.x,spawn->scale.y,spawn->scale.z};
        if (spawn->path) {
            bool found=false;
            for (size_t i=0;i<self.world.resources().paths.size();++i)
                if (&self.world.resources().paths[i]==spawn->path) { descriptor.path=i; found=true; break; }
            if (!found) return ISLE_GAME_INVALID_ARGUMENT;
        }
        self.initialized_automatic_spawn=&descriptor;
        self.initialized_automatic_index=ticket.automatic_definition;
        self.initialized_requested_id=descriptor.id;
        const auto child=self.facade->add_one(ticket.resource_definition,static_cast<isle::world::Block*>(block),
            static_cast<isle::world::Block*>(reference),descriptor);
        self.initialized_automatic_spawn=nullptr; self.initialized_automatic_index=isle::actor::actor_no_index;
        self.initialized_requested_id=-1;
        *ticket.result=child;
        if (!child) return child.status.error==isle::actor::OwnerError::capacity ? ISLE_GAME_EXHAUSTED : ISLE_GAME_EXECUTION_FAILURE;
        *output=child.object;
        return ISLE_GAME_OK;
    }
    const bool common=definition->identity==0x8038837cu && spawn->id==1003;
    const bool pidgey=definition->identity==0x802cd838u && spawn->id==16;
    const bool scyther=definition->identity==0x802cd848u && spawn->id==123;
    const bool special_ending=definition->identity==0x802ce0dcu && spawn->id==1006;
    if (!common && !pidgey && !scyther && !special_ending) return ISLE_GAME_INVALID_ARGUMENT;
    const auto factory_identity=common?0x8035e0d4u:pidgey?0x802c8d74u:scyther?0x802c9204u:0x802cbda0u;
    isle::world::Spawn descriptor{};
    descriptor.id=spawn->id; descriptor.behavior=spawn->behavior;
    descriptor.position={spawn->translation.x,spawn->translation.y,spawn->translation.z};
    descriptor.rotation=spawn->euler; descriptor.scale={spawn->scale.x,spawn->scale.y,spawn->scale.z};
    if (spawn->path) {
        bool found=false;
        for (size_t i=0;i<self.world.resources().paths.size();++i)
            if (&self.world.resources().paths[i]==spawn->path) { descriptor.path=i; found=true; break; }
        if (!found) return ISLE_GAME_INVALID_ARGUMENT;
    }
    size_t definition_index=isle::actor::actor_no_index;
    for (size_t i=0;i<self.resources.definitions().size();++i) {
        const auto& candidate=self.resources.definitions()[i];
        if (candidate.wire.guest_address==definition->identity && candidate.singleton &&
            candidate.actor_id==static_cast<uint32_t>(spawn->id) && candidate.factory_identity==factory_identity &&
            candidate.change_block_identity==0x80363dbcu && candidate.remove_identity==0x80364280u) {
            definition_index=i; break;
        }
    }
    if (definition_index==isle::actor::actor_no_index) return ISLE_GAME_INVALID_ARGUMENT;
    const auto child=self.facade->add_one(definition_index,static_cast<isle::world::Block*>(block),
        static_cast<isle::world::Block*>(reference),descriptor);
    if (!child) return child.status.error==isle::actor::OwnerError::capacity?ISLE_GAME_EXHAUSTED:ISLE_GAME_EXECUTION_FAILURE;
    *output=child.object;
    return ISLE_GAME_OK;
}
IsleGameResult isle_actor_owner_copy_position(IsleActorSource* source,IsleGameObject* child) {
    auto* parent=isle::actor::SourceAccess::slot(source);
    IsleActorSource* child_source=nullptr;
    if (!parent || isle_actor_source_get(source->game,child,&child_source)!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE;
    auto* target=isle::actor::SourceAccess::slot(child_source);
    if (!target || target->owner!=parent->owner) return ISLE_GAME_INVALID_ARGUMENT;
    const IsleGameModelPlacement* position=nullptr;
    if (isle_game_model_placement(source->game,parent->model,&position)!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE;
    return isle_actor_owner_trigger_position(child_source,position->transform.position);
}
IsleGameObject* isle_actor_owner_spawn_dl4(int32_t object_id,uint16_t id,
    IsleActorFactoryBlock* block,IsleActorFactoryBlock* reference,IsleActorFactorySpawn* spawn,
    const IsleActorFactoryInitialization* initialization) {
    return isle::actor::SourceAccess::factory(object_id,id,block,reference,spawn,initialization,
        isle::actor::FactoryWrapper::ordinary_dl4);
}
IsleGameResult isle_beach_ending_native_source(IsleActorSource* source,IsleBeachEndingSource** output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output=nullptr;
    auto* slot=isle::actor::SourceAccess::ending_slot(source);
    if (!slot) return ISLE_GAME_INVALID_STATE;
    *output=&slot->owner->ending;
    return ISLE_GAME_OK;
}
IsleGameResult isle_beach_ending_native_remove_items(IsleActorSource* source) {
    auto* slot=isle::actor::SourceAccess::ending_slot(source);
    return slot?slot->owner->ending_services.remove_items(slot->owner->ending_services.context):ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_beach_ending_native_reset_message(IsleActorSource* source) {
    auto* slot=isle::actor::SourceAccess::ending_slot(source);
    return slot?slot->owner->ending_services.reset_message(slot->owner->ending_services.context):ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_beach_ending_native_camera(IsleActorSource* source) {
    auto* slot=isle::actor::SourceAccess::ending_slot(source);
    return slot?slot->owner->ending_services.camera(slot->owner->ending_services.context):ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_beach_ending_native_cart_init(IsleActorSource* source,IsleGameObject** output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output=nullptr;
    auto* slot=isle::actor::SourceAccess::ending_slot(source);
    return slot?slot->owner->ending_services.cart_init(slot->owner->ending_services.context,output):ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_beach_ending_native_cart_animation(IsleActorSource* source) {
    auto* slot=isle::actor::SourceAccess::ending_slot(source);
    return slot?slot->owner->ending_services.cart_animation(slot->owner->ending_services.context):ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_beach_ending_native_cart_publish(IsleActorSource* source,IsleGameObject* object) {
    auto* slot=isle::actor::SourceAccess::ending_slot(source);
    if (!slot || !object) return ISLE_GAME_INVALID_STATE;
    auto& self=*slot->owner;
    const auto held=self.ending_services.cart_retain(self.ending_services.context,object);
    if (held!=ISLE_GAME_OK) return held;
    return self.services.source_reference_assign(slot->object,&self.ending.D_beach_80347658,object)?
        ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE;
}
IsleGameResult isle_beach_ending_native_cart_callback(IsleActorSource* source,IsleGameObject* object) {
    auto* slot=isle::actor::SourceAccess::ending_slot(source);
    if (!slot || slot->owner->ending.D_beach_80347658!=object) return ISLE_GAME_INVALID_STATE;
    auto& self=*slot->owner;
    return self.ending_services.cart_callback(self.ending_services.context,object,&self.ending);
}
IsleGameResult isle_beach_ending_native_sound_volume(IsleActorSource* source,int32_t handle,uint32_t volume) {
    auto* slot=isle::actor::SourceAccess::ending_slot(source);
    if (!slot || handle<0) return ISLE_GAME_INVALID_ARGUMENT;
    return slot->owner->services.audio.sound_volume(static_cast<unsigned>(handle),volume)==isle::audio::Error::none?
        ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE;
}
IsleGameResult isle_beach_ending_native_playing_sound(IsleActorSource* source,int32_t handle,int32_t* output) {
    auto* slot=isle::actor::SourceAccess::ending_slot(source);
    if (!slot || !output || handle<0) return ISLE_GAME_INVALID_ARGUMENT;
    *output=slot->owner->services.audio.playing_sound(static_cast<unsigned>(handle));
    return ISLE_GAME_OK;
}
}
extern "C" IsleGameResult isle_beach_ending_native_command(IsleActorSource* source,int32_t command) {
    auto* slot=isle::actor::SourceAccess::ending_slot(source);
    if (!slot || !slot->owner->player.object || (command!=6 && command!=7 && command!=9))
        return ISLE_GAME_INVALID_ARGUMENT;
    return slot->owner->services.send_command(slot->owner->player.object,command,nullptr)?
        ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE;
}
extern "C" IsleGameResult isle_beach_ending_native_play_sound(IsleActorSource* source,int32_t sound,int32_t* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output=-1;
    auto* slot=isle::actor::SourceAccess::ending_slot(source);
    if (!slot) return ISLE_GAME_INVALID_STATE;
    *output=slot->owner->services.play_sound(static_cast<uint32_t>(sound));
    return slot->owner->services.failed()?ISLE_GAME_EXECUTION_FAILURE:ISLE_GAME_OK;
}
extern "C" IsleGameResult isle_beach_ending_native_bgm_volume(IsleActorSource* source,int32_t player,int32_t volume) {
    auto* slot=isle::actor::SourceAccess::ending_slot(source);
    if (!slot || player<0 || volume<0) return ISLE_GAME_INVALID_ARGUMENT;
    return slot->owner->services.audio.bgm_volume(static_cast<unsigned>(player),static_cast<uint32_t>(volume))==isle::audio::Error::none?
        ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE;
}
extern "C" IsleGameResult isle_beach_ending_native_stop_sound(IsleActorSource* source,int32_t handle) {
    auto* slot=isle::actor::SourceAccess::ending_slot(source);
    if (!slot || handle<0) return ISLE_GAME_INVALID_ARGUMENT;
    return slot->owner->services.audio.stop_sound(static_cast<unsigned>(handle))==isle::audio::Error::none?
        ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE;
}
extern "C" IsleGameResult isle_beach_ending_native_stop_sounds(IsleActorSource* source) {
    auto* slot=isle::actor::SourceAccess::ending_slot(source);
    if (!slot) return ISLE_GAME_INVALID_STATE;
    return slot->owner->services.audio.stop_sounds()==isle::audio::Error::none?
        ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE;
}
extern "C" IsleGameResult isle_beach_ending_native_sky_speed(IsleActorSource* source,float speed) {
    auto* slot=isle::actor::SourceAccess::ending_slot(source);
    if (!slot) return ISLE_GAME_INVALID_STATE;
    slot->owner->world.set_sky_animation_speed(speed);
    return ISLE_GAME_OK;
}
extern "C" IsleGameResult isle_beach_ending_native_reset_camera(IsleActorSource* source) {
    auto* slot=isle::actor::SourceAccess::ending_slot(source);
    return slot?slot->owner->ending_services.reset_camera(slot->owner->ending_services.context):ISLE_GAME_INVALID_STATE;
}
extern "C" IsleGameResult isle_beach_ending_native_start_cutscene(IsleActorSource* source) {
    auto* slot=isle::actor::SourceAccess::ending_slot(source);
    return slot?slot->owner->ending_services.start_cutscene(slot->owner->ending_services.context,source->object):ISLE_GAME_INVALID_STATE;
}
extern "C" IsleGameResult isle_actor_owner_block_indices(IsleActorSource* source,
    int32_t* current_output,int32_t* base_output) {
    if (!current_output || !base_output) return ISLE_GAME_INVALID_ARGUMENT;
    auto* slot=isle::actor::SourceAccess::slot(source);
    if (!slot) return ISLE_GAME_INVALID_STATE;
    auto& world=slot->owner->world;
    const auto* current=world.current();
    const auto* base=static_cast<const isle::world::Block*>(source->baseBlock);
    bool current_found=false,base_found=false;
    for (const auto& block:world.blocks()) {
        current_found|=&block==current; base_found|=&block==base;
    }
    if (!current_found || !base_found || current->index>INT32_MAX || base->index>INT32_MAX)
        return ISLE_GAME_INVALID_STATE;
    *current_output=static_cast<int32_t>(current->index);
    *base_output=static_cast<int32_t>(base->index);
    return ISLE_GAME_OK;
}
extern "C" IsleGameResult isle_actor_owner_schedule_cleanup(IsleActorSource* source) {
    auto* slot=isle::actor::SourceAccess::slot(source);
    if (!slot || !slot->owner->facade) return ISLE_GAME_INVALID_STATE;
    return isle::actor::SourceAccess::collision_status(slot->owner->facade->request_cleanup(slot->instance));
}

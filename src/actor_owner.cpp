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
#include "player_root.hpp"
#include "actor_resource_catalog.hpp"
#include "rainbow_actor.hpp"
#include <bit>
#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>
#include <stdexcept>
#include <utility>

namespace isle::actor {
namespace {
OwnerStatus rejected(OwnerError error, const char* detail) noexcept { return {error,detail}; }
IsleGameModelTransform native(const menu::animation::Transform& t) noexcept {
    return {{t.position.x,t.position.y,t.position.z},{t.rotation.x,t.rotation.y,t.rotation.z},{t.scale.x,t.scale.y,t.scale.z}};
}
constexpr float maximum_distance = std::numeric_limits<float>::max();
enum class FactoryWrapper { ground, ordinary, ground_dl4, ordinary_dl4, meowth };
struct FactorySpec {
    std::uint32_t actor_id = 0, identity = 0;
    FactoryWrapper wrapper = FactoryWrapper::ground;
    IsleActorReturningFactory returning = nullptr;
    IsleActorVoidFactory nonreturning = nullptr;
    IsleGameSourceCallback initial_state = nullptr;
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
    if (course==0) {
        for (const auto& spec:beach_factories)
            if (spec.actor_id==id && spec.identity==factory) return spec;
    } else if (course==6) {
        for (const auto& binding:rainbow::actor_factories()) {
            if (binding.actor_id!=id || binding.factory!=factory) continue;
            for (const auto& callback:rainbow::actor_callbacks())
                if (callback.identity==binding.initial_state)
                    return {id,factory,binding.wrapper==rainbow::ActorFactoryWrapper::ground ?
                        FactoryWrapper::ground:FactoryWrapper::ordinary_dl4,
                        binding.function,nullptr,callback.function};
        }
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
        scene::RenderMetadata render{};
        std::uint64_t insertion = 0;
        std::array<IsleGameObject**,3> source_fields{};
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
    // Synchronous factory transport capability; never retained by a process.
    // No optional spawn field is read while this exact record is issued.
    const world::Spawn* transforms_only_spawn=nullptr;
    std::size_t transforms_only_definition=actor_no_index, transforms_only_automatic=actor_no_index;
    std::uint32_t transforms_only_helper=0, transforms_only_written_fields=0;
    const IsleActorFactorySpawn* factory_transport=nullptr;
    // Cold proof over immutable copied capacities and admitted resource shapes;
    // never records occupancy, Source state or future child reservations.
    bool rainbow_capacity_shape=false;

    Impl(scene::Services& s,const actor::Resources& r,CourseContext state,world::World& w,environment_sound::Environment& e,
         focus::Detector& d,IslePlayerSource& p,std::span<const menu::animation::SlotCapacity> c,std::span<const menu::EffectBank* const> banks,
         menu::animation::ExecutionCapacity execution)
        : services(s),resources(r),world(w),sound(e),detector(d),capacities(c.begin(),c.end()),slots(c.size()),bindings(c.size()),
          fx(banks,s.rng,{this,read_target,write_target}),block_heads(w.blocks().size(),-1),player(p),course_context(state) {
        resources.require_live();
        course_context.require_live();
        const auto setup_identity=course_context.course_id()==0?0x8011b914u:0x800f5da0u;
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
        std::size_t maximum_nodes=0;
        for (std::size_t i=0;i<c.size();++i) {
            slots[i].owner=this; slots[i].index=i;
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
        return {services,sound,fx,*runtime,slot.model,sound_enabled,particle_object,effect_object};
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
        self.invalidate(slot);
        self.unlink_logical(slot);
        slot.retiring=true;
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
        if (!self.rainbow_capacity_shape || !self.interactions || !self.interactions->admits_source())
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
        if (slot->owner->course_context.course_id()==6) {
            auto& course_source=slot->owner->course_context.rainbow().actors;
            if (rainbow_admission(&course_source)!=ISLE_GAME_OK) return nullptr;
        }
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
    static IsleGameObject* factory(std::int32_t object_id,std::uint16_t id,
        IsleActorFactoryBlock* block,IsleActorFactoryBlock* reference,IsleActorFactorySpawn* spawn,
        const IsleActorFactoryInitialization* initialization,FactoryWrapper wrapper) noexcept {
        if (!block || !reference || !spawn || !initialization || !block->owner ||
            reference->owner!=block->owner || !block->result || reference->result!=block->result) return nullptr;
        auto& owner=*static_cast<Owner*>(block->owner);
        auto& self=*owner.impl_;
        auto& result=*static_cast<SpawnResult*>(block->result);
        const auto fail=[&]() {
            result={{OwnerError::admission,"source wrapper differs from issued compiled factory proof"},{},nullptr};
            return static_cast<IsleGameObject*>(nullptr);
        };
        if (self.factory_transport!=spawn || spawn->definition>=self.resources.definitions().size()) return fail();
        const auto& definition=self.resources.definitions()[spawn->definition];
        const auto* init=owner.admitted_factory(id,definition.factory_identity);
        if (!init || definition.actor_id!=id || init->wire.guest_address!=initialization->identity ||
            definition.initialization>=self.resources.initializations().size() ||
            &self.resources.initializations()[definition.initialization]!=init) return fail();
        const auto spec=compiled_factory(self.resources.course_id(),id,definition.factory_identity);
        auto expected=spec.wrapper;
        const auto* descriptor=static_cast<const world::Spawn*>(spawn->spawn);
        if (expected==FactoryWrapper::meowth) {
            if (!descriptor || descriptor==self.transforms_only_spawn || spawn->behavior!=&descriptor->behavior) return fail();
            expected=*spawn->behavior==1?FactoryWrapper::ordinary:FactoryWrapper::ground;
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
             menu::animation::ExecutionCapacity execution) : impl_(std::make_unique<Impl>(s,r,state,w,e,d,p,c,banks,execution)) {
    impl_->facade=this;
    if (state.course_id()==6) {
        impl_->rainbow_capacity_shape=[this]() {
            for (const auto& factory:rainbow::actor_factories()) {
                const auto* init=admitted_factory(factory.actor_id,factory.factory);
                if (!init || init->model>=impl_->resources.models().size()) return false;
                const auto required=menu::animation::slot_capacity(impl_->resources.models()[init->model]);
                bool fits=false;
                for (const auto& capacity:impl_->capacities) {
                    if (capacity.nodes>=required.nodes && capacity.material_attachments>=required.material_attachments) {
                        fits=true;
                        break;
                    }
                }
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
        if (!spec.identity || !spec.initial_state || (!!spec.returning==!!spec.nonreturning)) return nullptr;
        const auto& catalog=detail::course_catalog(self.resources.course_id());
        for (std::size_t i=0;i<catalog.initializations.size();++i) {
            const auto& fact=catalog.initializations[i];
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
    if (self.course_context.course_id()==6) {
        auto& source=self.course_context.rainbow().actors;
        if (SourceAccess::rainbow_admission(&source)!=ISLE_GAME_OK)
            return {{OwnerError::admission,"Rainbow factory requires ACTIVE canonical complete Source closure"},{},nullptr};
    }
    const auto spec=compiled_factory(self.resources.course_id(),definition.actor_id,definition.factory_identity);
    const bool unwritten=request.spawn && request.spawn==self.transforms_only_spawn;
    if (spec.wrapper==FactoryWrapper::meowth && (!request.spawn || unwritten))
        return {{OwnerError::admission,"original Meowth factory requires its actual written behavior borrow"},{},nullptr};
    if (self.factory_transport)
        return {{OwnerError::admission,"source factory transport is already in use"},{},nullptr};
    SpawnResult result{{OwnerError::admission,"source factory did not publish a result"},{},nullptr};
    IsleActorFactoryBlock block{this,request.block,&result},reference{this,request.reference,&result};
    IsleActorFactorySpawn spawn{request.spawn,request.definition,
        spec.wrapper==FactoryWrapper::meowth?&request.spawn->behavior:nullptr,request.automatic_definition};
    IsleGameObject* returned=nullptr;
    self.factory_transport=&spawn;
    const auto called=isle_actor_source_factory_call(self.services.game,spec.returning,spec.nonreturning,
        static_cast<int32_t>(request.object_id),static_cast<uint16_t>(definition.actor_id),
        &block,&reference,&spawn,&returned);
    self.factory_transport=nullptr;
    if (called!=ISLE_GAME_OK)
        return {{OwnerError::native,"original C factory fence failed"},{},nullptr};
    if (spec.returning && returned!=result.object)
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
        &descriptor==self.transforms_only_spawn?self.transforms_only_automatic:actor_no_index});
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
    if (!beach) {
        IsleRainbowActorSource* canonical=nullptr;
        const auto& source=self.course_context.rainbow().actors;
        if (source.phase!=ISLE_RAINBOW_ACTOR_ACTIVE ||
            isle_rainbow_actor_native_context(self.services.game,&canonical)!=ISLE_GAME_OK || canonical!=&source)
            return reject(OwnerError::admission,"Rainbow native factory requires ACTIVE canonical Source");
    }
    if (request.spawn->id!=static_cast<std::int32_t>(definition.actor_id)) return reject(OwnerError::admission,"actor source spawn ID does not match definition");
    const bool unwritten=request.spawn==self.transforms_only_spawn;
    if (unwritten && (self.transforms_only_definition!=request.definition ||
        self.transforms_only_automatic!=request.automatic_definition ||
        self.transforms_only_written_fields!=ISLE_RAINBOW_SPAWN_ID_TRANSFORMS ||
        (beach && (!magikarp || definition.wire.guest_address!=0x802cbff4u ||
                   self.transforms_only_helper!=0x802cbff4u))))
        return reject(OwnerError::admission,"unwritten optional fields lack their exact issued Source origin");
    if (request.automatic_definition!=actor_no_index &&
        (!unwritten || beach || request.automatic_definition>=self.resources.automatic_definitions().size()))
        return reject(OwnerError::admission,"automatic Source definition requires the exact transforms-only helper capability");
    if (!unwritten && request.spawn->path!=world::no_index && request.spawn->path>=self.world.resources().paths.size())
        return reject(OwnerError::admission,"actor source spawn path outside World");
    if (ending && (!self.ending_services.context || !self.interactions ||
        self.ending_services.validate(self.ending_services.context,&self.ending)!=ISLE_GAME_OK ||
        !self.interactions->admits_ending(initialization)))
        return reject(OwnerError::admission,"ending requires complete native course/callback/resource closure");
    if (!common && !magikarp && !ending) {
        const void* path=unwritten || request.spawn->path==world::no_index ?
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
    slot->spawn_helper=unwritten?self.transforms_only_helper:0;
    slot->spawn_written_fields=unwritten?self.transforms_only_written_fields:0;
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
    slot->source_fields={&slot->source.apple,&slot->source.interactionTarget,&self.ending.D_beach_80347658};
    if (!self.services.source_owner_register(slot->object,
        std::span(slot->source_fields).first(ending?3u:2u))) {
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
    if (!unwritten) {
        if (spawn.path!=world::no_index) source.path=&self.world.resources().paths[spawn.path];
        source.behavior=spawn.behavior;
    }
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
        slot=Impl::Slot{}; slot.owner=impl_.get(); slot.index=index;
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
    if (!slot || !output || source->id!=129 || slot->initialization->wire.guest_address!=0x802cdaecu)
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
    isle::menu::EffectWorld world{};
    if (!isle::menu::model::source_effect_world(source->game,model,reference->node,world)) return ISLE_GAME_EXECUTION_FAILURE;
    auto& m=world.matrix;
    // effect.c1335-1360: normalize columns, never transform velocity as a
    // point or derive it from frame differences/renderer's quantized caches.
    for (unsigned col=0;col<3;++col) {
        if (m[0][col]==0 && m[1][col]==0 && m[2][col]==0) continue;
        const float n=std::sqrt(m[0][col]*m[0][col]+m[1][col]*m[1][col]+m[2][col]*m[2][col]);
        if (n==0 || !std::isfinite(n) || !std::isfinite(1.0f/n)) return ISLE_GAME_INVALID_ARGUMENT;
        const float inv=1.0f/n;
        m[0][col]*=inv; m[1][col]*=inv; m[2][col]*=inv;
    }
    const auto v=*velocity;
    const IsleGameVec3 result{m[0][0]*v.x+m[1][0]*v.y+m[2][0]*v.z,
        m[0][1]*v.x+m[1][1]*v.y+m[2][1]*v.z,m[0][2]*v.x+m[1][2]*v.y+m[2][2]*v.z};
    if (!std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.z)) return ISLE_GAME_INVALID_ARGUMENT;
    *position={m[3][0],m[3][1],m[3][2]}; *velocity=result;
    return ISLE_GAME_OK;
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

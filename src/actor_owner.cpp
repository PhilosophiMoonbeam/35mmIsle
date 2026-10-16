#include "actor_owner.hpp"
#include "actor_photo_eligibility.hpp"
#include "source_effect_world.hpp"
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
}
OwnerStatus dispatch_event(IsleActorSource& actor, const menu::animation::CallbackEvent& event, EventServices context) noexcept {
    IsleActorSource* bound = nullptr;
    const auto state = context.animation.model(event.instance);
    const IsleGameModelNode* nodes = nullptr;
    std::size_t count = 0;
    if (isle_actor_source_get(context.services.game,actor.object,&bound) != ISLE_GAME_OK || bound != &actor ||
        isle_actor_source_validate_model(context.services.game,actor.object,context.model)!=ISLE_GAME_OK ||
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
        const auto* header = static_cast<const beach::ActorAnimationHeader*>(actor.animHeader);
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
        isle_actor_source_validate_model(context.services.game,source.object,context.model)!=ISLE_GAME_OK ||
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
    const beach::ActorAnimationHeader* header,float start,bool force,const beach::ActorInitialization& initialization,
    const beach::ActorResources& resources,EventServices context) noexcept {
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
        isle_actor_source_validate_model(context.services.game,source.object,context.model)!=ISLE_GAME_OK)
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
        const beach::ActorInitialization* initialization = nullptr;
        const beach::ActorDefinition* definition = nullptr;
        scene::RenderMetadata render{};
        std::uint64_t insertion = 0;
        std::size_t pending_die_commands = 0;
        std::vector<std::size_t> incoming_identity_credits;
        bool active = false, retiring = false, joined = false, cleaned = false, identity_held = false;
        bool logical = false, registered = false;
    };
    struct BlockEntry { Slot* slot = nullptr; int next = -1; };
    scene::Services& services;
    Owner* facade = nullptr;
    const beach::ActorResources& resources;
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
    std::array<Slot*,40> logical{};
    std::size_t logical_count = 0;
    std::array<BlockEntry,100> block_entries{};
    std::vector<int> block_heads;
    std::size_t transform_cache = 0, pokemon_cache = 0;
    std::uint64_t insertion = 0;
    PlayerState player;
    CourseState course;
    // Beach func_beach_802C4340 configures inclusive IDs80..E4/link3.
    // getFreePokemonId's cursor survives resets, as its source static does.
    std::uint32_t pokemon_counter = 0;

    Impl(scene::Services& s,const beach::ActorResources& r,world::World& w,environment_sound::Environment& e,
         focus::Detector& d,std::span<const menu::animation::SlotCapacity> c,std::span<const menu::EffectBank* const> banks,
         menu::animation::ExecutionCapacity execution)
        : services(s),resources(r),world(w),sound(e),detector(d),capacities(c.begin(),c.end()),slots(c.size()),bindings(c.size()),
          fx(banks,s.rng,{this,read_target,write_target}),block_heads(w.blocks().size(),-1) {
        resources.require_live();
        if (c.empty() || !fx.status()) throw std::invalid_argument("actor owner needs native capacities and admitted shared FX banks");
        std::size_t maximum_nodes=0;
        for (std::size_t i=0;i<c.size();++i) {
            slots[i].owner=this; slots[i].index=i; slots[i].incoming_identity_credits.resize(c.size());
            maximum_nodes=std::max(maximum_nodes,c[i].nodes);
        }
        scratch.resize(maximum_nodes);
        auto result=menu::animation::Runtime::create({r.models(),r.materials(),r.animations(),menu::no_index,{}},c,{},execution);
        if (!result) throw std::runtime_error("actor animation Runtime admission failed");
        runtime=std::move(result.runtime);
        particle_object=services.object(static_cast<std::uint32_t>(-6));
        effect_object=services.object(static_cast<std::uint32_t>(-7));
        if (!particle_object || !effect_object ||
            !services.set_lifecycle(particle_object,{nullptr,fx_joined,this}) ||
            !services.set_lifecycle(effect_object,{nullptr,fx_joined,this}) ||
            !services.set_update(particle_object,particles_tick,this) || !services.set_update(effect_object,effects_tick,this)) {
            if (particle_object) services.erase(particle_object);
            if (effect_object) services.erase(effect_object);
            throw std::runtime_error("actor shared source FX owner construction failed");
        }
    }
    Slot* find(menu::animation::InstanceHandle handle,bool retiring=false) noexcept {
        if (!handle || handle.slot>=slots.size()) return nullptr;
        auto& slot=slots[handle.slot];
        return slot.active && slot.instance==handle && (retiring || !slot.retiring) ? &slot : nullptr;
    }
    const Slot* find(menu::animation::InstanceHandle handle,bool retiring=false) const noexcept {
        return const_cast<Impl*>(this)->find(handle,retiring);
    }
    static void fx_joined(IsleGameObject* object,void* user) noexcept {
        auto& self=*static_cast<Impl*>(user);
        if (self.particle_object==object) self.particle_object=nullptr;
        if (self.effect_object==object) self.effect_object=nullptr;
    }
    static void particles_tick(IsleGameObject*,void* user) noexcept {
        auto& self=*static_cast<Impl*>(user);
        if (!self.fx.update_particles()) self.services.fail("actor shared particle update");
    }
    static void effects_tick(IsleGameObject*,void* user) noexcept {
        auto& self=*static_cast<Impl*>(user);
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
        return {services,sound,fx,*runtime,slot.model,course.sound_enabled,particle_object,effect_object};
    }
    OwnerStatus step(Slot& slot) noexcept {
        return step_source_animation(slot.source,slot.instance,event_services(slot));
    }
    OwnerStatus animation(Slot& slot,const beach::ActorAnimationHeader* header,float start,bool force) noexcept {
        return apply_source_animation(slot.source,slot.instance,header,start,force,*slot.initialization,resources,event_services(slot));
    }
    void focus_cleanup(Slot& slot) noexcept {
        if (player.focus_latch && player.previous_focus_identity==slot.object) {
            player.previous_focus={}; player.previous_focus_identity=nullptr; player.focus_latch=false;
        }
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
        // Native deletion discards this receiver FIFO. Cancel only its recorded
        // identity borrows; never deliver discarded source commands as a fixup.
        for (std::size_t i=0;i<slot.incoming_identity_credits.size();++i) {
            const auto credits=slot.incoming_identity_credits[i];
            if (credits>self.slots[i].pending_die_commands) self.services.fail("actor command identity credit corruption");
            else self.slots[i].pending_die_commands-=credits;
            slot.incoming_identity_credits[i]=0;
        }
        if (!slot.cleaned) for (auto& receiver:self.slots) if (receiver.active && !receiver.retiring) {
            if (receiver.source.interactionTarget==slot.object) receiver.source.interactionTarget=nullptr;
            if (receiver.source.apple==slot.object) receiver.source.apple=nullptr;
        }
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
    static void update_callback(IsleGameObject* object,void*) noexcept { Pokemon_Update(object); }
    static void command_queued(IsleGameObject*,std::int32_t command,IsleGameObject* source,void* user) noexcept {
        if (command!=21 && command!=22) return;
        auto& receiver=*static_cast<Slot*>(user); auto& self=*receiver.owner;
        // A retiring source is comparison identity only. Never source-get it.
        for (auto& sender:self.slots) if (sender.active && sender.object==source) {
            ++sender.pending_die_commands; ++receiver.incoming_identity_credits[sender.index]; return;
        }
    }
    static void command_callback(IsleGameObject*,std::int32_t command,IsleGameObject* source,void* user) noexcept {
        auto& slot=*static_cast<Slot*>(user); auto& self=*slot.owner;
        // 21 and22 perform identity clearing even for a NULL transition graph.
        // Never liveness-filter or dereference a command22 retiring sender.
        if (command==21 && slot.source.apple==source) slot.source.apple=nullptr;
        if (command==22 && slot.source.interactionTarget==source) slot.source.interactionTarget=nullptr;
        if (command==21 || command==22) {
            for (std::size_t i=0;i<self.slots.size();++i) if (slot.incoming_identity_credits[i] && self.slots[i].object==source) {
                --slot.incoming_identity_credits[i]; --self.slots[i].pending_die_commands; break;
            }
        }
        if (slot.source.transitionGraph) { self.services.fail("actor command transition graph is not natively admitted"); return; }
        // All other source switch cases/default have no action with NULLgraph.
        if (!self.course.execution_enabled) {
            if (isle_actor_source_pause_object(&slot.source,1)!=ISLE_GAME_OK)
                self.services.fail("actor source execution pause");
        }
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
        for (std::size_t i=0;i<logical_count;++i) {
            auto& target=*logical[i];
            if (target.source.interactionTarget==slot.object) {
                if (!services.send_command(target.object,22,slot.object)) return rejected(OwnerError::services,"actor die-command enqueue");
            }
        }
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
        if (!player.model) { slot.source.playerDist=maximum_distance; return {}; }
        const IsleGameModelPlacement *actor_position=nullptr,*player_position=nullptr;
        if (isle_game_model_placement(services.game,slot.model,&actor_position)!=ISLE_GAME_OK ||
            isle_game_model_placement(services.game,player.model,&player_position)!=ISLE_GAME_OK)
            return rejected(OwnerError::native,"actor/player source placement distance");
        const float dx=actor_position->transform.position.x-player_position->transform.position.x;
        const float dy=actor_position->transform.position.y-player_position->transform.position.y;
        const float dz=actor_position->transform.position.z-player_position->transform.position.z;
        slot.source.playerDist=std::sqrt(dx*dx+dy*dy+dz*dz);
        return {};
    }
};

struct SourceAccess {
    static Owner::Impl::Slot* slot(IsleActorSource* source) noexcept {
        if (!source || !source->owner || !source->game || !source->object) return nullptr;
        IsleActorSource* bound=nullptr;
        if (isle_actor_source_get(source->game,source->object,&bound)!=ISLE_GAME_OK || bound!=source) return nullptr;
        auto* slot=static_cast<Owner::Impl::Slot*>(source->owner);
        if (&slot->source!=source || !slot->active || slot->retiring || slot->owner->services.game!=source->game) return nullptr;
        return slot;
    }
    static SpawnResult spawn(Owner& owner,const SpawnRequest& request) noexcept { return owner.spawn_native(request); }
};
Owner::Owner(scene::Services& s,const beach::ActorResources& r,world::World& w,environment_sound::Environment& e,
             focus::Detector& d,std::span<const menu::animation::SlotCapacity> c,std::span<const menu::EffectBank* const> banks,
             menu::animation::ExecutionCapacity execution) : impl_(std::make_unique<Impl>(s,r,w,e,d,c,banks,execution)) {
    impl_->facade=this;
}
Owner::~Owner() {
    if (!reset()) {
        // Failure shutdown is not a successful source reset. Still perform real
        // joins and queue cancellation before callback storage can disappear.
        for (auto& slot:impl_->slots) if (slot.active && !slot.retiring) impl_->services.erase(slot.object);
        if (!collect_joined()) std::terminate();
    }
    if (impl_->particle_object) impl_->services.erase(impl_->particle_object);
    if (impl_->effect_object) impl_->services.erase(impl_->effect_object);
    if (impl_->particle_object || impl_->effect_object) std::terminate();
}
SpawnResult Owner::spawn(const SpawnRequest& request) noexcept {
    if (request.definition>=impl_->resources.definitions().size())
        return {{OwnerError::admission,"actor definition outside admitted resources"},{},nullptr};
    const auto& definition=impl_->resources.definitions()[request.definition];
    const bool common=definition.factory_identity==0x8035e0d4u && definition.actor_id==1003;
    const bool magikarp=definition.factory_identity==0x802cac48u && definition.actor_id==129;
    if (!common && !magikarp)
        return {{OwnerError::admission,"configured factory identity is not natively admitted"},{},nullptr};
    SpawnResult result{{OwnerError::admission,"source factory did not publish a result"},{},nullptr};
    IsleActorFactoryBlock block{this,request.block,&result},reference{this,request.reference,&result};
    IsleActorFactorySpawn spawn{request.spawn,request.definition};
    if (common)
        func_8035E0D4_4FE4E4(static_cast<int32_t>(request.object_id),static_cast<uint16_t>(definition.actor_id),&block,&reference,&spawn);
    else
        func_beach_802CAC48(static_cast<int32_t>(request.object_id),static_cast<uint16_t>(definition.actor_id),&block,&reference,&spawn);
    return result;
}
SpawnResult Owner::add_one(std::size_t definition,world::Block* block,world::Block* reference,
                           const world::Spawn& descriptor) noexcept {
    auto& self=*impl_;
    // Exact source cursor/two half-open searches and inclusive max fallback.
    constexpr std::uint32_t minimum=0x80,maximum=0xe4;
    if (++self.pokemon_counter>maximum-minimum) self.pokemon_counter=0;
    std::uint32_t id=maximum;
    for (unsigned pass=0;pass<2 && id==maximum;++pass) {
        const auto first=pass?minimum:minimum+self.pokemon_counter;
        const auto last=pass?minimum+self.pokemon_counter:maximum;
        for (auto candidate=first;candidate<last;++candidate) {
            int used=0;
            if (isle_actor_source_id_in_use(self.services.game,candidate,&used)!=ISLE_GAME_OK)
                return {{OwnerError::native,"source native ID search failed"},{},nullptr};
            if (!used) { id=candidate; break; }
        }
    }
    return spawn({id,definition,block,reference,&descriptor});
}
OwnerStatus Owner::refresh_collision_positions() noexcept {
    auto& self=*impl_;
    for (std::size_t i=0;i<self.logical_count;++i) {
        auto& slot=*self.logical[i];
        IsleGameVec3 position;
        if (photo::collision_position(slot.source,slot.model,position)!=ISLE_GAME_OK)
            return rejected(OwnerError::native,"canonical source collision position refresh failed");
        slot.source.collPosition=position;
        slot.source.processFlags&=~std::uint32_t{0x80};
    }
    return {};
}
SpawnResult Owner::spawn_native(const SpawnRequest& request) noexcept {
    auto& self=*impl_;
    const auto reject=[](OwnerError error,const char* detail) { return SpawnResult{{error,detail},{},nullptr}; };
    if (!request.block || !request.block->resource || !request.reference || !request.reference->resource || !request.spawn ||
        request.definition>=self.resources.definitions().size() || request.block->index>=self.block_heads.size())
        return reject(OwnerError::admission,"actor factory requires admitted definition, block, reference and spawn");
    bool block_found=false,reference_found=false;
    for (auto& block:self.world.blocks()) { block_found|=&block==request.block; reference_found|=&block==request.reference; }
    if (!block_found || !reference_found) return reject(OwnerError::admission,"actor factory block belongs to another World");
    const auto& definition=self.resources.definitions()[request.definition];
    if (definition.initialization>=self.resources.initializations().size()) return reject(OwnerError::admission,"actor definition has no initialization");
    const auto& initialization=self.resources.initializations()[definition.initialization];
    // Immutable actual source callable binding, not guest-pointer casts or a task registry.
    const bool common=definition.actor_id==1003 && definition.factory_identity==0x8035e0d4u &&
        initialization.wire.guest_address==0x80388348u && initialization.render_callback_identity==0x800a1608u &&
        initialization.setup.wire.guest_address==0x80388334u && initialization.setup.initial_state_identity==0x8035df20u;
    const bool magikarp=definition.actor_id==129 && definition.factory_identity==0x802cac48u &&
        initialization.wire.guest_address==0x802cdaecu && initialization.render_callback_identity==0x803594dcu &&
        initialization.setup.wire.guest_address==0x802cdad8u && initialization.setup.initial_state_identity==0x802cab50u;
    if ((!common && !magikarp) || initialization.setup.transition_graph_identity!=0 ||
        initialization.setup.unknown_0c_identity!=0 || initialization.setup.process_kind!=0 ||
        !initialization.animations_enabled)
        return reject(OwnerError::admission,"configured actor factory/init/render/setup/state/graph tuple is not natively admitted");
    if (initialization.model>=self.resources.models().size() || initialization.setup.initial_header>=self.resources.headers().size())
        return reject(OwnerError::admission,"actor native tree/header missing");
    if (self.resources.headers()[initialization.setup.initial_header].wire.guest_address!=(common?0x80388310u:0x802cda8cu))
        return reject(OwnerError::admission,"actor source initial header is not the configured tuple");
    if (request.spawn->id!=static_cast<std::int32_t>(definition.actor_id)) return reject(OwnerError::admission,"actor source spawn ID does not match definition");
    if (request.spawn->path!=world::no_index && request.spawn->path>=self.world.resources().paths.size())
        return reject(OwnerError::admission,"actor source spawn path outside World");
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
    if (isle_game_object_retain_identity(self.services.game,slot->object)!=ISLE_GAME_OK) {
        rollback(); return reject(OwnerError::native,"actor native identity retain");
    }
    slot->identity_held=true;
    if (!self.services.set_update(slot->object,Impl::update_callback,slot) ||
        !self.services.set_command_handler(slot->object,Impl::command_callback,slot) ||
        !self.services.set_command_queued_observer(slot->object,Impl::command_queued,slot)) {
        rollback(); return reject(OwnerError::services,"actor native callbacks");
    }
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
    const auto animated=self.animation(*slot,&self.resources.headers()[initialization.setup.initial_header],0,true);
    if (!animated) { rollback(); return {animated,{},nullptr}; }
    if (isle_game_add_source_process(self.services.game,slot->object,isle_actor_source_distance_process,ISLE_GAME_SYNCHRONOUS,8,&process)!=ISLE_GAME_OK ||
        isle_game_add_source_process(self.services.game,slot->object,common?func_8035DF20_4FE330:func_beach_802CAB50,ISLE_GAME_STACKFUL,7,&source.stateProc)!=ISLE_GAME_OK) {
        rollback(); return reject(OwnerError::native,"actor distance/initial state process creation");
    }
    // Late factory resets occur only AFTER forced header and immediate events.
    source.unk_18=-1; source.processFlags=0; source.counter=0; source.pathProc=nullptr;
    source.transitionGraph=nullptr; source.pokemonLoopTarget=0; source.lastAnimationFrame=0;
    source.jumpVel=0; source.hSpeed=0; source.facingYaw=0;
    source.transitionGraph=nullptr; source.unk_E4=255;
    if (spawn.path!=world::no_index) {
        source.path=&self.world.resources().paths[spawn.path];
    }
    source.pathParam=0; source.behavior=spawn.behavior; source.eggGeo=nullptr; source.poseID=0; source.specialPoseID=0;
    source.playerDist=maximum_distance;
    self.logical[self.logical_count++]=slot; slot->logical=true;
    auto& entry=self.block_entries[static_cast<std::size_t>(registry)];
    entry.slot=slot; entry.next=self.block_heads[request.block->index]; self.block_heads[request.block->index]=registry; slot->registered=true;
    if (self.transform_cache) --self.transform_cache;
    if (self.pokemon_cache) --self.pokemon_cache;
    // Magikarp keeps Ground's original DL5/tag-1; only common's wrapper moves4.
    if (common) {
        slot->render={4,0x80000000u,UINT32_MAX};
        if (!self.services.set_render(slot->object,slot->render)) { rollback(); return reject(OwnerError::services,"actor source DL4 move"); }
    }
    slot->insertion=++self.insertion;
    return {{},slot->instance,slot->object};
}
OwnerStatus Owner::set_animation(menu::animation::InstanceHandle h,const beach::ActorAnimationHeader* header,float start,bool force) noexcept {
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
    const IsleGameModelPlacement* old=nullptr;
    if (isle_game_model_placement(impl_->services.game,slot->model,&old)!=ISLE_GAME_OK) return rejected(OwnerError::native,"actor block placement");
    auto placement=old->transform;
    const float dx=(previous.x-current.x)*100.0f,dy=(previous.y-current.y)*100.0f,dz=(previous.z-current.z)*100.0f;
    placement.position.x+=dx; placement.position.y+=dy; placement.position.z+=dz;
    auto result=set_placement(h,placement); if (!result) return result;
    slot->source.pos1.x+=dx; slot->source.pos1.y+=dy; slot->source.pos1.z+=dz;
    slot->source.targetPos.x+=dx; slot->source.targetPos.y+=dy; slot->source.targetPos.z+=dz;
    return {};
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
        if (!slot.joined || slot.pending_die_commands) return rejected(OwnerError::lifetime,"actor real native join/queued identity still outstanding");
        if (slot.object) impl_->sound.release_identity(slot.object);
        if (slot.identity_held) {
            if (isle_game_object_release_identity(impl_->services.game,slot.object)!=ISLE_GAME_OK)
                return rejected(OwnerError::native,"actor joined native identity release requires outside-OM collection");
            slot.identity_held=false;
        }
        if (!impl_->runtime->release_retired(slot.instance)) return rejected(OwnerError::animation,"actor physical Runtime release");
        const auto index=slot.index;
        auto credits=std::move(slot.incoming_identity_credits);
        slot=Impl::Slot{}; slot.owner=impl_.get(); slot.index=index;
        slot.incoming_identity_credits=std::move(credits);
    }
    return {};
}
OwnerStatus Owner::reset() noexcept {
    auto& self=*impl_;
    if (self.services.failed()) {
        for (auto& slot:self.slots) if (slot.active && !slot.retiring) self.services.erase(slot.object);
        const auto collected=collect_joined();
        if (!collected) return collected;
        self.fx.reset();
        return rejected(OwnerError::services,"failed native Services requires application reconstruction, not successful actor reset");
    }
    // Consume actual receiver FIFOs BEFORE retiring them. A queued retiring
    // source remains comparison-only and cannot name a recycled actor.
    for (auto& slot:self.slots) if (slot.active && !slot.retiring && !self.services.process_commands(slot.object))
        return rejected(OwnerError::services,"actor reset command drain");
    for (auto& slot:self.slots) if (slot.active && !slot.retiring) {
        auto result=self.cleanup(slot); if (!result) return result;
        for (auto& receiver:self.slots) if (receiver.active && !receiver.retiring && !self.services.process_commands(receiver.object))
            return rejected(OwnerError::services,"actor reset die-command drain");
    }
    auto result=collect_joined(); if (!result) return result;
    self.fx.reset(); self.player.previous_focus={}; self.player.previous_focus_identity=nullptr; self.player.focus_latch=false;
    self.player.focus_timer=0; self.player.direction_index=0;
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
IsleGameObject* Owner::particle_object() const noexcept { return impl_->particle_object; }
IsleGameObject* Owner::effect_object() const noexcept { return impl_->effect_object; }
OwnerStatus Owner::update_particles() noexcept { return impl_->fx.update_particles()?OwnerStatus{}:rejected(OwnerError::services,"actor particles update"); }
OwnerStatus Owner::update_effects() noexcept { return impl_->fx.update_effects(Impl::resolve,impl_.get())?OwnerStatus{}:rejected(OwnerError::services,"actor effects update"); }
OwnerStatus Owner::set_player(IsleGameObject* object,IsleGameModel* model,IsleGameCamera* camera) noexcept {
    if ((object==nullptr)!=(model==nullptr) || (model==nullptr)!=(camera==nullptr))
        return rejected(OwnerError::admission,"player object/model/camera binding must be genuinely present or absent");
    if ((object && isle_actor_source_validate_model(impl_->services.game,object,model)!=ISLE_GAME_OK) || !impl_->sound.set_listener(model,camera))
        return rejected(OwnerError::native,"actual player/listener binding");
    impl_->player.object=object; impl_->player.model=model; impl_->player.camera=camera;
    return {};
}
OwnerStatus Owner::set_focus(menu::animation::InstanceHandle h,bool latch,float timer) noexcept {
    if (!std::isfinite(timer)) return rejected(OwnerError::admission,"source focus timer must be finite");
    auto* slot=impl_->find(h);
    if (h && !slot) return rejected(OwnerError::lifetime,"source focus requires current full instance");
    impl_->player.previous_focus=h; impl_->player.previous_focus_identity=slot?slot->object:nullptr;
    impl_->player.focus_latch=latch; impl_->player.focus_timer=timer; return {};
}
void Owner::set_direction(std::int32_t direction) noexcept { impl_->player.direction_index=direction; }
const PlayerState& Owner::player() const noexcept { return impl_->player; }
CourseState& Owner::course() noexcept { return impl_->course; }
const CourseState& Owner::course() const noexcept { return impl_->course; }
} // namespace isle::actor

extern "C" {
IsleGameObject* isle_actor_owner_spawn_ground_dl4(int32_t object_id,uint16_t id,
    IsleActorFactoryBlock* block,IsleActorFactoryBlock* reference,IsleActorFactorySpawn* spawn,
    const IsleActorFactoryInitialization* initialization) {
    if (!block || !reference || !spawn || !initialization || initialization->identity!=0x80388348u ||
        id!=1003 || !block->owner || reference->owner!=block->owner || !block->result || reference->result!=block->result)
        return nullptr;
    auto& owner=*static_cast<isle::actor::Owner*>(block->owner);
    auto& result=*static_cast<isle::actor::SpawnResult*>(block->result);
    result=isle::actor::SourceAccess::spawn(owner,{static_cast<uint32_t>(object_id),spawn->definition,
        static_cast<isle::world::Block*>(block->block),static_cast<isle::world::Block*>(reference->block),
        static_cast<const isle::world::Spawn*>(spawn->spawn)});
    return result.object;
}
IsleGameObject* isle_actor_owner_spawn_ground(int32_t object_id,uint16_t id,
    IsleActorFactoryBlock* block,IsleActorFactoryBlock* reference,IsleActorFactorySpawn* spawn,
    const IsleActorFactoryInitialization* initialization) {
    if (!block || !reference || !spawn || !initialization || initialization->identity!=0x802cdaecu ||
        id!=129 || !block->owner || reference->owner!=block->owner || !block->result || reference->result!=block->result)
        return nullptr;
    auto& owner=*static_cast<isle::actor::Owner*>(block->owner);
    auto& result=*static_cast<isle::actor::SpawnResult*>(block->result);
    result=isle::actor::SourceAccess::spawn(owner,{static_cast<uint32_t>(object_id),spawn->definition,
        static_cast<isle::world::Block*>(block->block),static_cast<isle::world::Block*>(reference->block),
        static_cast<const isle::world::Spawn*>(spawn->spawn)});
    return result.object;
}
IsleGameResult isle_actor_owner_animation(IsleActorSource* source,uint32_t identity,float start,int force) {
    auto* slot=isle::actor::SourceAccess::slot(source);
    if (!slot) return ISLE_GAME_INVALID_STATE;
    const isle::beach::ActorAnimationHeader* header=nullptr;
    if (identity) {
        for (const auto& candidate:slot->owner->resources.headers())
            if (candidate.wire.guest_address==identity && candidate.initialization==slot->definition->initialization) {
                header=&candidate; break;
            }
        if (!header) return ISLE_GAME_INVALID_ARGUMENT;
    }
    return slot->owner->animation(*slot,header,start,force!=0)?ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE;
}
IsleGameResult isle_actor_owner_step(IsleActorSource* source) {
    auto* slot=isle::actor::SourceAccess::slot(source);
    return slot && slot->owner->step(*slot)?ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE;
}
IsleGameResult isle_actor_owner_interactions(IsleActorSource* source) {
    auto* slot=isle::actor::SourceAccess::slot(source);
    return slot && !source->transitionGraph?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_actor_owner_cleanup(IsleActorSource* source) {
    auto* slot=isle::actor::SourceAccess::slot(source);
    return slot && slot->owner->cleanup(*slot)?ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE;
}
IsleGameResult isle_actor_owner_commands(IsleActorSource* source) {
    auto* slot=isle::actor::SourceAccess::slot(source);
    return slot && slot->owner->services.process_commands(slot->object)?ISLE_GAME_OK:ISLE_GAME_EXECUTION_FAILURE;
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
    return slot && slot->owner->course.paused;
}
IsleGameResult isle_actor_owner_yaw(IsleActorSource* source,float yaw) {
    auto* slot=isle::actor::SourceAccess::slot(source);
    if (!slot || source->id!=129) return ISLE_GAME_INVALID_STATE;
    const IsleGameModelPlacement* current=nullptr;
    if (isle_game_model_placement(source->game,slot->model,&current)!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE;
    auto placement=current->transform; placement.rotation.y=yaw;
    return isle_game_model_set_placement(source->game,slot->model,ISLE_GAME_MODEL_PLACEMENT_RTS,&placement);
}
IsleGameResult isle_actor_owner_magikarp_active(IsleActorSource* source,int active) {
    auto* slot=isle::actor::SourceAccess::slot(source);
    if (!slot || source->id!=129) return ISLE_GAME_INVALID_STATE;
    slot->owner->course.magikarp_active=active!=0;
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
    auto* slot=isle::actor::SourceAccess::slot(source);
    if (!slot || !position || !velocity) return ISLE_GAME_INVALID_ARGUMENT;
    isle::menu::EffectWorld world{};
    if (!isle::menu::model::source_effect_world(source->game,slot->model,node,world)) return ISLE_GAME_EXECUTION_FAILURE;
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
IsleGameResult isle_actor_owner_ground(IsleActorSource* source,float x,float z,IsleActorGround* output) {
    auto* slot=isle::actor::SourceAccess::slot(source);
    if (!slot || !output || !std::isfinite(x) || !std::isfinite(z)) return ISLE_GAME_INVALID_ARGUMENT;
    isle::world::GroundResult ground{};
    slot->owner->world.ground_at(x,z,ground);
    output->height=ground.height;
    return ISLE_GAME_OK;
}
void* isle_actor_owner_current_block(IsleActorSource* source) {
    auto* slot=isle::actor::SourceAccess::slot(source);
    return slot?slot->owner->world.current():nullptr;
}
IsleGameResult isle_actor_owner_add_one(IsleActorSource* source,void* block,void* reference,
    const IsleActorGeoSpawn* spawn,const IsleActorGeoDefinition* definition,IsleGameObject** output) {
    if (output) *output=nullptr;
    auto* slot=isle::actor::SourceAccess::slot(source);
    if (!slot || !spawn || !definition || !output || definition->identity!=0x8038837cu ||
        spawn->id!=1003 || block!=slot->owner->world.current() || reference!=block || !block)
        return ISLE_GAME_INVALID_ARGUMENT;
    auto& self=*slot->owner;
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
    size_t definition_index=isle::beach::actor_no_index;
    for (size_t i=0;i<self.resources.definitions().size();++i)
        if (self.resources.definitions()[i].wire.guest_address==definition->identity) { definition_index=i; break; }
    if (definition_index==isle::beach::actor_no_index) return ISLE_GAME_INVALID_ARGUMENT;
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
}

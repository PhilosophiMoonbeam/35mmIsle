#include "item_owner.hpp"
#include "actor_owner_source.h"
#include "actor_spatial.h"
#include "source_factory_resources.h"
#include <algorithm>
#include <exception>
#include <stdexcept>
#include <utility>

namespace isle::items {
namespace {
OwnerStatus failure(OwnerError error, const char* detail) noexcept { return {error,detail}; }
OwnerStatus status(IsleGameResult result) noexcept {
    if (result==ISLE_GAME_OK) return {};
    if (result==ISLE_GAME_EXHAUSTED) return failure(OwnerError::capacity,"caller Item physical/native capacity exhausted");
    return failure(OwnerError::native,"nonwaiting Item source operation failed");
}
IsleGameResult native_status(menu::animation::Status result) noexcept {
    if (result) return ISLE_GAME_OK;
    return result.error==menu::animation::Error::capacity ||
        result.error==menu::animation::Error::exhausted ||
        result.error==menu::animation::Error::allocation?ISLE_GAME_EXHAUSTED:ISLE_GAME_INVALID_STATE;
}
IsleGameModelTransform native(const menu::animation::Transform& pose) noexcept {
    return {{pose.position.x,pose.position.y,pose.position.z},{pose.rotation.x,pose.rotation.y,pose.rotation.z},
            {pose.scale.x,pose.scale.y,pose.scale.z}};
}
menu::animation::Transform animated(const IsleGameModelTransform& pose) noexcept {
    return {{pose.position.x,pose.position.y,pose.position.z},{pose.rotation.x,pose.rotation.y,pose.rotation.z},
            {pose.scale.x,pose.scale.y,pose.scale.z}};
}
IsleGameResult player_context(IsleGame* game,const IslePlayerSource& source) noexcept {
    IslePlayerSource* active=nullptr;
    const auto result=isle_player_source_context(game,&active);
    if (result!=ISLE_GAME_OK) return result;
    return active==&source && source.game==game?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
}
IsleGameResult player_aux(IsleGame* game,const IslePlayerSource& binding,
    const IsleGameModelPlacement** output) noexcept {
    if (output) *output=nullptr;
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    auto result=player_context(game,binding);
    if (result!=ISLE_GAME_OK) return result;
    if (!binding.object || !binding.model) return ISLE_GAME_INVALID_STATE;
    IslePlayerSource* published=nullptr;
    result=isle_player_source_get(game,binding.object,&published);
    if (result!=ISLE_GAME_OK) return result;
    if (published!=&binding) return ISLE_GAME_INVALID_STATE;
    result=isle_game_model_validate_binding(game,binding.object,binding.model);
    if (result!=ISLE_GAME_OK) return result;
    const IsleGameModelPlacement* placement=nullptr;
    result=isle_game_model_placement(game,binding.model,&placement);
    if (result!=ISLE_GAME_OK) return result;
    if (!placement || (placement->mode!=ISLE_GAME_MODEL_PLACEMENT_RT &&
        placement->mode!=ISLE_GAME_MODEL_PLACEMENT_RTS)) return ISLE_GAME_INVALID_STATE;
    *output=placement;
    return ISLE_GAME_OK;
}
IsleGameResult published_player(IsleGame* game,const IslePlayerSource& binding) noexcept {
    if (!binding.camera) return ISLE_GAME_INVALID_STATE;
    const IsleGameModelPlacement* placement=nullptr;
    auto result=player_aux(game,binding,&placement);
    if (result!=ISLE_GAME_OK) return result;
    const IsleGameCameraParameters* camera=nullptr;
    result=isle_game_camera_parameters(game,binding.camera,&camera);
    if (result!=ISLE_GAME_OK) return result;
    return camera?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
}
// Public calls are nonwaiting status boundaries, never C execution fences.
struct Entry {
    IsleItemGlobals& g;
    explicit Entry(IsleItemGlobals& state) noexcept : g(state) { if (!g.nonwaiting) g.status=ISLE_GAME_OK; ++g.nonwaiting; }
    ~Entry() { --g.nonwaiting; }
};
}
struct PayloadStorage {
    struct Payload {
        IsleItemSource source{};
        bool used=false, held=false, cached=false;
    };
    std::vector<Payload> records;
    std::array<std::size_t,20> cache{};
};
Globals::Globals() : payload_storage(std::make_unique<PayloadStorage>()) {}
Globals::~Globals() {
    if (source.owner) std::terminate();
}
struct Owner::Impl {
    using Payload=PayloadStorage::Payload;
    struct Slot {
        Impl* owner=nullptr;
        std::size_t index=0;
        menu::animation::InstanceHandle instance;
        IsleGameObject* object=nullptr;
        IsleGameModel* model=nullptr;
        Payload* payload=nullptr;
        // Typed write-through staging only. Every C mutation publishes directly
        // into the authoritative Runtime and native root before another action.
        IsleGameModelTransform root{};
        Family family=Family::pester_ball;
        scene::RenderMetadata render{3,0x80000000u,UINT32_MAX};
        std::uint64_t insertion=0;
        bool active=false,retiring=false,joined=false,source_owned=false;
    };
    struct Factory {
        enum class Phase { idle, preparing, committed };
        Phase phase=Phase::idle;
        Slot* slot=nullptr;
        Payload* payload=nullptr;
        std::size_t cache_index=menu::no_index;
        Family family=Family::pester_ball;
        scene::Services::ObjectPreparation object;
        IsleSourceFactoryResources native{};
        bool object_prepared=false,runtime_prepared=false;
    } factory;
    scene::Services& services;
    const Resources& resources;
    world::World& world;
    actor::Owner& actors;
    Globals& globals;
    IslePlayerSource& player;
    std::vector<menu::animation::SlotCapacity> capacities;
    std::vector<Slot> slots;
    PayloadStorage& storage;
    std::vector<menu::model::PacketBinding> bindings;
    std::vector<IsleGameModelNodeInit> scratch;
    std::unique_ptr<menu::animation::Runtime> runtime;
    std::uint64_t insertion=0;
    Impl* next=nullptr;
    static Impl* owners;

    Impl(scene::Services& s,const Resources& r,world::World& w,actor::Owner& a,Globals& g,IslePlayerSource& p,
         std::span<const menu::animation::SlotCapacity> c,menu::animation::ExecutionCapacity execution)
        : services(s),resources(r),world(w),actors(a),globals(g),player(p),capacities(c.begin(),c.end()),
          slots(c.size()),storage(*g.payload_storage),bindings(c.size()) {
        resources.require_live();
        if (c.empty() || g.source.owner)
            throw std::invalid_argument("Item owner needs caller physical capacity and released Globals");
        if (g.source.extra || g.source.extra_context || g.source.extra_active)
            throw std::invalid_argument("Item owner requires the prior extra collision binding cleared after joins");
        if (r.definitions().size()!=4 || r.models().size()!=4)
            throw std::invalid_argument("Item four-family closure changed");
        for (const auto& definition:r.definitions()) {
            if (definition.model>=r.models().size())
                throw std::invalid_argument("Item family model escapes resource owner");
            const auto& model=r.models()[definition.model];
            const bool fume=definition.family==Family::water_splash || definition.family==Family::lava_splash;
            if (model.render_type!=(fume?menu::RenderType::d_fogged:menu::RenderType::b_fogged) ||
                model.matrix_kinds!=std::array<std::uint8_t,3>{0x1c,0,0})
                throw std::invalid_argument("Item source fog/hierarchy/root policy changed");
        }
        if (storage.records.empty()) storage.records.resize(c.size());
        else if (storage.records.size()!=c.size())
            throw std::invalid_argument("retained Item Globals requires its original fixed physical capacity");
        std::size_t maximum=0;
        for (std::size_t i=0;i<c.size();++i) { slots[i].owner=this; slots[i].index=i; maximum=std::max(maximum,c[i].nodes); }
        scratch.resize(maximum);
        auto made=menu::animation::Runtime::create({r.models(),r.materials(),r.animations(),menu::no_index,{}},c,{},execution);
        if (!made) throw std::invalid_argument("Item Runtime admission failed");
        runtime=std::move(made.runtime);
        g.source.owner=this; g.source.game=s.game; g.source.status=ISLE_GAME_OK;
        next=owners; owners=this;
    }
    ~Impl() {
        Impl** at=&owners;
        while (*at && *at!=this) at=&(*at)->next;
        if (*at) *at=next;
        globals.source.owner=nullptr; globals.source.game=nullptr;
    }
    Slot* find(IsleGameObject* object,bool retiring=false) noexcept {
        if (!object) return nullptr;
        for (auto& slot:slots) if (slot.active && slot.object==object && (retiring || !slot.retiring)) return &slot;
        return nullptr;
    }
    Slot* find(menu::animation::InstanceHandle instance,bool retiring=false) noexcept {
        if (!instance || instance.slot>=slots.size()) return nullptr;
        auto& slot=slots[instance.slot];
        return slot.active && slot.instance==instance && (retiring || !slot.retiring)?&slot:nullptr;
    }
    IsleGameResult read_root(Slot& slot,IsleGameModelTransform** output) noexcept {
        if (!output) return ISLE_GAME_INVALID_ARGUMENT;
        *output=nullptr;
        const IsleGameModelNode* nodes=nullptr; std::size_t count=0;
        const auto result=isle_game_model_peek_nodes(services.game,slot.model,&nodes,&count);
        if (result!=ISLE_GAME_OK) return result;
        if (!count) return ISLE_GAME_INVALID_STATE;
        slot.root=nodes[0].transform; *output=&slot.root;
        return ISLE_GAME_OK;
    }
    IsleGameResult publish(Slot& slot) noexcept {
        auto result=isle_game_model_validate_transform(services.game,slot.model,0,&slot.root);
        if (result!=ISLE_GAME_OK) return result;
        if (!runtime->set_node_transform(slot.instance,0,animated(slot.root))) return ISLE_GAME_INVALID_STATE;
        return isle_game_model_set_transform(services.game,slot.model,0,&slot.root);
    }
    IsleGameResult step(Slot& slot) noexcept {
        IsleGameObjectMetadata metadata{};
        if (isle_game_object_metadata(services.game,slot.object,&metadata)!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE;
        if (!runtime->set_object_flags(slot.instance,metadata.flags) || !runtime->step_model(slot.instance)) return ISLE_GAME_INVALID_STATE;
        const auto state=runtime->model(slot.instance);
        if (!state || state.nodes.empty()) return ISLE_GAME_INVALID_STATE;
        for (std::size_t i=0;i<state.nodes.size();++i) {
            const auto pose=native(state.nodes[i].transform);
            const auto result=isle_game_model_validate_transform(services.game,slot.model,i,&pose);
            if (result!=ISLE_GAME_OK) return result;
        }
        for (std::size_t i=0;i<state.nodes.size();++i) {
            const auto pose=native(state.nodes[i].transform);
            auto result=isle_game_model_set_transform(services.game,slot.model,i,&pose);
            if (result!=ISLE_GAME_OK) return result;
            result=isle_game_model_set_flags(services.game,slot.model,i,state.nodes[i].flags);
            if (result!=ISLE_GAME_OK) return result;
        }
        slot.root=native(state.nodes[0].transform);
        // Item source never installs an animation callback, even on water16.
        return runtime->events().empty()?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    }
    static void retiring(IsleGameObject*,void* context) noexcept {
        auto& slot=*static_cast<Slot*>(context);
        if (slot.retiring) return;
        slot.retiring=true;
        if (slot.payload) slot.payload->held=true;
        if (!slot.owner->runtime->begin_retire(slot.instance)) slot.owner->services.fail("Item Runtime retirement");
    }
    static void joined(IsleGameObject*,void* context) noexcept {
        auto& slot=*static_cast<Slot*>(context); slot.joined=true; slot.model=nullptr;
    }
    bool failed() const noexcept {
        return services.failed() || isle_game_status(services.game)!=ISLE_GAME_OK;
    }
    void cache_payload(Payload& payload) noexcept {
        if (!payload.cached && globals.source.payload_cache<storage.cache.size()) {
            storage.cache[globals.source.payload_cache++]=static_cast<std::size_t>(&payload-storage.records.data());
            payload.cached=true;
        }
        payload.held=true;
    }
    OwnerStatus dispose_failed() noexcept {
        services.fail("failed Item scene requires reconstruction after retained retirement");
        auto& g=globals.source;
        // Failed-cohort disposal is not the live Source delete algorithm. Use
        // actual retained list membership without GET_ITEM, commands or work.
        while (g.allocated_head) {
            auto* entry=g.allocated_head;
            g.allocated_head=entry->next;
            if (g.allocated_head) g.allocated_head->prev=nullptr;
            else g.allocated_tail=nullptr;
            entry->obj=nullptr; entry->next=g.free_head; g.free_head=entry;
        }
        g.total=0; g.balls=0; g.apples=0;
        for (auto& slot:slots) if (slot.active) {
            if (slot.payload) {
                slot.payload->source.flags|=ISLE_ITEM_DELETED;
                slot.payload->source.state=ISLE_ITEM_INVALID;
                cache_payload(*slot.payload);
            }
            if (!slot.retiring && slot.object) services.erase(slot.object);
        }
        return failure(OwnerError::services,"Item failed-scene disposal retained terminal Services status");
    }
    IsleGameResult cancel_factory() noexcept {
        IsleGameResult result=ISLE_GAME_OK;
        if (factory.native.active) result=isle_source_factory_cancel(&factory.native);
        if (factory.object_prepared) {
            const auto cancelled=services.cancel_object(factory.object);
            if (cancelled==ISLE_GAME_OK) factory.object_prepared=false;
            else if (result==ISLE_GAME_OK) result=cancelled;
        }
        if (factory.runtime_prepared) {
            const auto cancelled=native_status(runtime->cancel_prepared(factory.slot->index));
            if (cancelled==ISLE_GAME_OK) factory.runtime_prepared=false;
            else if (result==ISLE_GAME_OK) result=cancelled;
        }
        if (!factory.native.active && !factory.object_prepared && !factory.runtime_prepared) {
            factory.phase=Factory::Phase::idle; factory.slot=nullptr; factory.payload=nullptr;
            factory.cache_index=menu::no_index; factory.native={};
        }
        return result;
    }
    IsleGameResult terminal(IsleGameResult result) noexcept {
        services.fail("Item source factory failed after admission");
        if (globals.source.status==ISLE_GAME_OK) globals.source.status=result;
        const auto cancelled=cancel_factory();
        if (cancelled!=ISLE_GAME_OK) services.fail("Item unpublished factory cancellation failed");
        return result;
    }
    IsleGameResult prepare(int family,IsleGameObject* eviction) noexcept {
        if (family<0 || family>3) return ISLE_GAME_INVALID_ARGUMENT;
        if (failed() || factory.phase!=Factory::Phase::idle || globals.source.status!=ISLE_GAME_OK)
            return ISLE_GAME_INVALID_STATE;
        const auto typed=static_cast<Family>(family);
        const auto& definition=resources.definition(typed);
        const auto required=menu::animation::slot_capacity(resources.models()[definition.model]);
        Slot* selected=nullptr;
        for (auto& slot:slots) if (!slot.active && capacities[slot.index].nodes>=required.nodes &&
            capacities[slot.index].material_attachments>=required.material_attachments) { selected=&slot; break; }
        if (!selected) return ISLE_GAME_EXHAUSTED;
        Payload* candidate=nullptr;
        std::size_t cache_index=menu::no_index;
        if (family<2) {
            if (!globals.source.update || globals.source.update_kind>1) return ISLE_GAME_INVALID_STATE;
            if (!globals.source.free_head && !eviction) return ISLE_GAME_EXHAUSTED;
            for (std::size_t i=globals.source.payload_cache;i>0;--i) {
                auto& value=storage.records[storage.cache[i-1]];
                if (!value.held && !value.used) { candidate=&value; cache_index=i-1; break; }
            }
            if (!candidate) for (auto& value:storage.records)
                if (!value.used && !value.held && !value.cached) { candidate=&value; break; }
            if (!candidate) return ISLE_GAME_EXHAUSTED;
        } else if (eviction) return ISLE_GAME_INVALID_ARGUMENT;
        if (eviction) {
            const auto* old=find(eviction);
            if (!old || !old->payload || old->payload->source.state==ISLE_ITEM_INVALID ||
                (old->payload->source.flags&ISLE_ITEM_DELETED)) return ISLE_GAME_INVALID_STATE;
        }
        factory.phase=Factory::Phase::preparing; factory.slot=selected; factory.payload=candidate;
        factory.cache_index=cache_index; factory.family=typed;
        auto result=native_status(runtime->prepare_acquire(selected->index,definition.model,false));
        if (result==ISLE_GAME_OK) {
            factory.runtime_prepared=true;
            result=services.prepare_object(factory.object);
        }
        if (result==ISLE_GAME_OK) {
            factory.object_prepared=true;
            IsleSourceFactoryProcess processes[ISLE_SOURCE_FACTORY_PROCESS_CAPACITY]{};
            std::size_t count=0;
            processes[count++]={nullptr,isle_item_source_animation_process,ISLE_GAME_SYNCHRONOUS,3};
            if (family<2) {
                processes[count++]={nullptr,globals.source.update,
                    static_cast<IsleGameProcessKind>(globals.source.update_kind),7};
                processes[count++]={nullptr,Items_ShowDelayed,ISLE_GAME_STACKFUL,7};
                if (eviction) processes[count++]={eviction,Items_DoRemoveItem,ISLE_GAME_SYNCHRONOUS,4};
            } else processes[count++]={nullptr,Items_UpdateFume,ISLE_GAME_STACKFUL,7};
            result=isle_source_factory_prepare(services.game,required.nodes,processes,count,&factory.native);
        }
        if (result!=ISLE_GAME_OK) {
            const auto cancelled=cancel_factory();
            if (cancelled!=ISLE_GAME_OK) {
                services.fail("Item factory admission could not cancel actual unpublished ownership");
                return cancelled;
            }
            return result;
        }
        factory.phase=Factory::Phase::committed;
        return ISLE_GAME_OK;
    }
    IsleGameResult process(IsleGameObject* object,IsleGameSourceCallback callback,
        IsleGameProcessKind kind,std::uint32_t priority,IsleGameProcess** output) noexcept {
        if (output) *output=nullptr;
        if (failed() || globals.source.status!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE;
        const auto result=factory.phase==Factory::Phase::committed?
            isle_source_factory_publish_process(&factory.native,object,callback,kind,priority,output):
            factory.phase==Factory::Phase::idle?
                isle_game_add_source_process(services.game,object,callback,kind,priority,output):ISLE_GAME_INVALID_STATE;
        return result==ISLE_GAME_OK?result:terminal(result);
    }
    IsleGameResult finish() noexcept {
        if (globals.source.status!=ISLE_GAME_OK) {
            const auto result=globals.source.status;
            if (factory.phase!=Factory::Phase::idle) return terminal(result);
            return result;
        }
        if (factory.phase!=Factory::Phase::committed || factory.object_prepared || factory.runtime_prepared)
            return terminal(ISLE_GAME_INVALID_STATE);
        const auto result=isle_source_factory_finish(&factory.native);
        if (result!=ISLE_GAME_OK) return terminal(result);
        return cancel_factory();
    }
    IsleGameResult create(std::uint32_t id,int family,IsleGameObject** output) noexcept {
        if (!output) return ISLE_GAME_INVALID_ARGUMENT;
        *output=nullptr;
        if (factory.phase!=Factory::Phase::committed || family!=static_cast<int>(factory.family) ||
            !factory.object_prepared || !factory.runtime_prepared || failed())
            return terminal(ISLE_GAME_INVALID_STATE);
        auto& slot=*factory.slot;
        const auto& definition=resources.definition(factory.family);
        const auto& model=resources.models()[definition.model];
        const auto acquired=runtime->acquire_prepared(slot.index);
        if (!acquired) return terminal(native_status(acquired.status));
        factory.runtime_prepared=false;
        slot.active=true; slot.instance=acquired.handle; slot.family=factory.family;
        auto result=services.publish_object(factory.object,factory.native,id,4,0x80000000u,&slot.object);
        if (result==ISLE_GAME_OK || slot.object) factory.object_prepared=false;
        if (result!=ISLE_GAME_OK) {
            if (!slot.object) {
                slot.retiring=true; slot.joined=true;
                if (!runtime->begin_retire(slot.instance)) services.fail("Item unpublished Runtime retirement");
            }
            return terminal(result);
        }
        if (!services.set_lifecycle(slot.object,{retiring,joined,&slot}) ||
            !services.set_update(slot.object,family<2?isle_item_source_update:isle_item_source_default_update,nullptr) ||
            (family<2 && !services.set_command_handler(slot.object,isle_item_source_command,&globals.source)) ||
            !services.set_render(slot.object,slot.render)) return terminal(ISLE_GAME_INVALID_STATE);
        if (!menu::model::attach_source_model(services.game,slot.object,model,runtime->model(slot.instance),
                menu::model::RootRecipe::source,scratch,&slot.model,&factory.native))
            return terminal(ISLE_GAME_INVALID_STATE);
        // Source Item factories attach plain local trees, not Pokemon AUX.
        // An identity placement would make generic distance read synthetic zero.
        slot.root=native(runtime->model(slot.instance).nodes[0].transform);
        slot.root.scale={0.1f,0.1f,0.1f};
        result=publish(slot);
        if (result!=ISLE_GAME_OK) return terminal(result);
        if (!runtime->set_texture_speed(slot.instance,definition.texture_speed) ||
            (family>=2 && !runtime->set_model_speed(slot.instance,0.9f))) return terminal(ISLE_GAME_INVALID_STATE);
        if (factory.payload) {
            auto& payload=*factory.payload;
            if (payload.used || payload.held) return terminal(ISLE_GAME_INVALID_STATE);
            if (factory.cache_index!=menu::no_index) {
                auto& count=globals.source.payload_cache;
                if (factory.cache_index>=count || &storage.records[storage.cache[factory.cache_index]]!=&payload)
                    return terminal(ISLE_GAME_INVALID_STATE);
                for (std::size_t j=factory.cache_index+1;j<count;++j) storage.cache[j-1]=storage.cache[j];
                storage.cache[--count]=0; payload.cached=false;
            }
            payload.used=true; slot.payload=&payload;
            result=isle_item_source_bind(services.game,slot.object,&payload.source,&slot);
            if (result!=ISLE_GAME_OK) return terminal(result);
        }
        // Independent fumes retain native identity without an Item payload.
        if (!services.source_owner_register(slot.object,{})) return terminal(ISLE_GAME_INVALID_STATE);
        slot.source_owned=true;
        IsleGameProcess* animation=nullptr;
        result=process(slot.object,isle_item_source_animation_process,ISLE_GAME_SYNCHRONOUS,3,&animation);
        if (result!=ISLE_GAME_OK) return result;
        slot.insertion=++insertion; *output=slot.object;
        return ISLE_GAME_OK;
    }
};
Owner::Impl* Owner::Impl::owners=nullptr;
struct SourceAccess {
    static Owner::Impl* impl(IsleItemGlobals* g) noexcept {
        if (!g || !g->owner) return nullptr;
        auto* self=static_cast<Owner::Impl*>(g->owner);
        return &self->globals.source==g && self->services.game==g->game?self:nullptr;
    }
    static Owner::Impl::Slot* slot(IsleItemGlobals* g,IsleGameObject* object) noexcept {
        auto* self=impl(g);
        if (!self || !object || self->failed()) return nullptr;
        IsleItemSource* source=nullptr;
        if (isle_item_source_get(g->game,object,&source)==ISLE_GAME_OK) {
            auto* result=static_cast<Owner::Impl::Slot*>(source->owner);
            return result && result->owner==self && result->active && !result->retiring &&
                result->object==object && result->payload && &result->payload->source==source?result:nullptr;
        }
        auto* result=self->find(object);
        return result && !result->payload?result:nullptr;
    }
    static IsleItemGlobals* globals(IsleGameObject* object) noexcept {
        for (auto* self=Owner::Impl::owners;self;self=self->next)
            if (slot(&self->globals.source,object)) return &self->globals.source;
        return nullptr;
    }
};
Owner::Owner(scene::Services& s,const Resources& r,world::World& w,actor::Owner& a,Globals& g,IslePlayerSource& p,
             std::span<const menu::animation::SlotCapacity> capacities,menu::animation::ExecutionCapacity execution) {
    if (&a.player_source()!=&p || player_context(s.game,p)!=ISLE_GAME_OK)
        throw std::invalid_argument("Item owner requires the Actor's active canonical Player Source");
    impl_=std::make_unique<Impl>(s,r,w,a,g,p,capacities,execution);
}
Owner::~Owner() {
    // Sticky scene failure is not ownership. Always perform both phases, then
    // reject only real retained native/semantic/Runtime/preparation ownership.
    retire();
    collect_joined();
    if (impl_->factory.phase!=Impl::Factory::Phase::idle ||
        impl_->factory.native.active || impl_->factory.object_prepared || impl_->factory.runtime_prepared ||
        std::any_of(impl_->slots.begin(),impl_->slots.end(),[](const auto& slot) { return slot.active; }))
        std::terminate();
    if (Items_func_8035CA1C(&impl_->globals.source,impl_->services.game,nullptr,nullptr)!=ISLE_GAME_OK)
        std::terminate();
}
OwnerStatus Owner::init() noexcept {
    if (impl_->failed()) return failure(OwnerError::services,"failed Item scene cannot initialize live source state");
    if (published_player(impl_->services.game,impl_->player)!=ISLE_GAME_OK)
        return failure(OwnerError::admission,"Item initialization requires the published native Player model and camera");
    Entry entry(impl_->globals.source); Items_Init(&entry.g); return status(entry.g.status);
}
SpawnResult Owner::spawn(Family family,IsleGameVec3 position,IsleGameVec3 velocity) noexcept {
    auto& self=*impl_;
    if (self.failed()) return {failure(OwnerError::services,"failed Item scene cannot create source objects"),{},nullptr};
    Entry entry(self.globals.source);
    if (!entry.g.update || published_player(self.services.game,self.player)!=ISLE_GAME_OK)
        return {failure(OwnerError::admission,"Item factory requires published native Player and configured source update"),{},nullptr};
    IsleGameObject* object=nullptr;
    auto result=family==Family::pester_ball?Items_SpawnPesterBall(&entry.g,&position,&velocity,&object):
        Items_SpawnApple(&entry.g,&position,&velocity,&object);
    if (result!=ISLE_GAME_OK) {
        if (self.factory.phase!=Impl::Factory::Phase::idle) self.terminal(result);
        return {self.failed()?failure(OwnerError::services,"Item source construction failed terminally"):status(result),{},nullptr};
    }
    auto* slot=self.find(object);
    return slot?SpawnResult{{},slot->instance,object}:SpawnResult{failure(OwnerError::native,"source factory did not publish native object"),{},nullptr};
}
SpawnResult Owner::spawn_ball(IsleGameVec3 p,IsleGameVec3 v) noexcept { return spawn(Family::pester_ball,p,v); }
SpawnResult Owner::spawn_apple(IsleGameVec3 p,IsleGameVec3 v) noexcept { return spawn(Family::apple,p,v); }
OwnerStatus Owner::delete_item(IsleGameObject* object) noexcept {
    if (!object) return {};
    if (impl_->failed()) return failure(OwnerError::services,"failed Item scene requires retained retirement");
    auto* slot=impl_->find(object,true);
    if (!slot || !slot->payload) return failure(OwnerError::admission,"delete requires this Owner's Item identity");
    if (slot->retiring || slot->payload->source.state==ISLE_ITEM_INVALID || (slot->payload->source.flags&ISLE_ITEM_DELETED)) return {};
    Entry entry(impl_->globals.source); Items_DeleteItem(object); return status(entry.g.status);
}
bool Owner::exists(IsleGameObject* object) const noexcept { return object && Items_CheckObjectExists(&impl_->globals.source,object); }
void Owner::init_iterator() noexcept { if (!impl_->failed()) Items_InitIterator(&impl_->globals.source); }
IsleGameObject* Owner::next_valid() noexcept {
    if (impl_->failed()) return nullptr;
    Entry e(impl_->globals.source); return Items_NextValidItem(&e.g);
}
IsleGameObject* Owner::next_still() noexcept {
    if (impl_->failed()) return nullptr;
    Entry e(impl_->globals.source); return Items_func_80359900(&e.g);
}
OwnerStatus Owner::query_status() const noexcept {
    return impl_->failed()?failure(OwnerError::services,"Item scene remains terminally failed"):status(impl_->globals.source.status);
}
bool Owner::matches(scene::Services& services,world::World& world,actor::Owner& actors) const noexcept {
    return &impl_->services==&services && &impl_->world==&world && &impl_->actors==&actors;
}
IsleItemSource* Owner::source(menu::animation::InstanceHandle h) noexcept { auto* s=impl_->find(h); return s && s->payload?&s->payload->source:nullptr; }
const IsleItemSource* Owner::source(menu::animation::InstanceHandle h) const noexcept { auto* s=impl_->find(h); return s && s->payload?&s->payload->source:nullptr; }
IsleItemSource* Owner::source(IsleGameObject* object) noexcept { auto* s=impl_->find(object); return s && s->payload?&s->payload->source:nullptr; }
IsleGameObject* Owner::object(menu::animation::InstanceHandle h) const noexcept { auto* s=impl_->find(h); return s?s->object:nullptr; }
IsleGameModel* Owner::native_model(menu::animation::InstanceHandle h) const noexcept { auto* s=impl_->find(h); return s?s->model:nullptr; }
menu::animation::InstanceHandle Owner::instance(IsleGameObject* o) const noexcept { auto* s=impl_->find(o); return s?s->instance:menu::animation::InstanceHandle{}; }
menu::animation::InstanceHandle Owner::slot_instance(std::size_t index) const noexcept {
    if (index>=impl_->slots.size()) return {};
    const auto& slot=impl_->slots[index];
    return slot.active && !slot.retiring?slot.instance:menu::animation::InstanceHandle{};
}
Membership Owner::membership(menu::animation::InstanceHandle h) const noexcept {
    auto* s=impl_->find(h,true); return {s && s->payload && exists(s->object),s && s->retiring,s && s->joined,impl_->globals.source.payload_cache};
}
std::optional<Family> Owner::family(menu::animation::InstanceHandle h) const noexcept {
    const auto* slot=impl_->find(h);
    return slot?std::optional<Family>{slot->family}:std::nullopt;
}
std::span<const menu::model::PacketBinding> Owner::render_bindings() noexcept {
    std::size_t count=0;
    for (auto& slot:impl_->slots) if (slot.active && !slot.retiring) {
        auto state=impl_->runtime->model(slot.instance);
        if (!state || state.resource_model!=impl_->resources.definition(slot.family).model) {
            impl_->services.fail("Item stale instance or family/resource mismatch");
            state={};
        }
        menu::model::PacketBinding binding{{slot.index,state,slot.object,slot.model,menu::model::PoseSource::native,
            menu::model::RootRecipe::source,true,impl_->runtime.get()},slot.render};
        std::size_t at=count;
        while (at && impl_->slots[impl_->bindings[at-1].model.slot].insertion>slot.insertion) {
            impl_->bindings[at]=impl_->bindings[at-1]; --at;
        }
        impl_->bindings[at]=binding; ++count;
    }
    for (std::size_t i=count;i<impl_->bindings.size();++i) impl_->bindings[i]={};
    return impl_->bindings;
}
menu::animation::Runtime& Owner::animation() noexcept { return *impl_->runtime; }
const menu::animation::Runtime& Owner::animation() const noexcept { return *impl_->runtime; }
OwnerStatus Owner::set_custom_functions(IsleGameSourceCallback update,std::uint8_t kind,IsleItemCollisionCallback collide) noexcept {
    if (impl_->failed()) return failure(OwnerError::services,"failed Item scene rejects new Source hooks");
    if (update && kind>1) return failure(OwnerError::admission,"custom Item process kind is neither stackful nor synchronous");
    Items_SetCustomFunctions(&impl_->globals.source,update,kind,collide); return {};
}
OwnerStatus Owner::set_extra_collision(IsleGame* binding_game,IsleItemCollisionProvider provider,void* context) noexcept {
    if (binding_game!=impl_->services.game)
        return failure(OwnerError::admission,"extra collision binding requires this Item Owner's Game");
    if ((provider || context) && impl_->failed())
        return failure(OwnerError::services,"failed Item scene rejects new extra collision bindings");
    return status(Items_func_8035CA1C(&impl_->globals.source,binding_game,provider,context));
}
OwnerStatus Owner::set_beach_collision(beach::MagikarpScene& scene) noexcept {
    if (impl_->failed()) return failure(OwnerError::services,"failed Item scene rejects new collision hooks");
    impl_->globals.beach_scene=&scene; return set_custom_functions(nullptr,0,isle_item_source_beach);
}
OwnerStatus Owner::pause() noexcept {
    if (impl_->failed()) return failure(OwnerError::services,"failed Item scene rejects live Source pause");
    Entry e(impl_->globals.source); Items_Pause(&e.g); return status(e.g.status);
}
OwnerStatus Owner::resume() noexcept {
    if (impl_->failed()) return failure(OwnerError::services,"failed Item scene rejects live Source resume");
    Entry e(impl_->globals.source); Items_UnPause(&e.g); return status(e.g.status);
}
OwnerStatus Owner::remove_flying() noexcept {
    if (impl_->failed()) return failure(OwnerError::services,"failed Item scene requires retained retirement");
    Entry e(impl_->globals.source); Items_RemoveFlyingItems(&e.g); return status(e.g.status);
}
OwnerStatus Owner::displace(IsleGameVec3 d) noexcept {
    if (impl_->failed()) return failure(OwnerError::services,"failed Item scene rejects live Source displacement");
    Entry e(impl_->globals.source); Items_DisplaceAllItems(&e.g,d.x,d.y,d.z); return status(e.g.status);
}
OwnerStatus Owner::eat(IsleGameObject* actor) noexcept {
    if (impl_->failed()) return failure(OwnerError::services,"failed Item scene rejects Source interactions");
    IsleActorSource* source=nullptr;
    if (isle_actor_source_get(impl_->services.game,actor,&source)!=ISLE_GAME_OK) return failure(OwnerError::admission,"eat requires canonical Actor");
    Entry e(impl_->globals.source); Pokemon_EatApple(&e.g,actor); return status(e.g.status);
}
OwnerStatus Owner::kick(IsleGameObject* actor,float speed) noexcept {
    if (impl_->failed()) return failure(OwnerError::services,"failed Item scene rejects Source interactions");
    IsleActorSource* source=nullptr;
    if (isle_actor_source_get(impl_->services.game,actor,&source)!=ISLE_GAME_OK || !this->source(source->interactionTarget))
        return failure(OwnerError::admission,"kick requires canonical Actor target Item");
    Entry e(impl_->globals.source); Pokemon_KickItem(&e.g,actor,speed); return status(e.g.status);
}
Flute Owner::flute() noexcept { return Flute(impl_->services.audio,impl_->globals.flute); }
OwnerStatus Owner::retire() noexcept {
    auto& self=*impl_;
    const auto cancelled=self.cancel_factory();
    if (cancelled!=ISLE_GAME_OK) self.services.fail("Item shutdown could not cancel unpublished factory ownership");
    if (self.failed()) return self.dispose_failed();
    // Healthy shutdown keeps the exact Source 21/unlink/count/22/cache order.
    Entry entry(self.globals.source);
    for (auto& slot:self.slots) if (slot.active && !slot.retiring && slot.payload) {
        if (slot.payload->source.state!=ISLE_ITEM_INVALID) Items_DeleteItem(slot.object);
        auto result=entry.g.status;
        if (result==ISLE_GAME_OK) result=isle_item_owner_die(&entry.g,slot.object);
        if (result==ISLE_GAME_OK) result=isle_item_owner_free_payload(&entry.g,slot.object);
        if (result!=ISLE_GAME_OK) {
            self.terminal(result);
            return self.dispose_failed();
        }
        self.services.erase(slot.object);
        if (self.failed()) return self.dispose_failed();
    }
    for (auto& slot:self.slots) if (slot.active && !slot.retiring) self.services.erase(slot.object);
    return self.failed()?self.dispose_failed():OwnerStatus{};
}
OwnerStatus Owner::collect_joined() noexcept {
    auto& self=*impl_;
    if (self.factory.phase!=Impl::Factory::Phase::idle)
        return failure(OwnerError::lifetime,"Item unpublished factory ownership remains");
    bool pending=false;
    for (auto& slot:self.slots) if (slot.active && slot.retiring) {
        if (!slot.joined || (slot.source_owned && !self.services.source_owner_collectable(slot.object))) {
            pending=true;
            continue;
        }
        if (slot.source_owned) {
            if (!self.services.source_owner_release(slot.object))
                return failure(OwnerError::lifetime,"Item identity release requires outside native/command traversal");
            slot.source_owned=false;
        }
        if (!self.runtime->release_retired(slot.instance)) return failure(OwnerError::animation,"Item physical Runtime release failed");
        if (slot.payload) { slot.payload->source={}; slot.payload->used=false; slot.payload->held=false; }
        const auto index=slot.index; slot=Impl::Slot{}; slot.owner=&self; slot.index=index;
    }
    return pending?failure(OwnerError::lifetime,"Item native join/source sender or field borrows remain"):OwnerStatus{};
}
} // namespace isle::items

extern "C" {
IsleItemGlobals* isle_item_owner_globals(IsleGameObject* object) { return isle::items::SourceAccess::globals(object); }
IsleGameResult isle_item_owner_root(IsleItemGlobals* g,IsleGameObject* o,IsleGameModelTransform** out) {
    if (out) *out=nullptr;
    auto* slot=isle::items::SourceAccess::slot(g,o); return slot?slot->owner->read_root(*slot,out):ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_item_owner_publish(IsleItemGlobals* g,IsleGameObject* o) {
    auto* slot=isle::items::SourceAccess::slot(g,o); return slot?slot->owner->publish(*slot):ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_item_owner_prepare(IsleItemGlobals* g,int family,IsleGameObject* eviction) {
    auto* self=isle::items::SourceAccess::impl(g);
    return self?self->prepare(family,eviction):ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_item_owner_process(IsleItemGlobals* g,IsleGameObject* object,
    IsleGameSourceCallback callback,uint8_t kind,uint32_t priority,IsleGameProcess** output) {
    if (output) *output=nullptr;
    auto* self=isle::items::SourceAccess::impl(g);
    return self?self->process(object,callback,static_cast<IsleGameProcessKind>(kind),priority,output):ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_item_owner_finish(IsleItemGlobals* g) {
    auto* self=isle::items::SourceAccess::impl(g);
    return self?self->finish():ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_item_owner_create(IsleItemGlobals* g,uint32_t id,int family,IsleGameObject** out) {
    auto* self=isle::items::SourceAccess::impl(g); return self?self->create(id,family,out):ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_item_owner_texture(IsleItemGlobals* g,IsleGameObject* o,float speed) {
    auto* slot=isle::items::SourceAccess::slot(g,o);
    return slot && slot->owner->runtime->set_texture_speed(slot->instance,speed)?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_item_owner_animation_time(IsleItemGlobals* g,IsleGameObject* o,float* out) {
    auto* slot=isle::items::SourceAccess::slot(g,o);
    if (!slot || !out) return ISLE_GAME_INVALID_ARGUMENT;
    const auto state=slot->owner->runtime->model(slot->instance);
    if (!state) return ISLE_GAME_INVALID_STATE;
    *out=state.animation_time; return ISLE_GAME_OK;
}
IsleGameResult isle_item_owner_step(IsleItemGlobals* g,IsleGameObject* o) {
    auto* slot=isle::items::SourceAccess::slot(g,o); return slot?slot->owner->step(*slot):ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_item_owner_commands(IsleItemGlobals* g,IsleGameObject* o) {
    auto* self=isle::items::SourceAccess::impl(g);
    if (!self) return ISLE_GAME_INVALID_ARGUMENT;
    // Handler leaves accumulate status rather than crossing this C++ dispatch.
    const bool processed=self->services.process_commands(o);
    return processed?g->status:ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_item_owner_send(IsleItemGlobals* g,IsleGameObject* receiver,int32_t command,IsleGameObject* sender) {
    auto* self=isle::items::SourceAccess::impl(g);
    return self && self->services.send_command(receiver,command,sender)?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_item_owner_broadcast(IsleItemGlobals* g,int32_t command,IsleGameObject* sender) {
    auto* self=isle::items::SourceAccess::impl(g);
    return self && self->services.send_command_to_link(3,command,sender)?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_item_owner_ground(IsleItemGlobals* g,float x,float z,IsleActorGround* out) {
    auto* self=isle::items::SourceAccess::impl(g);
    if (!self || !out) return ISLE_GAME_INVALID_ARGUMENT;
    try {
        isle::world::GroundResult ground; self->world.ground_at(x,z,ground);
        *out={ground.height,{ground.normal.x,ground.normal.y,ground.normal.z},static_cast<uint32_t>(ground.surface)}; return ISLE_GAME_OK;
    } catch (...) { return ISLE_GAME_INVALID_STATE; }
}
IsleGameResult isle_item_owner_ceiling(IsleItemGlobals* g,float x,float z,IsleActorGround* out,int* present) {
    auto* self=isle::items::SourceAccess::impl(g);
    if (!self || !out || !present) return ISLE_GAME_INVALID_ARGUMENT;
    try {
        isle::world::GroundResult ground;
        *present=self->world.ceiling_at(x,z,ground);
        // Local absent-map semantics preserve the result, including any prior
        // caller sample. Do not assign fabricated zero/up data on absence.
        if (*present) *out={ground.height,{ground.normal.x,ground.normal.y,ground.normal.z},static_cast<uint32_t>(ground.surface)};
        return ISLE_GAME_OK;
    } catch (...) { return ISLE_GAME_INVALID_STATE; }
}
IsleGameResult isle_item_owner_collide(IsleItemGlobals* g,IsleGameVec3* p,IsleGameVec3* v,IsleGameVec3* outp,IsleGameVec3* outv,int32_t* id) {
    auto* self=isle::items::SourceAccess::impl(g);
    if (!self || !p || !v || !outp || !outv || !id) return ISLE_GAME_INVALID_ARGUMENT;
    try {
        isle::world::Vec3 position,velocity;
        *id=self->world.collide({p->x,p->y,p->z},{v->x,v->y,v->z},position,velocity);
        if (*id>=0) { *outp={position.x,position.y,position.z}; *outv={velocity.x,velocity.y,velocity.z}; }
        return ISLE_GAME_OK;
    } catch (...) { return ISLE_GAME_INVALID_STATE; }
}
IsleGameResult isle_item_owner_effect(IsleItemGlobals* g,IsleGameObject* object) {
    auto* slot=isle::items::SourceAccess::slot(g,object);
    if (!slot) return ISLE_GAME_INVALID_ARGUMENT;
    auto& effects=slot->owner->actors.effects();
    const auto result=effects.create_effect(3,0);
    if (!result.status) return ISLE_GAME_INVALID_STATE;
    if (!result.spawned) return ISLE_GAME_OK;
    const auto& p=slot->root.position;
    return effects.set_effect_position(result.handle,{p.x,p.y,p.z})?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_item_owner_player_position(IsleItemGlobals* g,IsleGameVec3* out) {
    auto* self=isle::items::SourceAccess::impl(g);
    if (!self || !out) return ISLE_GAME_INVALID_ARGUMENT;
    const IsleGameModelPlacement* placement=nullptr;
    const auto result=isle::items::player_aux(g->game,self->player,&placement);
    if (result!=ISLE_GAME_OK) return result;
    *out=placement->transform.position; return ISLE_GAME_OK;
}
IsleGameResult isle_item_owner_target(IsleItemGlobals* g,IsleGameVec3* out) {
    auto* self=isle::items::SourceAccess::impl(g);
    if (!self || !out) return ISLE_GAME_INVALID_ARGUMENT;
    const auto result=isle::items::published_player(g->game,self->player);
    if (result!=ISLE_GAME_OK) return result;
    *out=self->player.cam_target; return ISLE_GAME_OK;
}
IsleGameResult isle_item_owner_sound(IsleItemGlobals* g,int32_t id,int32_t volume) {
    auto* self=isle::items::SourceAccess::impl(g);
    if (!self) return ISLE_GAME_INVALID_ARGUMENT;
    isle::game_audio::Parameters parameters; parameters.volume=static_cast<uint16_t>(volume);
    self->services.play_sound(static_cast<uint32_t>(id),parameters);
    return self->services.failed()?ISLE_GAME_INVALID_STATE:ISLE_GAME_OK;
}
IsleGameResult isle_item_owner_die(IsleItemGlobals* g,IsleGameObject* object) {
    auto* self=isle::items::SourceAccess::impl(g);
    return self && self->actors.notify_target_death(object)?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_item_owner_free_payload(IsleItemGlobals* g,IsleGameObject* object) {
    auto* slot=isle::items::SourceAccess::slot(g,object);
    if (!slot || !slot->payload) return ISLE_GAME_INVALID_ARGUMENT;
    slot->owner->cache_payload(*slot->payload);
    return ISLE_GAME_OK;
}
IsleGameResult isle_item_owner_beach(IsleItemGlobals* g,IsleGameObject* object,IsleActorGround* ground) {
    auto* slot=isle::items::SourceAccess::slot(g,object);
    if (!slot || !ground || !slot->owner->globals.beach_scene) return ISLE_GAME_INVALID_ARGUMENT;
    const isle::world::GroundResult value{ground->height,{ground->normal.x,ground->normal.y,ground->normal.z},static_cast<int32_t>(ground->surfaceType)};
    const auto result=slot->owner->globals.beach_scene->choose({object,slot->model},value);
    return result?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_item_owner_actor_yaw(IsleItemGlobals* g,IsleGameObject* object,float* out) {
    IsleActorSource* actor=nullptr;
    if (!g || !out || isle_actor_source_get(g->game,object,&actor)!=ISLE_GAME_OK) return ISLE_GAME_INVALID_ARGUMENT;
    IsleGameModel* model=nullptr;
    const auto result=isle_actor_spatial_object_model(g->game,object,&model);
    if (result!=ISLE_GAME_OK) return result;
    const IsleGameModelPlacement* placement=nullptr;
    if (isle_game_model_placement(g->game,model,&placement)!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE;
    *out=placement->transform.rotation.y; return ISLE_GAME_OK;
}
IsleGameResult isle_item_owner_flute(IsleItemGlobals* g,int action,int32_t* output) {
    auto* self=isle::items::SourceAccess::impl(g);
    if (!self || !output) return ISLE_GAME_INVALID_ARGUMENT;
    isle::items::Flute flute(self->services.audio,self->globals.flute);
    switch (action) {
    case 0: return flute.play()==isle::audio::Error::none?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    case 1: return flute.stop()==isle::audio::Error::none?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    case 2: *output=flute.command(); return ISLE_GAME_OK;
    case 3: *output=flute.state(); return ISLE_GAME_OK;
    default: return ISLE_GAME_INVALID_ARGUMENT;
    }
}
} // extern C

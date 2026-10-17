#include "player_cart.hpp"
#include "source_factory_resources.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <exception>
#include <limits>
#include <stdexcept>
#include <utility>

struct IslePlayerCartModelAnimation {
    const isle::player::CartClip* clip;
};
struct IslePlayerCartMaterialAnimation {
    const isle::player::CartClip* clip;
};
namespace isle::player::cart {
namespace {
OwnerStatus failure(OwnerError error,const char* detail) noexcept { return {error,detail}; }
OwnerStatus status(IsleGameResult result) noexcept {
    if (result==ISLE_GAME_OK) return {};
    return result==ISLE_GAME_EXHAUSTED ? failure(OwnerError::capacity,"prepared Cart capacity unavailable") :
        failure(OwnerError::native,"Cart C source operation failed");
}
IsleGameResult native(menu::animation::Status value) noexcept {
    if (value) return ISLE_GAME_OK;
    return value.error==menu::animation::Error::capacity || value.error==menu::animation::Error::exhausted ||
        value.error==menu::animation::Error::allocation ? ISLE_GAME_EXHAUSTED : ISLE_GAME_INVALID_STATE;
}
IsleGameModelTransform pose(const menu::animation::Transform& value) noexcept {
    return {{value.position.x,value.position.y,value.position.z},{value.rotation.x,value.rotation.y,value.rotation.z},
        {value.scale.x,value.scale.y,value.scale.z}};
}
}
struct Owner::Impl {
    struct Slot {
        Impl* owner=nullptr;
        std::size_t index=0,caller_holds=0;
        scene::Services::ObjectPreparation preparation;
        IsleSourceFactoryResources factory{};
        menu::animation::InstanceHandle instance;
        IsleGameObject* object=nullptr;
        IsleGameModel* model=nullptr;
        IslePlayerCartCallback callback=nullptr;
        void* callback_user=nullptr;
        bool runtime_prepared=false,object_prepared=false,prepared=false;
        bool active=false,retiring=false,native_retiring=false,joined=false,source_owned=false,runtime_retiring=false;
    };
    scene::Services& services;
    const CartResources& resources;
    IslePlayerSource& player;
    IslePlayerCartSource source{};
    std::vector<Slot> slots;
    std::vector<RenderBinding> bindings;
    std::vector<IsleGameModelNodeInit> scratch;
    std::vector<IslePlayerCartEvent> events;
    std::unique_ptr<menu::animation::Runtime> runtime;
    IslePlayerCartModelAnimation model_descriptors[2];
    IslePlayerCartMaterialAnimation material_descriptors[2];
    Slot* building=nullptr;
    Slot* dispatch=nullptr;
    std::size_t borrows=0,dispatch_count=0;
    std::uint64_t next_lease=0,lease=0;
    bool closing=false;
    Impl(scene::Services& s,const CartResources& r,IslePlayerSource& p,
         std::span<const menu::animation::SlotCapacity> c,menu::animation::ExecutionCapacity execution)
        : services(s),resources(r),player(p),slots(c.size()),bindings(c.size()),
          model_descriptors{{&r.intro()},{&r.outro()}},
          material_descriptors{{&r.intro()},{&r.outro()}} {
        resources.require_live();
        IslePlayerSource* canonical=nullptr;
        if (c.empty() || p.cart || p.game!=s.game || isle_player_source_context(s.game,&canonical)!=ISLE_GAME_OK || canonical!=&p)
            throw std::invalid_argument("Cart requires caller physical slots and the active canonical Player Source");
        const auto input=r.animation_resources();
        if (input.models.size()!=1 || input.models[0].nodes.size()!=43 || input.materials.size()!=8 ||
            input.models[0].render_type!=menu::RenderType::j_fogged ||
            input.models[0].matrix_kinds!=std::array<std::uint8_t,3>{28,0,0})
            throw std::invalid_argument("Cart source tree/material/render identity changed");
        const auto required=menu::animation::slot_capacity(input.models[0]);
        std::size_t maximum=0;
        for (std::size_t i=0;i<c.size();++i) {
            if (c[i].nodes<required.nodes || c[i].material_attachments<required.material_attachments)
                throw std::invalid_argument("Cart physical slot cannot hold the actual source tree");
            slots[i].owner=this; slots[i].index=i; maximum=std::max(maximum,c[i].nodes);
        }
        scratch.resize(required.nodes);
        auto made=menu::animation::Runtime::create(input,c,{},execution);
        if (!made) throw std::invalid_argument("Cart Runtime admission failed");
        runtime=std::move(made.runtime);
        std::size_t count=execution.events;
        if (count==menu::no_index) {
            std::size_t callbacks=0;
            for (const auto& animation:input.animations) if (animation.kind==menu::AnimationKind::model) {
                std::size_t n=0;
                for (const auto& command:animation.commands) {
                    if (command.opcode==16 || command.opcode==14) ++n;
                    if (command.opcode==17) n+=std::popcount(static_cast<unsigned>(command.mask));
                }
                callbacks=std::max(callbacks,n);
            }
            if (callbacks>(std::numeric_limits<std::size_t>::max()-1)/maximum)
                throw std::invalid_argument("Cart callback capacity overflow");
            count=callbacks*maximum+1;
        }
        events.resize(count);
        source={&player,this,nullptr}; player.cart=&source;
    }
    bool failed() const noexcept { return services.failed() || player.status!=ISLE_GAME_OK || isle_game_status(services.game)!=ISLE_GAME_OK; }
    bool mutable_now() const noexcept { return !failed() && !closing && !borrows && !dispatch; }
    bool context() const noexcept {
        IslePlayerSource* canonical=nullptr;
        return source.player==&player && source.owner==this && player.cart==&source && player.game==services.game &&
            isle_player_source_context(services.game,&canonical)==ISLE_GAME_OK && canonical==&player;
    }
    Slot* find(IsleGameObject* object,bool retained=false) noexcept {
        if (!object) return nullptr;
        for (auto& slot:slots) if (slot.active && slot.object==object && (retained || !slot.retiring)) return &slot;
        return nullptr;
    }
    Slot* find(menu::animation::InstanceHandle instance,bool retained=false) noexcept {
        if (!instance || instance.slot>=slots.size()) return nullptr;
        auto& slot=slots[instance.slot];
        return slot.active && slot.instance==instance && (retained || !slot.retiring)?&slot:nullptr;
    }
    bool valid(Slot& slot) const noexcept {
        return slot.active && !slot.retiring && slot.object && slot.model && slot.instance &&
            bool(runtime->model(slot.instance)) &&
            isle_game_model_validate_binding(services.game,slot.object,slot.model)==ISLE_GAME_OK;
    }
    const CartClip* descriptor(const IslePlayerCartModelAnimation* value) const noexcept {
        for (const auto& candidate:model_descriptors) if (value==&candidate) return candidate.clip;
        return nullptr;
    }
    const CartClip* descriptor(const IslePlayerCartMaterialAnimation* value) const noexcept {
        for (const auto& candidate:material_descriptors) if (value==&candidate) return candidate.clip;
        return nullptr;
    }
    IsleGameResult cancel(Slot& slot) noexcept {
        IsleGameResult first=ISLE_GAME_OK;
        const auto remember=[&](IsleGameResult value) { if (first==ISLE_GAME_OK) first=value; };
        if (slot.factory.active) remember(isle_source_factory_cancel(&slot.factory));
        if (slot.object_prepared) {
            const auto result=services.cancel_object(slot.preparation); remember(result);
            if (result==ISLE_GAME_OK) slot.object_prepared=false;
        }
        if (slot.runtime_prepared) {
            const auto result=native(runtime->cancel_prepared(slot.index)); remember(result);
            if (result==ISLE_GAME_OK) slot.runtime_prepared=false;
        }
        if (!slot.factory.active && !slot.object_prepared && !slot.runtime_prepared) {
            slot.prepared=false; slot.factory={};
            if (building==&slot) building=nullptr;
        }
        return first;
    }
    IsleGameResult terminal(IsleGameResult result) noexcept {
        services.fail("Cart source operation failed after admission");
        if (player.status==ISLE_GAME_OK) player.status=result;
        for (auto& slot:slots) if (cancel(slot)!=ISLE_GAME_OK) services.fail("Cart unpublished cancellation remains held");
        return result;
    }
    IsleGameResult prepare(Slot& slot) noexcept {
        auto result=native(runtime->prepare_acquire(slot.index,0,false));
        if (result==ISLE_GAME_OK) {
            slot.runtime_prepared=true; result=services.prepare_object(slot.preparation);
        }
        if (result==ISLE_GAME_OK) {
            slot.object_prepared=true;
            const IsleSourceFactoryProcess process{nullptr,isle_player_cart_source_update,ISLE_GAME_SYNCHRONOUS,1};
            result=isle_source_factory_prepare(services.game,43,&process,1,&slot.factory);
        }
        if (result==ISLE_GAME_OK) slot.prepared=true;
        else if (cancel(slot)!=ISLE_GAME_OK) return terminal(ISLE_GAME_INVALID_STATE);
        return result;
    }
    static void on_retiring(IsleGameObject*,void* opaque) noexcept {
        auto& slot=*static_cast<Slot*>(opaque); auto& self=*slot.owner;
        slot.retiring=true; slot.native_retiring=true;
        if (slot.instance && self.dispatch!=&slot && !self.borrows && !slot.runtime_retiring) {
            if (self.runtime->begin_retire(slot.instance)) slot.runtime_retiring=true;
            else self.services.fail("Cart Runtime retirement failed");
        }
    }
    static void on_joined(IsleGameObject*,void* opaque) noexcept {
        auto& slot=*static_cast<Slot*>(opaque); slot.joined=true; slot.model=nullptr;
    }
    IsleGameResult flush_retirements() noexcept {
        if (dispatch || borrows) return ISLE_GAME_OK;
        for (auto& slot:slots) if (slot.active && slot.retiring) {
            if (slot.instance && !slot.runtime_retiring) {
                if (!runtime->begin_retire(slot.instance)) services.fail("Cart deferred Runtime retirement failed");
                else slot.runtime_retiring=true;
            }
            if (slot.object && !slot.native_retiring) services.erase(slot.object);
        }
        if (closing) source.cartGObj=nullptr;
        return services.failed()?ISLE_GAME_INVALID_STATE:ISLE_GAME_OK;
    }
    IsleGameResult create(IsleGameObject** output) noexcept {
        if (!output) return ISLE_GAME_INVALID_ARGUMENT;
        *output=nullptr;
        if (!mutable_now() || !context() || building) return ISLE_GAME_INVALID_STATE;
        Slot* selected=nullptr;
        for (auto& slot:slots) if (!slot.active && slot.prepared) { selected=&slot; break; }
        if (!selected) return ISLE_GAME_EXHAUSTED;
        auto& slot=*selected; building=&slot;
        const auto result=services.publish_object(slot.preparation,slot.factory,35,9,0x80000000u,&slot.object);
        if (slot.object || result==ISLE_GAME_OK) { slot.object_prepared=false; slot.active=true; }
        if (result!=ISLE_GAME_OK) return terminal(result==ISLE_GAME_EXHAUSTED?ISLE_GAME_INVALID_STATE:result);
        if (!services.set_lifecycle(slot.object,{on_retiring,on_joined,&slot}) ||
            !services.source_owner_register(slot.object,{})) return terminal(ISLE_GAME_INVALID_STATE);
        slot.source_owned=true; *output=slot.object; return ISLE_GAME_OK;
    }
    IsleGameResult attach(Slot& slot) noexcept {
        if (building!=&slot || !slot.runtime_prepared || slot.model) return terminal(ISLE_GAME_INVALID_STATE);
        const auto acquired=runtime->acquire_prepared(slot.index);
        if (!acquired) return terminal(native(acquired.status));
        slot.runtime_prepared=false; slot.instance=acquired.handle;
        // Native construction defaults, not extra source calls or Title policy.
        if (!runtime->set_model_speed(slot.instance,1) || !runtime->set_texture_speed(slot.instance,1))
            return terminal(ISLE_GAME_INVALID_STATE);
        if (!menu::model::attach_source_model(services.game,slot.object,resources.animation_resources().models[0],
                runtime->model(slot.instance),menu::model::RootRecipe::source,scratch,&slot.model,&slot.factory))
            return terminal(ISLE_GAME_INVALID_STATE);
        return ISLE_GAME_OK;
    }
    IsleGameResult preflight(Slot& slot,const IslePlayerCartModelAnimation* model,
        const IslePlayerCartMaterialAnimation* material,float start,bool change_speed,float speed) noexcept {
        if (!mutable_now() || !valid(slot)) return ISLE_GAME_INVALID_STATE;
        // Outer NULL/NULL performs no Runtime operation, including no numeric
        // validation invented ahead of the original C guards.
        if (!model && !material) return ISLE_GAME_OK;
        if (!std::isfinite(start) || (change_speed && !std::isfinite(speed))) return ISLE_GAME_INVALID_ARGUMENT;
        const auto state=runtime->model(slot.instance);
        const auto* m=model?descriptor(model):nullptr;
        const auto* t=material?descriptor(material):nullptr;
        if ((model && (!m || m->model_bindings.size()!=state.nodes.size())) ||
            (material && (!t || t->material_bindings.size()!=state.materials.size()))) return ISLE_GAME_INVALID_ARGUMENT;
        return ISLE_GAME_OK;
    }
    IsleGameResult step(Slot& slot,IslePlayerCartEventBatch* batch) noexcept {
        if (!batch) return ISLE_GAME_INVALID_ARGUMENT;
        *batch={};
        if (!mutable_now() || !context() || !valid(slot) || next_lease==UINT64_MAX) return ISLE_GAME_INVALID_STATE;
        IsleGameObjectMetadata metadata{};
        auto result=isle_game_object_metadata(services.game,slot.object,&metadata);
        if (result!=ISLE_GAME_OK) return result;
        if (!runtime->set_object_flags(slot.instance,metadata.flags) || !runtime->step_model(slot.instance))
            return terminal(ISLE_GAME_INVALID_STATE);
        const auto state=runtime->model(slot.instance);
        if (!state || state.nodes.size()!=43) return terminal(ISLE_GAME_INVALID_STATE);
        for (std::size_t i=0;i<state.nodes.size();++i) {
            const auto transform=pose(state.nodes[i].transform);
            result=isle_game_model_validate_transform(services.game,slot.model,i,&transform);
            if (result!=ISLE_GAME_OK) return terminal(result);
        }
        const auto pending=runtime->events();
        if (pending.size()>events.size()) return terminal(ISLE_GAME_INVALID_STATE);
        IsleGameModelRef reference{};
        result=isle_game_model_reference(services.game,slot.object,slot.model,&reference);
        if (result!=ISLE_GAME_OK) return terminal(result);
        for (std::size_t i=0;i<pending.size();++i) {
            const auto& event=pending[i];
            if (event.instance!=slot.instance || event.node>=state.nodes.size()) return terminal(ISLE_GAME_INVALID_STATE);
            events[i]={{reference,event.node},event.event_id,event.argument};
        }
        for (std::size_t i=0;i<state.nodes.size();++i) {
            const auto transform=pose(state.nodes[i].transform);
            result=isle_game_model_set_transform(services.game,slot.model,i,&transform);
            if (result==ISLE_GAME_OK) result=isle_game_model_set_flags(services.game,slot.model,i,state.nodes[i].flags);
            if (result!=ISLE_GAME_OK) return terminal(result);
        }
        dispatch=&slot; dispatch_count=pending.size(); lease=++next_lease;
        *batch={events.data(),pending.size(),slot.callback,slot.callback_user,lease};
        return ISLE_GAME_OK;
    }
    IsleGameResult finish_events(Slot& slot,const IslePlayerCartEventBatch* batch) noexcept {
        if (!batch || dispatch!=&slot || !lease || batch->lease!=lease || batch->events!=events.data() ||
            batch->count!=dispatch_count || batch->callback!=slot.callback || batch->user!=slot.callback_user)
            return ISLE_GAME_INVALID_STATE;
        dispatch=nullptr; dispatch_count=0; lease=0;
        return flush_retirements();
    }
};
struct SourceAccess {
    static Owner::Impl* impl(IslePlayerCartSource* source,bool cleanup=false) noexcept {
        if (!source || !source->owner) return nullptr;
        auto* self=static_cast<Owner::Impl*>(source->owner);
        return &self->source==source && self->source.player==&self->player && self->player.cart==source &&
            (cleanup || self->context())?self:nullptr;
    }
    static Owner::Impl::Slot* slot(IslePlayerCartSource* source,IsleGameObject* object,bool cleanup=false) noexcept {
        auto* self=impl(source,cleanup); return self?self->find(object,cleanup):nullptr;
    }
};
Owner::Owner(scene::Services& services,const CartResources& resources,IslePlayerSource& player,
    std::span<const menu::animation::SlotCapacity> capacities,menu::animation::ExecutionCapacity execution)
    : impl_(std::make_unique<Impl>(services,resources,player,capacities,execution)) {
    const auto prepared=refill_prepared();
    if (!prepared) {
        bool retained=false;
        for (auto& slot:impl_->slots) retained|=impl_->cancel(slot)!=ISLE_GAME_OK;
        if (retained) std::terminate();
        impl_->player.cart=nullptr;
        throw std::invalid_argument(prepared.detail);
    }
}
Owner::~Owner() {
    retire(); collect_joined();
    if (impl_->dispatch || impl_->borrows || impl_->building ||
        std::any_of(impl_->slots.begin(),impl_->slots.end(),[](const auto& slot) {
            return slot.active || slot.prepared || slot.factory.active || slot.object_prepared || slot.runtime_prepared;
        })) std::terminate();
    impl_->player.cart=nullptr; impl_->source.owner=nullptr; impl_->source.player=nullptr;
}
IslePlayerCartSource* Owner::source() noexcept { return &impl_->source; }
const IslePlayerCartSource* Owner::source() const noexcept { return &impl_->source; }
OwnerStatus Owner::refill_prepared() noexcept {
    auto& self=*impl_;
    if (!self.mutable_now() || !self.context() || self.building) return failure(OwnerError::lifetime,"Cart refill requires exclusive live outside-OM ownership");
    for (auto& slot:self.slots) if (!slot.active && !slot.prepared) {
        const auto result=self.prepare(slot);
        if (result!=ISLE_GAME_OK) return status(result);
    }
    return {};
}
InitResult Owner::init() noexcept {
    auto& self=*impl_; IsleGameObject* object=nullptr;
    const auto result=isle_player_cart_source_call_init(&self.source,&object);
    if (result!=ISLE_GAME_OK) { self.terminal(result); return {status(result),nullptr,{}}; }
    if (!object) return {failure(OwnerError::capacity,"prepared Cart capacity unavailable"),nullptr,{}};
    auto* slot=self.find(object);
    return slot?InitResult{{},object,slot->instance}:InitResult{failure(OwnerError::native,"Cart source init did not publish its native instance"),nullptr,{}};
}
OwnerStatus Owner::set_animation(const IslePlayerCartModelAnimation* m,const IslePlayerCartMaterialAnimation* t,float start,float speed) noexcept {
    const auto result=isle_player_cart_source_call_set(&impl_->source,m,t,start,speed);
    if (result!=ISLE_GAME_OK) impl_->terminal(result);
    return status(result);
}
OwnerStatus Owner::change_animation(const IslePlayerCartModelAnimation* m,const IslePlayerCartMaterialAnimation* t,float start) noexcept {
    const auto result=isle_player_cart_source_call_change(&impl_->source,m,t,start);
    if (result!=ISLE_GAME_OK) impl_->terminal(result);
    return status(result);
}
OwnerStatus Owner::set_animation_speed(float speed) noexcept {
    const auto result=isle_player_cart_source_call_speed(&impl_->source,speed);
    if (result!=ISLE_GAME_OK) impl_->terminal(result);
    return status(result);
}
OwnerStatus Owner::set_callback(IsleGameObject* object,IslePlayerCartCallback callback,void* user) noexcept {
    const auto result=isle_player_cart_source_call_callback(&impl_->source,object,callback,user);
    if (result!=ISLE_GAME_OK) impl_->terminal(result);
    return status(result);
}
const IslePlayerCartModelAnimation* Owner::intro_model() const noexcept { return &impl_->model_descriptors[0]; }
const IslePlayerCartMaterialAnimation* Owner::intro_material() const noexcept { return &impl_->material_descriptors[0]; }
const IslePlayerCartModelAnimation* Owner::outro_model() const noexcept { return &impl_->model_descriptors[1]; }
const IslePlayerCartMaterialAnimation* Owner::outro_material() const noexcept { return &impl_->material_descriptors[1]; }
IsleGameObject* Owner::object(menu::animation::InstanceHandle h) const noexcept { const auto* slot=impl_->find(h); return slot?slot->object:nullptr; }
IsleGameModel* Owner::native_model(menu::animation::InstanceHandle h) const noexcept { const auto* slot=impl_->find(h); return slot?slot->model:nullptr; }
menu::animation::InstanceHandle Owner::instance(IsleGameObject* object) const noexcept { const auto* slot=impl_->find(object); return slot?slot->instance:menu::animation::InstanceHandle{}; }
Membership Owner::membership(menu::animation::InstanceHandle h) const noexcept {
    const auto* slot=impl_->find(h,true);
    return {slot && !slot->retiring,slot && slot->retiring,slot && slot->joined,slot && slot->prepared,slot?slot->caller_holds:0};
}
const menu::animation::Runtime& Owner::animation() const noexcept { return *impl_->runtime; }
std::span<const RenderBinding> Owner::render_bindings(std::span<scene::RenderOrderEntry> order) noexcept {
    auto& self=*impl_; std::size_t count=0,ordered=0;
    if (self.borrows || self.dispatch || self.failed()) return {};
    // Services' actual draw list may contain unrelated DL3 objects. Obtain its
    // length without inventing an owner-only sort or a second render registry.
    for (auto& slot:self.slots) if (slot.active && !slot.retiring) ++count;
    if (!count) { std::fill(self.bindings.begin(),self.bindings.end(),RenderBinding{}); return self.bindings; }
    if (!self.services.render_order(3,order,ordered)) {
        self.services.fail("Cart render order exceeds prepared native order scratch"); return {};
    }
    count=0;
    for (const auto& entry:order.first(ordered)) if (auto* slot=self.find(entry.object)) {
        self.bindings[count++]={{slot->index,self.runtime->model(slot->instance),slot->object,slot->model,
            menu::model::PoseSource::native,menu::model::RootRecipe::source,true,self.runtime.get()},entry.render};
    }
    for (std::size_t i=count;i<self.bindings.size();++i) self.bindings[i]={};
    return self.bindings;
}
Owner::Borrow::Borrow(Owner* owner) noexcept : owner_(owner) { ++owner_->impl_->borrows; }
Owner::Borrow::~Borrow() { if (owner_) owner_->end_borrow(); }
Owner::Borrow::Borrow(Borrow&& other) noexcept : owner_(std::exchange(other.owner_,nullptr)) {}
Owner::Borrow& Owner::Borrow::operator=(Borrow&& other) noexcept {
    if (this!=&other) { if (owner_) owner_->end_borrow(); owner_=std::exchange(other.owner_,nullptr); } return *this;
}
Owner::Borrow Owner::borrow() noexcept {
    auto& self=*impl_;
    if (self.failed() || self.closing || self.dispatch || self.borrows==SIZE_MAX) return {};
    return Borrow(this);
}
void Owner::end_borrow() noexcept { if (!impl_->borrows) std::terminate(); --impl_->borrows; impl_->flush_retirements(); }
Owner::Reference::Reference(Owner* owner,IsleGameObject* object) noexcept : owner_(owner),object_(object) {}
Owner::Reference::~Reference() { if (owner_) owner_->release_reference(object_); }
Owner::Reference::Reference(Reference&& other) noexcept : owner_(std::exchange(other.owner_,nullptr)),object_(std::exchange(other.object_,nullptr)) {}
Owner::Reference& Owner::Reference::operator=(Reference&& other) noexcept {
    if (this!=&other) {
        if (owner_) owner_->release_reference(object_);
        owner_=std::exchange(other.owner_,nullptr); object_=std::exchange(other.object_,nullptr);
    } return *this;
}
Owner::Reference Owner::retain(IsleGameObject* object) noexcept {
    auto* slot=impl_->find(object,true);
    if (!slot || slot->caller_holds==SIZE_MAX) return {};
    ++slot->caller_holds; return Reference(this,object);
}
void Owner::release_reference(IsleGameObject* object) noexcept {
    auto* slot=impl_->find(object,true); if (!slot || !slot->caller_holds) std::terminate(); --slot->caller_holds;
}
OwnerStatus Owner::retire(IsleGameObject* object) noexcept {
    auto* slot=impl_->find(object,true); if (!slot) return failure(OwnerError::admission,"Cart retire requires its retained object identity");
    slot->retiring=true; impl_->flush_retirements();
    return impl_->services.failed()?failure(OwnerError::services,"Cart retirement retains terminal Services failure"):OwnerStatus{};
}
OwnerStatus Owner::retire() noexcept {
    auto& self=*impl_; self.closing=true; IsleGameResult first=ISLE_GAME_OK;
    for (auto& slot:self.slots) {
        const auto result=self.cancel(slot); if (first==ISLE_GAME_OK) first=result;
        if (slot.active) slot.retiring=true;
    }
    const auto retired=self.flush_retirements(); if (first==ISLE_GAME_OK) first=retired;
    return status(first);
}
OwnerStatus Owner::collect_joined() noexcept {
    auto& self=*impl_; bool held=self.dispatch || self.borrows || self.building;
    for (auto& slot:self.slots) if (slot.active && slot.retiring) {
        if (!slot.joined || slot.caller_holds || self.source.cartGObj==slot.object || self.dispatch==&slot || self.borrows ||
            (slot.source_owned && !self.services.source_owner_collectable(slot.object))) { held=true; continue; }
        if (slot.source_owned) {
            if (!self.services.source_owner_release(slot.object)) { held=true; continue; }
            slot.source_owned=false;
        }
        if (slot.instance && (!slot.runtime_retiring || !self.runtime->release_retired(slot.instance))) { held=true; continue; }
        slot.instance={}; slot.object=nullptr; slot.model=nullptr; slot.callback=nullptr; slot.callback_user=nullptr;
        slot.active=false; slot.retiring=false; slot.native_retiring=false; slot.joined=false; slot.runtime_retiring=false;
    }
    return held?failure(OwnerError::lifetime,"Cart current/caller/native/source/event/graphics ownership remains"):OwnerStatus{};
}
} // namespace isle::player::cart

extern "C" {
IsleGameResult isle_player_cart_backend_try_object(IslePlayerCartSource* s,IsleGameObject** out) {
    if (out) *out=nullptr;
    auto* self=isle::player::cart::SourceAccess::impl(s);
    return self?self->create(out):ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_player_cart_backend_link_render(IslePlayerCartSource* s,IsleGameObject* o) {
    auto* slot=isle::player::cart::SourceAccess::slot(s,o);
    return slot && slot->owner->building==slot && slot->owner->services.set_render(o,{3,0x80000000u,UINT32_MAX})?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_player_cart_backend_attach_model(IslePlayerCartSource* s,IsleGameObject* o) {
    auto* slot=isle::player::cart::SourceAccess::slot(s,o); return slot?slot->owner->attach(*slot):ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_player_cart_backend_publish_animation_process(IslePlayerCartSource* s,IsleGameObject* o,
    IsleGameSourceCallback callback,int32_t kind,int32_t priority) {
    auto* slot=isle::player::cart::SourceAccess::slot(s,o);
    if (!slot || slot->owner->building!=slot || callback!=isle_player_cart_source_update || kind!=1 || priority!=1)
        return ISLE_GAME_INVALID_STATE;
    IsleGameProcess* process=nullptr;
    return isle_source_factory_publish_process(&slot->factory,o,callback,ISLE_GAME_SYNCHRONOUS,1,&process);
}
IsleGameResult isle_player_cart_backend_finish_init(IslePlayerCartSource* s,IsleGameObject* o) {
    auto* slot=isle::player::cart::SourceAccess::slot(s,o);
    if (!slot || slot->owner->building!=slot || !slot->model || !slot->instance || slot->runtime_prepared || slot->object_prepared)
        return ISLE_GAME_INVALID_STATE;
    const auto result=isle_source_factory_finish(&slot->factory);
    if (result!=ISLE_GAME_OK) return slot->owner->terminal(result);
    slot->prepared=false; slot->owner->building=nullptr; return ISLE_GAME_OK;
}
IsleGameResult isle_player_cart_backend_preflight_requested_animations(IslePlayerCartSource* s,IsleGameObject* o,
    const IslePlayerCartModelAnimation* m,const IslePlayerCartMaterialAnimation* t,float start,int32_t change,float speed) {
    auto* slot=isle::player::cart::SourceAccess::slot(s,o);
    return slot?slot->owner->preflight(*slot,m,t,start,change!=0,speed):ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_player_cart_backend_preflight_speed(IslePlayerCartSource* s,IsleGameObject* o,float speed) {
    auto* slot=isle::player::cart::SourceAccess::slot(s,o);
    return slot && slot->owner->mutable_now() && slot->owner->valid(*slot) && std::isfinite(speed)?ISLE_GAME_OK:ISLE_GAME_INVALID_ARGUMENT;
}
IsleGameResult isle_player_cart_backend_bind_model(IslePlayerCartSource* s,IsleGameObject* o,const IslePlayerCartModelAnimation* m,float start) {
    auto* slot=isle::player::cart::SourceAccess::slot(s,o); const auto* clip=slot?slot->owner->descriptor(m):nullptr;
    return slot && clip && slot->owner->mutable_now() && slot->owner->valid(*slot)?
        isle::player::cart::native(slot->owner->runtime->bind_model(slot->instance,clip->model_bindings,start)):ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_player_cart_backend_bind_material(IslePlayerCartSource* s,IsleGameObject* o,const IslePlayerCartMaterialAnimation* t,float start) {
    auto* slot=isle::player::cart::SourceAccess::slot(s,o); const auto* clip=slot?slot->owner->descriptor(t):nullptr;
    return slot && clip && slot->owner->mutable_now() && slot->owner->valid(*slot)?
        isle::player::cart::native(slot->owner->runtime->bind_materials(slot->instance,clip->material_bindings,start)):ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_player_cart_backend_set_model_speed(IslePlayerCartSource* s,IsleGameObject* o,float speed) {
    auto* slot=isle::player::cart::SourceAccess::slot(s,o);
    return slot && slot->owner->mutable_now() && slot->owner->valid(*slot)?
        isle::player::cart::native(slot->owner->runtime->set_model_speed(slot->instance,speed)):ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_player_cart_backend_set_material_speed(IslePlayerCartSource* s,IsleGameObject* o,float speed) {
    auto* slot=isle::player::cart::SourceAccess::slot(s,o);
    return slot && slot->owner->mutable_now() && slot->owner->valid(*slot)?
        isle::player::cart::native(slot->owner->runtime->set_texture_speed(slot->instance,speed)):ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_player_cart_backend_set_callback(IslePlayerCartSource* s,IsleGameObject* o,IslePlayerCartCallback callback,void* user) {
    auto* slot=isle::player::cart::SourceAccess::slot(s,o);
    if (!slot || !slot->owner->mutable_now() || !slot->owner->valid(*slot)) return ISLE_GAME_INVALID_STATE;
    const auto result=slot->owner->runtime->set_callback_enabled(slot->instance,callback!=nullptr);
    if (!result) return isle::player::cart::native(result);
    slot->callback=callback; slot->callback_user=user; return ISLE_GAME_OK;
}
IsleGameResult isle_player_cart_backend_step(IslePlayerCartSource* s,IsleGameObject* o,IslePlayerCartEventBatch* out) {
    if (out) *out={};
    auto* slot=isle::player::cart::SourceAccess::slot(s,o);
    return slot?slot->owner->step(*slot,out):ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_player_cart_backend_finish_events(IslePlayerCartSource* s,IsleGameObject* o,const IslePlayerCartEventBatch* batch) {
    auto* slot=isle::player::cart::SourceAccess::slot(s,o,true);
    return slot?slot->owner->finish_events(*slot,batch):ISLE_GAME_INVALID_STATE;
}
}

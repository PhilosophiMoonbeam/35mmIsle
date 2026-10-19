#include "course_actor_egg_native.hpp"
#include "actor_native_context.hpp"
#include "actor_owner.hpp"
#include "actor_owner_source.h"
#include "actor_resource_catalog.hpp"
#include "course_native_host.hpp"
#include "model_data_decoder.hpp"
#include <algorithm>
#include <bit>
#include <limits>
namespace isle::course {
namespace {
constexpr std::array<std::uint8_t,20> admitted_sha1{
    0xed,0xc7,0xc4,0x9c,0xc5,0x68,0xc0,0x45,0xfe,0x48,
    0xbe,0x0d,0x18,0x01,0x1c,0x30,0xf3,0x93,0xcb,0xaf};
void decode(const rom::GuestView& wire, std::span<IsleEggVertex> vertices) {
    for (std::size_t i=0;i<vertices.size();++i) {
        auto& v=vertices[i]; const auto at=i*16;
        v.unk_00=std::bit_cast<std::int16_t>(wire.u16(at));
        v.unk_02=std::bit_cast<std::int16_t>(wire.u16(at+2));
        v.unk_04=std::bit_cast<std::int16_t>(wire.u16(at+4));
        for (std::size_t j=0;j<6;++j) v.unk_06[j]=wire.u8(at+6+j);
        v.unk_0C=std::bit_cast<std::int8_t>(wire.u8(at+12));
        v.unk_0D=std::bit_cast<std::int8_t>(wire.u8(at+13));
        v.unk_0E=std::bit_cast<std::int8_t>(wire.u8(at+14)); v.unk_0F=wire.u8(at+15);
    }
}
}
IsleGameResult EggGeometry::prepare(const rom::Snapshot& snapshot, IsleTunnelActorSource& source, void* owner) noexcept {
    return prepare(snapshot,&source,owner,1,source.D_802EFF14_5ECFE4,
        source.egg_frame0_803431f0,336);
}
IsleGameResult EggGeometry::prepare(const rom::Snapshot& snapshot, IsleCaveActorSource& source, void* owner) noexcept {
    static_assert(offsetof(IsleCaveEggFrameStorage,frame0)==0xc0);
    return prepare(snapshot,&source,owner,4,source.D_802C7C28_64A0D8,
        source.D_8033E150_6C0600.frame0,324);
}
IsleGameResult EggGeometry::prepare(const rom::Snapshot& snapshot, void* source, void* owner,
    std::int32_t course, IsleEgg& egg, IsleEggVertex* frame0, std::size_t count) noexcept {
    if (prepared_ || egg_ || !source || !owner || !frame0 || (course!=1 && course!=4)) return ISLE_GAME_INVALID_STATE;
    try {
        if (snapshot.canonical().size()!=rom::us_size || snapshot.identity().size!=rom::us_size ||
            snapshot.identity().sha1!=admitted_sha1) return ISLE_GAME_INVALID_ARGUMENT;
        const auto& catalog=actor::detail::course_catalog(course);
        const auto id=course==1 ? 602u : 601u;
        const actor::detail::InitializationSpec* initialization=nullptr;
        for (const auto& item:catalog.initializations) if (item.actor_id==id) {
            if (initialization) return ISLE_GAME_INVALID_STATE;
            initialization=&item;
        }
        if (!initialization) return ISLE_GAME_INVALID_STATE;
        std::uint32_t factory=0;
        for (const auto& definition:catalog.definitions) if (definition.actor_id==id) {
            if (definition.initialization>=catalog.initializations.size() ||
                &catalog.initializations[definition.initialization]!=initialization ||
                (factory && factory!=definition.factory_identity)) return ISLE_GAME_INVALID_STATE;
            factory=definition.factory_identity;
        }
        if (!factory) return ISLE_GAME_INVALID_STATE;
        const auto data=course==1 ? 0x80188d28u : 0x8017a780u;
        const auto frame1=course==1 ? 0x8018a6f0u : 0x8017c090u;
        const model_assets::Reader reader(snapshot,catalog.mappings);
        const auto original=reader.view(data,count*16,8);
        const auto endpoint=reader.view(frame1,count*16,8);
        decode(original,std::span(destination_).first(count));
        decode(endpoint,std::span(endpoint_).first(count));
        // Source prepare owns the once-only memset of the actual frame0 BSS.
        frames_={frame0,endpoint_.data()};
        preparation_={source,owner,reinterpret_cast<const std::uint8_t*>(snapshot.canonical().data()),
            snapshot.canonical().size(),course,data,course==1 ? 0x802eff0cu : 0x802c7c20u,
            course==1 ? 0x803431f0u : 0x8033e210u,frame1,static_cast<std::int32_t>(count),
            destination_.data(),frames_.data(),this,static_cast<IsleGameResult(*)(IsleEgg*,IsleActorSource*)>(&EggGeometry::bind)};
        egg_=&egg; snapshot_=&snapshot; initialization_=initialization->identity; factory_=factory; actor_id_=id;
        publication_[0]={data,std::span<const IsleEggVertex>(destination_).first(count),&egg.revision};
        prepared_=true;
        return ISLE_GAME_OK;
    } catch (...) { return ISLE_GAME_INVALID_STATE; }
}
bool EggGeometry::source_matches() const noexcept {
    if (!prepared_ || !egg_ || !snapshot_ || snapshot_->canonical().data()!=reinterpret_cast<const std::byte*>(preparation_.rom) ||
        egg_->data!=destination_.data() || egg_->temp_buffer!=frames_.data() ||
        frames_[1]!=endpoint_.data() || egg_->native_geometry!=this ||
        egg_->bind!=static_cast<IsleGameResult(*)(IsleEgg*,IsleActorSource*)>(&EggGeometry::bind) ||
        egg_->data_identity!=preparation_.data_identity || egg_->temp_buffer_identity!=preparation_.temp_buffer_identity ||
        egg_->vertex_count!=preparation_.vertex_count || egg_->end!=2 || !egg_->revision ||
        !egg_->course || egg_->course->owner!=preparation_.owner || egg_->course->course_id!=preparation_.course_id) return false;
    if (preparation_.course_id==1) {
        const auto* source=static_cast<const IsleTunnelActorSource*>(preparation_.source);
        return &source->base==egg_->course && &source->D_802EFF14_5ECFE4==egg_ && frames_[0]==source->egg_frame0_803431f0;
    }
    const auto* source=static_cast<const IsleCaveActorSource*>(preparation_.source);
    return &source->base==egg_->course && &source->D_802C7C28_64A0D8==egg_ && frames_[0]==source->D_8033E150_6C0600.frame0;
}
IsleGameResult EggGeometry::attach(NativeHost& host) noexcept {
    if (host_ || !source_matches() || !host.closure_prepared() || !host.actors() ||
        host.actor_resources().course_id()!=preparation_.course_id) return ISLE_GAME_INVALID_STATE;
    try {
        const auto& initialization=host.actor_resources().initialization(initialization_);
        if (initialization.actor_id!=actor_id_ ||
            host.actors()->admitted_factory(actor_id_,factory_)!=&initialization) return ISLE_GAME_INVALID_STATE;
        // The publisher independently proves exact vertex-load union/provenance
        // and the complete reservation union inside the real PreparationLease.
        host_=&host; return ISLE_GAME_OK;
    } catch (...) { return ISLE_GAME_INVALID_STATE; }
}
IsleGameResult EggGeometry::bind(IsleEgg* egg, IsleActorSource* actor) noexcept {
    if (!egg || !actor || !egg->native_geometry) return ISLE_GAME_INVALID_ARGUMENT;
    auto& self=*static_cast<EggGeometry*>(egg->native_geometry);
    if (self.egg_!=egg) return ISLE_GAME_INVALID_STATE;
    return self.bind(*actor);
}
IsleGameResult EggGeometry::bind(IsleActorSource& source) noexcept {
    if (!source_matches() || !host_ || source.game!=host_->game() || source.eggGeo!=egg_ || !source.object) return ISLE_GAME_INVALID_STATE;
    try {
        actor::Owner* owner=nullptr;
        IsleGameResult status;
        if (preparation_.course_id==1) {
            auto* course=static_cast<IsleTunnelActorSource*>(const_cast<void*>(preparation_.source));
            status=actor::native_owner(course,&owner);
            if (status!=ISLE_GAME_OK) return status;
            if (owner->course_context().tunnel().native_egg_geometry!=this) return ISLE_GAME_INVALID_STATE;
        } else {
            auto* course=static_cast<IsleCaveActorSource*>(const_cast<void*>(preparation_.source));
            status=actor::native_owner(course,&owner);
            if (status!=ISLE_GAME_OK) return status;
            if (owner->course_context().cave().native_egg_geometry!=this) return ISLE_GAME_INVALID_STATE;
        }
        if (owner!=host_->actors()) return ISLE_GAME_INVALID_STATE;
        const auto& initialization=host_->actor_resources().initialization(initialization_);
        if (source.initData!=&initialization || source.id!=static_cast<std::int32_t>(actor_id_) ||
            owner->admitted_factory(actor_id_,factory_)!=&initialization) return ISLE_GAME_INVALID_STATE;
        IsleGameModelRef model{};
        status=isle_actor_owner_self_model(&source,&model);
        if (status!=ISLE_GAME_OK) return status;
        if (model.object!=source.object || !model.model || !model.incarnation) return ISLE_GAME_INVALID_STATE;
        IsleGameModel* resolved=nullptr;
        status=isle_game_model_resolve(source.game,&model,&resolved);
        if (status!=ISLE_GAME_OK) return status;
        for (const auto& binding:owner->render_bindings()) {
            if (binding.model.object!=source.object) continue;
            if (binding.model.model!=resolved || !binding.model.state ||
                binding.model.state.resource_model!=initialization.model ||
                owner->source(binding.model.state.instance)!=&source ||
                owner->object(binding.model.state.instance)!=source.object ||
                owner->native_model(binding.model.state.instance)!=resolved) return ISLE_GAME_INVALID_STATE;
            return ISLE_GAME_OK;
        }
        return ISLE_GAME_INVALID_STATE;
    } catch (...) { return ISLE_GAME_INVALID_STATE; }
}
}

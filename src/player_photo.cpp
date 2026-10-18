#include "player_photo.hpp"
#include "actor_owner_source.h"
#include "menu_scene_bridge.h"
#include "rom_view.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <exception>
#include <stdexcept>

namespace isle::player {
namespace {
std::uint32_t word(std::span<const std::byte> bytes,std::size_t offset) {
    return rom::GuestView{0,0,bytes}.u32(offset);
}
std::size_t table_offset(std::uint32_t address) {
    // Own splat app_render start46270/vram8009A8C0, initialized data57A90.
    if(address<0x800ac0e0u || address>=0x800ae5f0u)
        throw std::invalid_argument("photo table outside own initialized app_render data");
    return std::size_t(address-0x8009a8c0u)+0x46270;
}
IsleGameVec3 vec(menu::Vec3 v) { return {v.x,v.y,v.z}; }
bool fits(float value,float low,float high) { return std::isfinite(value)&&value>=low&&value<high; }
}
PhotoResources::PhotoResources(const rom::Snapshot& snapshot):snapshot_(&snapshot) {
    auto bytes=snapshot.canonical();
    if(bytes.size()!=rom::us_size) throw std::invalid_argument("photo requires admitted retained ROM");
    for(std::size_t i=0;i<report_indices_.size();++i)
        report_indices_[i]=std::bit_cast<int8_t>(std::to_integer<uint8_t>(bytes[table_offset(0x800ae4e4u)+i]));
    // Own47380 rodata range5ADD0..5BB30, not SDK string/layout material.
    constexpr std::size_t strings_end=0x5bb30;
    for(std::size_t i=0;i<names_.size();++i) {
        const auto address=word(bytes,table_offset(0x800ae284u)+i*4);
        if(address<0x800af420u || address>=0x800b0180u)
            throw std::invalid_argument("photo name outside own47380 rodata");
        const auto offset=std::size_t(address-0x8009a8c0u)+0x46270;
        auto end=offset;
        while(end<strings_end && bytes[end]!=std::byte{0}) ++end;
        if(end==strings_end) throw std::invalid_argument("unterminated source Pokemon name");
        names_[i]=reinterpret_cast<const char*>(bytes.data()+offset);
    }
    for(std::size_t i=0;i<species_.size();++i) {
        auto address=i<67?0x800ad4b8u+std::uint32_t(i*12):0x800ad9a4u+std::uint32_t((i-67)*12);
        auto offset=table_offset(address);
        auto& species=species_[i];
        species.id=static_cast<int>(word(bytes,offset));
        auto entries=word(bytes,offset+4), count=word(bytes,offset+8);
        if(count>64) throw std::invalid_argument("photo animation index exceeds source six bits");
        species.first=animations_.size(); species.count=count;
        for(std::uint32_t j=0;j<count;++j) {
            auto entry=table_offset(entries+j*12);
            auto pose=std::bit_cast<float>(word(bytes,entry));
            if(!std::isfinite(pose)) throw std::invalid_argument("photo pose is not finite");
            animations_.push_back({word(bytes,entry+4),pose});
        }
    }
}
IsleGameResult PhotoResources::bind(IslePlayerPhotoSource& source,save::GameSession& session) const noexcept {
    if(source.owner || !session || snapshot_->canonical().empty()) return ISLE_GAME_INVALID_STATE;
    source.D_800AE4E4=report_indices_.data(); source.D_800AE284=names_.data();
    source.save=&session; source.status=ISLE_GAME_OK;
    return ISLE_GAME_OK;
}
int PhotoResources::animation_index(int id,std::uint32_t identity) const noexcept {
    const std::size_t first=id<1000?0:67, end=id<1000?67:83;
    for(std::size_t i=first;i<end;++i) if(species_[i].id==id) {
        for(std::size_t j=0;j<species_[i].count;++j)
            if(animations_[species_[i].first+j].identity==identity) return static_cast<int>(j);
        return -1;
    }
    return -1;
}
float PhotoResources::pose(int id,int animation) const noexcept {
    for(std::size_t i=0;i<67;++i) if(species_[i].id==id) {
        // Source stores -1 as unsigned six bits. Missing animation is not read
        // out of range on native hosts; it has the source no-pose outcome.
        if(animation<0 || std::size_t(animation)>=species_[i].count) return -1;
        return animations_[species_[i].first+std::size_t(animation)].pose;
    }
    return -1;
}
Photo::Photo(IslePlayerPhotoSource& source,IslePlayerSource& player,actor::Owner& actors,
    items::Owner& items,actor::DetectorBinding& detector,const PhotoResources& resources,
    world::World& world,int32_t level_id)
    :source_(source),player_(player),actors_(actors),items_(items),detector_(detector),
     resources_(resources),world_(world),level_id_(level_id) {
    if(source.owner || !source.D_800AE4E4 || !source.D_800AE284 || !source.save ||
       !player.game || !player.camera || player.photo!=&source || level_id<0 || level_id>63)
        throw std::invalid_argument("photo requires canonical Player, admitted resources/save and unbound application roll");
    // Original PokemonDetector_Create162-168 resets course context identities,
    // never the application-owned roll/counters.
    source_.PokemonDetector_TakenPhotoIndex=0;
    for(auto& context:source_.PokemonDetector_Photo) {
        context.pokemonInFocus=nullptr;
        std::fill_n(context.pokemonObjects,12,nullptr);
    }
    source_.owner=this;
}
Photo::~Photo() {
    if(!quiescent() || snapshot_owner_) std::terminate();
    if(source_.owner!=this) std::terminate();
    source_.owner=nullptr;
}
IsleGameResult Photo::bind_snapshot_owner(scene::Services& services,IsleGameObject* object) noexcept {
    if(services_ || snapshot_owner_ || services.game!=player_.game || !object)
        return ISLE_GAME_INVALID_STATE;
    IsleGameObjectMetadata metadata{};
    auto result=isle_game_object_metadata(player_.game,object,&metadata);
    if(result!=ISLE_GAME_OK) return result;
    if(metadata.id!=0 || metadata.update_link!=9 || metadata.priority!=0x80000000u)
        return ISLE_GAME_INVALID_ARGUMENT;
    // Original Create preserves the application-owned serialized photo headers
    // across course owners. It resets membership/focus and recreates the FIFO;
    // a fresh native Detector does not turn retained value data into a stage.
    if(detector_.history.create_source_contexts()!=focus::Error::none)
        return ISLE_GAME_INVALID_STATE;
    std::size_t count=0;
    for(auto& pending:pending_) for(auto& field:pending.holds) hold_fields_[count++]=&field;
    if(!services.source_owner_register(object,hold_fields_)) {
        detector_.history.source_created_=false;
        return ISLE_GAME_INVALID_STATE;
    }
    services_=&services; snapshot_owner_=object;
    detector_.context=1;
    return ISLE_GAME_OK;
}
IsleGameResult Photo::detach_snapshot_owner() noexcept {
    if(!services_ || !snapshot_owner_ || !quiescent() ||
       !services_->source_owner_joined(snapshot_owner_)) return ISLE_GAME_INVALID_STATE;
    const auto discarded=teardown_queue();
    if(discarded!=ISLE_GAME_OK) return discarded;
    if(!services_->source_owner_release(snapshot_owner_)) return ISLE_GAME_INVALID_STATE;
    snapshot_owner_=nullptr; services_=nullptr;
    return ISLE_GAME_OK;
}
Photo::Pending* Photo::matching(IslePlayerPhotoTicket ticket) noexcept {
    if(ticket.context>=2) return nullptr;
    auto& pending=pending_[ticket.context];
    return pending.phase!=Phase::idle && pending.ticket==ticket?&pending:nullptr;
}
IsleGameResult Photo::hold(Pending& pending,IsleGameObject* object) noexcept {
    if(!object) return ISLE_GAME_OK;
    for(std::size_t i=0;i<pending.held;++i) if(pending.holds[i]==object) return ISLE_GAME_OK;
    if(pending.held==pending.holds.size()) return ISLE_GAME_EXHAUSTED;
    if(!services_ || !snapshot_owner_) return ISLE_GAME_INVALID_STATE;
    if(!services_->source_reference_assign(snapshot_owner_,&pending.holds[pending.held],object))
        return ISLE_GAME_INVALID_STATE;
    ++pending.held;
    return ISLE_GAME_OK;
}
IsleGameResult Photo::camera(IslePlayerPhotoData& output) noexcept {
    const IsleGameCameraParameters* parameters=nullptr;
    auto result=isle_game_camera_parameters(player_.game,player_.camera,&parameters);
    if(result!=ISLE_GAME_OK) return result;
    result=actor::photo::prepare_camera(*parameters,camera_);
    if(result!=ISLE_GAME_OK) return result;
    world::Block* block=nullptr;
    for(auto& candidate:world_.blocks()) if(&candidate==player_.movement.block) { block=&candidate; break; }
    if(!block || block->index>255) return ISLE_GAME_INVALID_STATE;
    output.levelID=static_cast<int8_t>(level_id_);
    output.unk_00_16=static_cast<uint8_t>(block->index);
    output.unk_04.f32=world_.global_time();
    output.unk_08={parameters->look_at.eye[0],parameters->look_at.eye[1],parameters->look_at.eye[2]};
    output.unk_14={parameters->look_at.at[0],parameters->look_at.at[1],parameters->look_at.at[2]};
    return ISLE_GAME_OK;
}
IsleGameResult Photo::pokemon(IslePlayerPhotoContext& output) noexcept {
    Pending* pending=nullptr;
    for(std::size_t i=0;i<pending_.size();++i)
        if(&output==&source_.PokemonDetector_Photo[i]) pending=&pending_[i];
    if(!pending || pending->phase!=Phase::rejected || pending->history_retained)
        return ISLE_GAME_INVALID_STATE;
    // Own GAME 47380.c460-569: visit the actual link3 list until twelve
    // successful captures, applying hidden/tangible/visibility/species before
    // reading animation/AUX. History does not exist yet at this prefix.
    IsleGameObject* previous=nullptr;
    std::size_t count=0;
    while(count<12) {
        IsleGameObject* object=nullptr;
        IsleActorSource* source=nullptr;
        IsleGameModel* model=nullptr;
        auto result=isle_actor_source_link3_next(player_.game,previous,&object,&source,&model);
        if(result!=ISLE_GAME_OK) return result;
        if(!object) break;
        previous=object;
        bool admitted=false;
        result=actor::photo::admitted(*source,camera_,source->collPosition,admitted);
        if(result!=ISLE_GAME_OK) return result;
        if(!admitted) continue;
        IsleGameModelRef reference{};
        result=isle_actor_owner_self_model(source,&reference);
        if(result!=ISLE_GAME_OK) return result;
        const IsleGameModelTransform* placement=nullptr;
        result=isle_actor_owner_aux_read(source,&reference,&placement);
        if(result!=ISLE_GAME_OK || !model) return result==ISLE_GAME_OK?ISLE_GAME_INVALID_STATE:result;
        IslePlayerPhotoPokemonState native{};
        native.id=source->id; native.tangible=source->tangible;
        native.animation_index=resources_.animation_index(source->id,source->modelAnims);
        native.default_pose=static_cast<int32_t>(resources_.pose(source->id,native.animation_index));
        native.specialPoseID=source->specialPoseID; native.poseID=source->poseID;
        native.animationTime=isle_actor_owner_animation_time(source);
        native.position=placement->position; native.yaw=placement->rotation.y;
        if(!isle_player_photo_func_8009BF48(&source_,&output.main.pokemons[count],&native))
            return ISLE_GAME_INVALID_STATE;
        output.pokemonObjects[count++]=object;
        // Acquire as each real source identity is captured: later Item/FX or
        // producer failure must not discard earlier semantic/native payloads.
        result=hold(*pending,object);
        if(result!=ISLE_GAME_OK) return result;
    }
    return ISLE_GAME_OK;
}
IsleGameResult Photo::items(IslePlayerPhotoContext& output) noexcept {
    std::size_t count=0;
    // Link4, including independent splash objects, in original OM order. The
    // render binding supplies family/resource identity, not guessed userdata.
    for(auto* object=isle_menu_first_object(player_.game,4);object&&count<6;
        object=isle_menu_next_object(player_.game,object)) {
        IsleGameObjectMetadata metadata{};
        auto result=isle_game_object_metadata(player_.game,object,&metadata);
        if(result!=ISLE_GAME_OK) return result;
        if(metadata.flags&1) continue;
        for(const auto& binding:items_.render_bindings()) if(binding.model.object==object) {
            const IsleGameModelNode* nodes=nullptr;
            std::size_t node_count=0;
            result=isle_game_model_peek_nodes(player_.game,binding.model.model,&nodes,&node_count);
            if(result!=ISLE_GAME_OK) return result;
            if(!node_count) return ISLE_GAME_INVALID_STATE;
            const auto* root=&nodes[0].transform;
            bool visible=false;
            result=actor::photo::point_visible(camera_,root->position,visible);
            if(result!=ISLE_GAME_OK) return result;
            if(!visible) break;
            if(!binding.model.runtime) return ISLE_GAME_INVALID_STATE;
            const auto state=binding.model.runtime->model(binding.model.state.instance);
            if(!state) return ISLE_GAME_INVALID_STATE;
            const auto family=items_.family(binding.model.state.instance);
            if(!family) return ISLE_GAME_INVALID_STATE;
            IslePlayerPhotoItemState native{};
            native.position=root->position; native.scale=root->scale.x;
            native.model_time=state.animation_time;
            switch(*family) {
            case items::Family::water_splash: native.family=2; break;
            case items::Family::lava_splash: native.family=3; break;
            case items::Family::pester_ball:
            case items::Family::apple:
                if(state.nodes.size()<2 || state.nodes[1].material_count==0 ||
                   state.nodes[1].first_material>=state.materials.size()) return ISLE_GAME_INVALID_STATE;
                native.material_time=state.materials[state.nodes[1].first_material].clock.time_passed;
                native.family=*family==items::Family::pester_ball?0:1; break;
            }
            const float consumed_time=native.family<2?native.material_time:native.model_time;
            if(!fits(consumed_time,-2147483648.0f,2147483648.0f) ||
               (native.family==1 && !fits((native.scale*15.0f)/0.1f,-134217728.0f,134217728.0f)))
                return ISLE_GAME_INVALID_ARGUMENT;
            if(isle_player_photo_func_8009C304(&source_,&output.main.items[count],&native)) ++count;
            break;
        }
    }
    return ISLE_GAME_OK;
}
IsleGameResult Photo::effects(IslePlayerPhotoContext& output) noexcept {
    std::array<IslePlayerPhotoParticle,100> particles{};
    auto& runtime=actors_.effects();
    auto count=runtime.particle_count();
    if(count>particles.size()) return ISLE_GAME_INVALID_STATE;
    for(std::size_t i=0;i<count;++i) {
        menu::EffectParticleView value;
        if(!runtime.particle(i,value)) return ISLE_GAME_INVALID_STATE;
        if(!fits(value.size*128.0f,-2147483648.0f,2147483648.0f) ||
           !fits(value.position.x*8.0f,-2147483648.0f,2147483648.0f) ||
           !fits(value.position.y*8.0f,-2147483648.0f,2147483648.0f) ||
           !fits(value.position.z*8.0f,-2147483648.0f,2147483648.0f))
            return ISLE_GAME_INVALID_ARGUMENT;
        auto& particle=particles[i];
        particle.next=i+1<count?&particles[i+1]:nullptr;
        particle.pos=vec(value.position); particle.size=value.size;
        particle.flags=value.flags; particle.bankID=value.bank_id;
        particle.textureID=value.texture; particle.dataID=value.frame;
        particle.primColor={value.primitive[0],value.primitive[1],value.primitive[2],value.primitive[3]};
    }
    for(std::size_t i=0;i<4;++i) for(std::size_t j=0;j<4;++j)
        source_.D_803B1518_551928[i][j]=camera_.view_projection[i][j];
    isle_player_photo_capture_effects(&source_,&output,count?particles.data():nullptr);
    return ISLE_GAME_OK;
}
IsleGameResult Photo::seal_history(std::uint16_t context,std::uint64_t generation) noexcept {
    auto result=source_access();
    if(result!=ISLE_GAME_OK) return result;
    if(context>=2 || !generation) return ISLE_GAME_INVALID_STATE;
    auto& pending=pending_[context];
    if(pending.phase!=Phase::captured || pending.ticket!=IslePlayerPhotoTicket{context,generation,0} ||
       pending.history_retained || detector_.context!=context ||
       detector_.history.generation(context)!=generation || !detector_.history.context_available(context))
        return ISLE_GAME_INVALID_STATE;
    const auto* members=detector_.history.photo(context);
    if(!members) return ISLE_GAME_INVALID_STATE;
    const auto& captured=source_.PokemonDetector_Photo[context];
    for(std::size_t i=0;i<members->objects.size();++i)
        if(members->objects[i]!=reinterpret_cast<focus::Object>(captured.pokemonObjects[i]))
            return remember(ISLE_GAME_INVALID_STATE);
    // Adopt additional actual history identities only after exact commit.
    // Partial acquisition stays rejected and explicitly retireable.
    pending.phase=Phase::rejected;
    for(const auto& subject:detector_.history.history(context)) {
        result=hold(pending,reinterpret_cast<IsleGameObject*>(subject.object));
        if(result!=ISLE_GAME_OK) return remember(result);
    }
    if(detector_.history.retain_context(context,generation)!=focus::Error::none)
        return remember(ISLE_GAME_INVALID_STATE);
    pending.history_retained=true; pending.phase=Phase::prepared;
    return ISLE_GAME_OK;
}
IsleGameResult Photo::postprocess(IslePlayerPhotoTicket ticket,std::span<std::uint32_t> words,
    std::uint32_t regions,std::uint32_t framebuffer) noexcept {
    auto* pending=matching(ticket);
    if(!pending || pending->phase!=Phase::submitted || !pending->frame ||
       pending->frame->submission()!=ticket.submission) return ISLE_GAME_INVALID_STATE;
    if(regions!=detector_.copies.region_offset(0)) return ISLE_GAME_INVALID_ARGUMENT;
    if(detector_.history.analyze(ticket.context,words,regions)!=focus::Error::none) return ISLE_GAME_INVALID_STATE;
    if(detector_.history.mark_center(words,framebuffer,player_.gDirectionIndex)!=focus::Error::none)
        return ISLE_GAME_INVALID_STATE;
    const auto& computed=detector_.history.computed();
    source_.PokemonDetector_Photo[ticket.context].pokemonInFocus=
        reinterpret_cast<IsleGameObject*>(computed.present?computed.object:0);
    pending->phase=Phase::postprocessed; return ISLE_GAME_OK;
}
IsleGameResult Photo::copy_info() noexcept {
    const auto access=source_access();
    if(access!=ISLE_GAME_OK) return access;
    if(completed_<0) return ISLE_GAME_OK;
    auto& pending=pending_[static_cast<std::size_t>(completed_)];
    if(pending.phase!=Phase::complete ||
       detector_.history.publish_context(static_cast<uint16_t>(completed_),pending.ticket.generation)!=focus::Error::none)
        return remember(ISLE_GAME_INVALID_STATE);
    published_=completed_;
    source_.gPokemonIdInFocus=detector_.history.published().pokemon_id;
    return ISLE_GAME_OK;
}
IsleGameResult Photo::borrow(IslePlayerPhotoTicket ticket,const IslePlayerPhotoContext*& output) noexcept {
    output=nullptr; auto* pending=matching(ticket);
    if(!pending || pending->phase!=Phase::complete || pending->borrows==UINT32_MAX) return ISLE_GAME_INVALID_STATE;
    ++pending->borrows; output=&source_.PokemonDetector_Photo[ticket.context]; return ISLE_GAME_OK;
}
IsleGameResult Photo::release(IslePlayerPhotoTicket ticket) noexcept {
    auto* pending=matching(ticket);
    if(!pending || !pending->borrows) return ISLE_GAME_INVALID_STATE;
    --pending->borrows; return ISLE_GAME_OK;
}
IsleGameResult Photo::retire(IslePlayerPhotoTicket ticket) noexcept {
    auto* pending=matching(ticket);
    if(!pending || pending->borrows || (pending->phase!=Phase::complete&&
       pending->phase!=Phase::prepared&&pending->phase!=Phase::captured&&pending->phase!=Phase::rejected))
        return ISLE_GAME_INVALID_STATE;
    return release_pending(*pending);
}
IsleGameResult Photo::release_pending(Pending& pending) noexcept {
    const auto ticket=pending.ticket;
    // Services' actual canonical-field borrows retain Actor identities AND
    // semantic owner slots after logical cleanup/join. Native holds alone would
    // retain only dead GObj addresses, not the source storage.
    if(!services_ || !snapshot_owner_ || isle_menu_current_object(player_.game))
        return ISLE_GAME_INVALID_STATE;
    if(pending.history_retained) {
        if(detector_.history.release_context(ticket.context,ticket.generation)!=focus::Error::none)
            return remember(ISLE_GAME_INVALID_STATE);
        pending.history_retained=false;
    }
    for(std::size_t i=0;i<pending.held;++i) if(pending.holds[i]) {
        if(!services_->source_reference_assign(snapshot_owner_,&pending.holds[i],nullptr))
            return remember(ISLE_GAME_INVALID_STATE);
    }
    pending.held=0; pending.phase=Phase::idle; pending.frame=nullptr;
    if(completed_==ticket.context) completed_=-1;
    if(published_==ticket.context) published_=-1;
    return ISLE_GAME_OK;
}
bool Photo::quiescent() const noexcept {
    return std::all_of(pending_.begin(),pending_.end(),[](const auto& p){return p.phase==Phase::idle;});
}
IsleGameResult Photo::teardown_queue() noexcept {
    const auto access=source_access(true);
    if(access!=ISLE_GAME_OK) return access;
    if(!quiescent()) return ISLE_GAME_INVALID_STATE;
    head_=0;count_=0;
    return ISLE_GAME_OK;
}
} // namespace isle::player
extern "C" IsleGameResult isle_player_photo_publish_completed(IslePlayerPhotoSource* source) {
    if(!source || !source->owner) return ISLE_GAME_INVALID_ARGUMENT;
    return static_cast<isle::player::Photo*>(source->owner)->copy_info();
}

namespace isle::player {
namespace {
void put16(std::span<std::byte> bytes,std::size_t offset,std::uint16_t value) noexcept {
    bytes[offset]=std::byte(value>>8); bytes[offset+1]=std::byte(value);
}
void put32(std::span<std::byte> bytes,std::size_t offset,std::uint32_t value) noexcept {
    put16(bytes,offset,static_cast<uint16_t>(value>>16)); put16(bytes,offset+2,static_cast<uint16_t>(value));
}
std::uint16_t get16(std::span<const std::byte> bytes,std::size_t offset) noexcept {
    return static_cast<uint16_t>((std::to_integer<unsigned>(bytes[offset])<<8)|std::to_integer<unsigned>(bytes[offset+1]));
}
std::uint32_t get32(std::span<const std::byte> bytes,std::size_t offset) noexcept {
    return (uint32_t(get16(bytes,offset))<<16)|get16(bytes,offset+2);
}
void putvec(std::span<std::byte> bytes,std::size_t offset,IsleGameVec3 value) noexcept {
    put32(bytes,offset,std::bit_cast<uint32_t>(value.x));
    put32(bytes,offset+4,std::bit_cast<uint32_t>(value.y));
    put32(bytes,offset+8,std::bit_cast<uint32_t>(value.z));
}
IsleGameVec3 getvec(std::span<const std::byte> bytes,std::size_t offset) noexcept {
    return {std::bit_cast<float>(get32(bytes,offset)),std::bit_cast<float>(get32(bytes,offset+4)),
        std::bit_cast<float>(get32(bytes,offset+8))};
}
}
void encode_photo(const IslePlayerPhotoData& photo,std::span<std::byte,0x3a0> bytes) noexcept {
    std::fill(bytes.begin(),bytes.end(),std::byte{0});
    bytes[0]=std::byte((static_cast<uint8_t>(photo.levelID)&127)<<1|(photo.unk_00_24&1));
    bytes[1]=std::byte(photo.unk_00_16);
    put32(bytes,4,std::bit_cast<uint32_t>(photo.unk_04));
    putvec(bytes,8,photo.unk_08); putvec(bytes,20,photo.unk_14);
    for(std::size_t i=0;i<12;++i) {
        const auto& p=photo.pokemons[i]; auto offset=32+i*24;
        put32(bytes,offset,(static_cast<uint32_t>(p.pokemonID)&8191)<<19|
            (uint32_t(p.unk_00_13)&63)<<13|(uint32_t(p.specialID)&31)<<8|p.poseID);
        put32(bytes,offset+4,std::bit_cast<uint32_t>(p.animationTime));
        putvec(bytes,offset+8,p.position); put32(bytes,offset+20,std::bit_cast<uint32_t>(p.yaw));
    }
    for(std::size_t i=0;i<6;++i) {
        const auto& p=photo.items[i]; auto offset=0x140+i*16;
        bytes[offset]=std::byte(static_cast<uint8_t>(p.itemType)); bytes[offset+1]=std::byte(p.animationTime);
        putvec(bytes,offset+4,p.pos);
    }
    for(std::size_t i=0;i<32;++i) {
        const auto& p=photo.effects[i]; auto offset=0x1a0+i*16;
        bytes[offset]=std::byte(static_cast<uint8_t>(p.textureID)); bytes[offset+1]=std::byte(p.dataID);
        bytes[offset+2]=std::byte(p.bankID); bytes[offset+3]=std::byte(p.unk_03);
        put16(bytes,offset+4,static_cast<uint16_t>(p.posX)); put16(bytes,offset+6,static_cast<uint16_t>(p.posY));
        put16(bytes,offset+8,static_cast<uint16_t>(p.posZ)); put16(bytes,offset+10,p.size);
        for(std::size_t j=0;j<4;++j) bytes[offset+12+j]=std::byte(p.primColor[j]);
    }
}
IsleGameResult decode_photo(std::span<const std::byte,0x3a0> bytes,IslePlayerPhotoData& photo) noexcept {
    auto level=std::to_integer<unsigned>(bytes[0])>>1;
    photo.levelID=static_cast<int8_t>(level&64?static_cast<int>(level)-128:level);
    photo.unk_00_24=std::to_integer<uint8_t>(bytes[0])&1;
    photo.unk_00_16=std::to_integer<uint8_t>(bytes[1]);
    photo.unk_04.f32=std::bit_cast<float>(get32(bytes,4));
    photo.unk_08=getvec(bytes,8); photo.unk_14=getvec(bytes,20);
    for(std::size_t i=0;i<12;++i) {
        auto& p=photo.pokemons[i]; auto offset=32+i*24; auto packed=get32(bytes,offset);
        auto id=packed>>19; p.pokemonID=id&4096?static_cast<int32_t>(id)-8192:static_cast<int32_t>(id);
        p.unk_00_13=static_cast<uint8_t>((packed>>13)&63); p.specialID=static_cast<uint8_t>((packed>>8)&31);
        p.poseID=static_cast<uint8_t>(packed); p.animationTime=std::bit_cast<float>(get32(bytes,offset+4));
        p.position=getvec(bytes,offset+8); p.yaw=std::bit_cast<float>(get32(bytes,offset+20));
    }
    for(std::size_t i=0;i<6;++i) {
        auto& p=photo.items[i]; auto offset=0x140+i*16;
        p.itemType=std::bit_cast<int8_t>(std::to_integer<uint8_t>(bytes[offset]));
        p.animationTime=std::to_integer<uint8_t>(bytes[offset+1]); p.pos=getvec(bytes,offset+4);
    }
    for(std::size_t i=0;i<32;++i) {
        auto& p=photo.effects[i]; auto offset=0x1a0+i*16;
        p.textureID=std::bit_cast<int8_t>(std::to_integer<uint8_t>(bytes[offset]));
        p.dataID=std::to_integer<uint8_t>(bytes[offset+1]); p.bankID=std::to_integer<uint8_t>(bytes[offset+2]);
        p.unk_03=std::to_integer<uint8_t>(bytes[offset+3]);
        p.posX=std::bit_cast<int16_t>(get16(bytes,offset+4)); p.posY=std::bit_cast<int16_t>(get16(bytes,offset+6));
        p.posZ=std::bit_cast<int16_t>(get16(bytes,offset+8)); p.size=get16(bytes,offset+10);
        for(std::size_t j=0;j<4;++j) p.primColor[j]=std::to_integer<uint8_t>(bytes[offset+12+j]);
    }
    return ISLE_GAME_OK;
}
}

extern "C" IsleGameResult isle_player_photo_capture_camera(IslePlayerPhotoSource* source,IslePlayerPhotoContext* output) {
    if(!source || !source->owner || !output) return ISLE_GAME_INVALID_ARGUMENT;
    return static_cast<isle::player::Photo*>(source->owner)->camera(output->main);
}
extern "C" IsleGameResult isle_player_photo_capture_pokemon(IslePlayerPhotoSource* source,IslePlayerPhotoContext* output) {
    if(!source || !source->owner || !output) return ISLE_GAME_INVALID_ARGUMENT;
    return static_cast<isle::player::Photo*>(source->owner)->pokemon(*output);
}
extern "C" IsleGameResult isle_player_photo_capture_items(IslePlayerPhotoSource* source,IslePlayerPhotoContext* output) {
    if(!source || !source->owner || !output) return ISLE_GAME_INVALID_ARGUMENT;
    return static_cast<isle::player::Photo*>(source->owner)->items(*output);
}
extern "C" IsleGameResult isle_player_photo_capture_fx(IslePlayerPhotoSource* source,IslePlayerPhotoContext* output) {
    if(!source || !source->owner || !output) return ISLE_GAME_INVALID_ARGUMENT;
    return static_cast<isle::player::Photo*>(source->owner)->effects(*output);
}

IsleGameResult isle::player::Photo::remember(IsleGameResult result) noexcept {
    if(source_.status==ISLE_GAME_OK) source_.status=result;
    return result;
}
IsleGameResult isle::player::Photo::source_access(bool cleanup) noexcept {
    // counts checks the real game/continuation owner without rejecting a sticky
    // execution fault. In particular, a foreign thread may not change status.
    IsleGameCounts counts{};
    auto result=isle_game_counts(player_.game,&counts);
    if(result!=ISLE_GAME_OK) return result;
    if(source_.owner!=this || player_.photo!=&source_ || !player_.owner || !player_.rng)
        return remember(ISLE_GAME_INVALID_STATE);
    if(cleanup) return ISLE_GAME_OK;
    if(source_.status!=ISLE_GAME_OK) return source_.status;
    IslePlayerSource* active=nullptr;
    result=isle_player_source_context(player_.game,&active);
    if(result!=ISLE_GAME_OK) return remember(result);
    return remember(active==&player_?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE);
}
IsleGameResult isle::player::Photo::published_focus(IslePlayerPhotoFocus* output) noexcept {
    if(output) *output={};
    auto result=source_access();
    if(result!=ISLE_GAME_OK) return result;
    if(!output) return remember(ISLE_GAME_INVALID_ARGUMENT);
    const auto& published=detector_.history.published();
    *output={published.present?1:0,reinterpret_cast<IsleGameObject*>(published.object),
        published.pokemon_id,published.flags,published.context};
    return ISLE_GAME_OK;
}
IsleGameResult isle::player::Photo::clear_published_presence() noexcept {
    auto result=source_access();
    if(result!=ISLE_GAME_OK) return result;
    detector_.history.clear_published_presence();
    return ISLE_GAME_OK;
}
IsleGameResult isle::player::Photo::detector_enabled(bool enabled) noexcept {
    const auto result=source_access();
    if(result!=ISLE_GAME_OK) return result;
    detector_.enabled=enabled;
    return ISLE_GAME_OK;
}
IsleGameResult isle::player::Photo::borrow_taken(IslePlayerPhotoTicket* output) noexcept {
    if(output) *output={};
    auto result=source_access();
    if(result!=ISLE_GAME_OK) return result;
    if(!output) return remember(ISLE_GAME_INVALID_ARGUMENT);
    const auto context=source_.PokemonDetector_TakenPhotoIndex;
    if(context<0 || context>=2 || published_!=context ||
       detector_.history.published().context!=context) return remember(ISLE_GAME_INVALID_STATE);
    const auto ticket=pending_[static_cast<std::size_t>(context)].ticket;
    if(!ticket.submission) return remember(ISLE_GAME_INVALID_STATE);
    const IslePlayerPhotoContext* borrowed=nullptr;
    result=borrow(ticket,borrowed);
    if(result!=ISLE_GAME_OK) return remember(result);
    *output=ticket;
    return ISLE_GAME_OK;
}
IsleGameResult isle::player::Photo::release_taken(const IslePlayerPhotoTicket* ticket) noexcept {
    auto result=source_access(true);
    if(result!=ISLE_GAME_OK) return result;
    if(!ticket) return remember(ISLE_GAME_INVALID_ARGUMENT);
    // Release the exact borrow even if publication advanced or Source failed.
    // A successful cleanup must not erase the first Photo failure.
    return remember(release(*ticket));
}
IsleGameResult isle::player::Photo::require_taken_context() noexcept {
    auto result=source_access();
    if(result!=ISLE_GAME_OK) return result;
    const auto context=source_.PokemonDetector_TakenPhotoIndex;
    if(context<0 || context>=2 || published_!=context ||
       detector_.history.published().context!=context) return remember(ISLE_GAME_INVALID_STATE);
    const auto& pending=pending_[static_cast<std::size_t>(context)];
    return remember(pending.phase==Phase::complete && pending.borrows?
        ISLE_GAME_OK:ISLE_GAME_INVALID_STATE);
}
namespace {
IsleGameResult photo_owner_failure(IslePlayerPhotoSource* source) noexcept {
    if(!source) return ISLE_GAME_INVALID_ARGUMENT;
    if(source->status==ISLE_GAME_OK) source->status=ISLE_GAME_INVALID_STATE;
    return source->status;
}
}
extern "C" IsleGameResult isle_player_photo_published_focus(IslePlayerPhotoSource* source,IslePlayerPhotoFocus* output) {
    if(!source || !source->owner) {
        if(output) *output={};
        return photo_owner_failure(source);
    }
    return static_cast<isle::player::Photo*>(source->owner)->published_focus(output);
}
extern "C" IsleGameResult isle_player_photo_clear_published_presence(IslePlayerPhotoSource* source) {
    if(!source || !source->owner) return photo_owner_failure(source);
    return static_cast<isle::player::Photo*>(source->owner)->clear_published_presence();
}
extern "C" IsleGameResult isle_player_photo_native_detector_enabled(IslePlayerPhotoSource* source,int enabled) {
    if(!source || !source->owner) return photo_owner_failure(source);
    return static_cast<isle::player::Photo*>(source->owner)->detector_enabled(enabled!=0);
}
extern "C" IsleGameResult isle_player_photo_borrow_taken(IslePlayerPhotoSource* source,IslePlayerPhotoTicket* output) {
    if(!source || !source->owner) {
        if(output) *output={};
        return photo_owner_failure(source);
    }
    return static_cast<isle::player::Photo*>(source->owner)->borrow_taken(output);
}
extern "C" IsleGameResult isle_player_photo_release_taken(IslePlayerPhotoSource* source,const IslePlayerPhotoTicket* ticket) {
    if(!source || !source->owner) return photo_owner_failure(source);
    return static_cast<isle::player::Photo*>(source->owner)->release_taken(ticket);
}
extern "C" IsleGameResult isle_player_photo_require_taken_context(IslePlayerPhotoSource* source) {
    if(!source || !source->owner) return photo_owner_failure(source);
    return static_cast<isle::player::Photo*>(source->owner)->require_taken_context();
}

extern "C" int32_t isle_player_photo_checkPlayerFlag(IslePlayerPhotoSource* source,int32_t id) {
    if(!source) return -1;
    if(!source->save || id<0) { source->status=ISLE_GAME_INVALID_STATE; return -1; }
    try {
        auto snapshot=static_cast<isle::save::GameSession*>(source->save)->snapshot();
        if(!snapshot) { source->status=ISLE_GAME_INVALID_STATE; return -1; }
        auto value=snapshot.value.player_flag(static_cast<unsigned>(id));
        if(!value) { source->status=ISLE_GAME_INVALID_STATE; return -1; }
        return value.value?1:0;
    } catch(...) { source->status=ISLE_GAME_INVALID_STATE; return -1; }
}
extern "C" int32_t isle_player_photo_report_photo_present(IslePlayerPhotoSource* source,int32_t index) {
    if(!source) return -1;
    if(!source->save || index<0 || index>=static_cast<int32_t>(isle::save::game_report_count)) {
        source->status=ISLE_GAME_INVALID_STATE; return -1;
    }
    try {
        auto snapshot=static_cast<isle::save::GameSession*>(source->save)->snapshot();
        if(!snapshot) { source->status=ISLE_GAME_INVALID_STATE; return -1; }
        auto value=snapshot.value.photo_present(isle::save::PhotoSet::report,static_cast<std::size_t>(index));
        if(!value) { source->status=ISLE_GAME_INVALID_STATE; return -1; }
        return value.value?1:0;
    } catch(...) { source->status=ISLE_GAME_INVALID_STATE; return -1; }
}

#include "environment_sound.hpp"

#include <algorithm>
#include <cmath>
#include <new>
#include <stdexcept>

namespace isle::environment_sound {
namespace {
std::uintptr_t key(IsleGameObject* object) noexcept { return reinterpret_cast<std::uintptr_t>(object); }
}
BeachResources::BeachResources(const rom::Snapshot& snapshot) : angles_(snapshot) {
    const rom::GuestView table{0x802cbe10,0x563e80,snapshot.canonical().subspan(0x563e80,53*4)};
    for (std::size_t i=0; i<descriptors_.size(); ++i)
        descriptors_[i]={table.u16(i*4),table.u8(i*4+2),table.u8(i*4+3)};
}
std::span<const Descriptor> BeachResources::descriptors() const {
    if (!angles_.live()) throw std::logic_error("Environmental sound borrows moved Snapshot");
    return descriptors_;
}
Environment::Record* Environment::allocate() noexcept {
    isle_span storage{};
    if (isle_arena_allocate(&arena_,sizeof(Record),alignof(Record),&storage)!=ISLE_OK) {
        services_.fail("environmental sound record arena exhausted"); return nullptr;
    }
    return ::new (static_cast<void*>(storage.data)) Record{};
}
Environment::Environment(scene::Services& services,isle_arena& arena,const BeachResources& resources,
                         IsleGameModel* player,IsleGameCamera* camera)
    : services_(services),arena_(arena),resources_(resources),player_(player),camera_(camera) {
    static_cast<void>(resources.descriptors());
    records_=allocate();
    if (!records_) throw std::runtime_error("Environmental sound initial record allocation failed");
    update_object_=services_.object(400,9,1); // GAME OBJID_ENV_SOUND_PLAYER/LINK_PLAYER.
    if (!update_object_) throw std::runtime_error("Environmental sound update object failed");
    if (!services_.set_lifecycle(update_object_,{nullptr,joined,this}) ||
        !services_.process(update_object_,tick,this,ISLE_GAME_SYNCHRONOUS,1)) {
        services_.erase(update_object_);
        throw std::runtime_error("Environmental sound update process failed");
    }
}
Environment::~Environment() {
    cleanup();
    if (update_object_) services_.erase(update_object_);
}
void Environment::joined(IsleGameObject* object,void* user) noexcept {
    auto& owner=*static_cast<Environment*>(user);
    if (owner.update_object_==object) owner.update_object_=nullptr;
}
void Environment::tick(IsleGameObject*,void* user) noexcept {
    static_cast<Environment*>(user)->update();
}
void Environment::cleanup() noexcept { player_=nullptr; camera_=nullptr; }
bool Environment::set_listener(IsleGameModel* player, IsleGameCamera* camera) noexcept {
    if ((player == nullptr) != (camera == nullptr)) return false;
    if (player) {
        const IsleGameModelPlacement* placement = nullptr;
        const IsleGameCameraParameters* parameters = nullptr;
        if (isle_game_model_placement(services_.game,player,&placement) != ISLE_GAME_OK ||
            isle_game_camera_parameters(services_.game,camera,&parameters) != ISLE_GAME_OK) return false;
    }
    player_=player; camera_=camera;
    return true;
}
bool Environment::volume_pan(Source source,std::uint8_t range,std::int32_t& volume,std::int32_t& pan) noexcept {
    if (!source.player_distance || !std::isfinite(*source.player_distance) || *source.player_distance<0 ||
        !resources_.angles().live()) {
        services_.fail("invalid environmental sound source distance/resource"); return false;
    }
    const float hearing=static_cast<float>(range&0x7f);
    const float distance=*source.player_distance/100.0f;
    const float decay=hearing/(hearing+distance);
    float front=1.0f;
    std::int32_t p=0x40;
    if (!(range&0x80)) {
        const IsleGameModelPlacement *position=nullptr,*player=nullptr;
        const IsleGameCameraParameters* camera=nullptr;
        if (!services_.check(isle_game_model_placement(services_.game,source.model,&position),"environment source placement") ||
            !services_.check(isle_game_model_placement(services_.game,player_,&player),"environment player placement") ||
            !services_.check(isle_game_camera_parameters(services_.game,camera_,&camera),"environment camera parameters")) return false;
        const float dx=position->transform.position.x-player->transform.position.x;
        const float dz=position->transform.position.z-player->transform.position.z;
        std::uint16_t source_angle=0,camera_angle=0;
        // The source returns before reading camera direction for a quantized-zero target.
        std::int32_t angle=0;
        if (!(std::fabs(dx)<1.0f && std::fabs(dz)<1.0f)) {
            if (!resources_.angles().atan2s(dz,dx,source_angle) ||
                !resources_.angles().atan2s(camera->look_at.at[2]-camera->look_at.eye[2],
                                           camera->look_at.at[0]-camera->look_at.eye[0],camera_angle)) {
                services_.fail("environmental sound angle outside source conversion domain"); return false;
            }
            angle=(static_cast<std::int32_t>(source_angle)-camera_angle)&0x7fff;
        }
        p=(angle+0x2000)&0x7fff;
        if (p>0x4000) p=(-p)&0x7fff;
        p=std::min(p/128,127);
        if (angle>0x4000) angle=(-angle)&0x7fff;
        front=-0.000030517578f*static_cast<float>(angle)+1.0f;
    }
    const float result=(decay*front)*32767.0f;
    if (!std::isfinite(result) || result<0 || result>=2147483648.0f) {
        services_.fail("environmental sound volume outside defined source conversion domain"); return false;
    }
    volume=static_cast<std::int32_t>(result); pan=p;
    return true;
}
bool Environment::admits_sound(std::int32_t sound) const noexcept {
    if (!resources_.angles().live()) return false;
    for (const auto& descriptor : resources_.descriptors())
        if (descriptor.sound == sound) return services_.audio.has_sound(static_cast<std::uint32_t>(sound));
    return true; // The same source policy as play: absent descriptors are ignored.
}
bool Environment::play(Source source,std::uint8_t category,std::int32_t sound) noexcept {
    if (!player_ || !camera_ || !records_) return true;
    if (!resources_.angles().live()) { services_.fail("environmental sound resource retired"); return false; }
    for (const auto& descriptor:resources_.descriptors()) if (descriptor.sound==sound)
        return play(source,category,sound,descriptor.pitch_modifier,descriptor.hearing_range);
    return true; // GAME ignores sounds absent from its initialized descriptor table.
}
bool Environment::play(Source source,std::uint8_t category,std::int32_t sound,
                       std::uint8_t pitch_modifier,std::uint8_t range) noexcept {
    if (!source.identity || !source.player_distance || !std::isfinite(*source.player_distance) || *source.player_distance<0) {
        services_.fail("invalid environmental sound source"); return false;
    }
    if (static_cast<float>(2*range)<*source.player_distance/100.0f || !player_ || !camera_) return true;
    if (!stop_category(source.identity,category)) return false;
    Record* record=records_;
    Record* last=nullptr;
    for (;record && record->occupied;record=record->next) last=record;
    if (!record) {
        record=allocate(); if (!record) return false;
        if (last) last->next=record; else records_=record;
    }
    std::int32_t volume=0,pan=0;
    if (!volume_pan(source,range,volume,pan)) return false;
    const auto pitch=std::min<std::uint8_t>(pitch_modifier,127);
    float multiplier=1;
    switch (pitch) {
    case 1: multiplier=0.8f; break;
    case 2: multiplier=0.7f; break;
    case 3: multiplier=0.75f; break;
    case 4: multiplier=0.5f; break;
    default: break;
    }
    int handle=-1;
    if (!services_.check(services_.audio.play_sound(static_cast<std::uint32_t>(sound),
        {static_cast<std::uint16_t>(std::min(volume,32767)),static_cast<std::uint8_t>(std::min(pan,127)),0,multiplier},handle),
        "environmental sound play")) return false;
    if (handle!=-1) {
        record->identity=key(source.identity); record->source=source; record->handle=handle;
        record->category=category&3; record->hearing_range=range;
        record->sound=static_cast<std::uint16_t>(sound); record->fixed=false; record->occupied=true;
    }
    return true; // A source-priority refusal returns -1 without faulting audio.
}
bool Environment::stop_category(IsleGameObject* source,std::uint8_t category) noexcept {
    bool result=true;
    for (auto* record=records_;record;record=record->next) if (record->identity==key(source) && record->identity && record->category==category) {
        result=services_.check(services_.audio.stop_sound(static_cast<unsigned>(record->handle)),"environmental sound category stop") && result;
        record->identity=0; record->source={}; record->occupied=false;
    }
    return result;
}
bool Environment::stop(IsleGameObject* source) noexcept {
    bool result=true;
    for (auto* record=records_;record;record=record->next) if (record->identity==key(source) && record->identity) {
        result=services_.check(services_.audio.stop_sound(static_cast<unsigned>(record->handle)),"environmental sound source stop") && result;
        record->identity=0; record->source={}; record->occupied=false;
    }
    return result;
}
void Environment::fix_parameters(IsleGameObject* source) noexcept {
    for (auto* record=records_;record;record=record->next) if (record->identity==key(source) && record->identity) {
        record->fixed=true; record->source={};
    }
}
void Environment::release_identity(IsleGameObject* source) noexcept {
    for (auto* record=records_;record;record=record->next)
        if (record->identity==key(source) && record->identity) {
            record->fixed=true; record->source={}; record->identity=0;
        }
}
int Environment::sound_handle(std::int32_t sound) const noexcept {
    for (auto* record=records_;record;record=record->next)
        if (record->occupied && record->sound==sound) return record->handle;
    return -1;
}
bool Environment::update() noexcept {
    if (!player_ || !camera_) return true;
    for (auto* record=records_;record;record=record->next) if (record->occupied) {
        const auto sound=services_.audio.playing_sound(static_cast<unsigned>(record->handle));
        if (sound==-1 || sound!=record->sound) {
            record->identity=0; record->source={}; record->fixed=true; record->occupied=false;
        } else if (!record->fixed) {
            std::int32_t volume=0,pan=0;
            if (!volume_pan(record->source,record->hearing_range,volume,pan) ||
                !services_.check(services_.audio.sound_volume(static_cast<unsigned>(record->handle),static_cast<std::uint32_t>(volume)),
                                 "environmental sound volume")) return false;
            if (!(record->hearing_range&0x80) &&
                !services_.check(services_.audio.sound_pan(static_cast<unsigned>(record->handle),static_cast<std::uint32_t>(pan)),
                                 "environmental sound pan")) return false;
        }
    }
    return true;
}
} // namespace isle::environment_sound

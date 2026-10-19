#include "native_sign_puzzles.hpp"
#include "application_runtime.hpp"
#include "game_state.hpp"
#include "beach_lifecycle.hpp"
#include "tunnel_scene.hpp"
#include "volcano_scene.hpp"
#include "river_scene.hpp"
#include "cave_scene.hpp"
#include "valley_scene.hpp"
#include "actor_owner_source.h"
#include "actor_owner.hpp"
#include "actor_producer.hpp"
#include "tunnel_actor_source.h"
#include "volcano_actor_source.h"
#include "river_actor_source.h"
#include "player_source.h"
#include "player_photo_source.h"
#include "player_ui_source.h"
#include "item_owner.hpp"
#include "world_runtime.hpp"
#include <cmath>
#include <stdexcept>
#include <string>

namespace isle::private_campaign {
namespace {
// Immutable Source df8815e75f3d79618466e01e0827c6165b93d11c. ROM graph
// evidence below was read from the admitted US image, SHA1
// edc7c49cc568c045fe48be0d18011c30f393cbaf, using the course-local mappings
// in actor_behavior_resources.cpp109-130,142-144,180-199. No ROM data is
// embedded, no callback below is invoked, and no score is fabricated.
constexpr std::array<SignEvidence,6> evidence{{
    {1004,0,0x80136ac0,
     "src/beach/562370.c:10-54; Actor1004 init802cd948, state802ca300",
     "No item/flute producer: real progress0,.92 sets FLAG4;0,.99 clears it. GObj2 is not hidden1.",
     "pokemon_detect.c:82-106 bypasses membership; app_render/47380.c:785-829 encodes primary flags60"},
    {1010,1,0x801382e0,
     "src/tunnel/pkmn_1010/code.c:12-89; Actor1010 init802efafc, state802eafb0",
     "pikachu/code.c:223-427 behavior5 Apple->egg distance<600 + uninterrupted Flute180; zapdos_egg/code.c:38-71 cmd41->hatch->28; zapdos/code.c:24-45 powers bit31; pkmn_1009/code.c:32-40 reveals at1,.9. ROM5ebfa8/5ecffc/5ed01c/5ecc98 graphs802eeed8/802eff2c/802eff4c/802efbc8.",
     "Power THEN progress3,.613; FLAG4 flashes, permanently removed by cmd47 at4,.351. Ordinary detector membership AND primary Pokemon1010 required."},
    {1018,4,0x801450c0,
     "src/cave/647360.c:235-247; Actor1018 init802c7f30, state802c5580",
     "No item/flute prerequisite: looping actual model animation802c7f08; first child hidden by real spawn, not solver.",
     "pokemon_detect.c:82-106 bypasses membership; app_render/47380.c:785-829 encodes primary flags80"},
    {1022,3,0x80145b08,
     "src/river/6C66E0.c:40-92; Actor1022 init802e3dcc, state802dea00",
     "6C6A70.c:106-216 Vileplume45, graph802e3f6c (ROM6cbd4c) commands5/6/7->802df360; actual aux802df104 clears smoke after280 updates, not a solver clock.",
     "Progress1,.9 through2,.9; actual D_802E3DA0 controls FLAG4. Special membership bypass, primary flagsa0."},
    {1028,2,0x801140c0,
     "src/volcano/koffing_smoke/code.c:60-112; Actor1028 init802e3254, state802de390",
     "smoke_spawner/code.c:38-46,109-180 Actor1029, cmd10 real Pester sender; sender ground MUST be FF4C19. WaitSmokeFade prevents duplicate production; real smoke1028 follows its path and reveal animation.",
     "koffing_smoke/code.c:75-99 show->RevealFace->hide->cleanup. Ordinary detector membership AND primary Pokemon1028 required; never substitute1031 volcano smoke."},
    {1035,5,0x80119bb0,
     "src/valley/7AA720.c:3-8; Actor1035 init802ec7b8, state802d1190",
     "No item/flute producer: actual mountain model, initial state immediately ends. Dash only authorizes later sign marking.",
     "pokemon_detect.c:82-106 bypasses membership; app_render/47380.c:785-829 encodes primary flagse0"}
}};
constexpr std::uint16_t a_button=0x8000, b_button=0x4000, z_button=0x2000, flute_button=0x0004;
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
float distance(const IsleGameVec3& a, const IsleGameVec3& b) {
    return std::hypot(a.x-b.x,a.y-b.y,a.z-b.z);
}
struct Actor {
    bool present=false, revealed=false, member=false, focus=false;
    bool pursuing=false, charging=false, hatching=false, flying=false, producing=false, face=false;
    int id=0, behavior=0;
    std::uint16_t flags=0;
    std::uint32_t graph=0, processes=0;
    float player_distance=0;
    IsleGameVec3 position{}, collision{};
};
struct Observations { Actor sign, producer, egg, zapdos; };
Observations observe(const IslePlayerSource& player, int sign_id, const IslePlayerPhotoFocus& focus) {
    Observations result;
    IsleGameObject* previous=nullptr;
    for (;;) {
        IsleGameObject* object=nullptr;
        IsleActorSource* source=nullptr;
        IsleGameModel* model=nullptr;
        require(isle_actor_source_link3_next(player.game,previous,&object,&source,&model)==ISLE_GAME_OK,
                "sign solver actual Actor traversal rejected");
        if (!object) break;
        previous=object;
        Actor* target=nullptr;
        if (source->id==sign_id) target=&result.sign;
        else if ((sign_id==1010 && source->id==25 && source->behavior==5) ||
                 (sign_id==1022 && source->id==45) || (sign_id==1028 && source->id==1029))
            target=&result.producer;
        else if (sign_id==1010 && source->id==602) target=&result.egg;
        else if (sign_id==1010 && source->id==145) target=&result.zapdos;
        if (!target) continue;
        require(!target->present,"sign solver duplicate actual producer/sign");
        const IsleGameModelPlacement* placement=nullptr;
        IsleGameObjectMetadata metadata{};
        require(model && isle_game_model_placement(player.game,model,&placement)==ISLE_GAME_OK && placement &&
                placement->mode!=ISLE_GAME_MODEL_PLACEMENT_DISABLED &&
                isle_game_object_metadata(player.game,object,&metadata)==ISLE_GAME_OK,
                "sign solver actual model placement unavailable");
        target->present=true; target->id=source->id; target->behavior=source->behavior;
        target->revealed=!(metadata.flags&1u);
        target->flags=source->flags; target->processes=source->processFlags;
        target->graph=source->transitionGraph?source->transitionGraph->source_identity:0;
        target->position=placement->transform.position;
        // This is the canonical Source model/collision-offset/root transform,
        // refreshed by actual Actor physics, not a walkthrough coordinate.
        target->collision=source->collPosition;
        target->player_distance=source->playerDist;
        target->focus=focus.present && focus.object==object && focus.pokemon_id==source->id && (focus.flags&4u);
        if (focus.context<2 && player.photo)
            for (auto* member:player.photo->PokemonDetector_Photo[focus.context].pokemonObjects)
                target->member|=member==object;
        IsleGameSourceCallback state=nullptr;
        if (source->stateProc)
            require(isle_game_process_source_function(player.game,source->stateProc,&state)==ISLE_GAME_OK,
                    "sign solver actual state observation rejected");
        target->pursuing=state==func_802E796C_5E4A3C;
        target->charging=state==func_802E7D04_5E4DD4 || (source->id==25 && (source->processFlags&0x2000u));
        target->hatching=source->id==602 && (state==func_802EC078_5E9148 || state==func_802EC040_5E9110);
        target->flying=source->id==145 && state==func_802EB430_5E8500;
        target->producing=state==smoke_spawner_WaitSmokeFade || state==smoke_spawner_SpawnKoffingSmoke ||
                          state==func_802DF360_6C7140;
        target->face=state==koffing_smoke_RevealFace;
    }
    return result; // No native/Source/model handle survives the update.
}
int primary(const IslePlayerPhotoData& photo) {
    if (photo.levelID<0) return 0;
    // Original makePhoto and score preferred-subject flags, NOT pokemons[0]
    // for the four non-membership signs.1010/1028 use actual record subject0.
    switch (photo.unk_00_16&0xe0) {
    case 0x60:return 1004;
    case 0x80:return 1018;
    case 0xa0:return 1022;
    case 0xe0:return 1035;
    case 0x20:return photo.pokemons[0].pokemonID;
    default:return 0;
    }
}
template<class Entry> bool course_entry(const Entry* entry,const IslePlayerSource*& player,
                                        const course::NativeHost*& host) {
    if (!entry || entry->status()!=course::LifecycleStatus::running) return false;
    host=entry->native_host();
    if (!host || !host->execution_open()) return false;
    player=entry->adapter().native.player();
    return player!=nullptr;
}
bool lava_target(const world::World& world, const IslePlayerSource& player,
                 IsleGameVec3 origin, IsleGameVec3& target) {
    // Query the real crater around its actual1029 producer. This is a terrain
    // search, not an elapsed-input script: accepted interior and launch arc
    // must both exist. Never use the smoke's aerial position as an impact.
    for (int ring=0;ring<=40;++ring)
        for (int x=-ring;x<=ring;++x) for (int z=-ring;z<=ring;++z) {
            if (std::abs(x)!=ring && std::abs(z)!=ring) continue;
            const float px=origin.x+25.0f*static_cast<float>(x);
            const float pz=origin.z+25.0f*static_cast<float>(z);
            world::GroundResult ground{}, margin{};
            const auto lava=[&](float xx,float zz,world::GroundResult& out) {
                return world.ground_at(xx,zz,out) && out.surface==0xff4c19;
            };
            if (!lava(px,pz,ground) || !lava(px-25,pz,margin) || !lava(px+25,pz,margin) ||
                !lava(px,pz-25,margin) || !lava(px,pz+25,margin)) continue;
            const IsleGameVec3 point{px,ground.height,pz};
            IsleGameVec3 aimed{};
            if (projectile_target(player,point,aimed)) { target=point; return true; }
        }
    return false;
}
} // namespace
std::span<const SignEvidence,6> sign_evidence() noexcept { return evidence; }
SignPuzzles::SignPuzzles(int course,std::array<bool,6> reported,
                         private_post_course::Controller controller,Aim aim)
    :controller_(controller),aim_(aim) {
    require(course>=0 && course<=6 && controller_.set && aim_.point,
            "sign solver requires actual course, pad and shared aim bridges");
    facts_.course=course;
    for (std::size_t i=0;i<evidence.size();++i) if (evidence[i].course==course) {
        index_=static_cast<int>(i); reported_=reported[i]; facts_.id=evidence[i].id; break;
    }
}
void SignPuzzles::publish(std::uint16_t buttons,std::int8_t x,std::int8_t y) noexcept {
    controller_.set(controller_.owner,buttons,x,y);
}
bool SignPuzzles::own(SignDependency dependency,std::uint16_t buttons,std::int8_t x,std::int8_t y) noexcept {
    facts_.owns_pad=true; facts_.dependency=dependency; publish(buttons,x,y); return true;
}
bool SignPuzzles::zoom(const IslePlayerSource& player,bool desired) {
    if (player.TurnToDirSpeed!=0 || player.TargetDirectionZoomedIn!=0 || player.D_80382D08_523118!=0)
        return own(SignDependency::camera_turning,desired && player.ZoomSwitchMode==0?z_button:0);
    const bool zoomed=player.gDirectionIndex==-1;
    if (player.gDirectionIndex!=-2 && zoomed==desired) return false;
    const auto button=static_cast<std::uint16_t>(player.ZoomSwitchMode==0 ? (desired?z_button:0) :
        (player.gDirectionIndex!=-2 && !(player.input.current_buttons&z_button)?z_button:0));
    return own(SignDependency::camera_zoom,button);
}
bool SignPuzzles::throw_item(const IslePlayerSource& player,const IsleGameVec3& point,Press press) {
    if (zoom(player,false)) return true;
    const auto button=press==Press::apple?a_button:b_button;
    const int count=press==Press::apple?1:2;
    if (!player.ui || player.ui->Icons_NumItemsAvailable<count)
        return own(SignDependency::item_ui_not_ready);
    IsleGameVec3 aimed{};
    if (!projectile_target(player,point,aimed)) return own(SignDependency::projectile_out_of_reach);
    facts_.input_target=point;
    std::int8_t x=0,y=0;
    aim_.point(aim_.owner,player,aimed,false,x,y);
    std::uint16_t buttons=0;
    if (!x && !y && player.ThrowItemTimeout==0 && !(player.input.current_buttons&button)) {
        pending_=press; buttons=button; ++facts_.item_requests;
    }
    return own(SignDependency::none,buttons,x,y);
}
bool SignPuzzles::play_flute(const IslePlayerSource& player) {
    if (zoom(player,false)) return true;
    if (!player.ui || player.ui->Icons_NumItemsAvailable<3) return own(SignDependency::item_ui_not_ready);
    if (!facts_.flute_playing && player.PressPokeFluteTimeout==0 && !(player.input.current_buttons&flute_button)) {
        pending_=Press::flute; ++facts_.item_requests;
        return own(SignDependency::none,flute_button);
    }
    return own(SignDependency::none);
}
void SignPuzzles::frame(const application::RuntimeView& view,std::uint64_t ordinal) {
    facts_.geometry_ready=false;
    if (index_<0 || reported_ || facts_.capture_observed) return;
    const IslePlayerSource* player=nullptr;
    const actor::Resources* resources=nullptr;
    const actor::Owner* actors=nullptr;
    const actor::Producer* producer=nullptr;
    const world::World* world=nullptr;
    const auto native_entry=[&](const auto* entry) {
        const course::NativeHost* host=nullptr;
        if (!course_entry(entry,player,host)) return;
        resources=&host->actor_resources(); actors=host->actors();
        producer=host->actor_packet_producer(); world=host->world_owner();
    };
    switch (facts_.course) {
    case 0:
        if (view.beach && view.beach->status()==beach::LifecycleStatus::running) {
            player=view.beach->source().player;
            resources=&view.beach->actor_resources(); actors=view.beach->actors();
            producer=view.beach->actor_packet_producer(); world=view.beach->world_owner();
        }
        break;
    case 1:native_entry(view.tunnel);break;
    case 2:native_entry(view.volcano);break;
    case 3:native_entry(view.river);break;
    case 4:native_entry(view.cave);break;
    case 5:native_entry(view.valley);break;
    }
    if (!view.entry || !player || !resources || !actors || !producer || !world) return;
    const auto* block=world->current();
    if (!block || !block->resource) return;
    bool found=false;
    IsleGameObject* previous=nullptr;
    for (;;) {
        IsleGameObject* object=nullptr;
        IsleActorSource* source=nullptr;
        IsleGameModel* model=nullptr;
        require(isle_actor_source_link3_next(player->game,previous,&object,&source,&model)==ISLE_GAME_OK,
                "sign Frame actual Actor traversal rejected");
        if (!object) break;
        previous=object;
        if (source->id!=facts_.id) continue;
        require(!found,"sign Frame duplicate actual sign"); found=true;
        const actor::ActorInitialization* initialization=nullptr;
        for (const auto& candidate:resources->initializations())
            if (source->initData==&candidate) initialization=&candidate;
        require(initialization && model,"sign Frame actual initialization/model absent");
        const auto instance=actors->instance(object);
        require(bool(instance),"sign Frame actual model instance absent");
        const auto world_nodes=producer->world_nodes(instance);
        const auto native_nodes=producer->native_nodes(instance);
        // No prepared pose exists for a newly hidden/unpublished draw. Never
        // pair a recomputed pose with last Frame's cached mathematical worlds.
        if (world_nodes.empty() || native_nodes.empty()) continue;
        IsleGameObjectMetadata metadata{};
        require(isle_game_object_metadata(player->game,object,&metadata)==ISLE_GAME_OK,
                "sign Frame actual object visibility unavailable");
        test::SignGeometry::BillboardView camera_view;
        const test::SignGeometry::BillboardView* billboard=nullptr;
        // The admitted six-sign catalog has Source28 roots except smoke's
        // exact18+44 root. Read its prepared records, never compute matrices.
        if (native_nodes.front().matrix_kind==ISLE_GAME_MODEL_TRANSLATE) {
            require(isle_game_camera_peek_matrix(player->game,player->camera,0,&camera_view.perspective)==ISLE_GAME_OK &&
                    isle_game_camera_peek_matrix(player->game,player->camera,1,&camera_view.view)==ISLE_GAME_OK &&
                    isle_game_model_placement(player->game,model,&camera_view.placement)==ISLE_GAME_OK,
                    "sign Frame prepared billboard observations unavailable");
            billboard=&camera_view;
        }
        const auto geometry=geometry_.observe(*resources,*initialization,world_nodes,native_nodes,
                                             !(metadata.flags&1u) && !(source->flags&0x100u),billboard);
        facts_.geometry_rejection=static_cast<unsigned>(geometry.rejection);
        if (geometry.status==test::SignGeometry::Status::rejected)
            throw std::runtime_error("actual sign geometry rejected reason="+
                std::to_string(facts_.geometry_rejection)+" node="+std::to_string(geometry.node)+
                " command="+std::to_string(geometry.command_identity));
        if (geometry.status!=test::SignGeometry::Status::ready) continue;
        const auto origin=block->resource->metadata.world_position;
        geometry_global_={geometry.target.x+100*origin.x,geometry.target.y+100*origin.y,
                          geometry.target.z+100*origin.z};
        geometry_entry_=view.entry->serial;
        facts_.geometry_frame=ordinal; facts_.geometry_kind=static_cast<unsigned>(geometry.kind);
        facts_.geometry_ready=true;
    }
}
void SignPuzzles::retire() noexcept {
    geometry_.reset(); geometry_entry_=0; facts_.geometry_ready=false;
}
bool SignPuzzles::update(const application::RuntimeView& view,const game::State& state,std::uint64_t updates) {
    if (sampled_ && updates==last_update_) return facts_.owns_pad;
    require(!sampled_ || updates>last_update_,"sign solver update ordinal regressed");
    sampled_=true; last_update_=updates; facts_.owns_pad=false;
    facts_.dependency=SignDependency::course_not_active;
    if (index_<0) return false;
    if (reported_) { facts_.dependency=SignDependency::already_reported; return false; }
    const IslePlayerSource* player=nullptr;
    const course::NativeHost* host=nullptr;
    bool active=false;
    switch (facts_.course) {
    case 0:
        if (view.beach && view.beach->status()==beach::LifecycleStatus::running) {
            player=view.beach->source().player; active=player!=nullptr;
        }
        break;
    case 1:active=course_entry(view.tunnel,player,host);break;
    case 2:active=course_entry(view.volcano,player,host);break;
    case 3:active=course_entry(view.river,player,host);break;
    case 4:active=course_entry(view.cave,player,host);break;
    case 5:active=course_entry(view.valley,player,host);break;
    }
    if (!active) return false;
    require(state.save() && player->photo,"sign solver actual profile/photo owner absent");
    const auto ability=[&](unsigned flag) {
        const auto value=state.save()->player_flag(flag);
        require(bool(value),"sign solver nonsealing ability read rejected"); return value.value;
    };
    facts_.apple=ability(0); facts_.pester=ability(1); facts_.flute=ability(2); facts_.dash=ability(5);
    const auto& roll=*player->photo;
    require(roll.gPhotoCount>=0 && roll.gPhotoCount<=60,"sign solver actual roll count outside Source extent");
    facts_.photo_count=roll.gPhotoCount;
    if (!baseline_) { baseline_=true; facts_.initial_photo_count=roll.gPhotoCount; }
    require(roll.gPhotoCount>=facts_.initial_photo_count,"sign solver crossed a trip/roll reset");
    if (pending_==Press::shutter && roll.gPhotoCount>requested_photo_count_) {
        require(roll.gPhotoCount==requested_photo_count_+1,"sign solver ambiguous actual shutter slot");
        const auto& record=roll.D_800B0598[requested_photo_count_];
        const int recorded=primary(record);
        if (record.levelID==facts_.course && recorded==facts_.id && requested_photo_count_>=facts_.initial_photo_count) {
            facts_.capture_slot=requested_photo_count_; facts_.recorded_primary=recorded;
            facts_.capture_observed=true;
        }
    }
    if (pending_!=Press::none) {
        const std::uint16_t button=pending_==Press::pester?b_button:pending_==Press::flute?flute_button:a_button;
        if (player->input.current_buttons&button) {
            if (pending_==Press::shutter) ++facts_.shutters_consumed;
            else if ((pending_==Press::flute && player->LastItemId==161 && player->PressPokeFluteTimeout>0) ||
                     (pending_!=Press::flute && player->ThrowItemTimeout>0 &&
                      player->LastItemId==(pending_==Press::apple?163:162))) ++facts_.items_consumed;
            pending_=Press::none; release_button_=button;
        } else {
            // A disabled/paused Source cannot consume the request. Do not keep
            // an item/shutter asserted across that state or pretend it fired.
            pending_=Press::none;
        }
    }
    if (release_button_) {
        if (player->input.current_buttons&release_button_) return own(SignDependency::physical_release);
        release_button_=0;
    }
    if (facts_.capture_observed) { retire(); facts_.dependency=SignDependency::none; return false; }
    facts_.player_ready=player->D_80382D04_523114==1 && !player->IsInputDisabled && !player->IsPaused;
    if (!facts_.player_ready) { facts_.dependency=SignDependency::player_not_ready; return false; }
    if (!facts_.dash) { facts_.dependency=SignDependency::dash_reward; return false; }
    if (roll.gPhotoCount>=59) { facts_.dependency=SignDependency::film_reserve; return false; }
    IslePlayerPhotoFocus focus{};
    require(isle_player_photo_published_focus(player->photo,&focus)==ISLE_GAME_OK,
            "sign solver published real detector focus unavailable");
    const auto observed=observe(*player,facts_.id,focus);
    const auto& sign=observed.sign;
    const auto& producer=observed.producer;
    facts_.sign_present=sign.present; facts_.sign_position=sign.collision;
    facts_.sign_flags=sign.flags; facts_.sign_graph=sign.graph;
    facts_.sign_capture_flag=sign.present && (sign.flags&4u);
    facts_.producer_present=producer.present; facts_.producer_revealed=producer.revealed;
    facts_.producer_id=producer.id; facts_.producer_position=producer.position;
    facts_.producer_graph=producer.graph; facts_.producer_player_distance=producer.player_distance;
    facts_.producer_state=producer.present?SignProducerState::waiting:SignProducerState::absent;
    if (host && host->items_owner()) facts_.flute_playing=host->items_owner()->flute().command()!=0;
    if (facts_.id==1010) {
        require(view.tunnel,"sign solver Tunnel observation lost");
        const auto power=view.tunnel->actors_source().D_803430E8_6401B8;
        facts_.powered=(power&0x80000000u)!=0;
        facts_.egg_hatching=(power&0x40000000u)!=0;
        facts_.pikachu_egg_distance=producer.present && observed.egg.present?
            distance(producer.position,observed.egg.position):0;
        if (facts_.powered) facts_.producer_state=SignProducerState::powered;
        else if (observed.zapdos.flying) facts_.producer_state=SignProducerState::zapdos_flight;
        else if (facts_.egg_hatching || observed.egg.hatching) facts_.producer_state=SignProducerState::egg_hatching;
        else if (producer.charging) facts_.producer_state=SignProducerState::flute_charge;
        else if (producer.pursuing) facts_.producer_state=SignProducerState::apple_pursuit;
        if (!facts_.powered && !facts_.egg_hatching && !observed.zapdos.flying) {
            if (!facts_.flute) { facts_.dependency=SignDependency::flute_reward; return false; }
            if (!producer.present || !observed.egg.present || !observed.zapdos.present) {
                facts_.dependency=SignDependency::producer_not_present; return false;
            }
            if (!producer.revealed || !observed.egg.revealed) {
                facts_.dependency=SignDependency::producer_not_revealed; return false;
            }
            if (producer.charging) return play_flute(*player);
            if (facts_.pikachu_egg_distance<600) {
                if (producer.player_distance>=1400) { facts_.dependency=SignDependency::flute_out_of_range; return false; }
                return play_flute(*player);
            }
            if (!facts_.apple) { facts_.dependency=SignDependency::apple_reward; return false; }
            if (producer.pursuing) return own(SignDependency::pikachu_egg_separation);
            const auto* world=host?host->world_owner():nullptr;
            if (!world) { facts_.dependency=SignDependency::lure_ground; return false; }
            // A real grounded Apple within Pikachu's600-unit acquisition radius
            // draws it toward the actual egg; reobserve every walk before a new
            // Apple. Never hit Pikachu directly or teleport its egg/target.
            const float dx=observed.egg.position.x-producer.position.x;
            const float dz=observed.egg.position.z-producer.position.z;
            const float length=std::hypot(dx,dz);
            if (!(length>0)) { facts_.dependency=SignDependency::lure_ground; return false; }
            const float step=length<450?length:450;
            IsleGameVec3 target{producer.position.x+dx*step/length,0,producer.position.z+dz*step/length};
            world::GroundResult ground{};
            if (!world->ground_at(target.x,target.z,ground) || ground.surface<0 ||
                ground.surface==0xff4c19 || ground.surface==0x00ff00 || ground.surface==0x0019ff) {
                facts_.dependency=SignDependency::lure_ground; return false;
            }
            target.y=ground.height;
            if (distance(target,producer.position)>=600) { facts_.dependency=SignDependency::lure_ground; return false; }
            return throw_item(*player,target,Press::apple);
        }
        if (!facts_.powered) return own(SignDependency::producer_not_ready);
    }
    if (facts_.id==1022) {
        require(view.river,"sign solver River observation lost");
        facts_.smoke_cleared=view.river->actors_source().D_802E3DA0_6CBB80!=0;
        if (producer.producing) facts_.producer_state=SignProducerState::vileplume_reveal;
        if (!facts_.smoke_cleared) {
            if (!facts_.flute) { facts_.dependency=SignDependency::flute_reward; return false; }
            if (!producer.present) { facts_.dependency=SignDependency::producer_not_present; return false; }
            if(!facts_.flute_playing)return play_flute(*player);
            if(zoom(*player,false))return true;
            // Source directions[3]=PI/2; player.c474 therefore steers cpTime
            // toward1. The far lane0 cannot hear this flower before the sign
            // window closes. Real C-left turns, never a movement-state write.
            if(player->TurnToDirSpeed!=0)return own(SignDependency::camera_turning);
            constexpr std::uint16_t c_left=0x0002;
            if(player->gDirectionIndex!=3) {
                if(player->input.current_buttons&c_left)return own(SignDependency::physical_release);
                return own(SignDependency::camera_turning,c_left);
            }
            return own(SignDependency::none);
        }
    }
    if (facts_.id==1028 && !sign.present) {
        if (!facts_.pester) { facts_.dependency=SignDependency::pester_reward; return false; }
        if (!producer.present) { facts_.dependency=SignDependency::producer_not_present; return false; }
        if (producer.producing) {
            facts_.producer_state=SignProducerState::smoke_spawn;
            return own(SignDependency::producer_not_ready);
        }
        const auto* world=host?host->world_owner():nullptr;
        const auto* block=world?world->current():nullptr;
        if (!block || !block->resource) { facts_.dependency=SignDependency::volcano_lava_surface; return false; }
        const auto origin=block->resource->metadata.world_position;
        IsleGameVec3 target{};
        if (!lava_cached_) {
            lava_cached_=lava_target(*world,*player,producer.position,target);
            if (lava_cached_) global_lava_={target.x+100*origin.x,target.y+100*origin.y,target.z+100*origin.z};
        } else target={global_lava_.x-100*origin.x,global_lava_.y-100*origin.y,global_lava_.z-100*origin.z};
        world::GroundResult ground{};
        facts_.trigger_surface=lava_cached_ && world->ground_at(target.x,target.z,ground) && ground.surface==0xff4c19;
        if (!facts_.trigger_surface) { facts_.dependency=SignDependency::volcano_lava_surface; return false; }
        return throw_item(*player,target,Press::pester);
    }
    if (!sign.present) { facts_.dependency=SignDependency::sign_not_present; return false; }
    if (sign.face) facts_.producer_state=SignProducerState::smoke_face;
    // For smoke, acquire during rise but only release a shutter in the actual
    // face animation. Static signs may be pre-aimed before their FLAG4 window.
    if (zoom(*player,true)) return true;
    const std::uint16_t hold_zoom=player->ZoomSwitchMode==0?z_button:0;
    // Source visibility is tested against collPosition before the Producer
    // prepares any render pose. Acquire that real culling window first; this
    // point never authorizes a shutter or substitutes for mesh framing.
    if (sign.flags&0x100u) {
        std::int8_t x=0,y=0;
        aim_.point(aim_.owner,*player,sign.collision,true,x,y);
        facts_.input_target=sign.collision;
        return own(SignDependency::source_visibility_window,hold_zoom,x,y);
    }
    const auto* target_world=host?host->world_owner():(view.beach?view.beach->world_owner():nullptr);
    const auto* target_block=target_world?target_world->current():nullptr;
    if (!facts_.geometry_ready || !view.entry || geometry_entry_!=view.entry->serial ||
        !target_block || !target_block->resource)
        return own(SignDependency::geometry_not_ready,hold_zoom);
    const auto origin=target_block->resource->metadata.world_position;
    const IsleGameVec3 target{geometry_global_.x-100*origin.x,geometry_global_.y-100*origin.y,
                             geometry_global_.z-100*origin.z};
    std::int8_t x=0,y=0;
    aim_.point(aim_.owner,*player,target,true,x,y);
    facts_.input_target=target;
    const bool special=facts_.id!=1010 && facts_.id!=1028;
    facts_.focus_authorized=sign.revealed && sign.focus && (special || sign.member) &&
                            facts_.sign_capture_flag && (facts_.id!=1028 || sign.face);
    if (!facts_.sign_capture_flag || (facts_.id==1028 && !sign.face))
        return own(SignDependency::source_visibility_window,hold_zoom,x,y);
    // Actual detector authorization, not an exactly-zero quantized stick,
    // admits the shutter. Small aim corrections may remain in the dead zone.
    if (!facts_.focus_authorized) return own(SignDependency::focus_membership,hold_zoom,x,y);
    if (player->sTimerAfterPhotoTaken>=0) return own(SignDependency::shutter_cooldown,hold_zoom,x,y);
    if (player->input.current_buttons&a_button) return own(SignDependency::physical_release,hold_zoom,x,y);
    requested_photo_count_=roll.gPhotoCount; pending_=Press::shutter; ++facts_.shutter_requests;
    return own(SignDependency::none,hold_zoom|a_button,x,y);
}
} // namespace isle::private_campaign

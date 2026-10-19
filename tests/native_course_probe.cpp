#include "native_course_probe.hpp"
#include "application_runtime.hpp"
#include "game_state.hpp"
#include "tunnel_scene.hpp"
#include "volcano_scene.hpp"
#include "river_scene.hpp"
#include "cave_scene.hpp"
#include "valley_scene.hpp"
#include "player_photo_source.h"
#include "player_ui_source.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

namespace isle::private_course {
namespace {
constexpr std::uint16_t a=0x8000,b=0x4000,z=0x2000;
constexpr float pi=3.14159265358979323846f;
// Original world IDs100..126 admit at most27 primary blocks. Player's minimum
// undashed speed is .0005*(1-.25), i.e.2667 updates/block. Add independent intro
// and ending envelopes (1200 each). This is a FAILURE bound, never a success.
constexpr std::uint64_t travel_bound=27*2667+2*1200;
// Mew's actual7200+1800 waits, gate10000/5 traversal, Cart1200 and finish120.
constexpr std::uint64_t rainbow_bound=7200+1800+2000+1200+120+1200;
constexpr std::uint64_t ledger_bound=131072;
void require(bool yes,const char* message) {if(!yes)throw std::runtime_error(message);}
void accepted(save::GameError error,const char* message) {require(!error,message);}

struct Sample {
    const IslePlayerSource* player=nullptr;
    std::int32_t input=-1,output=-1,reason=0,status=0;
    bool running=false,retired=false,joined=false,source_complete=false,closed=false;
    bool intro=false,intro_done=false,ending=false,ending_wait=false,ending_terminal=false,frame=false;
    int hits1=3,hits2=3,mew_mode=0,gate=0,gate_terminal=0;
};
Sample sample(const beach::Lifecycle& owner) {
    const auto& s=owner.source();Sample v;
    v.player=s.player;v.input=s.input_scene;v.output=s.output_scene;v.reason=s.D_beach_802CC018;v.status=s.status;
    v.running=owner.status()==beach::LifecycleStatus::running;
    v.retired=owner.status()==beach::LifecycleStatus::retired;
    v.source_complete=s.completed && !s.running && s.scene_requested && s.scene_completion_ready && s.scene_completion_status==ISLE_GAME_OK;
    v.joined=owner.result().has_value();
    v.closed=!s.domain && !s.player && !s.music && !s.D_beach_802CC0E4 && !owner.ending_source() &&
        !owner.graphics_busy() && !owner.completed_frame() && !owner.pending_setup();
    v.intro=s.D_beach_802CC0E4 && !s.D_beach_802CC0E0;v.intro_done=s.D_beach_802CC0E0==1;
    if(const auto* ending=owner.ending_source()) {
        v.ending=ending->D_beach_80347658!=nullptr;
        v.ending_wait=v.ending && ending->D_beach_802CD9D4==0;
        v.ending_terminal=v.ending && ending->D_beach_802CD9D4==1;
    }
    v.frame=bool(owner.completed_frame());
    require(owner.error()==ISLE_GAME_OK,owner.message());return v;
}
Sample sample(const rainbow::Lifecycle& owner) {
    const auto& s=owner.source();const auto& actors=owner.actors_source();Sample v;
    v.player=s.player;v.input=s.input_scene;v.output=s.output_scene;v.reason=s.rainbow_endLevelReason;v.status=s.status;
    v.running=owner.status()==rainbow::LifecycleStatus::running;
    v.retired=owner.status()==rainbow::LifecycleStatus::retired;
    v.source_complete=s.completed && !s.running && s.scene_requested && s.scene_completion_ready && s.scene_completion_status==ISLE_GAME_OK;
    v.joined=owner.result().has_value();
    v.closed=!s.domain && !s.player && !s.D_8034B674_82ADE4 && !owner.graphics_busy() && !owner.completed_frame() && !owner.pending_setup() &&
        actors.phase==ISLE_RAINBOW_ACTOR_DETACHED && actors.status==ISLE_GAME_OK && !actors.nativeGame && !actors.services &&
        !actors.D_8034AB94_82A304 && !actors.D_8034AB98_82A308 && !actors.D_8034AF2C_82A69C &&
        !actors.mew_model.object && !actors.mew_model.model && !actors.mew_model.incarnation &&
        !actors.shield_model.object && !actors.shield_model.model && !actors.shield_model.incarnation &&
        !actors.gate_model.object && !actors.gate_model.model && !actors.gate_model.incarnation && !actors.D_8034AF20_82A690.native;
    v.intro=s.D_8034B674_82ADE4 && !s.D_8034B670_82ADE0;v.intro_done=s.D_8034B670_82ADE0==1;
    v.hits1=actors.D_8034AE48_82A5B8;v.hits2=actors.D_8034AE4C_82A5BC;v.mew_mode=actors.D_80350198_82F908;
    v.gate=actors.D_8034AF38_82A6A8;v.gate_terminal=actors.D_8034AF34_82A6A4;
    v.ending=v.gate!=0;v.ending_wait=v.ending && !v.gate_terminal;v.ending_terminal=v.ending && v.gate_terminal;
    v.frame=bool(owner.completed_frame());require(owner.error()==ISLE_GAME_OK,owner.message());return v;
}
template<class Adapter> Sample sample(const course::Lifecycle<Adapter>& owner) {
    const auto& s=owner.source();const auto& adapter=owner.adapter();const auto& actors=owner.actors_source();Sample v;
    v.player=adapter.native.player();v.input=s.input_scene;v.output=s.output_scene;v.status=s.status;
    v.running=owner.status()==course::LifecycleStatus::running;v.retired=owner.status()==course::LifecycleStatus::retired;
    v.source_complete=adapter.completed() && adapter.completion_ready();
    v.joined=owner.scene_join_receipt().joined() && owner.entry_joined() && owner.result().has_value();
    v.closed=!s.domain && !owner.native_host() && !owner.graphics_busy() && !owner.completed_frame() && !owner.pending_setup() &&
        actors.base.phase==ISLE_COURSE_ACTOR_DETACHED && actors.base.status==ISLE_GAME_OK && !actors.base.nativeGame && !actors.services &&
        std::all_of(adapter.reference_fields.begin(),adapter.reference_fields.end(),[](const auto* field){return !*field;});
    if constexpr(std::is_same_v<Adapter,tunnel::Adapter>) {
        v.reason=s.tunnel_EndLevelReason;v.intro=s.D_802F01E4_5ED2B4 && !s.D_802F01E0_5ED2B0;
        v.intro_done=s.D_802F01E0_5ED2B0==1;
    } else if constexpr(std::is_same_v<Adapter,volcano::Adapter>) {
        v.reason=actors.volcano_EndLevelReason;v.intro_done=actors.volcano_IntroEnded==1;
    } else if constexpr(std::is_same_v<Adapter,river::Adapter>) v.reason=actors.D_802E28B4_6CA694;
    else if constexpr(std::is_same_v<Adapter,cave::Adapter>) v.reason=actors.D_802C6378_648828;
    else if constexpr(std::is_same_v<Adapter,valley::Adapter>) v.reason=actors.D_802D297C_7ABF0C;
    // For unnamed intro words, original Root's show-controls process publishes
    // readiness ONLY after the course's genuine introductory Cart has joined.
    if(v.player) {
        v.intro|=!v.player->D_80382D04_523114;
        v.intro_done|=v.player->D_80382D04_523114==1;
        v.ending=v.player->D_80382D9C_5231AC!=0 && !v.player->IsCartStopped;
    }
    v.frame=bool(owner.completed_frame());require(owner.error()==ISLE_GAME_OK,owner.message());return v;
}
template<class View> bool selected(const View& view,std::int32_t course,Sample& out) {
    switch(course) {
    case 0:if(view.beach){out=sample(*view.beach);return true;}break;
    case 1:if(view.tunnel){out=sample(*view.tunnel);return true;}break;
    case 2:if(view.volcano){out=sample(*view.volcano);return true;}break;
    case 3:if(view.river){out=sample(*view.river);return true;}break;
    case 4:if(view.cave){out=sample(*view.cave);return true;}break;
    case 5:if(view.valley){out=sample(*view.valley);return true;}break;
    case 6:if(view.rainbow){out=sample(*view.rainbow);return true;}break;
    }
    return false;
}
bool followup(const IslePlayerUiSource* ui) {
    if(!ui || ui->Msg_MessageLength<=0 || std::size_t(ui->Msg_MessageLength)>=sizeof(ui->Msg_PrintedText))return false;
    const auto n=std::size_t(ui->Msg_MessageLength);
    if(ui->Msg_PrintDelay || ui->Msg_MessageText[n] || ui->Msg_PrintedText[n] ||
       !std::equal(ui->Msg_MessageText,ui->Msg_MessageText+n,ui->Msg_PrintedText))return false;
    const std::string_view text(reinterpret_cast<const char*>(ui->Msg_MessageText),n);
    return text.starts_with("Try to take a lot of Pok") || text=="Please use the Control Stick.";
}
float wrapped(float angle) {return std::remainder(angle,2*pi);}
std::int8_t stick(float value) {
    if(std::abs(value)<6)return 0;
    return static_cast<std::int8_t>(std::clamp(value,-80.0f,80.0f));
}
}

void seed_progress(game::State& state,std::int32_t course) {
    require(course>=0 && course<7,"authored course profile argument outside0..6");
    auto* session=state.save();require(state.initialized() && session,"real Name must precede authored unlocked profile");
    const auto before=session->source_player_name_prefix();require(before.size()>=16,"actual confirmed Name missing");
    std::array<std::byte,16> name{};std::copy_n(before.begin(),16,name.begin());
    require(std::any_of(name.begin(),name.end(),[](auto byte){return byte!=std::byte{};}),"authored profile cannot replace missing real Name confirmation");
    // Source6's known-course and unlocked-course must agree: otherwise its
    // new-course dialogue overwrites the selected field during Oak startup.
    accepted(session->set_selected_course(6),"authored known-course mutation rejected");
    accepted(session->set_unlocked_course(6),"authored unlock mutation rejected");
    for(unsigned flag=0;flag<3;++flag)accepted(session->set_player_flag(flag,true),"authored equipment mutation rejected");
    accepted(session->set_player_flag(12,false),"authored hold-Z control mutation rejected");
    accepted(session->set_player_flag(13,false),"authored ordinary Y-axis mutation rejected");
    const auto after=session->source_player_name_prefix();
    require(after.size()>=16 && std::equal(name.begin(),name.end(),after.begin()),"authored progress altered confirmed Name");
}

struct Probe::Impl {
    std::int32_t course;
    private_post_course::Controller controller;
    std::ofstream ledger;
    std::uint64_t rows=0,serial=0,updates=0,last_external=0,frames=0,presents=0,varied=0,movement=0,shutters=0,throws=0;
    std::uint64_t travel_presentations=0;
    int photos=0,qualified=0,reason=0,focus_id=0,hits1=3,hits2=3,mew_mode=0;
    int pending_mew_slot=-1,pending_mew_qualified=0;
    int controls=0,disabled=0,direction=0,zoom_target=0,cooldown=0;
    std::uint32_t consumed_buttons=0,pressed_buttons=0;
    float move_time=0;
    std::uint16_t buttons=0;
    std::int8_t x=0,y=0;
    IsleGameVec3 previous{};
    IsleGameVec3 previous_shield{};
    std::uint64_t previous_shield_update=0;
    bool have_shield=false;
    bool entered=false,have_position=false,intro_seen=false,intro_done=false,travel_seen=false;
    bool ending_seen=false,ending_wait=false,ending_terminal=false,retirement=false,post=false,surface=false;
    bool tutorial_followup=false,tutorial_clear=false,tutorial_stick=false,unshielded_photo=false;
    bool mew_shielded_seen=false,mew_progress_seen=false,mew_shields_broken=false;
    int mew_initial_hits1=0,mew_initial_hits2=0;
    struct FrameTag {std::uint64_t ordinal=0,serial=0,update=0;int photos=0,reason=0;unsigned phase=0;};
    std::array<FrameTag,2> tags{};
    explicit Impl(std::int32_t id,private_post_course::Controller pad,const std::filesystem::path& root):course(id),controller(pad) {
        require(course>=0 && course<7 && controller.owner && controller.set,"actual course and owned controller required");
        std::filesystem::create_directories(root);
        ledger.exceptions(std::ios::badbit|std::ios::failbit);
        ledger.open(root/("course-"+std::to_string(course)+"-source.tsv"),std::ios::out|std::ios::trunc);
        row("scope\tauthored unlocked canonical profile; actual Runtime/Manager course Source and owned inputs; not natural save progression or N64 parity");
        row("bounds\t27 primary world IDs *2667 minimum-speed updates +2400 intro/end; Rainbow7200+1800+2000+1200+120+1200;131072 rows;2 scalar frame tags;no images");
        row("kind\tcourse\tserial\tupdates\tframes\tpresents\tphotos\tqualified\tend_reason\tintro_done\tending\tbuttons\tx\ty\tfocus\thits1\thits2\tmew_mode\tmovement\tvalue\tcontrols\tdisabled\tdirection\tzoom_target\tcooldown\tconsumed_buttons\tpressed_buttons\tmove_time\tposition_x\tposition_y\tposition_z");
    }
    void set(std::uint16_t pad=0,std::int8_t sx=0,std::int8_t sy=0) noexcept {
        buttons=pad;x=sx;y=sy;controller.set(controller.owner,pad,sx,sy);
    }
    void row(const std::string& text) {
        require(++rows<=ledger_bound && text.size()<1024,"bounded actual course ledger exhausted");ledger<<text<<'\n';
    }
    void note(const char* kind,std::uint64_t value=0) {
        std::ostringstream text;text<<kind<<'\t'<<course<<'\t'<<serial<<'\t'<<updates<<'\t'<<frames<<'\t'<<presents
            <<'\t'<<photos<<'\t'<<qualified<<'\t'<<reason<<'\t'<<intro_done<<'\t'<<ending_seen
            <<'\t'<<buttons<<'\t'<<int(x)<<'\t'<<int(y)<<'\t'<<focus_id<<'\t'<<hits1<<'\t'<<hits2
            <<'\t'<<mew_mode<<'\t'<<movement<<'\t'<<value<<'\t'<<controls<<'\t'<<disabled<<'\t'<<direction
            <<'\t'<<zoom_target<<'\t'<<cooldown<<'\t'<<consumed_buttons<<'\t'<<pressed_buttons<<'\t'<<move_time
            <<'\t'<<previous.x<<'\t'<<previous.y<<'\t'<<previous.z;row(text.str());
    }
    [[noreturn]] void fail(const char* why) {
        note("failure");ledger.flush();set();
        std::ostringstream text;text<<why<<" course="<<course<<" serial="<<serial<<" updates="<<updates
            <<" photos="<<photos<<" qualified="<<qualified<<" reason="<<reason<<" focus="<<focus_id
            <<" hits="<<hits1<<','<<hits2<<" mew_mode="<<mew_mode<<" intro="<<intro_done<<" ending="<<ending_seen
            <<" controls="<<controls<<" disabled="<<disabled<<" direction="<<direction<<" zoom_target="<<zoom_target
            <<" cooldown="<<cooldown<<" consumed_buttons="<<consumed_buttons<<" move_time="<<move_time;
        throw std::runtime_error(text.str());
    }
    void observe(const Sample& s) {
        require(s.status==ISLE_GAME_OK,"actual course Source error; no normal completion receipt");
        reason=s.reason;hits1=s.hits1;hits2=s.hits2;mew_mode=s.mew_mode;
        intro_seen|=s.intro;intro_done|=s.intro_done;ending_seen|=s.ending;ending_wait|=s.ending_wait;
        ending_terminal|=ending_wait && s.ending_terminal;
        if(!s.player)return;
        const auto& player=*s.player;
        controls=player.D_80382D04_523114;disabled=player.IsInputDisabled;direction=player.gDirectionIndex;
        zoom_target=player.TargetDirectionZoomedIn;cooldown=player.sTimerAfterPhotoTaken;
        consumed_buttons=player.input.current_buttons;pressed_buttons=player.input.pressed_buttons;move_time=player.movement.move_time;
        if(course==6 && s.running && s.intro_done && controls) {
            if(!mew_shielded_seen && hits1>0 && hits2>0) {
                mew_initial_hits1=hits1;mew_initial_hits2=hits2;mew_shielded_seen=true;
            }
            if(mew_shielded_seen) {
                mew_progress_seen|=hits1<mew_initial_hits1 || hits2<mew_initial_hits2;
                mew_shields_broken|=hits1<=0 && hits2<=0;
            }
        }
        require(player.status==ISLE_GAME_OK && player.photo && player.photo->status==ISLE_GAME_OK,"canonical actual Player/Photo Source failed");
        photos=player.photo->gPhotoCount;qualified=player.photo->D_800AE27C;
        require(photos>=qualified && qualified>=0 && photos<=60,"actual captured roll counters invalid");
        if(pending_mew_slot>=0 && photos>pending_mew_slot && qualified>pending_mew_qualified) {
            const auto& captured=player.photo->D_800B0598[pending_mew_slot];
            unshielded_photo|=captured.levelID==6 &&
                std::any_of(std::begin(captured.pokemons),std::end(captured.pokemons),
                    [](const auto& subject){return subject.pokemonID==151;});
            pending_mew_slot=-1;
        }
        const auto position=player.movement.position;
        if(have_position && (position.x!=previous.x || position.y!=previous.y || position.z!=previous.z))++movement;
        previous=position;have_position=true;
        // Rainbow's original Player position is stationary. Its real shield
        // progression, not invented cart movement, proves playable travel.
        travel_seen|=s.running && intro_done && player.D_80382D04_523114 &&
            (course==6?mew_progress_seen:movement>0);
        const bool message=followup(player.ui);
        tutorial_followup|=message;tutorial_clear|=tutorial_followup && !message;
        tutorial_stick|=tutorial_clear && (std::abs(player.input.stick_x)>=6 || std::abs(player.input.stick_y)>=6);
    }
    // Read only the actual retained model placement. This never acquires a
    // semantic reference, recomputes matrices, writes pose or caches an object.
    bool target(const IsleRainbowActorSource& actors,IsleGameVec3& position,bool shield) {
        const auto& reference=shield?actors.shield_model:actors.mew_model;
        if(!reference.object)return false;
        IsleActorSource* actor=nullptr;
        require(isle_actor_source_get_nullable(actors.nativeGame,reference.object,&actor)==ISLE_GAME_OK,"actual Rainbow actor borrow rejected");
        if(!actor || !actor->tangible)return false;
        IsleGameModel* model=nullptr;const IsleGameModelPlacement* placement=nullptr;
        require(isle_game_model_resolve(actors.nativeGame,&reference,&model)==ISLE_GAME_OK && model &&
            isle_game_model_placement(actors.nativeGame,model,&placement)==ISLE_GAME_OK && placement &&
            placement->mode!=ISLE_GAME_MODEL_PLACEMENT_DISABLED,"actual Rainbow model placement borrow rejected");
        position=placement->transform.position;return true;
    }
    void aim(const IslePlayerSource& player,const IsleGameVec3& target,bool zoomed,std::int8_t& sx,std::int8_t& sy) {
        if(zoomed) {
            const float dx=target.x-player.CameraEyePos.x,dz=target.z-player.CameraEyePos.z;
            const float sn=std::sin(-player.movement.rotation.y),cs=std::cos(-player.movement.rotation.y);
            const float local_x=dx*cs+dz*sn,local_z=dz*cs-dx*sn;
            const float yaw=std::atan2(-local_x,local_z);
            const float pitch=std::atan2(target.y-player.CameraEyePos.y,std::hypot(dx,dz));
            sx=stick(wrapped(yaw-player.PlayerViewYaw)*80/.025f);
            sy=stick((player.ViewPitch-pitch)*80/.015f);return;
        }
        const IsleGameCameraParameters* camera=nullptr;
        require(isle_game_camera_parameters(player.game,player.camera,&camera)==ISLE_GAME_OK && camera,"actual Rainbow camera parameters rejected");
        const auto& look=camera->look_at;
        float fx=look.at[0]-look.eye[0],fy=look.at[1]-look.eye[1],fz=look.at[2]-look.eye[2];
        const float norm=std::sqrt(fx*fx+fy*fy+fz*fz);require(norm>0,"actual course camera direction degenerate");fx/=norm;fy/=norm;fz/=norm;
        const float right_norm=std::hypot(fx,fz);require(right_norm>0,"actual course camera vertical");
        const float rx=-fz/right_norm,rz=fx/right_norm;
        const float ux=-fy*rz,uy=rz*fx-rx*fz,uz=fy*rx;
        const float dx=target.x-look.eye[0],dy=target.y-look.eye[1],dz=target.z-look.eye[2];
        const float depth=dx*fx+dy*fy+dz*fz;
        const float side=dx*rx+dz*rz,up=dx*ux+dy*uy+dz*uz;
        const float scale=std::tan(camera->perspective.fovy*pi/360);
        const float px=depth>1?side/depth/scale/camera->perspective.aspect*160:(side>=0?320.0f:-320.0f);
        const float py=depth>1?-up/depth/scale*120:0;
        const float error_x=px-player.ReticleScreenX,error_y=py-player.ReticleScreenY;
        sx=std::abs(error_x)<6?0:stick(error_x*(player.gDirectionIndex==0?-16.0f:16.0f));
        sy=std::abs(error_y)<6?0:stick(error_y*8);
    }
    void control(const application::RuntimeView& view,const Sample& s) {
        const auto prior=buttons;set();
        if(!s.running || !s.player || !s.intro_done || !s.player->D_80382D04_523114 || s.ending)return;
        const auto& player=*s.player;
        IslePlayerPhotoFocus focus{};
        require(isle_player_photo_published_focus(player.photo,&focus)==ISLE_GAME_OK,"actual published course Photo focus rejected");
        focus_id=focus.pokemon_id;
        const bool subject=focus.present && focus.object && focus.pokemon_id>0 && focus.pokemon_id<=151 && (focus.flags&4u);
        if(course==6) {
            const auto& actors=view.rainbow->actors_source();
            // Shield CMD9/13 is forwarded to Mew: first3 hits break phase1,
            // next3 break phase2. B works ONLY zoomed out with timeout0. The
            // unshielded phase clears Mew's FLAG8 and permits real species151.
            const bool shielded=actors.D_8034AE48_82A5B8>0 || actors.D_8034AE4C_82A5BC>0;
            IsleGameVec3 position{};
            if(shielded) {
                if(player.IsInputDisabled || player.gDirectionIndex<0)return;
                if(target(actors,position,true)) {
                    // Original items.c launches at50 units, advances at half
                    // velocity/update and subtracts .6 vertical velocity each
                    // update. Lead the observed moving sphere and compensate
                    // that .3-position gravity plus its actual +100 ray bias.
                    const float distance=std::hypot(position.x-player.CameraEyePos.x,
                        position.y-player.CameraEyePos.y,position.z-player.CameraEyePos.z);
                    const float flight=distance/25.0f;
                    auto aimed=position;
                    if(have_shield && updates>previous_shield_update) {
                        const float delta=static_cast<float>(updates-previous_shield_update);
                        aimed.x+=(position.x-previous_shield.x)*flight/delta;
                        aimed.y+=(position.y-previous_shield.y)*flight/delta;
                        aimed.z+=(position.z-previous_shield.z)*flight/delta;
                    }
                    previous_shield=position;previous_shield_update=updates;have_shield=true;
                    const float ray=std::hypot(player.cam_target.x-player.CameraEyePos.x,
                        player.cam_target.y-player.CameraEyePos.y,player.cam_target.z-player.CameraEyePos.z);
                    if(ray>0)aimed.y+=.15f*flight*(flight+1)-100*distance/ray;
                    std::int8_t sx=0,sy=0;aim(player,aimed,false,sx,sy);
                    std::uint16_t pad=0;
                    // Actual cam_target is updated by the canonical reticle
                    // process. Its ray, not a native model write, aims the item.
                    if(!sx && !sy && player.ThrowItemTimeout==0 && !(prior&b) && !(player.input.current_buttons&b)) {pad=b;++throws;}
                    set(pad,sx,sy);
                } else have_shield=false;
                return;
            }
            if(player.IsInputDisabled)return;
            std::int8_t sx=0,sy=0;
            if(target(actors,position,false) && player.gDirectionIndex==-1)aim(player,position,true,sx,sy);
            std::uint16_t pad=z;
            if(subject && focus.pokemon_id==151 && !(focus.flags&8u) && player.gDirectionIndex==-1 &&
                !player.TargetDirectionZoomedIn && player.sTimerAfterPhotoTaken<0 && !(prior&a) && !(player.input.current_buttons&a) && photos<4) {
                pad|=a;++shutters;pending_mew_slot=photos;pending_mew_qualified=qualified;
            }
            set(pad,sx,sy);return;
        }
        if(player.IsTutorialEnabled && qualified<3) {
            if(!surface)return;
            std::uint16_t pad=z;
            if(subject && player.gDirectionIndex==-1 && !player.TargetDirectionZoomedIn && player.sTimerAfterPhotoTaken<0 &&
                !(prior&a) && !(player.input.current_buttons&a)) {pad|=a;++shutters;}
            set(pad);return;
        }
        if(player.IsTutorialEnabled && !player.D_803AE516_54E926) {
            const bool message=followup(player.ui);
            set(message && !(prior&a) && !(player.input.current_buttons&a)?a:0,tutorial_followup && !tutorial_stick?8:0);return;
        }
        if(player.IsInputDisabled)return;
        if(photos>=4)return;
        std::uint16_t pad=z;
        if(player.gDirectionIndex==-1 && !player.TargetDirectionZoomedIn && player.sTimerAfterPhotoTaken<0 &&
            !(prior&a) && !(player.input.current_buttons&a) && (subject || photos==0)) {pad|=a;++shutters;}
        // Search by actual camera heading, never transform writes. Stop once a
        // published qualifying subject exists; movement remains Source-owned.
        set(pad,!subject && photos>0?24:0);
    }
};

Probe::Probe(std::int32_t course,private_post_course::Controller controller,const std::filesystem::path& root)
    :p_(std::make_unique<Impl>(course,controller,root)) {}
Probe::~Probe()=default;
void Probe::update(const application::RuntimeView& view,std::uint64_t actualUpdates) {
    auto& p=*p_;Sample s;
    if(!selected(view,p.course,s)) {
        if(p.retirement && !p.post && view.post && view.entry) {
            require(view.entry->operation==ISLE_APPLICATION_ENTRY_CAMERA_CHECK && view.entry->argument==9,
                "natural captured course did not enter actual CameraCheck9");
            p.post=true;p.note("actual_post9",view.entry->serial);p.ledger.flush();
        }
        return;
    }
    require(view.entry && view.entry->argument==std::uint32_t(p.course),"actual Manager course argument differs from authored request");
    if(!p.entered) {
        p.set();p.entered=true;p.serial=view.entry->serial;p.last_external=actualUpdates;p.note("actual_source_enter");
    }
    require(view.entry->serial==p.serial && actualUpdates>=p.last_external,"actual course identity/update sequence changed");
    const auto delta=actualUpdates-p.last_external;
    const auto limit=p.course==6?rainbow_bound:travel_bound;
    if(delta>limit || p.updates>limit-delta)p.fail("actual Source course update failure bound exhausted");
    p.updates+=delta;p.last_external=actualUpdates;p.observe(s);
    if(delta)p.control(view,s);
    if(delta && (p.updates%8==0 || s.reason || s.ending))p.note("update");
}
void Probe::retired(application::SceneRetirementView view) {
    auto& p=*p_;Sample s;if(!selected(view,p.course,s))return;
    p.observe(s);p.set();
    if(!p.entered || p.retirement || !view.entry || view.entry->serial!=p.serial || view.entry->argument!=std::uint32_t(p.course) ||
       !s.retired || !s.joined || !s.source_complete || !s.closed || s.input!=p.course || s.output!=9 || view.source_result!=9 ||
       s.reason!=2 || !p.intro_seen || !p.intro_done || !p.travel_seen || (p.course!=6 && !p.movement) ||
       !p.photos || !p.shutters || !p.travel_presentations)
        p.fail("actual natural course/Original result9/join receipt rejected");
    if(p.course==0 && (!p.ending_wait || !p.ending_terminal))p.fail("actual Beach ending Cart handshake not observed");
    if(p.course==6 && (!p.ending_wait || !p.ending_terminal || !p.unshielded_photo || !p.mew_shields_broken || !p.throws))
        p.fail("actual Rainbow shield/Mew capture/gate Cart handshake not observed");
    p.retirement=true;p.note("joined_original_result9",view.source_result);p.ledger.flush();
}
void Probe::frame(const application::RuntimeView& view,std::uint64_t ordinal) {
    auto& p=*p_;Sample s;if(!selected(view,p.course,s))return;
    require(p.entered && view.entry && view.entry->serial==p.serial && s.frame,"actual course completed Frame provenance missing");
    auto& tag=p.tags[ordinal%p.tags.size()];
    require(!tag.serial,"actual course Frame provenance slot reused before real presentation");
    tag={ordinal,p.serial,p.updates,p.photos,p.reason,p.ending_seen?2u:p.intro_done?1u:0u};++p.frames;
}
void Probe::presented(std::int32_t course,std::uint64_t ordinal,std::uint64_t digest,bool varied) {
    auto& p=*p_;require(course==p.course,"actual course presentation argument mismatch");
    auto& tag=p.tags[ordinal%p.tags.size()];
    require(tag.ordinal==ordinal && tag.serial==p.serial,"actual course presentation lacks canonical Frame tag");
    ++p.presents;if(varied)++p.varied;
    if(varied && tag.phase==1) {
        ++p.travel_presentations;
        if(!p.surface) {
            p.surface=true;
            std::cout<<"course_api_surface_ready course="<<p.course<<" serial="<<p.serial<<" updates="<<tag.update
                <<" frame="<<ordinal<<" digest="<<digest<<" photos="<<tag.photos<<std::endl;
            p.note("course_api_surface_ready",digest);p.ledger.flush();
        }
    }
    if(p.presents%120==0 || tag.phase==2)p.note("presented",digest);
    tag={};
}
bool Probe::completed() const noexcept {return p_->retirement && p_->post;}
}

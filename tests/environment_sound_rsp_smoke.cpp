#include "environment_sound.hpp"
#include "actual_audio_driver.hpp"
#include "scene_storage.hpp"
#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace a=isle::audio;
namespace g=isle::game_audio;
namespace e=isle::environment_sound;
namespace sc=isle::scene;
namespace {
void require(bool value,const char* text) { if (!value) throw std::runtime_error(text); }
void require(a::Error value,const char* text) { require(value==a::Error::none,text); }
void native(IsleGameResult value) { require(value==ISLE_GAME_OK,"native spatial sound fixture operation rejected"); }
struct Energy {
    std::uint64_t left=0,right=0;
    void add(std::span<const std::int16_t> pcm) {
        for (std::size_t i=0;i<pcm.size();i+=2) {
            const auto l=static_cast<std::int64_t>(pcm[i]),r=static_cast<std::int64_t>(pcm[i+1]);
            left+=static_cast<std::uint64_t>(l*l); right+=static_cast<std::uint64_t>(r*r);
        }
    }
};
struct ActorBorrow {
    sc::Services* services=nullptr;
    e::Environment* environment=nullptr;
    unsigned entered=0,pre=0,joined=0;
};
void park(IsleGameObject*,void* p) noexcept {
    auto* state=static_cast<ActorBorrow*>(p);
    ++state->entered;
    for (;;) if (!state->services->wait()) return;
}
void before_retire(IsleGameObject* object,void* p) noexcept {
    auto* state=static_cast<ActorBorrow*>(p);
    ++state->pre; state->environment->fix_parameters(object);
}
void after_join(IsleGameObject*,void* p) noexcept { ++static_cast<ActorBorrow*>(p)->joined; }
IsleGameModel* model(sc::Services& services,IsleGameObject* object,float z) {
    const IsleGameModelNodeInit node{UINT32_MAX,{{},{},{1,1,1}},ISLE_GAME_MODEL_TRANSLATE};
    IsleGameModel* result=nullptr;
    native(isle_game_attach_model(services.game,object,&node,1,&result));
    const IsleGameModelTransform placement{{0,0,z},{},{1,1,1}};
    native(isle_game_model_set_placement(services.game,result,ISLE_GAME_MODEL_PLACEMENT_RTS,&placement));
    return result;
}
void angles(const e::Resources& resources) {
    struct Case { float y,x; std::uint16_t angle; };
    constexpr std::array cases{Case{0,1,0},Case{1,1,0x1000},Case{1,0,0x2000},Case{1,-1,0x3000},
        Case{0,-1,0x4000},Case{-1,-1,0x5000},Case{-1,0,0x6000},Case{-1,1,0x7000},Case{0.75f,0.5f,0},
        Case{2147483648.0f,1,0x2000},Case{-2147483648.0f,-1,0x6000},
        Case{std::numeric_limits<float>::max(),1,0x2000}};
    for (const auto& c:cases) {
        std::uint16_t value=0x1234;
        require(resources.angles().atan2s(c.y,c.x,value) && value==c.angle,"actual source angle cardinal/quadrant/quantized-zero boundary failed");
    }
    for (const float value:{std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}) {
        std::uint16_t held=0x1234;
        require(!resources.angles().atan2s(value,1,held) && held==0x1234,"invalid source angle conversion partially published output");
    }
    for (const auto& c:std::array{Case{1,2147483648.0f,0},Case{1,-2147483648.0f,0},
             Case{2147483648.0f,0.5f,0},Case{-2147483648.0f,-0.5f,0}}) {
        std::uint16_t held=0x1234;
        require(!resources.angles().atan2s(c.y,c.x,held) && held==0x1234,
                "Actually consumed source float-to-s32 overflow partially published output");
    }
}
void run(isle::rom::Snapshot& snapshot,const std::filesystem::path& profile) {
    require(std::filesystem::is_directory(profile) && std::filesystem::is_empty(profile),"explicit private profile directory must exist and be empty");
    e::Resources resources(snapshot,0); angles(resources);
    auto hydrated=g::hydrate(snapshot); require(bool(hydrated),"actual audio resource hydration failed");
    std::array<a::effects::Section,8> sections;
    a::effects::Config effects;
    require(g::native_startup_effects(hydrated.resources,sections,effects),"native source effects parameter construction failed");
    isle::private_audio::Capture capture;
    isle::private_audio::Execution execution{snapshot,nullptr,{}};
    auto made=g::Session::create(std::move(hydrated.resources),effects,capture.output(),execution.executor());
    require(bool(made),"real shared audio session creation failed");
    auto& session=*made.session;
    std::uint64_t tasks=0,tick=0;
    const auto execute=[&](const a::TaskView& task) {
        isle::private_audio::execute(session,execution,task); ++tasks;
        require(session.submit(),"actual checked PCM acceptance failed");
    };
    const auto pump=[&]() {
        ++tick;
        require(session.retrace(tick,tick*1000000000ull/60),"independent source audio retrace failed");
        require(session.submit(),"source-gated completed PCM acceptance failed");
        a::TaskView task; require(session.build(task),"source audio task build failed");
        if (task.token.serial) execute(task);
    };
    std::array<Energy,3> energy{};
    ActorBorrow lifetime{};
    {
        isle::game::State state(profile); require(state.initialize(1),"owned actual profile initialization failed");
        isle::game_input::Service input;
        sc::Storage storage(4*1024*1024); native(storage.create());
        {
            sc::Services services(storage.game(),storage.arena(),{{4,6,0,1,2},{}},input,session,state);
            require(!services.failed(),"native scene service construction failed");
            auto* player_object=services.object(1,9);
            auto* source_object=services.object(2,9);
            auto* camera_object=services.object(3,0);
            require(player_object && source_object && camera_object,"real native spatial-input objects failed");
            auto* player=model(services,player_object,0);
            auto* source=model(services,source_object,200);
            IsleGameCamera* camera=nullptr; native(isle_game_attach_camera(services.game,camera_object,&camera));
            IsleGameLookAt view{{0,0,0},{1,0,0},{0,1,0}};
            native(isle_game_camera_set_look_at(services.game,camera,&view));
            float distance=200;
            e::Environment environment(services,storage.arena(),resources);
            require(environment.play({source_object,source,&distance},1,0x34)&&
                environment.play({source_object,source,&distance},1,0x34,0,1)&&
                environment.sound_handle(0x34)==-1,
                "source sound escaped preparation before explicit environment initialization");
            require(environment.initialize(player,camera),"environment source initialization failed");
            lifetime={&services,&environment,0,0,0};
            require(services.set_lifecycle(source_object,{before_retire,after_join,&lifetime}),"real source retirement observer failed");
            require(services.process(source_object,park,&lifetime,ISLE_GAME_STACKFUL,8) &&
                    services.process(source_object,park,&lifetime,ISLE_GAME_STACKFUL,5),"real source continuation creation failed");
            // Source1006 starts actual environmental sound0x34, using Beach's
            // initialized descriptor, not a synthetic wave or playback backend.
            require(environment.play({source_object,source,&distance},1,0x34),"actual categorized source sound failed");
            int voice=environment.sound_handle(0x34);
            require(voice>=0,"actual source0x34 did not own a sound slot");
            std::int32_t alternate=-1;
            for (const auto& descriptor:resources.descriptors()) if (descriptor.sound!=0x34) { alternate=descriptor.sound; break; }
            require(alternate>=0,"Actual source table lacks the independent category subject");
            distance=100000;
            require(environment.play({source_object,source,&distance},1,alternate) &&
                    environment.sound_handle(0x34)==voice && environment.sound_handle(alternate)==-1,
                    "Out-of-hearing play replaced the existing category before its source cutoff");
            distance=200;
            require(environment.play({source_object,source,&distance},2,alternate) &&
                    environment.sound_handle(alternate)>=0 && environment.sound_handle(0x34)==voice,
                    "Independent actual source categories did not coexist");
            require(environment.stop_category(source_object,2) && environment.sound_handle(alternate)==-1 &&
                    environment.sound_handle(0x34)==voice,"Category stop also removed an unrelated source voice");
            require(environment.play({source_object,source,&distance},1,alternate) &&
                    environment.sound_handle(0x34)==-1 &&
                    session.playing_sound(static_cast<unsigned>(voice))!=0x34,
                    "Category replacement retained its previous actual native sound");
            require(environment.play({source_object,source,&distance},1,0x34) &&
                    environment.sound_handle(alternate)==-1,"Replacement-back retained a competing category voice");
            voice=environment.sound_handle(0x34);
            require(voice>=0,"Restored actual source0x34 did not own a native slot");
            view.at[2]=2147483648.0f;
            native(isle_game_camera_set_look_at(services.game,camera,&view));
            require(environment.update() && !services.failed(),
                    "Source-defined large-finite camera direction faulted environmental spatialization");
            view.at[2]=0;
            native(isle_game_camera_set_look_at(services.game,camera,&view));
            require(environment.update(),"Source sound camera restoration failed");
            a::TaskView bootstrap; require(session.build(bootstrap),"actual source bootstrap build failed");
            require(bootstrap.token.serial!=0,"source bootstrap omitted its real task"); execute(bootstrap);
            for (unsigned frame=1;frame<=56;++frame) {
                if (frame==16) { view.at[0]=-1; native(isle_game_camera_set_look_at(services.game,camera,&view)); }
                if (frame==32) {
                    require(lifetime.entered==2,"source continuations never parked");
                    services.erase(source_object);
                    require(lifetime.pre==1 && lifetime.joined==1 && environment.sound_handle(0x34)==voice,
                            "source retirement did not detach parameters while preserving its real voice");
                    IsleGameCounts counts{}; native(isle_game_counts(services.game,&counts));
                    require(counts.models==1 && counts.processes==1,"source model/continuations not actually removed before fixed-voice updates");
                }
                if (frame==40) { view.at[0]=1; native(isle_game_camera_set_look_at(services.game,camera,&view)); }
                if (!(frame&1u)) native(isle_game_tick(services.game));
                const auto begin=capture.used;
                pump();
                const auto pcm=std::span(capture.samples).subspan(begin,capture.used-begin);
                if (frame>=8 && frame<=11) energy[0].add(pcm);
                if (frame>=24 && frame<=27) energy[1].add(pcm);
                if (frame>=48 && frame<=51) energy[2].add(pcm);
                require(!services.failed(),services.error());
            }
            require(energy[0].right>16*energy[0].left,"actual source PCM did not pan right before camera reversal");
            require(energy[1].left>16*energy[1].right,"actual source PCM did not pan left after camera reversal");
            require(energy[2].left>16*energy[2].right,"fixed actual voice followed a changed camera or lost its retired source lifetime");
            require(environment.stop_category(player_object,1) && environment.sound_handle(0x34)==voice,
                    "unrelated native source stopped a detached fixed voice");
            // Reinitialization is a real new list/update publication, not a
            // cleanup or an idempotent shortcut. Detached voices keep playing.
            require(environment.initialize(nullptr,camera)&&environment.sound_handle(0x34)==-1&&
                session.playing_sound(static_cast<unsigned>(voice))==0x34,
                "reinitialization retained old list tracking or stopped its detached actual voice");
            require(environment.set_listener(player,camera),"reinitialized actual listener binding failed");
            require(environment.play({player_object,player,&distance},1,alternate)&&
                environment.sound_handle(alternate)>=0&&environment.stop_category(player_object,1)&&
                environment.sound_handle(alternate)==-1&&
                session.playing_sound(static_cast<unsigned>(voice))==0x34,
                "reinitialized source category replaced or stopped an old untracked voice");
            native(isle_game_tick(services.game));
            require(!services.failed(),services.error());
            environment.cleanup();
        }
        native(storage.destroy());
    }
    require(session.stop(),"actual audio stop request failed");
    for (unsigned n=0;n<240 && session.state()!=g::Session::State::draining;++n) pump();
    require(session.state()==g::Session::State::draining && session.service().voices_retired(),"actual categorized voice graph never retired");
    require(session.poll_drain(),"owned PCM first drain failed");
    require(session.poll_drain(),"owned PCM final drain failed");
    require(session.state()==g::Session::State::drained && !session.service().has_lease() &&
            session.accepted_frames()*2==capture.used,"actual graph/PCM ownership did not close");
    std::cout<<"environment_sound_actual_RSP_PCM=passed tasks="<<tasks<<" accepted_frames="<<session.accepted_frames()
        <<" right_energy="<<energy[0].right<<" right_opposite="<<energy[0].left
        <<" reversed_energy="<<energy[1].left<<" reversed_opposite="<<energy[1].right
        <<" fixed_energy="<<energy[2].left<<" fixed_opposite="<<energy[2].right
        <<" native_pre_join="<<lifetime.pre<<'/'<<lifetime.joined
        <<" hearing_hardware_Original_Windows11_actor_factory=not_claimed\n";
}
void run_rainbow(isle::rom::Snapshot& snapshot,const std::filesystem::path& profile) {
    require(std::filesystem::is_directory(profile) && std::filesystem::is_empty(profile),
            "explicit private profile directory must exist and be empty");
    e::Resources resources(snapshot,6); angles(resources);
    require(resources.course_id()==6 && resources.descriptors().size()==9,
            "Rainbow did not select its own nine environmental descriptors");
    const auto bytes=snapshot.canonical();
    // Pinned GAME rainbow/mew/code.c: AD3C uses {352,145}; ACEC uses
    // {350,147}. Their real root opcode16 events select categories0/1, not
    // descriptor ordinals. These are API-authored spatial inputs, not an
    // executed Mew animation/factory, earned progression or campaign proof.
    const isle::rom::GuestView hit_header{0x8034ad3c,0x82a4ac,bytes.subspan(0x82a4ac,20)};
    const isle::rom::GuestView flee_header{0x8034acec,0x82a45c,bytes.subspan(0x82a45c,20)};
    require(hit_header.u32(8)==0x8012fb60 && hit_header.u32(16)==0x8034aca8 &&
            flee_header.u32(8)==0x80133040 && flee_header.u32(16)==0x8034ac98,
            "actual Rainbow headers lost their model/sound-table identities");
    struct Selection { std::int32_t sound; std::uint8_t category; };
    const auto select=[](const isle::rom::GuestView& event,const isle::rom::GuestView& ids) {
        const auto word=event.u32(0),mask=(word>>15)&0x3ffu,index=mask&255u;
        require((word>>25)==16 && index>=1 && index<=ids.bytes.size()/4,
                "Rainbow root event does not select a real current-header sound");
        return Selection{static_cast<std::int32_t>(ids.u32((index-1)*4)),
                         static_cast<std::uint8_t>(mask>>8)};
    };
    const isle::rom::GuestView hit_ids{0x8034aca8,0x82a418,bytes.subspan(0x82a418,8)};
    const isle::rom::GuestView flee_ids{0x8034ac98,0x82a408,bytes.subspan(0x82a408,8)};
    const auto primary=select({0x8012fc50,0x4e2020,bytes.subspan(0x4e2020,4)},hit_ids);
    const auto alternate=select({0x80133114,0x4e54e4,bytes.subspan(0x4e54e4,4)},flee_ids);
    const auto independent=select({0x801330f0,0x4e54c0,bytes.subspan(0x4e54c0,4)},flee_ids);
    require(primary.sound==352 && alternate.sound==350 && independent.sound==147 &&
            primary.category==0 && alternate.category==0 && independent.category==1,
            "actual Rainbow root event/header category semantics changed");
    const auto descriptor=[&](std::int32_t sound)->const e::Descriptor& {
        for (const auto& value:resources.descriptors()) if (value.sound==sound) return value;
        throw std::runtime_error("actual Rainbow header sound lacks its course descriptor");
    };
    require(descriptor(primary.sound).pitch_modifier==1 && descriptor(primary.sound).hearing_range==22 &&
            descriptor(alternate.sound).pitch_modifier==1 && descriptor(alternate.sound).hearing_range==22 &&
            descriptor(independent.sound).pitch_modifier==0 && descriptor(independent.sound).hearing_range==25,
            "Rainbow Mew sound pitch/hearing inputs disagree with pinned GAME");
    auto hydrated=g::hydrate(snapshot); require(bool(hydrated),"actual Rainbow audio hydration failed");
    const auto banks=hydrated.resources.sounds.banks();
    require(!banks.empty() && banks[0] && !banks[0]->instruments.empty() && banks[0]->instruments[0],
            "actual Rainbow sound font is absent");
    const auto& font=banks[0]->instruments[0]->sounds;
    for (const auto selection:{primary,alternate,independent}) {
        require(static_cast<std::size_t>(selection.sound)<font.size() && font[selection.sound],
                "selected Rainbow sound is an empty font slot; no substitute is admitted");
        const auto& sound=*font[selection.sound];
        require(sound.wave && sound.wave->type==a::WaveType::adpcm && sound.wave->book &&
                sound.wave->book->order==2 && sound.wave->book->predictors==4 &&
                sound.wave->loop==nullptr && sound.keymap && sound.envelope &&
                sound.envelope->attack_us==0 && sound.envelope->decay_us>0 &&
                sound.envelope->decay_volume==127 && sound.envelope->release_us>0 && sound.volume>0,
                "selected Rainbow voice lacks its real finite ADPCM/font/envelope domain");
    }
    // EnvSound supplies reverb0. Keep the real startup network installed,
    // without adding Rainbow lifecycle's separate global40/BGM/ambient voices.
    // These exact source angles select pan127/0. Native envelope retargeting
    // floors current gain to1, so assert channel dominance, not exact silence
    // on the opposite side or a transplanted Beach/numerical Original ratio.
    require(hydrated.resources.coefficients.pan[0]==32767 &&
            hydrated.resources.coefficients.pan[127]==0,
            "actual endpoint pan coefficients do not support the channel predicate");
    std::array<a::effects::Section,8> sections;
    a::effects::Config effects;
    require(g::native_startup_effects(hydrated.resources,sections,effects),
            "actual Rainbow native startup effects construction failed");
    isle::private_audio::Capture capture;
    isle::private_audio::Execution execution{snapshot,nullptr,{}};
    auto made=g::Session::create(std::move(hydrated.resources),effects,capture.output(),execution.executor());
    require(bool(made),"real Rainbow shared audio Session creation failed");
    auto& session=*made.session;
    std::uint64_t tasks=0,tick=0;
    const auto execute=[&](const a::TaskView& task) {
        isle::private_audio::execute(session,execution,task); ++tasks;
        require(session.submit(),"actual Rainbow checked PCM acceptance failed");
    };
    const auto pump=[&]() {
        ++tick;
        require(session.retrace(tick,tick*1000000000ull/60),"independent Rainbow audio retrace failed");
        require(session.submit(),"source-gated Rainbow completed PCM acceptance failed");
        a::TaskView task; require(session.build(task),"real Rainbow task build failed");
        if (task.token.serial) execute(task);
    };
    std::array<Energy,5> energy{};
    ActorBorrow lifetime{};
    {
        isle::game::State state(profile); require(state.initialize(1),"owned Rainbow private profile initialization failed");
        isle::game_input::Service input;
        sc::Storage storage(4*1024*1024); native(storage.create());
        {
            sc::Services services(storage.game(),storage.arena(),{{4,6,0,1,2},{}},input,session,state);
            require(!services.failed(),"real Rainbow scene Services construction failed");
            auto* player_object=services.object(1,9);
            auto* source_object=services.object(2,9);
            auto* camera_object=services.object(3,0);
            require(player_object && source_object && camera_object,"real Rainbow spatial objects failed");
            auto* player=model(services,player_object,0);
            auto* source=model(services,source_object,200);
            IsleGameCamera* camera=nullptr; native(isle_game_attach_camera(services.game,camera_object,&camera));
            IsleGameLookAt view{{0,0,0},{1,0,0},{0,1,0}};
            native(isle_game_camera_set_look_at(services.game,camera,&view));
            float distance=200;
            e::Environment environment(services,storage.arena(),resources);
            require(environment.play({source_object,source,&distance},primary.category,primary.sound) &&
                    environment.sound_handle(primary.sound)==-1,
                    "Rainbow sound escaped preparation before environment initialization");
            require(environment.initialize(player,camera),"real Rainbow environment initialization failed");
            lifetime={&services,&environment,0,0,0};
            require(services.set_lifecycle(source_object,{before_retire,after_join,&lifetime}) &&
                    services.process(source_object,park,&lifetime,ISLE_GAME_STACKFUL,8) &&
                    services.process(source_object,park,&lifetime,ISLE_GAME_STACKFUL,5),
                    "real Rainbow retirement observer/continuations failed");
            const auto window=[&](unsigned frames) {
                Energy result;
                for (unsigned frame=0;frame<frames;++frame) {
                    native(isle_game_tick(services.game));
                    const auto begin=capture.used;
                    pump();
                    // Four settled retraces, after actual task/AI publication.
                    if (frame+4>=frames)
                        result.add(std::span(capture.samples).subspan(begin,capture.used-begin));
                    require(!services.failed(),services.error());
                }
                return result;
            };
            a::TaskView bootstrap; require(session.build(bootstrap),"Rainbow bootstrap build failed");
            require(bootstrap.token.serial!=0,"Rainbow bootstrap omitted its actual task"); execute(bootstrap);
            const auto silent=window(8);
            require(silent.left==0 && silent.right==0,"voice-free actual Rainbow PCM was not silent");
            require(session.has_sound(0x34) &&
                    environment.play({source_object,source,&distance},primary.category,0x34) &&
                    environment.sound_handle(0x34)==-1,
                    "Rainbow accepted Beach's absent descriptor merely because its font sound exists");
            require(environment.admits_sound(primary.sound) &&
                    environment.play({source_object,source,&distance},primary.category,primary.sound),
                    "real Rainbow descriptor voice play failed");
            auto voice=environment.sound_handle(primary.sound);
            require(voice>=0 && session.playing_sound(static_cast<unsigned>(voice))==primary.sound,
                    "Rainbow descriptor did not select its actual native voice");
            energy[0]=window(8);
            require(energy[0].right>energy[0].left,
                    "actual Rainbow352 dry PCM did not consume source-derived hard-right pan");
            distance=100.0f*(2.0f*static_cast<float>(descriptor(alternate.sound).hearing_range)+1.0f);
            require(environment.play({source_object,source,&distance},alternate.category,alternate.sound) &&
                    environment.sound_handle(primary.sound)==voice && environment.sound_handle(alternate.sound)==-1 &&
                    session.playing_sound(static_cast<unsigned>(voice))==primary.sound,
                    "out-of-hearing Rainbow request replaced its live category");
            distance=200;
            require(environment.play({source_object,source,&distance},independent.category,independent.sound),
                    "real Rainbow independent category play failed");
            const auto second=environment.sound_handle(independent.sound);
            require(second>=0 && second!=voice,"Rainbow category1 did not own an independent native slot");
            window(4);
            require(session.playing_sound(static_cast<unsigned>(second))==independent.sound &&
                    session.playing_sound(static_cast<unsigned>(voice))==primary.sound &&
                    environment.stop_category(source_object,independent.category) &&
                    environment.sound_handle(independent.sound)==-1 &&
                    session.playing_sound(static_cast<unsigned>(second))==-1 &&
                    environment.sound_handle(primary.sound)==voice,
                    "Rainbow category1 stop failed or also stopped category0");
            window(4);
            require(environment.play({source_object,source,&distance},alternate.category,alternate.sound) &&
                    environment.sound_handle(primary.sound)==-1 &&
                    session.playing_sound(static_cast<unsigned>(voice))!=primary.sound,
                    "Rainbow category0 replacement retained its old actual voice");
            const auto replacement=environment.sound_handle(alternate.sound);
            energy[1]=window(8);
            require(replacement>=0 && session.playing_sound(static_cast<unsigned>(replacement))==alternate.sound &&
                    energy[1].right>energy[1].left,
                    "actual Rainbow350 replacement did not reach native right-channel PCM");
            require(environment.play({source_object,source,&distance},primary.category,primary.sound) &&
                    environment.sound_handle(alternate.sound)==-1,
                    "Rainbow category0 restoration retained its competing descriptor");
            voice=environment.sound_handle(primary.sound);
            require(voice>=0,"restored Rainbow352 lacks its actual native voice");
            window(8);
            view.at[0]=-1; native(isle_game_camera_set_look_at(services.game,camera,&view));
            energy[2]=window(8);
            require(energy[2].left>energy[2].right,
                    "actual Rainbow352 PCM did not follow source-derived hard-left pan");
            require(lifetime.entered==2,"real Rainbow source continuations never parked");
            services.erase(source_object);
            require(lifetime.pre==1 && lifetime.joined==1 &&
                    environment.sound_handle(primary.sound)==voice,
                    "Rainbow actor retirement did not detach borrows while retaining its native voice");
            IsleGameCounts counts{}; native(isle_game_counts(services.game,&counts));
            require(counts.models==1 && counts.processes==1,
                    "Rainbow source model/continuations survived joined actor retirement");
            view.at[0]=1; native(isle_game_camera_set_look_at(services.game,camera,&view));
            energy[3]=window(8);
            require(session.playing_sound(static_cast<unsigned>(voice))==primary.sound &&
                    energy[3].left>energy[3].right,
                    "fixed Rainbow voice followed the camera or lost its retired actor lifetime");
            require(environment.stop_category(player_object,primary.category) &&
                    environment.sound_handle(primary.sound)==voice &&
                    environment.stop_category(source_object,primary.category) &&
                    environment.sound_handle(primary.sound)==-1 &&
                    session.playing_sound(static_cast<unsigned>(voice))==-1,
                    "Rainbow fixed category stop lost comparison identity or matched another actor");
            energy[4]=window(12);
            require(energy[4].left==0 && energy[4].right==0,
                    "stopped Rainbow categories left actual PCM active");
            // A completed task still owns PCM until its source notification.
            // Two <=640-frame AI buffers drain within three >=533-frame
            // retraces. Accept that lease without building another task.
            const auto tail_begin=capture.used;
            for (unsigned n=0;n<3 && session.service().has_lease();++n) {
                ++tick;
                require(session.retrace(tick,tick*1000000000ull/60)==a::Error::none &&
                        session.submit()==a::Error::none,
                        "Rainbow final PCM source notification/acceptance failed");
            }
            Energy tail;
            tail.add(std::span(capture.samples).subspan(tail_begin,capture.used-tail_begin));
            require(tail.left==0 && tail.right==0 && !session.service().has_lease() &&
                    session.service().voices_retired(),
                    "stopped Rainbow voices or final silent PCM lease did not retire");
            environment.cleanup();
        }
        native(storage.destroy());
    }
    require(session.stop(),"actual Rainbow audio stop failed");
    for (unsigned n=0;n<240 && session.state()!=g::Session::State::draining;++n) pump();
    require(session.state()==g::Session::State::draining && session.service().voices_retired() &&
            !session.service().has_lease() && capture.state==g::Output::State::draining &&
            capture.finishes==1 && capture.polls==0,
            "Rainbow graph retirement incorrectly completed the independent PCM consumer");
    require(session.poll_drain(),"Rainbow first independent PCM drain failed");
    require(session.state()==g::Session::State::draining && capture.polls==1,
            "Rainbow graph retirement bypassed the held PCM drain");
    require(session.poll_drain(),"Rainbow final independent PCM drain failed");
    require(session.state()==g::Session::State::drained && capture.polls==2 &&
            session.accepted_frames()*2==capture.used,
            "actual Rainbow PCM accounting/ownership did not close");
    std::cout<<"environment_sound_Rainbow_actual_RSP_PCM=passed descriptors=9 primary="<<primary.sound
        <<" category="<<unsigned(primary.category)<<" replacement="<<alternate.sound
        <<" independent="<<independent.sound<<" independent_category="<<unsigned(independent.category)
        <<" tasks="<<tasks<<" accepted_frames="<<session.accepted_frames()
        <<" right_energy="<<energy[0].right<<" replacement_right_energy="<<energy[1].right
        <<" reversed_left_energy="<<energy[2].left<<" fixed_left_energy="<<energy[3].left
        <<" stopped_energy="<<energy[4].left+energy[4].right
        <<" native_pre_join="<<lifetime.pre<<'/'<<lifetime.joined
        <<" API_authored_inputs=yes animation_factory_campaign_hearing_hardware_Original_Windows11=not_claimed\n";
}
} // namespace
int main(int argc,char** argv) {
    try {
        const bool rainbow=argc==4 && std::string_view(argv[3])=="--rainbow";
        require(argc==3 || rainbow,
                "usage: environment-sound-rsp-smoke PRIVATE_ROM EXISTING_EMPTY_PRIVATE_PROFILE_DIRECTORY [--rainbow]");
        auto snapshot=isle::rom::read_snapshot(argv[1]);
        if (rainbow) run_rainbow(snapshot,std::filesystem::path(argv[2]));
        else run(snapshot,std::filesystem::path(argv[2]));
        isle::source_math::Angles held(snapshot);
        auto moved=std::move(snapshot); std::uint16_t value=0x1234;
        require(!held.live() && !held.atan2s(1,1,value) && value==0x1234 && moved.canonical().size()==isle::rom::us_size,
                "source angle moved-Snapshot rejection failed");
        return EXIT_SUCCESS;
    } catch (const std::exception& error) { std::cerr<<"environment-sound-rsp-smoke: "<<error.what()<<'\n'; return EXIT_FAILURE; }
}

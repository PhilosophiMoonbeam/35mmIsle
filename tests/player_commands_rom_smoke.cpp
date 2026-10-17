// Private admitted-ROM component consumer; not a CTest, game entrypoint,
// complete Root/reticle implementation, renderer or physical audio claim.
#include "player_commands.hpp"
#include "player_commands_rom_fixture.h"
#include "player_command_reference.h"
#include "actual_audio_driver.hpp"
#include "scene_services.hpp"
#include "scene_storage.hpp"
#include <array>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <utility>

namespace {
using namespace isle;
void require(bool value,const char* message) { if (!value) throw std::runtime_error(message); }
void native(IsleGameResult result) { require(result==ISLE_GAME_OK,"Player FIFO consumer native operation failed"); }
struct SourceContext {
    IslePlayerSource value{};
    explicit SourceContext(scene::Services& services) {
        native(isle_player_source_prepare(services.game,&value,&services.rng,this,10));
        native(isle_player_source_activate(&value));
    }
    ~SourceContext() {
        const auto result=isle_player_source_deactivate(&value);
        if (result!=ISLE_GAME_OK && result!=isle_game_status(value.game)) std::terminate();
    }
};
/* Own only fixture handles and their real cleanup. The production Binding owns
 * no registry. Every registered native identity remains held until its actual
 * join and zero semantic credits, including on exceptions/terminal faults. */
class NativeLifetime final {
public:
    NativeLifetime(scene::Services& services,player::commands::Binding& binding,
        IslePlayerSource& player,PlayerCommandsRomTrace& trace)
        :services_(services),binding_(binding),player_(player),trace_(trace) {}
    ~NativeLifetime() { if (!finished_ && !finish()) std::terminate(); }
    void attach_trace() { native(player_commands_rom_attach(&trace_)); attached_=true; }
    IsleGameObject* owner(std::uint32_t id,std::uint8_t link,std::uint32_t priority=0x80000000u) {
        require(size_<slots_.size(),"Player FIFO fixture handle capacity exceeded");
        auto* object=services_.object(id,link,priority);
        require(object,"Player FIFO native object allocation failed");
        auto& slot=slots_[size_++]; slot.object=object;
        require(services_.source_owner_register(object,{}),"Player FIFO real source-owner registration failed");
        slot.registered=true;
        require(services_.set_command_queued_observer(object,player_commands_rom_queued,&trace_),
            "Player FIFO enqueue observation failed");
        return object;
    }
    IsleGameObject* camera() {
        auto* object=owner(1,0);
        native(isle_game_attach_camera(services_.game,object,&player_.camera));
        require(services_.source_commands_native(object),"Player FIFO camera native scope admission failed");
        native(isle_game_object_set_source_update(services_.game,object,player_command_reference_camera_update));
        trace_.camera=object;
        return object;
    }
    IsleGameObject* sprite(std::uint32_t id,std::uint8_t link,std::uint32_t priority=100) {
        auto* object=owner(id,link,priority);
        IsleGameSprite* sprite=nullptr;
        native(isle_game_add_sprite(services_.game,object,nullptr,&sprite));
        require(services_.source_commands_native(object),"Player FIFO sprite native scope admission failed");
        native(isle_game_object_set_source_update(services_.game,object,player_commands_rom_sprite_update));
        return object;
    }
    void process(IsleGameObject* object) {
        IsleGameProcess* process=nullptr;
        native(isle_game_add_source_process(services_.game,object,player_commands_rom_process,
            ISLE_GAME_SYNCHRONOUS,1,&process));
    }
    void lifecycle(IsleGameObject* object) {
        require(services_.set_lifecycle(object,{player_commands_rom_retiring,player_commands_rom_joined,&trace_}),
            "Player FIFO native lifecycle observation failed");
    }
    void erase(IsleGameObject* object) { services_.erase(object); }
    bool release(IsleGameObject* object) noexcept {
        for (auto& slot:slots_) if (slot.object==object) {
            if (!slot.registered || !services_.source_owner_release(object)) return false;
            slot={}; return true;
        }
        return false;
    }
    bool finish() noexcept {
        if (finished_) return true;
        for (auto& slot:slots_) if (slot.object) services_.erase(slot.object);
        bool closed=true;
        for (auto& slot:slots_) if (slot.object && slot.registered) {
            const auto borrows=services_.source_owner_borrows(slot.object);
            if (!services_.source_owner_joined(slot.object) || borrows.queued || borrows.fields ||
                !services_.source_owner_release(slot.object)) closed=false;
            else slot={};
        }
        if (!closed) return false;
        if (attached_) {
            if (player_commands_rom_detach(&trace_)!=ISLE_GAME_OK) return false;
            attached_=false;
        }
        // Only actual native joins above authorize ending these borrows.
        player_.camera=nullptr;
        if (binding_.detach()!=ISLE_GAME_OK) return false;
        finished_=true;
        return true;
    }
private:
    struct Slot { IsleGameObject* object=nullptr; bool registered=false; };
    scene::Services& services_;
    player::commands::Binding& binding_;
    IslePlayerSource& player_;
    PlayerCommandsRomTrace& trace_;
    std::array<Slot,16> slots_{};
    std::size_t size_=0;
    bool attached_=false,finished_=false;
};
IsleGame* create(scene::Storage& storage) { native(storage.create()); return storage.game(); }
PlayerCommandsRomTopology topology(IslePlayerSource& source,IsleGameObject* camera=nullptr) {
    PlayerCommandsRomTopology out{}; native(player_commands_rom_topology(&source,camera,&out)); return out;
}
bool idle(const PlayerCommandsRomTopology& value) {
    return !value.callback_depth && !value.command_fault_armed && !value.source_frame_open && !value.traversing;
}
bool empty(const PlayerCommandsRomTopology& value) {
    return idle(value) && !value.held_identities && !value.counts.objects && !value.counts.processes &&
        !value.counts.sprites && !value.counts.cameras && !value.counts.matrices &&
        !value.counts.models && !value.counts.model_nodes;
}
void send(player::commands::Binding& binding,IsleGameObject* receiver,int32_t command,IsleGameObject* sender) {
    native(isle_player_commands_backend_send(binding.source(),receiver,command,sender));
}
bool delivery(const PlayerCommandsRomEvent& event,IsleGameObject* receiver,int32_t command,
    IsleGameObject* sender,bool hidden,bool paused) {
    return event.receiver==receiver && event.command==command && event.sender==sender &&
        bool(event.flags&1)==hidden && bool(event.paused)==paused;
}
void ordinary(game_audio::Session& audio,game::State& state) {
    game_input::Service input; scene::Storage storage(4*1024*1024);
    scene::Services services(create(storage),storage.arena(),{{8,4,4,1,0},{}},input,audio,state);
    SourceContext context(services); auto& source=context.value;
    player::commands::Binding binding(services,source);
    PlayerCommandsRomTrace trace{}; trace.player=&source; trace.broadcast_link=5;
    trace.probe_update_admission=1;
    NativeLifetime lifetime(services,binding,source,trace); lifetime.attach_trace();
    auto* camera=lifetime.camera();
    auto* first=lifetime.sprite(31,5); auto* second=lifetime.sprite(32,5);
    lifetime.process(first);
    auto* sender=lifetime.owner(90,6);
    const auto admitted=topology(source,camera);
    // A disabled actual one-argument update retains the real FIFO across OM.
    // Restoring the exact original function must then consume those commands.
    native(isle_game_object_set_source_update(services.game,camera,nullptr));
    IsleGameResult foreign_send=ISLE_GAME_OK,foreign_update=ISLE_GAME_OK;
    std::thread foreign([&] {
        foreign_send=isle_player_commands_backend_send(binding.source(),camera,ISLE_PLAYER_REFERENCE_CAMERA_BLINK,sender);
        foreign_update=isle_game_object_set_source_update(services.game,camera,player_command_reference_camera_update);
    });
    foreign.join();
    send(binding,camera,ISLE_PLAYER_REFERENCE_CAMERA_BLINK,sender);
    send(binding,camera,PLAYER_COMMANDS_ROM_UNKNOWN,sender);
    send(binding,first,-4,sender); send(binding,first,PLAYER_COMMANDS_ROM_UNKNOWN,sender);
    send(binding,first,-2,sender); send(binding,first,PLAYER_COMMANDS_ROM_UNKNOWN,sender);
    send(binding,second,-4,sender); send(binding,second,PLAYER_COMMANDS_ROM_UNKNOWN,sender);
    send(binding,second,-5,sender); send(binding,second,PLAYER_COMMANDS_ROM_UNKNOWN,sender);
    const auto queued=services.source_owner_borrows(sender).queued;
    lifetime.erase(sender);
    const bool retained=services.source_owner_joined(sender) && !services.source_owner_collectable(sender);
    auto* pressure=lifetime.owner(91,6);
    native(isle_game_tick(services.game));
    bool suffix=trace.delivered_count==4 && trace.handlers_returned==4 && trace.process_runs==0 &&
        source.MainCameraBlinkTimeout==0 && services.source_owner_borrows(sender).queued==2;
    if (trace.delivered_count==4) {
        suffix=suffix && delivery(trace.delivered[0],first,PLAYER_COMMANDS_ROM_UNKNOWN,sender,true,false) &&
            delivery(trace.delivered[1],first,PLAYER_COMMANDS_ROM_UNKNOWN,sender,true,true) &&
            delivery(trace.delivered[2],second,PLAYER_COMMANDS_ROM_UNKNOWN,sender,true,false) &&
            delivery(trace.delivered[3],second,PLAYER_COMMANDS_ROM_UNKNOWN,sender,false,false);
        for (std::size_t i=0;i<4;++i)
            suffix=suffix && trace.delivered[i].retired_sender && trace.delivered[i].sender_id==90;
    }
    native(isle_game_object_set_source_update(services.game,camera,player_command_reference_camera_update));
    native(isle_game_tick(services.game));
    const auto restored=topology(source,camera);
    const bool blink=source.MainCameraBlinkTimeout==1 && services.source_owner_borrows(sender).queued==0 &&
        services.source_owner_collectable(sender) && trace.process_runs==0;
    const bool released=lifetime.release(sender);
    auto* reused=lifetime.owner(92,6);
    source.MainCameraBlinkTimeout=0;
    send(binding,first,-3,reused); send(binding,first,-5,reused);
    send(binding,first,PLAYER_COMMANDS_ROM_EMIT,reused);
    send(binding,first,PLAYER_COMMANDS_ROM_UNKNOWN,reused);
    send(binding,second,PLAYER_COMMANDS_ROM_UNKNOWN,reused);
    const auto before_emit=trace.queued_count;
    native(isle_game_tick(services.game));
    bool sends=source.MainCameraBlinkTimeout==0 && trace.delivered_count==9 && trace.process_runs==1 &&
        services.source_owner_borrows(first).queued==1 && trace.queued_count==before_emit+3;
    if (trace.delivered_count==9) {
        sends=sends && delivery(trace.delivered[4],first,PLAYER_COMMANDS_ROM_EMIT,reused,false,false) &&
            delivery(trace.delivered[5],first,PLAYER_COMMANDS_ROM_UNKNOWN,reused,false,false) &&
            delivery(trace.delivered[6],first,PLAYER_COMMANDS_ROM_BROADCAST,first,false,false) &&
            delivery(trace.delivered[7],second,PLAYER_COMMANDS_ROM_UNKNOWN,reused,false,false) &&
            delivery(trace.delivered[8],second,PLAYER_COMMANDS_ROM_BROADCAST,first,false,false);
    }
    if (trace.queued_count==before_emit+3) {
        const auto* events=trace.queued+before_emit;
        sends=sends && events[0].receiver==camera && events[0].command==ISLE_PLAYER_REFERENCE_CAMERA_BLINK &&
            events[0].sender==first && events[1].receiver==first && events[2].receiver==second &&
            events[1].command==PLAYER_COMMANDS_ROM_BROADCAST && events[2].command==PLAYER_COMMANDS_ROM_BROADCAST &&
            events[1].sender==first && events[2].sender==first;
    }
    native(isle_game_tick(services.game));
    const bool delayed=source.MainCameraBlinkTimeout==1 && services.source_owner_borrows(first).queued==0 &&
        trace.process_runs==2 && trace.handlers_returned==9 && trace.updates_entered==8 && trace.updates_returned==8;
    const bool healthy=!services.failed() && source.status==ISLE_GAME_OK && trace.invalid==0;
    const bool cleaned=lifetime.finish(); const auto final=topology(source);
    require(cleaned && empty(final) && source.commands==nullptr && queued==10 && retained && pressure!=sender &&
        released && reused==sender && admitted.camera_original_update && admitted.camera_none_payload && admitted.camera_attached &&
        restored.camera_original_update && restored.camera_none_payload && restored.camera_attached &&
        suffix && blink && sends && delayed && healthy && foreign_send==ISLE_GAME_WRONG_OWNER &&
        foreign_update==ISLE_GAME_WRONG_OWNER && trace.null_update_probed==1 &&
        trace.null_during_update==ISLE_GAME_INVALID_ARGUMENT,
        "Player FIFO original camera/empty handler, native builtins/order/sends/retired identity or real cleanup failed");
}
void deletion(game_audio::Session& audio,game::State& state,bool builtin) {
    game_input::Service input; scene::Storage storage(4*1024*1024);
    scene::Services services(create(storage),storage.arena(),{{8,4,4,1,0},{}},input,audio,state);
    SourceContext context(services); auto& source=context.value;
    player::commands::Binding binding(services,source);
    PlayerCommandsRomTrace trace{}; trace.player=&source; trace.broadcast_link=5;
    trace.probe_update_admission=1;
    NativeLifetime lifetime(services,binding,source,trace); lifetime.attach_trace();
    auto* camera=lifetime.camera(); auto* receiver=lifetime.sprite(31,5);
    lifetime.process(receiver); lifetime.lifecycle(receiver);
    auto* sender=lifetime.owner(90,6);
    send(binding,receiver,builtin?-1:PLAYER_COMMANDS_ROM_DELETE,sender);
    send(binding,receiver,PLAYER_COMMANDS_ROM_UNKNOWN,sender);
    native(isle_game_tick(services.game));
    const bool joined=services.source_owner_joined(receiver) && services.source_owner_collectable(receiver) &&
        services.source_owner_borrows(sender).queued==0 && trace.retiring==1 && trace.joined==1 &&
        trace.join_invalid==0 && trace.process_runs==0 && trace.delivered_count==(builtin?0u:1u) &&
        trace.handlers_returned==(builtin?0u:1u) && trace.updates_entered==1 && trace.updates_returned==1;
    auto* pressure=lifetime.owner(91,6);
    const bool released=lifetime.release(receiver);
    auto* reused=lifetime.sprite(32,5);
    // The reused native address must own a fresh real FIFO, not the deleted
    // receiver's unknown suffix or its old Source/callback bookkeeping.
    send(binding,reused,PLAYER_COMMANDS_ROM_UNKNOWN,sender);
    send(binding,camera,ISLE_PLAYER_REFERENCE_CAMERA_BLINK,sender);
    native(isle_game_tick(services.game));
    const auto expected=builtin?1u:2u;
    const bool fresh=trace.delivered_count==expected && trace.handlers_returned==expected &&
        trace.delivered[expected-1].receiver==reused && trace.delivered[expected-1].command==PLAYER_COMMANDS_ROM_UNKNOWN &&
        source.MainCameraBlinkTimeout==1 && services.source_owner_borrows(sender).queued==0 &&
        !services.failed() && source.status==ISLE_GAME_OK && trace.invalid==0;
    const bool cleaned=lifetime.finish(); const auto final=topology(source);
    require(cleaned && empty(final) && source.commands==nullptr && joined && pressure!=receiver &&
        released && reused==receiver && fresh && trace.null_update_probed==1 &&
        trace.null_during_update==ISLE_GAME_INVALID_ARGUMENT &&
        trace.live_during_retirement==ISLE_GAME_INVALID_STATE &&
        trace.retiring_update==ISLE_GAME_INVALID_STATE,
        "Player FIFO real self/builtin deletion, current/suffix cleanup, join or address-reuse isolation failed");
}
enum class Failure { plain, nested, appended, foreign_source };
void failure(game_audio::Session& audio,game::State& state,Failure mode) {
    // Terminal native faults never share a Source/game scope with later cases.
    const bool nested=mode==Failure::nested, mixed=mode==Failure::appended;
    const bool foreign=mode==Failure::foreign_source;
    game_input::Service input; scene::Storage storage(4*1024*1024);
    scene::Services services(create(storage),storage.arena(),{{8,4,4,1,0},{}},input,audio,state);
    SourceContext context(services); auto& source=context.value;
    player::commands::Binding binding(services,source);
    PlayerCommandsRomTrace trace{}; trace.player=&source; trace.broadcast_link=5;
    IslePlayerSource inactive{};
    std::array<unsigned char,sizeof(inactive)> inactive_before;
    if (foreign) {
        native(isle_player_source_prepare(services.game,&inactive,&services.rng,&inactive,10));
        trace.inactive_source=&inactive;
        std::memcpy(inactive_before.data(),&inactive,sizeof(inactive));
    }
    NativeLifetime lifetime(services,binding,source,trace); lifetime.attach_trace();
    auto* camera=lifetime.camera(); auto* receiver=lifetime.sprite(31,5); lifetime.lifecycle(receiver);
    auto* current=lifetime.owner(90,6); auto* suffix=lifetime.owner(91,6);
    IsleGameObject* prefix=nullptr;
    if (mixed) {
        prefix=lifetime.owner(92,6);
        send(binding,receiver,-4,prefix);
        lifetime.erase(prefix);
    }
    const auto command=foreign?PLAYER_COMMANDS_ROM_FOREIGN_SOURCE:
        mixed?PLAYER_COMMANDS_ROM_APPEND_FAULT:nested?PLAYER_COMMANDS_ROM_NESTED_FAULT:PLAYER_COMMANDS_ROM_FAULT;
    send(binding,receiver,command,current);
    send(binding,receiver,PLAYER_COMMANDS_ROM_UNKNOWN,suffix);
    lifetime.erase(current); lifetime.erase(suffix);
    const auto result=isle_game_tick(services.game);
    const auto faulted=topology(source,camera);
    const bool current_closed=services.source_owner_borrows(current).queued==0 &&
        services.source_owner_collectable(current);
    // Existing abort semantics preserve the genuinely pending suffix. It stays
    // held until actual receiver cancellation/retirement, never a fake clear.
    const bool suffix_owned=services.source_owner_borrows(suffix).queued==1 &&
        !services.source_owner_collectable(suffix);
    const bool appended_owned=services.source_owner_borrows(receiver).queued==(mixed?1u:0u) &&
        trace.queued_count==(mixed?4u:2u) && (!mixed ||
        (services.source_owner_borrows(prefix).queued==0 && services.source_owner_collectable(prefix) &&
         (trace.delivered[0].flags&1)!=0 && trace.queued[3].receiver==receiver &&
         trace.queued[3].sender==receiver && trace.queued[3].command==PLAYER_COMMANDS_ROM_UNKNOWN));
    const auto first_error=foreign?ISLE_GAME_INVALID_STATE:ISLE_GAME_INVALID_ARGUMENT;
    const bool inactive_untouched=!foreign ||
        (std::memcmp(inactive_before.data(),&inactive,sizeof(inactive))==0 &&
         source.MainCameraBlinkTimeout==0);
    const bool checked=result==ISLE_GAME_EXECUTION_FAILURE && source.status==first_error &&
        services.failed() && trace.fault_entered==1 && trace.fault_returned==0 &&
        trace.delivered_count==1 && trace.handlers_returned==0 && trace.updates_entered==1 &&
        trace.updates_returned==0 && trace.invalid==0 && trace.delivered[0].retired_sender &&
        trace.delivered[0].sender_id==90 && (!nested ||
        (trace.nested_result==ISLE_GAME_INVALID_ARGUMENT && trace.nested_restored));
    const bool cleaned=lifetime.finish(); const auto final=topology(source);
    require(cleaned && empty(final) && source.commands==nullptr && checked && current_closed && suffix_owned &&
        appended_owned && inactive_untouched &&
        idle(faulted) && faulted.camera_original_update && faulted.camera_none_payload && faulted.camera_attached &&
        trace.retiring==1 && trace.joined==1 && trace.join_invalid==0 &&
        source.status==first_error && isle_game_status(services.game)==ISLE_GAME_EXECUTION_FAILURE,
        "Player FIFO real native wrong-payload fault, local nested C guard, first-error/current/suffix cleanup or joins failed");
}
void run(const std::filesystem::path& filename,const std::filesystem::path& profile) {
    require(std::filesystem::is_directory(profile) && std::filesystem::is_empty(profile),
        "Player FIFO private profile must exist and be empty");
    const auto snapshot=rom::read_snapshot(filename);
    auto hydrated=game_audio::hydrate(snapshot);
    require(bool(hydrated),"Player FIFO actual Services audio resource admission failed");
    std::array<audio::effects::Section,8> sections; audio::effects::Config effects;
    require(game_audio::native_startup_effects(hydrated.resources,sections,effects)==audio::Error::none,
        "Player FIFO actual Services audio configuration failed");
    private_audio::Capture pcm; private_audio::Execution execution{snapshot,nullptr,{}};
    auto audio=game_audio::Session::create(std::move(hydrated.resources),effects,pcm.output(),execution.executor());
    require(bool(audio),"Player FIFO actual borrowed Services Session failed");
    game::State state(profile); require(state.initialize(1),"Player FIFO private save profile initialization failed");
    ordinary(*audio.session,state);
    deletion(*audio.session,state,false); deletion(*audio.session,state,true);
    failure(*audio.session,state,Failure::plain); failure(*audio.session,state,Failure::nested);
    failure(*audio.session,state,Failure::appended); failure(*audio.session,state,Failure::foreign_source);
    std::cout<<"Player FIFO original camera/empty C handlers, actual native updater/builtins/send/link/order, retained retired sender, real deletion/reuse and nested C fault cleanup passed; no full Root/game/device claim\n";
}
}
int main(int argc,char** argv) {
    if (argc!=3) {
        std::cerr<<"usage: player-commands-rom-smoke PRIVATE_ADMITTED_ROM EMPTY_PRIVATE_PROFILE\n";
        return EXIT_FAILURE;
    }
    try { run(argv[1],argv[2]); return EXIT_SUCCESS; }
    catch (const std::exception& error) { std::cerr<<error.what()<<'\n'; return EXIT_FAILURE; }
}

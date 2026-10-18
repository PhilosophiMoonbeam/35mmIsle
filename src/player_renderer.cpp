#include "player_renderer.hpp"
#include "player_commands.hpp"
#include "actor_owner.hpp"
#include "player_cart.hpp"
#include "player_ui.hpp"
#include "screen_transition_resources.hpp"
#include "graphics_task.hpp"
#include "source_effect_camera.hpp"
#include "menu_scene_bridge.h"
#include <algorithm>
#include <array>
#include <bit>
#include <exception>
#include <limits>
#include <stdexcept>
#include <vector>
#include <optional>

namespace isle::player {
namespace {
namespace r=scene::render;
namespace g=scene::graphics;
namespace w=graphics;
struct Failure { IsleGameResult result; const char* message; };
void require(bool condition,const char* message,IsleGameResult result=ISLE_GAME_INVALID_STATE) {
    if(!condition) throw Failure{result,message};
}
void native(IsleGameResult result,const char* message) { require(result==ISLE_GAME_OK,message,result); }
bool overlaps(sprite::RamRange a,sprite::RamRange b) noexcept {
    return a.size&&b.size&&a.address<std::uint64_t(b.address)+b.size&&b.address<std::uint64_t(a.address)+a.size;
}
std::uint32_t xy(std::int32_t x,std::int32_t y) noexcept {
    // GBI _SHIFTL masks each signed input independently. In particular Ymin-1
    // remains a twelve-bit signed packet field; never clamp source endpoints.
    return ((static_cast<std::uint32_t>(x)*4u&0xfffu)<<12)|
           (static_cast<std::uint32_t>(y)*4u&0xfffu);
}
}
struct Renderer::Impl {
    struct Record {
        Impl* owner=nullptr;
        Camera metadata{};
        IsleGameCamera* attachment=nullptr;
        std::array<IsleGameObject**,2> references{};
        std::uint64_t insertion=0;
        bool active=false,retiring=false,joined=false,registered=false,identity_held=false;
        bool effects=false,helper=false,message=false,transition=false;
        screen_transition::Owner* transition_owner=nullptr;
        r::Kind kind=r::Kind::compiled;
        Dispatch dispatch{};
        std::array<std::uint32_t,4> wire_begin{},wire_cursor{},wire_end{};
        std::uint32_t reset_address=0;
        std::size_t reset_count=0;
    };
    struct Sink {
        Impl& owner;
        Record& record;
        g::SourceControl& control;
        w::Memory memory;
        std::array<std::uint32_t,4>& begin;
        std::array<std::uint32_t,4>& cursor;
        std::array<std::uint32_t,4>& end;
        bool initialized=false,prepared=false,posted=false;
        r::Prepared lists{};
        Sink(Impl& self,Record& camera,g::SourceControl& capability):owner(self),record(camera),control(capability),
            memory(capability.ram()),begin(camera.wire_begin),cursor(camera.wire_cursor),end(camera.wire_end) {
            require(!self.sink,"source render sink already active");
            const auto index=static_cast<std::size_t>(&camera-self.records.data());
            const auto base=self.config.lanes.address+static_cast<std::uint32_t>(index)*self.record_bytes;
            const auto lane_bytes=(self.record_bytes-0x200u)/4u&~7u;
            for(unsigned i=0;i<4;++i) { begin[i]=cursor[i]=base+i*lane_bytes;end[i]=begin[i]+lane_bytes; }
            camera.reset_address=base+self.record_bytes-0x200u;camera.reset_count=0;
            camera.dispatch={};camera.dispatch.epoch=control.epoch();
            camera.dispatch.framebuffer=control.framebuffer();camera.dispatch.depth=control.depth();
            self.link_zero(camera,memory);
            self.sink=this;
        }
        ~Sink() { owner.sink=nullptr; }
        void command(unsigned lane,std::uint32_t first,std::uint32_t second) {
            require(lane<4,"source wire lane is outside four bounded cursors",ISLE_GAME_INVALID_ARGUMENT);
            require(cursor[lane]<=end[lane]-8,"source wire lane cold capacity exhausted",ISLE_GAME_EXHAUSTED);
            memory.word(cursor[lane],first);memory.word(cursor[lane]+4,second);cursor[lane]+=8;
        }
        w::CommandRange reset_range() {return owner.ensure_reset(control,memory,record);}
        void load(unsigned lane,w::SourceProgram program) {
            const auto packet=owner.config.programs->load(program,reset_range());
            for(std::size_t i=0;i<packet.word_count;i+=2) command(lane,packet.words[i],packet.words[i+1]);
        }
        void restore(unsigned lane) {
            if(lane==1&&(record.metadata.flags&0x20u)) {
                load(lane,owner.config.program_selection.alternate);
                owner.config.program_selection.alternate_active=true;
            }
            cursor[lane]=control.restore_camera(lane,cursor[lane],end[lane]);
        }
        void call(unsigned lane,r::List list) {
            if(!list.words) return;
            require(!(list.address&7u)&&!(list.words&1u)&&list.words>=2&&
                list.address<=control.ram().size()*4&&list.words<=(control.ram().size()*4-list.address)/4,
                "source provider list is not a bounded physical range");
            bool covered=false;
            for(const auto range:owner.reservations) covered=covered||
                (range.address<=list.address&&std::uint64_t(list.address)+list.words*4<=std::uint64_t(range.address)+range.size);
            require(covered,"source provider list lacks a cold resource reservation");
            command(lane,0xde000000u,list.address);
        }
    };
    scene::Services& services;
    IslePlayerSource& source;
    const photo::LightingInputs& lighting;
    Config config;
    std::array<Record,camera_capacity> records{};
    std::array<r::Pass,camera_capacity> passes{};
    std::array<Record*,4> wire_heads{},wire_tails{};
    std::vector<scene::SpriteDraw> sprites;
    std::vector<sprite::RamRange> reservations;
    std::vector<sprite::AssetSpan> immutable;
    std::vector<sprite::RamRange> message_protected;
    photo::LightDirection direction;
    std::uint32_t record_bytes=0;
    std::uint64_t insertion=0;
    std::uint32_t drawn_frames=0;
    Sink* sink=nullptr;
    g::Frame* frame=nullptr;
    g::CompletedWork downstream{};
    actor::Owner* actors=nullptr;
    cart::Owner* cart_owner=nullptr;
    std::span<scene::RenderOrderEntry> cart_order;
    std::optional<cart::Owner::Borrow> cart_borrow;
    Ui* message_ui=nullptr;
    sprite::RamRange message_commands{};
    screen_transition::Resources* transitions=nullptr;
    screen_transition::Owner* transition_owner=nullptr;
    Record* transition_record=nullptr;
    r::Pass transition_pass{};
    g::Frame* reserved_frame=nullptr;
    IsleGameObject* ui=nullptr;
    IsleGameObject* reference_owner=nullptr;
    bool main_removal_requested=false,main_removed=false;
    IsleGameResult error=ISLE_GAME_OK;
    std::array<char,512> diagnostic{};
    bool detached=false,preparing=false,postprocess_registered=true,presentation_task=false,started=false;
    Impl(scene::Services& s,IslePlayerSource& p,const photo::LightingInputs& light,Config c):
        services(s),source(p),lighting(light),config(c),direction(light.initial_direction()) {
        IslePlayerSource* canonical=nullptr;
        native(isle_player_source_context(s.game,&canonical),"source renderer canonical context rejected");
        require(canonical==&p&&p.game==s.game&&!p.native_renderer&&p.commands&&!s.failed(),
            "source renderer requires active canonical Source, Services and native command binding");
        require(config.prepare&&config.prepare_user&&config.resources.identity&&config.resources.pre_render&&config.programs,
            "source renderer requires real global producer, programs and admitted resources",ISLE_GAME_INVALID_ARGUMENT);
        static_cast<void>(config.programs->descriptor(graphics::source_task_program(config.program_selection,false)));
        require(!(config.lanes.address&63u)&&!(config.lanes.size&63u)&&
            config.lanes.size>=camera_capacity*0x400u&&config.lanes.address<=0x800000u&&
            config.lanes.size<=0x800000u-config.lanes.address,
            "source lane cold plan is unaligned or exceeds physical RAM",ISLE_GAME_INVALID_ARGUMENT);
        require(config.sprite_capacity>0&&config.resources.sprites&&!(config.sprite_commands.address&7u)&&
            !(config.sprite_commands.size&7u)&&config.sprite_commands.size>=camera_capacity*64u,
            "source renderer requires a cold sprite producer and disjoint camera arenas",ISLE_GAME_INVALID_ARGUMENT);
        record_bytes=(config.lanes.size/static_cast<std::uint32_t>(camera_capacity))&~63u;
        sprites.resize(config.sprite_capacity);
        reservations.assign(config.resources.reservations.begin(),config.resources.reservations.end());
        for(const auto range:reservations) require(!overlaps(config.lanes,range),"source lane arena aliases a provider reservation");
        reservations.push_back(config.lanes);
        // Sprite/FX arenas may already be provider-owned covering reservations.
        for(const auto arena:{config.sprite_commands,config.effect_commands}) {
            require(!overlaps(config.lanes,arena),"source lane arena aliases sprite/FX commands");
            bool covered=false;
            for(const auto range:reservations) covered=covered||(range.address<=arena.address&&
                std::uint64_t(arena.address)+arena.size<=std::uint64_t(range.address)+range.size);
            if(!covered) reservations.push_back(arena);
        }
        config.resources.reservations=reservations;
        p.native_renderer=this;
    }
    void fail(IsleGameResult result,const char* message) noexcept {
        if(error!=ISLE_GAME_OK) return;
        error=result;
        if(!message) message="source renderer failure";
        std::size_t n=0;
        while(n+1<diagnostic.size()&&message[n]) {diagnostic[n]=message[n];++n;}
        diagnostic[n]='\0';
    }
    void ready() {
        require(!detached&&source.native_renderer==this,"source renderer is detached");
        native(error,"source renderer retains its first failure");
        native(source.status,"original source retains its first failure");
        require(!services.failed(),services.error());
        IslePlayerSource* active=nullptr;
        native(isle_player_source_context(services.game,&active),"source renderer context rejected");
        require(active==&source,"source renderer canonical context changed");
    }
    Record* find(IsleGameObject* object) noexcept {
        for(auto& record:records) if(record.active&&record.metadata.scheduling_object==object) return &record;
        return nullptr;
    }
    Record& camera(IsleGameCamera* value) {
        for(auto& record:records) if(record.active&&!record.retiring&&record.attachment==value) return record;
        throw Failure{ISLE_GAME_INVALID_ARGUMENT,"camera metadata has no live native attachment"};
    }
    Record& free_record() {
        require(!frame&&!sink&&(!config.photos||config.photos->quiescent()),
            "camera registration cannot cross GPU/Photo/dispatch ownership");
        for(auto& record:records) if(!record.active&&!record.transition) return record;
        throw Failure{ISLE_GAME_EXHAUSTED,"source scheduling camera capacity exhausted"};
    }
    static void retiring(IsleGameObject* object,void* user) noexcept {
        auto& record=*static_cast<Record*>(user);auto& self=*record.owner;
        if(record.metadata.scheduling_object!=object||record.retiring) std::terminate();
        record.retiring=true;
        // Deregister while native payload still exists. Shared projection does
        // not imply scheduling identity. No camera lookup after actual join.
        if(self.source.camera==record.attachment) self.source.camera=nullptr;
        if(record.transition&&record.transition_owner) {
            auto& transition=record.transition_owner->source;
            if(transition.camera_object==object&&transition.D_800BE2C8==record.attachment) {
                transition.camera_object=nullptr;transition.D_800BE2C8=nullptr;
            }
        }
        for(auto& dependent:self.records) if(dependent.active&&
            dependent.metadata.projection==record.attachment)
            dependent.metadata.projection=nullptr;
        if(self.ui==object) self.ui=nullptr;
    }
    static void joined(IsleGameObject* object,void* user) noexcept {
        auto& record=*static_cast<Record*>(user);
        if(record.metadata.scheduling_object!=object||!record.retiring) std::terminate();
        record.joined=true;record.attachment=nullptr;record.metadata.projection=nullptr;
        auto& self=*record.owner;
        if(self.main_removal_requested&&object==self.reference_owner)
            self.main_removed=true;
    }
    Record& register_camera(const Camera& metadata,bool created,bool helper=false,Record* reserved=nullptr) {
        require(metadata.scheduling_object&&!find(metadata.scheduling_object)&&
            (helper||((created||metadata.projection)&&(metadata.render||(reserved&&metadata.prepare)))),
            "invalid or duplicate genuine scheduling camera",ISLE_GAME_INVALID_ARGUMENT);
        require(!(metadata.flags&0x50u),"task-flushing source camera requires actual multi-task Frame admission");
        auto& record=reserved?*reserved:free_record();
        require(!record.active,"reserved source scheduling camera is already active");
        record={};record.owner=this;record.metadata=metadata;
        record.metadata.tag_match=(metadata.flags&8u)?r::TagMatch::equality:r::TagMatch::intersection;
        require(insertion!=std::numeric_limits<std::uint64_t>::max(),"source camera insertion order exhausted",ISLE_GAME_EXHAUSTED);
        record.helper=helper;record.insertion=++insertion;
        IsleGameObjectMetadata native_metadata{};
        native(isle_game_object_metadata(services.game,metadata.scheduling_object,&native_metadata),"scheduling object is not live");
        // A payload-less FX scheduling helper receives its own native camera;
        // its projection remains the actual main camera, independently.
        if(!created) native(isle_game_object_camera(services.game,metadata.scheduling_object,&record.attachment),"scheduling camera attachment missing");
        const bool fields=created&&!reference_owner&&native_metadata.id==1;
        if(fields) record.references={&source.D_80382D10_523120,&source.D_80382D14_523124};
        require(services.observe_lifecycle(metadata.scheduling_object,{retiring,joined,&record}),"camera lifecycle observation failed");
        record.active=true;
        if(services.source_owner_registered(metadata.scheduling_object)) {
            require(!fields,"main canonical fields already belong to another receiver");
            native(isle_game_object_retain_identity(services.game,metadata.scheduling_object),"existing camera independent native identity hold failed");
            record.identity_held=true;
        } else {
            require(services.source_owner_register(metadata.scheduling_object,fields?
                std::span<IsleGameObject** const>(record.references):std::span<IsleGameObject** const>{}),
                "camera native/source identity retain failed");
            record.registered=true;
        }
        if(fields) reference_owner=metadata.scheduling_object;
        return record;
    }
    IsleGameResult stop_main() noexcept {
        auto* record=find(reference_owner);
        if(!record||record->retiring||record->joined||
           (source.camera&&record->attachment!=source.camera)) return ISLE_GAME_INVALID_STATE;
        // Original deletion excludes the camera's remaining work immediately.
        // Keep that scheduling effect even when real GPU/Photo ownership delays
        // physical deletion: pause the actual updater and every native process,
        // then hide the actual scheduling object from already-prepared passes.
        auto stopped=isle_game_pause_updates(services.game,reference_owner,1);
        if(stopped!=ISLE_GAME_OK) return stopped;
        stopped=isle_game_pause_object_processes(services.game,reference_owner,1);
        if(stopped!=ISLE_GAME_OK) return stopped;
        IsleGameObjectMetadata metadata{};
        stopped=isle_game_object_metadata(services.game,reference_owner,&metadata);
        if(stopped!=ISLE_GAME_OK) return stopped;
        stopped=isle_game_object_set_flags(services.game,reference_owner,metadata.flags|1u);
        if(stopped!=ISLE_GAME_OK) return stopped;
        main_removal_requested=true;
        return ISLE_GAME_OK;
    }
    void adopt_effect_object(actor::Owner& owner,IsleGameObject* object,bool particle) {
        require((!actors||actors==&owner)&&source.camera&&reference_owner&&config.effect_resources&&
                !main_removal_requested,
                "source FX adoption needs its actual live main camera and cold FX resources");
        auto** field=particle?&source.D_80382D10_523120:&source.D_80382D14_523124;
        require(object&&object==(particle?owner.particle_object():owner.effect_object())&&
                !*field&&!find(object),
                "source FX adoption requires one unpublished exact factory result");
        auto* receiver=find(reference_owner);
        require(receiver&&!receiver->retiring&&!receiver->joined&&receiver->attachment==source.camera,
                "source FX adoption has no live canonical receiver");
        free_record();
        IsleGameObjectMetadata metadata{};
        native(isle_game_object_metadata(services.game,object,&metadata),"FX helper identity rejected");
        require(metadata.id==static_cast<std::uint32_t>(particle?-6:-7),
                "FX helper is not the exact source -6/-7 factory result");
        IsleGameCamera* attached=nullptr;
        native(isle_game_attach_camera(services.game,object,&attached),"FX native scheduling camera attachment failed");
        Camera camera{object,source.camera,isle_player_render_sprite_camera,nullptr,4,0,0,0,UINT32_MAX};
        auto& record=register_camera(camera,false,!particle);
        record.effects=particle;
        // source_reference_assign obtains the sender's real field credit before
        // publishing the pointer. The immediately following original C store
        // stores the same value and must not be reconciled/credited a second time.
        require(services.source_reference_assign(reference_owner,field,object),
                "actual source FX canonical reference adoption failed");
        actors=&owner;
    }
    void cold_binding() {
        require(!frame&&!sink&&!started&&!reserved_frame&&(!config.photos||config.photos->quiescent()),
            "source camera binding must precede reserve and every Frame dispatch");
    }
    void reserve_arena(sprite::RamRange arena) {
        require(arena.size&&!(arena.address&7u)&&!(arena.size&7u)&&
            arena.address<=0x800000u&&arena.size<=0x800000u-arena.address,
            "source camera command arena exceeds aligned physical RAM",ISLE_GAME_INVALID_ARGUMENT);
        for(const auto owned:{config.lanes,config.sprite_commands,config.effect_commands})
            require(!overlaps(arena,owned),"Source message/transition workspace aliases renderer commands");
        for(const auto framebuffer:config.resources.framebuffers)
            require(!overlaps(arena,{framebuffer,320u*240u*2u}),"Source message/transition workspace aliases a framebuffer");
        bool covered=false;
        for(const auto range:reservations) {
            if(range.address<=arena.address&&std::uint64_t(arena.address)+arena.size<=std::uint64_t(range.address)+range.size)
                covered=true;
            else require(!overlaps(arena,range),"source camera arena partially overlaps another producer reservation");
        }
        if(!covered) reservations.push_back(arena);
        config.resources.reservations=reservations;
    }
    IsleGameResult collect_record(Record& record) noexcept {
        // Keep foreign primary-owner and secondary-observer userdata intact
        // until that owner really releases its joined Services record.
        if(record.identity_held&&services.source_owner_registered(record.metadata.scheduling_object))
            return ISLE_GAME_INVALID_STATE;
        if(record.registered&&!services.source_owner_collectable(record.metadata.scheduling_object))
            return ISLE_GAME_INVALID_STATE;
        if(record.registered&&!services.source_owner_release(record.metadata.scheduling_object))
            return ISLE_GAME_INVALID_STATE;
        if(record.identity_held) {
            const auto released=isle_game_object_release_identity(services.game,record.metadata.scheduling_object);
            if(released!=ISLE_GAME_OK) return released;
        }
        if(reference_owner==record.metadata.scheduling_object) reference_owner=nullptr;
        record={};
        if(&record==transition_record) {
            record.owner=this;record.transition=true;record.transition_owner=transition_owner;
        }
        return ISLE_GAME_OK;
    }
    void refresh_transition() {
        if(!transitions) return;
        auto& record=*transition_record;auto& owner=*transition_owner;
        if(record.active&&record.joined)
            native(collect_record(record),"joined Source transition camera still has actual owner borrowers");
        native(owner.source.status,"Source screen transition retains its first failure");
        transition_pass=transitions->pass(owner);
        if(!transition_pass.camera) {
            require(!record.active,"Source transition camera disappeared without its native owner joining");
            for(auto* object=owner.objects;object;object=object->next)
                require(object->joined,"live Source transition geometry has no actual camera");
            return;
        }
        require(owner.source.camera_object,"Source transition camera has no scheduling object");
        if(!record.active) {
            Camera metadata{owner.source.camera_object,transition_pass.camera,nullptr,nullptr,
                static_cast<std::int32_t>(transition_pass.priority),transition_pass.flags,
                transition_pass.background_rgba,static_cast<std::uint32_t>(transition_pass.link_mask),
                transition_pass.camera_tag,transition_pass.tag_match,transition_pass.prepare,transition_pass.userdata};
            auto& adopted=register_camera(metadata,false,false,&record);
            adopted.transition=true;adopted.transition_owner=&owner;
        } else {
            require(!record.retiring&&!record.joined&&record.metadata.scheduling_object==owner.source.camera_object&&
                record.metadata.projection==transition_pass.camera,"Source transition replaced a borrowed scheduling identity");
            require(!(transition_pass.flags&0x50u),"task-flushing transition requires actual multi-task Frame admission");
            record.metadata.flags=transition_pass.flags;
            record.metadata.background=transition_pass.background_rgba;
            record.metadata.link_mask=static_cast<std::uint32_t>(transition_pass.link_mask);
            record.metadata.tag=transition_pass.camera_tag;record.metadata.tag_match=transition_pass.tag_match;
        }
    }
    static bool prepare_provider(void* user,std::span<std::uint32_t> words,std::uint64_t epoch,
                                 const r::Pass& pass,r::Prepared& output) {
        auto& record=*static_cast<Record*>(user);auto& self=*record.owner;
        if(record.message) {
            require(self.message_ui&& &self.message_ui->producer()==self.config.resources.sprites&&
                self.message_ui->source().Msg_CameraObject==record.metadata.scheduling_object&&
                self.message_ui->source().Msg_Camera==pass.camera,
                "actual Source message camera or shared glyph producer changed");
            native(self.message_ui->message_draw(),"complete original C message draw failed");
            const auto draws=self.message_ui->message_draws();
            record.dispatch.sprite_views=draws.size();
            const auto list=self.message_ui->producer().prepare(words,epoch,draws,{},
                self.message_commands,self.message_protected);
            output.lanes[0]={list.address,list.words};return true;
        }
        if(record.transition) {
            require(self.transition_pass.transition_owner==record.transition_owner&&
                self.transition_pass.camera==pass.camera&&self.transition_pass.prepare,
                "Source transition lost its actual owner or preparation recipe");
            return self.transition_pass.prepare(self.transition_pass.userdata,words,epoch,self.transition_pass,output);
        }
        if(record.effects) {
            require(self.actors&&self.config.effect_resources,"source FX renderer resources missing");
            menu::EffectCamera projection;
            require(menu::source_effect_camera(self.services.game,record.metadata.projection,projection),"source FX main projection rejected");
            const auto arena=self.config.effect_commands;
            require(arena.size>=(menu::EffectRuntime::max_render_words+2)*4&&arena.address/4<=words.size()&&
                arena.size/4<=words.size()-arena.address/4,"source FX command arena exceeds cold plan");
            const auto result=self.actors->effects().render(projection,
                {words.subspan(arena.address/4,menu::EffectRuntime::max_render_words),
                 self.config.effect_resources->bindings(),self.config.effect_resources->reservations()});
            require(bool(result.status)&&!(result.words&1u)&&result.words<=menu::EffectRuntime::max_render_words,
                "actual source FX render failed");
            w::Memory memory(words);memory.word(arena.address+static_cast<std::uint32_t>(result.words*4),0xdf000000u);
            memory.word(arena.address+static_cast<std::uint32_t>(result.words*4+4),0);
            output.lanes[0]={arena.address,result.words+2};return true;
        }
        const auto prepare=record.metadata.prepare?record.metadata.prepare:self.config.prepare;
        void* provider_user=record.metadata.prepare?record.metadata.prepare_user:self.config.prepare_user;
        require(prepare&&provider_user,"source camera has no actual compiled provider");
        return prepare(provider_user,words,epoch,pass,output);
    }
    static bool dispatch(void* user,g::SourceControl& control) {
        auto& record=*static_cast<Record*>(user);auto& self=*record.owner;
        self.ready();
        if(record.retiring||record.joined||!record.metadata.projection) return true;
        IsleGameObjectMetadata metadata{};
        native(isle_game_object_metadata(self.services.game,record.metadata.scheduling_object,&metadata),"scheduled camera visibility lookup failed");
        if(metadata.flags&1u) return true;
        Sink sink(self,record,control);
        if(record.message) {
            // Msg_DrawMessage is a distinct original callback, not main render:
            // its C parser/counter and real glyph lane0 run once, with no Photo
            // prefix or ProcessImage call. Frame owns the shared Producer lease.
            sink.cursor[0]=control.draw_into(sink.cursor[0],sink.end[0]);
            sink.initialized=true;self.refresh(record);sink.posted=true;return true;
        }
        if(record.transition) {
            self.render(record.metadata.scheduling_object);return true;
        }
        if(record.effects) {
            if(record.metadata.flags&0x20u) {
                sink.load(0,self.config.program_selection.alternate);
                self.config.program_selection.alternate_active=true;
            }
            sink.cursor[0]=control.draw_into(sink.cursor[0],sink.end[0]);sink.initialized=true;
            self.refresh(record);sink.posted=true;return true;
        }
        // Every first callback, including source sprite render, enters a C-only
        // guard. A backend's nested preRender uses another C-only guard.
        native(isle_player_render_invoke_callback(&self.source,record.metadata.render,
            record.metadata.scheduling_object),"original C camera callback failed");
        require(sink.posted,"original camera callback returned without its actual source post phase");
        return true;
    }
    void emit(unsigned lane,std::uint32_t first,std::uint32_t second) {
        require(sink,"source GBI leaf has no active stack sink");sink->command(lane,first,second);
    }
    void initialize(IsleGameCamera* value,unsigned lane,int mode) {
        require(sink&&lane==0&&mode==0&&!sink->initialized&&value==sink->record.metadata.projection,
            "source camera initialization is not the actual current lane0 camera");
        if(sink->record.metadata.flags&0x20u) {
            sink->load(0,config.program_selection.alternate);
            config.program_selection.alternate_active=true;
        }
        sink->record.dispatch.prefix={sink->begin[0],(sink->cursor[0]-sink->begin[0])/4u};
        sink->cursor[0]=sink->control.initialize_camera_into(sink->cursor[0],sink->end[0]);sink->initialized=true;
    }
    void render(IsleGameObject* object) {
        require(sink&&sink->record.metadata.scheduling_object==object&&!sink->prepared&&!sink->posted,
            "source camera render does not match its active scheduling object");
        auto& current=*sink;auto& record=current.record;
        if(!current.initialized) initialize(record.metadata.projection,0,0);
        if(!record.transition)
            native(isle_game_camera_compute_all(services.game,record.metadata.projection),"source camera matrix compute failed");
        if(record.metadata.prerender) {
            record.dispatch.prerender_order[record.dispatch.prerender_count++]=0;
            native(isle_player_render_invoke_prerender(&source,record.metadata.prerender,
                record.metadata.projection,0),"source preRender lane0 failed");
        }
        if(!record.transition) {
            lighting.install(std::as_writable_bytes(current.control.ram()));
            lighting.install_direction(std::as_writable_bytes(current.control.ram()),direction);
        }
        if(record.metadata.flags&0x20u) current.restore(1);
        current.lists=current.control.prepare();current.prepared=true;
        for(unsigned lane=0;lane<4;++lane) if(current.lists.lanes[lane].words) {
            if(lane&&(lane!=1||!(record.metadata.flags&0x20u))) current.restore(lane);
            if(lane&&record.metadata.prerender) {
                record.dispatch.prerender_order[record.dispatch.prerender_count++]=static_cast<std::uint8_t>(lane);
                native(isle_player_render_invoke_prerender(&source,record.metadata.prerender,
                    record.metadata.projection,static_cast<int>(lane)),"source secondary preRender failed");
            }
            // Restore the producer's canonical camera/matrix state on every
            // populated secondary lane, never repeat depth/color initialization.
            current.call(lane,current.lists.camera);current.call(lane,current.lists.lanes[lane]);
        }
        post(record.metadata.projection);
    }
    void post(IsleGameCamera* value);
    void append(Record&,unsigned,std::uint32_t,std::uint32_t,w::Memory&);
    void refresh(Record&) noexcept;
    void link_zero(Record&,w::Memory&);
    void publish_aux(Record&,w::Memory&);
    w::CommandRange ensure_reset(g::SourceControl&,w::Memory&,Record&);
    void join(g::SourceControl&,w::Memory&,Record&,bool final);
    static bool finalize(void*,g::SourceControl&);
    static void completed_memory(void* user,rsp::CanonicalExecutor::MemoryLease& lease,std::uint32_t framebuffer) {
        auto& self=*static_cast<Impl*>(user);
        require(self.frame,"completed source mapping has no actual Frame");
        if(self.config.photos)
            native(self.config.photos->completed_memory(*self.frame,lease,framebuffer,
                self.postprocess_registered&&self.presentation_task),"source Photo completed-memory analysis failed");
        if(self.downstream.function) self.downstream.function(self.downstream.context,lease,framebuffer);
    }
};
void Renderer::Impl::refresh(Record& record) noexcept {
    for(unsigned lane=0;lane<4;++lane)
        record.dispatch.lanes[lane]=record.wire_cursor[lane]>record.wire_begin[lane]?
            r::List{record.wire_begin[lane],(record.wire_cursor[lane]-record.wire_begin[lane])/4u}:r::List{};
}
void Renderer::Impl::append(Record& record,unsigned lane,std::uint32_t first,std::uint32_t second,w::Memory& memory) {
    require(lane<4&&record.wire_cursor[lane]<=record.wire_end[lane]-8,
        "source continuation or task tail exceeds cold lane capacity",ISLE_GAME_EXHAUSTED);
    memory.word(record.wire_cursor[lane],first);memory.word(record.wire_cursor[lane]+4,second);
    record.wire_cursor[lane]+=8;refresh(record);
}
void Renderer::Impl::link_zero(Record& record,w::Memory& memory) {
    if(wire_tails[0]) append(*wire_tails[0],0,0xde010000u,record.wire_begin[0],memory);
    else wire_heads[0]=&record;
    wire_tails[0]=&record;
}
void Renderer::Impl::publish_aux(Record& record,w::Memory& memory) {
    for(unsigned lane=1;lane<4;++lane) if(record.wire_cursor[lane]!=record.wire_begin[lane]) {
        if(wire_tails[lane]) append(*wire_tails[lane],lane,0xde010000u,record.wire_begin[lane],memory);
        else wire_heads[lane]=&record;
        wire_tails[lane]=&record;
    }
    refresh(record);
}
w::CommandRange Renderer::Impl::ensure_reset(g::SourceControl& control,w::Memory& memory,Record& record) {
    if(!record.reset_count) {
        struct Cursor {
            w::Memory& memory;std::uint32_t cursor,end;
            static void emit(void* user,std::uint32_t first,std::uint32_t second) {
                auto& self=*static_cast<Cursor*>(user);
                require(self.cursor<=self.end-8,"source reset exceeds its cold reservation",ISLE_GAME_EXHAUSTED);
                self.memory.word(self.cursor,first);self.memory.word(self.cursor+4,second);self.cursor+=8;
            }
        } cursor{memory,record.reset_address,record.reset_address+0x100};
        control.reset_program_state(&cursor,Cursor::emit);Cursor::emit(&cursor,0xdf000000u,0);
        record.reset_count=(cursor.cursor-record.reset_address)/8u;
        record.dispatch.reset={record.reset_address,record.reset_count*2u};
    }
    return {record.reset_address,record.reset_count};
}
void Renderer::Impl::join(g::SourceControl& control,w::Memory& memory,Record& reset_record,bool final) {
    std::array<bool,4> populated{};
    for(unsigned lane=0;lane<4;++lane) populated[lane]=wire_heads[lane]!=nullptr;
    // Every admitted Player/create/FX/UI camera emits actual lane0 init or
    // scalar packets. Independently admitted task8 remains available in Input.
    require(populated[0],"source camera driver lost its actual lane0 task head");
    const auto head=[&](unsigned lane) {return wire_heads[lane]->wire_begin[lane];};
    const auto cursor=[&](unsigned lane) {return wire_tails[lane]->wire_cursor[lane];};
    const auto emit=[&](unsigned lane,std::uint32_t first,std::uint32_t second) {
        append(*wire_tails[lane],lane,first,second,memory);
    };
    const auto load=[&](unsigned lane,w::SourceProgram program) {
        const auto packet=config.programs->load(program,ensure_reset(control,memory,reset_record));
        for(std::size_t i=0;i<packet.word_count;i+=2) emit(lane,packet.words[i],packet.words[i+1]);
    };
    const auto branch=[&](unsigned lane,std::uint32_t destination) {emit(lane,0xde010000u,destination);};
    r::List terminal{};
    const auto finish=[&](unsigned lane) {
        terminal={cursor(lane),4};emit(lane,0xe9000000u,0);emit(lane,0xdf000000u,0);
    };
    const auto selection=config.program_selection;
    const auto primary=selection.primary,line=w::source_line_program(selection);
    // Pinned gtlCombine552–616 / gtlProcess447–550 share these branch choices,
    // but NOT their tails. Preserve the literal lane1->3 load into lane0 and
    // then-current cursor0 addresses, including the unreachable appended load.
    if(populated[2]) {load(0,line);branch(0,head(2));}
    else if(populated[1]) {if(selection.alternate_active) load(0,primary);branch(0,head(1));}
    else if(populated[3]) {load(0,line);branch(0,head(3));}
    else if(final) finish(0);
    if(populated[2]) {
        if(populated[1]) {load(2,primary);branch(2,head(1));}
        else if(populated[3]) branch(2,head(3));
        else if(final) finish(2);
        else {load(2,primary);branch(2,cursor(0));}
    }
    if(populated[1]) {
        if(populated[3]) {load(0,line);branch(1,head(3));}
        else if(final) finish(1);
        else {if(selection.alternate_active) load(1,primary);branch(1,cursor(0));}
    }
    if(populated[3]) {
        if(final) finish(3);
        else {load(3,primary);branch(3,cursor(0));}
    }
    if(final) {
        // Native equivalence of gtlProcess's post-END reset/branch task entry:
        // the shared Frame root already performs that exact reset body once,
        // then calls this NOPUSH-linked source0 chain. Its terminal returns to
        // the outer ENDDL; finish_task_group omits a duplicate FullSync.
        control.finish_task_group(wire_heads[0]->dispatch.lanes[0],terminal);
    } else {
        // Source saved0 deliberately stays at the frame's source0 head. Its
        // current tail/return slot is linked to the next scheduling camera.
        for(unsigned lane=1;lane<4;++lane) wire_heads[lane]=wire_tails[lane]=nullptr;
        config.program_selection.alternate_active=false;
    }
}
void Renderer::Impl::post(IsleGameCamera* value) {
    require(sink&&sink->initialized&&!sink->posted&&value==sink->record.metadata.projection,
        "source post does not match current initialized scheduling camera");
    auto& current=*sink;
    require(current.cursor[0]!=current.begin[0],"source camera returned without actual lane0 commands");
    publish_aux(current.record,current.memory);
    if(current.record.metadata.flags&4u) {
        join(current.control,current.memory,current.record,false);
        current.record.dispatch.combined=true;
    }
    current.posted=true;
}
bool Renderer::Impl::finalize(void* user,g::SourceControl& control) {
    auto& self=*static_cast<Impl*>(user);
    try {
        self.ready();require(!self.sink,"source callback outlived its final wire boundary");
        if(self.wire_heads[0]) {
            w::Memory memory(control.ram());
            self.join(control,memory,*self.wire_tails[0],true);
            self.presentation_task=true;
        }
        return true;
    } catch(const Failure& failure) {self.fail(failure.result,failure.message);}
      catch(const std::exception& failure) {self.fail(ISLE_GAME_EXECUTION_FAILURE,failure.what());}
      catch(...) {self.fail(ISLE_GAME_EXECUTION_FAILURE,"source final wire failure");}
    return false;
}
struct RendererAccess {
    static Renderer::Impl* owner(IslePlayerSource* source) noexcept {
        if(!source||!source->native_renderer) return nullptr;
        auto* self=static_cast<Renderer::Impl*>(source->native_renderer);
        return &self->source==source&&!self->detached?self:nullptr;
    }
    template<class F> static IsleGameResult invoke(IslePlayerSource* source,F&& function) noexcept {
        auto* self=owner(source);if(!self) return ISLE_GAME_INVALID_STATE;
        try { self->ready();function(*self);return ISLE_GAME_OK; }
        catch(const Failure& failure) { self->fail(failure.result,failure.message);return failure.result; }
        catch(const std::exception& failure) { self->fail(ISLE_GAME_EXECUTION_FAILURE,failure.what());return self->error; }
        catch(...) { self->fail(ISLE_GAME_EXECUTION_FAILURE,"source renderer unknown C++ failure");return self->error; }
    }
};
Renderer::Renderer(scene::Services& services,IslePlayerSource& source,const photo::LightingInputs& light,Config config) try :
    impl_(std::make_unique<Impl>(services,source,light,config)) {}
catch(const Failure& failure) {
    throw std::invalid_argument(failure.message);
}
Renderer::~Renderer() { if(impl_&&!impl_->detached) std::terminate(); }
IsleGameResult Renderer::create_main(std::int32_t background,IsleGameObject** output) noexcept {
    if(!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output=nullptr;
    const auto result=isle_player_render_invoke_create(&impl_->source,background,output);
    if(*output) impl_->ui=*output;
    if(result!=ISLE_GAME_OK) impl_->fail(result,"complete original C camera creation failed");
    return result;
}
IsleGameResult Renderer::attach(const Camera& camera) noexcept {
    return RendererAccess::invoke(&impl_->source,[&](Impl& self){self.register_camera(camera,false);});
}
IsleGameResult Renderer::adopt_particle(actor::Owner& actors,IsleGameObject* object) noexcept {
    return RendererAccess::invoke(&impl_->source,[&](Impl& self){self.adopt_effect_object(actors,object,true);});
}
IsleGameResult Renderer::adopt_effect(actor::Owner& actors,IsleGameObject* object) noexcept {
    return RendererAccess::invoke(&impl_->source,[&](Impl& self){self.adopt_effect_object(actors,object,false);});
}
IsleGameResult Renderer::adopt_effects(actor::Owner& actors) noexcept {
    return RendererAccess::invoke(&impl_->source,[&](Impl& self) {
        require(!self.actors&&actors.particle_object()&&actors.effect_object(),
                "ActorOwner has not published both actual -6/-7 helpers");
        self.adopt_effect_object(actors,actors.particle_object(),true);
        self.adopt_effect_object(actors,actors.effect_object(),false);
    });
}
IsleGameResult Renderer::set_near_clipping_disabled(bool disabled) noexcept {
    return RendererAccess::invoke(&impl_->source,[&](Impl& self) {
        require(!self.frame&&!self.sink&&!self.preparing&&!self.main_removal_requested&&
                (!self.config.photos||self.config.photos->quiescent()),
                "source near-clipping setup crossed actual graphics/Photo ownership");
        auto selection=self.config.program_selection;
        selection.disable_near_clipping=disabled;
        static_cast<void>(self.config.programs->descriptor(w::source_task_program(selection,false)));
        self.config.program_selection=selection;
    });
}
IsleGameObject* Renderer::ui_camera_object() const noexcept { return impl_->ui; }
IsleGameResult Renderer::metadata(IsleGameObject* object,Camera& output) const noexcept {
    const auto* record=impl_->find(object);if(!record||record->retiring) return ISLE_GAME_INVALID_ARGUMENT;
    output=record->metadata;return ISLE_GAME_OK;
}
IsleGameResult Renderer::dispatch_metadata(IsleGameObject* object,Dispatch& output) const noexcept {
    const auto* record=impl_->find(object);if(!record) return ISLE_GAME_INVALID_ARGUMENT;
    output=record->dispatch;return ISLE_GAME_OK;
}
IsleGameResult Renderer::bind_photos(Controller& photos) noexcept {
    return RendererAccess::invoke(&impl_->source,[&](Impl& self) {
        require(!self.frame&&!self.sink&&!self.config.photos,"source Photo controller is already bound or borrowed");
        self.config.photos=&photos;
        self.postprocess_registered=true;
    });
}
IsleGameResult Renderer::bind_cart(cart::Owner& owner,std::span<scene::RenderOrderEntry> scratch) noexcept {
    return RendererAccess::invoke(&impl_->source,[&](Impl& self) {
        require(!self.frame&&!self.sink&&!self.cart_owner&&!scratch.empty(),"Cart cold binding is already borrowed or lacks order capacity");
        self.cart_owner=&owner;self.cart_order=scratch;
    });
}
IsleGameResult Renderer::bind_ui(Ui& ui,sprite::RamRange commands) noexcept {
    return RendererAccess::invoke(&impl_->source,[&](Impl& self) {
        self.cold_binding();
        auto& source=ui.source();
        require(!self.message_ui&&source.player==&self.source&&source.owner==&ui&&
            source.Msg_CameraObject&&source.Msg_Camera&& &ui.producer()==self.config.resources.sprites,
            "Source message binding requires the actual late Ui camera and shared glyph producer");
        native(source.status,"Source Ui retains its first failure");
        IsleGameObjectMetadata object{};
        native(isle_game_object_metadata(self.services.game,source.Msg_CameraObject,&object),"actual Source message object is not live");
        scene::RenderMetadata render;
        require(object.id==29&&object.update_link==0&&object.priority==0x80000000u&&
            self.services.render_metadata(source.Msg_CameraObject,render)&&render.link==0&&
            render.priority==3&&render.camera_tag==0,"Source message camera construction metadata changed");
        require(!self.transitions||!overlaps(commands,screen_transition::Resources::workspace),
            "Source glyph commands alias transition workspace");
        auto& slot=self.free_record();
        self.reserve_arena(commands);
        Camera camera{source.Msg_CameraObject,source.Msg_Camera,nullptr,nullptr,3,0,0,0,0,
            r::TagMatch::intersection,Impl::prepare_provider,nullptr};
        auto& record=self.register_camera(camera,false,false,&slot);record.message=true;
        self.message_ui=&ui;self.message_commands=commands;
        // Original Msg_InitCamera runs after the main/UI constructors. Bind at
        // that point, before any later priority3 camera: original equal3 appends.
    });
}
IsleGameResult Renderer::bind_transition(screen_transition::Resources& resources,screen_transition::Owner& owner) noexcept {
    return RendererAccess::invoke(&impl_->source,[&](Impl& self) {
        self.cold_binding();
        require(!self.transitions&& &owner.services==&self.services&&owner.source.game==self.services.game&&
            owner.source.owner==&owner,"Source transition binding requires its actual cold owner");
        native(owner.source.status,"Source transition retains its first failure");
        require(!self.message_ui||!overlaps(self.message_commands,screen_transition::Resources::workspace),
            "Source transition workspace aliases glyph commands");
        auto& record=self.free_record();
        self.reserve_arena(screen_transition::Resources::workspace);
        self.immutable.assign(self.config.resources.immutable.begin(),self.config.resources.immutable.end());
        for(const auto span:resources.immutable()) {
            bool covered=false;
            for(const auto existing:self.immutable) if(existing.address<=span.address&&
                std::uint64_t(span.address)+span.bytes.size()<=std::uint64_t(existing.address)+existing.bytes.size()&&
                existing.bytes.data()+(span.address-existing.address)==span.bytes.data()) covered=true;
            if(!covered) self.immutable.push_back(span);
        }
        self.config.resources.immutable=self.immutable;
        self.transitions=&resources;self.transition_owner=&owner;self.transition_record=&record;
        record.owner=&self;record.transition=true;record.transition_owner=&owner;
        // Reserve one of the SAME32 camera slots now, without creating a camera
        // or claiming a draw before the original Source constructor runs.
        self.refresh_transition();
    });
}
IsleGameResult Renderer::set_postprocess(bool enabled) noexcept {
    auto& self=*impl_;
    IsleGameCounts counts{};
    const auto owner=isle_game_counts(self.services.game,&counts);
    if(owner!=ISLE_GAME_OK) return owner;
    if(self.detached || self.source.native_renderer!=&self || !self.source.owner ||
       self.source.game!=self.services.game) return ISLE_GAME_INVALID_STATE;
    // SC registration remains available for real cleanup after a sticky failure;
    // applying it neither clears that failure nor fabricates a failed GPU token.
    self.postprocess_registered=enabled;
    return ISLE_GAME_OK;
}
std::int32_t Renderer::drawn_frame_counter() const noexcept {
    return std::bit_cast<std::int32_t>(impl_->drawn_frames);
}
bool Renderer::reserve(g::Frame& frame) noexcept {
    auto& self=*impl_;
    try {
        self.ready();
        require(!self.frame&&!self.sink&&!self.preparing&&(!self.config.photos||self.config.photos->quiescent()),
            "Source render reservations cannot cross actual Frame/Photo/dispatch borrowers");
        self.reserved_frame=&frame;
        if(self.message_ui) {
            self.message_protected.clear();
            const auto append=[&](sprite::RamRange range) {
                const auto end=std::uint64_t(range.address)+range.size;
                const auto own_end=std::uint64_t(self.message_commands.address)+self.message_commands.size;
                if(!overlaps(range,self.message_commands)) self.message_protected.push_back(range);
                else {
                    if(range.address<self.message_commands.address)
                        self.message_protected.push_back({range.address,self.message_commands.address-range.address});
                    if(own_end<end)
                        self.message_protected.push_back({static_cast<std::uint32_t>(own_end),static_cast<std::uint32_t>(end-own_end)});
                }
            };
            for(const auto range:self.reservations) append(range);
            for(const auto range:frame.reservations()) append(range);
            for(const auto framebuffer:self.config.resources.framebuffers) append({framebuffer,320u*240u*2u});
        }
        return frame.reserve_render_capacity({self.config.sprite_capacity,self.reservations.size()});
    } catch(const Failure& failure) {self.fail(failure.result,failure.message);}
      catch(const std::exception& failure) {self.fail(ISLE_GAME_EXECUTION_FAILURE,failure.what());}
      catch(...) {self.fail(ISLE_GAME_EXECUTION_FAILURE,"Source render reservation exception");}
    return false;
}
bool Renderer::begin(g::Frame& frame,g::FrameContext context,g::CompletedWork work) noexcept {
    auto& self=*impl_;
    try {
        self.ready();require(!self.frame&&!self.sink&&!self.preparing&&frame.quiescent(),"source Frame/camera storage still borrowed");
        require(!self.main_removal_requested,"source main camera removal already requested");
        save::ScreenOffsets offsets;
        require(self.services.state.screen_offsets().translation(offsets),
            "source live VI endpoints cannot be represented by native translation");
        self.started=true;
        require(!self.message_ui||self.reserved_frame==&frame,"Source glyph preparation requires its actual cold Frame reservations");
        self.refresh_transition();
        for(const auto core:frame.reservations()) for(const auto range:self.reservations)
            require(!overlaps(core,range),"source cold plan overlaps Frame task/depth storage");
        self.frame=&frame;self.downstream=work;
        self.wire_heads.fill(nullptr);self.wire_tails.fill(nullptr);
        self.presentation_task=false;
        // gtlInitDLists173 resets only this source program selector.
        self.config.program_selection.alternate_active=false;
        if(self.cart_owner) {
            const auto bindings=self.cart_owner->render_bindings(self.cart_order);
            require(!bindings.empty(),"actual Cart render bindings were not refreshed before graphics borrow");
            self.cart_borrow.emplace(self.cart_owner->borrow());
            require(bool(*self.cart_borrow),"actual Cart graphics borrow rejected");
        }
        std::size_t count=0;
        // Pinned ohCreateCamera/omLinkGObjDLCamera appends after equal priority.
        // Frame stably sorts priorities; equal cameras keep Source insertion.
        std::array<Impl::Record*,camera_capacity> order{};
        for(auto& record:self.records) if(record.active&&!record.retiring&&!record.helper&&record.metadata.projection) {
            auto n=count;while(n&&order[n-1]->insertion>record.insertion) {order[n]=order[n-1];--n;}
            order[n]=&record;++count;
        }
        for(std::size_t n=0;n<count;++n) {
            auto& record=*order[n];auto& pass=self.passes[n];const auto& camera=record.metadata;
            pass=record.transition?self.transition_pass:r::Pass{};
            pass.camera=camera.projection;pass.priority=static_cast<std::uint32_t>(camera.priority);
            pass.flags=camera.flags;pass.background_rgba=camera.background;pass.link_mask=camera.link_mask;
            pass.camera_tag=camera.tag;pass.tag_match=camera.tag_match;pass.userdata=&record;pass.control=Impl::dispatch;
            const bool sprites=camera.render==isle_player_render_sprite_camera&&!record.effects;
            pass.kind=sprites?r::Kind::sprites:r::Kind::compiled;
            pass.prepare=sprites?nullptr:Impl::prepare_provider;
            const auto sprite_bytes=(self.config.sprite_commands.size/static_cast<std::uint32_t>(camera_capacity))&~7u;
            pass.sprite_commands={self.config.sprite_commands.address+
                static_cast<std::uint32_t>(&record-self.records.data())*sprite_bytes,sprite_bytes};
            record.kind=pass.kind;
        }
        if(self.config.photos) { native(self.config.photos->begin_preparation(frame),"source Photo preparation admission failed");self.preparing=true; }
        const bool begun=frame.begin({self.services.game,self.config.resources,
            std::span<const r::Pass>(self.passes).first(count),offsets,
            w::source_task_program(self.config.program_selection,false),Impl::finalize,&self},context,{&self,Impl::completed_memory});
        IsleGameResult ended=ISLE_GAME_OK;
        if(self.preparing) { ended=self.config.photos->end_preparation();self.preparing=false; }
        if(!begun||ended!=ISLE_GAME_OK) {
            if(self.config.photos&&self.config.photos->has_pending(frame))
                native(self.config.photos->retire_unsubmitted(frame),"failed source preparation Photo retirement rejected");
            self.cart_borrow.reset();self.frame=nullptr;self.downstream={};native(ended,"source Photo collector restoration failed");
            self.fail(ISLE_GAME_EXECUTION_FAILURE,frame.message());return false;
        }
        if(self.config.photos&&(self.config.photos->has_pending(frame)||
           (self.presentation_task&&self.config.photos->queued_requests()))) {
            IslePlayerPhotoTicket ticket{};native(self.config.photos->submitted(frame,ticket),"source Photo actual submission binding failed");
        }
        // The admitted standard whole draw has returned, once for all cameras.
        // Unsigned arithmetic defines the original signed32 equality/wrap domain.
        ++self.drawn_frames;
        return true;
    } catch(const Failure& failure) {self.fail(failure.result,failure.message);}
      catch(const std::exception& failure) {self.fail(ISLE_GAME_EXECUTION_FAILURE,failure.what());}
      catch(...) {self.fail(ISLE_GAME_EXECUTION_FAILURE,"source Frame preparation exception");}
    if(self.preparing) { const auto restored=self.config.photos->end_preparation();self.preparing=false;if(restored!=ISLE_GAME_OK) self.fail(restored,"source Photo collector cleanup failed"); }
    if(self.frame&&!self.frame->busy()) {
        if(self.config.photos&&self.config.photos->has_pending(*self.frame)) {
            const auto retired=self.config.photos->retire_unsubmitted(*self.frame);
            if(retired!=ISLE_GAME_OK) self.fail(retired,"source Photo unsubmitted cleanup failed");
        }
        self.cart_borrow.reset();self.frame=nullptr;
    }
    return false;
}
g::Poll Renderer::poll() noexcept {
    auto& self=*impl_;if(!self.frame) return g::Poll::idle;
    const auto result=self.frame->poll();
    if(result==g::Poll::completed) {
        self.cart_borrow.reset();
        if(self.config.photos) {
            const auto completed=self.config.photos->completed(*self.frame);
            if(completed!=ISLE_GAME_OK) {self.fail(completed,"source Photo actual completed poll rejected");return g::Poll::failed;}
        }
        if(!self.config.photos||!self.config.photos->has_pending(*self.frame)) {self.frame=nullptr;self.downstream={};}
    } else if(result==g::Poll::failed&&self.frame->quiescent()) {
        self.cart_borrow.reset();
        if(self.config.photos&&self.config.photos->has_pending(*self.frame)) {
            const auto retired=self.config.photos->retire_failed(*self.frame);
            if(retired!=ISLE_GAME_OK) {self.fail(retired,"source Photo actual failed token retirement rejected");return result;}
        }
        self.fail(ISLE_GAME_EXECUTION_FAILURE,self.frame->message());self.frame=nullptr;self.downstream={};
    }
    return result;
}
IsleGameResult Renderer::release_completed() noexcept {
    auto& self=*impl_;
    if(!self.frame||!self.frame->quiescent()||!self.frame->completed()) return ISLE_GAME_INVALID_STATE;
    if(self.config.photos&&self.config.photos->has_pending(*self.frame)) {
        const auto result=self.config.photos->retire(*self.frame);if(result!=ISLE_GAME_OK) return result;
    }
    self.frame=nullptr;self.downstream={};return ISLE_GAME_OK;
}
IsleGameResult Renderer::remove_main() noexcept {
    auto& self=*impl_;
    IsleGameCounts counts{};
    const auto access=isle_game_counts(self.services.game,&counts);
    if(access!=ISLE_GAME_OK) return access;
    if(self.detached||self.source.native_renderer!=&self) return ISLE_GAME_INVALID_STATE;
    if(self.main_removed) return ISLE_GAME_OK;
    if(!self.main_removal_requested) {
        if(!self.source.camera||!self.reference_owner) return ISLE_GAME_INVALID_STATE;
        const auto stopped=self.stop_main();
        if(stopped!=ISLE_GAME_OK) return stopped;
    }
    // This is a logical request, not a synthetic Source deletion. Consumer
    // ownership and all canonical field credits stay intact until actual idle.
    if(self.frame||self.sink||self.preparing||self.source.nonwaiting_call_frame||
       (self.config.photos&&!self.config.photos->quiescent())) return ISLE_GAME_OK;
    const auto idle=isle_game_require_idle(self.services.game);
    if(idle==ISLE_GAME_INVALID_STATE) return ISLE_GAME_OK;
    if(idle!=ISLE_GAME_OK) return idle;
    return service_main_removal();
}
IsleGameResult Renderer::service_main_removal() noexcept {
    auto& self=*impl_;
    const auto idle=isle_game_require_idle(self.services.game);
    if(idle!=ISLE_GAME_OK) return idle;
    if(self.detached||self.source.native_renderer!=&self) return ISLE_GAME_INVALID_STATE;
    if(!self.main_removal_requested||self.main_removed) return ISLE_GAME_OK;
    if(self.frame||self.sink||self.preparing||self.source.nonwaiting_call_frame||
       (self.config.photos&&!self.config.photos->quiescent())) return ISLE_GAME_INVALID_STATE;
    const auto removed=isle_player_render_invoke_remove(&self.source);
    if(removed!=ISLE_GAME_OK) return removed;
    auto* record=self.find(self.reference_owner);
    if(self.source.camera||!record||!record->retiring||!record->joined)
        return ISLE_GAME_INVALID_STATE;
    self.main_removed=true;
    return ISLE_GAME_OK;
}
IsleGameResult Renderer::abort_main() noexcept {
    auto& self=*impl_;
    const auto idle=isle_game_require_idle(self.services.game);
    if(idle!=ISLE_GAME_OK) return idle;
    if(self.detached||self.source.native_renderer!=&self) return ISLE_GAME_INVALID_STATE;
    if(self.frame||self.sink||self.preparing||self.source.nonwaiting_call_frame||
       (self.config.photos&&!self.config.photos->quiescent())) return ISLE_GAME_INVALID_STATE;
    if(self.main_removed) return ISLE_GAME_OK;
    if(!self.reference_owner) return self.source.camera?ISLE_GAME_INVALID_STATE:ISLE_GAME_OK;
    auto* record=self.find(self.reference_owner);
    if(!record) return ISLE_GAME_INVALID_STATE;
    if(record->retiring&&record->joined&&!self.source.camera) {
        self.main_removal_requested=true;
        self.main_removed=true;
        return ISLE_GAME_OK;
    }
    if(!self.main_removal_requested) {
        const auto stopped=self.stop_main();
        if(stopped!=ISLE_GAME_OK) return stopped;
    }
    // Cancellation is native teardown, not execution of the ordinary Source
    // removal body. Services closes the real receiver's FX fields and native
    // observers establish the genuine join even after a sticky Source/OM fault.
    // Retain this joined record for collection after its other owners retire.
    self.services.erase(self.reference_owner);
    if(self.source.camera||!record->retiring||!record->joined||!self.main_removed)
        return ISLE_GAME_INVALID_STATE;
    return ISLE_GAME_OK;
}
bool Renderer::main_removal_pending() const noexcept {
    return impl_->main_removal_requested&&!impl_->main_removed;
}
IsleGameResult Renderer::collect_joined() noexcept {
    auto& self=*impl_;
    if(self.frame||self.sink||self.preparing||(self.config.photos&&!self.config.photos->quiescent()))
        return ISLE_GAME_INVALID_STATE;
    const auto idle=isle_game_require_idle(self.services.game);
    if(idle!=ISLE_GAME_OK) return idle;
    const auto removed=service_main_removal();
    if(removed!=ISLE_GAME_OK) return removed;
    for(auto& record:self.records) if(record.active&&record.joined) {
        const auto collected=self.collect_record(record);
        if(collected!=ISLE_GAME_OK) return collected;
    }
    return ISLE_GAME_OK;
}
IsleGameResult Renderer::detach() noexcept {
    auto& self=*impl_;
    if(self.detached) return ISLE_GAME_OK;
    if(self.frame||self.sink||self.preparing||(self.config.photos&&!self.config.photos->quiescent())||
        self.source.nonwaiting_call_frame)
        return ISLE_GAME_INVALID_STATE;
    const auto idle=isle_game_require_idle(self.services.game);
    if(idle!=ISLE_GAME_OK) return idle;
    // On an executable Source path, preserve its genuine removal body. A
    // terminal Source/OM failure forbids entering that C fence; abort teardown
    // still performs real native erase/join below, without clearing the fault
    // or claiming that the Source body executed.
    if(self.source.status==ISLE_GAME_OK&&isle_game_status(self.services.game)==ISLE_GAME_OK) {
        const auto removed=service_main_removal();
        if(removed!=ISLE_GAME_OK) return removed;
    }
    // Actual deletion invokes the composed observer before primary ActorOwner.
    // Source reference cleanup is Services-owned, including after sticky fail.
    for(auto& record:self.records) if(record.active&&!record.retiring) self.services.erase(record.metadata.scheduling_object);
    const auto collected=collect_joined();if(collected!=ISLE_GAME_OK) return collected;
    for(const auto& record:self.records) if(record.active) return ISLE_GAME_INVALID_STATE;
    self.source.native_renderer=nullptr;self.detached=true;return ISLE_GAME_OK;
}
IsleGameResult Renderer::status() const noexcept {return impl_->error;}
const char* Renderer::message() const noexcept {return impl_->diagnostic.data();}
photo::LightDirection Renderer::light_direction() const noexcept {return impl_->direction;}
}

extern "C" {
using isle::player::RendererAccess;
IsleGameResult isle_player_render_backend_dimensions(IslePlayerSource* source,int32_t* width,int32_t* height) {
    if(!width||!height) return ISLE_GAME_INVALID_ARGUMENT;
    *width=0;*height=0;return RendererAccess::invoke(source,[&](auto&){*width=320;*height=240;});
}
IsleGameResult isle_player_render_backend_create_camera(IslePlayerSource* source,const IslePlayerRenderCameraSpec* spec,IsleGameObject** output) {
    if(!spec||!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output=nullptr;
    return RendererAccess::invoke(source,[&](auto& self) {
        isle::player::require(spec->link<32&&spec->render&&spec->process_kind<=1,"source camera spec rejected",ISLE_GAME_INVALID_ARGUMENT);
        self.free_record();
        auto* object=self.services.object(spec->id,spec->link,spec->object_priority);
        isle::player::require(object,"source camera actual native creation failed",ISLE_GAME_EXHAUSTED);
        struct Publication {
            isle::scene::Services& services;
            IsleGameObject* object;
            bool committed=false;
            ~Publication() {if(!committed) services.erase(object);}
        } publication{self.services,object};
        *output=object;IsleGameCamera* camera=nullptr;
        isle::player::Renderer::Camera metadata{object,nullptr,spec->render,nullptr,spec->camera_priority,
            0,0,spec->display_link_mask,static_cast<std::uint32_t>(spec->camera_tag)};
        auto& record=self.register_camera(metadata,true);
        isle::player::native(isle_game_attach_camera(self.services.game,object,&camera),"source camera native attachment failed");
        record.attachment=camera;record.metadata.projection=camera;
        isle::player::require(self.services.source_commands_native(object),"source camera default command scope registration failed");
        isle::player::native(isle_game_object_set_source_update(self.services.game,object,spec->update),"source camera original updater registration failed");
        if(spec->add_default_matrices) {
            isle::player::native(isle_game_camera_add_matrix(self.services.game,camera,ISLE_GAME_CAMERA_PERSPECTIVE_FAST),"source default projection matrix failed");
            isle::player::native(isle_game_camera_add_matrix(self.services.game,camera,ISLE_GAME_CAMERA_LOOK_AT),"source default view matrix failed");
        }
        if(spec->process) { IsleGameProcess* process=nullptr;
            isle::player::native(isle_game_add_source_process(self.services.game,object,spec->process,
                static_cast<IsleGameProcessKind>(spec->process_kind),spec->process_priority,&process),"source camera original process registration failed"); }
        if(spec->default_flags) {record.metadata.flags=7;record.metadata.background=0xff;}
        publication.committed=true;
    });
}
IsleGameResult isle_player_render_backend_camera_flags(IslePlayerSource* source,IsleGameCamera* camera,uint32_t flags) {
    return RendererAccess::invoke(source,[&](auto& self){
        auto& record=self.camera(camera);auto& metadata=record.metadata;
        isle::player::require(!((metadata.flags|flags)&0x50u),"task-flushing source camera requires actual multi-task Frame admission");
        metadata.flags|=flags;
        metadata.tag_match=(metadata.flags&8u)?isle::scene::render::TagMatch::equality:isle::scene::render::TagMatch::intersection;
        for(auto& pass:self.passes) if(pass.userdata==&record) {pass.flags=metadata.flags;pass.tag_match=metadata.tag_match;}
    });
}
IsleGameResult isle_player_render_backend_camera_background(IslePlayerSource* source,IsleGameCamera* camera,uint32_t color) {
    return RendererAccess::invoke(source,[&](auto& self){
        auto& record=self.camera(camera);record.metadata.background=color;
        for(auto& pass:self.passes) if(pass.userdata==&record) pass.background_rgba=color;
    });
}
IsleGameResult isle_player_render_backend_camera_prerender(IslePlayerSource* source,IsleGameCamera* camera,IslePlayerPreRenderCallback callback) {
    return RendererAccess::invoke(source,[&](auto& self){self.camera(camera).metadata.prerender=callback;});
}
IsleGameResult isle_player_render_backend_sprite_camera(IslePlayerSource* source,IsleGameObject* object) {
    return RendererAccess::invoke(source,[&](auto& self) {
        isle::player::require(self.sink&&self.sink->record.metadata.scheduling_object==object&&
            self.sink->record.kind==isle::scene::render::Kind::sprites,"source sprite dispatch does not match actual UI camera");
        std::size_t count=0;
        isle::player::require(self.services.sprite_count()<=self.sprites.size()&&self.services.sprite_draws(self.sprites,count),
            "source late UI sprite projection exceeds cold plan");
        self.sink->record.dispatch.sprite_views=count;
        // Frame borrows this exact mutable pass at dispatch, not an early view.
        for(auto& pass:self.passes) if(pass.userdata==&self.sink->record) pass.sprites=std::span<const isle::scene::SpriteDraw>(self.sprites).first(count);
        if(self.sink->record.metadata.flags&0x20u) {
            self.sink->load(0,self.config.program_selection.alternate);
            self.config.program_selection.alternate_active=true;
        }
        self.sink->cursor[0]=self.sink->control.draw_into(self.sink->cursor[0],self.sink->end[0]);
        self.sink->initialized=true;self.refresh(self.sink->record);self.sink->posted=true;
        for(std::size_t i=0;i<count;++i) self.sprites[i]={};
    });
}
IsleGameResult isle_player_render_backend_pipe_sync(IslePlayerSource* source,uint32_t lane) {
    return RendererAccess::invoke(source,[&](auto& self){self.emit(lane,0xe7000000u,0);});
}
IsleGameResult isle_player_render_backend_cycle(IslePlayerSource* source,uint32_t lane,IslePlayerRenderCycle cycle) {
    return RendererAccess::invoke(source,[&](auto& self){isle::player::require(cycle==ISLE_PLAYER_RENDER_ONE_CYCLE||cycle==ISLE_PLAYER_RENDER_FILL_CYCLE,
        "invalid source cycle",ISLE_GAME_INVALID_ARGUMENT);self.emit(lane,0xe3000a01u,cycle==ISLE_PLAYER_RENDER_FILL_CYCLE?0x300000u:0);});
}
IsleGameResult isle_player_render_backend_mode(IslePlayerSource* source,uint32_t lane,IslePlayerRenderMode mode) {
    return RendererAccess::invoke(source,[&](auto& self){isle::player::require(mode==ISLE_PLAYER_RENDER_NOOP||mode==ISLE_PLAYER_RENDER_AA_ZB_OPAQUE,
        "invalid source render mode",ISLE_GAME_INVALID_ARGUMENT);self.emit(lane,0xe200001cu,mode==ISLE_PLAYER_RENDER_NOOP?0:0x00552078u);});
}
IsleGameResult isle_player_render_backend_scissor(IslePlayerSource* source,uint32_t lane,int32_t xmin,int32_t ymin,int32_t xmax,int32_t ymax) {
    return RendererAccess::invoke(source,[&](auto& self){self.emit(lane,0xed000000u|isle::player::xy(xmin,ymin),isle::player::xy(xmax,ymax));});
}
IsleGameResult isle_player_render_backend_fill_color(IslePlayerSource* source,uint32_t lane,uint32_t color) {
    return RendererAccess::invoke(source,[&](auto& self){self.emit(lane,0xf7000000u,color);});
}
IsleGameResult isle_player_render_backend_fill_rectangle(IslePlayerSource* source,uint32_t lane,int32_t xmin,int32_t ymin,int32_t xmax,int32_t ymax) {
    return RendererAccess::invoke(source,[&](auto& self){self.emit(lane,0xf6000000u|isle::player::xy(xmax,ymax),isle::player::xy(xmin,ymin));});
}
IsleGameResult isle_player_render_backend_color_target(IslePlayerSource* source,uint32_t lane,IslePlayerRenderTarget target) {
    return RendererAccess::invoke(source,[&](auto& self){
        isle::player::require(self.sink && (target==ISLE_PLAYER_RENDER_DEPTH_TARGET || target==ISLE_PLAYER_RENDER_COLOR_TARGET),
            "invalid source actual target",ISLE_GAME_INVALID_ARGUMENT);
        isle::player::require(target!=ISLE_PLAYER_RENDER_DEPTH_TARGET || self.sink->control.depth()!=0,
            "Source depth-as-color requires an actual depth allocation",ISLE_GAME_INVALID_STATE);
        self.emit(lane,0xff10013fu,target==ISLE_PLAYER_RENDER_DEPTH_TARGET ? self.sink->control.depth() : self.sink->control.framebuffer());
    });
}
IsleGameResult isle_player_render_backend_camera_init(IslePlayerSource* source,uint32_t lane,IsleGameCamera* camera,int32_t mode) {
    return RendererAccess::invoke(source,[&](auto& self){self.initialize(camera,lane,mode);});
}
IsleGameResult isle_player_render_backend_camera_post(IslePlayerSource* source,IsleGameCamera* camera) {
    return RendererAccess::invoke(source,[&](auto& self){self.post(camera);});
}
IsleGameResult isle_player_render_backend_camera_render(IslePlayerSource* source,IsleGameObject* object) {
    return RendererAccess::invoke(source,[&](auto& self){self.render(object);});
}
IsleGameResult isle_player_render_backend_process_image(IslePlayerSource* source) {
    return RendererAccess::invoke(source,[&](auto& self){isle::player::require(self.sink&&self.sink->posted&&self.config.photos,
        "source ProcessImage requires actual draw dispatch and production Photo controller");
        const auto result=self.config.photos->request();
        // The original NOBLOCK send ignores a full queue, not an invalid context.
        if(result!=ISLE_GAME_EXHAUSTED) isle::player::native(result,"source ProcessImage context request failed");
    });
}
IsleGameResult isle_player_render_backend_light_component(IslePlayerSource* source,uint32_t component,int8_t value) {
    return RendererAccess::invoke(source,[&](auto& self){isle::player::require(component<3&&!self.sink&&(!self.frame||self.frame->quiescent()),
        "source light update crossed RAM/GPU ownership",ISLE_GAME_INVALID_STATE);self.direction[component]=value;});
}
}

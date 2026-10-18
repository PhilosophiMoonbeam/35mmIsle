#include "post_course_scene_internal.hpp"
#include "window_widgets.hpp"
#include <algorithm>
#include <limits>

namespace isle::post_course {
namespace {
void copy_message(std::span<char> destination,const char* text) noexcept {
    std::size_t count=0;
    while(count+1<destination.size() && text && text[count]) {
        destination[count]=text[count]; ++count;
    }
    if(!destination.empty())destination[count]=0;
}
IsleGameResult continuation_result(IsleContinuationResult result) noexcept {
    switch(result) {
    case ISLE_CONT_OK:return ISLE_GAME_OK;
    case ISLE_CONT_INVALID_ARGUMENT:return ISLE_GAME_INVALID_ARGUMENT;
    case ISLE_CONT_WRONG_OWNER:return ISLE_GAME_WRONG_OWNER;
    case ISLE_CONT_RESOURCE_FAILURE:return ISLE_GAME_EXHAUSTED;
    case ISLE_CONT_EXECUTION_FAILURE:return ISLE_GAME_EXECUTION_FAILURE;
    case ISLE_CONT_INVALID_STATE:return ISLE_GAME_INVALID_STATE;
    }
    return ISLE_GAME_EXECUTION_FAILURE;
}
bool overlaps(sprite::RamRange left,sprite::RamRange right) noexcept {
    return std::uint64_t(left.address)<std::uint64_t(right.address)+right.size &&
           std::uint64_t(right.address)<std::uint64_t(left.address)+left.size;
}
// These are data-only mappings of the selected pinned overlay, not guest code
// execution or a second mutable ScreenSettings/SceneSetup implementation.
std::uint32_t initialized_word(const rom::Snapshot& snapshot,Resources::Kind kind,
                               std::uint32_t guest) {
    const bool camera=kind==Resources::Kind::camera_check;
    const std::uint32_t base=camera?0x801e3ff0u:0x801e53a0u;
    const std::size_t begin=camera?0x8817e0u:0x994e10u;
    const std::size_t end=camera?0x8a70e0u:0x9a6b10u;
    if(guest<base || std::size_t(guest-base)>end-begin-4)
        throw std::out_of_range("post-course Source setup initialized-data extent");
    const auto bytes=snapshot.canonical();
    const auto offset=begin+std::size_t(guest-base);
    if(offset>bytes.size() || 4>bytes.size()-offset)
        throw std::out_of_range("post-course Source setup ROM extent");
    return (std::to_integer<std::uint32_t>(bytes[offset])<<24) |
           (std::to_integer<std::uint32_t>(bytes[offset+1])<<16) |
           (std::to_integer<std::uint32_t>(bytes[offset+2])<<8) |
           std::to_integer<std::uint32_t>(bytes[offset+3]);
}
}

Scene::Impl::Impl(const rom::Snapshot& rom,application::Manager& app,game::State& persistent,
    game_input::Service& controllers,game_audio::Session& sound,host::GtlClock& gtl,
    application::Photos& records,host::Progress& host_progress,ReplayNativeProgress outer,
    Modules& mutable_modules,Resources::Kind selection,std::int32_t entry_argument,bool cpu)
    :snapshot(rom),manager(app),state(persistent),input(controllers),audio(sound),clock(gtl),
     photos(records),progress(host_progress),outer_progress(outer),modules(mutable_modules),
     kind(selection),argument(entry_argument),require_cpu(cpu) {
    if(kind!=Resources::Kind::camera_check && kind!=Resources::Kind::photo_check)
        throw std::invalid_argument("post-course Source kind");
    if(!state.initialized() || !outer_progress.owner || !outer_progress.service)
        throw std::invalid_argument("post-course requires canonical State and actual native progress");
}
Scene::Impl::~Impl() {
    if(creator!=std::this_thread::get_id())std::terminate();
    if(entry && isle_continuation_cancel(entry)!=ISLE_CONT_OK)std::terminate();
    while(native && !native->retire()) {
        (void)native_progress(this);
        (void)progress.idle();
    }
    native.reset();
    if(entry && isle_continuation_destroy(entry)!=ISLE_CONT_OK)std::terminate();
    entry=nullptr;
    if(domain && isle_continuation_domain_destroy(domain)!=ISLE_CONT_OK)std::terminate();
    domain=nullptr;
}
IslePostCourseSceneSource& Scene::Impl::source() noexcept {
    if(auto* camera=std::get_if<IsleCameraCheckSource>(&context))return camera->common;
    if(auto* photo=std::get_if<IslePhotoCheckSource>(&context))return photo->common;
    std::terminate();
}
const IslePostCourseSceneSource& Scene::Impl::source() const noexcept {
    if(const auto* camera=std::get_if<IsleCameraCheckSource>(&context))return camera->common;
    if(const auto* photo=std::get_if<IslePhotoCheckSource>(&context))return photo->common;
    std::terminate();
}
Scene::Impl::Native& Scene::Impl::live() {
    if(!native || native->retired || !native->activated)
        throw std::logic_error("post-course native scene is not live");
    return *native;
}
IsleGameResult Scene::Impl::request_stop() noexcept {
    cancelling=true;
    if(context.index()!=0 && source().status==ISLE_GAME_OK)source().status=ISLE_GAME_STOPPED;
    if(phase!=SceneStatus::retired)phase=native_retired?SceneStatus::cancelled:SceneStatus::retiring;
    return failure==ISLE_GAME_OK?ISLE_GAME_STOPPED:failure;
}
IsleGameResult Scene::Impl::fail(IsleGameResult result,const char* text) noexcept {
    if(result==ISLE_GAME_OK)return failure;
    if(result==ISLE_GAME_STOPPED)return request_stop();
    if(failure==ISLE_GAME_OK) {
        if(context.index()!=0 && source().failure[0])text=source().failure;
        failure=result;copy_message(diagnostic,text);
    }
    if(context.index()!=0 && (source().status==ISLE_GAME_OK || source().status==ISLE_GAME_STOPPED))
        source().status=failure;
    cancelling=true;
    if(phase!=SceneStatus::retired)phase=SceneStatus::retiring;
    return failure;
}
IsleGameResult Scene::Impl::prepare_entry() noexcept {
    if(creator!=std::this_thread::get_id())return ISLE_GAME_WRONG_OWNER;
    if(domain || entry || context.index()!=0)return fail(ISLE_GAME_INVALID_STATE,"post-course entry initialized twice");
    auto result=continuation_result(isle_continuation_domain_create(&domain));
    if(result!=ISLE_GAME_OK)return fail(result,"post-course independent entry domain");
    if(kind==Resources::Kind::camera_check)context.emplace<IsleCameraCheckSource>();
    else context.emplace<IslePhotoCheckSource>();
    auto& common_source=source();
    common_source.owner=this;common_source.domain=domain;
    common_source.photos=photos.native_source();common_source.status=ISLE_GAME_OK;
    if(auto* camera=std::get_if<IsleCameraCheckSource>(&context)) {
        isle_camera_check_source_initialize(camera);
        camera->argument=argument;
        if(camera->common.status!=ISLE_GAME_OK)
            return fail(camera->common.status,"CameraCheck9 Source module initialization");
        result=continuation_result(isle_continuation_create(domain,isle_camera_check_source_entry,camera,&entry));
    } else {
        auto& photo=std::get<IslePhotoCheckSource>(context);
        isle_photo_check_source_initialize(&photo);
        photo_entry={&photo,argument,0,ISLE_GAME_OK};
        result=continuation_result(isle_continuation_create(domain,isle_photo_check_source_entry,&photo_entry,&entry));
    }
    if(result!=ISLE_GAME_OK)return fail(result,"post-course independent original C entry");
    return ISLE_GAME_OK;
}
IsleGameResult Scene::Impl::setup_begin(std::int32_t entry_argument,std::uint32_t screen,
    std::uint32_t setup,std::uint32_t flags,int near_disabled,int update_interval,int draw_interval) noexcept {
    try {
        const bool camera=kind==Resources::Kind::camera_check;
        if(requested || native || native_retired || entry_argument!=argument ||
           screen!=(camera?0x801e4060u:0x801e5410u) ||
           setup!=(camera?0x801e407cu:0x801e542cu) || flags!=0x16a99u || near_disabled!=1 ||
           update_interval!=1 || draw_interval!=(camera?2:1))
            return fail(ISLE_GAME_INVALID_ARGUMENT,"original post-course Screen/Setup intent mismatch");
        for(std::size_t i=0;i<screen_words.size();++i)
            screen_words[i]=initialized_word(snapshot,kind,screen+static_cast<std::uint32_t>(i*4));
        for(std::size_t i=0;i<setup_words.size();++i)
            setup_words[i]=initialized_word(snapshot,kind,setup+static_cast<std::uint32_t>(i*4));
        if(screen_words!=std::array<std::uint32_t,7>{0x803b5000,0x803da800,0,0,320,240,flags} ||
           setup_words[0]!=0 || !setup_words[1] || !setup_words[2] ||
           setup_words[3]<0x80200000u || setup_words[3]>=0x80369f80u ||
           setup_words[5]!=1 || setup_words[6]!=1 || setup_words[12]!=0x00020000u ||
           setup_words[34]!=(camera?0x801dca60u:0x801dcbf4u))
            return fail(ISLE_GAME_INVALID_STATE,"post-course actual ROM Screen/Setup admission");
        // Both originals patch this precise guest arena extent before omSetupScene.
        // Native sizeof/alignment expansion belongs to Storage, not this guest word.
        setup_words[4]=0x80369f80u-setup_words[3];
        requested_screen=screen;requested_setup=setup;
        near_clipping_disabled=near_disabled!=0;requested=true;
        phase=SceneStatus::setup_pending;
        return ISLE_GAME_OK;
    } catch(const std::exception& error) {return fail(ISLE_GAME_INVALID_STATE,error.what());}
    catch(...) {return fail(ISLE_GAME_EXECUTION_FAILURE,"post-course setup publication threw");}
}
IsleGameResult Scene::Impl::setup_poll(int* complete,std::int32_t* result) noexcept {
    if(complete)*complete=0;
    if(result)*result=0;
    if(!complete || !result)return ISLE_GAME_INVALID_ARGUMENT;
    if(!requested)return fail(ISLE_GAME_INVALID_STATE,"post-course entry polled before setup");
    if(failure!=ISLE_GAME_OK)return failure;
    if(cancelling)return ISLE_GAME_STOPPED;
    if(native_retired) {*complete=1;*result=output_scene;}
    return ISLE_GAME_OK;
}

Scene::Impl::Native::Native(Impl& parent):owner(parent),
    heap_bytes(parent.kind==Resources::Kind::camera_check?0x40000:0x32000) {
    try {
        const auto created=storage.create();
        if(created!=ISLE_GAME_OK)throw std::runtime_error("post-course native Game construction");
        common=::isle::window::Resources::load(owner.snapshot);
        resources=Resources::load(owner.snapshot,common,owner.kind);
        producer=std::make_unique<sprite::Producer>(resources->roots(),resources->banks());
        services=std::make_unique<scene::Services>(storage.game(),storage.arena(),resources->view(),
                                                  owner.input,owner.audio,owner.state);
        heap=std::make_shared<::isle::window::Heap>(heap_bytes);
        buttons=std::make_unique<::isle::window::Buttons>(common,heap,*services);
        text=std::make_unique<::isle::window::Text>(common->font());
        ui_frame=std::make_unique<::isle::window::Frame>(*services,*common);
        focus=std::make_unique<::isle::window::Focus>(*services,*common);
        layout=std::make_unique<::isle::window::Layout>(*services,*common);
        reservations.reserve(1+element_slots.size()*2+16);
        reservations.push_back({0x590000,0x10000});
        for(std::size_t i=0;i<element_slots.size();++i) {
            const auto base=0x710000u+static_cast<std::uint32_t>(i)*0x20000u;
            element_slots[i]={{base,base+0x10000},0x10000};
            reservations.push_back({base,0x10000});
            reservations.push_back({base+0x10000,0x10000});
        }
        const bool camera=owner.kind==Resources::Kind::camera_check;
        const std::size_t thumbnails=camera?8:6;
        thumbnail_slots.reserve(thumbnails);
        std::uint32_t next=0x790000;
        for(std::size_t i=0;i<thumbnails;++i) {
            const auto identity=camera?(i==7?0x80208900u:0x80201520u):0x801f3b10u;
            const auto bytes=Window::thumbnail_plane_bytes(*resources,identity);
            if(bytes>std::numeric_limits<std::uint32_t>::max()-63u)
                throw std::runtime_error("post-course thumbnail plane size");
            const auto plane=(static_cast<std::uint32_t>(bytes)+63u)&~63u;
            if(!plane || next>0x7be600u || std::uint64_t(next)+2ull*plane>0x7be600u)
                throw std::runtime_error("post-course thumbnail planes overlap retained Replay targets");
            thumbnail_slots.push_back({{next,next+plane},plane});
            reservations.push_back({next,plane});reservations.push_back({next+plane,plane});
            next+=plane*2;
        }
        const auto sprite_assets=producer->assets();
        const auto common_assets=common->common_render_assets();
        immutable.reserve(sprite_assets.size()+common_assets.size());
        immutable.insert(immutable.end(),sprite_assets.begin(),sprite_assets.end());
        immutable.insert(immutable.end(),common_assets.begin(),common_assets.end());
        for(const auto& asset:immutable) {
            if(asset.bytes.size()>std::numeric_limits<std::uint32_t>::max())
                throw std::runtime_error("post-course immutable sprite extent");
            for(const auto range:reservations)
                if(overlaps({asset.address,static_cast<std::uint32_t>(asset.bytes.size())},range))
                    throw std::runtime_error("post-course raster/command plan overlaps actual immutable assets");
        }
        window=std::make_unique<Window>(*services,*text,*ui_frame,*focus,*layout,*buttons,
            *resources,heap,*producer,WindowCapacity{element_slots,8,16,64,64,thumbnail_slots});
        scene::graphics::Config config;
        config.require_cpu=owner.require_cpu;
        config.memory={{0x6b0000,0x10000},{0x6c0000,0x10000},{0x6d0000,0x40000},0x600000};
        auto created_frame=scene::graphics::Frame::create(owner.snapshot,config);
        if(!created_frame)throw std::runtime_error(created_frame.message.data());
        graphics=std::move(created_frame.frame);
        scene::graphics::SourceScreen screen;
        screen.width=owner.screen_words[4];screen.height=owner.screen_words[5];screen.flags=owner.screen_words[6];
        std::copy_n(owner.screen_words.begin(),3,screen.framebuffer_identity.begin());
        // Explicit NULL is Source viZBuffer=NULL. Frame still protects its private
        // core depth; no Source depth allocation is fabricated for entries9/15.
        if(!graphics->configure_source_screen(screen,nullptr) ||
           !graphics->reserve_render_capacity({draws.size(),1}))
            throw std::runtime_error(graphics->message());
        source_objects.reserve(32);
    } catch(const std::exception& error) {
        owner.fail(ISLE_GAME_INVALID_STATE,error.what());
        const auto primary=std::current_exception();
        while(!retire()) {(void)owner.outer_progress.service(owner.outer_progress.owner);(void)owner.progress.idle();}
        std::rethrow_exception(primary);
    } catch(...) {
        owner.fail(ISLE_GAME_EXECUTION_FAILURE,"post-course partial native construction");
        const auto primary=std::current_exception();
        while(!retire()) {(void)owner.outer_progress.service(owner.outer_progress.owner);(void)owner.progress.idle();}
        std::rethrow_exception(primary);
    }
}
Scene::Impl::Native::~Native() {if(!retired)std::terminate();}
void Scene::Impl::Native::add_camera(IsleGameObject* object,IsleGameCamera* camera,
    std::uint32_t priority,std::uint64_t mask,std::uint32_t tag) {
    if(retired || !object || !camera || pass_count==passes.size())
        throw std::invalid_argument("post-course native camera pass capacity");
    IsleGameObject* actual=nullptr;
    const auto status=isle_game_camera_object(storage.game(),camera,&actual);
    if(status!=ISLE_GAME_OK || actual!=object)
        throw std::invalid_argument("post-course camera belongs to another native object");
    for(std::size_t i=0;i<pass_count;++i)if(passes[i].camera==camera)
        throw std::invalid_argument("post-course camera registered twice");
    auto& pass=passes[pass_count++];pass={};
    pass.kind=scene::render::Kind::sprites;pass.camera=camera;
    pass.priority=priority;pass.link_mask=mask;pass.camera_tag=tag;
    pass.tag_match=scene::render::TagMatch::intersection;
}
scene::render::Input Scene::Impl::Native::render_input() {
    if(retired || !activated || !services || services->failed())
        throw std::logic_error("post-course render requires live Source Services");
    std::size_t count=0;
    if(!services->sprite_draws(draws,count))throw std::runtime_error(services->error());
    if(!pass_count)throw std::runtime_error("post-course Source created no native cameras");
    for(std::size_t i=0;i<pass_count;++i) {
        auto& pass=passes[i];
        IsleGameCameraRenderState state{};
        if(isle_game_camera_render_state(storage.game(),pass.camera,&state)!=ISLE_GAME_OK)
            throw std::runtime_error("post-course native camera render state");
        pass.flags=state.flags;pass.background_rgba=state.background_rgba;
        pass.sprites=std::span<const scene::SpriteDraw>(draws).first(count);
        pass.sprite_commands={0x590000u,0x10000u};
    }
    save::ScreenOffsets offsets{};
    if(!owner.state.screen_offsets().translation(offsets))
        throw std::runtime_error("post-course live VI endpoints cannot be represented by native translation");
    ::isle::graphics::SourceProgramSelection programs;
    programs.disable_near_clipping=owner.near_clipping_disabled;
    // Raster allocations belong to this very Producer. They are external to
    // Replay, but must NOT be passed back as exclusions against their own upload.
    return {storage.game(),{owner.kind==Resources::Kind::camera_check?9u:15u,producer.get(),immutable,
        std::span<const sprite::RamRange>(reservations).first(1),
        {owner.screen_words[0]&0x1fffffffu,owner.screen_words[1]&0x1fffffffu},common->common_pre_render()},
        std::span<const scene::render::Pass>(passes).first(pass_count),offsets,
        ::isle::graphics::source_task_program(programs,false)};
}

IsleGameResult Scene::Impl::initialize_replay() noexcept {
    if(creator!=std::this_thread::get_id()) {
        struct Call {Impl* owner;IsleGameResult result;};
        Call call{this,ISLE_GAME_INVALID_STATE};
        const auto routed=isle_continuation_on_owner([](void* raw) {
            auto& call=*static_cast<Call*>(raw);call.result=call.owner->initialize_replay();
        },&call);
        return routed==ISLE_CONT_OK?call.result:continuation_result(routed);
    }
    try {
        auto& scene=live();
        if(scene.replay_initialized || scene.pipeline || scene.replay || source().replay)
            return fail(ISLE_GAME_INVALID_STATE,"post-course Source initialized Replay twice");
        photo::ReplayConfig config;
        ::isle::graphics::SourceProgramSelection programs;
        programs.disable_near_clipping=near_clipping_disabled;
        config.program=::isle::graphics::source_task_program(programs,false);
        auto& memory=config.memory;
        memory.world={{0x400000,0x20000},{0x420000,0x10000},{0x430000,0x10000}};
        memory.subjects={{0x440000,0x60000},{0x4a0000,0x20000},{0x4c0000,0x20000},{}};
        memory.items={{0x4e0000,0x10000},{0x4f0000,0x10000},{0x500000,0x10000},{}};
        memory.commands={0x510000,0x20000};memory.workspace={0x530000,0x10000};memory.output={0x540000,0x40000};
        memory.effects={0x580000,0x10000};memory.effect_assets={0x630000,0x80000};
        memory.preview_color={0x7be600,0x20d00};memory.preview_depth={0x7df300,0x20d00};
        memory.feedback={0x7be600,0x27000};
        std::vector<menu::RamRange> external;
        external.reserve(scene.reservations.size()+scene.graphics->reservations().size());
        for(const auto range:scene.graphics->reservations()) {
            if(range.address==0 && range.size==::isle::graphics::bss_end)continue;
            external.push_back({range.address,range.size});
        }
        for(const auto range:scene.reservations)external.push_back({range.address,range.size});
        config.external_reservations=external;
        scene.pipeline=std::make_unique<photo::Replay>(scene.storage.game(),snapshot,modules.world,
            photo::ReplayGpuAccess{this,acquire_gpu,release_gpu},config);
        std::vector<sprite::RamRange> retained;
        retained.reserve(scene.pipeline->reservations().size());
        for(const auto range:scene.pipeline->reservations())
            retained.push_back({static_cast<std::uint32_t>(range.offset),static_cast<std::uint32_t>(range.size)});
        if(!scene.graphics->reserve_source_gpu_storage(retained))
            return fail(ISLE_GAME_INVALID_STATE,scene.graphics->message());
        scene.replay=std::make_unique<Replay>(scene.storage.game(),photos,*scene.pipeline,progress,audio,
                                            ReplayNativeProgress{this,native_progress});
        source().replay=scene.replay->source();scene.replay_initialized=true;
        return ISLE_GAME_OK;
    } catch(const std::exception& error) {return fail(ISLE_GAME_INVALID_STATE,error.what());}
    catch(...) {return fail(ISLE_GAME_EXECUTION_FAILURE,"post-course Replay construction threw");}
}
IsleGameResult Scene::Impl::cleanup_replay() noexcept {
    if(creator!=std::this_thread::get_id()) {
        struct Call {Impl* owner;IsleGameResult result;};
        Call call{this,ISLE_GAME_INVALID_STATE};
        const auto routed=isle_continuation_on_owner([](void* raw) {
            auto& call=*static_cast<Call*>(raw);call.result=call.owner->cleanup_replay();
        },&call);
        return routed==ISLE_CONT_OK?call.result:continuation_result(routed);
    }
    if(!native || !native->replay_initialized || !native->replay || !native->pipeline)
        return fail(ISLE_GAME_INVALID_STATE,"post-course Source cleanup without initialized Replay");
    bool joined=false;
    const auto status=native->replay->retire(joined);
    if(status!=ISLE_GAME_OK && status!=ISLE_GAME_STOPPED)
        return fail(status,isle_post_replay_error(native->replay->source()));
    if(!joined || !native->pipeline->retire())
        return fail(ISLE_GAME_INVALID_STATE,"post-course Source cleanup still owns actual Replay GPU/mapping/Photo lease");
    if(status==ISLE_GAME_STOPPED)return request_stop();
    // Keep the joined facade and pipeline alive until every Source process joins.
    // A cleanup call is not permission to destroy a callback's borrowed context.
    return ISLE_GAME_OK;
}
IsleGameResult Scene::Impl::request_exit(std::int32_t result) noexcept {
    try {
        auto& scene=live();
        if(scene.services->exit_requested())
            return fail(ISLE_GAME_INVALID_STATE,"post-course Source result published twice");
        scene.services->request_exit(result);
        return ISLE_GAME_OK;
    } catch(const std::exception& error) {return fail(ISLE_GAME_INVALID_STATE,error.what());}
    catch(...) {return fail(ISLE_GAME_EXECUTION_FAILURE,"post-course Source result publication threw");}
}
IsleGameResult Scene::Impl::native_progress(void* raw) noexcept {
    if(!raw)return ISLE_GAME_INVALID_ARGUMENT;
    auto& owner=*static_cast<Impl*>(raw);
    if(owner.creator!=std::this_thread::get_id())return ISLE_GAME_WRONG_OWNER;
    if(owner.native && owner.native->graphics) {
        const auto polled=owner.native->graphics->poll();
        if(polled==scene::graphics::Poll::failed)
            owner.fail(ISLE_GAME_EXECUTION_FAILURE,owner.native->graphics->message());
    }
    // The outer owner copies/presents this genuine completion and resolves its
    // real FrameToken. This callback itself creates no OM, VI or GTL credit.
    const auto result=owner.outer_progress.service(owner.outer_progress.owner);
    if(result==ISLE_GAME_STOPPED)return owner.request_stop();
    if(result!=ISLE_GAME_OK)owner.fail(result,"post-course outer native progress failed");
    return owner.failure!=ISLE_GAME_OK?owner.failure:owner.cancelling?ISLE_GAME_STOPPED:ISLE_GAME_OK;
}
photo::ReplayAcquire Scene::Impl::acquire_gpu(void* raw,photo::ReplayGpuBinding& output) noexcept {
    output={};
    if(!raw)return photo::ReplayAcquire::failed;
    auto& owner=*static_cast<Impl*>(raw);
    if(owner.creator!=std::this_thread::get_id() || !owner.native || !owner.native->graphics)
        return photo::ReplayAcquire::failed;
    auto& native=*owner.native;
    const auto completed=native.graphics->completed();
    if(completed && completed.ordinal!=native.released_ordinal)return photo::ReplayAcquire::pending;
    scene::graphics::SourceGpuBinding binding;
    switch(native.graphics->acquire_source_gpu(binding)) {
    case scene::graphics::SourceGpuAcquire::pending:return photo::ReplayAcquire::pending;
    case scene::graphics::SourceGpuAcquire::failed:
        owner.fail(ISLE_GAME_EXECUTION_FAILURE,native.graphics->message());return photo::ReplayAcquire::failed;
    case scene::graphics::SourceGpuAcquire::admitted:
        output={binding.executor,binding.programs,binding.epoch,binding.admission};return photo::ReplayAcquire::admitted;
    }
    return photo::ReplayAcquire::failed;
}
bool Scene::Impl::release_gpu(void* raw,const photo::ReplayGpuBinding& binding) noexcept {
    if(!raw)return false;
    auto& owner=*static_cast<Impl*>(raw);
    if(owner.creator!=std::this_thread::get_id() || !owner.native || !owner.native->graphics)return false;
    const bool released=owner.native->graphics->release_source_gpu(
        {binding.executor,binding.programs,binding.epoch,binding.admission});
    if(!released)owner.fail(ISLE_GAME_INVALID_STATE,"post-course actual Replay GPU release rejected");
    return released;
}

bool Scene::Impl::Native::retire() noexcept {
    if(retired)return true;
    if(owner.creator!=std::this_thread::get_id())return false;
    if(graphics) {
        const auto status=graphics->poll();
        if(status==scene::graphics::Poll::failed)owner.fail(ISLE_GAME_EXECUTION_FAILURE,graphics->message());
        const auto completed=graphics->completed();
        if(completed && completed.ordinal!=released_ordinal) {
            return false;
        }
    }
    if(replay) {
        bool joined=false;
        const auto status=replay->retire(joined);
        if(status==ISLE_GAME_STOPPED)(void)owner.request_stop();
        else if(status!=ISLE_GAME_OK)owner.fail(status,isle_post_replay_error(replay->source()));
        if(!joined)return false;
    }
    if(pipeline && !pipeline->retire())return false;
    if(graphics && !graphics->quiescent())return false;
    if(activated && !callbacks_joined) {
        const auto status=isle_post_course_source_join_processes(&owner.source());
        if(status!=ISLE_GAME_OK)owner.fail(status,"post-course native process join retained Source fault");
        IsleGameCounts counts{};
        if(isle_game_counts(storage.game(),&counts)!=ISLE_GAME_OK || counts.processes)return false;
        callbacks_joined=true;
    }
    if(window) {
        const auto cancelled=isle_post_window_cancel_all(window->source());
        if(cancelled!=ISLE_GAME_OK)owner.fail(cancelled,"post-course Window operation cancellation retained fault");
        if(auto* camera=std::get_if<IsleCameraCheckSource>(&owner.context);camera && activated) {
            const auto released=isle_camera_check_source_release(camera);
            if(released!=ISLE_GAME_OK) {
                owner.fail(released,"CameraCheck thumbnail release failed after GPU/process joins");return false;
            }
        }
        if(window->retire()!=ISLE_GAME_OK) {
            owner.fail(ISLE_GAME_INVALID_STATE,"post-course Window bindings have not retired");return false;
        }
    }
    if(!owner.cancelling && services && services->exit_requested())owner.output_scene=services->exit_result();
    replay.reset();pipeline.reset();window.reset();
    layout.reset();focus.reset();ui_frame.reset();text.reset();buttons.reset();heap.reset();
    if(services)services->stop_sound_ramps();
    services.reset();source_objects.clear();
    if(activated) {
        const auto status=isle_post_course_source_deactivate(&owner.source());
        if(status!=ISLE_GAME_OK)owner.fail(status,"post-course context deactivation retained native fault");
        if(owner.source().game)return false;
        activated=false;
    }
    if(graphics && !graphics->retire())return false;
    graphics.reset();producer.reset();resources.reset();common.reset();
    if(storage.game()) {
        const auto destroyed=storage.destroy();
        if(destroyed!=ISLE_GAME_OK) {
            owner.fail(destroyed,"post-course native Storage destroy has not joined");return false;
        }
    }
    retired=true;return true;
}

Scene::Scene(std::unique_ptr<Impl> owner) noexcept:p_(std::move(owner)) {}
Scene::~Scene()=default;
SceneCreateResult Scene::create(const rom::Snapshot& snapshot,application::Manager& manager,
    game::State& state,game_input::Service& input,game_audio::Session& audio,host::GtlClock& clock,
    application::Photos& photos,host::Progress& progress,ReplayNativeProgress native_progress,
    Modules& modules,Resources::Kind kind,std::int32_t argument,bool cpu) noexcept {
    SceneCreateResult result;
    try {
        auto owner=std::make_unique<Impl>(snapshot,manager,state,input,audio,clock,photos,progress,
                                        native_progress,modules,kind,argument,cpu);
        result.error=owner->prepare_entry();
        if(result.error!=ISLE_GAME_OK) {copy_message(result.message,owner->diagnostic.data());return result;}
        result.scene=std::unique_ptr<Scene>(new Scene(std::move(owner)));
    } catch(const std::bad_alloc&) {
        result.error=ISLE_GAME_EXHAUSTED;copy_message(result.message,"post-course owner allocation failed");
    } catch(const std::exception& error) {
        result.error=ISLE_GAME_INVALID_STATE;copy_message(result.message,error.what());
    } catch(...) {
        result.error=ISLE_GAME_EXECUTION_FAILURE;copy_message(result.message,"post-course owner construction threw");
    }
    return result;
}
IsleGameResult Scene::resume() noexcept {
    auto& owner=*p_;
    if(owner.creator!=std::this_thread::get_id())return ISLE_GAME_WRONG_OWNER;
    if(owner.failure!=ISLE_GAME_OK)return owner.failure;
    if(owner.cancelling)return ISLE_GAME_STOPPED;
    if(owner.phase==SceneStatus::completed)return ISLE_GAME_OK;
    if(!owner.entry || (owner.requested && !owner.native_retired))return ISLE_GAME_OK;
    const auto resumed=isle_continuation_resume(owner.entry);
    if(resumed!=ISLE_CONT_OK)
        return owner.fail(continuation_result(resumed),isle_continuation_error(owner.domain));
    if(owner.source().status==ISLE_GAME_STOPPED)return owner.request_stop();
    if(owner.source().status!=ISLE_GAME_OK)
        return owner.fail(owner.source().status,"post-course original entry retained a leaf fault");
    IsleContinuationState state;
    const auto inspected=isle_continuation_state(owner.entry,&state);
    if(inspected!=ISLE_CONT_OK)return owner.fail(continuation_result(inspected),"post-course entry state");
    if(state==ISLE_CONT_COMPLETED) {
        if(!owner.native_retired)return owner.fail(ISLE_GAME_INVALID_STATE,"original entry returned before actual native retirement");
        if(const auto* camera=std::get_if<IsleCameraCheckSource>(&owner.context)) {
            if(!camera->entry_complete || camera->source_result!=owner.output_scene)
                return owner.fail(ISLE_GAME_INVALID_STATE,"CameraCheck original entry result mismatch");
        } else if(owner.photo_entry.status!=ISLE_GAME_OK || owner.photo_entry.result!=owner.output_scene) {
            return owner.fail(ISLE_GAME_INVALID_STATE,"PhotoCheck original entry result mismatch");
        }
        owner.phase=SceneStatus::completed;
    } else if(state==ISLE_CONT_FAILED || state==ISLE_CONT_CANCELLED) {
        return owner.fail(ISLE_GAME_EXECUTION_FAILURE,"post-course original entry did not complete");
    }
    return ISLE_GAME_OK;
}
bool Scene::setup_pending() const noexcept {return p_->phase==SceneStatus::setup_pending && !p_->native;}
IsleGameResult Scene::initialize_scene() noexcept {
    auto& owner=*p_;
    if(owner.creator!=std::this_thread::get_id())return ISLE_GAME_WRONG_OWNER;
    if(owner.failure!=ISLE_GAME_OK)return owner.failure;
    if(!setup_pending() || owner.native_retired || owner.cancelling)
        return owner.fail(ISLE_GAME_INVALID_STATE,"post-course native initialization without original setup intent");
    try {
        if(owner.kind==Resources::Kind::camera_check && owner.clock.set_intervals(1,2)!=host::ClockError::none)
            return owner.fail(ISLE_GAME_INVALID_STATE,"CameraCheck original GTL intervals1/2");
        owner.native=std::make_unique<Impl::Native>(owner);
        auto& source=owner.source();
        source.game=owner.native->storage.game();source.window=owner.native->window->source();
        const auto activated=isle_post_course_source_activate(&source);
        if(activated!=ISLE_GAME_OK) {
            source.game=nullptr;source.window=nullptr;
            return owner.fail(activated,"post-course Source context activation");
        }
        owner.native->activated=true;
        const auto initialized=isle_post_course_source_call(&source,
            owner.kind==Resources::Kind::camera_check?isle_camera_check_source_init:isle_photo_check_source_init_call,nullptr);
        if(initialized!=ISLE_GAME_OK && initialized!=ISLE_GAME_STOPPED)
            return owner.fail(initialized,"original post-course nonwaiting C initialization");
        if(initialized==ISLE_GAME_STOPPED)return owner.request_stop();
        if(!owner.native->heap_admitted || !owner.native->input_initialized || !owner.native->replay_initialized)
            return owner.fail(ISLE_GAME_INVALID_STATE,"original post-course C initialization omitted native service admission");
        owner.phase=SceneStatus::running;return ISLE_GAME_OK;
    } catch(const std::exception& error) {return owner.fail(ISLE_GAME_INVALID_STATE,error.what());}
    catch(...) {return owner.fail(ISLE_GAME_EXECUTION_FAILURE,"post-course native initialization threw");}
}
IsleGameResult Scene::update(std::uint32_t raw_game_time) noexcept {
    auto& owner=*p_;
    if(owner.creator!=std::this_thread::get_id())return ISLE_GAME_WRONG_OWNER;
    if(owner.failure!=ISLE_GAME_OK)return owner.failure;
    if(owner.cancelling)return ISLE_GAME_STOPPED;
    if(owner.phase!=SceneStatus::running || !owner.native)return ISLE_GAME_INVALID_STATE;
    auto& native=*owner.native;
    if(native.graphics->busy() || completed_frame())return ISLE_GAME_INVALID_STATE;
    native.services->raw_game_time=raw_game_time;
    const auto updated=isle_game_tick(native.storage.game());
    if(updated!=ISLE_GAME_OK && updated!=ISLE_GAME_STOPPED)
        return owner.fail(updated,isle_game_error(native.storage.game()));
    if(owner.source().status!=ISLE_GAME_OK && owner.source().status!=ISLE_GAME_STOPPED)
        return owner.fail(owner.source().status,"post-course Source update retained a leaf fault");
    if(native.services->failed())return owner.fail(ISLE_GAME_EXECUTION_FAILURE,native.services->error());
    if(updated==ISLE_GAME_STOPPED || owner.source().status==ISLE_GAME_STOPPED || owner.cancelling)
        return owner.request_stop();
    if(native.services->exit_requested())owner.phase=SceneStatus::retiring;
    return ISLE_GAME_OK;
}
bool Scene::draw(scene::graphics::FrameContext context) noexcept {
    auto& owner=*p_;
    if(owner.creator!=std::this_thread::get_id() || owner.phase!=SceneStatus::running ||
       owner.failure!=ISLE_GAME_OK || owner.cancelling || !owner.native || graphics_busy() || completed_frame())return false;
    try {
        if(!owner.native->graphics->begin(owner.native->render_input(),context)) {
            owner.fail(ISLE_GAME_EXECUTION_FAILURE,owner.native->graphics->message());return false;
        }
        return true;
    } catch(const std::exception& error) {owner.fail(ISLE_GAME_EXECUTION_FAILURE,error.what());return false;}
    catch(...) {owner.fail(ISLE_GAME_EXECUTION_FAILURE,"post-course rendering threw");return false;}
}
bool Scene::graphics_busy() const noexcept {return p_->native && p_->native->graphics && p_->native->graphics->busy();}
scene::graphics::Poll Scene::poll() noexcept {
    auto& owner=*p_;
    if(owner.creator!=std::this_thread::get_id())return scene::graphics::Poll::failed;
    if(!owner.native || !owner.native->graphics)return scene::graphics::Poll::idle;
    const auto status=owner.native->graphics->poll();
    if(status==scene::graphics::Poll::failed)owner.fail(ISLE_GAME_EXECUTION_FAILURE,owner.native->graphics->message());
    if(status==scene::graphics::Poll::completed && !completed_frame())return scene::graphics::Poll::idle;
    return status;
}
scene::graphics::Completed Scene::completed_frame() const noexcept {
    if(!p_->native || !p_->native->graphics)return {};
    const auto completed=p_->native->graphics->completed();
    return completed && completed.ordinal!=p_->native->released_ordinal?completed:scene::graphics::Completed{};
}
IsleGameResult Scene::release_completed() noexcept {
    if(p_->creator!=std::this_thread::get_id())return ISLE_GAME_WRONG_OWNER;
    const auto completed=completed_frame();
    if(!completed)return ISLE_GAME_INVALID_STATE;
    p_->native->released_ordinal=completed.ordinal;
    return ISLE_GAME_OK;
}
bool Scene::retire_scene() noexcept {
    auto& owner=*p_;
    if(owner.creator!=std::this_thread::get_id())return false;
    if(owner.native_retired)return true;
    if(!owner.cancelling && owner.failure==ISLE_GAME_OK && owner.phase!=SceneStatus::retiring)return false;
    if(owner.native && !owner.native->retire())return false;
    owner.native.reset();owner.native_retired=true;
    if(owner.failure!=ISLE_GAME_OK)owner.phase=SceneStatus::failed;
    else if(owner.cancelling)owner.phase=SceneStatus::cancelled;
    return true;
}
IsleGameResult Scene::cancel() noexcept {
    auto& owner=*p_;
    if(owner.creator!=std::this_thread::get_id())return ISLE_GAME_WRONG_OWNER;
    if(owner.phase==SceneStatus::retired)return owner.failure;
    (void)owner.request_stop();
    if(owner.entry) {
        const auto result=isle_continuation_cancel(owner.entry);
        if(result!=ISLE_CONT_OK)return owner.fail(continuation_result(result),"post-course original entry cancellation did not join");
    }
    (void)retire_scene();return owner.failure;
}
IsleGameResult Scene::retire() noexcept {
    auto& owner=*p_;
    if(owner.creator!=std::this_thread::get_id())return ISLE_GAME_WRONG_OWNER;
    if(owner.phase==SceneStatus::retired)return owner.failure;
    if(owner.phase!=SceneStatus::completed && owner.phase!=SceneStatus::cancelled && owner.phase!=SceneStatus::failed)
        (void)cancel();
    if(!retire_scene())return owner.failure;
    if(owner.entry) {
        const auto result=isle_continuation_destroy(owner.entry);
        if(result!=ISLE_CONT_OK)return owner.fail(continuation_result(result),"post-course original entry destruction did not join");
        owner.entry=nullptr;
    }
    if(owner.domain) {
        const auto result=isle_continuation_domain_destroy(owner.domain);
        if(result!=ISLE_CONT_OK)return owner.fail(continuation_result(result),"post-course entry domain destruction did not join");
        owner.domain=nullptr;owner.source().domain=nullptr;
    }
    owner.phase=SceneStatus::retired;return owner.failure;
}
SceneStatus Scene::status() const noexcept {return p_->phase;}
IsleGameResult Scene::error() const noexcept {return p_->failure;}
const char* Scene::message() const noexcept {return p_->diagnostic.data();}
std::optional<std::int32_t> Scene::result() const noexcept {
    if((p_->phase==SceneStatus::completed || p_->phase==SceneStatus::retired) &&
       p_->native_retired && !p_->cancelling && p_->failure==ISLE_GAME_OK)return p_->output_scene;
    return std::nullopt;
}
const IslePostCourseSceneSource& Scene::source() const noexcept {return p_->source();}
const IsleCameraCheckSource* Scene::camera_source() const noexcept {return std::get_if<IsleCameraCheckSource>(&p_->context);}
const IslePhotoCheckSource* Scene::photo_source() const noexcept {return std::get_if<IslePhotoCheckSource>(&p_->context);}
const IsleCameraCheckInput* Scene::camera_input_observation(std::size_t controller) const noexcept {
    if (!camera_source() || !p_->native || p_->native->retired ||
        controller >= p_->native->camera_inputs.size() || !p_->native->camera_input_observed[controller])
        return nullptr;
    return &p_->native->camera_inputs[controller];
}
}

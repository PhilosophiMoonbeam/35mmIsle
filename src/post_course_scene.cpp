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
    const auto origin=Resources::origin(kind);
    const auto base=origin.guest_begin;
    const auto begin=origin.rom_begin;
    const auto end=origin.rom_end;
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
    const auto ordinal=static_cast<std::size_t>(kind);
    constexpr std::array<std::int32_t,7> scenes{9,15,12,14,22,24,17};
    if(ordinal>=scenes.size() ||
       (kind==Resources::Kind::gallery ? argument!=22 && argument!=23 :
        kind==Resources::Kind::credits ? argument<17 || argument>20 : argument!=scenes[ordinal]))
        throw std::invalid_argument("post-course Source kind/entry argument");
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
    return std::visit([](auto& value)->IslePostCourseSceneSource& {
        if constexpr(std::is_same_v<std::remove_cvref_t<decltype(value)>,std::monostate>)
            std::terminate();
        else return value.common;
    },context);
}
const IslePostCourseSceneSource& Scene::Impl::source() const noexcept {
    return std::visit([](const auto& value)->const IslePostCourseSceneSource& {
        if constexpr(std::is_same_v<std::remove_cvref_t<decltype(value)>,std::monostate>)
            std::terminate();
        else return value.common;
    },context);
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
    switch(kind) {
    case Resources::Kind::camera_check:context.emplace<IsleCameraCheckSource>();break;
    case Resources::Kind::photo_check:context.emplace<IslePhotoCheckSource>();break;
    case Resources::Kind::album:context.emplace<IsleAlbumSource>();break;
    case Resources::Kind::report:context.emplace<IsleReportSource>();break;
    case Resources::Kind::gallery:context.emplace<IsleGallerySource>();break;
    case Resources::Kind::end_level_8:context.emplace<IsleEndLevel8Source>();break;
    case Resources::Kind::credits:context.emplace<IsleCreditsSource>();break;
    }
    auto& common_source=source();
    common_source.owner=this;common_source.domain=domain;common_source.scene=argument;
    common_source.photos=photos.native_source();common_source.status=ISLE_GAME_OK;
    void (*function)(void*)=nullptr;
    void* entry_argument=nullptr;
    if(auto* camera=std::get_if<IsleCameraCheckSource>(&context)) {
        isle_camera_check_source_initialize(camera);camera->argument=argument;
        function=isle_camera_check_source_entry;entry_argument=camera;
    } else if(auto* photo=std::get_if<IslePhotoCheckSource>(&context)) {
        isle_photo_check_source_initialize(photo);
        auto& call=entry_context.emplace<IslePhotoCheckEntry>();
        call={photo,argument,0,ISLE_GAME_OK};
        function=isle_photo_check_source_entry;entry_argument=&call;
    } else {
        result=admit_layout();
        if(result!=ISLE_GAME_OK)return result;
        try { native=std::make_unique<Native>(*this); }
        catch(const std::exception& error) {return fail(ISLE_GAME_INVALID_STATE,error.what());}
        catch(...) {return fail(ISLE_GAME_EXECUTION_FAILURE,"post-course cold native admission threw");}
        common_source.game=native->storage.game();common_source.window=native->window->source();
        result=isle_post_course_source_activate(&common_source);
        if(result!=ISLE_GAME_OK)return fail(result,"post-course cold Source activation");
        native->activated=true;
        result=prepare_menu();
        if(result!=ISLE_GAME_OK)return result;
        if(auto* album=std::get_if<IsleAlbumSource>(&context)) {
            auto& call=entry_context.emplace<IsleAlbumEntry>();
            call.source=album;call.argument=argument;call.status=ISLE_GAME_OK;
            function=isle_album_source_entry;entry_argument=&call;
        } else if(auto* report=std::get_if<IsleReportSource>(&context)) {
            auto& call=entry_context.emplace<IsleReportEntry>();
            call.source=report;call.argument=argument;call.status=ISLE_GAME_OK;
            function=isle_report_source_entry;entry_argument=&call;
        } else if(auto* gallery=std::get_if<IsleGallerySource>(&context)) {
            auto& call=entry_context.emplace<IsleGalleryEntry>();
            call.source=gallery;call.argument=argument;call.status=ISLE_GAME_OK;
            function=isle_gallery_source_entry;entry_argument=&call;
        } else if(auto* end=std::get_if<IsleEndLevel8Source>(&context)) {
            auto& call=entry_context.emplace<IsleEndLevel8Entry>();
            call.source=end;call.argument=argument;call.status=ISLE_GAME_OK;
            function=isle_end_level_8_source_entry;entry_argument=&call;
        } else {
            function=isle_credits_source_entry;entry_argument=&std::get<IsleCreditsSource>(context);
        }
    }
    if(common_source.status!=ISLE_GAME_OK)
        return fail(common_source.status,"post-course Source module initialization");
    result=continuation_result(isle_continuation_create(domain,function,entry_argument,&entry));
    if(result!=ISLE_GAME_OK)return fail(result,"post-course independent original C entry");
    return ISLE_GAME_OK;
}
IsleGameResult Scene::Impl::admit_layout() noexcept {
    try {
        constexpr std::array<std::array<std::uint32_t,2>,7> addresses{{
            {0x801e4060,0x801e407c},{0x801e5410,0x801e542c},
            {0x801e4250,0x801e426c},{0x801e3c80,0x801e3c9c},
            {0x801e1690,0x801e16ac},{0x801dd130,0x801dd14c},
            {0x801deba0,0x801dec30}}};
        const auto& address=addresses.at(static_cast<std::size_t>(kind));
        requested_screen=address[0];requested_setup=address[1];
        for(std::size_t i=0;i<screen_words.size();++i)
            screen_words[i]=initialized_word(snapshot,kind,requested_screen+static_cast<std::uint32_t>(i*4));
        for(std::size_t i=0;i<setup_words.size();++i)
            setup_words[i]=initialized_word(snapshot,kind,requested_setup+static_cast<std::uint32_t>(i*4));
        const auto contexts=kind==Resources::Kind::album || kind==Resources::Kind::report ? 2u:1u;
        if(screen_words!=std::array<std::uint32_t,7>{0x803b5000,0x803da800,0,0,320,240,0x16a99} ||
           setup_words[0] || !setup_words[1] || !setup_words[2] ||
           setup_words[3]<0x80100000u || setup_words[3]>=0x80369f80u ||
           setup_words[5]!=contexts || setup_words[6]!=1 || !setup_words[34])
            return fail(ISLE_GAME_INVALID_STATE,"post-course actual ROM Screen/Setup admission");
        setup_words[4]=0x80369f80u-setup_words[3];
        return ISLE_GAME_OK;
    } catch(const std::exception& error) {return fail(ISLE_GAME_INVALID_STATE,error.what());}
    catch(...) {return fail(ISLE_GAME_EXECUTION_FAILURE,"post-course initialized layout admission threw");}
}
IsleGameResult Scene::Impl::prepare_menu() noexcept {
    try {
        auto& scene=live();
        IsleGameResult result=ISLE_GAME_OK;
        if(kind!=Resources::Kind::album) {
            result=scene.window->bind_graphics_frame(*scene.graphics);
            if(result!=ISLE_GAME_OK)return fail(result,"post-course Window Frame binding");
        }
        if(auto* source=std::get_if<IsleAlbumSource>(&context)) {
            auto& menu=scene.menu.emplace<AlbumSceneNative>(photos);
            result=isle_album_source_initialize(source,&album_scene_native_table(),
                menu.first_working.source(),menu.second_working.source());
            if(result==ISLE_GAME_OK)result=album_scene_native_prepare(source,menu);
        } else if(auto* source=std::get_if<IsleReportSource>(&context)) {
            auto& menu=scene.menu.emplace<ReportSceneNative>(photos);
            source->native=&report_scene_native_table();
            result=report_scene_native_prepare(source,menu);
            if(result==ISLE_GAME_OK)result=isle_post_course_source_call(&source->common,
                [](IslePostCourseSceneSource*,void* raw) {
                    isle_report_source_initialize(static_cast<IsleReportSource*>(raw));
                },source);
        } else if(auto* source=std::get_if<IsleGallerySource>(&context)) {
            auto& menu=scene.menu.emplace<GallerySceneNative>(photos);
            result=gallery_scene_native_prepare(source,menu);
            if(result==ISLE_GAME_OK)result=isle_gallery_source_initialize(source,&gallery_scene_native_table(),
                &menu.initial,menu.working_handles.data(),argument);
        } else if(auto* source=std::get_if<IsleEndLevel8Source>(&context)) {
            auto& menu=scene.menu.emplace<EndLevel8SceneNative>(photos);
            result=end_level_8_scene_native_prepare(source,menu);
            if(result==ISLE_GAME_OK)result=isle_post_course_source_call(&source->common,
                [](IslePostCourseSceneSource*,void* raw) {
                    auto& owner=*static_cast<Impl*>(raw);
                    isle_end_level_8_source_initialize(&std::get<IsleEndLevel8Source>(owner.context),
                        owner.modules.window_generation);
                },this);
        } else if(auto* source=std::get_if<IsleCreditsSource>(&context)) {
            auto& menu=scene.menu.emplace<CreditsSceneNative>(photos);
            source->native=&credits_scene_native_table();
            result=isle_credits_source_initialize(source,argument);
            if(result==ISLE_GAME_OK)result=credits_scene_native_prepare(source,menu);
        } else return fail(ISLE_GAME_INVALID_STATE,"post-course cold menu kind");
        if(result!=ISLE_GAME_OK)return fail(result,"post-course typed native/data preparation");
        scene.base_immutable_count=scene.immutable.size();
        return ISLE_GAME_OK;
    } catch(const std::exception& error) {return fail(ISLE_GAME_INVALID_STATE,error.what());}
    catch(...) {return fail(ISLE_GAME_EXECUTION_FAILURE,"post-course typed menu preparation threw");}
}
IsleGameResult Scene::Impl::setup_begin(std::int32_t entry_argument,std::uint32_t screen,
    std::uint32_t setup,std::uint32_t flags,int near_disabled,int update_interval,int draw_interval) noexcept {
    try {
        if(!requested_screen) {
            const auto admitted=admit_layout();
            if(admitted!=ISLE_GAME_OK)return admitted;
        }
        const int expected_draw=kind==Resources::Kind::camera_check || kind==Resources::Kind::credits ? 2:1;
        if(requested || native_retired || entry_argument!=argument ||
           screen!=requested_screen || setup!=requested_setup || flags!=screen_words[6] ||
           near_disabled!=1 || update_interval!=1 || draw_interval!=expected_draw)
            return fail(ISLE_GAME_INVALID_ARGUMENT,"original post-course Screen/Setup intent mismatch");
        if(native && (kind==Resources::Kind::album || kind==Resources::Kind::report)) {
            scene::graphics::SourceScreen applied;
            applied.width=screen_words[4];applied.height=screen_words[5];applied.flags=screen_words[6];
            std::copy_n(screen_words.begin(),3,applied.framebuffer_identity.begin());
            if(!native->graphics->configure_source_screen(applied,nullptr))
                return fail(ISLE_GAME_INVALID_STATE,native->graphics->message());
        }
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
    if(setup_returned || native_retired) {*complete=1;*result=output_scene;}
    return ISLE_GAME_OK;
}

Scene::Impl::Native::Native(Impl& parent):owner(parent),
    heap_bytes([&] {
        switch(parent.kind) {
        case Resources::Kind::camera_check:return std::size_t{0x40000};
        case Resources::Kind::photo_check:return std::size_t{0x32000};
        case Resources::Kind::album:case Resources::Kind::gallery:return std::size_t{0x46000};
        case Resources::Kind::report:return std::size_t{0x2d000};
        case Resources::Kind::end_level_8:return std::size_t{0x32000};
        case Resources::Kind::credits:return std::size_t{parent.argument==19?0x64000u:0x78000u};
        }
        std::terminate();
    }()) {
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
        const bool original_pair=owner.kind==Resources::Kind::camera_check || owner.kind==Resources::Kind::photo_check;
        element_slots.reserve(original_pair?4:32);
        reservations.reserve(96);
        reservations.push_back({0x590000,0x10000});
        std::uint32_t next=0x710000;
        const auto add_elements=[&](std::size_t count,std::uint32_t bytes,bool alternate=false) {
            for(std::size_t i=0;i<count;++i) {
                const auto planes=alternate?2u:1u;
                if(std::uint64_t(next)+std::uint64_t(bytes)*planes>0x7be600u)
                    throw std::runtime_error("post-course Element planes overlap retained Replay targets");
                element_slots.push_back({{next,alternate?next+bytes:0},bytes});
                reservations.push_back({next,bytes});
                if(alternate)reservations.push_back({next+bytes,bytes});
                next+=bytes*planes;
            }
        };
        switch(owner.kind) {
        case Resources::Kind::camera_check:case Resources::Kind::photo_check:
            add_elements(4,0x10000,true);break;
        case Resources::Kind::album:
            add_elements(29,0x2000);add_elements(2,0x10000);add_elements(1,0x24000);break;
        case Resources::Kind::report:
            add_elements(28,0x2000);add_elements(2,0x10000);add_elements(2,0x18000);break;
        case Resources::Kind::gallery:add_elements(8,0x10000);break;
        case Resources::Kind::end_level_8:add_elements(1,0x10000);break;
        case Resources::Kind::credits:add_elements(14,0x5000);break;
        }
        const auto add_thumbnails=[&](std::size_t count,std::uint32_t identity,bool primary_only=false) {
            const auto bytes=primary_only?std::size_t{8*3920}:Window::thumbnail_plane_bytes(*resources,identity);
            if(bytes>std::numeric_limits<std::uint32_t>::max()-63u)
                throw std::runtime_error("post-course thumbnail plane size");
            const auto plane=(static_cast<std::uint32_t>(bytes)+63u)&~63u;
            for(std::size_t i=0;i<count;++i) {
                const auto planes=primary_only?1u:2u;
                if(!plane || std::uint64_t(next)+std::uint64_t(planes)*plane>0x7be600u)
                    throw std::runtime_error("post-course thumbnail planes overlap retained Replay targets");
                thumbnail_slots.push_back({{next,primary_only?0:next+plane},plane});
                reservations.push_back({next,plane});
                if(!primary_only)reservations.push_back({next+plane,plane});
                next+=plane*planes;
            }
        };
        thumbnail_slots.reserve(11);
        switch(owner.kind) {
        case Resources::Kind::camera_check:
            add_thumbnails(7,0x80201520);add_thumbnails(1,0x80208900);break;
        case Resources::Kind::photo_check:add_thumbnails(6,0x801f3b10);break;
        case Resources::Kind::album:
            add_thumbnails(7,0x80201728);add_thumbnails(1,0x80208b10);break;
        case Resources::Kind::report:
            add_thumbnails(6,0x801f67e8);add_thumbnails(1,0x80202ce8);break;
        case Resources::Kind::gallery:add_thumbnails(11,0x801ea1b0);break;
        case Resources::Kind::end_level_8:break;
        case Resources::Kind::credits:add_thumbnails(5,0x801e6058,true);break;
        }
        const auto sprite_assets=producer->assets();
        const auto common_assets=common->common_render_assets();
        immutable.reserve(sprite_assets.size()+common_assets.size()+32);
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
        config.transition_memory={0x5a0000,0x10000};
        auto created_frame=scene::graphics::Frame::create(owner.snapshot,config);
        if(!created_frame)throw std::runtime_error(created_frame.message.data());
        graphics=std::move(created_frame.frame);
        frame_reservations.reserve(32);
        frame_reservations.push_back({0x590000,0x10000});
        const auto transition_memory=graphics->transition_workspace();
        reservations.push_back(transition_memory);frame_reservations.push_back(transition_memory);
        for(const auto asset:graphics->transition_immutable()) {
            bool present=false;
            for(const auto& existing:immutable) {
                if(existing.address!=asset.address)continue;
                if(existing.bytes.size()!=asset.bytes.size() ||
                   !std::equal(existing.bytes.begin(),existing.bytes.end(),asset.bytes.begin()))
                    throw std::runtime_error("post-course transition immutable identity collision");
                present=true;break;
            }
            if(!present)immutable.push_back(asset);
        }
        for(const auto& asset:immutable)
            if(overlaps({asset.address,static_cast<std::uint32_t>(asset.bytes.size())},transition_memory))
                throw std::runtime_error("post-course transition workspace overlaps immutable bytes");
        base_immutable_count=immutable.size();
        if(original_pair) {
            scene::graphics::SourceScreen screen;
            screen.width=owner.screen_words[4];screen.height=owner.screen_words[5];screen.flags=owner.screen_words[6];
            std::copy_n(owner.screen_words.begin(),3,screen.framebuffer_identity.begin());
            // Explicit Source NULL is not permission to substitute core depth.
            if(!graphics->configure_source_screen(screen,nullptr))
                throw std::runtime_error(graphics->message());
        }
        if(original_pair && !graphics->reserve_render_capacity({draws.size(),32,33,33}))
            throw std::runtime_error(graphics->message());
        source_objects.reserve(128);
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
        config.background_rgb=&state.background_rgb;
        if(kind==Resources::Kind::end_level_8) {
            config.dl_link=5;config.world_object_link=9;config.photo_object_link=8;
        }
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
        std::uint32_t background=0;
        const auto color=background_color(&background);
        if(color!=ISLE_GAME_OK)return color;
        if(!scene.pipeline->initialize_preview_camera(background,&scene.preview_object,&scene.preview_camera))
            return fail(ISLE_GAME_INVALID_STATE,scene.pipeline->error());
        scene.replay=std::make_unique<Replay>(scene.storage.game(),photos,*scene.pipeline,progress,audio,
                                            ReplayNativeProgress{this,native_progress});
        source().replay=scene.replay->source();scene.replay_initialized=true;
        if(kind==Resources::Kind::gallery || kind==Resources::Kind::end_level_8) {
            const auto initialized=initialize_presentation();
            if(initialized!=ISLE_GAME_OK)return initialized;
        }
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
        output={binding.executor,binding.programs,binding.epoch,binding.admission,native.graphics.get()};
        return photo::ReplayAcquire::admitted;
    }
    return photo::ReplayAcquire::failed;
}
bool Scene::Impl::release_gpu(void* raw,const photo::ReplayGpuBinding& binding) noexcept {
    if(!raw)return false;
    auto& owner=*static_cast<Impl*>(raw);
    if(owner.creator!=std::this_thread::get_id() || !owner.native || !owner.native->graphics ||
       binding.frame!=owner.native->graphics.get())return false;
    const bool released=owner.native->graphics->release_source_gpu(
        {binding.executor,binding.programs,binding.epoch,binding.admission});
    if(!released)owner.fail(ISLE_GAME_INVALID_STATE,"post-course actual Replay GPU release rejected");
    return released;
}

bool Scene::Impl::Native::join_setup() noexcept {
    if(owner.setup_returned)return true;
    if(owner.creator!=std::this_thread::get_id() || owner.callbacks_active ||
       owner.entry_resuming || owner.native_pumping || !activated || !services ||
       !services->exit_requested())return false;
    if(graphics) {
        const auto status=graphics->poll();
        if(status==scene::graphics::Poll::failed)owner.fail(ISLE_GAME_EXECUTION_FAILURE,graphics->message());
        const auto completed=graphics->completed();
        if((completed && completed.ordinal!=released_ordinal) || !graphics->quiescent())return false;
    }
    if(isle_game_require_idle(storage.game())!=ISLE_GAME_OK)return false;
    // omSetupScene has stopped dispatch. Suspended main processes and every
    // backing owner stay alive while Original entry joins its independent worker.
    owner.output_scene=services->exit_result();owner.setup_returned=true;
    return true;
}
bool Scene::Impl::Native::retire() noexcept {
    if(retired)return true;
    if(owner.creator!=std::this_thread::get_id() || owner.entry_resuming ||
       owner.callbacks_active || owner.native_pumping)return false;
    if(graphics) {
        const auto status=graphics->poll();
        if(status==scene::graphics::Poll::failed)owner.fail(ISLE_GAME_EXECUTION_FAILURE,graphics->message());
        const auto completed=graphics->completed();
        if(completed && completed.ordinal!=released_ordinal) {
            return false;
        }
    }
    if(auto* credits=std::get_if<CreditsSceneNative>(&menu);credits && credits->worker && !credits->joined()) {
        owner.native_pumping=true;
        const auto stopped=credits->request_cancel();
        int joined=0;const auto status=credits->join(&joined);
        owner.native_pumping=false;
        if(stopped!=ISLE_GAME_OK && stopped!=ISLE_GAME_STOPPED)
            owner.fail(stopped,"Credits worker cancellation retained a native failure");
        if(status!=ISLE_GAME_OK && status!=ISLE_GAME_STOPPED)
            owner.fail(status,"Credits worker join retained a native failure");
        if(!joined)return false;
    }
    if(presentation) {
        const auto status=presentation->retire();
        if(presentation->error()[0])owner.fail(ISLE_GAME_EXECUTION_FAILURE,presentation->error());
        if(status!=ISLE_PHOTO_PRESENTATION_RETIRED)return false;
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
        if(auto* credits=std::get_if<IsleCreditsSource>(&owner.context);credits && credits->worker && activated) {
            const auto released=isle_credits_source_release(credits);
            if(released!=ISLE_GAME_OK && released!=ISLE_GAME_STOPPED)
                owner.fail(released,"Credits image release retained a worker failure");
            // Release reports the retained worker fault even after its real
            // stack, native borrowers and every destination have joined.
            if(!credits->worker->joined() || credits->destination_count)return false;
        }
        if(window->retire()!=ISLE_GAME_OK) {
            owner.fail(ISLE_GAME_INVALID_STATE,"post-course Window bindings have not retired");return false;
        }
    }
    if(!owner.setup_returned && !owner.cancelling && services && services->exit_requested())
        owner.output_scene=services->exit_result();
    menu.emplace<std::monostate>();
    presentation.reset();replay.reset();pipeline.reset();window.reset();
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
IsleGameResult Scene::Impl::accept_entry_result() noexcept {
    if(!setup_returned && !native_retired)
        return fail(ISLE_GAME_INVALID_STATE,"original entry returned before setup retirement");
    if(const auto* camera=std::get_if<IsleCameraCheckSource>(&context)) {
        if(!camera->entry_complete || camera->source_result!=output_scene)
            return fail(ISLE_GAME_INVALID_STATE,"CameraCheck original entry result mismatch");
    } else if(const auto* credits=std::get_if<IsleCreditsSource>(&context)) {
        if(!credits->entry_complete || credits->source_result!=8 ||
           !native || !std::get<CreditsSceneNative>(native->menu).joined())
            return fail(ISLE_GAME_INVALID_STATE,"Credits original entry returned before its worker joined");
        output_scene=credits->source_result;
    } else {
        const auto accepted=std::visit([&](const auto& call)->bool {
            using T=std::remove_cvref_t<decltype(call)>;
            if constexpr(std::is_same_v<T,std::monostate>)return false;
            else {
                if(call.status!=ISLE_GAME_OK)return false;
                if constexpr(std::is_same_v<T,IsleAlbumEntry>) {if(!call.complete)return false;}
                if constexpr(std::is_same_v<T,IsleGalleryEntry>) {if(!call.completed)return false;}
                if constexpr(std::is_same_v<T,IslePhotoCheckEntry>) {if(call.result!=output_scene)return false;}
                output_scene=call.result;return true;
            }
        },entry_context);
        if(!accepted)return fail(ISLE_GAME_INVALID_STATE,"original post-course entry result admission failed");
    }
    entry_returned=true;
    return ISLE_GAME_OK;
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
    if(owner.entry_returned) {
        if(owner.native_retired)owner.phase=SceneStatus::completed;
        return ISLE_GAME_OK;
    }
    if(!owner.entry || (owner.requested && !owner.setup_returned && !owner.native_retired))
        return ISLE_GAME_OK;
    owner.entry_resuming=true;
    const auto resumed=isle_continuation_resume(owner.entry);
    owner.entry_resuming=false;
    if(resumed!=ISLE_CONT_OK)
        return owner.fail(continuation_result(resumed),isle_continuation_error(owner.domain));
    if(owner.source().status==ISLE_GAME_STOPPED)return owner.request_stop();
    if(owner.source().status!=ISLE_GAME_OK)
        return owner.fail(owner.source().status,"post-course original entry retained a leaf fault");
    IsleContinuationState state;
    const auto inspected=isle_continuation_state(owner.entry,&state);
    if(inspected!=ISLE_CONT_OK)return owner.fail(continuation_result(inspected),"post-course entry state");
    if(state==ISLE_CONT_COMPLETED) {
        const auto accepted=owner.accept_entry_result();
        if(accepted!=ISLE_GAME_OK)return accepted;
        if(owner.native_retired)owner.phase=SceneStatus::completed;
    } else if(state==ISLE_CONT_FAILED || state==ISLE_CONT_CANCELLED) {
        return owner.fail(ISLE_GAME_EXECUTION_FAILURE,"post-course original entry did not complete");
    }
    return ISLE_GAME_OK;
}
bool Scene::setup_pending() const noexcept {return p_->phase==SceneStatus::setup_pending && !p_->source_initialized;}
IsleGameResult Scene::initialize_scene() noexcept {
    auto& owner=*p_;
    if(owner.creator!=std::this_thread::get_id())return ISLE_GAME_WRONG_OWNER;
    if(owner.failure!=ISLE_GAME_OK)return owner.failure;
    if(!setup_pending() || owner.native_retired || owner.cancelling)
        return owner.fail(ISLE_GAME_INVALID_STATE,"post-course native initialization without original setup intent");
    try {
        const int draws=owner.kind==Resources::Kind::camera_check || owner.kind==Resources::Kind::credits?2:1;
        if(owner.clock.set_intervals(1,draws)!=host::ClockError::none)
            return owner.fail(ISLE_GAME_INVALID_STATE,"original post-course GTL interval admission");
        auto& source=owner.source();
        if(!owner.native) {
            owner.native=std::make_unique<Impl::Native>(owner);
            source.game=owner.native->storage.game();source.window=owner.native->window->source();
            const auto activated=isle_post_course_source_activate(&source);
            if(activated!=ISLE_GAME_OK) {
                source.game=nullptr;source.window=nullptr;
                return owner.fail(activated,"post-course Source context activation");
            }
            owner.native->activated=true;
        }
        IslePostCourseNonwaitingCall initialize=nullptr;
        switch(owner.kind) {
        case Resources::Kind::camera_check:initialize=isle_camera_check_source_init;break;
        case Resources::Kind::photo_check:initialize=isle_photo_check_source_init_call;break;
        case Resources::Kind::album:initialize=isle_album_source_init_call;break;
        case Resources::Kind::report:initialize=isle_report_source_init_call;break;
        case Resources::Kind::gallery:initialize=isle_gallery_source_init_call;break;
        case Resources::Kind::end_level_8:initialize=isle_end_level_8_source_init_call;break;
        case Resources::Kind::credits:initialize=isle_credits_source_init;break;
        }
        owner.callbacks_active=true;
        const auto initialized=isle_post_course_source_call(&source,initialize,nullptr);
        owner.callbacks_active=false;
        if(initialized!=ISLE_GAME_OK && initialized!=ISLE_GAME_STOPPED)
            return owner.fail(initialized,"original post-course nonwaiting C initialization");
        if(initialized==ISLE_GAME_STOPPED)return owner.request_stop();
        if(!owner.native->heap_admitted || !owner.native->input_initialized || !owner.native->replay_initialized)
            return owner.fail(ISLE_GAME_INVALID_STATE,"original post-course C initialization omitted native service admission");
        // Cold menus apply their actual Source screen during entry/C init.
        // Capacity seals that configuration only after those calls complete.
        if(owner.kind!=Resources::Kind::camera_check && owner.kind!=Resources::Kind::photo_check &&
           !owner.native->graphics->reserve_render_capacity({owner.native->draws.size(),32,33,33}))
            return owner.fail(ISLE_GAME_INVALID_STATE,owner.native->graphics->message());
        owner.source_initialized=true;owner.phase=SceneStatus::running;return ISLE_GAME_OK;
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
    owner.callbacks_active=true;
    const auto updated=native.services->tick();
    owner.callbacks_active=false;
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
    if(owner.entry_resuming || owner.callbacks_active || owner.native_pumping)return false;
    if(owner.native_retired)return true;
    if(!owner.cancelling && owner.failure==ISLE_GAME_OK && owner.phase!=SceneStatus::retiring)return false;
    const bool original_pair=owner.kind==Resources::Kind::camera_check || owner.kind==Resources::Kind::photo_check;
    if(!original_pair && !owner.cancelling && owner.failure==ISLE_GAME_OK && !owner.entry_returned) {
        if(!owner.native || !owner.native->join_setup())return false;
        const auto resumed=resume();
        if(resumed!=ISLE_GAME_OK && resumed!=ISLE_GAME_STOPPED)return false;
        if(!owner.entry_returned)return false;
    }
    if(owner.native && !owner.native->retire())return false;
    owner.native.reset();owner.native_retired=true;
    if(owner.failure!=ISLE_GAME_OK)owner.phase=SceneStatus::failed;
    else if(owner.cancelling)owner.phase=SceneStatus::cancelled;
    else if(owner.entry_returned)owner.phase=SceneStatus::completed;
    return true;
}
IsleGameResult Scene::cancel() noexcept {
    auto& owner=*p_;
    if(owner.creator!=std::this_thread::get_id())return ISLE_GAME_WRONG_OWNER;
    if(owner.phase==SceneStatus::retired)return owner.failure;
    (void)owner.request_stop();
    // Host progress may request stop while a parked C caller still owns this
    // native RPC. Signal only until that real callback/resume returns.
    if(owner.entry_resuming || owner.callbacks_active || owner.native_pumping)return owner.failure;
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
const IsleAlbumSource* Scene::album_source() const noexcept {return std::get_if<IsleAlbumSource>(&p_->context);}
const IsleReportSource* Scene::report_source() const noexcept {return std::get_if<IsleReportSource>(&p_->context);}
const IsleGallerySource* Scene::gallery_source() const noexcept {return std::get_if<IsleGallerySource>(&p_->context);}
const IsleEndLevel8Source* Scene::end_level_8_source() const noexcept {return std::get_if<IsleEndLevel8Source>(&p_->context);}
const IsleCreditsSource* Scene::credits_source() const noexcept {return std::get_if<IsleCreditsSource>(&p_->context);}
const IsleCameraCheckInput* Scene::camera_input_observation(std::size_t controller) const noexcept {
    if (!camera_source() || !p_->native || p_->native->retired ||
        controller >= p_->native->camera_inputs.size() || !p_->native->camera_input_observed[controller])
        return nullptr;
    return &p_->native->camera_inputs[controller];
}
}

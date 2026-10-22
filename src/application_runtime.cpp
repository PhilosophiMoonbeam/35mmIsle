#include "application_runtime.hpp"
#include "beach_initial_species.hpp"
#include "beach_snorlax_kangaskhan.hpp"
#include "beach_meowth.hpp"
#include "beach_eevee_chansey.hpp"
#include "beach_scyther_special.hpp"
#include "new_game_scene.hpp"
#include "oaks_lab_scene.hpp"
#include "oaks_lab_resources.hpp"
#include "startup_runtime.hpp"
#include "post_course_scene.hpp"
#include "tunnel_scene.hpp"
#include "volcano_scene.hpp"
#include "river_scene.hpp"
#include "cave_scene.hpp"
#include "valley_scene.hpp"
#include <algorithm>
#include <cstdio>
#include <exception>
#include <limits>
#include <new>
#include <utility>
#include <vector>
#include <variant>

namespace isle::application {
namespace {
void text(std::array<char,512>& output,const char* value) noexcept {
    std::snprintf(output.data(),output.size(),"%s",value?value:"native application failure");
}
template<class T,std::size_t N>
std::vector<T> concatenate(const std::array<std::span<const T>,N>& groups) {
    std::size_t count=0;
    for (const auto& group:groups) count+=group.size();
    std::vector<T> result;
    result.reserve(count);
    for (const auto& group:groups) result.insert(result.end(),group.begin(),group.end());
    return result;
}
void unique(std::vector<std::uint32_t>& values) {
    std::sort(values.begin(),values.end());
    values.erase(std::unique(values.begin(),values.end()),values.end());
}
struct WindowScene {
    std::vector<const IsleSprite*> roots;
    std::optional<sprite::Producer> producer;
    std::optional<scene::Services> services;
    std::optional<new_game::Scene> name;
    std::optional<oaks_lab::Scene> oak;
    void retire() noexcept {
        if (name) name->retire();
        if (oak) oak->retire();
        oak.reset();name.reset();services.reset();producer.reset();
    }
};
}
struct Runtime::Impl {
    const rom::Snapshot& snapshot;
    const menu::Catalog& catalog;
    game::State& state;
    game_input::Service& input;
    game_audio::Session& audio;
    host::GtlClock& clock;
    RuntimeConfig config;
    std::vector<actor::BehaviorCallbackBinding> callbacks;
    std::vector<std::uint32_t> graphs,roots;
    std::unique_ptr<Manager> manager;
    EntryBorrow entry;
    std::unique_ptr<Photos> photos;
    std::shared_ptr<const window::Resources> window_resources;
    std::shared_ptr<const oaks_lab::Resources> oak_resources;
    scene::Storage storage;
    std::unique_ptr<scene::graphics::Frame> graphics;
    std::unique_ptr<startup::Runtime> startup;
    std::unique_ptr<WindowScene> window;
    std::unique_ptr<beach::Lifecycle> beach;
    std::unique_ptr<rainbow::Lifecycle> rainbow;
    using NativeCourses=std::variant<std::unique_ptr<tunnel::Lifecycle>,
        std::unique_ptr<volcano::Lifecycle>,std::unique_ptr<river::Lifecycle>,
        std::unique_ptr<cave::Lifecycle>,std::unique_ptr<valley::Lifecycle>>;
    NativeCourses courses;
    std::unique_ptr<post_course::Scene> post;
    post_course::Modules post_modules;
    host::Progress* progress = nullptr;
    post_course::ReplayNativeProgress native_service{};
    std::optional<host::FrameToken> executing;
    game_input::RequestToken publication{};
    std::optional<std::int32_t> result;
    std::array<char,512> diagnostic{};
    IsleGameResult failure=ISLE_GAME_OK;
    std::uint64_t resource_identity=1;
    bool updating=false,publication_ready=false,draw_accepted=false,update_abandoned=false;
    bool completed_hold=false,transition=false,stopping=false,closed=false;

    Impl(const rom::Snapshot& s,const menu::Catalog& c,game::State& g,
         game_input::Service& i,game_audio::Session& a,host::GtlClock& h,RuntimeConfig options)
        : snapshot(s),catalog(c),state(g),input(i),audio(a),clock(h),config(options),
          callbacks(concatenate<actor::BehaviorCallbackBinding>(std::array{
              beach::initial_species_callbacks(),beach::snorlax_kangaskhan_callbacks(),
              beach::meowth_callbacks(),beach::eevee_chansey_callbacks(),beach::scyther_special_callbacks()})),
          graphs(concatenate<std::uint32_t>(std::array{
              beach::initial_species_graphs(),beach::snorlax_kangaskhan_graphs(),
              beach::meowth_graphs(),beach::eevee_chansey_graphs(),beach::scyther_special_graphs()})) {
        std::sort(callbacks.begin(),callbacks.end(),[](const auto& lhs,const auto& rhs){return lhs.identity<rhs.identity;});
        unique(graphs);
        // Resolution heads include complete compiled families, including
        // original callbacks not reachable from a particular initial state.
        // This admits code; original C alone chooses what actually executes.
        roots.reserve(callbacks.size());
        for (const auto& callback:callbacks) roots.push_back(callback.identity);
    }
    bool has_course() const noexcept {
        return std::visit([](const auto& value) { return bool(value); },courses);
    }
    template<class F> decltype(auto) with_course(F&& function) noexcept {
        return std::visit([&](auto& value)->decltype(auto) { return function(*value); },courses);
    }
    template<class F> decltype(auto) with_course(F&& function) const noexcept {
        return std::visit([&](const auto& value)->decltype(auto) { return function(*value); },courses);
    }
    template<class C> C* course_pointer() const noexcept {
        const auto* value=std::get_if<std::unique_ptr<C>>(&courses);
        return value ? value->get() : nullptr;
    }
    template<class C> bool checked_course(C& current,IsleGameResult value) noexcept {
        const auto error=current.error();
        if (error!=ISLE_GAME_OK && error!=ISLE_GAME_STOPPED) return fail(error,current.message());
        if (value==ISLE_GAME_STOPPED || error==ISLE_GAME_STOPPED ||
            current.status()==course::LifecycleStatus::cancelled) {
            request_stop(); return false;
        }
        return checked(value,current.message());
    }
    template<class C> void observe_course_join(C& current,IsleGameResult value) noexcept {
        const auto error=current.error();
        if (value==ISLE_GAME_STOPPED || error==ISLE_GAME_STOPPED ||
            current.status()==course::LifecycleStatus::cancelled) request_stop();
        if (error!=ISLE_GAME_OK && error!=ISLE_GAME_STOPPED) fail(error,current.message());
        else if (value!=ISLE_GAME_OK && value!=ISLE_GAME_STOPPED && value!=ISLE_GAME_INVALID_STATE)
            fail(value,current.message());
    }
    void request_stop() noexcept {
        stopping=true;clock.request_stop();
    }
    bool fail(IsleGameResult error,const char* message) noexcept {
        if (failure==ISLE_GAME_OK) { failure=error;text(diagnostic,message); }
        request_stop();
        return false;
    }
    bool checked(IsleGameResult value,const char* message) noexcept {
        return value==ISLE_GAME_OK||fail(value,message);
    }
    bool checked(host::ClockError value,const char* message) noexcept {
        return value==host::ClockError::none||fail(ISLE_GAME_INVALID_STATE,message);
    }
    bool checked_rainbow(IsleGameResult value) noexcept {
        const auto error=rainbow->error();
        if (error!=ISLE_GAME_OK&&error!=ISLE_GAME_STOPPED) return fail(error,rainbow->message());
        if (value==ISLE_GAME_STOPPED||error==ISLE_GAME_STOPPED||
            rainbow->status()==rainbow::LifecycleStatus::cancelled) {
            request_stop();return false;
        }
        return checked(value,rainbow->message());
    }
    void observe_rainbow_join(IsleGameResult value) noexcept {
        const auto error=rainbow->error();
        if (value==ISLE_GAME_STOPPED||error==ISLE_GAME_STOPPED||
            rainbow->status()==rainbow::LifecycleStatus::cancelled) request_stop();
        if (error!=ISLE_GAME_OK&&error!=ISLE_GAME_STOPPED) fail(error,rainbow->message());
        // Native cancellation/retirement is nonblocking. INVALID_STATE with
        // no latched error retains real borrowers for the next host iteration.
        else if (value!=ISLE_GAME_OK&&value!=ISLE_GAME_STOPPED&&value!=ISLE_GAME_INVALID_STATE)
            fail(value,rainbow->message());
    }
    bool active() const noexcept { return has_course()||bool(startup)||bool(window)||bool(beach)||bool(rainbow)||bool(post); }
    bool busy() const noexcept {
        if (has_course()) return with_course([](const auto& current) { return current.graphics_busy(); });
        return post?post->graphics_busy():rainbow?rainbow->graphics_busy():beach?beach->graphics_busy():graphics&&graphics->busy();
    }
    const char* graphics_message() const noexcept {
        if (has_course()) return with_course([](const auto& current) { return current.message(); });
        return post?post->message():rainbow?rainbow->message():beach?beach->message():graphics->message();
    }
    scene::graphics::Completed completed() const noexcept {
        if (!completed_hold) return {};
        if (has_course()) return with_course([](const auto& current) { return current.completed_frame(); });
        return post?post->completed_frame():rainbow?rainbow->completed_frame():beach?beach->completed_frame():graphics->completed();
    }
    bool terminal(host::Terminal value) noexcept {
        if (!executing) return fail(ISLE_GAME_INVALID_STATE,"application lost its actual graphics context");
        const auto token=*executing;
        const auto ended=draw_accepted?clock.offer_terminal(token,value):clock.abandon_draw(token);
        executing.reset();draw_accepted=false;
        return checked(ended,"application GTL terminal rejected");
    }
    bool poll_graphics() noexcept {
        if (!executing||completed_hold) return true;
        const auto polled=has_course()?with_course([](auto& current) { return current.poll(); }):
            post?post->poll():rainbow?rainbow->poll():beach?beach->poll():graphics->poll();
        if (polled==scene::graphics::Poll::pending) return true;
        if (polled==scene::graphics::Poll::completed) {
            completed_hold=true;
            return true;
        }
        if (has_course()) return with_course([&](auto& current) {
            const auto error=current.error();
            if (error==ISLE_GAME_STOPPED || current.status()==course::LifecycleStatus::cancelled) request_stop();
            else fail(error==ISLE_GAME_OK?ISLE_GAME_EXECUTION_FAILURE:error,current.message());
            if (current.graphics_busy()) return true;
            if (current.release_completed()!=ISLE_GAME_OK) {
                observe_course_join(current,current.cancel());
                if (!current.retire_scene()) { observe_course_join(current,ISLE_GAME_OK); return true; }
            }
            observe_course_join(current,ISLE_GAME_OK);
            return terminal(failure==ISLE_GAME_OK?host::Terminal::cancelled:host::Terminal::failed);
        });
        if (rainbow) {
            const auto error=rainbow->error();
            if (error==ISLE_GAME_STOPPED||rainbow->status()==rainbow::LifecycleStatus::cancelled)
                request_stop();
            else fail(error==ISLE_GAME_OK?ISLE_GAME_EXECUTION_FAILURE:error,rainbow->message());
            if (busy()) return true;
            // A failed renderer can retain a completed Photo capability after
            // GPU execution has ceased. Close that actual consumer before the
            // GTL terminal, without publishing a successful Source result.
            if (rainbow->release_completed()!=ISLE_GAME_OK) {
                observe_rainbow_join(rainbow->cancel());
                if (!rainbow->retire_scene()) { observe_rainbow_join(ISLE_GAME_OK);return true; }
            }
            observe_rainbow_join(ISLE_GAME_OK);
            return terminal(failure==ISLE_GAME_OK?host::Terminal::cancelled:host::Terminal::failed);
        }
        fail(ISLE_GAME_EXECUTION_FAILURE,graphics_message());
        if (busy()) return true;
        if (beach) {
            // A failed frame may still own a completed Photo capability. It is
            // not an image or an invented successful terminal; close its real
            // local consumers through the lifecycle's cancellation/retirement.
            if (beach->release_completed()!=ISLE_GAME_OK) {
                (void)beach->cancel();
                if (!beach->retire_scene()) return true;
            }
        }
        return terminal(host::Terminal::failed);
    }
    bool release() noexcept {
        if (!completed_hold||!executing) return fail(ISLE_GAME_INVALID_STATE,"no completed application frame to release");
        if (post&&!checked(post->release_completed(),post->message())) return false;
        // Beach/Rainbow presentation release is only the caller/GTL terminal.
        // Their last completed Photo stays owned for Source CopyInfo/shutter
        // until the lifecycle's next draw preparation or actual retirement.
        completed_hold=false;
        return terminal(host::Terminal::completed);
    }
    static IsleGameResult service_post_course(void* raw) noexcept {
        auto& owner=*static_cast<Impl*>(raw);
        (void)owner.poll_graphics();
        const auto result=owner.native_service.service(owner.native_service.owner);
        if (result==ISLE_GAME_STOPPED) owner.request_stop();
        else if (result!=ISLE_GAME_OK) owner.fail(result,"actual post-course native display service failed");
        return owner.failure!=ISLE_GAME_OK?owner.failure:owner.stopping?ISLE_GAME_STOPPED:ISLE_GAME_OK;
    }
    bool create_window(bool name) {
        if (!window_resources) window_resources=window::Resources::load(snapshot);
        if (!name&&!oak_resources) oak_resources=oaks_lab::Resources::load(snapshot,window_resources);
        if (resource_identity==std::numeric_limits<std::uint64_t>::max())
            return fail(ISLE_GAME_EXHAUSTED,"application resource identity exhausted");
        if (!checked(storage.reset(),"window scene arena reset failed")||
            !checked(storage.create(),"window scene native domain creation failed")) return false;
        window=std::make_unique<WindowScene>();
        if (name) {
            const auto common=window_resources->sprites(window::ResourceBank::Group::common);
            const auto names=window_resources->sprites(window::ResourceBank::Group::name);
            const auto all=window_resources->sprites();
            if (common.data()!=all.data()||names.data()!=all.data()+common.size())
                return fail(ISLE_GAME_INVALID_STATE,"name sprite banks are not the original contiguous domain");
            const auto sprites=all.first(common.size()+names.size());
            window->roots.reserve(sprites.size());
            for (const auto& sprite:sprites) window->roots.push_back(sprite.sprite.view());
            const auto banks=window_resources->banks();
            if (banks.size()!=3) return fail(ISLE_GAME_INVALID_STATE,"window bank residency is incomplete");
            const std::array<sprite::BankMapping,2> mapping{{{banks[0].owner,banks[0].guest},{banks[1].owner,0x700000}}};
            window->producer.emplace(window->roots,mapping);
            window->services.emplace(storage.game(),storage.arena(),
                scene::ResourceView{{64,64,128,8,1024},sprites},input,audio,state);
            new_game::Config options;
            options.resource_identity=resource_identity++;
            window->name.emplace(*window->services,window_resources,*window->producer,options);
            return window->name->start()||fail(ISLE_GAME_EXECUTION_FAILURE,window->name->error());
        }
        const auto banks=oak_resources->banks();
        if (banks.size()!=2) return fail(ISLE_GAME_INVALID_STATE,"Oak bank residency is incomplete");
        const std::array<sprite::BankMapping,2> mapping{{{banks[0].owner,banks[0].guest_begin},{banks[1].owner,0x700000}}};
        window->producer.emplace(oak_resources->roots(),mapping);
        window->services.emplace(storage.game(),storage.arena(),oak_resources->view(),input,audio,state);
        oaks_lab::Config options;
        options.level_id=manager->source_level();
        options.resource_identity=resource_identity++;
        window->oak.emplace(*window->services,oak_resources,*window->producer,options);
        return window->oak->start()||fail(ISLE_GAME_EXECUTION_FAILURE,window->oak->error());
    }
    template<class C> bool create_course(const EntryIntent& request) {
        if (!state.save()) return fail(ISLE_GAME_INVALID_STATE,"course requested before actual boot profile initialization");
        if (!photos) photos=std::make_unique<Photos>(snapshot,*state.save());
        auto made=C::create(snapshot,*manager,state,input,audio,clock,*photos,post_modules.world,
            static_cast<std::int32_t>(request.argument),config.require_cpu_renderer,
            course::EntryNativeProfile::hosted_canonical_image);
        if (!made) {
            if (made.error==ISLE_GAME_STOPPED) { request_stop(); return true; }
            return fail(made.error,made.message.data());
        }
        courses=std::move(made.lifecycle);
        auto& current=*std::get<std::unique_ptr<C>>(courses);
        if (!checked_course(current,current.resume())) return failure==ISLE_GAME_OK;
        if (!current.pending_setup())
            return fail(ISLE_GAME_INVALID_STATE,"Original course omitted its actual scene setup");
        return checked_course(current,current.initialize_scene()) || failure==ISLE_GAME_OK;
    }
    bool enter(std::uint32_t raw_time) {
        const auto* request=manager->pending_entry();
        if (!request) return true;
        entry=manager->borrow_entry(request->serial);
        if (!entry) return fail(ISLE_GAME_INVALID_STATE,"original manager entry lifetime admission failed");
        post_modules.observe_world(manager->resources());
        switch (request->operation) {
        case ISLE_APPLICATION_ENTRY_BOOT:
        case ISLE_APPLICATION_ENTRY_TITLE: {
            auto made=request->operation==ISLE_APPLICATION_ENTRY_BOOT?
                startup::Runtime::create_boot(catalog,input,audio,state,storage,raw_time):
                startup::Runtime::create_title(catalog,input,audio,state,storage,raw_time);
            // CreateResult owns the native wrapper; never copy its ownership.
            if (!made) return fail(ISLE_GAME_EXECUTION_FAILURE,made.message.data());
            startup=std::move(made.runtime);
            return true;
        }
        case ISLE_APPLICATION_ENTRY_NAME:
            if (!checked(clock.set_intervals(1,2),"Name Source intervals rejected")) return false;
            return create_window(true);
        case ISLE_APPLICATION_ENTRY_OAKS_LAB:
            if (!checked(clock.set_intervals(1,2),"Oak Source intervals rejected")) return false;
            return create_window(false);
        case ISLE_APPLICATION_ENTRY_BEACH: {
            if (!state.save()) return fail(ISLE_GAME_INVALID_STATE,"Beach requested before actual boot profile initialization");
            if (!photos) photos=std::make_unique<Photos>(snapshot,*state.save());
            auto made=beach::Lifecycle::create(snapshot,*manager,state,input,audio,clock,*photos,post_modules.world,
                static_cast<std::int32_t>(request->argument),{callbacks,graphs,roots},config.require_cpu_renderer);
            if (!made) return fail(made.error,made.message.data());
            beach=std::move(made.lifecycle);
            if (!checked(beach->resume(),beach->message())) return false;
            if (!beach->pending_setup()) return fail(ISLE_GAME_INVALID_STATE,"original Beach did not publish its actual scene setup");
            return checked(beach->initialize_scene(),beach->message());
        }
        case ISLE_APPLICATION_ENTRY_RAINBOW: {
            if (!state.save()) return fail(ISLE_GAME_INVALID_STATE,"Rainbow requested before actual boot profile initialization");
            if (!photos) photos=std::make_unique<Photos>(snapshot,*state.save());
            auto made=rainbow::Lifecycle::create(snapshot,*manager,state,input,audio,clock,*photos,post_modules.world,
                static_cast<std::int32_t>(request->argument),config.require_cpu_renderer);
            if (!made) {
                if (made.error==ISLE_GAME_STOPPED) { request_stop();return true; }
                return fail(made.error,made.message.data());
            }
            rainbow=std::move(made.lifecycle);
            if (!checked_rainbow(rainbow->resume())) return failure==ISLE_GAME_OK;
            if (!rainbow->pending_setup())
                return fail(ISLE_GAME_INVALID_STATE,"original Rainbow did not publish its actual scene setup");
            if (!checked_rainbow(rainbow->initialize_scene())) return failure==ISLE_GAME_OK;
            return true;
        }
        case ISLE_APPLICATION_ENTRY_TUNNEL: return create_course<tunnel::Lifecycle>(*request);
        case ISLE_APPLICATION_ENTRY_VOLCANO: return create_course<volcano::Lifecycle>(*request);
        case ISLE_APPLICATION_ENTRY_RIVER: return create_course<river::Lifecycle>(*request);
        case ISLE_APPLICATION_ENTRY_CAVE: return create_course<cave::Lifecycle>(*request);
        case ISLE_APPLICATION_ENTRY_VALLEY: return create_course<valley::Lifecycle>(*request);
        case ISLE_APPLICATION_ENTRY_CAMERA_CHECK:
        case ISLE_APPLICATION_ENTRY_PHOTO_CHECK:
        case ISLE_APPLICATION_ENTRY_ALBUM:
        case ISLE_APPLICATION_ENTRY_REPORT:
        case ISLE_APPLICATION_ENTRY_GALLERY:
        case ISLE_APPLICATION_ENTRY_END_LEVEL_8:
        case ISLE_APPLICATION_ENTRY_CREDITS: {
            if (!progress||!native_service.service)
                return fail(ISLE_GAME_INVALID_STATE,"post-course entry requires bound actual Host progress");
            if (!photos) photos=std::make_unique<Photos>(snapshot,*state.save());
            post_modules.observe_window(snapshot,manager->resources());
            post_course::Resources::Kind kind;
            switch (request->operation) {
            case ISLE_APPLICATION_ENTRY_CAMERA_CHECK: kind=post_course::Resources::Kind::camera_check; break;
            case ISLE_APPLICATION_ENTRY_PHOTO_CHECK: kind=post_course::Resources::Kind::photo_check; break;
            case ISLE_APPLICATION_ENTRY_ALBUM: kind=post_course::Resources::Kind::album; break;
            case ISLE_APPLICATION_ENTRY_REPORT: kind=post_course::Resources::Kind::report; break;
            case ISLE_APPLICATION_ENTRY_GALLERY: kind=post_course::Resources::Kind::gallery; break;
            case ISLE_APPLICATION_ENTRY_END_LEVEL_8: kind=post_course::Resources::Kind::end_level_8; break;
            case ISLE_APPLICATION_ENTRY_CREDITS: kind=post_course::Resources::Kind::credits; break;
            default: return fail(ISLE_GAME_INVALID_STATE,"post-course entry has no native resource kind");
            }
            auto made=post_course::Scene::create(snapshot,*manager,entry,state,input,audio,clock,*photos,
                *progress,{this,service_post_course},post_modules,kind,
                static_cast<std::int32_t>(request->argument),config.require_cpu_renderer);
            if (!made) return fail(made.error,made.message.data());
            post=std::move(made.scene);
            const auto resumed=post->resume();
            if (resumed==ISLE_GAME_STOPPED) { request_stop();return true; }
            if (!checked(resumed,post->message())) return false;
            if (!post->setup_pending()) return fail(ISLE_GAME_INVALID_STATE,"original post-course entry omitted actual setup");
            const auto initialized=post->initialize_scene();
            if (initialized==ISLE_GAME_STOPPED) { request_stop();return true; }
            return checked(initialized,post->message());
        }
        default:
            // Lack of an executable entry is an application failure, never an
            // invented successful return, course alias or swallowed request.
            return fail(ISLE_GAME_INVALID_STATE,"original manager requested an entry without an executable native scene owner");
        }
    }
    bool update(std::uint32_t raw_time) noexcept {
        update_abandoned=false;
        if (has_course()) {
            const bool advanced=with_course([&](auto& current) {
                if (!checked_course(current,current.update(raw_time))) return false;
                update_abandoned=current.update_abandoned();
                transition=current.status()==course::LifecycleStatus::retiring || clock.stopping();
                return true;
            });
            if (!advanced) return failure==ISLE_GAME_OK;
        } else
        if (startup) {
            const auto step=startup->tick(raw_time);
            if (step==startup::Step::failed) return fail(ISLE_GAME_EXECUTION_FAILURE,startup->message());
            if (step==startup::Step::boot_complete) result=0; // Original Boot entry returns void.
            if (step==startup::Step::scene_exit) result=static_cast<std::int32_t>(startup->exit().scene);
            transition=result.has_value();
        } else if (window) {
            window->services->raw_game_time=raw_time;
            const auto updated=window->services->tick();
            if (updated==ISLE_GAME_STOPPED) { request_stop();return true; }
            if (!checked(updated,"native window OM update failed")) return false;
            if (window->services->failed()) return fail(ISLE_GAME_EXECUTION_FAILURE,window->services->error());
            if (window->name) {
                if (window->name->failed()) return fail(ISLE_GAME_EXECUTION_FAILURE,window->name->error());
                transition=window->name->exit_requested();
            } else {
                if (window->oak->failed()) return fail(ISLE_GAME_EXECUTION_FAILURE,window->oak->error());
                transition=window->oak->view().exit_requested;
            }
        } else if (beach) {
            if (input.begin_update()!=game_input::Error::none)
                return fail(ISLE_GAME_INVALID_STATE,"Beach common input cache invalidation failed");
            if (!checked(beach->update(raw_time),beach->message())) return false;
            update_abandoned=beach->update_abandoned();
            transition=beach->status()==beach::LifecycleStatus::retiring||clock.stopping();
        } else if (rainbow) {
            if (!checked_rainbow(rainbow->update(raw_time))) return failure==ISLE_GAME_OK;
            update_abandoned=rainbow->update_abandoned();
            transition=rainbow->status()==rainbow::LifecycleStatus::retiring||clock.stopping();
        } else if (post) {
            const auto updated=post->update(raw_time);
            if (updated==ISLE_GAME_STOPPED) { request_stop();return true; }
            if (!checked(updated,post->message())) return false;
            transition=post->status()==post_course::SceneStatus::retiring||clock.stopping();
        } else return fail(ISLE_GAME_INVALID_STATE,"OM update has no original scene owner");
        if (transition) clock.request_stop();
        return true;
    }
    bool draw(host::FrameToken token) noexcept {
        const bool begun=has_course()?with_course([&](auto& current) { return current.draw({token.context}); }):
            post?post->draw({token.context}):rainbow?rainbow->draw({token.context}):beach?beach->draw({token.context}):startup?
            graphics->begin(startup->view(),{token.context}):
            graphics->begin(window->name?window->name->render_input():window->oak->render_input(),{token.context});
        if (!begun && has_course()) with_course([&](auto& current) {
            const auto error=current.error();
            if (error==ISLE_GAME_STOPPED || current.status()==course::LifecycleStatus::cancelled) request_stop();
            else fail(error==ISLE_GAME_OK?ISLE_GAME_EXECUTION_FAILURE:error,current.message());
        });
        if (!begun&&rainbow) {
            const auto error=rainbow->error();
            if (error==ISLE_GAME_STOPPED||rainbow->status()==rainbow::LifecycleStatus::cancelled)
                request_stop();
            else fail(error==ISLE_GAME_OK?ISLE_GAME_EXECUTION_FAILURE:error,rainbow->message());
        }
        if (!begun&&!busy()) {
            (void)checked(clock.abandon_draw(token),"unadmitted graphics context abandonment failed");
            return (rainbow||has_course())?failure==ISLE_GAME_OK:fail(ISLE_GAME_EXECUTION_FAILURE,graphics_message());
        }
        executing=token;
        const auto accepted=clock.accept_draw(token);
        draw_accepted=accepted==host::ClockError::none;
        if (!checked(accepted,"actual graphics admission lost its GTL context")) return false;
        return begun||((rainbow||has_course())?failure==ISLE_GAME_OK:fail(ISLE_GAME_EXECUTION_FAILURE,graphics_message()));
    }
    bool retire_scene(bool completed_source) noexcept {
        if (executing||completed_hold||updating) return false;
        if (has_course()) {
            const bool joined=with_course([&](auto& current) {
                if (!completed_source) observe_course_join(current,current.cancel());
                if (!current.retire_scene()) { observe_course_join(current,ISLE_GAME_OK); return false; }
                if (completed_source) {
                    if (!checked_course(current,ISLE_GAME_OK)) return false;
                    if (!result) {
                        if (!checked_course(current,current.resume())) return false;
                        result=current.result();
                        if (!result) return fail(ISLE_GAME_INVALID_STATE,"course retired without its Original post-scene result");
                    }
                }
                observe_course_join(current,current.retire());
                return current.status()==course::LifecycleStatus::retired;
            });
            if (!joined) return false;
        }
        if (post) {
            if (!completed_source) {
                const auto cancelled=post->cancel();
                if (cancelled==ISLE_GAME_STOPPED) request_stop();
                else if (cancelled!=ISLE_GAME_OK) fail(cancelled,post->message());
            }
            if (!post->retire_scene()) return false;
            if (completed_source&&post->status()==post_course::SceneStatus::cancelled) {
                request_stop();return false;
            }
            if (completed_source) {
                const auto resumed=post->resume();
                if (resumed==ISLE_GAME_STOPPED) { request_stop();return false; }
                if (!checked(resumed,post->message())) return false;
                result=post->result();
                if (!result) return fail(ISLE_GAME_INVALID_STATE,"post-course scene retired without original entry result");
            }
            const auto retired=post->retire();
            if (retired==ISLE_GAME_STOPPED) request_stop();
            else if (retired!=ISLE_GAME_OK) fail(retired,post->message());
            if (post->status()!=post_course::SceneStatus::retired) return false;
        }
        if (beach) {
            if (!completed_source&&!checked(beach->cancel(),beach->message())) return false;
            if (!beach->retire_scene()) return false;
            if (completed_source) {
                if (!checked(beach->resume(),beach->message())) return false;
                result=beach->result();
                if (!result) return fail(ISLE_GAME_INVALID_STATE,"Beach retired without original post-scene result");
            }
            if (!checked(beach->retire(),beach->message())) return false;
        }
        if (rainbow) {
            if (!completed_source) observe_rainbow_join(rainbow->cancel());
            if (!rainbow->retire_scene()) { observe_rainbow_join(ISLE_GAME_OK);return false; }
            if (completed_source) {
                if (!checked_rainbow(ISLE_GAME_OK)) return false;
                // Entry continuation and result adoption each run once, even
                // when independent Source/domain retirement needs more pumps.
                if (!result) {
                    if (!checked_rainbow(rainbow->resume())) return false;
                    result=rainbow->result();
                    if (!result) return fail(ISLE_GAME_INVALID_STATE,"Rainbow retired without original post-scene result");
                }
            }
            observe_rainbow_join(rainbow->retire());
            if (rainbow->status()!=rainbow::LifecycleStatus::retired) return false;
        }
        if (startup) {
            if (!startup->retire()) return fail(ISLE_GAME_EXECUTION_FAILURE,startup->message());
            startup.reset();
        }
        if (window) {
            if (completed_source&&window->name) {
                std::int32_t source_result=0;
                if (!window->name->complete_scene()||!window->name->take_result(source_result))
                    return fail(ISLE_GAME_EXECUTION_FAILURE,window->name->error());
                result=source_result;
            } else if (completed_source&&window->oak) result=window->oak->view().source_result;
            window->retire();window.reset();
        }
        if (storage.game()&&!checked(storage.destroy(),"native window arena/domain retirement failed")) return false;
        if (completed_source&&stopping) return false;
        if (completed_source&&config.scene_retired) {
            if (!entry||!result) return fail(ISLE_GAME_INVALID_STATE,"retired scene lost its original entry/result");
            config.scene_retired(config.retirement_user,{&entry.intent(),*result,beach.get(),post.get(),rainbow.get(),
                course_pointer<tunnel::Lifecycle>(),course_pointer<volcano::Lifecycle>(),course_pointer<river::Lifecycle>(),
                course_pointer<cave::Lifecycle>(),course_pointer<valley::Lifecycle>()});
        }
        beach.reset();
        rainbow.reset();
        post.reset();
        courses=NativeCourses{};
        return true;
    }
    bool finish_transition() noexcept {
        if (!retire_scene(true)) return false;
        if (stopping) return false; // Cancellation never acknowledges an original result.
        if (!entry||!result) return fail(ISLE_GAME_INVALID_STATE,"scene retirement lost its actual entry/result");
        const auto serial=entry.intent().serial;
        entry={};
        if (!checked(manager->complete_entry(serial,*result),manager->message())) return false;
        result.reset();transition=false;
        return true;
    }
    bool stop() noexcept {
        clock.request_stop();
        if (updating) {
            if (!checked(clock.abandon_update(),"stopped application update still owned its GTL phase")) return false;
            updating=false;publication={};publication_ready=false;
        }
        if (executing||completed_hold) return false;
        if (!clock.stopped()) {
            (void)clock.poll();
            if (!clock.stopped()) return false;
        }
        if (!retire_scene(false)) return false;
        entry={};
        if (manager&&!checked(manager->cancel(),manager->message())) return false;
        if (manager&&!checked(manager->retire(),manager->message())) return false;
        if (graphics&&!graphics->retire()) return fail(ISLE_GAME_EXECUTION_FAILURE,graphics->message());
        graphics.reset();photos.reset();closed=true;
        return true;
    }
    bool pump(std::uint32_t raw_time) {
        if (closed) return failure==ISLE_GAME_OK;
        (void)poll_graphics();
        if (completed_hold) return failure==ISLE_GAME_OK;
        if (stopping) { (void)stop();return failure==ISLE_GAME_OK; }
        if (executing) return true;
        if (transition) {
            if (!clock.stopped()) (void)clock.poll();
            if (clock.stopped()) (void)finish_transition();
            return failure==ISLE_GAME_OK;
        }
        if (!active()) {
            if (clock.stopped()&&!checked(clock.reset({1,1,2,true,0}),"next original scene GTL epoch rejected")) return false;
            if (!checked(manager->resume(),manager->message())) return false;
            return enter(raw_time);
        }
        if (!updating) {
            const auto action=clock.poll();
            if (action.kind==host::ActionKind::draw) return draw(action.token);
            if (action.kind!=host::ActionKind::update) return true;
            updating=true;
        }
        if (!publication) {
            // A blocked real read is ordinary SI backpressure. Retain the GTL
            // update claim, but do not invent/lose a publication request.
            if (input.pending_read()) return true;
            const auto published=input.update();
            if (published.error!=game_input::Error::none||!published.accepted)
                return fail(ISLE_GAME_INVALID_STATE,"actual controller publication request failed");
            publication=published.accepted;
            publication_ready=published.acknowledged==publication;
        }
        if (!publication_ready) return true;
        if (!update(raw_time)) return false;
        if (stopping) { // C has exited; abandon, never finish, its claim.
            (void)stop();return failure==ISLE_GAME_OK;
        }
        if (update_abandoned) {
            if (!checked(clock.abandon_update(),"scene-exit traversal still owned its GTL phase")) return false;
        } else if (!checked(clock.finish_update(),"actual OM update completion rejected")) return false;
        updating=false;publication={};publication_ready=false;
        return true;
    }
};
Runtime::Runtime(std::unique_ptr<Impl> impl) noexcept:impl_(std::move(impl)) {}
Runtime::~Runtime() { if (!impl_->closed) std::terminate(); }
RuntimeCreateResult Runtime::create(const rom::Snapshot& snapshot,const menu::Catalog& catalog,
    game::State& state,game_input::Service& input,game_audio::Session& audio,
    host::GtlClock& clock,RuntimeConfig options) noexcept {
    RuntimeCreateResult made;
    try {
        if (!input.running()) { made.error=ISLE_GAME_INVALID_STATE;text(made.message,"application requires actual initialized input");return made; }
        auto impl=std::make_unique<Impl>(snapshot,catalog,state,input,audio,clock,options);
        if (std::adjacent_find(impl->callbacks.begin(),impl->callbacks.end(),[](const auto& a,const auto& b){return a.identity==b.identity;})!=impl->callbacks.end()) {
            made.error=ISLE_GAME_INVALID_STATE;text(made.message,"course callback Source identity has multiple owners");return made;
        }
        auto manager=Manager::create(snapshot,state,input,audio,clock,catalog);
        if (!manager) { made.error=manager.error;made.message=manager.message;return made; }
        impl->manager=std::move(manager.manager);
        auto graphics=scene::graphics::Frame::create(catalog,snapshot,{options.require_cpu_renderer,{16000000,4*1024*1024}});
        if (!graphics) { made.error=ISLE_GAME_EXECUTION_FAILURE;made.message=graphics.message;return made; }
        impl->graphics=std::move(graphics.frame);
        clock.request_stop();
        if (clock.poll().kind!=host::ActionKind::stopped) {
            made.error=ISLE_GAME_INVALID_STATE;text(made.message,"application creation requires a cold unborrowed GTL clock");return made;
        }
        made.runtime=std::unique_ptr<Runtime>(new Runtime(std::move(impl)));
    } catch (const std::bad_alloc&) {
        made.error=ISLE_GAME_EXHAUSTED;text(made.message,"native application allocation failed");
    } catch (const std::exception& error) {
        made.error=ISLE_GAME_EXECUTION_FAILURE;text(made.message,error.what());
    } catch (...) {
        made.error=ISLE_GAME_EXECUTION_FAILURE;text(made.message,"native application resource admission failed");
    }
    return made;
}
bool Runtime::pump(std::uint32_t raw_time) noexcept {
    try { return impl_->pump(raw_time); }
    catch (const std::bad_alloc&) { return impl_->fail(ISLE_GAME_EXHAUSTED,"native scene allocation failed"); }
    catch (const std::exception& error) { return impl_->fail(ISLE_GAME_EXECUTION_FAILURE,error.what()); }
    catch (...) { return impl_->fail(ISLE_GAME_EXECUTION_FAILURE,"native application scene transition failed"); }
}
IsleGameResult Runtime::acknowledge_input(const game_input::EventResult& event) noexcept {
    if (event.error!=game_input::Error::none) {
        impl_->fail(ISLE_GAME_INVALID_STATE,"actual controller read completion failed");return impl_->failure;
    }
    if (impl_->publication&&event.acknowledged==impl_->publication) impl_->publication_ready=true;
    return ISLE_GAME_OK;
}
IsleGameResult Runtime::bind_post_course_progress(host::Progress& progress,
                                                 post_course::ReplayNativeProgress native) noexcept {
    auto& owner=*impl_;
    if (owner.failure!=ISLE_GAME_OK) return owner.failure;
    if (!native.service||owner.progress||owner.active()||owner.entry||owner.executing||
        owner.updating||owner.stopping||owner.closed) {
        owner.fail(ISLE_GAME_INVALID_STATE,"post-course Host progress must bind once before first application pump");
        return owner.failure;
    }
    owner.progress=&progress;owner.native_service=native;
    return ISLE_GAME_OK;
}
scene::graphics::Completed Runtime::completed_frame() const noexcept { return impl_->completed(); }
IsleGameResult Runtime::release_completed() noexcept { return impl_->release()?ISLE_GAME_OK:impl_->failure; }
void Runtime::request_stop() noexcept { impl_->request_stop(); }
bool Runtime::stopped() const noexcept { return impl_->closed; }
bool Runtime::scene_active() const noexcept { return impl_->active(); }
IsleGameResult Runtime::error() const noexcept { return impl_->failure; }
const char* Runtime::message() const noexcept { return impl_->diagnostic.data(); }
RuntimeView Runtime::view() const noexcept {
    return {impl_->entry?&impl_->entry.intent():nullptr,impl_->startup.get(),
        impl_->window&&impl_->window->name?&*impl_->window->name:nullptr,
        impl_->window&&impl_->window->oak?&*impl_->window->oak:nullptr,impl_->beach.get(),impl_->post.get(),impl_->rainbow.get(),
        impl_->course_pointer<tunnel::Lifecycle>(),impl_->course_pointer<volcano::Lifecycle>(),
        impl_->course_pointer<river::Lifecycle>(),impl_->course_pointer<cave::Lifecycle>(),impl_->course_pointer<valley::Lifecycle>()};
}
}

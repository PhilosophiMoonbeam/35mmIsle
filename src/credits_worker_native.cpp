#include "credits_worker_native.hpp"
#include "credits_window_extension.h"
#include <exception>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace {
IsleGameResult converted(IsleContinuationResult r) noexcept {
    switch(r) {
    case ISLE_CONT_OK:return ISLE_GAME_OK;
    case ISLE_CONT_INVALID_ARGUMENT:return ISLE_GAME_INVALID_ARGUMENT;
    case ISLE_CONT_WRONG_OWNER:return ISLE_GAME_WRONG_OWNER;
    case ISLE_CONT_RESOURCE_FAILURE:return ISLE_GAME_EXHAUSTED;
    case ISLE_CONT_EXECUTION_FAILURE:return ISLE_GAME_EXECUTION_FAILURE;
    case ISLE_CONT_INVALID_STATE:return ISLE_GAME_INVALID_STATE;
    }
    return ISLE_GAME_EXECUTION_FAILURE;
}
template<bool Dispatch=false,class F> IsleGameResult on_owner(IsleCreditsWorker* worker,F&& f) noexcept {
    if(!worker)return ISLE_GAME_INVALID_ARGUMENT;
    if(worker->check_access()==ISLE_GAME_OK)return f();
    using Function=std::remove_reference_t<F>;
    struct Request {IsleCreditsWorker* worker;Function* function;IsleGameResult result;};
    Request request{worker,&f,ISLE_GAME_WRONG_OWNER};
    const auto action=+[](void* p) {
        auto& r=*static_cast<Request*>(p);
        if(r.worker->check_access()==ISLE_GAME_OK)r.result=(*r.function)();
    };
    const auto routed=Dispatch?isle_continuation_on_owner_dispatch(action,&request):
        isle_continuation_on_owner(action,&request);
    return routed==ISLE_CONT_OK?request.result:converted(routed);
}
}
IsleCreditsWorker::IsleCreditsWorker(IsleCreditsSource& s):source(s) {
    if(!s.native || !s.native->request_cancel || !s.native->cancel_drain || s.worker)
        throw std::invalid_argument("Credits worker requires its stable typed Source owner");
    auto result=isle_continuation_domain_create(&domain);
    if(result!=ISLE_CONT_OK)throw std::runtime_error("Credits worker domain admission");
    result=isle_continuation_create(domain,isle_credits_source_worker,&source,&continuation);
    if(result!=ISLE_CONT_OK) {
        (void)isle_continuation_domain_destroy(domain);domain=nullptr;
        throw std::runtime_error("Credits worker C continuation admission");
    }
    source.worker=this;
}
IsleCreditsWorker::~IsleCreditsWorker() {
    if(creator!=std::this_thread::get_id() || !retired)std::terminate();
    if(continuation && isle_continuation_destroy(continuation)!=ISLE_CONT_OK)std::terminate();
    if(domain && isle_continuation_domain_destroy(domain)!=ISLE_CONT_OK)std::terminate();
    if(source.worker==this)source.worker=nullptr;
}
IsleGameResult IsleCreditsWorker::check_access() const noexcept {
    return creator==std::this_thread::get_id()?ISLE_GAME_OK:ISLE_GAME_WRONG_OWNER;
}
IsleGameResult IsleCreditsWorker::fail(IsleGameResult r) noexcept {
    if(r!=ISLE_GAME_OK && (failure==ISLE_GAME_OK ||
       (failure==ISLE_GAME_STOPPED && r!=ISLE_GAME_STOPPED)))failure=r;
    return r;
}
IsleGameResult IsleCreditsWorker::queue(size_t capacity) noexcept {
    if(configured || created || started || capacity!=fifo.size())return fail(ISLE_GAME_INVALID_STATE);
    configured=true;return ISLE_GAME_OK;
}
IsleGameResult IsleCreditsWorker::create_thread(int32_t id,IsleContinuationEntry entry,void* argument,int32_t priority_value) noexcept {
    if(!configured || created || started || id!=0x64 || entry!=func_credits_801DCA1C ||
       argument || priority_value!=0x31)return fail(ISLE_GAME_INVALID_ARGUMENT);
    created=true;current_priority=priority_value;return ISLE_GAME_OK;
}
IsleGameResult IsleCreditsWorker::start() noexcept {
    if(!created || started || retired || cancelling)return fail(ISLE_GAME_INVALID_STATE);
    started=true;return ISLE_GAME_OK;
}
IsleGameResult IsleCreditsWorker::image(IsleGameSprite* sprite,IsleCreditsDestination* out,int* width,int* height) noexcept {
    if(out)*out={};if(width)*width=0;if(height)*height=0;
    if(!out || !width || !height || !sprite || !source.common.window)return ISLE_GAME_INVALID_ARGUMENT;
    for(size_t i=0;i<source.destination_count;++i) {
        const auto& d=source.destinations[i];
        if(d.sprite!=sprite)continue;
        IsleGameSprite* actual=nullptr;
        auto r=isle_post_window_credits_image_resolve(source.common.window,d.image,&actual,width,height);
        if(r!=ISLE_GAME_OK)return fail(r);
        if(actual!=sprite || d.incarnation!=d.image.serial)return fail(ISLE_GAME_INVALID_STATE);
        *out=d;return ISLE_GAME_OK;
    }
    return fail(ISLE_GAME_INVALID_ARGUMENT);
}
IsleGameResult IsleCreditsWorker::send(IsleGameSprite* sprite,int* queued) noexcept {
    if(queued)*queued=0;
    if(!queued || !started)return ISLE_GAME_INVALID_ARGUMENT;
    if(cancelling)return ISLE_GAME_STOPPED;
    if(failure!=ISLE_GAME_OK)return failure;
    if(count==fifo.size())return ISLE_GAME_OK;
    IsleCreditsDestination d{};int width=0,height=0;
    auto r=image(sprite,&d,&width,&height);
    if(r!=ISLE_GAME_OK)return r;
    r=isle_post_window_credits_image_retain(source.common.window,d.image);
    if(r!=ISLE_GAME_OK)return fail(r);
    fifo[(head+count)%fifo.size()]=d;++count;*queued=1;
    return ISLE_GAME_OK;
}
IsleGameResult IsleCreditsWorker::receive(IsleGameSprite** out,int* received) noexcept {
    if(out)*out=nullptr;if(received)*received=0;
    if(!out || !received || !started)return ISLE_GAME_INVALID_ARGUMENT;
    if(cancelling)return ISLE_GAME_STOPPED;
    if(failure!=ISLE_GAME_OK)return failure;
    if(inflight.image.serial || excluded)return fail(ISLE_GAME_INVALID_STATE);
    if(!count){blocked_receive=true;return ISLE_GAME_OK;}
    inflight=fifo[head];fifo[head]={};head=(head+1)%fifo.size();--count;
    IsleGameSprite* actual=nullptr;int width=0,height=0;
    const auto r=isle_post_window_credits_image_resolve(source.common.window,inflight.image,&actual,&width,&height);
    if(r!=ISLE_GAME_OK || actual!=inflight.sprite)return fail(r==ISLE_GAME_OK?ISLE_GAME_INVALID_STATE:r);
    blocked_receive=false;*out=actual;*received=1;return ISLE_GAME_OK;
}
IsleGameResult IsleCreditsWorker::priority(int32_t value) noexcept {
    if(cancelling)return ISLE_GAME_STOPPED;
    if(!inflight.image.serial || (value!=0x31 && value!=0x34))return fail(ISLE_GAME_INVALID_STATE);
    current_priority=value;return ISLE_GAME_OK;
}
IsleGameResult IsleCreditsWorker::release_inflight() noexcept {
    if(!inflight.image.serial)return ISLE_GAME_OK;
    auto r=isle_post_window_credits_image_release_hold(source.common.window,inflight.image);
    if(r==ISLE_GAME_OK)inflight={};
    return r;
}
IsleGameResult IsleCreditsWorker::exclude_main(int value) noexcept {
    if(cancelling)return ISLE_GAME_STOPPED;
    if(!inflight.image.serial || current_priority!=0x31 || (value!=0 && value!=1) ||
       excluded==(value!=0))return fail(ISLE_GAME_INVALID_STATE);
    if(value){excluded=true;return ISLE_GAME_OK;}
    // Original osStartThread(main) follows the completed destination update.
    // The queue incarnation/backing cannot be released before this boundary.
    auto r=release_inflight();if(r!=ISLE_GAME_OK)return fail(r);
    excluded=false;return ISLE_GAME_OK;
}
IsleGameResult IsleCreditsWorker::pump() noexcept {
    if(check_access()!=ISLE_GAME_OK)return ISLE_GAME_WRONG_OWNER;
    if(cancelling)return ISLE_GAME_STOPPED;
    if(failure!=ISLE_GAME_OK)return failure;
    if(!started || retired || servicing)return ISLE_GAME_OK;
    if(blocked_receive && !count)return ISLE_GAME_OK;
    servicing=true;source.worker_active=1;
    const auto r=converted(isle_continuation_resume(continuation));
    source.worker_active=0;servicing=false;
    if(source.common.status!=ISLE_GAME_OK)return fail(source.common.status);
    if(r!=ISLE_GAME_OK)return fail(r);
    IsleContinuationState state{};
    const auto inspected=converted(isle_continuation_state(continuation,&state));
    if(inspected!=ISLE_GAME_OK)return fail(inspected);
    if(state!=ISLE_CONT_WAITING)return fail(ISLE_GAME_INVALID_STATE);
    return failure;
}
IsleGameResult IsleCreditsWorker::request_cancel() noexcept {
    if(check_access()!=ISLE_GAME_OK)return ISLE_GAME_WRONG_OWNER;
    cancelling=true;blocked_receive=false;
    if(!cancel_requested) {
        const auto r=source.native->request_cancel(&source);
        if(r!=ISLE_GAME_OK && r!=ISLE_GAME_STOPPED)fail(r);
        else cancel_requested=true;
    }
    if(source.common.status==ISLE_GAME_OK)source.common.status=ISLE_GAME_STOPPED;
    return failure==ISLE_GAME_OK?ISLE_GAME_STOPPED:failure;
}
IsleGameResult IsleCreditsWorker::join(int* done) noexcept {
    if(done)*done=0;
    if(check_access()!=ISLE_GAME_OK)return ISLE_GAME_WRONG_OWNER;
    if(!done || servicing)return ISLE_GAME_INVALID_ARGUMENT;
    if(retired){*done=1;return failure;}
    cancelling=true;blocked_receive=false;
    if(!cancel_requested) {
        const auto r=source.native->request_cancel(&source);
        if(r!=ISLE_GAME_OK && r!=ISLE_GAME_STOPPED)fail(r);
        else cancel_requested=true;
    }
    // Construction starts the actual parked continuation before Original's
    // logical create/start calls. Even that READY stack must really join.
    if(!stack_joined) {
        const auto r=converted(isle_continuation_cancel(continuation));
        if(r!=ISLE_GAME_OK)return fail(r);
        stack_joined=true;source.worker_active=0;
    }
    // Cancel returns only after the separate worker stack is really joined.
    // Its abandoned C preview may still own real submitted GPU/readbacks.
    int drained=0;
    const auto drain=source.native->cancel_drain(&source,&drained);
    if(drain!=ISLE_GAME_OK && drain!=ISLE_GAME_STOPPED)fail(drain);
    if(!drained)return failure;
    auto r=release_inflight();if(r!=ISLE_GAME_OK)return fail(r);
    while(count) {
        auto& d=fifo[head];
        r=isle_post_window_credits_image_release_hold(source.common.window,d.image);
        if(r!=ISLE_GAME_OK)return fail(r);
        d={};head=(head+1)%fifo.size();--count;
    }
    excluded=false;retired=true;*done=1;
    return failure;
}

extern "C" {
IsleGameResult isle_credits_worker_queue(IsleCreditsWorker* w,size_t n){return on_owner(w,[=]{return w->queue(n);});}
IsleGameResult isle_credits_worker_create_thread(IsleCreditsWorker* w,int32_t id,IsleContinuationEntry e,void* a,int32_t p){return on_owner(w,[=]{return w->create_thread(id,e,a,p);});}
IsleGameResult isle_credits_worker_start(IsleCreditsWorker* w){return on_owner(w,[=]{return w->start();});}
IsleGameResult isle_credits_worker_send(IsleCreditsWorker* w,IsleGameSprite* s,int* q){return on_owner(w,[=]{return w->send(s,q);});}
IsleGameResult isle_credits_worker_receive(IsleCreditsWorker* w,IsleGameSprite** s,int* q){return on_owner(w,[=]{return w->receive(s,q);});}
IsleGameResult isle_credits_worker_priority(IsleCreditsWorker* w,int32_t p){return on_owner(w,[=]{return w->priority(p);});}
IsleGameResult isle_credits_worker_exclude_main(IsleCreditsWorker* w,int e){return on_owner(w,[=]{return w->exclude_main(e);});}
IsleGameResult isle_credits_worker_service(IsleCreditsWorker* w){return on_owner<true>(w,[=]{return w->pump();});}
IsleGameResult isle_credits_worker_cancel(IsleCreditsWorker* w){return on_owner(w,[=]{return w->request_cancel();});}
IsleGameResult isle_credits_worker_join(IsleCreditsWorker* w,int* done){return on_owner(w,[=]{return w->join(done);});}
IsleGameResult isle_credits_worker_image(IsleCreditsWorker* w,IsleGameSprite* s,IsleCreditsDestination* d,int* x,int* y){return on_owner(w,[=]{return w->image(s,d,x,y);});}
/* Returning pointer lookup only. The C fence, not this native frame, waits. */
IsleContinuationDomain* isle_credits_worker_domain(IsleCreditsWorker* w){return w?w->domain_handle():nullptr;}
}

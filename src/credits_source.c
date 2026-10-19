#include "credits_source.h"
#include "credits_window_extension.h"
#include "engine_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef uint64_t OSTime;
typedef float f32;
typedef IsleGameObject GObj;
typedef IsleGameSprite SObj;
typedef IslePlayerPhotoData PhotoData;
typedef IsleCreditsElement UIElement;
typedef IsleCreditsCredit Credit;
typedef IsleCreditsDisplayListCursor Gfx;
#define true 1
#define false 0
#define ARRAY_COUNT(a) (sizeof(a)/sizeof((a)[0]))
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define SCENE_MAIN_MENU 8
#define SCENE_CREDITS_17 17
#define SCENE_CREDITS_18 18
#define SCENE_CREDITS_19 19
#define SCENE_CREDITS_20 20
#define A_BUTTON 0x8000
#define START_BUTTON 0x1000
#define UI_FLAG_32BIT 0x400
#define UI_BACKGROUND 0
#define FONT_12 12
#define SP_TEXSHUF ISLE_SP_TEXSHUF
#define SP_SCALE ISLE_SP_SCALE
#define SP_HIDDEN ISLE_SP_HIDDEN
#define SP_TRANSPARENT ISLE_SP_TRANSPARENT
#define SET_SPRITE_POS(s,x_,y_) ((s).x=(x_),(s).y=(y_))
#define cr_checked(c,r) cr_check_at((c),(r),__func__,__LINE__)

extern IsleContinuationDomain* isle_credits_worker_domain(IsleCreditsWorker*);
static IsleGameResult cr_cont(IsleContinuationResult r) {
    switch(r) {
    case ISLE_CONT_OK:return ISLE_GAME_OK;
    case ISLE_CONT_INVALID_ARGUMENT:return ISLE_GAME_INVALID_ARGUMENT;
    case ISLE_CONT_WRONG_OWNER:return ISLE_GAME_WRONG_OWNER;
    case ISLE_CONT_RESOURCE_FAILURE:return ISLE_GAME_EXHAUSTED;
    case ISLE_CONT_EXECUTION_FAILURE:return ISLE_GAME_EXECUTION_FAILURE;
    default:return ISLE_GAME_INVALID_STATE;
    }
}
void isle_credits_worker_checked(IsleCreditsSource* c,IsleGameResult r,const char* where,int32_t line) {
    IsleContinuationDomain* domain;
    if(r==ISLE_GAME_OK)return;
    if(c->common.status==ISLE_GAME_OK || (c->common.status==ISLE_GAME_STOPPED && r!=ISLE_GAME_STOPPED))c->common.status=r;
    domain=isle_credits_worker_domain(c->worker);
    if(r==ISLE_GAME_STOPPED) {
        (void)isle_continuation_finish(domain);
    } else {
        if(!c->common.failure[0])snprintf(c->common.failure,sizeof(c->common.failure),"Credits worker %.96s:%d -> %d",where,(int)line,(int)r);
        (void)isle_continuation_fail(domain,c->common.failure);
    }
    abort();
}
static void cr_check_at(IsleCreditsSource* c,IsleGameResult r,const char* where,int32_t line) {
    if(c->worker_active)isle_credits_worker_checked(c,r,where,line);
    else isle_post_course_source_checked(&c->common,r,where,"Credits returning leaf",line);
}
IsleGameResult isle_credits_worker_suspend(IsleCreditsWorker* w) {
    return cr_cont(isle_continuation_wait(isle_credits_worker_domain(w),1));
}
static IsleSprite* cr_sprite(IsleCreditsSource* c,SObj* s) {
    IsleSprite* out=NULL;cr_checked(c,isle_post_window_sprite_view(c->common.window,s,&out));return out;
}
static SObj* cr_root(IsleCreditsSource* c,GObj* g) {
    SObj* out=NULL;cr_checked(c,isle_post_window_sprite_root(c->common.window,g,&out));return out;
}
static SObj* cr_next(IsleCreditsSource* c,SObj* s) {
    GObj* owner=NULL;SObj *parent=NULL,*child=NULL,*next=NULL,*previous=NULL;
    cr_checked(c,isle_post_window_sprite_links(c->common.window,s,&owner,&parent,&child,&next,&previous));return next;
}
static void cr_aux_set(IsleCreditsSource* c,SObj* s,int value) {cr_checked(c,isle_post_window_sprite_aux_set(c->common.window,s,value));}
static void cr_wait(IsleCreditsSource* c,int n) {isle_post_course_source_wait(&c->common,n);}
static void cr_screen(IsleCreditsSource* c,uint32_t a) {cr_checked(c,c->native->screen(c,a));}
static void cr_heap(IsleCreditsSource* c,size_t n) {cr_checked(c,c->native->heap(c,n));}
static void cr_free(IsleCreditsSource* c,void* p) {cr_checked(c,c->native->heap_free(c,p));}
static void cr_input_init(IsleCreditsSource* c) {
    cr_checked(c,c->native->input_initialize(c));
    c->common.input_invalidate=isle_post_course_native_input_invalidate;
    cr_checked(c,isle_post_course_source_install_input(&c->common));
}
static void cr_window_init(IsleCreditsSource* c) {cr_checked(c,c->native->window_initialize(c));}
static void cr_cameras(IsleCreditsSource* c) {cr_checked(c,c->native->cameras(c));}
static void cr_mask(IsleCreditsSource* c,int port,uint32_t mask) {cr_checked(c,c->native->input_mask(c,port,mask));}
static void cr_process(IsleCreditsSource* c,GObj* g,IsleGameSourceCallback fn,int kind,int priority) {
    IsleGameProcess* out=NULL;
    if(kind!=0)cr_checked(c,ISLE_GAME_INVALID_ARGUMENT);
    cr_checked(c,isle_game_add_source_process(c->common.game,g,fn,ISLE_GAME_STACKFUL,priority,&out));
}
static void cr_initial_process(IsleCreditsSource* c,IsleGameSourceCallback fn) {
    GObj* out=NULL;cr_checked(c,c->native->process_owner(c,0xE,0,0x80000000u,&out));cr_process(c,out,fn,0,1);
}
static GObj* cr_sprite_object(IsleCreditsSource* c,uint32_t prototype) {
    GObj* out=NULL;SObj* root=NULL;cr_checked(c,c->native->sprite_object(c,prototype,&out,&root));
    cr_checked(c,isle_post_window_adopt_object(c->common.window,out));return out;
}
static void cr_append(IsleCreditsSource* c,GObj* g,uint32_t prototype) {
    SObj* out=NULL;cr_checked(c,c->native->sprite_append(c,g,prototype,&out));
}
static void cr_writable(IsleCreditsSource* c,SObj* s,size_t pieces,size_t bytes) {
    IsleCreditsDestination* d;
    if(c->destination_count>=ARRAY_COUNT(c->destinations))cr_checked(c,ISLE_GAME_EXHAUSTED);
    d=&c->destinations[c->destination_count];
    cr_checked(c,isle_post_window_credits_image_create(c->common.window,s,0x801e6058u,pieces,bytes,&d->image));
    d->sprite=s;d->incarnation=d->image.serial;++c->destination_count;
}
static void cr_service(IsleCreditsSource* c) {cr_checked(c,isle_credits_worker_service(c->worker));}
static void cr_remove(IsleCreditsSource* c,GObj* g) {
    /* Quiesce the actual queued C destination updates before original SObj
     * removal. The service creates no OM/GTL update and never runs menu logic. */
    cr_service(c);
    cr_checked(c,isle_post_window_credits_image_detach_object(c->common.window,g));
    cr_checked(c,c->native->remove_sprites(c,g));
}
static void cr_end_all(IsleCreditsSource* c,GObj* g) {
    IsleGameObjectMetadata metadata;
    cr_checked(c,isle_game_object_metadata(c->common.game,g,&metadata));
    while(g->processListHead)cr_checked(c,isle_game_end_process(c->common.game,g->processListHead));
}
static void cr_end(IsleCreditsSource* c,IsleGameProcess* p) {cr_checked(c,isle_game_end_process(c->common.game,p));}
static uint32_t cr_pressed(IsleCreditsSource* c) {uint32_t b=0;cr_checked(c,c->native->pressed(c,&b));return b;}
static int32_t cr_sound(IsleCreditsSource* c,int id,int volume,int pan,float pitch,int reverb) {
    int32_t out=0;cr_checked(c,c->native->sound(c,id,volume,pan,pitch,reverb,&out));return out;
}
static void cr_volume(IsleCreditsSource* c,int player,int value) {cr_checked(c,c->native->volume(c,player,value,0));}
static void cr_volume_smooth(IsleCreditsSource* c,int player,int value,int frames) {cr_checked(c,c->native->volume(c,player,value,frames));}
static void cr_song(IsleCreditsSource* c,int player,int song) {cr_checked(c,c->native->song(c,player,song));}
static void cr_fade_color(IsleCreditsSource* c,int r,int g,int b) {cr_checked(c,c->native->fade_color(c,r,g,b));}
static void cr_fade_duration(IsleCreditsSource* c,int direction,float duration) {cr_checked(c,c->native->fade_duration(c,direction,duration));}
static void cr_exit(IsleCreditsSource* c) {cr_checked(c,c->native->exit_scene(c));}
static void cr_display(IsleCreditsSource* c,Gfx** cursor,uint32_t address) {cr_checked(c,c->native->display_list(c,cursor,address));}
static uint64_t cr_time(IsleCreditsSource* c) {uint64_t out=0;cr_checked(c,c->native->time(c,&out));return out;}
static void cr_prepare_course(IsleCreditsSource* c,const PhotoData* p) {
    int done=0;
    do {
        cr_checked(c,c->native->prepare_course(c,p,&done));
        if(!done)cr_checked(c,isle_post_course_native_progress(&c->common));
    } while(!done);
}
static void cr_near(IsleCreditsSource* c,int n) {if(n!=1)cr_checked(c,ISLE_GAME_INVALID_ARGUMENT);c->near_disabled=n;}
static void cr_intervals(IsleCreditsSource* c,int u,int d) {if(u!=1 || d!=2)cr_checked(c,ISLE_GAME_INVALID_ARGUMENT);c->update_interval=u;c->draw_interval=d;}
static void cr_setup(IsleCreditsSource* c,uint32_t setup) {
    int complete=0;
    cr_checked(c,c->native->setup_begin(c,c->argument,0x801deba0u,setup,c->D_credits_801DEC40,c->near_disabled,c->update_interval,c->draw_interval));
    for(;;) {
        cr_checked(c,c->native->setup_poll(c,&complete));if(complete)break;
        cr_checked(c,cr_cont(isle_continuation_wait(c->common.domain,1)));
    }
}
static void cr_queue(IsleCreditsSource* c,size_t n) {cr_checked(c,isle_credits_worker_queue(c->worker,n));}
static void cr_thread_create(IsleCreditsSource* c,int id,IsleContinuationEntry e,void* arg,int priority) {
    cr_checked(c,isle_credits_worker_create_thread(c->worker,id,e,arg,priority));
}
static void cr_thread_start(IsleCreditsSource* c) {cr_checked(c,isle_credits_worker_start(c->worker));}
static void cr_thread_stop(IsleCreditsSource* c) {
    int joined=0;IsleGameResult r;
    do {r=isle_credits_worker_join(c->worker,&joined);if(!joined && r!=ISLE_GAME_OK)cr_checked(c,r);}while(!joined);
    cr_checked(c,r);
}
static void cr_thread_destroy(IsleCreditsSource* c) {
    int joined=0;cr_checked(c,isle_credits_worker_join(c->worker,&joined));if(!joined)cr_checked(c,ISLE_GAME_INVALID_STATE);
}
static void cr_receive(IsleCreditsSource* c,void** out) {
    SObj* sprite=NULL;int ready=0;
    for(;;) {
        cr_checked(c,isle_credits_worker_receive(c->worker,&sprite,&ready));if(ready)break;
        cr_checked(c,isle_credits_worker_suspend(c->worker));
    }
    *out=sprite;
}
static void cr_send(IsleCreditsSource* c,SObj* sprite) {
    int ready=0;
    for(;;) {
        cr_checked(c,isle_credits_worker_send(c->worker,sprite,&ready));
        /* A successful send wakes Original's higher-priority worker. Its
         * separate C stack runs until genuine blocking receive, never an OM
         * update; native GPU polling stays in its returning progress leaves. */
        cr_service(c);if(ready)break;
    }
}
static void cr_priority(IsleCreditsSource* c,int priority) {cr_checked(c,isle_credits_worker_priority(c->worker,priority));}
static void cr_main(IsleCreditsSource* c,int exclude) {cr_checked(c,isle_credits_worker_exclude_main(c->worker,exclude));}

typedef struct CreditsReportRead {IsleCreditsSource* source;int32_t id;IslePhotoRecord record;IsleGameResult result;} CreditsReportRead;
static void cr_report_on_owner(void* arg) {
    CreditsReportRead* r=arg;r->result=isle_application_photos_report(r->source->common.photos,r->id,&r->record);
}
static const PhotoData* cr_report(IsleCreditsSource* c,int32_t id) {
    CreditsReportRead r={c,id,{0},ISLE_GAME_INVALID_STATE};
    cr_checked(c,cr_cont(isle_continuation_on_owner(cr_report_on_owner,&r)));cr_checked(c,r.result);return r.record.photo;
}
typedef struct CreditsPreview {IsleCreditsSource* source;IslePostCourseThumbnail image;const IslePostCoursePreview* view;IsleGameResult result;} CreditsPreview;
static void cr_consume_on_owner(void* raw) {
    CreditsPreview* p=raw;
    /* Original DE8A0 never calls the reducer on NULL: leave pixels unchanged. */
    p->result=p->view?isle_post_window_thumbnail_write_photo(p->source->common.window,p->image,p->view->rgba5551,p->view->size,p->view->stride_bytes):ISLE_GAME_INVALID_STATE;
}
static IsleGameResult cr_consume(void* raw,const IslePostCoursePreview* view) {
    CreditsPreview* p=raw;IsleContinuationResult r;p->view=view;
    r=isle_continuation_on_owner(cr_consume_on_owner,p);return r==ISLE_CONT_OK?p->result:cr_cont(r);
}
static void cr_preview(IsleCreditsSource* c,const PhotoData* photo,SObj* s) {
    IsleCreditsDestination d={0};int width=0,height=0;float scale=0;CreditsPreview p;
    cr_checked(c,isle_credits_worker_image(c->worker,s,&d,&width,&height));
    cr_checked(c,c->native->preview_scale(c,&scale));
    p=(CreditsPreview){c,d.image,NULL,ISLE_GAME_INVALID_STATE};
    cr_checked(c,isle_post_replay_preview(c->common.replay,photo,width*2,height*2,scale,cr_consume,&p));
}
static UIElement* cr_element_create(IsleCreditsSource* c,int x,int y,int w,int h,uint32_t flags) {
    UIElement* out;if(c->element_count>=14)cr_checked(c,ISLE_GAME_EXHAUSTED);
    out=&c->elements[c->element_count++];cr_checked(c,isle_post_window_element_create(c->common.window,x,y,w,h,flags,&out->handle));return out;
}
static void cr_element_pos(IsleCreditsSource* c,UIElement* e,int x,int y) {cr_checked(c,isle_post_window_element_position(c->common.window,e->handle,x,y));}
static void cr_element_font(IsleCreditsSource* c,UIElement* e,int f) {cr_checked(c,isle_post_window_element_font(c->common.window,e->handle,f));}
static void cr_element_draw(IsleCreditsSource* c,UIElement* e) {cr_checked(c,isle_post_window_element_clear(c->common.window,e->handle));}
static void cr_element_background(IsleCreditsSource* c,UIElement* e) {cr_checked(c,isle_post_window_element_background(c->common.window,e->handle));}
static void cr_element_textpos(IsleCreditsSource* c,UIElement* e,int x,int y) {cr_checked(c,isle_post_window_element_text_position(c->common.window,e->handle,x,y));}
static void cr_element_color(IsleCreditsSource* c,UIElement* e,int which,int r,int g,int b,int a) {cr_checked(c,isle_post_window_element_color(c->common.window,e->handle,which!=0,(IslePostCourseColor){r,g,b,a}));}
static void cr_delay(IsleCreditsSource* c,int d) {cr_checked(c,isle_post_window_text_delay(c->common.window,d));}
static int cr_width(IsleCreditsSource* c,const char* t) {int out=0;cr_checked(c,isle_post_window_text_width(c->common.window,(IslePostCourseBytes){t,strlen(t)},&out));return out;}
static void cr_print(IsleCreditsSource* c,UIElement* e,const char* t) {cr_checked(c,isle_post_window_print(c->common.window,e->handle,(IslePostCourseBytes){t,strlen(t)},0,NULL));}

/* Complete pinned Original bodies/data are emitted privately, never vendored. */
#include "credits_source.inc"

static IsleCreditsSource* cr_context(GObj* object) {
    IslePostCourseSceneSource* common;
    /* Require validates actual native object membership, not userData. The
     * callback's actual variant is not ResourceKind ordinal6. */
    IsleCreditsSource* c;
    IsleGame* game;
    if(!object)abort();
    game=object->game;
    common=game?game->postCourseSource:NULL;
    if(!common)abort();
    common=isle_post_course_source_require(object,common->scene);
    c=(IsleCreditsSource*)common;
    if(c->argument<17 || c->argument>20 || c->common.scene!=c->argument)cr_checked(c,ISLE_GAME_INVALID_STATE);
    return c;
}
#define CREDITS_CALLBACK(name) void name(GObj* object) {IsleCreditsSource* c=cr_context(object);name##_body(c,object);cr_checked(c,c->common.status);}
CREDITS_CALLBACK(func_credits_801DCEF0)
CREDITS_CALLBACK(func_credits_801DD168)
CREDITS_CALLBACK(func_credits_801DD49C)
CREDITS_CALLBACK(func_credits_801DD540)
CREDITS_CALLBACK(func_credits_801DDE24)
CREDITS_CALLBACK(func_credits_801DDF48)
CREDITS_CALLBACK(func_credits_801DE734)
CREDITS_CALLBACK(func_credits_801DE844)
#undef CREDITS_CALLBACK
void func_credits_801DC8A0(IsleCreditsDisplayListCursor** cursor) {
    IsleCreditsSource* c=NULL;
    IsleGameResult r=isle_credits_native_display_context(cursor,&c);
    if(r!=ISLE_GAME_OK || !c)abort();
    func_credits_801DC8A0_body(c,cursor);
}
void func_credits_801DC9FC(void* allocation) {
    IsleCreditsSource* c=NULL;
    IsleGameResult r=isle_credits_native_cleanup_context(allocation,&c);
    if(r!=ISLE_GAME_OK || !c)abort();
    func_credits_801DC9FC_body(c,allocation);
}
#if defined(_MSC_VER)
static __declspec(thread) IsleCreditsSource* cr_worker_context;
#else
static _Thread_local IsleCreditsSource* cr_worker_context;
#endif
void func_credits_801DCA1C(void* original_argument) {
    IsleCreditsSource* c=cr_worker_context;
    if(!c || !c->worker_active)abort();
    func_credits_801DCA1C_body(c,original_argument);
}
void isle_credits_source_worker(void* raw) {
    IsleCreditsSource* c=raw;
    cr_worker_context=c;
    func_credits_801DCA1C(NULL);
    isle_credits_worker_checked(c,ISLE_GAME_INVALID_STATE,"Original worker returned",0);
}
void isle_credits_source_entry(void* raw) {
    IsleCreditsSource* c=raw;
    c->source_result=func_credits_801DCB24(c,c->argument);
    c->entry_complete=1;
}
void isle_credits_source_init(IslePostCourseSceneSource* common,void* unused) {
    (void)unused;func_credits_801DC8C4((IsleCreditsSource*)common);
}
IsleGameResult isle_credits_source_initialize(IsleCreditsSource* c,int32_t argument) {
    IslePostCourseSceneSource common;const IsleCreditsNative* native;IsleCreditsWorker* worker;
    if(!c || argument<17 || argument>20 || !c->native)return ISLE_GAME_INVALID_ARGUMENT;
    if(c->argument || c->entry_complete || c->worker_active)return ISLE_GAME_INVALID_STATE;
    native=c->native;
#define REQUIRE_NATIVE(name) if(!native->name)return ISLE_GAME_INVALID_ARGUMENT
    REQUIRE_NATIVE(screen);REQUIRE_NATIVE(heap);REQUIRE_NATIVE(heap_free);
    REQUIRE_NATIVE(input_initialize);REQUIRE_NATIVE(window_initialize);REQUIRE_NATIVE(cameras);
    REQUIRE_NATIVE(input_mask);REQUIRE_NATIVE(process_owner);REQUIRE_NATIVE(sprite_object);
    REQUIRE_NATIVE(sprite_append);REQUIRE_NATIVE(remove_sprites);REQUIRE_NATIVE(setup_begin);
    REQUIRE_NATIVE(setup_poll);REQUIRE_NATIVE(pressed);REQUIRE_NATIVE(sound);REQUIRE_NATIVE(song);
    REQUIRE_NATIVE(volume);REQUIRE_NATIVE(fade_color);REQUIRE_NATIVE(fade_duration);
    REQUIRE_NATIVE(exit_scene);REQUIRE_NATIVE(display_list);REQUIRE_NATIVE(time);
    REQUIRE_NATIVE(prepare_course);REQUIRE_NATIVE(preview_scale);REQUIRE_NATIVE(request_cancel);REQUIRE_NATIVE(cancel_drain);
#undef REQUIRE_NATIVE
    common=c->common;worker=c->worker;memset(c,0,sizeof(*c));
    c->common=common;c->native=native;c->worker=worker;
    c->argument=argument;c->common.scene=argument;
    return c->common.status;
}
IsleGameResult isle_credits_source_release(IsleCreditsSource* c) {
    int joined=0;IsleGameResult r;
    if(!c || !c->worker)return ISLE_GAME_INVALID_ARGUMENT;
    r=isle_credits_worker_join(c->worker,&joined);
    if(!joined)return r==ISLE_GAME_OK?ISLE_GAME_INVALID_STATE:r;
    while(c->destination_count) {
        IsleCreditsDestination* d=&c->destinations[c->destination_count-1];
        IsleGameResult released=isle_post_window_thumbnail_release(c->common.window,d->image);
        if(released!=ISLE_GAME_OK)return released;
        *d=(IsleCreditsDestination){0};--c->destination_count;
    }
    return r;
}

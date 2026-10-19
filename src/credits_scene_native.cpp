#include "credits_scene_native.hpp"
#include "post_course_native_helpers.hpp"
#include "photo_presentation.hpp"
#include "menu_scene_bridge.h"
#include <bit>
#include <cmath>
#include <exception>
#include <utility>

namespace isle::post_course {
namespace {
constexpr std::uint32_t screen_address=0x801deba0u;
constexpr std::uint32_t setup_address=0x801dec30u;
constexpr std::uint32_t pre_render_address=0x801debd8u;
constexpr std::uint32_t arena_bytes=0x80369f80u-0x801ed020u;
constexpr std::array<std::uint32_t,7> screen_words{
    0x803b5000,0x803da800,0,0,320,240,0x16a99};
constexpr std::array<std::uint32_t,35> setup_words{
    0,0x8000affc,0x8000ada0,0x801ed020,0,1,1,0x8000,0x2000,0,0,
    0x1800,0,0,0x800a1a50,0x80004860,8,0x2800,0x20,0,8,0x10,
    0x5c,0x200,0,0x801dc9fc,0,0,0,0x88,0x60,0x58,2,0x90,0x801dc8c4};
constexpr std::array<std::uint32_t,22> pre_render_words{
    0xe7000000,0,0xe3000a01,0x00100000,0xe200001c,0xc8112078,
    0xdb080000,0x0100fe00,0xf8000000,0xff,0xd9ffffff,0x00010000,
    0xd9ffffff,0x00020000,0xdb020000,0x18,0xdc08060a,0x801debc8,
    0xdc08090a,0x801debc0,0xdf000000,0};
struct Prototype {std::uint32_t address;int width,height,pieces;};
constexpr std::array<Prototype,5> prototypes{{
    {0x801e6058,140,105,8},{0x801e6430,3,111,1},
    {0x801e67d8,140,3,1},{0x801e6aa8,16,10,20},{0x801e8b50,128,32,4}}};

IsleGameResult remember(CreditsSceneNative& state,IsleGameResult result) noexcept {
    if(result!=ISLE_GAME_OK && result!=ISLE_GAME_STOPPED && state.failure==ISLE_GAME_OK)
        state.failure=result;
    return result;
}
/* Worker callbacks rendezvous with the existing continuation owner. None of the
 * native service objects is accessed from a portable worker's executor thread.
 * The retiring entry differs only in allowing the actual sticky Game fault. */
template<bool Retiring=false,class F>
IsleGameResult credits_invoke(IsleCreditsSource* source,F&& function) noexcept {
    if(!source)return ISLE_GAME_INVALID_ARGUMENT;
    auto body=[&](auto& owner)->IsleGameResult {
        if(std::get_if<IsleCreditsSource>(&owner.context)!=source ||
           !owner.native || owner.kind!=Resources::Kind::credits)
            return ISLE_GAME_INVALID_STATE;
        auto* state=std::get_if<CreditsSceneNative>(&owner.native->menu);
        if(!state || state->source!=source || !state->prepared ||
           source->native!=&credits_scene_native_table() ||
           source->argument!=owner.argument || owner.argument<17 || owner.argument>20 ||
           source->common.scene!=owner.argument)
            return ISLE_GAME_INVALID_STATE;
        if constexpr(!Retiring) {
            if(state->failure!=ISLE_GAME_OK)return state->failure;
            if(state->cancel_requested)return ISLE_GAME_STOPPED;
        }
        return remember(*state,function(owner,*state));
    };
    if constexpr(Retiring)
        return SceneAccess::invoke_retiring(&source->common,body);
    else
        return SceneAccess::invoke(&source->common,body);
}
bool prototype_admitted(std::uint32_t address) noexcept {
    for(const auto& p:prototypes)if(p.address==address)return true;
    return false;
}
template<class O>
IsleGameResult check_resources(O& owner) {
    auto& n=*owner.native;
    if(!n.resources || n.resources->kind()!=Resources::Kind::credits ||
       !n.window || !n.heap || !n.services || !n.graphics ||
       owner.source().window!=n.window->source() || owner.source().game!=n.storage.game())
        return ISLE_GAME_INVALID_STATE;
    const auto& r=*n.resources;
    for(std::size_t i=0;i<screen_words.size();++i)
        if(r.word(screen_address+std::uint32_t(i*4))!=screen_words[i])return ISLE_GAME_INVALID_STATE;
    for(std::size_t i=0;i<setup_words.size();++i)
        if(r.word(setup_address+std::uint32_t(i*4))!=setup_words[i])return ISLE_GAME_INVALID_STATE;
    for(std::size_t i=0;i<pre_render_words.size();++i)
        if(r.word(pre_render_address+std::uint32_t(i*4))!=pre_render_words[i])return ISLE_GAME_INVALID_STATE;
    for(const auto& p:prototypes) {
        const auto& sprite=*r.roots()[r.sprite_index(p.address)];
        if(sprite.width!=p.width || sprite.height!=p.height || sprite.nbitmaps!=p.pieces ||
           sprite.x!=0 || sprite.y!=0 || sprite.scalex!=1 || sprite.scaley!=1 ||
           sprite.attr!=ISLE_SP_TEXSHUF || !sprite.bitmap || sprite.LUT || sprite.LUT_bytes || sprite.nTLUT)
            return ISLE_GAME_INVALID_STATE;
        for(int i=0;i<p.pieces;++i)if(!sprite.bitmap[i].buf || !sprite.bitmap[i].byte_count)
            return ISLE_GAME_INVALID_STATE;
    }
    const auto& photo=*r.roots()[r.sprite_index(0x801e6058)];
    if(photo.bmfmt!=0 || photo.bmsiz!=2 || photo.bmheight!=14 || photo.bmHreal!=14)
        return ISLE_GAME_INVALID_STATE;
    for(std::size_t i=0;i<8;++i) {
        const auto& b=photo.bitmap[i];
        if(b.width!=140 || b.width_img!=140 || b.s || b.t || b.LUToffset ||
           b.actualHeight!=(i==7?7:14) || b.byte_count!=(i==7?1960u:3920u))
            return ISLE_GAME_INVALID_STATE;
    }
    return ISLE_GAME_OK;
}
IsleGameResult screen(IsleCreditsSource* source,std::uint32_t address) noexcept {
    return credits_invoke(source,[&](auto& owner,auto& state) {
        auto& n=*owner.native;
        if(address!=screen_address || state.screen_applied ||
           owner.requested_screen!=screen_address || owner.screen_words!=screen_words)
            return ISLE_GAME_INVALID_STATE;
        scene::graphics::SourceScreen value;
        value.width=screen_words[4];value.height=screen_words[5];value.flags=screen_words[6];
        for(std::size_t i=0;i<3;++i)value.framebuffer_identity[i]=screen_words[i];
        if(!n.graphics->configure_source_screen(value,nullptr))return ISLE_GAME_INVALID_STATE;
        state.screen_applied=true;
        return ISLE_GAME_OK;
    });
}
IsleGameResult heap(IsleCreditsSource* source,std::size_t bytes) noexcept {
    return credits_invoke(source,[&](auto& owner,auto& state) {
        auto& n=*owner.native;
        const auto expected=owner.argument==19?std::size_t{0x64000}:std::size_t{0x78000};
        if(bytes!=expected)return ISLE_GAME_INVALID_ARGUMENT;
        // Host descriptor/alignment expansion does not alter UIMem's Source
        // request. Native heap capacity may be larger, never smaller.
        if(n.heap_admitted || !n.heap || n.heap->capacity()<expected)return ISLE_GAME_INVALID_STATE;
        n.heap_admitted=true;state.logical_heap_bytes=bytes;
        return ISLE_GAME_OK;
    });
}
IsleGameResult heap_free(IsleCreditsSource* source,void* allocation) noexcept {
    return credits_invoke<true>(source,[&](auto& owner,auto&) {
        return SceneAccess::deallocate_native(owner,allocation);
    });
}
IsleGameResult input_initialize(IsleCreditsSource* source) noexcept {
    return credits_invoke(source,[](auto& owner,auto&) {return initialize_input(owner);});
}
IsleGameResult window_initialize(IsleCreditsSource* source) noexcept {
    return credits_invoke(source,[](auto& owner,auto& state) {
        if(!owner.native->heap_admitted || !state.screen_applied || state.window_initialized)
            return ISLE_GAME_INVALID_STATE;
        // Shared actual80374D40 owns its two preview buffers, exact preview
        // camera and recorded-photo links19/18/17; never another worker Replay.
        const auto result=owner.initialize_replay();
        if(result!=ISLE_GAME_OK)return result;
        if(!owner.native->pipeline || !owner.native->replay ||
           !owner.native->preview_object || !owner.native->preview_camera ||
           owner.source().replay!=owner.native->replay->source())
            return ISLE_GAME_INVALID_STATE;
        state.preview_object=owner.native->preview_object;
        state.preview_camera=owner.native->preview_camera;
        state.window_initialized=true;
        return ISLE_GAME_OK;
    });
}
template<class O>
IsleGameResult bare_camera(O& owner,std::uint32_t id,IsleGameObject*& object,IsleGameCamera*& camera) {
    auto& n=*owner.native;
    object=n.services->object(id,0,0x80000000);
    if(!object)return ISLE_GAME_EXHAUSTED;
    n.source_objects.push_back(object);
    return isle_game_attach_camera(owner.source().game,object,&camera);
}
IsleGameResult cameras(IsleCreditsSource* source) noexcept {
    return credits_invoke(source,[](auto& owner,auto& state) {
        if(!state.window_initialized || state.cameras_initialized)return ISLE_GAME_INVALID_STATE;
        auto& n=*owner.native;
        // Actual ohCreateCameraWrapper: no default camera matrices.
        auto result=bare_camera(owner,0xffffffffu,state.clear_object,state.clear_camera);
        if(result!=ISLE_GAME_OK)return result;
        const IsleGameCameraRenderState clear_render{6,0xff,0};
        result=isle_game_camera_set_render_state(owner.source().game,state.clear_camera,&clear_render);
        if(result!=ISLE_GAME_OK)return result;
        n.add_camera(state.clear_object,state.clear_camera,0x64,0,0);
        n.passes[n.pass_count-1].kind=scene::render::Kind::clear;
        // Actual ohCreateCamera(... defaultMatrices=1, defaultFlags=1), then
        // original assignment flags=CAMERA_FLAG_8 (not inherited flags7).
        result=camera_owner(owner,3,0,0x80000000,state.sprite_object,state.sprite_camera);
        if(result!=ISLE_GAME_OK)return result;
        const IsleGameCameraRenderState render{8,0xff,0xffffffffu};
        result=isle_game_camera_set_render_state(owner.source().game,state.sprite_camera,&render);
        if(result!=ISLE_GAME_OK)return result;
        n.add_camera(state.sprite_object,state.sprite_camera,3,2,0xffffffffu);
        state.cameras_initialized=true;
        return ISLE_GAME_OK;
    });
}
/* The provider field is historically named input_mask. Its actual Original
 * target is func800A7F68, the transition camera, NOT controller input masking. */
IsleGameResult input_mask(IsleCreditsSource* source,int link,std::uint32_t mode) noexcept {
    return credits_invoke(source,[&](auto& owner,auto& state) {
        if(link!=0 || mode!=0x101 || !state.cameras_initialized || state.transition_initialized)
            return ISLE_GAME_INVALID_ARGUMENT;
        auto& n=*owner.native;
        auto* transition=n.services->screen_transition_source();
        if(!transition || transition->camera_object || !n.graphics)return ISLE_GAME_INVALID_STATE;
        auto result=isle_screen_transition_camera(transition,link,int(mode));
        if(result!=ISLE_GAME_OK)return result;
        auto* camera=transition->D_800BE2C8;
        result=viewport(owner,camera,320,240);if(result!=ISLE_GAME_OK)return result;
        const IsleGameCameraParameters* current=nullptr;
        result=isle_game_camera_parameters(owner.source().game,camera,&current);
        if(result!=ISLE_GAME_OK)return result;
        auto perspective=current->perspective;
        perspective.fovy=45;perspective.near_plane=8;perspective.far_plane=1024;perspective.scale=1;
        result=isle_game_camera_set_perspective(owner.source().game,camera,&perspective);
        if(result!=ISLE_GAME_OK)return result;
        const IsleGameLookAt pose{{290,0,0},{0,0,0},{0,0,1}};
        result=isle_game_camera_set_look_at(owner.source().game,camera,&pose);
        if(result!=ISLE_GAME_OK)return result;
        const IsleGameCameraRenderState render{4,0x10001,1};
        result=isle_game_camera_set_render_state(owner.source().game,camera,&render);
        if(result!=ISLE_GAME_OK)return result;
        transition->D_800BE2C0=0;transition->camera_flags=4;
        transition->camera_mask=1;transition->camera_buffer_mask=1;
        transition->D_800AF054=transition->D_800AF058=transition->D_800AF05C=transition->D_800AF050=0;
        n.source_objects.push_back(transition->camera_object);
        n.add_camera(transition->camera_object,camera,1,1,1);
        n.passes[n.pass_count-1]=n.graphics->transition_pass(n.services->screen_transition());
        state.transition_initialized=true;
        return ISLE_GAME_OK;
    });
}
IsleGameResult process_owner(IsleCreditsSource* source,std::int32_t id,
    std::uint32_t link,std::uint32_t priority,IsleGameObject** output) noexcept {
    if(output)*output=nullptr;
    if(!output)return ISLE_GAME_INVALID_ARGUMENT;
    return credits_invoke(source,[&](auto& owner,auto&) {
        if(id!=0xe || link!=0 || priority!=0x80000000)return ISLE_GAME_INVALID_ARGUMENT;
        auto& n=*owner.native;
        auto* object=n.services->object(0xe,0,0x80000000);
        if(!object)return ISLE_GAME_EXHAUSTED;
        n.source_objects.push_back(object);
        // omAddGObj's updater is actually NULL; C installs DE844 afterward.
        if(!n.services->set_update(object,nullptr,nullptr))return ISLE_GAME_INVALID_STATE;
        *output=object;return ISLE_GAME_OK;
    });
}
IsleGameResult sprite_object(IsleCreditsSource* source,std::uint32_t prototype,
    IsleGameObject** object,IsleGameSprite** sprite) noexcept {
    if(object)*object=nullptr;if(sprite)*sprite=nullptr;
    if(!object || !sprite)return ISLE_GAME_INVALID_ARGUMENT;
    return credits_invoke(source,[&](auto& owner,auto&) {
        if(!prototype_admitted(prototype))return ISLE_GAME_INVALID_ARGUMENT;
        auto& n=*owner.native;
        const auto created=n.services->sprite(prototype,1,0x80000000,0xe);
        if(!created.object || !created.sprite)return ISLE_GAME_EXHAUSTED;
        n.source_objects.push_back(created.object);
        *object=created.object;*sprite=created.sprite;return ISLE_GAME_OK;
    });
}
IsleGameResult sprite_append(IsleCreditsSource* source,IsleGameObject* object,
    std::uint32_t prototype,IsleGameSprite** output) noexcept {
    if(output)*output=nullptr;if(!output)return ISLE_GAME_INVALID_ARGUMENT;
    return credits_invoke(source,[&](auto& owner,auto&) {
        if(!prototype_admitted(prototype))return ISLE_GAME_INVALID_ARGUMENT;
        IsleGameSprite* root=nullptr;
        auto result=isle_post_window_sprite_root(owner.source().window,object,&root);
        if(result!=ISLE_GAME_OK)return result;
        if(!root)return ISLE_GAME_INVALID_STATE;
        auto* appended=owner.native->services->append_sprite(object,prototype);
        if(!appended)return ISLE_GAME_EXHAUSTED;
        *output=appended;return ISLE_GAME_OK;
    });
}
IsleGameResult remove_sprites(IsleCreditsSource* source,IsleGameObject* object) noexcept {
    return credits_invoke(source,[&](auto& owner,auto&) {
        IsleGameSprite* root=nullptr;
        const auto result=isle_post_window_sprite_root(owner.source().window,object,&root);
        if(result!=ISLE_GAME_OK)return result;
        // Source has already serviced its real worker and detached its aliases.
        // Remove SObjs only; the real waiting process/GObj remains alive.
        return owner.native->services->remove_sprites(object)?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult setup_begin(IsleCreditsSource* source,std::int32_t argument,
    std::uint32_t screen,std::uint32_t setup,std::uint32_t arena,
    int near_disabled,int updates,int draws) noexcept {
    return credits_invoke(source,[&](auto& owner,auto&) {
        if(argument!=owner.argument || screen!=screen_address || setup!=setup_address ||
           arena!=arena_bytes || near_disabled!=1 || updates!=1 || draws!=2)
            return ISLE_GAME_INVALID_ARGUMENT;
        return owner.setup_begin(argument,screen,setup,owner.screen_words[6],near_disabled,updates,draws);
    });
}
IsleGameResult setup_poll(IsleCreditsSource* source,int* complete) noexcept {
    if(complete)*complete=0;if(!complete)return ISLE_GAME_INVALID_ARGUMENT;
    return credits_invoke(source,[&](auto& owner,auto&) {
        int done=0;std::int32_t latch=0;
        const auto result=owner.setup_poll(&done,&latch);
        if(result==ISLE_GAME_OK)*complete=done;
        // The setup latch is not Original entry's later literal return8.
        return result;
    });
}
IsleGameResult pressed(IsleCreditsSource* source,std::uint32_t* output) noexcept {
    if(output)*output=0;if(!output)return ISLE_GAME_INVALID_ARGUMENT;
    return credits_invoke(source,[&](auto& owner,auto&) {
        if(!owner.native->input_initialized)return ISLE_GAME_INVALID_STATE;
        const auto value=owner.input.menu(0);
        if(value.error!=game_input::Error::none || !value.input)return ISLE_GAME_INVALID_STATE;
        *output=value.input->pressed;return ISLE_GAME_OK;
    });
}
IsleGameResult sound_leaf(IsleCreditsSource* source,int id,int value,int pan,float pitch,
    int reverb,std::int32_t* output) noexcept {
    if(output)*output=0;if(!output)return ISLE_GAME_INVALID_ARGUMENT;
    return credits_invoke(source,[&](auto& owner,auto&) {return sound(owner,id,value,pan,pitch,reverb,output);});
}
IsleGameResult song_leaf(IsleCreditsSource* source,int player,int id) noexcept {
    return credits_invoke(source,[&](auto& owner,auto&) {return song(owner,player,id);});
}
IsleGameResult volume_leaf(IsleCreditsSource* source,int player,int value,int frames) noexcept {
    return credits_invoke(source,[&](auto& owner,auto&) {return volume(owner,player,value,frames);});
}
IsleGameResult fade_color(IsleCreditsSource* source,int r,int g,int b) noexcept {
    return credits_invoke(source,[&](auto& owner,auto& state) {
        if(!state.transition_initialized || r<0 || r>255 || g<0 || g>255 || b<0 || b>255)
            return ISLE_GAME_INVALID_ARGUMENT;
        auto* actual=owner.native->services->screen_transition_source();
        if(actual->status!=ISLE_GAME_OK)return actual->status;
        actual->D_800AF054=r;actual->D_800AF058=g;actual->D_800AF05C=b;
        return ISLE_GAME_OK;
    });
}
IsleGameResult fade_duration(IsleCreditsSource* source,int direction,float duration) noexcept {
    return credits_invoke(source,[&](auto& owner,auto& state) {
        if(!state.transition_initialized || (direction!=0 && direction!=1) ||
           !std::isfinite(duration) || duration<=0 ||
           !std::isfinite(float(255.0/(double(duration)*60.0))))return ISLE_GAME_INVALID_ARGUMENT;
        auto* actual=owner.native->services->screen_transition_source();
        if(actual->status!=ISLE_GAME_OK)return actual->status;
        const auto callback=direction?isle_screen_transition_func_800A7760:isle_screen_transition_func_800A7660;
        auto* fade=isle_screen_transition_create(actual,callback,0x800aecb0);
        if(!fade)return actual->status==ISLE_GAME_OK?ISLE_GAME_EXHAUSTED:actual->status;
        fade->duration=duration;fade->render_kind=0;
        const IsleGameModelTransform transform{{0,0,0},{0,0,0},{1,3.2f,2.4f}};
        auto result=isle_game_model_set_transform(owner.source().game,fade->model,0,&transform);
        if(result!=ISLE_GAME_OK)return result;
        actual->D_800AF050=1;actual->D_800AF060=direction?255:0;actual->D_800BE2CC=0;
        return ISLE_GAME_OK;
    });
}
IsleGameResult exit_scene(IsleCreditsSource* source) noexcept {
    return credits_invoke(source,[](auto& owner,auto&) {return owner.request_exit(0);});
}
IsleGameResult display_list(IsleCreditsSource* source,IsleCreditsDisplayListCursor** cursor,
    std::uint32_t address) noexcept {
    return credits_invoke(source,[&](auto& owner,auto& state) {
        if(address!=pre_render_address || !state.display_active ||
           cursor!=&state.display_current || *cursor!=&state.display[0])return ISLE_GAME_INVALID_ARGUMENT;
        auto& current=state.display[0];
        if(current.allocation!=owner.native.get() || current.source!=source || current.state!=&state ||
           !current.generation || !current.control)return ISLE_GAME_INVALID_STATE;
        current.control->call({pre_render_address&0x1fffffffu,pre_render_words.size()});
        state.display_current=&state.display[1];
        return ISLE_GAME_OK;
    });
}
IsleGameResult time(IsleCreditsSource* source,std::uint64_t* output) noexcept {
    if(output)*output=0;if(!output)return ISLE_GAME_INVALID_ARGUMENT;
    return credits_invoke(source,[&](auto& owner,auto&) {
        // One actual osGetTime-domain sample. No Host poll/RNG/update credit.
        *output=host::RetraceClock::guest_ticks(owner.progress.elapsed_ns());
        return ISLE_GAME_OK;
    });
}
/* Integration applies the precise provider ABI patch in the receipt. Source C
 * alone repeats a pending attempt and services real native progress between
 * attempts; this leaf never spins or advances OM/GTL. */
IsleGameResult prepare_course(IsleCreditsSource* source,const IslePlayerPhotoData* photo,int* done) noexcept {
    if(done)*done=0;if(!done || !photo)return ISLE_GAME_INVALID_ARGUMENT;
    return credits_invoke(source,[&](auto& owner,auto&) {
        if(!owner.native->pipeline)return ISLE_GAME_INVALID_STATE;
        const auto identity=owner.photos.identify(photo);
        if(!identity || !identity.value.photo || identity.value.storage!=ISLE_PHOTO_REPORT)
            return ISLE_GAME_INVALID_ARGUMENT;
        if(owner.native->pipeline->prepare_course(photo->levelID)) {
            *done=1;return ISLE_GAME_OK;
        }
        return owner.native->pipeline->error()[0]?ISLE_GAME_EXECUTION_FAILURE:ISLE_GAME_OK;
    });
}
IsleGameResult preview_scale(IsleCreditsSource* source,float* output) noexcept {
    if(output)*output=0;if(!output)return ISLE_GAME_INVALID_ARGUMENT;
    return credits_invoke(source,[&](auto& owner,auto&) {
        const auto scale=owner.modules.preview_scale;
        if(!std::isfinite(scale) || scale<=0)return ISLE_GAME_INVALID_STATE;
        *output=scale;return ISLE_GAME_OK;
    });
}
IsleGameResult request_cancel(IsleCreditsSource* source) noexcept {
    return credits_invoke<true>(source,[](auto& owner,auto& state) {
        state.cancel_requested=true;
        // Signal only, including a nested Host-progress call while the C worker
        // stack is parked. No join/World teardown/lease or raster release here.
        if(owner.native->pipeline)owner.native->pipeline->cancel();
        return state.failure;
    });
}
IsleGameResult cancel_drain(IsleCreditsSource* source,int* done) noexcept {
    if(done)*done=0;if(!done)return ISLE_GAME_INVALID_ARGUMENT;
    return credits_invoke<true>(source,[&](auto& owner,auto& state) {
        if(source->worker_active || !state.worker || !state.worker->continuation_joined())
            return ISLE_GAME_INVALID_STATE;
        if(state.native_drained) {*done=1;return state.failure;}
        if(!state.cancel_requested)return ISLE_GAME_INVALID_STATE;
        auto& n=*owner.native;
        const auto idle=isle_game_require_idle(owner.source().game);
        if(idle!=ISLE_GAME_OK)return idle;
        const auto progressed=owner.native_progress(&owner);
        remember(state,progressed);
        // Actual Presentation World handoff precedes shared Replay retirement.
        if(n.presentation) {
            const auto status=n.presentation->retire();
            if(n.presentation->error()[0])remember(state,ISLE_GAME_EXECUTION_FAILURE);
            if(status!=ISLE_PHOTO_PRESENTATION_RETIRED)return state.failure;
        }
        bool joined=true;
        if(n.replay) {
            const auto status=n.replay->retire(joined);
            remember(state,status);
            if(!joined)return state.failure;
        }
        if(n.pipeline && !n.pipeline->retire()) {
            if(n.pipeline->error()[0])remember(state,ISLE_GAME_EXECUTION_FAILURE);
            return state.failure;
        }
        if(n.pipeline && n.pipeline->error()[0])remember(state,ISLE_GAME_EXECUTION_FAILURE);
        if(n.graphics) {
            if(!n.graphics->quiescent())return state.failure;
            const auto completed=n.graphics->completed();
            if(completed && completed.ordinal!=n.released_ordinal)return state.failure;
        }
        state.native_drained=true;*done=1;
        return state.failure;
    });
}
}

CreditsSceneNative::CreditsSceneNative(application::Photos&) noexcept {}
CreditsSceneNative::~CreditsSceneNative() {
    if(display_active || (worker && !worker->joined()))std::terminate();
}
IsleGameResult CreditsSceneNative::pump() noexcept {
    if(!prepared || !worker)return ISLE_GAME_INVALID_STATE;
    return remember(*this,worker->pump());
}
bool CreditsSceneNative::main_excluded() const noexcept {return worker && worker->main_excluded();}
IsleGameResult CreditsSceneNative::request_cancel() noexcept {
    if(!worker)return ISLE_GAME_INVALID_STATE;
    return remember(*this,worker->request_cancel());
}
IsleGameResult CreditsSceneNative::join(int* done) noexcept {
    if(done)*done=0;if(!worker || !done)return ISLE_GAME_INVALID_ARGUMENT;
    const auto result=worker->join(done);
    remember(*this,result);
    return failure!=ISLE_GAME_OK?failure:result;
}
bool CreditsSceneNative::joined() const noexcept {return worker && worker->joined();}
IsleCreditsDisplayListCursor** CreditsSceneNative::begin_display(void* allocation,
    std::uint64_t generation,scene::graphics::SourceControl& control) noexcept {
    if(!prepared || !source || !allocation || !generation || display_active || cancel_requested)
        return nullptr;
    for(auto& entry:display)entry={allocation,source,this,generation,&control};
    display_current=&display[0];display_active=true;
    return &display_current;
}
IsleGameResult CreditsSceneNative::end_display() noexcept {
    if(!display_active)return ISLE_GAME_INVALID_STATE;
    const auto result=display_current==&display[1]?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    display_active=false;display_current=nullptr;
    for(auto& entry:display)entry={};
    return remember(*this,result);
}
const IsleCreditsNative& credits_scene_native_table() noexcept {
    static const IsleCreditsNative table{
        screen,heap,heap_free,input_initialize,window_initialize,cameras,input_mask,
        process_owner,sprite_object,sprite_append,remove_sprites,setup_begin,setup_poll,
        pressed,sound_leaf,song_leaf,volume_leaf,fade_color,fade_duration,exit_scene,
        display_list,time,prepare_course,preview_scale,request_cancel,cancel_drain};
    return table;
}
IsleGameResult credits_scene_native_prepare(IsleCreditsSource* source,CreditsSceneNative& state) noexcept {
    if(!source || source->native!=&credits_scene_native_table() || source->worker || state.prepared ||
       source->argument<17 || source->argument>20 || source->common.scene!=source->argument)
        return ISLE_GAME_INVALID_ARGUMENT;
    return SceneAccess::invoke(&source->common,[&](auto& owner) {
        if(std::get_if<IsleCreditsSource>(&owner.context)!=source || !owner.native ||
           std::get_if<CreditsSceneNative>(&owner.native->menu)!=&state ||
           owner.kind!=Resources::Kind::credits || owner.argument!=source->argument)
            return ISLE_GAME_INVALID_STATE;
        auto result=check_resources(owner);
        if(result!=ISLE_GAME_OK)return result;
        auto& n=*owner.native;
        state.source=source;
        for(std::size_t i=0;i<3;++i) {
            const auto value=n.resources->word(0x801e9414u+std::uint32_t(i*4));
            if(value!=0)return ISLE_GAME_INVALID_STATE;
            source->D_credits_801E9414[i]=std::bit_cast<std::int32_t>(value);
        }
        state.prepared=true;
        state.worker=std::make_unique<IsleCreditsWorker>(*source);
        if(source->worker!=state.worker.get() || state.worker->check_access()!=ISLE_GAME_OK)
            return ISLE_GAME_INVALID_STATE;
        return ISLE_GAME_OK;
    });
}
}

extern "C" IsleGameResult isle_credits_native_cleanup_context(void* allocation,IsleCreditsSource** output) {
    if(output)*output=nullptr;
    if(!output || !allocation)return ISLE_GAME_INVALID_ARGUMENT;
    IsleCreditsSource* value=nullptr;
    const auto result=isle::post_course::SceneAccess::credits_cleanup_context(allocation,&value);
    if(result!=ISLE_GAME_OK)return result;
    if(!value)return ISLE_GAME_INVALID_STATE;
    *output=value;return ISLE_GAME_OK;
}
extern "C" IsleGameResult isle_credits_native_display_context(IsleCreditsDisplayListCursor** cursor,IsleCreditsSource** output) {
    if(output)*output=nullptr;
    if(!output || !cursor)return ISLE_GAME_INVALID_ARGUMENT;
    IsleCreditsSource* value=nullptr;
    const auto result=isle::post_course::SceneAccess::credits_display_context(cursor,&value);
    if(result!=ISLE_GAME_OK)return result;
    if(!value)return ISLE_GAME_INVALID_STATE;
    *output=value;return ISLE_GAME_OK;
}

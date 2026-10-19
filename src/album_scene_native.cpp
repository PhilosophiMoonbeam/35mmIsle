#include "album_scene_native.hpp"
#include "post_course_native_helpers.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <utility>

namespace isle::post_course {
namespace {
constexpr std::uint32_t ram_bytes = 0x800000;
constexpr std::array<std::uint32_t,28> initialization_words{
    0xe7000000,0,0xe3000800,0,0xd9fdffff,0,0xe3000f00,0,
    0xe3000d01,0,0xe3000c00,0x00080000,0xe3001001,0,
    0xe3001201,0x00002000,0xe3001402,0x00000c00,
    0xdb040004,6,0xdb04000c,6,0xdb040014,0xfffa,
    0xdb04001c,0xfffa,0xdf000000,0};
constexpr std::array<std::uint32_t,18> sprite_identities{
    0x801e4518,0x801ed550,0x801f2d28,0x801f2f88,0x801f3108,
    0x801f3288,0x801f3408,0x801f3588,0x801f6b50,0x801fb1d0,
    0x801ffa10,0x801ffc80,0x80200058,0x80200400,0x802005b8,
    0x802006e8,0x80201728,0x80208b10};
void word_bytes(std::uint8_t* output,std::uint32_t value) noexcept {
    for(unsigned i=0;i<4;++i)output[i]=static_cast<std::uint8_t>(value>>(24-i*8));
}
bool physical_range(std::uint32_t address,std::size_t size) noexcept {
    return address && !(address&7u) && address<=ram_bytes && size<=ram_bytes-address;
}

template<class O> bool typed(O& owner,IsleAlbumSource* source) noexcept {
    return owner.kind==Resources::Kind::album &&
        std::get_if<IsleAlbumSource>(&owner.context)==source;
}
template<class F> IsleGameResult call(IsleAlbumSource* source,F&& function) noexcept {
    return SceneAccess::invoke(source?&source->common:nullptr,[&](auto& owner) {
        if(!typed(owner,source) || !owner.native)return ISLE_GAME_INVALID_STATE;
        auto* state=std::get_if<AlbumSceneNative>(&owner.native->menu);
        if(!state || state->source!=source)return ISLE_GAME_INVALID_STATE;
        if(state->status!=ISLE_GAME_OK)return state->status;
        if(source->common.status!=ISLE_GAME_OK)return source->common.status;
        const auto result=function(owner,*state);
        if(result!=ISLE_GAME_OK && result!=ISLE_GAME_STOPPED) {
            if(state->status==ISLE_GAME_OK)state->status=result;
            return owner.fail(result,"Album12 native leaf failed");
        }
        return result;
    });
}
IsleGameResult ui_heap(IsleAlbumSource* source,std::size_t bytes) noexcept {
    return call(source,[&](auto& owner,auto&) {return isle::post_course::admit_heap(owner,bytes,0x46000);});
}
IsleGameResult input_initialize(IsleAlbumSource* source) noexcept {
    return call(source,[](auto& owner,auto&) {return isle::post_course::initialize_input(owner);});
}
IsleGameResult input(IsleAlbumSource* source,std::int32_t port,IsleAlbumInput* output) noexcept {
    if(output)*output={};
    if(!output)return ISLE_GAME_INVALID_ARGUMENT;
    IsleAlbumInput value{};
    const auto result=call(source,[&](auto& owner,auto&) {
        if(!owner.live().input_initialized)return ISLE_GAME_INVALID_STATE;
        const auto sample=owner.input.menu(port);
        if(sample.error!=game_input::Error::none || !sample.input)return ISLE_GAME_INVALID_STATE;
        value={sample.input->current,sample.input->pressed};return ISLE_GAME_OK;
    });
    if(result==ISLE_GAME_OK)*output=value;
    return result;
}
IsleGameResult input_repeat(IsleAlbumSource* source,std::int32_t delay,std::int32_t interval) noexcept {
    return call(source,[&](auto& owner,auto&) {
        return owner.input.set_menu_repeat(delay,interval)==game_input::Error::none?
            ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult input_mask(IsleAlbumSource* source,std::uint32_t mask) noexcept {
    return call(source,[&](auto& owner,auto&) {
        return owner.input.set_menu_mask(mask)==game_input::Error::none?
            ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult camera(IsleAlbumSource* source,IsleGameObject** output) noexcept {
    if(output)*output=nullptr;
    if(!output)return ISLE_GAME_INVALID_ARGUMENT;
    IsleGameObject* object=nullptr;
    const auto result=call(source,[&](auto& owner,auto& state) {
        if(state.main_object || state.main_camera)return ISLE_GAME_INVALID_STATE;
        auto status=isle::post_course::camera_owner(owner,0x200,5,5,state.main_object,state.main_camera);
        if(status!=ISLE_GAME_OK)return status;
        // Exact Album102-124 tuple; no CameraCheck boolean-pose shortcut.
        status=isle::post_course::viewport(owner,state.main_camera,320,240);
        if(status!=ISLE_GAME_OK)return status;
        const IsleGamePerspective perspective{33,321.0f/241.0f,62.5f,8000,1};
        status=isle_game_camera_set_perspective(source->common.game,state.main_camera,&perspective);
        if(status!=ISLE_GAME_OK)return status;
        const IsleGameLookAt pose{{-1,0,0},{0,0,0},{0,0,1}};
        status=isle_game_camera_set_look_at(source->common.game,state.main_camera,&pose);
        if(status!=ISLE_GAME_OK)return status;
        const IsleGameCameraRenderState render{4,1,0x100000};
        status=isle_game_camera_set_render_state(source->common.game,state.main_camera,&render);
        if(status!=ISLE_GAME_OK)return status;
        owner.live().add_camera(state.main_object,state.main_camera,20,0x100000,0x100000);
        object=state.main_object;return ISLE_GAME_OK;
    });
    if(result==ISLE_GAME_OK)*output=object;
    return result;
}
IsleGameResult aux_camera(IsleAlbumSource* source,std::int32_t link,std::int32_t mode) noexcept {
    return call(source,[&](auto& owner,auto& state) {
        if(link!=1 || mode!=0x101)return ISLE_GAME_INVALID_ARGUMENT;
        if(state.auxiliary_object || state.auxiliary_camera)return ISLE_GAME_INVALID_STATE;
        auto& native=owner.live();
        auto* transition=native.services->screen_transition_source();
        if(!transition || transition->camera_object)return ISLE_GAME_INVALID_STATE;
        auto status=isle_screen_transition_camera(transition,link,mode);
        if(status!=ISLE_GAME_OK)return status;
        state.auxiliary_object=transition->camera_object;
        state.auxiliary_camera=transition->D_800BE2C8;
        status=isle::post_course::viewport(owner,state.auxiliary_camera,320,240);
        if(status!=ISLE_GAME_OK)return status;
        const IsleGamePerspective perspective{45,321.0f/241.0f,8,1024,1};
        status=isle_game_camera_set_perspective(source->common.game,state.auxiliary_camera,&perspective);
        if(status!=ISLE_GAME_OK)return status;
        const IsleGameLookAt pose{{290,0,0},{0,0,0},{0,0,1}};
        status=isle_game_camera_set_look_at(source->common.game,state.auxiliary_camera,&pose);
        if(status!=ISLE_GAME_OK)return status;
        const IsleGameCameraRenderState render{4,0x10001,0};
        status=isle_game_camera_set_render_state(source->common.game,state.auxiliary_camera,&render);
        if(status!=ISLE_GAME_OK)return status;
        transition->D_800BE2C0=link;
        transition->camera_flags=4;transition->camera_mask=2;transition->camera_buffer_mask=2;
        transition->D_800AF054=0;transition->D_800AF058=0;
        transition->D_800AF05C=0;transition->D_800AF050=0;
        native.source_objects.push_back(state.auxiliary_object);
        native.add_camera(state.auxiliary_object,state.auxiliary_camera,1,2,2);
        return ISLE_GAME_OK;
    });
}
IsleGameResult process_object(IsleAlbumSource* source,std::int32_t link,
    std::int32_t draw_link,const void* display_list,IsleGameObject** output) noexcept {
    if(output)*output=nullptr;
    if(!output)return ISLE_GAME_INVALID_ARGUMENT;
    IsleGameObject* object=nullptr;
    const auto result=call(source,[&](auto& owner,auto&) {
        return isle::post_course::process_object(owner,link,draw_link,display_list,&object);
    });
    if(result==ISLE_GAME_OK)*output=object;
    return result;
}
IsleGameResult replay_initialize(IsleAlbumSource* source) noexcept {
    return call(source,[](auto& owner,auto&) {return owner.initialize_replay();});
}
IsleGameResult replay_cleanup(IsleAlbumSource* source) noexcept {
    return call(source,[](auto& owner,auto&) {return owner.cleanup_replay();});
}
IsleGameResult preview_scale(IsleAlbumSource* source,float* output) noexcept {
    if(output)*output=0;
    if(!output)return ISLE_GAME_INVALID_ARGUMENT;
    float value=0;
    const auto result=call(source,[&](auto& owner,auto&) {
        if(!std::isfinite(owner.modules.preview_scale) || owner.modules.preview_scale<=0)
            return ISLE_GAME_INVALID_STATE;
        value=owner.modules.preview_scale;return ISLE_GAME_OK;
    });
    if(result==ISLE_GAME_OK)*output=value;
    return result;
}
IsleGameResult sound(IsleAlbumSource* source,std::int32_t id) noexcept {
    return call(source,[&](auto& owner,auto&) {return isle::post_course::sound(owner,id,0x7fff,64,1,0);});
}
IsleGameResult song(IsleAlbumSource* source,std::int32_t player,std::int32_t id) noexcept {
    return call(source,[&](auto& owner,auto&) {return isle::post_course::song(owner,player,id);});
}
IsleGameResult volume(IsleAlbumSource* source,std::int32_t player,std::int32_t value) noexcept {
    return call(source,[&](auto& owner,auto&) {return isle::post_course::volume(owner,player,value,0);});
}
IsleGameResult volume_smooth(IsleAlbumSource* source,std::int32_t player,
    std::int32_t value,std::int32_t frames) noexcept {
    return call(source,[&](auto& owner,auto&) {return isle::post_course::volume(owner,player,value,frames);});
}
template<class F> IsleGameResult name(IsleAlbumSource* source,const char** output,F&& function) noexcept {
    if(output)*output=nullptr;
    if(!output)return ISLE_GAME_INVALID_ARGUMENT;
    const char* value=nullptr;
    const auto result=call(source,[&](auto& owner,auto&) {
        const auto status=function(owner,value);
        return status!=ISLE_GAME_OK?status:value?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
    if(result==ISLE_GAME_OK)*output=value;
    return result;
}
IsleGameResult player_name(IsleAlbumSource* source,const char** output) noexcept {
    return name(source,output,[](auto& owner,const char*& value) {return isle::post_course::player_name(owner,value);});
}
IsleGameResult pokemon_name(IsleAlbumSource* source,std::int32_t id,const char** output) noexcept {
    return name(source,output,[&](auto& owner,const char*& value) {return isle::post_course::pokemon_name(owner,id,value);});
}
IsleGameResult level_name(IsleAlbumSource* source,std::int32_t id,const char** output) noexcept {
    return name(source,output,[&](auto& owner,const char*& value) {return isle::post_course::course_name(owner,id,value);});
}
IsleGameResult scene_result(IsleAlbumSource* source,std::int32_t result) noexcept {
    return call(source,[&](auto& owner,auto&) {return owner.request_exit(result);});
}
IsleGameResult entry_begin(IsleAlbumSource* source,std::int32_t argument) noexcept {
    return call(source,[&](auto& owner,auto&) {
        if(argument!=12)return ISLE_GAME_INVALID_ARGUMENT;
        return owner.setup_begin(argument,0x801e4250,0x801e426c,0x16a99,1,1,1);
    });
}
IsleGameResult entry_poll(void* raw,int* complete,std::int32_t* output) noexcept {
    if(complete)*complete=0;
    if(output)*output=0;
    if(!complete || !output)return ISLE_GAME_INVALID_ARGUMENT;
    int done=0;std::int32_t value=0;
    const auto result=SceneAccess::invoke_owner(raw,[&](auto& owner) {
        if(owner.kind!=Resources::Kind::album || !std::get_if<IsleAlbumSource>(&owner.context))
            return ISLE_GAME_INVALID_STATE;
        return owner.setup_poll(&done,&value);
    });
    if(result==ISLE_GAME_OK){*complete=done;*output=value;}
    return result;
}
IsleGameResult notices_or(void* raw,std::uint32_t mask) noexcept {
    return SceneAccess::invoke_owner(raw,[&](auto& owner) {
        if(owner.kind!=Resources::Kind::album || !std::get_if<IsleAlbumSource>(&owner.context) ||
           mask!=0x100000 || !owner.setup_returned || owner.cancelling || owner.failure!=ISLE_GAME_OK)
            return ISLE_GAME_INVALID_STATE;
        owner.state.add_scene_notices(mask);return ISLE_GAME_OK;
    });
}
IsleGameResult display_list(IsleAlbumSource* source,void* raw) noexcept {
    return call(source,[&](auto&,auto& state) {
        auto& cursor=state.display_cursor;
        if(raw!=&cursor || !cursor.active || !state.render_bound ||
           cursor.address>cursor.end || cursor.end-cursor.address<16)
            return ISLE_GAME_INVALID_STATE;
        // gSPDisplayList(++(*arg0),list): preincrement the pointer-reference,
        // write there, and DO NOT postincrement after that write.
        const auto address=cursor.address+8;
        cursor.ram[address/4]=0xde000000;
        cursor.ram[address/4+1]=state.initialization_address;
        cursor.address=address;return ISLE_GAME_OK;
    });
}
IsleGameResult render_state(IsleAlbumSource* source,std::int32_t,std::int32_t) noexcept {
    return call(source,[&](auto&,auto& state) {
        auto& cursor=state.display_cursor;
        if(!cursor.active || !state.render_bound || cursor.address>cursor.end ||
           cursor.end-cursor.address<40)return ISLE_GAME_INVALID_STATE;
        constexpr std::uint32_t render_mode=0x8|0x10|0x20|0x40|0x2000|
            (3u<<26)|(1u<<22)|(3u<<18); // cycle2 G_RM_NOOP2=0
        constexpr std::uint32_t combine0=0xfc000000u|(1u<<20)|(4u<<15)|
            (1u<<12)|(4u<<9)|(15u<<5)|31u;
        constexpr std::uint32_t combine1=(15u<<28)|(7u<<15)|(7u<<12)|(7u<<9)|
            (15u<<24)|(7u<<21)|(7u<<18)|(7u<<3);
        const std::array<std::uint32_t,10> words{0xde000000,state.initialization_address,
            0xe3001801,0,0xe7000000,0,0xe200001c,render_mode,combine0,combine1};
        std::copy(words.begin(),words.end(),cursor.ram.begin()+cursor.address/4);
        cursor.address+=40;return ISLE_GAME_OK;
    });
}
IsleGameResult unwritten_sp4(IsleAlbumSource* source,IsleGameSprite** output) noexcept {
    if(output)*output=nullptr;
    if(!output)return ISLE_GAME_INVALID_ARGUMENT;
    IsleGameSprite* root=nullptr;
    const auto result=call(source,[&](auto&,auto&) {
        if(!source->album_DraggedPhotoGObj || !source->album_DraggedPhotoSObj)
            return ISLE_GAME_INVALID_STATE;
        const auto status=isle_post_window_sprite_root(source->common.window,
            source->album_DraggedPhotoGObj,&root);
        if(status!=ISLE_GAME_OK)return status;
        return root && root!=source->album_DraggedPhotoSObj?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
    if(result==ISLE_GAME_OK)*output=root;
    return result;
}
IsleGameResult cover_overflow_get(IsleAlbumSource* source,std::int32_t index,
    IsleGameSprite** output) noexcept {
    if(output)*output=nullptr;
    if(!output)return ISLE_GAME_INVALID_ARGUMENT;
    IsleGameSprite* root=nullptr;
    const auto result=call(source,[&](auto&,auto& state) {
        if(index==5)return source->album_D_8025007C_A1A2CC==0?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
        if(index!=4)return ISLE_GAME_INVALID_ARGUMENT;
        auto* object=source->album_CoverTitleGObj;
        if(!object)return ISLE_GAME_OK;
        if(state.cover_allocation && state.cover_allocation!=object)return ISLE_GAME_INVALID_STATE;
        const auto status=isle_post_window_sprite_root(source->common.window,object,&root);
        if(status!=ISLE_GAME_OK)return status;
        if(!root || root!=source->album_CoverTitleSObj[0])return ISLE_GAME_INVALID_STATE;
        state.cover_allocation=object;return ISLE_GAME_OK;
    });
    if(result==ISLE_GAME_OK)*output=root;
    return result;
}
IsleGameResult cover_overflow_set(IsleAlbumSource* source,std::int32_t index,
    IsleGameSprite* value) noexcept {
    return call(source,[&](auto&,auto& state) {
        if(value)return ISLE_GAME_INVALID_STATE;
        if(index==5) {
            if(source->album_D_8025007C_A1A2CC)return ISLE_GAME_INVALID_STATE;
            source->album_D_8025007C_A1A2CC=0;return ISLE_GAME_OK;
        }
        if(index!=4)return ISLE_GAME_INVALID_ARGUMENT;
        if(auto* object=source->album_CoverTitleGObj) {
            if(state.cover_allocation && state.cover_allocation!=object)return ISLE_GAME_INVALID_STATE;
            IsleGameSprite* root=nullptr;
            const auto status=isle_post_window_sprite_root(source->common.window,object,&root);
            if(status!=ISLE_GAME_OK)return status;
            if(!root || root!=source->album_CoverTitleSObj[0])return ISLE_GAME_INVALID_STATE;
            state.cover_allocation=object;
        }
        // Explicit semantic repair, NOT linked-layout/GObj.prevDl emulation.
        source->album_CoverTitleGObj=nullptr;return ISLE_GAME_OK;
    });
}
IsleGameResult empty_last_photo(IsleAlbumSource* source,std::int32_t* output) noexcept {
    if(output)*output=0;
    if(!output)return ISLE_GAME_INVALID_ARGUMENT;
    const auto result=call(source,[](auto&,auto&) {return ISLE_GAME_OK;});
    // The counted C lowering calls only after its actual scan found no photos.
    // Do not rerun/replace that scan or use the prior album_PhotoCount value.
    if(result==ISLE_GAME_OK)*output=-1;
    return result;
}
IsleGameResult character_grid(IsleAlbumSource* source,std::uint32_t page,
    const std::uint8_t** output,std::size_t* size) noexcept {
    if(output)*output=nullptr;
    if(size)*size=0;
    if(!output || !size)return ISLE_GAME_INVALID_ARGUMENT;
    const std::uint8_t* bytes=nullptr;
    const auto result=call(source,[&](auto&,auto& state) {
        if(page>2)return ISLE_GAME_INVALID_ARGUMENT;
        if(page!=0 || state.character_page0.size()!=190)return ISLE_GAME_INVALID_STATE;
        bytes=state.character_page0.data();return ISLE_GAME_OK;
    });
    if(result==ISLE_GAME_OK){*output=bytes;*size=190;}
    return result;
}
IsleGameResult float_format_integer(IsleAlbumSource* source,std::int32_t,char* output,
    std::size_t capacity,std::int32_t* count) noexcept {
    if(output && capacity)output[0]=0;
    if(count)*count=0;
    if(!output || !capacity || !count)return ISLE_GAME_INVALID_ARGUMENT;
    // Dormant %5.2f/s32 exception has no qualified target varargs value.
    return call(source,[](auto&,auto&) {return ISLE_GAME_INVALID_STATE;});
}
IsleGameResult inactive_letter_width(IsleAlbumSource* source,std::int32_t* output) noexcept {
    if(output)*output=0;
    if(!output)return ISLE_GAME_INVALID_ARGUMENT;
    return call(source,[&](auto&,auto&) {
        if(source->album_CommentCursorPos!=31 || source->album_D_8024FFE8_A1A238 ||
           source->D_80208B54_9D2DA4<0 || source->D_80208B54_9D2DA4>=60)
            return ISLE_GAME_INVALID_STATE;
        const std::uint16_t* comment=nullptr;
        auto result=isle_application_photos_album_comment(source->common.photos,
            source->D_80208B54_9D2DA4,&comment);
        if(result!=ISLE_GAME_OK)return result;
        if(!comment || comment[31])return ISLE_GAME_INVALID_STATE;
        std::array<char,64> text{};std::size_t used=0;
        for(std::size_t i=0;i<31 && comment[i];++i) {
            if(comment[i]==0xa1a1)text[used++]=' ';
            else {text[used++]=static_cast<char>(comment[i]>>8);text[used++]=static_cast<char>(comment[i]);}
        }
        int width=0;
        result=isle_post_window_text_width(source->common.window,{text.data(),used},&width);
        if(result!=ISLE_GAME_OK)return result;
        // width0 is equivalent to Source's defined full inactive placement -16
        // ONLY for actual valid Source comment widths <=168. No fake bytes.
        return width>=0 && width<=168?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult sprite_clear_links(IsleAlbumSource* source,IsleGameSprite* sprite) noexcept {
    return call(source,[&](auto&,auto&) {
        return isle_post_window_sprite_clear_root_links(source->common.window,sprite);
    });
}
IsleGameResult deallocate(IsleAlbumSource* source,void* allocation) noexcept {
    return SceneAccess::invoke_retiring(source?&source->common:nullptr,[&](auto& owner) {
        if(!typed(owner,source) || !owner.native)return ISLE_GAME_INVALID_STATE;
        auto* state=std::get_if<AlbumSceneNative>(&owner.native->menu);
        if(!state || state->source!=source)return ISLE_GAME_INVALID_STATE;
        return SceneAccess::deallocate_native(owner,allocation);
    });
}
}

IsleGameResult AlbumSceneNative::bind_render(std::uint32_t initialization,
    std::uint32_t prefix,std::uint32_t common) noexcept {
    if(render_bound || source || !physical_range(initialization,initialization_bytes.size()) ||
       !physical_range(prefix,pre_render_bytes.size()) || !physical_range(common,8) ||
       (std::uint64_t(initialization)<std::uint64_t(prefix)+pre_render_bytes.size() &&
        std::uint64_t(prefix)<std::uint64_t(initialization)+initialization_bytes.size()) ||
       (common>=initialization && common<initialization+initialization_bytes.size()) ||
       (common>=prefix && common<prefix+pre_render_bytes.size()))return ISLE_GAME_INVALID_ARGUMENT;
    initialization_address=initialization;pre_render_address=prefix;
    // Call common, then tail-branch to the actual Album initialized list. Its
    // EndDL returns to the Frame reset caller, not into uninitialized RAM.
    word_bytes(pre_render_bytes.data(),0xde000000);
    word_bytes(pre_render_bytes.data()+4,common);
    word_bytes(pre_render_bytes.data()+8,0xde010000);
    word_bytes(pre_render_bytes.data()+12,initialization);
    render_assets={sprite::AssetSpan{initialization,initialization_bytes},
                   sprite::AssetSpan{prefix,pre_render_bytes}};
    render_bound=true;return ISLE_GAME_OK;
}
IsleGameResult AlbumSceneNative::apply_render(scene::render::Input& input) noexcept {
    if(!source || status!=ISLE_GAME_OK || !render_bound || input.game!=source->common.game)
        return ISLE_GAME_INVALID_STATE;
    for(const auto& required:render_assets) {
        bool found=false;
        for(const auto& asset:input.resources.immutable)
            if(asset.address==required.address && asset.bytes.data()==required.bytes.data() &&
               asset.bytes.size()==required.bytes.size())found=true;
        if(!found)return ISLE_GAME_INVALID_STATE;
    }
    input.resources.pre_render=pre_render_address;return ISLE_GAME_OK;
}
IsleGameResult AlbumSceneNative::begin_cursor(scene::graphics::SourceControl& control,
    std::uint32_t address,std::uint32_t end) noexcept {
    const auto ram=control.ram();const auto epoch=control.epoch();
    if(!source || status!=ISLE_GAME_OK || !render_bound || display_cursor.active ||
       ram.size()!=ram_bytes/4 || !epoch || (address&7u) || (end&7u) ||
       address>end || end>ram_bytes)return ISLE_GAME_INVALID_STATE;
    display_cursor={ram,epoch,address,end,true};return ISLE_GAME_OK;
}
IsleGameResult AlbumSceneNative::end_cursor(std::uint32_t* address) noexcept {
    if(address)*address=0;
    if(!address)return ISLE_GAME_INVALID_ARGUMENT;
    if(!display_cursor.active)return ISLE_GAME_INVALID_STATE;
    const auto value=display_cursor.address;display_cursor={};
    if(status!=ISLE_GAME_OK)return status;
    *address=value;return ISLE_GAME_OK;
}
const IsleAlbumNative& album_scene_native_table() noexcept {
    static const IsleAlbumNative table{
        ui_heap,input_initialize,input,input_repeat,input_mask,camera,aux_camera,process_object,
        replay_initialize,replay_cleanup,preview_scale,sound,song,volume,volume_smooth,
        player_name,pokemon_name,level_name,scene_result,entry_begin,entry_poll,notices_or,
        display_list,render_state,unwritten_sp4,cover_overflow_get,cover_overflow_set,
        empty_last_photo,character_grid,float_format_integer,inactive_letter_width,
        sprite_clear_links,deallocate};
    return table;
}
IsleGameResult album_scene_native_prepare(IsleAlbumSource* source,AlbumSceneNative& state) noexcept {
    return SceneAccess::invoke(source?&source->common:nullptr,[&](auto& owner) {
        if(!typed(owner,source) || !owner.native || state.source || state.status!=ISLE_GAME_OK ||
           std::get_if<AlbumSceneNative>(&owner.native->menu)!=&state)
            return ISLE_GAME_INVALID_STATE;
        auto& native=*owner.native;
        if(!native.resources || native.resources->kind()!=Resources::Kind::album ||
           !native.window || source->common.window!=native.window->source() ||
           source->common.game!=native.storage.game() || source->common.photos!=owner.photos.native_source() ||
           !native.graphics || native.thumbnail_slots.size()!=8 || native.element_slots.size()<26)
            return ISLE_GAME_INVALID_STATE;
        const auto& resources=*native.resources;
        const auto data=resources.bytes(0x801e41e0,112);
        for(std::size_t i=0;i<initialization_words.size();++i)
            if(resources.word(0x801e41e0+static_cast<std::uint32_t>(i*4))!=initialization_words[i])
                return ISLE_GAME_INVALID_STATE;
        std::copy(data.begin(),data.end(),state.initialization_bytes.begin());
        for(const auto identity:sprite_identities) {
            const auto index=resources.sprite_index(identity);
            if(index>=resources.roots().size() || !resources.roots()[index])return ISLE_GAME_INVALID_STATE;
        }
        if(resources.word(0x80208b70)!=0x80208c50 || resources.word(0x80208b74)!=1 ||
           resources.word(0x80208b78)!=10)return ISLE_GAME_INVALID_STATE;
        const auto page=resources.bytes(0x80208c50,190);
        for(std::size_t i=0;i<page.size();i+=2)
            if(page[i]<0xa1 || page[i]>0xfe || page[i+1]<0xa1 || page[i+1]>0xfe)
                return ISLE_GAME_INVALID_STATE;
        const auto small=Window::thumbnail_plane_bytes(resources,0x80201728);
        const auto large=Window::thumbnail_plane_bytes(resources,0x80208b10);
        for(std::size_t i=0;i<8;++i)
            if(native.thumbnail_slots[i].plane_capacity<(i==7?large:small))return ISLE_GAME_INVALID_STATE;
        const auto bound=native.window->bind_graphics_frame(*native.graphics);
        if(bound!=ISLE_GAME_OK)return bound;
        state.character_page0=page;state.source=source;
        return ISLE_GAME_OK;
    });
}
}

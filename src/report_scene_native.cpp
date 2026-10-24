#include "report_scene_native.hpp"
#include "post_course_native_helpers.hpp"
#include "application_photos.hpp"
#include "scene_graphics.hpp"
#include <bit>
#include <cmath>
#include <cstring>
#include <utility>

/* Precise Window/Services extensions delivered in integration-receipt.json.
 * They operate on the existing Window's actual native registrations. */
extern "C" {
IsleGameResult isle_post_window_sprite_add(IslePostCourseWindow*,IsleGameObject*,std::uint32_t,IsleGameSprite**);
IsleGameResult isle_post_window_sprite_remove(IslePostCourseWindow*,IsleGameObject*);
IsleGameResult isle_post_window_sprite_clear_link(IslePostCourseWindow*,IsleGameSprite*,int);
IsleGameResult isle_post_window_thumbnail_pixels(IslePostCourseWindow*,IslePostCourseThumbnail,std::uint8_t**,std::size_t*);
IsleGameResult isle_post_window_thumbnail_publish(IslePostCourseWindow*,IslePostCourseThumbnail,std::size_t,std::size_t);
IsleGameResult isle_post_window_thumbnail_select(IslePostCourseWindow*,IslePostCourseThumbnail,std::uint32_t);
}
namespace isle::post_course {
namespace {
constexpr std::array<std::uint32_t,6> sign_sprites{{0x801f67e8,0x801f7820,0x801f8858,0x801f9890,0x801fa8c8,0x801fb900}};
constexpr std::array<std::uint32_t,6> sign_pixels{{0x801f5800,0x801f6838,0x801f7870,0x801f88a8,0x801f98e0,0x801fa918}};
constexpr std::array<std::int32_t,6> sign_ids{{1004,1010,1028,1022,1018,1035}};
constexpr std::array<std::int16_t,66> poses{{
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,3,2,2,3,1,1,3,3,2,1,1,3,3,
    1,1,1,1,1,1,3,1,3,1,1,3,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,1,3,1,-1,-1}};
constexpr std::array<std::uint32_t,12> caption_lists{{
    0x80202e00,0x80202e0c,0x80202e18,0x80202e24,0x80202e30,0x80202e38,
    0x80202e44,0x80202e50,0x80202e5c,0x80202e68,0x80202e74,0x80202e80}};
constexpr std::array<std::array<std::uint32_t,3>,12> caption_words{{
    {{0x80203218,0x80203268,0}},{{0x802032c4,0x80203314,0}},
    {{0x8020334c,0x8020338c,0}},{{0x802033bc,0x80203414,0}},
    {{0x80203470,0,0}},{{0x802034ac,0x80203504,0}},
    {{0x80203530,0x8020357c,0}},{{0x802035dc,0x80203630,0}},
    {{0x8020367c,0x802036b4,0}},{{0x802036ec,0x80203720,0}},
    {{0x80203764,0x802037a4,0}},{{0x802037f4,0x80203840,0}}}};
constexpr std::array<std::uint32_t,8> button_tables{{0x80202ec8,0x80202ef8,0x80202f20,0x80202f48,0x80202f68,0x80202f88,0x80202fa8,0x80202fc0}};
constexpr std::array<std::size_t,8> button_counts{{6,5,5,4,4,4,3,7}};
constexpr std::array<std::array<int,7>,8> button_ids{{
    {{2,25,18,34,17,35,0}},{{2,25,34,17,35,0,0}},{{29,0,34,31,35,0,0}},
    {{28,34,5,35,0,0,0}},{{29,34,31,35,0,0,0}},{{28,34,31,35,0,0,0}},
    {{34,5,35,0,0,0,0}},{{13,14,15,16,34,5,35}}}};
constexpr std::array<std::uint32_t,24> sprite_roots{{
    0x801e3f48,0x801e8f70,0x801edd60,0x801edef0,0x801ee150,0x801ef7d0,
    0x801f0e50,0x801f2278,0x801f36a0,0x801f3860,0x801f3a10,0x801f3b90,
    0x801f4d48,0x801f5120,0x801f54c8,0x801f5680,0x801f57b0,0x80202ce8,
    0x801f67e8,0x801f7820,0x801f8858,0x801f9890,0x801fa8c8,0x801fb900}};
constexpr std::array<std::uint32_t,28> render_words{{
    0xe7000000,0,0xe3000800,0,0xd9ffffff,0x20000,0xe3000f00,0,
    0xe3000d01,0,0xe3000c00,0x80000,0xe3001001,0,0xe3001201,0x2000,
    0xe3001402,0xc00,0xdb040004,6,0xdb04000c,6,0xdb040014,0xfffa,
    0xdb04001c,0xfffa,0xdf000000,0}};

template<class F>
IsleGameResult with(IsleReportSource* source,F&& function) noexcept {
    return isle::post_course::invoke(source,[&](auto& owner) {
        if(std::get_if<IsleReportSource>(&owner.context)!=source || owner.kind!=Resources::Kind::report ||
           source->common.scene!=14 || !owner.native)
            return ISLE_GAME_INVALID_STATE;
        auto* state=std::get_if<ReportSceneNative>(&owner.live().menu);
        if(!state || (state->source && state->source!=source))return ISLE_GAME_INVALID_STATE;
        if(owner.failure!=ISLE_GAME_OK)return owner.failure;
        if(owner.cancelling)return ISLE_GAME_STOPPED;
        const auto result=function(owner,*state);
        return result==ISLE_GAME_OK?result:owner.fail(result,"Report14 returning native leaf");
    });
}

template<class O>
IsleGameResult admit(O& owner,ReportSceneNative& state) {
    auto& resources=*owner.live().resources;
    if(resources.kind()!=Resources::Kind::report || !owner.manager)return ISLE_GAME_INVALID_STATE;
    const auto generation=owner.manager->resources().overlay_generation(ISLE_APPLICATION_OVERLAY_POKEMON_REPORT);
    if(!generation || owner.manager->resources().overlay(ISLE_APPLICATION_OVERLAY_POKEMON_REPORT).bytes.empty())
        return ISLE_GAME_INVALID_STATE;
    if(state.admitted)return state.generation==generation?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    if(state.generation)return ISLE_GAME_INVALID_STATE;
    auto& data=state.data;
    for(const auto root:sprite_roots) {
        const auto index=resources.sprite_index(root);
        if(index>=resources.roots().size() || !resources.roots()[index])return ISLE_GAME_INVALID_STATE;
    }
    for(std::size_t i=0;i<render_words.size();++i)
        if(resources.word(0x801e3c10+std::uint32_t(i*4))!=render_words[i])return ISLE_GAME_INVALID_STATE;
    if(resources.word(0x80202d2c)!=0 || resources.word(0x80202d30)!=0xffffffffu)
        return ISLE_GAME_INVALID_STATE;
    data.comparator_origin=resources.word(0x80202d2c);
    data.D_80202D30_9F96A0=std::bit_cast<std::int32_t>(resources.word(0x80202d30));
    constexpr std::array<std::uint32_t,9> scalar_origins{{0x80202ec0,0x80202ec4,0x80202ff8,0x80202ffc,0x80203000,0x80203004,0x80203008,0x8020300c,0x80203014}};
    constexpr std::array<std::uint32_t,9> scalar_values{{0,0,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0,0,0}};
    const std::array<std::int32_t*,9> scalars{{&data.D_80202EC0_9F9830,&data.D_80202EC4_9F9834,
        &data.D_80202FF8_9F9968,&data.D_80202FFC_9F996C,&data.D_80203000_9F9970,
        &data.D_80203004_9F9974,&data.D_80203008_9F9978,&data.D_8020300C_9F997C,&data.D_80203014_9F9984}};
    for(std::size_t i=0;i<scalars.size();++i) {
        const auto value=resources.word(scalar_origins[i]);
        if(value!=scalar_values[i])return ISLE_GAME_INVALID_STATE;
        *scalars[i]=std::bit_cast<std::int32_t>(value);
    }
    const auto pose_bytes=resources.bytes(0x80202d4c,poses.size()*2);
    for(std::size_t i=0;i<poses.size();++i) {
        const auto value=std::bit_cast<std::int16_t>(std::uint16_t((std::uint16_t(pose_bytes[i*2])<<8)|pose_bytes[i*2+1]));
        if(value!=poses[i])return ISLE_GAME_INVALID_STATE;
        data.poses[i]=value;
    }
    for(std::size_t i=0;i<6;++i) {
        if(resources.word(0x80202d34+std::uint32_t(i*4))!=std::uint32_t(sign_ids[i]) ||
           resources.word(0x80202dd0+std::uint32_t(i*4))!=sign_sprites[i] ||
           resources.word(0x80202de8+std::uint32_t(i*4))!=sign_pixels[i])return ISLE_GAME_INVALID_STATE;
        const auto pixels=resources.bytes(sign_pixels[i],0xfd8);
        const auto& sprite=*resources.roots()[resources.sprite_index(sign_sprites[i])];
        if(sprite.width!=52 || sprite.height!=39 || sprite.bmfmt!=0 || sprite.bmsiz!=2 ||
           sprite.nbitmaps!=1 || !sprite.bitmap || sprite.bitmap[0].buf!=pixels.data() ||
           sprite.bitmap[0].byte_count!=pixels.size() || sprite.bitmap[0].width_img!=52 ||
           sprite.bitmap[0].actualHeight!=39 || sprite.LUT || sprite.LUT_bytes || sprite.nTLUT)
            return ISLE_GAME_INVALID_STATE;
        data.sign_ids[i]=sign_ids[i];data.sign_sprites[i]=sign_sprites[i];
        data.sign_pixels[i]=pixels.data();data.sign_pixel_sizes[i]=pixels.size();
    }
    /* Two Original arrays of six, not one unbounded merged table. Every NULL
     * written below is justified by an actual initialized zero word INSIDE
     * that list's admitted byte bound; next-table proximity is not evidence. */
    for(std::size_t table=0;table<2;++table) {
        const auto origin=table?0x80202ea4u:0x80202e8cu;
        if(resources.table_size(origin)!=6)return ISLE_GAME_INVALID_STATE;
        for(std::size_t sign=0;sign<6;++sign) {
            const auto i=table*6+sign;
            if(resources.table_word(origin,sign)!=caption_lists[i])return ISLE_GAME_INVALID_STATE;
            const auto words=i==4?std::size_t{2}:std::size_t{3};
            (void)resources.bytes(caption_lists[i],words*4);
            bool terminated=false;
            for(std::size_t j=0;j<words;++j) {
                const auto address=resources.word(caption_lists[i]+std::uint32_t(j*4));
                if(address!=caption_words[i][j])return ISLE_GAME_INVALID_STATE;
                if(!address) {
                    if(j+1!=words)return ISLE_GAME_INVALID_STATE;
                    state.captions[i][j]=nullptr;data.caption_count[i]=j;terminated=true;break;
                }
                const auto text=resources.text(address);
                const auto bytes=resources.bytes(address,text.size()+1);
                if(bytes.data()!=reinterpret_cast<const std::uint8_t*>(text.data()) || bytes.back()!=0 ||
                   std::memchr(bytes.data(),0,text.size()))return ISLE_GAME_INVALID_STATE;
                state.captions[i][j]=text.data();
            }
            if(!terminated)return ISLE_GAME_INVALID_STATE;
            data.captions[i]=state.captions[i].data();
        }
    }
    for(std::size_t i=0;i<button_tables.size();++i) {
        const auto decoded=resources.buttons(button_tables[i]);
        if(decoded.size()!=button_counts[i])return ISLE_GAME_INVALID_STATE;
        for(std::size_t j=0;j<decoded.size();++j) {
            const auto address=button_tables[i]+std::uint32_t(j*8);
            if(resources.word(address)!=std::uint32_t(button_ids[i][j]) || resources.word(address+4)!=0 ||
               decoded[j].id!=button_ids[i][j] || !decoded[j].text.empty())return ISLE_GAME_INVALID_STATE;
            state.buttons[i][j]={decoded[j].id,{nullptr,0}};
        }
        data.buttons[i]=state.buttons[i].data();data.button_count[i]=decoded.size();
    }
    state.source=std::get_if<IsleReportSource>(&owner.context);
    state.generation=generation;data.generation=generation;
    state.graphics={&state,nullptr,generation};state.admitted=true;
    return ISLE_GAME_OK;
}

ReportSceneNative::Destination* destination(ReportSceneNative& state,IsleGameSprite* sprite) noexcept {
    for(auto& item:state.destinations)if(item.sprite==sprite && sprite && item.thumbnail.serial)return &item;
    return nullptr;
}
IsleGameResult release_destination(IslePostCourseWindow* window,ReportSceneNative::Destination& item) {
    if(item.thumbnail.serial) {
        const auto result=isle_post_window_thumbnail_release(window,item.thumbnail);
        if(result!=ISLE_GAME_OK)return result;
    }
    item={};return ISLE_GAME_OK;
}
IsleGameResult prepare_data(IsleReportSource* source,const IsleReportData** output) {
    if(output)*output=nullptr;
    if(!output)return ISLE_GAME_INVALID_ARGUMENT;
    return with(source,[&](auto& owner,auto& state) {
        const auto result=admit(owner,state);
        if(result==ISLE_GAME_OK)*output=&state.data;
        return result;
    });
}
IsleGameResult ui_heap(IsleReportSource* source,std::size_t bytes) {
    return with(source,[&](auto& owner,auto&) {return isle::post_course::admit_heap(owner,bytes,0x2d000);});
}
IsleGameResult input_initialize(IsleReportSource* source) {
    return with(source,[](auto& owner,auto&) {return isle::post_course::initialize_input(owner);});
}
IsleGameResult input(IsleReportSource* source,std::int32_t controller,IsleReportInput* output) {
    if(output)*output={};if(!output)return ISLE_GAME_INVALID_ARGUMENT;
    return with(source,[&](auto& owner,auto&) {
        if(!owner.live().input_initialized)return ISLE_GAME_INVALID_STATE;
        const auto value=owner.input.menu(controller);
        if(value.error!=game_input::Error::none || !value.input)return ISLE_GAME_INVALID_STATE;
        *output={value.input->current,value.input->pressed};return ISLE_GAME_OK;
    });
}
IsleGameResult repeat(IsleReportSource* source,std::int32_t delay,std::int32_t interval) {
    return with(source,[&](auto& owner,auto&) {return owner.input.set_menu_repeat(delay,interval)==game_input::Error::none?ISLE_GAME_OK:ISLE_GAME_INVALID_ARGUMENT;});
}
IsleGameResult input_mask(IsleReportSource* source,std::uint32_t mask) {
    return with(source,[&](auto& owner,auto&) {return owner.input.set_menu_mask(mask)==game_input::Error::none?ISLE_GAME_OK:ISLE_GAME_INVALID_ARGUMENT;});
}
IsleGameResult camera(IsleReportSource* source,IsleGameObject** output) {
    if(output)*output=nullptr;if(!output)return ISLE_GAME_INVALID_ARGUMENT;
    return with(source,[&](auto& owner,auto&) {return isle::post_course::create_main_camera(owner,true,output);});
}
IsleGameResult aux_camera(IsleReportSource* source,std::int32_t link,std::int32_t mode) {
    return with(source,[&](auto& owner,auto&) {
        if(link!=1 || mode!=0x101)return ISLE_GAME_INVALID_ARGUMENT;
        auto& live=owner.live();auto* transition=live.services->screen_transition_source();
        if(!transition || transition->camera_object)return ISLE_GAME_INVALID_STATE;
        auto result=isle_screen_transition_camera(transition,link,mode);
        if(result!=ISLE_GAME_OK)return result;
        auto* view=transition->D_800BE2C8;
        result=isle::post_course::viewport(owner,view,320,240);if(result!=ISLE_GAME_OK)return result;
        const IsleGameCameraParameters* existing=nullptr;
        result=isle_game_camera_parameters(owner.source().game,view,&existing);if(result!=ISLE_GAME_OK)return result;
        auto perspective=existing->perspective;
        perspective.fovy=45;perspective.near_plane=8;perspective.far_plane=1024;perspective.scale=1;
        result=isle_game_camera_set_perspective(owner.source().game,view,&perspective);if(result!=ISLE_GAME_OK)return result;
        const IsleGameLookAt pose{{290,0,0},{0,0,0},{0,0,1}};
        result=isle_game_camera_set_look_at(owner.source().game,view,&pose);if(result!=ISLE_GAME_OK)return result;
        const IsleGameCameraRenderState render{4,0x10001,0};
        result=isle_game_camera_set_render_state(owner.source().game,view,&render);if(result!=ISLE_GAME_OK)return result;
        transition->D_800BE2C0=link;transition->camera_flags=4;
        transition->camera_mask=transition->camera_buffer_mask=2;
        transition->D_800AF054=transition->D_800AF058=transition->D_800AF05C=transition->D_800AF050=0;
        live.source_objects.push_back(transition->camera_object);
        live.add_camera(transition->camera_object,view,1,2,2);return ISLE_GAME_OK;
    });
}
IsleGameResult process_object(IsleReportSource* source,std::int32_t link,std::int32_t draw_link,void* list,IsleGameObject** output) {
    if(output)*output=nullptr;if(!output)return ISLE_GAME_INVALID_ARGUMENT;
    return with(source,[&](auto& owner,auto&) {return isle::post_course::process_object(owner,link,draw_link,list,output);});
}
IsleGameResult replay_initialize(IsleReportSource* source) {
    return with(source,[](auto& owner,auto&) {return owner.initialize_replay();});
}
IsleGameResult sound(IsleReportSource* source,std::int32_t id) {
    return with(source,[&](auto& owner,auto&) {return isle::post_course::sound(owner,id,0x7fff,64,1.0f,0);});
}
IsleGameResult song(IsleReportSource* source,std::int32_t player,std::int32_t id) {
    return with(source,[&](auto& owner,auto&) {return isle::post_course::song(owner,player,id);});
}
IsleGameResult volume(IsleReportSource* source,std::int32_t player,std::int32_t value) {
    return with(source,[&](auto& owner,auto&) {return isle::post_course::volume(owner,player,value,0);});
}
IsleGameResult volume_smooth(IsleReportSource* source,std::int32_t player,std::int32_t value,std::int32_t frames) {
    return with(source,[&](auto& owner,auto&) {return isle::post_course::volume(owner,player,value,frames);});
}
IsleGameResult player_flag(IsleReportSource* source,std::int32_t id,std::int32_t* output) {
    if(output)*output=0;if(!output)return ISLE_GAME_INVALID_ARGUMENT;
    return with(source,[&](auto& owner,auto&) {return isle::post_course::player_flag(owner,id,*output);});
}
IsleGameResult player_name(IsleReportSource* source,const char** output) {
    if(output)*output=nullptr;if(!output)return ISLE_GAME_INVALID_ARGUMENT;
    return with(source,[&](auto& owner,auto&) {return isle::post_course::player_name(owner,*output);});
}
IsleGameResult pokemon_name(IsleReportSource* source,std::int32_t id,const char** output) {
    if(output)*output=nullptr;if(!output)return ISLE_GAME_INVALID_ARGUMENT;
    return with(source,[&](auto& owner,auto&) {return isle::post_course::pokemon_name(owner,id,*output);});
}
IsleGameResult level_name(IsleReportSource* source,std::int32_t id,const char** output) {
    if(output)*output=nullptr;if(!output)return ISLE_GAME_INVALID_ARGUMENT;
    return with(source,[&](auto& owner,auto&) {return isle::post_course::course_name(owner,id,*output);});
}
IsleGameResult selected_course(IsleReportSource* source,std::int32_t* output) {
    if(output)*output=0;if(!output)return ISLE_GAME_INVALID_ARGUMENT;
    return with(source,[&](auto& owner,auto&) {
        const auto saved=owner.photos.save_session().snapshot();if(!saved)return ISLE_GAME_INVALID_STATE;
        const auto value=saved.value.selected_course();if(!value)return ISLE_GAME_INVALID_STATE;
        *output=static_cast<std::int32_t>(value.value);return ISLE_GAME_OK;
    });
}
IsleGameResult pokemon_index(IsleReportSource* source,std::int32_t id,std::int32_t* output) {
    if(output)*output=0;if(!output)return ISLE_GAME_INVALID_ARGUMENT;
    return with(source,[&](auto& owner,auto&) {
        auto& player=owner.photos.source();if(player.status!=ISLE_GAME_OK)return player.status;
        const auto value=isle_player_photo_func_8009BB4C(&player,id);
        if(player.status!=ISLE_GAME_OK)return player.status;
        *output=value;return ISLE_GAME_OK;
    });
}
IsleGameResult scene_result(IsleReportSource* source,std::int32_t result) {
    return with(source,[&](auto& owner,auto&) {return owner.request_exit(result);});
}
IsleGameResult sprite_add(IsleReportSource* source,IsleGameObject* object,std::uint32_t prototype,IsleGameSprite** output) {
    if(output)*output=nullptr;if(!output)return ISLE_GAME_INVALID_ARGUMENT;
    return with(source,[&](auto&,auto&) {return isle_post_window_sprite_add(source->common.window,object,prototype,output);});
}
IsleGameResult sprite_remove(IsleReportSource* source,IsleGameObject* object) {
    return with(source,[&](auto&,auto& state) {
        /* Remove only this object's bindings, after real display retirement.
         * The registered six sign owners and detail owner otherwise stay live. */
        for(auto& item:state.destinations)if(item.sprite) {
            IsleGameObject* parent=nullptr;IsleGameSprite *a=nullptr,*b=nullptr,*c=nullptr,*d=nullptr;
            auto result=isle_post_window_sprite_links(source->common.window,item.sprite,&parent,&a,&b,&c,&d);
            if(result!=ISLE_GAME_OK)return result;
            if(parent==object) {result=release_destination(source->common.window,item);if(result!=ISLE_GAME_OK)return result;}
        }
        return isle_post_window_sprite_remove(source->common.window,object);
    });
}
IsleGameResult sprite_detach(IsleReportSource* source,IsleGameSprite* sprite,std::int32_t which) {
    return with(source,[&](auto&,auto&) {return isle_post_window_sprite_clear_link(source->common.window,sprite,which);});
}
IsleGameResult sprite_bitmap(IsleReportSource* source,IsleGameSprite* sprite,const std::uint8_t* pixels,std::size_t size) {
    return with(source,[&](auto& owner,auto& state) {
        if(size!=0xfd8 || !pixels)return ISLE_GAME_INVALID_ARGUMENT;
        std::size_t index=0;while(index<6 && state.data.sign_pixels[index]!=pixels)++index;
        if(index==6)return ISLE_GAME_INVALID_ARGUMENT;
        IsleSprite* current=nullptr;
        auto result=isle_post_window_sprite_view(source->common.window,sprite,&current);if(result!=ISLE_GAME_OK)return result;
        const auto resource=owner.live().resources->sprite_index(sign_sprites[index]);
        const auto& prototype=*owner.live().resources->roots()[resource];
        if(!current || current->bitmap!=prototype.bitmap || prototype.bitmap[0].buf!=pixels ||
           prototype.bitmap[0].byte_count!=size)return ISLE_GAME_INVALID_STATE;
        /* The immutable first bitmap already names the exact admitted Source
         * span. Re-publish the actual native descriptor/resource, not a copied
         * bitmap structure with an arbitrary external pointer. */
        return owner.live().services->replace_sprite(sprite,resource,*current)?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    });
}
IsleGameResult thumbnail_bind(IsleReportSource* source,std::int32_t sign,IsleGameSprite* sprite,
    IslePostCourseThumbnail* output,std::uint8_t** pixels,std::size_t* size) {
    if(output)*output={};if(pixels)*pixels=nullptr;if(size)*size=0;
    if(!output || !pixels || !size)return ISLE_GAME_INVALID_ARGUMENT;
    return with(source,[&](auto&,auto& state) {
        if(!state.admitted || sign<0 || sign>=6 || !sprite)return ISLE_GAME_INVALID_ARGUMENT;
        auto& item=state.destinations[std::size_t(sign)];
        if(item.thumbnail.serial || item.sprite || destination(state,sprite))return ISLE_GAME_INVALID_STATE;
        auto result=isle_post_window_thumbnail_create(source->common.window,sprite,sign_sprites[std::size_t(sign)],&item.thumbnail);
        if(result!=ISLE_GAME_OK)return result;
        item.sprite=sprite;item.prototype=sign_sprites[std::size_t(sign)];
        std::uint8_t* borrowed=nullptr;std::size_t extent=0;
        result=isle_post_window_thumbnail_pixels(source->common.window,item.thumbnail,&borrowed,&extent);
        if(result!=ISLE_GAME_OK)return result;
        if(!borrowed || extent!=0xfd8)return ISLE_GAME_INVALID_STATE;
        *output=item.thumbnail;*pixels=borrowed;*size=extent;return ISLE_GAME_OK;
    });
}
IsleGameResult preview(IsleReportSource* source,const IslePlayerPhotoData* photo,IsleGameSprite* sprite) {
    return with(source,[&](auto& owner,auto& state) {
        if(!source->common.replay || !state.admitted || !sprite)return ISLE_GAME_INVALID_STATE;
        if(photo) {const auto identity=owner.photos.identify(photo);if(!identity || identity.value.photo!=photo)return ISLE_GAME_INVALID_ARGUMENT;}
        if(auto* item=destination(state,sprite)) {
            IsleSprite* view=nullptr;
            const auto result=isle_post_window_sprite_view(source->common.window,sprite,&view);
            if(result!=ISLE_GAME_OK)return result;
            const auto expected=item->prototype==0x80202ce8u?140:52;
            if(!view || view->width!=expected || view->height!=(expected==140?105:39))
                return ISLE_GAME_INVALID_STATE;
            return ISLE_GAME_OK;
        }
        /* Only the one actual full-detail child is admitted here. The six
         * sign children were independently bound at their Source allocations. */
        if(source->D_80230C14_A27584!=sprite)return ISLE_GAME_INVALID_ARGUMENT;
        auto& item=state.destinations[6];if(item.thumbnail.serial || item.sprite)return ISLE_GAME_INVALID_STATE;
        auto result=isle_post_window_thumbnail_create(source->common.window,sprite,0x80202ce8,&item.thumbnail);
        if(result!=ISLE_GAME_OK)return result;
        item.sprite=sprite;item.prototype=0x80202ce8;return ISLE_GAME_OK;
    });
}
IsleGameResult window_mode(IsleReportSource* source,std::int32_t flags,std::int32_t set) {
    return with(source,[&](auto&,auto&) {
        if(flags!=1 || (set!=0 && set!=1))return ISLE_GAME_INVALID_ARGUMENT;
        return isle_post_window_text_proportional(source->common.window,set);
    });
}
IsleGameResult render_bind(IsleReportSource* source,IsleReportGraphics** output) {
    /* An in/out command cursor is cleared on failure, never on success. */
    auto* cursor=output?*output:nullptr;if(output)*output=nullptr;
    if(!output)return ISLE_GAME_INVALID_ARGUMENT;
    return with(source,[&](auto&,auto& state) {
        if(cursor!=&state.graphics || cursor->allocation!=&state || !cursor->control ||
           cursor->generation!=state.generation)return ISLE_GAME_INVALID_STATE;
        cursor->control->call({0x001e3c10,render_words.size()});
        *output=cursor;return ISLE_GAME_OK;
    });
}
IsleGameResult render_state(IsleReportSource* source,std::int32_t,std::int32_t) {
    return with(source,[&](auto&,auto& state) {
        auto& cursor=state.graphics;
        if(cursor.allocation!=&state || !cursor.control || cursor.generation!=state.generation)
            return ISLE_GAME_INVALID_STATE;
        auto& control=*cursor.control;
        control.call({0x001e3c10,render_words.size()});
        control.command(0xe3001801,0); // G_CD_MAGICSQ
        control.command(0xe7000000,0);
        control.command(0xe200001c,0x0c4c2078); // exact Source flags/blender + NOOP2
        control.command(0xfc1219ff,0xfffffe38); // MODULATEIA / PASS2
        return ISLE_GAME_OK;
    });
}
IsleGameResult setup_begin(IsleReportSource* source,std::int32_t argument) {
    return isle::post_course::invoke(source,[&](auto& owner) {
        if(std::get_if<IsleReportSource>(&owner.context)!=source || owner.kind!=Resources::Kind::report || argument!=14)
            return ISLE_GAME_INVALID_ARGUMENT;
        return owner.setup_begin(argument,0x801e3c80,0x801e3c9c,0x16a99,1,1,1);
    });
}
IsleGameResult setup_poll(void* raw,int* retired,std::int32_t* output) {
    if(retired)*retired=0;if(output)*output=0;if(!retired || !output)return ISLE_GAME_INVALID_ARGUMENT;
    int complete=0;std::int32_t result=0;
    const auto status=SceneAccess::invoke_owner(raw,[&](auto& owner) {
        if(owner.kind!=Resources::Kind::report || !std::holds_alternative<IsleReportSource>(owner.context))return ISLE_GAME_INVALID_STATE;
        return owner.setup_poll(&complete,&result);
    });
    if(status==ISLE_GAME_OK){*retired=complete;*output=result;}return status;
}
IsleGameResult notices_or_owner(void* raw,std::uint32_t bits) {
    return SceneAccess::invoke_owner(raw,[&](auto& owner) {
        if(owner.kind!=Resources::Kind::report || bits!=0x200000 || !owner.setup_returned ||
           owner.cancelling || owner.failure!=ISLE_GAME_OK)return owner.failure!=ISLE_GAME_OK?owner.failure:owner.cancelling?ISLE_GAME_STOPPED:ISLE_GAME_INVALID_STATE;
        owner.state.add_scene_notices(bits);return ISLE_GAME_OK;
    });
}
const IsleReportNative table{
    prepare_data,ui_heap,input_initialize,input,repeat,input_mask,camera,aux_camera,process_object,
    replay_initialize,sound,song,volume,volume_smooth,player_flag,player_name,pokemon_name,level_name,
    selected_course,pokemon_index,scene_result,sprite_add,sprite_remove,sprite_detach,sprite_bitmap,
    thumbnail_bind,preview,window_mode,render_bind,render_state,setup_begin,setup_poll,notices_or_owner};

bool control_dispatch(void* raw,scene::graphics::SourceControl& control,bool state_commands) noexcept {
    if(!raw)return false;
    auto& state=*static_cast<ReportSceneNative*>(raw);
    const auto result=with(state.source,[&](auto&,auto& owned) {
        if(&owned!=&state || !state.admitted || state.graphics.control)return ISLE_GAME_INVALID_STATE;
        state.graphics.control=&control;
        struct Clear {IsleReportGraphics& cursor;~Clear(){cursor.control=nullptr;}} clear{state.graphics};
        control.initialize_camera();
        IsleGameResult result;
        if(state_commands)result=render_state(state.source,0,0);
        else {auto* cursor=&state.graphics;result=render_bind(state.source,&cursor);}
        if(result!=ISLE_GAME_OK)return result;
        control.draw();return ISLE_GAME_OK;
    });
    return result==ISLE_GAME_OK;
}
}
ReportSceneNative::ReportSceneNative(application::Photos&) noexcept {}
const IsleReportNative& report_scene_native_table() noexcept {return table;}
IsleGameResult report_scene_native_prepare(IsleReportSource* source,ReportSceneNative& state) noexcept {
    return with(source,[&](auto& owner,auto& owned) {
        if(&owned!=&state || source->native!=&table)return ISLE_GAME_INVALID_STATE;
        return admit(owner,state);
    });
}
bool report_scene_native_render_bind_control(void* state,scene::graphics::SourceControl& control) noexcept {
    return control_dispatch(state,control,false);
}
bool report_scene_native_render_state_control(void* state,scene::graphics::SourceControl& control) noexcept {
    return control_dispatch(state,control,true);
}
}
extern "C" IsleGameResult isle_report_native_preview_consume(IsleReportSource* source,IsleGameSprite* sprite,const IslePostCoursePreview* image) noexcept {
    return isle::post_course::with(source,[&](auto&,auto& state) {
        auto* item=isle::post_course::destination(state,sprite);
        if(!item)return ISLE_GAME_INVALID_ARGUMENT;
        if(!image)return isle_post_window_thumbnail_clear(source->common.window,item->thumbnail);
        const IsleSprite* view=nullptr;
        auto result=isle::post_course::Window::observe_sprite(source->common.window,sprite,&view);
        if(result!=ISLE_GAME_OK)return result;
        if(!view || image->width!=std::uint32_t(view->width)*2 || image->height!=std::uint32_t(view->height)*2)
            return ISLE_GAME_INVALID_STATE;
        return isle_post_window_thumbnail_write_photo(source->common.window,item->thumbnail,image->rgba5551,image->size,image->stride_bytes);
    });
}
extern "C" IsleGameResult isle_report_native_sprite_rebind(IsleReportSource* source,IsleGameSprite* sprite,std::uint32_t prototype) noexcept {
    return isle::post_course::with(source,[&](auto&,auto& state) {
        if(auto* item=isle::post_course::destination(state,sprite))
            return isle_post_window_thumbnail_select(source->common.window,item->thumbnail,prototype);
        return isle_post_window_sprite_rebind(source->common.window,sprite,prototype);
    });
}
extern "C" IsleGameResult isle_report_native_thumbnail_publish(IsleReportSource* source,std::int32_t sign) noexcept {
    return isle::post_course::with(source,[&](auto&,auto& state) {
        if(sign<0 || sign>=6)return ISLE_GAME_INVALID_ARGUMENT;
        const auto& item=state.destinations[std::size_t(sign)];
        if(!item.thumbnail.serial || !item.sprite || source->D_80230DC8_A27738[sign].unk_04!=item.sprite ||
           source->D_80230DC8_A27738[sign].thumbnail.serial!=item.thumbnail.serial ||
           source->D_80230DC8_A27738[sign].thumbnail.owner!=item.thumbnail.owner)return ISLE_GAME_INVALID_STATE;
        return isle_post_window_thumbnail_publish(source->common.window,item.thumbnail,0,0xfd8);
    });
}
extern "C" IsleGameResult isle_report_native_preview_scale(IsleReportSource* source,float* output) noexcept {
    if(output)*output=0;
    if(!output)return ISLE_GAME_INVALID_ARGUMENT;
    return isle::post_course::with(source,[&](auto& owner,auto&) {
        const auto scale=owner.modules.preview_scale;
        if(!std::isfinite(scale) || scale<=0)return ISLE_GAME_INVALID_STATE;
        *output=scale;return ISLE_GAME_OK;
    });
}

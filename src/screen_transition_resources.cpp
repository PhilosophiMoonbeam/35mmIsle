#include "screen_transition_resources.hpp"
#include "graphics_task.hpp"
#include "menu_scene_bridge.h"
#include "rom_view.hpp"
#include <stdexcept>
namespace isle::screen_transition {
Resources::Resources(const rom::Snapshot& snapshot) {
    const auto bytes=snapshot.canonical().subspan(0x5a620,0x3e0);
    const rom::GuestView solid{0x800aecb0,0x5a660,bytes.subspan(0x40,0x30)};
    const rom::GuestView round{0x800aeee0,0x5a890,bytes.subspan(0x270,0xa0)};
    const rom::GuestView shape{0x800af020,0x5a9d0,bytes.subspan(0x3b0,0x30)};
    if (solid.u32(24)!=0x01004008 || solid.u32(28)!=0x800aec70 ||
        solid.u32(40)!=0xdf000000 || round.u32(24)!=0x01020040 ||
        round.u32(28)!=0x800aece0 || shape.u32(0)!=0x0100a014 ||
        shape.u32(4)!=0x800aef80 || shape.u32(40)!=0xdf000000)
        throw std::invalid_argument("admitted transition GBI closure differs");
    assets_[0]={0xaec70,{reinterpret_cast<const uint8_t*>(bytes.data()),bytes.size()}};
}
render::Pass Resources::pass(Owner& owner) noexcept {
    render::Pass result;
    auto& s=owner.source;
    result.camera=s.D_800BE2C8;
    result.priority=1;
    result.flags=s.camera_flags;
    result.background_rgba=0x10001;
    result.link_mask=s.camera_mask;
    result.camera_tag=s.camera_buffer_mask;
    result.prepare=prepare;
    result.userdata=&owner;
    result.transition_owner=&owner;
    return result;
}
bool Resources::prepare(void* user,std::span<uint32_t> ram,uint64_t,
    const render::Pass& pass,render::Prepared& output) {
    auto& owner=*static_cast<Owner*>(user);
    auto& s=owner.source;
    if (s.status!=ISLE_GAME_OK || s.D_800BE2C8!=pass.camera) return false;
    auto check=[&](IsleGameResult r) { if (r!=ISLE_GAME_OK && s.status==ISLE_GAME_OK) s.status=r; return r==ISLE_GAME_OK; };
    if (!check(isle_game_camera_compute_all(s.game,pass.camera))) return false;
    const IsleGameCameraMatrix *projection=nullptr,*view=nullptr;
    uint16_t normalization=0;
    if (!check(isle_game_camera_matrix(s.game,pass.camera,0,&projection)) ||
        !check(isle_game_camera_matrix(s.game,pass.camera,1,&view)) ||
        !check(isle_game_camera_perspective_normalization(s.game,pass.camera,&normalization))) return false;
    graphics::Memory memory(ram);
    constexpr uint32_t projection_at=workspace.address,view_at=projection_at+64;
    constexpr uint32_t commands_at=workspace.address+0x100;
    uint32_t cursor=commands_at, matrix=workspace.address+0x8000;
    auto cmd=[&](uint32_t a,uint32_t b) {
        if (cursor>workspace.address+0x8000-8) throw std::length_error("transition source command storage exhausted");
        memory.word(cursor,a);memory.word(cursor+4,b);cursor+=8;
    };
    memory.matrix(projection_at,projection->fixed);memory.matrix(view_at,view->fixed);
    cmd(0xda380007,projection_at);cmd(0xdb0e0000,normalization);cmd(0xda380005,view_at);
    for (auto* r=owner.objects;r;r=r->next) {
        if (r->joined) continue;
        scene::RenderMetadata metadata;
        if (!owner.services.render_metadata(r->object,metadata)) return false;
        if (!pass.includes(metadata)) continue;
        const IsleGameModelNode* nodes=nullptr;size_t count=0;
        if (!check(isle_game_model_nodes(s.game,r->model,&nodes,&count)) || count!=1) return false;
        if (matrix>workspace.address+workspace.size-128) return false;
        IsleGameFixedMatrix scale;
        if (!check(isle_menu_scale_matrix(&nodes[0].transform.scale,&scale))) return false;
        memory.matrix(matrix,nodes[0].fixed);memory.matrix(matrix+64,scale);
        // Source model's independent translation and scale: no kind28 quantization.
        cmd(0xe7000000,0);cmd(0xda380000,matrix);cmd(0xda380001,matrix+64);
        IsleScreenTransitionRender render{};
        const auto result=r->render_kind==0 ? isle_screen_transition_func_800A750C(&s,r,&render) :
            r->render_kind==1 ? isle_screen_transition_func_800A7948(&s,r,&render) :
                               isle_screen_transition_func_800A7A58(&s,r,&render);
        if (!check(result)) return false;
        cmd(0xfa000000,render.primitive_rgba);
        cmd(0xe200001c,render.render_mode);
        cmd(0xde000000,render.display_list);cmd(0xd8380002,64);
        matrix+=128;
    }
    cmd(0xdf000000,0);
    output.lanes[1]={commands_at,(cursor-commands_at)/4};
    return true;
}
}

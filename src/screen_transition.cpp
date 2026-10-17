#include "screen_transition.hpp"
#include "scene_services.hpp"
#include "menu_scene_bridge.h"
#include <cmath>
#include <exception>
#include <new>
namespace isle::screen_transition {
Owner::Owner(scene::Services& s) noexcept : services(s) { source.game=s.game; source.owner=this; }
Owner::~Owner() {
    if (borrows_) std::terminate();
    for (auto* r=objects;r;) {
        auto* next=r->next;
        if (!r->joined) std::terminate();
        delete r;
        r=next;
    }
}
const IsleScreenTransitionObject* Owner::solid() const noexcept {
    for (auto* r=objects;r;r=r->next) if (!r->joined && r->render_kind==0) return r;
    return nullptr;
}
bool Owner::borrow() noexcept { if (borrows_==SIZE_MAX) return false; ++borrows_; return true; }
void Owner::release() noexcept { if (!borrows_) std::terminate(); --borrows_; }
bool Owner::retirement_ready() const noexcept {
    if (borrows_) return false;
    for (auto* r=objects;r;r=r->next) if (!r->joined) return false;
    return true;
}
static void joined(IsleGameObject*, void* user) noexcept {
    auto& r=*static_cast<IsleScreenTransitionObject*>(user);
    if (r.source->D_800BE2D0==r.object) r.source->D_800BE2D0=nullptr;
    r.joined=1;
}
}
extern "C" IsleScreenTransitionObject* isle_screen_transition_create(IsleScreenTransitionSource* s,
    IsleGameSourceCallback callback, uint32_t list) {
    if (!s || !s->owner || s->status!=ISLE_GAME_OK) return nullptr;
    auto& owner=*static_cast<isle::screen_transition::Owner*>(s->owner);
    auto& services=owner.services;
    auto* r=new(std::nothrow) IsleScreenTransitionObject{};
    if (!r) {s->status=ISLE_GAME_EXHAUSTED; return nullptr;}
    r->source=s; r->display_list=list; r->callback=callback;
    r->object=services.object(2,1,1);
    if (!r->object) {delete r;s->status=ISLE_GAME_EXHAUSTED;return nullptr;}
    auto** tail=&owner.objects;
    while (*tail) tail=&(*tail)->next;
    *tail=r;
    if (!services.set_lifecycle(r->object,{nullptr,isle::screen_transition::joined,r})) {
        s->status=ISLE_GAME_INVALID_STATE; return nullptr;
    }
    s->status=isle_menu_set_update(s->game,r->object,nullptr,nullptr);
    if (s->status!=ISLE_GAME_OK) return nullptr;
    const IsleGameModelNodeInit init{UINT32_MAX,{{0,0,0},{0,0,0},{1,1,1}},ISLE_GAME_MODEL_TRANSLATE};
    s->status=isle_game_attach_model(s->game,r->object,&init,1,&r->model);
    if (s->status!=ISLE_GAME_OK) return nullptr;
    s->status=isle_screen_transition_bind(r);
    if (s->status!=ISLE_GAME_OK) return nullptr;
    if (!services.set_render(r->object,{static_cast<uint8_t>(s->D_800BE2C0),1,static_cast<uint32_t>(1u<<s->D_800BE2C0)})) {
        s->status=ISLE_GAME_INVALID_STATE; return nullptr;
    }
    if (callback) {
        IsleGameProcess* process=nullptr;
        s->status=isle_game_add_source_process(s->game,r->object,callback,ISLE_GAME_STACKFUL,0,&process);
        if (s->status!=ISLE_GAME_OK) return nullptr;
    }
    return r;
}
extern "C" IsleGameResult isle_screen_transition_transform(IsleScreenTransitionObject* r,
    IsleGameModelTransform* pose,int write) {
    if (!r || !pose || r->joined) return ISLE_GAME_INVALID_ARGUMENT;
    if (write) return isle_game_model_set_transform(r->source->game,r->model,0,pose);
    const IsleGameModelNode* nodes=nullptr; size_t count=0;
    auto result=isle_game_model_peek_nodes(r->source->game,r->model,&nodes,&count);
    if (result!=ISLE_GAME_OK) return result;
    if (count!=1) return ISLE_GAME_INVALID_STATE;
    *pose=nodes[0].transform;
    return ISLE_GAME_OK;
}
extern "C" IsleGameResult isle_screen_transition_camera(IsleScreenTransitionSource* s,int32_t link,int32_t mode) {
    if (!s || !s->owner || link<0 || link>=32) return ISLE_GAME_INVALID_ARGUMENT;
    if (s->status!=ISLE_GAME_OK) return s->status;
    auto& services=static_cast<isle::screen_transition::Owner*>(s->owner)->services;
    // Native allocation/publication leaf only. Original C applies every camera
    // flag, viewport, eye/up/perspective reset in its original order.
    (void)mode;
    auto* object=services.object(5,0,0x80000000u);
    if (!object) return s->status=ISLE_GAME_EXHAUSTED;
    auto result=isle_menu_set_update(s->game,object,nullptr,nullptr);
    if (result!=ISLE_GAME_OK) return s->status=result;
    IsleGameCamera* camera=nullptr;
    result=isle_game_attach_camera(s->game,object,&camera);
    if (result!=ISLE_GAME_OK) return s->status=result;
    s->camera_object=object; s->D_800BE2C8=camera;
    if ((result=isle_game_camera_add_matrix(s->game,camera,ISLE_GAME_CAMERA_PERSPECTIVE_FAST))!=ISLE_GAME_OK ||
        (result=isle_game_camera_add_matrix(s->game,camera,ISLE_GAME_CAMERA_LOOK_AT))!=ISLE_GAME_OK)
        return s->status=result;
    return ISLE_GAME_OK;
}

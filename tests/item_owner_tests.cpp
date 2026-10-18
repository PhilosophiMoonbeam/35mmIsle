// CPU source-API contracts. These exercise the privately transformed actual C
// definitions, not an alternative Item simulation, resource fixture or Player.
#include "item_owner.hpp"
#include "scene_storage.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value,const char* text) { if (!value) throw std::runtime_error(text); }
void native(IsleGameResult value) { require(value==ISLE_GAME_OK,"native Item API operation failed"); }
void globals_and_setters() {
    isle::items::Globals retained;
    auto& g=retained.source;
    g.total=19; g.balls=7; g.apples=11; g.object_counter=17;
    g.update=Items_RemovePesterBall; g.update_kind=0; g.collide=isle_item_source_beach;
    retained.flute.song_index=2; retained.flute.playing=true; retained.flute.song_start_epoch=987;
    Items_Init(&g);
    require(g.total==0 && g.balls==7 && g.apples==11 && g.object_counter==17,
            "source Items_Init reset family counts or persistent ID cursor");
    require(g.update==Items_RemovePesterBall && g.update_kind==0 && g.collide==isle_item_source_beach,
            "source Items_Init reset callback state");
    require(retained.flute.song_index==2 && retained.flute.playing && retained.flute.song_start_epoch==987,
            "Item initialization reset caller-retained flute");
    require(!g.allocated_head && !g.allocated_tail && g.free_head==&g.entries[0],"source logical heads differ");
    for (unsigned i=0;i<20;++i) {
        require(g.entries[i].index==static_cast<int>(i) && !g.entries[i].obj,"source entry/index initialization differs");
        require(g.entries[i].prev==(i?&g.entries[i-1]:nullptr) &&
                g.entries[i].next==(i==19?nullptr:&g.entries[i+1]),"source initial ascending free chain differs");
    }
    Items_SetCustomFunctions(&g,Items_ShowDelayed,0,nullptr);
    require(g.update==Items_ShowDelayed && g.update_kind==0 && g.collide==isle_item_source_beach,
            "nonnull update setter changed collision hook");
    Items_SetCustomFunctions(&g,nullptr,0,nullptr);
    require(g.update==Items_UpdateItemMovement && g.update_kind==1 && g.collide==isle_item_source_beach,
            "NULL update failed to reinstate kind1 movement or erased prior collision hook");
}
void actual_native_array_iteration() {
    isle::scene::Storage storage(1024*1024); native(storage.create());
    IsleItemGlobals g{}; Items_Init(&g); g.game=storage.game();
    native(Items_func_8035CA1C(&g,g.game,nullptr,nullptr));
    IsleGameObject *first=nullptr,*second=nullptr,*third=nullptr;
    native(isle_game_add_object(g.game,301,4,0x80000000u,nullptr,nullptr,&first));
    native(isle_game_add_object(g.game,302,4,0x80000000u,nullptr,nullptr,&second));
    native(isle_game_add_object(g.game,303,4,0x80000000u,nullptr,nullptr,&third));
    IsleItemSource a{},b{},c{};
    // Explicit native source-API fixture, not an Owner factory or Player FSM.
    native(isle_item_source_bind(g.game,first,&a,&g)); native(isle_item_source_bind(g.game,second,&b,&g));
    native(isle_item_source_bind(g.game,third,&c,&g));
    a.itemID=ISLE_ITEM_PESTER_BALL; a.state=ISLE_ITEM_FLYING;
    b.itemID=ISLE_ITEM_APPLE; b.state=ISLE_ITEM_STILL; c.state=ISLE_ITEM_INVALID;
    g.entries[9].obj=first; g.entries[2].obj=second; g.entries[0].obj=third;
    Items_InitIterator(&g);
    require(Items_NextValidItem(&g)==second && Items_NextValidItem(&g)==first && !Items_NextValidItem(&g),
            "valid iterator used native/list order, excluded balls, or included INVALID");
    Items_InitIterator(&g);
    require(Items_func_80359900(&g)==second && !Items_func_80359900(&g),"still iterator did not use source state equality");
    require(Items_CheckObjectExists(&g,third)==third,"membership was incorrectly filtered by INVALID state");
    // Cursor searches logical ARRAY occupancy, not real native IDs/link4. Native
    // ID301 is occupied but logical entry1 is free, so source must return301.
    g.object_counter=0; require(Items_GetFreeObjectID(&g)==301,"ID cursor was conflated with native object allocation");
    g.object_counter=19; require(Items_GetFreeObjectID(&g)==301 && g.object_counter==0,"ID cursor wrap/search differs");
    for (auto& entry:g.entries) entry.obj=first;
    require(Items_GetFreeObjectID(&g)==300,"full logical-array ID fallback is not BASE300");
    native(isle_game_object_retain_identity(g.game,first));
    native(isle_game_delete_object(g.game,first)); native(isle_game_delete_object(g.game,second));
    native(isle_game_delete_object(g.game,third));
    Items_InitIterator(&g); g.status=ISLE_GAME_OK;
    require(!Items_NextValidItem(&g) && g.status!=ISLE_GAME_OK,
            "retired canonical binding was mistaken for normal iterator exhaustion or dereferenced NULL");
    native(isle_game_object_release_identity(g.game,first));
    native(storage.destroy());
}
}
int main() {
    try { globals_and_setters(); actual_native_array_iteration(); return EXIT_SUCCESS; }
    catch (const std::exception& error) { std::cerr<<"item-owner-tests: "<<error.what()<<'\n'; return EXIT_FAILURE; }
}

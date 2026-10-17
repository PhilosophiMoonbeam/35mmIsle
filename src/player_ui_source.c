#include "player_ui_source.h"
#include "engine_internal.h"
void isle_player_ui_source_record(IslePlayerUiSource* ui,IsleGameResult result) {
    if(ui && result!=ISLE_GAME_OK && ui->status==ISLE_GAME_OK) ui->status=result;
}
IslePlayerUiSource* isle_player_ui_source_current(IsleGameObject* object) {
    IslePlayerSource* player=isle_player_source_current(object);
    if(!player || !player->ui) isle_native_execution_fault(object?object->game:NULL,"Player UI callback has no canonical context");
    return player->ui;
}
IsleGameObject* isle_player_ui_source_object(IslePlayerUiSource* ui,uint32_t id,IsleGameSourceCallback update) {
    IsleGameObject* object=NULL;
    isle_player_ui_source_record(ui,isle_player_ui_native_object(ui,id,update,&object));
    /* NULL represents source ohUpdateDefault; retain Services' command drain. */
    if(object && update) object->fnUpdate=update;
    return object;
}
IsleGameSprite* isle_player_ui_source_sprite(IslePlayerUiSource* ui,IsleGameObject* object,const IsleSprite* sprite) {
    IsleGameSprite* output=NULL;
    isle_player_ui_source_record(ui,isle_player_ui_native_sprite(ui,object,sprite,&output));
    return output;
}
void isle_player_ui_source_process(IslePlayerUiSource* ui,IsleGameObject* object,IsleGameSourceCallback entry,uint8_t kind,uint32_t priority) {
    IsleGameProcess* process=NULL;
    isle_player_ui_source_record(ui,isle_game_add_source_process(ui->player->game,object,entry,(IsleGameProcessKind)kind,priority,&process));
}
void isle_player_ui_source_pause(IslePlayerUiSource* ui,IsleGameObject* object,int paused) {
    GObjProcess* process;
    if(!object){isle_player_ui_source_record(ui,ISLE_GAME_INVALID_ARGUMENT);return;}
    for(process=object->processListHead;process;process=process->next)
        isle_player_ui_source_record(ui,isle_game_pause_process(ui->player->game,process,paused));
}
IsleGameSprite* isle_player_ui_source_find_sprite(IslePlayerUiSource* ui,const IsleSprite* view) {
    IsleGameObject* objects[]={ui->Icons_MainObject,ui->D_803B09D8_550DE8};
    unsigned i;
    for(i=0;i<sizeof(objects)/sizeof(objects[0]);++i) {
        SObj* sprite;
        if(!objects[i] || !objects[i]->alive || objects[i]->retirementStarted)continue;
        for(sprite=objects[i]->data.sobj;sprite;sprite=sprite->next)
            if(&sprite->sprite==view)return sprite;
    }
    return NULL;
}
IsleGameObject* isle_player_ui_source_find_id(IslePlayerUiSource* ui,uint32_t id) {
    unsigned link;
    for(link=0;link<ARRAY_COUNT(omGObjListHead);++link) {
        GObj* object;
        for(object=omGObjListHead[link];object;object=object->next)
            if(object->game==ui->player->game && object->alive && object->id==id)return object;
    }
    return NULL;
}

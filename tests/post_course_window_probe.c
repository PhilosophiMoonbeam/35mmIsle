#include "post_course_window_probe.h"
#include "engine_internal.h"

void isle_window_probe_frame_cycle(IsleGameObject*);

void isle_window_probe_bind(IsleGameObject* object,IsleWindowProbe* probe) { object->userData=probe; }
static IsleGameResult remember(IsleWindowProbe* p,uint16_t character) {
    if(p->count>=16)return ISLE_GAME_INVALID_STATE;
    p->characters[p->count]=character;p->character_beats[p->count]=p->beats;++p->count;
    return ISLE_GAME_OK;
}
static IsleGameResult replacement(void* user,uint16_t character) {
    IsleWindowProbe* p=user;++p->new_callbacks;return remember(p,character);
}
IsleGameResult isle_window_probe_character(void* user,uint16_t character) {
    IsleWindowProbe* p=user;
    IslePostCourseTextCallbacks callbacks={0};
    IsleGameResult result;
    ++p->old_callbacks;
    result=remember(p,character);
    if(result!=ISLE_GAME_OK)return result;
    if(p->mode!=ISLE_WINDOW_PROBE_PRINT)return ISLE_GAME_OK;
    ++p->callback_waits;
    p->callback_result=isle_game_source_wait(p->game,1);
    if(p->callback_result!=ISLE_GAME_OK)return p->callback_result;
    callbacks.user=p;callbacks.character=replacement;
    p->callback_result=isle_post_window_text_callbacks(p->window,&callbacks);
    return p->callback_result;
}
static void nested_print(IslePlayerSource* source,void* user) {
    IsleWindowProbe* p=user;
    p->zero_wait_result=isle_game_source_wait(source->game,0);
    p->result=isle_post_window_print(p->window,p->element,(IslePostCourseBytes){"ABC",3},0,NULL);
}
static IsleGameResult layout(IsleWindowProbe* p) {
    IsleGameResult result;
    int i,slot;
    for(i=0;i<9;++i) {
        result=isle_post_window_layout_slot_id(p->window,(size_t)i,&p->slots_before[i]);
        if(result!=ISLE_GAME_OK)return result;
    }
    result=isle_post_window_layout_create(p->window,p->table);
    if(result!=ISLE_GAME_OK)return result;
    result=isle_post_window_layout_wait(p->window);
    if(result!=ISLE_GAME_OK)return result;
    for(i=0;i<9;++i) {
        result=isle_post_window_layout_slot_id(p->window,(size_t)i,&p->slots_after[i]);
        if(result!=ISLE_GAME_OK)return result;
    }
    /* Original retained selector usage: every indexed read reacquires the live
     * canonical slot; never maintain/refresh an authoritative C slot array. The
     * arrays above are evidence observations only, not navigation inputs. */
    slot=0;
    for(i=0;i<3;++i) {
        int id,attempt;
        for(attempt=0;attempt<8;++attempt) {
            result=isle_post_window_layout_slot_id(p->window,(size_t)slot,&id);
            if(result!=ISLE_GAME_OK)return result;
            slot=(slot+1)%8;
            if(id!=35)break;
        }
        if(attempt==8)return ISLE_GAME_INVALID_STATE;
        p->actions[i]=id;
        {
            IslePostCourseBytes help={0};
            result=isle_post_window_table_text(p->window,p->table,id,&help);
            if(result!=ISLE_GAME_OK)return result;
            if(!help.data || !help.size)return ISLE_GAME_INVALID_STATE;
            p->help_sizes[i]=help.size;
        }
    }
    return ISLE_GAME_OK;
}
void isle_window_probe_main(IsleGameObject* object) {
    IsleWindowProbe* p=object->userData;
    if(p->entered)return;
    p->entered=1;
    if(p->mode==ISLE_WINDOW_PROBE_LAYOUT)p->result=layout(p);
    else if(p->mode==ISLE_WINDOW_PROBE_NESTED)
        p->nested_result=isle_player_source_call(p->player,nested_print,p);
    else p->result=isle_post_window_print(p->window,p->element,(IslePostCourseBytes){"ABC",3},0,NULL);
    p->returned=1;
}
void isle_window_probe_heartbeat(IsleGameObject* object) {
    IsleWindowProbe* p=object->userData;
    ++p->beats;
    if(p->aux_target) {
        int32_t value=0;
        p->aux_result=isle_post_window_sprite_aux_get(p->window,p->aux_target,&value);
        p->aux_observed=value;++p->aux_reads;
    }
}

/* The existing test record's element/table handles name successive Frames in
 * this case. All fades and waits remain in compiled C on the real Source
 * process; only evidence is written to the test-owned record. */
void isle_window_probe_frame_cycle(IsleGameObject* object) {
    IsleWindowProbe* p=object->userData;
    int cycle;
    if(p->entered)return;
    p->entered=1;
    p->result=isle_post_window_frame_create(p->window,&p->element);
    if(p->result!=ISLE_GAME_OK)goto done;
    for(cycle=0;cycle<3;++cycle) {
        p->result=isle_post_window_fade_in(p->window,p->element,3);
        if(p->result!=ISLE_GAME_OK)goto done;
        p->count=cycle*2+1;
        p->result=isle_game_source_wait(p->game,1);
        if(p->result!=ISLE_GAME_OK)goto done;
        p->result=isle_post_window_fade_out(p->window,p->element);
        if(p->result!=ISLE_GAME_OK)goto done;
        p->count=cycle*2+2;
        p->result=isle_game_source_wait(p->game,1);
        if(p->result!=ISLE_GAME_OK)goto done;
        p->table=p->element;
        p->slots_before[cycle]=p->beats;
        p->result=isle_post_window_frame_retire_hidden(p->window,p->element);
        if(p->result!=ISLE_GAME_OK)goto done;
        p->result=isle_post_window_frame_create(p->window,&p->element);
        if(p->result!=ISLE_GAME_OK)goto done;
        p->slots_after[cycle]=p->beats;
        if(p->element.owner!=p->table.owner || p->element.serial==p->table.serial) {
            p->result=ISLE_GAME_INVALID_STATE;
            goto done;
        }
        ++p->callback_waits;
    }
done:
    p->returned=1;
}

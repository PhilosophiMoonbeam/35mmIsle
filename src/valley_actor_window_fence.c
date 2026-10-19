#include "valley_actor_window_fence.h"
#include <stdlib.h>

typedef struct ValleyMessageInvocation {
    struct ValleyMessageInvocation* previous;
    IsleValleyActorSource* source;
    IslePlayerUiSource* ui;
    IsleValleyMessageCallback callback;
    const char* text;
    int32_t x,y,delay;
    uint8_t r,g,b,a,alignment;
    IsleGameResult result;
} ValleyMessageInvocation;
/* Stack-linked only during a genuine nonwaiting C invocation, not an ambient
 * course router or persistent Source registration. Every return, including a
 * Source checked failure caught by invoke, restores the preceding frame. */
#if defined(_MSC_VER)
static __declspec(thread) ValleyMessageInvocation* message_invocation;
#else
static _Thread_local ValleyMessageInvocation* message_invocation;
#endif
static void special_handler(uint8_t value) {
    ValleyMessageInvocation* call=message_invocation;
    if (!call || !call->callback) abort();
    call->callback(call->source,value);
}
IsleGameResult isle_valley_window_install_handler(IslePlayerUiSource* ui,int enabled) {
    if (!ui || !ui->player) return ISLE_GAME_INVALID_ARGUMENT;
    if (ui->status!=ISLE_GAME_OK) return ui->status;
    return isle_player_ui_Msg_SetSpecialHandler(ui,enabled?special_handler:NULL);
}
static void show_call(void* user) {
    ValleyMessageInvocation* call=user;
    call->result=isle_player_ui_Msg_ShowMessage(call->ui,
        (const uint8_t*)call->text,call->x,call->y,call->r,call->g,call->b,
        call->a,call->delay,call->alignment);
}
IsleGameResult isle_valley_window_invoke_show(IsleValleyActorSource* source,
    IslePlayerUiSource* ui,IsleValleyMessageCallback callback,const char* text,
    int32_t x,int32_t y,uint8_t r,uint8_t g,uint8_t b,uint8_t a,
    int32_t delay,uint8_t alignment) {
    IsleGameResult result;
    ValleyMessageInvocation call={message_invocation,source,ui,callback,text,
        x,y,delay,r,g,b,a,alignment,ISLE_GAME_INVALID_STATE};
    if (!source || !ui || !ui->player || ui->player->game!=source->base.nativeGame || !text ||
        (callback && callback!=isle_valley_actor_message_event)) return ISLE_GAME_INVALID_ARGUMENT;
    if (ui->status!=ISLE_GAME_OK) return ui->status;
    message_invocation=&call;
    result=isle_valley_actor_invoke(source,show_call,&call);
    message_invocation=call.previous;
    return result==ISLE_GAME_OK?call.result:result;
}

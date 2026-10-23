#include "printer_diagnostic_source.h"
#include <stdbool.h>
#include <string.h>
typedef IslePrinterDiagnosticSource Context;
typedef IsleGameObject GObj;
typedef IsleGameCamera OMCamera;
typedef const IslePhotoRecord PhotoData;
typedef int32_t s32;
enum { START_BUTTON=0x1000 };
#define ARRAY_COUNT(a) (sizeof(a)/sizeof((a)[0]))
#define pd_check(s,r) isle_post_course_source_checked(&(s)->common,(r),__func__,#r,__LINE__)
#include "printer_diagnostic_declarations.inc"
IsleGameResult isle_post_course_native_progress(IslePostCourseSceneSource*);
static void pd_wait(Context* s,int n) { isle_post_course_source_wait(&s->common,n); }
static int32_t pd_status(Context* s) { int32_t value=0; pd_check(s,s->requiredNativeTable->printer_status(s,&value)); return value; }
static void pd_command(Context* s,int value) { pd_check(s,s->requiredNativeTable->printer_command(s,value)); }
static uint32_t pd_pressed(Context* s) { uint32_t value=0; pd_check(s,s->requiredNativeTable->pressed_buttons(s,&value)); return value; }
static void pd_terminal(Context* s) {
    pd_check(s,s->requiredNativeTable->terminal(s));
    pd_check(s,isle_game_end_process(s->common.game,NULL));
}
static PhotoData* pd_saved(Context* s,int index) { const IslePhotoRecord* value=NULL; pd_check(s,s->requiredNativeTable->saved_photo(s,index,&value)); return value; }
static void pd_cleanup(Context* s) {
    int done=0; IsleGameResult first=ISLE_GAME_OK,r;
    do {
        r=s->requiredNativeTable->replay_cleanup(s,&done);
        if (first==ISLE_GAME_OK && r!=ISLE_GAME_OK) first=r;
        if (!done) { r=isle_post_course_native_progress(&s->common); if (first==ISLE_GAME_OK && r!=ISLE_GAME_OK) first=r; }
    } while (!done);
    pd_check(s,first);
}
#include "printer_diagnostic_original.inc"
void isle_printer_diagnostic_source_initialize(Context* s,uint64_t generation) {
    IslePostCourseSceneSource common=s->common;
    const IslePrinterDiagnosticNative* n=s->requiredNativeTable;
    if (!generation || !n || !n->camera_setup || !n->hidden || !n->saved_photo || !n->replay_initialize ||
        !n->presentation_begin || !n->postprocess || !n->printer_status || !n->printer_command ||
        !n->pressed_buttons || !n->replay_cleanup || !n->terminal || !n->panic || !n->entry_begin || !n->entry_poll) {
        pd_check(s,ISLE_GAME_INVALID_STATE); return;
    }
    if (s->module_generation==generation) return;
    memset(s,0,sizeof(*s)); s->common=common; s->requiredNativeTable=n;
    s->module_generation=generation; s->common.scene=-2; s->result_words[0]=-1;
}
void isle_printer_diagnostic_source_init_call(IslePostCourseSceneSource* common,void* user) {
    (void)user; isle_printer_diagnostic_func_801DCCDC_AA1D1C((Context*)common);
}
void isle_printer_diagnostic_source_entry(void* user) {
    IslePrinterDiagnosticEntry* e=user; Context* s=e->source;
    IsleContinuationDomain* domain=s->common.domain; void* owner=s->common.owner;
    const IslePrinterDiagnosticNative* n=s->requiredNativeTable;
    int retired=0; int32_t result=0;
    s->argument=e->argument;
    e->status=n->entry_begin(s,e->argument);
    if (e->status!=ISLE_GAME_OK) { (void)isle_continuation_fail(domain,"Printer diagnostic setup"); return; }
    while (!retired) {
        if (isle_continuation_wait(domain,1)!=ISLE_CONT_OK) { e->status=ISLE_GAME_INVALID_STATE; (void)isle_continuation_fail(domain,"Printer diagnostic wait"); return; }
        e->status=n->entry_poll(owner,&retired,&result);
        if (e->status!=ISLE_GAME_OK) { (void)isle_continuation_fail(domain,"Printer diagnostic retirement"); return; }
    }
    e->result=result;
}

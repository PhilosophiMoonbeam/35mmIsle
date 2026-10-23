#ifndef ISLE_PRINTER_DIAGNOSTIC_SOURCE_H
#define ISLE_PRINTER_DIAGNOSTIC_SOURCE_H
#include "post_course_scene_source.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct IslePrinterDiagnosticSource IslePrinterDiagnosticSource;
typedef struct IslePrinterDiagnosticNative {
    IsleGameResult (*camera_setup)(IslePrinterDiagnosticSource*);
    IsleGameResult (*hidden)(IslePrinterDiagnosticSource*,IsleGameObject*,int);
    IsleGameResult (*saved_photo)(IslePrinterDiagnosticSource*,int,const IslePhotoRecord**);
    IsleGameResult (*replay_initialize)(IslePrinterDiagnosticSource*,int,int,int);
    IsleGameResult (*presentation_begin)(IslePrinterDiagnosticSource*,IsleGameCamera*,const IslePhotoRecord*);
    IsleGameResult (*postprocess)(IslePrinterDiagnosticSource*,int);
    IsleGameResult (*printer_status)(IslePrinterDiagnosticSource*,int32_t*);
    IsleGameResult (*printer_command)(IslePrinterDiagnosticSource*,int);
    IsleGameResult (*pressed_buttons)(IslePrinterDiagnosticSource*,uint32_t*);
    IsleGameResult (*replay_cleanup)(IslePrinterDiagnosticSource*,int*);
    IsleGameResult (*terminal)(IslePrinterDiagnosticSource*);
    IsleGameResult (*panic)(IslePrinterDiagnosticSource*);
    IsleGameResult (*entry_begin)(IslePrinterDiagnosticSource*,int32_t);
    IsleGameResult (*entry_poll)(void*,int*,int32_t*);
} IslePrinterDiagnosticNative;
struct IslePrinterDiagnosticSource {
    IslePostCourseSceneSource common;
    const IslePrinterDiagnosticNative* requiredNativeTable;
    uint64_t module_generation;
    int32_t result_words[211],argument,render_complete;
    IsleGameObject* sprite_camera;
    IsleGameCamera* camera;
    const IslePhotoRecord* saved[4];
    uint8_t photo_link,actor_link,item_link;
};
typedef struct IslePrinterDiagnosticEntry {
    IslePrinterDiagnosticSource* source;
    int32_t argument,result;
    IsleGameResult status;
} IslePrinterDiagnosticEntry;
void isle_printer_diagnostic_source_initialize(IslePrinterDiagnosticSource*,uint64_t);
void isle_printer_diagnostic_source_init_call(IslePostCourseSceneSource*,void*);
void isle_printer_diagnostic_source_entry(void*);
void isle_printer_diagnostic_func_801DC9D0_AA1A10(IsleGameObject*);
void isle_printer_diagnostic_func_801DC9BC_AA19FC(void*);
#ifdef __cplusplus
}
#endif
#endif

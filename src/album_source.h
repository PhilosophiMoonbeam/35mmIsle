#ifndef ISLE_ALBUM_SOURCE_H
#define ISLE_ALBUM_SOURCE_H
#include "post_course_scene_source.h"
#include "application_photos_source.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct IsleAlbumSource IsleAlbumSource;
typedef struct IsleAlbumElement { IslePostCourseElement handle; } IsleAlbumElement;
typedef struct IsleAlbumFrame { IslePostCourseFrame handle; } IsleAlbumFrame;
typedef struct IsleAlbumComment { uint16_t text[32]; } IsleAlbumComment;
typedef struct IsleAlbumPhoto { IsleGameObject* gObj; IsleGameSprite* sObj; uint8_t isShown; } IsleAlbumPhoto;
typedef struct IsleAlbumWorking { IslePhotoWorking* photoData; IsleAlbumComment comment; } IsleAlbumWorking;
typedef struct IsleAlbumInput { uint32_t currentButtons, pressedButtons; } IsleAlbumInput;
/* Required immutable returning leaves. Native implementations may not invoke
 * Source, wait, pump OM/GTL, invent an update, or perform terminal transfers.
 * Every out parameter clears on failure; typed context addresses remain stable
 * from binding through actual process/replay/graphics retirement. */
typedef struct IsleAlbumNative {
    IsleGameResult (*ui_heap)(IsleAlbumSource*,size_t);
    IsleGameResult (*input_initialize)(IsleAlbumSource*);
    IsleGameResult (*input)(IsleAlbumSource*,int32_t,IsleAlbumInput*);
    IsleGameResult (*input_repeat)(IsleAlbumSource*,int32_t,int32_t);
    IsleGameResult (*input_mask)(IsleAlbumSource*,uint32_t);
    IsleGameResult (*camera)(IsleAlbumSource*,IsleGameObject**);
    IsleGameResult (*aux_camera)(IsleAlbumSource*,int32_t,int32_t);
    IsleGameResult (*process_object)(IsleAlbumSource*,int32_t,int32_t,const void*,IsleGameObject**);
    IsleGameResult (*replay_initialize)(IsleAlbumSource*);
    IsleGameResult (*replay_cleanup)(IsleAlbumSource*);
    IsleGameResult (*preview_scale)(IsleAlbumSource*,float*);
    IsleGameResult (*sound)(IsleAlbumSource*,int32_t);
    IsleGameResult (*song)(IsleAlbumSource*,int32_t,int32_t);
    IsleGameResult (*volume)(IsleAlbumSource*,int32_t,int32_t);
    IsleGameResult (*volume_smooth)(IsleAlbumSource*,int32_t,int32_t,int32_t);
    IsleGameResult (*player_name)(IsleAlbumSource*,const char**);
    IsleGameResult (*pokemon_name)(IsleAlbumSource*,int32_t,const char**);
    IsleGameResult (*level_name)(IsleAlbumSource*,int32_t,const char**);
    IsleGameResult (*scene_result)(IsleAlbumSource*,int32_t);
    IsleGameResult (*entry_begin)(IsleAlbumSource*,int32_t);
    IsleGameResult (*entry_poll)(void*,int*,int32_t*);
    IsleGameResult (*notices_or)(void*,uint32_t);
    /* Exact renderer callbacks for 9A6B10.c75/79, decoded original DL state.
     * No SDK display-list pointer is a host function/data address. */
    IsleGameResult (*display_list)(IsleAlbumSource*,void*);
    IsleGameResult (*render_state)(IsleAlbumSource*,int32_t,int32_t);
    /* Finite Source ambiguities: matched MIPS/data or explicitly scoped native
     * portability receipt REQUIRED at actual Resource admission, no defaults.
     * sp4: 9A6F70.c479-488, called c1586. cover indices4/5 alias receipt:
     * c836-842,c890-891; actual declared extent stays4. Empty last:9ADAD0.c183.
     * Character pages1/2: exact initialized pointer-table origins, not repeating
     * page0 or truncating the original modulo3 selection. */
    IsleGameResult (*unwritten_sp4)(IsleAlbumSource*,IsleGameSprite**);
    IsleGameResult (*cover_overflow_get)(IsleAlbumSource*,int32_t,IsleGameSprite**);
    IsleGameResult (*cover_overflow_set)(IsleAlbumSource*,int32_t,IsleGameSprite*);
    IsleGameResult (*empty_last_photo)(IsleAlbumSource*,int32_t*);
    IsleGameResult (*character_grid)(IsleAlbumSource*,uint32_t,const uint8_t**,size_t*);
    /* Exact additional uninitialized/vararg paths, 9A6F70.c209/c741.
     * Float-format integer call MUST NOT reach host variadic printf. */
    IsleGameResult (*float_format_integer)(IsleAlbumSource*,int32_t,char*,size_t,int32_t*);
    IsleGameResult (*inactive_letter_width)(IsleAlbumSource*,int32_t*);
    /* Actual SObj unk58/5C clear, NOT a copied SDK struct. */
    IsleGameResult (*sprite_clear_links)(IsleAlbumSource*,IsleGameSprite*);
    /* Source overlay cleanup passes its real allocation record, not userData. */
    IsleGameResult (*deallocate)(IsleAlbumSource*,void*);
} IsleAlbumNative;
struct IsleAlbumSource {
    IslePostCourseSceneSource common;
    const IsleAlbumNative* native;
    int32_t D_80208B50_9D2DA0;
    int32_t D_80208B54_9D2DA4;
    int album_IsEditingComment;
    int32_t album_CursorAlpha;
    int32_t album_CursorAlphaRate;
    int32_t album_LetterAlpha;
    int32_t album_LetterAlphaRate;
    int D_80208B88_9D2DD8;
    int32_t album_D_8024EFE0_A19230;
    int album_D_8024FFE8_A1A238;
    int32_t album_D_8024FFEC_A1A23C;
    IsleAlbumElement* album_UiCommentActiveLetter;
    IsleAlbumElement* album_UiAlbumStats;
    IsleAlbumElement* album_UiAlbumHeader;
    IsleAlbumElement* album_UiPhotoComment;
    IsleAlbumElement* album_D_80250000_A1A250;
    IsleAlbumElement* album_UiRows[20];
    IsleAlbumElement* album_UiPageNumber;
    IsleAlbumFrame* album_D_8025005C_A1A2AC;
    IsleGameSprite* album_BackgroundSObj;
    IsleGameObject* album_BackgroundGObj;
    IsleGameSprite* album_CoverTitleSObj[4];
    IsleGameObject* album_CoverTitleGObj;
    int32_t album_D_8025007C_A1A2CC;
    IsleGameSprite* album_PhotoCorners[7];
    IsleGameSprite* album_D_8025009C_A1A2EC;
    IsleGameSprite* album_CommentBackground;
    IsleGameSprite* album_Cursor;
    IsleGameSprite* album_PhotoDescBackground;
    IsleGameObject* album_DraggedPhotoGObj;
    IsleGameSprite* album_DraggedPhotoSObj;
    int32_t album_AlbumPage;
    IslePostCourseScoredPhoto* album_D_802500B8_A1A308;
    uint8_t album_CharGridId;
    int16_t album_CommentCursorPos;
    IsleAlbumPhoto album_Photos[6];
    IsleGameObject* album_D_80250108_A1A358;
    IsleGameSprite* album_D_8025010C_A1A35C;
    int32_t album_D_80250110_A1A360;
    int32_t D_80208B90_9D2DE0;
    int32_t album_SelectedPhoto;
    int32_t D_80208C18_9D2E68;
    int32_t album_CurrentColumn;
    int32_t album_CurrentRow;
    int32_t album_DraggedPhotoColumn;
    int32_t album_DraggedPhotoRow;
    int32_t D_80208C2C_9D2E7C;
    int32_t D_80208C30_9D2E80;
    const IslePostCourseButton* album_CoverButtons;
    IslePostCourseWindow* album_PanelButtons;
    int32_t album_D_80208C40_9D2E90;
    IsleAlbumComment album_EmptyComment;
    IsleAlbumWorking album_D_80250170_A1A3C0;
    int32_t album_D_80250550_A1A7A0;
    int32_t album_D_80250554_A1A7A4;
    IsleAlbumWorking album_D_80250558_A1A7A8;
    int32_t album_D_80250938_A1AB88;
    int32_t album_PhotoCount;
    int32_t album_LastPhotoIndex;
    int32_t album_FirstPhotoIndex;
    IsleAlbumElement elements[32];
    IsleAlbumFrame frame;
    IsleAlbumInput input;
    IslePostCourseScoredPhoto score_scratch;
    IslePostCourseThumbnail thumbnails[8];
    IsleGameSprite* thumbnail_children[8];
    size_t thumbnail_count;
    IsleAlbumComment comment_scratch;
    char formatted[400],number_ascii[32],number_source[66];
    int32_t entry_argument,actual_result;
    int entry_complete;
};
/* Native owners bind both cold PhotoWorking members BEFORE initialize; source
 * retains opaque pointers only, never serializes/copies/reconstructs wire data. */
IsleGameResult isle_album_source_initialize(IsleAlbumSource*,const IsleAlbumNative*,IslePhotoWorking*,IslePhotoWorking*);
void isle_album_source_init_call(IslePostCourseSceneSource*,void*);
void isle_album_source_init(IsleAlbumSource*);
typedef struct IsleAlbumEntry {
    IsleAlbumSource* source;
    int32_t argument,result;
    int complete;
    IsleGameResult status;
} IsleAlbumEntry;
void isle_album_source_entry(void*);
void isle_album_func_801DCA14_9A6C64(IsleGameObject*);
void isle_album_album_BlinkCursor(IsleGameObject*);
void isle_album_album_BlinkLetter(IsleGameObject*);
void isle_album_album_CreateAlbumUI(IsleGameObject*);
/* Distinct render/setup/cleanup ABIs. Their envelope passes common context,
 * never casts an address read from Original initialized data into code. */
void isle_album_source_display_list(IsleAlbumSource*,void*);
void isle_album_source_render_state(IsleAlbumSource*,int32_t,int32_t);
/* Stable native allocation envelope, owned by Scene until cleanup returns.
 * It is neither guest heap bytes nor a GObj.userData context link. */
typedef struct IsleAlbumDeallocation { IsleAlbumSource* source; void* allocation; } IsleAlbumDeallocation;
void isle_album_func_801DCC74_9A6EC4(void*);
#ifdef __cplusplus
}
#endif
#endif

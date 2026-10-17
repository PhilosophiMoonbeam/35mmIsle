#ifndef ISLE_PLAYER_UI_SOURCE_H
#define ISLE_PLAYER_UI_SOURCE_H
#include "player_source.h"
#include "35mmisle/sprite.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct IslePlayerUiIcon {
    uint32_t x, y;
    IsleGameSprite* spriteObj;
} IslePlayerUiIcon;
typedef struct IslePlayerUiIconDef {
    uint32_t x, y;
    int32_t shownWhenZoomedIn, moveOutDirection, unk_10;
    const IsleSprite* spriteDef;
} IslePlayerUiIconDef;
/* One retained semantic record. Input, LastItemId and the photo roll are borrowed
 * directly from player, never sampled into an independent UI authority. */
struct IslePlayerUiSource {
    IslePlayerSource* player;
    void* owner;
    IsleGameResult status;
    IsleGameObject* Msg_CameraObject;
    IsleGameCamera* Msg_Camera;
    int32_t Msg_PosX, Msg_PosY, Msg_PrintDelay, Msg_MessageLength;
    uint8_t Msg_TextColorR, Msg_TextColorG, Msg_TextColorB, Msg_TextColorA;
    int32_t Msg_LineSpacing, Msg_IsInteractive, Msg_IsPrinted;
    int32_t Msg_ShadowEnabled, Msg_BlinkEnabled, Msg_BlinkState;
    float Msg_BlinkAlpha, Message_ScaleX, Message_ScaleY;
    void (*Msg_SpecialHandler)(uint8_t);
    uint32_t Msg_FrameCounter;
    uint8_t Msg_PrintedText[256], Msg_MessageText[256];
    int32_t Msg_LineHeight, Msg_BlinkFrameCounter;
    float Msg_CharOffsetX, Msg_CharOffsetY;
    int32_t Msg_CharType, Msg_ImageIndex;
    const IsleBitmap* font_rgb_bitmaps;
    const IsleBitmap* font_mono_bitmaps;
    const uint8_t* D_803936B8_533AC8;
    const uint8_t* D_80393738_533B48;
    const int16_t* D_8039379C_533BAC;
    IslePlayerUiIcon Icons_IconObjects[9];
    IslePlayerUiIconDef Icons_IconDefs[8];
    int32_t D_80388204_528614[3];
    IsleGameSprite* Icons_ButtonIconsCopy[3];
    uint32_t D_8038821C_52862C;
    uint8_t Icons_ItemFlags;
    IsleGameSprite* Icons_ButtonIcons[3];
    IsleGameObject* Icons_MainObject;
    int32_t Icons_IsShown, Icons_NumItemsAvailable;
    uint8_t Icons_DashEngineEnabled, Icons_FluteIsPlayed, Icons_IsZoomedIn;
    int32_t Icons_TotalMoveOutFrameCounter, D_803AF8BC_54FCCC;
    int32_t Icons_MoveOutCounter[8];
    int32_t D_8038A024_52A434, D_8038A028_52A438;
    int32_t D_8038A02C_52A43C, D_8038A030_52A440;
    const IsleSprite* D_80388E00_529210;
    const IsleSprite* D_80388F60_529370;
    const IsleSprite* D_803890C0_5294D0;
    const IsleSprite* D_80389220_529630;
    IsleGameObject* D_803B09D8_550DE8;
    uint32_t D_803B09DC_550DEC, D_803B0A10_550E20;
    IsleGameSprite* D_803B0A14_550E24;
    IsleGameSprite* D_803B0A18_550E28;
    IsleGameSprite* D_803B0A1C_550E2C;
    uint8_t draw_red, draw_green, draw_blue, draw_alpha;
};

void isle_player_ui_Msg_UpdatePrinter(IsleGameObject*);
uint8_t isle_player_ui_Msg_GetGlyphWidth(IslePlayerUiSource*, int32_t);
const uint8_t* isle_player_ui_Msg_GetCharParams(IslePlayerUiSource*, const uint8_t*, int32_t*, int32_t*);
IsleGameResult isle_player_ui_Msg_DrawChar(IslePlayerUiSource*, const uint8_t*, int32_t, int32_t, int32_t, int32_t);
IsleGameResult isle_player_ui_Msg_DrawMessage(IslePlayerUiSource*, IsleGameObject*);
IsleGameObject* isle_player_ui_Msg_InitCamera(IslePlayerUiSource*);
IsleGameResult isle_player_ui_Msg_ShowMessage(IslePlayerUiSource*, const uint8_t*, int32_t, int32_t, uint8_t, uint8_t, uint8_t, uint8_t, int32_t, uint8_t);
IsleGameResult isle_player_ui_Msg_Reset(IslePlayerUiSource*);
IsleGameResult isle_player_ui_Msg_SetSpecialHandler(IslePlayerUiSource*, void (*)(uint8_t));
IsleGameResult isle_player_ui_Msg_SetNonInteractive(IslePlayerUiSource*);
IsleGameResult isle_player_ui_Msg_SetInteractive(IslePlayerUiSource*);
int isle_player_ui_Msg_IsMessagePrinted(IslePlayerUiSource*);
void isle_player_ui_Icons_UpdateDefault(IsleGameObject*);
IsleGameResult isle_player_ui_func_8035CBB8_4FCFC8(IslePlayerUiSource*, IsleSprite*, const IsleSprite*);
IsleGameResult isle_player_ui_Icons_ShakeIcon(IslePlayerUiSource*, int32_t);
void isle_player_ui_Icons_UpdateDashEngineIcon(IsleGameObject*);
void isle_player_ui_Icons_UpdatePhotoIcon(IsleGameObject*);
IsleGameResult isle_player_ui_func_8035CE74_4FD284(IslePlayerUiSource*);
/* Original blocking helper: only called from its real C processes. */
IsleGameResult isle_player_ui_Icons_ShrinkAndRestore(IslePlayerUiSource*, int32_t, float, float);
void isle_player_ui_Icons_UpdateAppleIcon(IsleGameObject*);
void isle_player_ui_Icons_UpdatePesterBallIcon(IsleGameObject*);
void isle_player_ui_Icons_UpdateFluteIcon(IsleGameObject*);
IsleGameResult isle_player_ui_Icons_Init(IslePlayerUiSource*);
IsleGameResult isle_player_ui_Icons_SetDashEngineEnabled(IslePlayerUiSource*, int32_t);
IsleGameResult isle_player_ui_Icons_MoveIcon(IslePlayerUiSource*, int32_t, int32_t);
void isle_player_ui_Icons_ProcessZoomIn(IsleGameObject*);
void isle_player_ui_Icons_FinishZoomOut(IsleGameObject*);
void isle_player_ui_Icons_ProcessZoomOut(IsleGameObject*);
void isle_player_ui_Icons_FinishZoomIn(IsleGameObject*);
IsleGameResult isle_player_ui_Icons_ProcessZoom(IslePlayerUiSource*, int32_t);
IsleGameResult isle_player_ui_Icons_Show(IslePlayerUiSource*);
IsleGameResult isle_player_ui_Icons_Hide(IslePlayerUiSource*);
IsleGameResult isle_player_ui_Icons_ProcessButtonPress(IslePlayerUiSource*, int32_t);
IsleGameResult isle_player_ui_Icons_ProcessTakePhotoPressed(IslePlayerUiSource*);
IsleGameResult isle_player_ui_Icons_Freeze(IslePlayerUiSource*);
IsleGameResult isle_player_ui_Icons_UnFreeze(IslePlayerUiSource*);
int32_t isle_player_ui_func_8035E370_4FE780(IslePlayerUiSource*);
IsleGameResult isle_player_ui_func_8035E37C_4FE78C(IslePlayerUiSource*);
IsleGameResult isle_player_ui_func_8035E4D0_4FE8E0(IslePlayerUiSource*);
int32_t isle_player_ui_func_8035E508_4FE918(IslePlayerUiSource*);
int32_t isle_player_ui_func_8035E52C_4FE93C(IslePlayerUiSource*);
IsleGameResult isle_player_ui_func_8035E754_4FEB64(IslePlayerUiSource*);
IsleGameResult isle_player_ui_func_8035E764_4FEB74(IslePlayerUiSource*);

/* Returning leaves. These never wait, end a process, fault, or unwind over C. */
IsleGameResult isle_player_ui_native_object(IslePlayerUiSource*, uint32_t, IsleGameSourceCallback, IsleGameObject**);
IsleGameResult isle_player_ui_native_render(IslePlayerUiSource*, IsleGameObject*, uint8_t, uint32_t);
IsleGameResult isle_player_ui_native_sprite(IslePlayerUiSource*, IsleGameObject*, const IsleSprite*, IsleGameSprite**);
IsleGameResult isle_player_ui_native_digit(IslePlayerUiSource*, unsigned, int32_t);
IsleGameResult isle_player_ui_native_glyph(IslePlayerUiSource*, const uint8_t*, int32_t, int32_t, int32_t, int32_t, int);
IsleGameResult isle_player_ui_native_message_camera(IslePlayerUiSource*, IsleGameObject**);
IsleGameResult isle_player_ui_native_delete(IslePlayerUiSource*, IsleGameObject*);
IsleGameResult isle_player_ui_native_copy_bitmap(IslePlayerUiSource*, IsleSprite*, const IsleSprite*);
void isle_player_ui_source_record(IslePlayerUiSource*, IsleGameResult);
IslePlayerUiSource* isle_player_ui_source_current(IsleGameObject*);
IsleGameObject* isle_player_ui_source_object(IslePlayerUiSource*, uint32_t, IsleGameSourceCallback);
IsleGameSprite* isle_player_ui_source_sprite(IslePlayerUiSource*, IsleGameObject*, const IsleSprite*);
void isle_player_ui_source_process(IslePlayerUiSource*, IsleGameObject*, IsleGameSourceCallback, uint8_t, uint32_t);
void isle_player_ui_source_pause(IslePlayerUiSource*, IsleGameObject*, int);
IsleGameSprite* isle_player_ui_source_find_sprite(IslePlayerUiSource*, const IsleSprite*);
IsleGameObject* isle_player_ui_source_find_id(IslePlayerUiSource*, uint32_t);
#ifdef __cplusplus
}
#endif
#endif

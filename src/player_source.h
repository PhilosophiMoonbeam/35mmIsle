#ifndef ISLE_PLAYER_SOURCE_H
#define ISLE_PLAYER_SOURCE_H

#include "35mmisle/engine.h"
#include "35mmisle/rng.h"
#include "world_movement_state.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct IslePlayerUiSource IslePlayerUiSource;
typedef struct IslePlayerPhotoSource IslePlayerPhotoSource;
typedef struct IslePlayerBumpSource IslePlayerBumpSource;

typedef struct IsleScreenTransitionSource IsleScreenTransitionSource;
/* Published once before OM. Original idle/pause C may then modify these same
 * six scalars; callback entry must never restore an immutable input snapshot. */
typedef struct IslePlayerInput {
    uint32_t current_buttons, pressed_buttons, held_buttons, released_buttons;
    float stick_x, stick_y;
} IslePlayerInput;

typedef struct IslePlayerIdleScript {
    const IslePlayerInput* inputs;
    size_t input_count;
    uint32_t rand_seed, player_flags;
} IslePlayerIdleScript;

typedef IsleGameResult (*IslePlayerEventCallback)(void* user, int32_t value);

/* Source semantics, never a console Player/Camera/DObj layout. The fixed-address
 * owner retains this single record through all C continuations, native command
 * borrows and independent graphics/readback/photo/audio fences. Working camera
 * vectors and previous PlayerPos are actual original C state, not new mirrors
 * of the canonical native model/camera or World movement record.
 *
 * game->playerSource is only this nonowning per-game execution-context link.
 * Root bind/get separately discriminate PLAYER payloads. Helper objects may
 * retain NONE payloads; no Actor/Item/Player kind router interprets their data.
 * Native C render/nonwaiting calls receive this context explicitly. */
typedef struct IslePlayerSource {
    IsleGame* game;
    IsleGameObject* object;
    IsleGameModel* model;
    IsleGameCamera* camera;
    isle_rng* rng;
    void* owner;
    IsleGameResult status; /* First checked C execution/admission failure. */
    void* nonwaiting_call_frame; /* Private C-only; positive waits are forbidden. */
    IsleWorldMovementState movement;
    IslePlayerInput input;
    IslePlayerUiSource* ui;
    IslePlayerPhotoSource* photo;
    IsleScreenTransitionSource* screen;
    IslePlayerBumpSource* bump;
    IsleGameVec3 cam_target;

    int32_t gDirectionIndex;
    float ViewPitch;
    int32_t ReticleScreenX, ReticleScreenY;
    float StickXAccum, StickYAccum;
    IsleGameObject* D_80382C38_523048;
    int32_t D_80382C3C_52304C;
    IsleGameObject* gPreviousPokemonInFocus;
    float TotalSpeedMult, CameraShakingDeltaY;
    int32_t TargetDirectionZoomedIn;
    float TargetDirectionYawZommedIn;
    float CameraVibrationDeltaY, CameraVibrationAmplitude, CameraVibrationSpeed;
    int32_t IsCartStopped;
    float DashEngineSpeedMult;
    IsleGameObject* ObjectPauseMenu;
    IsleGameSprite* D_80382C70_523080;
    IsleGameSprite* Pause_LabelPause2;
    IsleGameSprite* Pause_LabelPause;
    IsleGameSprite* Pause_QuitCourseSelected;
    IsleGameSprite* Pause_QuitCourse;
    IsleGameSprite* Pause_ContinueSelected;
    IsleGameSprite* Pause_Continue;
    IsleGameSprite* Pause_RetrySelected;
    IsleGameSprite* Pause_Retry;
    IsleGameVec3 PlayerPos, PlayerVelocity;
    int32_t ThrowItemTimeout, PressPokeFluteTimeout, DashEngineSoundTimeout;
    float StickXValue, StickYValue, PlayerViewYaw, TurnToDirSpeed, CurrentDirectionYaw;
    float D_80382CD4_5230E4;
    float DirectionsList[5];
    float MinPitch, MaxPitch;
    int32_t LastItemId;
    uint32_t sTimerInFocus;
    int32_t D_80382CFC_52310C;
    int8_t D_80382D00_523110, D_80382D04_523114;
    int32_t D_80382D08_523118, IsInputDisabled;
    IsleGameObject* D_80382D10_523120;
    IsleGameObject* D_80382D14_523124;
    IslePlayerEventCallback EndLevelCb, PauseCb;
    void* end_level_user;
    void* pause_user;
    uint8_t IsPaused, PauseCurrentSelection;
    int32_t D_80382D28_523138;
    uint8_t D_80382D2C_52313C, D_80382D30_523140, D_80382D34_523144;
    int32_t D_80382D38_523148;
    float D_80382D3C_52314C;
    uint8_t PauseButtonPressTimeout, D_80382D44_523154;
    int32_t LevelBGMSongID, sMinTimeBetweenPhotos;

    int32_t D_803AE408_54E818;
    IsleGameVec3 CameraEyePos, CameraAtPos, CameraViewVector;
    IsleGameSprite* MainCameraReticles[4];
    IsleGameSprite* D_803AE458_54E868[6];
    int32_t D_803AE470_54E880, D_803AE474_54E884;
    float D_803AE478_54E888, D_803AE47C_54E88C;
    IsleGameVec3 D_803AE4D8_54E8E8, D_803AE4E8_54E8F8;
    IsleGameVec3 D_803AE4F8_54E908, D_803AE508_54E918;
    int8_t ForceDashEngineDisabled, IsTutorialEnabled, D_803AE516_54E926;
    const IslePlayerIdleScript* gIdleScript;
    uint32_t ProgressFlags;
    int8_t ZoomSwitchMode, IsAxisYInverted, IsDashEngineAvailable;
    int32_t MainCameraViewport[4];
    int32_t MainCameraBorderXmin, MainCameraBorderYmin;
    int32_t MainCameraBorderXmax, MainCameraBorderYmax;

    int32_t IsDashEngineOn, D_80382D54_523164, D_80382D78_523188;
    int32_t Pause_StickReleased, D_80382D9C_5231AC, MainCameraBlinkTimeout;
    int32_t D_80382DA4_5231B4, D_80382DA8_5231B8;
    int32_t D_80382DAC_5231BC, D_80382DB0_5231C0;

    /* Original function-static state, isolated per retained Player owner. */
    int32_t dashEngineSoundHandle;
    float D_80382D58_523168, D_80382D5C_52316C;
    float D_80382D60_523170, D_80382D64_523174;
    int32_t sTimerAfterPhotoTaken;
} IslePlayerSource;

/* Prepare expects a zero-initialized, caller-owned record. Zero/reuse is legal
 * only after deactivation and every real borrower closes. It allocates and
 * publishes nothing; minimum_photo_interval comes from initialized ROM.
 * Activate installs only the one contextual link before original camera/root/UI
 * initialization; it does not create or claim an initialized Player model. */
IsleGameResult isle_player_source_prepare(IsleGame*, IslePlayerSource*, isle_rng*,
    void* owner, int32_t minimum_photo_interval);
IsleGameResult isle_player_source_activate(IslePlayerSource*);
/* Owner calls only after every actual native and nonnative borrower closes. */
IsleGameResult isle_player_source_deactivate(IslePlayerSource*);
IsleGameResult isle_player_source_bind(IsleGame*, IsleGameObject*, IslePlayerSource*);
IsleGameResult isle_player_source_get(IsleGame*, IsleGameObject*, IslePlayerSource**);
/* Returning native-leaf lookup; never faults or interprets object payloads.
 * Success with NULL means no active context. An active context may precede
 * original Root/model publication. The caller must retain its real lifetime. */
IsleGameResult isle_player_source_context(IsleGame*, IslePlayerSource**);
/* Callback context lookup never casts helper userdata to a Player payload.
 * Only original C process/update entrypoints may use its failure transfer. */
IslePlayerSource* isle_player_source_current(IsleGameObject*);
/* Original one-argument callbacks are also called directly by original initUI.
 * Preserve that same C body/address: outside OM, only the active local C guard
 * authorizes lookup. This never installs a second context or casts userdata. */
IslePlayerSource* isle_player_source_entry(IsleGameObject*);

typedef void (*IslePlayerNonwaitingCall)(IslePlayerSource*, void* user);
/* callback and intervening source frames must be C, not owning C++. Returning
 * C++ leaves finish before checked() transfers into this local C-only fence.
 * Positive waits reject before any continuation handoff. */
IsleGameResult isle_player_source_call(IslePlayerSource*, IslePlayerNonwaitingCall, void*);
void isle_player_source_checked(IslePlayerSource*, IsleGameResult);
void isle_player_source_wait(IslePlayerSource*, int32_t updates);

/* C-only checked borrows, not returning setup APIs. source must be a retained,
 * prepared record with a live game; the active canonical per-game Source and
 * an original C OM callback or source_call's local C fence are required.
 * All intervening frames must be C: native leaves return before checked()
 * transfers failure, never across an owning C++ frame. C++ setup uses the raw
 * returning engine APIs instead. object is explicit; no payload kind is cast.
 * Returned handles/views are the actual native attachment, not retained copies.
 * Quiesce users before removal/deletion; no borrow survives owner retirement or
 * reuse. Reacquire after waits when attachment lifetime is not otherwise held. */
IsleGameSprite* isle_player_source_first_sprite(IslePlayerSource*, IsleGameObject*);
IsleSprite* isle_player_source_sprite_view(IslePlayerSource*, IsleGameSprite*);

#ifdef __cplusplus
}
#endif
#endif

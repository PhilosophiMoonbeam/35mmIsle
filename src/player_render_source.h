#ifndef ISLE_PLAYER_RENDER_SOURCE_H
#define ISLE_PLAYER_RENDER_SOURCE_H
#include "player_commands_source.h"
#include "player_camera_viewport.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Native semantic metadata only. No console camera, viewport or GBI layout.
 * Callbacks are original C and execute inside nested source_call fences. */
typedef void (*IslePlayerRenderCallback)(IslePlayerSource*, IsleGameObject*);
typedef void (*IslePlayerPreRenderCallback)(IslePlayerSource*, IsleGameCamera*, int32_t);
typedef struct IslePlayerRenderCameraSpec {
    uint32_t id;
    IsleGameSourceCallback update;
    uint8_t link;
    uint32_t object_priority;
    IslePlayerRenderCallback render;
    int32_t camera_priority;
    uint32_t display_link_mask;
    int32_t camera_tag;
    int add_default_matrices;
    uint8_t process_kind;
    IsleGameSourceCallback process;
    uint32_t process_priority;
    int default_flags;
} IslePlayerRenderCameraSpec;

typedef enum IslePlayerRenderCycle {
    ISLE_PLAYER_RENDER_ONE_CYCLE,
    ISLE_PLAYER_RENDER_FILL_CYCLE
} IslePlayerRenderCycle;
typedef enum IslePlayerRenderMode {
    ISLE_PLAYER_RENDER_NOOP,
    ISLE_PLAYER_RENDER_AA_ZB_OPAQUE
} IslePlayerRenderMode;
typedef enum IslePlayerRenderTarget {
    ISLE_PLAYER_RENDER_DEPTH_TARGET,
    ISLE_PLAYER_RENDER_COLOR_TARGET
} IslePlayerRenderTarget;

/* Checked C leaves: lane identifies one of four real bounded native wire
 * cursors; it is not a Gfx pointer, alias, copied list or simulated cursor. */
int32_t isle_player_render_screen_width(IslePlayerSource*);
int32_t isle_player_render_screen_height(IslePlayerSource*);
IsleGameObject* isle_player_render_create_camera(IslePlayerSource*, uint32_t,
    IsleGameSourceCallback, uint8_t, uint32_t, IslePlayerRenderCallback, int32_t,
    uint32_t, int32_t, int, uint8_t, IsleGameSourceCallback, uint32_t, int);
IsleGameCamera* isle_player_render_object_camera(IslePlayerSource*, IsleGameObject*);
IsleGameObject* isle_player_render_camera_object(IslePlayerSource*, IsleGameCamera*);
const IsleGameCameraParameters* isle_player_render_parameters(IslePlayerSource*, IsleGameCamera*);
void isle_player_render_camera_flags(IslePlayerSource*, IsleGameCamera*, uint32_t set);
void isle_player_render_camera_background(IslePlayerSource*, IsleGameCamera*, uint32_t);
void isle_player_render_camera_prerender(IslePlayerSource*, IsleGameCamera*, IslePlayerPreRenderCallback);
void isle_player_render_object_hidden(IslePlayerSource*, IsleGameObject*, int);
void isle_player_render_delete_object(IslePlayerSource*, IsleGameObject*);
void isle_player_render_default_update(IsleGameObject*);
void isle_player_render_sprite_camera(IslePlayerSource*, IsleGameObject*);
void isle_player_render_pipe_sync(IslePlayerSource*, uint32_t lane);
void isle_player_render_cycle(IslePlayerSource*, uint32_t lane, IslePlayerRenderCycle);
void isle_player_render_mode(IslePlayerSource*, uint32_t lane, IslePlayerRenderMode);
void isle_player_render_scissor(IslePlayerSource*, uint32_t lane, int32_t, int32_t, int32_t, int32_t);
void isle_player_render_fill_color(IslePlayerSource*, uint32_t lane, uint32_t);
void isle_player_render_fill_rectangle(IslePlayerSource*, uint32_t lane, int32_t, int32_t, int32_t, int32_t);
void isle_player_render_color_target(IslePlayerSource*, uint32_t lane, IslePlayerRenderTarget);
void isle_player_render_camera_init(IslePlayerSource*, uint32_t lane, IsleGameCamera*, int32_t mode);
void isle_player_render_camera_post(IslePlayerSource*, IsleGameCamera*);
void isle_player_render_camera_render(IslePlayerSource*, IsleGameObject*);
void isle_player_render_process_image(IslePlayerSource*);
void isle_player_render_light_component(IslePlayerSource*, uint32_t component, int8_t value);

/* Returning C++ leaves, implemented by the retained renderer domain. They
 * finish all owning C++ frames before checked C transfers any failure. Original
 * C callbacks may reenter only through a nested plain-C source_call fence at
 * the exact source point; never an unfenced call or function-pointer cast. */
IsleGameResult isle_player_render_backend_dimensions(IslePlayerSource*, int32_t*, int32_t*);
IsleGameResult isle_player_render_backend_create_camera(IslePlayerSource*, const IslePlayerRenderCameraSpec*, IsleGameObject**);
IsleGameResult isle_player_render_backend_camera_flags(IslePlayerSource*, IsleGameCamera*, uint32_t);
IsleGameResult isle_player_render_backend_camera_background(IslePlayerSource*, IsleGameCamera*, uint32_t);
IsleGameResult isle_player_render_backend_camera_prerender(IslePlayerSource*, IsleGameCamera*, IslePlayerPreRenderCallback);
IsleGameResult isle_player_render_backend_sprite_camera(IslePlayerSource*, IsleGameObject*);
IsleGameResult isle_player_render_backend_pipe_sync(IslePlayerSource*, uint32_t);
IsleGameResult isle_player_render_backend_cycle(IslePlayerSource*, uint32_t, IslePlayerRenderCycle);
IsleGameResult isle_player_render_backend_mode(IslePlayerSource*, uint32_t, IslePlayerRenderMode);
IsleGameResult isle_player_render_backend_scissor(IslePlayerSource*, uint32_t, int32_t, int32_t, int32_t, int32_t);
IsleGameResult isle_player_render_backend_fill_color(IslePlayerSource*, uint32_t, uint32_t);
IsleGameResult isle_player_render_backend_fill_rectangle(IslePlayerSource*, uint32_t, int32_t, int32_t, int32_t, int32_t);
IsleGameResult isle_player_render_backend_color_target(IslePlayerSource*, uint32_t, IslePlayerRenderTarget);
IsleGameResult isle_player_render_backend_camera_init(IslePlayerSource*, uint32_t, IsleGameCamera*, int32_t);
IsleGameResult isle_player_render_backend_camera_post(IslePlayerSource*, IsleGameCamera*);
IsleGameResult isle_player_render_backend_camera_render(IslePlayerSource*, IsleGameObject*);
IsleGameResult isle_player_render_backend_process_image(IslePlayerSource*);
IsleGameResult isle_player_render_backend_light_component(IslePlayerSource*, uint32_t, int8_t);

/* Complete original C component entrypoints; camera commands are compiled
 * separately into isle-player-commands so CPU-only FIFO users need no renderer. */
void isle_player_setMainCameraViewport(IslePlayerSource*, int32_t, int32_t, int32_t, int32_t);
void isle_player_mainCameraSetScissor(IslePlayerSource*, uint32_t lane);
void isle_player_mainCameraPreRender(IslePlayerSource*, IsleGameCamera*, int32_t);
void isle_player_fillBorderBlack(IslePlayerSource*, uint32_t lane, int32_t, int32_t, int32_t, int32_t);
void isle_player_func_80357FD4_4F83E4(IsleGameObject*);
void isle_player_mainCameraRender(IslePlayerSource*, IsleGameObject*);
IsleGameObject* isle_player_createMainCameras(IslePlayerSource*, int32_t);
void isle_player_removeMainCamera(IslePlayerSource*);
IsleGameObject* isle_player_getMainCameraObj(IslePlayerSource*);
IsleGameCamera* isle_player_getMainCamera(IslePlayerSource*);
void isle_player_func_800A1E6C(IslePlayerSource*, IsleGameVec3*);

/* Returning dispatch adapters own their local failure fences entirely in C.
 * A C++ renderer never calls an original callback directly. */
IsleGameResult isle_player_render_invoke_create(IslePlayerSource*, int32_t, IsleGameObject**);
IsleGameResult isle_player_render_invoke_callback(IslePlayerSource*, IslePlayerRenderCallback, IsleGameObject*);
IsleGameResult isle_player_render_invoke_prerender(IslePlayerSource*, IslePlayerPreRenderCallback, IsleGameCamera*, int32_t);
IsleGameResult isle_player_render_invoke_remove(IslePlayerSource*);
IsleGameResult isle_player_render_invoke_viewport(IslePlayerSource*, int32_t, int32_t, int32_t, int32_t);
IsleGameResult isle_player_render_invoke_getters(IslePlayerSource*, IsleGameObject**, IsleGameCamera**);

#ifdef __cplusplus
}
#endif
#endif

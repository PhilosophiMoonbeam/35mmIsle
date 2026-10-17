#include "player_render_source.h"
#include "engine_internal.h"

/* Check the same canonical execution context as every source bridge before
 * entering a returning C++ leaf. Never transfer across its owning frames. */
static void render_source(IslePlayerSource* player) {
    IslePlayerSource* active = NULL;
    IslePlayerSource* canonical = player->game->playerSource;
    IslePlayerSource* failure = canonical != NULL ? canonical : player;
    isle_player_source_checked(failure, isle_player_source_context(player->game, &active));
    if (active != player || (player->nonwaiting_call_frame == NULL &&
        (!player->game->ticking || omCurrentObject == NULL)))
        isle_player_source_checked(failure, ISLE_GAME_INVALID_STATE);
    if (player->native_renderer == NULL)
        isle_player_source_checked(player, ISLE_GAME_INVALID_STATE);
}

int32_t isle_player_render_screen_width(IslePlayerSource* player) {
    int32_t width = 0, height = 0;
    render_source(player);
    isle_player_source_checked(player, isle_player_render_backend_dimensions(player, &width, &height));
    return width;
}
int32_t isle_player_render_screen_height(IslePlayerSource* player) {
    int32_t width = 0, height = 0;
    render_source(player);
    isle_player_source_checked(player, isle_player_render_backend_dimensions(player, &width, &height));
    return height;
}
IsleGameObject* isle_player_render_create_camera(IslePlayerSource* player, uint32_t id,
    IsleGameSourceCallback update, uint8_t link, uint32_t object_priority,
    IslePlayerRenderCallback render, int32_t camera_priority, uint32_t display_link_mask,
    int32_t camera_tag, int add_default_matrices, uint8_t process_kind,
    IsleGameSourceCallback process, uint32_t process_priority, int default_flags) {
    IsleGameObject* object = NULL;
    const IslePlayerRenderCameraSpec spec = {id, update, link, object_priority, render,
        camera_priority, display_link_mask, camera_tag, add_default_matrices,
        process_kind, process, process_priority, default_flags};
    render_source(player);
    isle_player_source_checked(player, isle_player_render_backend_create_camera(player, &spec, &object));
    return object;
}
IsleGameCamera* isle_player_render_object_camera(IslePlayerSource* player, IsleGameObject* object) {
    IsleGameCamera* camera = NULL;
    render_source(player);
    isle_player_source_checked(player, isle_game_object_camera(player->game, object, &camera));
    return camera;
}
IsleGameObject* isle_player_render_camera_object(IslePlayerSource* player, IsleGameCamera* camera) {
    IsleGameObject* object = NULL;
    render_source(player);
    isle_player_source_checked(player, isle_game_camera_object(player->game, camera, &object));
    return object;
}
const IsleGameCameraParameters* isle_player_render_parameters(IslePlayerSource* player, IsleGameCamera* camera) {
    const IsleGameCameraParameters* parameters = NULL;
    render_source(player);
    isle_player_source_checked(player, isle_game_camera_parameters(player->game, camera, &parameters));
    return parameters;
}
void isle_player_render_object_hidden(IslePlayerSource* player, IsleGameObject* object, int hidden) {
    IsleGameObjectMetadata metadata;
    render_source(player);
    isle_player_source_checked(player, isle_game_object_metadata(player->game, object, &metadata));
    isle_player_source_checked(player, isle_game_object_set_flags(player->game, object,
        hidden ? metadata.flags | UINT32_C(1) : metadata.flags & ~UINT32_C(1)));
}
void isle_player_render_delete_object(IslePlayerSource* player, IsleGameObject* object) {
    render_source(player);
    isle_player_source_checked(player, isle_game_delete_object(player->game, object));
}
void isle_player_render_default_update(IsleGameObject* object) {
    IslePlayerSource* player = isle_player_source_entry(object);
    isle_player_commands_process(player, object, NULL);
}

#define CHECKED_WRAPPER(name, declaration, arguments) \
    void isle_player_render_##name declaration { \
        render_source(player); \
        isle_player_source_checked(player, isle_player_render_backend_##name arguments); \
    }
CHECKED_WRAPPER(camera_flags, (IslePlayerSource* player, IsleGameCamera* camera, uint32_t set), (player, camera, set))
CHECKED_WRAPPER(camera_background, (IslePlayerSource* player, IsleGameCamera* camera, uint32_t color), (player, camera, color))
CHECKED_WRAPPER(camera_prerender, (IslePlayerSource* player, IsleGameCamera* camera, IslePlayerPreRenderCallback callback), (player, camera, callback))
CHECKED_WRAPPER(sprite_camera, (IslePlayerSource* player, IsleGameObject* object), (player, object))
CHECKED_WRAPPER(pipe_sync, (IslePlayerSource* player, uint32_t lane), (player, lane))
CHECKED_WRAPPER(cycle, (IslePlayerSource* player, uint32_t lane, IslePlayerRenderCycle cycle), (player, lane, cycle))
CHECKED_WRAPPER(mode, (IslePlayerSource* player, uint32_t lane, IslePlayerRenderMode mode), (player, lane, mode))
CHECKED_WRAPPER(scissor, (IslePlayerSource* player, uint32_t lane, int32_t xmin, int32_t ymin, int32_t xmax, int32_t ymax), (player, lane, xmin, ymin, xmax, ymax))
CHECKED_WRAPPER(fill_color, (IslePlayerSource* player, uint32_t lane, uint32_t color), (player, lane, color))
CHECKED_WRAPPER(fill_rectangle, (IslePlayerSource* player, uint32_t lane, int32_t xmin, int32_t ymin, int32_t xmax, int32_t ymax), (player, lane, xmin, ymin, xmax, ymax))
CHECKED_WRAPPER(color_target, (IslePlayerSource* player, uint32_t lane, IslePlayerRenderTarget target), (player, lane, target))
CHECKED_WRAPPER(camera_init, (IslePlayerSource* player, uint32_t lane, IsleGameCamera* camera, int32_t mode), (player, lane, camera, mode))
CHECKED_WRAPPER(camera_post, (IslePlayerSource* player, IsleGameCamera* camera), (player, camera))
CHECKED_WRAPPER(camera_render, (IslePlayerSource* player, IsleGameObject* object), (player, object))
CHECKED_WRAPPER(process_image, (IslePlayerSource* player), (player))
CHECKED_WRAPPER(light_component, (IslePlayerSource* player, uint32_t component, int8_t value), (player, component, value))
#undef CHECKED_WRAPPER

typedef struct RenderCreateCall {
    int32_t background;
    IsleGameObject* object;
} RenderCreateCall;
static void invoke_create(IslePlayerSource* player, void* user) {
    RenderCreateCall* call = user;
    call->object = isle_player_createMainCameras(player, call->background);
}
IsleGameResult isle_player_render_invoke_create(IslePlayerSource* player, int32_t background,
    IsleGameObject** output) {
    RenderCreateCall call = {background, NULL};
    IsleGameResult result;
    if (output == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    *output = NULL;
    result = isle_player_source_call(player, invoke_create, &call);
    *output = call.object;
    return result;
}
typedef struct RenderCallbackCall {
    IslePlayerRenderCallback callback;
    IsleGameObject* object;
} RenderCallbackCall;
static void invoke_callback(IslePlayerSource* player, void* user) {
    RenderCallbackCall* call = user;
    call->callback(player, call->object);
}
IsleGameResult isle_player_render_invoke_callback(IslePlayerSource* player,
    IslePlayerRenderCallback callback, IsleGameObject* object) {
    RenderCallbackCall call = {callback, object};
    if (callback == NULL || object == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    return isle_player_source_call(player, invoke_callback, &call);
}
typedef struct RenderPreCall {
    IslePlayerPreRenderCallback callback;
    IsleGameCamera* camera;
    int32_t mode;
} RenderPreCall;
static void invoke_prerender(IslePlayerSource* player, void* user) {
    RenderPreCall* call = user;
    call->callback(player, call->camera, call->mode);
}
IsleGameResult isle_player_render_invoke_prerender(IslePlayerSource* player,
    IslePlayerPreRenderCallback callback, IsleGameCamera* camera, int32_t mode) {
    RenderPreCall call = {callback, camera, mode};
    if (callback == NULL || camera == NULL || mode < 0 || mode >= 4)
        return ISLE_GAME_INVALID_ARGUMENT;
    return isle_player_source_call(player, invoke_prerender, &call);
}
static void invoke_remove(IslePlayerSource* player, void* user) {
    (void)user;
    isle_player_removeMainCamera(player);
}
IsleGameResult isle_player_render_invoke_remove(IslePlayerSource* player) {
    return isle_player_source_call(player, invoke_remove, NULL);
}
typedef struct RenderViewportCall { int32_t xmin, ymin, xmax, ymax; } RenderViewportCall;
static void invoke_viewport(IslePlayerSource* player, void* user) {
    RenderViewportCall* call = user;
    isle_player_setMainCameraViewport(player, call->xmin, call->ymin, call->xmax, call->ymax);
}
IsleGameResult isle_player_render_invoke_viewport(IslePlayerSource* player,
    int32_t xmin, int32_t ymin, int32_t xmax, int32_t ymax) {
    RenderViewportCall call = {xmin, ymin, xmax, ymax};
    return isle_player_source_call(player, invoke_viewport, &call);
}

typedef struct RenderGettersCall { IsleGameObject* object; IsleGameCamera* camera; } RenderGettersCall;
static void invoke_getters(IslePlayerSource* player, void* user) {
    RenderGettersCall* call = user;
    call->object = isle_player_getMainCameraObj(player);
    call->camera = isle_player_getMainCamera(player);
}
IsleGameResult isle_player_render_invoke_getters(IslePlayerSource* player,
    IsleGameObject** object, IsleGameCamera** camera) {
    RenderGettersCall call = {NULL, NULL};
    IsleGameResult result;
    if (object == NULL || camera == NULL) return ISLE_GAME_INVALID_ARGUMENT;
    *object = NULL; *camera = NULL;
    result = isle_player_source_call(player, invoke_getters, &call);
    *object = call.object; *camera = call.camera;
    return result;
}

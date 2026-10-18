#include "beach_eevee_chansey_source.h"
#include "engine_internal.h"

const void** isle_beach_eevee_chansey_scene_path(IsleGameObject* object) {
    IsleActorSource* actor = isle_actor_source_require(object);
    const void** path = NULL;
    IsleGameResult result = isle_actor_interaction_scene_path(actor, UINT32_C(0x802cd3f4), &path);
    /* The returning native leaf has released its C++ frames before a C fault. */
    if (result != ISLE_GAME_OK || path == NULL)
        isle_native_execution_fault(actor->game, "Beach Eevee/Chansey shared path unavailable");
    return path;
}

IsleActorInteractionGraph isle_beach_eevee_chansey_local_graph(
    IsleGameObject* object, IsleActorInteraction rows[4]) {
    IsleActorSource* actor = isle_actor_source_require(object);
    const IsleActorInteractionGraph* source = isle_actor_source_graph(object, UINT32_C(0x802cd438));
    IsleActorInteractionGraph graph;
    size_t i;
    if (rows == NULL || source == NULL || source->rows == NULL || source->count != 4)
        isle_native_execution_fault(actor->game, "Beach Chansey automatic graph extent differs from pinned source");
    for (i = 0; i < 4; ++i)
        rows[i] = source->rows[i];
    graph.rows = rows;
    graph.count = 4;
    graph.source_identity = source->source_identity;
    return graph;
}

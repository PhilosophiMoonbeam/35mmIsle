#include "item_source_fixture.h"
#include "engine_internal.h"

/* Items deliberately rely on synchronous self-end returning into the source
 * suffix. The identical source callback must not resume for kind-0 threads. */
void item_source_end_and_publish_suffix(IsleGameObject* object) {
    IsleItemSource* item = isle_item_source_require(object);
    item->restTimer += 0.5f;
    item->flags |= ISLE_ITEM_BOUNCED;
    omEndProcess(NULL);
    item->restTimer += 0.5f;
    item->flags |= ISLE_ITEM_TOUCHED_GROUND;
}

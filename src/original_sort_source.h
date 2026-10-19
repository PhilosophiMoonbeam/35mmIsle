#ifndef ISLE_ORIGINAL_SORT_SOURCE_H
#define ISLE_ORIGINAL_SORT_SOURCE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* All three Original bodies compile once in low-level C. A per-call stack
 * record replaces SDK-global comparator/width slots; no callback pointer casts.
 * Comparisons may call returning leaves, never C++-owned Source callbacks.
 * Caller owns writable count*item_size bytes, context and comparison through
 * return; count <= INT32_MAX, item_size <= INT32_MAX, nonzero item_size.
 * count<2 performs no pointer arithmetic and invokes no comparator. */
typedef int32_t (*IsleOriginalSourceCompare)(void*, const void*, const void*);
void isle_original_source_qsort2(void*, void*, uint32_t, uint32_t, IsleOriginalSourceCompare);
/* Exact func_803750CC_84887C: NULL on either side compares equal; otherwise
 * original plain-char subtraction/termination, on original encoded byte strings.
 * No normalization, locale, UTF-8 conversion, or host collation is involved. */
int32_t isle_original_source_name_compare(const char*, const char*);
#ifdef __cplusplus
}
#endif
#endif

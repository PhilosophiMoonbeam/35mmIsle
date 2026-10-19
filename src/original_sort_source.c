#include "original_sort_source.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
/* No allocation, copying record arrays, TLS/global dispatch slot, or C++ frame.
 * Original byte-wise swap and original inversion/partition order stay in C. */
typedef struct IsleOriginalSortCall {
    void* context;
    uint32_t item_size;
    IsleOriginalSourceCompare compare;
} IsleOriginalSortCall;
#include "original_sort_original.inc"

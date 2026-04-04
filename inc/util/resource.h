#ifndef UTIL_RESOURCE_H
#define UTIL_RESOURCE_H

#include "util/types.h"

#define ADD_RESOURCE(array, item) Util_AddNewResource(REF(array), &(item), &(item).compare, &(item).next_freed)
#define REUSE_RESOURCE(array, item, root_idx) Util_ReuseResource(REF(array), &(item), &(item).compare, &(array)[root_idx].compare, &root_idx, (array)[root_idx].next_freed)

handle Util_AddNewResource(void** array_ptr, const void* item_ptr, handle* inout_compare, u16* inout_next_idx);
handle Util_ReuseResource(void** array_ptr, const void* item_ptr, handle* inout_compare, handle* inout_old_compare, u16* inout_root_idx, u16 next_idx);

#endif

#ifndef ECT_HANDLE_H
#define ECT_HANDLE_H

#include "util/types.h"
#include "util/array.h"

#define ADD_HANDLE(array, item) Util_AddNewHandle(REF(array), &(item), &(item).compare, &(item).next_freed)
#define REUSE_HANDLE(array, item, root_idx) Util_ReuseHandle(REF(array), &(item), &(item).compare, &(array)[root_idx].compare, &root_idx, (array)[root_idx].next_freed)

static inline bool Util_IsHandleValid(void* array, handle res_handle)
{
   if (array == NULL)
      return false;

   u16 length = (u16)Util_ArrayLength(array);
   return (res_handle.handle < length && res_handle.id != INVALID_HANDLE_ID);
}

handle Util_AddNewHandle(void** array_ptr, const void* item_ptr, handle* inout_compare, u16* inout_next_idx);
handle Util_ReuseHandle(void** array_ptr, const void* item_ptr, handle* inout_compare, handle* inout_old_compare, u16* inout_root_idx, u16 next_idx);

#endif

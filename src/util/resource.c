#include "util/types.h"
#include "util/resource.h"
#include "util/array.h"

#include <string.h>

handle Util_AddNewResource(void** array_ptr, const void* item_ptr, handle* inout_compare, u16* inout_next_idx)
{
   if (array_ptr == NULL || inout_compare == NULL || inout_next_idx == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   u32 index = Util_ArrayLength(*array_ptr);
   uS size = Util_ArrayTypeSize(*array_ptr);
   Util_InsertArrayIndex(array_ptr, index);

   u8* ptr = (u8*)(*array_ptr) + (uS)index * size;

   (*inout_compare) = (handle){ .handle = (u16)index };
   (*inout_next_idx) = INVALID_HANDLE;

   if (item_ptr != NULL)
      memcpy(ptr, item_ptr, size);

   return (*inout_compare);
}

handle Util_ReuseResource(void** array_ptr, const void* item_ptr, handle* inout_compare, handle* inout_old_compare, u16* inout_root_idx, u16 next_idx)
{
   if (array_ptr == NULL || inout_compare == NULL || inout_old_compare == NULL || inout_root_idx == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   (*inout_old_compare).ref++;
   (*inout_compare) = (*inout_old_compare);
   (*inout_root_idx) = next_idx;

   u16 index = (*inout_compare).handle;
   uS size = Util_ArrayTypeSize(*array_ptr);

   u8* ptr = (u8*)(*array_ptr) + (uS)index * size;

   if (item_ptr != NULL)
      memcpy(ptr, item_ptr, size);
   else
      memset(ptr, 0, size);

   return (*inout_compare);
}

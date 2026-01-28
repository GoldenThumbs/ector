#include "util/types.h"
#include "util/resource.h"
#include "util/array.h"

#include <string.h>

handle Util_AddNewResource(void** array_ptr, const void* item_ptr, handle* compare)
{
   if (array_ptr == NULL || item_ptr == NULL || compare == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   u32 index = Util_ArrayLength(*array_ptr);
   uS size = Util_ArrayTypeSize(*array_ptr);
   Util_InsertArrayIndex(array_ptr, index);

   u8* ptr = (u8*)(*array_ptr) + (uS)index * size;

   (*compare) = (handle){ .handle = (u16)index };

   memcpy(ptr, item_ptr, size);

   return (*compare);
}

handle Util_ReuseResource(void** array_ptr, const void* item_ptr, handle* compare, handle* old_compare, u16* root_idx, u16 next_idx)
{
   if (array_ptr == NULL || item_ptr == NULL || compare == NULL || old_compare == NULL || root_idx == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   (*old_compare).ref++; 
   (*compare) = (*old_compare);
   (*root_idx) = next_idx;

   u32 index = (u32)(*compare).handle;
   uS size = Util_ArrayTypeSize(*array_ptr);

   u8* ptr = (u8*)(*array_ptr) + (uS)index * size;

   memcpy(ptr, item_ptr, size);

   return (*compare);
}

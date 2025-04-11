#include "util/types.h"
#include "util/resource.h"
#include "util/array.h"

#include <string.h>

handle Util_AddResource(u16* ref_counter, void** array_ptr, const void* item_ptr)
{
   u32 index = Util_ArrayLength(*array_ptr);
   uS size = Util_ArrayTypeSize(*array_ptr);
   Util_InsertArrayIndex(array_ptr, index);

   u8* ptr = (u8*)(*array_ptr) + (uS)index * size;
   memcpy(ptr, item_ptr, size);

   handle res_handle = { 0 };
   res_handle.handle = index;
   res_handle.ref = (*ref_counter);
   (*ref_counter)++;

   return res_handle;
}

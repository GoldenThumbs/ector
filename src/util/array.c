#include "util/array.h"
#include "util/types.h"

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define ARR_DATA(array) (&(array)->data)
#define ARR_PTR_VALID(array_ptr) (((array_ptr) != NULL) && (*(array_ptr) != NULL))
#define ARR_N_OK(array, offs) ((1u + (array)->length + (offs)) < (u32)(array)->memory)

uS LeadingZeros_uS(uS x)
{
   if (!x)
      return 0;
   uS n_bits = sizeof(uS) * CHAR_BIT;
   uS mask = ((uS)1 << (n_bits - 1u));

   uS i = 0;
   while ((i<n_bits) && (((x << i) & mask) != mask))
      i++;

   return i;
}

uS Log2_uS(uS x)
{
   return sizeof(uS) * CHAR_BIT - clz(x) - 1;
}

uS Pow2_uS(uS x)
{
   return (uS)1 << x;
}

void Util_SetArrayMemory(void** array_ptr, u32 desired_length)
{
   Array* array = ARRAY_HEADER(*array_ptr);

   if ((uS)desired_length >= array->memory)
      Util_ReallocArray(array_ptr, desired_length);
}

void Util_SetArrayLength(void** array_ptr, u32 desired_length)
{
   Util_SetArrayMemory(array_ptr, desired_length);

   Array* array = ARRAY_HEADER(*array_ptr);
   array->length = desired_length;
}

uS Util_ArrayNeededMemory(uS length)
{
   if (!length)
      length = 2u;
   return Pow2_uS(Log2_uS(length) + 1u);
}

uS Util_ArrayNeededBytes(uS memory, uS type_size)
{
   return (memory * type_size) + sizeof(Array);
}

void* Util_CreateArrayOfLength(u32 length, uS type_size)
{
   uS memory = Util_ArrayNeededMemory((uS)length);
   uS num_bytes = Util_ArrayNeededBytes(memory, type_size);

   Array* array = malloc(num_bytes);
   if (array == NULL)
      return NULL;
   
   memset(array, 0, num_bytes);

   array->size = type_size;
   array->memory = memory;
   array->length = 0;
   array->err = (error){ .total_bits = 0u };

   return ARR_DATA(array);
}

void Util_ReallocArray(void** array_ptr, u32 desired_length)
{
   if (!ARR_PTR_VALID(array_ptr))
      abort();

   Array* array = ARRAY_HEADER(*array_ptr);

   uS size = array->size;
   uS memory = Util_ArrayNeededMemory((uS)desired_length);
   uS num_bytes = Util_ArrayNeededBytes(memory, size);
   uS old_memory = array->memory;

   void* tmp = realloc(array, num_bytes);

   if (tmp == NULL)
   {
      array->err.general = ERR_ERROR;
      array->err.extra = ERR_ARRAY_REALLOC_FAILED;
      fprintf(stderr, "ERROR [ARRAY]: Realloc Failed!\n");
      return;
   }

   uS offset = size * (old_memory);
   uS diff = size * (memory - old_memory);

   array = (Array*)tmp;
   array->memory = memory;
   *array_ptr = ARR_DATA(array);

   u8* ptr_end = (u8*)(*array_ptr) + offset;
   memset(ptr_end, 0, diff);
}

void Util_InsertArrayIndex(void** array_ptr, u32 index)
{
   if (!ARR_PTR_VALID(array_ptr))
      abort();

   Array* array = ARRAY_HEADER(*array_ptr);

   u32 length = array->length;
   uS size = array->size;

   if (!ARR_N_OK(array, 1u))
   {
      Util_ReallocArray(array_ptr, length + 1u);
      array = ARRAY_HEADER(*array_ptr);

      if (array->err.general == ERR_ERROR)
         return;
   }

   if (index >= length)
   {
      array->length = length + 1u;
      return;
   }

   u8* ptr_a = (u8*)(*array_ptr) + (uS)(index + 1u) * size;
   u8* ptr_b = (u8*)(*array_ptr) + (uS)index * size;
   uS num_bytes = size * (uS)(length - index);
   memmove(ptr_a, ptr_b, num_bytes);

   array->length = length + 1u;
}

void Util_RemoveArrayIndex(void** array_ptr, u32 index)
{
   if (!ARR_PTR_VALID(array_ptr))
      abort();

   Array* array = ARRAY_HEADER(*array_ptr);

   u32 length = array->length;
   uS size = array->size;

   if (index >= length)
   {
      array->err.general = ERR_WARN;
      array->err.flags |= ERR_ARRAY_INDEX_OVER;
      index = length - 1u;

      fprintf(stderr, "WARNING [ARRAY]: Cannot Remove Index Larger Than Array Length - 1!\nAssuming Last Element.\n");
   }

   if (index < (length - 1u))
   {
      u8* ptr_a = (u8*)(*array_ptr) + (uS)index * size;
      u8* ptr_b = (u8*)(*array_ptr) + (uS)(index + 1u) * size;
      u8* ptr_c = (u8*)(*array_ptr) + (uS)length * size;

      memcpy(ptr_c, ptr_a, size);

      uS num_bytes = size * (uS)(length - index + 1u);
      memmove(ptr_a, ptr_b, num_bytes);
   }

   array->length = length - 1u;
}

u32 Util_UsableArrayIndex(void* ptr, u32 index)
{
   if (ptr == NULL)
      abort();

   Array* array = ARRAY_HEADER(ptr);

   if (array->err.general == ERR_ERROR)
      return array->length;

   return ((index >= array->length) ? (array->length - 1u) : index);
}

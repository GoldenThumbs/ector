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

rU LeadingZeros_uS(rU x)
{
   if (!x)
      return 0;
   rU n_bits = sizeof(rU) * CHAR_BIT;
   rU mask = ((rU)1 << (n_bits - 1u));

   rU i = 0;
   while ((i<n_bits) && (((x << i) & mask) != mask))
      i++;

   return i;
}

rU Log2_uS(rU x)
{
   return sizeof(rU) * CHAR_BIT - clz(x) - 1;
}

rU Pow2_uS(rU x)
{
   return (rU)1 << x;
}

void Util_SetArrayMemory(void** array_ptr, u32 desired_length)
{
   Array* array = ARRAY_HEADER(*array_ptr);

   if ((rU)desired_length >= array->memory)
      Util_ReallocArray(array_ptr, desired_length);
}

void Util_SetArrayLength(void** array_ptr, u32 desired_length)
{
   Util_SetArrayMemory(array_ptr, desired_length);

   Array* array = ARRAY_HEADER(*array_ptr);
   array->length = desired_length;
}

rU Util_ArrayNeededMemory(rU length)
{
   if (!length)
      length = 2u;
   return Pow2_uS(Log2_uS(length) + 1u);
}

rU Util_ArrayNeededBytes(rU memory, rU type_size)
{
   return (memory * type_size) + sizeof(Array);
}

void* Util_CreateArrayOfLength(u32 length, rU type_size)
{
   rU memory = Util_ArrayNeededMemory((rU)length);
   rU num_bytes = Util_ArrayNeededBytes(memory, type_size);

   Array* array = calloc(1, num_bytes);
   if (array == NULL)
      return NULL;

   array->size = type_size;
   array->memory = memory;
   array->length = length;
   array->err = (error){ .total_bits = 0u };

   return ARR_DATA(array);
}

void Util_ReallocArray(void** array_ptr, u32 desired_length)
{
   if (!ARR_PTR_VALID(array_ptr))
      abort();

   Array* array = ARRAY_HEADER(*array_ptr);

   rU memory = Util_ArrayNeededMemory((rU)desired_length);
   rU num_bytes = Util_ArrayNeededBytes(memory, array->size);

   void* tmp = realloc(array, num_bytes);

   if (tmp == NULL)
   {
      array->err.general = ERR_ERROR;
      array->err.extra = ERR_ARRAY_REALLOC_FAILED;
      fprintf(stderr, "ERROR [ARRAY]: Realloc Failed!\n");
      return;
   }

   array = (Array*)tmp;
   array->memory = memory;
   *array_ptr = ARR_DATA(array);
}

void Util_InsertArrayIndex(void** array_ptr, u32 index)
{
   if (!ARR_PTR_VALID(array_ptr))
      abort();

   Array* array = ARRAY_HEADER(*array_ptr);
   u32 length = array->length;
   rU size = array->size;

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

   u8* ptr_a = (u8*)(*array_ptr) + (rU)(index + 1u) * size;
   u8* ptr_b = (u8*)(*array_ptr) + (rU)index * size;
   rU num_bytes = size * (rU)(length - index);
   memmove(ptr_a, ptr_b, num_bytes);

   array->length = length + 1u;
   return;
}

void Util_RemoveArrayIndex(void** array_ptr, u32 index)
{
   if (!ARR_PTR_VALID(array_ptr))
      abort();

   Array* array = ARRAY_HEADER(*array_ptr);

   u32 length = array->length;
   rU size = array->size;

   if (index >= length)
   {
      array->err.general = ERR_WARN;
      array->err.flags |= ERR_ARRAY_INDEX_OVER;
      index = length - 1u;

      fprintf(stderr, "WARNING [ARRAY]: Cannot Remove Index Larger Than Array Length - 1!\nAssuming Last Element.\n");
   }

   if (index < (length - 1u))
   {
      u8* ptr_a = (u8*)(*array_ptr) + (rU)index * size;
      u8* ptr_b = (u8*)(*array_ptr) + (rU)(index + 1u) * size;
      u8* ptr_c = (u8*)(*array_ptr) + (rU)length * size;

      memcpy(ptr_c, ptr_a, size);

      rU num_bytes = size * (rU)(length - index + 1u);
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

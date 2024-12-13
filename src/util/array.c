#include "util/array.h"
#include "ect_types.h"

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define ARRAY_PTR_VALID(array_ptr) (((array_ptr) != NULL) && (*(array_ptr) != NULL))

uS LeadingZeros(uS x)
{
   uS n_bits = (sizeof(x) * 8u);
   uS mask = (1u << (n_bits - 1u));

   uS i = 0;
   while ((i<n_bits) && (((x << i) & mask) != mask))
      i++;

   return i;
}

uS Log2i(uS x)
{
   return sizeof(uS) * CHAR_BIT - clz(x) - 1u;
}

uS Pow2i(uS x)
{
   return (1u) << x;
}

uS Util_ArrayNeededMemory(uS length)
{
   if (length == 0u)
      length = 1u;
   return Pow2i(Log2i(length) + 1u) + 1u;
}

uS Util_ArrayNeededBytes(uS memory, uS type_size)
{
   return memory * type_size + sizeof(Array);
}

void* Util_CreateArrayOfLength(u32 length, uS type_size)
{
   uS memory = Util_ArrayNeededMemory((uS)length);
   uS num_bytes = Util_ArrayNeededBytes(memory, type_size);
   Array* result = malloc(num_bytes);
   if (result == NULL)
      return NULL;

   result->size = type_size;
   result->memory = memory;
   result->length = length;
   result->err = (error){ .total_bits = 0u };

   memset(result->data, 0, (length + 1u) * type_size);

   return (void*)result->data;
}

void Util_ReallocArray(void** array_ptr, u32 desired_length)
{
   if (!ARRAY_PTR_VALID(array_ptr))
      abort();

   Array* array = ARRAY_HEADER(*array_ptr);

   uS memory = Util_ArrayNeededMemory((uS)desired_length);
   uS num_bytes = Util_ArrayNeededBytes(memory, array->size);

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
   *array_ptr = (void*)array->data;
}

void Util_InsertArrayIndex(void** array_ptr, u32 index)
{
   if (!ARRAY_PTR_VALID(array_ptr))
      abort();

   Array* array = ARRAY_HEADER(*array_ptr);

   if ((array->length + 1u) >= (u32)array->memory)
   {
      Util_ReallocArray(array_ptr, array->length + 1u);
      array = ARRAY_HEADER(*array_ptr);

      if (array->err.general == ERR_ERROR)
         return;
   }

   if (index >= array->length)
   {
      array->length = array->length + 1u;
      return;
   }

   void* ptr_a = *array_ptr + (uS)(index + 1u) * array->size;
   void* ptr_b = *array_ptr + (uS)(index) * array->size;
   uS num_bytes = array->size * (uS)(array->length - index);
   memmove(ptr_a, ptr_b, num_bytes);

   array->length = array->length + 1u;
   return;
}

void Util_RemoveArrayIndex(void** array_ptr, u32 index)
{
   if (!ARRAY_PTR_VALID(array_ptr))
      abort();

   Array* array = ARRAY_HEADER(*array_ptr);

   if (index >= array->length)
   {
      array->err.general = ERR_WARN;
      array->err.flags |= ERR_ARRAY_INDEX_OVER;
      index = array->length - 1u;

      fprintf(stderr, "WARNING [ARRAY]: Cannot Remove Index Larger Than Array Length - 1!\nAssuming Last Element.\n");
   }

   if (index < (array->length - 1u))
   {
      void* ptr_a = *array_ptr + (uS)(index) * array->size;
      void* ptr_b = *array_ptr + (uS)(index + 1u) * array->size;
      void* ptr_c = *array_ptr + (uS)(array->length) * array->size;

      memcpy(ptr_c, ptr_a, array->size);

      uS num_bytes = array->size * (uS)(array->length - index + 1u);
      memmove(ptr_a, ptr_b, num_bytes);
   }

   array->length = array->length - 1u;
}

u32 Util_UsableArrayIndex(void* ptr, u32 index)
{
   if (ptr == NULL)
      abort();

   Array* array = ARRAY_HEADER(ptr);

   if (array->err.general == ERR_ERROR)
      return array->length;

   index = ((index >= array->length) ? (array->length - 1u) : index);
   return index;
}

/*uS ArrayRequiredMemory(uS length)
{
   if (!length)
      length = 1u;
   return Pow2i(Log2i(length) + 1u);
}

uS ArrayRequiredBytes(uS memory, uS size)
{
   return (memory * size) + sizeof(array);
}

array* ArrayInitWithLength(uS size, uS length)
{
   array* res = NULL;

   uS memory = ArrayRequiredMemory(length);
   res = malloc(ArrayRequiredBytes(memory, size));
   if (!res)
      abort();

   res->size = size;
   res->memory = memory;
   res->length = length;

   memset(res->data, 0, length * size);

   return res;
}

array* ArrayInit(uS size)
{
   return ArrayInitWithLength(size, 0u);
}

bool ArrayReallocate(void* arr, uS memory)
{
   if (!arr)
      abort();

   array* arr_header = ARRAY_HEADER(arr);

   uS size = arr_header->size;
   uS length = arr_header->length;
   uS old_memory = arr_header->memory;
   uS num_bytes = ArrayRequiredBytes(memory, size);

   void* tmp = realloc(arr, num_bytes);

   if (!tmp)
      return false;

   arr = tmp;

   arr_header = ARRAY_HEADER(arr);
   arr_header->memory = memory;

   return true;
}


uS ArrayInsert(void* arr, uS index)
{
   if (!arr)
      abort();

   array* arr_header = ARRAY_HEADER(arr);

   uS size = arr_header->size;
   uS length = arr_header->length;
   uS memory = arr_header->memory;

   if ((length + 1u) >= memory)
   {
      memory = ArrayRequiredMemory(length + 1u);
      if (!ArrayReallocate(arr, memory))
         return 0;
   }

   uS num_bytes = size * (length - index);
   memmove(arr + (index + 1u) * size, arr + index * size, num_bytes);

   arr_header = ARRAY_HEADER(arr);

   arr_header->length = length + 1u;
   return index;
}

uS ArrayRemove(void* arr, uS index)
{
   if (!arr)
      abort();

   array* arr_header = ARRAY_HEADER(arr);

   uS size = arr_header->size;
   uS length = arr_header->length;

   void* tmp = malloc(size);
   memcpy(tmp, arr + index * size, size);

   uS num_bytes = size * (length - index);
   memmove(arr + index * size, arr + (index + 1u) * size, num_bytes);
   memcpy(arr + --length * size, tmp, size);
   free(tmp);

   arr_header = ARRAY_HEADER(arr);

   arr_header->length = length;
   return length;
}

uS ArrayPushBack(void* arr)
{
   if (!arr)
      abort();

   array* arr_header = ARRAY_HEADER(arr);

   uS size = arr_header->size;
   uS length = arr_header->length;
   uS memory = arr_header->memory;

   if ((length + 1u) >= memory)
   {
      memory = ArrayRequiredMemory(length + 1u);
      if (!ArrayReallocate(arr, memory))
         return 0;
   }

   arr_header = ARRAY_HEADER(arr);

   arr_header->length = length + 1u;
   return length;
}

uS ArrayPushFront(void* arr)
{
   return ArrayInsert(arr, 0);
}

uS ArrayPopBack(void* arr)
{
   if (!arr)
      abort();

   array* arr_header = ARRAY_HEADER(arr);

   return --arr_header->length;
}

uS ArrayPopFront(void* arr)
{
   return ArrayRemove(arr, 0);
}*/

#ifndef ECT_ARRAY_H
#define ECT_ARRAY_H

#include "util/types.h"
#include "util/files.h"

// #include <stdalign.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifndef clz
   //#define clz __builtin_clzll
   #define clz LeadingZeros_uS
#endif

#define ARRAY_HEADER(ptr) ((Array*)(ptr) - 1)

// utility macros for denoting arrays and maps
// --<

#define ARRAY_TYPE(T) __Array_##T
#define MAP_TYPE(T) __Map_##T
#define ARRAY_TYPEDEF(T) typedef T* ARRAY_TYPE(T)
#define MAP_TYPEDEF(T) typedef T* MAP_TYPE(T)

// >--

#define ARRAY_SIZE Util_ArrayTypeSize
#define ARRAY_MEMORY Util_ArrayMemory
#define ARRAY_LENGTH Util_ArrayLength
#define ARRAY_ERROR Util_ArrayError

#define NEW_ARRAY_N(T, N) Util_CreateArrayOfLength(N, sizeof(T))
#define NEW_ARRAY(T) NEW_ARRAY_N(T, 2u)
#define SET_ARRAY_LENGTH(ptr, N) Util_SetArrayLength(REF(ptr), N)
#define FREE_ARRAY(ptr) (((ptr) != NULL) ? free(ARRAY_HEADER(ptr)) : ((void)0))
#define JOIN_ARRAYS(ptr_a, ptr_b) Util_JoinArrays(REF(ptr_a), (ptr_b))
#define INSERT_ARRAY(ptr, i, item) (Util_InsertArrayIndex(REF(ptr), i), ptr[Util_UsableArrayIndex((ptr), i)] = (item))
#define REMOVE_ARRAY(ptr, i) (Util_RemoveArrayIndex(REF(ptr), i), ptr[Util_ArrayLength(ptr)])
#define ADD_BACK_ARRAY(ptr, item) INSERT_ARRAY(ptr, Util_ArrayLength(ptr), (item))
#define ADD_FRONT_ARRAY(ptr, item) INSERT_ARRAY(ptr, 0u, (item))
#define POP_BACK_ARRAY(ptr) REMOVE_ARRAY(ptr, Util_ArrayLength(ptr) - 1u)
#define POP_FRONT_ARRAY(ptr) REMOVE_ARRAY(ptr, 0u)

#define MAP_SIZE Util_ArrayTypeSize
#define MAP_MEMORY Util_ArrayMemory
#define MAP_LENGTH Util_ArrayLength
#define MAP_ERROR Util_ArrayError

#define NEW_MAP_N(T, N) Util_CreateArrayOfLength(N, sizeof(T) + sizeof(MapItem))
#define NEW_MAP(T) NEW_MAP_N(T, 2u)
#define SET_MAP_LENGTH(ptr, N) SET_ARRAY_LENGTH(ptr, N)
#define FREE_MAP(ptr) Util_FreeMap(ptr)
#define JOIN_MAPS(ptr_a, ptr_b) JOIN_ARRAYS(ptr_a, ptr_b)
#define ADD_MAP_ITEM(ptr, key, item) Util_AddMapItem(REF(ptr), key, &(item))
#define ADD_MAP_KEY(ptr, key) Util_AddMapItem(REF(ptr), key, NULL)
#define GET_MAP_ITEM(ptr, key) Util_GetMapItem(ptr, key)
#define REMOVE_MAP_ITEM(ptr, key) Util_RemoveMapItem(REF(ptr), key)

#define ARRAY_MODULE "Array"

enum {
   ERR_ARRAY_REALLOC_FAILED = 1,
   ERR_ARRAY_INVALID
};

#define WARN_ARRAY_INDEX_OVER (1u << 0u)
#define WARN_ARRAY_JOIN_SIZE_MISMATCH (1u << 1u)

typedef struct Array_t
{
   uS size;
   uS memory;
   u32 length;
   error err;
   u8 data[];

} Array;

typedef struct MapItem_t
{
   char* key;
   u8 value[];

} MapItem;

uS LeadingZeros_uS(uS x);
uS Log2_uS(uS x);
uS Pow2_uS(uS x);

void Util_SetArrayMemory(void** array_ptr, u32 desired_length);
void Util_SetArrayLength(void** array_ptr, u32 desired_length);

uS Util_ArrayNeededMemory(uS length);
uS Util_ArrayNeededBytes(uS memory, uS type_size);

void* Util_CreateArrayOfLength(u32 length, uS type_size);
void Util_ReallocArray(void** array_ptr, u32 desired_length);
void Util_InsertArrayIndex(void** array_ptr, u32 index);
void Util_RemoveArrayIndex(void** array_ptr, u32 index);
void Util_JoinArrays(void** array_ptr_a, void* ptr_b);

u32 Util_UsableArrayIndex(void* ptr, u32 index);

static inline bool Util_ArrayIsValid(void* ptr)
{
   if (ptr != NULL)
   {
      error array_err = ARRAY_HEADER(ptr)->err;
      if (array_err.general >= ERR_LEVEL_ERROR)
      {
         Util_Log(NULL, ARRAY_MODULE, array_err, "Array has errors!");

         return false;
      }

      return true;
   }

   error err = { 0 };
   err.general = ERR_LEVEL_ERROR;
   err.extra = ERR_ARRAY_INVALID;

   Util_Log(NULL, ARRAY_MODULE, err, "Array is NULL!");

   return false;
}

static bool Util_ArrayPtrIsValid(void** array_ptr)
{
   return (array_ptr != NULL && Util_ArrayIsValid(*array_ptr));
}

static inline uS Util_ArrayTypeSize(void* ptr)
{
   if (!Util_ArrayIsValid(ptr))
      return 0;

   return ARRAY_HEADER(ptr)->size;
}

static inline uS Util_ArrayMemory(void* ptr)
{
   if (!Util_ArrayIsValid(ptr))
      return 0;

   return ARRAY_HEADER(ptr)->memory;
}

static inline u32 Util_ArrayLength(void* ptr)
{
   if (!Util_ArrayIsValid(ptr))
      return 0;

   return ARRAY_HEADER(ptr)->length;
}

static inline error Util_ArrayError(void* ptr)
{
   if (ptr == NULL)
   {
      error err = { 0 };
      err.general = ERR_LEVEL_ERROR;
      err.extra = ERR_ARRAY_INVALID;

      return err;
   }

   return ARRAY_HEADER(ptr)->err;
}

static inline MapItem* Util_GetMapItemFromIndex(void* ptr, u32 index)
{
   if (!Util_ArrayIsValid(ptr))
      return NULL;

   Array* array = ARRAY_HEADER(ptr);
   if (index > array->length) // removed items go to back of map/array, right after its canon length.
      return NULL;

   return (MapItem*)(array->data + index * array->size);
}

static inline u32 Util_GetMapItemIndex(void* ptr, const char* key)
{
   if (key == NULL)
      return INVALID_INDEX_U32;

   for (u32 item_i = 0; item_i < Util_ArrayLength(ptr); item_i++)
   {
      MapItem* map_item = Util_GetMapItemFromIndex(ptr, item_i);
      if (map_item->key != NULL && strncmp(map_item->key, key, 512) == 0)
         return item_i;

   }

   return INVALID_INDEX_U32;
}

static inline void* Util_GetMapItem(void* ptr, const char* key)
{
   u32 index = Util_GetMapItemIndex(ptr, key);

   MapItem* map_item =  Util_GetMapItemFromIndex(ptr, index);

   if (map_item != NULL)
      return map_item->value;

   return NULL;
}

static inline void Util_FreeMap(void* ptr)
{
   if (!Util_ArrayIsValid(ptr))
      return;

   for (u32 item_i = 0; item_i < Util_ArrayLength(ptr); item_i++)
   {
      MapItem* map_item = Util_GetMapItemFromIndex(ptr, item_i);
      if (map_item->key != NULL)
         free(map_item->key);

   }

   FREE_ARRAY(ptr);

}

static inline void* Util_AddMapItem(void** array_ptr, const char* key, const void* value)
{
   if (!Util_ArrayPtrIsValid(array_ptr) || key == NULL)
      return NULL;

   uS size = Util_ArrayTypeSize(*array_ptr) - sizeof(MapItem);
   u32 length = Util_ArrayLength(*array_ptr);

   MapItem* map_item = Util_GetMapItem(*array_ptr, key);
   if (map_item == NULL)
   {
      Util_SetArrayLength(array_ptr, length + 1);
      map_item = Util_GetMapItemFromIndex(*array_ptr, length);

   }

   if (map_item != NULL && value != NULL)
      memcpy(map_item->value, value, size);

   if (map_item != NULL)
   {
      uS string_size = strnlen(key, 512) + 1;
      map_item->key = calloc(string_size, sizeof(char));
      memcpy(map_item->key, key, string_size);

      return map_item->value;
   }

   return NULL;
}

static inline void* Util_RemoveMapItem(void** array_ptr, const char* key)
{
   if (!Util_ArrayPtrIsValid(array_ptr) || key == NULL)
      return NULL;

   u32 index = Util_GetMapItemIndex(*array_ptr, key);
   if (index == INVALID_INDEX_U32)
      return NULL;


   Util_RemoveArrayIndex(array_ptr, index);
   MapItem* map_item = Util_GetMapItemFromIndex(*array_ptr, index);

   // free the key so we can avoid losing track of allocated memory.
   free(map_item->key);

   // set key to NULL, ensure we pass key == NULL comparisons and dont fail due to invalid memory.
   map_item->key = NULL;

   return map_item->value;
}

#endif

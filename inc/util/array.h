#ifndef ECT_ARRAY_H
#define ECT_ARRAY_H

#include "ect_types.h"

#include <stdalign.h>
#include <stddef.h>
#include <stdlib.h>

#ifndef clz
   //#define clz __builtin_clzll
   #define clz LeadingZeros
#endif

#define ARRAY_HEADER(ptr) ((Array*)((ptr) - offsetof(Array, data)))

#define NEW_ARRAY_N(T, N) Util_CreateArrayOfLength(N, alignof(T))
#define NEW_ARRAY(T) NEW_ARRAY_N(T, 0u)
#define FREE_ARRAY(ptr) Util_ArrayFree(REF(ptr))
#define INSERT_ARRAY(ptr, i, item) (Util_InsertArrayIndex(REF(ptr), i), ptr[Util_UsableArrayIndex((ptr), i)] = item)
#define REMOVE_ARRAY(ptr, i) (Util_RemoveArrayIndex(REF(ptr), i), ptr[Util_ArrayLength(ptr)])
#define ADD_BACK_ARRAY(ptr, item) INSERT_ARRAY(ptr, Util_ArrayLength(ptr), item)
#define ADD_FRONT_ARRAY(ptr, item) INSERT_ARRAY(ptr, 0u, item)
#define POP_BACK_ARRAY(ptr) REMOVE_ARRAY(ptr, Util_ArrayLength(ptr) - 1u)
#define POP_FRONT_ARRAY(ptr) REMOVE_ARRAY(ptr, 0u)

enum {
   ERR_ARRAY_REALLOC_FAILED = 1
};

#define ERR_ARRAY_INDEX_OVER (1u>>0u)

typedef struct Array_t
{
   uS size;
   uS memory;
   u32 length;
   error err;
   u8 data[];
} Array;

uS LeadingZeros(uS x);
uS Log2i(uS x);
uS Pow2i(uS x);

uS Util_ArrayNeededMemory(uS length);
uS Util_ArrayNeededBytes(uS memory, uS type_size);

void* Util_CreateArrayOfLength(u32 length, uS type_size);
void Util_ReallocArray(void** array_ptr, u32 desired_length);
void Util_InsertArrayIndex(void** array_ptr, u32 index);
void Util_RemoveArrayIndex(void** array_ptr, u32 index);

u32 Util_UsableArrayIndex(void* ptr, u32 index);

static inline uS Util_ArrayTypeSize(void* ptr)
{
   return ARRAY_HEADER(ptr)->size;
}

static inline uS Util_ArrayMemory(void* ptr)
{
   return ARRAY_HEADER(ptr)->memory;
}

static inline u32 Util_ArrayLength(void* ptr)
{
   return ARRAY_HEADER(ptr)->length;
}

static inline error Util_ArrayError(void* array)
{
   return ARRAY_HEADER(array)->err;
}

static inline void Util_ArrayFree(void** array_ptr)
{
   Array* array = ARRAY_HEADER(*array_ptr);
   free(array);
   *array_ptr = NULL;
}

#endif

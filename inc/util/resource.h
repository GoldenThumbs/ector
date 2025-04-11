#ifndef UTIL_RESOURCE_H
#define UTIL_RESOURCE_H

#include "util/types.h"

handle Util_AddResource(u16* ref_counter, void** array_ptr, const void* item_ptr);

#endif

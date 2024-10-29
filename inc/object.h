#ifndef ECT_OBJECT_H
#define ECT_OBJECT_H

#include "ect_types.h"

#define ECT_NONE    0u
#define ECT_FLOAT   1u
#define ECT_INTEGER 2u
#define ECT_STRING  3u
#define ECT_RETURNNONE    4u
#define ECT_RETURNFLOAT   5u
#define ECT_RETURNINTEGER 6u
#define ECT_RETURNSTRING  7u

typedef struct EctField_t
{
    union {
        u8* data;
    };
    union {
        u8 bytes[4];
        struct {
            u8 type_enum;
            u8 size;
        };
    } header;
} EctField;

#endif

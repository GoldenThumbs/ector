#ifndef ECT_TYPES_H
#define ECT_TYPES_H

#include <stdint.h>

// Denotes a pointer that is a hashmap.
#define HM(T) T*

// Denotes a pointer that is a dynamic array.
#define AR(T) T*

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float  f32;
typedef double f64;

typedef union vec2_t
{
    f32 arr[2];
    struct
    { f32 x, y; };
} vec2;

typedef union vec3_t
{
    f32 arr[3];
    struct {
        f32 x;
        union {
            vec2 v_yz;
            struct { f32 y, z; };
        };
    };
    vec2 v_xy;
} vec3;

typedef union vec4_t
{
    f32 arr[4];
    struct {
        vec2 v_xy;
        vec2 v_zw;
    };
    struct {
        f32 x;
        union {
            vec3 v_yzw;
            struct { f32 y, z, w; };
        };
    };
    vec3 v_xyz;
} vec4;

typedef vec4 quat;

typedef union mat3x3_t
{
    f32 arr[9];
    f32 m[3][3];
    vec3 v[3];
} mat3x3;

typedef union mat4x4_t
{
    f32 arr[16];
    f32 m[4][4];
    vec4 v[4];
} mat4x4;

typedef struct spatial3d_t
{
    vec3 origin;
    quat rotation;
    vec3 scale;
} spatial3d;

typedef struct bbox_t
{
    vec3 center;
    vec3 extents;
} bbox;

#endif

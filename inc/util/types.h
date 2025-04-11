#ifndef ECT_TYPES_H
#define ECT_TYPES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define ECT_STRINGIFY(x) ECT_STRINGIFY_2(x)
#define ECT_STRINGIFY_2(x) #x

#define DATABLOB(data) (memblob) { (data), sizeof((data)) }

#define VEC2(...) (vec2)   { { __VA_ARGS__ } }
#define VEC3(...) (vec3)   { { __VA_ARGS__ } }
#define VEC4(...) (vec4)   { { __VA_ARGS__ } }
#define QUAT(...) (quat)   { { __VA_ARGS__ } }
#define MAT3(...) (mat3x3) { { __VA_ARGS__ } }
#define MAT4(...) (mat4x4) { { __VA_ARGS__ } }

#define REF(ptr) (void**)(&(ptr))

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef size_t uS;

typedef float  f32;
typedef double f64;

typedef union color8_t
{
   u8 arr[4];
   u32 hex;
   struct {
      u8 r, g, b, a;
   };
} color8;

enum {
   ERR_OK = 0,
   ERR_WARN,
   ERR_ERROR,
   ERR_FATAL
};

#define ERR_EXTRA_NONE ERR_OK

typedef union error_t
{
   u32 total_bits;
   struct {
       u32 flags: 14;
       u32 general: 2;
       u32 extra: 16;
   };
} error;

typedef union handle_t
{
   u32 id;
   struct {
      u16 handle;
      u16 ref;
   };
} handle;

typedef struct memblob_t
{
   void* data;
   uS size;
} memblob;

typedef struct size2i_t
{
   i32 width, height;
} size2i;

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
         vec2 yz;
         struct { f32 y, z; };
      };
   };
   vec2 xy;
   struct {
      f32 r, g, b;
   };
} vec3;

typedef union vec4_t
{
   f32 arr[4];
   struct {
      vec2 xy;
      vec2 zw;
   };
   struct {
      f32 x;
      union {
         vec2 yz;
         vec3 yzw;
         struct { f32 y, z, w; };
      };
   };
   vec3 xyz;
   struct {
      f32 r, g, b, a;
   };
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

#endif

#ifndef ECT_TYPES_H
#define ECT_TYPES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
#include <stdlib.h>
#define ECT_PATH_MAX _MAX_PATH
#define ECT_FILE_NAME_MAX _MAX_FNAME
#else
#include <linux/limits.h>
#define ECT_PATH_MAX PATH_MAX
#define ECT_FILE_NAME_MAX NAME_MAX
#endif

#define ECT_NAME_MAX 255

#define ECT_STRINGIFY(x) ECT_STRINGIFY_2(x)
#define ECT_STRINGIFY_2(x) #x

#define INVALID_HANDLE UINT16_MAX
#define INVALID_HANDLE_REF UINT16_MAX
#define INVALID_HANDLE_ID UINT32_MAX

#define DATABLOB(data) (memblob) { (void*)(data), sizeof((data)) }

#define VEC2(...) (vec2)   { { __VA_ARGS__ } }
#define VEC3(...) (vec3)   { { __VA_ARGS__ } }
#define VEC4(...) (vec4)   { { __VA_ARGS__ } }
#define QUAT(...) (quat)   { { __VA_ARGS__ } }
#define MAT3(...) (mat3x3) { { __VA_ARGS__ } }
#define MAT4(...) (mat4x4) { { __VA_ARGS__ } }

#define REF(ptr) (void**)(&(ptr))

#define TOBYTE(x) ((u8)((x) * 255.0f))

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
   struct {
      u8 r, g, b, a;

   };

   u32 hex;
   u8 arr[4];

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

typedef union res2D_t
{
   struct {
      i32 width, height;

   };
   
   i32 arr[2];

} res2D;

typedef union res3D_t
{
   struct {
      i32 width, height, depth;

   };

   res2D width_height;
   i32 arr[3];

} res3D;

typedef union vec2_t
{
   struct
   { f32 x, y; };
   f32 arr[2];
} vec2;

typedef union vec3_t
{
   struct {
      f32 x;
      union {
         vec2 yz;
         struct { f32 y, z; };
      };
   };
   struct {
      f32 r, g, b;
   };
   vec2 xy;
   f32 arr[3];
} vec3;

typedef union vec4_t
{
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
   struct {
      f32 r, g, b, a;
   };
   vec3 xyz;
   f32 arr[4];
} vec4;

typedef vec4 quat;

typedef union mat3x3_t
{
   vec3 v[3];
   f32 m[3][3];
   f32 arr[9];
} mat3x3;

typedef union mat4x4_t
{
   vec4 v[4];
   f32 m[4][4];
   f32 arr[16];
} mat4x4;

static inline color8 Util_IntToColor(u32 hex)
{
   color8 color = { 0 };
   color.r = hex >> 24u;
   color.g = (hex >> 16u) & 255u;
   color.b = (hex >> 8u) & 255u;
   color.a = hex & 255u;
   return color;
}

static inline vec3 Util_FillVec3_XY_Z(vec2 xy, f32 z)
{
   vec3 res = { 0 };
   res.xy = xy;
   res.z = z;
   return res;
}

static inline vec4 Util_FillVec4_XY_ZW(vec2 xy, vec2 zw)
{
   vec4 res = { 0 };
   res.xy = xy;
   res.zw = zw;
   return res;
}

static inline vec4 Util_FillVec4_XYZ_W(vec3 xyz, f32 w)
{
   vec4 res = { 0 };
   res.xyz = xyz;
   res.w = w;
   return res;
}

#endif

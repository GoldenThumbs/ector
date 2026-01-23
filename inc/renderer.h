#ifndef ECT_RENDERER_H
#define ECT_RENDERER_H

#include "util/types.h"
// #include "util/extra_types.h"
#include "graphics.h"

#define RENDERER_MODULE "renderer"

typedef handle Drawable;

typedef struct CameraData_t
{
   mat4x4 mat_view;
   mat4x4 mat_proj;
   mat4x4 mat_invview;
   mat4x4 mat_invproj;
   vec2 u_near_far;
   u32 u_width, u_height;
   vec4 u_proj_info;
} CameraData;

typedef struct ModelData_t
{
   mat4x4 mat_model;
   vec4 mat_normal_model[3];
   mat4x4 mat_invmodel;
   mat4x4 mat_mvp;
   vec4 u_color;
} ModelData;

struct Renderer_t;
typedef void (*DrawableFunc)(struct Renderer_t* renderer, Drawable self);

typedef struct Renderer_t Renderer;

Renderer* Renderer_Init(Graphics* graphics);
void Renderer_Free(Renderer* renderer);

#endif

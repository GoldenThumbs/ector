#include "util/types.h"
#include "util/extra_types.h"
#include "util/array.h"
#include "util/resource.h"
#include "util/math.h"
#include "util/vec3.h"
#include "util/vec4.h"
#include "util/quaternion.h"
#include "util/matrix.h"
#include "graphics.h"
#include "module_glue.h"

#include "renderer.h"
#include "renderer/shaders.h"
#include "renderer/internal.h"

#include <stdlib.h>

Renderer* MOD_InitRenderer(error* err, GraphicsContext* graphics_context)
{
   if ((graphics_context == NULL) || (err->flags == ERR_GFX_CONTEXT_FAILED))
   {
      err->general = ERR_FATAL;
      err->flags |= ERR_FLAG_RENDERER_FAILED;
      return NULL;
   }

   Renderer* renderer = malloc(sizeof(Renderer));
   renderer->graphics_context = graphics_context;
   renderer->objects = NEW_ARRAY(rndr_Object);
   renderer->lights = NEW_ARRAY_N(rndr_Light, RNDR_INIT_LIGHT_COUNT);

   renderer->camera.view = Util_IdentityMat4();
   renderer->camera.proj = Util_IdentityMat4();
   renderer->camera.inv_view = Util_IdentityMat4();
   renderer->camera.inv_proj = Util_IdentityMat4();
   renderer->camera.fov = 36;
   renderer->camera.aspect_ratio = 1;
   renderer->camera.near = 0.1f;
   renderer->camera.far  = 150.0f;

   u32 cluster_size[3] = { RNDR_CLUSTER_X, RNDR_CLUSTER_Y, RNDR_CLUSTER_Z };
   f32 cull_cfg[2] = { 0.01f, 5.0f };

   renderer->gfx.camera_ubo   = Graphics_CreateBuffer(graphics_context, NULL, 1, sizeof(struct rndr_CameraData_s), GFX_DRAWMODE_STREAM, GFX_BUFFERTYPE_UNIFORM);
   renderer->gfx.model_ubo    = Graphics_CreateBuffer(graphics_context, NULL, 1, sizeof(struct rndr_ModelData_s), GFX_DRAWMODE_STREAM, GFX_BUFFERTYPE_UNIFORM);
   renderer->gfx.culling_ubo  = Graphics_CreateBuffer(graphics_context, cull_cfg, 1, sizeof(cull_cfg), GFX_DRAWMODE_STATIC, GFX_BUFFERTYPE_UNIFORM);

   renderer->gfx.cluster_ssbo = Graphics_CreateBuffer(graphics_context, NULL, RNDR_CLUSTER_COUNT, sizeof(rndr_Cluster), GFX_DRAWMODE_STATIC_COPY, GFX_BUFFERTYPE_STORAGE);

   renderer->gfx.light_ssbo   = Graphics_CreateBuffer(graphics_context, renderer->lights, Util_ArrayMemory(renderer->lights), sizeof(rndr_Light), GFX_DRAWMODE_DYNAMIC, GFX_BUFFERTYPE_STORAGE);

   renderer->gfx.cluster_comp = Graphics_CreateComputeShader(graphics_context, rndr_CLUSTER_SHADER_GLSL);
   renderer->gfx.culling_comp = Graphics_CreateComputeShader(graphics_context, rndr_CULLING_SHADER_GLSL);

   Graphics_UseBuffer(renderer->graphics_context, renderer->gfx.camera_ubo, 1);
   Graphics_UseBuffer(renderer->graphics_context, renderer->gfx.model_ubo, 2);

   Graphics_UseBuffer(renderer->graphics_context, renderer->gfx.cluster_ssbo, 1);
   Graphics_UseBuffer(renderer->graphics_context, renderer->gfx.light_ssbo, 2);

   return renderer;
}

void MOD_FreeRenderer(Renderer* renderer)
{
   Graphics_FreeShader(renderer->graphics_context, renderer->gfx.cluster_comp);
   Graphics_FreeShader(renderer->graphics_context, renderer->gfx.culling_comp);
   Graphics_FreeBuffer(renderer->graphics_context, renderer->gfx.camera_ubo);
   Graphics_FreeBuffer(renderer->graphics_context, renderer->gfx.model_ubo);
   Graphics_FreeBuffer(renderer->graphics_context, renderer->gfx.culling_ubo);
   Graphics_FreeBuffer(renderer->graphics_context, renderer->gfx.cluster_ssbo);
   Graphics_FreeBuffer(renderer->graphics_context, renderer->gfx.light_ssbo);

   FREE_ARRAY(renderer->objects);
   FREE_ARRAY(renderer->lights);

   renderer->graphics_context = NULL;

   free(renderer);
}

void Renderer_SetView(Renderer* renderer, mat4x4 view)
{
   renderer->camera.view = view;
   renderer->camera.inv_view = Util_InverseViewMatrix(renderer->camera.view);
}

void Renderer_RenderLit(Renderer* renderer, resolution2d size)
{
   Graphics_Viewport(renderer->graphics_context, size);

   renderer->camera.aspect_ratio = (f32)size.height / (f32)size.width;
   renderer->camera.proj = Util_PerspectiveMatrix(
      renderer->camera.fov,
      renderer->camera.aspect_ratio,
      renderer->camera.near,
      renderer->camera.far
   );
   renderer->camera.inv_proj = Util_InversePerspectiveMatrix(renderer->camera.proj);

   struct rndr_CameraData_s cam_data = {
      .view = renderer->camera.view,
      .proj = renderer->camera.proj,
      .inv_view = renderer->camera.inv_view,
      .inv_proj = renderer->camera.inv_proj,
      .near = renderer->camera.near,
      .far = renderer->camera.far,
      .width = (u32)size.width,
      .height = (u32)size.height,
      .cluster_size = { RNDR_CLUSTER_X, RNDR_CLUSTER_Y, RNDR_CLUSTER_Z, 0 }
   };

   Graphics_UpdateBuffer(renderer->graphics_context, renderer->gfx.camera_ubo, (void*)&cam_data, 1, sizeof(struct rndr_CameraData_s));

   Graphics_Dispatch(renderer->graphics_context, renderer->gfx.cluster_comp, RNDR_CLUSTER_X, RNDR_CLUSTER_Y, RNDR_CLUSTER_Z, (Uniforms){ .count = 0 });
   Graphics_DispatchBarrier();

   Graphics_Dispatch(renderer->graphics_context, renderer->gfx.culling_comp, RNDR_CLUSTER_COUNT / 128u, 1, 1,
      (Uniforms){
         .blocks[0] = { .binding = 3, renderer->gfx.culling_ubo },
         .count = 1
   });
   Graphics_DispatchBarrier();

   u32 object_count = Util_ArrayLength(renderer->objects);
   for (u32 i=0; i<object_count; i++)
   {
      rndr_Object object = renderer->objects[i];

      struct rndr_ModelData_s model_data = {
         .model = object.matrix.model,
         .inv_model = object.matrix.inv_model,
         .normal = {
            [0] = Util_VecF32Vec4(object.matrix.normal.v[0], 0),
            [1] = Util_VecF32Vec4(object.matrix.normal.v[1], 0),
            [2] = Util_VecF32Vec4(object.matrix.normal.v[2], 0)
         }
      };
      Graphics_UpdateBuffer(renderer->graphics_context, renderer->gfx.model_ubo, (void*)&model_data, 1, sizeof(struct rndr_ModelData_s));

      Graphics_Draw(renderer->graphics_context, object.shader, object.geometry, object.uniforms);
   }
}

Object Renderer_AddObject(Renderer* renderer, ObjectDesc* desc, Transform3D transform)
{
   if (desc == NULL)
      desc = &(ObjectDesc){ 0 };

   mat3x3 rot = Util_QuatToMat3(transform.rotation);
   vec3 origin = transform.origin;

   mat3x3 inv_rot = Util_TransposeMat3(rot);
   vec3 inv_origin = Util_ScaleVec3(Util_MulMat3Vec3(inv_rot, transform.origin),-1);

   mat3x3 scale = MAT3(
      transform.scale.x, 0, 0,
      0, transform.scale.y, 0,
      0, 0, transform.scale.z
   );

   rot = Util_MulMat3(
      scale,
      rot
   );

   inv_rot = Util_MulMat3(
      Util_InverseDiagonalMat3(scale),
      inv_rot
   );

   rndr_Object object = { 0 };
   object.compare.ref = renderer->ref;
   object.shader = desc->shader;
   object.geometry = desc->geometry;
   object.uniforms = desc->uniforms;
   object.bounds = desc->bounds;
   object.aabb = Util_ResizeBBox(desc->bounds, rot);

   object.matrix.model = MAT4(
      rot.m[0][0], rot.m[0][1], rot.m[0][2], 0,
      rot.m[1][0], rot.m[1][1], rot.m[1][2], 0,
      rot.m[2][0], rot.m[2][1], rot.m[2][2], 0,
      origin.x, origin.y, origin.z, 1
   );

   object.matrix.inv_model = MAT4(
      inv_rot.m[0][0], inv_rot.m[0][1], inv_rot.m[0][2], 0,
      inv_rot.m[1][0], inv_rot.m[1][1], inv_rot.m[1][2], 0,
      inv_rot.m[2][0], inv_rot.m[2][1], inv_rot.m[2][2], 0,
      inv_origin.x, inv_origin.y, inv_origin.z, 1
   );

   object.matrix.normal = Util_TransposeMat3(inv_rot);

   return Util_AddResource(&renderer->ref, REF(renderer->objects), &object);
}

void Renderer_RemoveObject(Renderer* renderer, Object res_object)
{
   rndr_Object object = renderer->objects[res_object.handle];
   if (object.compare.ref != res_object.ref)
      return;

   REMOVE_ARRAY(renderer->objects, (u32)res_object.handle);
}

Light Renderer_AddLight(Renderer* renderer, LightDesc* desc)
{
   if (desc == NULL)
      desc = &(LightDesc){ 0 };

   rndr_Light light = { 0 };
   light.origin = desc->origin;
   light.radius = desc->radius;
   light.rotation = (M_ABS(Util_MaxElmVec4(desc->rotation)) > M_FLOAT_FUZZ) ? desc->rotation : Util_IdentityQuat();
   light.color = Util_MakeRGBE(Util_ScaleVec3(desc->color, desc->strength));
   light.cos_half_angle = TOBYTE(M_COS(desc->cone_angle * 0.5f) * 0.5f + 0.5f);
   light.softness = TOBYTE(desc->softness_fac);
   light.fade_weight = 0;
   light.light_type = (u8)desc->light_type;
   light.importance_bias = desc->importance.bias;
   light.importance_scale = desc->importance.scale;

   u32 light_count = Util_ArrayLength(renderer->lights) + 1;
   bool too_large = ((1 + light_count) >= Util_ArrayMemory(renderer->lights));
   
   ADD_BACK_ARRAY(renderer->lights, light);
   
   if (too_large)
   {
      Graphics_ReuseBuffer(
         renderer->graphics_context,
         renderer->lights,
         Util_ArrayMemory(renderer->lights),
         sizeof(rndr_Light),
         renderer->gfx.light_ssbo
      );
      Graphics_UpdateBuffer(renderer->graphics_context, renderer->gfx.light_ssbo, renderer->lights, Util_ArrayLength(renderer->lights), sizeof(rndr_Light));
   } else
      Graphics_UpdateBuffer(renderer->graphics_context, renderer->gfx.light_ssbo, renderer->lights, Util_ArrayLength(renderer->lights), sizeof(rndr_Light));

   return (Light){ .id = light_count - 1 }; // ignoring ref number stuff for lights
}

void Renderer_RemoveLight(Renderer* renderer, Light res_light)
{
   // NOTE: this function shifts the elements in the array around
   // TODO: look into maintaining a list of free lights instead
   REMOVE_ARRAY(renderer->lights, res_light.id);

   u32 light_count = Util_ArrayLength(renderer->lights);
   Graphics_UpdateBuffer(renderer->graphics_context, renderer->gfx.light_ssbo, renderer->lights, light_count, sizeof(rndr_Light));
}

Shader Renderer_LitShader(GraphicsContext* graphics_context)
{
   return Graphics_CreateShader(graphics_context, rndr_LIT_VRTSHADER_GLSL, rndr_LIT_FRGSHADER_GLSL);
}

#include "ect_math.h"
#include "ect_types.h"
// #include "ect_math.h"

#include "core/engine.h"
#include "core/renderer.h"

#include <stb_ds.h>
#include <sokol_gfx.h>
#include <sokol_log.h>

#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

#define ECT_VIEW_BLOCKSIZE 16

struct EctRenderer_t
{
   struct {
      AR(EctView) available;
      EctView* active;
   } view;
   struct {
      sg_pixel_format gui;
      sg_pixel_format color;
      sg_pixel_format depth;
      i32 msaa_samples;
   } framebuffer;
   vec2 render_scale;
};

EctRenderer* EctRendererInit(void)
{
   EctRenderer* renderer = (EctRenderer*)calloc(1, sizeof(EctRenderer));
   renderer->view.available = NULL;
   renderer->view.active = NULL;
   renderer->framebuffer.gui = SG_PIXELFORMAT_RGBA8;
   renderer->framebuffer.color = SG_PIXELFORMAT_RGBA8;
   renderer->framebuffer.depth = SG_PIXELFORMAT_DEPTH_STENCIL;
   renderer->framebuffer.msaa_samples = 1;
   renderer->render_scale = VEC2(1, 1);

   sg_environment env = (sg_environment) {
      .defaults = {
         .color_format = renderer->framebuffer.color,
         .depth_format = renderer->framebuffer.depth,
         .sample_count = renderer->framebuffer.msaa_samples
      }
   };

   sg_setup(&(sg_desc) {
      .environment = env,
      .logger.func = slog_func
   });

   assert(sg_isvalid());

   return renderer;
}

void EctRendererFree(EctRenderer* renderer)
{
   sg_shutdown();
   free(renderer);
}

frame EctRendererGetSize(EctRenderer* renderer)
{
   frame r_size = EctGetSize();
   vec2 r_sizef = VEC2(r_size.width, r_size.height);
   r_sizef = EctMulVec2(r_sizef, renderer->render_scale);

   return (frame){ (i32)r_sizef.x, (i32)r_sizef.y };
}

sg_swapchain EctRendererGetSwapchain(EctRenderer* renderer)
{
   frame r_size = EctGetSize();
   sg_swapchain res = (sg_swapchain) {
      .width = r_size.width,
      .height = r_size.height,
      .sample_count = renderer->framebuffer.msaa_samples,
      .color_format = renderer->framebuffer.color,
      .depth_format = renderer->framebuffer.depth,
      .gl.framebuffer = 0
   };

   return res;
}

EctGeometry EctRendererCreateGeometry(const EctBuffer vertices, const EctBuffer indices)
{
   sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){
      .data = (sg_range){
         .ptr = vertices.data,
         .size = (u64)vertices.size * (u64)vertices.count
      }
   });

   sg_buffer ibuf = sg_make_buffer(&(sg_buffer_desc){
      .type = SG_BUFFERTYPE_INDEXBUFFER,
      .data = (sg_range){
         .ptr = indices.data,
         .size = (u64)indices.size * (u64)indices.count
      }
   });

   return (EctGeometry){
      .bind = {
         .vertex_buffers[0] = vbuf,
         .index_buffer = ibuf
      },
      .vertex_count = vertices.count,
      .vertex_size = vertices.size,
      .index_count = indices.count,
      .index_size = indices.size
   };
}

void EctRendererDrawGeometry(EctGeometry geometry)
{
   sg_apply_bindings(&geometry.bind);
   sg_draw(0, geometry.index_count, 1);
}

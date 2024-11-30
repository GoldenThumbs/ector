#ifndef ECT_RENDERER_H
#define ECT_RENDERER_H

#include "ect_types.h"

typedef struct EctRenderer_t
{
   struct {
      vec4 gui_frame;
      u32 render_width;
      u32 render_height;
      u8 fb_format;
      u8 gui_fb_format;
      u8 quality;
      u8 gui_quality;
   };
   void* internal;
} EctRenderer;

#endif

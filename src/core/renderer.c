#include "ect_types.h"
// #include "ect_math.h"

// #include "core/renderer.h"

#include <sokol_gfx.h>
#include <sokol_log.h>

typedef struct ECT_RendererInternal_t
{
   mat4x4 view_proj;
   i32 msaa_samples;
   sg_pixel_format out_pixelformat;
   sg_pixel_format depth_pixelformat;
   sg_pixel_format gui_pixelformat;
} ECT_RendererInternal;



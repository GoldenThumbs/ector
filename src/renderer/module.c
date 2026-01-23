#include "util/types.h"
#include "util/array.h"
// #include "util/extra_types.h"
#include "graphics.h"

#include "renderer/default_shaders.h"

#include "renderer.h"
#include "renderer/internal.h"

#include <stdlib.h>
#include <string.h>

Renderer* Renderer_Init(Graphics* graphics)
{
    if (graphics == NULL)
        return NULL;

    Renderer* renderer = malloc(sizeof(Renderer));
    if (renderer == NULL)
        return NULL;

    renderer->graphics = graphics;

    renderer->point_lights = NEW_ARRAY_N(rndr_PointLightSource, 16);
    renderer->drawable_types = NEW_ARRAY_N(rndr_DrawableType, 8);

    renderer->built_in.texture.white = RNDR_LoadColorTexture(renderer->graphics, (color8){ .hex = 0XFFFFFFFF }, GFX_TEXTURETYPE_2D);
    renderer->built_in.texture.black = RNDR_LoadColorTexture(renderer->graphics, (color8){ .hex = 0xFF000000 }, GFX_TEXTURETYPE_2D);
    renderer->built_in.texture.gray = RNDR_LoadColorTexture(renderer->graphics, (color8){ .hex = 0xFF888888 }, GFX_TEXTURETYPE_2D);
    renderer->built_in.texture.normal = RNDR_LoadColorTexture(renderer->graphics, (color8){ .hex = 0xFFFF8888 }, GFX_TEXTURETYPE_2D);

    renderer->built_in.geometry.plane = RNDR_Plane(graphics);
    renderer->built_in.geometry.cube = RNDR_Box(graphics);

    renderer->built_in.shader.basic = Graphics_CreateShader(graphics, builtin_vertex_code, builtin_fragment_code);

    renderer->cluster_dimensions.x = RNDR_CLUSTER_X;
    renderer->cluster_dimensions.y = RNDR_CLUSTER_Y;
    renderer->cluster_dimensions.z = RNDR_CLUSTER_Z;

    u32 xy_cluster_count = renderer->cluster_dimensions.x * renderer->cluster_dimensions.y;
    renderer->cluster_dimensions.total_clusters = xy_cluster_count * renderer->cluster_dimensions.z;
    renderer->clusters = calloc((uS)renderer->cluster_dimensions.total_clusters, sizeof(rndr_Cluster));

    for (u32 z_i = 0; z_i < renderer->cluster_dimensions.z; z_i++)
        for (u32 y_i = 0; y_i < renderer->cluster_dimensions.y; y_i++)
            for (u32 x_i = 0; x_i < renderer->cluster_dimensions.x; x_i++)
    {
        u32 cluster_idx = z_i * xy_cluster_count + y_i * renderer->cluster_dimensions.x + x_i;
        renderer->clusters[cluster_idx].frustum_idx[0] = x_i;
        renderer->clusters[cluster_idx].frustum_idx[1] = y_i;
        renderer->clusters[cluster_idx].frustum_idx[2] = z_i;
    }

    return renderer;
}

void Renderer_Free(Renderer* renderer)
{

}

Texture RNDR_LoadColorTexture(Graphics* graphics, color8 color, u8 texture_type)
{
   return Graphics_CreateTexture(graphics, color.arr, (TextureDesc){ { 1, 1 }, 1, 1, texture_type, GFX_TEXTUREFORMAT_RGBA_U8_NORM });
}

Geometry RNDR_Plane(Graphics* graphics)
{
   Mesh plane_mesh = Mesh_CreatePlane(1, 1, VEC2(2, 2));
   Geometry plane = Graphics_CreateGeometry(graphics, plane_mesh, GFX_DRAWMODE_STATIC);
   Mesh_Free(&plane_mesh);
   return plane;
}

Geometry RNDR_Box(Graphics* graphics)
{
   Mesh box_mesh = Mesh_CreateBoxAdvanced(1, 1, 1, VEC3(2, 2, 2), false);
   Geometry box = Graphics_CreateGeometry(graphics, box_mesh, GFX_DRAWMODE_STATIC);
   Mesh_Free(&box_mesh);
   return box;
}

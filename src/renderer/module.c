#include "graphics.h"

#include "renderer.h"
#include "renderer/shaders.h"

Shader Renderer_LitShader(GraphicsContext *context)
{
   return Graphics_CreateShader(context, rndr_LIT_VRTSHADER_GLSL, rndr_LIT_FRGSHADER_GLSL);
}

Shader Renderer_ClusterShader(GraphicsContext *context)
{
   return Graphics_CreateComputeShader(context, rndr_CLUSTER_SHADER_GLSL);
}

Shader Renderer_CullShader(GraphicsContext *context)
{
   return Graphics_CreateComputeShader(context, rndr_CULL_SHADER_GLSL);
}

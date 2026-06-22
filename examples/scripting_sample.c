#include "image.h"
#include "util/files.h"
#include <stdlib.h>
#include <util/types.h>
#include <util/matrix.h>
#include <util/keymap.h>
#include <mesh.h>
#include <engine.h>
#include <graphics.h>
#include <scripting.h>
#include <default_modules.h>

const char* VertShaderCode(void);
const char* FragShaderCode(void);

int main(int argc, char* argv[])
{
   Engine* engine = Engine_Init(
      argc, argv,
      &(EngineDesc){ .app_name = "Scripting Sample", .window.title = "Ector Sample - Scripting Sample" }
   );

   Engine_RegisterModule(engine, Module_Graphics());
   Engine_RegisterModule(engine, Module_Scripting());

   mat4x4 m = Util_RotationMatrix(VEC3(1, 0, 0), -50.0f);
   m = Util_MulMat4(Util_TranslationMatrix(VEC3(0, 0, -1)), m);

   Graphics* graphics = Engine_FetchModule(engine, GRAPHICS_MODULE);
   Graphics_SetClearColor(graphics, (color8){ 25, 25, 25, 255 });

   Shader shader = Graphics_CreateShader(graphics, VertShaderCode(), FragShaderCode());
   Buffer buffer = Graphics_CreateBufferExplicit(graphics, m.arr, sizeof(mat4x4), GFX_DRAWMODE_STATIC, GFX_BUFFERTYPE_UNIFORM);

   memblob image_data = Util_LoadFileFromBasePath(Engine_GetAppPath(engine), "assets/textures/ector_logo_large.png", true);
   Image image = Image_CreateImage(image_data, IMG_TYPE_2D, (res2D){ 0 }, true);
   Mesh mesh = Mesh_CreatePlane(1, 1, VEC2(1, 1));
   Image_GenerateMipmaps(&image);

   Geometry geometry = Graphics_CreateGeometry(graphics, mesh, GFX_DRAWMODE_STATIC);
   Texture texture = Graphics_CreateTexture(graphics, image.data, (TextureDesc){
      .size = image.size.width_height,
      .depth = image.size.depth,
      .mipmap_count = image.mipmap_count,
      .texture_format = GFX_TEXTUREFORMAT_SRGB_ALPHA,
      .texture_type = GFX_TEXTURETYPE_2D
   });

   ScriptHandler* script_handler = Engine_FetchModule(engine, SCRIPTING_MODULE);
   Script script_init = Scripting_LoadScriptFromFile(script_handler, "assets/scripts/scripting_sample_init.lua");
   Script script_update = Scripting_LoadScriptFromFile(script_handler, "assets/scripts/scripting_sample_update.lua");

   Scripting_RunScript(script_handler, script_init);
   while(!Engine_CheckExitConditions(engine))
   {
      Scripting_RunScript(script_handler, script_update);

      res2D size = Engine_GetFrameSize(engine);
      Graphics_Viewport(graphics, size);
      Graphics_Clear(graphics);

      f32 aspect = (f32)size.height / (f32)size.width;
      mat4x4 m2 = Util_PerspectiveMatrix(50, aspect, 0.05f, 10.0f);
      // m2.m[0][0] = aspect;
      m2 = Util_MulMat4(m2, m);
      Graphics_UpdateBufferExplicit(graphics, buffer, m2.arr, 0, sizeof(mat4x4));

      Graphics_BindTexture(graphics, texture, 0);
      Graphics_Draw(graphics, shader, geometry, (UniformBlockList){
         .count = 1,
         .blocks[0] = {
            .binding = 0,
            .size = sizeof(mat4x4),
            .ubo = buffer
         }
      });

      Engine_Present(engine);

   }

   free(image_data.data);
   Image_Free(&image);
   Mesh_Free(&mesh);

   Engine_Free(engine);
   return 0;
}

const char* VertShaderCode(void)
{
   const char* shader_code =
      "#version 430 core\n"
      "layout(location=0) in vec3 vrt_position;\n"
      "layout(location=2) in vec2 vrt_texcoord;\n"
      "layout(std140, binding=0) uniform UBO\n{\nmat4 mat_mvp;\n };\n"
      "out vec2 v2f_texcoord;\n"
      "void main()\n{\n"
      "v2f_texcoord = vrt_texcoord;\n"
      "gl_Position = mat_mvp * vec4(vrt_position, 1.0);\n"
      "}\n"
   ;

   return shader_code;
}

const char* FragShaderCode(void)
{
   const char* shader_code =
      "#version 430 core\n"
      "layout(binding=0) uniform sampler2D tex_color;\n"
      "in vec2 v2f_texcoord;\n"
      "out vec4 frg_color;\n"
      "void main()\n{\n"
      "frg_color = texture(tex_color, v2f_texcoord);\n"
      "frg_color.rgb *= frg_color.a;\n"
      "}\n"
   ;

   return shader_code;
}

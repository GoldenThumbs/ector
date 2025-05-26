#include "renderer.h"
#include "util/types.h"
#include "engine.h"
#include "graphics.h"

#include "default_modules.h"

#include "module/glue.h"

#include <string.h>

Module Module_Graphics(void)
{
   Module mod_graphics = {
      .name = GRAPHICS_MODULE,
      .mod_init = MOD_GraphicsInit,
      .mod_free = MOD_GraphicsFree
   };

   return mod_graphics;
}

Module Module_Renderer(void)
{
   Module mod_renderer = {
      .name = RENDERER_MODULE,
      .mod_init = MOD_RendererInit,
      .mod_free = MOD_RendererFree
   };

   return mod_renderer;
}

void Module_Defaults(Engine* engine, u32 exclude_count, char* module_excludes[])
{
   const u32 default_count = 2;
   const Module mod_defaults[] = {
      Module_Graphics(),
      Module_Renderer()
   };

   for (u32 i_default=0; i_default<default_count; i_default++)
   {
      Module mod_default = mod_defaults[i_default];

      bool exclude_this_module = false;
      for (u32 i_exclude=0; i_exclude<exclude_count; i_exclude++)
      {
         char* exclude_name =  module_excludes[i_exclude];
         exclude_this_module = (strncmp(mod_default.name, exclude_name, 64) == 0);
         if (exclude_this_module)
            break;
      }

      if (!exclude_this_module)
         Engine_RegisterModule(engine, mod_default);
   }
}

error MOD_GraphicsInit(Module* self, Engine* engine)
{
   self->data = Graphics_Init();

   if (self->data == NULL)
   {
      error res = { .general = ERR_FATAL };
      res.flags = ERR_FLAG_GRAPHICS_FAILED;
      return res;
   }
   return (error){ .general = ERR_OK };
}

error MOD_GraphicsFree(Module* self, Engine* engine)
{
   Graphics_Free((Graphics*)self->data);

   return (error){ .general = ERR_OK };
}

error MOD_RendererInit(Module* self, Engine* engine)
{
   Graphics* graphics = (Graphics*)Engine_FetchModule(engine, "graphics");

   if (graphics == NULL)
   {
      error res = { .general = ERR_FATAL };
      res.flags = ERR_FLAG_GRAPHICS_FAILED & ERR_FLAG_RENDERER_FAILED;
      return res;
   }

   self->data = Renderer_Init(graphics);

   if (self->data == NULL)
   {
      error res = { .general = ERR_FATAL };
      res.flags = ERR_FLAG_RENDERER_FAILED;
      return res;
   }

   return (error){ .general = ERR_OK };
}

error MOD_RendererFree(Module* self, Engine* engine)
{
   Renderer_Free((Renderer*)self->data);

   return (error){ .general = ERR_OK };
}

#include "renderer.h"
#include "util/types.h"
#include "engine.h"
#include "graphics.h"
#include "physics.h"

#include "module/glue.h"

void MOD_DefaultModules(Engine* engine)
{
   Module m_graphics = {
      .name = "gfx",
      .mod_init = MOD_GraphicsInit,
      .mod_free = MOD_GraphicsFree,
   };
   Engine_RegisterModule(engine, m_graphics);

   Module m_renderer = {
      .name = "rndr",
      .mod_init = MOD_RendererInit,
      .mod_free = MOD_RendererFree,
   };
   Engine_RegisterModule(engine, m_renderer);

   Module m_physics = {
      .name = "phys",
      .mod_init = MOD_PhysicsInit,
      .mod_free = MOD_PhysicsFree,
   };
   Engine_RegisterModule(engine, m_physics);
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
   Graphics_Free((GraphicsContext*)self->data);

   return (error){ .general = ERR_OK };
}

error MOD_RendererInit(Module* self, Engine* engine)
{
   GraphicsContext* gfx = (GraphicsContext*)Engine_FetchModule(engine, "gfx");

   if (gfx == NULL)
   {
      error res = { .general = ERR_FATAL };
      res.flags = ERR_FLAG_GRAPHICS_FAILED & ERR_FLAG_RENDERER_FAILED;
      return res;
   }

   self->data = Renderer_Init(gfx);

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

error MOD_PhysicsInit(Module* self, Engine* engine)
{
   self->data = Physics_Init();

   if (self->data == NULL)
   {
      error res = { .general = ERR_FATAL };
      res.flags = ERR_FLAG_GRAPHICS_FAILED;
      return res;
   }
   return (error){ .general = ERR_OK };
}

error MOD_PhysicsFree(Module* self, Engine* engine)
{
   Physics_Free((PhysicsWorld*)self->data);

   return (error){ .general = ERR_OK };
}

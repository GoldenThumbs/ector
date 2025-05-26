#include <util/types.h>
#include <util/keymap.h>
#include <engine.h>
#include <graphics.h>
#include <default_modules.h>

int main(int argc, char* argv[])
{
   Engine* engine = Engine_Init(
      &(EngineDesc){ .app_name = "Game", .window.title = "Game Window" }
   );

   Engine_RegisterModule(engine, Module_Graphics());

   Graphics* graphics = Engine_FetchModule(engine, "graphics");
   Graphics_SetClearColor(graphics, (color8){ 15, 15, 15, 255 });

   while(!Engine_CheckExitConditions(engine))
   {
      if (Engine_CheckKey(engine, KEY_ESCAPE, KEY_IS_DOWN))
         Engine_RequestExit(engine);

      resolution2d size = Engine_GetFrameSize(engine);
      Graphics_Viewport(graphics, size);
      
      Engine_Present(engine);
   }

   Engine_Free(engine);
   return 0;
}

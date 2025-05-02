#include <util/types.h>
#include <util/keymap.h>
#include <engine.h>
#include <graphics.h>

int main(int argc, char* argv[])
{
   Engine* engine = Engine_Init(
      &(EngineDesc){ .app_name = "Game", .window.title = "Game Window" }
   );

   GraphicsContext* gfx = Engine_FetchModule(engine, "gfx");
   Graphics_SetClearColor(gfx, (color8){ 15, 15, 15, 255 });

   while(!Engine_CheckExitConditions(engine))
   {
      if (Engine_CheckKey(engine, KEY_ESCAPE, KEY_IS_DOWN))
         Engine_RequestExit(engine);

      resolution2d size = Engine_GetFrameSize(engine);
      Graphics_Viewport(gfx, size);
      
      Engine_Present(engine);
   }

   Engine_Free(engine);
   return 0;
}

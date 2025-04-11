#include <util/types.h>
#include <util/keymap.h>
#include <engine.h>
#include <graphics.h>

int main(int argc, char* argv[])
{
   Engine* engine = Engine_Init(
      &(EngineDesc){ .app_name = "Game", .window.title = "Game Window" }
   );

   GraphicsContext* gfx = Engine_GraphicsContext(engine);
   Graphics_SetClearColor(gfx, (color8){ 127, 127, 255, 255 });

   while(!Engine_ShouldQuit(engine))
   {
      if (Engine_CheckKey(engine, KEY_ESCAPE, KEY_IS_DOWN))
         Engine_Quit(engine);

      size2i size = Engine_GetSize(engine);
      Graphics_Viewport(gfx, size);
      
      Engine_Present(engine);
   }

   Engine_Free(engine);
   return 0;
}

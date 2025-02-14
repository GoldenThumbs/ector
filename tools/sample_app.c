#include <util/types.h>
#include <util/math.h>
#include <util/keymap.h>
#include <engine.h>

int main(int argc, char* argv[])
{
   Engine* engine = Engine_Init(
      &(EngineDesc){ .app_name = "Game", .window.title = "Game Window" }
   );

   while(!Engine_ShouldQuit(engine))
   {
      if (Engine_CheckKey(engine, KEY_ESCAPE, KEY_IS_DOWN))
         Engine_Quit(engine);

      Engine_Present(engine);
   }

   Engine_Free(engine);
   return 0;
}

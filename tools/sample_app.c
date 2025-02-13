#include <util/types.h>
#include <util/math.h>
#include <util/keymap.h>
#include <core/engine.h>

int main(int argc, char* argv[])
{
   Engine* engine = Engine_Init(
      &(EngineDesc){ 0 }
   );

   while(!Engine_ShouldQuit(engine))
   {
      if (Engine_CheckKey(engine, KEY_ESCAPE, KEY_IS_DOWN))
         Engine_Quit(engine);

      Engine_Render(engine);
   }

   Engine_Free(engine);
   return 0;
}

#include <util/types.h>
#include <util/keymap.h>
#include <engine.h>
#include <graphics.h>
#include <scripting.h>
#include <default_modules.h>

int main(int argc, char* argv[])
{
   Engine* engine = Engine_Init(
      argc, argv,
      &(EngineDesc){ .app_name = "Game", .window.title = "Game Window" }
   );

   Engine_RegisterModule(engine, Module_Graphics());
   Engine_RegisterModule(engine, Module_Scripting());

   Graphics* graphics = Engine_FetchModule(engine, GRAPHICS_MODULE);
   Graphics_SetClearColor(graphics, (color8){ 127, 100, 10, 255 });

   ScriptHandler* script_handler = Engine_FetchModule(engine, SCRIPTING_MODULE);
   Script script = Scripting_LoadScriptFromFile(script_handler, "assets/scripts/scripting_sample.lua");

   while(!Engine_CheckExitConditions(engine))
   {
      Scripting_RunScript(script_handler, script);

      res2D size = Engine_GetFrameSize(engine);
      Graphics_Viewport(graphics, size);
      Graphics_Clear(graphics);

      Engine_Present(engine);

   }

   Engine_Free(engine);
   return 0;
}

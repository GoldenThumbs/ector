-- check every frame if the escape key has been pressed...
if Ector.Input.GetKeyPressed(Ector.Input.Key.KEY_ESCAPE) then
   -- if it was pressed, tell Ector to exit!
   Ector.RequestExit()
   io.write("Requesting exit from Lua...\n")
end

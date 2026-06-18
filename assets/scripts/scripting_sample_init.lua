io.write("Welcome to the Ector scripting sample app!\n")
io.write("This script runs once just before the main loop...\n")
io.write("The script \"scripting_sample_init.lua\" runs every frame!\n\n")

-- write some app info to the console
io.write("Application Path: " .. Ector.GetAppPath() .. "\n")
io.write("Application Name: " .. Ector.GetAppName() .. "\n\n")

-- set window title
Ector.SetWindowTitle("The window title can be changed from Lua too!")

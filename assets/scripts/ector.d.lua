---@meta

--- Returns the sign of x.
---@generic Number:number
---@param x Number
---@return Number
function math.sign(x) end

--- Clamps x between min and max.
---@generic Number:number
---@param x Number
---@param min? Number
---@param max? Number
---@return Number
function math.clamp(x, min, max) end

--- Namespace of all Ector functionality.
---@class Ector
Ector = {}

--- Request application to exit and close the window.
function Ector.RequestExit() end

--- Get the size of the default framebuffer.
---@return Ector.Vector frame_size # size of framebuffer, stored in a vector,
function Ector.GetFrameSize() end

--- Get the amount of seconds it took to render the last frame.
---@return number frame_delta
function Ector.GetFrameDelta() end

--- Get the name of the application.
---@return string
function Ector.GetAppName() end

--- Get the path of the application.
---@return string
function Ector.GetAppPath() end

--- Get the title of the current window.
---@return string
function Ector.GetWindowTitle() end

--- Set the name of the application.
---@param app_name string
function Ector.SetAppName(app_name) end

--- Set the title of the current window.
---@param window_title string
function Ector.SetWindowTitle(window_title) end

--- Convert an angle in radians to turns
---@param angle_radians number
---@return number
function Ector.RadiansToTurns(angle_radians) end

--- Convert an angle in turns to radians
---@param angle_turns number
---@return number
function Ector.TurnsToRadians(angle_turns) end

--- Namespace of input-related functionality.
---@class Ector.Input
Ector.Input = {}

--- Check if the specified mouse_button is currently held down.
---@param mouse_button EctorMouseButton
---@return boolean
function Ector.Input.GetMouseButtonDown(mouse_button) end

--- Check if the specified mouse button was just pressed.
---@param mouse_button EctorMouseButton
---@return boolean
function Ector.Input.GetMouseButtonPressed(mouse_button) end

--- Check if the specified mouse button was just released.
---@param mouse_button EctorMouseButton
---@return boolean
function Ector.Input.GetMouseButtonReleased(mouse_button) end

--- Check if the specified key is currently held down.
---@param key EctorKey
---@return boolean
function Ector.Input.GetKeyDown(key) end

--- Check if the specified key was just pressed.
---@param key EctorKey
---@return boolean
function Ector.Input.GetKeyPressed(key) end

--- Check if the specified key was just released.
---@param key EctorKey
---@return boolean
function Ector.Input.GetKeyReleased(key) end

--- Get the position of the mouse cursor in pixels.
---@return Ector.Vector # the position of the mouse cursor on screen, stored in a vector,
function Ector.Input.GetMousePosition() end

--- Get how far the mouse has moved between this frame and the previous.
---@return Ector.Vector # how far the mouse has moved, stored in a vector.
function Ector.Input.GetMouseDelta() end

--- Get the x and y scroll wheel values from the mouse.
---@return Ector.Vector # scroll values from the mouse scroll wheel, stored in a vector.
function Ector.Input.GetMouseScroll() end

--- Set how the mouse cursor is handled.
---@param mouse_mode EctorMouseMode
function Ector.Input.SetMouseMode(mouse_mode) end

--- Enable/Disable raw mouse input.
---@param use_raw_input boolean
function Ector.Input.SetRawMouseInput(use_raw_input) end

--- Table of Ector lua modules.
---@class Ector.Module
Ector.Module = {}

--- Four-component vector type.
---@class Ector.Vector
---@field x number # X component of vector.
---@field y number # Y component of vector.
---@field z number # Z component of vector.
---@field w number # W component of vector.
---@operator add(Ector.Vector): Ector.Vector
---@operator add(number): Ector.Vector
---@operator sub(Ector.Vector): Ector.Vector
---@operator sub(number): Ector.Vector
---@operator mul(Ector.Vector): Ector.Vector
---@operator mul(number): Ector.Vector
---@operator div(Ector.Vector): Ector.Vector
---@operator div(number): Ector.Vector
---@operator unm(Ector.Vector): Ector.Vector
---@operator len(Ector.Vector): number
Ector.Vector = {}

--- Create a new vector.
---@param x? number
---@param y? number
---@param z? number
---@param w? number
---@return Ector.Vector
function Ector.Vector:New(x, y, z, w) end

--- Get a vector component by index.
---@param index integer
---@return number
function Ector.Vector:Index(index) end

--- Normalize a vector.
---@return Ector.Vector
function Ector.Vector:Normalize() end

--- Get the cross product between two vectors.
---@param b Ector.Vector
---@return Ector.Vector
function Ector.Vector:Cross(b) end

--- Get the dot product between two vectors.
---@param b Ector.Vector
---@return number
function Ector.Vector:Dot(b) end

--- Linearly interpolate two vectors.
---@param b Ector.Vector
---@param fac number
---@return Ector.Vector
function Ector.Vector:Lerp(b, fac) end


--- Create a duplicate of a vector.
---@return Ector.Vector
function Ector.Vector:Dup() end

--- Get the overall rotation angle of a quaternion-vector along its axis.
--- NOTE: returned angle is in turns, not radians.
---@return number
function Ector.Vector:QuatAngle() end

--- Create a new quaternion-vector, using euler angles as input.
--- When no input is given returns an identity quaternion.
--- NOTE: angles are expected in turns, not radians.
---@param euler? Ector.Vector
---@return Ector.Vector quaternion
function Ector.Vector:NewQuat(euler) end

--- Create a new quaternion-vector, using axis-angle as input.
--- NOTE: angles are expected in turns, not radians.
---@param axis Ector.Vector
---@param angle number
---@return Ector.Vector quaternion
function Ector.Vector:NewQuat(axis, angle) end

--- Get inverse of a quaternion-vector.
---@return Ector.Vector quaternion
function Ector.Vector:InverseQuat() end

--- Rotate a vector with a quaternion-vector.
---@param rotation Ector.Vector # quaternion-based rotation.
---@return Ector.Vector rotated vector
function Ector.Vector:Rotate(rotation) end

--- Multiply two quaternion-vectors.
---@param b Ector.Vector # quaternion
---@return Ector.Vector quaternion
function Ector.Vector:MulQuat(b) end

--- Interpolate two quaternion-vectors using the shortest available path.
---@param b Ector.Vector # quaternion
---@param fac number
---@return Ector.Vector quaternion
function Ector.Vector:Slerp(b, fac) end

--- Make a quaternion-vector looking towards a given target from a given origin.
---@param origin Ector.Vector
---@param target Ector.Vector
---@param front? Ector.Vector
---@param up? Ector.Vector
---@return Ector.Vector quaternion
function Ector.Vector:MakeLookingAt(origin, target, front, up) end

--- Make quaternion-vector relative to another.
---@param b Ector.Vector # quaternion
---@return Ector.Vector quaternion
function Ector.Vector:RelativeToQuat(b) end

---@class EctorMouseButton integer

---@class EctorMouseMode integer

---@class EctorKey integer

--- Mouse buttons and options
---
---@enum
---@class Ector.Input.Mouse
---@field MODE_DEFAULT EctorMouseMode
---@field MODE_HIDE_CURSOR EctorMouseMode
---@field MODE_DISABLE_CURSOR EctorMouseMode
---@field MODE_CAPTURE_CURSOR EctorMouseMode
---@field LEFT_CLICK EctorMouseButton
---@field RIGHT_CLICK EctorMouseButton
---@field MIDDLE_CLICK EctorMouseButton
---@field BUTTON_4 EctorMouseButton
---@field BUTTON_5 EctorMouseButton
---@field BUTTON_6 EctorMouseButton
---@field BUTTON_7 EctorMouseButton
---@field BUTTON_8 EctorMouseButton
Ector.Input.Mouse = {}

--- Keyboard keys
---
---@enum
---@class Ector.Input.Key
---@field KEY_APOSTROPHE EctorKey
---@field KEY_COMMA EctorKey
---@field KEY_MINUS EctorKey
---@field KEY_PERIOD EctorKey
---@field KEY_SLASH EctorKey
---@field KEY_0 EctorKey
---@field KEY_1 EctorKey
---@field KEY_2 EctorKey
---@field KEY_3 EctorKey
---@field KEY_4 EctorKey
---@field KEY_5 EctorKey
---@field KEY_6 EctorKey
---@field KEY_7 EctorKey
---@field KEY_8 EctorKey
---@field KEY_9 EctorKey
---@field KEY_SEMICOLON EctorKey
---@field KEY_EQUAL EctorKey
---@field KEY_A EctorKey
---@field KEY_B EctorKey
---@field KEY_C EctorKey
---@field KEY_D EctorKey
---@field KEY_E EctorKey
---@field KEY_F EctorKey
---@field KEY_G EctorKey
---@field KEY_H EctorKey
---@field KEY_I EctorKey
---@field KEY_J EctorKey
---@field KEY_K EctorKey
---@field KEY_L EctorKey
---@field KEY_M EctorKey
---@field KEY_N EctorKey
---@field KEY_O EctorKey
---@field KEY_P EctorKey
---@field KEY_Q EctorKey
---@field KEY_R EctorKey
---@field KEY_S EctorKey
---@field KEY_T EctorKey
---@field KEY_U EctorKey
---@field KEY_V EctorKey
---@field KEY_W EctorKey
---@field KEY_X EctorKey
---@field KEY_Y EctorKey
---@field KEY_Z EctorKey
---@field KEY_LEFT_BRACKET EctorKey
---@field KEY_BACKSLASH EctorKey
---@field KEY_RIGHT_BRACKET EctorKey
---@field KEY_GRAVE_ACCENT EctorKey
---@field KEY_SPACE EctorKey
---@field KEY_ESCAPE EctorKey
---@field KEY_ENTER EctorKey
---@field KEY_TAB EctorKey
---@field KEY_BACKSPACE EctorKey
---@field KEY_INSERT EctorKey
---@field KEY_DELETE EctorKey
---@field KEY_RIGHT EctorKey
---@field KEY_LEFT EctorKey
---@field KEY_DOWN EctorKey
---@field KEY_UP EctorKey
---@field KEY_PAGE_UP EctorKey
---@field KEY_PAGE_DOWN EctorKey
---@field KEY_HOME EctorKey
---@field KEY_END EctorKey
---@field KEY_CAPS_LOCK EctorKey
---@field KEY_SCROLL_LOCK EctorKey
---@field KEY_NUM_LOCK EctorKey
---@field KEY_PRINT_SCREEN EctorKey
---@field KEY_PAUSE EctorKey
---@field KEY_F1 EctorKey
---@field KEY_F2 EctorKey
---@field KEY_F3 EctorKey
---@field KEY_F4 EctorKey
---@field KEY_F5 EctorKey
---@field KEY_F6 EctorKey
---@field KEY_F7 EctorKey
---@field KEY_F8 EctorKey
---@field KEY_F9 EctorKey
---@field KEY_F10 EctorKey
---@field KEY_F11 EctorKey
---@field KEY_F12 EctorKey
---@field KEY_F13 EctorKey
---@field KEY_F14 EctorKey
---@field KEY_F15 EctorKey
---@field KEY_F16 EctorKey
---@field KEY_F17 EctorKey
---@field KEY_F18 EctorKey
---@field KEY_F19 EctorKey
---@field KEY_F20 EctorKey
---@field KEY_F21 EctorKey
---@field KEY_F22 EctorKey
---@field KEY_F23 EctorKey
---@field KEY_F24 EctorKey
---@field KEY_F25 EctorKey
---@field KEY_LEFT_SHIFT EctorKey
---@field KEY_LEFT_CONTROL EctorKey
---@field KEY_LEFT_ALT EctorKey
---@field KEY_LEFT_SUPER EctorKey
---@field KEY_RIGHT_SHIFT EctorKey
---@field KEY_RIGHT_CONTROL EctorKey
---@field KEY_RIGHT_ALT EctorKey
---@field KEY_RIGHT_SUPER EctorKey
---@field KEY_MENU EctorKey
---@field KEY_KP_0 EctorKey
---@field KEY_KP_1 EctorKey
---@field KEY_KP_2 EctorKey
---@field KEY_KP_3 EctorKey
---@field KEY_KP_4 EctorKey
---@field KEY_KP_5 EctorKey
---@field KEY_KP_6 EctorKey
---@field KEY_KP_7 EctorKey
---@field KEY_KP_8 EctorKey
---@field KEY_KP_9 EctorKey
---@field KEY_KP_DECIMAL EctorKey
---@field KEY_KP_DIVIDE EctorKey
---@field KEY_KP_MULTIPLY EctorKey
---@field KEY_KP_SUBTRACT EctorKey
---@field KEY_KP_ADD EctorKey
---@field KEY_KP_ENTER EctorKey
---@field KEY_KP_EQUAL EctorKey
Ector.Input.Key = {}

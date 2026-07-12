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
Ector.Input.Key = {}
Ector.Input.Mouse = {}

--- Check if the specified mouse_button is currently held down.
---@param mouse_button
---| 'Ector.Input.Mouse.LEFT_CLICK'
---| 'Ector.Input.Mouse.RIGHT_CLICK'
---| 'Ector.Input.Mouse.MIDDLE_CLICK'
---| 'Ector.Input.Mouse.BUTTON_4'
---| 'Ector.Input.Mouse.BUTTON_5'
---| 'Ector.Input.Mouse.BUTTON_6'
---| 'Ector.Input.Mouse.BUTTON_7'
---| 'Ector.Input.Mouse.BUTTON_8'
---@return boolean
function Ector.Input.GetMouseButtonDown(mouse_button) end

--- Check if the specified mouse button was just pressed.
---@param mouse_button
---| 'Ector.Input.Mouse.LEFT_CLICK'
---| 'Ector.Input.Mouse.RIGHT_CLICK'
---| 'Ector.Input.Mouse.MIDDLE_CLICK'
---| 'Ector.Input.Mouse.BUTTON_4'
---| 'Ector.Input.Mouse.BUTTON_5'
---| 'Ector.Input.Mouse.BUTTON_6'
---| 'Ector.Input.Mouse.BUTTON_7'
---| 'Ector.Input.Mouse.BUTTON_8'
---@return boolean
function Ector.Input.GetMouseButtonPressed(mouse_button) end

--- Check if the specified mouse button was just released.
---@param mouse_button
---| 'Ector.Input.Mouse.LEFT_CLICK'
---| 'Ector.Input.Mouse.RIGHT_CLICK'
---| 'Ector.Input.Mouse.MIDDLE_CLICK'
---| 'Ector.Input.Mouse.BUTTON_4'
---| 'Ector.Input.Mouse.BUTTON_5'
---| 'Ector.Input.Mouse.BUTTON_6'
---| 'Ector.Input.Mouse.BUTTON_7'
---| 'Ector.Input.Mouse.BUTTON_8'
---@return boolean
function Ector.Input.GetMouseButtonReleased(mouse_button) end

--- Check if the specified key is currently held down.
---@param key Ector.Input.Key
---@return boolean
function Ector.Input.GetKeyDown(key) end

--- Check if the specified key was just pressed.
---@param key Ector.Input.Key
---@return boolean
function Ector.Input.GetKeyPressed(key) end

--- Check if the specified key was just released.
---@param key Ector.Input.Key
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
---@param mouse_mode
---| 'Ector.Input.Mouse.MODE_DEFAULT'
---| 'Ector.Input.Mouse.MODE_HIDE_CURSOR'
---| 'Ector.Input.Mouse.MODE_DISABLE_CURSOR'
---| 'Ector.Input.Mouse.MODE_CAPTURE_CURSOR'
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

--- Keyboard keys
---
---@alias Ector.Input.Key
---
---| 'KEY_APOSTROPHE'
---
---| 'KEY_COMMA'
---
---| 'KEY_MINUS'
---
---| 'KEY_PERIOD'
---
---| 'KEY_SLASH'
---
---| 'KEY_0'
---
---| 'KEY_1'
---
---| 'KEY_2'
---
---| 'KEY_3'
---
---| 'KEY_4'
---
---| 'KEY_5'
---
---| 'KEY_6'
---
---| 'KEY_7'
---
---| 'KEY_8'
---
---| 'KEY_9'
---
---| 'KEY_SEMICOLON'
---
---| 'KEY_EQUAL'
---
--- A key
---
---| 'KEY_A'
---
--- B key
---
---| 'KEY_B'
---
--- C key
---
---| 'KEY_C'
---
--- D key
---
---| 'KEY_D'
---
--- E key
---
---| 'KEY_E'
---
--- F key
---
---| 'KEY_F'
---
--- G key
---
---| 'KEY_G'
---
--- H key
---
---| 'KEY_H'
---
--- I key
---
---| 'KEY_I'
---
--- J key
---
---| 'KEY_J'
---
--- K key
---
---| 'KEY_K'
---
--- L key
---
---| 'KEY_L'
---
--- M key
---
---| 'KEY_M'
---
--- N key
---
---| 'KEY_N'
---
--- O key
---
---| 'KEY_O'
---
--- P key
---
---| 'KEY_P'
---
--- Q key
---
---| 'KEY_Q'
---
--- R key
---
---| 'KEY_R'
---
--- S key
---
---| 'KEY_S'
---
--- T key
---
---| 'KEY_T'
---
--- U key
---
---| 'KEY_U'
---
--- V key
---
---| 'KEY_V'
---
--- W key
---
---| 'KEY_W'
---
--- X key
---
---| 'KEY_X'
---
--- Y key
---
---| 'KEY_Y'
---
--- Z key
---
---| 'KEY_Z'
---
---| 'KEY_LEFT_BRACKET'
---
---| 'KEY_BACKSLASH'
---
---| 'KEY_RIGHT_BRACKET'
---
---| 'KEY_GRAVE_ACCENT'
---
---| 'KEY_SPACE'
---
---| 'KEY_ESCAPE'
---
---| 'KEY_ENTER'
---
---| 'KEY_TAB'
---
---| 'KEY_BACKSPACE'
---
---| 'KEY_INSERT'
---
---| 'KEY_DELETE'
---
---| 'KEY_RIGHT'
---
---| 'KEY_LEFT'
---
---| 'KEY_DOWN'
---
---| 'KEY_UP'
---
---| 'KEY_PAGE_UP'
---
---| 'KEY_PAGE_DOWN'
---
---| 'KEY_HOME'
---
---| 'KEY_END'
---
---| 'KEY_CAPS_LOCK'
---
---| 'KEY_SCROLL_LOCK'
---
---| 'KEY_NUM_LOCK'
---
---| 'KEY_PRINT_SCREEN'
---
---| 'KEY_PAUSE'
---
---| 'KEY_F1'
---
---| 'KEY_F2'
---
---| 'KEY_F3'
---
---| 'KEY_F4'
---
---| 'KEY_F5'
---
---| 'KEY_F6'
---
---| 'KEY_F7'
---
---| 'KEY_F8'
---
---| 'KEY_F9'
---
---| 'KEY_F10'
---
---| 'KEY_F11'
---
---| 'KEY_F12'
---
---| 'KEY_F13'
---
---| 'KEY_F14'
---
---| 'KEY_F15'
---
---| 'KEY_F16'
---
---| 'KEY_F17'
---
---| 'KEY_F18'
---
---| 'KEY_F19'
---
---| 'KEY_F20'
---
---| 'KEY_F21'
---
---| 'KEY_F22'
---
---| 'KEY_F23'
---
---| 'KEY_F24'
---
---| 'KEY_F25'
---
---| 'KEY_LEFT_SHIFT'
---
---| 'KEY_LEFT_CONTROL'
---
---| 'KEY_LEFT_ALT'
---
---| 'KEY_LEFT_SUPER'
---
---| 'KEY_RIGHT_SHIFT'
---
---| 'KEY_RIGHT_CONTROL'
---
---| 'KEY_RIGHT_ALT'
---
---| 'KEY_RIGHT_SUPER'
---
---| 'KEY_MENU'
---
---| 'KEY_KP_0'
---
---| 'KEY_KP_1'
---
---| 'KEY_KP_2'
---
---| 'KEY_KP_3'
---
---| 'KEY_KP_4'
---
---| 'KEY_KP_5'
---
---| 'KEY_KP_6'
---
---| 'KEY_KP_7'
---
---| 'KEY_KP_8'
---
---| 'KEY_KP_9'
---
---| 'KEY_KP_DECIMAL'
---
---| 'KEY_KP_DIVIDE'
---
---| 'KEY_KP_MULTIPLY'
---
---| 'KEY_KP_SUBTRACT'
---
---| 'KEY_KP_ADD'
---
---| 'KEY_KP_ENTER'
---
---| 'KEY_KP_EQUAL'
---

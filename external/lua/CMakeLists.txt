PROJECT(Lua)

option(LUA_BUILD_INTERPRETER "Build stand-alone Lua interpreter." true)
option(LUA_BUILD_COMPILER "Build Lua bytecode compiler." true)

set(lua_lib_src
   "src/lapi.c"
   "src/lauxlib.c"
   "src/lbaselib.c"
   "src/lcode.c"
   "src/lcorolib.c"
   "src/lctype.c"
   "src/ldblib.c"
   "src/ldebug.c"
   "src/ldo.c"
   "src/ldump.c"
   "src/lfunc.c"
   "src/lgc.c"
   "src/linit.c"
   "src/liolib.c"
   "src/llex.c"
   "src/lmathlib.c"
   "src/lmem.c"
   "src/loadlib.c"
   "src/lobject.c"
   "src/lopcodes.c"
   "src/loslib.c"
   "src/lparser.c"
   "src/lstate.c"
   "src/lstring.c"
   "src/lstrlib.c"
   "src/ltable.c"
   "src/ltablib.c"
   "src/ltm.c"
   "src/lundump.c"
   "src/lutf8lib.c"
   "src/lvm.c"
   "src/lzio.c"
)

add_library(LUA STATIC ${lua_lib_src})
target_compile_features(LUA PRIVATE c_std_11)
target_include_directories(LUA PUBLIC "src/")

find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
   target_link_libraries(LUA PUBLIC ${MATH_LIBRARY})
endif()

if(${LUA_BUILD_INTERPRETER})
   add_executable(LuaInterpeter "src/lua.c")
   target_compile_features(LuaInterpeter PRIVATE c_std_11)
   target_link_libraries(LuaInterpeter PRIVATE LUA)
endif()

if(${LUA_BUILD_COMPILER})
   add_executable(LuaCompiler "src/luac.c")
   target_compile_features(LuaCompiler PRIVATE c_std_11)
   target_link_libraries(LuaCompiler PRIVATE LUA)
endif()

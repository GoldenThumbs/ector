set(src_util
   "util/array.c"
)
append_source(SRC_FILES "${src_util}" "${CMAKE_CURRENT_SOURCE_DIR}")

set(src_core
   "core/engine.c"
   "core/renderer.c"
)
append_source(SRC_FILES "${src_core}" "${CMAKE_CURRENT_SOURCE_DIR}")

set(EctorSrc "${SRC_FILES}" PARENT_SCOPE)

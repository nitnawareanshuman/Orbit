# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "apps/shell/CMakeFiles/orbit-shell_autogen.dir/AutogenUsed.txt"
  "apps/shell/CMakeFiles/orbit-shell_autogen.dir/ParseCache.txt"
  "apps/shell/orbit-shell_autogen"
  )
endif()

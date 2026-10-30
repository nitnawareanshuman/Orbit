# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "apps/filesystem/CMakeFiles/orbit-filesystem_autogen.dir/AutogenUsed.txt"
  "apps/filesystem/CMakeFiles/orbit-filesystem_autogen.dir/ParseCache.txt"
  "apps/filesystem/orbit-filesystem_autogen"
  "apps/shell/CMakeFiles/orbit-shell_autogen.dir/AutogenUsed.txt"
  "apps/shell/CMakeFiles/orbit-shell_autogen.dir/ParseCache.txt"
  "apps/shell/orbit-shell_autogen"
  )
endif()

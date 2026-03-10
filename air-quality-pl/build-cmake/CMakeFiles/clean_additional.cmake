# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "CMakeFiles/air_quality_pl_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/air_quality_pl_autogen.dir/ParseCache.txt"
  "CMakeFiles/air_quality_pl_tests_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/air_quality_pl_tests_autogen.dir/ParseCache.txt"
  "air_quality_pl_autogen"
  "air_quality_pl_tests_autogen"
  )
endif()

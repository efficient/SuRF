cmake_minimum_required (VERSION 2.6)
project (SuRF)

message(STATUS "Configuring..." ${CMAKE_PROJECT_NAME})

# Code coverage test config
# include("${CMAKE_CURRENT_SOURCE_DIR}/CodeCoverage.cmake")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -mpopcnt -pthread -std=c++11")
# append_coverage_compiler_flags()
# set(COVERAGE_EXCLUDES 'ARF/*' 'bench/*' '/usr/include/gtest/*' '/usr/lib/gcc/x86_64-linux-gnu/6/*' '6/*')
# setup_target_for_coverage(
#   NAME coverage
#   EXECUTABLE make test
# )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -Wall -Werror -mpopcnt -pthread -std=c++11")
add_definitions(-DNDEBUG)

enable_testing()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

add_subdirectory(test)
add_subdirectory(bench)

#include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ARF/include")
#add_subdirectory(ARF)

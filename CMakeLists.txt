cmake_minimum_required (VERSION 2.6)
project (SuRF)

message(STATUS "Configuring..." ${CMAKE_PROJECT_NAME})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -Wall -Werror -mpopcnt -pthread -std=c++11")

add_definitions(-DNDEBUG)

enable_testing()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ARF/include")

add_subdirectory(test)
add_subdirectory(bench)
add_subdirectory(ARF)

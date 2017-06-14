cmake_minimum_required (VERSION 2.6)
project (microbench)

message(STATUS "Configuring..." ${CMAKE_PROJECT_NAME})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -Werror -mpopcnt -pthread -std=c++11")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../bloom")

set(COMMON_DEPENDENCIES tcmalloc_minimal pthread)

add_executable(email_mixed_tput email_mixed_tput.cpp)
target_link_libraries(email_mixed_tput ${COMMON_DEPENDENCIES} surf)
add_executable(email_mixed_fp email_mixed_fp.cpp)
target_link_libraries(email_mixed_fp ${COMMON_DEPENDENCIES} surf)


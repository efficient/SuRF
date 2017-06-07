include_directories(${PROJECT_SOURCE_DIR}/src)
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})

function (add_fst_test file_name )
  add_executable(${file_name} ${file_name}.cpp)
  target_link_libraries(${file_name} surf gtest)
  add_test(NAME ${file_name} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${file_name})
endfunction()

add_fst_test(TestSurf)

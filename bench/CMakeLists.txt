add_executable(workload workload.cpp)
target_link_libraries(workload)

add_executable(workload_multi_thread workload_multi_thread.cpp)
target_link_libraries(workload_multi_thread)

#add_executable(workload_arf workload_arf.cpp)
#target_link_libraries(workload_arf ARF)

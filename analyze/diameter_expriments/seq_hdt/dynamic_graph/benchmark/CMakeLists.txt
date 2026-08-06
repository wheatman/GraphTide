# add_executable(benchmark_dynamic_connectivity
#   benchmark_dynamic_connectivity.cpp
# )
# target_include_directories(benchmark_dynamic_connectivity PRIVATE
#   ../include
# )
# target_link_libraries(benchmark_dynamic_connectivity
#   lib_dynamic_connectivity
# )
# add_executable(seq_hdt seq_hdt.cc)
# target_include_directories(seq_hdt PRIVATE
#   ../include
# )
# target_link_libraries(seq_hdt PRIVATE
#   lib_dynamic_connectivity
#   dycon
# )
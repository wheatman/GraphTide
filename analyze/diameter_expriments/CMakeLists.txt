cmake_minimum_required(VERSION 3.16)
project(convert_to_undirect_project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -O3 -g -gdwarf-4)

# Build Abseil as a subproject so we can link its components
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../external/abseil-cpp ${CMAKE_BINARY_DIR}/absl_build)

# Parlaylib and Abseil headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../external/parlaylib/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../external/abseil-cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../external/dynconn/include)

# Build ParlayLib as a subproject so we can link its target
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../external/parlaylib ${CMAKE_BINARY_DIR}/parlay_build)


# Build seq_hdt after dynconn/parlay are available
add_subdirectory(seq_hdt)

find_package(Threads REQUIRED)

add_executable(convert_to_undirect convert_to_undirect.cpp)

# Ensure the IO headers are visible (source includes "../../IO/IO.hpp")
target_include_directories(convert_to_undirect PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../)

# Link the Abseil container target used by flat_hash_map
target_link_libraries(convert_to_undirect PRIVATE absl::flat_hash_map Threads::Threads)

add_executable(shuffle_undirect shuffle_undirect.cpp)

target_include_directories(shuffle_undirect PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../)

target_link_libraries(shuffle_undirect PRIVATE absl::flat_hash_map parlay Threads::Threads)

add_executable(test_dnd test_dnd.cpp)

add_executable(benchmark_dnd benchmark_dnd.cpp)

target_include_directories(benchmark_dnd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../)

target_link_libraries(benchmark_dnd PRIVATE absl::flat_hash_map parlay Threads::Threads)

add_executable(benchmark_cf benchmark_cf.cpp)

target_include_directories(benchmark_cf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../ ${CMAKE_CURRENT_SOURCE_DIR}/../../external/dynconn/include)

target_link_libraries(benchmark_cf PRIVATE  absl::flat_hash_map parlay Threads::Threads)

add_executable(benchmark_hdt benchmark_hdt.cpp)

target_include_directories(benchmark_hdt PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR}/seq_hdt/dynamic_graph/include ${CMAKE_CURRENT_SOURCE_DIR}/../../)

target_link_libraries(benchmark_hdt PRIVATE absl::flat_hash_map parlay Threads::Threads lib_dynamic_connectivity)

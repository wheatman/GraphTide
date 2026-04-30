cmake_minimum_required(VERSION 3.10)
project(graph_statistics)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --------------------------------------------------------------------
# Default build type
# --------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type specified. Defaulting to RelWithDebInfo.")
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the build type." FORCE)
endif()

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")

set(ANALYZE_DEP_INCLUDE_DIRS
    "${graph_statistics_SOURCE_DIR}/../external/parlaylib/include"
    "${graph_statistics_SOURCE_DIR}/../external/BYO"
    "${graph_statistics_SOURCE_DIR}/../external/BYO/external/reducer"
    "${graph_statistics_SOURCE_DIR}/../external/BYO/external/semisort"
    "${graph_statistics_SOURCE_DIR}/../external/dynconn/include"
    "${graph_statistics_SOURCE_DIR}/../external/dynconn/include/dycon/localTree"
)

foreach(dep IN ITEMS
    "${graph_statistics_SOURCE_DIR}/../external/abseil-cpp/CMakeLists.txt"
    "${graph_statistics_SOURCE_DIR}/../external/parlaylib/include/parlay/primitives.h"
    "${graph_statistics_SOURCE_DIR}/../external/BYO/gbbs/gbbs.h"
    "${graph_statistics_SOURCE_DIR}/../external/dynconn/include/dycon/localTree/SCCWN.hpp"
)
  if(NOT EXISTS "${dep}")
    message(FATAL_ERROR
      "Missing external dependency file: ${dep}\n"
      "Initialize dependencies from repo root with:\n"
      "  git submodule update --init --recursive")
  endif()
endforeach()

add_executable(run graph_statistics.cpp)
target_include_directories(run PUBLIC ${ANALYZE_DEP_INCLUDE_DIRS})

add_executable(run_nodes node_statistics.cpp)
target_include_directories(run_nodes PUBLIC ${ANALYZE_DEP_INCLUDE_DIRS})

add_executable(run_edges edge_statistics.cpp)
target_include_directories(run_edges PUBLIC ${ANALYZE_DEP_INCLUDE_DIRS})

add_executable(update_fraction update_fraction.cpp)
target_include_directories(update_fraction PUBLIC ${ANALYZE_DEP_INCLUDE_DIRS})

add_executable(edge_alive edge_alive_statistics.cpp)
target_include_directories(edge_alive PUBLIC ${ANALYZE_DEP_INCLUDE_DIRS})

add_executable(orientation orientation_statistics.cpp)
target_include_directories(orientation PUBLIC ${ANALYZE_DEP_INCLUDE_DIRS})

# Pick the C++ standard to compile with.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

foreach(tgt IN ITEMS run run_nodes run_edges update_fraction edge_alive orientation)
  target_compile_features(${tgt} PUBLIC cxx_std_20)
endforeach()

add_subdirectory("${graph_statistics_SOURCE_DIR}/../external/abseil-cpp" external_abseil_build)


# LTO
include(CheckIPOSupported)
check_ipo_supported(RESULT lto_supported OUTPUT lto_error LANGUAGES CXX)
if(lto_supported)
  set_target_properties(run PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION true
  )
  set_target_properties(run_nodes PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION true
  )
    set_target_properties(run_edges PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION true
  )
endif()

# --------------------------------------------------------------------
# Compiler optimizations and tuning
# --------------------------------------------------------------------
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if(COMPILER_SUPPORTS_MARCH_NATIVE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif()

# Use -O3 for RelWithDebInfo instead of the default -O2
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -gdwarf-4 -DNDEBUG" CACHE STRING "Flags for RelWithDebInfo" FORCE)
elseif(MSVC)
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /Zi /DNDEBUG" CACHE STRING "Flags for RelWithDebInfo" FORCE)
endif()

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
  target_link_libraries(run absl::flat_hash_map absl::flat_hash_set absl::flags absl::flags_parse OpenMP::OpenMP_CXX)
  target_link_libraries(run_nodes absl::flat_hash_map absl::flat_hash_set absl::flags absl::flags_parse OpenMP::OpenMP_CXX)
  target_link_libraries(run_edges absl::flat_hash_map absl::flat_hash_set absl::flags absl::flags_parse OpenMP::OpenMP_CXX)
  target_link_libraries(update_fraction absl::flat_hash_map absl::flat_hash_set absl::flags absl::flags_parse OpenMP::OpenMP_CXX)
  target_link_libraries(edge_alive absl::flat_hash_map absl::flat_hash_set absl::flags absl::flags_parse OpenMP::OpenMP_CXX)
  target_link_libraries(orientation absl::flat_hash_map absl::flat_hash_set absl::flags absl::flags_parse OpenMP::OpenMP_CXX)
  else()
  target_link_libraries(run absl::flat_hash_map absl::flat_hash_set absl::flags absl::flags_parse)
  target_link_libraries(run_nodes absl::flat_hash_map absl::flat_hash_set absl::flags absl::flags_parse)
  target_link_libraries(run_edges absl::flat_hash_map absl::flat_hash_set absl::flags absl::flags_parse)
  target_link_libraries(update_fraction absl::flat_hash_map absl::flat_hash_set absl::flags absl::flags_parse)
  target_link_libraries(edge_alive absl::flat_hash_map absl::flat_hash_set absl::flags absl::flags_parse)
  target_link_libraries(orientation absl::flat_hash_map absl::flat_hash_set absl::flags absl::flags_parse)
endif()


if(CMAKE_CXX_STANDARD LESS 17)
  message(FATAL_ERROR
      "requires CMAKE_CXX_STANDARD >= 17 (got: ${CMAKE_CXX_STANDARD})")
endif()

cmake_minimum_required(VERSION 3.14)
project(graph_statistics)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(FetchContent)

# --------------------------------------------------------------------
# Default build type
# --------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type specified. Defaulting to RelWithDebInfo.")
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the build type." FORCE)
endif()

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")

FetchContent_Declare(
    abseilcpp
    GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
    GIT_TAG 20240116.2
)
FetchContent_MakeAvailable(abseilcpp)

FetchContent_Declare(
    parlaylib
    GIT_REPOSITORY https://github.com/cmuparlay/parlaylib.git
    GIT_TAG 51017699dcc421f80479cdb238d3092233ad0d26
)
FetchContent_GetProperties(parlaylib)
if(NOT parlaylib_POPULATED)
    FetchContent_Populate(parlaylib)
endif()

FetchContent_Declare(
    byo
    GIT_REPOSITORY https://github.com/wheatman/BYO.git
    GIT_TAG 00f8ead3925a3346d4322fd2b6f0b1ba1f1f9834
)
FetchContent_GetProperties(byo)
if(NOT byo_POPULATED)
    FetchContent_Populate(byo)
endif()

FetchContent_Declare(
    dynconn
    GIT_REPOSITORY https://github.com/ParAlg/DynamicConnectivity.git
    GIT_TAG 0707a6539013734fdafbb89f8edbf1000f323ef2
)
FetchContent_GetProperties(dynconn)
if(NOT dynconn_POPULATED)
    FetchContent_Populate(dynconn)
endif()

set(ANALYZE_DEP_INCLUDE_DIRS
    "${parlaylib_SOURCE_DIR}/include"
    "${byo_SOURCE_DIR}"
    "${byo_SOURCE_DIR}/external/reducer"
    "${byo_SOURCE_DIR}/external/semisort"
    "${dynconn_SOURCE_DIR}/include"
    "${dynconn_SOURCE_DIR}/include/dycon/localTree"
)

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

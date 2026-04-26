
cmake_minimum_required(VERSION 3.10)
project(parse_flight_opensky)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-O3 -pthread")

add_subdirectory(abseil-cpp)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(parse parse.cpp)
target_include_directories(parse PUBLIC "${parse_flight_opensky_SOURCE_DIR}/../../external/parlaylib/include")
target_link_libraries(parse absl::flat_hash_map)
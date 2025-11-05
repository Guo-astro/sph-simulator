cmake_minimum_required(VERSION 3.13)

# Load Conan toolchain before project()
get_filename_component(SPH_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../" ABSOLUTE)
if(EXISTS ${SPH_ROOT}/build/conan_toolchain.cmake)
  include(${SPH_ROOT}/build/conan_toolchain.cmake)
endif()

project(khi_workflow)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Paths
set(SPH_INCLUDE_DIR "${SPH_ROOT}/include")
set(SPH_SRC_DIR "${SPH_ROOT}/src")

# Find dependencies
find_package(OpenMP REQUIRED)
find_package(Boost REQUIRED)
find_package(nlohmann_json REQUIRED)

file(GLOB SPH_SOURCES "${SPH_SRC_DIR}/*.cpp")
list(FILTER SPH_SOURCES EXCLUDE REGEX "main\\.cpp$")
list(FILTER SPH_SOURCES EXCLUDE REGEX "main_1d\\.cpp$")
list(FILTER SPH_SOURCES EXCLUDE REGEX "main_2d\\.cpp$")
list(FILTER SPH_SOURCES EXCLUDE REGEX "main_3d\\.cpp$")
list(FILTER SPH_SOURCES EXCLUDE REGEX "solver\\.cpp$")
list(FILTER SPH_SOURCES EXCLUDE REGEX "sample/")

# Add algorithm-specific sources
file(GLOB SPH_DISPH_SOURCES "${SPH_SRC_DIR}/disph/*.cpp")
file(GLOB SPH_GSPH_SOURCES "${SPH_SRC_DIR}/gsph/*.cpp")

# Add core parameter validation/estimation sources
file(GLOB SPH_CORE_SOURCES "${SPH_SRC_DIR}/core/*.cpp")
list(FILTER SPH_CORE_SOURCES EXCLUDE REGEX "plugin_loader\\.cpp$")

# Build plugin as shared library
add_library(khi_plugin SHARED 
    src/plugin.cpp
    ${SPH_SOURCES}
    ${SPH_DISPH_SOURCES}
    ${SPH_GSPH_SOURCES}
    ${SPH_CORE_SOURCES}
)

target_compile_features(khi_plugin PUBLIC cxx_std_17)
target_compile_options(khi_plugin PRIVATE
    -Wall
    -Wno-sign-compare
    -funroll-loops
    -ffast-math
)

target_include_directories(khi_plugin PUBLIC 
    ${SPH_INCLUDE_DIR}
)

target_link_libraries(khi_plugin PUBLIC
    OpenMP::OpenMP_CXX
    Boost::headers
    nlohmann_json::nlohmann_json
)

set_target_properties(khi_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)

# Build standalone SPH executable for this workflow
add_executable(sph2d 
    ${SPH_SRC_DIR}/main_2d.cpp
    ${SPH_SRC_DIR}/solver.cpp
)

target_compile_options(sph2d PRIVATE
    -Wall
    -Wno-sign-compare
    -funroll-loops
    -ffast-math
)

target_include_directories(sph2d PUBLIC 
    ${SPH_INCLUDE_DIR}
)

target_link_libraries(sph2d PUBLIC
    khi_plugin
)

set_target_properties(sph2d PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

message(STATUS "Building khi_plugin for 2D")
message(STATUS "Building local sph2d executable")
message(STATUS "SPH root: ${SPH_ROOT}")

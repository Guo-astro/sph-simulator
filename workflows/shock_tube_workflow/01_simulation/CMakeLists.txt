cmake_minimum_required(VERSION 3.13)

# Load Conan toolchain before project()
get_filename_component(SPH_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../" ABSOLUTE)
if(EXISTS ${SPH_ROOT}/build/conan_toolchain.cmake)
  include(${SPH_ROOT}/build/conan_toolchain.cmake)
endif()

project(shock_tube_workflow)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Paths
set(SPH_INCLUDE_DIR "${SPH_ROOT}/include")
set(SPH_SRC_DIR "${SPH_ROOT}/src")

# Find dependencies
find_package(OpenMP REQUIRED)
find_package(Boost REQUIRED)
find_package(nlohmann_json REQUIRED)

# Collect SPH library sources (excluding main.cpp, solver.cpp, and sample files)
file(GLOB SPH_SOURCES "${SPH_SRC_DIR}/*.cpp")
list(FILTER SPH_SOURCES EXCLUDE REGEX "main\\.cpp$")
list(FILTER SPH_SOURCES EXCLUDE REGEX "solver\\.cpp$")
list(FILTER SPH_SOURCES EXCLUDE REGEX "sample/")

# Add algorithm-specific sources
file(GLOB SPH_DISPH_SOURCES "${SPH_SRC_DIR}/disph/*.cpp")
file(GLOB SPH_GSPH_SOURCES "${SPH_SRC_DIR}/gsph/*.cpp")
file(GLOB SPH_ALGORITHM_SOURCES "${SPH_SRC_DIR}/algorithms/riemann/*.cpp")

# Add core parameter validation/estimation sources (exclude plugin_loader which needs C++17)
file(GLOB SPH_CORE_SOURCES "${SPH_SRC_DIR}/core/*.cpp")
list(FILTER SPH_CORE_SOURCES EXCLUDE REGEX "plugin_loader\\.cpp$")

# Build plugin as shared library
add_library(shock_tube_plugin SHARED 
    src/plugin_enhanced.cpp
    ${SPH_SOURCES}
    ${SPH_DISPH_SOURCES}
    ${SPH_GSPH_SOURCES}
    ${SPH_ALGORITHM_SOURCES}
    ${SPH_CORE_SOURCES}
)

target_compile_features(shock_tube_plugin PUBLIC cxx_std_17)
target_compile_definitions(shock_tube_plugin PRIVATE DIM=1)
target_compile_options(shock_tube_plugin PRIVATE
    -Wall
    -Wno-sign-compare
    -funroll-loops
    -ffast-math
)

target_include_directories(shock_tube_plugin PUBLIC 
    ${SPH_INCLUDE_DIR}
)

target_link_libraries(shock_tube_plugin PUBLIC
    OpenMP::OpenMP_CXX
    Boost::headers
    nlohmann_json::nlohmann_json
)

set_target_properties(shock_tube_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)

# Build standalone SPH executable for this workflow
add_executable(sph 
    ${SPH_SRC_DIR}/main.cpp
    ${SPH_SRC_DIR}/solver.cpp
    ${SPH_SRC_DIR}/core/plugin_loader.cpp
)

target_compile_definitions(sph PRIVATE DIM=1)
target_compile_options(sph PRIVATE
    -Wall
    -Wno-sign-compare
    -funroll-loops
    -ffast-math
)

target_include_directories(sph PUBLIC 
    ${SPH_INCLUDE_DIR}
)

target_link_libraries(sph PUBLIC
    shock_tube_plugin
)

set_target_properties(sph PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

message(STATUS "Building shock_tube_plugin for 1D")
message(STATUS "Building local sph executable")
message(STATUS "SPH root: ${SPH_ROOT}")

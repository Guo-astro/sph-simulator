cmake_minimum_required(VERSION 3.13)

# Load Conan toolchain before project()
get_filename_component(SPH_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../" ABSOLUTE)

# Add SPH root and build directory to CMAKE_PREFIX_PATH so find_package can locate Conan-generated configs
list(APPEND CMAKE_PREFIX_PATH 
    ${SPH_ROOT}
    ${SPH_ROOT}/build
    ${SPH_ROOT}/build_tests
    ${SPH_ROOT}/build_release
)

if(EXISTS ${SPH_ROOT}/build/conan_toolchain.cmake)
  include(${SPH_ROOT}/build/conan_toolchain.cmake)
elseif(EXISTS ${SPH_ROOT}/build_tests/conan_toolchain.cmake)
  include(${SPH_ROOT}/build_tests/conan_toolchain.cmake)
elseif(EXISTS ${SPH_ROOT}/build_release/conan_toolchain.cmake)
  include(${SPH_ROOT}/build_release/conan_toolchain.cmake)
elseif(EXISTS ${SPH_ROOT}/conan_toolchain.cmake)
  include(${SPH_ROOT}/conan_toolchain.cmake)
endif()

project(evrard_workflow)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Paths
set(SPH_INCLUDE_DIR "${SPH_ROOT}/include")
set(SPH_SRC_DIR "${SPH_ROOT}/src")

# Find dependencies
find_package(OpenMP REQUIRED)
find_package(Boost REQUIRED)
find_package(nlohmann_json REQUIRED)

# Collect SPH library sources (excluding main_*.cpp and sample files)
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
file(GLOB SPH_ALGORITHM_SOURCES "${SPH_SRC_DIR}/algorithms/riemann/*.cpp")
file(GLOB SPH_ALGORITHM_REGISTRY_SOURCES "${SPH_SRC_DIR}/core/algorithms/*.cpp")

# Add core parameter validation/estimation sources
file(GLOB SPH_CORE_SOURCES "${SPH_SRC_DIR}/core/*.cpp")
list(FILTER SPH_CORE_SOURCES EXCLUDE REGEX "plugin_loader\\.cpp$")

# Add parameter builder sources (required for type-safe parameter construction)
file(GLOB SPH_PARAMETER_SOURCES "${SPH_SRC_DIR}/core/parameters/*.cpp")

# Add boundary configuration sources
file(GLOB SPH_BOUNDARY_SOURCES "${SPH_SRC_DIR}/core/boundaries/*.cpp")

# Build plugin as shared library
add_library(evrard_plugin SHARED 
    src/plugin.cpp
    ${SPH_SOURCES}
    ${SPH_DISPH_SOURCES}
    ${SPH_GSPH_SOURCES}
    ${SPH_ALGORITHM_SOURCES}
    ${SPH_ALGORITHM_REGISTRY_SOURCES}
    ${SPH_CORE_SOURCES}
    ${SPH_PARAMETER_SOURCES}
    ${SPH_BOUNDARY_SOURCES}
)

target_compile_features(evrard_plugin PUBLIC cxx_std_17)
target_compile_options(evrard_plugin PRIVATE
    -Wall
    -Wno-sign-compare
    -funroll-loops
    -ffast-math
)

target_include_directories(evrard_plugin PUBLIC 
    ${SPH_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
)

target_link_libraries(evrard_plugin PUBLIC
    OpenMP::OpenMP_CXX
    Boost::headers
    Boost::boost
    nlohmann_json::nlohmann_json
)

set_target_properties(evrard_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)

# Build standalone SPH executable for this workflow
add_executable(sph3d 
    ${SPH_SRC_DIR}/main_3d.cpp
    ${SPH_SRC_DIR}/solver.cpp
)

target_compile_options(sph3d PRIVATE
    -Wall
    -Wno-sign-compare
    -funroll-loops
    -ffast-math
)

target_include_directories(sph3d PUBLIC 
    ${SPH_INCLUDE_DIR}
)

target_link_libraries(sph3d PUBLIC
    evrard_plugin
)

set_target_properties(sph3d PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

message(STATUS "Building evrard_plugin for 3D")
message(STATUS "Building local sph3d executable")
message(STATUS "SPH root: ${SPH_ROOT}")

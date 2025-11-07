cmake_minimum_required(VERSION 3.13)

# Load Conan toolchain before project()
get_filename_component(SPH_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../" ABSOLUTE)

# Add SPH root and build directory to CMAKE_PREFIX_PATH so find_package can locate Conan-generated configs
list(APPEND CMAKE_PREFIX_PATH 
    ${SPH_ROOT}
    ${SPH_ROOT}/build
)

if(EXISTS ${SPH_ROOT}/build/conan_toolchain.cmake)
  include(${SPH_ROOT}/build/conan_toolchain.cmake)
elseif(EXISTS ${SPH_ROOT}/conan_toolchain.cmake)
  include(${SPH_ROOT}/conan_toolchain.cmake)
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

# Add core parameter sources
file(GLOB_RECURSE SPH_CORE_SOURCES "${SPH_SRC_DIR}/core/*.cpp")
list(FILTER SPH_CORE_SOURCES EXCLUDE REGEX "plugin_loader\\.cpp$")
list(FILTER SPH_CORE_SOURCES EXCLUDE REGEX "bhtree_templates\\.cpp$")  # Has broken include path

# Build GSPH plugin
add_library(shock_tube_plugin_gsph SHARED 
    src/plugin_gsph.cpp
    ${SPH_SOURCES}
    ${SPH_DISPH_SOURCES}
    ${SPH_GSPH_SOURCES}
    ${SPH_ALGORITHM_SOURCES}
    ${SPH_CORE_SOURCES}
)

target_compile_features(shock_tube_plugin_gsph PUBLIC cxx_std_17)
target_compile_options(shock_tube_plugin_gsph PRIVATE
    -Wall -Wno-sign-compare -funroll-loops -ffast-math
)
target_include_directories(shock_tube_plugin_gsph PUBLIC ${SPH_INCLUDE_DIR})
target_link_libraries(shock_tube_plugin_gsph PUBLIC
    OpenMP::OpenMP_CXX Boost::headers nlohmann_json::nlohmann_json
)
set_target_properties(shock_tube_plugin_gsph PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib"
)

# Build DISPH plugin
add_library(shock_tube_plugin_disph SHARED 
    src/plugin_disph.cpp
    ${SPH_SOURCES}
    ${SPH_DISPH_SOURCES}
    ${SPH_GSPH_SOURCES}
    ${SPH_ALGORITHM_SOURCES}
    ${SPH_CORE_SOURCES}
)

target_compile_features(shock_tube_plugin_disph PUBLIC cxx_std_17)
target_compile_options(shock_tube_plugin_disph PRIVATE
    -Wall -Wno-sign-compare -funroll-loops -ffast-math
)
target_include_directories(shock_tube_plugin_disph PUBLIC ${SPH_INCLUDE_DIR})
target_link_libraries(shock_tube_plugin_disph PUBLIC
    OpenMP::OpenMP_CXX Boost::headers nlohmann_json::nlohmann_json
)
set_target_properties(shock_tube_plugin_disph PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib"
)

# Build Enhanced plugin (for compatibility)
add_library(shock_tube_plugin_enhanced SHARED 
    src/plugin_enhanced.cpp
    ${SPH_SOURCES}
    ${SPH_DISPH_SOURCES}
    ${SPH_GSPH_SOURCES}
    ${SPH_ALGORITHM_SOURCES}
    ${SPH_CORE_SOURCES}
    ${SPH_PARAMETER_SOURCES}
    ${SPH_BOUNDARY_SOURCES}
    ${SPH_ALGORITHM_REGISTRY_SOURCES}
)

target_compile_features(shock_tube_plugin_enhanced PUBLIC cxx_std_17)
target_compile_options(shock_tube_plugin_enhanced PRIVATE
    -Wall -Wno-sign-compare -funroll-loops -ffast-math
)
target_include_directories(shock_tube_plugin_enhanced PUBLIC ${SPH_INCLUDE_DIR})
target_link_libraries(shock_tube_plugin_enhanced PUBLIC
    OpenMP::OpenMP_CXX Boost::headers nlohmann_json::nlohmann_json
)
set_target_properties(shock_tube_plugin_enhanced PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib"
)

# Build Baseline plugin
add_library(shock_tube_plugin_baseline SHARED 
    src/plugin_baseline.cpp
    ${SPH_SOURCES}
    ${SPH_DISPH_SOURCES}
    ${SPH_GSPH_SOURCES}
    ${SPH_ALGORITHM_SOURCES}
    ${SPH_CORE_SOURCES}
    ${SPH_PARAMETER_SOURCES}
    ${SPH_BOUNDARY_SOURCES}
    ${SPH_ALGORITHM_REGISTRY_SOURCES}
)

target_compile_features(shock_tube_plugin_baseline PUBLIC cxx_std_17)
target_compile_options(shock_tube_plugin_baseline PRIVATE
    -Wall -Wno-sign-compare -funroll-loops -ffast-math
)
target_include_directories(shock_tube_plugin_baseline PUBLIC ${SPH_INCLUDE_DIR})
target_link_libraries(shock_tube_plugin_baseline PUBLIC
    OpenMP::OpenMP_CXX Boost::headers nlohmann_json::nlohmann_json
)
set_target_properties(shock_tube_plugin_baseline PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib"
)

# Build Modern plugin
add_library(shock_tube_plugin_modern SHARED 
    src/plugin_modern.cpp
    ${SPH_SOURCES}
    ${SPH_DISPH_SOURCES}
    ${SPH_GSPH_SOURCES}
    ${SPH_ALGORITHM_SOURCES}
    ${SPH_CORE_SOURCES}
    ${SPH_PARAMETER_SOURCES}
    ${SPH_BOUNDARY_SOURCES}
    ${SPH_ALGORITHM_REGISTRY_SOURCES}
)

target_compile_features(shock_tube_plugin_modern PUBLIC cxx_std_17)
target_compile_options(shock_tube_plugin_modern PRIVATE
    -Wall -Wno-sign-compare -funroll-loops -ffast-math
)
target_include_directories(shock_tube_plugin_modern PUBLIC ${SPH_INCLUDE_DIR})
target_link_libraries(shock_tube_plugin_modern PUBLIC
    OpenMP::OpenMP_CXX Boost::headers nlohmann_json::nlohmann_json
)
set_target_properties(shock_tube_plugin_modern PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib"
)

# Build standalone SPH executable for this workflow
add_executable(sph 
    ${SPH_SRC_DIR}/main_1d.cpp
    ${SPH_SRC_DIR}/solver.cpp
)

target_compile_definitions(sph PRIVATE 
    # DEBUG_GHOST_PARTICLES  # Uncomment for ghost particle debug output
    # EXHAUSTIVE_SEARCH_ONLY_FOR_DEBUG  # Uncomment for debugging - tree-based search incompatible with dynamic ghosts
)
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
    shock_tube_plugin_enhanced  # Default to enhanced plugin
)

set_target_properties(sph PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

message(STATUS "Building shock_tube_plugin_gsph for GSPH")
message(STATUS "Building shock_tube_plugin_disph for DISPH")
message(STATUS "Building shock_tube_plugin_enhanced for Enhanced")
message(STATUS "Building local sph executable")
message(STATUS "SPH root: ${SPH_ROOT}")

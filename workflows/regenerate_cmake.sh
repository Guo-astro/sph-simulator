#!/bin/bash

# Generate proper CMakeLists.txt for all workflow plugins

WORKFLOWS_DIR="/Users/guo/sph-simulation/workflows"

# Function to create CMakeLists.txt
create_cmake() {
    local plugin_name=$1
    local dim=$2
    local workflow_dir=$3
    
    cat > "${workflow_dir}/CMakeLists.txt" << 'EOF'
cmake_minimum_required(VERSION 3.13)

# Load Conan toolchain before project()
get_filename_component(SPH_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../" ABSOLUTE)
if(EXISTS ${SPH_ROOT}/build/conan_toolchain.cmake)
  include(${SPH_ROOT}/build/conan_toolchain.cmake)
endif()

EOF

    cat >> "${workflow_dir}/CMakeLists.txt" << EOF
project(${plugin_name}_plugin)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Paths
set(SPH_INCLUDE_DIR "\${SPH_ROOT}/include")
set(SPH_SRC_DIR "\${SPH_ROOT}/src")

# Define dimension
add_compile_definitions(DIM=${dim})

# Find dependencies
find_package(OpenMP REQUIRED)
find_package(Boost REQUIRED)
find_package(nlohmann_json REQUIRED)

# Collect SPH library sources (excluding main.cpp, solver.cpp, and sample files)
file(GLOB SPH_SOURCES "\${SPH_SRC_DIR}/*.cpp")
list(FILTER SPH_SOURCES EXCLUDE REGEX "main\\\\.cpp\$")
list(FILTER SPH_SOURCES EXCLUDE REGEX "solver\\\\.cpp\$")
list(FILTER SPH_SOURCES EXCLUDE REGEX "sample/")

# Add algorithm-specific sources
file(GLOB SPH_DISPH_SOURCES "\${SPH_SRC_DIR}/disph/*.cpp")
file(GLOB SPH_GSPH_SOURCES "\${SPH_SRC_DIR}/gsph/*.cpp")

# Build plugin as shared library
add_library(${plugin_name}_plugin SHARED 
    src/plugin.cpp
    \${SPH_SOURCES}
    \${SPH_DISPH_SOURCES}
    \${SPH_GSPH_SOURCES}
)

target_compile_features(${plugin_name}_plugin PUBLIC cxx_std_14)
target_compile_options(${plugin_name}_plugin PRIVATE
    -Wall
    -Wno-sign-compare
    -funroll-loops
    -ffast-math
)

target_include_directories(${plugin_name}_plugin PUBLIC 
    \${SPH_INCLUDE_DIR}
)

target_link_libraries(${plugin_name}_plugin PUBLIC
    OpenMP::OpenMP_CXX
    Boost::headers
    nlohmann_json::nlohmann_json
)

set_target_properties(${plugin_name}_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "\${CMAKE_CURRENT_SOURCE_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "\${CMAKE_CURRENT_SOURCE_DIR}/bin"
)

message(STATUS "Building ${plugin_name}_plugin for ${dim}D")
message(STATUS "SPH root: \${SPH_ROOT}")
EOF
}

# Create CMakeLists.txt for all workflows
echo "Generating CMakeLists.txt for all workflows..."

create_cmake "shock_tube" "1" "${WORKFLOWS_DIR}/shock_tube_workflow/01_simulation"
create_cmake "evrard" "3" "${WORKFLOWS_DIR}/evrard_workflow/01_simulation"
create_cmake "khi" "2" "${WORKFLOWS_DIR}/khi_workflow/01_simulation"
create_cmake "gresho_chan_vortex" "2" "${WORKFLOWS_DIR}/gresho_chan_vortex_workflow/01_simulation"
create_cmake "pairing_instability" "2" "${WORKFLOWS_DIR}/pairing_instability_workflow/01_simulation"
create_cmake "hydrostatic" "2" "${WORKFLOWS_DIR}/hydrostatic_workflow/01_simulation"

echo "✓ CMakeLists.txt files generated for all workflows"

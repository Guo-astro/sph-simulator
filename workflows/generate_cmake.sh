#!/bin/bash

# Generate CMakeLists.txt for all workflow plugins

WORKFLOWS_DIR="/Users/guo/sph-simulation/workflows"

# Function to create CMakeLists.txt
create_cmake() {
    local plugin_name=$1
    local dim=$2
    local workflow_dir=$3
    
    cat > "${workflow_dir}/CMakeLists.txt" << EOF
cmake_minimum_required(VERSION 3.13)
project(${plugin_name}_plugin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Paths
get_filename_component(SPH_ROOT "\${CMAKE_CURRENT_SOURCE_DIR}/../../../" ABSOLUTE)
set(SPH_INCLUDE_DIR "\${SPH_ROOT}/include")

include_directories(\${SPH_INCLUDE_DIR})

# Define dimension
add_compile_definitions(DIM=${dim})

if(EXISTS \${SPH_ROOT}/build/conan_toolchain.cmake)
  include(\${SPH_ROOT}/build/conan_toolchain.cmake)
endif()

find_package(OpenMP REQUIRED)
find_package(Boost REQUIRED)
find_package(nlohmann_json REQUIRED)

add_library(${plugin_name}_plugin SHARED src/plugin.cpp)

target_compile_features(${plugin_name}_plugin PUBLIC cxx_std_14)
target_compile_options(${plugin_name}_plugin PUBLIC -Wall -Wno-sign-compare -funroll-loops -ffast-math)
target_include_directories(${plugin_name}_plugin PUBLIC \${SPH_INCLUDE_DIR})
target_link_libraries(${plugin_name}_plugin PUBLIC OpenMP::OpenMP_CXX Boost::boost nlohmann_json::nlohmann_json)

set_target_properties(${plugin_name}_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "\${CMAKE_CURRENT_SOURCE_DIR}/build"
)

message(STATUS "Building ${plugin_name}_plugin for ${dim}D")
EOF
}

# Create CMakeLists.txt for each workflow
create_cmake "khi" "2" "${WORKFLOWS_DIR}/khi_workflow/01_simulation"
create_cmake "gresho_chan_vortex" "2" "${WORKFLOWS_DIR}/gresho_chan_vortex_workflow/01_simulation"
create_cmake "pairing_instability" "2" "${WORKFLOWS_DIR}/pairing_instability_workflow/01_simulation"
create_cmake "hydrostatic" "2" "${WORKFLOWS_DIR}/hydrostatic_workflow/01_simulation"

echo "CMakeLists.txt files generated for all workflows"

#!/bin/bash

# Build all workflow plugins with proper configuration

WORKFLOWS_DIR="/Users/guo/sph-simulation/workflows"
SPH_ROOT="/Users/guo/sph-simulation"

echo "=== Building All Workflow Plugins ==="
echo ""

# Function to build a workflow
build_workflow() {
    local name=$1
    local dim=$2
    local workflow_path="${WORKFLOWS_DIR}/${name}_workflow/01_simulation"
    
    echo "Building ${name} workflow (${dim}D)..."
    
    if [ ! -d "$workflow_path" ]; then
        echo "  ERROR: Directory not found: $workflow_path"
        return 1
    fi
    
    cd "$workflow_path"
    
    # Clean previous build
    rm -rf build
    
    # Configure with Release build type
    cmake -B build -S . \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE="${SPH_ROOT}/build/conan_toolchain.cmake" \
        -DCMAKE_CXX_STANDARD=14
    
    if [ $? -ne 0 ]; then
        echo "  ERROR: CMake configuration failed for ${name}"
        return 1
    fi
    
    # Build
    cmake --build build --config Release
    
    if [ $? -eq 0 ]; then
        echo "  ✓ ${name} built successfully"
    else
        echo "  ✗ ${name} build failed"
        return 1
    fi
    
    echo ""
}

# Build each workflow
build_workflow "shock_tube" "1"
build_workflow "evrard" "3"
build_workflow "khi" "2"
build_workflow "gresho_chan_vortex" "2"
build_workflow "hydrostatic" "2"
build_workflow "pairing_instability" "2"

echo "=== Build Complete ==="

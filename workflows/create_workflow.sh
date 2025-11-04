#!/bin/bash
#
# create_workflow.sh - Scaffold a new SPH workflow with complete structure
#
# Usage: ./create_workflow.sh <workflow_name> <workflow_type> [options]
#
# Examples:
#   ./create_workflow.sh my_collision_sim collision
#   ./create_workflow.sh binary_star_merger merger --dim=3 --sph-type=gdisph
#

set -e

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
DIM=3
SPH_TYPE="gdisph"
PLUGIN_NAME=""

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKFLOWS_DIR="$SCRIPT_DIR"

# Help message
show_help() {
    cat << EOF
${BLUE}═══════════════════════════════════════════════════════════════${NC}
${GREEN}SPH Workflow Scaffolding Tool${NC}
${BLUE}═══════════════════════════════════════════════════════════════${NC}

${YELLOW}USAGE:${NC}
    ./create_workflow.sh <workflow_name> <workflow_type> [options]

${YELLOW}ARGUMENTS:${NC}
    workflow_name    Name of the workflow (e.g., my_simulation)
    workflow_type    Type of simulation:
                     - collision     : Cloud-cloud collision
                     - merger        : Binary merger
                     - disk          : Disk relaxation
                     - explosion     : Blast wave (Sedov-Taylor)
                     - shock         : Shock tube
                     - custom        : Custom simulation

${YELLOW}OPTIONS:${NC}
    --dim=<1|2|3>           Dimension (default: 3)
    --sph-type=<type>       SPH type: gdisph, disph, ssph (default: gdisph)
    --plugin-name=<name>    Custom plugin name (default: from workflow_name)
    --multi-step            Create multi-step workflow (01_step1, 02_step2)
    --help                  Show this help message

${YELLOW}EXAMPLES:${NC}
    # Simple 3D GDISPH collision
    ./create_workflow.sh cloud_collision collision

    # 2D disk with custom plugin name
    ./create_workflow.sh keplerian_disk disk --dim=2 --plugin-name=keplerian

    # Multi-step merger workflow
    ./create_workflow.sh binary_merger merger --multi-step

${BLUE}═══════════════════════════════════════════════════════════════${NC}
EOF
}

# Parse arguments
if [ $# -lt 2 ]; then
    show_help
    exit 1
fi

WORKFLOW_NAME="$1"
WORKFLOW_TYPE="$2"
shift 2

MULTI_STEP=false

# Parse options
while [[ $# -gt 0 ]]; do
    case $1 in
        --dim=*)
            DIM="${1#*=}"
            shift
            ;;
        --sph-type=*)
            SPH_TYPE="${1#*=}"
            shift
            ;;
        --plugin-name=*)
            PLUGIN_NAME="${1#*=}"
            shift
            ;;
        --multi-step)
            MULTI_STEP=true
            shift
            ;;
        --help)
            show_help
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

# Set plugin name if not provided
if [ -z "$PLUGIN_NAME" ]; then
    PLUGIN_NAME="$WORKFLOW_NAME"
fi

# Validate inputs
if [[ ! "$DIM" =~ ^[123]$ ]]; then
    echo -e "${RED}Error: --dim must be 1, 2, or 3${NC}"
    exit 1
fi

if [[ ! "$SPH_TYPE" =~ ^(gdisph|disph|ssph)$ ]]; then
    echo -e "${RED}Error: --sph-type must be gdisph, disph, or ssph${NC}"
    exit 1
fi

# Create workflow directory
WORKFLOW_DIR="$WORKFLOWS_DIR/${WORKFLOW_NAME}_workflow"

if [ -d "$WORKFLOW_DIR" ]; then
    echo -e "${RED}Error: Workflow directory already exists: $WORKFLOW_DIR${NC}"
    exit 1
fi

echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}Creating SPH Workflow: $WORKFLOW_NAME${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""
echo -e "${YELLOW}Configuration:${NC}"
echo "  Workflow Name:  $WORKFLOW_NAME"
echo "  Type:           $WORKFLOW_TYPE"
echo "  Dimension:      ${DIM}D"
echo "  SPH Type:       $(echo $SPH_TYPE | tr '[:lower:]' '[:upper:]')"
echo "  Plugin Name:    $PLUGIN_NAME"
echo "  Multi-step:     $MULTI_STEP"
echo "  Location:       $WORKFLOW_DIR"
echo ""

# Function to create directory structure
create_structure() {
    local STEP_DIR=$1
    local STEP_NAME=$2
    
    echo -e "${BLUE}Creating step: $STEP_NAME...${NC}"
    
    mkdir -p "$STEP_DIR"/{config,src,scripts,data,docs,results/{animations,plots,analysis},output,build}
    touch "$STEP_DIR/output/.gitkeep"
    touch "$STEP_DIR/build/.gitkeep"
    
    # Create .gitignore
    cat > "$STEP_DIR/.gitignore" << 'GITIGNORE'
# Build
build/
!build/.gitkeep
*.dylib
*.so
*.o
*.a
CMakeCache.txt
CMakeFiles/
cmake_install.cmake

# Output
output/
!output/.gitkeep

# Temp
*.swp
*~
.DS_Store
__pycache__/
*.pyc
GITIGNORE
    
    echo -e "${GREEN}  ✓ Directory structure created${NC}"
}

# Function to create plugin source file
create_plugin_source() {
    local STEP_DIR=$1
    local STEP_NAME=$2
    
    local PLUGIN_FILE="$STEP_DIR/src/plugin.cpp"
    
    echo -e "${BLUE}Creating plugin source: $PLUGIN_FILE${NC}"
    
    # Get template based on workflow type
    case $WORKFLOW_TYPE in
        collision)
            create_collision_template "$PLUGIN_FILE" "$STEP_NAME"
            ;;
        merger)
            create_merger_template "$PLUGIN_FILE" "$STEP_NAME"
            ;;
        disk)
            create_disk_template "$PLUGIN_FILE" "$STEP_NAME"
            ;;
        explosion)
            create_explosion_template "$PLUGIN_FILE" "$STEP_NAME"
            ;;
        shock)
            create_shock_template "$PLUGIN_FILE" "$STEP_NAME"
            ;;
        custom)
            create_custom_template "$PLUGIN_FILE" "$STEP_NAME"
            ;;
        *)
            create_custom_template "$PLUGIN_FILE" "$STEP_NAME"
            ;;
    esac
    
    echo -e "${GREEN}  ✓ Plugin source created${NC}"
}

# Template: Custom/Generic
create_custom_template() {
    local FILE=$1
    local STEP_NAME=$2
    local SPH_TYPE_UPPER=$(echo ${SPH_TYPE} | tr '[:lower:]' '[:upper:]')
    local WORKFLOW_NAME_UPPER=$(echo ${WORKFLOW_NAME:0:1} | tr '[:lower:]' '[:upper:]')${WORKFLOW_NAME:1}
    local PLUGIN_NAME_UPPER=$(echo ${PLUGIN_NAME:0:1} | tr '[:lower:]' '[:upper:]')${PLUGIN_NAME:1}
    
    cat > "$FILE" << EOF
#include "core/simulation_plugin.hpp"
#include "core/simulation.hpp"
#include "core/parameters.hpp"
#include "core/particle.hpp"
#include "utilities/exception.hpp"
#include "utilities/defines.hpp"
#include <iostream>
#include <cmath>
#include <vector>

namespace sph {

/**
 * @brief ${WORKFLOW_NAME_UPPER} Simulation
 * 
 * TODO: Add description of your simulation
 * 
 * Physics:
 * - TODO: Describe the physics
 * - TODO: List key equations
 * 
 * Initial Conditions:
 * - TODO: Describe initial setup
 */
class ${PLUGIN_NAME_UPPER}Plugin : public SimulationPlugin {
public:
    std::string get_name() const override {
        return "${PLUGIN_NAME}";
    }
    
    std::string get_description() const override {
        return "${WORKFLOW_NAME_UPPER} - ${WORKFLOW_TYPE} simulation";
    }
    
    std::string get_version() const override {
        return "1.0.0";
    }
    
    void initialize(std::shared_ptr<Simulation> sim,
                   std::shared_ptr<SPHParameters> param) override {
#if DIM != $DIM
        THROW_ERROR("${PLUGIN_NAME_UPPER} requires DIM=$DIM");
#endif

        std::cout << "Initializing ${WORKFLOW_NAME_UPPER} simulation...\\n";
        
        // ═══════════════════════════════════════════════════════════
        // TODO: Define simulation parameters
        // ═══════════════════════════════════════════════════════════
        
        const real box_size = 1.0;          // Domain size
        const int particles_per_dim = 50;   // Resolution
        
        // Physical parameters
        const real density0 = 1.0;       // Background density
        const real pressure0 = 1.0;      // Background pressure
        const real gamma = param->physics.gamma;  // Adiabatic index (default 5/3)
        
        // ═══════════════════════════════════════════════════════════
        // TODO: Create particles
        // ═══════════════════════════════════════════════════════════
        
        std::vector<SPHParticle>& particles = sim->get_particles();
        
        const real dx = box_size / particles_per_dim;
        const real mass_per_particle = density0 * std::pow(dx, DIM);
        
        int n = 0;
#if DIM == 1
        // 1D uniform grid
        for (int i = 0; i < particles_per_dim; ++i) {
#elif DIM == 2
        // 2D uniform grid
        for (int i = 0; i < particles_per_dim; ++i) {
            for (int j = 0; j < particles_per_dim; ++j) {
#else // DIM == 3
        // 3D uniform grid
        for (int i = 0; i < particles_per_dim; ++i) {
            for (int j = 0; j < particles_per_dim; ++j) {
                for (int k = 0; k < particles_per_dim; ++k) {
#endif
                    SPHParticle pp;
                    
                    // Position
#if DIM == 1
                    pp.pos[0] = (i + 0.5) * dx;
#elif DIM == 2
                    pp.pos[0] = (i + 0.5) * dx;
                    pp.pos[1] = (j + 0.5) * dx;
#else
                    pp.pos[0] = (i + 0.5) * dx;
                    pp.pos[1] = (j + 0.5) * dx;
                    pp.pos[2] = (k + 0.5) * dx;
#endif
                    
                    // Velocity (initially at rest)
                    pp.vel[0] = 0.0;
                    pp.vel[1] = 0.0;
                    pp.vel[2] = 0.0;
                    
                    // Thermodynamic properties
                    pp.mass = mass_per_particle;
                    pp.dens = density0;
                    pp.pres = pressure0;
                    pp.ene = pressure0 / ((gamma - 1.0) * density0);
                    pp.sml = 2.0 * dx;  // Initial smoothing length
                    pp.id = n;
                    
                    // Initialize volume for DISPH/GDISPH
                    pp.volume = mass_per_particle / density0;
                    
                    particles.push_back(pp);
                    n++;
#if DIM == 3
                }
#endif
#if DIM >= 2
            }
#endif
        }
        
        sim->set_particle_num(n);
        
        std::cout << "Created " << n << " particles\\n";
        std::cout << "  Particle mass:   " << mass_per_particle << "\\n";
        std::cout << "  Smoothing length: " << (2.0 * dx) << "\\n";
        
        // ═══════════════════════════════════════════════════════════
        // TODO: Set simulation parameters
        // ═══════════════════════════════════════════════════════════
        
        param->type = SPHType::${SPH_TYPE_UPPER};
        param->time.end = 1.0;           // End time
        param->time.output = 0.1;        // Snapshot interval
        param->cfl.sound = 0.3;          // CFL factor
        param->physics.neighbor_number = 50;  // Neighbor count
        
        // TODO: Add any perturbations or special initial conditions here
        
        std::cout << "Initialization complete!\\n";
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"${PLUGIN_NAME}.cpp"};
    }
};

} // namespace sph

// Export the plugin
DEFINE_SIMULATION_PLUGIN(sph::${PLUGIN_NAME_UPPER}Plugin)
EOF
}

# Template: Collision
create_collision_template() {
    local FILE=$1
    local STEP_NAME=$2
    
    cat > "$FILE" << 'EOF'
#include "core/simulation_plugin.hpp"
#include "core/simulation.hpp"
#include "core/parameters.hpp"
#include "core/particle.hpp"
#include "utilities/exception.hpp"
#include <iostream>
#include <cmath>

namespace sph {

class CollisionPlugin : public SimulationPlugin {
public:
    std::string get_name() const override { return "collision"; }
    std::string get_description() const override { return "Cloud-cloud collision"; }
    std::string get_version() const override { return "1.0.0"; }
    
    void initialize(std::shared_ptr<Simulation> sim,
                   std::shared_ptr<SPHParameters> param) override {
        std::cout << "Initializing collision simulation...\n";
        
        // TODO: Implement cloud-cloud collision setup
        // - Create two clouds
        // - Set relative velocity
        // - Position clouds for collision
        
        param->type = SPHType::GDISPH;
        param->time_end = 10.0;
        param->output_directory = "output/collision";
        
        std::cout << "Collision setup complete!\n";
    }
};

} // namespace sph

extern "C" {
    sph::SimulationPlugin* create_plugin() { return new sph::CollisionPlugin(); }
    void destroy_plugin(sph::SimulationPlugin* p) { delete p; }
}
EOF
}

# Template: Disk
create_disk_template() {
    local FILE=$1
    create_custom_template "$FILE" "$STEP_NAME"  # Use custom for now, can specialize later
}

# Template: Explosion
create_explosion_template() {
    local FILE=$1
    create_custom_template "$FILE" "$STEP_NAME"  # Use custom for now
}

# Template: Shock
create_shock_template() {
    local FILE=$1
    create_custom_template "$FILE" "$STEP_NAME"  # Use custom for now
}

# Template: Merger
create_merger_template() {
    local FILE=$1
    create_custom_template "$FILE" "$STEP_NAME"  # Use custom for now
}

# Function to create CMakeLists.txt
create_cmakelists() {
    local STEP_DIR=$1
    local STEP_NAME=$2
    
    local CMAKE_FILE="$STEP_DIR/CMakeLists.txt"
    
    echo -e "${BLUE}Creating CMakeLists.txt${NC}"
    
    cat > "$CMAKE_FILE" << EOF
cmake_minimum_required(VERSION 3.23)
project(${PLUGIN_NAME}_plugin)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Paths - adjust based on your workflow depth
get_filename_component(SPH_ROOT "\${CMAKE_CURRENT_SOURCE_DIR}/../../../../" ABSOLUTE)
set(SPH_BUILD_DIR "\${SPH_ROOT}/build")
set(SPH_INCLUDE_DIR "\${SPH_ROOT}/include")

# Include directories
include_directories(\${SPH_INCLUDE_DIR})

# Define dimension
add_compile_definitions(DIM=$DIM)

# Build plugin
add_library(${PLUGIN_NAME}_plugin SHARED src/plugin.cpp)
target_link_libraries(${PLUGIN_NAME}_plugin PRIVATE \${SPH_BUILD_DIR}/libsph_lib.a)

# Set output directory
set_target_properties(${PLUGIN_NAME}_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "\${CMAKE_CURRENT_SOURCE_DIR}/build"
)

# Show build info
message(STATUS "Building ${PLUGIN_NAME}_plugin for ${DIM}D")
message(STATUS "SPH root: \${SPH_ROOT}")
EOF
    
    echo -e "${GREEN}  ✓ CMakeLists.txt created${NC}"
}

# Function to create configuration files
create_configs() {
    local STEP_DIR=$1
    local STEP_NAME=$2
    local WORKFLOW_NAME_UPPER=$(echo ${WORKFLOW_NAME:0:1} | tr '[:lower:]' '[:upper:]')${WORKFLOW_NAME:1}
    
    echo -e "${BLUE}Creating configuration files${NC}"
    
    # Production config
    cat > "$STEP_DIR/config/production.json" << EOF
{
  "simulation": {
    "name": "${PLUGIN_NAME}",
    "description": "${WORKFLOW_NAME_UPPER} simulation - production run",
    "dimension": $DIM,
    "SPHType": "${SPH_TYPE}"
  },
  "physics": {
    "gamma": 1.666667,
    "gravityEnabled": false
  },
  "numerics": {
    "kernelType": "CubicSpline",
    "smoothingLengthEta": 1.2,
    "alphaViscosity": 1.0,
    "betaViscosity": 2.0
  },
  "time": {
    "endTime": 1.0,
    "dtMax": 0.01,
    "cflFactor": 0.3
  },
  "output": {
    "directory": "output/${PLUGIN_NAME}",
    "format": "csv",
    "snapshotInterval": 0.1
  }
}
EOF
    
    # Test config
    cat > "$STEP_DIR/config/test.json" << EOF
{
  "simulation": {
    "name": "${PLUGIN_NAME}_test",
    "description": "${WORKFLOW_NAME_UPPER} simulation - quick test",
    "dimension": $DIM,
    "SPHType": "${SPH_TYPE}"
  },
  "physics": {
    "gamma": 1.666667,
    "gravityEnabled": false
  },
  "numerics": {
    "kernelType": "CubicSpline",
    "smoothingLengthEta": 1.2,
    "alphaViscosity": 1.0,
    "betaViscosity": 2.0
  },
  "time": {
    "endTime": 0.1,
    "dtMax": 0.01,
    "cflFactor": 0.3
  },
  "output": {
    "directory": "output/${PLUGIN_NAME}_test",
    "format": "csv",
    "snapshotInterval": 0.05
  }
}
EOF
    
    echo -e "${GREEN}  ✓ Configuration files created${NC}"
}

# Function to create README
create_readme() {
    local STEP_DIR=$1
    local STEP_NAME=$2
    local WORKFLOW_NAME_UPPER=$(echo ${WORKFLOW_NAME:0:1} | tr '[:lower:]' '[:upper:]')${WORKFLOW_NAME:1}
    local SPH_TYPE_UPPER=$(echo ${SPH_TYPE} | tr '[:lower:]' '[:upper:]')
    local WORKFLOW_TYPE_UPPER=$(echo ${WORKFLOW_TYPE:0:1} | tr '[:lower:]' '[:upper:]')${WORKFLOW_TYPE:1}
    
    echo -e "${BLUE}Creating README.md${NC}"
    
    cat > "$STEP_DIR/README.md" << EOF
# $STEP_NAME - ${WORKFLOW_NAME_UPPER}

${WORKFLOW_TYPE_UPPER} simulation using ${SPH_TYPE_UPPER} in ${DIM}D.

## Quick Start

\`\`\`bash
# Build the plugin
cmake -B build -S .
cmake --build build

# Run test simulation
/path/to/sph${DIM}d build/lib${PLUGIN_NAME}_plugin.dylib config/test.json

# Run production simulation
/path/to/sph${DIM}d build/lib${PLUGIN_NAME}_plugin.dylib config/production.json
\`\`\`

## Directory Structure

\`\`\`
$STEP_NAME/
├── config/              # Configuration files
│   ├── production.json  # Full simulation
│   └── test.json       # Quick test
├── src/                # Source code
│   └── plugin.cpp      # Main plugin
├── scripts/            # Analysis scripts
├── data/               # Input data
├── docs/               # Documentation
├── results/            # Post-processed results
│   ├── animations/
│   ├── plots/
│   └── analysis/
├── output/             # Raw simulation data (gitignored)
└── build/              # Build artifacts (gitignored)
\`\`\`

## Configuration

- **production.json**: Full simulation parameters
- **test.json**: Quick test (reduced time/resolution)

## Physics

TODO: Describe the physics of this simulation

## Expected Results

TODO: Describe what you expect to see

## Notes

- Dimension: ${DIM}D
- SPH Type: ${SPH_TYPE_UPPER}
- Plugin: ${PLUGIN_NAME}

EOF
    
    echo -e "${GREEN}  ✓ README.md created${NC}"
}

# Main workflow creation
echo -e "${YELLOW}Creating workflow directory structure...${NC}"
mkdir -p "$WORKFLOW_DIR"

# Create workflow-level folders
mkdir -p "$WORKFLOW_DIR"/{workflow_logs,shared_data,workflow_results/{animations,plots,reports}}

# Create step(s)
if [ "$MULTI_STEP" = true ]; then
    # Multi-step workflow
    create_structure "$WORKFLOW_DIR/01_simulation" "01_simulation"
    create_plugin_source "$WORKFLOW_DIR/01_simulation" "01_simulation"
    create_cmakelists "$WORKFLOW_DIR/01_simulation" "01_simulation"
    create_configs "$WORKFLOW_DIR/01_simulation" "01_simulation"
    create_readme "$WORKFLOW_DIR/01_simulation" "01_simulation"
    
    create_structure "$WORKFLOW_DIR/02_simulation" "02_simulation"
    create_plugin_source "$WORKFLOW_DIR/02_simulation" "02_simulation"
    create_cmakelists "$WORKFLOW_DIR/02_simulation" "02_simulation"
    create_configs "$WORKFLOW_DIR/02_simulation" "02_simulation"
    create_readme "$WORKFLOW_DIR/02_simulation" "02_simulation"
else
    # Single-step workflow
    create_structure "$WORKFLOW_DIR/01_simulation" "01_simulation"
    create_plugin_source "$WORKFLOW_DIR/01_simulation" "01_simulation"
    create_cmakelists "$WORKFLOW_DIR/01_simulation" "01_simulation"
    create_configs "$WORKFLOW_DIR/01_simulation" "01_simulation"
    create_readme "$WORKFLOW_DIR/01_simulation" "01_simulation"
fi

# Create workflow-level README
echo -e "${BLUE}Creating workflow README.md${NC}"
WORKFLOW_NAME_UPPER=$(echo ${WORKFLOW_NAME:0:1} | tr '[:lower:]' '[:upper:]')${WORKFLOW_NAME:1}
WORKFLOW_TYPE_UPPER=$(echo ${WORKFLOW_TYPE:0:1} | tr '[:lower:]' '[:upper:]')${WORKFLOW_TYPE:1}
SPH_TYPE_UPPER=$(echo ${SPH_TYPE} | tr '[:lower:]' '[:upper:]')

cat > "$WORKFLOW_DIR/README.md" << EOF
# ${WORKFLOW_NAME_UPPER} Workflow

${WORKFLOW_TYPE_UPPER} simulation workflow.

## Overview

This workflow simulates ${WORKFLOW_NAME} using ${SPH_TYPE_UPPER} in ${DIM}D.

## Quick Start

\`\`\`bash
# Build
cd 01_simulation
cmake -B build -S .
cmake --build build

# Run
/path/to/sph${DIM}d build/lib${PLUGIN_NAME}_plugin.dylib config/test.json
\`\`\`

## Workflow Structure

\`\`\`
${WORKFLOW_NAME}_workflow/
├── README.md                  # This file
├── workflow_logs/            # Execution logs
├── shared_data/              # Data shared between steps
├── workflow_results/         # Cross-step analysis
$(if [ "$MULTI_STEP" = true ]; then
    echo "├── 01_simulation/            # Step 1"
    echo "└── 02_simulation/            # Step 2"
else
    echo "└── 01_simulation/            # Main simulation"
fi)
\`\`\`

## Configuration

- Dimension: ${DIM}D
- SPH Type: ${SPH_TYPE_UPPER}
- Plugin: ${PLUGIN_NAME}

## Steps

TODO: Describe the workflow steps

## Expected Results

TODO: Describe expected outcomes

## References

TODO: Add relevant papers or documentation

EOF

echo -e "${GREEN}  ✓ Workflow README.md created${NC}"

# Create Makefile (optional)
echo -e "${BLUE}Creating Makefile${NC}"
cat > "$WORKFLOW_DIR/Makefile" << 'EOF'
.PHONY: build clean run-test run-prod help

SPH_ROOT = ../../..
SPH_BUILD = $(SPH_ROOT)/build

build:
	@echo "Building workflow..."
	cd 01_simulation && cmake -B build -S . && cmake --build build

clean:
	@echo "Cleaning build artifacts..."
	rm -rf 01_simulation/build/*
	rm -rf 01_simulation/output/*

run-test:
	@echo "Running test simulation..."
	$(SPH_BUILD)/sph3d 01_simulation/build/lib*_plugin.dylib 01_simulation/config/test.json

run-prod:
	@echo "Running production simulation..."
	$(SPH_BUILD)/sph3d 01_simulation/build/lib*_plugin.dylib 01_simulation/config/production.json

help:
	@echo "Available targets:"
	@echo "  build     - Build the workflow plugin"
	@echo "  clean     - Clean build and output"
	@echo "  run-test  - Run quick test"
	@echo "  run-prod  - Run production simulation"
	@echo "  help      - Show this help"
EOF

echo -e "${GREEN}  ✓ Makefile created${NC}"

# Success message
echo ""
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}✅ Workflow created successfully!${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""
echo -e "${YELLOW}Location:${NC} $WORKFLOW_DIR"
echo ""
echo -e "${YELLOW}Next steps:${NC}"
echo "  1. Edit the plugin source:"
echo "     ${BLUE}$WORKFLOW_DIR/01_simulation/src/plugin.cpp${NC}"
echo ""
echo "  2. Build the plugin:"
echo "     ${BLUE}cd $WORKFLOW_DIR${NC}"
echo "     ${BLUE}make build${NC}"
echo ""
echo "  3. Run a test:"
echo "     ${BLUE}make run-test${NC}"
echo ""
echo "  4. Customize configurations:"
echo "     ${BLUE}$WORKFLOW_DIR/01_simulation/config/${NC}"
echo ""
echo -e "${GREEN}Happy simulating! 🚀${NC}"
echo ""

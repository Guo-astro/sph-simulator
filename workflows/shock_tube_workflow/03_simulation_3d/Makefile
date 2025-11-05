# Makefile for 3D Shock Tube Workflow
# Usage:
#   make build     - Build the plugin
#   make clean     - Clean build artifacts
#   make run-gsph  - Run GSPH simulation
#   make run-disph - Run DISPH simulation
#   make run-all   - Run both simulations
#   make full      - Build + Run all

# Paths
SPH_ROOT := ../../..
BUILD_DIR := build
SPH_BIN := $(BUILD_DIR)/sph3d
LIB_DIR := lib
PLUGIN := $(LIB_DIR)/libshock_tube_3d_plugin.dylib
RESULTS_DIR := results
OUTPUT_DIR := output
GSPH_RESULTS := $(RESULTS_DIR)/gsph
DISPH_RESULTS := $(RESULTS_DIR)/disph

.PHONY: all clean build run-gsph run-disph run-all help

# Default target
all: build

help:
	@echo "3D Shock Tube Workflow Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  make build     - Build the 3D shock tube plugin"
	@echo "  make clean     - Clean build artifacts"
	@echo "  make run-gsph  - Run GSPH simulation"
	@echo "  make run-disph - Run DISPH simulation"  
	@echo "  make run-all   - Run both simulations"
	@echo "  make full      - Build + Run all"

# Build plugin
build:
	@echo "=== Building 3D Shock Tube Plugin ==="
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=Debug .. && make
	@echo "✓ Plugin built: $(PLUGIN)"

# Clean build artifacts
clean:
	@echo "=== Cleaning Build Artifacts ==="
	@rm -rf $(BUILD_DIR)
	@rm -f $(LIB_DIR)/*.dylib
	@echo "✓ Build artifacts cleaned"

# Run GSPH simulation
run-gsph: build
	@echo "=== Running 3D GSPH Simulation ==="
	@mkdir -p $(GSPH_RESULTS)
	@echo "Using OpenMP with all available CPU cores"
	OMP_NUM_THREADS=$$(sysctl -n hw.ncpu) $(SPH_BIN) $(PLUGIN)
	@echo "Copying GSPH results..."
	@cp -f $(OUTPUT_DIR)/shock_tube_3d/*.dat $(GSPH_RESULTS)/ 2>/dev/null || true
	@echo "✓ GSPH simulation complete"

# Run DISPH simulation
run-disph: build
	@echo "=== Running 3D DISPH Simulation ==="
	@mkdir -p $(DISPH_RESULTS)
	@echo "Using OpenMP with all available CPU cores"
	@echo "Note: DISPH 3D not yet configured"
	@echo "✓ DISPH simulation skipped"

# Run all simulations
run-all: run-gsph
	@echo "✓ All 3D simulations complete"

# Full workflow
full: clean build run-all
	@echo ""
	@echo "========================================="
	@echo "✓ FULL 3D WORKFLOW COMPLETE"
	@echo "========================================="
	@echo "Results:"
	@echo "  GSPH:  $(RESULTS_DIR)/gsph/"
	@echo "========================================="

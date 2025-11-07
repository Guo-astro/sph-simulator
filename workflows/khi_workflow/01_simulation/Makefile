# Makefile for Kelvin-Helmholtz Instability (KHI) Workflow (2D)
# Usage:
#   make           - Build the plugin
#   make clean     - Clean build artifacts
#   make run       - Run simulation
#   make animate   - Generate animation
#   make viz       - Generate all visualizations
#   make full      - Build + Run + Visualize (incremental)
#   make full-clean - Full workflow with clean rebuild

# Paths
SPH_ROOT := ../../..
BUILD_DIR := build
SPH_BIN := $(BUILD_DIR)/sph2d
LIB_DIR := lib
PLUGIN := $(LIB_DIR)/libkhi_plugin.dylib
RESULTS_DIR := results
ANALYSIS_DIR := $(RESULTS_DIR)/analysis
PLOTS_DIR := $(ANALYSIS_DIR)/plots
ANIMATIONS_DIR := $(ANALYSIS_DIR)/animations
SCRIPTS_DIR := scripts
OUTPUT_DIR := output

.PHONY: all clean clean-all clean-outputs build run animate viz help full full-clean

# Default target
all: build

help:
	@echo "Kelvin-Helmholtz Instability Workflow Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  make build         - Build the KHI plugin"
	@echo "  make clean         - Clean build artifacts only"
	@echo "  make clean-outputs - Clean all outputs (results, plots, animations)"
	@echo "  make clean-all     - Clean everything (build + outputs)"
	@echo "  make run           - Run simulation"
	@echo "  make animate       - Generate animation"
	@echo "  make viz           - Generate all visualizations"
	@echo "  make full          - Build + Run + Visualize (incremental build)"
	@echo "  make full-clean    - Full workflow with clean rebuild from scratch"

# Build plugin
build:
	@echo "=== Building KHI Plugin ==="
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=Debug .. && make
	@echo "✓ Plugin built: $(PLUGIN)"

# Clean build artifacts only
clean:
	@echo "=== Cleaning Build Artifacts ==="
	@rm -rf $(BUILD_DIR)
	@rm -f $(LIB_DIR)/*.dylib
	@rm -f *.dylib
	@echo "✓ Build artifacts cleaned"

# Clean all outputs (results, plots, animations)
clean-outputs:
	@echo "=== Cleaning All Outputs ==="
	@echo "Removing simulation results..."
	@rm -rf $(RESULTS_DIR)/*.csv
	@rm -rf $(OUTPUT_DIR)/khi_*
	@echo "Removing analysis outputs..."
	@rm -rf $(ANALYSIS_DIR)/plots/*.png
	@rm -rf $(ANALYSIS_DIR)/animations/*.mp4
	@echo "Removing temporary files..."
	@rm -f $(SCRIPTS_DIR)/*.png
	@echo "✓ All outputs cleaned"

# Clean everything (build + outputs)
clean-all: clean clean-outputs
	@echo "✓ Everything cleaned"

# Run simulation
run: build
	@echo "=== Running KHI Simulation ==="
	@mkdir -p $(RESULTS_DIR)
	@mkdir -p $(OUTPUT_DIR)
	@echo "Using OpenMP with all available CPU cores"
	@echo "Running KHI plugin..."
	OMP_NUM_THREADS=$$(sysctl -n hw.ncpu) $(SPH_BIN) $(PLUGIN)
	@echo "Copying results..."
	@if [ -d "$(OUTPUT_DIR)/khi/snapshots" ]; then \
		cp -f $(OUTPUT_DIR)/khi/snapshots/*.csv $(RESULTS_DIR)/ 2>/dev/null || \
		cp -f $(OUTPUT_DIR)/khi/*.csv $(RESULTS_DIR)/ 2>/dev/null || true; \
		cp -f $(OUTPUT_DIR)/khi/energy.csv $(RESULTS_DIR)/ 2>/dev/null || true; \
	else \
		echo "Warning: No output found in expected location"; \
	fi
	@echo "✓ Simulation complete"

# Generate animation
animate:
	@echo "=== Generating Animation ==="
	@mkdir -p $(ANIMATIONS_DIR)
	@echo "Creating KHI animation..."
	@cd $(SCRIPTS_DIR) && python3 animate_khi.py
	@echo "✓ Animation saved: $(ANIMATIONS_DIR)/khi.mp4"

# Generate all visualizations
viz: animate
	@echo "✓ All visualizations complete"

# Full workflow (incremental build - no clean)
full: build run viz
	@echo ""
	@echo "========================================="
	@echo "✓ FULL WORKFLOW COMPLETE"
	@echo "========================================="
	@echo "Results:"
	@echo "  Data:      $(RESULTS_DIR)/"
	@echo "  Analysis:  $(ANALYSIS_DIR)/"
	@echo "  Animation: $(ANIMATIONS_DIR)/"
	@echo "========================================="

# Full workflow from scratch (clean rebuild)
full-clean: clean build run viz
	@echo ""
	@echo "========================================="
	@echo "✓ FULL WORKFLOW COMPLETE (CLEAN BUILD)"
	@echo "========================================="
	@echo "Results:"
	@echo "  Data:      $(RESULTS_DIR)/"
	@echo "  Analysis:  $(ANALYSIS_DIR)/"
	@echo "  Animation: $(ANIMATIONS_DIR)/"
	@echo "========================================="

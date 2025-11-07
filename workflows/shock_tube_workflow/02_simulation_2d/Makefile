# Makefile for 2D Shock Tube Workflow
# Usage:
#   make           - Build the plugin (Release mode, no debug logs)
#   make BUILD_TYPE=Debug  - Build with debug logs enabled
#   make clean     - Clean build artifacts
#   make run-ssph  - Run SSPH simulation
#   make run-gsph  - Run GSPH simulation
#   make run-disph - Run DISPH simulation
#   make run-all   - Run all simulations
#   make plot      - Generate comparison plots
#   make animate   - Generate comparison animation
#   make viz       - Generate all visualizations

# Build configuration
BUILD_TYPE ?= Release

# Paths
SPH_ROOT := ../../..
BUILD_DIR := build
SPH_BIN := $(BUILD_DIR)/sph2d
LIB_DIR := lib
PLUGIN_SSPH := $(LIB_DIR)/libshock_tube_2d_ssph_plugin.dylib
PLUGIN_GSPH := $(LIB_DIR)/libshock_tube_2d_gsph_plugin.dylib
PLUGIN_DISPH := $(LIB_DIR)/libshock_tube_2d_disph_plugin.dylib
CONFIG_DIR := config
RESULTS_DIR := results
ANALYSIS_DIR := $(RESULTS_DIR)/analysis
PLOTS_DIR := $(ANALYSIS_DIR)/plots
ANIMATIONS_DIR := $(ANALYSIS_DIR)/animations
SCRIPTS_DIR := scripts

# Configs
SSPH_CONFIG := $(CONFIG_DIR)/ssph.json
GSPH_CONFIG := $(CONFIG_DIR)/gsph.json
DISPH_CONFIG := $(CONFIG_DIR)/disph.json

# Results
SSPH_RESULTS := $(RESULTS_DIR)/ssph
GSPH_RESULTS := $(RESULTS_DIR)/gsph
DISPH_RESULTS := $(RESULTS_DIR)/disph

.PHONY: all clean clean-all clean-outputs clean-build build run-ssph run-gsph run-disph run-all plot animate viz help full full-clean

# Default target
all: build

help:
	@echo "2D Shock Tube Workflow Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  make build              - Build plugins (Release mode, no debug logs)"
	@echo "  make BUILD_TYPE=Debug   - Build with debug logs enabled"
	@echo "  make clean              - Clean build artifacts only"
	@echo "  make clean-outputs      - Clean all outputs (results, plots, animations)"
	@echo "  make clean-all          - Clean everything (build + outputs)"
	@echo "  make run-ssph           - Run SSPH simulation"
	@echo "  make run-gsph           - Run GSPH simulation"
	@echo "  make run-disph          - Run DISPH simulation"
	@echo "  make run-all            - Run all simulations"
	@echo "  make plot               - Generate comparison plots"
	@echo "  make animate            - Generate comparison animation"
	@echo "  make viz                - Generate all visualizations (plot + animate)"
	@echo "  make full               - Build + Run all + Visualize (incremental build)"
	@echo "  make full-clean         - Full workflow with clean rebuild from scratch"
	@echo ""
	@echo "Build Types:"
	@echo "  Release (default) - Optimized, no debug logs"
	@echo "  Debug             - Debug symbols, verbose logging"

# Build plugin
build:
	@echo "=== Building 2D Shock Tube Plugins ($(BUILD_TYPE) mode) ==="
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) .. && make
	@echo "✓ Plugins built:"
	@echo "  - $(PLUGIN_SSPH)"
	@echo "  - $(PLUGIN_GSPH)"
	@echo "  - $(PLUGIN_DISPH)"

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
	@rm -rf $(RESULTS_DIR)/ssph/*.csv
	@rm -rf $(RESULTS_DIR)/gsph/*.csv
	@rm -rf $(RESULTS_DIR)/disph/*.csv
	@rm -rf output/ssph_shock_tube_2d
	@rm -rf output/gsph_shock_tube_2d
	@rm -rf output/disph_shock_tube_2d
	@echo "Removing analysis outputs..."
	@rm -rf $(ANALYSIS_DIR)/plots/*.png
	@rm -rf $(ANALYSIS_DIR)/animations/*.mp4
	@echo "Removing temporary files..."
	@rm -f $(SCRIPTS_DIR)/*.png
	@echo "✓ All outputs cleaned"

# Clean everything (build + outputs)
clean-all: clean clean-outputs
	@echo "✓ Everything cleaned"

# Run SSPH simulation
run-ssph: build
	@echo "=== Running 2D SSPH Simulation ==="
	@mkdir -p $(SSPH_RESULTS)
	@mkdir -p output
	@echo "Using OpenMP with all available CPU cores"
	@echo "Running SSPH plugin..."
	OMP_NUM_THREADS=$$(sysctl -n hw.ncpu) $(SPH_BIN) $(PLUGIN_SSPH)
	@echo "Copying SSPH results..."
	@if [ -d "output/ssph_shock_tube_2d/snapshots" ]; then \
		cp -f output/ssph_shock_tube_2d/snapshots/*.csv $(SSPH_RESULTS)/ 2>/dev/null || \
		cp -f output/ssph_shock_tube_2d/*.csv $(SSPH_RESULTS)/ 2>/dev/null || true; \
		cp -f output/ssph_shock_tube_2d/energy.csv $(SSPH_RESULTS)/ 2>/dev/null || true; \
	else \
		echo "Warning: No SSPH output found in expected location"; \
	fi
	@echo "✓ SSPH simulation complete"

# Run GSPH simulation
run-gsph: build
	@echo "=== Running 2D GSPH Simulation ==="
	@mkdir -p $(GSPH_RESULTS)
	@mkdir -p output
	@echo "Using OpenMP with all available CPU cores"
	@echo "Running GSPH plugin..."
	OMP_NUM_THREADS=$$(sysctl -n hw.ncpu) $(SPH_BIN) $(PLUGIN_GSPH)
	@echo "Copying GSPH results..."
	@if [ -d "output/gsph_shock_tube_2d/snapshots" ]; then \
		cp -f output/gsph_shock_tube_2d/snapshots/*.csv $(GSPH_RESULTS)/ 2>/dev/null || \
		cp -f output/gsph_shock_tube_2d/*.csv $(GSPH_RESULTS)/ 2>/dev/null || true; \
		cp -f output/gsph_shock_tube_2d/energy.csv $(GSPH_RESULTS)/ 2>/dev/null || true; \
	else \
		echo "Warning: No GSPH output found in expected location"; \
	fi
	@echo "✓ GSPH simulation complete"

# Run DISPH simulation
run-disph: build
	@echo "=== Running 2D DISPH Simulation ==="
	@mkdir -p $(DISPH_RESULTS)
	@mkdir -p output
	@echo "Using OpenMP with all available CPU cores"
	@echo "Running DISPH plugin..."
	OMP_NUM_THREADS=$$(sysctl -n hw.ncpu) $(SPH_BIN) $(PLUGIN_DISPH)
	@echo "Copying DISPH results..."
	@if [ -d "output/disph_shock_tube_2d/snapshots" ]; then \
		cp -f output/disph_shock_tube_2d/snapshots/*.csv $(DISPH_RESULTS)/ 2>/dev/null || \
		cp -f output/disph_shock_tube_2d/*.csv $(DISPH_RESULTS)/ 2>/dev/null || true; \
		cp -f output/disph_shock_tube_2d/energy.csv $(DISPH_RESULTS)/ 2>/dev/null || true; \
	else \
		echo "Warning: No DISPH output found in expected location"; \
	fi
	@echo "✓ DISPH simulation complete"

# Run all simulations
run-all: run-ssph run-gsph run-disph
	@echo "✓ All simulations complete"

# Generate comparison plot
plot:
	@echo "=== Generating Comparison Plot ==="
	@mkdir -p $(PLOTS_DIR)
	@cd $(SCRIPTS_DIR) && python3 plot_comparison.py
	@echo "✓ Plot saved: $(PLOTS_DIR)/comparison_plot.png"

# Generate animation
animate:
	@echo "=== Generating Comparison Animations ==="
	@mkdir -p $(ANIMATIONS_DIR)
	@echo "Creating SSPH animation..."
	@cd $(SCRIPTS_DIR) && python3 animate_ssph.py || echo "Warning: SSPH animation script not found or failed"
	@echo "Creating GSPH animation..."
	@cd $(SCRIPTS_DIR) && python3 animate_gsph.py || echo "Warning: GSPH animation script not found or failed"
	@echo "Creating DISPH animation..."
	@cd $(SCRIPTS_DIR) && python3 animate_disph.py || echo "Warning: DISPH animation script not found or failed"
	@echo "✓ Animations saved to $(ANIMATIONS_DIR)/"

# Generate all visualizations
viz: plot animate
	@echo "✓ All visualizations complete"

# Full workflow (incremental build - no clean)
full: build run-all viz
	@echo ""
	@echo "========================================="
	@echo "✓ FULL 2D WORKFLOW COMPLETE"
	@echo "========================================="
	@echo "Results:"
	@echo "  SSPH:      $(RESULTS_DIR)/ssph/"
	@echo "  GSPH:      $(RESULTS_DIR)/gsph/"
	@echo "  DISPH:     $(RESULTS_DIR)/disph/"
	@echo "  Analysis:  $(ANALYSIS_DIR)/"
	@echo "  Plots:     $(PLOTS_DIR)/"
	@echo "  Animation: $(ANIMATIONS_DIR)/"
	@echo "========================================="

# Full workflow from scratch (clean rebuild)
full-clean: clean build run-all viz
	@echo ""
	@echo "========================================="
	@echo "✓ FULL 2D WORKFLOW COMPLETE (CLEAN BUILD)"
	@echo "========================================="
	@echo "Results:"
	@echo "  SSPH:      $(RESULTS_DIR)/ssph/"
	@echo "  GSPH:      $(RESULTS_DIR)/gsph/"
	@echo "  DISPH:     $(RESULTS_DIR)/disph/"
	@echo "  Analysis:  $(ANALYSIS_DIR)/"
	@echo "  Plots:     $(PLOTS_DIR)/"
	@echo "  Animation: $(ANIMATIONS_DIR)/"
	@echo "========================================="

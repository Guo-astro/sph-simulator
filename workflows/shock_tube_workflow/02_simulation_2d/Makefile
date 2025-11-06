# Makefile for 2D Shock Tube Workflow
# Usage:
#   make           - Build the plugin
#   make clean     - Clean build artifacts
#   make run-gsph  - Run GSPH simulation
#   make run-disph - Run DISPH simulation
#   make run-all   - Run both simulations
#   make plot      - Generate comparison plots
#   make animate   - Generate comparison animation
#   make viz       - Generate all visualizations

# Paths
SPH_ROOT := ../../..
BUILD_DIR := build
SPH_BIN := $(BUILD_DIR)/sph2d
LIB_DIR := lib
PLUGIN := $(LIB_DIR)/libshock_tube_2d_plugin.dylib
CONFIG_DIR := config
RESULTS_DIR := results
ANALYSIS_DIR := $(RESULTS_DIR)/analysis
PLOTS_DIR := $(ANALYSIS_DIR)/plots
ANIMATIONS_DIR := $(ANALYSIS_DIR)/animations
SCRIPTS_DIR := scripts
OUTPUT_DIR := output

# Conan toolchain path
CONAN_TOOLCHAIN := $(SPH_ROOT)/build_tests/conan_toolchain.cmake

# Configs
GSPH_CONFIG := $(CONFIG_DIR)/gsph.json
DISPH_CONFIG := $(CONFIG_DIR)/disph.json

# Results
SSPH_RESULTS := $(RESULTS_DIR)/ssph
SSPH_GHOSTS_RESULTS := $(RESULTS_DIR)/ssph_ghosts
GSPH_RESULTS := $(RESULTS_DIR)/gsph
DISPH_RESULTS := $(RESULTS_DIR)/disph

# Plugins
SSPH_PLUGIN := $(LIB_DIR)/libshock_tube_2d_ssph_plugin.dylib
SSPH_GHOSTS_PLUGIN := $(LIB_DIR)/libshock_tube_2d_ssph_ghosts_plugin.dylib
GSPH_PLUGIN := $(LIB_DIR)/libshock_tube_2d_gsph_plugin.dylib
DISPH_PLUGIN := $(LIB_DIR)/libshock_tube_2d_disph_plugin.dylib

.PHONY: all clean clean-all clean-outputs clean-build build run-ssph run-ssph-ghosts run-gsph run-disph run-all plot animate viz help full full-clean full-all-algorithms

# Default target
all: build

help:
	@echo "2D Shock Tube Workflow Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  make build         - Build all 4 SPH method plugins"
	@echo "  make clean         - Clean build artifacts only"
	@echo "  make clean-outputs - Clean all outputs (results, plots, animations)"
	@echo "  make clean-all     - Clean everything (build + outputs)"
	@echo "  make run-ssph      - Run SSPH simulation"
	@echo "  make run-ssph-ghosts - Run SSPH + ghosts simulation"
	@echo "  make run-gsph      - Run GSPH simulation"
	@echo "  make run-disph     - Run DISPH simulation"
	@echo "  make run-all       - Run all 4 simulations"
	@echo "  make plot          - Generate comparison plots (currently unavailable)"
	@echo "  make animate       - Generate comparison animations (5 files)"
	@echo "  make viz           - Generate all visualizations"
	@echo "  make full          - Build + Run all + Visualize (incremental build)"
	@echo "  make full-clean    - Full workflow with clean rebuild from scratch"
	@echo "  make full-all-algorithms - Full workflow testing all SPH algorithms"

# Build plugin
build:
	@echo "=== Building 2D Shock Tube Plugins ==="
	@mkdir -p $(BUILD_DIR)
	@if [ ! -f "$(CONAN_TOOLCHAIN)" ]; then \
		echo "ERROR: Conan toolchain not found at $(CONAN_TOOLCHAIN)"; \
		echo "Please run 'cd $(SPH_ROOT) && conan install . --build=missing' first"; \
		exit 1; \
	fi
	@cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=$(SPH_ROOT)/build_tests .. && make
	@echo "✓ Plugins built:"
	@echo "  - $(SSPH_PLUGIN)"
	@echo "  - $(SSPH_GHOSTS_PLUGIN)"
	@echo "  - $(GSPH_PLUGIN)"
	@echo "  - $(DISPH_PLUGIN)"

# Clean build artifacts only
clean:
	@echo "=== Cleaning Build Artifacts ==="
	@rm -rf $(BUILD_DIR)
	@rm -f $(LIB_DIR)/*.dylib
	@rm -f shock_tube_2d_plugin.dylib
	@echo "✓ Build artifacts cleaned"

# Clean all outputs (results, plots, animations)
clean-outputs:
	@echo "=== Cleaning All Outputs ==="
	@echo "Removing simulation results..."
	@rm -rf $(RESULTS_DIR)/ssph/*.dat
	@rm -rf $(RESULTS_DIR)/ssph_ghosts/*.dat
	@rm -rf $(RESULTS_DIR)/gsph/*.dat
	@rm -rf $(RESULTS_DIR)/disph/*.dat
	@echo "Removing output directories..."
	@rm -rf $(OUTPUT_DIR)/ssph_shock_tube_2d/*.dat
	@rm -rf $(OUTPUT_DIR)/ssph_ghosts_shock_tube_2d/*.dat
	@rm -rf $(OUTPUT_DIR)/gsph_shock_tube_2d/*.dat
	@rm -rf $(OUTPUT_DIR)/disph_shock_tube_2d/*.dat
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
	@mkdir -p $(OUTPUT_DIR)/ssph_shock_tube_2d
	@echo "Using OpenMP with all available CPU cores"
	OMP_NUM_THREADS=$$(sysctl -n hw.ncpu) $(SPH_BIN) $(SSPH_PLUGIN)
	@echo "Copying SSPH results..."
	@if [ -d "$(OUTPUT_DIR)/ssph_shock_tube_2d" ]; then \
		cp -rf $(OUTPUT_DIR)/ssph_shock_tube_2d/* $(SSPH_RESULTS)/ 2>/dev/null || true; \
	fi
	@echo "✓ SSPH simulation complete"

# Run SSPH + ghosts simulation
run-ssph-ghosts: build
	@echo "=== Running 2D SSPH + Ghosts Simulation ==="
	@mkdir -p $(SSPH_GHOSTS_RESULTS)
	@mkdir -p $(OUTPUT_DIR)/ssph_ghosts_shock_tube_2d
	@echo "Using OpenMP with all available CPU cores"
	OMP_NUM_THREADS=$$(sysctl -n hw.ncpu) $(SPH_BIN) $(SSPH_GHOSTS_PLUGIN)
	@echo "Copying SSPH + ghosts results..."
	@if [ -d "$(OUTPUT_DIR)/ssph_ghosts_shock_tube_2d" ]; then \
		cp -rf $(OUTPUT_DIR)/ssph_ghosts_shock_tube_2d/* $(SSPH_GHOSTS_RESULTS)/ 2>/dev/null || true; \
	fi
	@echo "✓ SSPH + ghosts simulation complete"

# Run GSPH simulation
run-gsph: build
	@echo "=== Running 2D GSPH Simulation ==="
	@mkdir -p $(GSPH_RESULTS)
	@mkdir -p $(OUTPUT_DIR)/gsph_shock_tube_2d
	@echo "Using OpenMP with all available CPU cores"
	OMP_NUM_THREADS=$$(sysctl -n hw.ncpu) $(SPH_BIN) $(GSPH_PLUGIN)
	@echo "Copying GSPH results..."
	@if [ -d "$(OUTPUT_DIR)/gsph_shock_tube_2d" ]; then \
		cp -rf $(OUTPUT_DIR)/gsph_shock_tube_2d/* $(GSPH_RESULTS)/ 2>/dev/null || true; \
	fi
	@echo "✓ GSPH simulation complete"

# Run DISPH simulation
run-disph: build
	@echo "=== Running 2D DISPH Simulation ==="
	@mkdir -p $(DISPH_RESULTS)
	@mkdir -p $(OUTPUT_DIR)/disph_shock_tube_2d
	@echo "Using OpenMP with all available CPU cores"
	OMP_NUM_THREADS=$$(sysctl -n hw.ncpu) $(SPH_BIN) $(DISPH_PLUGIN)
	@echo "Copying DISPH results..."
	@if [ -d "$(OUTPUT_DIR)/disph_shock_tube_2d" ]; then \
		cp -rf $(OUTPUT_DIR)/disph_shock_tube_2d/* $(DISPH_RESULTS)/ 2>/dev/null || true; \
	fi
	@echo "✓ DISPH simulation complete"

# Run all simulations
run-all: run-ssph run-ssph-ghosts run-gsph run-disph
	@echo "✓ All 4 simulations complete"

# Generate comparison plot
plot:
	@echo "=== Generating Comparison Plot ==="
	@echo "Note: No plotting script available. Skipping plot generation."
	@echo "✓ Plot generation skipped (no script available)"

# Generate animation
animate:
	@echo "=== Generating 2D Comparison Animations ==="
	@mkdir -p $(ANIMATIONS_DIR)
	@echo "Creating All Methods Comparison..."
	@cd $(SCRIPTS_DIR) && python3 animate_all_methods_comparison.py
	@echo "Creating SSPH vs Analytical..."
	@cd $(SCRIPTS_DIR) && python3 animate_ssph.py
	@echo "Creating GSPH vs Analytical..."
	@cd $(SCRIPTS_DIR) && python3 animate_gsph.py
	@echo "Creating DISPH vs Analytical..."
	@cd $(SCRIPTS_DIR) && python3 animate_disph.py
	@echo "Creating SSPH + Ghosts vs Analytical..."
	@cd $(SCRIPTS_DIR) && python3 animate_ssph_ghosts.py
	@echo "✓ Animations saved:"
	@echo "  - $(ANIMATIONS_DIR)/all_methods_comparison_2d.mp4"
	@echo "  - $(ANIMATIONS_DIR)/ssph_2d.mp4"
	@echo "  - $(ANIMATIONS_DIR)/gsph_2d.mp4"
	@echo "  - $(ANIMATIONS_DIR)/disph_2d.mp4"
	@echo "  - $(ANIMATIONS_DIR)/ssph_ghosts_2d.mp4"

# Generate all visualizations
viz: animate
	@echo "✓ All visualizations complete"

# Full workflow (incremental build - no clean)
full: build run-all viz
	@echo ""
	@echo "========================================="
	@echo "✓ FULL 2D WORKFLOW COMPLETE"
	@echo "========================================="
	@echo "Results:"
	@echo "  SSPH:        $(RESULTS_DIR)/ssph/"
	@echo "  SSPH+Ghosts: $(RESULTS_DIR)/ssph_ghosts/"
	@echo "  GSPH:        $(RESULTS_DIR)/gsph/"
	@echo "  DISPH:       $(RESULTS_DIR)/disph/"
	@echo "  Analysis:    $(ANALYSIS_DIR)/"
	@echo "  Animations:"
	@echo "    - $(ANIMATIONS_DIR)/all_methods_comparison_2d.mp4"
	@echo "    - $(ANIMATIONS_DIR)/ssph_2d.mp4"
	@echo "    - $(ANIMATIONS_DIR)/ssph_ghosts_2d.mp4"
	@echo "    - $(ANIMATIONS_DIR)/gsph_2d.mp4"
	@echo "    - $(ANIMATIONS_DIR)/disph_2d.mp4"
	@echo "========================================="

# Full workflow from scratch (clean rebuild)
full-clean: clean build run-all viz
	@echo ""
	@echo "========================================="
	@echo "✓ FULL 2D WORKFLOW COMPLETE (CLEAN BUILD)"
	@echo "========================================="
	@echo "Results:"
	@echo "  SSPH:        $(RESULTS_DIR)/ssph/"
	@echo "  SSPH+Ghosts: $(RESULTS_DIR)/ssph_ghosts/"
	@echo "  GSPH:        $(RESULTS_DIR)/gsph/"
	@echo "  DISPH:       $(RESULTS_DIR)/disph/"
	@echo "  Analysis:    $(ANALYSIS_DIR)/"
	@echo "  Animations:"
	@echo "    - $(ANIMATIONS_DIR)/all_methods_comparison_2d.mp4"
	@echo "    - $(ANIMATIONS_DIR)/ssph_2d.mp4"
	@echo "    - $(ANIMATIONS_DIR)/ssph_ghosts_2d.mp4"
	@echo "    - $(ANIMATIONS_DIR)/gsph_2d.mp4"
	@echo "    - $(ANIMATIONS_DIR)/disph_2d.mp4"
	@echo "========================================="

# Full workflow testing all algorithms (GSPH currently, DISPH/SPH when configs available)
full-all-algorithms: clean build run-all viz
	@echo ""
	@echo "========================================="
	@echo "✓ FULL 2D ALGORITHM COMPARISON COMPLETE"
	@echo "========================================="
	@echo "Algorithms tested:"
	@echo "  - SSPH (Standard SPH without ghosts)"
	@echo "  - SSPH + Ghosts (SSPH with ghost particles)"
	@echo "  - GSPH (Godunov SPH with HLL Riemann solver)"
	@echo "  - DISPH (Density-Independent SPH)"
	@echo ""
	@echo "Results:"
	@echo "  SSPH:        $(RESULTS_DIR)/ssph/"
	@echo "  SSPH+Ghosts: $(RESULTS_DIR)/ssph_ghosts/"
	@echo "  GSPH:        $(RESULTS_DIR)/gsph/"
	@echo "  DISPH:       $(RESULTS_DIR)/disph/"
	@echo "  Analysis:    $(ANALYSIS_DIR)/"
	@echo ""
	@echo "Animations:"
	@echo "  - $(ANIMATIONS_DIR)/all_methods_comparison_2d.mp4"
	@echo "  - $(ANIMATIONS_DIR)/ssph_2d.mp4"
	@echo "  - $(ANIMATIONS_DIR)/ssph_ghosts_2d.mp4"
	@echo "  - $(ANIMATIONS_DIR)/gsph_2d.mp4"
	@echo "  - $(ANIMATIONS_DIR)/disph_2d.mp4"
	@echo "========================================="

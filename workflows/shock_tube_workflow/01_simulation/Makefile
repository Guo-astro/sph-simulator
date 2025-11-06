# Makefile for Shock Tube Workflow
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
SPH_BIN := $(BUILD_DIR)/sph
LIB_DIR := lib
PLUGIN_ENHANCED := $(LIB_DIR)/libshock_tube_plugin_enhanced.dylib
PLUGIN_GSPH := $(LIB_DIR)/libshock_tube_plugin_gsph.dylib
PLUGIN_DISPH := $(LIB_DIR)/libshock_tube_plugin_disph.dylib
CONFIG_DIR := config
RESULTS_DIR := results
ANALYSIS_DIR := $(RESULTS_DIR)/analysis
PLOTS_DIR := $(ANALYSIS_DIR)/plots
ANIMATIONS_DIR := $(ANALYSIS_DIR)/animations
SCRIPTS_DIR := scripts

# Configs
GSPH_CONFIG := $(CONFIG_DIR)/gsph.json
DISPH_CONFIG := $(CONFIG_DIR)/disph.json

# Results
GSPH_RESULTS := $(RESULTS_DIR)/gsph
DISPH_RESULTS := $(RESULTS_DIR)/disph

.PHONY: all clean clean-all clean-outputs clean-build build run-gsph run-disph run-all plot animate viz help full full-clean

# Default target
all: build

help:
	@echo "Shock Tube Workflow Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  make build         - Build the shock tube plugin"
	@echo "  make clean         - Clean build artifacts only"
	@echo "  make clean-outputs - Clean all outputs (results, plots, animations)"
	@echo "  make clean-all     - Clean everything (build + outputs)"
	@echo "  make run-gsph      - Run GSPH simulation"
	@echo "  make run-disph     - Run DISPH simulation"
	@echo "  make run-all       - Run both simulations"
	@echo "  make plot          - Generate comparison plots (currently unavailable)"
	@echo "  make animate       - Generate comparison animation"
	@echo "  make viz           - Generate all visualizations (animate only)"
	@echo "  make full          - Build + Run all + Visualize (incremental build)"
	@echo "  make full-clean    - Full workflow with clean rebuild from scratch"

# Build plugin
build:
	@echo "=== Building Shock Tube Plugin ==="
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
	@rm -rf $(RESULTS_DIR)/gsph/*.csv
	@rm -rf $(RESULTS_DIR)/disph/*.csv
	@rm -rf output/shock_tube_*
	@echo "Removing analysis outputs..."
	@rm -rf $(ANALYSIS_DIR)/plots/*.png
	@rm -rf $(ANALYSIS_DIR)/animations/*.mp4
	@echo "Removing temporary files..."
	@rm -f $(SCRIPTS_DIR)/*.png
	@echo "✓ All outputs cleaned"

# Clean everything (build + outputs)
clean-all: clean clean-outputs
	@echo "✓ Everything cleaned"

# Run GSPH simulation
run-gsph: build
	@echo "=== Running GSPH Simulation ==="
	@mkdir -p $(GSPH_RESULTS)
	@mkdir -p output
	@echo "Using OpenMP with all available CPU cores"
	@echo "Running GSPH plugin..."
	OMP_NUM_THREADS=$$(sysctl -n hw.ncpu) $(SPH_BIN) $(PLUGIN_GSPH)
	@echo "Copying GSPH results..."
	@if [ -d "output/shock_tube_gsph/snapshots" ]; then \
		cp -f output/shock_tube_gsph/snapshots/*.csv $(GSPH_RESULTS)/ 2>/dev/null || \
		cp -f output/shock_tube_gsph/*.csv $(GSPH_RESULTS)/ 2>/dev/null || true; \
		cp -f output/shock_tube_gsph/energy.csv $(GSPH_RESULTS)/ 2>/dev/null || true; \
	else \
		echo "Warning: No GSPH output found in expected location"; \
	fi
	@echo "✓ GSPH simulation complete"

# Run DISPH simulation
run-disph: build
	@echo "=== Running DISPH Simulation ==="
	@mkdir -p $(DISPH_RESULTS)
	@mkdir -p output
	@echo "Using OpenMP with all available CPU cores"
	@echo "Running DISPH plugin..."
	OMP_NUM_THREADS=$$(sysctl -n hw.ncpu) $(SPH_BIN) $(PLUGIN_DISPH)
	@echo "Copying DISPH results..."
	@if [ -d "output/shock_tube_disph/snapshots" ]; then \
		cp -f output/shock_tube_disph/snapshots/*.csv $(DISPH_RESULTS)/ 2>/dev/null || \
		cp -f output/shock_tube_disph/*.csv $(DISPH_RESULTS)/ 2>/dev/null || true; \
		cp -f output/shock_tube_disph/energy.csv $(DISPH_RESULTS)/ 2>/dev/null || true; \
	else \
		echo "Warning: No DISPH output found in expected location"; \
	fi
	@echo "✓ DISPH simulation complete"

# Run both simulations
run-all: run-gsph run-disph
	@echo "✓ All simulations complete"

# Generate comparison plot
plot:
	@echo "=== Generating Comparison Plot ==="
	@echo "Note: No plotting script available. Skipping plot generation."
	@echo "✓ Plot generation skipped (no script available)"

# Generate animation
animate:
	@echo "=== Generating Comparison Animations ==="
	@mkdir -p $(ANIMATIONS_DIR)
	@echo "Creating GSPH vs Analytical..."
	@cd $(SCRIPTS_DIR) && python3 animate_gsph_vs_analytical.py
	@echo "Creating DISPH vs Analytical..."
	@cd $(SCRIPTS_DIR) && python3 animate_disph_vs_analytical.py
	@echo "✓ Animations saved:"
	@echo "  - $(ANIMATIONS_DIR)/gsph_vs_analytical.mp4"
	@echo "  - $(ANIMATIONS_DIR)/disph_vs_analytical.mp4"

# Generate all visualizations
viz: animate
	@echo "✓ All visualizations complete"

# Full workflow (incremental build - no clean)
full: build run-all viz
	@echo ""
	@echo "========================================="
	@echo "✓ FULL WORKFLOW COMPLETE"
	@echo "========================================="
	@echo "Results:"
	@echo "  GSPH:      $(RESULTS_DIR)/gsph/"
	@echo "  DISPH:     $(RESULTS_DIR)/disph/"
	@echo "  Analysis:  $(ANALYSIS_DIR)/"
	@echo "  Animation: $(ANIMATIONS_DIR)/"
	@echo "Note: Plot generation not available (no plotting script)"
	@echo "========================================="

# Full workflow from scratch (clean rebuild)
full-clean: clean build run-all viz
	@echo ""
	@echo "========================================="
	@echo "✓ FULL WORKFLOW COMPLETE (CLEAN BUILD)"
	@echo "========================================="
	@echo "Results:"
	@echo "  GSPH:      $(RESULTS_DIR)/gsph/"
	@echo "  DISPH:     $(RESULTS_DIR)/disph/"
	@echo "  Analysis:  $(ANALYSIS_DIR)/"
	@echo "  Animation: $(ANIMATIONS_DIR)/"
	@echo "Note: Plot generation not available (no plotting script)"
	@echo "========================================="

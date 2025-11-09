# Makefile for Evrard Workflow (3D)
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
SPH_BIN := $(BUILD_DIR)/sph3d
LIB_DIR := lib
PLUGIN := $(LIB_DIR)/libevrard_plugin.dylib
RESULTS_DIR := results
ANALYSIS_DIR := $(RESULTS_DIR)/analysis
PLOTS_DIR := $(ANALYSIS_DIR)/plots
ANIMATIONS_DIR := $(ANALYSIS_DIR)/animations
SCRIPTS_DIR := scripts
OUTPUT_DIR := output

# Build configuration (Debug or Release)
BUILD_TYPE ?= Release

.PHONY: all clean clean-all clean-outputs build run animate animate-3d energy-analysis viz help full full-clean

# Default target
all: build

help:
	@echo "Evrard Workflow Makefile"
	@echo ""
	@echo "Build Configuration:"
	@echo "  BUILD_TYPE = $(BUILD_TYPE) (Debug or Release)"
	@echo "  Override with: make BUILD_TYPE=Debug <target>"
	@echo ""
	@echo "Targets:"
	@echo "  make build           - Build the evrard plugin (default: Release)"
	@echo "  make clean           - Clean build artifacts only"
	@echo "  make clean-outputs   - Clean all outputs (results, plots, animations)"
	@echo "  make clean-all       - Clean everything (build + outputs)"
	@echo "  make run             - Run simulation"
	@echo "  make energy-analysis - Generate energy conservation plots"
	@echo "  make animate         - Generate 2D animation"
	@echo "  make animate-3d      - Generate 3D animation"
	@echo "  make viz             - Generate all visualizations (energy + 2D + 3D)"
	@echo "  make full            - Build + Run + Visualize (incremental build)"
	@echo "  make full-clean      - Full workflow with clean rebuild from scratch"
	@echo ""
	@echo "Examples:"
	@echo "  make full-clean                    # Release mode (faster, optimized)"
	@echo "  make BUILD_TYPE=Debug full-clean   # Debug mode (for debugging)"

# Build plugin
build:
	@echo "=== Building Evrard Plugin ($(BUILD_TYPE)) ==="
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) .. && make
	@echo "✓ Plugin built: $(PLUGIN) ($(BUILD_TYPE) mode)"

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
	@rm -rf output/evrard_*
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
	@echo "=== Running Evrard Simulation ==="
	@mkdir -p $(RESULTS_DIR)
	@mkdir -p output
	@echo "Using OpenMP with all available CPU cores"
	@echo "Running Evrard plugin..."
	@cd $(BUILD_DIR) && OMP_NUM_THREADS=$$(sysctl -n hw.ncpu) ./sph3d ../$(PLUGIN)
	@echo "Copying results..."
	@if [ -d "output/evrard_collapse/snapshots" ]; then \
		cp -f output/evrard_collapse/snapshots/*.csv $(RESULTS_DIR)/ 2>/dev/null || true; \
		cp -f output/evrard_collapse/energy.csv $(RESULTS_DIR)/ 2>/dev/null || true; \
	fi
	@echo "✓ Simulation complete"

# Generate energy conservation analysis plots
energy-analysis:
	@echo "=== Generating Energy Conservation Analysis ==="
	@mkdir -p $(PLOTS_DIR)
	@if [ -f "output/evrard_collapse/energy.csv" ]; then \
		echo "Running energy analysis..."; \
		python3 visualize_evrard.py output/evrard_collapse; \
		echo "Copying plots to $(PLOTS_DIR)..."; \
		cp -f output/evrard_collapse/energy_plots/*.png $(PLOTS_DIR)/ 2>/dev/null || true; \
	else \
		echo "Warning: energy.csv not found. Run simulation first."; \
	fi
	@echo "✓ Energy analysis complete"

# Generate animation
animate:
	@echo "=== Generating Animation ==="
	@mkdir -p $(ANIMATIONS_DIR)
	@if [ -d "$(SCRIPTS_DIR)" ] && [ -f "$(SCRIPTS_DIR)/animate_evrard.py" ]; then \
		echo "Creating Evrard collapse animation..."; \
		cd $(SCRIPTS_DIR) && python3 animate_evrard.py; \
		echo "✓ Animation saved: $(ANIMATIONS_DIR)/evrard_collapse.mp4"; \
	else \
		echo "Note: animate_evrard.py not found, skipping 2D animation"; \
	fi

# Generate 3D animation
animate-3d:
	@echo "=== Generating 3D Animation ==="
	@mkdir -p $(ANIMATIONS_DIR)
	@if [ -d "$(SCRIPTS_DIR)" ] && [ -f "$(SCRIPTS_DIR)/animate_evrard_3d.py" ]; then \
		echo "Creating Evrard collapse 3D animation..."; \
		cd $(SCRIPTS_DIR) && python3 animate_evrard_3d.py; \
		echo "✓ 3D Animation saved: $(ANIMATIONS_DIR)/evrard_collapse_3d.mp4"; \
	else \
		echo "Note: animate_evrard_3d.py not found, skipping 3D animation"; \
	fi

# Generate all visualizations
viz: energy-analysis animate animate-3d
	@echo "✓ All visualizations complete"

# Full workflow (incremental build - no clean)
full: build run viz
	@echo ""
	@echo "========================================="
	@echo "✓ FULL WORKFLOW COMPLETE"
	@echo "========================================="
	@echo "Results:"
	@echo "  Simulation data:     $(RESULTS_DIR)/"
	@echo "  Energy plots:        $(PLOTS_DIR)/"
	@echo "  Animations:          $(ANIMATIONS_DIR)/"
	@echo "  Energy data:         output/evrard_collapse/energy.csv"
	@echo "========================================="

# Full workflow from scratch (clean rebuild)
full-clean: clean build run viz
	@echo ""
	@echo "========================================="
	@echo "✓ FULL WORKFLOW COMPLETE (CLEAN BUILD)"
	@echo "========================================="
	@echo "Results:"
	@echo "  Simulation data:     $(RESULTS_DIR)/"
	@echo "  Energy plots:        $(PLOTS_DIR)/"
	@echo "  Animations:          $(ANIMATIONS_DIR)/"
	@echo "  Energy data:         output/evrard_collapse/energy.csv"
	@echo ""
	@echo "Energy Conservation Summary:"
	@if [ -f "$(PLOTS_DIR)/energy_evolution.png" ]; then \
		echo "  ✓ energy_evolution.png    - Energy components + conservation error"; \
		echo "  ✓ energy_paper_style.png  - Normalized energies (Evrard 1988 style)"; \
		echo "  ✓ energy_normalized.png   - Energy component fractions"; \
	fi
	@echo "========================================="

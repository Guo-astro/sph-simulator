# Workflow Standardization Guide

## Current Status (2025-11-06)

### ✅ Completed
- **Root directory cleanup**: Moved 29 Conan/CMake build artifacts to `build/`
- **Test file organization**: Moved manual test files to `tests/manual/`
- **Workflow audit**: Analyzed all 6 workflows for structural inconsistencies

### 🔍 Issues Identified

#### 1. Inconsistent Directory Structures

| Workflow | Makefile | Output Dirs | Log Files | Extra Files |
|----------|----------|-------------|-----------|-------------|
| **shock_tube/01_simulation** | ✅ | ✅ output/, output_baseline/, output_modern/ | ✅ output.log, run.log, baseline_full.log | ✅ plot_with_ghosts.py |
| **shock_tube/02_simulation_2d** | ✅ | ✅ bin/, output/, output_baseline/ | ❌ | ❌ |
| **shock_tube/03_simulation_3d** | ✅ | ❌ | ❌ | ❌ |
| **gresho_chan_vortex/01_simulation** | ❌ | ❌ | ✅ build.log | ❌ |
| **hydrostatic/01_simulation** | ❌ | ❌ | ✅ build.log | ❌ |
| **evrard/01_simulation** | ❌ | ❌ | ✅ build.log | ❌ |
| **khi/01_simulation** | ❌ | ❌ | ✅ build.log | ❌ |
| **pairing_instability/01_simulation** | ❌ | ❌ | ✅ build.log | ❌ |

**Problems:**
- Missing Makefiles for 5 workflows (inconsistent build process)
- Loose `.log` and `.py` files not in proper directories
- Inconsistent output directory naming (`output/` vs `output_baseline/` vs `bin/`)

---

## 📋 Recommended Standard Structure

```
workflows/
├── README.md                          # Workflow system overview
├── USAGE_GUIDE.md                    # User guide (exists)
├── WORKFLOW_STANDARDIZATION.md       # This document
├── create_workflow.sh                # Workflow scaffolding tool
├── visualize.py                      # Shared visualization utility
├── workflow.sh                       # Shared workflow runner
│
└── <workflow_name>/                  # e.g., shock_tube_workflow
    ├── README.md                     # Workflow-specific documentation
    ├── GHOST_PARTICLE_OUTPUT.md      # Optional workflow docs
    │
    └── <step>_<description>/         # e.g., 01_simulation, 02_analysis
        ├── CMakeLists.txt           # Build configuration (REQUIRED)
        ├── Makefile                 # Build + run automation (REQUIRED)
        ├── README.md                # Step documentation (optional)
        │
        ├── src/                     # Plugin source code
        │   ├── plugin.cpp           # Standard single-variant plugins
        │   ├── plugin_baseline.cpp  # Multi-variant plugins
        │   ├── plugin_modern.cpp
        │   ├── plugin_gsph.cpp
        │   └── plugin_disph.cpp
        │
        ├── scripts/                 # Analysis/visualization scripts
        │   ├── animate_*.py         # Animation generators
        │   ├── visualize*.py        # Static visualization
        │   └── *_utils.py           # Shared utilities
        │
        ├── data/                    # Input data files (optional)
        ├── docs/                    # Step-specific docs (optional)
        │
        ├── build/                   # Build artifacts (gitignored)
        ├── lib/                     # Generated .dylib plugins (gitignored)
        ├── output/                  # Simulation output (gitignored)
        │   ├── baseline_*/          # Per-variant subdirs if needed
        │   └── modern_*/
        └── results/                 # Analysis results (gitignored)
            ├── plots/
            └── animations/
```

---

## 🎯 Standardization Rules

### Directory Standards

1. **Build artifacts** → `build/` (gitignored)
2. **Generated plugins** → `lib/` (gitignored)
3. **Simulation output** → `output/<variant>/` (gitignored)
4. **Analysis products** → `results/` (gitignored)
5. **Scripts** → `scripts/` (version controlled)
6. **Documentation** → `docs/` or `README.md` (version controlled)

### Naming Conventions

1. **Workflows**: `<physics_test>_workflow/`
   - Examples: `shock_tube_workflow`, `evrard_workflow`

2. **Steps**: `<number>_<description>/`
   - Examples: `01_simulation`, `02_analysis`, `03_visualization`

3. **Plugins**: 
   - Single variant: `plugin.cpp`
   - Multiple variants: `plugin_<variant>.cpp` (baseline, modern, gsph, disph)

4. **Output directories**:
   - Standard: `output/`
   - Multi-variant: `output_<variant>/` or `output/<variant>/`

### Makefile Requirements

**Every simulation step MUST have a Makefile with:**

```makefile
# Standard targets (REQUIRED)
build:        # Compile plugin(s)
run:          # Execute simulation
clean:        # Remove build artifacts
visualize:    # Generate plots/animations (if applicable)

# Optional targets
test:         # Run validation tests
full:         # Build + run + visualize
help:         # Display available targets
```

**Example reference:** `/workflows/shock_tube_workflow/01_simulation/Makefile`

### File Placement Rules

| File Type | Location | Example |
|-----------|----------|---------|
| `.log` files | `output/` or `.gitignore` them | `output/baseline.log` |
| Python scripts | `scripts/` | `scripts/animate_baseline.py` |
| Data files | `data/` | `data/initial_conditions.json` |
| Build artifacts | `build/` | `build/plugin.o` |
| Shared libraries | `lib/` | `lib/libshock_tube_baseline_plugin.dylib` |
| Plots/images | `results/plots/` | `results/plots/snapshot_000.png` |
| Animations | `results/animations/` | `results/animations/shock_evolution.mp4` |

---

## 🔧 Refactoring Action Plan

### Priority 1: Add Missing Makefiles (CRITICAL)

**Affected workflows:**
- `gresho_chan_vortex_workflow/01_simulation`
- `hydrostatic_workflow/01_simulation`
- `evrard_workflow/01_simulation`
- `khi_workflow/01_simulation`
- `pairing_instability_workflow/01_simulation`

**Action:** Copy and adapt from `shock_tube_workflow/01_simulation/Makefile`

**Template structure:**
```makefile
# Project paths
ROOT_DIR := $(shell cd ../../.. && pwd)
BUILD_DIR := $(ROOT_DIR)/build

# Plugin configuration
PLUGIN_NAME := <workflow>_plugin
PLUGIN_SRC := src/plugin.cpp

# Standard targets
.PHONY: build run clean

build:
	@mkdir -p lib build
	@$(CXX) $(CXXFLAGS) -shared $(PLUGIN_SRC) -o lib/lib$(PLUGIN_NAME).dylib

run: build
	@mkdir -p output
	@cd output && $(BUILD_DIR)/sph ../lib/lib$(PLUGIN_NAME).dylib

clean:
	@rm -rf build lib output results
```

### Priority 2: Reorganize Loose Files

**shock_tube_workflow/01_simulation:**
```bash
# Move loose Python script
mkdir -p scripts
mv plot_with_ghosts.py scripts/

# Move log files
mv output.log run.log baseline_full.log output/
```

**shock_tube_workflow/02_simulation_2d:**
```bash
# Rename inconsistent directory
mv bin build  # If bin/ contains build artifacts
```

### Priority 3: Standardize Output Directories

**Current inconsistencies:**
- `output/`, `output_baseline/`, `output_modern/` (mixed)
- `bin/` (wrong name for build artifacts)

**Recommended approach:**

**Option A: Flat structure (single-variant workflows)**
```
output/
  ├── *.dat
  └── simulation.log
```

**Option B: Nested structure (multi-variant workflows)**
```
output/
  ├── baseline/
  │   ├── *.dat
  │   └── baseline.log
  ├── modern/
  ├── gsph/
  └── disph/
```

**Apply to:** `shock_tube_workflow/01_simulation`, `shock_tube_workflow/02_simulation_2d`

---

## 📝 Migration Checklist

### Per-Workflow Tasks

- [ ] **Create Makefile** (if missing)
  - [ ] Add `build`, `run`, `clean` targets
  - [ ] Test build process
  - [ ] Verify runtime execution

- [ ] **Organize loose files**
  - [ ] Move `.log` files to `output/`
  - [ ] Move `.py` scripts to `scripts/`
  - [ ] Move data files to `data/`

- [ ] **Standardize directories**
  - [ ] Rename `bin/` → `build/` (if needed)
  - [ ] Consolidate `output_*/` → `output/*/` (if needed)
  - [ ] Create `results/` for analysis products

- [ ] **Update .gitignore**
  - [ ] Verify `build/`, `lib/`, `output/`, `results/` are ignored
  - [ ] Check no gitignored files exist in repo

- [ ] **Test workflow**
  - [ ] `make clean && make build` succeeds
  - [ ] `make run` executes simulation
  - [ ] Output lands in correct directories

---

## 🚀 Quick Start: Fixing Your Workflow

### 1. Add Missing Makefile
```bash
cd workflows/<your_workflow>/01_simulation
cp ../../shock_tube_workflow/01_simulation/Makefile .
# Edit plugin names and paths as needed
```

### 2. Clean Up Loose Files
```bash
# Move logs
mkdir -p output
mv *.log output/ 2>/dev/null || true

# Move scripts
mkdir -p scripts
mv *.py scripts/ 2>/dev/null || true
```

### 3. Test Build
```bash
make clean
make build
make run
```

---

## 📊 Benefits of Standardization

1. **Developer Experience**
   - Predictable structure across all workflows
   - Same commands work everywhere (`make build`, `make run`)
   - Easy to onboard new contributors

2. **Maintainability**
   - Clear separation of source vs. generated files
   - Easier to debug build issues
   - Version control only tracks meaningful files

3. **Automation**
   - CI/CD can rely on standard targets
   - Scripted testing across all workflows
   - Batch processing workflows systematically

4. **Professionalism**
   - Industry-standard project layout
   - Clean repository presentation
   - Easier to share and publish

---

## 🔗 References

- **Example Workflow**: `workflows/shock_tube_workflow/` (fully refactored)
- **Makefile Template**: `workflows/shock_tube_workflow/01_simulation/Makefile`
- **Root .gitignore**: `/Users/guo/sph-simulation/.gitignore` (already comprehensive)
- **Project Structure Memory**: Serena MCP `project_structure_refactoring`

---

## ✅ Completion Criteria

A workflow is **standardized** when:

1. ✅ Has `Makefile` with `build`, `run`, `clean` targets
2. ✅ All `.log` files in `output/` or gitignored
3. ✅ All scripts in `scripts/` directory
4. ✅ Build artifacts only in `build/` and `lib/`
5. ✅ Output data only in `output/` (gitignored)
6. ✅ No loose files in workflow root
7. ✅ `make clean && make build && make run` works end-to-end

---

**Document Version:** 1.0  
**Last Updated:** 2025-11-06  
**Status:** Initial version - refactoring in progress

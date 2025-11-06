# Workflow Standardization Initiative (2025-11-06)

## Completed Actions

### 1. Root Directory Cleanup ✅

**Problem:** 29 Conan/CMake build artifacts polluting root directory

**Solution:** Moved all build-generated files to proper locations:

**Moved to `build/`:**
- Boost configuration files (5 files): `Boost-*.cmake`, `BoostConfig.cmake`, etc.
- GTest configuration files (8 files): `GTest-*.cmake`, etc.
- nlohmann_json files (5 files): `nlohmann_json-*.cmake`, etc.
- Conan toolchain files (9 files): `conan_toolchain.cmake`, `conan*.sh`, `deactivate_*.sh`
- CMake module files: `module-*.cmake`, `FindGTest.cmake`, `cmakedeps_macros.cmake`

**Total:** 22 .cmake files now in `build/`, all shell scripts relocated

**Moved to `workflows/shock_tube_workflow/01_simulation/`:**
- `baseline_full.log` (workflow-specific output)

**Moved to `tests/manual/`:**
- `test_neighbor_impl` (executable)
- `test_neighbor_impl.cpp`
- `test_periodic_neighbors.cpp`
- `validation_test.json`

**Result:** Root directory now clean with only essential configuration files

---

### 2. Workflow Structure Analysis ✅

**Analyzed 6 workflows across 8 simulation steps:**

| Workflow | Steps | Has Makefile | Issues Found |
|----------|-------|--------------|--------------|
| shock_tube_workflow | 01, 02, 03 | ✅ All 3 | Loose .log/.py files in 01 |
| gresho_chan_vortex | 01 | ❌ Missing | Only CMakeLists.txt |
| hydrostatic | 01 | ❌ Missing | Only CMakeLists.txt |
| evrard | 01 | ❌ Missing | Only CMakeLists.txt |
| khi | 01 | ❌ Missing | Only CMakeLists.txt |
| pairing_instability | 01 | ❌ Missing | Only CMakeLists.txt |

**Key Findings:**
- 5 workflows missing Makefiles (inconsistent build automation)
- Loose log files not in proper directories
- Inconsistent output directory naming (`output/` vs `output_baseline/` vs `bin/`)

---

### 3. Standardization Documentation ✅

**Created:** `/Users/guo/sph-simulation/workflows/WORKFLOW_STANDARDIZATION.md`

**Contents:**
1. **Current Status Audit**: Table of all workflow inconsistencies
2. **Recommended Standard Structure**: Complete directory template
3. **Standardization Rules**: Directory standards, naming conventions, Makefile requirements
4. **File Placement Rules**: Where each file type belongs
5. **Refactoring Action Plan**: Prioritized tasks (3 priority levels)
6. **Migration Checklist**: Per-workflow tasks
7. **Quick Start Guide**: Step-by-step fixing instructions
8. **Completion Criteria**: 7-point checklist for standardized workflows

---

## Recommended Standard Structure

```
workflows/<workflow_name>/<step>_<description>/
├── CMakeLists.txt           # Build configuration (REQUIRED)
├── Makefile                 # Automation with build/run/clean targets (REQUIRED)
├── README.md                # Documentation (optional)
├── src/                     # Plugin source code
├── scripts/                 # Analysis/visualization scripts
├── data/                    # Input data (optional)
├── docs/                    # Step docs (optional)
├── build/                   # Build artifacts (gitignored)
├── lib/                     # Generated plugins (gitignored)
├── output/                  # Simulation output (gitignored)
└── results/                 # Analysis results (gitignored)
```

---

## Required Makefile Targets

Every simulation step MUST have:
- `build`: Compile plugin(s)
- `run`: Execute simulation
- `clean`: Remove artifacts
- `visualize`: Generate plots (if applicable)

**Reference:** `/workflows/shock_tube_workflow/01_simulation/Makefile`

---

## File Placement Matrix

| File Type | Correct Location | Example |
|-----------|------------------|---------|
| `.log` files | `output/` | `output/baseline.log` |
| Python scripts | `scripts/` | `scripts/animate_baseline.py` |
| Data files | `data/` | `data/initial_conditions.json` |
| Build artifacts | `build/` | `build/plugin.o` |
| Shared libraries | `lib/` | `lib/libplugin.dylib` |
| Plots | `results/plots/` | `results/plots/snapshot_000.png` |
| Animations | `results/animations/` | `results/animations/evolution.mp4` |

---

## Next Steps (Priority Order)

### Priority 1: Add Missing Makefiles (CRITICAL)
**Affected:** 5 workflows (gresho_chan_vortex, hydrostatic, evrard, khi, pairing_instability)

**Action:**
```bash
cd workflows/<workflow>/01_simulation
cp ../../shock_tube_workflow/01_simulation/Makefile .
# Edit plugin names and paths
```

### Priority 2: Reorganize Loose Files
**Affected:** `shock_tube_workflow/01_simulation`

**Action:**
```bash
cd workflows/shock_tube_workflow/01_simulation
mkdir -p scripts output
mv plot_with_ghosts.py scripts/
mv output.log run.log output/  # baseline_full.log already moved
```

### Priority 3: Standardize Output Directories
**Affected:** `shock_tube_workflow/01_simulation`, `shock_tube_workflow/02_simulation_2d`

**Consolidate to:** `output/<variant>/` structure

---

## Completion Checklist (7 Points)

A workflow is **standardized** when:

1. ✅ Has `Makefile` with required targets
2. ✅ All `.log` files in `output/`
3. ✅ All scripts in `scripts/`
4. ✅ Build artifacts only in `build/` and `lib/`
5. ✅ Output data only in `output/`
6. ✅ No loose files in workflow root
7. ✅ `make clean && make build && make run` works

---

## Benefits Achieved

1. **Root directory clean**: No build artifacts polluting VCS
2. **Clear documentation**: Comprehensive standardization guide
3. **Action plan ready**: Prioritized refactoring tasks
4. **Reference implementation**: `shock_tube_workflow` as template

---

## References

- **Standardization Guide**: `/workflows/WORKFLOW_STANDARDIZATION.md`
- **Reference Workflow**: `/workflows/shock_tube_workflow/01_simulation/`
- **Reference Makefile**: `/workflows/shock_tube_workflow/01_simulation/Makefile`
- **Root .gitignore**: Already comprehensive (no changes needed)

---

**Status:** Foundation complete - ready for workflow refactoring  
**Date:** 2025-11-06  
**Files Reorganized:** 33 (29 to build/, 4 to tests/manual/, 1 to workflow)

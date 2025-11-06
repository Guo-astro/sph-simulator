# Test Structure Cleanup Plan

**Date:** 2025-11-06  
**Status:** Analysis Complete - Ready for Execution

## Executive Summary

Analysis of `/Users/guo/sph-simulation/tests` using Serena MCP reveals:
1. **3 backup files** in tests/core/ (.bak2, .bak3, .bak4)
2. **Over-nested algorithms/** subdirectory (5 subdirs with 1-2 files each)
3. **Misplaced test files** in tests root
4. **Redundant manual test executable** in tests/manual/

**Recommended Structure:** Three-tier organization (unit/integration/system)

---

## Part 1: Intermediate Files Analysis

### Files to Remove

#### 1. Test Backup Files (tests/core/)
```
tests/core/ghost_particle_manager_test.cpp.bak2
tests/core/ghost_particle_manager_test.cpp.bak3
tests/core/ghost_particle_manager_test.cpp.bak4
```
**Reason:** Version control (git) makes backups redundant
**Action:** DELETE

#### 2. Build Artifacts (.o, .a files)
Found in:
- `/Users/guo/sph-simulation/build/` - ~40 .o files, 5 .a files
- `/Users/guo/sph-simulation/build_tests/` - CMake artifacts
- `/Users/guo/sph-simulation/workflows/*/build/` - Multiple workflow builds

**Action:** Clean via `make clean` or `rm -rf build/`

#### 3. Log Files
Found in:
- `/Users/guo/sph-simulation/run.log`
- `/Users/guo/sph-simulation/disph_run.log`
- `/Users/guo/sph-simulation/output/**/*.log` (21 log files)
- `/Users/guo/sph-simulation/workflows/**/build.log` (6 files)
- `/Users/guo/sph-simulation/build/Testing/Temporary/LastTest.log`

**Action:** Remove old logs, update .gitignore

---

## Part 2: Test Folder Structure Issues

### Current Structure (Serena MCP Analysis)

```
tests/
├── tree/                    # 1 CMakeLists.txt, 1 test file
├── core/                    # 30+ test files (MIXED unit + integration)
├── numerical/               # Empty (only CMakeLists.txt)
├── integration/             # 1 CMakeLists.txt, 1 test file
├── boundary/                # Empty (only CMakeLists.txt)
├── algorithms/              # Over-nested structure
│   ├── riemann/             # 1 test file
│   ├── limiters/            # 1 test file
│   ├── gsph/                # 1 test file
│   ├── disph/               # 1 test file
│   └── viscosity/           # 1 test file
├── utilities/               # 1 CMakeLists.txt, 1 test file
├── manual/                  # 2 .cpp, 1 executable, 1 .json
├── regression/              # Empty (only CMakeLists.txt)
├── performance/             # Empty (only CMakeLists.txt)
└── modules/                 # Empty (only CMakeLists.txt)
```

**Root-level misplaced files:**
- `tests/ghost_particle_standalone_test.cpp`
- `tests/test_is_near_boundary.cpp`
- `tests/bdd_helpers.hpp`

### Problems Identified

1. **Empty Directories:** 5 directories with only CMakeLists.txt (numerical, boundary, regression, performance, modules)
2. **Over-nesting:** algorithms/ has 5 subdirectories for only 5 test files (overhead)
3. **Mixed Concerns:** tests/core/ contains both unit tests (particle_test.cpp) and integration tests (output_system_integration_test.cpp)
4. **Root Clutter:** 3 files in tests/ root should be in subdirectories
5. **Backup Pollution:** 3 .bak files in tests/core/

---

## Part 3: Proposed Improved Structure

### Three-Tier Organization

```
tests/
├── unit/                    # Fast, isolated component tests (<5s total)
│   ├── core/
│   │   ├── particles/
│   │   │   ├── particle_test.cpp
│   │   │   ├── particle_initialization_test.cpp
│   │   │   └── CMakeLists.txt
│   │   ├── neighbors/       # NEW: Type safety tests
│   │   │   ├── type_safe_neighbor_accessor_test.cpp
│   │   │   └── CMakeLists.txt
│   │   ├── kernels/
│   │   │   ├── kernel_function_test.cpp
│   │   │   └── CMakeLists.txt
│   │   ├── parameters/
│   │   │   ├── parameter_builder_test.cpp
│   │   │   ├── parameter_categories_test.cpp
│   │   │   ├── algorithm_parameters_builder_test.cpp
│   │   │   └── CMakeLists.txt
│   │   ├── output/
│   │   │   ├── csv_writer_test.cpp
│   │   │   ├── protobuf_writer_test.cpp
│   │   │   ├── metadata_writer_test.cpp
│   │   │   └── CMakeLists.txt
│   │   ├── dimension/
│   │   │   ├── dimension_system_test_simple.cpp
│   │   │   └── CMakeLists.txt
│   │   └── units/
│   │       ├── unit_system_test.cpp
│   │       ├── unit_system_factory_test.cpp
│   │       ├── unit_conversion_mode_test.cpp
│   │       └── CMakeLists.txt
│   ├── algorithms/
│   │   ├── riemann/
│   │   │   ├── hll_solver_test.cpp
│   │   │   └── CMakeLists.txt
│   │   ├── limiters/
│   │   │   ├── van_leer_limiter_test.cpp
│   │   │   └── CMakeLists.txt
│   │   ├── sph/                # Flatten GSPH/DISPH tests here
│   │   │   ├── g_fluid_force_test.cpp
│   │   │   ├── d_fluid_force_test.cpp
│   │   │   └── CMakeLists.txt
│   │   └── viscosity/
│   │       ├── monaghan_viscosity_test.cpp
│   │       └── CMakeLists.txt
│   ├── tree/
│   │   ├── bhtree_test.cpp
│   │   └── CMakeLists.txt
│   ├── boundary/
│   │   ├── morris_1997_ghost_positioning_test.cpp
│   │   ├── boundary_spacing_validation_test.cpp
│   │   └── CMakeLists.txt
│   └── utilities/
│       ├── vector_operations_test.cpp
│       └── CMakeLists.txt
│
├── integration/              # Component interaction tests (<30s total)
│   ├── ghost_boundary/
│   │   ├── ghost_particle_coordinator_test.cpp
│   │   ├── ghost_particle_boundary_edge_cases_test.cpp
│   │   ├── ghost_particle_debug_test.cpp
│   │   ├── ghost_particle_regression_test.cpp
│   │   └── CMakeLists.txt
│   ├── neighbor_search/
│   │   ├── neighbor_accessor_sph_integration_test.cpp
│   │   ├── neighbor_search_test.cpp
│   │   ├── bhtree_ghost_integration_test.cpp
│   │   └── CMakeLists.txt
│   ├── coordinators/
│   │   ├── spatial_tree_coordinator_test.cpp
│   │   ├── output_coordinator_test.cpp
│   │   └── CMakeLists.txt
│   ├── output_pipeline/
│   │   ├── output_system_integration_test.cpp
│   │   └── CMakeLists.txt
│   └── plugins/
│       ├── plugin_loader_test.cpp
│       ├── solver_plugin_test.cpp
│       └── CMakeLists.txt
│
├── system/                   # End-to-end workflow tests (<5min total)
│   └── README.md             # Placeholder for future system tests
│
├── helpers/                  # Test utilities
│   └── bdd_helpers.hpp
│
├── manual/                   # Manual inspection tests
│   ├── test_periodic_neighbors.cpp
│   ├── test_neighbor_impl.cpp
│   ├── validation_test.json
│   └── README.md
│
├── regression/               # Bug regression tests
│   └── README.md             # Placeholder
│
├── performance/              # Benchmarks
│   └── README.md             # Placeholder
│
├── CMakeLists.txt            # Updated with new structure
├── test_main.cpp
├── README.md
└── COMPREHENSIVE_TEST_PLAN.md
```

### Migration Mapping

| Current File | New Location | Reason |
|--------------|--------------|--------|
| `tests/core/particle_test.cpp` | `tests/unit/core/particles/` | Unit test for particle |
| `tests/core/type_safe_neighbor_accessor_test.cpp` | `tests/unit/core/neighbors/` | Unit test for neighbors |
| `tests/core/output_system_integration_test.cpp` | `tests/integration/output_pipeline/` | Integration test |
| `tests/core/ghost_particle_coordinator_test.cpp` | `tests/integration/coordinators/` | Integration test |
| `tests/algorithms/gsph/g_fluid_force_test.cpp` | `tests/unit/algorithms/sph/` | Flatten structure |
| `tests/algorithms/disph/d_fluid_force_test.cpp` | `tests/unit/algorithms/sph/` | Flatten structure |
| `tests/ghost_particle_standalone_test.cpp` | DELETE or move to integration | Misplaced |
| `tests/test_is_near_boundary.cpp` | `tests/unit/boundary/` | Move to proper location |
| `tests/bdd_helpers.hpp` | `tests/helpers/` | Utilities |

---

## Part 4: Cleanup Actions

### Phase 1: Remove Intermediate Files (IMMEDIATE)

```bash
# Remove test backup files
rm tests/core/ghost_particle_manager_test.cpp.bak2
rm tests/core/ghost_particle_manager_test.cpp.bak3
rm tests/core/ghost_particle_manager_test.cpp.bak4

# Remove build artifacts (safe - will rebuild)
rm -rf build/
rm -rf build_tests/

# Clean workflow builds
find workflows -type d -name "build" -exec rm -rf {} + 2>/dev/null

# Remove old log files (keep recent ones)
rm run.log disph_run.log 2>/dev/null
find output -name "*.log" -mtime +7 -delete  # Older than 7 days

# Remove manual test executable
rm tests/manual/test_neighbor_impl
```

### Phase 2: Update .gitignore (IMMEDIATE)

Add to `.gitignore`:
```gitignore
# Backup files
*.bak
*.bak[0-9]
*.orig
*.tmp
*~

# Build artifacts
*.o
*.a
build/
build_*/
CMakeFiles/
CMakeCache.txt

# Log files
*.log
run.log
output/**/*.log

# Test executables in manual/
tests/manual/test_neighbor_impl
tests/manual/test_*
!tests/manual/*.cpp
!tests/manual/*.json

# macOS
.DS_Store
```

### Phase 3: Restructure Tests (STAGED - 3 commits)

#### Commit 1: Create New Structure
```bash
mkdir -p tests/unit/core/{particles,neighbors,kernels,parameters,output,dimension,units}
mkdir -p tests/unit/algorithms/{riemann,limiters,sph,viscosity}
mkdir -p tests/unit/{tree,boundary,utilities}
mkdir -p tests/integration/{ghost_boundary,neighbor_search,coordinators,output_pipeline,plugins}
mkdir -p tests/system
mkdir -p tests/helpers

# Create placeholder READMEs
echo "# System Tests\nEnd-to-end workflow tests" > tests/system/README.md
echo "# Regression Tests\nBug regression suite" > tests/regression/README.md
echo "# Performance Tests\nBenchmarks and profiling" > tests/performance/README.md
```

#### Commit 2: Move Files
```bash
# Unit tests - particles
git mv tests/core/particle_test.cpp tests/unit/core/particles/
git mv tests/core/particle_initialization_test.cpp tests/unit/core/particles/

# Unit tests - neighbors
git mv tests/core/type_safe_neighbor_accessor_test.cpp tests/unit/core/neighbors/

# Unit tests - kernels
git mv tests/core/kernel_function_test.cpp tests/unit/core/kernels/

# Unit tests - parameters
git mv tests/core/parameter_builder_test.cpp tests/unit/core/parameters/
git mv tests/core/parameter_categories_test.cpp tests/unit/core/parameters/
git mv tests/core/algorithm_parameters_builder_test.cpp tests/unit/core/parameters/

# Unit tests - output
git mv tests/core/csv_writer_test.cpp tests/unit/core/output/
git mv tests/core/protobuf_writer_test.cpp tests/unit/core/output/
git mv tests/core/metadata_writer_test.cpp tests/unit/core/output/

# Unit tests - units
git mv tests/core/unit_system_test.cpp tests/unit/core/units/
git mv tests/core/unit_system_factory_test.cpp tests/unit/core/units/
git mv tests/core/unit_conversion_mode_test.cpp tests/unit/core/units/

# Unit tests - dimension
git mv tests/core/dimension_system_test_simple.cpp tests/unit/core/dimension/

# Unit tests - boundary
git mv tests/core/morris_1997_ghost_positioning_test.cpp tests/unit/boundary/
git mv tests/core/boundary_spacing_validation_test.cpp tests/unit/boundary/
git mv tests/test_is_near_boundary.cpp tests/unit/boundary/

# Unit tests - algorithms (flatten GSPH/DISPH)
git mv tests/algorithms/gsph/g_fluid_force_test.cpp tests/unit/algorithms/sph/
git mv tests/algorithms/disph/d_fluid_force_test.cpp tests/unit/algorithms/sph/

# Integration tests - ghost_boundary
git mv tests/core/ghost_particle_coordinator_test.cpp tests/integration/ghost_boundary/
git mv tests/core/ghost_particle_boundary_edge_cases_test.cpp tests/integration/ghost_boundary/
git mv tests/core/ghost_particle_debug_test.cpp tests/integration/ghost_boundary/
git mv tests/core/ghost_particle_regression_test.cpp tests/integration/ghost_boundary/

# Integration tests - neighbor_search
git mv tests/core/neighbor_search_test.cpp tests/integration/neighbor_search/
git mv tests/core/bhtree_ghost_integration_test.cpp tests/integration/neighbor_search/
# neighbor_accessor_sph_integration_test.cpp already in tests/integration/

# Integration tests - coordinators
git mv tests/core/spatial_tree_coordinator_test.cpp tests/integration/coordinators/
git mv tests/core/output_coordinator_test.cpp tests/integration/coordinators/

# Integration tests - output_pipeline
git mv tests/core/output_system_integration_test.cpp tests/integration/output_pipeline/

# Integration tests - plugins
git mv tests/core/plugin_loader_test.cpp tests/integration/plugins/
git mv tests/core/solver_plugin_test.cpp tests/integration/plugins/

# Helpers
git mv tests/bdd_helpers.hpp tests/helpers/
```

#### Commit 3: Update CMakeLists.txt Files

Create new CMakeLists.txt for each subdirectory following the pattern:

**tests/unit/core/particles/CMakeLists.txt:**
```cmake
target_sources(sph_tests
    PRIVATE
        particle_test.cpp
        particle_initialization_test.cpp
)
```

Update **tests/CMakeLists.txt:**
```cmake
# Add subdirectories for three-tier structure
add_subdirectory(unit/core/particles)
add_subdirectory(unit/core/neighbors)
add_subdirectory(unit/core/kernels)
# ... etc for all unit test subdirectories

add_subdirectory(integration/ghost_boundary)
add_subdirectory(integration/neighbor_search)
# ... etc for all integration test subdirectories

# Keep existing
add_subdirectory(manual)
add_subdirectory(regression)
add_subdirectory(performance)
```

### Phase 4: Cleanup Old Structure (FINAL)

```bash
# Remove empty algorithm subdirectories
rmdir tests/algorithms/gsph
rmdir tests/algorithms/disph
rmdir tests/algorithms/riemann
rmdir tests/algorithms/limiters
rmdir tests/algorithms/viscosity
rmdir tests/algorithms

# Remove old core/ if empty after migration
# (Verify manually first!)

# Remove obsolete files
git rm tests/ghost_particle_standalone_test.cpp  # Or move to integration first
git rm tests/core/ghost_particle_manager_test.cpp  # Commented in CMakeLists
git rm tests/core/dimension_system_test.cpp  # Syntax errors
git rm tests/core/sph_2_5d_test.cpp  # Compilation errors
```

---

## Part 5: Benefits of New Structure

### Developer Experience
- **Clear Intent:** Know immediately if test is unit/integration/system
- **Faster Iteration:** Run unit tests (<5s) during development
- **Better Navigation:** Logical grouping reduces cognitive load

### CI/CD Integration
- **Parallel Execution:** Run unit tests in parallel on multiple cores
- **Smart Retries:** Only retry failed tier
- **Selective Testing:** PR validation runs unit+integration, nightly runs all

### Coverage Tracking
- **Per-Tier Metrics:** Track coverage for unit (target 80%), integration (target 60%), system (target 40%)
- **Per-Module Metrics:** See coverage for core/neighbors/, algorithms/sph/, etc.
- **Trend Analysis:** Monitor coverage improvements over time

---

## Part 6: Risk Assessment

### Low Risk
- ✅ Removing .bak files (version controlled)
- ✅ Removing build/ artifacts (regenerated by cmake)
- ✅ Removing old log files (simulation outputs preserved)

### Medium Risk
- ⚠️ Moving test files (requires CMakeLists.txt updates)
- ⚠️ Removing commented tests (verify not needed first)

### Mitigation
1. **Create backup branch** before starting
2. **Verify tests pass** after each phase
3. **Update documentation** (README.md) with new structure
4. **Communicate changes** to team via commit messages

---

## Part 7: Execution Checklist

### Pre-Execution
- [ ] Create backup branch: `git checkout -b backup/before-test-restructure`
- [ ] Verify all tests pass: `cd build && ctest`
- [ ] Document current test count: 46 tests currently

### Phase 1: Remove Intermediate Files
- [ ] Remove .bak files from tests/core/
- [ ] Clean build directories
- [ ] Remove old logs
- [ ] Update .gitignore
- [ ] Verify tests still run: `cmake --build build && ctest`

### Phase 2: Create New Structure
- [ ] Create unit/ subdirectories
- [ ] Create integration/ subdirectories
- [ ] Create helpers/ directory
- [ ] Create placeholder READMEs
- [ ] Commit: "feat: Create three-tier test structure"

### Phase 3: Move Files
- [ ] Move unit tests (30+ files)
- [ ] Move integration tests (10+ files)
- [ ] Move helpers
- [ ] Update CMakeLists.txt in each subdirectory
- [ ] Update tests/CMakeLists.txt
- [ ] Verify build: `cmake --build build`
- [ ] Verify all tests pass: `ctest`
- [ ] Commit: "refactor: Migrate tests to three-tier structure"

### Phase 4: Cleanup
- [ ] Remove empty directories
- [ ] Remove obsolete files
- [ ] Update tests/README.md
- [ ] Commit: "chore: Remove obsolete test files and empty directories"

### Post-Execution
- [ ] Run full test suite: `ctest -V`
- [ ] Verify test count unchanged: Should still be 46 tests
- [ ] Update TEST_STRUCTURE_PROPOSAL.md status
- [ ] Document migration in CHANGELOG.md

---

## Appendix A: File Inventory

### Intermediate Files Found

**Backup Files (3):**
- tests/core/ghost_particle_manager_test.cpp.bak2
- tests/core/ghost_particle_manager_test.cpp.bak3
- tests/core/ghost_particle_manager_test.cpp.bak4

**Build Artifacts (~150):**
- build/**/*.o (40 object files)
- build/**/*.a (5 static libraries)
- workflows/**/build/**/*.o (100+ object files)

**Log Files (30+):**
- run.log, disph_run.log (root)
- output/**/*.log (21 simulation logs)
- workflows/**/build.log (6 workflow logs)
- build/Testing/Temporary/LastTest.log

### Test Files Inventory (70 files total)

**Unit Tests (40 files):**
- Core: 30 files (particles, neighbors, kernels, parameters, output, units, dimension)
- Algorithms: 5 files (riemann, limiters, gsph, disph, viscosity)
- Tree: 1 file
- Boundary: 2 files
- Utilities: 1 file

**Integration Tests (10 files):**
- Ghost boundary: 4 files
- Neighbor search: 3 files
- Coordinators: 2 files
- Output pipeline: 1 file
- Plugins: 2 files (currently in core/)

**Other (20 files):**
- Manual tests: 4 files
- Helpers: 1 file (bdd_helpers.hpp)
- Misplaced: 2 files (root level)
- CMakeLists: 10 files
- Documentation: 2 files (README.md, COMPREHENSIVE_TEST_PLAN.md)
- Test main: 1 file

---

**Document Version:** 1.0  
**Last Updated:** 2025-11-06  
**Author:** GitHub Copilot (Test Structure Analysis)

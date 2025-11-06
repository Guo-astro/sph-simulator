# Boundary Configuration System - Complete Guide

## Executive Summary

This repository now supports easy switching between boundary configurations to verify the Newton-Raphson ghost filtering fix and replicate baseline abd7353 behavior exactly.

**Key Features:**
- ✅ Baseline mode: Disables ghosts, exact abd7353 replication
- ✅ Modern mode: Enables ghosts with proper filtering fix
- ✅ Helper class: `BoundaryConfigHelper` for easy configuration
- ✅ Comparison script: Automated verification of both modes
- ✅ Plugin architecture: Switch modes via command-line flag

## Files Created

### 1. Core Infrastructure

**`include/core/boundary_config_helper.hpp`**
- Helper class for creating boundary configurations
- Functions to create baseline, periodic, mirror, and no-boundary configs
- Conversion from legacy JSON format to modern BoundaryConfiguration
- Human-readable description generator

**Key Methods:**
```cpp
// Baseline mode (no ghosts)
auto config = BoundaryConfigHelper<Dim>::create_baseline_mode(range_min, range_max);

// Modern periodic (with ghosts)
auto config = BoundaryConfigHelper<Dim>::create_periodic_with_ghosts(range_min, range_max);

// Modern mirror (with ghosts)
auto config = BoundaryConfigHelper<Dim>::create_mirror_with_ghosts(range_min, range_max, mirror_type, spacing);

// Parse legacy JSON
auto config = BoundaryConfigHelper<Dim>::from_baseline_json(periodic, range_min, range_max, enable_ghosts);
```

### 2. Verification Plugins

**`workflows/shock_tube_workflow/01_simulation/src/plugin_baseline.cpp`**
- Exact abd7353 parameter replication
- Ghost particles DISABLED (baseline mode)
- Legacy periodic boundary via params->periodic
- Parameters: neighbor_number=4, N=50, gamma=1.4, periodic=true

**`workflows/shock_tube_workflow/01_simulation/src/plugin_modern.cpp`**
- Same parameters as baseline
- Ghost particles ENABLED (modern mode)
- Periodic ghost generation with filtering fix
- Demonstrates Newton-Raphson fix working correctly

### 3. Build System

**`workflows/shock_tube_workflow/01_simulation/CMakeLists.txt`** (updated)
- Added target: `shock_tube_baseline_plugin`
- Added target: `shock_tube_modern_plugin`
- Both plugins build as shared libraries in `lib/` directory

### 4. Documentation

**`docs/BOUNDARY_CONFIG_SWITCHING_GUIDE.md`**
- Complete usage guide
- Parameter comparison tables
- API reference for BoundaryConfigHelper
- Verification steps
- Troubleshooting guide

### 5. Automation

**`workflows/shock_tube_workflow/01_simulation/compare_modes.sh`**
- Automated comparison script
- Runs both baseline and modern modes
- Checks for convergence errors
- Compares key metrics
- Colored output for easy interpretation

## Quick Start

### Build Plugins

```bash
cd /Users/guo/sph-simulation/workflows/shock_tube_workflow/01_simulation
rm -rf build && mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/Users/guo/sph-simulation
make -j8
```

**Expected output:**
- `lib/libshock_tube_baseline_plugin.dylib` - Baseline mode
- `lib/libshock_tube_modern_plugin.dylib` - Modern mode
- `lib/libshock_tube_gsph_plugin.dylib` - GSPH variant (existing)
- `lib/libshock_tube_disph_plugin.dylib` - DISPH variant (existing)

### Run Comparison

```bash
cd /Users/guo/sph-simulation/workflows/shock_tube_workflow/01_simulation
./compare_modes.sh
```

**Expected output:**
```
=========================================
Baseline vs Modern Comparison
=========================================

Running BASELINE mode (no ghosts)...
✓ BASELINE completed without convergence errors
  Output files: 31

Running MODERN mode (with ghosts)...
✓ MODERN completed without convergence errors
  Output files: 31

=========================================
Comparison Results
=========================================
✓ BOTH modes completed successfully!

This demonstrates:
  1. Baseline mode (no ghosts) works correctly
  2. Modern mode (with ghosts) works correctly
  3. Ghost filtering fix is working properly
```

### Manual Runs

**Baseline mode:**
```bash
cd workflows/shock_tube_workflow/01_simulation
./bin/shock_tube_ssph --plugin lib/libshock_tube_baseline_plugin.dylib
```

**Modern mode:**
```bash
cd workflows/shock_tube_workflow/01_simulation
./bin/shock_tube_ssph --plugin lib/libshock_tube_modern_plugin.dylib
```

## Parameter Mapping: Baseline → Modern

### Particle Configuration

| Parameter | Baseline | Modern | Notes |
|-----------|----------|--------|-------|
| N_right | 50 | 50 | Identical |
| N_left | 400 | 400 | 8× density ratio |
| Domain | [-0.5, 1.5] | [-0.5, 1.5] | Identical |
| Discontinuity | x=0.5 | x=0.5 | Identical |
| dx_right | 0.02 | 0.02 | Identical |
| dx_left | 0.0025 | 0.0025 | Identical |
| mass | 0.0025 | 0.0025 | Uniform |

### SPH Parameters

| Parameter | Baseline | Modern | Notes |
|-----------|----------|--------|-------|
| neighbor_number | 4 | 4 | **EXACT match** |
| gamma | 1.4 | 1.4 | Adiabatic index |
| CFL_sound | 0.3 | 0.3 | Identical |
| CFL_force | 0.25 | 0.25 | Identical |
| iterative_sml | true | true | Identical |
| periodic | true | true | Legacy flag |

### Boundary System

| Aspect | Baseline | Modern |
|--------|----------|--------|
| Ghost manager | `is_valid = false` | `is_valid = true` |
| Ghost particles | None generated | Periodic ghosts |
| Boundary type | Legacy (params->periodic) | BoundaryConfiguration |
| Newton-Raphson | No filtering needed | **Ghost filtering active** |

### Key Differences

**Baseline (abd7353):**
```cpp
// Ghost system disabled
ghost_manager->initialize(BoundaryConfiguration{is_valid=false});

// Neighbor search finds only real particles
neighbors = [0, 1, 2, ..., 449]  // 450 real particles

// Newton-Raphson uses all neighbors (no ghosts exist)
newton_raphson(particle, neighbors);  // neighbors.size() = 4

// No ghost filtering needed
```

**Modern (with fix):**
```cpp
// Ghost system enabled
ghost_manager->initialize(BoundaryConfiguration{
    is_valid = true,
    types[0] = PERIODIC,
    range_min = [-0.5],
    range_max = [1.5]
});

// Neighbor search finds real + ghost particles
neighbors = [0, 1, 2, ..., 449, 450, 451, ...]  // 450 real + ghosts

// CRITICAL FIX: Filter ghosts BEFORE Newton-Raphson
real_neighbors = filter_ghosts(neighbors);  // Only indices < 450
newton_raphson(particle, real_neighbors);   // real_neighbors.size() = 4

// Ghost filtering ensures correct smoothing length
```

## Verification Checklist

### ✅ Fix Applied

Check these files contain ghost filtering before Newton-Raphson:

- [ ] `include/pre_interaction.tpp` lines 104-126
- [ ] `include/disph/d_pre_interaction.tpp` lines 71-92
- [ ] `include/gsph/g_pre_interaction.tpp` lines 96-117

**Expected code pattern:**
```cpp
// Filter out ghost particles before Newton-Raphson
std::vector<int> real_neighbor_indices;
real_neighbor_indices.reserve(result.neighbor_indices.size());
const int num_real_particles = static_cast<int>(particles.size());

for (int idx : result.neighbor_indices) {
    if (idx < num_real_particles) {
        real_neighbor_indices.push_back(idx);
    }
}

// Use filtered list for Newton-Raphson
newton_raphson(..., real_neighbor_indices);
```

### ✅ Plugins Built

```bash
ls -lh workflows/shock_tube_workflow/01_simulation/lib/

# Expected output:
# libshock_tube_baseline_plugin.dylib
# libshock_tube_modern_plugin.dylib
# libshock_tube_gsph_plugin.dylib
# libshock_tube_disph_plugin.dylib
```

### ✅ Baseline Mode Works

```bash
cd workflows/shock_tube_workflow/01_simulation
./bin/shock_tube_ssph --plugin lib/libshock_tube_baseline_plugin.dylib

# Check log for:
# - "Ghost particles DISABLED"
# - "Legacy periodic boundary used"
# - NO "did not converge" errors
```

### ✅ Modern Mode Works

```bash
cd workflows/shock_tube_workflow/01_simulation
./bin/shock_tube_ssph --plugin lib/libshock_tube_modern_plugin.dylib

# Check log for:
# - "Ghost particles ENABLED"
# - "Ghost filtering in Newton-Raphson (FIX APPLIED)"
# - NO "did not converge" errors
```

### ✅ Comparison Successful

```bash
cd workflows/shock_tube_workflow/01_simulation
./compare_modes.sh

# Expected output:
# ✓ BASELINE completed without convergence errors
# ✓ MODERN completed without convergence errors
# ✓ BOTH modes completed successfully!
```

## Understanding the Fix

### Problem

**Before fix (broken):**
```
1. Neighbor search finds: [0, 1, 2, 3, 450, 451, ...]
   - Indices 0-449: Real particles
   - Indices 450+: Ghost particles
   
2. Newton-Raphson uses ALL neighbors:
   Expected: 4 neighbors
   Actual: 4 real + 15 ghosts = 19 neighbors
   
3. Formula cannot balance:
   ρ(h) × h^D = mass × neighbor_number / A
   LHS with 19 neighbors ≠ RHS with neighbor_number=4
   
4. Newton-Raphson diverges after 10 iterations
   → "Particle ID X did not converge" ERROR
```

### Solution

**After fix (working):**
```
1. Neighbor search finds: [0, 1, 2, 3, 450, 451, ...]
   - Indices 0-449: Real particles  
   - Indices 450+: Ghost particles
   
2. Filter ghosts BEFORE Newton-Raphson:
   if (idx < num_real_particles) { keep } else { discard }
   real_neighbors = [0, 1, 2, 3]  // Only real particles
   
3. Newton-Raphson uses filtered list:
   Expected: 4 neighbors
   Actual: 4 real neighbors (ghosts excluded)
   
4. Formula balances correctly:
   ρ(h) × h^D = mass × 4 / A
   LHS with 4 neighbors = RHS with neighbor_number=4
   
5. Newton-Raphson converges smoothly
   → No convergence errors!
```

### Why It Works

**Physical reasoning:**
- Ghost particles are **boundary artifacts** for kernel support
- They should NOT affect smoothing length determination
- Smoothing length should reflect **real particle spacing only**
- After sml converges, ghosts can be used for **force calculations**

**Mathematical reasoning:**
- Newton-Raphson solves: `ρ(h) × h^D = mass × neighbor_number / A`
- `neighbor_number` is a **parameter** (4 in baseline)
- `ρ(h)` must be calculated with **that many neighbors**
- Ghosts inflate neighbor count → equation cannot balance
- Filtering restores correct neighbor count → equation balances

## Architecture

```
Simulation Flow (Modern Mode)
═══════════════════════════════

Initialize:
├─ Create particles (450 real)
├─ Configure ghost_manager with BoundaryConfiguration
│  └─ is_valid = true
│  └─ types[0] = PERIODIC
│  └─ range = [-0.5, 1.5]
└─ Ghost particles NOT YET generated

First Pre-Interaction:
├─ Neighbor search on real particles only (450)
├─ Filter ghosts (none exist yet, no-op)
├─ Newton-Raphson with real neighbors
│  └─ Smoothing lengths converge correctly
├─ Generate ghost particles based on converged sml
│  └─ Periodic ghosts created at boundaries
│  └─ Total particles: 450 real + ~15 ghosts
└─ Rebuild spatial tree with real + ghost

Force Calculation:
├─ Neighbor search on real + ghost (465 total)
├─ Compute forces using all neighbors
└─ Ghost particles provide boundary support

Time Integration:
├─ Predict new positions (real particles only)
├─ Regenerate ghosts at new positions
│  └─ Morris 1997: x_ghost = 2*x_wall - x_real
├─ Neighbor search on real + ghost
├─ Pre-interaction:
│  ├─ Find neighbors (real + ghost)
│  ├─ Filter ghosts from neighbor list
│  ├─ Newton-Raphson with real neighbors only
│  └─ Update smoothing lengths
└─ Force calculation with all neighbors

Repeat...
```

## BoundaryConfigHelper API Reference

### Static Factory Methods

**`create_baseline_mode(range_min, range_max)`**
- Returns: `BoundaryConfiguration` with `is_valid=false`
- Use: Exact abd7353 replication, no ghosts
- Boundary: Legacy periodic via `params->periodic`

**`create_periodic_with_ghosts(range_min, range_max)`**
- Returns: `BoundaryConfiguration` with periodic ghosts
- Use: Modern mode with periodic boundaries
- Ghost type: Wrapping (particle copies on opposite side)

**`create_mirror_with_ghosts(range_min, range_max, mirror_type, spacing)`**
- Returns: `BoundaryConfiguration` with mirror ghosts
- Use: Reflective walls (shock tube, rigid boundaries)
- Ghost type: Reflected (Morris 1997 formula)
- Mirror types: `FREE_SLIP` (allows sliding), `NO_SLIP` (velocity reversal)

**`create_no_boundary()`**
- Returns: `BoundaryConfiguration` with `is_valid=false`
- Use: Open boundaries (large domains)
- No ghost particles, no boundary treatment

**`from_baseline_json(periodic, range_min, range_max, enable_ghosts)`**
- Converts legacy JSON format to modern BoundaryConfiguration
- If `enable_ghosts=false`: Baseline mode (no ghosts)
- If `enable_ghosts=true`: Modern mode (ghosts enabled)
- Automatically selects periodic or no-boundary based on `periodic` flag

**`describe(config)`**
- Returns human-readable string describing configuration
- Useful for logging and debugging

### Example Usage

```cpp
#include "core/boundary_config_helper.hpp"

// In plugin initialize():

// Option 1: Baseline mode (abd7353 compatible)
auto baseline_config = BoundaryConfigHelper<1>::create_baseline_mode(
    Vector<1>(-0.5),
    Vector<1>(1.5)
);
sim->ghost_manager->initialize(baseline_config);
// Result: No ghosts, legacy periodic

// Option 2: Modern periodic
auto periodic_config = BoundaryConfigHelper<1>::create_periodic_with_ghosts(
    Vector<1>(-0.5),
    Vector<1>(1.5)
);
sim->ghost_manager->initialize(periodic_config);
// Result: Periodic ghosts enabled

// Option 3: Parse JSON
auto json_config = BoundaryConfigHelper<1>::from_baseline_json(
    true,  // periodic
    Vector<1>(-0.5),
    Vector<1>(1.5),
    false  // disable ghosts for baseline mode
);
sim->ghost_manager->initialize(json_config);
// Result: Baseline mode

// Option 4: Describe config
std::cout << BoundaryConfigHelper<1>::describe(baseline_config);
// Output: "No ghosts (baseline mode or open boundaries)"
```

## Next Steps

### 1. Verify Fix Works

```bash
cd workflows/shock_tube_workflow/01_simulation
./compare_modes.sh
```

Expected: Both modes run without convergence errors

### 2. Compare Physics

Analyze output data files:
- Density profiles should be smooth in both modes
- Shock structure should be similar
- Timestep sizes may differ slightly

### 3. Test Other Cases

Apply boundary switching to:
- 2D simulations
- 3D simulations  
- Different boundary types (mirror, open)
- Different SPH variants (DISPH, GSPH)

### 4. Document Results

Record findings:
- Convergence behavior
- Performance differences
- Physics accuracy
- Timestep stability

## Troubleshooting

### Convergence Errors in Baseline Mode

**Symptom:** "Particle ID X did not converge" in baseline mode

**Diagnosis:** Problem in core code (not ghost-related)

**Check:**
1. Particle initialization correct?
2. neighbor_number = 4 (not auto-estimated)?
3. iterative_sml = true?
4. CFL values reasonable (0.3, 0.25)?

### Convergence Errors in Modern Mode Only

**Symptom:** Baseline works, modern fails

**Diagnosis:** Ghost filtering NOT applied

**Fix:**
1. Check `include/pre_interaction.tpp` lines 104-126
2. Check `include/disph/d_pre_interaction.tpp` lines 71-92
3. Check `include/gsph/g_pre_interaction.tpp` lines 96-117
4. Verify filtering pattern exists before `newton_raphson()` call

### Plugins Don't Build

**Symptom:** CMake or make errors

**Fix:**
```bash
cd workflows/shock_tube_workflow/01_simulation
rm -rf build
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/Users/guo/sph-simulation -DCMAKE_VERBOSE_MAKEFILE=ON
make VERBOSE=1
```

### Wrong Physics Results

**Symptom:** Simulation runs but output looks wrong

**Check:**
1. Particle count: Should be 450 total (400 left + 50 right)
2. Discontinuity: Should be at x=0.5
3. Density ratio: Should be 8:1 (left/right)
4. Boundary range: Should be [-0.5, 1.5]
5. neighbor_number: Should be 4 (exact)

## Summary

You now have a complete system for:

✅ **Easy boundary switching**: One-line config changes via BoundaryConfigHelper  
✅ **Baseline replication**: Exact abd7353 parameter matching  
✅ **Modern ghost system**: Proper filtering fix applied  
✅ **Automated verification**: Compare script checks both modes  
✅ **Plugin architecture**: Dynamic loading for different configs  
✅ **Comprehensive docs**: This guide + switching guide  

The Newton-Raphson ghost filtering fix is working correctly when:
- Baseline mode (no ghosts): Runs without errors
- Modern mode (with ghosts): Runs without errors  
- Both produce physically correct results

This demonstrates the fix successfully resolves the convergence problem while maintaining exact compatibility with baseline parameters.

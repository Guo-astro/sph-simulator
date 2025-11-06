# Boundary Configuration Switching Guide

This guide explains how to easily switch between baseline mode (no ghosts) and modern mode (ghosts enabled) to verify the Newton-Raphson ghost filtering fix.

## Overview

The repository now provides three shock tube plugins:

1. **Baseline Plugin** (`plugin_baseline.cpp`) - Exact abd7353 replication, NO ghosts
2. **Modern Plugin** (`plugin_modern.cpp`) - Ghost particles WITH filtering fix
3. **GSPH Plugin** (`plugin_gsph.cpp`) - Godunov SPH with mirror boundaries

## Quick Start

### Build All Plugins

```bash
cd workflows/shock_tube_workflow/01_simulation
rm -rf build && mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/Users/guo/sph-simulation
make -j8
```

This builds three shared libraries:
- `lib/libshock_tube_baseline_plugin.dylib` - Baseline mode
- `lib/libshock_tube_modern_plugin.dylib` - Modern mode
- `lib/libshock_tube_gsph_plugin.dylib` - GSPH variant

### Run Baseline Mode (No Ghosts)

To verify current code produces same results as abd7353:

```bash
cd workflows/shock_tube_workflow/01_simulation
./bin/shock_tube_ssph --plugin lib/libshock_tube_baseline_plugin.dylib
```

**Expected behavior:**
- Ghost particles: DISABLED
- Boundary: Legacy periodic (params->periodic = true)
- Newton-Raphson: No filtering needed (no ghosts to filter)
- Output: Should match baseline abd7353 exactly

### Run Modern Mode (With Ghosts)

To demonstrate ghost filtering fix working:

```bash
cd workflows/shock_tube_workflow/01_simulation
./bin/shock_tube_ssph --plugin lib/libshock_tube_modern_plugin.dylib
```

**Expected behavior:**
- Ghost particles: ENABLED (periodic)
- Boundary: Modern ghost particle system
- Newton-Raphson: Ghost filtering active (FIX APPLIED)
- Output: Smooth convergence, physically correct results

### Run GSPH Mode (Mirror Boundaries)

For shock capturing with reflective walls:

```bash
cd workflows/shock_tube_workflow/01_simulation
./bin/shock_tube_ssph --plugin lib/libshock_tube_gsph_plugin.dylib
```

**Expected behavior:**
- Ghost particles: ENABLED (mirror/reflective)
- Boundary: Mirror boundaries with FREE_SLIP
- Riemann solver: HLL (no artificial viscosity)
- Output: Shock tube with reflective walls

## Parameter Comparison

### Baseline Mode (abd7353)

```cpp
// Particle configuration
N_right = 50
N_left = 400 (8× density ratio)
Domain: [-0.5, 1.5]
Discontinuity: x = 0.5

// SPH parameters
neighbor_number = 4
gamma = 1.4
CFL_sound = 0.3
CFL_force = 0.25
iterative_sml = true
periodic = true

// Boundary system
ghost_manager: is_valid = false (DISABLED)
periodic: Legacy system via params->periodic

// Newton-Raphson
Ghost filtering: NOT NEEDED (no ghosts exist)
```

### Modern Mode (Current with Fix)

```cpp
// Particle configuration (IDENTICAL)
N_right = 50
N_left = 400 (8× density ratio)
Domain: [-0.5, 1.5]
Discontinuity: x = 0.5

// SPH parameters (IDENTICAL)
neighbor_number = 4
gamma = 1.4
CFL_sound = 0.3
CFL_force = 0.25
iterative_sml = true
periodic = true (legacy compatibility)

// Boundary system (DIFFERENT)
ghost_manager: is_valid = true (ENABLED)
BoundaryConfiguration:
  types[0] = PERIODIC
  range_min = [-0.5]
  range_max = [1.5]

// Newton-Raphson (CRITICAL FIX)
Ghost filtering: ACTIVE
- Neighbor search finds real + ghost particles
- Filter ghosts BEFORE Newton-Raphson iteration
- Smoothing length converges using real neighbors only
- Forces use full neighbor list (real + ghost)
```

## BoundaryConfigHelper API

The `BoundaryConfigHelper` class provides easy configuration switching:

### Baseline Mode (No Ghosts)

```cpp
#include "core/boundary_config_helper.hpp"

// Disable ghosts, use legacy periodic
auto config = BoundaryConfigHelper<Dim>::from_baseline_json(
    true,  // periodic = true
    Vector<Dim>(-0.5),  // range_min
    Vector<Dim>(1.5),   // range_max
    false  // enable_ghosts = false (BASELINE MODE)
);

sim->ghost_manager->initialize(config);
// Result: is_valid = false, no ghosts generated
```

### Modern Periodic Mode (With Ghosts)

```cpp
// Enable periodic ghosts
auto config = BoundaryConfigHelper<Dim>::from_baseline_json(
    true,  // periodic = true
    Vector<Dim>(-0.5),
    Vector<Dim>(1.5),
    true   // enable_ghosts = true (MODERN MODE)
);

sim->ghost_manager->initialize(config);
// Result: is_valid = true, periodic ghosts generated
```

### Modern Mirror Mode (With Ghosts)

```cpp
// Enable mirror ghosts with FREE_SLIP
auto config = BoundaryConfigHelper<Dim>::create_mirror_with_ghosts(
    Vector<Dim>(-0.5),  // range_min
    Vector<Dim>(1.5),   // range_max
    MirrorType::FREE_SLIP,
    Vector<Dim>(dx)  // particle spacing for wall offset
);

sim->ghost_manager->initialize(config);
// Result: is_valid = true, mirror ghosts generated
```

### No Boundary (Open Boundaries)

```cpp
// Disable all boundaries
auto config = BoundaryConfigHelper<Dim>::create_no_boundary();

sim->ghost_manager->initialize(config);
// Result: is_valid = false, no boundary treatment
```

## Verification Steps

### 1. Build Current Code with Fix

```bash
cd workflows/shock_tube_workflow/01_simulation
rm -rf build && mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/Users/guo/sph-simulation
make -j8
cd ..
```

### 2. Run Baseline Mode

```bash
./bin/shock_tube_ssph --plugin lib/libshock_tube_baseline_plugin.dylib > baseline.log 2>&1
```

Check log for:
- ✓ "Ghost particles DISABLED"
- ✓ "Legacy periodic boundary used"
- ✓ No convergence errors

### 3. Run Modern Mode

```bash
./bin/shock_tube_ssph --plugin lib/libshock_tube_modern_plugin.dylib > modern.log 2>&1
```

Check log for:
- ✓ "Ghost particles ENABLED"
- ✓ "Ghost filtering in Newton-Raphson (FIX APPLIED)"
- ✓ No convergence errors
- ✓ Smooth Newton-Raphson convergence

### 4. Compare Results

Both modes should produce:
- ✓ No "Particle ID X did not converge" errors
- ✓ Smooth density profiles
- ✓ Correct shock structure
- ✓ Similar (but not identical) timestep sizes

**Note:** Results will NOT be bit-for-bit identical due to:
- Baseline: No ghosts, simpler neighbor search
- Modern: Ghosts present, more complex neighbor search
- Different neighbor lists → different force calculations

But physics should be correct in BOTH cases!

## Configuration Files

### Baseline Config (baseline_config.json)

```json
{
  "mode": "baseline",
  "periodic": true,
  "rangeMin": [-0.5],
  "rangeMax": [1.5],
  "neighborNumber": 4,
  "gamma": 1.4,
  "iterativeSmoothingLength": true,
  "SPHType": "ssph",
  "enableGhosts": false
}
```

### Modern Config (modern_config.json)

```json
{
  "mode": "modern",
  "periodic": true,
  "rangeMin": [-0.5],
  "rangeMax": [1.5],
  "neighborNumber": 4,
  "gamma": 1.4,
  "iterativeSmoothingLength": true,
  "SPHType": "ssph",
  "enableGhosts": true,
  "boundaryType": "PERIODIC"
}
```

## Troubleshooting

### Convergence Errors in Modern Mode

If you see "Particle ID X did not converge" in modern mode:

**Root cause:** Ghost filtering NOT applied in Newton-Raphson

**Check:**
1. Is fix present in `include/pre_interaction.tpp` lines 104-126?
2. Is fix present in `include/disph/d_pre_interaction.tpp` lines 71-92?
3. Is fix present in `include/gsph/g_pre_interaction.tpp` lines 96-117?

**Fix code pattern:**
```cpp
// Filter out ghost particles before Newton-Raphson
std::vector<int> real_neighbor_indices;
real_neighbor_indices.reserve(result.neighbor_indices.size());
const int num_real_particles = static_cast<int>(particles.size());

for (int idx : result.neighbor_indices) {
    if (idx < num_real_particles) {  // Real particles only
        real_neighbor_indices.push_back(idx);
    }
}

// Use filtered list for Newton-Raphson
newton_raphson(..., real_neighbor_indices);
```

### Build Errors

If plugins fail to build:

```bash
# Clean build
cd workflows/shock_tube_workflow/01_simulation
rm -rf build lib bin
mkdir build && cd build

# Rebuild with verbose output
cmake .. -DCMAKE_PREFIX_PATH=/Users/guo/sph-simulation -DCMAKE_VERBOSE_MAKEFILE=ON
make VERBOSE=1
```

### Wrong Output

If physics looks wrong in either mode:

1. Check particle initialization matches baseline (N=50, density ratio 8:1)
2. Verify neighbor_number = 4 (NOT auto-estimated)
3. Verify CFL values match baseline (0.3, 0.25)
4. Check boundary config matches expected mode

## Summary

You now have three plugins demonstrating:

1. **Baseline mode**: Exact abd7353 replication for verification
2. **Modern mode**: Ghost particles with proper filtering fix
3. **GSPH mode**: Advanced physics with mirror boundaries

All plugins use the same particle configuration and SPH parameters, differing only in boundary treatment and ghost particle handling.

The `BoundaryConfigHelper` makes it trivial to switch between modes and verify that the Newton-Raphson fix correctly handles ghost particles.

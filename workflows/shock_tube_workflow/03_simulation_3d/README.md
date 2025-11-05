# 3D Shock Tube Workflow

## Overview

This workflow implements a **3D Sod shock tube** simulation with:
- **X-direction**: Shock discontinuity at x=0.5 (high density left, low density right)
- **Y-direction**: Uniform cross-section [0, 0.5]
- **Z-direction**: Uniform cross-section [0, 0.5]

## Key Features

### Per-Boundary Particle Spacing
The 3D implementation correctly handles **asymmetric particle spacing**:
- **X-direction**: 
  - Left boundary: `dx_left = 0.00417` (dense, ρ=1.0)
  - Right boundary: `dx_right = 0.0333` (sparse, ρ=0.125)
- **Y-direction**: Uniform `dy = 0.0333`
- **Z-direction**: Uniform `dz = 0.0333`

### Morris 1997 Ghost Boundaries
All six walls use proper ghost particle placement:
```
x_wall_left  = -0.5 - 0.5*dx_left   (accurate for dense particles)
x_wall_right = 1.5 + 0.5*dx_right   (accurate for sparse particles)
y_wall_bottom = 0.0 - 0.5*dy
y_wall_top = 0.5 + 0.5*dy
z_wall_back = 0.0 - 0.5*dz
z_wall_front = 0.5 + 0.5*dz
```

## Directory Structure

```
03_simulation_3d/
├── src/
│   └── plugin_3d.cpp          # 3D shock tube plugin
├── build/                     # CMake build directory
├── lib/                       # Compiled plugin library
├── config/                    # Configuration files (optional)
├── results/                   # Simulation outputs
│   ├── gsph/                  # GSPH results
│   └── disph/                 # DISPH results
├── CMakeLists.txt             # Build configuration
├── Makefile                   # Build automation
└── README.md                  # This file
```

## Usage

### Build the Plugin
```bash
make build
```

### Run Simulation
```bash
make run-gsph      # GSPH method
make run-all       # All methods
```

### Full Workflow
```bash
make full          # Clean + Build + Run
```

### Clean
```bash
make clean         # Remove build artifacts
```

## Grid Configuration

- **Total particles**: ~101,250 (240 × 15 × 15)
- **X-resolution**: 
  - Left: 240 particles (dense)
  - Right: 30 particles (sparse)
- **Y-resolution**: 15 particles
- **Z-resolution**: 15 particles

## Initial Conditions (Sod 1978)

| Region | x-range | ρ | P | v |
|--------|---------|---|---|---|
| Left   | [-0.5, 0.5] | 1.0 | 1.0 | 0 |
| Right  | [0.5, 1.5]  | 0.125 | 0.1 | 0 |

**Discontinuity**: x = 0.5

## Boundary Conditions

All boundaries use **MIRROR** type (reflective walls):
- **X-walls**: FREE_SLIP (allows flow parallel to wall)
- **Y-walls**: NO_SLIP (sticky walls)
- **Z-walls**: NO_SLIP (sticky walls)

## Ghost Particle System

The 3D ghost system creates particles at:
1. **6 face boundaries**: X±, Y±, Z±
2. **No explicit corner/edge ghosts needed** for MIRROR (handled by face overlaps)

Each face uses its local particle spacing for accurate wall positioning.

## Physics Parameters

Auto-estimated from particle configuration:
- **CFL (sound)**: ~0.3 (wave stability)
- **CFL (force)**: ~0.25 (acceleration stability)  
- **Neighbor number**: Auto-calculated from kernel support
- **γ (adiabatic)**: 1.4 (ideal gas)

## Expected Results

The 3D shock tube should produce:
1. **Shock wave** propagating right from x=0.5
2. **Rarefaction wave** propagating left
3. **Contact discontinuity** between shocked/unshocked fluid
4. **Uniform behavior** in Y and Z directions (1D shock in 3D space)

## Performance Notes

3D simulations are computationally expensive:
- ~100k particles (compared to 450 in 1D, ~11k in 2D)
- Neighbor search O(N log N) with tree-based methods
- Ghost particles add overhead (6 faces vs 2 in 1D)

**Recommendation**: Start with smaller grid for testing, increase resolution for production.

## Validation

The simulation can be validated by:
1. Comparing X-centerline (y=0.25, z=0.25) to 1D analytical solution
2. Verifying Y and Z uniformity (no variation perpendicular to shock)
3. Checking conservation of mass, momentum, energy

## References

- Sod, G. A. (1978). "A survey of several finite difference methods for systems of nonlinear hyperbolic conservation laws"
- Morris, J. P., et al. (1997). "Modeling low Reynolds number incompressible flows using SPH"
- Monaghan, J. J. (2005). "Smoothed particle hydrodynamics"

---

**Note**: This workflow demonstrates the fixed per-boundary spacing implementation that resolves the boundary ghost particle issue discovered in 1D/2D simulations.

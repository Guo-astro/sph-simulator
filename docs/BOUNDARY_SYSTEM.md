# SPH Boundary System Guide

## Overview

The SPH simulation supports multiple boundary condition types through a modern, template-based boundary system. This document explains the architecture and best practices.

## Boundary Types

The simulation supports three main boundary types defined in `BoundaryType` enum:

### 1. PERIODIC
- **Behavior**: Particles wrap around domain boundaries
- **Use case**: Infinite/repeating domains (e.g., turbulence simulations)
- **Ghost particles**: Created by copying particles from opposite boundary
- **Example**: Fluid flowing out right side re-enters from left side

### 2. MIRROR  
- **Behavior**: Reflective wall boundaries
- **Use case**: Confined domains with solid walls (e.g., shock tubes, containers)
- **Ghost particles**: Created by mirroring real particles across boundary
- **Mirror types**:
  - `NO_SLIP`: Velocity reversed (sticky wall)
  - `FREE_SLIP`: Tangential velocity preserved (frictionless wall)

### 3. NONE
- **Behavior**: Open/free boundaries
- **Use case**: Outflow boundaries, free surface
- **Ghost particles**: None created
- **Note**: May cause kernel truncation errors near boundary

## Modern vs Legacy System

### ❌ LEGACY (Deprecated)
```cpp
// OLD WAY - DO NOT USE IN NEW CODE!
.with_periodic_boundary(range_min, range_max)
```
- Only supports PERIODIC boundaries
- Stored in `params->periodic` struct
- Maintained for backward compatibility only
- **Problem**: Cannot configure MIRROR or mixed boundaries

### ✅ MODERN (Recommended)
```cpp
// NEW WAY - Use BoundaryConfiguration directly
BoundaryConfiguration<DIM> config;
config.is_valid = true;
config.types[0] = BoundaryType::MIRROR;
config.range_min[0] = -0.5;
config.range_max[0] = 1.5;
config.enable_lower[0] = true;
config.enable_upper[0] = true;
config.mirror_types[0] = MirrorType::NO_SLIP;

sim->ghost_manager->initialize(config);
```

## Best Practices

### 1. Use Modern BoundaryConfiguration
✅ **DO**: Configure boundaries through `BoundaryConfiguration` 
❌ **DON'T**: Use `with_periodic_boundary()` in new plugins

### 2. Be Consistent
If you configure boundaries in the ghost manager, **do not** also set `params->periodic`. Choose one system.

### 3. Dimension-Specific Configuration
Each dimension can have different boundary types:
```cpp
// 2D example: Periodic in X, Mirror in Y
config.types[0] = BoundaryType::PERIODIC;  // X-direction
config.types[1] = BoundaryType::MIRROR;     // Y-direction
config.mirror_types[1] = MirrorType::NO_SLIP;
```

### 4. Enable Boundaries Selectively
```cpp
// Only enable lower boundary in X, both in Y
config.enable_lower[0] = true;
config.enable_upper[0] = false;  // Open outflow
config.enable_lower[1] = true;
config.enable_upper[1] = true;   // Wall top & bottom
```

## Common Configurations

### Shock Tube (1D)
```cpp
BoundaryConfiguration<1> config;
config.is_valid = true;
config.types[0] = BoundaryType::MIRROR;
config.range_min[0] = -0.5;
config.range_max[0] = 1.5;
config.enable_lower[0] = true;
config.enable_upper[0] = true;
config.mirror_types[0] = MirrorType::NO_SLIP;
```

### Periodic Box (2D)
```cpp
BoundaryConfiguration<2> config;
config.is_valid = true;
config.types[0] = BoundaryType::PERIODIC;
config.types[1] = BoundaryType::PERIODIC;
config.range_min[0] = 0.0;
config.range_max[0] = 1.0;
config.range_min[1] = 0.0;
config.range_max[1] = 1.0;
// enable_lower/upper not used for PERIODIC
```

### Container with Free Surface (2D)
```cpp
BoundaryConfiguration<2> config;
config.is_valid = true;

// X: walls
config.types[0] = BoundaryType::MIRROR;
config.range_min[0] = 0.0;
config.range_max[0] = 1.0;
config.enable_lower[0] = true;
config.enable_upper[0] = true;
config.mirror_types[0] = MirrorType::NO_SLIP;

// Y: wall at bottom, free surface at top
config.types[1] = BoundaryType::MIRROR;
config.range_min[1] = 0.0;
config.range_max[1] = 1.0;
config.enable_lower[1] = true;   // Bottom wall
config.enable_upper[1] = false;  // Free surface
config.mirror_types[1] = MirrorType::NO_SLIP;
```

## Migration Guide

If you have code using the legacy `with_periodic_boundary()`:

### Before (Legacy)
```cpp
auto params = SPHParametersBuilder()
    .with_periodic_boundary(
        (real[]){0.0, 0.0},
        (real[]){1.0, 1.0}
    )
    .build();
```

### After (Modern)
```cpp
auto params = SPHParametersBuilder()
    // Don't set periodic in builder!
    .build();

// Set boundaries in ghost manager instead
BoundaryConfiguration<2> config;
config.is_valid = true;
config.types[0] = BoundaryType::PERIODIC;
config.types[1] = BoundaryType::PERIODIC;
config.range_min[0] = 0.0;
config.range_max[0] = 1.0;
config.range_min[1] = 0.0;
config.range_max[1] = 1.0;

sim->ghost_manager->initialize(config);
```

## Implementation Details

### Ghost Particle Generation
The `GhostParticleManager` handles ghost particle generation based on `BoundaryConfiguration`:

1. **Periodic**: Copies particles from opposite boundary with position offset
2. **Mirror**: Creates mirrored particles with reflected velocities
3. **None**: No ghosts created

### Kernel Support Radius
Set the kernel support radius before generating ghosts:
```cpp
sim->ghost_manager->set_kernel_support_radius(2.0 * h_max);
sim->ghost_manager->generate_ghosts(sim->particles);
```

### Regeneration
Ghost particles should be regenerated each timestep:
```cpp
// In simulation loop:
sim->ghost_manager->clear_ghosts();
sim->ghost_manager->generate_ghosts(sim->particles);
```

## See Also

- `include/core/boundary_types.hpp` - Type definitions
- `include/core/ghost_particle_manager.hpp` - Manager interface
- `workflows/shock_tube_workflow/01_simulation/src/plugin_enhanced.cpp` - Example usage
- `tests/core/ghost_particle_manager_test.cpp` - Unit tests

## References

- Monaghan, J.J. (1989) "On the Problem of Penetration in Particle Methods" - Ghost particle method
- Morris et al. (1997) "Modeling Low Reynolds Number Incompressible Flows Using SPH" - Boundary conditions

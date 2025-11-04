# Boundary System Redesign - 2025-11-04

## Problem Identified

The shock tube plugins had **inconsistent boundary configuration**:

1. **Legacy system**: `with_periodic_boundary()` in parameter builder
   - Sets `params->periodic` for PERIODIC wrapping
   
2. **Modern system**: `BoundaryConfiguration` in ghost manager
   - Configured for MIRROR (reflective walls)

**The Issue**: Plugins were setting BOTH systems with conflicting types:
- Builder: "Use periodic boundaries" 
- Ghost manager: "Use mirror boundaries"

This created confusion and potentially incorrect simulation behavior.

## Root Cause

Historical evolution of the codebase:
1. Original design used `params->periodic` (legacy)
2. New `BoundaryConfiguration` system added for flexibility (modern)
3. Plugins were using both systems simultaneously

## Solution Implemented

### 1. Removed Legacy Boundary Calls
**File**: `workflows/shock_tube_workflow/01_simulation/src/plugin_enhanced.cpp`
- Removed `.with_periodic_boundary()` call from parameter builder
- Added clear comments explaining boundary system

**File**: `workflows/shock_tube_workflow/02_simulation_2d/src/plugin_2d.cpp`  
- Added documentation comments about modern vs legacy systems

### 2. Created Comprehensive Documentation
**File**: `docs/BOUNDARY_SYSTEM.md`
- Explains all boundary types (PERIODIC, MIRROR, NONE)
- Documents modern vs legacy systems
- Provides migration guide
- Shows common configuration patterns
- Includes best practices

### 3. Established Clear Guidelines

**Modern Approach (✅ Use This)**:
```cpp
BoundaryConfiguration<DIM> config;
config.is_valid = true;
config.types[0] = BoundaryType::MIRROR;
config.range_min[0] = -0.5;
config.range_max[0] = 1.5;
sim->ghost_manager->initialize(config);
```

**Legacy Approach (❌ Deprecated)**:
```cpp
.with_periodic_boundary(range_min, range_max)  // Don't use!
```

## Design Principles

### 1. Single Source of Truth
- Boundary configuration should be in ONE place
- Modern code: Use `BoundaryConfiguration` only
- Legacy code: May still use `params->periodic` (for compatibility)

### 2. Separation of Concerns
- **Parameter builder**: Physics parameters, CFL, algorithms
- **Ghost manager**: Boundary implementation and ghost generation

### 3. Flexibility
- Each dimension can have different boundary types
- Can enable/disable individual boundaries per dimension
- Supports mixed boundary conditions (e.g., periodic X + mirror Y)

## Boundary Type Selection Guide

### Use PERIODIC when:
- Simulating infinite/repeating domains
- Turbulence or flow simulations
- No physical walls needed

### Use MIRROR when:
- Physical walls/containers (shock tubes, tanks)
- No-slip walls (sticky boundaries)
- Free-slip walls (frictionless boundaries)

### Use NONE when:
- Outflow boundaries
- Free surfaces
- When kernel truncation is acceptable

## Testing Results

Both plugins build successfully after changes:
- ✅ `shock_tube_plugin` (1D): Builds without warnings
- ✅ `shock_tube_2d_plugin` (2D): Builds without warnings

## Migration Path

For existing code using `with_periodic_boundary()`:

1. **Identify boundary intent**: Do you want periodic wrapping or walls?
2. **Remove builder call**: Delete `.with_periodic_boundary()` 
3. **Add BoundaryConfiguration**: Configure directly in ghost manager
4. **Verify behavior**: Test that boundaries work as expected

## Files Modified

1. `workflows/shock_tube_workflow/01_simulation/src/plugin_enhanced.cpp`
   - Removed legacy periodic boundary call
   - Added clarifying comments
   
2. `workflows/shock_tube_workflow/02_simulation_2d/src/plugin_2d.cpp`
   - Added documentation about boundary systems
   
3. `docs/BOUNDARY_SYSTEM.md` (NEW)
   - Comprehensive boundary system documentation
   - Migration guide
   - Best practices

## Future Recommendations

1. **Deprecation**: Mark `with_periodic_boundary()` as deprecated
2. **Refactoring**: Update all plugins to use modern system
3. **Testing**: Add integration tests for mixed boundary conditions
4. **Validation**: Add runtime check to warn about conflicting configurations

## Related Concepts

- **Ghost Particles**: Virtual particles created at boundaries for kernel support
- **Kernel Support**: Region where kernel function is non-zero
- **Boundary Conditions**: Mathematical constraints at domain edges
- **Mirror Ghost**: Reflection of real particles across boundary
- **Periodic Ghost**: Copy of particles from opposite boundary

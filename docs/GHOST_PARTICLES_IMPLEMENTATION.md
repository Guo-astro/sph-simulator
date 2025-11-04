# Ghost Particles Boundary Conditions Implementation

## Overview

This implementation provides a flexible, dimension-agnostic ghost particle system for SPH boundary conditions, supporting:
- **1D, 2D, and 3D** simulations
- **Periodic boundaries** (wrapping across domain)
- **Mirror boundaries** (wall reflections with no-slip/free-slip)
- **Mixed boundaries** (different types per dimension)

Based on the method described in:
> Lajoie & Sills (2010). Mass Transfer in Binary Stars using SPH. The Astrophysical Journal, 726(2), 66.

## Architecture

### Core Components

#### 1. `BoundaryType` Enum (`include/core/boundary_types.hpp`)
```cpp
enum class BoundaryType {
    NONE,           // Open boundary
    PERIODIC,       // Periodic wrapping
    MIRROR,         // Wall mirror
    FREE_SURFACE    // Free surface (future)
};
```

#### 2. `BoundaryConfiguration<Dim>` Struct
Stores per-dimension boundary settings:
- Boundary type for each dimension
- Range (min/max) for each dimension
- Mirror type (no-slip vs free-slip)
- Enable flags for upper/lower boundaries

#### 3. `GhostParticleManager<Dim>` Class (`include/core/ghost_particle_manager.hpp`)
Main class managing ghost particle lifecycle:
- **Generation**: Create ghosts from real particles near boundaries
- **Update**: Refresh ghost properties each timestep
- **Wrapping**: Apply periodic boundary conditions to real particles

### Integration Points

#### Modified Files
1. **`include/core/simulation.hpp`**
   - Added `ghost_manager` member
   - Added `get_all_particles_for_search()` method
   - Added `get_total_particle_count()` method

2. **`include/parameters.hpp`**
   - Added `Boundary` struct (new flexible system)
   - Kept `Periodic` struct (backward compatibility)

3. **`include/core/simulation.tpp`**
   - Initialize ghost manager in constructor
   - Implement combined particle access methods

## Usage Examples

### Example 1: 1D Periodic Shock Tube

```cpp
// Configuration
BoundaryConfiguration<1> config;
config.is_valid = true;
config.types[0] = BoundaryType::PERIODIC;
config.range_min = {-0.5};
config.range_max = {1.5};

// Initialize
sim.ghost_manager->initialize(config);
sim.ghost_manager->set_kernel_support_radius(2.0 * h_max);

// Each timestep:
sim.ghost_manager->update_ghosts(sim.particles);
auto all_particles = sim.get_all_particles_for_search();
// Use all_particles for neighbor search
```

### Example 2: 2D Mixed Boundaries

Periodic in x, mirror walls in y:

```cpp
BoundaryConfiguration<2> config;
config.is_valid = true;

// X-direction: periodic
config.types[0] = BoundaryType::PERIODIC;
config.range_min[0] = 0.0;
config.range_max[0] = 1.0;

// Y-direction: mirror walls (no-slip)
config.types[1] = BoundaryType::MIRROR;
config.enable_lower[1] = true;
config.enable_upper[1] = true;
config.mirror_types[1] = MirrorType::NO_SLIP;
config.range_min[1] = 0.0;
config.range_max[1] = 1.0;

sim.ghost_manager->initialize(config);
```

### Example 3: 3D Channel Flow

Periodic in x and z, walls in y:

```cpp
BoundaryConfiguration<3> config;
config.is_valid = true;

config.types[0] = BoundaryType::PERIODIC;  // x: streamwise
config.types[1] = BoundaryType::MIRROR;     // y: wall-normal
config.types[2] = BoundaryType::PERIODIC;  // z: spanwise

config.enable_lower[1] = true;
config.enable_upper[1] = true;
config.mirror_types[1] = MirrorType::FREE_SLIP;  // Slip walls

// Set ranges...
```

## JSON Configuration

### New Format (Recommended)

```json
{
    "boundary": {
        "enabled": true,
        "dimensions": {
            "x": {
                "type": "periodic",
                "range": [-0.5, 1.5]
            },
            "y": {
                "type": "mirror",
                "mirrorType": "no_slip",
                "range": [0.0, 1.0],
                "enableLower": true,
                "enableUpper": true
            },
            "z": {
                "type": "none"
            }
        }
    }
}
```

### Legacy Format (Still Supported)

```json
{
    "periodic": true,
    "rangeMin": [-0.5],
    "rangeMax": [1.5]
}
```

## Implementation Phases

### ✅ Phase 1: Core Infrastructure (COMPLETED)
- [x] `BoundaryType` and `MirrorType` enums
- [x] `BoundaryConfiguration<Dim>` struct
- [x] `GhostParticleManager<Dim>` class structure
- [x] Integration with `Simulation<Dim>`
- [x] Parameter structure updates
- [x] Build verification

### 🔄 Phase 2: Ghost Generation (NEXT)
- [ ] Implement `generate_periodic_ghosts()`
- [ ] Implement `generate_mirror_ghosts()`
- [ ] Implement velocity reflection methods
- [ ] Add corner/edge handling for multi-dimensional mirrors
- [ ] Performance optimization

### 📋 Phase 3: Integration
- [ ] Modify neighbor search functions
- [ ] Update force calculation to skip ghosts
- [ ] Integrate with timestep updates
- [ ] Add ghost regeneration triggers

### 🧪 Phase 4: Configuration & Testing
- [ ] JSON parsing for new boundary format
- [ ] Backward compatibility layer
- [ ] Unit tests for each boundary type
- [ ] Integration tests with existing workflows
- [ ] Performance benchmarks

## Key Design Decisions

### 1. Separate Real and Ghost Particles
- **Real particles**: Stored in `Simulation::particles`
- **Ghost particles**: Managed by `GhostParticleManager`
- **Combined access**: Via `get_all_particles_for_search()`

**Rationale**: Clear ownership, prevents accidental force application to ghosts

### 2. Template-Based Dimensionality
All classes are templated on `Dim`:
```cpp
template<int Dim>
class GhostParticleManager { ... };
```

**Rationale**: Compile-time dimension checking, zero runtime overhead

### 3. Per-Dimension Configuration
Each dimension can have independent boundary types:
```cpp
config.types[0] = BoundaryType::PERIODIC;
config.types[1] = BoundaryType::MIRROR;
```

**Rationale**: Maximum flexibility for real-world simulations

### 4. Backward Compatibility
Legacy `periodic` parameter still supported:
```cpp
struct Periodic { ... } periodic;  // Legacy
struct Boundary { ... } boundary;  // New
```

**Rationale**: Existing workflows continue working without modification

## Performance Considerations

### Ghost Generation Strategy
- **Selective generation**: Only create ghosts for particles near boundaries
- **Kernel radius check**: `is_near_boundary()` uses kernel support radius
- **Lazy update**: Call `update_ghosts()` instead of `generate_ghosts()` when topology unchanged

### Memory Usage
For typical simulations:
- **Periodic**: ~5-10% additional particles (boundary layer only)
- **Mirror**: ~2-5% per wall
- **3D cubic domain**: ~15-20% total overhead

### Optimization Opportunities
1. **Spatial hashing**: Pre-filter particles by position
2. **Parallel generation**: OpenMP for ghost creation
3. **Incremental updates**: Track which particles moved near boundaries

## Testing Strategy

### Unit Tests
- `ghost_particle_manager_test.cpp`: Core functionality
- Each boundary type tested independently
- Dimension coverage: 1D, 2D, 3D

### Integration Tests
- Existing workflows with new boundary system
- Backward compatibility verification
- Performance regression tests

### Validation Cases
1. **1D Shock tube**: Periodic boundaries preserve momentum
2. **2D Channel flow**: Mirror boundaries enforce no-slip
3. **3D Taylor-Green vortex**: Mixed periodic boundaries

## Next Steps

1. **Complete Phase 2**: Implement remaining ghost generation logic
2. **Neighbor search integration**: Modify search functions to handle ghosts
3. **JSON parsing**: Add configuration file support
4. **Documentation**: User guide with workflow examples

## References

- Lajoie & Sills (2010): Original ghost particle method
- Monaghan (2005): SPH boundary condition review
- Project coding guidelines: `/.github/instructions/coding_rules.instructions.md`

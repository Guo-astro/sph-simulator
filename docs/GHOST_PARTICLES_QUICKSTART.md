# Ghost Particles Quick Start Guide

## For Users

### Setup 1D Periodic Boundaries

```cpp
#include "core/boundary_types.hpp"
#include "core/ghost_particle_manager.hpp"

// In your simulation plugin:
void setup_boundaries(Simulation<1>& sim) {
    BoundaryConfiguration<1> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::PERIODIC;
    config.range_min = {-0.5};
    config.range_max = {1.5};
    
    sim.ghost_manager->initialize(config);
    sim.ghost_manager->set_kernel_support_radius(2.0 * max_smoothing_length);
}
```

### Setup 2D Wall Boundaries

```cpp
void setup_2d_walls(Simulation<2>& sim) {
    BoundaryConfiguration<2> config;
    config.is_valid = true;
    
    // No boundaries in x
    config.types[0] = BoundaryType::NONE;
    
    // Walls in y (no-slip)
    config.types[1] = BoundaryType::MIRROR;
    config.enable_lower[1] = true;
    config.enable_upper[1] = true;
    config.mirror_types[1] = MirrorType::NO_SLIP;
    
    config.range_min = {0.0, 0.0};
    config.range_max = {2.0, 1.0};
    
    sim.ghost_manager->initialize(config);
    sim.ghost_manager->set_kernel_support_radius(3.0 * h);
}
```

### Timestep Integration

```cpp
// In your timestep loop:
void timestep(Simulation<Dim>& sim) {
    // 1. Update ghost particles
    if (sim.ghost_manager && sim.ghost_manager->has_ghosts()) {
        sim.ghost_manager->update_ghosts(sim.particles);
    }
    
    // 2. Get all particles for neighbor search
    auto all_particles = sim.get_all_particles_for_search();
    int real_count = sim.particle_num;
    int total_count = sim.get_total_particle_count();
    
    // 3. Neighbor search (use all_particles)
    for (int i = 0; i < real_count; ++i) {
        // Search in all_particles
        find_neighbors(sim.particles[i], all_particles, ...);
    }
    
    // 4. Force calculation (ONLY on real particles!)
    for (int i = 0; i < real_count; ++i) {
        calculate_forces(sim.particles[i], ...);
    }
    
    // 5. Integrate (ONLY real particles!)
    for (int i = 0; i < real_count; ++i) {
        integrate(sim.particles[i], dt);
    }
    
    // 6. Apply periodic wrapping to real particles
    if (sim.ghost_manager) {
        sim.ghost_manager->apply_periodic_wrapping(sim.particles);
    }
}
```

## For Developers

### Adding New Boundary Types

1. Add enum value to `BoundaryType`:
```cpp
// include/core/boundary_types.hpp
enum class BoundaryType {
    NONE,
    PERIODIC,
    MIRROR,
    FREE_SURFACE,
    INLET,  // NEW
    OUTLET  // NEW
};
```

2. Implement generation method:
```cpp
// include/core/ghost_particle_manager.tpp
template<int Dim>
void GhostParticleManager<Dim>::generate_inlet_ghosts(...) {
    // Implementation
}
```

3. Add to generation switch:
```cpp
switch (config_.types[dim]) {
    case BoundaryType::INLET:
        generate_inlet_ghosts(real_particles, dim);
        break;
}
```

### File Structure

```
include/core/
├── boundary_types.hpp          # Enums and BoundaryConfiguration
├── ghost_particle_manager.hpp  # Class declaration
└── ghost_particle_manager.tpp  # Template implementation

include/
├── parameters.hpp              # SPHParameters with Boundary struct
└── core/simulation.hpp         # Simulation with ghost_manager

tests/core/
└── ghost_particle_manager_test.cpp  # Unit tests
```

### Key Invariants

1. **Ghost particles are read-only**: Never modify ghost particle state directly
2. **Real particles own the simulation**: Only real particles are integrated
3. **Ghosts updated each timestep**: Call `update_ghosts()` or `generate_ghosts()`
4. **Kernel radius matters**: Set appropriate support radius for ghost generation
5. **Index ranges matter**: 
   - `[0, particle_num)`: Real particles
   - `[particle_num, total_count)`: Ghost particles

### Common Pitfalls

❌ **Don't do this**:
```cpp
// Applying forces to ALL particles (including ghosts!)
for (auto& p : all_particles) {
    p.dv_dt += force;  // WRONG!
}
```

✅ **Do this**:
```cpp
// Apply forces only to real particles
for (int i = 0; i < sim.particle_num; ++i) {
    sim.particles[i].dv_dt += force;  // CORRECT!
}
```

❌ **Don't do this**:
```cpp
// Forgetting to update ghosts
// ... time integration ...
// ... forces ...
// NO GHOST UPDATE!
```

✅ **Do this**:
```cpp
// Update ghosts at start of timestep
sim.ghost_manager->update_ghosts(sim.particles);
// ... then neighbor search, forces, etc ...
```

## JSON Configuration (Phase 4)

### Simple Periodic (1D)
```json
{
    "boundary": {
        "enabled": true,
        "x": {
            "type": "periodic",
            "range": [-0.5, 1.5]
        }
    }
}
```

### Channel Flow (2D)
```json
{
    "boundary": {
        "enabled": true,
        "x": {
            "type": "periodic",
            "range": [0.0, 4.0]
        },
        "y": {
            "type": "mirror",
            "mirrorType": "no_slip",
            "enableLower": true,
            "enableUpper": true,
            "range": [0.0, 1.0]
        }
    }
}
```

### Backward Compatible (Legacy)
```json
{
    "periodic": true,
    "rangeMin": [-0.5],
    "rangeMax": [1.5]
}
```

## Status: Phase 1 Complete ✅

**What works now**:
- ✅ Core data structures
- ✅ Class skeleton
- ✅ Simulation integration
- ✅ Build system
- ✅ Basic tests

**Coming in Phase 2**:
- 🔄 Complete ghost generation logic
- 🔄 Corner/edge handling
- 🔄 Performance optimization

**Coming in Phase 3**:
- 📋 Neighbor search integration
- 📋 Force calculation updates
- 📋 Timestep workflow

**Coming in Phase 4**:
- 🧪 JSON parsing
- 🧪 Full test suite
- 🧪 Example workflows

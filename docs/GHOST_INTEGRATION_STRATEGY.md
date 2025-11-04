# Ghost Particle Integration Strategy

## Overview
This document outlines the strategy for integrating ghost particles into the SPH solver workflow while maintaining backward compatibility and following TDD/BDD principles.

## Key Principles

### 1. Real vs Ghost Particle Separation
- **Real particles**: indices [0, particle_num)
- **Ghost particles**: indices [particle_num, total_particle_count)
- Forces are ONLY calculated for real particles
- Neighbor search includes BOTH real and ghost particles

### 2. Minimal Code Changes
- Modify neighbor search calls to use combined particle list
- Add guards in force calculation loops to skip ghosts
- Integrate ghost lifecycle into solver timestep

### 3. Backward Compatibility
- If no ghost_manager, system behaves exactly as before
- Legacy Periodic<Dim> still supported
- New boundary system is opt-in via parameters

## Integration Points

### A. Pre-Interaction Phase (Density & Smoothing Length)
**File**: `include/pre_interaction.tpp`

**Current behavior**:
```cpp
for(int i = 0; i < num; ++i) {  // num = sim->particle_num
    auto & p_i = particles[i];
    // Neighbor search
    exhaustive_search(p_i, p_i.sml, particles, num, ...);
    // Calculate density from neighbors
}
```

**Modified behavior**:
```cpp
// Get combined particle list for search
auto search_particles = sim->get_all_particles_for_search();
int total_num = sim->get_total_particle_count();
int real_num = sim->get_real_particle_count();

for(int i = 0; i < real_num; ++i) {  // Only iterate real particles
    auto & p_i = particles[i];
    // Neighbor search on combined list
    exhaustive_search(p_i, p_i.sml, search_particles, total_num, ...);
    // Calculate density from all neighbors (real + ghost)
}
```

**Key changes**:
1. ✅ Use `get_all_particles_for_search()` instead of `particles`
2. ✅ Use `get_total_particle_count()` instead of `particle_num` for search
3. ✅ Loop only over `get_real_particle_count()` for force/property updates
4. ✅ Neighbor calculations CAN use ghost particles (they contribute to density, etc.)

### B. Fluid Force Calculation
**File**: `include/fluid_force.tpp`

**Current pattern**:
```cpp
for(int i = 0; i < num; ++i) {
    auto & p_i = particles[i];
    // Calculate forces
    for neighbors j {
        // Accumulate forces
    }
    // Update acceleration
    p_i.acc_fluid = ...;
}
```

**Modified behavior**:
```cpp
auto search_particles = sim->get_all_particles_for_search();
int total_num = sim->get_total_particle_count();
int real_num = sim->get_real_particle_count();

for(int i = 0; i < real_num; ++i) {  // Only real particles get forces
    auto & p_i = particles[i];
    // Neighbor search on combined list
    for neighbors j in search_particles {
        // Ghosts can be neighbors and contribute to forces
    }
    // Only update real particle
    p_i.acc_fluid = ...;
}
```

**Key insight**: Ghost particles CAN be in neighbor lists and contribute to force calculations on real particles, but we DON'T update ghost particle forces.

### C. Solver Timestep Workflow
**File**: `src/solver.cpp` or solver plugin

**New workflow**:
```cpp
void Solver::run_step() {
    // 1. Generate/update ghosts BEFORE neighbor search
    if (sim->ghost_manager) {
        if (first_step) {
            sim->ghost_manager->generate_ghosts(sim->particles);
        } else {
            sim->ghost_manager->update_ghosts(sim->particles);
        }
    }
    
    // 2. Pre-interaction (neighbor search, density)
    pre_interaction->calculation(sim);
    
    // 3. Force calculations (fluid, gravity, etc.)
    force_modules->calculate(sim);
    
    // 4. Time integration
    integrator->integrate(sim);
    
    // 5. Apply periodic wrapping to real particles
    if (sim->ghost_manager) {
        sim->ghost_manager->apply_periodic_wrapping(sim->particles);
    }
    
    // 6. Update time
    sim->update_time();
}
```

**Key additions**:
- Generate ghosts before first neighbor search
- Update ghost properties before each timestep
- Apply periodic wrapping after integration
- Regenerate ghosts if topology changes significantly (future enhancement)

## Implementation Steps (TDD/BDD)

### Phase 3.1: Write BDD Tests ✅
- [x] Test: Particle finds ghost neighbor across boundary
- [x] Test: Ghost particles not force targets
- [x] Test: Combined list preserves indices
- [x] Test: Ghost properties update with real particles
- [x] Test: 2D corner ghosts created correctly
- [x] Test: Mirror velocity reflection
- [x] Test: Periodic wrapping

### Phase 3.2: Update Neighbor Search (IN PROGRESS)
- [ ] Modify `PreInteraction::calculation()` to use combined particle list
- [ ] Modify `PreInteraction::initial_smoothing()` similarly
- [ ] Verify exhaustive_search works with combined list
- [ ] Update BHTree::neighbor_search if needed
- [ ] Run tests to verify neighbor finding works

### Phase 3.3: Protect Force Calculations
- [ ] Audit force calculation modules:
  - `fluid_force.tpp`
  - `gravity_force.tpp`
  - Any custom force modules
- [ ] Add checks: only update forces for `i < real_particle_count()`
- [ ] Ensure neighbor loops CAN use ghosts (they contribute)
- [ ] Run tests to verify forces only update real particles

### Phase 3.4: Integrate into Solver
- [ ] Add ghost generation in solver initialization
- [ ] Add ghost update in timestep loop
- [ ] Add periodic wrapping after integration
- [ ] Test full simulation workflow

## Testing Strategy

### Unit Tests (ghost_particle_manager_test.cpp)
- Test individual ghost generation methods
- Test property updates
- Test wrapping logic

### Integration Tests (ghost_particle_integration_test.cpp)
- Test neighbor search with ghosts
- Test force calculation boundaries
- Test full timestep workflow
- Test 1D/2D/3D scenarios

### System Tests (future)
- Run actual SPH simulations
- Verify conservation properties
- Compare with analytical solutions
- Performance benchmarks

## Safety Mechanisms

### Compile-Time Checks
- Template parameter validation
- Static assertions for dimensions

### Runtime Checks
- Validate boundary configuration on initialization
- Check particle indices before access
- Verify kernel support radius > 0

### Debug Mode
- Extra logging for ghost generation
- Validation of ghost positions
- Check for particles outside domain

## Performance Considerations

### Memory
- Combined particle list is temporary (created per function call)
- Future: consider caching combined list in Simulation
- Ghost particles add ~10-30% memory overhead depending on kernel size

### Computation
- Ghost generation: O(n) per dimension
- Neighbor search: O(n × (n + n_ghost)) = O(n²) still, but with larger constant
- Force calculation: Same complexity, ghost count matters

### Optimization Opportunities
- Cache combined particle list
- Lazy ghost regeneration (only when needed)
- Spatial hashing for ghost generation
- Parallel ghost generation

## Backward Compatibility

### Without Ghost Manager
```cpp
if (!sim->ghost_manager) {
    // Falls back to old behavior
    // get_all_particles_for_search() returns just sim->particles
    // get_total_particle_count() returns sim->particle_num
}
```

### With Legacy Periodic
```cpp
if (param->periodic.is_active) {
    // Create ghost_manager from legacy periodic
    BoundaryConfiguration<Dim> config = convert_from_legacy(param->periodic);
    sim->ghost_manager = std::make_shared<GhostParticleManager<Dim>>();
    sim->ghost_manager->initialize(config);
}
```

## Next Steps

1. ✅ Write comprehensive BDD integration tests
2. 🔄 Modify PreInteraction to use combined particle list
3. ⬜ Protect force calculations from updating ghosts
4. ⬜ Integrate into solver workflow
5. ⬜ Run full test suite
6. ⬜ Performance validation
7. ⬜ Documentation and examples

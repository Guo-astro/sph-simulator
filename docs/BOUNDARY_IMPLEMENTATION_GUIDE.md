# Boundary Implementation Quick Reference

## TL;DR - Critical Rule

> **ALWAYS enable ghost particles for ALL boundary types when using Barnes-Hut tree neighbor search**

## Quick Configuration Examples

### Periodic Boundaries (1D Example)

```cpp
// Use BoundaryConfigHelper for periodic boundaries
std::array<real, 3> range_min = {-0.5, 0.0, 0.0};
std::array<real, 3> range_max = {1.5, 0.0, 0.0};

auto boundary_config = BoundaryConfigHelper<Dim>::from_baseline_json(
    true,          // is_periodic = true
    range_min,
    range_max,
    true           // enable_ghosts = TRUE (CRITICAL!)
);

sim->ghost_manager->initialize(boundary_config);
```

### Mirror Boundaries (2D/3D Example)

```cpp
BoundaryConfiguration<Dim> ghost_config;
ghost_config.is_valid = true;

// X-direction: reflective walls
ghost_config.types[0] = BoundaryType::MIRROR;
ghost_config.range_min[0] = -0.5;
ghost_config.range_max[0] = 1.5;
ghost_config.enable_lower[0] = true;
ghost_config.enable_upper[0] = true;
ghost_config.mirror_types[0] = MirrorType::FREE_SLIP;  // or NO_SLIP
ghost_config.spacing_lower[0] = dx;
ghost_config.spacing_upper[0] = dx;

// Repeat for Y and Z dimensions...

sim->ghost_manager->initialize(ghost_config);
```

## Common Mistakes

### ❌ WRONG: Periodic without ghosts

```cpp
auto boundary_config = BoundaryConfigHelper<Dim>::from_baseline_json(
    true, range_min, range_max,
    false  // ❌ BUG! Causes neighbor search failure
);
```

**Symptoms**:
- Density explosion or collapse
- Smoothing length divergence (→ 0 or → ∞)
- Timestep collapse (dt → 0)
- Simulation crash

### ✅ CORRECT: Periodic with ghosts

```cpp
auto boundary_config = BoundaryConfigHelper<Dim>::from_baseline_json(
    true, range_min, range_max,
    true   // ✅ Ghost particles enable correct neighbor finding
);
```

## Verification Checklist

After implementing boundaries, verify:

1. **Ghost particles created**:
   ```
   [GhostParticleManager] Creating ghost particles...
   [GhostParticleManager] Created X ghost particles
   ```

2. **Neighbor counts correct**:
   - Check particles near boundaries have expected neighbor counts
   - Should match analytical expectations (e.g., ~50 neighbors for typical 3D simulation)

3. **Physical quantities stable**:
   - Density remains in expected range (e.g., 0.1 to 1.0 for Sod shock)
   - Smoothing length stable (e.g., 0.01 to 0.05)
   - Timestep reasonable (e.g., 1e-5 to 1e-3)

4. **Simulation completes**:
   - No crashes or NaN/Inf values
   - Reaches target end time

## Boundary Types Reference

| Type | Use Case | Ghost Particles | Code Example |
|------|----------|-----------------|--------------|
| `PERIODIC` | Infinite domain, wrapping | **Required** | `from_baseline_json(..., true)` |
| `MIRROR` | Walls, confined flow | **Required** | `types[i] = BoundaryType::MIRROR` |
| `OPEN` | Outflow, non-reflecting | **Not implemented yet** | - |

## Mirror Type Reference

| Mirror Type | Physics | Use Case |
|-------------|---------|----------|
| `FREE_SLIP` | Frictionless wall | Symmetry planes, inviscid flow |
| `NO_SLIP` | Friction wall | Viscous flow, real walls |

## Debugging Commands

### Check ghost particle creation

```bash
# Run simulation with logging enabled
./build/sph ./lib/your_plugin.dylib 2>&1 | grep -i ghost
```

Expected output:
```
[GhostParticleManager] Creating ghost particles...
[GhostParticleManager] Created 234 ghost particles for periodic X
```

### Monitor neighbor counts

Add temporary logging in your plugin:
```cpp
// After pre_interaction
for (int i = 0; i < 10; ++i) {
    std::cout << "Particle " << i 
              << ": dens=" << particles[i].dens 
              << ", sml=" << particles[i].sml 
              << ", neighbor_count=" << particles[i].pair_num 
              << "\n";
}
```

### Check boundary ranges

```cpp
std::cout << "Domain: [" << range_min[0] << ", " << range_max[0] << "]\n";
std::cout << "Kernel support: " << 2.0 * initial_sml << "\n";
std::cout << "Ghost layer needed: " << (2.0 * initial_sml < dx ? "NO" : "YES") << "\n";
```

## Performance Notes

### Ghost Particle Overhead

- **Periodic 1D**: ~2× particles (ghosts on both ends)
- **Periodic 2D**: ~4× particles (ghosts on all 4 edges + corners)
- **Periodic 3D**: ~8× particles (ghosts on all 6 faces + edges + corners)

**Optimization**: Ghost particles are only created near boundaries (within kernel support distance), not for entire domain.

### Memory Usage

| Dimension | Base Particles | Ghost Particles | Total | Memory Factor |
|-----------|----------------|-----------------|-------|---------------|
| 1D | 1,000 | ~40 | ~1,040 | 1.04× |
| 2D | 10,000 | ~400 | ~10,400 | 1.04× |
| 3D | 100,000 | ~6,000 | ~106,000 | 1.06× |

**Note**: Ghost overhead is typically <10% for reasonably sized domains.

## Related Documentation

- **Detailed Fix**: `/docs/PERIODIC_BOUNDARY_FIX.md` - 1D baseline bug analysis
- **Architecture**: `/docs/ARCHITECTURE_BOUNDARY_CONSISTENCY.md` - Complete guide
- **Code**: `/include/core/ghost_particle_manager.tpp` - Implementation
- **Code**: `/include/core/bhtree.tpp` - Tree neighbor search

## Quick Diagnostic

Run this checklist if simulation fails:

```
□ Is enable_ghosts = true for periodic boundaries?
□ Is ghost_config.is_valid = true for mirror boundaries?
□ Is ghost_manager initialized before first timestep?
□ Are boundary ranges correct (min < max)?
□ Does kernel support radius fit in domain (sml < domain_size)?
□ Are ghost particles actually created (check logs)?
□ Do particles near boundaries have correct neighbor counts?
```

If all checked and still failing → See detailed debugging in `PERIODIC_BOUNDARY_FIX.md`

---

**Last Updated**: January 2025  
**Applies To**: All SPH simulations using Barnes-Hut tree neighbor search

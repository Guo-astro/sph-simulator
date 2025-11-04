# Ghost Particle Output with Type Markers

## Summary

Successfully implemented output of ghost particles with type markers for visualization. Ghost particles now appear in output files with `type=1` while real particles have `type=0`.

## Changes Made

### 1. Added ParticleType Enum (`include/core/sph_particle.hpp`)
```cpp
enum class ParticleType : int {
    REAL = 0,
    GHOST = 1
};
```
- **Purpose**: Eliminate magic numbers (0 and 1)
- **Type field**: Added `int type` member initialized to `ParticleType::REAL`

### 2. Updated Output System (`include/output.tpp`)
- **Added type field to output**: `OUTPUT_SCALAR(type);` writes particle type to file
- **Output ghost particles**: Added loop in `output_particle()` to write ghost particles after real particles
```cpp
// Output ghost particles if they exist
if (sim->ghost_manager && sim->ghost_manager->has_ghosts()) {
    const auto& ghosts = sim->ghost_manager->get_ghost_particles();
    for (const auto& ghost : ghosts) {
        output(ghost, out);
    }
}
```

### 3. Mark Ghost Particles (`include/core/ghost_particle_manager.tpp`)
Set `type = ParticleType::GHOST` at all ghost particle generation points:
- Periodic boundary ghosts
- Corner ghosts (2D)
- 3D corner ghosts
- Mirror boundary ghosts

### 4. Fixed Kernel Support Radius (`workflows/shock_tube_workflow/01_simulation/src/plugin_enhanced.cpp`)
**Problem**: Smoothing lengths (`sml`) not yet calculated during plugin initialization
**Solution**: Estimate from particle spacing
```cpp
const real estimated_sml = 2.0 * dx_right;  // Typical for cubic spline
const real kernel_support_radius = 2.0 * estimated_sml;
sim->ghost_manager->set_kernel_support_radius(kernel_support_radius);
```

## Output Format

For 1D simulations, output columns are:
1. pos[0] - position
2. vel[0] - velocity
3. acc[0] - acceleration
4. mass
5. dens - density
6. pres - pressure
7. ene - specific internal energy
8. sml - smoothing length
9. id - particle ID
10. neighbor - neighbor count
11. alpha - artificial viscosity parameter
12. gradh - smoothing length gradient term
13. **type** - particle type (0=REAL, 1=GHOST)

## Verification

### Simulation Results
```
--- Ghost Particle System ---
✓ Ghost particle system initialized
  Boundary type: MIRROR (NO_SLIP)
  Domain range: [-0.5, 1.5]
  Estimated smoothing length: 0.04
  Kernel support radius: 0.08
  Generated 36 ghost particles
```

### Output Files
- **Initial state (t=0)**: 450 real particles + 36 ghost particles = 486 total
- **Later state (t=0.1)**: 450 real particles + 32 ghost particles = 482 total
- Ghost count varies as simulation evolves (expected behavior)

### Particle Distribution
- **Real particles** (type=0): Inside domain [-0.5, 1.5]
  - Example positions: -0.49875, -0.49625, ..., 1.47, 1.49
- **Ghost particles** (type=1): Outside domain boundaries
  - Lower boundary: positions < -0.5 (e.g., -0.50125, -0.57875)
  - Upper boundary: positions > 1.5 (e.g., 1.51, 1.57)

## Visualization

Created `plot_with_ghosts.py` to visualize:
- **Blue dots**: Real particles (type=0)
- **Red crosses**: Ghost particles (type=1)
- **Gray dashed lines**: Domain boundaries at x=-0.5 and x=1.5

Run with:
```bash
cd workflows/shock_tube_workflow/01_simulation
python3 plot_with_ghosts.py
```

Output saved to `plots/snapshot_*.png`

## Code Quality Improvements

✅ **No magic numbers**: All hardcoded 0/1 values replaced with `ParticleType::REAL` and `ParticleType::GHOST`
✅ **Type safety**: Using `enum class` prevents implicit conversions
✅ **Clear intent**: Code explicitly shows particle type assignment
✅ **Maintainability**: Easy to add more particle types in future (e.g., FLUID, BOUNDARY, SOLID)

## Usage for Animation

The type field allows post-processing tools to:
1. Filter out ghost particles if desired
2. Color-code particles by type for visualization
3. Analyze ghost particle behavior separately
4. Verify boundary condition implementation

Example filtering in Python:
```python
data = np.loadtxt('00010.dat', skiprows=1)
real_particles = data[data[:, 12] == 0]  # type column
ghost_particles = data[data[:, 12] == 1]
```

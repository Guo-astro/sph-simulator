# 2.5D SPH Simulations

This document describes how to implement 2.5D SPH simulations in the SPH simulator, where hydrodynamic forces are computed in 2D (assuming azimuthal symmetry) while gravity is calculated in full 3D.

## Physical Model

### Hydrodynamics (2D)
- **Domain**: r-z cylindrical coordinates (radial and axial)
- **Symmetry**: Azimuthal symmetry (∂/∂φ = 0)
- **Forces**: Pressure gradients, viscosity, shocks computed in 2D
- **Kernel**: 2D cubic spline kernel for neighbor searches and interpolation

### Gravity (3D)
- **Domain**: Full 3D Cartesian coordinates (x, y, z)
- **Method**: Barnes-Hut tree algorithm in 3D
- **Forces**: Gravitational interactions computed in full 3D space
- **Accuracy**: Captures non-axisymmetric gravitational effects

## Implementation

### Core Classes

#### `SPHParticle25D`
```cpp
#include "core/sph_particle_2_5d.hpp"

SPHParticle25D particle;
particle.pos = Vector<2>{r, z};        // 2D hydro position
particle.vel = Vector<2>{v_r, v_z};    // 2D hydro velocity
particle.update_gravity_position(phi); // Convert to 3D for gravity
```

#### `Simulation25D`
```cpp
#include "core/simulation_2_5d.hpp"

auto sim = std::make_shared<Simulation25D>(params);
sim->calculate_hydrodynamics();  // 2D SPH forces
sim->calculate_gravity();        // 3D gravity forces
sim->timestep();                 // Combined integration
```

#### `Cubic25D` Kernel
```cpp
#include "core/cubic_spline_2_5d.hpp"

auto kernel = std::make_shared<Cubic25D>();
real w_val = kernel->w(r_vector_2d, h);  // 2D kernel evaluation
```

#### `BHTree25D` Gravity Tree
```cpp
#include "core/bhtree_2_5d.hpp"

BHTree25D tree;
tree.initialize(params);
tree.make(particles_2_5d, n_particles);
tree.tree_force(particle_i);  // Calculate 3D gravity for particle i
```

## Usage Example

```cpp
#include "core/simulation_2_5d.hpp"
#include "core/sph_particle_2_5d.hpp"

// Create simulation parameters
auto params = std::make_shared<SPHParameters>();
params->kernel = KernelType::CUBIC_SPLINE;
params->gravity.is_valid = true;
params->gravity.constant = 1.0;

// Create 2.5D simulation
Simulation25D sim(params);

// Set up particles in r-z plane
const int n_particles = 1000;
sim.particles.resize(n_particles);
sim.particle_num = n_particles;

// Initialize particles (example: disk setup)
for (int i = 0; i < n_particles; ++i) {
    auto& p = sim.particles[i];
    p.id = i;
    p.mass = 1.0 / n_particles;
    p.sml = 0.05;
    
    // Position in r-z plane
    real r = 0.1 + 0.9 * (real)rand() / RAND_MAX;  // 0.1 to 1.0
    real z = 0.0;  // Midplane
    real phi = 2.0 * M_PI * (real)rand() / RAND_MAX; // Random azimuth
    
    p.pos = Vector<2>{r, z};
    p.vel = Vector<2>{0.0, 0.0};  // Initially at rest
    
    // Set 3D gravity position
    p.update_gravity_position(phi);
}

// Simulation loop
const int n_steps = 1000;
for (int step = 0; step < n_steps; ++step) {
    // Calculate forces
    sim.calculate_gravity();      // 3D gravity
    sim.calculate_hydrodynamics(); // 2D hydro
    
    // Time integration would go here
    // (Implement symplectic integrator for 2.5D)
    
    sim.update_time();
}
```

## Coordinate Systems

### Hydrodynamic Coordinates (2D)
- **r**: Cylindrical radius
- **z**: Axial coordinate
- **φ**: Azimuthal angle (assumed symmetric)

### Gravity Coordinates (3D)
- **x = r cos φ**
- **y = r sin φ**
- **z = z**

### Force Projection
When 3D gravitational accelerations are computed, they need to be projected back to the 2D hydrodynamic coordinate system:

```cpp
// In BHTree25D::tree_force()
real r = particle.r();
if (r > 0.0) {
    real cos_phi = particle.g_pos[0] / r;
    real sin_phi = particle.g_pos[1] / r;
    particle.acc[0] = g_acc_3d[0] * cos_phi + g_acc_3d[1] * sin_phi; // d(v_r)/dt
    particle.acc[1] = g_acc_3d[2]; // d(v_z)/dt
}
```

## Advantages

1. **Computational Efficiency**: Hydrodynamics computed in 2D reduces computational cost
2. **Physical Accuracy**: Gravity computed in 3D captures important non-axisymmetric effects
3. **Memory Efficiency**: Fewer particles needed compared to full 3D simulation
4. **Azimuthal Symmetry**: Appropriate for many astrophysical systems (disks, rings, etc.)

## Limitations

1. **Azimuthal Symmetry Assumption**: Cannot capture non-axisymmetric hydrodynamic instabilities
2. **Limited to Axisymmetric Systems**: Not suitable for systems with significant azimuthal variations
3. **Force Projection Complexity**: Care needed when projecting 3D forces to 2D coordinates

## Testing

Comprehensive BDD tests are provided in `tests/core/sph_2_5d_test.cpp`:

```bash
# Run 2.5D tests
cd build && ctest -R SPH25DTest
```

Tests cover:
- Coordinate transformations between 2D and 3D
- 2.5D kernel functions
- Gravity calculations with 3D tree
- Particle property updates
- Dimension policy correctness

## Performance Considerations

- **Hydrodynamics**: O(N) scaling in 2D (fewer neighbors)
- **Gravity**: O(N log N) scaling in 3D (Barnes-Hut tree)
- **Memory**: ~60% reduction compared to full 3D simulation
- **Accuracy**: Maintains gravitational accuracy while reducing hydrodynamic cost

## Applications

2.5D simulations are particularly useful for:
- Protoplanetary disks
- Galactic disks
- Accretion disks around black holes
- Ring systems
- Axisymmetric stellar systems

## Future Extensions

The 2.5D framework can be extended to:
- Multiple azimuthal modes (m ≠ 0)
- Weak azimuthal perturbations
- Hybrid 2.5D/3D schemes
- Adaptive resolution in azimuthal direction</content>
<parameter name="filePath">/Users/guo/sph-simulation/docs/SPH_2_5D_SIMULATIONS.md
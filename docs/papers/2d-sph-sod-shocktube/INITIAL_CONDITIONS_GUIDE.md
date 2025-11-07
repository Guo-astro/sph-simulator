# 2D SPH Sod Shock Tube - Initial Conditions Setup Guide

## Overview

This guide provides practical instructions for setting up initial conditions for 2D SPH Sod shock tube simulations based on reviewed academic literature.

## Standard 1D Sod Shock Tube (Reference)

### Physical Domain
- Length: L = 1.0 m
- Discontinuity at: x₀ = 0.5 m (midpoint)
- Time: typically run until t = 0.2 s

### Initial Conditions

**Left State (0 ≤ x < 0.5):**
```
ρ_L = 1.0 kg/m³
u_L = 0.0 m/s
p_L = 1.0 Pa
```

**Right State (0.5 ≤ x ≤ 1.0):**
```
ρ_R = 0.125 kg/m³
u_R = 0.0 m/s
p_R = 0.1 Pa
```

**Equation of State:**
- Ideal gas: γ = 1.4 (diatomic gas)
- Internal energy: e = p / ((γ-1) * ρ)

## 2D Extension Methods

### Method 1: Planar 2D (Recommended for Testing)

**Purpose**: Verify 1D behavior is preserved in 2D code

**Domain Setup:**
```
X-direction: [0, 1.0] m (primary shock direction)
Y-direction: [0, h] m where h << 1.0 (typically h = 0.1 to 0.2)
```

**Boundary Conditions:**
```
X-boundaries: Free/transmissive (allow waves to exit)
Y-boundaries: Periodic or reflective (maintain planarity)
```

**Particle Distribution:**
```
N_x particles in X direction (e.g., 400-800)
N_y particles in Y direction (e.g., 40-80 for h=0.1)
Total particles: N = N_x × N_y

Spacing: Δx = 1.0/N_x, Δy = h/N_y
Equal mass particles with uniform initial distribution
```

**Initial State:**
```
For each particle at position (x_i, y_j):
  if x_i < 0.5:
    ρ = 1.0, u = 0.0, v = 0.0, p = 1.0
  else:
    ρ = 0.125, u = 0.0, v = 0.0, p = 0.1
```

### Method 2: Axisymmetric 2D

**Purpose**: Circular/cylindrical shock wave propagation

**Domain Setup (Polar Coordinates):**
```
Radial direction: r ∈ [0, R_max]
Angular direction: θ ∈ [0, 2π] or sectorial
```

**Initial Conditions:**
```
Inner region (r < r₀): High pressure state
Outer region (r ≥ r₀): Low pressure state

r₀ = 0.5 * R_max (discontinuity radius)

Inner (r < r₀):
  ρ = 1.0, u_r = 0.0, u_θ = 0.0, p = 1.0

Outer (r ≥ r₀):
  ρ = 0.125, u_r = 0.0, u_θ = 0.0, p = 0.1
```

### Method 3: 2D Cross Configuration

**Purpose**: Test shock interactions in 2D

**Domain Setup:**
```
Square domain: [0, 1] × [0, 1]
Four quadrants with different initial states
```

**Example Configuration:**
```
Quadrant I  (x>0.5, y>0.5): ρ=0.5, p=0.4
Quadrant II (x<0.5, y>0.5): ρ=1.0, p=1.0
Quadrant III(x<0.5, y<0.5): ρ=0.8, p=1.0
Quadrant IV (x>0.5, y<0.5): ρ=1.0, p=1.0
All velocities initially zero
```

## SPH Particle Setup

### Particle Number Estimation

**For Planar 2D:**
```cpp
const double domain_x = 1.0;
const double domain_y = 0.1;  // Small height
const int particles_per_direction = 400;
const int particles_y = 40;
const int total_particles = particles_per_direction * particles_y; // 16,000

const double dx = domain_x / particles_per_direction;
const double dy = domain_y / particles_y;
```

### Smoothing Length

**Initial smoothing length:**
```cpp
// Typically 1.2 to 2.0 times the particle spacing
const double h_factor = 1.5;
const double h = h_factor * sqrt(dx * dx + dy * dy);
```

For equal spacing: `h = 1.5 * Δx`

### Particle Mass

**Equal mass particles:**
```cpp
// Left side mass per particle
double total_mass_left = rho_left * (0.5 * domain_x) * domain_y;
int num_particles_left = (particles_per_direction / 2) * particles_y;
double mass_left = total_mass_left / num_particles_left;

// Right side mass per particle
double total_mass_right = rho_right * (0.5 * domain_x) * domain_y;
int num_particles_right = (particles_per_direction / 2) * particles_y;
double mass_right = total_mass_right / num_particles_right;
```

**Note**: For uniform particle distribution with density jump, use equal spacing but different masses on each side.

### Initial Particle Positions

**Cartesian grid (regular lattice):**
```cpp
std::vector<Particle> particles;
particles.reserve(total_particles);

int id = 0;
for (int j = 0; j < particles_y; ++j) {
  for (int i = 0; i < particles_per_direction; ++i) {
    double x = (i + 0.5) * dx;  // Cell-centered
    double y = (j + 0.5) * dy;
    
    // Determine state
    if (x < 0.5) {
      // Left state
      particles.push_back(create_particle(id++, x, y, 
                                         rho_left, 0.0, 0.0, p_left));
    } else {
      // Right state
      particles.push_back(create_particle(id++, x, y,
                                         rho_right, 0.0, 0.0, p_right));
    }
  }
}
```

**Alternative: Hexagonal close packing (HCP):**
- More isotropic kernel support
- Better stability in some cases
- Offset every other row by dx/2

## Smoothing the Discontinuity

Some implementations smooth the initial discontinuity over a few particle spacings to avoid numerical artifacts.

**Smoothing function (tanh profile):**
```cpp
double smooth_factor(double x, double x0, double width) {
  // Smooth transition from 0 to 1 centered at x0
  return 0.5 * (1.0 + tanh((x - x0) / width));
}

double rho = rho_right + (rho_left - rho_right) * smooth_factor(x, 0.5, width);
double p = p_right + (p_left - p_right) * smooth_factor(x, 0.5, width);
```

**Width parameter:**
- Typically 1-3 smoothing lengths: `width = 2.0 * h`
- Paper recommendation (Busegnies et al., 2007): `s0 = 1`

## Time Integration Settings

### Time Step

**CFL condition:**
```cpp
double dt = CFL * h / (c_s + |v_max|)
```

Where:
- `CFL` = 0.3 to 0.5 (typical for explicit schemes)
- `c_s` = sound speed = sqrt(γ * p / ρ)
- `|v_max|` = maximum particle velocity magnitude

### Simulation Duration

**Standard test:**
- Final time: t_final = 0.2 s
- Expected features by t=0.2:
  - Left-traveling rarefaction wave
  - Contact discontinuity
  - Right-traveling shock wave

## Artificial Viscosity Parameters

Essential for shock capturing in SPH:

**Standard Monaghan viscosity:**
```cpp
const double alpha_visc = 1.0;  // Viscosity strength
const double beta_visc = 2.0;   // For shocks
const double eta_squared = 0.01 * h * h;  // Regularization
```

**Alternatives:**
- Time-dependent artificial viscosity (reduces away from shocks)
- Riemann solver based approaches (Price, 2024)

## Boundary Treatment

### Open/Transmissive Boundaries

**Ghost particle method:**
- Mirror particles outside domain with extrapolated properties
- Allow waves to exit without reflection

**Buffer zone method:**
- Extended domain with damping near boundaries
- Absorb outgoing waves

### Periodic Boundaries (Y-direction for planar 2D)

```cpp
// Wrap particles in Y
if (particle.y < 0) particle.y += domain_y;
if (particle.y >= domain_y) particle.y -= domain_y;

// Also apply to ghost particles for kernel interactions
```

## Expected Results

### Physical Features

At t = 0.2 s, the solution should show:

1. **Rarefaction fan** (left side, x < 0.3):
   - Smooth decrease in density and pressure
   - Velocity increases linearly

2. **Contact discontinuity** (middle, x ≈ 0.6):
   - Density jump
   - Pressure and velocity continuous
   - Nearly stationary or slowly moving

3. **Shock wave** (right side, x ≈ 0.8):
   - Sharp jump in all quantities
   - Moving to the right

### Quantitative Checks

**Shock position:**
- Theory: x_shock ≈ 0.85 at t=0.2
- SPH: within 1-2 smoothing lengths of theory

**Contact position:**
- Theory: x_contact ≈ 0.69 at t=0.2

**Maximum velocity:**
- Theory: u_max ≈ 0.93 at contact

## Common Issues and Solutions

### Issue 1: Particles Clustering

**Symptom**: Particles accumulate at discontinuities

**Solutions:**
- Use XSPH velocity correction: `v_advection = v + ε * Σ(v_j - v_i) * W_ij`
- Apply density diffusion term
- Use particle shifting/regularization

### Issue 2: Excessive Noise at Contact

**Symptom**: Wiggles in density profile

**Solutions:**
- Reduce artificial viscosity parameter α
- Use Riemann solver instead of artificial viscosity
- Apply smoothing of initial conditions

### Issue 3: Shock Too Diffuse

**Symptom**: Shock spread over many particles

**Solutions:**
- Increase artificial viscosity β parameter
- Reduce smoothing length
- Use higher-order kernel function
- Implement artificial conductivity for energy

### Issue 4: 2D Artifacts

**Symptom**: Non-planar waves in nominally 1D test

**Solutions:**
- Check Y-boundary conditions (must be periodic or reflective)
- Verify particle distribution is uniform in Y
- Ensure sufficient particles in Y direction (min 20-40)

## Code Implementation Checklist

- [ ] Set up physical constants (γ = 1.4)
- [ ] Define domain dimensions (planar: L=1.0, h=0.1)
- [ ] Calculate particle spacing and smoothing length
- [ ] Initialize particle positions on regular grid
- [ ] Assign initial densities based on x-coordinate
- [ ] Set initial velocities to zero
- [ ] Set initial pressures based on x-coordinate
- [ ] Calculate internal energies from EOS
- [ ] Set boundary conditions (transmissive X, periodic Y)
- [ ] Configure artificial viscosity (α=1.0, β=2.0)
- [ ] Set CFL number (0.3-0.5) and calculate time step
- [ ] Set final time (t=0.2) and output frequency
- [ ] Verify total mass conservation
- [ ] Check particle neighbor lists are working
- [ ] Run simulation and compare with exact solution

## References

See PAPERS_SUMMARY.md for complete bibliography.

Key references:
- Puri & Ramachandran (2014): SPH schemes comparison
- Price (2024): Modern shock capturing methods
- Chen et al. (2019): Axisymmetric 2D SPH
- Owen et al. (1997): Adaptive SPH for 2D/3D

## Exact Solution Data

For comparison, the exact Riemann solution can be computed analytically or using:
- Python: `scipy.optimize` with Rankine-Hugoniot conditions
- External tools: Exact Riemann solver codes
- Tabulated data from literature

Store exact solution for comparison at t=0.2.

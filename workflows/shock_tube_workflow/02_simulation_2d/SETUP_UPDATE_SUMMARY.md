# 2D SPH Sod Shock Tube - Setup Update Summary

## Overview

Updated the 2D SPH shock tube simulation to follow literature-recommended practices based on:
- Puri & Ramachandran (2014): "A comparison of SPH schemes for the compressible Euler equations"
- Price (2024): "On shock capturing in smoothed particle hydrodynamics"
- Chen et al. (2019): "Simulations for the explosion in a water-filled tube..."
- Owen et al. (1997): "Adaptive Smoothed Particle Hydrodynamics: Methodology II"

## Key Changes

### 1. Domain Configuration

**Before:**
- X-direction: [-0.5, 1.5] (2.0 units)
- Y-direction: [0, 0.5] (0.5 units)
- Discontinuity at x = 0.5

**After (Literature Standard):**
- X-direction: [0, 1.0] (1.0 units)
- Y-direction: [0, 0.1] (0.1 units) - **Planar 2D**
- Discontinuity at x = 0.5 (midpoint)

**Rationale:** 
- Standard Sod shock tube domain is [0, 1] with discontinuity at 0.5
- Small Y-height (0.1-0.2) ensures quasi-1D planar behavior
- Allows clean comparison with 1D analytical solution

### 2. Particle Spacing Strategy

**Before:**
- Variable spacing with ~2.86:1 ratio
- Left: 60 particles, Right: 21 particles
- Target density ratio: ~4:1

**After (Correct SPH Physics):**
- Variable spacing with 8:1 ratio
- Left: 200 particles, Right: 25 particles
- Target density ratio: 8:1 (Standard Sod)

**Mathematical Basis:**

For equal mass particles in 2D:
```
ρ = Σ m_j W(r_ij, h) ∝ particle number density

With constant dy:
ρ ∝ 1/dx

Therefore:
ρ_left/ρ_right = dx_right/dx_left

For Sod standard (ρ_L/ρ_R = 1.0/0.125 = 8):
dx_right = 8.0 * dx_left
```

### 3. Initial Conditions

**Standard Sod Shock Tube:**

| Quantity | Left State (x < 0.5) | Right State (x ≥ 0.5) |
|----------|---------------------|----------------------|
| Density  | ρ = 1.0            | ρ = 0.125            |
| Pressure | P = 1.0            | P = 0.1              |
| Velocity | u = 0.0            | u = 0.0              |
| γ (gamma)| 1.4                | 1.4                  |

**Particle Configuration:**
- Left region: dx = 0.0025 (200 particles in 0.5 units)
- Right region: dx = 0.02 (25 particles in 0.5 units)
- Y-spacing: dy = 0.0025 (40 particles in 0.1 units)
- Total particles: 225 × 40 = 9,000 particles
- Equal mass: m = 1.0 × dx_left × dy = 6.25e-6

### 4. Boundary Conditions

**Configuration:**
- X-direction: Mirror (NO_SLIP) - Reflective walls at x=0 and x=1
- Y-direction: Periodic - Maintains planar symmetry

**Purpose:**
- X: Confines shock tube, prevents particles from escaping
- Y: Infinite planar extent, enforces 1D-like behavior

### 5. Animation Script Updates

**Updated Parameters:**
- Domain: [0, 1] × [0, 0.1]
- Centerline extraction: y = 0.05 (middle of planar domain)
- Tolerance: ±0.03 (captures centerline particles)
- Point sizes adjusted for 8:1 spacing ratio
- Analytical solution domain: [0, 1]

## Expected Physical Results

At t = 0.2s, the solution should exhibit:

### 1. Left-Traveling Rarefaction Wave
- Location: x ≈ 0.0 to 0.35
- Smooth density/pressure decrease
- Velocity increases linearly

### 2. Contact Discontinuity
- Location: x ≈ 0.69
- Sharp density jump
- Pressure and velocity continuous

### 3. Right-Traveling Shock Wave
- Location: x ≈ 0.85
- Sharp jump in all quantities
- Moving right with speed ~1.75

### Verification Metrics

**Initial State (t=0):**
- Density ratio: ρ_L/ρ_R ≈ 8.0 ± 0.5
- Pressure ratio: P_L/P_R = 10.0
- Zero velocity everywhere

**At t=0.2s:**
- Shock position: 0.84 ± 0.02
- Contact position: 0.68 ± 0.02
- Maximum velocity: 0.93 ± 0.05
- Post-shock density: ≈ 0.27

## File Changes Summary

### C++ Plugin Files

1. **plugin_disph.cpp**
   - Domain: [0,1] × [0,0.1]
   - Spacing: 8:1 ratio (200:25 particles)
   - Boundaries: Mirror X, Periodic Y
   - References added to literature

2. **plugin_gsph.cpp**
   - Domain: [0,1] × [0,0.1]
   - Spacing: 8:1 ratio (200:25 particles)
   - Boundaries: Mirror X, Periodic Y
   - Initial density guess: 0.125 (right side)

3. **plugin_ssph.cpp**
   - Domain: [0,1] × [0,0.1]
   - Spacing: 8:1 ratio (200:25 particles)
   - Boundaries: Mirror X, Periodic Y
   - Consistent with GSPH/DISPH

### Python Animation Scripts

1. **animate_2d.py**
   - Analytical domain: [0, 1]
   - Centerline: y = 0.05 (planar center)
   - Plot limits: x ∈ [0,1], y ∈ [0,0.1]
   - Point sizes adjusted for 8:1 ratio
   - Documentation updated

2. **animate_disph.py, animate_gsph.py, animate_ssph.py**
   - No changes needed (use animate_2d.py)

## Building and Running

### Clean Build
```bash
cd /Users/guo/sph-simulation/workflows/shock_tube_workflow/02_simulation_2d
make full-clean
make build
```

### Run Simulations
```bash
# DISPH
make run-disph

# GSPH
make run-gsph

# SSPH
make run-ssph
```

### Generate Animations
```bash
cd scripts
./animate_disph.py
./animate_gsph.py
./animate_ssph.py
```

## Validation Checklist

- [ ] Initial density ratio ≈ 8.0:1
- [ ] Initial pressure ratio = 10.0:1
- [ ] Shock wave position at t=0.2s ≈ 0.85
- [ ] Contact discontinuity at t=0.2s ≈ 0.69
- [ ] No artificial 2D effects (check Y-variation is minimal)
- [ ] Smooth rarefaction wave profile
- [ ] Sharp shock wave (within 2-3 particles)
- [ ] Total mass conservation
- [ ] Total energy conservation (within dissipation)

## References

1. **Puri, K., & Ramachandran, P.** (2014). "A comparison of SPH schemes for the compressible Euler equations." *Journal of Computational Physics*, 256, 308-333.
   - Standard 1D/2D shock tube test descriptions
   - Initial condition specifications

2. **Price, D.J.** (2024). "On shock capturing in smoothed particle hydrodynamics." *arXiv preprint arXiv:2407.10176*.
   - Modern SPH shock capturing methods
   - Sod test setup with 450 equal mass particles in 1D

3. **Chen, J.Y., et al.** (2019). "Simulations for the explosion in a water-filled tube including cavitation using the SPH method." *Computational Particle Mechanics*, 6, 515-537.
   - Axisymmetric 2D SPH methodology
   - Initial conditions on left/right sides

4. **Owen, J.M., et al.** (1997). "Adaptive Smoothed Particle Hydrodynamics: Methodology II." *The Astrophysical Journal Supplement Series*, 116(2), 155.
   - Comprehensive 2D/3D ASPH methodology
   - 92 pages with detailed test cases

## Troubleshooting

### Issue: Density ratio not matching 8:1

**Cause:** Kernel support or smoothing length issues

**Solution:**
- Verify smoothing length computation
- Check neighbor search radius
- Ensure iterative sml is enabled

### Issue: Non-planar waves in 2D

**Cause:** Insufficient Y-resolution or wrong boundaries

**Solution:**
- Verify Y-boundaries are PERIODIC (not mirror)
- Check particle distribution is uniform in Y
- Ensure at least 40 particles in Y-direction

### Issue: Excessive diffusion at shock

**Cause:** Artificial viscosity too high or kernel too smooth

**Solution:**
- For GSPH: Use HLL solver, disable MUSCL if needed
- For SSPH: Reduce alpha parameter (try 0.5-1.0)
- For DISPH: Check density-independent formulation

## Notes

- All three methods (DISPH, GSPH, SSPH) now use identical initial conditions
- Only difference is the SPH algorithm (artificial viscosity vs Riemann solver)
- Fair comparison requires using SPH-computed initial densities in analytical solution
- Variable spacing is the correct approach for equal-mass SPH particles
- Planar 2D geometry (small Y-height) allows validation against 1D analytical solution

## Date Updated
2025-11-07

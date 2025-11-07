# 2D SPH Sod Shock Tube - Quick Reference

## Standard Initial Conditions

```cpp
// Domain
const real Lx = 1.0;           // X-direction [0, 1.0]
const real Ly = 0.1;           // Y-direction [0, 0.1] - planar 2D
const real x_disc = 0.5;       // Discontinuity position

// Left State (x < 0.5)
const real rho_L = 1.0;        // Density
const real p_L = 1.0;          // Pressure
const real u_L = 0.0;          // Velocity

// Right State (x ≥ 0.5)
const real rho_R = 0.125;      // Density (8:1 ratio)
const real p_R = 0.1;          // Pressure (10:1 ratio)
const real u_R = 0.0;          // Velocity

// Physics
const real gamma = 1.4;        // Adiabatic index (diatomic gas)
```

## Particle Setup (Equal Mass, Variable Spacing)

```cpp
// Spacing ratio for 8:1 density ratio
const real spacing_ratio = 8.0;

// Left region (fine resolution)
const int Nx_left = 200;
const real dx_left = 0.5 / Nx_left;  // = 0.0025

// Right region (coarse resolution)
const real dx_right = spacing_ratio * dx_left;  // = 0.02
const int Nx_right = static_cast<int>(0.5 / dx_right);  // = 25

// Y-direction
const real dy = dx_left;  // = 0.0025
const int Ny = static_cast<int>(Ly / dy);  // = 40

// Total particles
const int total = (Nx_left + Nx_right) * Ny;  // 225 × 40 = 9,000

// Equal mass (SPH convention)
const real mass = rho_L * dx_left * dy;  // = 6.25e-6
```

## Boundary Conditions

```cpp
// X-direction: Mirror (reflective walls)
.with_mirror_in_dimension(0, MirrorType::NO_SLIP, dx_left, dx_right)

// Y-direction: Periodic (planar symmetry)
.with_periodic_in_dimension(1)

// Domain range
.in_range(Vector<Dim>{0.0, 0.0}, Vector<Dim>{1.0, Ly})
```

## Time Integration

```cpp
// Simulation time
t_start = 0.0
t_end = 0.2      // Standard Sod test duration
dt_output = 0.01  // Output every 0.01s (20 snapshots)

// CFL conditions (auto-computed from particle config)
CFL_sound = 0.3-0.5
CFL_force = 0.5-1.0
```

## Smoothing Length

```cpp
// Initial smoothing length (κ = 1.2 typical)
const real kappa = 1.2;
const real sml_left = kappa * dx_left;   // Left region
const real sml_right = kappa * dx_right; // Right region

// Enable iterative smoothing length computation
.with_iterative_smoothing_length(true)
```

## Expected Results at t = 0.2s

| Feature | Position | Description |
|---------|----------|-------------|
| Rarefaction head | x ≈ 0.26 | Left edge of rarefaction wave |
| Rarefaction tail | x ≈ 0.48 | Right edge of rarefaction wave |
| Contact discontinuity | x ≈ 0.69 | Density jump |
| Shock wave | x ≈ 0.85 | Sharp jump in all quantities |

### Quantitative Values

```python
# At contact discontinuity (x ≈ 0.69)
u_contact = 0.927      # Velocity
p_contact = 0.303      # Pressure
rho_left_contact = 0.426   # Density (left of contact)
rho_right_contact = 0.265  # Density (right of contact)

# Post-shock state (between shock and contact)
rho_post_shock = 0.265  # Density
p_post_shock = 0.303    # Pressure
u_post_shock = 0.927    # Velocity

# Pre-shock state (right of shock)
rho_pre_shock = 0.125   # Density (initial)
p_pre_shock = 0.1       # Pressure (initial)
u_pre_shock = 0.0       # Velocity (initial)
```

## Verification Metrics

### Initial State (t = 0)
- [ ] Density ratio: 8.0 ± 0.5
- [ ] Pressure ratio: 10.0 (exact)
- [ ] Velocity everywhere: 0.0
- [ ] Total particles: 9,000

### During Evolution
- [ ] Total mass conserved: Δm < 0.1%
- [ ] Total energy conserved: ΔE < 5% (with dissipation)
- [ ] No artificial 2D effects (Y-variation minimal)

### Final State (t = 0.2s)
- [ ] Shock position: 0.84 ± 0.02
- [ ] Contact position: 0.68 ± 0.02
- [ ] Maximum velocity: 0.93 ± 0.05
- [ ] Shock width: 2-3 particles (~0.04-0.06 units)

## Python Animation Setup

```python
# Domain
domain_x = [0.0, 1.0]
domain_y = [0.0, 0.1]

# Centerline extraction (for comparison with 1D)
y_center = 0.05  # Middle of planar domain
tolerance = 0.03  # Capture particles near centerline

# Analytical solution
x_analytical = np.linspace(0.0, 1.0, 500)
sod.x_discontinuity = 0.5

# Use SPH-computed initial densities
sod.rho_L = rho_left_actual   # From SPH at t=0
sod.rho_R = rho_right_actual  # From SPH at t=0
```

## Common Mistakes to Avoid

❌ **Wrong domain**: Using [-0.5, 1.5] instead of [0, 1]
✅ **Correct**: Standard Sod domain is [0, 1]

❌ **Large Y-height**: Using Y=0.5 (creates 2D effects)
✅ **Correct**: Use Y=0.1 for planar 2D

❌ **Equal spacing**: dx_left = dx_right (wrong density ratio)
✅ **Correct**: dx_right = 8 × dx_left for equal mass particles

❌ **Variable mass**: Adjusting mass to match density
✅ **Correct**: Equal mass for all particles (SPH convention)

❌ **Wrong discontinuity**: x = 0.0 or other positions
✅ **Correct**: x = 0.5 (midpoint of [0,1] domain)

## References to Paper Database

See `/Users/guo/sph-simulation/docs/papers/2d-sph-sod-shocktube/` for:
- `PAPERS_SUMMARY.md` - Complete bibliography
- `INITIAL_CONDITIONS_GUIDE.md` - Detailed setup instructions

Key papers:
1. Puri & Ramachandran (2014) - SPH schemes comparison
2. Price (2024) - Modern shock capturing
3. Chen et al. (2019) - 2D SPH methods
4. Owen et al. (1997) - Adaptive SPH 2D/3D

## Build Commands

```bash
# Clean build
cd /Users/guo/sph-simulation/workflows/shock_tube_workflow/02_simulation_2d
make full-clean
make build

# Run simulations
make run-disph    # DISPH method
make run-gsph     # GSPH method (Riemann solver)
make run-ssph     # SSPH method (artificial viscosity)

# Generate animations
cd scripts
python3 animate_disph.py
python3 animate_gsph.py
python3 animate_ssph.py
```

## Debugging Tips

**Density ratio not 8:1?**
- Check smoothing length computation
- Verify neighbor search includes enough neighbors
- Ensure iterative sml is enabled

**Shock too diffuse?**
- GSPH: Verify HLL solver is active
- SSPH: Increase artificial viscosity α (try 1.0-2.0)
- Check particle resolution (need ~200 particles in left region)

**Non-planar waves?**
- Verify Y-boundaries are PERIODIC
- Check uniform particle distribution in Y
- Ensure sufficient Y-resolution (≥40 particles)

**Simulation crashes?**
- Check boundary range includes all particles
- Verify ghost particle generation
- Check for negative densities/pressures

## Date: 2025-11-07

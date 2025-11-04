# GSPH Algorithm Documentation

**Godunov Smoothed Particle Hydrodynamics**

## Overview

GSPH (Godunov SPH) is an advanced SPH variant that uses Riemann solvers to compute inter-particle fluxes, providing superior shock-capturing capabilities compared to standard SPH. This implementation follows the MUSCL-Hancock scheme with optional second-order spatial accuracy.

### Key Features

- **Riemann Solver Integration**: HLL solver for robust shock handling
- **MUSCL Reconstruction**: Second-order spatial accuracy via slope limiting
- **Van Leer Limiter**: TVD property preservation
- **Monaghan Viscosity**: Artificial dissipation with Balsara switch
- **Modular Architecture**: Independently testable algorithm components

## Theoretical Foundation

### 1. SPH Discretization

SPH approximates field quantities using kernel interpolation:

```
A(r) = Σ_j m_j (A_j / ρ_j) W(|r - r_j|, h)
```

where:
- `A(r)` = field quantity at position r
- `m_j` = mass of particle j
- `ρ_j` = density of particle j  
- `W(r, h)` = smoothing kernel with support radius h

**Kernel Properties:**
- Compact support: W(r, h) = 0 for r > 2h (cubic spline)
- Normalization: ∫ W(r, h) dr = 1
- Delta function limit: lim(h→0) W(r, h) = δ(r)

### 2. Godunov SPH Method

GSPH replaces standard SPH pressure forces with Riemann-solver-based fluxes.

#### Standard SPH Momentum Equation:
```
dv_i/dt = -Σ_j m_j (P_i/ρ_i² + P_j/ρ_j² + π_ij) ∇W_ij
```

#### GSPH Momentum Equation:
```
dv_i/dt = -Σ_j m_j (P*/ρ* + π_ij) ∇W_ij
```

where `P*/ρ*` comes from solving the Riemann problem at the particle interface.

### 3. MUSCL Reconstruction (Second Order)

To achieve second-order spatial accuracy, we reconstruct states at particle interfaces:

#### Left and Right States:
```
ρ_L = ρ_i + 0.5 * φ(r_i) * ∇ρ_i · (r_j - r_i)
ρ_R = ρ_j - 0.5 * φ(r_j) * ∇ρ_j · (r_j - r_i)
```

where:
- `φ(r)` = slope limiter (ensures TVD property)
- `∇ρ_i` = density gradient at particle i
- Factor 0.5 = MUSCL extrapolation coefficient

**Purpose:**
- Prevents spurious oscillations near discontinuities
- Maintains second-order accuracy in smooth regions
- Ensures physical realizability (positive density/pressure)

### 4. Van Leer Slope Limiter

Ensures Total Variation Diminishing (TVD) property:

```
φ(r) = (r + |r|) / (1 + r)  if dq1 * dq2 > 0
φ(r) = 0                     if dq1 * dq2 ≤ 0
```

where r = dq1/dq2 (ratio of consecutive gradients).

**Properties:**
- Symmetric: φ(r) = φ(1/r) / r
- Second-order in smooth regions (φ → 1 as r → 1)
- First-order near extrema (φ → 0 when gradients change sign)
- TVD: prevents new extrema from forming

### 5. HLL Riemann Solver

Solves the 1D Riemann problem between particles:

```
        Left State           Right State
     ρ_L, P_L, v_L  |  ρ_R, P_R, v_R
                    x=0
```

**HLL Approximate Solution:**
```
        S_L < 0 < S_R
    ┌────────┬────────┬────────┐
    │   L    │   *    │   R    │
    └────────┴────────┴────────┘
    
If v* < 0:  Use left state
If v* > 0:  Use right state
```

**Wave Speeds (with Roe averaging):**
```
S_L = min(v_L - c_L, ṽ - c̃)
S_R = max(v_R + c_R, ṽ + c̃)
```

where:
- ṽ = Roe-averaged velocity
- c̃ = Roe-averaged sound speed
- c = √(γ P / ρ) = sound speed

**Star Region State:**
```
ρ* = ρ_K * (S_K - v_K) / (S_K - v*)
P* = P_K + ρ_K * (S_K - v_K) * (v* - v_K)
v* = (P_R - P_L + ρ_L*v_L*(S_L - v_L) - ρ_R*v_R*(S_R - v_R)) / 
     (ρ_L*(S_L - v_L) - ρ_R*(S_R - v_R))
```

### 6. Artificial Viscosity

Monaghan (1997) artificial viscosity for shock stabilization:

```
π_ij = -α_ij * v_sig * w_ij / (2 * ρ_ij)   if v_ij · r_ij < 0
π_ij = 0                                    if v_ij · r_ij ≥ 0
```

where:
- `α_ij = (α_i + α_j) / 2` = average viscosity coefficient
- `v_sig = c_i + c_j - 3 * w_ij` = signal velocity
- `w_ij = (v_ij · r_ij) / |r_ij|` = radial velocity component
- `ρ_ij = (ρ_i + ρ_j) / 2` = average density

**Balsara Switch** (Morris & Monaghan 1997):

Reduces viscosity in shear flows:
```
f_i = |∇·v| / (|∇·v| + |∇×v| + ε * c/h)
```

Applied as: `π_ij → f_ij * π_ij` where `f_ij = (f_i + f_j) / 2`

## Implementation Architecture

### Modular Components

```
algorithms/
├── riemann/
│   ├── riemann_solver.hpp       # Abstract interface
│   └── hll_solver.hpp           # HLL implementation
├── limiters/
│   ├── slope_limiter.hpp        # Abstract interface
│   └── van_leer_limiter.hpp     # Van Leer implementation
└── viscosity/
    ├── artificial_viscosity.hpp # Abstract interface
    └── monaghan_viscosity.hpp   # Monaghan implementation
```

### GSPH FluidForce Class

Located in `include/gsph/g_fluid_force.hpp`:

```cpp
template<int Dim>
class FluidForce : public sph::FluidForce<Dim> {
private:
    bool m_is_2nd_order;
    std::unique_ptr<algorithms::riemann::RiemannSolver> m_riemann_solver;
    std::unique_ptr<algorithms::limiters::SlopeLimiter> m_slope_limiter;
    
public:
    void initialize(std::shared_ptr<SPHParameters> param) override;
    void calculation(std::shared_ptr<Simulation<Dim>> sim) override;
};
```

## Algorithm Flow

### Main Calculation Loop

```
For each particle i:
    1. Find neighbors within 2h
    
    For each neighbor j:
        2. Compute kernel gradient ∇W_ij
        
        If second-order mode:
            3. Reconstruct left/right states (MUSCL)
            4. Apply slope limiter (Van Leer)
        Else:
            3. Use particle states directly
            
        5. Solve Riemann problem (HLL)
        6. Extract P*/ρ* from solution
        7. Compute artificial viscosity π_ij
        8. Update acceleration:
           a_i -= m_j * (P*/ρ* + π_ij) * ∇W_ij
```

### Pseudo-Code

```cpp
// For each particle i
for (int i = 0; i < num_particles; ++i) {
    auto& p_i = particles[i];
    Vector<Dim> acc = {0};
    
    // For each neighbor j
    for (int n = 0; n < n_neighbors; ++n) {
        auto& p_j = neighbors[n];
        
        // 1. Kernel gradient
        Vector<Dim> dw_ij = kernel->dw(r_ij, r, h);
        
        if (is_2nd_order) {
            // 2. MUSCL reconstruction
            real rho_L = p_i.dens + 0.5 * phi_i * grad_rho_i;
            real rho_R = p_j.dens - 0.5 * phi_j * grad_rho_j;
            // ... (pressure, velocity similarly)
            
            // 3. Slope limiting
            phi_i = slope_limiter->limit(grad_upstream, grad_local);
        } else {
            rho_L = p_i.dens;
            rho_R = p_j.dens;
        }
        
        // 4. Riemann solver
        RiemannState left{rho_L, P_L, v_L};
        RiemannState right{rho_R, P_R, v_R};
        auto solution = riemann_solver->solve(left, right, gamma);
        
        // 5. Artificial viscosity
        ViscosityState visc_state{p_i, p_j, r_ij, r};
        real pi_ij = viscosity->compute(visc_state);
        
        // 6. Force accumulation
        acc -= m_j * (solution.pressure/solution.density + pi_ij) * dw_ij;
    }
    
    p_i.acc = acc;
}
```

## Usage Examples

### Basic GSPH Setup (First Order)

```cpp
#include "core/simulation.hpp"
#include "gsph/g_fluid_force.hpp"

// Create simulation
auto sim = std::make_shared<Simulation<1>>();
sim->particles = initialize_shock_tube();

// Configure GSPH (first order)
auto params = SPHParametersBuilder()
    .with_time(0.0, 0.2, 0.01)
    .with_sph_type("gsph")
    .with_cfl(0.3, 0.25)
    .with_physics(50, 1.4)
    .with_kernel("cubic_spline")
    .with_artificial_viscosity(1.0, true, false, 1.0, 0.1, 0.01)
    .build();

// GSPH-specific: disable second order
params->gsph.is_2nd_order = false;

// Initialize and run
auto fluid_force = std::make_shared<gsph::FluidForce<1>>();
fluid_force->initialize(params);

while (sim->time < params->time.end) {
    fluid_force->calculation(sim);
    timestep->update(sim);
}
```

### Second-Order GSPH (MUSCL-Hancock)

```cpp
// Enable second-order accuracy
params->gsph.is_2nd_order = true;

// This automatically activates:
// - MUSCL reconstruction
// - Van Leer slope limiting
// - Gradient calculations
```

### Custom Riemann Solver

```cpp
// Implement custom solver
class ExactRiemannSolver : public algorithms::riemann::RiemannSolver {
public:
    RiemannSolution solve(
        const RiemannState& left,
        const RiemannState& right,
        real gamma) const override {
        // Exact solution via iterative Newton-Raphson
        // ...
    }
};

// Use in FluidForce
m_riemann_solver = std::make_unique<ExactRiemannSolver>();
```

## Physical Parameters

### Adiabatic Index (γ)

- **Monoatomic gas**: γ = 5/3 ≈ 1.667 (argon, helium)
- **Diatomic gas**: γ = 7/5 = 1.4 (air, nitrogen, oxygen)
- **Polytropic**: γ = (n+1)/n

Defined in simulation as:
```cpp
real m_adiabatic_index;  // γ in ideal gas EOS: P = (γ-1)ρe
```

### CFL Conditions

**Sound-based timestep:**
```
dt_sound = CFL_sound * h / (c_s + |v|)
```
- Typical: CFL_sound = 0.3
- Range: 0.2 - 0.4

**Force-based timestep:**
```
dt_force = CFL_force * √(h / |a|)
```
- Typical: CFL_force = 0.25  
- Range: 0.15 - 0.3

### Artificial Viscosity Coefficient

- **Standard**: α = 1.0
- **Low dissipation**: α = 0.5
- **High dissipation (shocks)**: α = 1.5 - 2.0

## Test Cases

### Sod Shock Tube (1D)

**Initial Conditions:**
```
Left:  ρ = 1.0,   P = 1.0,   v = 0.0    (x < 0.5)
Right: ρ = 0.125, P = 0.1,   v = 0.0    (x > 0.5)
γ = 1.4
```

**Expected Results at t = 0.2:**
- Shock wave at x ≈ 0.85
- Contact discontinuity at x ≈ 0.69
- Rarefaction wave 0.26 < x < 0.48

**GSPH Performance:**
- First-order: Captures shock, some smearing
- Second-order: Sharp discontinuities, minimal oscillations

### 2D Sedov Blast Wave

**Initial Conditions:**
```
Background: ρ = 1.0, P = 10⁻⁵, v = 0
Energy injection: E = 1.0 at origin
γ = 1.4
```

**Expected Results:**
- Spherical shock wave
- Self-similar solution: R(t) ∝ t^(2/5)
- Density jump: ρ_shock / ρ_0 = (γ+1)/(γ-1) = 6 for γ=1.4

## Performance Considerations

### Computational Cost

**First-order GSPH:**
- ~2× slower than standard SPH
- Riemann solver: O(1) per interaction

**Second-order GSPH:**
- ~3× slower than standard SPH
- Additional gradient calculations
- Slope limiting overhead

### Optimization Tips

1. **Use HLL instead of exact solver** - 10× faster, 95% accuracy
2. **Disable second-order for initial relaxation** - Faster convergence
3. **Adaptive viscosity coefficient** - Reduce α in smooth regions
4. **Tree-based neighbor search** - O(N log N) vs O(N²)

## References

### Core Papers

1. **Monaghan & Gingold (1983)**  
   "Shock simulation by the particle method SPH"  
   *J. Comp. Phys.* 52, 374-389  
   [Original SPH shock simulation]

2. **Inutsuka (2002)**  
   "Reformulation of SPH with Riemann solver"  
   *J. Comp. Phys.* 179, 238-267  
   [GSPH method introduction]

3. **Monaghan (1997)**  
   "SPH and Riemann solvers"  
   *J. Comp. Phys.* 136, 298-307  
   [Artificial viscosity, signal velocity]

4. **Morris & Monaghan (1997)**  
   "A switch to reduce SPH viscosity"  
   *J. Comp. Phys.* 136, 41-50  
   [Balsara switch]

5. **Van Leer (1979)**  
   "Towards the ultimate conservative difference scheme V"  
   *J. Comp. Phys.* 32, 101-136  
   [MUSCL reconstruction, slope limiters]

### Implementation Details

6. **Toro (2009)**  
   *Riemann Solvers and Numerical Methods for Fluid Dynamics*  
   Springer  
   [HLL solver, exact Riemann solver]

7. **Price (2012)**  
   "Smoothed particle hydrodynamics and magnetohydrodynamics"  
   *J. Comp. Phys.* 231, 759-794  
   [Comprehensive SPH review]

## Troubleshooting

### Numerical Instabilities

**Symptom:** Particle disorder, negative densities/pressures  
**Solution:**
- Reduce timestep (lower CFL coefficients)
- Increase artificial viscosity (α = 1.5 - 2.0)
- Enable Balsara switch
- Check initial particle distribution (avoid large voids)

### Excessive Dissipation

**Symptom:** Shock too smeared, contact discontinuity diffused  
**Solution:**
- Enable second-order mode
- Reduce artificial viscosity (α = 0.5 - 0.75)
- Use exact Riemann solver instead of HLL
- Increase resolution (more particles)

### Spurious Oscillations

**Symptom:** Unphysical wiggles near discontinuities  
**Solution:**
- Ensure slope limiter is active
- Switch to more dissipative limiter (MinMod instead of Van Leer)
- Disable second-order mode temporarily
- Check for programming errors in gradient calculation

## Extending the Implementation

### Adding a New Riemann Solver

```cpp
// 1. Create header: include/algorithms/riemann/my_solver.hpp
class MySolver : public RiemannSolver {
public:
    RiemannSolution solve(
        const RiemannState& left,
        const RiemannState& right,
        real gamma) const override {
        // Your implementation
    }
    
    std::string name() const override {
        return "My Custom Solver";
    }
};

// 2. Write BDD tests: tests/algorithms/riemann/my_solver_test.cpp
FEATURE("My Riemann Solver") {
    SCENARIO("Sod shock tube") {
        // Test cases
    }
}

// 3. Integrate into FluidForce initialization
m_riemann_solver = std::make_unique<algorithms::riemann::MySolver>();
```

### Adding a New Slope Limiter

```cpp
// 1. Create header: include/algorithms/limiters/my_limiter.hpp
class MyLimiter : public SlopeLimiter {
public:
    real limit(real dq1, real dq2) const override {
        // Your limiter formula
        // Ensure: φ(r) symmetric, TVD property
    }
};

// 2. Write comprehensive tests ensuring:
// - TVD property
// - Symmetry: φ(r) = φ(1/r) / r
// - Second-order in smooth regions
// - First-order near extrema

// 3. Integrate
m_slope_limiter = std::make_unique<algorithms::limiters::MyLimiter>();
```

## See Also

- `RIEMANN_SOLVER_GUIDE.md` - Detailed Riemann solver theory
- `SLOPE_LIMITER_THEORY.md` - TVD schemes and limiting
- `CONTRIBUTING.md` - Development workflow
- `TESTING_GUIDELINES.md` - BDD test patterns

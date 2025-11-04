# Architecture Review & Modularization Recommendations

**Date:** 2025-11-05  
**Reviewer:** Architectural Analysis via Serena MCP  
**Project:** SPH Simulation Framework

---

## Executive Summary

This document provides a comprehensive architectural review of the SPH simulation codebase with focus on:
- **Modularization** - Extracting algorithms into testable units
- **Naming conventions** - Consistent, self-documenting code
- **Testability** - BDD-style comprehensive test coverage
- **Code quality** - Eliminating magic numbers, improving separation of concerns

---

## 🔍 Current State Analysis

### Strengths ✅

1. **Good BDD Test Infrastructure**
   - `bdd_helpers.hpp` provides FEATURE/SCENARIO/GIVEN/WHEN/THEN macros
   - Test helpers for edge cases, benchmarking, vector comparisons
   - Already testing some Riemann solver edge cases

2. **Template-Based Design**
   - Dimension-agnostic code (`Dim` template parameter)
   - Type aliases for 1D/2D/3D specializations
   - Consistent pattern across modules

3. **Module Pattern**
   - Abstract `Module<Dim>` base class
   - Consistent interface: `initialize()` and `calculation()`
   - Easy to swap algorithm implementations

4. **Multiple Algorithm Variants**
   - GSPH (Godunov SPH)
   - DISPH (Density-Independent SPH)
   - SSPH, SRGSPH, GDISPH variants
   - Shows extensibility of design

5. **Plugin Architecture**
   - Workflow-based simulation setup
   - Dynamic loading of simulation configurations
   - Good separation from core library

### Critical Issues ❌

#### Issue #1: Embedded Algorithms (Not Testable)

**Problem:** Core algorithms are embedded within larger classes as lambdas or protected methods.

**Example - HLL Riemann Solver:**
```cpp
// Current: In gsph::FluidForce<Dim>::hll_solver()
template<int Dim>
void FluidForce<Dim>::hll_solver()
{
    this->m_solver = [&](const real left[], const real right[], 
                         real & pstar, real & vstar) {
        // 40 lines of Riemann solver logic embedded here
        // Cannot test independently!
    };
}
```

**Impact:**
- ❌ Cannot unit test Riemann solver with edge cases (vacuum, strong shocks, sonic points)
- ❌ Cannot benchmark different solvers (HLL vs HLLC vs Exact)
- ❌ Cannot reuse solver in other contexts
- ❌ Difficult to verify numerical correctness against known solutions

**Example - Van Leer Limiter:**
```cpp
// Current: Inline in g_fluid_force.tpp calculation() method
const real limited = (dv_ij * dve_i <= 0.0) ? 0.0 : 
                     2.0 * dv_ij * dve_i / (dv_ij + dve_i);
```

**Impact:**
- ❌ Cannot test limiter properties (TVD, monotonicity)
- ❌ Cannot compare with other limiters (MinMod, Superbee)
- ❌ Limiter logic scattered in multiple places

#### Issue #2: Magic Numbers Everywhere

**Examples:**
```cpp
// What is 0.5? (MUSCL extrapolation coefficient)
const real delta_i = 0.5 * (1.0 - p_i.sound * dt * r_inv);

// What is gamma - 1.0? (Polytropic index minus one for ideal gas)
grad_p[i] = (dd * p_i.ene + du) * (this->m_gamma - 1.0);

// Why 3.0? (Signal velocity estimation coefficient)
const real v_sig = p_i.sound + p_j.sound - 3.0 * inner_product(...);

// What is this A? (Sphere volume coefficient for kernel normalization)
constexpr real A = DIM == 1 ? 2.0 : 
                   DIM == 2 ? M_PI : 
                              4.0 * M_PI / 3.0;

// What is 5? (Default tree size multiplier)
void resize(const int particle_num, const int tree_size = 5);
```

**Impact:**
- ❌ Code is difficult to understand without deep physics knowledge
- ❌ Easy to make mistakes when modifying
- ❌ Impossible to grep for usage of specific constants
- ❌ Violates coding_rules.instructions.md: "Never use Magic Numbers"

#### Issue #3: Poor Separation of Concerns

**Example - FluidForce::calculation() does too much:**
```cpp
template<int Dim>
void FluidForce<Dim>::calculation(std::shared_ptr<Simulation<Dim>> sim)
{
    // 1. MUSCL gradient reconstruction
    // 2. Van Leer slope limiting
    // 3. State extrapolation
    // 4. Riemann problem solving
    // 5. Flux calculation
    // 6. Artificial viscosity
    // 7. Artificial conductivity
    // 8. Acceleration accumulation
    // ALL IN ONE 200+ LINE METHOD!
}
```

**Impact:**
- ❌ Cannot test individual steps
- ❌ Difficult to understand algorithm flow
- ❌ Hard to modify one aspect without breaking others
- ❌ Violates Single Responsibility Principle

#### Issue #4: Inconsistent Naming Conventions

**Examples:**
```cpp
// Functions: mix of styles
void hll_solver();        // snake_case (good)
void calculation();       // snake_case (good)
void make_tree();        // snake_case (good)
void update_time();      // snake_case (good)

// Variables: inconsistent prefixes
real m_gamma;            // m_ prefix for member
real p_i;                // no prefix for local
real v_sig_max;          // no prefix
real rho2_inv_i;         // cryptic abbreviations

// Constants: not using UPPER_CASE
constexpr real A = ...;  // Should be SPHERE_VOLUME_COEFF
const real delta_i = 0.5;  // 0.5 should be named constant
```

**Impact:**
- ❌ Harder to read and understand
- ❌ Easy to confuse member vs local variables
- ❌ Inconsistent with coding_rules.instructions.md

#### Issue #5: Limited Test Coverage for Edge Cases

**Current test coverage:**
```cpp
// tests/algorithms/gsph/g_fluid_force_test.cpp
// ✅ Has basic Riemann solver test
// ✅ Has Van Leer limiter test
// ❌ Missing vacuum state tests
// ❌ Missing strong shock tests
// ❌ Missing sonic rarefaction tests
// ❌ Missing extreme density ratio tests (1e-6 to 1e6)
// ❌ Missing contact discontinuity preservation tests
```

**Impact:**
- ❌ Cannot verify correctness for difficult problems
- ❌ Regressions may not be caught
- ❌ Unclear if algorithms handle edge cases properly

---

## 🎯 Recommended Architecture

### Proposed Directory Structure

```
include/
├── core/                          # Core simulation framework (KEEP AS-IS)
│   ├── simulation.hpp
│   ├── sph_particle.hpp
│   ├── ghost_particle_manager.hpp
│   ├── bhtree.hpp
│   └── ...
│
├── algorithms/                    # Algorithm implementations
│   ├── riemann/                  # ⭐ NEW: Riemann solver module
│   │   ├── riemann_solver.hpp    # Abstract base interface
│   │   ├── hll_solver.hpp        # HLL (Harten-Lax-van Leer)
│   │   ├── hllc_solver.hpp       # HLLC (with contact restoration)
│   │   └── exact_solver.hpp      # Exact Riemann solver
│   │
│   ├── limiters/                 # ⭐ NEW: Slope limiters
│   │   ├── slope_limiter.hpp     # Abstract base interface
│   │   ├── van_leer_limiter.hpp  # Van Leer (smooth)
│   │   ├── minmod_limiter.hpp    # MinMod (most diffusive)
│   │   └── superbee_limiter.hpp  # Superbee (least diffusive)
│   │
│   ├── kernels/                  # ⭐ REFACTOR: Move from core/
│   │   ├── kernel_function.hpp   # Abstract base (already exists)
│   │   ├── cubic_spline.hpp      # Move from core/
│   │   ├── wendland_kernel.hpp   # Move from core/
│   │   └── quintic_spline.hpp    # NEW: Add if needed
│   │
│   ├── reconstruction/           # ⭐ NEW: MUSCL reconstruction
│   │   ├── gradient_estimator.hpp
│   │   └── muscl_hancock.hpp
│   │
│   ├── viscosity/                # ⭐ EXTRACT: From FluidForce
│   │   ├── artificial_viscosity.hpp
│   │   └── artificial_conductivity.hpp
│   │
│   └── time_integration/         # ⭐ NEW: Time stepping schemes
│       ├── time_integrator.hpp   # Abstract base
│       ├── euler_integrator.hpp
│       ├── runge_kutta.hpp
│       └── predictor_corrector.hpp
│
├── physics/                       # ⭐ NEW: Physics models
│   ├── equation_of_state.hpp     # EOS interface
│   ├── ideal_gas_eos.hpp         # Ideal gas: P = (γ-1)ρe
│   ├── polytropic_eos.hpp        # Polytropic: P = Kρ^γ
│   └── sound_speed.hpp           # Sound speed calculator
│
├── utilities/                     # ⭐ NEW: Math & helper functions
│   ├── constants.hpp             # Physical/numerical constants
│   ├── math_utils.hpp            # Safe math operations
│   ├── vector_ops.hpp            # Vector operations (may exist)
│   └── floating_point.hpp        # Safe comparisons
│
└── [existing directories]
    ├── gsph/                     # ⭐ REFACTOR: Use new modules
    ├── disph/                    # ⭐ REFACTOR: Use new modules
    └── ...
```

### Test Structure

```
tests/
├── algorithms/
│   ├── riemann/                  # ⭐ NEW: Riemann solver tests
│   │   ├── hll_solver_test.cpp
│   │   ├── hllc_solver_test.cpp
│   │   ├── riemann_edge_cases_test.cpp
│   │   └── riemann_problems_test.cpp  # Sod, Toro benchmarks
│   │
│   ├── limiters/                 # ⭐ NEW: Limiter tests
│   │   ├── van_leer_test.cpp
│   │   ├── minmod_test.cpp
│   │   └── limiter_properties_test.cpp  # TVD, monotonicity
│   │
│   ├── kernels/                  # ⭐ NEW: Kernel tests
│   │   ├── cubic_spline_test.cpp
│   │   ├── wendland_test.cpp
│   │   └── kernel_properties_test.cpp   # Normalization, partition
│   │
│   ├── viscosity/                # ⭐ NEW: Viscosity tests
│   │   ├── artificial_viscosity_test.cpp
│   │   └── viscosity_properties_test.cpp
│   │
│   └── reconstruction/           # ⭐ NEW: Reconstruction tests
│       └── muscl_test.cpp
│
├── physics/                      # ⭐ NEW: Physics tests
│   ├── ideal_gas_eos_test.cpp
│   └── sound_speed_test.cpp
│
├── utilities/                    # ⭐ NEW: Utility tests
│   ├── constants_test.cpp
│   ├── math_utils_test.cpp
│   └── floating_point_test.cpp
│
└── [existing test directories]
    ├── core/                     # ⭐ KEEP: Core tests
    ├── gsph/                     # ⭐ REFACTOR: Update to use new modules
    └── ...
```

---

## 📝 Implementation Examples

### Example 1: Riemann Solver Interface

**File:** `include/algorithms/riemann/riemann_solver.hpp`

```cpp
#pragma once

#include "defines.hpp"
#include <string>

namespace sph {
namespace algorithms {
namespace riemann {

/**
 * @brief State vector for 1D Riemann problem
 * 
 * Represents thermodynamic state on left or right side of discontinuity
 */
struct RiemannState {
    real velocity;      // u: Velocity in normal direction
    real density;       // ρ: Mass density
    real pressure;      // P: Pressure
    real sound_speed;   // c: Sound speed
};

/**
 * @brief Solution of Riemann problem
 * 
 * Interface state between left and right regions
 */
struct RiemannSolution {
    real pressure;      // P*: Interface pressure
    real velocity;      // u*: Interface velocity
};

/**
 * @brief Abstract interface for Riemann solvers
 * 
 * Riemann problem: Given left state (u_L, ρ_L, P_L) and right state (u_R, ρ_R, P_R),
 * find interface state (u*, P*) that satisfies conservation laws.
 * 
 * Common solvers:
 * - HLL: Harten-Lax-van Leer (fast, diffusive)
 * - HLLC: HLL with contact wave (restores contact discontinuity)
 * - Exact: Iterative exact solver (accurate, expensive)
 * 
 * All solvers must be testable independently with:
 * - Sod shock tube
 * - Vacuum formation
 * - Strong shocks
 * - Contact discontinuities
 * - Sonic rarefactions
 */
class RiemannSolver {
public:
    virtual ~RiemannSolver() = default;
    
    /**
     * @brief Solve Riemann problem for interface state
     * 
     * @param left Left state (u_L, ρ_L, P_L, c_L)
     * @param right Right state (u_R, ρ_R, P_R, c_R)
     * @return Interface state (P*, u*)
     */
    virtual RiemannSolution solve(const RiemannState& left, 
                                   const RiemannState& right) const = 0;
    
    /**
     * @brief Get solver name for logging/debugging
     */
    virtual std::string get_name() const = 0;
    
    /**
     * @brief Validate input states (positive density, pressure, sound speed)
     */
    static bool validate_states(const RiemannState& left, const RiemannState& right);
};

} // namespace riemann
} // namespace algorithms
} // namespace sph
```

**File:** `include/algorithms/riemann/hll_solver.hpp`

```cpp
#pragma once

#include "riemann_solver.hpp"
#include "utilities/constants.hpp"
#include <algorithm>
#include <cmath>

namespace sph {
namespace algorithms {
namespace riemann {

/**
 * @brief HLL (Harten-Lax-van Leer) Riemann solver
 * 
 * Two-wave approximation using Roe-averaged wave speeds.
 * 
 * Algorithm:
 * 1. Compute Roe-averaged sound speed: c̃ = (√ρ_L c_L + √ρ_R c_R) / (√ρ_L + √ρ_R)
 * 2. Compute Roe-averaged velocity: ũ = (√ρ_L u_L + √ρ_R u_R) / (√ρ_L + √ρ_R)
 * 3. Estimate wave speeds: S_L = min(u_L - c_L, ũ - c̃), S_R = max(u_R + c_R, ũ + c̃)
 * 4. Compute interface state from jump conditions
 * 
 * Reference: Toro (2009) "Riemann Solvers and Numerical Methods for Fluid Dynamics"
 */
class HLLSolver : public RiemannSolver {
public:
    RiemannSolution solve(const RiemannState& left, 
                         const RiemannState& right) const override;
    
    std::string get_name() const override { return "HLL"; }

private:
    /**
     * @brief Compute Roe-averaged quantities
     * 
     * Uses density-weighted averaging: q̃ = (√ρ_L q_L + √ρ_R q_R) / (√ρ_L + √ρ_R)
     */
    struct RoeAverages {
        real velocity;
        real sound_speed;
    };
    
    RoeAverages compute_roe_averages(const RiemannState& left,
                                     const RiemannState& right) const;
};

} // namespace riemann
} // namespace algorithms
} // namespace sph
```

**File:** `src/algorithms/riemann/hll_solver.cpp`

```cpp
#include "algorithms/riemann/hll_solver.hpp"
#include "utilities/constants.hpp"
#include <cmath>
#include <algorithm>

namespace sph {
namespace algorithms {
namespace riemann {

HLLSolver::RoeAverages 
HLLSolver::compute_roe_averages(const RiemannState& left,
                                const RiemannState& right) const 
{
    const real sqrt_rho_left = std::sqrt(left.density);
    const real sqrt_rho_right = std::sqrt(right.density);
    const real inv_sum = constants::ONE / (sqrt_rho_left + sqrt_rho_right);
    
    RoeAverages avg;
    avg.velocity = (sqrt_rho_left * left.velocity + 
                    sqrt_rho_right * right.velocity) * inv_sum;
    avg.sound_speed = (sqrt_rho_left * left.sound_speed + 
                       sqrt_rho_right * right.sound_speed) * inv_sum;
    
    return avg;
}

RiemannSolution 
HLLSolver::solve(const RiemannState& left, const RiemannState& right) const 
{
    // Compute Roe-averaged wave speeds
    const auto avg = compute_roe_averages(left, right);
    
    // Estimate left and right wave speeds
    const real wave_speed_left = std::min(left.velocity - left.sound_speed,
                                         avg.velocity - avg.sound_speed);
    const real wave_speed_right = std::max(right.velocity + right.sound_speed,
                                          avg.velocity + avg.sound_speed);
    
    // Compute intermediate state coefficients
    const real coeff_left = left.density * (wave_speed_left - left.velocity);
    const real coeff_right = right.density * (wave_speed_right - right.velocity);
    const real coeff_inv = constants::ONE / (coeff_left - coeff_right);
    
    // Compute momentum flux difference
    const real flux_diff_left = left.pressure - left.velocity * coeff_left;
    const real flux_diff_right = right.pressure - right.velocity * coeff_right;
    
    // Interface state from conservation laws
    RiemannSolution solution;
    solution.velocity = (flux_diff_right - flux_diff_left) * coeff_inv;
    solution.pressure = (coeff_left * flux_diff_right - 
                        coeff_right * flux_diff_left) * coeff_inv;
    
    return solution;
}

bool RiemannSolver::validate_states(const RiemannState& left, 
                                    const RiemannState& right) 
{
    return (left.density > constants::ZERO) &&
           (left.pressure > constants::ZERO) &&
           (left.sound_speed > constants::ZERO) &&
           (right.density > constants::ZERO) &&
           (right.pressure > constants::ZERO) &&
           (right.sound_speed > constants::ZERO);
}

} // namespace riemann
} // namespace algorithms
} // namespace sph
```

### Example 2: Constants Header

**File:** `include/utilities/constants.hpp`

```cpp
#pragma once

#include "defines.hpp"

namespace sph {
namespace utilities {
namespace constants {

// ============================================================================
// Mathematical Constants
// ============================================================================

constexpr real ZERO = 0.0;
constexpr real ONE = 1.0;
constexpr real TWO = 2.0;
constexpr real THREE = 3.0;
constexpr real FOUR = 4.0;
constexpr real FIVE = 5.0;
constexpr real HALF = 0.5;
constexpr real QUARTER = 0.25;
constexpr real ONE_THIRD = 1.0 / 3.0;
constexpr real TWO_THIRDS = 2.0 / 3.0;

// ============================================================================
// Physical Constants (Ideal Gas)
// ============================================================================

/** Adiabatic index for monoatomic ideal gas (argon, helium) */
constexpr real GAMMA_MONOATOMIC = 5.0 / 3.0;

/** Adiabatic index for diatomic ideal gas (air, nitrogen, oxygen) */
constexpr real GAMMA_DIATOMIC = 7.0 / 5.0;

/** Commonly used: γ - 1 for ideal gas EOS */
constexpr real GAMMA_MINUS_ONE_MONOATOMIC = GAMMA_MONOATOMIC - ONE;
constexpr real GAMMA_MINUS_ONE_DIATOMIC = GAMMA_DIATOMIC - ONE;

// ============================================================================
// Geometric Constants
// ============================================================================

/** Volume of unit sphere in 1D (length of interval [-0.5, 0.5]) */
constexpr real UNIT_SPHERE_VOLUME_1D = TWO;

/** Volume of unit sphere in 2D (area of unit circle) */
constexpr real UNIT_SPHERE_VOLUME_2D = M_PI;

/** Volume of unit sphere in 3D (4π/3) */
constexpr real UNIT_SPHERE_VOLUME_3D = FOUR * M_PI / THREE;

// ============================================================================
// SPH Algorithm Constants
// ============================================================================

/** MUSCL reconstruction: extrapolation distance coefficient */
constexpr real MUSCL_EXTRAPOLATION_COEFF = HALF;

/** Signal velocity estimation: coefficient for relative velocity */
constexpr real SIGNAL_VELOCITY_COEFF = THREE;

/** Default BH-tree size multiplier (for memory allocation) */
constexpr int BHTREE_SIZE_MULTIPLIER = 5;

/** Neighbor list size (for fixed-size arrays) */
constexpr int NEIGHBOR_LIST_SIZE = 20;

// ============================================================================
// Numerical Tolerances
// ============================================================================

/** Machine epsilon for double precision */
constexpr real EPSILON = 1e-15;

/** Small number for avoiding division by zero */
constexpr real TINY = 1e-10;

/** Large number for comparisons */
constexpr real VERY_LARGE = 1e10;

/** Tolerance for floating-point comparisons */
constexpr real FLOAT_TOLERANCE = 1e-12;

} // namespace constants
} // namespace utilities
} // namespace sph
```

### Example 3: Van Leer Limiter

**File:** `include/algorithms/limiters/slope_limiter.hpp`

```cpp
#pragma once

#include "defines.hpp"
#include <string>

namespace sph {
namespace algorithms {
namespace limiters {

/**
 * @brief Abstract interface for slope limiters
 * 
 * Slope limiters are used in high-order reconstruction schemes (MUSCL, etc)
 * to prevent spurious oscillations near discontinuities while maintaining
 * high-order accuracy in smooth regions.
 * 
 * Properties of good limiters:
 * - TVD (Total Variation Diminishing): Don't create new extrema
 * - Monotonicity preserving: Don't amplify existing extrema
 * - Smooth: Avoid switching between limiting and non-limiting
 * 
 * Common limiters (ordered by diffusivity):
 * 1. MinMod - Most diffusive, most robust
 * 2. Van Leer - Moderate diffusivity, smooth
 * 3. Superbee - Least diffusive, can be compressive
 */
class SlopeLimiter {
public:
    virtual ~SlopeLimiter() = default;
    
    /**
     * @brief Compute limited slope
     * 
     * @param gradient_backward Backward difference: q_i - q_{i-1}
     * @param gradient_forward Forward difference: q_{i+1} - q_i
     * @return Limited slope (zero if gradients have opposite signs)
     */
    virtual real limit(real gradient_backward, real gradient_forward) const = 0;
    
    /**
     * @brief Get limiter name
     */
    virtual std::string get_name() const = 0;
};

} // namespace limiters
} // namespace algorithms
} // namespace sph
```

**File:** `include/algorithms/limiters/van_leer_limiter.hpp`

```cpp
#pragma once

#include "slope_limiter.hpp"
#include "utilities/constants.hpp"

namespace sph {
namespace algorithms {
namespace limiters {

/**
 * @brief Van Leer slope limiter
 * 
 * Formula: φ(r) = (r + |r|) / (1 + |r|)
 * 
 * In gradient form:
 * - If gradients have opposite signs: return 0 (flatten)
 * - Otherwise: return harmonic mean 2*g1*g2/(g1+g2)
 * 
 * Properties:
 * - Smooth (differentiable)
 * - Symmetric
 * - TVD (Total Variation Diminishing)
 * - Moderate diffusivity
 * 
 * Reference: van Leer (1974) "Towards the ultimate conservative difference scheme"
 */
class VanLeerLimiter : public SlopeLimiter {
public:
    real limit(real gradient_backward, real gradient_forward) const override;
    
    std::string get_name() const override { return "VanLeer"; }
};

} // namespace limiters
} // namespace algorithms
} // namespace sph
```

**File:** `src/algorithms/limiters/van_leer_limiter.cpp`

```cpp
#include "algorithms/limiters/van_leer_limiter.hpp"
#include "utilities/constants.hpp"

namespace sph {
namespace algorithms {
namespace limiters {

real VanLeerLimiter::limit(real gradient_backward, real gradient_forward) const 
{
    using namespace utilities::constants;
    
    // If gradients have opposite signs, flatten (no oscillations)
    const real product = gradient_backward * gradient_forward;
    if (product <= ZERO) {
        return ZERO;
    }
    
    // Both gradients have same sign: use harmonic mean
    // Formula: 2*g1*g2/(g1+g2) - gives smooth transition
    const real sum = gradient_backward + gradient_forward;
    return TWO * product / sum;
}

} // namespace limiters
} // namespace algorithms
} // namespace sph
```

### Example 4: BDD Tests for Riemann Solver

**File:** `tests/algorithms/riemann/hll_solver_test.cpp`

```cpp
#include "../../bdd_helpers.hpp"
#include "algorithms/riemann/hll_solver.hpp"
#include "utilities/constants.hpp"
#include <cmath>

using namespace sph::algorithms::riemann;
using namespace sph::utilities::constants;

FEATURE(HLLRiemannSolver) {

SCENARIO(HLLSolver, SodShockTube) {
    GIVEN("Sod shock tube initial conditions") {
        // Classic Riemann problem: high pressure left, low pressure right
        RiemannState left;
        left.density = 1.0;
        left.pressure = 1.0;
        left.velocity = ZERO;
        left.sound_speed = std::sqrt(GAMMA_MONOATOMIC * left.pressure / left.density);
        
        RiemannState right;
        right.density = 0.125;
        right.pressure = 0.1;
        right.velocity = ZERO;
        right.sound_speed = std::sqrt(GAMMA_MONOATOMIC * right.pressure / right.density);
        
        WHEN("Solving for interface state") {
            HLLSolver solver;
            auto solution = solver.solve(left, right);
            
            THEN("Interface pressure should be between left and right") {
                EXPECT_GT(solution.pressure, right.pressure);
                EXPECT_LT(solution.pressure, left.pressure);
            }
            
            AND("Interface velocity should be positive (rightward)") {
                EXPECT_GT(solution.velocity, ZERO);
            }
            
            AND("Solution should be physically valid") {
                EXPECT_TRUE(std::isfinite(solution.pressure));
                EXPECT_TRUE(std::isfinite(solution.velocity));
                EXPECT_GT(solution.pressure, ZERO);
            }
        }
    }
}

SCENARIO(HLLSolver, VacuumFormation) {
    GIVEN("Two states moving away from each other (vacuum formation)") {
        constexpr real GAMMA = GAMMA_MONOATOMIC;
        
        RiemannState left;
        left.density = 1.0;
        left.pressure = 1.0;
        left.velocity = -10.0;  // Large leftward velocity
        left.sound_speed = std::sqrt(GAMMA * left.pressure / left.density);
        
        RiemannState right;
        right.density = 1.0;
        right.pressure = 1.0;
        right.velocity = 10.0;  // Large rightward velocity
        right.sound_speed = std::sqrt(GAMMA * right.pressure / right.density);
        
        WHEN("Solving for interface state") {
            HLLSolver solver;
            auto solution = solver.solve(left, right);
            
            THEN("Interface pressure should be very low (near vacuum)") {
                EXPECT_LT(solution.pressure, 0.1 * left.pressure);
            }
            
            AND("Interface velocity should be near zero") {
                EXPECT_NEAR(solution.velocity, ZERO, 1.0);
            }
            
            AND("Solution should remain finite (no NaN)") {
                EXPECT_TRUE(std::isfinite(solution.pressure));
                EXPECT_TRUE(std::isfinite(solution.velocity));
            }
        }
    }
}

SCENARIO(HLLSolver, StrongShock) {
    GIVEN("Strong shock with extreme pressure ratio") {
        constexpr real GAMMA = GAMMA_MONOATOMIC;
        constexpr real PRESSURE_RATIO = 1e6;  // Very strong shock
        
        RiemannState left;
        left.density = 10.0;
        left.pressure = PRESSURE_RATIO;
        left.velocity = ZERO;
        left.sound_speed = std::sqrt(GAMMA * left.pressure / left.density);
        
        RiemannState right;
        right.density = 1.0;
        right.pressure = ONE;
        right.velocity = ZERO;
        right.sound_speed = std::sqrt(GAMMA * right.pressure / right.density);
        
        WHEN("Solving for interface state") {
            HLLSolver solver;
            auto solution = solver.solve(left, right);
            
            THEN("Interface pressure should be much higher than right state") {
                EXPECT_GT(solution.pressure, 100.0 * right.pressure);
            }
            
            AND("Interface velocity should be large and positive") {
                EXPECT_GT(solution.velocity, 10.0);
            }
            
            AND("Solution should not overflow") {
                EXPECT_TRUE(std::isfinite(solution.pressure));
                EXPECT_TRUE(std::isfinite(solution.velocity));
                EXPECT_LT(solution.pressure, 1e20);  // No overflow
            }
        }
    }
}

SCENARIO(HLLSolver, ContactDiscontinuity) {
    GIVEN("Contact discontinuity (pressure equilibrium, density jump)") {
        constexpr real GAMMA = GAMMA_MONOATOMIC;
        constexpr real PRESSURE_EQUILIBRIUM = 1.0;
        
        RiemannState left;
        left.density = 2.0;       // High density
        left.pressure = PRESSURE_EQUILIBRIUM;
        left.velocity = ZERO;
        left.sound_speed = std::sqrt(GAMMA * left.pressure / left.density);
        
        RiemannState right;
        right.density = 1.0;      // Low density
        right.pressure = PRESSURE_EQUILIBRIUM;  // Same pressure
        right.velocity = ZERO;
        right.sound_speed = std::sqrt(GAMMA * right.pressure / right.density);
        
        WHEN("Solving for interface state") {
            HLLSolver solver;
            auto solution = solver.solve(left, right);
            
            THEN("Interface pressure should be approximately maintained") {
                EXPECT_NEAR(solution.pressure, PRESSURE_EQUILIBRIUM, 0.1);
            }
            
            AND("Interface velocity should be near zero") {
                EXPECT_NEAR(solution.velocity, ZERO, 0.1);
            }
        }
    }
}

SCENARIO(HLLSolver, ExtremeDensityRatio) {
    GIVEN("Extreme density ratio (low density right side)") {
        constexpr real GAMMA = GAMMA_MONOATOMIC;
        constexpr real DENSITY_RATIO = 1e6;
        
        RiemannState left;
        left.density = DENSITY_RATIO;
        left.pressure = 1.0;
        left.velocity = ZERO;
        left.sound_speed = std::sqrt(GAMMA * left.pressure / left.density);
        
        RiemannState right;
        right.density = ONE;
        right.pressure = 1.0;
        right.velocity = ZERO;
        right.sound_speed = std::sqrt(GAMMA * right.pressure / right.density);
        
        WHEN("Solving for interface state") {
            HLLSolver solver;
            auto solution = solver.solve(left, right);
            
            THEN("Solution should remain stable") {
                EXPECT_TRUE(std::isfinite(solution.pressure));
                EXPECT_TRUE(std::isfinite(solution.velocity));
            }
            
            AND("Pressure should be positive") {
                EXPECT_GT(solution.pressure, ZERO);
            }
        }
    }
}

SCENARIO(HLLSolver, SonicPoint) {
    GIVEN("Left state with transonic flow") {
        constexpr real GAMMA = GAMMA_MONOATOMIC;
        
        RiemannState left;
        left.density = 1.0;
        left.pressure = 1.0;
        left.velocity = ZERO;
        left.sound_speed = std::sqrt(GAMMA * left.pressure / left.density);
        
        RiemannState right;
        right.density = 1.0;
        right.pressure = 1.0;
        right.velocity = left.sound_speed;  // Exactly sonic!
        right.sound_speed = std::sqrt(GAMMA * right.pressure / right.density);
        
        WHEN("Solving for interface state") {
            HLLSolver solver;
            auto solution = solver.solve(left, right);
            
            THEN("Solution should handle sonic point correctly") {
                EXPECT_TRUE(std::isfinite(solution.pressure));
                EXPECT_TRUE(std::isfinite(solution.velocity));
                EXPECT_GT(solution.pressure, ZERO);
            }
        }
    }
}

} // FEATURE(HLLRiemannSolver)
```

---

## 📋 Step-by-Step Implementation Plan

### Phase 1: Extract Riemann Solver ⭐ **HIGHEST PRIORITY**

**Goal:** Make Riemann solver independently testable

**Steps:**
1. ✅ Create `include/algorithms/riemann/riemann_solver.hpp` (interface)
2. ✅ Create `include/algorithms/riemann/hll_solver.hpp` (implementation)
3. ✅ Create `src/algorithms/riemann/hll_solver.cpp`
4. ✅ Create comprehensive BDD tests in `tests/algorithms/riemann/hll_solver_test.cpp`
5. ✅ Refactor `gsph::FluidForce` to use `RiemannSolver` interface
6. ✅ Update CMakeLists.txt to build new files
7. ✅ Verify all tests pass

**Acceptance Criteria:**
- [ ] Can test HLL solver with Sod shock tube independently
- [ ] Can test vacuum formation edge case
- [ ] Can test extreme pressure/density ratios
- [ ] FluidForce uses injected RiemannSolver

**Estimated Effort:** 4-6 hours

---

### Phase 2: Create Utilities and Constants ⭐ **HIGH PRIORITY**

**Goal:** Eliminate magic numbers

**Steps:**
1. ✅ Create `include/utilities/constants.hpp` with all named constants
2. ✅ Create `include/utilities/math_utils.hpp` with safe math operations
3. ✅ Create tests for utilities
4. ✅ Search and replace magic numbers in:
   - `include/gsph/g_fluid_force.tpp`
   - `include/gsph/g_pre_interaction.tpp`
   - `include/disph/d_fluid_force.tpp`
   - Other algorithm files
5. ✅ Verify code compiles and tests pass

**Acceptance Criteria:**
- [ ] No magic numbers (0.5, 2.0, 3.0, etc.) in algorithm code
- [ ] All constants have descriptive names
- [ ] Can grep for constant usage (e.g., `MUSCL_EXTRAPOLATION_COEFF`)

**Estimated Effort:** 2-3 hours

---

### Phase 3: Extract Slope Limiters ⭐ **HIGH PRIORITY**

**Goal:** Make limiters testable and reusable

**Steps:**
1. ✅ Create `include/algorithms/limiters/slope_limiter.hpp` (interface)
2. ✅ Create `include/algorithms/limiters/van_leer_limiter.hpp`
3. ✅ Create `src/algorithms/limiters/van_leer_limiter.cpp`
4. ✅ Create comprehensive BDD tests for Van Leer limiter
5. ✅ Optionally add MinMod, Superbee limiters
6. ✅ Refactor FluidForce to use SlopeLimiter interface
7. ✅ Verify tests pass

**Acceptance Criteria:**
- [ ] Can test limiter with same/opposite sign gradients
- [ ] Can verify TVD property
- [ ] Can swap limiters via dependency injection

**Estimated Effort:** 3-4 hours

---

### Phase 4: Refactor Kernel Hierarchy

**Goal:** Clean kernel organization

**Steps:**
1. ✅ Move `include/core/cubic_spline.hpp` to `include/algorithms/kernels/`
2. ✅ Move `include/core/wendland_kernel.hpp` to `include/algorithms/kernels/`
3. ✅ Ensure common `KernelFunction` interface
4. ✅ Create comprehensive kernel tests:
   - Normalization (∫W(r)dr = 1)
   - Partition of unity (∑W_i = 1)
   - Compact support (W = 0 beyond radius)
   - Symmetry
5. ✅ Update includes throughout codebase

**Acceptance Criteria:**
- [ ] All kernels in `algorithms/kernels/`
- [ ] Kernel properties verified by tests
- [ ] Easy to add new kernels

**Estimated Effort:** 2-3 hours

---

### Phase 5: Extract Artificial Viscosity/Conductivity

**Goal:** Make dissipation terms testable

**Steps:**
1. ✅ Create `include/algorithms/viscosity/artificial_viscosity.hpp`
2. ✅ Create `include/algorithms/viscosity/artificial_conductivity.hpp`
3. ✅ Extract from `FluidForce` protected methods
4. ✅ Create BDD tests for:
   - Converging flow (viscosity activates)
   - Diverging flow (viscosity inactive)
   - Parallel flow (no viscosity)
   - Alpha parameter sensitivity
5. ✅ Refactor FluidForce to use new classes

**Acceptance Criteria:**
- [ ] Can test viscosity formulation independently
- [ ] Can verify behavior for different flow types
- [ ] Can easily modify viscosity model

**Estimated Effort:** 3-4 hours

---

### Phase 6: Naming Conventions & Code Quality

**Goal:** Consistent, readable code

**Steps:**
1. ✅ Define naming standard:
   - Classes: `PascalCase`
   - Functions: `snake_case()`
   - Member variables: `m_snake_case`
   - Constants: `UPPER_SNAKE_CASE`
   - Local variables: `snake_case`
2. ✅ Update `.github/instructions/coding_rules.instructions.md`
3. ✅ Apply systematically across codebase (use regex search/replace)
4. ✅ Update documentation

**Acceptance Criteria:**
- [ ] Consistent naming throughout
- [ ] Code is more readable
- [ ] Follows coding_rules.instructions.md

**Estimated Effort:** 4-6 hours

---

### Phase 7: Documentation

**Goal:** Document new architecture

**Steps:**
1. ✅ Create `docs/MODULAR_ARCHITECTURE.md` explaining:
   - Algorithm separation principle
   - Interface-based design
   - Dependency injection pattern
   - How to add new algorithms
2. ✅ Create `docs/BDD_TESTING_GUIDE.md` documenting:
   - BDD approach (GIVEN/WHEN/THEN)
   - Edge case catalog
   - How to write algorithm tests
3. ✅ Update main README.md with links

**Acceptance Criteria:**
- [ ] New developers can understand architecture
- [ ] Clear guidelines for adding algorithms
- [ ] Testing approach is documented

**Estimated Effort:** 2-3 hours

---

### Phase 8: Integration & Validation

**Goal:** Ensure refactoring doesn't break existing functionality

**Steps:**
1. ✅ Run all unit tests
2. ✅ Run integration tests (full simulations)
3. ✅ Verify shock tube workflows produce identical results
4. ✅ Benchmark performance (before/after)
5. ✅ Code review
6. ✅ Merge to main branch

**Acceptance Criteria:**
- [ ] All tests pass (unit + integration)
- [ ] Simulation results unchanged (within tolerance)
- [ ] No performance regression (< 5% slowdown acceptable)

**Estimated Effort:** 2-3 hours

---

## 📊 Summary

### Total Estimated Effort
**28-35 hours** of focused development work

### Phases Breakdown
| Phase | Priority | Effort | Dependencies |
|-------|----------|--------|--------------|
| 1. Riemann Solver | ⭐⭐⭐ Critical | 4-6h | None |
| 2. Utilities/Constants | ⭐⭐⭐ Critical | 2-3h | None |
| 3. Slope Limiters | ⭐⭐⭐ Critical | 3-4h | Phase 2 |
| 4. Kernel Refactor | ⭐⭐ High | 2-3h | None |
| 5. Viscosity Extract | ⭐⭐ High | 3-4h | Phase 2 |
| 6. Naming Conventions | ⭐ Medium | 4-6h | None |
| 7. Documentation | ⭐ Medium | 2-3h | All phases |
| 8. Integration | ⭐⭐⭐ Critical | 2-3h | All phases |

### Benefits After Completion

✅ **Testability:** Every algorithm testable with comprehensive edge cases  
✅ **Modularity:** Easy to swap/add algorithms (new limiters, solvers)  
✅ **Readability:** No magic numbers, clear naming conventions  
✅ **Maintainability:** Clean separation of concerns  
✅ **Confidence:** Comprehensive BDD test coverage  
✅ **Performance:** Can benchmark individual algorithms  
✅ **Documentation:** Clear architecture for new developers

---

## 🚀 Quick Start (Implementation Order)

**Week 1:** Core Algorithm Extraction
1. **Day 1-2:** Phase 1 - Riemann Solver extraction + tests
2. **Day 2-3:** Phase 2 - Constants and utilities
3. **Day 3-4:** Phase 3 - Slope limiters + tests

**Week 2:** Refactoring & Polish
4. **Day 1:** Phase 4 - Kernel refactoring
5. **Day 2:** Phase 5 - Viscosity extraction
6. **Day 3:** Phase 6 - Naming conventions
7. **Day 4:** Phase 7 - Documentation
8. **Day 5:** Phase 8 - Integration testing

---

## ❓ Questions for User

Before proceeding with implementation, please clarify:

1. **Priority:** Should I start with Phase 1 (Riemann Solver) immediately?
2. **Scope:** Do you want all 8 phases, or focus on specific areas?
3. **Testing:** Should I create tests first (TDD) or refactor then test?
4. **Breaking Changes:** Is it OK to modify existing code (will update workflows)?
5. **Performance:** Should I benchmark current code before refactoring?

Please let me know how you'd like to proceed!

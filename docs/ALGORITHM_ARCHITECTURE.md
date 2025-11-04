# Modular Algorithm Architecture

**Design Principles and Extension Guidelines**

## Overview

The SPH codebase follows a modular, interface-based architecture where algorithms are extracted into independently testable components. This design enables:

- **Testability**: Algorithms can be tested in isolation with comprehensive BDD test suites
- **Extensibility**: New implementations can be added without modifying existing code
- **Maintainability**: Clear separation of concerns, single responsibility principle
- **Reusability**: Algorithms can be shared across different SPH variants (GSPH, DISPH, etc.)

## Architecture Principles

### 1. Interface-Based Design

Each algorithm category defines an abstract interface:

```cpp
template<int Dim>
class AlgorithmInterface {
public:
    virtual ~AlgorithmInterface() = default;
    
    // Pure virtual methods defining the contract
    virtual ResultType compute(const InputState& state) const = 0;
    virtual std::string name() const = 0;
};
```

**Benefits:**
- Polymorphic behavior through virtual dispatch
- Clear contract definition
- Easy to mock for testing
- Supports runtime algorithm selection

### 2. Header-Only Implementation

Most algorithms are header-only for template instantiation:

```cpp
// algorithm.hpp
template<int Dim>
class ConcreteAlgorithm : public AlgorithmInterface<Dim> {
public:
    ResultType compute(const InputState& state) const override {
        // Implementation inline
    }
};
```

**Advantages:**
- No link-time dependencies
- Full template instantiation visibility
- Compiler optimization opportunities
- Simplified build process

### 3. State-Based Computation

Algorithms receive state objects containing all necessary data:

```cpp
template<int Dim>
struct ComputationState {
    const Particle<Dim>& p_i;
    const Particle<Dim>& p_j;
    Vector<Dim> r_ij;
    real r;
};
```

**Rationale:**
- Explicit dependencies
- Easier to test (construct state, verify output)
- No hidden coupling to particle structure
- Future-proof for state evolution

### 4. Dependency Injection

Algorithm instances are owned by the using class via `unique_ptr`:

```cpp
template<int Dim>
class FluidForce {
private:
    std::unique_ptr<RiemannSolver> m_riemann_solver;
    std::unique_ptr<SlopeLimiter> m_slope_limiter;
    std::unique_ptr<ArtificialViscosity<Dim>> m_viscosity;
    
public:
    void initialize(Parameters params) {
        m_riemann_solver = std::make_unique<HLLSolver>();
        // ...
    }
};
```

**Benefits:**
- Clear ownership semantics
- Easy to swap implementations
- Testable via dependency injection
- RAII resource management

## Modular Components

### 1. Riemann Solvers

**Location:** `include/algorithms/riemann/`

**Interface:**
```cpp
class RiemannSolver {
public:
    virtual RiemannSolution solve(
        const RiemannState& left,
        const RiemannState& right,
        real gamma) const = 0;
};
```

**Implementations:**
- `HLLSolver` - Harten-Lax-van Leer approximate solver

**Future Implementations:**
- `HLLCSolver` - HLLC with contact wave resolution
- `ExactRiemannSolver` - Iterative exact solution
- `ROESolver` - Roe approximate solver

**Testing Strategy:**
- Sod shock tube (standard test)
- Vacuum formation
- Strong shocks (pressure ratio > 10⁶)
- Contact discontinuities
- Extreme density ratios
- Sonic points
- Symmetric states

### 2. Slope Limiters

**Location:** `include/algorithms/limiters/`

**Interface:**
```cpp
class SlopeLimiter {
public:
    virtual real limit(real dq1, real dq2) const = 0;
};
```

**Implementations:**
- `VanLeerLimiter` - φ(r) = (r + |r|) / (1 + r)

**Future Implementations:**
- `MinModLimiter` - Most dissipative, φ(r) = max(0, min(1, r))
- `SuperbeeLimiter` - Least dissipative
- `MCLimiter` - Monotonized Central limiter
- `VanAlbadaLimiter` - Smooth variant

**Testing Strategy:**
- TVD property verification
- Symmetry: φ(r) = φ(1/r) / r
- Second-order accuracy in smooth regions
- First-order at extrema
- Edge cases (zero gradients, huge ratios)
- Gradient sign changes

### 3. Artificial Viscosity

**Location:** `include/algorithms/viscosity/`

**Interface:**
```cpp
template<int Dim>
class ArtificialViscosity {
public:
    virtual real compute(const ViscosityState<Dim>& state) const = 0;
};
```

**Implementations:**
- `MonaghanViscosity<Dim>` - Standard with Balsara switch

**Future Implementations:**
- `RosswogViscosity` - Time-dependent viscosity
- `CullenDehnenViscosity` - Inviscid SPH variant
- `VonNeumannRichtmyerViscosity` - Classical approach

**Testing Strategy:**
- Approaching particles (compression)
- Receding particles (expansion)
- Balsara switch behavior (shear vs compression)
- Sonic/supersonic conditions
- Zero velocity edge cases
- 2D/3D angular dependencies

### 4. Kernel Functions

**Location:** `include/core/` (already modular)

**Interface:**
```cpp
template<int Dim>
class KernelFunction {
public:
    virtual real w(real r, real h) const = 0;
    virtual Vector<Dim> dw(const Vector<Dim>& rij, real r, real h) const = 0;
    virtual real dhw(real r, real h) const = 0;
};
```

**Implementations:**
- `Cubic<Dim>` - Cubic spline (M4)
- `C4Kernel<Dim>` - Wendland C4 (higher order)
- `Cubic25D` - Special 2.5D variant

**Notes:**
- Already follows modular architecture
- Comprehensive tests exist
- No extraction needed (Phase 4 skipped)

## Directory Structure

```
include/
├── algorithms/
│   ├── riemann/
│   │   ├── riemann_solver.hpp          # Abstract interface
│   │   ├── hll_solver.hpp              # HLL implementation
│   │   └── [future solvers]
│   ├── limiters/
│   │   ├── slope_limiter.hpp           # Abstract interface
│   │   ├── van_leer_limiter.hpp        # Van Leer implementation
│   │   └── [future limiters]
│   └── viscosity/
│       ├── artificial_viscosity.hpp    # Abstract interface
│       ├── monaghan_viscosity.hpp      # Monaghan implementation
│       └── [future viscosity schemes]
├── core/
│   ├── kernel_function.hpp             # Kernel interface
│   ├── cubic_spline.hpp                # Cubic spline
│   ├── wendland_kernel.hpp             # Wendland kernels
│   └── [other core components]
├── utilities/
│   ├── constants.hpp                   # Named constants
│   └── [utility functions]
└── [SPH variants: gsph/, disph/, etc.]

tests/
├── algorithms/
│   ├── riemann/
│   │   └── hll_solver_test.cpp         # 9 BDD scenarios
│   ├── limiters/
│   │   └── van_leer_limiter_test.cpp   # 8 BDD scenarios  
│   └── viscosity/
│       └── monaghan_viscosity_test.cpp # 6 BDD scenarios
└── core/
    └── kernel_function_test.cpp        # Kernel tests
```

## Extension Guidelines

### Adding a New Algorithm Implementation

#### Step 1: Create Interface (if new category)

```cpp
// include/algorithms/category/algorithm_interface.hpp
#pragma once

#include "../../core/sph_particle.hpp"
#include "../../defines.hpp"

namespace sph {
namespace algorithms {
namespace category {

template<int Dim>
struct InputState {
    // Define input parameters
    const SPHParticle<Dim>& particle;
    real parameter;
};

struct OutputResult {
    // Define output
    real value;
};

template<int Dim>
class AlgorithmInterface {
public:
    virtual ~AlgorithmInterface() = default;
    
    virtual OutputResult compute(const InputState<Dim>& state) const = 0;
    virtual std::string name() const = 0;
};

} // namespace category
} // namespace algorithms
} // namespace sph
```

#### Step 2: Implement Concrete Algorithm

```cpp
// include/algorithms/category/my_algorithm.hpp
#pragma once

#include "algorithm_interface.hpp"
#include "../../utilities/constants.hpp"
#include <string>

namespace sph {
namespace algorithms {
namespace category {

using namespace sph::utilities::constants;

template<int Dim>
class MyAlgorithm : public AlgorithmInterface<Dim> {
public:
    MyAlgorithm(real parameter) : m_param(parameter) {}
    
    OutputResult compute(const InputState<Dim>& state) const override {
        // Implementation
        // - Use named constants (ZERO, ONE, HALF, etc.)
        // - Document physical meaning
        // - Handle edge cases
        
        OutputResult result;
        result.value = state.parameter * m_param;
        return result;
    }
    
    std::string name() const override {
        return "My Algorithm (Author YEAR)";
    }
    
private:
    real m_param;
};

} // namespace category
} // namespace algorithms
} // namespace sph
```

#### Step 3: Write BDD Tests

```cpp
// tests/algorithms/category/my_algorithm_test.cpp
#include "../bdd_helpers.hpp"
#include "../../include/algorithms/category/my_algorithm.hpp"
#include <cmath>

using namespace sph;
using namespace sph::algorithms::category;
using namespace sph::utilities::constants;

FEATURE("My Algorithm") {
    
    SCENARIO("Basic functionality") {
        GIVEN("An algorithm instance") {
            MyAlgorithm<1> algo(2.0);
            
            THEN("Name is correct") {
                EXPECT_NE(algo.name().find("My Algorithm"), std::string::npos);
            }
        }
    }
    
    SCENARIO("Normal operation") {
        GIVEN("Standard input state") {
            SPHParticle<1> p;
            p.pos[0] = 1.0;
            
            InputState<1> state{p, 3.0};
            MyAlgorithm<1> algo(2.0);
            
            WHEN("Computing result") {
                auto result = algo.compute(state);
                
                THEN("Output is correct") {
                    EXPECT_NEAR(result.value, 6.0, 1e-10);
                }
            }
        }
    }
    
    SCENARIO("Edge cases") {
        GIVEN("Zero parameter") {
            InputState<1> state{/* ... */, ZERO};
            MyAlgorithm<1> algo(1.0);
            
            WHEN("Computing result") {
                auto result = algo.compute(state);
                
                THEN("Handles zero gracefully") {
                    EXPECT_EQ(result.value, ZERO);
                }
            }
        }
    }
    
    // Add more scenarios:
    // - Physical validity checks
    // - Extreme values
    // - Known analytical solutions
    // - Comparison with reference implementations
}
```

#### Step 4: Integrate into Simulation Module

```cpp
// include/gsph/g_fluid_force.hpp
#include "algorithms/category/my_algorithm.hpp"

template<int Dim>
class FluidForce {
private:
    std::unique_ptr<algorithms::category::AlgorithmInterface<Dim>> m_algorithm;
    
public:
    void initialize(Parameters params) {
        m_algorithm = std::make_unique<algorithms::category::MyAlgorithm<Dim>>(
            params.my_param
        );
    }
    
    void calculation(Simulation<Dim>& sim) {
        // Use algorithm
        InputState<Dim> state{particle, value};
        auto result = m_algorithm->compute(state);
        // ...
    }
};
```

#### Step 5: Update CMakeLists.txt

```cmake
# include/CMakeLists.txt
target_sources(sph_lib
  PUBLIC
    # ... existing sources ...
    ${CMAKE_CURRENT_SOURCE_DIR}/algorithms/category/algorithm_interface.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/algorithms/category/my_algorithm.hpp
)
```

#### Step 6: Document

- Add to `GSPH_ALGORITHM.md` or create category-specific doc
- Update this `ALGORITHM_ARCHITECTURE.md`
- Add usage examples
- Document physical meaning and references

### Testing Best Practices

#### BDD Structure

Use descriptive FEATURE/SCENARIO/GIVEN/WHEN/THEN:

```cpp
FEATURE("Physical Algorithm Behavior") {
    SCENARIO("Standard test case from literature") {
        GIVEN("Initial conditions from Paper (2020)") {
            // Setup
        }
        
        WHEN("Algorithm is applied") {
            // Execute
        }
        
        THEN("Results match expected values") {
            EXPECT_NEAR(computed, expected, tolerance);
        }
        
        AND_THEN("Physical constraints are satisfied") {
            EXPECT_GT(density, ZERO);  // Positive density
        }
    }
}
```

#### Coverage Goals

For each algorithm, test:

1. **Basic Functionality**
   - Name/description correct
   - Construction/initialization
   - Simple known cases

2. **Physical Validity**
   - Conservation properties
   - Positivity (where applicable)
   - Symmetry properties
   - Dimensional correctness

3. **Edge Cases**
   - Zero values
   - Extreme ratios (1e-12, 1e12)
   - Boundary conditions
   - Degenerate inputs

4. **Reference Solutions**
   - Standard test cases from literature
   - Analytical solutions
   - Cross-validation with other codes

5. **Numerical Properties**
   - Convergence rates
   - Stability conditions
   - Limiter properties (TVD, monotonicity)

#### Naming Conventions

```cpp
// Test names should be self-documenting
TEST(AlgorithmName, PropertyBeingTested_ExpectedBehavior)

// Examples:
TEST(HLLSolver, SodShockTube_CapturesesDiscontinuity)
TEST(VanLeerLimiter, OppositeGradients_ReturnsZero)
TEST(MonaghanViscosity, RecedeingParticles_NoViscosity)
```

## Design Patterns

### Factory Pattern (Future)

For runtime algorithm selection:

```cpp
class RiemannSolverFactory {
public:
    static std::unique_ptr<RiemannSolver> create(const std::string& name) {
        if (name == "hll") return std::make_unique<HLLSolver>();
        if (name == "hllc") return std::make_unique<HLLCSolver>();
        if (name == "exact") return std::make_unique<ExactSolver>();
        throw std::runtime_error("Unknown solver: " + name);
    }
};
```

### Strategy Pattern (Current)

Algorithms are interchangeable strategies:

```cpp
// Different strategies for same interface
auto conservative_limiter = std::make_unique<MinModLimiter>();
auto aggressive_limiter = std::make_unique<SuperbeeLimiter>();

// Swap at runtime
if (near_shock) {
    m_limiter = std::move(conservative_limiter);
} else {
    m_limiter = std::move(aggressive_limiter);
}
```

### Template Method Pattern

Base class defines skeleton, derived classes implement details:

```cpp
template<int Dim>
class FluidForceBase {
public:
    void calculation(Simulation<Dim>& sim) {
        find_neighbors();
        compute_gradients();
        apply_algorithm();  // Virtual, overridden by derived
        update_particles();
    }
    
protected:
    virtual void apply_algorithm() = 0;
};
```

## Code Quality Standards

### Constants

- ✅ **DO**: Use named constants from `utilities/constants.hpp`
- ❌ **DON'T**: Use magic numbers (0.5, 2.0, 3.0)

```cpp
// Good
const real delta = MUSCL_EXTRAPOLATION_COEFF * gradient;
const real rho_inv = TWO / (rho_i + rho_j);

// Bad
const real delta = 0.5 * gradient;
const real rho_inv = 2.0 / (rho_i + rho_j);
```

### Naming

- Types: `PascalCase` (classes, structs, enums)
- Functions: `snake_case`
- Variables: `snake_case`
- Constants: `SCREAMING_SNAKE_CASE` or `kCamelCase`
- Members: `m_snake_case`

### Documentation

Every algorithm should have:

```cpp
/**
 * @brief One-line description
 * 
 * Detailed explanation of algorithm, physical meaning,
 * and usage context.
 * 
 * Mathematical formulation:
 * φ(r) = f(r) for appropriate conditions
 * 
 * @tparam Dim Spatial dimension (1, 2, or 3)
 * 
 * References:
 * - Author (Year): "Title", Journal
 */
template<int Dim>
class AlgorithmImplementation {
    // ...
};
```

### Error Handling

```cpp
// Validate inputs
if (gamma <= ONE) {
    throw std::invalid_argument("Adiabatic index must be > 1");
}

// Check physical constraints
if (density < ZERO) {
    throw std::runtime_error("Negative density detected");
}

// Use asserts for programming errors
assert(neighbor_count >= 0 && "Invalid neighbor count");
```

## Performance Considerations

### Inline Small Functions

```cpp
// Mark small, frequently-called functions inline
inline real compute_simple_formula(real x) {
    return x * x + ONE;
}
```

### Avoid Virtual Calls in Inner Loops

```cpp
// Cache virtual function result if possible
auto solver_func = [solver = m_riemann_solver.get()](auto& left, auto& right) {
    return solver->solve(left, right, gamma);
};

// Use in tight loop
for (auto& pair : particle_pairs) {
    auto solution = solver_func(pair.left, pair.right);
}
```

### Use constexpr Where Possible

```cpp
constexpr real compute_coefficient(int dim) {
    return (dim == 1) ? TWO : (dim == 2) ? M_PI : FOUR * M_PI / THREE;
}
```

## Migration Path

### Refactoring Existing Code

1. **Identify** embedded algorithm (look for inline functions, lambdas)
2. **Extract** interface definition
3. **Implement** concrete class
4. **Test** thoroughly with BDD
5. **Integrate** via dependency injection
6. **Remove** old embedded code
7. **Verify** build passes
8. **Document** in relevant guides

### Backward Compatibility

During transition:
- Keep old code commented out initially
- Add deprecation warnings
- Provide migration guide
- Run validation tests

## See Also

- `GSPH_ALGORITHM.md` - GSPH physics and implementation
- `CONTRIBUTING.md` - Development workflow
- `TESTING_GUIDELINES.md` - Test-driven development practices
- `CODE_QUALITY.md` - Coding standards

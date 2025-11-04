# SPH Codebase Modularization - Quick Reference

## 🎯 Main Issues Identified

### 1. ❌ Embedded Algorithms (Cannot Test Independently)
```cpp
// BEFORE: Riemann solver is a lambda inside FluidForce
void FluidForce<Dim>::hll_solver() {
    this->m_solver = [&](const real left[], const real right[], ...) {
        // 40 lines of algorithm HERE
        // ❌ Cannot test independently!
    };
}

// AFTER: Standalone testable class
class HLLSolver : public RiemannSolver {
    RiemannSolution solve(const RiemannState& left, 
                         const RiemannState& right) const override;
    // ✅ Fully testable!
};
```

### 2. ❌ Magic Numbers Everywhere
```cpp
// BEFORE
const real delta_i = 0.5 * (1.0 - p_i.sound * dt * r_inv);  // What is 0.5?
const real v_sig = p_i.sound + p_j.sound - 3.0 * ...;       // Why 3.0?

// AFTER
const real delta_i = MUSCL_EXTRAPOLATION_COEFF * (ONE - p_i.sound * dt * r_inv);
const real v_sig = p_i.sound + p_j.sound - SIGNAL_VELOCITY_COEFF * ...;
```

### 3. ❌ God Classes Doing Too Much
```cpp
// BEFORE: FluidForce does EVERYTHING
FluidForce::calculation() {
    // 1. Gradient reconstruction
    // 2. Slope limiting  
    // 3. State extrapolation
    // 4. Riemann solving
    // 5. Flux calculation
    // 6. Artificial viscosity
    // 7. Artificial conductivity
    // ALL IN 200+ LINES!
}

// AFTER: Clean separation
FluidForce::calculation() {
    auto gradient = gradient_estimator_->compute(particles);
    auto limited = slope_limiter_->limit(gradient);
    auto flux = riemann_solver_->solve(left_state, right_state);
    auto dissipation = viscosity_->compute(particles);
    // Each component is testable!
}
```

---

## 📁 Proposed Directory Structure

```
include/
├── algorithms/              # ⭐ NEW: Testable algorithm modules
│   ├── riemann/            # Riemann solvers (HLL, HLLC, Exact)
│   ├── limiters/           # Slope limiters (VanLeer, MinMod, Superbee)
│   ├── kernels/            # SPH kernels (refactored from core/)
│   ├── reconstruction/     # MUSCL, gradient estimation
│   ├── viscosity/          # Artificial viscosity/conductivity
│   └── time_integration/   # Euler, RK2, predictor-corrector
│
├── physics/                # ⭐ NEW: Physics models
│   ├── equation_of_state.hpp
│   ├── ideal_gas_eos.hpp
│   └── sound_speed.hpp
│
├── utilities/              # ⭐ NEW: Math helpers & constants
│   ├── constants.hpp       # No more magic numbers!
│   ├── math_utils.hpp      # Safe math operations
│   └── floating_point.hpp  # Safe comparisons
│
└── core/                   # Keep as-is (simulation framework)
    ├── simulation.hpp
    └── ...
```

---

## 🧪 BDD Test Structure

```
tests/algorithms/riemann/hll_solver_test.cpp

FEATURE(HLLRiemannSolver) {
  SCENARIO(HLLSolver, SodShockTube) {
    GIVEN("Sod shock tube initial conditions") {
      // Setup states
      WHEN("Solving for interface state") {
        auto solution = solver.solve(left, right);
        THEN("Pressure should be between left and right") {
          EXPECT_GT(solution.pressure, right.pressure);
          EXPECT_LT(solution.pressure, left.pressure);
        }
      }
    }
  }
  
  SCENARIO(HLLSolver, VacuumFormation) { ... }
  SCENARIO(HLLSolver, StrongShock) { ... }
  SCENARIO(HLLSolver, ExtremeDensityRatio) { ... }
}
```

**Edge Cases to Test:**
- ✅ Sod shock tube (classic test)
- ✅ Vacuum formation (states moving apart)
- ✅ Strong shocks (pressure ratio 1e6)
- ✅ Contact discontinuities (density jump, no pressure jump)
- ✅ Extreme density ratios (1e-6 to 1e6)
- ✅ Sonic points (transonic flow)

---

## 📋 Implementation Phases (Priority Order)

### Phase 1: Extract Riemann Solver ⭐⭐⭐ CRITICAL
**Effort:** 4-6 hours  
**Why First:** Most embedded, most complex, highest impact

**Deliverables:**
- [ ] `algorithms/riemann/riemann_solver.hpp` (interface)
- [ ] `algorithms/riemann/hll_solver.hpp` (HLL implementation)
- [ ] Comprehensive BDD tests (6+ edge cases)
- [ ] Refactor `gsph::FluidForce` to use interface

---

### Phase 2: Create Utilities & Constants ⭐⭐⭐ CRITICAL
**Effort:** 2-3 hours  
**Why Second:** Needed by all other phases

**Deliverables:**
- [ ] `utilities/constants.hpp` - All named constants
- [ ] `utilities/math_utils.hpp` - Safe operations
- [ ] Replace ALL magic numbers in codebase

**Constants to Define:**
```cpp
// Mathematical
ZERO, ONE, TWO, THREE, HALF, ONE_THIRD

// Physical  
GAMMA_MONOATOMIC, GAMMA_DIATOMIC

// Geometric
UNIT_SPHERE_VOLUME_1D/2D/3D

// Algorithm
MUSCL_EXTRAPOLATION_COEFF
SIGNAL_VELOCITY_COEFF
BHTREE_SIZE_MULTIPLIER
```

---

### Phase 3: Extract Slope Limiters ⭐⭐⭐ CRITICAL
**Effort:** 3-4 hours  
**Why Third:** Builds on constants, needed by reconstruction

**Deliverables:**
- [ ] `algorithms/limiters/slope_limiter.hpp` (interface)
- [ ] `algorithms/limiters/van_leer_limiter.hpp`
- [ ] BDD tests for TVD property, edge cases
- [ ] Optionally: MinMod, Superbee limiters

---

### Phase 4: Refactor Kernels ⭐⭐ HIGH
**Effort:** 2-3 hours

**Deliverables:**
- [ ] Move kernels from `core/` to `algorithms/kernels/`
- [ ] Test kernel properties (normalization, partition of unity)

---

### Phase 5: Extract Viscosity ⭐⭐ HIGH
**Effort:** 3-4 hours

**Deliverables:**
- [ ] `algorithms/viscosity/artificial_viscosity.hpp`
- [ ] Tests for converging/diverging/parallel flows

---

### Phase 6: Naming Conventions ⭐ MEDIUM
**Effort:** 4-6 hours

**Standards:**
- Classes: `PascalCase`
- Functions: `snake_case()`
- Members: `m_snake_case`
- Constants: `UPPER_SNAKE_CASE`
- Locals: `snake_case`

---

### Phase 7: Documentation ⭐ MEDIUM
**Effort:** 2-3 hours

**Deliverables:**
- [ ] `docs/MODULAR_ARCHITECTURE.md`
- [ ] `docs/BDD_TESTING_GUIDE.md`

---

### Phase 8: Integration & Validation ⭐⭐⭐ CRITICAL
**Effort:** 2-3 hours

**Checklist:**
- [ ] All unit tests pass
- [ ] Shock tube workflows produce identical results
- [ ] No performance regression (< 5%)

---

## 📊 Total Effort Estimate

**28-35 hours** across 8 phases

---

## ✅ Benefits After Completion

| Aspect | Before | After |
|--------|--------|-------|
| **Testability** | ❌ Embedded algorithms | ✅ Every algorithm has BDD tests |
| **Readability** | ❌ Magic numbers everywhere | ✅ Named constants |
| **Modularity** | ❌ God classes | ✅ Single responsibility |
| **Maintainability** | ❌ Tight coupling | ✅ Dependency injection |
| **Extensibility** | ❌ Hard to add variants | ✅ Interface-based design |
| **Confidence** | ❌ Limited edge case coverage | ✅ Comprehensive test suite |

---

## 🚀 Quick Start Commands

```bash
# 1. Review the detailed plan
cat docs/ARCHITECTURE_REVIEW_AND_RECOMMENDATIONS.md

# 2. Create feature branch
git checkout -b refactor/modular-architecture

# 3. Start with Phase 1 (Riemann solver)
mkdir -p include/algorithms/riemann
mkdir -p src/algorithms/riemann
mkdir -p tests/algorithms/riemann

# 4. Implement, test, commit each phase
git add .
git commit -m "Phase 1: Extract HLL Riemann solver"

# 5. After all phases, validate
make test
make full  # Run shock tube workflows
```

---

## 📖 Key Design Principles

### 1. **Single Responsibility Principle**
Each class/function does ONE thing well.

### 2. **Dependency Injection**
Inject algorithms via interfaces, not hardcode.

### 3. **Interface Segregation**
Small, focused interfaces (RiemannSolver, SlopeLimiter).

### 4. **Open/Closed Principle**
Open for extension (new solvers), closed for modification.

### 5. **Test-Driven Development (BDD)**
Write edge case tests FIRST, then implement.

---

## 📞 Next Steps

Please review:
1. `docs/ARCHITECTURE_REVIEW_AND_RECOMMENDATIONS.md` (detailed plan)
2. This quick reference

Then let me know:
- **Which phase to start with?** (Recommend: Phase 1)
- **Should I create the files and implement?**
- **Any questions or modifications to the plan?**

Ready to begin implementation when you are! 🚀

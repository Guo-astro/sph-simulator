# Algorithm-Specific Parameter Builder Design

## Problem Statement

The current `SPHParametersBuilder` allows setting parameters that may not be used or may be incompatible with the chosen SPH algorithm:

```cpp
// Current: Allows setting artificial viscosity for GSPH (but GSPH doesn't use it!)
auto params = SPHParametersBuilder()
    .with_sph_type("gsph")
    .with_artificial_viscosity(1.0, true, false)  // ❌ Misleading - GSPH ignores this!
    .build();
```

**Issues:**
1. **No compile-time safety**: Can set incompatible parameters
2. **Misleading API**: Users think they're configuring behavior that's ignored
3. **No validation**: Build succeeds even with nonsensical combinations
4. **Poor discoverability**: Users don't know which parameters apply to which algorithms

---

## Declarative Design: Type-Safe Algorithm Builders

### Architecture: Phantom Type Pattern

Use **phantom types** to encode algorithm-specific constraints at compile time:

```cpp
// Phantom types - zero runtime cost, compile-time type safety
struct SSPHTag {};
struct DISPHTag {};
struct GSPHTag {};

template<typename AlgorithmTag>
class AlgorithmParametersBuilder;
```

### Core Builder Interface

```cpp
/**
 * @brief Base parameter builder - algorithm-agnostic settings
 * 
 * This builder handles parameters common to ALL SPH algorithms.
 * After setting common parameters, call .as_ssph(), .as_disph(), 
 * or .as_gsph() to get an algorithm-specific builder.
 */
class SPHParametersBuilderBase {
private:
    std::shared_ptr<SPHParameters> params;
    
    bool has_time = false;
    bool has_cfl = false;
    bool has_physics = false;
    bool has_kernel = false;

public:
    SPHParametersBuilderBase();
    
    // ========================================
    // Common Parameters (All Algorithms)
    // ========================================
    
    SPHParametersBuilderBase& with_time(real start, real end, real output, real energy = -1.0);
    SPHParametersBuilderBase& with_cfl(real sound, real force);
    SPHParametersBuilderBase& with_physics(int neighbor_number, real gamma);
    SPHParametersBuilderBase& with_kernel(const std::string& kernel_name);
    SPHParametersBuilderBase& with_tree_params(int max_level = 20, int leaf_particle_num = 1);
    SPHParametersBuilderBase& with_iterative_smoothing_length(bool enable = true);
    
    // Boundary conditions (all algorithms)
    SPHParametersBuilderBase& with_periodic_boundary(
        const std::array<real, 3>& range_min,
        const std::array<real, 3>& range_max
    );
    
    // Gravity (all algorithms)
    SPHParametersBuilderBase& with_gravity(real constant, real theta = 0.5);
    
    // ========================================
    // Algorithm Selection (Type Transition)
    // ========================================
    
    /**
     * @brief Transition to Standard SPH builder
     * @return SSPH-specific builder with artificial viscosity support
     */
    AlgorithmParametersBuilder<SSPHTag> as_ssph();
    
    /**
     * @brief Transition to Density Independent SPH builder
     * @return DISPH-specific builder with artificial viscosity support
     */
    AlgorithmParametersBuilder<DISPHTag> as_disph();
    
    /**
     * @brief Transition to Godunov SPH builder
     * @return GSPH-specific builder with Riemann solver options
     */
    AlgorithmParametersBuilder<GSPHTag> as_gsph();
    
    // ========================================
    // Validation & Helpers
    // ========================================
    
    bool is_complete_base() const;
    std::string get_missing_base_parameters() const;
    
    // Allow algorithm builders to access internal state
    template<typename AlgorithmTag>
    friend class AlgorithmParametersBuilder;
};
```

---

### Algorithm-Specific Builders

#### 1. Standard SPH Builder

```cpp
/**
 * @brief Standard SPH parameter builder
 * 
 * SSPH uses:
 * - ✅ Artificial viscosity (Monaghan 1997)
 * - ✅ Artificial conductivity (optional)
 * - ❌ NO Riemann solver
 * - ❌ NO MUSCL reconstruction
 */
template<>
class AlgorithmParametersBuilder<SSPHTag> {
private:
    std::shared_ptr<SPHParameters> params;
    bool has_artificial_viscosity = false;

public:
    explicit AlgorithmParametersBuilder(std::shared_ptr<SPHParameters> base_params);
    
    // ========================================
    // SSPH-Specific Parameters
    // ========================================
    
    /**
     * @brief Configure artificial viscosity (REQUIRED for SSPH)
     * 
     * Standard Monaghan (1997) artificial viscosity:
     *   π_ij = -α v_sig w_ij / (2ρ_ij)
     * 
     * @param alpha Viscosity coefficient (typical: 0.5-2.0)
     * @param use_balsara_switch Reduce viscosity in shear (Morris & Monaghan 1997)
     * @param use_time_dependent Dynamic α adjustment (Rosswog 2015)
     * @param alpha_max Maximum α (if time-dependent)
     * @param alpha_min Minimum α (if time-dependent)
     * @param epsilon Time constant: τ = h/(ε·c)
     */
    AlgorithmParametersBuilder& with_artificial_viscosity(
        real alpha,
        bool use_balsara_switch = true,
        bool use_time_dependent = false,
        real alpha_max = 2.0,
        real alpha_min = 0.1,
        real epsilon = 0.2
    );
    
    /**
     * @brief Configure artificial conductivity (OPTIONAL)
     * 
     * Thermal energy dissipation for shock stabilization:
     *   κ_ij = α_AC (c_i + c_j) / 2
     * 
     * @param alpha Conductivity coefficient (typical: 0.1-1.0)
     */
    AlgorithmParametersBuilder& with_artificial_conductivity(real alpha);
    
    // ========================================
    // Build
    // ========================================
    
    /**
     * @brief Build final SPHParameters
     * @throws std::runtime_error if artificial viscosity not set
     */
    std::shared_ptr<SPHParameters> build();
    
    bool is_complete() const;
    std::string get_missing_parameters() const;
};
```

---

#### 2. DISPH Builder

```cpp
/**
 * @brief Density Independent SPH parameter builder
 * 
 * DISPH uses:
 * - ✅ Artificial viscosity (same as SSPH)
 * - ✅ Artificial conductivity (optional)
 * - ✅ Modified pressure force formulation
 * - ❌ NO Riemann solver
 */
template<>
class AlgorithmParametersBuilder<DISPHTag> {
private:
    std::shared_ptr<SPHParameters> params;
    bool has_artificial_viscosity = false;

public:
    explicit AlgorithmParametersBuilder(std::shared_ptr<SPHParameters> base_params);
    
    // Same artificial viscosity API as SSPH
    AlgorithmParametersBuilder& with_artificial_viscosity(
        real alpha,
        bool use_balsara_switch = true,
        bool use_time_dependent = false,
        real alpha_max = 2.0,
        real alpha_min = 0.1,
        real epsilon = 0.2
    );
    
    AlgorithmParametersBuilder& with_artificial_conductivity(real alpha);
    
    std::shared_ptr<SPHParameters> build();
    bool is_complete() const;
    std::string get_missing_parameters() const;
};
```

---

#### 3. GSPH Builder

```cpp
/**
 * @brief Godunov SPH parameter builder
 * 
 * GSPH uses:
 * - ✅ Riemann solver (HLL, HLLC, Exact)
 * - ✅ MUSCL reconstruction (optional 2nd order)
 * - ✅ Slope limiters (Van Leer, MinMod, Superbee)
 * - ❌ NO artificial viscosity (Riemann solver provides dissipation)
 * - ❌ NO artificial conductivity
 * 
 * Key insight: GSPH's Riemann solver replaces artificial viscosity
 * with physics-based shock capturing!
 */
template<>
class AlgorithmParametersBuilder<GSPHTag> {
private:
    std::shared_ptr<SPHParameters> params;
    bool has_riemann_solver = false;

public:
    explicit AlgorithmParametersBuilder(std::shared_ptr<SPHParameters> base_params);
    
    // ========================================
    // GSPH-Specific Parameters
    // ========================================
    
    /**
     * @brief Enable 2nd order MUSCL reconstruction (OPTIONAL)
     * 
     * When enabled:
     * - Uses Van Leer slope limiter
     * - Reconstructs left/right states at interfaces
     * - Higher accuracy but ~1.5x slower
     * 
     * When disabled:
     * - First-order Godunov scheme
     * - Direct particle states used
     * - Faster but more diffusive
     * 
     * @param enable Enable MUSCL-Hancock scheme
     */
    AlgorithmParametersBuilder& with_2nd_order_muscl(bool enable = true);
    
    /**
     * @brief Select Riemann solver type (OPTIONAL - defaults to HLL)
     * 
     * Available solvers:
     * - "hll":   Fast, robust, diffusive at contacts (DEFAULT)
     * - "hllc":  Restores contact discontinuity, more accurate
     * - "exact": Iterative exact solver, slow but most accurate
     * 
     * @param solver_name Solver type
     */
    AlgorithmParametersBuilder& with_riemann_solver(const std::string& solver_name = "hll");
    
    /**
     * @brief Select slope limiter for MUSCL (OPTIONAL - defaults to Van Leer)
     * 
     * Available limiters (by diffusivity):
     * - "minmod":   Most diffusive, most robust
     * - "van_leer": Moderate diffusivity (DEFAULT)
     * - "superbee": Least diffusive, can overshoot
     * - "mc":       Monotonized Central
     * 
     * Only used if 2nd order MUSCL is enabled.
     * 
     * @param limiter_name Limiter type
     */
    AlgorithmParametersBuilder& with_slope_limiter(const std::string& limiter_name = "van_leer");
    
    // ========================================
    // Build
    // ========================================
    
    std::shared_ptr<SPHParameters> build();
    bool is_complete() const;
    std::string get_missing_parameters() const;
};
```

---

## Usage Examples

### Example 1: Standard SPH with Artificial Viscosity

```cpp
#include "core/sph_parameters_builder.hpp"

auto params = SPHParametersBuilderBase()
    // Common parameters
    .with_time(0.0, 0.2, 0.01)
    .with_cfl(0.3, 0.125)
    .with_physics(50, 1.4)
    .with_kernel("cubic_spline")
    .with_tree_params(20, 1)
    .with_iterative_smoothing_length(true)
    
    // Transition to SSPH
    .as_ssph()
    
    // SSPH-specific: Artificial viscosity (REQUIRED)
    .with_artificial_viscosity(
        1.0,    // alpha
        true,   // Balsara switch
        false   // time-dependent
    )
    
    // SSPH-specific: Artificial conductivity (OPTIONAL)
    .with_artificial_conductivity(1.0)
    
    .build();

// ✅ Compile-time safety: Cannot call .with_2nd_order_muscl() on SSPH!
// ✅ Runtime validation: Build fails if artificial viscosity not set
```

---

### Example 2: GSPH with Riemann Solver (NO Artificial Viscosity)

```cpp
auto params = SPHParametersBuilderBase()
    // Common parameters
    .with_time(0.0, 0.2, 0.01)
    .with_cfl(0.3, 0.125)
    .with_physics(50, 1.4)
    .with_kernel("cubic_spline")
    
    // Transition to GSPH
    .as_gsph()
    
    // GSPH-specific: MUSCL reconstruction (OPTIONAL)
    .with_2nd_order_muscl(false)  // Use 1st order for speed
    
    // GSPH-specific: Riemann solver (OPTIONAL - defaults to HLL)
    .with_riemann_solver("hll")
    
    .build();

// ✅ Cannot call .with_artificial_viscosity() on GSPH!
// ✅ Parameters are self-documenting
// ✅ No misleading configuration
```

---

### Example 3: GSPH with 2nd Order Accuracy

```cpp
auto params = SPHParametersBuilderBase()
    .with_time(0.0, 0.2, 0.01)
    .with_cfl(0.25, 0.1)  // Tighter CFL for 2nd order
    .with_physics(50, 1.4)
    .with_kernel("cubic_spline")
    
    .as_gsph()
    
    // Enable MUSCL-Hancock scheme
    .with_2nd_order_muscl(true)
    
    // Choose slope limiter
    .with_slope_limiter("van_leer")  // Smooth, balanced
    
    // Choose Riemann solver
    .with_riemann_solver("hllc")  // More accurate than HLL
    
    .build();
```

---

### Example 4: DISPH

```cpp
auto params = SPHParametersBuilderBase()
    .with_time(0.0, 1.0, 0.1)
    .with_cfl(0.3, 0.125)
    .with_physics(50, 1.4)
    .with_kernel("wendland")  // C4 kernel
    
    .as_disph()
    
    // DISPH uses same viscosity as SSPH
    .with_artificial_viscosity(1.0, true, false)
    
    .build();
```

---

## Validation & Error Messages

### Compile-Time Safety

```cpp
auto builder = SPHParametersBuilderBase()
    .with_time(0.0, 0.2, 0.01)
    .as_gsph();

// ❌ Compile error: 'AlgorithmParametersBuilder<GSPHTag>' has no member 'with_artificial_viscosity'
builder.with_artificial_viscosity(1.0, true, false);

// ✅ OK: GSPH-specific method available
builder.with_2nd_order_muscl(true);
```

### Runtime Validation

```cpp
try {
    auto params = SPHParametersBuilderBase()
        .with_time(0.0, 0.2, 0.01)
        .as_ssph()
        // Missing: .with_artificial_viscosity()
        .build();
} catch (const std::runtime_error& e) {
    // Error: "SSPH requires artificial viscosity configuration. 
    //         Call with_artificial_viscosity() before build()."
}
```

---

## Implementation Strategy

### Phase 1: Add Algorithm-Specific Builders (Non-Breaking)

1. Create new builders alongside existing `SPHParametersBuilder`
2. Both systems coexist during transition
3. Update documentation to recommend new API

**Files to create:**
```
include/core/
├── sph_parameters_builder_base.hpp
├── ssph_parameters_builder.hpp
├── disph_parameters_builder.hpp
└── gsph_parameters_builder.hpp
```

### Phase 2: Update Plugins

Migrate example plugins to use new API:
```cpp
// Old (still works)
auto params = SPHParametersBuilder()
    .with_sph_type("gsph")
    .with_artificial_viscosity(1.0, true, false)  // Misleading!
    .build();

// New (declarative, type-safe)
auto params = SPHParametersBuilderBase()
    .with_time(...)
    .as_gsph()
    .with_2nd_order_muscl(false)
    .build();
```

### Phase 3: Deprecate Old API

1. Add deprecation warnings to `SPHParametersBuilder`
2. Update all workflows to use new API
3. Remove old builder in next major version

---

## Benefits

### 1. Type Safety
```cpp
// ❌ Compile error - cannot mix incompatible parameters
auto params = builder.as_gsph()
    .with_artificial_viscosity(1.0, true, false);  // Compile error!
```

### 2. Self-Documenting Code
```cpp
// Old: Unclear what parameters apply
.with_sph_type("gsph")
.with_artificial_viscosity(...)  // Does GSPH use this? Who knows!

// New: Crystal clear
.as_gsph()                      // GSPH selected
.with_2nd_order_muscl(true)     // Only GSPH-specific methods available
```

### 3. Better Error Messages
```
Old: "Build failed: missing parameters"
New: "GSPH builder incomplete: Please set 2nd order mode with .with_2nd_order_muscl()"
```

### 4. Discoverability
```cpp
auto gsph_builder = base.as_gsph();
// IDE autocomplete shows ONLY GSPH-relevant methods:
//   - with_2nd_order_muscl()
//   - with_riemann_solver()
//   - with_slope_limiter()
// 
// Does NOT show:
//   - with_artificial_viscosity()  ❌
```

---

## Alternative Design: Validation-Based (Simpler)

If phantom types are too complex, use **runtime validation** instead:

```cpp
class SPHParametersBuilder {
public:
    SPHParametersBuilder& with_sph_type(const std::string& type_name);
    
    SPHParametersBuilder& with_artificial_viscosity(...) {
        // Validate at call time
        if (params->type == SPHType::GSPH) {
            throw std::runtime_error(
                "GSPH does not use artificial viscosity. "
                "Use .with_2nd_order_muscl() instead."
            );
        }
        // ... set params
        return *this;
    }
    
    SPHParametersBuilder& with_2nd_order_muscl(bool enable) {
        // Validate at call time
        if (params->type != SPHType::GSPH) {
            throw std::runtime_error(
                "MUSCL reconstruction only available for GSPH. "
                "Selected algorithm: " + to_string(params->type)
            );
        }
        params->gsph.is_2nd_order = enable;
        return *this;
    }
};
```

**Pros:**
- Simpler implementation
- No template metaprogramming
- Still provides clear error messages

**Cons:**
- Runtime errors instead of compile-time
- Requires setting `sph_type` first
- Less discoverable (all methods visible in IDE)

---

## Recommendation

**Use Phantom Type Pattern** for:
- ✅ Compile-time safety
- ✅ Better IDE support
- ✅ Self-documenting API
- ✅ Future extensibility (easy to add new algorithms)

**Fallback to Validation-Based** if:
- Team unfamiliar with template metaprogramming
- Rapid prototyping phase
- Backward compatibility critical

---

## File Structure

```
include/core/
├── sph_parameters_builder.hpp          # Legacy (deprecated)
├── sph_parameters_builder_base.hpp     # Common parameters
├── algorithm_parameters_builder.hpp    # Template declaration
├── ssph_parameters_builder.hpp         # SSPH specialization
├── disph_parameters_builder.hpp        # DISPH specialization
└── gsph_parameters_builder.hpp         # GSPH specialization

src/core/
├── sph_parameters_builder_base.cpp
├── ssph_parameters_builder.cpp
├── disph_parameters_builder.cpp
└── gsph_parameters_builder.cpp

tests/core/
├── ssph_parameters_builder_test.cpp
├── disph_parameters_builder_test.cpp
└── gsph_parameters_builder_test.cpp
```

---

## Summary

This design provides:
1. **Type safety** - Compile-time prevention of invalid parameter combinations
2. **Clarity** - API self-documents which parameters apply to which algorithms
3. **Validation** - Runtime checks for required parameters per algorithm
4. **Extensibility** - Easy to add new algorithms with their own constraints
5. **Backward compatibility** - Can coexist with legacy builder during migration

The key insight: **Different SPH algorithms have fundamentally different parameter requirements**. The builder API should reflect this at the type level!

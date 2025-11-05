# Algorithm-Specific Parameter Builder - Implementation Example

## Quick Reference: Current vs. Proposed

### Current API (Misleading)
```cpp
// Problem: Can set parameters that GSPH ignores!
auto params = SPHParametersBuilder()
    .with_time(0.0, 0.2, 0.01)
    .with_sph_type("gsph")
    .with_cfl(0.3, 0.125)
    .with_physics(50, 1.4)
    .with_kernel("cubic_spline")
    
    // ❌ MISLEADING: GSPH doesn't use artificial viscosity!
    .with_artificial_viscosity(1.0, true, false)
    
    .build();
```

### Proposed API (Type-Safe)
```cpp
// ✅ Clear: Only GSPH-relevant parameters available
auto params = SPHParametersBuilderBase()
    .with_time(0.0, 0.2, 0.01)
    .with_cfl(0.3, 0.125)
    .with_physics(50, 1.4)
    .with_kernel("cubic_spline")
    
    // Type transition
    .as_gsph()
    
    // GSPH-specific parameters only
    .with_2nd_order_muscl(false)
    .with_riemann_solver("hll")
    
    .build();
```

---

## Minimal Implementation

### 1. Base Builder (Common Parameters)

```cpp
// include/core/sph_parameters_builder_base.hpp
#pragma once

#include "parameters.hpp"
#include <memory>
#include <string>

namespace sph {

// Forward declarations for algorithm-specific builders
template<typename AlgorithmTag> class AlgorithmParametersBuilder;

struct SSPHTag {};
struct DISPHTag {};
struct GSPHTag {};

/**
 * @brief Base builder for algorithm-agnostic SPH parameters
 * 
 * Sets common parameters, then transitions to algorithm-specific builder.
 */
class SPHParametersBuilderBase {
private:
    std::shared_ptr<SPHParameters> params_;
    bool has_time_ = false;
    bool has_cfl_ = false;
    bool has_physics_ = false;
    bool has_kernel_ = false;

protected:
    // Allow algorithm builders to access state
    std::shared_ptr<SPHParameters> get_params() { return params_; }
    bool all_base_params_set() const {
        return has_time_ && has_cfl_ && has_physics_ && has_kernel_;
    }

public:
    SPHParametersBuilderBase();

    // Common parameters
    SPHParametersBuilderBase& with_time(real start, real end, real output, real energy = -1.0);
    SPHParametersBuilderBase& with_cfl(real sound, real force);
    SPHParametersBuilderBase& with_physics(int neighbor_number, real gamma);
    SPHParametersBuilderBase& with_kernel(const std::string& kernel_name);
    SPHParametersBuilderBase& with_tree_params(int max_level = 20, int leaf_particle_num = 1);
    SPHParametersBuilderBase& with_iterative_smoothing_length(bool enable = true);
    
    // Algorithm transitions
    AlgorithmParametersBuilder<SSPHTag> as_ssph();
    AlgorithmParametersBuilder<DISPHTag> as_disph();
    AlgorithmParametersBuilder<GSPHTag> as_gsph();
    
    std::string get_missing_base_parameters() const;
};

} // namespace sph
```

---

### 2. SSPH Builder (With Artificial Viscosity)

```cpp
// include/core/ssph_parameters_builder.hpp
#pragma once

#include "sph_parameters_builder_base.hpp"

namespace sph {

/**
 * @brief Standard SPH parameter builder
 * 
 * SSPH uses artificial viscosity for shock capturing.
 */
template<>
class AlgorithmParametersBuilder<SSPHTag> {
private:
    std::shared_ptr<SPHParameters> params_;
    bool has_artificial_viscosity_ = false;

public:
    explicit AlgorithmParametersBuilder(std::shared_ptr<SPHParameters> base_params)
        : params_(base_params) 
    {
        params_->type = SPHType::SSPH;
    }

    /**
     * @brief Configure artificial viscosity (REQUIRED for SSPH)
     */
    AlgorithmParametersBuilder& with_artificial_viscosity(
        real alpha,
        bool use_balsara_switch = true,
        bool use_time_dependent = false,
        real alpha_max = 2.0,
        real alpha_min = 0.1,
        real epsilon = 0.2
    ) {
        params_->av.alpha = alpha;
        params_->av.use_balsara_switch = use_balsara_switch;
        params_->av.use_time_dependent_av = use_time_dependent;
        params_->av.alpha_max = alpha_max;
        params_->av.alpha_min = alpha_min;
        params_->av.epsilon = epsilon;
        has_artificial_viscosity_ = true;
        return *this;
    }

    /**
     * @brief Configure artificial conductivity (OPTIONAL)
     */
    AlgorithmParametersBuilder& with_artificial_conductivity(real alpha) {
        params_->ac.is_valid = true;
        params_->ac.alpha = alpha;
        return *this;
    }

    /**
     * @brief Build final parameters
     * @throws std::runtime_error if artificial viscosity not configured
     */
    std::shared_ptr<SPHParameters> build() {
        if (!has_artificial_viscosity_) {
            throw std::runtime_error(
                "SSPH requires artificial viscosity configuration.\n"
                "Call .with_artificial_viscosity(alpha, ...) before build().\n"
                "Typical values: alpha=1.0, use_balsara_switch=true"
            );
        }
        return params_;
    }

    bool is_complete() const {
        return has_artificial_viscosity_;
    }

    std::string get_missing_parameters() const {
        if (!has_artificial_viscosity_) {
            return "Missing: artificial_viscosity (required for SSPH)";
        }
        return "";
    }
};

} // namespace sph
```

---

### 3. GSPH Builder (With Riemann Solver, NO Viscosity)

```cpp
// include/core/gsph_parameters_builder.hpp
#pragma once

#include "sph_parameters_builder_base.hpp"

namespace sph {

/**
 * @brief Godunov SPH parameter builder
 * 
 * GSPH uses Riemann solver for shock capturing.
 * Does NOT use artificial viscosity (Riemann solver provides physics-based dissipation).
 */
template<>
class AlgorithmParametersBuilder<GSPHTag> {
private:
    std::shared_ptr<SPHParameters> params_;

public:
    explicit AlgorithmParametersBuilder(std::shared_ptr<SPHParameters> base_params)
        : params_(base_params) 
    {
        params_->type = SPHType::GSPH;
        
        // Set GSPH defaults
        params_->gsph.is_2nd_order = false;  // Default to 1st order (faster)
    }

    /**
     * @brief Enable 2nd order MUSCL reconstruction (OPTIONAL)
     * 
     * When true:
     * - Uses MUSCL-Hancock scheme
     * - Van Leer slope limiter
     * - Higher accuracy, ~1.5x slower
     * 
     * When false:
     * - 1st order Godunov
     * - Faster, more diffusive
     * 
     * @param enable Enable 2nd order
     */
    AlgorithmParametersBuilder& with_2nd_order_muscl(bool enable = true) {
        params_->gsph.is_2nd_order = enable;
        return *this;
    }

    /**
     * @brief Select Riemann solver (OPTIONAL - defaults to HLL)
     * 
     * Available solvers:
     * - "hll":   Fast, robust (DEFAULT)
     * - "hllc":  More accurate at contacts
     * - "exact": Most accurate, expensive
     * 
     * @param solver_name Solver type
     */
    AlgorithmParametersBuilder& with_riemann_solver(const std::string& solver_name = "hll") {
        // For now, only HLL is implemented
        // Future: Add solver selection to SPHParameters
        if (solver_name != "hll") {
            throw std::runtime_error(
                "Only 'hll' Riemann solver currently implemented.\n"
                "Future versions will support: hllc, exact"
            );
        }
        return *this;
    }

    /**
     * @brief Select slope limiter (OPTIONAL - defaults to Van Leer)
     * 
     * Only used if 2nd order MUSCL is enabled.
     * 
     * @param limiter_name Limiter type
     */
    AlgorithmParametersBuilder& with_slope_limiter(const std::string& limiter_name = "van_leer") {
        // For now, only Van Leer is implemented
        if (limiter_name != "van_leer") {
            throw std::runtime_error(
                "Only 'van_leer' slope limiter currently implemented.\n"
                "Future versions will support: minmod, superbee, mc"
            );
        }
        return *this;
    }

    /**
     * @brief Build final parameters
     * 
     * GSPH has no required algorithm-specific parameters
     * (Riemann solver and MUSCL have sensible defaults).
     */
    std::shared_ptr<SPHParameters> build() {
        return params_;
    }

    bool is_complete() const {
        return true;  // No required GSPH-specific params
    }

    std::string get_missing_parameters() const {
        return "";  // All GSPH params are optional
    }
};

} // namespace sph
```

---

### 4. DISPH Builder

```cpp
// include/core/disph_parameters_builder.hpp
#pragma once

#include "sph_parameters_builder_base.hpp"

namespace sph {

/**
 * @brief Density Independent SPH parameter builder
 * 
 * DISPH uses artificial viscosity (same as SSPH).
 */
template<>
class AlgorithmParametersBuilder<DISPHTag> {
private:
    std::shared_ptr<SPHParameters> params_;
    bool has_artificial_viscosity_ = false;

public:
    explicit AlgorithmParametersBuilder(std::shared_ptr<SPHParameters> base_params)
        : params_(base_params) 
    {
        params_->type = SPHType::DISPH;
    }

    // Same artificial viscosity API as SSPH
    AlgorithmParametersBuilder& with_artificial_viscosity(
        real alpha,
        bool use_balsara_switch = true,
        bool use_time_dependent = false,
        real alpha_max = 2.0,
        real alpha_min = 0.1,
        real epsilon = 0.2
    ) {
        params_->av.alpha = alpha;
        params_->av.use_balsara_switch = use_balsara_switch;
        params_->av.use_time_dependent_av = use_time_dependent;
        params_->av.alpha_max = alpha_max;
        params_->av.alpha_min = alpha_min;
        params_->av.epsilon = epsilon;
        has_artificial_viscosity_ = true;
        return *this;
    }

    AlgorithmParametersBuilder& with_artificial_conductivity(real alpha) {
        params_->ac.is_valid = true;
        params_->ac.alpha = alpha;
        return *this;
    }

    std::shared_ptr<SPHParameters> build() {
        if (!has_artificial_viscosity_) {
            throw std::runtime_error(
                "DISPH requires artificial viscosity configuration.\n"
                "Call .with_artificial_viscosity(alpha, ...) before build()."
            );
        }
        return params_;
    }

    bool is_complete() const {
        return has_artificial_viscosity_;
    }

    std::string get_missing_parameters() const {
        if (!has_artificial_viscosity_) {
            return "Missing: artificial_viscosity (required for DISPH)";
        }
        return "";
    }
};

} // namespace sph
```

---

## Real-World Usage Examples

### Example 1: Shock Tube with GSPH

```cpp
// workflows/shock_tube_workflow/01_simulation/src/plugin_enhanced.cpp

void ShockTubePluginEnhanced::initialize(
    std::shared_ptr<Simulation<1>> sim,
    std::shared_ptr<SPHParameters> params
) {
    // ... initialize particles ...
    
    if (params->time.end == 0) {
        // Build parameters with new type-safe API
        auto builder_params = SPHParametersBuilderBase()
            // Common parameters
            .with_time(0.0, 0.15, 0.01, 0.01)
            .with_cfl(0.3, 0.125)
            .with_physics(50, 1.4)
            .with_kernel("cubic_spline")
            .with_tree_params(20, 1)
            .with_iterative_smoothing_length(true)
            
            // Transition to GSPH
            .as_gsph()
            
            // GSPH-specific: No artificial viscosity!
            .with_2nd_order_muscl(false)  // 1st order for speed
            .with_riemann_solver("hll")   // HLL Riemann solver
            
            .build();
        
        *params = *builder_params;
    }
    
    // ... rest of initialization ...
}
```

### Example 2: Dam Break with SSPH

```cpp
void DamBreakPlugin::initialize(
    std::shared_ptr<Simulation<2>> sim,
    std::shared_ptr<SPHParameters> params
) {
    // ... initialize particles ...
    
    auto builder_params = SPHParametersBuilderBase()
        .with_time(0.0, 1.0, 0.01)
        .with_cfl(0.25, 0.1)
        .with_physics(32, 7.0)  // Weakly compressible
        .with_kernel("wendland")
        
        // Transition to SSPH
        .as_ssph()
        
        // SSPH-specific: Artificial viscosity required
        .with_artificial_viscosity(
            0.01,   // Low α for low-speed flows
            true,   // Balsara switch
            false   // No time-dependent
        )
        
        .build();
    
    *params = *builder_params;
}
```

### Example 3: Sedov Blast with DISPH

```cpp
void SedovBlastPlugin::initialize(
    std::shared_ptr<Simulation<2>> sim,
    std::shared_ptr<SPHParameters> params
) {
    auto builder_params = SPHParametersBuilderBase()
        .with_time(0.0, 0.1, 0.001)
        .with_cfl(0.2, 0.1)
        .with_physics(48, 1.4)
        .with_kernel("cubic_spline")
        
        // Transition to DISPH
        .as_disph()
        
        // DISPH uses artificial viscosity
        .with_artificial_viscosity(
            1.5,    // Higher α for strong shocks
            true,   // Balsara switch
            false
        )
        
        .build();
    
    *params = *builder_params;
}
```

---

## Error Handling Examples

### Compile-Time Error (Type Safety)

```cpp
auto params = SPHParametersBuilderBase()
    .with_time(0.0, 0.2, 0.01)
    .as_gsph();

// ❌ COMPILE ERROR: No member 'with_artificial_viscosity' in 'AlgorithmParametersBuilder<GSPHTag>'
params.with_artificial_viscosity(1.0, true, false);

// ✅ OK: GSPH has this method
params.with_2nd_order_muscl(true);
```

### Runtime Error (Missing Required Parameter)

```cpp
try {
    auto params = SPHParametersBuilderBase()
        .with_time(0.0, 0.2, 0.01)
        .with_cfl(0.3, 0.125)
        .with_physics(50, 1.4)
        .with_kernel("cubic_spline")
        .as_ssph()
        // Missing: .with_artificial_viscosity()
        .build();
        
} catch (const std::runtime_error& e) {
    std::cerr << e.what() << std::endl;
    // Output:
    // "SSPH requires artificial viscosity configuration.
    //  Call .with_artificial_viscosity(alpha, ...) before build().
    //  Typical values: alpha=1.0, use_balsara_switch=true"
}
```

---

## Implementation Checklist

### Phase 1: Core Implementation
- [ ] Create `sph_parameters_builder_base.hpp`
- [ ] Create `ssph_parameters_builder.hpp`
- [ ] Create `disph_parameters_builder.hpp`
- [ ] Create `gsph_parameters_builder.hpp`
- [ ] Implement `.cpp` files for each builder
- [ ] Add unit tests for each builder
- [ ] Add integration tests for type safety

### Phase 2: Migration
- [ ] Update shock tube plugin to use new API
- [ ] Update 2D shock tube plugin
- [ ] Add migration guide to docs
- [ ] Keep old `SPHParametersBuilder` with deprecation warning

### Phase 3: Documentation
- [ ] Update `QUICKSTART.md` with new examples
- [ ] Update `plugin_architecture.md`
- [ ] Add compile-time safety section to docs
- [ ] Create video tutorial showing IDE autocomplete

### Phase 4: Cleanup
- [ ] Remove old `SPHParametersBuilder` (next major version)
- [ ] Update all example workflows
- [ ] Final validation tests

---

## Benefits Summary

| Feature | Current API | New API |
|---------|-------------|---------|
| Type safety | ❌ Runtime only | ✅ Compile-time |
| Self-documenting | ❌ Unclear | ✅ Crystal clear |
| Error messages | ❌ Generic | ✅ Algorithm-specific |
| IDE support | ❌ All methods visible | ✅ Only relevant methods |
| Validation | ❌ Weak | ✅ Strong |
| Extensibility | ⚠️ Medium | ✅ Easy |

---

## Conclusion

This design provides:
1. **Compile-time safety** - Cannot set incompatible parameters
2. **Self-documenting API** - Algorithm choice makes available methods clear
3. **Better error messages** - Algorithm-specific validation
4. **IDE discoverability** - Autocomplete shows only relevant methods
5. **Backward compatibility** - Can coexist with legacy builder

**Next step:** Implement base builder and GSPH builder as a proof of concept!

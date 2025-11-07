# Parameter Builder Enforcement - Type-Safe Refactoring Guide

## Problem Statement

**Current Issue:** Plugins directly mutate `SPHParameters` via raw pointers:
```cpp
// ❌ VIOLATION: Direct parameter mutation (legacy, unsafe)
param->time.end = 3.0;
param->gravity.is_valid = true;
param->gravity.constant = 1.0;
```

**Problems with Direct Access:**
1. **No Type Safety** - Can set invalid combinations
2. **No Validation** - Missing required parameters go undetected
3. **Runtime Errors** - Invalid state only discovered during simulation
4. **Poor Discoverability** - IDE can't guide developers to required fields
5. **Brittle** - Changes to parameter structure break all plugins
6. **No Compile-Time Guarantees** - Algorithm-specific requirements not enforced

---

## Solution: Builder Pattern with Type-Level Enforcement

### Architecture Overview

```
┌─────────────────────────────────────────┐
│   SPHParametersBuilderBase              │
│   (Common Parameters)                   │
│   - with_time()                         │
│   - with_cfl()                          │
│   - with_physics()                      │
│   - with_kernel()                       │
│   - with_gravity()          ← CRITICAL  │
│   - with_tree_params()                  │
│   - with_periodic_boundary()            │
└──────────────┬──────────────────────────┘
               │ .as_ssph() / .as_disph() / .as_gsph()
               │
       ┌───────┴─────────┬────────────────┐
       │                 │                │
       ▼                 ▼                ▼
┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│ SSPH Builder │  │ DISPH Builder│  │ GSPH Builder │
│ - with_av()  │  │ - with_av()  │  │ - riemann()  │
│   REQUIRED   │  │   REQUIRED   │  │   REQUIRED   │
└──────────────┘  └──────────────┘  └──────────────┘
       │                 │                │
       └────────┬────────┴────────────────┘
                │ .build()
                ▼
        std::shared_ptr<SPHParameters>
        (Fully validated, immutable)
```

---

## Type-Safe Refactoring for Evrard Plugin

### Step 1: Current (Legacy) Code

```cpp
// ❌ LEGACY CODE - DO NOT USE
void initialize(std::shared_ptr<Simulation<3>> sim,
               std::shared_ptr<SPHParameters> param) override {
    // ... particle setup ...
    
    // Direct mutation - NO TYPE SAFETY
    param->time.end = 3.0;
    param->time.output = 0.1;
    param->cfl.sound = 0.3;
    param->cfl.force = 0.25;
    param->physics.neighbor_number = 50;
    param->physics.gamma = 5.0 / 3.0;
    param->gravity.is_valid = true;      // ❌ Easy to forget!
    param->gravity.constant = 1.0;
    param->gravity.theta = 0.5;
    param->av.alpha = 1.0;
}
```

### Step 2: Type-Safe Builder Pattern (Recommended)

```cpp
// ✅ TYPE-SAFE, MODERN APPROACH
void initialize(std::shared_ptr<Simulation<3>> sim,
               std::shared_ptr<SPHParameters> param) override {
    static constexpr int Dim = 3;
    static constexpr real G = 1.0;
    static constexpr real gamma = 5.0 / 3.0;
    
    std::cout << "Initializing Evrard collapse simulation...\n";
    
    // ... particle setup code (unchanged) ...
    
    // ✅ TYPE-SAFE PARAMETER CONSTRUCTION
    auto built_params = SPHParametersBuilderBase()
        // Required common parameters
        .with_time(
            /*start=*/0.0,
            /*end=*/3.0,
            /*output=*/0.1
        )
        .with_cfl(
            /*sound=*/0.3,
            /*force=*/0.25
        )
        .with_physics(
            /*neighbor_number=*/50,
            /*gamma=*/gamma
        )
        .with_kernel("cubic_spline")
        
        // CRITICAL: Gravity is now type-safe and discoverable
        .with_gravity(
            /*constant=*/G,
            /*theta=*/0.5  // Barnes-Hut opening angle
        )
        .with_tree_params(
            /*max_level=*/20,
            /*leaf_particle_num=*/1
        )
        
        // Transition to SSPH (requires artificial viscosity)
        .as_ssph()
        .with_artificial_viscosity(
            /*alpha=*/1.0,
            /*use_balsara_switch=*/true
        )
        
        // Build validated, immutable parameters
        .build();
    
    // Copy built parameters to provided param object
    *param = *built_params;
    
    std::cout << "✓ Parameters validated and set:\n";
    std::cout << "  Gravity enabled: " << param->gravity.is_valid << "\n";
    std::cout << "  G = " << param->gravity.constant << "\n";
    std::cout << "  θ = " << param->gravity.theta << "\n";
    
    sim->particle_num = particles.size();
}
```

---

## Enforcement Strategy: Making Legacy Code Impossible

### Option 1: Make `SPHParameters` Members Private (Aggressive)

```cpp
// In parameters.hpp
struct SPHParameters {
    // Grant builder exclusive access
    template<typename T> friend class AlgorithmParametersBuilder;
    friend class SPHParametersBuilderBase;
    
private:
    // ❌ Plugins can no longer do this:
    // param->gravity.is_valid = true;  // COMPILE ERROR
    
    TimeParameters time;
    CFLParameters cfl;
    PhysicsParameters physics;
    GravityParameters gravity;
    // ... other parameters ...
    
public:
    // ✅ Read-only access only
    const TimeParameters& get_time() const { return time; }
    const GravityParameters& get_gravity() const { return gravity; }
    // ... getters for all parameters ...
};
```

**Pros:**
- ✅ **Compile-time enforcement** - Legacy code won't compile
- ✅ **Impossible to bypass** - Type system prevents misuse
- ✅ **Forced migration** - All plugins must use builder

**Cons:**
- ❌ **Breaking change** - ALL existing plugins break
- ❌ **Migration effort** - Need to update every plugin
- ❌ **Gradual migration impossible** - All-or-nothing

---

### Option 2: Deprecation Warnings (Gradual)

```cpp
// In parameters.hpp
struct SPHParameters {
    // Deprecated direct access - compiler warns
    [[deprecated("Use SPHParametersBuilderBase().with_gravity() instead")]]
    GravityParameters gravity;
    
    [[deprecated("Use SPHParametersBuilderBase().with_time() instead")]]
    TimeParameters time;
    
    // ... other parameters with deprecation warnings ...
};
```

**Pros:**
- ✅ **Gradual migration** - Old code still works
- ✅ **Clear warnings** - Compiler guides developers
- ✅ **Low risk** - No immediate breakage

**Cons:**
- ❌ **Can be ignored** - Warnings don't prevent misuse
- ❌ **Inconsistent codebase** - Mix of old/new styles
- ❌ **Technical debt** - Eventually need breaking change

---

### Option 3: Runtime Validation Guard (Hybrid Approach)

```cpp
// In parameters.hpp
struct SPHParameters {
    // Parameters remain public for backwards compatibility
    GravityParameters gravity;
    TimeParameters time;
    // ... etc ...
    
private:
    mutable bool validated_by_builder = false;
    
    template<typename T> friend class AlgorithmParametersBuilder;
    friend class SPHParametersBuilderBase;
    
public:
    // Builders mark parameters as validated
    void mark_validated() { validated_by_builder = true; }
    
    // Solver checks validation before use
    void require_builder_validation() const {
        if (!validated_by_builder) {
            THROW_ERROR(
                "SPHParameters must be constructed via builder pattern.\n"
                "Use SPHParametersBuilderBase() instead of direct mutation.\n"
                "See docs/refactor/PARAMETER_BUILDER_ENFORCEMENT.md"
            );
        }
    }
};

// In solver.cpp
template<int Dim>
void Solver<Dim>::initialize() {
    // ✅ Enforce builder usage at runtime
    m_param->require_builder_validation();
    
    // ... rest of initialization ...
}
```

**Pros:**
- ✅ **Runtime enforcement** - Catches misuse immediately
- ✅ **Clear error messages** - Guides developers to solution
- ✅ **Backwards compatible** - Old code runs but fails with clear message
- ✅ **Easy to add** - Minimal code changes

**Cons:**
- ❌ **Runtime check** - Not caught at compile time
- ❌ **Can be bypassed** - Could call `mark_validated()` manually

---

## Recommended Migration Path

### Phase 1: Add Validation Guard (Week 1)

1. Add `validated_by_builder` flag to `SPHParameters`
2. Add `mark_validated()` method to builder `.build()`
3. Add `require_builder_validation()` check in `Solver::initialize()`
4. Update documentation with migration guide

```cpp
// All existing plugins fail at runtime with helpful error:
// "SPHParameters must be constructed via builder pattern..."
```

### Phase 2: Migrate All Plugins (Week 2-3)

Migrate plugins one by one using builder pattern:

**Priority Order:**
1. ✅ `evrard_plugin.cpp` (your current issue)
2. ✅ `shock_tube_plugin_*.cpp` (already use builder)
3. ✅ Other workflow plugins
4. ✅ Test plugins
5. ✅ Example plugins

### Phase 3: Add Deprecation Warnings (Week 4)

```cpp
[[deprecated("Use builder pattern")]]
GravityParameters gravity;
```

### Phase 4: Make Members Private (Future Release)

Once all code migrated:
```cpp
struct SPHParameters {
    template<typename T> friend class AlgorithmParametersBuilder;
    friend class SPHParametersBuilderBase;
    
private:
    GravityParameters gravity;  // Now truly immutable
    // ...
};
```

---

## Complete Evrard Plugin Refactored Code

```cpp
// workflows/evrard_workflow/01_simulation/src/plugin.cpp

#include "core/plugins/simulation_plugin.hpp"
#include "core/simulation/simulation.hpp"
#include "core/parameters/sph_parameters_builder_base.hpp"  // ← ADDED
#include "core/parameters/ssph_parameters_builder.hpp"      // ← ADDED
#include "parameters.hpp"
#include "core/particles/sph_particle.hpp"
#include "exception.hpp"
#include "defines.hpp"
#include <iostream>
#include <vector>
#include <cmath>

namespace sph {

class EvrardPlugin : public SimulationPlugin<3> {
public:
    std::string get_name() const override {
        return "evrard_collapse";
    }
    
    std::string get_description() const override {
        return "3D Evrard gravitational collapse (type-safe builder pattern)";
    }
    
    std::string get_version() const override {
        return "3.0.0";  // Bumped for type-safe refactor
    }
    
    void initialize(std::shared_ptr<Simulation<3>> sim,
                   std::shared_ptr<SPHParameters> param) override {
        static constexpr int Dim = 3;
        static constexpr real G = 1.0;           // Gravitational constant
        static constexpr real gamma = 5.0 / 3.0; // Polytropic index
        static constexpr real u_thermal = 0.05;  // Thermal energy coefficient

        std::cout << "Initializing Evrard collapse (type-safe parameters)...\n";
        
        // ==================== PARTICLE SETUP ====================
        
        const int N = 20;  // Grid resolution
        auto & particles = sim->particles;
        const real dx = 2.0 / N;
        
        std::cout << "Creating sphere with N=" << N << "^3 grid...\n";
        
        // Create particles in a sphere
        for(int i = 0; i < N; ++i) {
            for(int j = 0; j < N; ++j) {
                for(int k = 0; k < N; ++k) {
                    Vector<Dim> r = {
                        (i + 0.5) * dx - 1.0,
                        (j + 0.5) * dx - 1.0,
                        (k + 0.5) * dx - 1.0
                    };
                    const real r_0 = abs(r);
                    if(r_0 > 1.0) {
                        continue;
                    }
                    
                    // Apply radial distortion for better sampling
                    if(r_0 > 0.0) {
                        const real r_abs = std::pow(r_0, 1.5);
                        r = r * (r_abs / r_0);
                    }
                    
                    SPHParticle<Dim> p_i;
                    p_i.pos = r;
                    particles.emplace_back(p_i);
                }
            }
        }
        
        const real mass = 1.0 / particles.size();
        const real u = u_thermal * G;
        
        std::cout << "  Total particles: " << particles.size() << "\n";
        std::cout << "  Particle mass: " << mass << "\n";
        std::cout << "  Thermal energy: " << u << "\n";
        
        // Set particle properties
        int i = 0;
        for(auto & p_i : particles) {
            // Zero velocity (initially at rest)
            for (int d = 0; d < Dim; ++d) {
                p_i.vel[d] = 0.0;
            }
            
            p_i.mass = mass;
            real r_mag = std::sqrt(inner_product(p_i.pos, p_i.pos));
            
            // Guard against division by zero at center
            constexpr real epsilon = 1.0e-10;
            if (r_mag < epsilon) {
                r_mag = epsilon;
            }
            
            // Evrard initial conditions: ρ(r) ∝ 1/r
            p_i.dens = 1.0 / (2.0 * M_PI * r_mag);
            p_i.ene = u;
            p_i.pres = (gamma - 1.0) * p_i.dens * u;
            p_i.id = i;
            
            // Debug initial state
            if (i < 5 || !std::isfinite(p_i.dens) || !std::isfinite(p_i.pres)) {
                std::cout << "  Particle " << i << ": pos=(" << p_i.pos[0] << "," 
                          << p_i.pos[1] << "," << p_i.pos[2] << "), r_mag=" << r_mag
                          << ", dens=" << p_i.dens << ", pres=" << p_i.pres 
                          << ", ene=" << p_i.ene << "\n";
            }
            
            ++i;
        }
        
        sim->particle_num = particles.size();
        
        // ==================== TYPE-SAFE PARAMETER CONSTRUCTION ====================
        
        std::cout << "Building type-safe parameters...\n";
        
        auto built_params = SPHParametersBuilderBase()
            // Time parameters
            .with_time(
                /*start=*/0.0,
                /*end=*/3.0,
                /*output=*/0.1,
                /*energy=*/0.1  // Energy output interval
            )
            
            // CFL parameters (stability)
            .with_cfl(
                /*sound=*/0.3,  // CFL for sound speed
                /*force=*/0.25  // CFL for force term
            )
            
            // Physical parameters
            .with_physics(
                /*neighbor_number=*/50,  // Target neighbors for kernel
                /*gamma=*/gamma          // Adiabatic index
            )
            
            // Kernel function
            .with_kernel("cubic_spline")
            
            // ✅ GRAVITY - Type-safe, discoverable, validated
            .with_gravity(
                /*constant=*/G,      // Gravitational constant
                /*theta=*/0.5        // Barnes-Hut opening angle
            )
            
            // Tree construction parameters
            .with_tree_params(
                /*max_level=*/20,           // Maximum tree depth
                /*leaf_particle_num=*/1     // Particles per leaf
            )
            
            // Transition to Standard SPH
            .as_ssph()
            
            // SSPH requires artificial viscosity (type-enforced!)
            .with_artificial_viscosity(
                /*alpha=*/1.0,                  // Viscosity coefficient
                /*use_balsara_switch=*/true,    // Reduce viscosity in shear
                /*use_time_dependent_av=*/false // Static viscosity
            )
            
            // Build and validate
            .build();
        
        // Copy to provided parameter object
        *param = *built_params;
        
        std::cout << "✓ Parameters validated and set:\n";
        std::cout << "  Algorithm: Standard SPH (SSPH)\n";
        std::cout << "  Gravity enabled: " << param->gravity.is_valid << "\n";
        std::cout << "  G = " << param->gravity.constant << "\n";
        std::cout << "  θ (theta) = " << param->gravity.theta << "\n";
        std::cout << "  γ (gamma) = " << param->physics.gamma << "\n";
        std::cout << "  Artificial viscosity α = " << param->av.alpha << "\n";
        std::cout << "  Neighbor number = " << param->physics.neighbor_number << "\n";
        
        std::cout << "Initialization complete!\n";
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"plugin.cpp"};
    }
};

} // namespace sph

DEFINE_SIMULATION_PLUGIN(sph::EvrardPlugin, 3)
```

---

## Benefits of This Refactoring

### Type Safety
```cpp
// ❌ Old way - can forget gravity
param->time.end = 3.0;
// ... forget to set gravity.is_valid ...
// Simulation runs but gravity doesn't work!

// ✅ New way - compiler guides you
auto params = SPHParametersBuilderBase()
    .with_time(...)
    .with_gravity(...)  // ← IDE autocomplete shows this
    .build();           // ← Validation ensures nothing missing
```

### Discoverability
```cpp
// Old: What parameters exist? Read docs or source code.
// New: Type `.` and IDE shows all available methods:
SPHParametersBuilderBase()
    .with_time(...)
    .with_cfl(...)
    .with_physics(...)
    .with_gravity(...)  // ← IDE shows this exists!
```

### Validation
```cpp
// Old: Runtime crash if parameters invalid
// New: Build-time error
auto params = SPHParametersBuilderBase()
    .with_time(0.0, 3.0, 0.1)
    .as_ssph()
    .build();  // ❌ ERROR: SSPH requires artificial_viscosity!
```

### Immutability
```cpp
// Old: Parameters can change mid-simulation (bug-prone)
param->gravity.constant = 2.0;  // Whoops!

// New: Parameters are immutable after build
auto params = builder.build();
// params->gravity.constant = 2.0;  // ❌ COMPILE ERROR (if private)
```

---

## Summary: Action Items

### Immediate (This Week)
1. ✅ **Refactor `evrard_plugin.cpp`** using builder pattern
2. ✅ **Add validation guard** to `SPHParameters`
3. ✅ **Test** that simulation still works correctly

### Short-term (Next 2 Weeks)
4. ✅ Migrate remaining workflow plugins
5. ✅ Add deprecation warnings to direct access
6. ✅ Update all documentation

### Long-term (Future Release)
7. ✅ Make `SPHParameters` members private
8. ✅ Remove validation guard (no longer needed)
9. ✅ Pure compile-time type safety

---

## Key Takeaway

**Before:** "I hope I remembered to set all required parameters..."
**After:** "The compiler won't let me forget required parameters!"

This is the difference between **hope-driven development** and **type-driven development**.

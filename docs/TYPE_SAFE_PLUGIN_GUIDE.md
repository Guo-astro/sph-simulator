# Type-Safe Plugin Development Guide

> **Note:** This document describes the historical V2 approach using phantom types.
> **The current standard is V3** (pure functional interface), which provides superior
> compile-time type safety. See `workflows/USAGE_GUIDE.md` for V3 documentation.

## Problem: Accessing Uninitialized State

### The Bug (Bus Error: 10)

The original shock tube plugin crashed with **Bus error: 10** because it accessed uninitialized memory:

```cpp
// ❌ BUG: Accessing p.sml before it's computed
real max_sml = 0.0;
for (const auto& p : sim->particles) {
    max_sml = std::max(max_sml, p.sml);  // p.sml is GARBAGE!
}
```

This happened because:
1. Plugin creates particles and sets initial conditions (pos, vel, mass, dens, pres)
2. Plugin tries to access `p.sml` (smoothing length) 
3. **But `p.sml` hasn't been computed yet!** → Undefined behavior → Crash

The smoothing length is only computed later by `Solver::initialize()` during the `PreInteraction::calculation()` phase.

### Root Cause: No Compile-Time Protection

The old `SimulationPlugin` interface gave plugins full access to `Simulation`, allowing them to:
- Access uninitialized fields (`sml`, `acc`, `dt_cfl`)
- Generate ghost particles before `sml` is computed
- Build spatial tree before `sml` is computed

**There was no compile-time protection against these bugs.**

---

## Solution: Type-Safe Initialization Phases

We use **phantom types** (zero-cost abstraction) to track initialization state at compile time.

### Phase Tags

```cpp
namespace phase {
    struct Uninitialized {};  // Particles created, derived fields NOT computed
    struct Initialized {};     // All fields computed, ready for simulation
}
```

These are never instantiated - they exist purely as compile-time markers.

### Phase-Aware Simulation View

```cpp
template<int Dim, typename Phase>
class SimulationPhaseView {
    // Operations valid in ALL phases
    std::vector<SPHParticle<Dim>>& particles();
    void set_particle_num(int num);
    
    // Operations ONLY valid in Initialized phase (SFINAE)
    template<typename P = Phase>
    std::enable_if_t<allows_ghost_operations<P>::value, 
                     std::shared_ptr<GhostParticleManager<Dim>>>
    ghost_manager();  // ← Compile error if Phase = Uninitialized!
    
    template<typename P = Phase>
    std::enable_if_t<allows_tree_operations<P>::value, void>
    make_tree();      // ← Compile error if Phase = Uninitialized!
};
```

Type aliases for convenience:
```cpp
template<int Dim>
using UninitializedSimulation = SimulationPhaseView<Dim, phase::Uninitialized>;

template<int Dim>
using InitializedSimulation = SimulationPhaseView<Dim, phase::Initialized>;
```

### Type-Safe Plugin Interface (V2)

```cpp
template<int Dim>
class SimulationPluginV2 {
    virtual void initialize(UninitializedSimulation<Dim> sim,  // ← Uninitialized!
                          std::shared_ptr<SPHParameters> params) = 0;
};
```

**Key insight:** Plugins receive `UninitializedSimulation`, which:
- ✅ **Allows:** Setting particles, configuring parameters
- ❌ **Prevents (compile error):** Accessing `ghost_manager()`, calling `make_tree()`

---

## Usage Examples

### ✅ Correct: Type-Safe Plugin

```cpp
class TypeSafePlugin : public SimulationPluginV2<2> {
    void initialize(UninitializedSimulation<2> sim,
                   std::shared_ptr<SPHParameters> params) override {
        
        // ✅ SAFE: Create and configure particles
        std::vector<SPHParticle<2>> particles(1000);
        for (auto& p : particles) {
            p.pos = /* ... */;
            p.vel = /* ... */;
            p.mass = /* ... */;
            p.dens = /* ... */;  // Initial density (from IC)
            p.pres = /* ... */;  // Initial pressure (from IC)
            // Note: Do NOT set sml, acc - they're computed by Solver
        }
        
        sim.particles() = std::move(particles);
        sim.set_particle_num(1000);
        
        // ✅ SAFE: Configure boundary (just config, no ghost generation)
        auto boundary_config = BoundaryBuilder<2>()
            .with_periodic_boundaries()
            .in_range(/* ... */)
            .build();
        
        auto underlying = sim.underlying_simulation();
        underlying->ghost_manager->initialize(boundary_config);
        
        // ❌ COMPILE ERROR if uncommented:
        // sim.ghost_manager()->generate_ghosts(particles);  // Requires Initialized phase!
        // sim.make_tree();  // Requires Initialized phase!
    }
};
```

### ❌ Wrong: Accessing Uninitialized State

```cpp
void initialize(UninitializedSimulation<2> sim, ...) {
    // ... create particles ...
    
    // ❌ COMPILE ERROR: ghost_manager() not available in Uninitialized phase
    auto ghosts = sim.ghost_manager();
    
    // Compiler error message:
    //   "no matching member function for call to 'ghost_manager'"
    //   note: candidate template ignored: requirement
    //   'allows_ghost_operations<sph::phase::Uninitialized>::value' was not satisfied
}
```

### Phase Transition (Solver Only)

Only `Solver::initialize()` transitions to the `Initialized` phase after computing derived quantities:

```cpp
void Solver<Dim>::initialize() {
    m_sim = std::make_shared<Simulation<Dim>>(m_param);
    
    // Call plugin (Uninitialized phase)
    auto uninit_view = UninitializedSimulation<Dim>::create_uninitialized(m_sim);
    m_plugin->initialize(uninit_view, m_param);
    
    // Compute derived quantities
    m_pre->calculation(m_sim);  // Computes sml, dens from neighbors
    m_fforce->calculation(m_sim);  // Computes acc
    
    // Now safe to generate ghosts (sml is computed)
    if (m_sim->ghost_manager && m_sim->ghost_manager->get_config().is_valid) {
        real max_sml = /* find max among particles */;
        m_sim->ghost_manager->set_kernel_support_radius(2.0 * max_sml);
        m_sim->ghost_manager->generate_ghosts(m_sim->particles);
    }
    
    // Transition to Initialized phase (internal to Solver)
    // auto init_view = uninit_view.unsafe_transition_to_initialized();
}
```

---

## Compile-Time Guarantees

The type system **prevents** these bugs at compile time:

| Operation | Uninitialized Phase | Initialized Phase |
|-----------|---------------------|-------------------|
| `particles()` | ✅ Read/Write | ✅ Read/Write |
| `set_particle_num()` | ✅ Yes | ✅ Yes |
| `ghost_manager()` | ❌ **Compile Error** | ✅ Yes |
| `make_tree()` | ❌ **Compile Error** | ✅ Yes |
| `sync_particle_cache()` | ❌ **Compile Error** | ✅ Yes |
| Accessing `p.sml` directly | ⚠️ Runtime UB* | ✅ Safe |

\* We can't prevent direct field access through the `particles()` reference. **Best practice:** Only access initial condition fields (`pos`, `vel`, `mass`, `dens`, `pres`) in plugins.

---

## Migration Guide

---

## Current Recommendation: Use V3

**The V3 pure functional interface is now the standard approach** for all new plugins.
V3 provides superior type safety by preventing plugins from accessing `Simulation` state entirely:

```cpp
template<int Dim>
class MyPlugin : public SimulationPluginV3<Dim> {
    InitialCondition<Dim> create_initial_condition() const override {
        std::vector<SPHParticle<Dim>> particles;
        // ... create particles ...
        
        auto param = SPHParametersBuilderBase()
            .with_time(0.0, 1.0, 0.1, 0.1)
            .as_ssph()
            .build();
        
        auto boundary_config = BoundaryBuilder<Dim>()
            .with_no_boundaries()
            .build();
        
        // Pure functional: return initial condition, no state access
        return InitialCondition<Dim>{
            .particles = std::move(particles),
            .parameters = param,
            .boundary_config = boundary_config
        };
    }
};

DEFINE_SIMULATION_PLUGIN_V3(MyPlugin, DIM)
```

**V3 Benefits:**
- **Architecturally impossible** to access uninitialized state (plugin never sees Simulation)
- Compile-time dimension checking (no runtime template instantiation errors)
- Pure functional (no side effects, easier to test and reason about)
- Immutable initial conditions (returned, not mutated)

See `workflows/USAGE_GUIDE.md` for complete V3 documentation.

---

## Historical: V2 Phantom Types Approach

The rest of this document describes the V2 approach using phantom types.
**V2 is deprecated** in favor of V3. This is preserved for historical reference.

### For Existing Plugins

**Option 1: Quick fix (keep old interface)**
```cpp
class MyPlugin : public SimulationPlugin<Dim> {
    void initialize(std::shared_ptr<Simulation<Dim>> sim, ...) override {
        // ... setup particles ...
        
        // ❌ REMOVE: Don't access sml or generate ghosts
        // real max_sml = 0.0;
        // for (const auto& p : sim->particles) {
        //     max_sml = std::max(max_sml, p.sml);  // BUG!
        // }
        
        // ✅ KEEP: Just configure boundary, let Solver handle ghosts
        auto config = BoundaryBuilder<Dim>()...build();
        sim->ghost_manager->initialize(config);
    }
};
```

**Option 2: Migrate to type-safe interface**
```cpp
class MyPlugin : public SimulationPluginV2<Dim> {  // ← V2
    void initialize(UninitializedSimulation<Dim> sim, ...) override {  // ← Type-safe
        // Compiler enforces correct usage
    }
};

DEFINE_SIMULATION_PLUGIN_V2(MyPlugin, Dim)  // ← V2 export macro
```

---

## Design Rationale

### Why Phantom Types?

**Zero-cost abstraction:**
- No runtime overhead
- All checks happen at compile time
- Optimizes away completely in release builds

**Clear error messages:**
```
error: no matching member function for call to 'ghost_manager'
note: candidate template ignored: requirement 
'allows_ghost_operations<sph::phase::Uninitialized>::value' was not satisfied
```

**Self-documenting:**
- Signature `UninitializedSimulation<2>` clearly indicates constraints
- No need to read documentation to know what's safe

### Limitations

1. **Cannot prevent direct field access:**
   ```cpp
   for (auto& p : sim.particles()) {
       p.sml = /* ... */;  // Not caught at compile time
   }
   ```
   **Mitigation:** Coding discipline + documentation

2. **Requires discipline for underlying_simulation():**
   ```cpp
   auto raw_sim = sim.underlying_simulation();
   raw_sim->ghost_manager->generate_ghosts(...);  // Bypasses type safety
   ```
   **Mitigation:** Only use for configuration, not operations

3. **Template complexity:**
   - SFINAE can be intimidating
   - Error messages can be verbose (but informative)
   **Mitigation:** Provide type aliases and examples

---

## References

- **Current Standard (V3):**
  - `include/core/plugins/simulation_plugin_v3.hpp`
  - `include/core/plugins/initial_condition.hpp`
  - `workflows/USAGE_GUIDE.md`

- **Historical (V2 - Deprecated):**
  - ~~`include/core/simulation/initialization_phases.hpp`~~ (deleted)
  - ~~`include/core/simulation/simulation_phase_view.hpp`~~ (deleted)
  - ~~`include/core/plugins/simulation_plugin_v2.hpp`~~ (deleted)

- **V3 Examples:**
  - All workflows in `workflows/` directory now use V3
  - 13/15 plugins successfully migrated and built

---

## Summary

**Before (Unsafe):**
```cpp
void initialize(std::shared_ptr<Simulation<Dim>> sim, ...) {
    // Can access ANYTHING - no compile-time safety
    real max_sml = 0.0;
    for (const auto& p : sim->particles) {
        max_sml = std::max(max_sml, p.sml);  // BUS ERROR: sml uninitialized!
    }
}
```

**After (Type-Safe):**
```cpp
void initialize(UninitializedSimulation<Dim> sim, ...) {
    // Compiler PREVENTS accessing uninitialized state
    // auto ghosts = sim.ghost_manager();  // COMPILE ERROR ✓
    
    // Only safe operations allowed
    sim.particles() = create_initial_particles();
}
```

**Result:** **Bus error: 10** prevented at **compile time** instead of crashing at runtime!

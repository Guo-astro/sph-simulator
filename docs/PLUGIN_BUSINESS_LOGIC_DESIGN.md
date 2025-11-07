# Plugin Business Logic Design - Separation of Concerns

## Current Problem

**User Concern:** Plugins currently mix **business logic** (defining initial conditions) with **system initialization** (managing simulation state):

```cpp
// ❌ CURRENT: Plugin does BOTH business logic AND system management
void initialize(UninitializedSimulation<2> sim, std::shared_ptr<SPHParameters> param) {
    // Business logic: Create particles with physics
    std::vector<SPHParticle<Dim>> particles = create_shock_tube_particles();
    
    // System management: Transfer to simulation (NOT business logic!)
    sim.particles() = std::move(particles);         // ← System detail
    sim.set_particle_num(num);                      // ← System detail
    
    // System management: Configure ghost manager (NOT business logic!)
    auto underlying_sim = sim.underlying_simulation();  // ← Escape hatch
    underlying_sim->ghost_manager->initialize(config);  // ← System detail
}
```

**Issues:**
1. Plugin authors must know about `sim.particles()`, `sim.set_particle_num()`, `underlying_simulation()`
2. Business logic (shock tube physics) is mixed with plumbing (moving data into sim)
3. Plugins can bypass type safety using `underlying_simulation()`

---

## Ideal Design: Pure Business Logic

**Goal:** Plugins should only express **WHAT** (physics), not **HOW** (system management)

### Proposed Interface

```cpp
/**
 * @brief Pure business logic plugin interface
 * 
 * Plugins return initial conditions as data. The framework handles
 * all system initialization automatically.
 */
template<int Dim>
class SimulationPluginV3 {
public:
    virtual ~SimulationPluginV3() = default;
    
    // Metadata
    virtual std::string get_name() const = 0;
    virtual std::string get_description() const = 0;
    virtual std::string get_version() const = 0;
    
    /**
     * @brief Define initial conditions (PURE business logic)
     * 
     * Plugin ONLY returns:
     * 1. Initial particles (positions, velocities, densities, etc.)
     * 2. SPH parameters (CFL, neighbor count, kernel, etc.)
     * 3. Boundary configuration (periodic, mirror, etc.)
     * 
     * The framework automatically:
     * - Moves particles into simulation
     * - Sets particle count
     * - Initializes ghost manager
     * - Syncs particle cache
     * - Builds spatial tree
     * - Computes smoothing lengths
     * - Generates ghost particles
     * 
     * @return Initial condition data (particles + config)
     */
    virtual InitialCondition<Dim> create_initial_condition() const = 0;
    
    virtual std::vector<std::string> get_source_files() const = 0;
};
```

### Data Structure

```cpp
/**
 * @brief Initial condition data (pure business logic output)
 * 
 * This is a PLAIN DATA OBJECT - no system dependencies.
 * Plugins construct this and return it. Framework handles the rest.
 */
template<int Dim>
struct InitialCondition {
    // Particle data (BUSINESS LOGIC)
    std::vector<SPHParticle<Dim>> particles;
    
    // SPH parameters (BUSINESS LOGIC)
    std::shared_ptr<SPHParameters> parameters;
    
    // Boundary configuration (BUSINESS LOGIC)
    std::optional<BoundaryConfiguration<Dim>> boundary_config;
    
    // Optional: Output configuration
    std::optional<OutputConfiguration> output_config;
    
    // Convenience builders
    static InitialCondition<Dim> with_particles(std::vector<SPHParticle<Dim>> p) {
        InitialCondition<Dim> ic;
        ic.particles = std::move(p);
        return ic;
    }
    
    InitialCondition& with_parameters(std::shared_ptr<SPHParameters> params) {
        parameters = std::move(params);
        return *this;
    }
    
    InitialCondition& with_boundaries(BoundaryConfiguration<Dim> config) {
        boundary_config = std::move(config);
        return *this;
    }
};
```

### Example Plugin (V3 - Pure Business Logic)

```cpp
class ShockTubePlugin : public SimulationPluginV3<2> {
public:
    std::string get_name() const override { 
        return "shock_tube_2d"; 
    }
    
    std::string get_description() const override { 
        return "2D Sod shock tube - SSPH"; 
    }
    
    std::string get_version() const override { 
        return "3.0.0"; 
    }
    
    /**
     * @brief Create shock tube initial conditions (PURE physics)
     * 
     * NO system calls - just return the data!
     */
    InitialCondition<2> create_initial_condition() const override {
        constexpr int Dim = 2;
        constexpr real gamma = 1.4;
        
        // ===== BUSINESS LOGIC: Create particles =====
        const real Ly = 0.5;
        const int Nx_left = 40, Nx_right = 10;
        const real dx_left = 0.025, dx_right = 0.1;
        const real dy = dx_left;
        const int Ny = static_cast<int>(Ly / dy);
        const int num = Nx_left * Ny + Nx_right * Ny;
        
        std::vector<SPHParticle<Dim>> particles(num);
        int idx = 0;
        
        // Left state (high pressure)
        for (int j = 0; j < Ny; ++j) {
            real y = dy * (j + 0.5);
            for (int i = 0; i < Nx_left; ++i) {
                real x = -0.5 + dx_left * (i + 0.5);
                auto& p = particles[idx++];
                p.pos = Vector<Dim>{x, y};
                p.vel = Vector<Dim>{0.0, 0.0};
                p.dens = 1.0;
                p.pres = 1.0;
                p.mass = 1.0 * dx_left * dy;
                p.ene = p.pres / ((gamma - 1.0) * p.dens);
                p.id = idx - 1;
            }
        }
        
        // Right state (low pressure)
        for (int j = 0; j < Ny; ++j) {
            real y = dy * (j + 0.5);
            for (int i = 0; i < Nx_right; ++i) {
                real x = 0.5 + dx_right * (i + 0.5);
                auto& p = particles[idx++];
                p.pos = Vector<Dim>{x, y};
                p.vel = Vector<Dim>{0.0, 0.0};
                p.dens = 0.25;
                p.pres = 0.1;
                p.mass = 1.0 * dx_left * dy;  // Uniform mass
                p.ene = p.pres / ((gamma - 1.0) * p.dens);
                p.id = idx - 1;
            }
        }
        
        // ===== BUSINESS LOGIC: Configure SPH parameters =====
        auto suggestions = ParameterEstimator::suggest_parameters<Dim>(particles, 2.0);
        
        auto parameters = SPHParametersBuilderBase()
            .with_time(0.0, 0.2, 0.01)
            .with_physics(suggestions.neighbor_number, gamma)
            .with_cfl(0.3, 0.25)
            .with_kernel("cubic_spline")
            .with_iterative_smoothing_length(true)
            .as_ssph()
            .with_artificial_viscosity(1.0)
            .build();
        
        // ===== BUSINESS LOGIC: Configure boundaries =====
        auto boundary_config = BoundaryBuilder<Dim>()
            .with_periodic_boundaries()
            .in_range(
                Vector<Dim>{-0.5, 0.0},
                Vector<Dim>{1.5, 0.5}
            )
            .build();
        
        // ===== RETURN DATA: No system calls! =====
        return InitialCondition<Dim>::with_particles(std::move(particles))
            .with_parameters(std::move(parameters))
            .with_boundaries(std::move(boundary_config));
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"plugin_shock_tube_v3.cpp"};
    }
};

DEFINE_SIMULATION_PLUGIN_V3(ShockTubePlugin, 2)
```

**Notice:**
- ✅ **NO** `sim.particles() = ...`
- ✅ **NO** `sim.set_particle_num(...)`
- ✅ **NO** `underlying_simulation()`
- ✅ **NO** `ghost_manager->initialize(...)`
- ✅ **ONLY** physics and configuration

---

## Framework Implementation

The `Solver` handles all system initialization automatically:

```cpp
template<int Dim>
void Solver<Dim>::make_initial_condition() {
    if (!m_plugin) {
        THROW_ERROR("No plugin loaded.");
    }
    
    WRITE_LOG << "Initializing simulation from plugin: " << m_plugin->get_name();
    
    // ===== PLUGIN RETURNS DATA (pure business logic) =====
    auto initial_condition = m_plugin->create_initial_condition();
    
    // ===== FRAMEWORK HANDLES SYSTEM INITIALIZATION =====
    
    // 1. Transfer particles to simulation
    m_sim->particles = std::move(initial_condition.particles);
    m_sim->particle_num = static_cast<int>(m_sim->particles.size());
    
    // 2. Apply parameters
    if (initial_condition.parameters) {
        *m_param = *initial_condition.parameters;
    }
    
    // 3. Configure boundaries
    if (initial_condition.boundary_config) {
        m_sim->ghost_manager->initialize(*initial_condition.boundary_config);
    }
    
    // 4. Configure output (if specified)
    if (initial_condition.output_config) {
        apply_output_config(*initial_condition.output_config);
    }
    
    WRITE_LOG << "Plugin initialization complete";
    WRITE_LOG << "Particles loaded: " << m_sim->particle_num;
    
    // Re-initialize tree with plugin parameters
    m_sim->tree->initialize(m_param);
}
```

**Benefits:**
1. ✅ Plugin code is **pure physics** - easy to understand
2. ✅ No system details leak into business logic
3. ✅ Impossible to bypass type safety
4. ✅ Framework can optimize system initialization
5. ✅ Easy to test plugins (just check returned data)
6. ✅ Clear separation of concerns

---

## Migration Strategy

### Phase 1: Add V3 Interface (Non-Breaking)

Create `SimulationPluginV3` alongside existing `SimulationPlugin` and `SimulationPluginV2`:

```cpp
// include/core/plugins/simulation_plugin_v3.hpp
template<int Dim>
class SimulationPluginV3 {
    virtual InitialCondition<Dim> create_initial_condition() const = 0;
    // ...
};
```

### Phase 2: Update Solver to Support All Versions

```cpp
void Solver<Dim>::make_initial_condition() {
    // Try V3 first (pure business logic)
    if (auto plugin_v3 = dynamic_cast<SimulationPluginV3<Dim>*>(m_plugin.get())) {
        auto ic = plugin_v3->create_initial_condition();
        apply_initial_condition(ic);  // Framework handles system init
        return;
    }
    
    // Fall back to V2 (type-safe but mixed concerns)
    if (auto plugin_v2 = dynamic_cast<SimulationPluginV2<Dim>*>(m_plugin.get())) {
        auto uninit_view = UninitializedSimulation<Dim>::create_uninitialized(m_sim);
        plugin_v2->initialize(uninit_view, m_param);
        return;
    }
    
    // Fall back to V1 (legacy)
    m_plugin->initialize(m_sim, m_param);
}
```

### Phase 3: Migrate Plugins Incrementally

Convert one plugin at a time:

1. ✅ **V1 → V3**: `evrard_plugin` (simple 3D case)
2. ✅ **V1 → V3**: `shock_tube_1d` (simple 1D case)
3. ✅ **V1 → V3**: `shock_tube_2d` (moderate complexity)
4. ✅ **V1 → V3**: `shock_tube_3d` (full featured)

### Phase 4: Deprecate Old Interfaces

After all plugins migrated:
- Mark `SimulationPlugin` and `SimulationPluginV2` as `[[deprecated]]`
- Add compiler warnings
- Eventually remove in major version bump

---

## Alternative: Builder Pattern (Intermediate Complexity)

If full V3 is too big a change, consider a **builder-based API**:

```cpp
template<int Dim>
class SimulationPluginV2_5 {
    virtual void initialize(SimulationBuilder<Dim>& builder) = 0;
};

// Framework provides builder that hides system details
template<int Dim>
class SimulationBuilder {
public:
    // Business logic API (clear intent)
    SimulationBuilder& add_particles(std::vector<SPHParticle<Dim>> particles) {
        // Framework handles: sim->particles = move(particles)
        // Framework handles: sim->particle_num = particles.size()
        return *this;
    }
    
    SimulationBuilder& with_parameters(std::shared_ptr<SPHParameters> params) {
        // Framework handles: parameter validation and application
        return *this;
    }
    
    SimulationBuilder& with_boundaries(BoundaryConfiguration<Dim> config) {
        // Framework handles: ghost_manager->initialize(config)
        return *this;
    }
    
private:
    std::shared_ptr<Simulation<Dim>> sim_;
    std::shared_ptr<SPHParameters> params_;
};
```

**Plugin usage:**
```cpp
void initialize(SimulationBuilder<2>& builder) override {
    auto particles = create_shock_tube_particles();
    auto parameters = build_ssph_parameters();
    auto boundaries = configure_periodic_boundaries();
    
    builder
        .add_particles(std::move(particles))
        .with_parameters(std::move(parameters))
        .with_boundaries(std::move(boundaries));
}
```

**Benefits over V2:**
- ✅ No direct `sim.particles()` access
- ✅ No `underlying_simulation()` escape hatch
- ✅ Builder enforces correct initialization order
- ✅ Chainable API (fluent interface)

**Compared to V3:**
- ⚠️ Still has some system coupling (builder reference)
- ✅ Easier migration from V2 (similar structure)
- ⚠️ Less pure than V3 (not just data return)

---

## Recommendations

### Short Term (Next Sprint)

**Option 1: Fix Current V2 with Builder** (Least disruptive)
```cpp
// Update SimulationPhaseView to use builder pattern
template<int Dim>
class UninitializedSimulation {
    // Remove direct access methods
    // std::vector<SPHParticle<Dim>>& particles();  // ❌ Remove
    // void set_particle_num(int);                  // ❌ Remove
    
    // Add builder methods
    UninitializedSimulation& set_particles(std::vector<SPHParticle<Dim>> p) {
        sim_->particles = std::move(p);
        sim_->particle_num = static_cast<int>(sim_->particles.size());
        return *this;
    }
    
    UninitializedSimulation& configure_boundaries(BoundaryConfiguration<Dim> config) {
        sim_->ghost_manager->initialize(std::move(config));
        return *this;
    }
};
```

**Effort:** 🟢 Low (1-2 days)
**Impact:** 🟡 Medium (cleaner API, still some coupling)

### Medium Term (Next Month)

**Option 2: Implement V3 Interface** (Recommended)
- Create `InitialCondition<Dim>` data structure
- Create `SimulationPluginV3<Dim>` interface
- Update `Solver::make_initial_condition()` to handle V3
- Migrate one plugin as proof-of-concept
- Document and provide migration guide

**Effort:** 🟡 Medium (1 week)
**Impact:** 🟢 High (pure business logic, best separation)

### Long Term (Future)

**Option 3: Functional/Declarative API** (Most flexible)
```cpp
auto shock_tube_ic = InitialConditionBuilder<2>()
    .with_domain({-0.5, 0.0}, {1.5, 0.5})
    .with_left_state(dens=1.0, pres=1.0, vel={0,0})
    .with_right_state(dens=0.25, pres=0.1, vel={0,0})
    .with_discontinuity_at(x=0.5)
    .with_resolution(dx=0.025)
    .build();
```

**Effort:** 🔴 High (1 month+)
**Impact:** 🟢 Very High (DSL for initial conditions)

---

## Decision Matrix

| Approach | Effort | Business Logic Purity | Type Safety | Migration Cost |
|----------|--------|----------------------|-------------|----------------|
| **Current V2** | - | ⚠️ Low | ✅ High | - |
| **V2 + Builder** | 🟢 Low | 🟡 Medium | ✅ High | 🟢 Low |
| **V3 (Data Return)** | 🟡 Medium | ✅ Very High | ✅ Very High | 🟡 Medium |
| **Declarative DSL** | 🔴 High | ✅ Very High | ✅ Very High | 🔴 High |

---

## Conclusion

**Recommended Path Forward:**

1. **Immediate (This Week):**
   - Fix current plugin to remove system calls (done ✅)
   - Document the pattern in plugin development guide

2. **Short Term (Next 2 Weeks):**
   - Implement **V3 interface** (`InitialCondition<Dim>` + `SimulationPluginV3`)
   - Migrate shock tube 2D plugin to V3 as proof-of-concept
   - Update documentation with V3 examples

3. **Medium Term (Next Month):**
   - Migrate all shock tube plugins to V3
   - Migrate Evrard plugin to V3
   - Deprecate V1 and V2 interfaces (keep for compatibility)

4. **Long Term (Future):**
   - Consider declarative DSL for common initial condition patterns
   - Add validation and error handling at framework level
   - Potentially generate plugins from configuration files

**Rationale:**
- V3 provides the **cleanest separation** between business logic and system management
- **Data return** pattern is well-established in functional programming
- Makes plugins **easily testable** (just check returned data)
- Prevents **all system coupling issues** at compile time
- Aligns with **modern C++ best practices** (prefer data over side effects)

---

**References:**
- Functional Core, Imperative Shell pattern
- Hexagonal Architecture (Ports and Adapters)
- Separation of Concerns principle
- Dependency Inversion Principle (plugins don't depend on system internals)

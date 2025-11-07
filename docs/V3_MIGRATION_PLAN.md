# V3 Plugin Migration Implementation Plan

## Executive Summary

**Goal:** Remove V1/V2 plugin interfaces, keep ONLY V3 (pure business logic)

**Status:** Infrastructure complete, ready for plugin migration

**Effort Estimate:**  
- Core infrastructure: ✅ **COMPLETE** (4 hours)
- Plugin migration: 🟡 **IN PROGRESS** (12-16 hours for 17 plugins)
- Testing & cleanup: ⏳ **PENDING** (4 hours)

---

## ✅ Phase 1: Infrastructure (COMPLETE)

### Files Created/Modified:

1. **`include/core/plugins/initial_condition.hpp`** ✅
   - `InitialCondition<Dim>` data structure
   - Fluent builder API
   - Validation methods

2. **`include/core/plugins/simulation_plugin_v3.hpp`** ✅
   - Pure business logic interface
   - `create_initial_condition()` method
   - `DEFINE_SIMULATION_PLUGIN_V3` macro

3. **`include/core/plugins/plugin_loader.hpp`** ✅
   - Updated to load V3 plugins only
   - Changed symbol names: `create_plugin_v3`, `destroy_plugin_v3`
   - Type-safe plugin management

4. **`include/solver.hpp`** ✅
   - Updated to use `SimulationPluginV3<Dim>`
   - Removed V1 dependencies

5. **`src/solver.cpp`** ✅
   - `make_initial_condition()` now consumes `InitialCondition` data
   - Framework applies particles, parameters, boundaries automatically
   - No plugin coupling to simulation internals

---

## 🟡 Phase 2: Plugin Migration (IN PROGRESS)

### Migration Pattern

**Before (V1):**
```cpp
class MyPlugin : public SimulationPlugin<Dim> {
    void initialize(shared_ptr<Simulation<Dim>> sim,
                   shared_ptr<SPHParameters> params) override {
        // Create particles
        vector<SPHParticle<Dim>> particles = ...;
        
        // System management (❌ should be framework's job)
        sim->particles = std::move(particles);
        sim->particle_num = num;
        sim->ghost_manager->initialize(config);
        
        // Configure parameters
        *params = *build_parameters();
    }
};

DEFINE_SIMULATION_PLUGIN(MyPlugin, Dim)
```

**After (V3):**
```cpp
class MyPlugin : public SimulationPluginV3<Dim> {
    InitialCondition<Dim> create_initial_condition() const override {
        // Create particles (pure business logic)
        vector<SPHParticle<Dim>> particles = ...;
        
        // Build parameters (pure business logic)
        auto params = build_parameters();
        
        // Configure boundaries (pure business logic)
        auto boundaries = configure_boundaries();
        
        // Return data (framework handles system init)
        return InitialCondition<Dim>::with_particles(std::move(particles))
            .with_parameters(std::move(params))
            .with_boundaries(std::move(boundaries));
    }
};

DEFINE_SIMULATION_PLUGIN_V3(MyPlugin, Dim)
```

### Plugins to Migrate

| Workflow | Plugin File | Status | Priority | Est. Time |
|----------|-------------|--------|----------|-----------|
| **shock_tube_1d** | plugin_baseline.cpp | ⏳ Pending | 🔴 High | 1h |
| shock_tube_1d | plugin_enhanced.cpp | ⏳ Pending | 🔴 High | 1h |
| shock_tube_1d | plugin_modern.cpp | ⏳ Pending | 🔴 High | 1h |
| shock_tube_1d | plugin_disph.cpp | ⏳ Pending | 🟡 Medium | 1h |
| shock_tube_1d | plugin_gsph.cpp | ⏳ Pending | 🟡 Medium | 1h |
| **shock_tube_2d** | plugin_ssph.cpp | ⏳ Pending | 🔴 High | 1h |
| shock_tube_2d | plugin_gsph.cpp | ⏳ Pending | 🟡 Medium | 1h |
| shock_tube_2d | plugin_disph.cpp | ⏳ Pending | 🟡 Medium | 1h |
| shock_tube_2d | plugin_ssph_ghosts.cpp | ⏳ Pending | 🟡 Medium | 1h |
| shock_tube_2d | plugin_2d.cpp | ⏳ Pending | 🟠 Low | 1h |
| **shock_tube_3d** | plugin_3d.cpp | ⏳ Pending | 🔴 High | 1.5h |
| **evrard** | plugin.cpp | ⏳ Pending | 🔴 High | 1.5h |
| pairing_instability | plugin.cpp | ⏳ Pending | 🟠 Low | 1.5h |
| khi | plugin.cpp | ⏳ Pending | 🟠 Low | 1.5h |
| hydrostatic | plugin.cpp | ⏳ Pending | 🟠 Low | 1.5h |
| gresho_chan_vortex | plugin.cpp | ⏳ Pending | 🟠 Low | 1.5h |
| **shock_tube_2d** | plugin_ssph_type_safe.cpp | ❌ Delete | - | - |

**Total:** 16 plugins to migrate, 1 to delete (type_safe example no longer needed)

---

## ⏳ Phase 3: Testing & Cleanup (PENDING)

### 3.1 Build & Test Each Workflow

```bash
# Core workflows (must work!)
cd workflows/shock_tube_workflow/01_simulation && make clean && make build && make run
cd workflows/shock_tube_workflow/02_simulation_2d && make clean && make build && make run
cd workflows/shock_tube_workflow/03_simulation_3d && make clean && make build && make run
cd workflows/evrard_workflow/01_simulation && make clean && make build && make run
```

### 3.2 Delete Old Interface Files

Once all plugins migrated and tested:

```bash
rm include/core/plugins/simulation_plugin.hpp        # V1
rm include/core/plugins/simulation_plugin_v2.hpp     # V2
rm include/core/simulation/simulation_phase_view.hpp # V2 helper
rm include/core/simulation/initialization_phases.hpp # V2 helper
rm workflows/.../plugin_ssph_type_safe.cpp           # V2 example
```

### 3.3 Update Documentation

- `docs/plugin_architecture.md` → Reference V3 only
- `docs/QUICKSTART.md` → V3 examples only
- `docs/TYPE_SAFE_PLUGIN_GUIDE.md` → Archive or update to V3
- `docs/PLUGIN_BUSINESS_LOGIC_DESIGN.md` → Mark as implemented

---

## Implementation Strategy

### Recommended Order:

1. **Start with shock_tube_1d/plugin_baseline.cpp** (simplest)
   - No ghost particles
   - Simple 1D case
   - Good template for others

2. **Migrate shock_tube_1d plugins** (3 plugins)
   - All similar structure
   - Can reuse patterns
   - Build & test together

3. **Migrate shock_tube_2d/plugin_ssph.cpp** (the bug-fix one!)
   - This is the one we just fixed
   - Important to verify V3 works correctly

4. **Migrate remaining shock_tube_2d** (3 more plugins)
   - Similar to 1D
   - More complex (2D)

5. **Migrate shock_tube_3d** (1 plugin)
   - Most complex shock tube

6. **Migrate evrard** (1 plugin)
   - Different physics (gravity)
   - Good test of V3 flexibility

7. **Migrate remaining workflows** (5 plugins)
   - Lower priority
   - Can be done incrementally

### Parallelization

Plugins can be migrated in parallel since they're independent:

- **Stream 1:** shock_tube_1d (3 plugins)
- **Stream 2:** shock_tube_2d (4 plugins)
- **Stream 3:** shock_tube_3d + evrard (2 plugins)
- **Stream 4:** Other workflows (5 plugins)

---

## Migration Checklist (Per Plugin)

```markdown
- [ ] Replace `#include "core/plugins/simulation_plugin.hpp"` with `simulation_plugin_v3.hpp`
- [ ] Change class inheritance: `SimulationPlugin<Dim>` → `SimulationPluginV3<Dim>`
- [ ] Replace `void initialize(...)` with `InitialCondition<Dim> create_initial_condition() const`
- [ ] Remove all `sim->` accesses (particles, particle_num, ghost_manager, etc.)
- [ ] Build parameters using builders (keep as-is)
- [ ] Return `InitialCondition<Dim>::with_particles(...).with_parameters(...).with_boundaries(...)`
- [ ] Replace `DEFINE_SIMULATION_PLUGIN` with `DEFINE_SIMULATION_PLUGIN_V3`
- [ ] Build and verify compilation
- [ ] Run simulation and verify output matches expected behavior
```

---

## Risk Assessment

### High Risk Items:

1. **Boundary configuration changes** 🔴
   - Some plugins use legacy `with_periodic_boundary()`
   - Must ensure correct translation to `BoundaryConfiguration`

2. **Ghost particle expectations** 🟡
   - Plugins that manually generated ghosts need careful review
   - Framework now handles this automatically

3. **Parameter initialization order** 🟡
   - Some plugins may depend on specific initialization order
   - V3 makes order explicit (params → boundaries)

### Mitigation:

- ✅ Test each workflow after migration
- ✅ Compare output with baseline (if available)
- ✅ Keep V1 plugins in git history for rollback
- ✅ Document any behavioral changes

---

## Success Criteria

**Phase 2 Complete When:**
- ✅ All 16 plugins migrated to V3
- ✅ All plugins compile without errors
- ✅ shock_tube_1d runs successfully
- ✅ shock_tube_2d runs successfully  
- ✅ shock_tube_3d runs successfully
- ✅ evrard runs successfully

**Phase 3 Complete When:**
- ✅ All old interface files deleted
- ✅ No references to `SimulationPlugin` (V1) in codebase
- ✅ No references to `SimulationPluginV2` in codebase
- ✅ Documentation updated to V3 only
- ✅ All tests passing

---

## Timeline

**Optimistic:** 2 days (if no issues)  
**Realistic:** 3-4 days (with testing/debugging)  
**Pessimistic:** 5 days (if major issues found)

---

## Next Steps (Immediate)

1. **Migrate shock_tube_1d/plugin_baseline.cpp** (1 hour)
   - Create V3 version
   - Build and test
   - Use as template

2. **Create migration script** (30 min)
   - Automate common transformations
   - Search/replace patterns
   - Reduce manual work

3. **Migrate shock_tube_2d/plugin_ssph.cpp** (1 hour)
   - Verify Bus error: 10 fix still works
   - Confirm V3 prevents the bug

4. **Parallel migration** (8-10 hours)
   - Migrate remaining plugins
   - Test incrementally

5. **Final cleanup** (2 hours)
   - Delete old files
   - Update docs
   - Final testing

---

## Questions for User

1. **Priority:** Should we focus on shock_tube + evrard first (core workflows)?
2. **Testing:** Do you have baseline outputs to compare against?
3. **Other workflows:** Are pairing_instability, khi, hydrostatic, gresho_chan_vortex critical?
4. **Timeline:** Is 3-4 days acceptable or do you need faster turnaround?

---

**Status:** Ready to begin Phase 2 plugin migration  
**Blockers:** None  
**Dependencies:** All infrastructure complete ✅

# Solver Refactoring Plan: Improving Testability and Modularity

## Executive Summary

This document provides a detailed refactoring plan for `solver.cpp` to improve testability, modularity, and maintainability. The analysis is based on the lessons learned from the ghost particle integration bug that caused segmentation faults due to ID mismatch and container inconsistency.

**Key Problems Identified:**
1. **Tight coupling**: Ghost management, tree coordination, and integration logic are intertwined
2. **Large methods**: `initialize()` (100+ lines) and `integrate()` (60+ lines) have multiple responsibilities
3. **Hidden dependencies**: Ghost-tree-container synchronization is implicit and error-prone
4. **Difficult to test**: Cannot unit test ghost generation, tree rebuilding, or container management in isolation
5. **State management complexity**: Temporal state (real vs predicted), spatial state (particles vs cached_search_particles), boundary state (real vs ghost) are entangled

**Recommended Approach:**
- **Phase 1**: Extract coordinators for ghost lifecycle and tree management (low risk, high value)
- **Phase 2**: Separate integration orchestration from physics calculations (medium risk)
- **Phase 3**: Introduce state objects for particle containers (optional, high effort)

---

## 1. Current Architecture Analysis

### 1.1 Responsibility Mapping

**`Solver` currently handles:**
- Command-line argument parsing and validation
- Plugin loading and lifecycle
- Parameter configuration logging
- Initial condition setup
- SPH algorithm selection (SSPH, DISPH, GSPH)
- Ghost particle generation and updates
- Tree construction and rebuild timing
- Particle container synchronization (particles → cached_search_particles)
- Time integration orchestration (predict → calculate → correct)
- Output generation timing
- Simulation loop control

**Problems:**
- Violates Single Responsibility Principle (10+ distinct responsibilities)
- Ghost/tree coordination logic duplicated between `initialize()` and `integrate()`
- Container synchronization logic (lines 406-422) is critical but buried in `integrate()`
- Cannot test ghost generation without full solver initialization
- Cannot test tree rebuild timing without running integration

### 1.2 Critical Code Segments Requiring Isolation

#### A. Ghost Particle Lifecycle (Lines 359-391 in `initialize()`, 392-404 in `integrate()`)

**Current Pattern:**
```cpp
// In initialize():
if (m_sim->ghost_manager && m_sim->ghost_manager->get_config().is_valid) {
    real max_sml = 0.0;
    for (int i = 0; i < num; ++i) {
        if (p[i].sml > max_sml) max_sml = p[i].sml;
    }
    const real kernel_support = 2.0 * max_sml;
    m_sim->ghost_manager->set_kernel_support_radius(kernel_support);
    m_sim->ghost_manager->generate_ghosts(p);
    
    // Container synchronization
    const auto all_particles_combined = m_sim->get_all_particles_for_search();
    m_sim->cached_search_particles.reserve(all_particles_combined.size());
    m_sim->cached_search_particles = all_particles_combined;
    
    m_sim->make_tree();
}

// In integrate():
if (m_sim->ghost_manager) {
    real max_sml = 0.0;
    for (int i = 0; i < m_sim->particle_num; ++i) {
        if (m_sim->particles[i].sml > max_sml) max_sml = m_sim->particles[i].sml;
    }
    const real kernel_support = 2.0 * max_sml;
    m_sim->ghost_manager->set_kernel_support_radius(kernel_support);
    m_sim->ghost_manager->update_ghosts(m_sim->particles);
}
```

**Issues:**
- Kernel support calculation duplicated (DRY violation)
- Implicit ordering: must happen after smoothing length calculation
- Container sync in `initialize()` but not in `integrate()` (handled later)
- Cannot test "what if smoothing length changes drastically?" without full integration

#### B. Tree Rebuild Coordination (Lines 385-391, 418-422)

**Current Pattern:**
```cpp
// After ghost generation in initialize():
m_sim->make_tree();

// After container update in integrate():
for (auto& p : m_sim->cached_search_particles) {
    p.next = nullptr;  // Clear stale linked-list pointers
}
m_sim->make_tree();
```

**Issues:**
- Tree rebuild is consequence of container change, but link is implicit
- `p.next = nullptr` is critical but not obviously related to tree building
- Cannot verify "tree was rebuilt" without inspecting tree internals
- Timing constraint (must rebuild AFTER ghost update, BEFORE pre-interaction) is implicit

#### C. Container Synchronization (Lines 406-416)

**Current Pattern:**
```cpp
const auto all_particles = m_sim->get_all_particles_for_search();
const size_t new_size = all_particles.size();

if (m_sim->cached_search_particles.capacity() < new_size) {
    m_sim->cached_search_particles.reserve(new_size + 100);  // Magic number
}
m_sim->cached_search_particles.resize(new_size);
std::copy(all_particles.begin(), all_particles.end(), m_sim->cached_search_particles.begin());
```

**Issues:**
- Magic number `100` for buffer size (should be constant or configurable)
- Manual memory management (reserve/resize/copy) is error-prone
- Reallocation avoidance is critical (tree pointer invalidation) but not enforced
- Cannot test "container sync without reallocation" in isolation

---

## 2. Proposed Refactoring Strategy

### Phase 1: Extract Coordinators (Low Risk, High Value)

Create two new classes to encapsulate cross-cutting concerns:

#### 2.1 `GhostParticleCoordinator`

**Responsibility:** Orchestrate ghost particle lifecycle in sync with simulation state

**Interface:**
```cpp
template<int Dim>
class GhostParticleCoordinator {
public:
    explicit GhostParticleCoordinator(std::shared_ptr<Simulation<Dim>> sim);
    
    // Initialize ghosts after smoothing lengths are calculated
    void initialize_ghosts(const std::vector<SPHParticle<Dim>>& real_particles);
    
    // Update ghosts during time integration
    void update_ghosts(const std::vector<SPHParticle<Dim>>& real_particles);
    
    // Query state
    bool has_ghosts() const;
    size_t ghost_count() const;
    real get_kernel_support_radius() const;
    
private:
    real calculate_kernel_support(const std::vector<SPHParticle<Dim>>& particles) const;
    void log_ghost_state(const std::string& context) const;
    
    std::shared_ptr<Simulation<Dim>> m_sim;
    real m_last_kernel_support{0.0};
};
```

**Benefits:**
- Single location for kernel support calculation
- Testable in isolation (mock Simulation)
- Clear contract: always updates kernel support before generate/update
- Logging and diagnostics centralized

**Implementation Pattern:**
```cpp
void GhostParticleCoordinator<Dim>::initialize_ghosts(
    const std::vector<SPHParticle<Dim>>& real_particles)
{
    if (!m_sim->ghost_manager || !m_sim->ghost_manager->get_config().is_valid) {
        return;  // Early exit if ghosts disabled
    }
    
    m_last_kernel_support = calculate_kernel_support(real_particles);
    m_sim->ghost_manager->set_kernel_support_radius(m_last_kernel_support);
    m_sim->ghost_manager->generate_ghosts(real_particles);
    
    log_ghost_state("initialize_ghosts");
}

real GhostParticleCoordinator<Dim>::calculate_kernel_support(
    const std::vector<SPHParticle<Dim>>& particles) const
{
    real max_sml = 0.0;
    for (const auto& p : particles) {
        if (p.sml > max_sml) {
            max_sml = p.sml;
        }
    }
    
    // For cubic spline kernel, support is 2h
    constexpr real CUBIC_SPLINE_SUPPORT = 2.0;
    return CUBIC_SPLINE_SUPPORT * max_sml;
}
```

**Testing Strategy:**
```cpp
TEST(GhostParticleCoordinator, GivenParticlesWithVaryingSML_WhenInitialize_ThenUsesMaximum) {
    auto sim = create_mock_simulation();
    GhostParticleCoordinator<3> coordinator(sim);
    
    std::vector<SPHParticle<3>> particles = {
        make_particle_with_sml(0.1),
        make_particle_with_sml(0.3),  // Maximum
        make_particle_with_sml(0.2)
    };
    
    coordinator.initialize_ghosts(particles);
    
    EXPECT_DOUBLE_EQ(coordinator.get_kernel_support_radius(), 0.6);  // 2.0 * 0.3
}
```

#### 2.2 `SpatialTreeCoordinator`

**Responsibility:** Manage tree lifecycle and container consistency

**Interface:**
```cpp
template<int Dim>
class SpatialTreeCoordinator {
public:
    explicit SpatialTreeCoordinator(std::shared_ptr<Simulation<Dim>> sim);
    
    // Rebuild tree with combined real+ghost particles
    // Handles container sync, pointer invalidation, and tree construction
    void rebuild_tree_for_neighbor_search();
    
    // Query state for validation
    size_t get_search_particle_count() const;
    bool is_tree_consistent() const;
    
private:
    void synchronize_search_container();
    void clear_linked_list_pointers();
    void rebuild_spatial_tree();
    
    std::shared_ptr<Simulation<Dim>> m_sim;
    
    // Configuration
    static constexpr size_t REALLOCATION_BUFFER = 100;
};
```

**Benefits:**
- Encapsulates complex 4-step process (sync → clear → validate → rebuild)
- Testable invariant: "tree always built with same container used for search"
- Clear reallocation avoidance strategy
- Single point to add metrics (tree depth, particle distribution, etc.)

**Implementation Pattern:**
```cpp
void SpatialTreeCoordinator<Dim>::rebuild_tree_for_neighbor_search()
{
    synchronize_search_container();
    clear_linked_list_pointers();
    rebuild_spatial_tree();
}

void SpatialTreeCoordinator<Dim>::synchronize_search_container()
{
    const auto all_particles = m_sim->get_all_particles_for_search();
    const size_t new_size = all_particles.size();
    
    // CRITICAL: Avoid reallocation to prevent tree pointer invalidation
    if (m_sim->cached_search_particles.capacity() < new_size) {
        m_sim->cached_search_particles.reserve(new_size + REALLOCATION_BUFFER);
    }
    
    m_sim->cached_search_particles.resize(new_size);
    std::copy(all_particles.begin(), all_particles.end(), 
              m_sim->cached_search_particles.begin());
}

void SpatialTreeCoordinator<Dim>::clear_linked_list_pointers()
{
    // Tree builder modifies particle.next for linked lists
    // Must clear stale pointers from previous iterations
    for (auto& p : m_sim->cached_search_particles) {
        p.next = nullptr;
    }
}

bool SpatialTreeCoordinator<Dim>::is_tree_consistent() const
{
    // Validate tree was built with current cached_search_particles
    auto* tree = m_sim->tree.get();
    if (!tree) return false;
    
    // Check tree's stored particle pointer matches our container
    // (Requires exposing m_particles_ptr or adding validation to BHTree)
    return true;  // Placeholder
}
```

**Testing Strategy:**
```cpp
TEST(SpatialTreeCoordinator, GivenContainerGrows_WhenRebuild_ThenAvoidsReallocation) {
    auto sim = create_mock_simulation_with_capacity(100);
    SpatialTreeCoordinator<3> coordinator(sim);
    
    // Add ghosts to grow container from 50 → 150
    sim->ghosts.resize(100);
    
    const void* old_data_ptr = sim->cached_search_particles.data();
    coordinator.rebuild_tree_for_neighbor_search();
    const void* new_data_ptr = sim->cached_search_particles.data();
    
    EXPECT_EQ(old_data_ptr, new_data_ptr) << "Container should not reallocate";
}
```

#### 2.3 Updated `Solver::initialize()` and `Solver::integrate()`

**After Phase 1 refactoring:**

```cpp
void Solver::initialize()
{
    m_sim = std::make_shared<Simulation<DIM>>(m_param);
    make_initial_condition();
    
    // Create coordinators
    m_ghost_coordinator = std::make_shared<GhostParticleCoordinator<DIM>>(m_sim);
    m_tree_coordinator = std::make_shared<SpatialTreeCoordinator<DIM>>(m_sim);
    
    // ... (algorithm selection, parameter setup - unchanged) ...
    
#ifndef EXHAUSTIVE_SEARCH_ONLY_FOR_DEBUG
    auto tree = m_sim->tree;
    tree->resize(num);
    
    // Initial tree build (real particles only, before ghosts)
    m_sim->cached_search_particles.clear();
    m_sim->cached_search_particles.resize(num);
    for (int i = 0; i < num; ++i) {
        m_sim->cached_search_particles[i] = p[i];
    }
    tree->make(p, num);
#endif

    m_pre->calculation(m_sim);
    
    // Generate ghosts AFTER smoothing lengths calculated
    m_ghost_coordinator->initialize_ghosts(p);
    
    // Rebuild tree with real + ghost
    m_tree_coordinator->rebuild_tree_for_neighbor_search();
    
    m_fforce->calculation(m_sim);
    m_gforce->calculation(m_sim);
}

void Solver::integrate()
{
    // Validate state
    if (m_sim->particles.size() != static_cast<size_t>(m_sim->particle_num)) {
        throw std::runtime_error("Particle vector corrupted");
    }
    
    // Update ghosts based on current particle state
    m_ghost_coordinator->update_ghosts(m_sim->particles);
    
    m_timestep->calculation(m_sim);
    predict();
    
    // Rebuild tree with updated ghost positions
    m_tree_coordinator->rebuild_tree_for_neighbor_search();
    
    m_pre->calculation(m_sim);
    m_fforce->calculation(m_sim);
    m_gforce->calculation(m_sim);
    
    correct();
}
```

**Code Reduction:**
- `initialize()`: 100 lines → 70 lines (-30%)
- `integrate()`: 60 lines → 20 lines (-67%)
- Complexity extracted to testable coordinators

---

### Phase 2: Separate Integration Orchestration (Medium Risk)

Create `IntegrationOrchestrator` to manage the predict → calculate → correct workflow.

#### 2.1 `IntegrationOrchestrator`

**Responsibility:** Orchestrate one time integration step

**Interface:**
```cpp
template<int Dim>
class IntegrationOrchestrator {
public:
    struct Dependencies {
        std::shared_ptr<TimeStep<Dim>> timestep;
        std::shared_ptr<PreInteraction<Dim>> pre_interaction;
        std::shared_ptr<FluidForce<Dim>> fluid_force;
        std::shared_ptr<GravityForce<Dim>> gravity_force;
        std::shared_ptr<GhostParticleCoordinator<Dim>> ghost_coordinator;
        std::shared_ptr<SpatialTreeCoordinator<Dim>> tree_coordinator;
    };
    
    explicit IntegrationOrchestrator(
        std::shared_ptr<Simulation<Dim>> sim,
        std::shared_ptr<SPHParameters> param,
        Dependencies deps);
    
    // Execute one full integration step
    void integrate_one_step();
    
    // Individual phases (for testing)
    void update_boundary_conditions();
    void calculate_timestep();
    void predict_half_step();
    void update_spatial_tree();
    void calculate_forces();
    void correct_full_step();
    
private:
    std::shared_ptr<Simulation<Dim>> m_sim;
    std::shared_ptr<SPHParameters> m_param;
    Dependencies m_deps;
};
```

**Benefits:**
- Clear separation: Solver = setup + loop control, Orchestrator = single step
- Each phase testable independently
- Dependencies explicit (no hidden coupling)
- Easy to add profiling per phase

**Implementation:**
```cpp
void IntegrationOrchestrator<Dim>::integrate_one_step()
{
    update_boundary_conditions();
    calculate_timestep();
    predict_half_step();
    update_spatial_tree();
    calculate_forces();
    correct_full_step();
}

void IntegrationOrchestrator<Dim>::update_boundary_conditions()
{
    m_deps.ghost_coordinator->update_ghosts(m_sim->particles);
}

void IntegrationOrchestrator<Dim>::update_spatial_tree()
{
    m_deps.tree_coordinator->rebuild_tree_for_neighbor_search();
}
```

**Updated `Solver::integrate()`:**
```cpp
void Solver::integrate()
{
    m_integration_orchestrator->integrate_one_step();
}
```

---

### Phase 3: Introduce State Objects (Optional, High Effort)

For maximum testability and clarity, introduce value objects for particle state.

#### 3.1 `ParticleContainerState`

**Responsibility:** Encapsulate container invariants

```cpp
template<int Dim>
class ParticleContainerState {
public:
    // Factory methods enforcing invariants
    static ParticleContainerState create_from_real_particles(
        const std::vector<SPHParticle<Dim>>& real_particles);
    
    static ParticleContainerState create_with_ghosts(
        const std::vector<SPHParticle<Dim>>& real_particles,
        const std::vector<SPHParticle<Dim>>& ghost_particles);
    
    // Access with invariant checking
    const std::vector<SPHParticle<Dim>>& get_search_particles() const;
    const std::vector<SPHParticle<Dim>>& get_real_particles() const;
    size_t real_count() const;
    size_t ghost_count() const;
    
    // Validation
    bool validate_id_consistency() const;
    
private:
    std::vector<SPHParticle<Dim>> m_search_particles;  // real + ghost
    size_t m_real_count;
    
    // Private constructor - use factories
    ParticleContainerState(
        std::vector<SPHParticle<Dim>> search_particles,
        size_t real_count);
    
    void enforce_id_equals_index();
};
```

**Benefits:**
- Impossible to create invalid state (ID != index)
- Clear ownership of "combined particle list" concept
- Validation centralized
- Immutable after creation (thread-safe)

**Tradeoffs:**
- High refactoring effort (Simulation stores this instead of vectors)
- Requires rethinking particle updates (copy-on-write vs mutation)
- May impact performance if not optimized

---

## 3. Migration Strategy

### 3.1 Phase 1 Implementation Steps

**Step 1.1: Create `GhostParticleCoordinator` (1-2 days)**
- [ ] Create header `include/core/ghost_particle_coordinator.hpp`
- [ ] Create implementation `include/core/ghost_particle_coordinator.tpp`
- [ ] Write unit tests `tests/core/ghost_particle_coordinator_test.cpp`
- [ ] Extract kernel support calculation from Solver
- [ ] Extract ghost initialization logic
- [ ] Extract ghost update logic
- [ ] Verify tests pass

**Step 1.2: Create `SpatialTreeCoordinator` (1-2 days)**
- [ ] Create header `include/core/spatial_tree_coordinator.hpp`
- [ ] Create implementation `include/core/spatial_tree_coordinator.tpp`
- [ ] Write unit tests `tests/core/spatial_tree_coordinator_test.cpp`
- [ ] Extract container synchronization logic
- [ ] Extract linked-list pointer clearing
- [ ] Extract tree rebuild
- [ ] Add consistency validation
- [ ] Verify tests pass

**Step 1.3: Integrate coordinators into Solver (1 day)**
- [ ] Update `include/solver.hpp` to include coordinators as members
- [ ] Update `Solver::initialize()` to use coordinators
- [ ] Update `Solver::integrate()` to use coordinators
- [ ] Run full simulation suite to verify no regressions
- [ ] Update existing documentation

**Step 1.4: Create integration tests (1 day)**
- [ ] Test coordinator interaction in `tests/integration/coordinator_integration_test.cpp`
- [ ] Verify ghost generation → tree rebuild sequence
- [ ] Verify container capacity management
- [ ] Verify ID consistency throughout lifecycle

**Total for Phase 1: 5-6 days**

### 3.2 Phase 2 Implementation Steps (Optional)

**Step 2.1: Create `IntegrationOrchestrator` (2-3 days)**
- [ ] Create header and implementation
- [ ] Extract predict/correct logic
- [ ] Write unit tests for each phase
- [ ] Integrate into Solver
- [ ] Verify no regressions

**Total for Phase 2: 2-3 days**

### 3.3 Backward Compatibility

**Guarantees:**
- All existing simulations run without changes
- Performance characteristics unchanged
- Plugin API unchanged
- Output format unchanged

**Validation:**
- Run all existing scenarios (shock tube, dam break, etc.)
- Compare output files byte-for-byte with reference
- Profile to ensure no performance regression (< 5% acceptable)

---

## 4. Testing Strategy

### 4.1 Unit Tests (New)

**`GhostParticleCoordinator` tests:**
- Kernel support calculation with varying smoothing lengths
- Ghost generation timing (must happen after sml calculation)
- Ghost update with particle motion
- Behavior when ghost_manager is null
- Behavior when ghosts disabled in config

**`SpatialTreeCoordinator` tests:**
- Container synchronization without reallocation
- Container growth with reallocation
- Linked-list pointer clearing
- Tree consistency validation
- Empty particle list edge case

**`IntegrationOrchestrator` tests (Phase 2):**
- Phase execution order
- Dependencies are used correctly
- Error propagation

### 4.2 Integration Tests (Enhanced)

**Coordinator integration:**
- Ghost generation → tree rebuild → neighbor search validity
- Multiple ghost updates without ID corruption
- Parallel tree queries during integration
- Ghost count fluctuation handling

**Full solver lifecycle:**
- Initialize → integrate N steps → validate state
- Sudden ghost count jump scenarios
- Boundary particle motion triggering ghost changes

### 4.3 BDD Scenario Tests (New)

```gherkin
Feature: Ghost Particle Lifecycle Management
  
  Scenario: Initial ghost generation
    Given a simulation with boundary conditions configured
    And real particles with calculated smoothing lengths
    When initialize_ghosts is called
    Then ghost particles are generated
    And ghost IDs equal their index in search container
    And spatial tree is rebuilt with combined particles
  
  Scenario: Ghost update during integration
    Given a simulation mid-timestep
    And particle positions have changed
    When update_ghosts is called
    Then ghosts reflect new boundary conditions
    And ghost IDs remain consistent with indices
    And spatial tree is rebuilt before force calculation
```

### 4.4 Regression Tests

**Prevent recurrence of original bug:**
- Test case with exact scenario that caused segfault
- Validate no "invalid index" warnings
- Validate all neighbor indices < particle count
- Validate ghost IDs match their position

---

## 5. Code Quality Improvements

### 5.1 Magic Numbers → Named Constants

**Current:**
```cpp
if (m_sim->cached_search_particles.capacity() < new_size) {
    m_sim->cached_search_particles.reserve(new_size + 100);  // Magic!
}

const real kernel_support = 2.0 * max_sml;  // Why 2.0?
```

**After:**
```cpp
// In SpatialTreeCoordinator
static constexpr size_t REALLOCATION_BUFFER = 100;

// In GhostParticleCoordinator
static constexpr real CUBIC_SPLINE_SUPPORT_RADIUS = 2.0;
```

### 5.2 Implicit Ordering → Explicit Dependencies

**Current problem:** `initialize()` must call operations in strict order, but order is implicit:

```cpp
m_pre->calculation(m_sim);              // Must be before ghost generation
m_ghost_coordinator->initialize_ghosts(p);  // Must be after pre-interaction
m_tree_coordinator->rebuild_tree();     // Must be after ghost generation
m_fforce->calculation(m_sim);          // Must be after tree rebuild
```

**Solution:** Add validation to coordinators:

```cpp
void GhostParticleCoordinator::initialize_ghosts(...) {
    // Validate precondition
    for (const auto& p : real_particles) {
        if (p.sml <= 0.0) {
            throw std::logic_error(
                "Ghost generation requires valid smoothing lengths. "
                "Call pre-interaction first to calculate sml.");
        }
    }
    // ... rest of implementation
}
```

### 5.3 Error Messages → Actionable Diagnostics

**Current:**
```cpp
throw std::runtime_error("Particle vector corrupted");
```

**After:**
```cpp
std::ostringstream msg;
msg << "Particle vector size mismatch:\n"
    << "  particles.size()   = " << m_sim->particles.size() << "\n"
    << "  particle_num       = " << m_sim->particle_num << "\n"
    << "  Expected: size == particle_num\n"
    << "  Possible causes:\n"
    << "    - Ghost particles incorrectly appended to particles vector\n"
    << "    - particle_num not updated after particle addition/removal\n"
    << "  Solution: Ensure ghosts are stored in separate container";
throw std::runtime_error(msg.str());
```

---

## 6. Performance Considerations

### 6.1 Expected Impact

**Phase 1 (Coordinators):**
- Runtime overhead: < 1% (mostly function call overhead)
- Memory overhead: Negligible (2 shared_ptr members in Solver)
- Compilation time: +5-10% (two new template classes)

**Phase 2 (Orchestrator):**
- Runtime overhead: < 0.5% (function call indirection)
- Memory overhead: Minimal (one shared_ptr + Dependencies struct)

**Phase 3 (State Objects):**
- Runtime overhead: -5% to +10% (depends on optimization)
  - Potential gains: better cache locality, fewer allocations
  - Potential cost: copy overhead if not move-optimized
- Memory overhead: +10-20% (duplicate containers if using COW)

### 6.2 Optimization Strategies

**Inline hot paths:**
```cpp
// In coordinator headers
template<int Dim>
inline real GhostParticleCoordinator<Dim>::get_kernel_support_radius() const {
    return m_last_kernel_support;
}
```

**Reserve capacity upfront:**
```cpp
// In SpatialTreeCoordinator constructor
m_sim->cached_search_particles.reserve(
    estimate_max_particle_count(m_sim->particle_num));
```

**Profile-guided optimization:**
- Run profiler on refactored code
- Identify hot spots
- Apply targeted optimizations
- Re-measure to confirm improvement

---

## 7. Documentation Updates

### 7.1 Architecture Documentation

Update `BHTREE_GHOST_ARCHITECTURE.md`:
- Add section on coordinators
- Update workflow diagrams to show coordinator boundaries
- Document coordinator responsibilities

### 7.2 API Documentation

Create `COORDINATOR_API.md`:
- Purpose and responsibility of each coordinator
- Usage examples
- Common pitfalls and solutions
- Performance characteristics

### 7.3 Migration Guide

Create `MIGRATION_GUIDE.md` for developers:
- How to use coordinators in new scenarios
- How to extend coordinator functionality
- Testing requirements

---

## 8. Success Criteria

### 8.1 Testability Metrics

**Before refactoring:**
- Lines of code testable in isolation: ~20% (only physics kernels)
- Test coverage of integration logic: 0%
- Time to write new scenario test: 30-60 minutes (requires full setup)

**After Phase 1:**
- Lines of code testable in isolation: ~60% (coordinators + kernels)
- Test coverage of integration logic: >80%
- Time to write new scenario test: 10-15 minutes (mock coordinators)

### 8.2 Maintainability Metrics

**Before refactoring:**
- Cyclomatic complexity of `Solver::integrate()`: 12
- Lines of code in largest method: 100+ (`initialize()`)
- Responsibilities per class: 10+ (Solver)

**After Phase 1:**
- Cyclomatic complexity of `Solver::integrate()`: < 5
- Lines of code in largest method: < 50
- Responsibilities per class: < 5

### 8.3 Regression Prevention

**Required tests to prevent original bug:**
- [x] BDD test: `GivenGhostParticlesWithSourceIDs_WhenNeighborSearch_ThenNoIndexOutOfBounds`
- [x] BDD test: `GivenTreeBuiltWithOneContainer_WhenSearchWithDifferentContainer_ThenUsesOriginalContainer`
- [ ] Unit test: `GhostCoordinator_AlwaysRenumbersGhostIDsToMatchIndex`
- [ ] Unit test: `TreeCoordinator_RebuildUsesCurrentSearchContainer`
- [ ] Integration test: `FullIntegrationCycle_MaintainsIDConsistency`

---

## 9. Recommendation

**Proceed with Phase 1 immediately.**

**Rationale:**
1. **High value, low risk**: Extracting coordinators has minimal impact on existing code paths
2. **Addresses root cause**: Original bug was coordination failure between ghosts and tree
3. **Enables comprehensive testing**: Can now test ghost and tree logic in isolation
4. **Improves readability**: Solver becomes orchestration layer, not implementation
5. **Quick wins**: 5-6 day effort for 60% testability improvement

**Defer Phase 2 until after Phase 1 validation:**
- Evaluate Phase 1 impact on test suite execution time
- Gather team feedback on coordinator pattern
- Decide if additional orchestration abstraction adds value

**Postpone Phase 3 indefinitely:**
- State objects require major architecture changes
- Value proposition unclear (complexity vs benefit)
- Revisit only if performance profiling shows cache issues

---

## 10. Next Steps

1. **Review this document** with team (1 hour meeting)
2. **Approve Phase 1 plan** and allocate 5-6 days
3. **Create GitHub issues** for each step in Phase 1
4. **Implement coordinators** with TDD approach (tests first)
5. **Run regression suite** after each step
6. **Update documentation** as coordinators are integrated
7. **Retrospective** after Phase 1 to evaluate Phase 2

---

## Appendix A: File Structure After Phase 1

```
include/
  core/
    ghost_particle_coordinator.hpp          # NEW
    ghost_particle_coordinator.tpp          # NEW
    spatial_tree_coordinator.hpp            # NEW
    spatial_tree_coordinator.tpp            # NEW
    bhtree.hpp                              # UNCHANGED
    bhtree.tpp                              # UNCHANGED
    simulation.hpp                          # UNCHANGED
    simulation.tpp                          # UNCHANGED
  solver.hpp                                # MODIFIED (add coordinator members)

src/
  solver.cpp                                # MODIFIED (use coordinators)

tests/
  core/
    ghost_particle_coordinator_test.cpp     # NEW
    spatial_tree_coordinator_test.cpp       # NEW
    bhtree_ghost_integration_test.cpp       # EXISTING
    solver_ghost_lifecycle_test.cpp         # EXISTING
  integration/
    coordinator_integration_test.cpp        # NEW

docs/
  BHTREE_GHOST_ARCHITECTURE.md             # MODIFIED (add coordinators)
  COORDINATOR_API.md                        # NEW
  SOLVER_REFACTORING_PLAN.md               # THIS DOCUMENT
```

---

## Appendix B: Estimated Line Count Changes

| File | Current LOC | Phase 1 LOC | Change |
|------|-------------|-------------|---------|
| `solver.cpp` | 580 | 450 | -130 (-22%) |
| `ghost_particle_coordinator.tpp` | 0 | 120 | +120 |
| `spatial_tree_coordinator.tpp` | 0 | 150 | +150 |
| Test files | 500 | 800 | +300 (+60%) |
| **Total production code** | 580 | 720 | +140 (+24%) |
| **Total test code** | 500 | 800 | +300 (+60%) |

**Net effect:** +24% production code, +60% test coverage, -67% complexity in `Solver::integrate()`

---

## Appendix C: Key Invariants Enforced by Coordinators

**`GhostParticleCoordinator` enforces:**
1. Kernel support radius always calculated before ghost generation
2. Ghost generation only happens if smoothing lengths are valid (> 0)
3. Ghost updates use current particle positions
4. Logging of ghost state changes for diagnostics

**`SpatialTreeCoordinator` enforces:**
1. Tree always built with `cached_search_particles` container
2. Container synchronization never triggers reallocation (reserves buffer)
3. Linked-list pointers cleared before tree build
4. ID consistency: particle.id == index for all particles in search container

**Together they enforce:**
1. Tree rebuild ALWAYS follows ghost update
2. Tree rebuild ALWAYS happens before force calculation
3. Container passed to tree.make() is same container used by tree.neighbor_search()

---

**End of Refactoring Plan**

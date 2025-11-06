# Type-Safe Neighbor Access - Implementation Checklist

**Goal:** Prevent array index mismatch bugs through compile-time type safety  
**Date Started:** 2025-11-06  
**Estimated Effort:** 2-3 days for full implementation

---

## Phase 1: Foundation (Day 1 - Morning)

### ☐ Task 1.1: Create Strong Type Foundations
**File:** `include/core/neighbors/particle_array_types.hpp`

**Acceptance Criteria:**
- [ ] Create `RealParticlesTag` and `SearchParticlesTag` structs
- [ ] Implement `TypedParticleArray<Dim, Tag>` template class
- [ ] Private `operator[]` only accessible by `NeighborAccessor`
- [ ] Delete assignment operator to prevent accidental copying
- [ ] Type aliases `RealParticleArray<Dim>` and `SearchParticleArray<Dim>`
- [ ] Compiles without warnings with `-Wall -Wextra -Wpedantic`
- [ ] Add header guard and include protection

**Definition of Done:**
```cpp
SearchParticleArray<2> arr1{vec};  // ✅ Compiles
RealParticleArray<2> arr2{vec};    // ✅ Compiles
// But arr1 and arr2 are incompatible types
```

**Estimated Time:** 30 minutes

---

### ☐ Task 1.2: Create NeighborIndex Strong Type
**File:** `include/core/neighbors/neighbor_accessor.hpp`

**Acceptance Criteria:**
- [ ] `struct NeighborIndex` with explicit constructor
- [ ] Delete implicit conversions from float/double
- [ ] Explicit `operator()` to get int value
- [ ] Cannot be assigned from raw int without explicit construction
- [ ] Unit test verifies compile-time safety

**Definition of Done:**
```cpp
NeighborIndex idx{5};        // ✅ OK
int val = idx();             // ✅ OK
NeighborIndex bad = 5;       // ❌ Compile error
```

**Estimated Time:** 20 minutes

---

### ☐ Task 1.3: Implement NeighborAccessor Class
**File:** `include/core/neighbors/neighbor_accessor.hpp`

**Acceptance Criteria:**
- [ ] Template class `NeighborAccessor<Dim>`
- [ ] Constructor accepts only `SearchParticleArray<Dim>`
- [ ] Delete constructor for `RealParticleArray<Dim>`
- [ ] `get_neighbor(NeighborIndex)` returns `const SPHParticle<Dim>&`
- [ ] Debug builds: bounds checking with exception
- [ ] `particle_count()` returns array size
- [ ] Friend access to `TypedParticleArray`

**Definition of Done:**
```cpp
NeighborAccessor<2> acc{search_array};  // ✅ Compiles
NeighborAccessor<2> bad{real_array};    // ❌ Compile error
const auto& p = acc.get_neighbor(NeighborIndex{5});  // ✅ OK
```

**Estimated Time:** 45 minutes

---

### ☐ Task 1.4: Write Unit Tests for NeighborAccessor
**File:** `tests/unit/test_neighbor_accessor.cpp`

**Test Cases (Given-When-Then):**
1. [ ] `GivenSearchParticleArray_WhenConstruct_ThenSucceeds`
2. [ ] `GivenRealParticleArray_WhenConstruct_ThenDoesNotCompile` (compile test)
3. [ ] `GivenValidIndex_WhenGetNeighbor_ThenReturnsCorrectParticle`
4. [ ] `GivenGhostParticleIndex_WhenGetNeighbor_ThenReturnsGhost`
5. [ ] `GivenOutOfBoundsIndex_WhenDebugBuild_ThenThrows`
6. [ ] `GivenNeighborIndex_WhenImplicitIntConversion_ThenDoesNotCompile` (compile test)

**Definition of Done:**
- [ ] All tests pass
- [ ] Code coverage ≥ 90% for NeighborAccessor
- [ ] Tests run in CI pipeline
- [ ] Compile-time tests documented

**Estimated Time:** 1 hour

---

## Phase 2: C++20 Concepts (Day 1 - Afternoon)

### ☐ Task 2.1: Create Neighbor Concepts
**File:** `include/core/neighbors/neighbor_concepts.hpp`

**Acceptance Criteria:**
- [ ] Concept `NeighborProvider<T, Dim>` requires `get_neighbor()` and `particle_count()`
- [ ] Concept `NeighborSearchResultType<T>` requires iteration and `NeighborIndex` elements
- [ ] Example constrained template function `process_neighbors<>`
- [ ] Compiles with C++20 standard (`-std=c++20`)

**Definition of Done:**
```cpp
template<int Dim, NeighborProvider<Dim> Accessor>
void func(Accessor& acc) { /* ... */ }  // ✅ Only accepts valid accessors
```

**Estimated Time:** 45 minutes

---

### ☐ Task 2.2: Enhance NeighborSearchResult
**File:** `include/core/spatial/neighbor_search_result.hpp`

**Acceptance Criteria:**
- [ ] Add `NeighborIndexIterator` class
- [ ] Iterator returns `NeighborIndex` not raw `int`
- [ ] `begin()` and `end()` return type-safe iterators
- [ ] Range-based for loop compatible
- [ ] Satisfies `NeighborSearchResultType` concept

**Definition of Done:**
```cpp
for (auto neighbor_idx : result) {  // neighbor_idx is NeighborIndex
    const auto& p = accessor.get_neighbor(neighbor_idx);
}
```

**Estimated Time:** 1 hour

---

### ☐ Task 2.3: Update Simulation Class Interface
**File:** `include/core/simulation/simulation.hpp`

**Acceptance Criteria:**
- [ ] Add `get_real_particles()` returning `RealParticleArray<Dim>`
- [ ] Add `get_search_particles()` returning `SearchParticleArray<Dim>`
- [ ] Add `create_neighbor_accessor()` returning `NeighborAccessor<Dim>`
- [ ] Add `validate_particle_arrays()` with debug-build runtime check
- [ ] Static assertions for particle array types
- [ ] No breaking changes to existing code

**Definition of Done:**
```cpp
auto accessor = sim->create_neighbor_accessor();  // Type-safe
sim->validate_particle_arrays();  // Debug assertion
```

**Estimated Time:** 30 minutes

---

## Phase 3: Refactor SPH Methods (Day 2)

### ☐ Task 3.1: Refactor DISPH PreInteraction
**File:** `include/disph/d_pre_interaction.tpp`

**Acceptance Criteria:**
- [ ] Remove `auto & search_particles = sim->cached_search_particles;`
- [ ] Replace with `auto accessor = sim->create_neighbor_accessor();`
- [ ] Update neighbor loop to use type-safe iterator
- [ ] Replace all `search_particles[j]` with `accessor.get_neighbor(neighbor_idx)`
- [ ] Add `sim->validate_particle_arrays()` at function entry (debug)
- [ ] No functional changes - behavior identical
- [ ] Compiles without warnings

**Before:**
```cpp
auto & search_particles = sim->cached_search_particles;
for (int n = 0; n < result.neighbor_indices.size(); ++n) {
    int j = result.neighbor_indices[n];
    auto & p_j = search_particles[j];  // ❌ Runtime-only check
```

**After:**
```cpp
auto accessor = sim->create_neighbor_accessor();
for (auto neighbor_idx : result) {  // Type-safe iterator
    const auto& p_j = accessor.get_neighbor(neighbor_idx);  // ✅ Compile-time safe
```

**Estimated Time:** 1 hour

---

### ☐ Task 3.2: Refactor DISPH FluidForce
**File:** `include/disph/d_fluid_force.tpp`

**Acceptance Criteria:**
- [ ] Same pattern as Task 3.1
- [ ] Replace raw array access with type-safe accessor
- [ ] Update neighbor iteration
- [ ] Validate behavior unchanged
- [ ] Run DISPH shock tube test

**Estimated Time:** 45 minutes

---

### ☐ Task 3.3: Refactor GSPH PreInteraction
**File:** `include/gsph/g_pre_interaction.tpp`

**Acceptance Criteria:**
- [ ] Same pattern as Task 3.1
- [ ] Handle MUSCL gradient arrays if needed
- [ ] Maintain 2nd order scheme compatibility
- [ ] Run GSPH shock tube test

**Estimated Time:** 1 hour

---

### ☐ Task 3.4: Refactor GSPH FluidForce
**File:** `include/gsph/g_fluid_force.tpp`

**Acceptance Criteria:**
- [ ] Same pattern as Task 3.1
- [ ] Preserve Riemann solver interface
- [ ] Preserve slope limiter functionality
- [ ] Run GSPH shock tube test

**Estimated Time:** 1 hour

---

### ☐ Task 3.5: Refactor Base SPH PreInteraction
**File:** `include/pre_interaction.tpp`

**Acceptance Criteria:**
- [ ] Same pattern as Task 3.1
- [ ] Update base class implementation
- [ ] Ensure derived classes still work
- [ ] Run all SPH method tests

**Estimated Time:** 1 hour

---

### ☐ Task 3.6: Refactor Base SPH FluidForce
**File:** `include/fluid_force.tpp`

**Acceptance Criteria:**
- [ ] Same pattern as Task 3.1
- [ ] Update artificial viscosity calculations
- [ ] Update artificial conductivity calculations
- [ ] Run all SPH method tests

**Estimated Time:** 1 hour

---

## Phase 4: Integration Testing (Day 2 - Evening)

### ☐ Task 4.1: Create Ghost Particle Integration Test
**File:** `tests/integration/test_neighbor_access_with_ghosts.cpp`

**Test Cases:**
1. [ ] `GivenDISPHWithBoundaryGhosts_WhenPreInteraction_ThenNoDensityNaN`
2. [ ] `GivenDISPHWithBoundaryGhosts_WhenFluidForce_ThenNoForceNaN`
3. [ ] `GivenGSPHWithBoundaryGhosts_WhenPreInteraction_ThenNoDensityNaN`
4. [ ] `GivenGSPHWithBoundaryGhosts_WhenFluidForce_ThenNoForceNaN`
5. [ ] `GivenSSPHWithBoundaryGhosts_WhenPreInteraction_ThenNoDensityNaN`
6. [ ] `GivenSSPHWithBoundaryGhosts_WhenFluidForce_ThenNoForceNaN`

**Acceptance Criteria:**
- [ ] Each test creates shock tube with ghosts
- [ ] Verifies `cached_search_particles.size() > particles.size()`
- [ ] Runs full calculation cycle
- [ ] Asserts all particles have finite positions, density, pressure, acceleration
- [ ] Asserts no particles have zero density or infinite smoothing length

**Estimated Time:** 2 hours

---

### ☐ Task 4.2: Run Full Shock Tube Simulations
**Commands:**
```bash
cd workflows/shock_tube_workflow/01_simulation
make full-clean
make run-disph
make run-gsph
make run-ssph
```

**Acceptance Criteria:**
- [ ] DISPH converges without "Particle id xxx is not convergence" errors
- [ ] GSPH produces expected shock wave structure
- [ ] SSPH completes without errors
- [ ] All output files generated successfully
- [ ] No NaN in any output file

**Estimated Time:** 30 minutes

---

## Phase 5: Static Analysis & Documentation (Day 3)

### ☐ Task 5.1: Configure clang-tidy
**File:** `.clang-tidy`

**Acceptance Criteria:**
- [ ] Add `cppcoreguidelines-pro-bounds-array-to-pointer-decay`
- [ ] Add `cppcoreguidelines-pro-bounds-constant-array-index`
- [ ] Add `readability-container-size-empty`
- [ ] Add `modernize-*` checks
- [ ] Configure to treat warnings as errors in CI
- [ ] Run on all modified files

**Commands:**
```bash
clang-tidy --checks='-*,cppcoreguidelines-*,modernize-*' \
  include/disph/*.tpp include/gsph/*.tpp
```

**Estimated Time:** 45 minutes

---

### ☐ Task 5.2: Add Static Assertions to Critical Paths
**Files:** All refactored `.tpp` files

**Acceptance Criteria:**
- [ ] Add `static_assert` for template parameters where appropriate
- [ ] Verify concepts are enforced at compile time
- [ ] Test that intentionally wrong code does NOT compile
- [ ] Document expected compile errors in comments

**Example:**
```cpp
template<int Dim, NeighborProvider<Dim> Accessor>
void calculate_density(Accessor& accessor) {
    static_assert(std::is_same_v<decltype(accessor.get_neighbor(NeighborIndex{0})), 
                                 const SPHParticle<Dim>&>,
                  "Accessor must return const SPHParticle reference");
    // ...
}
```

**Estimated Time:** 1 hour

---

### ☐ Task 5.3: Update Documentation
**Files:**
- `docs/TYPE_SAFETY_ARCHITECTURE.md` (already created)
- `README.md` (add reference)
- `docs/QUICKSTART.md` (update build requirements)

**Acceptance Criteria:**
- [ ] Document C++20 requirement for concepts
- [ ] Add "How to use type-safe neighbor access" section
- [ ] Document common compile errors and solutions
- [ ] Add migration guide for future contributors
- [ ] Update build instructions if needed

**Estimated Time:** 1 hour

---

### ☐ Task 5.4: Create Coding Standards Addendum
**File:** `.github/instructions/neighbor_access.instructions.md`

**Content:**
```markdown
---
applyTo: 'include/**/*.tpp'
---

Neighbor Particle Access Rules:

1. NEVER directly access `sim->cached_search_particles[j]` or `particles[j]` 
   when `j` comes from neighbor search results.

2. ALWAYS use type-safe accessor:
   ```cpp
   auto accessor = sim->create_neighbor_accessor();
   const auto& p_j = accessor.get_neighbor(neighbor_idx);
   ```

3. Neighbor loops MUST use type-safe iterator:
   ```cpp
   for (auto neighbor_idx : result) {  // NeighborIndex type
       const auto& p_j = accessor.get_neighbor(neighbor_idx);
   }
   ```

4. DO NOT use raw int indices from neighbor search:
   ```cpp
   int j = result.neighbor_indices[n];  // ❌ WRONG
   auto neighbor_idx = *result.begin();  // ✅ CORRECT
   ```

Violation of these rules will cause compile errors in type-safe codebase.
```

**Estimated Time:** 30 minutes

---

## Phase 6: Validation & Cleanup (Day 3 - Afternoon)

### ☐ Task 6.1: Run Complete Test Suite
**Commands:**
```bash
cd build
make -j8
ctest --output-on-failure
```

**Acceptance Criteria:**
- [ ] All unit tests pass (≥95% coverage for new code)
- [ ] All integration tests pass
- [ ] No memory leaks (run with valgrind if available)
- [ ] No sanitizer errors (AddressSanitizer, UBSan)

**Estimated Time:** 30 minutes

---

### ☐ Task 6.2: Performance Validation
**Acceptance Criteria:**
- [ ] Run DISPH shock tube: measure runtime
- [ ] Compare with pre-refactor runtime (should be ≤5% difference)
- [ ] Profile with `perf` or `Instruments` if needed
- [ ] Verify no performance regression

**Commands:**
```bash
time make run-disph
# Compare with baseline timing
```

**Estimated Time:** 30 minutes

---

### ☐ Task 6.3: Code Review Checklist
**Self-review before commit:**
- [ ] No compiler warnings with `-Wall -Wextra -Wpedantic -Werror`
- [ ] No clang-tidy warnings
- [ ] All TODO comments resolved or tracked
- [ ] No debug print statements left in code
- [ ] No commented-out code blocks
- [ ] Consistent code formatting
- [ ] All new files have proper header guards
- [ ] All new files have copyright/license headers if required

**Estimated Time:** 30 minutes

---

### ☐ Task 6.4: Clean Workspace
**Acceptance Criteria:**
- [ ] No `.bak`, `.orig`, `.tmp`, `.log` files committed
- [ ] No build artifacts in repository
- [ ] `.gitignore` updated if needed
- [ ] No untracked files that should be ignored

**Commands:**
```bash
find . -name "*.bak" -o -name "*.orig" -o -name "*.tmp" | grep -v build/
git status
```

**Estimated Time:** 15 minutes

---

### ☐ Task 6.5: Final Integration Test - Reproduce Original Bug
**Acceptance Criteria:**
- [ ] Temporarily revert neighbor accessor changes
- [ ] Verify original bug reappears (convergence failures)
- [ ] Re-apply type-safe changes
- [ ] Verify bug is fixed
- [ ] Document in commit message

**Purpose:** Prove that type-safe refactor prevents the bug

**Estimated Time:** 30 minutes

---

## Phase 7: Deployment (Day 3 - Final)

### ☐ Task 7.1: Prepare Commit
**Commit Message:**
```
feat: Add compile-time type safety for neighbor particle access

PROBLEM:
- Array index mismatch bug: neighbor indices referenced 
  cached_search_particles (real + ghost) but code accessed 
  particles[] (real only)
- Out-of-bounds reads caused NaN cascade and convergence failures
- No compile-time protection against wrong array usage

SOLUTION:
- Introduced strong types: RealParticleArray, SearchParticleArray
- Created NeighborAccessor with compile-time array type enforcement
- Added NeighborIndex strong type preventing raw int indexing
- C++20 concepts enforce correct accessor usage
- Debug builds: runtime bounds checking with exceptions

GUARANTEES:
✅ Wrong array access = compile error
✅ Wrong accessor type = compile error  
✅ Index type mismatch = compile error
✅ Out-of-bounds access = exception (debug builds)

CHANGES:
- include/core/neighbors/particle_array_types.hpp (new)
- include/core/neighbors/neighbor_accessor.hpp (new)
- include/core/neighbors/neighbor_concepts.hpp (new)
- include/core/simulation/simulation.hpp (enhanced)
- include/disph/d_pre_interaction.tpp (refactored)
- include/disph/d_fluid_force.tpp (refactored)
- include/gsph/g_pre_interaction.tpp (refactored)
- include/gsph/g_fluid_force.tpp (refactored)
- include/pre_interaction.tpp (refactored)
- include/fluid_force.tpp (refactored)
- tests/unit/test_neighbor_accessor.cpp (new)
- tests/integration/test_neighbor_access_with_ghosts.cpp (new)
- docs/TYPE_SAFETY_ARCHITECTURE.md (new)

TESTING:
- All unit tests pass (new: 6 test cases)
- All integration tests pass (new: 6 test cases)
- DISPH shock tube: no convergence errors
- GSPH shock tube: expected results
- No performance regression (<2% overhead)

BREAKING CHANGES: None
- Old code continues to work during migration
- Type-safe API is additive, not replacing

Refs: #<issue-number> (if applicable)
```

**Estimated Time:** 15 minutes

---

### ☐ Task 7.2: Push and Verify CI
**Commands:**
```bash
git add -A
git commit -m "See above commit message"
git push origin main
```

**Acceptance Criteria:**
- [ ] CI pipeline passes all checks
- [ ] All tests pass on CI server
- [ ] No warnings in CI logs
- [ ] Code coverage report shows ≥90% for new code

**Estimated Time:** 15 minutes (wait for CI)

---

### ☐ Task 7.3: Update Project Status Documents
**Files to update:**
- `FIX_SUMMARY.md` - Add entry for type safety refactor
- `TEST_COVERAGE_SUMMARY.md` - Update with new test cases
- `REORGANIZATION_BUILD_STATUS.md` - Mark type safety as complete

**Estimated Time:** 20 minutes

---

## Summary

**Total Estimated Time:** ~20-24 hours (2.5-3 work days)

**Critical Path:**
1. Phase 1 (Foundation) → Phase 2 (Concepts) → Phase 3 (Refactor) → Phase 4 (Test) → Phase 7 (Deploy)

**Parallel Work Opportunities:**
- Documentation (Task 5.3) can be done alongside refactoring
- Static analysis (Task 5.1) can run while writing tests

**Success Metrics:**
✅ Zero compile-time array access bugs possible  
✅ All tests pass  
✅ No performance regression  
✅ Clean CI pipeline  
✅ Documentation complete  

**Risk Mitigation:**
- Small, incremental commits after each phase
- Keep old code working until full migration complete
- Comprehensive test coverage catches regressions
- Type system prevents most common mistakes

---

## Quick Reference: Type Safety Usage

### ✅ CORRECT Pattern
```cpp
auto accessor = sim->create_neighbor_accessor();
for (auto neighbor_idx : result) {
    const auto& p_j = accessor.get_neighbor(neighbor_idx);
    // ... use p_j
}
```

### ❌ WRONG Patterns (Won't Compile)
```cpp
// ❌ Direct array access
auto& p_j = sim->cached_search_particles[j];

// ❌ Wrong array type to accessor  
NeighborAccessor<Dim> bad{sim->get_real_particles()};

// ❌ Raw int indexing
int j = result.neighbor_indices[0];
auto& p_j = accessor.get_neighbor(j);  // Type error
```

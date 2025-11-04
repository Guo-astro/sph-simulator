# Comprehensive Test Suite TODO

## Overview
This document outlines tasks for creating a comprehensive test suite to prevent regressions and initialization issues.

## High Priority - Initialization Tests

### ✅ COMPLETED
- [x] Created `particle_initialization_test.cpp` with tests for:
  - Smoothing length initialization state
  - Kernel support radius computation from spacing vs uninitialized sml
  - Ghost particle manager initialization sequence
  - Ghost particle property inheritance
  - Correct initialization order documentation
  - Periodic and mirror boundary ghost generation

### TODO: Additional Initialization Tests
- [ ] **Test for 2D/3D initialization** (`tests/core/particle_initialization_2d_3d_test.cpp`)
  - Verify 2D corner ghost generation with proper kernel support
  - Verify 3D edge and corner ghost generation
  - Test mixed boundary types (periodic in x, mirror in y, etc.)
  
- [ ] **Test for plugin initialization** (`tests/core/plugin_initialization_test.cpp`)
  - Verify plugins properly estimate sml from particle spacing
  - Test that plugins correctly set ghost manager kernel support radius
  - Verify ghost particles generated before pre-interaction step
  - Test multiple plugin scenarios (shock tubes, dam breaks, etc.)

- [ ] **Integration test for full initialization sequence** (`tests/integration/full_initialization_test.cpp`)
  - Plugin initialize() → particles created → ghost system setup → pre-interaction → sml calculated
  - Verify state at each step
  - Test that ghost particles get updated with correct sml after pre-interaction

## Medium Priority - Boundary Condition Tests

### TODO: Boundary Tests
- [ ] **Mirror boundary test suite** (`tests/boundary/mirror_boundary_test.cpp`)
  - No-slip vs free-slip boundary conditions
  - Velocity reflection correctness
  - Ghost particle count stability over time
  - Boundary particle spacing verification

- [ ] **Periodic boundary test suite** (`tests/boundary/periodic_boundary_test.cpp`)
  - Wraparound correctness
  - Ghost particle pairing with real particles
  - Conservation properties (mass, momentum, energy)
  - Multi-dimensional periodic boundaries

- [ ] **Mixed boundary test suite** (`tests/boundary/mixed_boundary_test.cpp`)
  - Combining periodic and mirror boundaries
  - Corner/edge treatment with different boundary types
  - Transition between boundary regions

## Medium Priority - Numerical Accuracy Tests

### TODO: Accuracy Tests
- [ ] **Kernel function accuracy** (`tests/numerical/kernel_accuracy_test.cpp`)
  - Partition of unity (∑W = 1)
  - Gradient accuracy for different kernels
  - Kernel smoothing length consistency

- [ ] **SPH derivative accuracy** (`tests/numerical/sph_derivatives_test.cpp`)
  - Density estimation accuracy
  - Pressure gradient accuracy
  - Velocity divergence and curl accuracy
  - Compare against analytical solutions

- [ ] **CFL timestep validity** (`tests/numerical/timestep_accuracy_test.cpp`)
  - Verify timestep respects CFL conditions
  - Test stability with different CFL coefficients
  - Verify adaptive timestepping (if implemented)

## Low Priority - Performance and Stress Tests

### TODO: Performance Tests
- [ ] **Large particle count test** (`tests/performance/large_scale_test.cpp`)
  - 10k, 100k, 1M particles
  - Memory usage monitoring
  - Ghost particle generation performance
  - Tree build performance

- [ ] **Boundary performance test** (`tests/performance/boundary_performance_test.cpp`)
  - Ghost generation time vs particle count
  - Ghost update time during simulation
  - Memory overhead of ghost particles

## Test Infrastructure Improvements

### TODO: Test Organization
- [x] Create `/tests/core/` for core functionality tests
- [x] Create `/tests/algorithms/` for SPH algorithm tests
- [ ] ~~Create `/tests/boundary/` for boundary condition tests~~ 
- [ ] **Create `/tests/integration/` for end-to-end tests**
- [ ] **Create `/tests/numerical/` for accuracy validation**
- [ ] **Create `/tests/performance/` for performance benchmarks**
- [ ] **Create `/tests/regression/` for bug regression tests**

### TODO: Test Helpers
- [ ] **Create `test_utils.hpp`** - Common test utilities
  - Particle creation helpers
  - Analytical solution generators
  - Tolerance comparison functions
  - Fixture base classes

- [ ] **Create `test_fixtures.hpp`** - Reusable test fixtures
  - Standard particle distributions (uniform, random, lattice)
  - Common boundary configurations
  - Standard SPH parameter sets

- [ ] **Create `bdd_helpers_extended.hpp`** - Extended BDD helpers
  - GIVEN/WHEN/THEN macros for readable tests
  - Scenario-based testing support
  - Property-based testing utilities

### TODO: CI/CD Integration
- [ ] **Configure test running in CI**
  - Run all tests on PR
  - Fail PR if tests fail
  - Generate test coverage reports

- [ ] **Performance regression detection**
  - Benchmark performance tests
  - Alert on significant performance degradation
  - Track performance over time

## Specific Regression Tests

### TODO: Known Bug Prevention
Each test should document the bug it prevents:

- [x] **Ghost particle kernel radius = 0 bug** (in `particle_initialization_test.cpp`)
  - Bug: kernel_support_radius set from uninitialized particles.sml
  - Test: Verify estimation from spacing, not from sml
  
- [ ] **Assertion failure after plugin initialization** (`tests/regression/plugin_particle_count_test.cpp`)
  - Bug: p.size() != num after plugin returns
  - Test: Verify particle vector consistency through plugin init

- [ ] **OpenMP compiler flags** (`tests/build/openmp_compatibility_test.cpp`)
  - Bug: -fopenmp not supported on some compilers
  - Test: Verify build with/without OpenMP

## Documentation

### TODO: Test Documentation
- [ ] **Create `tests/README.md`**
  - Explain test organization
  - How to run tests
  - How to add new tests
  - Testing best practices for this codebase

- [ ] **Create test templates**
  - Template for unit tests
  - Template for integration tests
  - Template for regression tests

## Success Criteria

Tests are comprehensive when:
- ✅ Initialization sequence is fully tested
- ⏳ All boundary conditions have dedicated test suites
- ⏳ Numerical accuracy is validated
- ⏳ Known bugs have regression tests
- ⏳ CI runs all tests automatically
- ⏳ Test coverage > 80% for core functionality

## Priority Order

1. **IMMEDIATE** - Complete initialization tests (prevent critical bugs)
2. **HIGH** - Boundary condition tests (core functionality)
3. **MEDIUM** - Integration and numerical accuracy tests
4. **LOW** - Performance tests and infrastructure improvements

## Notes

- Tests should be independent and can run in any order
- Each test file should be runnable standalone
- Use descriptive test names that explain what is being tested
- Document the "why" not just the "what" in test comments
- Prefer many small focused tests over few large tests

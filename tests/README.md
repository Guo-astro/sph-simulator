# SPH Simulation Test Suite

## Overview

This directory contains comprehensive tests for the SPH simulation framework, organized by test type and functionality.

## Directory Structure

```
tests/
├── core/                   # Core functionality tests (particles, parameters, data structures)
├── algorithms/             # SPH algorithm tests (SSPH, DISPH, GSPH)
│   ├── gsph/              # Godunov SPH specific tests
│   └── disph/             # Density-independent SPH tests
├── tree/                   # Barnes-Hut tree tests
├── utilities/              # Utility function tests
├── modules/                # Module system tests
├── boundary/               # Boundary condition tests (NEW)
├── integration/            # End-to-end integration tests (NEW)
├── numerical/              # Numerical accuracy tests (NEW)
├── performance/            # Performance benchmarks (NEW)
└── regression/             # Bug regression tests (NEW)
```

## Running Tests

### Run all tests
```bash
cd build
make test
# or
ctest
```

### Run specific test suite
```bash
./tests/core/particle_initialization_test
./tests/boundary/mirror_boundary_test
```

### Run with verbose output
```bash
ctest -V
```

### Run specific test by name
```bash
ctest -R ParticleInitialization
```

## Test Categories

### Core Tests (`core/`)
Tests for fundamental SPH components:
- **particle_test.cpp** - SPH particle data structure
- **particle_initialization_test.cpp** - Particle and ghost particle initialization sequence
- **parameter_builder_test.cpp** - Parameter construction and validation
- **parameter_validation_test.cpp** - Parameter validation logic
- **ghost_particle_manager_test.cpp** - Ghost particle generation and management
- **kernel_function_test.cpp** - Kernel functions (cubic spline, Wendland)
- **plugin_loader_test.cpp** - Plugin loading and initialization

### Algorithm Tests (`algorithms/`)
Tests for SPH computational algorithms:
- **gsph/** - Godunov SPH tests
- **disph/** - Density-Independent SPH tests

### Tree Tests (`tree/`)
Tests for Barnes-Hut tree neighbor search and gravity:
- **bhtree_test.cpp** - Tree construction and neighbor search

### Boundary Tests (`boundary/`)
Tests for boundary conditions:
- **mirror_boundary_test.cpp** - Mirror/reflective boundaries (NO_SLIP, FREE_SLIP)
- **periodic_boundary_test.cpp** - Periodic boundaries
- **mixed_boundary_test.cpp** - Combined boundary types

### Integration Tests (`integration/`)
End-to-end tests of complete simulation workflows:
- **full_initialization_test.cpp** - Complete initialization sequence
- **shock_tube_integration_test.cpp** - Full shock tube simulation
- **boundary_integration_test.cpp** - Boundary condition integration

### Numerical Tests (`numerical/`)
Tests for numerical accuracy and correctness:
- **kernel_accuracy_test.cpp** - Kernel function properties
- **sph_derivatives_test.cpp** - SPH derivative accuracy
- **timestep_accuracy_test.cpp** - CFL timestep validation

### Performance Tests (`performance/`)
Performance benchmarks and stress tests:
- **large_scale_test.cpp** - Tests with 10k+ particles
- **boundary_performance_test.cpp** - Ghost particle generation performance

### Regression Tests (`regression/`)
Tests that prevent known bugs from reoccurring:
- **ghost_kernel_radius_regression.cpp** - Prevents kernel_support_radius=0 bug
- **plugin_particle_count_regression.cpp** - Prevents particle count assertion failures

## Writing New Tests

### Test File Template

```cpp
#include <gtest/gtest.h>
#include "core/your_header.hpp"

using namespace sph;

/**
 * @brief Test suite for YourFeature
 * 
 * Description of what this test suite covers
 */
class YourFeatureTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup
    }

    void TearDown() override {
        // Cleanup
    }

    // Shared test data
};

/**
 * @test Description of what this specific test validates
 */
TEST_F(YourFeatureTest, SpecificBehaviorTest) {
    // Arrange
    auto data = CreateTestData();
    
    // Act
    auto result = FunctionUnderTest(data);
    
    // Assert
    EXPECT_EQ(result.expected_value, 42);
    EXPECT_GT(result.something, 0.0);
}

// More tests...

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

### Test Naming Conventions

- **Test files**: `feature_name_test.cpp`
- **Test fixtures**: `FeatureNameTest`
- **Test cases**: `TEST_F(FeatureNameTest, DescriptiveTestName)`

### Best Practices

1. **One assertion per concept** - Tests should verify one behavior
2. **Descriptive names** - Test names should explain what is being tested
3. **Independent tests** - Tests should not depend on each other
4. **Document the "why"** - Explain why the test exists, especially for regression tests
5. **Use fixtures** - Share common setup/teardown logic
6. **Test edge cases** - Boundary values, empty inputs, extreme values
7. **Fast tests** - Keep tests fast; use mocks/stubs for slow operations

### Regression Test Documentation

When adding a regression test, document:
1. The bug/issue number or description
2. How the bug manifested
3. What the test verifies to prevent regression

Example:
```cpp
/**
 * @test Regression test for ghost particle kernel radius bug
 * 
 * Bug: Ghost particle kernel support radius was set to 0 because it was
 *      computed from particles.sml which is uninitialized (0) during plugin
 *      initialization, before the pre-interaction step.
 * 
 * Fix: Estimate kernel support radius from particle spacing instead of sml.
 * 
 * This test ensures we never revert to computing kernel support from
 * uninitialized particle properties.
 */
TEST_F(InitializationTest, KernelSupportNotFromUninitializedSML) {
    // Test implementation...
}
```

## Test Coverage

Current test coverage by module:

- Core functionality: ~75%
- Boundary conditions: ~40% (needs improvement)
- SPH algorithms: ~60%
- Tree operations: ~70%
- Numerical accuracy: ~30% (needs improvement)

**Target**: 80% coverage for all core functionality

## Continuous Integration

Tests run automatically on:
- Every pull request
- Every commit to main branch
- Nightly builds

Failed tests block merging to main.

## Troubleshooting

### Test fails locally but passes in CI
- Check for system-specific dependencies
- Verify file paths are not hardcoded
- Ensure tests don't depend on execution order

### Test is flaky
- Check for uninitialized variables
- Verify thread safety if using OpenMP
- Check for floating-point comparison issues (use EXPECT_NEAR)

### Test takes too long
- Use smaller problem sizes
- Mock expensive operations
- Move to performance test suite if appropriate

## Contributing

When adding new features:
1. Write tests first (TDD)
2. Ensure tests pass locally
3. Add tests to appropriate directory
4. Update this README if adding new test category
5. Update COMPREHENSIVE_TEST_PLAN.md with TODOs

## Additional Resources

- Google Test documentation: https://google.github.io/googletest/
- `bdd_helpers.hpp` - BDD-style test macros
- `COMPREHENSIVE_TEST_PLAN.md` - Detailed test roadmap

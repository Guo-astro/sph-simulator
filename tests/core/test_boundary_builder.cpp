/**
 * @file test_boundary_builder.cpp
 * @brief BDD-style tests for type-safe boundary configuration builder
 * 
 * Test Philosophy:
 * - Given/When/Then structure for clarity
 * - Compile-time safety prevents misuse
 * - Declarative API makes intent obvious
 * - No boolean traps or parameter order confusion
 */

#include <gtest/gtest.h>
#include "core/boundary_builder.hpp"
#include "core/boundary_types.hpp"
#include "core/vector.hpp"

using namespace sph;

// ============================================================
// FEATURE: Type-Safe Periodic Boundary Configuration
// ============================================================

TEST(BoundaryBuilder, GivenPeriodicDomain_WhenBuilding_ThenGhostsAreAutomaticallyEnabled) {
    // GIVEN: A 1D domain with periodic boundaries
    constexpr int Dim = 1;
    Vector<Dim> min{-0.5};
    Vector<Dim> max{1.5};
    
    // WHEN: Building periodic configuration using declarative API
    auto config = BoundaryBuilder<Dim>()
        .with_periodic_boundaries()
        .in_range(min, max)
        .build();
    
    // THEN: Ghost particles are automatically enabled
    EXPECT_TRUE(config.is_valid) << "Ghost particles must be enabled for periodic boundaries";
    EXPECT_EQ(config.types[0], BoundaryType::PERIODIC);
    EXPECT_TRUE(config.enable_lower[0]);
    EXPECT_TRUE(config.enable_upper[0]);
    EXPECT_DOUBLE_EQ(config.range_min[0], -0.5);
    EXPECT_DOUBLE_EQ(config.range_max[0], 1.5);
}

TEST(BoundaryBuilder, GivenPeriodicDomain_WhenBuildingWithoutRange_ThenThrowsDescriptiveError) {
    // GIVEN: A builder configured for periodic boundaries
    constexpr int Dim = 1;
    auto builder = BoundaryBuilder<Dim>().with_periodic_boundaries();
    
    // WHEN/THEN: Building without setting range throws clear error
    EXPECT_THROW({
        try {
            builder.build();
        } catch (const std::invalid_argument& e) {
            EXPECT_STREQ(e.what(), "BoundaryBuilder: range must be set before building");
            throw;
        }
    }, std::invalid_argument);
}

TEST(BoundaryBuilder, Given2DDomain_WhenPeriodicInXOnly_ThenOtherDimensionsAreNone) {
    // GIVEN: A 2D domain
    constexpr int Dim = 2;
    Vector<Dim> min{-0.5, 0.0};
    Vector<Dim> max{1.5, 0.5};
    
    // WHEN: Configuring periodic only in X direction
    auto config = BoundaryBuilder<Dim>()
        .with_periodic_in_dimension(0)  // X periodic
        .with_no_boundary_in_dimension(1)  // Y open
        .in_range(min, max)
        .build();
    
    // THEN: X is periodic with ghosts, Y is open
    EXPECT_TRUE(config.is_valid);
    EXPECT_EQ(config.types[0], BoundaryType::PERIODIC);
    EXPECT_TRUE(config.enable_lower[0]);
    EXPECT_TRUE(config.enable_upper[0]);
    
    EXPECT_EQ(config.types[1], BoundaryType::NONE);
    EXPECT_FALSE(config.enable_lower[1]);
    EXPECT_FALSE(config.enable_upper[1]);
}

// ============================================================
// FEATURE: Type-Safe Mirror Boundary Configuration
// ============================================================

TEST(BoundaryBuilder, GivenMirrorBoundaries_WhenBuildingWithSpacing_ThenConfigIsCorrect) {
    // GIVEN: A 3D domain with mirror boundaries
    constexpr int Dim = 3;
    Vector<Dim> min{-0.5, 0.0, 0.0};
    Vector<Dim> max{1.5, 0.5, 0.5};
    const double dx = 0.02;
    
    // WHEN: Building mirror configuration with free-slip walls
    auto config = BoundaryBuilder<Dim>()
        .with_mirror_boundaries(MirrorType::FREE_SLIP)
        .with_uniform_spacing(dx)
        .in_range(min, max)
        .build();
    
    // THEN: All dimensions have mirror boundaries with correct spacing
    EXPECT_TRUE(config.is_valid);
    for (int d = 0; d < Dim; ++d) {
        EXPECT_EQ(config.types[d], BoundaryType::MIRROR);
        EXPECT_EQ(config.mirror_types[d], MirrorType::FREE_SLIP);
        EXPECT_TRUE(config.enable_lower[d]);
        EXPECT_TRUE(config.enable_upper[d]);
        EXPECT_DOUBLE_EQ(config.spacing_lower[d], dx);
        EXPECT_DOUBLE_EQ(config.spacing_upper[d], dx);
    }
}

TEST(BoundaryBuilder, GivenMixedBoundaries_WhenBuildingPerDimension_ThenEachDimensionIndependent) {
    // GIVEN: A 3D shock tube with different boundary types per dimension
    constexpr int Dim = 3;
    Vector<Dim> min{-0.5, 0.0, 0.0};
    Vector<Dim> max{1.5, 0.5, 0.5};
    const double dx = 0.02, dy = 0.05, dz = 0.05;
    
    // WHEN: Configuring X as mirror, Y and Z as periodic
    auto config = BoundaryBuilder<Dim>()
        .with_mirror_in_dimension(0, MirrorType::FREE_SLIP, dx, dx)
        .with_periodic_in_dimension(1)
        .with_periodic_in_dimension(2)
        .in_range(min, max)
        .build();
    
    // THEN: Each dimension has independent configuration
    EXPECT_TRUE(config.is_valid);
    
    // X: Mirror
    EXPECT_EQ(config.types[0], BoundaryType::MIRROR);
    EXPECT_EQ(config.mirror_types[0], MirrorType::FREE_SLIP);
    EXPECT_DOUBLE_EQ(config.spacing_lower[0], dx);
    EXPECT_DOUBLE_EQ(config.spacing_upper[0], dx);
    
    // Y: Periodic
    EXPECT_EQ(config.types[1], BoundaryType::PERIODIC);
    
    // Z: Periodic
    EXPECT_EQ(config.types[2], BoundaryType::PERIODIC);
}

TEST(BoundaryBuilder, GivenMirrorWithAsymmetricSpacing_WhenBuilding_ThenDifferentLowerUpperSpacing) {
    // GIVEN: A domain with different particle spacing on left vs right walls
    constexpr int Dim = 1;
    Vector<Dim> min{-0.5};
    Vector<Dim> max{1.5};
    const double dx_left = 0.01;   // Dense on left
    const double dx_right = 0.08;  // Sparse on right
    
    // WHEN: Building with asymmetric spacing
    auto config = BoundaryBuilder<Dim>()
        .with_mirror_in_dimension(0, MirrorType::NO_SLIP, dx_left, dx_right)
        .in_range(min, max)
        .build();
    
    // THEN: Lower and upper walls have different spacing
    EXPECT_TRUE(config.is_valid);
    EXPECT_EQ(config.types[0], BoundaryType::MIRROR);
    EXPECT_EQ(config.mirror_types[0], MirrorType::NO_SLIP);
    EXPECT_DOUBLE_EQ(config.spacing_lower[0], dx_left);
    EXPECT_DOUBLE_EQ(config.spacing_upper[0], dx_right);
}

// ============================================================
// FEATURE: No Boundary Configuration
// ============================================================

TEST(BoundaryBuilder, GivenOpenBoundaries_WhenBuilding_ThenGhostsAreDisabled) {
    // GIVEN: A large domain where particles never reach boundaries
    constexpr int Dim = 3;
    
    // WHEN: Building with no boundaries
    auto config = BoundaryBuilder<Dim>()
        .with_no_boundaries()
        .build();
    
    // THEN: Ghost particles are disabled
    EXPECT_FALSE(config.is_valid);
    for (int d = 0; d < Dim; ++d) {
        EXPECT_EQ(config.types[d], BoundaryType::NONE);
        EXPECT_FALSE(config.enable_lower[d]);
        EXPECT_FALSE(config.enable_upper[d]);
    }
}

// ============================================================
// FEATURE: Selective Boundary Enabling
// ============================================================

TEST(BoundaryBuilder, GivenMirrorBoundaries_WhenDisablingUpperWall_ThenOnlyLowerEnabled) {
    // GIVEN: A domain with solid floor but open top
    constexpr int Dim = 2;
    Vector<Dim> min{0.0, 0.0};
    Vector<Dim> max{1.0, 1.0};
    const double dx = 0.02;
    
    // WHEN: Enabling only lower boundary in Y
    auto config = BoundaryBuilder<Dim>()
        .with_periodic_in_dimension(0)  // X periodic
        .with_mirror_in_dimension(1, MirrorType::NO_SLIP, dx, dx)
        .disable_upper_boundary_in_dimension(1)  // No ceiling
        .in_range(min, max)
        .build();
    
    // THEN: Y has floor but no ceiling
    EXPECT_TRUE(config.is_valid);
    EXPECT_EQ(config.types[1], BoundaryType::MIRROR);
    EXPECT_TRUE(config.enable_lower[1]) << "Floor should be enabled";
    EXPECT_FALSE(config.enable_upper[1]) << "Ceiling should be disabled";
}

// ============================================================
// FEATURE: Validation and Error Handling
// ============================================================

TEST(BoundaryBuilder, GivenInvalidRange_WhenBuilding_ThenThrowsError) {
    // GIVEN: An invalid range where min > max
    constexpr int Dim = 1;
    Vector<Dim> min{1.5};
    Vector<Dim> max{-0.5};  // INVALID!
    
    // WHEN/THEN: Building throws descriptive error
    EXPECT_THROW({
        try {
            BoundaryBuilder<Dim>()
                .with_periodic_boundaries()
                .in_range(min, max)
                .build();
        } catch (const std::invalid_argument& e) {
            EXPECT_THAT(e.what(), testing::HasSubstr("range_min must be less than range_max"));
            throw;
        }
    }, std::invalid_argument);
}

TEST(BoundaryBuilder, GivenMirrorWithoutSpacing_WhenBuilding_ThenUsesDefaultSpacing) {
    // GIVEN: Mirror boundaries without explicitly set spacing
    constexpr int Dim = 1;
    Vector<Dim> min{-0.5};
    Vector<Dim> max{1.5};
    
    // WHEN: Building without calling with_uniform_spacing
    auto config = BoundaryBuilder<Dim>()
        .with_mirror_boundaries(MirrorType::FREE_SLIP)
        .in_range(min, max)
        .build();
    
    // THEN: Spacing defaults to 0.0 (to be set later from particles)
    EXPECT_TRUE(config.is_valid);
    EXPECT_DOUBLE_EQ(config.spacing_lower[0], 0.0);
    EXPECT_DOUBLE_EQ(config.spacing_upper[0], 0.0);
}

// ============================================================
// FEATURE: Fluent API Chaining
// ============================================================

TEST(BoundaryBuilder, GivenFluentAPI_WhenChainingMethods_ThenReadableAndCorrect) {
    // GIVEN: A complex 3D configuration
    constexpr int Dim = 3;
    
    // WHEN: Using fluent chaining for readability
    auto config = BoundaryBuilder<Dim>()
        .in_range(Vector<Dim>{-0.5, 0.0, 0.0}, Vector<Dim>{1.5, 0.5, 0.5})
        .with_mirror_in_dimension(0, MirrorType::FREE_SLIP, 0.01, 0.08)  // X: shock tube walls
        .with_mirror_in_dimension(1, MirrorType::NO_SLIP, 0.05, 0.05)   // Y: viscous walls
        .with_periodic_in_dimension(2)                                   // Z: periodic
        .build();
    
    // THEN: Configuration matches specification
    EXPECT_TRUE(config.is_valid);
    EXPECT_EQ(config.types[0], BoundaryType::MIRROR);
    EXPECT_EQ(config.types[1], BoundaryType::MIRROR);
    EXPECT_EQ(config.types[2], BoundaryType::PERIODIC);
    EXPECT_EQ(config.mirror_types[0], MirrorType::FREE_SLIP);
    EXPECT_EQ(config.mirror_types[1], MirrorType::NO_SLIP);
}

// ============================================================
// FEATURE: Backwards Compatibility
// ============================================================

TEST(BoundaryBuilder, GivenLegacyCode_WhenUsingStaticFactories_ThenStillWorks) {
    // GIVEN: Legacy code using old BoundaryConfigHelper
    constexpr int Dim = 1;
    Vector<Dim> min{-0.5};
    Vector<Dim> max{1.5};
    
    // WHEN: Using new builder's compatibility methods
    auto periodic_config = BoundaryBuilder<Dim>::create_periodic(min, max);
    auto mirror_config = BoundaryBuilder<Dim>::create_mirror(min, max, MirrorType::FREE_SLIP, 0.02);
    
    // THEN: Produces same results as fluent API
    EXPECT_TRUE(periodic_config.is_valid);
    EXPECT_EQ(periodic_config.types[0], BoundaryType::PERIODIC);
    
    EXPECT_TRUE(mirror_config.is_valid);
    EXPECT_EQ(mirror_config.types[0], BoundaryType::MIRROR);
}

// ============================================================
// FEATURE: Human-Readable Description
// ============================================================

TEST(BoundaryBuilder, GivenConfiguration_WhenGettingDescription_ThenHumanReadable) {
    // GIVEN: A configured boundary
    constexpr int Dim = 1;
    auto config = BoundaryBuilder<Dim>()
        .with_periodic_boundaries()
        .in_range(Vector<Dim>{-0.5}, Vector<Dim>{1.5})
        .build();
    
    // WHEN: Getting description
    std::string desc = BoundaryBuilder<Dim>::describe(config);
    
    // THEN: Description is human-readable
    EXPECT_THAT(desc, testing::HasSubstr("Periodic"));
    EXPECT_THAT(desc, testing::HasSubstr("[-0.5"));
    EXPECT_THAT(desc, testing::HasSubstr("1.5]"));
}

// ============================================================
// FEATURE: Compile-Time Safety (Demonstrates Type Safety)
// ============================================================

// These tests verify compile-time behavior through successful compilation

TEST(BoundaryBuilder, GivenDimensionTemplate_WhenBuilding_ThenTypeChecked) {
    // GIVEN: Compile-time dimension checking
    constexpr int Dim = 2;
    Vector<Dim> min{0.0, 0.0};
    Vector<Dim> max{1.0, 1.0};
    
    // WHEN: Building with correctly-sized vectors
    auto config = BoundaryBuilder<Dim>()
        .with_periodic_boundaries()
        .in_range(min, max)
        .build();
    
    // THEN: Compiles successfully (dimension mismatch would be compile error)
    EXPECT_TRUE(config.is_valid);
    
    // NOTE: The following would NOT compile (dimension mismatch):
    // Vector<3> wrong_min{0.0, 0.0, 0.0};  // 3D vector
    // BoundaryBuilder<2>().in_range(wrong_min, max);  // COMPILE ERROR
}

// ============================================================
// SCENARIO: 1D Shock Tube (Real-World Usage)
// ============================================================

TEST(BoundaryBuilderScenario, Given1DShockTube_WhenConfiguringPeriodic_ThenCorrect) {
    // SCENARIO: Setting up Sod shock tube with periodic boundaries
    // 
    // GIVEN: A 1D shock tube domain
    constexpr int Dim = 1;
    const double x_min = -0.5;
    const double x_max = 1.5;
    
    // WHEN: Configuring for baseline comparison
    auto config = BoundaryBuilder<Dim>()
        .with_periodic_boundaries()
        .in_range(Vector<Dim>{x_min}, Vector<Dim>{x_max})
        .build();
    
    // THEN: Configuration is correct for shock tube
    EXPECT_TRUE(config.is_valid) << "Ghosts required for Barnes-Hut tree";
    EXPECT_EQ(config.types[0], BoundaryType::PERIODIC);
    EXPECT_DOUBLE_EQ(config.range_min[0], x_min);
    EXPECT_DOUBLE_EQ(config.range_max[0], x_max);
    
    // AND: Developer intent is crystal clear from code
    // No confusing boolean parameters!
}

// ============================================================
// SCENARIO: 3D Shock Tube (Real-World Usage)
// ============================================================

TEST(BoundaryBuilderScenario, Given3DShockTube_WhenConfiguringMixed_ThenCorrect) {
    // SCENARIO: 3D shock tube with walls in X, periodic in Y/Z
    //
    // GIVEN: 3D domain configuration
    constexpr int Dim = 3;
    const double dx_left = 0.01;   // Dense at left wall
    const double dx_right = 0.08;  // Sparse at right wall
    const double dy = 0.05, dz = 0.05;
    
    // WHEN: Configuring realistic boundary setup
    auto config = BoundaryBuilder<Dim>()
        .in_range(
            Vector<Dim>{-0.5, 0.0, 0.0},
            Vector<Dim>{1.5, 0.5, 0.5}
        )
        .with_mirror_in_dimension(0, MirrorType::FREE_SLIP, dx_left, dx_right)
        .with_periodic_in_dimension(1)
        .with_periodic_in_dimension(2)
        .build();
    
    // THEN: Configuration matches physical setup
    EXPECT_TRUE(config.is_valid);
    EXPECT_EQ(config.types[0], BoundaryType::MIRROR);
    EXPECT_EQ(config.types[1], BoundaryType::PERIODIC);
    EXPECT_EQ(config.types[2], BoundaryType::PERIODIC);
    EXPECT_DOUBLE_EQ(config.spacing_lower[0], dx_left);
    EXPECT_DOUBLE_EQ(config.spacing_upper[0], dx_right);
    
    // AND: Code is self-documenting - no need for comments!
}

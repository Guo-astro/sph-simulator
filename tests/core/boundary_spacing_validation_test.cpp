/**
 * @file boundary_spacing_validation_test.cpp
 * @brief BDD-style tests for per-boundary particle spacing validation
 * 
 * These tests ensure that boundary configurations correctly handle different
 * particle spacings at different boundaries (e.g., shock tubes with density discontinuities).
 * 
 * Root Cause: The original bug was that uniform particle_spacing was used for both
 * lower and upper boundaries, causing incorrect wall positions when particles had
 * different spacing near each boundary.
 * 
 * Fix: Introduced spacing_lower[] and spacing_upper[] arrays in BoundaryConfiguration
 * to allow per-boundary spacing configuration.
 */

#include <gtest/gtest.h>
#include "core/boundaries/boundary_types.hpp"
#include "core/particles/sph_particle.hpp"
#include <cmath>

using namespace sph;

// ============================================================================
// 1D Tests: Shock Tube with Density Discontinuity
// ============================================================================

/**
 * GIVEN: A 1D shock tube with dense particles on left (dx=0.0025) and 
 *        sparse particles on right (dx=0.02)
 * WHEN: Boundary configuration uses per-boundary spacing
 * THEN: Left wall position should use dx_left and right wall should use dx_right
 */
TEST(BoundarySpacingValidation, Given1DShockTubeWithDensityRatio_WhenUsingPerBoundarySpacing_ThenWallPositionsAreCorrect) {
    // GIVEN: Shock tube configuration
    constexpr int Dim = 1;
    constexpr real domain_min = -0.5;
    constexpr real domain_max = 1.5;
    constexpr real dx_left = 0.0025;   // Dense particles
    constexpr real dx_right = 0.02;    // Sparse particles (8× larger)
    
    // WHEN: Configure boundary with per-boundary spacing
    BoundaryConfiguration<Dim> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::MIRROR;
    config.range_min[0] = domain_min;
    config.range_max[0] = domain_max;
    config.enable_lower[0] = true;
    config.enable_upper[0] = true;
    config.spacing_lower[0] = dx_left;   // Left boundary uses dense spacing
    config.spacing_upper[0] = dx_right;  // Right boundary uses sparse spacing
    
    // THEN: Wall positions should be correct
    const real expected_left_wall = domain_min - 0.5 * dx_left;    // -0.50125
    const real expected_right_wall = domain_max + 0.5 * dx_right;  // 1.51
    
    const real actual_left_wall = config.get_wall_position(0, false);
    const real actual_right_wall = config.get_wall_position(0, true);
    
    EXPECT_NEAR(actual_left_wall, expected_left_wall, 1e-10)
        << "Left wall should be offset by 0.5*dx_left from domain boundary";
    EXPECT_NEAR(actual_right_wall, expected_right_wall, 1e-10)
        << "Right wall should be offset by 0.5*dx_right from domain boundary";
}

/**
 * GIVEN: A 1D shock tube configuration
 * WHEN: Old-style uniform particle_spacing is used (backward compatibility)
 * THEN: Both walls should use the same spacing (fallback behavior)
 */
TEST(BoundarySpacingValidation, Given1DConfiguration_WhenUsingLegacyUniformSpacing_ThenFallbackToUniformSpacing) {
    // GIVEN: Configuration with only uniform spacing set
    constexpr int Dim = 1;
    constexpr real dx = 0.01;
    
    BoundaryConfiguration<Dim> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::MIRROR;
    config.range_min[0] = -0.5;
    config.range_max[0] = 1.5;
    config.particle_spacing[0] = dx;  // Legacy uniform spacing
    // spacing_lower and spacing_upper are 0.0 (not set)
    
    // THEN: Both walls use the uniform spacing
    const real expected_left_wall = -0.5 - 0.5 * dx;
    const real expected_right_wall = 1.5 + 0.5 * dx;
    
    EXPECT_NEAR(config.get_wall_position(0, false), expected_left_wall, 1e-10);
    EXPECT_NEAR(config.get_wall_position(0, true), expected_right_wall, 1e-10);
}

/**
 * GIVEN: A 1D shock tube with asymmetric particle distribution
 * WHEN: Per-boundary spacing overrides uniform spacing
 * THEN: Per-boundary spacing takes precedence
 */
TEST(BoundarySpacingValidation, Given1DConfiguration_WhenPerBoundarySpacingOverridesUniform_ThenPerBoundaryTakesPrecedence) {
    // GIVEN: Both uniform and per-boundary spacing set
    constexpr int Dim = 1;
    constexpr real dx_uniform = 0.01;
    constexpr real dx_left = 0.0025;
    constexpr real dx_right = 0.02;
    
    BoundaryConfiguration<Dim> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::MIRROR;
    config.range_min[0] = -0.5;
    config.range_max[0] = 1.5;
    config.particle_spacing[0] = dx_uniform;  // Should be overridden
    config.spacing_lower[0] = dx_left;
    config.spacing_upper[0] = dx_right;
    
    // THEN: Per-boundary spacing is used, not uniform
    const real expected_left_wall = -0.5 - 0.5 * dx_left;   // NOT dx_uniform
    const real expected_right_wall = 1.5 + 0.5 * dx_right;  // NOT dx_uniform
    
    EXPECT_NEAR(config.get_wall_position(0, false), expected_left_wall, 1e-10);
    EXPECT_NEAR(config.get_wall_position(0, true), expected_right_wall, 1e-10);
}

// ============================================================================
// 2D Tests: Shock Tube with Asymmetric X-direction and Uniform Y-direction
// ============================================================================

/**
 * GIVEN: A 2D shock tube with asymmetric X-spacing and uniform Y-spacing
 * WHEN: Per-boundary spacing is configured for both dimensions
 * THEN: X-walls use different spacing, Y-walls use same spacing
 */
TEST(BoundarySpacingValidation, Given2DShockTube_WhenAsymmetricXAndUniformY_ThenWallPositionsAreCorrect) {
    // GIVEN: 2D shock tube configuration
    constexpr int Dim = 2;
    constexpr real x_min = -0.5, x_max = 1.5;
    constexpr real y_min = 0.0,  y_max = 0.5;
    constexpr real dx_left = 0.0025;
    constexpr real dx_right = 0.02;
    constexpr real dy = 0.02;  // Uniform in Y
    
    // WHEN: Configure boundaries
    BoundaryConfiguration<Dim> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::MIRROR;
    config.types[1] = BoundaryType::MIRROR;
    
    config.range_min[0] = x_min;
    config.range_max[0] = x_max;
    config.range_min[1] = y_min;
    config.range_max[1] = y_max;
    
    // X-direction: asymmetric
    config.spacing_lower[0] = dx_left;
    config.spacing_upper[0] = dx_right;
    
    // Y-direction: symmetric
    config.spacing_lower[1] = dy;
    config.spacing_upper[1] = dy;
    
    // THEN: Wall positions match expectations
    EXPECT_NEAR(config.get_wall_position(0, false), x_min - 0.5 * dx_left, 1e-10)
        << "Left X-wall uses dx_left";
    EXPECT_NEAR(config.get_wall_position(0, true), x_max + 0.5 * dx_right, 1e-10)
        << "Right X-wall uses dx_right";
    EXPECT_NEAR(config.get_wall_position(1, false), y_min - 0.5 * dy, 1e-10)
        << "Bottom Y-wall uses dy";
    EXPECT_NEAR(config.get_wall_position(1, true), y_max + 0.5 * dy, 1e-10)
        << "Top Y-wall uses dy";
}

/**
 * GIVEN: A 2D configuration with fully asymmetric spacing in both dimensions
 * WHEN: Each boundary has different particle spacing
 * THEN: Each wall uses its local particle spacing
 */
TEST(BoundarySpacingValidation, Given2DConfiguration_WhenFullyAsymmetric_ThenEachWallUsesLocalSpacing) {
    // GIVEN: Asymmetric in both X and Y
    constexpr int Dim = 2;
    constexpr real dx_left = 0.001, dx_right = 0.01;
    constexpr real dy_bottom = 0.002, dy_top = 0.008;
    
    BoundaryConfiguration<Dim> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::MIRROR;
    config.types[1] = BoundaryType::MIRROR;
    config.range_min = Vector<Dim>{0.0, 0.0};
    config.range_max = Vector<Dim>{1.0, 1.0};
    
    config.spacing_lower[0] = dx_left;
    config.spacing_upper[0] = dx_right;
    config.spacing_lower[1] = dy_bottom;
    config.spacing_upper[1] = dy_top;
    
    // THEN: Each wall uses its specific spacing
    EXPECT_NEAR(config.get_wall_position(0, false), 0.0 - 0.5 * dx_left, 1e-10);
    EXPECT_NEAR(config.get_wall_position(0, true), 1.0 + 0.5 * dx_right, 1e-10);
    EXPECT_NEAR(config.get_wall_position(1, false), 0.0 - 0.5 * dy_bottom, 1e-10);
    EXPECT_NEAR(config.get_wall_position(1, true), 1.0 + 0.5 * dy_top, 1e-10);
}

// ============================================================================
// 3D Tests: Future-proofing for 3D simulations
// ============================================================================

/**
 * GIVEN: A 3D configuration with different spacing in all dimensions
 * WHEN: Per-boundary spacing is configured for X, Y, and Z
 * THEN: All six walls use their respective local spacing
 */
TEST(BoundarySpacingValidation, Given3DConfiguration_WhenAsymmetricAllDimensions_ThenAllWallsUseLocalSpacing) {
    // GIVEN: 3D configuration with varying spacing
    constexpr int Dim = 3;
    constexpr real dx_left = 0.001, dx_right = 0.01;
    constexpr real dy_bottom = 0.002, dy_top = 0.008;
    constexpr real dz_back = 0.003, dz_front = 0.009;
    
    BoundaryConfiguration<Dim> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::MIRROR;
    config.types[1] = BoundaryType::MIRROR;
    config.types[2] = BoundaryType::MIRROR;
    config.range_min = Vector<Dim>{0.0, 0.0, 0.0};
    config.range_max = Vector<Dim>{1.0, 1.0, 1.0};
    
    config.spacing_lower[0] = dx_left;
    config.spacing_upper[0] = dx_right;
    config.spacing_lower[1] = dy_bottom;
    config.spacing_upper[1] = dy_top;
    config.spacing_lower[2] = dz_back;
    config.spacing_upper[2] = dz_front;
    
    // THEN: All six walls use their specific spacing
    // X-walls
    EXPECT_NEAR(config.get_wall_position(0, false), 0.0 - 0.5 * dx_left, 1e-10)
        << "X-lower wall";
    EXPECT_NEAR(config.get_wall_position(0, true), 1.0 + 0.5 * dx_right, 1e-10)
        << "X-upper wall";
    
    // Y-walls
    EXPECT_NEAR(config.get_wall_position(1, false), 0.0 - 0.5 * dy_bottom, 1e-10)
        << "Y-lower wall";
    EXPECT_NEAR(config.get_wall_position(1, true), 1.0 + 0.5 * dy_top, 1e-10)
        << "Y-upper wall";
    
    // Z-walls
    EXPECT_NEAR(config.get_wall_position(2, false), 0.0 - 0.5 * dz_back, 1e-10)
        << "Z-lower wall";
    EXPECT_NEAR(config.get_wall_position(2, true), 1.0 + 0.5 * dz_front, 1e-10)
        << "Z-upper wall";
}

// ============================================================================
// Regression Tests: Ensure the bug doesn't come back
// ============================================================================

/**
 * GIVEN: The original buggy configuration (uniform spacing for asymmetric particles)
 * WHEN: Using the old approach
 * THEN: We can detect the incorrect wall position
 * 
 * This is a NEGATIVE test showing what happens with the bug.
 */
TEST(BoundarySpacingValidation, GivenOriginalBuggyConfig_WhenUsingUniformSpacingForAsymmetricParticles_ThenLeftWallPositionIsWrong) {
    // GIVEN: Original bug - using dx_right for both boundaries
    constexpr int Dim = 1;
    constexpr real dx_right = 0.02;
    constexpr real domain_min = -0.5;
    
    BoundaryConfiguration<Dim> buggy_config;
    buggy_config.is_valid = true;
    buggy_config.types[0] = BoundaryType::MIRROR;
    buggy_config.range_min[0] = domain_min;
    buggy_config.range_max[0] = 1.5;
    buggy_config.particle_spacing[0] = dx_right;  // WRONG for left boundary!
    // spacing_lower and spacing_upper not set (0.0)
    
    // THEN: Left wall position is incorrect
    const real buggy_left_wall = buggy_config.get_wall_position(0, false);
    const real expected_buggy = domain_min - 0.5 * dx_right;  // -0.51 (WRONG!)
    
    EXPECT_NEAR(buggy_left_wall, expected_buggy, 1e-10)
        << "This demonstrates the bug: left wall uses dx_right instead of dx_left";
    
    // The CORRECT position should be:
    constexpr real dx_left = 0.0025;
    const real correct_left_wall = domain_min - 0.5 * dx_left;  // -0.50125
    
    // Show that buggy position differs from correct position
    const real error = std::abs(buggy_left_wall - correct_left_wall);
    EXPECT_GT(error, 0.008)  // Error is about 0.00875
        << "Buggy configuration has significant error in wall position";
}

/**
 * GIVEN: A corrected configuration with per-boundary spacing
 * WHEN: Applied to the same shock tube setup
 * THEN: Wall positions are accurate
 * 
 * This is the POSITIVE test showing the fix works.
 */
TEST(BoundarySpacingValidation, GivenCorrectedConfig_WhenUsingPerBoundarySpacing_ThenLeftWallPositionIsCorrect) {
    // GIVEN: Corrected configuration
    constexpr int Dim = 1;
    constexpr real dx_left = 0.0025;
    constexpr real dx_right = 0.02;
    constexpr real domain_min = -0.5;
    
    BoundaryConfiguration<Dim> correct_config;
    correct_config.is_valid = true;
    correct_config.types[0] = BoundaryType::MIRROR;
    correct_config.range_min[0] = domain_min;
    correct_config.range_max[0] = 1.5;
    correct_config.spacing_lower[0] = dx_left;   // CORRECT!
    correct_config.spacing_upper[0] = dx_right;
    
    // THEN: Left wall position is correct
    const real actual_left_wall = correct_config.get_wall_position(0, false);
    const real expected_correct = domain_min - 0.5 * dx_left;  // -0.50125
    
    EXPECT_NEAR(actual_left_wall, expected_correct, 1e-10)
        << "Corrected configuration produces accurate wall position";
}

// ============================================================================
// Edge Cases and Validation
// ============================================================================

/**
 * GIVEN: Zero particle spacing (invalid but should not crash)
 * WHEN: Getting wall position
 * THEN: Should return boundary position without offset
 */
TEST(BoundarySpacingValidation, GivenZeroSpacing_WhenGettingWallPosition_ThenReturnsRangeBoundary) {
    constexpr int Dim = 1;
    
    BoundaryConfiguration<Dim> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::MIRROR;
    config.range_min[0] = -0.5;
    config.range_max[0] = 1.5;
    config.spacing_lower[0] = 0.0;
    config.spacing_upper[0] = 0.0;
    
    // With zero spacing, wall is at the range boundary
    EXPECT_NEAR(config.get_wall_position(0, false), -0.5, 1e-10);
    EXPECT_NEAR(config.get_wall_position(0, true), 1.5, 1e-10);
}

/**
 * GIVEN: Negative spacing (invalid configuration)
 * WHEN: Getting wall position
 * THEN: Should still compute (even though physically invalid)
 */
TEST(BoundarySpacingValidation, GivenNegativeSpacing_WhenGettingWallPosition_ThenComputesButPhysicallyInvalid) {
    constexpr int Dim = 1;
    constexpr real dx = -0.01;  // Invalid but shouldn't crash
    
    BoundaryConfiguration<Dim> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::MIRROR;
    config.range_min[0] = 0.0;
    config.range_max[0] = 1.0;
    config.spacing_lower[0] = dx;
    config.spacing_upper[0] = dx;
    
    // Should compute but result is physically meaningless
    const real lower_wall = config.get_wall_position(0, false);
    const real upper_wall = config.get_wall_position(0, true);
    
    // Wall positions will be inverted (inside domain)
    EXPECT_NEAR(lower_wall, 0.0 + 0.5 * 0.01, 1e-10);  // 0.005 (inside domain!)
    EXPECT_NEAR(upper_wall, 1.0 - 0.5 * 0.01, 1e-10);  // 0.995 (inside domain!)
}

// ============================================================================
// Corner/Edge Ghost Particle Tests - MIRROR Boundaries
// ============================================================================

/**
 * GIVEN: A 2D MIRROR boundary configuration
 * WHEN: Particles exist near corners (e.g., bottom-left)
 * THEN: Corner regions are naturally handled by overlapping edge ghosts
 * 
 * NOTE: MIRROR boundaries don't need explicit corner ghosts because:
 * 1. Each dimension creates independent mirror ghosts
 * 2. A particle near (x_min, y_min) will have:
 *    - X-mirror ghost at (2*x_wall - x, y)
 *    - Y-mirror ghost at (x, 2*y_wall - y)
 *    - The "corner ghost" at (2*x_wall - x, 2*y_wall - y) would be created
 *      when the X-mirror ghost gets Y-mirrored (or vice versa)
 * 3. Unlike PERIODIC, mirrors don't wrap - they reflect
 */
TEST(BoundarySpacingValidation, Given2DMirrorBoundaries_WhenParticleNearCorner_ThenEdgeGhostsHandleCornerNaturally) {
    // GIVEN: 2D mirror boundaries with asymmetric spacing
    constexpr int Dim = 2;
    constexpr real dx_left = 0.001, dx_right = 0.01;
    constexpr real dy_bottom = 0.002, dy_top = 0.008;
    
    BoundaryConfiguration<Dim> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::MIRROR;
    config.types[1] = BoundaryType::MIRROR;
    config.range_min = Vector<Dim>{0.0, 0.0};
    config.range_max = Vector<Dim>{1.0, 1.0};
    config.enable_lower[0] = true;
    config.enable_upper[0] = true;
    config.enable_lower[1] = true;
    config.enable_upper[1] = true;
    
    config.spacing_lower[0] = dx_left;
    config.spacing_upper[0] = dx_right;
    config.spacing_lower[1] = dy_bottom;
    config.spacing_upper[1] = dy_top;
    
    // THEN: Each wall uses its spacing - corners are implicit
    // Bottom-left corner region uses (dx_left, dy_bottom)
    const real x_left_wall = config.get_wall_position(0, false);
    const real y_bottom_wall = config.get_wall_position(1, false);
    
    EXPECT_NEAR(x_left_wall, 0.0 - 0.5 * dx_left, 1e-10)
        << "Left wall for corner uses dx_left";
    EXPECT_NEAR(y_bottom_wall, 0.0 - 0.5 * dy_bottom, 1e-10)
        << "Bottom wall for corner uses dy_bottom";
    
    // Top-right corner region uses (dx_right, dy_top)
    const real x_right_wall = config.get_wall_position(0, true);
    const real y_top_wall = config.get_wall_position(1, true);
    
    EXPECT_NEAR(x_right_wall, 1.0 + 0.5 * dx_right, 1e-10)
        << "Right wall for corner uses dx_right";
    EXPECT_NEAR(y_top_wall, 1.0 + 0.5 * dy_top, 1e-10)
        << "Top wall for corner uses dy_top";
}

/**
 * GIVEN: A 3D MIRROR boundary configuration with varying spacing
 * WHEN: All 8 corners have different particle density
 * THEN: Each of the 8 corner regions uses appropriate wall spacing
 */
TEST(BoundarySpacingValidation, Given3DMirrorBoundaries_WhenAllCornersHaveDifferentDensity_ThenEachCornerUsesCorrectSpacing) {
    // GIVEN: 3D configuration - each wall has different spacing
    constexpr int Dim = 3;
    constexpr real dx_left = 0.001, dx_right = 0.01;
    constexpr real dy_bottom = 0.002, dy_top = 0.008;
    constexpr real dz_back = 0.003, dz_front = 0.009;
    
    BoundaryConfiguration<Dim> config;
    config.is_valid = true;
    for (int d = 0; d < Dim; ++d) {
        config.types[d] = BoundaryType::MIRROR;
        config.enable_lower[d] = true;
        config.enable_upper[d] = true;
    }
    config.range_min = Vector<Dim>{0.0, 0.0, 0.0};
    config.range_max = Vector<Dim>{1.0, 1.0, 1.0};
    
    config.spacing_lower[0] = dx_left;
    config.spacing_upper[0] = dx_right;
    config.spacing_lower[1] = dy_bottom;
    config.spacing_upper[1] = dy_top;
    config.spacing_lower[2] = dz_back;
    config.spacing_upper[2] = dz_front;
    
    // THEN: Each of 8 corners implicitly defined by 3 wall positions
    // Corner 1: (x_min, y_min, z_min) - uses (dx_left, dy_bottom, dz_back)
    EXPECT_NEAR(config.get_wall_position(0, false), 0.0 - 0.5 * dx_left, 1e-10);
    EXPECT_NEAR(config.get_wall_position(1, false), 0.0 - 0.5 * dy_bottom, 1e-10);
    EXPECT_NEAR(config.get_wall_position(2, false), 0.0 - 0.5 * dz_back, 1e-10);
    
    // Corner 8: (x_max, y_max, z_max) - uses (dx_right, dy_top, dz_front)
    EXPECT_NEAR(config.get_wall_position(0, true), 1.0 + 0.5 * dx_right, 1e-10);
    EXPECT_NEAR(config.get_wall_position(1, true), 1.0 + 0.5 * dy_top, 1e-10);
    EXPECT_NEAR(config.get_wall_position(2, true), 1.0 + 0.5 * dz_front, 1e-10);
    
    // All other 6 corners are combinations of the 6 wall positions
    // The spacing configuration is complete and correct
}

/**
 * GIVEN: A 2D configuration with one MIRROR and one PERIODIC dimension
 * WHEN: Mixed boundary types
 * THEN: MIRROR dimension uses per-boundary spacing, PERIODIC uses uniform
 * 
 * NOTE: This is a realistic scenario - e.g., shock tube (X: mirror) + 
 * periodic in cross-flow direction (Y: periodic)
 */
TEST(BoundarySpacingValidation, Given2DMixedBoundaries_WhenMirrorXAndPeriodicY_ThenEachUsesAppropriateSpacing) {
    // GIVEN: Mixed boundary configuration
    constexpr int Dim = 2;
    constexpr real dx_left = 0.001, dx_right = 0.01;
    constexpr real dy_periodic = 0.005;  // Uniform for periodic
    
    BoundaryConfiguration<Dim> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::MIRROR;   // X: walls
    config.types[1] = BoundaryType::PERIODIC; // Y: wrapping
    config.range_min = Vector<Dim>{0.0, 0.0};
    config.range_max = Vector<Dim>{1.0, 1.0};
    
    config.spacing_lower[0] = dx_left;
    config.spacing_upper[0] = dx_right;
    // For periodic, wall position doesn't apply the same way,
    // but spacing can still be set for consistency
    config.spacing_lower[1] = dy_periodic;
    config.spacing_upper[1] = dy_periodic;
    
    // THEN: Mirror walls use asymmetric spacing
    EXPECT_NEAR(config.get_wall_position(0, false), 0.0 - 0.5 * dx_left, 1e-10);
    EXPECT_NEAR(config.get_wall_position(0, true), 1.0 + 0.5 * dx_right, 1e-10);
    
    // Periodic dimension has symmetric spacing
    EXPECT_NEAR(config.get_wall_position(1, false), 0.0 - 0.5 * dy_periodic, 1e-10);
    EXPECT_NEAR(config.get_wall_position(1, true), 1.0 + 0.5 * dy_periodic, 1e-10);
}


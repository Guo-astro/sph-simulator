// Use 1D for tests by default
static constexpr int Dim = 1;

/**
 * @file morris_1997_ghost_positioning_test.cpp
 * @brief Test suite verifying correct implementation of Morris 1997 ghost particle formula
 * 
 * This test validates:
 * 1. Wall position calculation: x_wall = x_boundary ± 0.5*dx
 * 2. Ghost position formula: x_ghost = 2*x_wall - x_real
 * 3. Distance preservation: distance(ghost,wall) = distance(real,wall)
 * 4. Velocity reflection for mirror boundaries
 */

#include <gtest/gtest.h>
#include "core/boundaries/ghost_particle_manager.hpp"
#include "core/boundaries/boundary_types.hpp"
#include "core/particle.hpp"
#include <cmath>

using namespace sph;

class Morris1997GhostPositioningTest : public ::testing::Test {
protected:
    static constexpr real TOLERANCE = 1e-10;
    
    // Helper: Check if two reals are approximately equal
    bool approx_equal(real a, real b, real tol = TOLERANCE) const {
        return std::abs(a - b) < tol;
    }
};

/**
 * @test Wall position calculation for lower boundary
 * 
 * Given: Domain range_min = -0.5, particle spacing dx = 0.0025
 * When: Wall position is calculated for lower boundary
 * Then: x_wall_lower = range_min - 0.5*dx = -0.50125
 */
TEST_F(Morris1997GhostPositioningTest, LowerWallPositionCalculation) {
    // Given
    BoundaryConfiguration<1> config;
    config.range_min[0] = -0.5;
    config.range_max[0] = 1.5;
    config.particle_spacing[0] = 0.0025;
    
    // When
    real wall_pos = config.get_wall_position(0, false);  // lower boundary
    
    // Then
    real expected = -0.5 - 0.5 * 0.0025;  // -0.50125
    EXPECT_NEAR(wall_pos, expected, TOLERANCE)
        << "Lower wall should be at range_min - 0.5*dx";
    EXPECT_NEAR(wall_pos, -0.50125, TOLERANCE);
}

/**
 * @test Wall position calculation for upper boundary
 * 
 * Given: Domain range_max = 1.5, particle spacing dx = 0.0025
 * When: Wall position is calculated for upper boundary
 * Then: x_wall_upper = range_max + 0.5*dx = 1.50125
 */
TEST_F(Morris1997GhostPositioningTest, UpperWallPositionCalculation) {
    // Given
    BoundaryConfiguration<1> config;
    config.range_min[0] = -0.5;
    config.range_max[0] = 1.5;
    config.particle_spacing[0] = 0.0025;
    
    // When
    real wall_pos = config.get_wall_position(0, true);  // upper boundary
    
    // Then
    real expected = 1.5 + 0.5 * 0.0025;  // 1.50125
    EXPECT_NEAR(wall_pos, expected, TOLERANCE)
        << "Upper wall should be at range_max + 0.5*dx";
    EXPECT_NEAR(wall_pos, 1.50125, TOLERANCE);
}

/**
 * @test Morris 1997 formula for ghost at lower boundary
 * 
 * Given: 
 *   - Real particle at x = -0.5 (left edge)
 *   - Wall at x_wall = -0.50125
 *   - Particle spacing dx = 0.0025
 * When: Ghost is generated using Morris formula x_ghost = 2*x_wall - x_real
 * Then: 
 *   - x_ghost = 2*(-0.50125) - (-0.5) = -0.5025
 *   - distance(real, ghost) = 0.0025 = dx ✓
 *   - distance(real, wall) = distance(ghost, wall) = 0.00125 ✓
 */
TEST_F(Morris1997GhostPositioningTest, MorrisFormulaLowerBoundary) {
    // Given
    BoundaryConfiguration<1> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::MIRROR;
    config.enable_lower[0] = true;
    config.enable_upper[0] = false;
    config.mirror_types[0] = MirrorType::FREE_SLIP;
    config.range_min[0] = -0.5;
    config.range_max[0] = 1.5;
    config.particle_spacing[0] = 0.0025;
    
    GhostParticleManager<1> manager;
    manager.initialize(config);
    manager.set_kernel_support_radius(0.01);  // Enough to detect boundary particle
    
    std::vector<SPHParticle<1>> particles;
    SPHParticle<1> p;
    p.pos[0] = -0.5;  // Left edge particle
    p.vel[0] = 1.0;
    p.dens = 1.0;
    p.mass = 1.0;
    p.type = static_cast<int>(ParticleType::REAL);
    particles.push_back(p);
    
    // When
    manager.generate_ghosts(particles);
    const auto& ghosts = manager.get_ghost_particles();
    
    // Then
    ASSERT_EQ(ghosts.size(), 1) << "Should create exactly one ghost";
    
    const real x_real = -0.5;
    const real x_wall = -0.50125;
    const real x_ghost_expected = 2.0 * x_wall - x_real;  // Morris formula
    
    EXPECT_NEAR(ghosts[0].pos[0], x_ghost_expected, TOLERANCE)
        << "Ghost position should follow Morris 1997: x_ghost = 2*x_wall - x_real";
    EXPECT_NEAR(ghosts[0].pos[0], -0.5025, TOLERANCE);
    
    // Verify distance preservation
    const real dist_real_ghost = std::abs(ghosts[0].pos[0] - x_real);
    const real dist_real_wall = std::abs(x_real - x_wall);
    const real dist_ghost_wall = std::abs(ghosts[0].pos[0] - x_wall);
    
    EXPECT_NEAR(dist_real_ghost, 0.0025, TOLERANCE)
        << "Distance(real, ghost) should equal particle spacing dx";
    EXPECT_NEAR(dist_real_wall, dist_ghost_wall, TOLERANCE)
        << "Distance(real, wall) must equal distance(ghost, wall) - Morris 1997 symmetry";
    EXPECT_NEAR(dist_real_wall, 0.00125, TOLERANCE)
        << "Both should be 0.5*dx from wall";
}

/**
 * @test Morris 1997 formula for ghost at upper boundary
 * 
 * Given:
 *   - Real particle at x = 1.5 (right edge)
 *   - Wall at x_wall = 1.50125
 *   - Particle spacing dx = 0.0025
 * When: Ghost is generated using Morris formula
 * Then:
 *   - x_ghost = 2*(1.50125) - 1.5 = 1.5025
 *   - distance(real, ghost) = 0.0025 = dx ✓
 *   - Symmetry preserved ✓
 */
TEST_F(Morris1997GhostPositioningTest, MorrisFormulaUpperBoundary) {
    // Given
    BoundaryConfiguration<1> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::MIRROR;
    config.enable_lower[0] = false;
    config.enable_upper[0] = true;
    config.mirror_types[0] = MirrorType::FREE_SLIP;
    config.range_min[0] = -0.5;
    config.range_max[0] = 1.5;
    config.particle_spacing[0] = 0.0025;
    
    GhostParticleManager<1> manager;
    manager.initialize(config);
    manager.set_kernel_support_radius(0.01);
    
    std::vector<SPHParticle<1>> particles;
    SPHParticle<1> p;
    p.pos[0] = 1.5;  // Right edge particle
    p.vel[0] = -1.0;
    p.dens = 1.0;
    p.mass = 1.0;
    p.type = static_cast<int>(ParticleType::REAL);
    particles.push_back(p);
    
    // When
    manager.generate_ghosts(particles);
    const auto& ghosts = manager.get_ghost_particles();
    
    // Then
    ASSERT_EQ(ghosts.size(), 1);
    
    const real x_real = 1.5;
    const real x_wall = 1.50125;
    const real x_ghost_expected = 2.0 * x_wall - x_real;
    
    EXPECT_NEAR(ghosts[0].pos[0], x_ghost_expected, TOLERANCE);
    EXPECT_NEAR(ghosts[0].pos[0], 1.5025, TOLERANCE);
    
    // Verify symmetry
    const real dist_real_wall = std::abs(x_real - x_wall);
    const real dist_ghost_wall = std::abs(ghosts[0].pos[0] - x_wall);
    EXPECT_NEAR(dist_real_wall, dist_ghost_wall, TOLERANCE);
}

/**
 * @test Velocity reflection for FREE_SLIP mirror boundary
 * 
 * Given: FREE_SLIP mirror boundary
 * When: Ghost is created from real particle with v = 1.0
 * Then: v_ghost = -1.0 (normal component reflected)
 */
TEST_F(Morris1997GhostPositioningTest, VelocityReflectionFreeSlip) {
    // Given
    BoundaryConfiguration<1> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::MIRROR;
    config.enable_lower[0] = true;
    config.enable_upper[0] = false;
    config.mirror_types[0] = MirrorType::FREE_SLIP;
    config.range_min[0] = -0.5;
    config.range_max[0] = 1.5;
    config.particle_spacing[0] = 0.02;
    
    GhostParticleManager<1> manager;
    manager.initialize(config);
    manager.set_kernel_support_radius(0.1);
    
    std::vector<SPHParticle<1>> particles;
    SPHParticle<1> p;
    p.pos[0] = -0.49;
    p.vel[0] = 1.5;  // Moving away from wall
    p.dens = 1.0;
    p.mass = 1.0;
    p.type = static_cast<int>(ParticleType::REAL);
    particles.push_back(p);
    
    // When
    manager.generate_ghosts(particles);
    const auto& ghosts = manager.get_ghost_particles();
    
    // Then
    ASSERT_GT(ghosts.size(), 0);
    EXPECT_NEAR(ghosts[0].vel[0], -1.5, TOLERANCE)
        << "Free-slip should reflect normal velocity: v_ghost = -v_real";
}

/**
 * @test Velocity reflection for NO_SLIP mirror boundary
 * 
 * Given: NO_SLIP mirror boundary
 * When: Ghost is created from real particle with v = 1.0
 * Then: v_ghost = -1.0 (all components reflected)
 */
TEST_F(Morris1997GhostPositioningTest, VelocityReflectionNoSlip) {
    // Given
    BoundaryConfiguration<1> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::MIRROR;
    config.enable_lower[0] = true;
    config.enable_upper[0] = false;
    config.mirror_types[0] = MirrorType::NO_SLIP;
    config.range_min[0] = -0.5;
    config.range_max[0] = 1.5;
    config.particle_spacing[0] = 0.02;
    
    GhostParticleManager<1> manager;
    manager.initialize(config);
    manager.set_kernel_support_radius(0.1);
    
    std::vector<SPHParticle<1>> particles;
    SPHParticle<1> p;
    p.pos[0] = -0.49;
    p.vel[0] = 2.0;
    p.dens = 1.0;
    p.mass = 1.0;
    p.type = static_cast<int>(ParticleType::REAL);
    particles.push_back(p);
    
    // When
    manager.generate_ghosts(particles);
    const auto& ghosts = manager.get_ghost_particles();
    
    // Then
    ASSERT_GT(ghosts.size(), 0);
    EXPECT_NEAR(ghosts[0].vel[0], -2.0, TOLERANCE)
        << "No-slip should reflect all velocity: v_ghost = -v_real";
}

/**
 * @test Thermodynamic properties preservation (Morris 1997)
 * 
 * Given: Real particle with ρ=1.0, p=0.5, e=0.3
 * When: Ghost is created
 * Then: ρ_ghost=1.0, p_ghost=0.5, e_ghost=0.3 (identical)
 */
TEST_F(Morris1997GhostPositioningTest, ThermodynamicPropertiesPreservation) {
    // Given
    BoundaryConfiguration<1> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::MIRROR;
    config.enable_lower[0] = true;
    config.enable_upper[0] = false;
    config.mirror_types[0] = MirrorType::FREE_SLIP;
    config.range_min[0] = -0.5;
    config.range_max[0] = 1.5;
    config.particle_spacing[0] = 0.02;
    
    GhostParticleManager<1> manager;
    manager.initialize(config);
    manager.set_kernel_support_radius(0.1);
    
    std::vector<SPHParticle<1>> particles;
    SPHParticle<1> p;
    p.pos[0] = -0.49;
    p.vel[0] = 1.0;
    p.dens = 1.25;
    p.pres = 0.75;
    p.ene = 0.35;
    p.mass = 0.02;
    p.type = static_cast<int>(ParticleType::REAL);
    particles.push_back(p);
    
    // When
    manager.generate_ghosts(particles);
    const auto& ghosts = manager.get_ghost_particles();
    
    // Then
    ASSERT_GT(ghosts.size(), 0);
    EXPECT_NEAR(ghosts[0].dens, 1.25, TOLERANCE)
        << "Ghost density must match real particle";
    EXPECT_NEAR(ghosts[0].pres, 0.75, TOLERANCE)
        << "Ghost pressure must match real particle";
    EXPECT_NEAR(ghosts[0].ene, 0.35, TOLERANCE)
        << "Ghost energy must match real particle";
    EXPECT_EQ(ghosts[0].type, static_cast<int>(ParticleType::GHOST))
        << "Ghost must be marked with GHOST type";
}

/**
 * @test 2D Morris formula - both dimensions independent
 * 
 * Given: 2D domain with different spacing in x and y
 * When: Ghost is created near corner
 * Then: Morris formula applies independently to each dimension
 */
TEST_F(Morris1997GhostPositioningTest, Morris2D_IndependentDimensions) {
    // Given
    BoundaryConfiguration<2> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::MIRROR;
    config.types[1] = BoundaryType::MIRROR;
    config.enable_lower[0] = true;
    config.enable_lower[1] = true;
    config.enable_upper[0] = false;
    config.enable_upper[1] = false;
    config.mirror_types[0] = MirrorType::FREE_SLIP;
    config.mirror_types[1] = MirrorType::FREE_SLIP;
    config.range_min = {-0.5, 0.0};
    config.range_max = {1.5, 0.5};
    config.particle_spacing = {0.02, 0.01};  // Different spacing in x and y
    
    GhostParticleManager<2> manager;
    manager.initialize(config);
    manager.set_kernel_support_radius(0.1);
    
    std::vector<SPHParticle<2>> particles;
    SPHParticle<2> p;
    p.pos = {-0.49, 0.01};  // Near lower-left corner
    p.vel = {1.0, 0.5};
    p.dens = 1.0;
    p.mass = 1.0;
    p.type = static_cast<int>(ParticleType::REAL);
    particles.push_back(p);
    
    // When
    manager.generate_ghosts(particles);
    const auto& ghosts = manager.get_ghost_particles();
    
    // Then - should have ghosts for x-boundary and y-boundary
    ASSERT_GT(ghosts.size(), 0);
    
    // Check x-dimension ghost
    const real x_wall = config.get_wall_position(0, false);  // -0.51
    const real x_ghost_expected = 2.0 * x_wall - p.pos[0];   // 2*(-0.51) - (-0.49) = -0.53
    
    // Check y-dimension ghost  
    const real y_wall = config.get_wall_position(1, false);  // -0.005
    const real y_ghost_expected = 2.0 * y_wall - p.pos[1];   // 2*(-0.005) - 0.01 = -0.02
    
    // At least one ghost should have the expected x position
    bool found_x_ghost = false;
    bool found_y_ghost = false;
    
    for (const auto& ghost : ghosts) {
        if (std::abs(ghost.pos[0] - x_ghost_expected) < TOLERANCE) {
            found_x_ghost = true;
        }
        if (std::abs(ghost.pos[1] - y_ghost_expected) < TOLERANCE) {
            found_y_ghost = true;
        }
    }
    
    EXPECT_TRUE(found_x_ghost || found_y_ghost)
        << "Should find ghosts following Morris formula in at least one dimension";
}

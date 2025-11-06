// Use 1D for tests by default
static constexpr int Dim = 1;

/**
 * @file ghost_particle_regression_test.cpp
 * @brief Regression tests to prevent the specific bugs reported on 2025-11-04
 * 
 * These tests ensure:
 * 1. Particles at boundaries get proper ghost support (no density under/over-estimation)
 * 2. Periodic ghost particles maintain velocity direction (no "running away")
 */

#include <gtest/gtest.h>
#include "core/boundaries/ghost_particle_manager.hpp"
#include "core/particles/sph_particle.hpp"
#include "core/boundaries/boundary_types.hpp"
#include "defines.hpp"

using namespace sph;

/**
 * Regression Test: Shock Tube Density Calculation at Boundaries
 * 
 * Scenario: User reported density underestimate at x=-0.5 and overestimate at x=1.5
 * Given: Shock tube domain [-0.5, 1.5] with N=100 particles
 * And: Typical smoothing length h = domain_length/N = 2.0/100 = 0.02
 * And: Kernel support radius = 2h = 0.04
 * When: Ghost particles are generated for boundary particles
 * Then: Particles at EXACTLY x=-0.5 and x=1.5 should get ghost support
 * And: Particles within 0.04 of boundaries should get ghost support
 */
TEST(GhostParticleRegressionTest, ShockTubeBoundaryDensitySupport) {
    // Given: Exact shock tube configuration from user's simulation
    BoundaryConfiguration<1> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::PERIODIC;
    config.range_min = Vector<1>{-0.5};
    config.range_max = Vector<1>{1.5};
    
    GhostParticleManager<1> manager;
    manager.initialize(config);
    
    // Typical values for 100 particles
    const real domain_length = 2.0;
    const int N = 100;
    const real h = domain_length / N;  // 0.02
    const real kernel_support = 2.0 * h;  // 0.04
    manager.set_kernel_support_radius(kernel_support);
    
    // Critical test particles at problematic positions
    std::vector<SPHParticle<1>> particles;
    
    // Particle exactly at left boundary (x = -0.5)
    SPHParticle<1> p_left_boundary;
    p_left_boundary.pos = Vector<1>{-0.5};
    p_left_boundary.vel = Vector<1>{1.0};
    p_left_boundary.dens = 1.0;
    p_left_boundary.mass = 1.0;
    p_left_boundary.sml = h;
    p_left_boundary.type = static_cast<int>(ParticleType::REAL);
    particles.push_back(p_left_boundary);
    
    // Particle exactly at right boundary (x = 1.5)
    SPHParticle<1> p_right_boundary;
    p_right_boundary.pos = Vector<1>{1.5};
    p_right_boundary.vel = Vector<1>{-1.0};
    p_right_boundary.dens = 1.0;
    p_right_boundary.mass = 1.0;
    p_right_boundary.sml = h;
    p_right_boundary.type = static_cast<int>(ParticleType::REAL);
    particles.push_back(p_right_boundary);
    
    // Particle exactly at kernel support distance from left boundary
    SPHParticle<1> p_left_kernel_edge;
    p_left_kernel_edge.pos = Vector<1>{-0.5 + kernel_support};  // -0.46
    p_left_kernel_edge.vel = Vector<1>{1.0};
    p_left_kernel_edge.dens = 1.0;
    p_left_kernel_edge.mass = 1.0;
    p_left_kernel_edge.sml = h;
    p_left_kernel_edge.type = static_cast<int>(ParticleType::REAL);
    particles.push_back(p_left_kernel_edge);
    
    // Particle exactly at kernel support distance from right boundary
    SPHParticle<1> p_right_kernel_edge;
    p_right_kernel_edge.pos = Vector<1>{1.5 - kernel_support};  // 1.46
    p_right_kernel_edge.vel = Vector<1>{-1.0};
    p_right_kernel_edge.dens = 1.0;
    p_right_kernel_edge.mass = 1.0;
    p_right_kernel_edge.sml = h;
    p_right_kernel_edge.type = static_cast<int>(ParticleType::REAL);
    particles.push_back(p_right_kernel_edge);
    
    // When: Generate ghosts
    manager.generate_ghosts(particles);
    const auto& ghosts = manager.get_ghost_particles();
    
    // Then: All 4 particles should generate ghosts
    EXPECT_GE(ghosts.size(), 4) 
        << "All boundary particles should generate ghosts to prevent density errors";
    
    // Verify each critical particle has a ghost
    bool has_left_boundary_ghost = false;
    bool has_right_boundary_ghost = false;
    bool has_left_edge_ghost = false;
    bool has_right_edge_ghost = false;
    
    for (const auto& ghost : ghosts) {
        // Ghost from left boundary appears at right (x ≈ 1.5)
        if (std::abs(ghost.pos[0] - 1.5) < 1e-6) {
            has_left_boundary_ghost = true;
        }
        // Ghost from right boundary appears at left (x ≈ -0.5)
        if (std::abs(ghost.pos[0] - (-0.5)) < 1e-6) {
            has_right_boundary_ghost = true;
        }
        // Ghost from left edge particle (x=-0.46) appears at right (x ≈ 1.54)
        if (std::abs(ghost.pos[0] - 1.54) < 1e-6) {
            has_left_edge_ghost = true;
        }
        // Ghost from right edge particle (x=1.46) appears at left (x ≈ -0.54)
        if (std::abs(ghost.pos[0] - (-0.54)) < 1e-6) {
            has_right_edge_ghost = true;
        }
    }
    
    EXPECT_TRUE(has_left_boundary_ghost) 
        << "Particle at x=-0.5 must have ghost to prevent density underestimation";
    EXPECT_TRUE(has_right_boundary_ghost)
        << "Particle at x=1.5 must have ghost to prevent density overestimation";
    EXPECT_TRUE(has_left_edge_ghost)
        << "Particle at kernel edge x=-0.46 must have ghost (floating point edge case)";
    EXPECT_TRUE(has_right_edge_ghost)
        << "Particle at kernel edge x=1.46 must have ghost (floating point edge case)";
}

/**
 * Regression Test: Periodic Ghost Velocity Direction
 * 
 * Scenario: User reported ghosts having opposite velocity sign, causing them to "run away"
 * Given: Periodic boundaries
 * And: Particles with various velocity magnitudes and directions
 * When: Ghosts are generated
 * Then: ALL ghost velocities must exactly match their source particles
 * And: NO velocity component should be negated (no reflection)
 */
TEST(GhostParticleRegressionTest, PeriodicGhostVelocityPreservation) {
    // Given: Periodic domain
    BoundaryConfiguration<1> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::PERIODIC;
    config.range_min = Vector<1>{-0.5};
    config.range_max = Vector<1>{1.5};
    
    GhostParticleManager<1> manager;
    manager.initialize(config);
    manager.set_kernel_support_radius(0.04);
    
    // And: Particles with various velocities
    std::vector<SPHParticle<1>> particles;
    std::vector<real> test_velocities = {1.0, -1.0, 5.5, -3.2, 0.1, -0.1};
    
    for (size_t i = 0; i < test_velocities.size(); ++i) {
        SPHParticle<1> p;
        // Position near boundary
        p.pos = Vector<1>{-0.49 + i * 0.001};
        p.vel = Vector<1>{test_velocities[i]};
        p.dens = 1.0;
        p.mass = 1.0;
        p.type = static_cast<int>(ParticleType::REAL);
        particles.push_back(p);
    }
    
    // When: Generate ghosts
    manager.generate_ghosts(particles);
    const auto& ghosts = manager.get_ghost_particles();
    
    // Then: All ghosts must have same velocity as source
    EXPECT_EQ(ghosts.size(), particles.size()) 
        << "Should create one ghost per boundary particle";
    
    for (size_t i = 0; i < ghosts.size() && i < particles.size(); ++i) {
        const real source_vel = particles[i].vel[0];
        const real ghost_vel = ghosts[i].vel[0];
        
        // Velocity must be EXACTLY equal (not negated, not modified)
        EXPECT_DOUBLE_EQ(ghost_vel, source_vel)
            << "Periodic ghost velocity must match source exactly for particle " << i;
        
        // Sign must be preserved
        if (source_vel > 0) {
            EXPECT_GT(ghost_vel, 0.0)
                << "Positive velocity must remain positive, not flip to negative";
        } else if (source_vel < 0) {
            EXPECT_LT(ghost_vel, 0.0)
                << "Negative velocity must remain negative, not flip to positive";
        }
        
        // Magnitude must be preserved
        EXPECT_DOUBLE_EQ(std::abs(ghost_vel), std::abs(source_vel))
            << "Velocity magnitude must be preserved";
    }
}

/**
 * Regression Test: Floating Point Edge Cases
 * 
 * Scenario: The original bug was due to strict comparison without epsilon tolerance
 * Given: Particles at positions that result in distances exactly equal to kernel support
 * When: Distances are computed with floating point arithmetic
 * Then: Particles should still generate ghosts despite rounding errors
 */
TEST(GhostParticleRegressionTest, FloatingPointEdgeCases) {
    BoundaryConfiguration<1> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::PERIODIC;
    config.range_min = Vector<1>{-0.5};
    config.range_max = Vector<1>{1.5};
    
    GhostParticleManager<1> manager;
    manager.initialize(config);
    
    // Set kernel support that commonly causes floating point issues
    const real kernel_support = 0.04;  // 2.0/100 * 2 = 0.04
    manager.set_kernel_support_radius(kernel_support);
    
    // Test multiple positions that should be exactly at kernel support distance
    // These are prone to floating point precision issues
    std::vector<real> edge_positions = {
        -0.5 + 0.04,        // Exactly kernel_support from left boundary
        1.5 - 0.04,         // Exactly kernel_support from right boundary
        -0.5 + 2.0/100*2,   // Same as above, but computed differently
        1.5 - 2.0/100*2     // Same as above, but computed differently
    };
    
    for (real pos_x : edge_positions) {
        std::vector<SPHParticle<1>> particles;
        SPHParticle<1> p;
        p.pos = Vector<1>{pos_x};
        p.vel = Vector<1>{0.0};
        p.dens = 1.0;
        p.mass = 1.0;
        p.type = static_cast<int>(ParticleType::REAL);
        particles.push_back(p);
        
        manager.generate_ghosts(particles);
        const auto& ghosts = manager.get_ghost_particles();
        
        EXPECT_GT(ghosts.size(), 0)
            << "Particle at x=" << pos_x << " (distance=kernel_support) must generate ghost "
            << "even with floating point rounding errors";
        
        manager.clear();  // Clear for next test
    }
}
